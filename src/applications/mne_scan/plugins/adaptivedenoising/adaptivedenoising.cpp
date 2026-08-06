//=============================================================================================================
/**
 * @file     adaptivedenoising.cpp
 * @brief    Adaptive Denoising mne_scan algorithm plugin implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoising.h"

#include "adaptivedenoisingblockqueue.h"
#include "adaptivedenoisingprocessor.h"

#include <fiff/fiff_info.h>

#include <scMeas/realtimemultisamplearray.h>

#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWidget>

#include <Eigen/Core>

#include <atomic>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ADAPTIVEDENOISINGPLUGIN;
using namespace FIFFLIB;
using namespace RTPROCESSINGLIB;
using namespace SCMEASLIB;
using namespace SCSHAREDLIB;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace
{

constexpr Eigen::Index kMaximumChannelCount = 512;
constexpr Eigen::Index kMaximumBlockSamples = 2048;
constexpr std::size_t kQueueCapacity = 4;
constexpr int kWorkerWaitTimeoutMilliseconds = 50;
constexpr int kLifecycleWaitTimeoutMilliseconds = 3000;

// One atomic publishes the admission epoch and its in-flight count. The epoch prevents an entrant that observed
// an earlier run from succeeding after a close/reopen cycle.
constexpr std::uint32_t kProducerCountMask = 0x0000FFFFu;
constexpr std::uint32_t kProducerEpochMask = 0x7FFF0000u;
constexpr std::uint32_t kProducerEpochStep = 0x00010000u;
constexpr std::uint32_t kProducerClosedBit = 0x80000000u;

enum class ProducerAdmission : std::uint8_t {
    Entered,
    Busy,
    Closed
};

template<typename UnsignedInteger>
constexpr bool mapsToAlwaysLockFreeStandardAtomic() noexcept
{
    return (std::is_same<UnsignedInteger, unsigned char>::value && ATOMIC_CHAR_LOCK_FREE == 2)
        || (std::is_same<UnsignedInteger, unsigned short>::value && ATOMIC_SHORT_LOCK_FREE == 2)
        || (std::is_same<UnsignedInteger, unsigned int>::value && ATOMIC_INT_LOCK_FREE == 2)
        || (std::is_same<UnsignedInteger, unsigned long>::value && ATOMIC_LONG_LOCK_FREE == 2)
        || (std::is_same<UnsignedInteger, unsigned long long>::value && ATOMIC_LLONG_LOCK_FREE == 2);
}

static_assert(sizeof(std::uint32_t) * CHAR_BIT == 32,
              "Adaptive Denoising producer admission requires an exact 32-bit unsigned type.");
static_assert(sizeof(std::uint64_t) * CHAR_BIT == 64,
              "Adaptive Denoising drop accounting requires an exact 64-bit unsigned type.");
static_assert(mapsToAlwaysLockFreeStandardAtomic<std::uint32_t>(),
              "Adaptive Denoising producer admission requires always-lock-free uint32 atomics.");
static_assert(mapsToAlwaysLockFreeStandardAtomic<std::uint64_t>(),
              "Adaptive Denoising drop accounting requires always-lock-free uint64 atomics.");

} // NAMESPACE

//=============================================================================================================
// DEFINE PRIVATE IMPLEMENTATION
//=============================================================================================================

class AdaptiveDenoising::Impl
{
public:
    class ProducerGuard final
    {
    public:
        explicit ProducerGuard(Impl& impl) noexcept
        : m_impl(impl)
        , m_admission(m_impl.tryEnterProducer())
        {
        }

        ~ProducerGuard()
        {
            if(m_admission == ProducerAdmission::Entered) {
                m_impl.leaveProducer();
            }
        }

        ProducerGuard(const ProducerGuard&) = delete;
        ProducerGuard& operator=(const ProducerGuard&) = delete;

        ProducerAdmission admission() const noexcept
        {
            return m_admission;
        }

    private:
        Impl&             m_impl;
        ProducerAdmission m_admission;
    };

    ProducerAdmission tryEnterProducer() noexcept
    {
        std::uint32_t observed = producerState.load(std::memory_order_acquire);
        if((observed & kProducerClosedBit) != 0u) {
            return ProducerAdmission::Closed;
        }
        if((observed & kProducerCountMask) != 0u) {
            return ProducerAdmission::Busy;
        }

        const std::uint32_t requestedEpoch = observed & kProducerEpochMask;
        const std::uint32_t entered = observed + 1u;
        if(!producerState.compare_exchange_strong(observed,
                                                  entered,
                                                  std::memory_order_acq_rel,
                                                  std::memory_order_acquire)) {
            if((observed & kProducerClosedBit) != 0u
               || (observed & kProducerEpochMask) != requestedEpoch) {
                return ProducerAdmission::Closed;
            }
            return ProducerAdmission::Busy;
        }

        const std::uint32_t confirmed = producerState.load(std::memory_order_acquire);
        if((confirmed & (kProducerClosedBit | kProducerEpochMask))
           != (entered & kProducerEpochMask)) {
            // This provisional Entered owns the successful zero-to-one transition until it reports Closed.
            leaveProducer();
            return ProducerAdmission::Closed;
        }

        return ProducerAdmission::Entered;
    }

    void leaveProducer() noexcept
    {
        producerState.fetch_sub(1u, std::memory_order_release);
    }

    void closeProducerGate() noexcept
    {
        producerState.fetch_or(kProducerClosedBit, std::memory_order_acq_rel);
    }

    void openProducerGate() noexcept
    {
        const std::uint32_t state = producerState.load(std::memory_order_relaxed);
        const std::uint32_t nextEpoch =
            ((state & kProducerEpochMask) + kProducerEpochStep) & kProducerEpochMask;
        producerState.store(nextEpoch, std::memory_order_release);
    }

    bool producerGateIsClosedAndQuiescent() const noexcept
    {
        const std::uint32_t state = producerState.load(std::memory_order_acquire);
        return (state & kProducerClosedBit) != 0u
            && (state & kProducerCountMask) == 0u;
    }

    bool waitForProducerQuiescence(int timeoutMilliseconds) const
    {
        QElapsedTimer timer;
        timer.start();

        while((producerState.load(std::memory_order_acquire) & kProducerCountMask) != 0u) {
            if(timeoutMilliseconds >= 0 && timer.elapsed() >= timeoutMilliseconds) {
                return false;
            }
            QThread::msleep(1);
        }

        return true;
    }

    void disarmProcessor()
    {
        const AdaptiveDenoisingStreamDescriptor invalidStream{0.0, {}};
        processor.configure(invalidStream, 0, settings);
    }

    void resetWorkerState()
    {
        disarmProcessor();
        queuedBlock.rowCount = 0;
        queuedBlock.sampleCount = 0;
        queuedBlock.fiffInfo.clear();
        workerBlock.resize(0, 0);
        workerFiffInfo.clear();
        workerSamplingFrequencyHz = 0.0;
        workerRowCount = 0;
        workerSampleCount = 0;
        workerMetadataValid = false;
        workerHasState = false;
    }

    bool currentMetadataIsValid() const noexcept
    {
        const QSharedPointer<const FiffInfo>& info = queuedBlock.fiffInfo;
        return info
            && info->nchan > 0
            && info->chs.size() == info->nchan
            && static_cast<Eigen::Index>(info->nchan) == queuedBlock.rowCount
            && std::isfinite(static_cast<double>(info->sfreq))
            && info->sfreq > 0.0f;
    }

    bool currentBlockNeedsConfiguration(bool metadataValid) const noexcept
    {
        const QSharedPointer<const FiffInfo>& info = queuedBlock.fiffInfo;
        const double samplingFrequencyHz = info ? static_cast<double>(info->sfreq) : 0.0;

        return !workerHasState
            || workerFiffInfo.data() != info.data()
            || workerRowCount != queuedBlock.rowCount
            || workerSampleCount != queuedBlock.sampleCount
            || workerMetadataValid != metadataValid
            || (metadataValid && workerSamplingFrequencyHz != samplingFrequencyHz);
    }

    void rememberCurrentBlockState(bool metadataValid)
    {
        workerFiffInfo = queuedBlock.fiffInfo;
        workerSamplingFrequencyHz = workerFiffInfo
            ? static_cast<double>(workerFiffInfo->sfreq)
            : 0.0;
        workerRowCount = queuedBlock.rowCount;
        workerSampleCount = queuedBlock.sampleCount;
        workerMetadataValid = metadataValid;
        workerHasState = true;
    }

    void configureCurrentBlock(bool metadataValid)
    {
        if(!metadataValid) {
            disarmProcessor();
            rememberCurrentBlockState(false);
            return;
        }

        try {
            AdaptiveDenoisingStreamDescriptor stream;
            stream.samplingFrequencyHz = static_cast<double>(queuedBlock.fiffInfo->sfreq);
            stream.channels.reserve(static_cast<std::size_t>(queuedBlock.rowCount));

            for(Eigen::Index row = 0; row < queuedBlock.rowCount; ++row) {
                const FiffChInfo& channel = queuedBlock.fiffInfo->chs.at(static_cast<int>(row));
                stream.channels.push_back(AdaptiveDenoisingChannelDescriptor{
                    static_cast<int>(channel.kind),
                    queuedBlock.fiffInfo->bads.contains(channel.ch_name)});
            }

            processor.configure(stream, queuedBlock.sampleCount, settings);
        } catch(...) {
            // configure() is transactional, so explicitly disarm its preserved old ownership before forwarding
            // a block belonging to the new metadata/layout.
            disarmProcessor();
        }

        // RealTimeMultiSampleArray's legacy initializer is not const-correct. This is the sole ownership bridge;
        // neither the adapter nor the output path mutates the shared FIFF metadata.
        QSharedPointer<FiffInfo> outputInfo = queuedBlock.fiffInfo.constCast<FiffInfo>();
        output->measurementData()->initFromFiffInfo(outputInfo);
        output->measurementData()->setMultiArraySize(1);

        rememberCurrentBlockState(true);
    }

    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingQueuedBlock queuedBlock;
    Eigen::MatrixXd workerBlock;
    AdaptiveDenoisingProcessor processor;
    const AdaptiveDenoisingSettings settings;

    QSharedPointer<const FiffInfo> workerFiffInfo;
    double workerSamplingFrequencyHz = 0.0;
    Eigen::Index workerRowCount = 0;
    Eigen::Index workerSampleCount = 0;
    bool workerMetadataValid = false;
    bool workerHasState = false;

    PluginInputData<RealTimeMultiSampleArray>::SPtr input;
    PluginOutputData<RealTimeMultiSampleArray>::SPtr output;

    QMutex lifecycleMutex;
    bool lifecycleQuiesced = true;
    std::atomic<std::uint32_t> producerState{kProducerClosedBit};
    std::atomic<std::uint64_t> droppedBlocks{0u};
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AdaptiveDenoising::AdaptiveDenoising()
: m_impl(new Impl)
{
}

//=============================================================================================================

AdaptiveDenoising::~AdaptiveDenoising()
{
    if(!stop()) {
        QMutexLocker locker(&m_impl->lifecycleMutex);
        m_impl->closeProducerGate();
        m_impl->queue.stop();
        requestInterruption();

        if(isRunning()) {
            wait();
        }

        m_impl->waitForProducerQuiescence(-1);
        if(m_impl->output) {
            m_impl->output->measurementData()->clear();
        }
        m_impl->lifecycleQuiesced = true;
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> AdaptiveDenoising::clone() const
{
    return QSharedPointer<AdaptiveDenoising>(new AdaptiveDenoising);
}

//=============================================================================================================

void AdaptiveDenoising::init()
{
    m_impl->input = PluginInputData<RealTimeMultiSampleArray>::create(
        this,
        "AdaptiveDenoisingIn",
        "Adaptive Denoising input data");
    connect(m_impl->input.data(),
            &PluginInputConnector::notify,
            this,
            &AdaptiveDenoising::update,
            Qt::DirectConnection);
    m_inputConnectors.append(m_impl->input);

    m_impl->output = PluginOutputData<RealTimeMultiSampleArray>::create(
        this,
        "AdaptiveDenoisingOut",
        "Adaptive Denoising output data");
    m_impl->output->measurementData()->setName(getName());
    m_outputConnectors.append(m_impl->output);
}

//=============================================================================================================

void AdaptiveDenoising::unload()
{
}

//=============================================================================================================

bool AdaptiveDenoising::start()
{
    QMutexLocker locker(&m_impl->lifecycleMutex);

    if(isRunning()
       || !m_impl->lifecycleQuiesced
       || !m_impl->producerGateIsClosedAndQuiescent()
       || !m_impl->input
       || !m_impl->output) {
        return false;
    }

    try {
        const AdaptiveDenoisingQueueConfigureStatus status = m_impl->queue.configure(
            AdaptiveDenoisingBlockQueueConfig{
                kMaximumChannelCount,
                kMaximumBlockSamples,
                kQueueCapacity});
        if(status != AdaptiveDenoisingQueueConfigureStatus::Ready) {
            return false;
        }

        m_impl->queuedBlock.data.resize(kMaximumChannelCount, kMaximumBlockSamples);
    } catch(...) {
        m_impl->queue.stop();
        return false;
    }

    m_impl->resetWorkerState();
    m_impl->droppedBlocks.store(0u, std::memory_order_relaxed);
    m_impl->lifecycleQuiesced = false;

    QThread::start();
    m_impl->openProducerGate();
    return true;
}

//=============================================================================================================

bool AdaptiveDenoising::stop()
{
    QMutexLocker locker(&m_impl->lifecycleMutex);

    m_impl->closeProducerGate();
    m_impl->queue.stop();
    requestInterruption();

    if(isRunning() && !wait(kLifecycleWaitTimeoutMilliseconds)) {
        return false;
    }

    if(!m_impl->waitForProducerQuiescence(kLifecycleWaitTimeoutMilliseconds)) {
        return false;
    }

    if(m_impl->output) {
        m_impl->output->measurementData()->clear();
    }
    m_impl->lifecycleQuiesced = true;
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType AdaptiveDenoising::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString AdaptiveDenoising::getName() const
{
    return "Adaptive Denoising";
}

//=============================================================================================================

QWidget* AdaptiveDenoising::setupWidget()
{
    return new QWidget;
}

//=============================================================================================================

QString AdaptiveDenoising::getBuildInfo()
{
    return QString(ADAPTIVEDENOISINGPLUGIN::buildDateTime())
        + QString(" - ")
        + QString(ADAPTIVEDENOISINGPLUGIN::buildHash());
}

//=============================================================================================================

void AdaptiveDenoising::update(Measurement::SPtr pMeasurement)
{
    Impl::ProducerGuard producerGuard(*m_impl);
    const ProducerAdmission admission = producerGuard.admission();
    if(admission == ProducerAdmission::Closed) {
        return;
    }

    QSharedPointer<RealTimeMultiSampleArray> input =
        pMeasurement.dynamicCast<RealTimeMultiSampleArray>();
    if(!input) {
        return;
    }

    const QList<Eigen::MatrixXd>& matrices = input->getMultiSampleArray();
    if(admission == ProducerAdmission::Busy) {
        m_impl->droppedBlocks.fetch_add(static_cast<std::uint64_t>(matrices.size()),
                                        std::memory_order_relaxed);
        return;
    }

    const QSharedPointer<const FiffInfo> info = input->info();
    for(const Eigen::MatrixXd& matrix : matrices) {
        const AdaptiveDenoisingQueuePushStatus status = m_impl->queue.tryPush(matrix, info);
        if(status != AdaptiveDenoisingQueuePushStatus::Pushed) {
            m_impl->droppedBlocks.fetch_add(1u, std::memory_order_relaxed);
        }
    }
}

//=============================================================================================================

void AdaptiveDenoising::run()
{
    while(!isInterruptionRequested()) {
        const AdaptiveDenoisingQueuePopStatus status = m_impl->queue.waitPop(
            m_impl->queuedBlock,
            kWorkerWaitTimeoutMilliseconds);

        if(status == AdaptiveDenoisingQueuePopStatus::Timeout) {
            continue;
        }
        if(status != AdaptiveDenoisingQueuePopStatus::Popped) {
            break;
        }

        m_impl->workerBlock = m_impl->queuedBlock.data.topLeftCorner(
            m_impl->queuedBlock.rowCount,
            m_impl->queuedBlock.sampleCount);

        const bool metadataValid = m_impl->currentMetadataIsValid();
        if(m_impl->currentBlockNeedsConfiguration(metadataValid)) {
            m_impl->configureCurrentBlock(metadataValid);
        }

        if(!metadataValid) {
            continue;
        }

        m_impl->processor.process(m_impl->workerBlock, DenoisingMode::ApplyAndLearn);
        if(!isInterruptionRequested()) {
            m_impl->output->measurementData()->setValue(m_impl->workerBlock);
        }
    }
}

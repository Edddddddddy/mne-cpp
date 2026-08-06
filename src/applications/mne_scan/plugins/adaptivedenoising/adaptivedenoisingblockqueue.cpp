//=============================================================================================================
/**
 * @file     adaptivedenoisingblockqueue.cpp
 * @brief    Preallocated single-producer/single-consumer input queue for adaptive denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingblockqueue.h"

#include <QSemaphore>

#include <atomic>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

namespace
{

using NativeFiffInfoHandle = QSharedPointer<const FIFFLIB::FiffInfo>;

static_assert(
    std::is_nothrow_copy_constructible<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle copy construction must not throw");
static_assert(
    std::is_nothrow_copy_assignable<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle copy assignment must not throw");
static_assert(
    std::is_nothrow_move_constructible<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle move construction must not throw");
static_assert(
    std::is_nothrow_move_assignable<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle move assignment must not throw");

} // namespace

//=============================================================================================================

class AdaptiveDenoisingBlockQueue::Impl final
{
public:
    struct Slot
    {
        Slot(Eigen::Index maxChannelCount, Eigen::Index maxBlockSamples)
        : data(maxChannelCount, maxBlockSamples)
        , rowCount(0)
        , sampleCount(0)
        {
        }

        Eigen::MatrixXd                         data;
        Eigen::Index                            rowCount;
        Eigen::Index                            sampleCount;
        NativeFiffInfoHandle                    fiffInfo;
    };

    explicit Impl(const AdaptiveDenoisingBlockQueueConfig& config)
    : maxChannelCount(config.maxChannelCount)
    , maxBlockSamples(config.maxBlockSamples)
    , capacity(config.capacity)
    , freeSlots(static_cast<int>(config.capacity))
    , filledSlots(0)
    , running(false)
    , producerIndex(0)
    , consumerIndex(0)
    {
        queueSlots.reserve(capacity);
        for (std::size_t index = 0; index < capacity; ++index) {
            queueSlots.emplace_back(maxChannelCount, maxBlockSamples);
        }
    }

    const Eigen::Index maxChannelCount;
    const Eigen::Index maxBlockSamples;
    const std::size_t  capacity;
    std::vector<Slot>  queueSlots;
    QSemaphore         freeSlots;
    QSemaphore         filledSlots;
    std::atomic<bool>  running;
    std::size_t        producerIndex;
    std::size_t        consumerIndex;
};

//=============================================================================================================

AdaptiveDenoisingBlockQueue::AdaptiveDenoisingBlockQueue() noexcept = default;

//=============================================================================================================

AdaptiveDenoisingBlockQueue::~AdaptiveDenoisingBlockQueue() = default;

//=============================================================================================================

AdaptiveDenoisingQueueConfigureStatus AdaptiveDenoisingBlockQueue::configure(
    const AdaptiveDenoisingBlockQueueConfig& config)
{
    if (m_impl && m_impl->running.load(std::memory_order_acquire)) {
        return AdaptiveDenoisingQueueConfigureStatus::AlreadyRunning;
    }

    if (config.maxChannelCount <= 0
        || config.maxBlockSamples <= 0
        || config.capacity == 0
        || config.capacity > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration;
    }

    std::unique_ptr<Impl> candidate(new Impl(config));
    candidate->running.store(true, std::memory_order_release);
    m_impl.swap(candidate);
    return AdaptiveDenoisingQueueConfigureStatus::Ready;
}

//=============================================================================================================

AdaptiveDenoisingQueuePushStatus AdaptiveDenoisingBlockQueue::tryPush(
    Eigen::Ref<const Eigen::MatrixXd> block,
    QSharedPointer<const FIFFLIB::FiffInfo> fiffInfo) noexcept
{
    Impl* const impl = m_impl.get();
    if (!impl || !impl->running.load(std::memory_order_acquire)) {
        return AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    const Eigen::Index rowCount = block.rows();
    const Eigen::Index sampleCount = block.cols();
    if (rowCount <= 0
        || rowCount > impl->maxChannelCount
        || sampleCount <= 0
        || sampleCount > impl->maxBlockSamples) {
        return AdaptiveDenoisingQueuePushStatus::InvalidBlock;
    }

    if (!impl->freeSlots.tryAcquire(1, 0)) {
        return impl->running.load(std::memory_order_acquire)
            ? AdaptiveDenoisingQueuePushStatus::Full
            : AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    std::atomic_thread_fence(std::memory_order_acquire);
    if (!impl->running.load(std::memory_order_acquire)) {
        impl->freeSlots.release(1);
        return AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    Impl::Slot& slot = impl->queueSlots[impl->producerIndex];
    for (Eigen::Index column = 0; column < sampleCount; ++column) {
        for (Eigen::Index row = 0; row < rowCount; ++row) {
            slot.data(row, column) = block(row, column);
        }
    }
    slot.rowCount = rowCount;
    slot.sampleCount = sampleCount;
    slot.fiffInfo = std::move(fiffInfo);

    ++impl->producerIndex;
    if (impl->producerIndex == impl->capacity) {
        impl->producerIndex = 0;
    }

    std::atomic_thread_fence(std::memory_order_release);
    impl->filledSlots.release(1);
    return AdaptiveDenoisingQueuePushStatus::Pushed;
}

//=============================================================================================================

AdaptiveDenoisingQueuePopStatus AdaptiveDenoisingBlockQueue::waitPop(
    AdaptiveDenoisingQueuedBlock& destination,
    int timeoutMilliseconds) noexcept
{
    Impl* const impl = m_impl.get();
    if (!impl || !impl->running.load(std::memory_order_acquire)) {
        return AdaptiveDenoisingQueuePopStatus::Stopped;
    }

    if (destination.data.rows() != impl->maxChannelCount
        || destination.data.cols() != impl->maxBlockSamples) {
        return AdaptiveDenoisingQueuePopStatus::InvalidDestination;
    }

    if (!impl->filledSlots.tryAcquire(1, timeoutMilliseconds)) {
        return impl->running.load(std::memory_order_acquire)
            ? AdaptiveDenoisingQueuePopStatus::Timeout
            : AdaptiveDenoisingQueuePopStatus::Stopped;
    }

    std::atomic_thread_fence(std::memory_order_acquire);
    if (!impl->running.load(std::memory_order_acquire)) {
        return AdaptiveDenoisingQueuePopStatus::Stopped;
    }

    Impl::Slot& slot = impl->queueSlots[impl->consumerIndex];
    for (Eigen::Index column = 0; column < slot.sampleCount; ++column) {
        for (Eigen::Index row = 0; row < slot.rowCount; ++row) {
            destination.data(row, column) = slot.data(row, column);
        }
    }
    destination.rowCount = slot.rowCount;
    destination.sampleCount = slot.sampleCount;
    destination.fiffInfo = std::move(slot.fiffInfo);
    slot.rowCount = 0;
    slot.sampleCount = 0;

    ++impl->consumerIndex;
    if (impl->consumerIndex == impl->capacity) {
        impl->consumerIndex = 0;
    }

    std::atomic_thread_fence(std::memory_order_release);
    impl->freeSlots.release(1);
    return AdaptiveDenoisingQueuePopStatus::Popped;
}

//=============================================================================================================

void AdaptiveDenoisingBlockQueue::stop() noexcept
{
    Impl* const impl = m_impl.get();
    if (impl && impl->running.exchange(false, std::memory_order_acq_rel)) {
        // A consumer cannot be blocked while a filled token is available. Avoid adding a wake token to a full
        // semaphore, whose configured capacity is allowed to equal QSemaphore's maximum int count.
        if (impl->filledSlots.available() == 0) {
            impl->filledSlots.release(1);
        }
    }
}

//=============================================================================================================

} // NAMESPACE

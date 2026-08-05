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
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

class AdaptiveDenoisingBlockQueue::Impl final
{
public:
    struct Slot
    {
        Slot(Eigen::Index channelCount, Eigen::Index maxBlockSamples)
        : data(channelCount, maxBlockSamples)
        , sampleCount(0)
        {
        }

        Eigen::MatrixXd                          data;
        Eigen::Index                             sampleCount;
        std::shared_ptr<const FIFFLIB::FiffInfo> fiffInfo;
    };

    explicit Impl(const AdaptiveDenoisingBlockQueueConfig& config)
    : channelCount(config.channelCount)
    , maxBlockSamples(config.maxBlockSamples)
    , capacity(config.capacity)
    , freeSlots(static_cast<int>(config.capacity))
    , filledSlots(0)
    , running(false)
    , producerIndex(0)
    , consumerIndex(0)
    {
        slots.reserve(capacity);
        for (std::size_t index = 0; index < capacity; ++index) {
            slots.emplace_back(channelCount, maxBlockSamples);
        }
    }

    const Eigen::Index channelCount;
    const Eigen::Index maxBlockSamples;
    const std::size_t  capacity;
    std::vector<Slot>  slots;
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

    if (config.channelCount <= 0
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
    std::shared_ptr<const FIFFLIB::FiffInfo> fiffInfo) noexcept
{
    Impl* const impl = m_impl.get();
    if (!impl || !impl->running.load(std::memory_order_acquire)) {
        return AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    if (block.rows() != impl->channelCount
        || block.cols() <= 0
        || block.cols() > impl->maxBlockSamples) {
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

    Impl::Slot& slot = impl->slots[impl->producerIndex];
    for (Eigen::Index column = 0; column < block.cols(); ++column) {
        for (Eigen::Index row = 0; row < block.rows(); ++row) {
            slot.data(row, column) = block(row, column);
        }
    }
    slot.sampleCount = block.cols();
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

    if (destination.data.rows() != impl->channelCount
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

    Impl::Slot& slot = impl->slots[impl->consumerIndex];
    for (Eigen::Index column = 0; column < slot.sampleCount; ++column) {
        for (Eigen::Index row = 0; row < impl->channelCount; ++row) {
            destination.data(row, column) = slot.data(row, column);
        }
    }
    destination.sampleCount = slot.sampleCount;
    destination.fiffInfo = slot.fiffInfo;
    slot.fiffInfo.reset();

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
        impl->filledSlots.release(1);
    }
}

//=============================================================================================================

} // NAMESPACE

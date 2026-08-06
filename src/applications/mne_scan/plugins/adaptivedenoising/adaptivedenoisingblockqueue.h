//=============================================================================================================
/**
 * @file     adaptivedenoisingblockqueue.h
 * @brief    Preallocated single-producer/single-consumer input queue for adaptive denoising.
 */

#ifndef ADAPTIVEDENOISINGBLOCKQUEUE_ADAPTIVEDENOISINGPLUGIN_H
#define ADAPTIVEDENOISINGBLOCKQUEUE_ADAPTIVEDENOISINGPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <QSharedPointer>

#include <cstddef>
#include <cstdint>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

enum class AdaptiveDenoisingQueueConfigureStatus : std::uint8_t {
    Ready,
    InvalidConfiguration,
    AlreadyRunning
};

//=============================================================================================================

enum class AdaptiveDenoisingQueuePushStatus : std::uint8_t {
    Pushed,
    Full,
    InvalidBlock,
    Stopped
};

//=============================================================================================================

enum class AdaptiveDenoisingQueuePopStatus : std::uint8_t {
    Popped,
    Timeout,
    Stopped,
    InvalidDestination
};

//=============================================================================================================

struct AdaptiveDenoisingBlockQueueConfig
{
    Eigen::Index maxChannelCount;
    Eigen::Index maxBlockSamples;
    std::size_t  capacity;
};

//=============================================================================================================

struct AdaptiveDenoisingQueuedBlock
{
    Eigen::MatrixXd                              data;
    Eigen::Index                                 rowCount;
    Eigen::Index                                 sampleCount;
    QSharedPointer<const FIFFLIB::FiffInfo>      fiffInfo;
};

//=============================================================================================================

/**
 * Plugin-private bounded SPSC queue. Configuration is the only allocation phase and describes maximum matrix
 * dimensions; each pushed block may have any positive shape within those maxima. Exactly one producer and one
 * consumer may use a running queue. stop() may race with those operations, but the caller must quiesce both
 * before reconfiguration or destruction. Stopping may discard pending blocks.
 */
class AdaptiveDenoisingBlockQueue final
{
public:
    AdaptiveDenoisingBlockQueue() noexcept;
    ~AdaptiveDenoisingBlockQueue();
    AdaptiveDenoisingBlockQueue(const AdaptiveDenoisingBlockQueue&) = delete;
    AdaptiveDenoisingBlockQueue& operator=(const AdaptiveDenoisingBlockQueue&) = delete;
    AdaptiveDenoisingBlockQueue(AdaptiveDenoisingBlockQueue&&) = delete;
    AdaptiveDenoisingBlockQueue& operator=(AdaptiveDenoisingBlockQueue&&) = delete;

    /**
     * Transactionally preallocates an empty running queue. Invalid configurations and allocation exceptions
     * preserve the prior stopped implementation; a running queue rejects reconfiguration.
     */
    AdaptiveDenoisingQueueConfigureStatus configure(
        const AdaptiveDenoisingBlockQueueConfig& config);

    /**
     * Makes one zero-time attempt to copy a positive in-bounds block into the queue. A full queue drops this
     * newest block without changing either ring position.
     */
    AdaptiveDenoisingQueuePushStatus tryPush(
        Eigen::Ref<const Eigen::MatrixXd> block,
        QSharedPointer<const FIFFLIB::FiffInfo> fiffInfo) noexcept;

    /**
     * Waits once for a block and copies its valid rectangle into a matrix exactly matching the configured
     * maxima. The matrix tail is untouched; non-Popped results preserve the complete destination.
     */
    AdaptiveDenoisingQueuePopStatus waitPop(
        AdaptiveDenoisingQueuedBlock& destination,
        int timeoutMilliseconds) noexcept;

    /**
     * Idempotently prevents new operations from committing and wakes an empty-queue consumer. An operation
     * that already acquired its semaphore token and observed the running state is ordered before the stop and
     * may finish concurrently.
     */
    void stop() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISINGBLOCKQUEUE_ADAPTIVEDENOISINGPLUGIN_H

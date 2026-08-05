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
    Eigen::Index channelCount;
    Eigen::Index maxBlockSamples;
    std::size_t  capacity;
};

//=============================================================================================================

struct AdaptiveDenoisingQueuedBlock
{
    Eigen::MatrixXd                            data;
    Eigen::Index                               sampleCount;
    std::shared_ptr<const FIFFLIB::FiffInfo>   fiffInfo;
};

//=============================================================================================================

/**
 * Plugin-private bounded SPSC queue. Configuration is the only allocation phase. The caller must quiesce the
 * producer and consumer before reconfiguration or destruction.
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

    AdaptiveDenoisingQueueConfigureStatus configure(
        const AdaptiveDenoisingBlockQueueConfig& config);

    AdaptiveDenoisingQueuePushStatus tryPush(
        Eigen::Ref<const Eigen::MatrixXd> block,
        std::shared_ptr<const FIFFLIB::FiffInfo> fiffInfo) noexcept;

    AdaptiveDenoisingQueuePopStatus waitPop(
        AdaptiveDenoisingQueuedBlock& destination,
        int timeoutMilliseconds) noexcept;

    void stop() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISINGBLOCKQUEUE_ADAPTIVEDENOISINGPLUGIN_H

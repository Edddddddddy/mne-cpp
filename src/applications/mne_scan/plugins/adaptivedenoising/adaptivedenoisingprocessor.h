//=============================================================================================================
/**
 * @file     adaptivedenoisingprocessor.h
 * @brief    Data-only adapter for causal adaptive denoising.
 */

#ifndef ADAPTIVEDENOISINGPROCESSOR_ADAPTIVEDENOISINGPLUGIN_H
#define ADAPTIVEDENOISINGPROCESSOR_ADAPTIVEDENOISINGPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtprocessing/causalreferencedenoiser.h>

#include <Eigen/Core>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

/**
 * @brief Data-only descriptor for one stream channel in row order.
 *
 * The adapter uses only non-bad `FIFFV_REF_MEG_CH` rows as references and
 * non-bad `FIFFV_MEG_CH` rows as targets. Other channel kinds and bad rows are
 * neither selected nor modified by processing.
 */
struct AdaptiveDenoisingChannelDescriptor
{
    int  fiffKind;
    bool isBad;
};

//=============================================================================================================

/**
 * @brief Data-only stream metadata consumed at a processing-worker boundary.
 *
 * `channels` is in matrix row order; this descriptor does not own or inspect
 * Qt/FIFF metadata.
 */
struct AdaptiveDenoisingStreamDescriptor
{
    double                                      samplingFrequencyHz;
    std::vector<AdaptiveDenoisingChannelDescriptor> channels;
};

//=============================================================================================================

/**
 * @brief Settings applied when a fresh numerical candidate is configured.
 *
 * Defaults are 4 taps, a 128-sample interval, 30 seconds of memory, and
 * `1e-3` regularization. The inclusive UI ranges are taps 1..32, interval
 * 16..2048, memory 1..300 seconds, and regularization `1e-8`..1; the numerical
 * feature count `P = referenceCount * tapCount` must additionally be <= 256.
 */
struct AdaptiveDenoisingSettings
{
    Eigen::Index tapCount = 4;
    Eigen::Index adaptationIntervalSamples = 128;
    double       memoryTimeSeconds = 30.0;
    double       regularization = 1e-3;
};

//=============================================================================================================

/** Fixed statuses returned by `AdaptiveDenoisingProcessor::configure()`. */
enum class AdaptiveDenoisingConfigureStatus : std::uint8_t {
    Ready,
    InvalidMetadata,
    MissingReferences,
    MissingTargets,
    InvalidSettings
};

//=============================================================================================================

/**
 * @brief Fixed committed configuration snapshot returned by the adapter.
 *
 * On `Ready`, the counts are selected references, selected targets, and
 * numerical features (`P`) respectively. Every disarmed status returns all
 * three counts as zero.
 */
struct AdaptiveDenoisingConfigureResult
{
    AdaptiveDenoisingConfigureStatus status;
    Eigen::Index                     referenceCount;
    Eigen::Index                     targetCount;
    Eigen::Index                     featureCount;
};

//=============================================================================================================

/**
 * @brief Single-stream, processing-worker-owned concrete adapter.
 *
 * Configuration and numerical ownership belong to one processing worker; the
 * adapter is intentionally noncopyable and nonmovable. Its public descriptors
 * are data-only and contain no strategy, registry, Qt, or FIFF seam.
 */
class AdaptiveDenoisingProcessor final
{
public:
    AdaptiveDenoisingProcessor() noexcept = default;
    AdaptiveDenoisingProcessor(const AdaptiveDenoisingProcessor&) = delete;
    AdaptiveDenoisingProcessor& operator=(const AdaptiveDenoisingProcessor&) = delete;
    AdaptiveDenoisingProcessor(AdaptiveDenoisingProcessor&&) = delete;
    AdaptiveDenoisingProcessor& operator=(AdaptiveDenoisingProcessor&&) = delete;

    /**
     * @brief Configure at a processing block boundary.
     *
     * This call may allocate and throw. A normal `InvalidMetadata`,
     * `MissingReferences`, `MissingTargets`, or `InvalidSettings` (including
     * numerical core rejection) disarms old ownership and returns zero counts.
     * `Ready` installs a fresh numerical candidate, resetting its model and
     * history.
     * Allocation exceptions propagate before the swap and preserve the prior
     * ownership and snapshot; a caller handling new metadata must therefore
     * fail closed rather than process it with that old model.
     */
    AdaptiveDenoisingConfigureResult configure(
        const AdaptiveDenoisingStreamDescriptor& stream,
        Eigen::Index maxBlockSamples,
        const AdaptiveDenoisingSettings& settings);

    /**
     * @brief Process one block through the committed numerical model.
     *
     * When armed this is `noexcept` delegation; only selected target rows may
     * change. The inherited `InvalidShape` and `NonFiniteInput` cases pass the
     * whole block through with state unchanged. When disarmed, the block is an
     * exact pass-through with fixed `NotConfigured` status and zero counts.
     * The fixed numerical statuses are `NotConfigured`, `InvalidShape`,
     * `NonFiniteInput`, `Bypassed`, and `Processed`; `Bypassed` and `Processed`
     * are the only valid armed outcomes. After a successful configure,
     * processing performs no allocation, locking, string construction, or
     * Qt/FIFF metadata work.
     */
    RTPROCESSINGLIB::DenoiserProcessResult process(
        Eigen::Ref<Eigen::MatrixXd> block,
        RTPROCESSINGLIB::DenoisingMode mode) noexcept;

    /**
     * @brief Clear numerical model, history, and pending state when armed.
     *
     * Reset is `noexcept`, leaves the committed configuration and ownership in
     * place, and is a no-op while disarmed.
     */
    void reset() noexcept;

    /** @brief Return the fixed committed configuration snapshot by value. */
    AdaptiveDenoisingConfigureResult configuration() const noexcept;

private:
    AdaptiveDenoisingConfigureResult disarm(
        AdaptiveDenoisingConfigureStatus status) noexcept;

    std::unique_ptr<RTPROCESSINGLIB::CausalReferenceDenoiser> m_denoiser;
    AdaptiveDenoisingConfigureResult m_configuration{
        AdaptiveDenoisingConfigureStatus::InvalidMetadata,
        0,
        0,
        0};
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISINGPROCESSOR_ADAPTIVEDENOISINGPLUGIN_H

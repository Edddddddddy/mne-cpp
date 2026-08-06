//=============================================================================================================
/**
 * @file     causalreferencedenoiser.h
 * @brief    Public interface for causal reference denoising.
 */

#ifndef CAUSALREFERENCEDENOISER_RTPROCESSING_H
#define CAUSALREFERENCEDENOISER_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <Eigen/Core>

#include <cstdint>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================

enum class DenoisingMode {
    BypassTrackHistory,
    ApplyOnly,
    ApplyAndLearn
};

//=============================================================================================================

enum class DenoiserStatus {
    Configured,
    InvalidConfiguration
};

//=============================================================================================================

enum class DenoiserProcessStatus : std::uint8_t {
    NotConfigured,
    InvalidShape,
    NonFiniteInput,
    Bypassed,
    Processed
};

//=============================================================================================================

struct RTPROCESINGSHARED_EXPORT CausalReferenceDenoiserConfig
{
    double        samplingFrequencyHz;
    Eigen::Index  channelCount;
    Eigen::Index  maxBlockSamples;
    Eigen::VectorXi referenceRows;
    Eigen::VectorXi targetRows;
    Eigen::Index  tapCount;
    Eigen::Index  adaptationIntervalSamples;
    double        memoryTimeSeconds;
    double        regularization;
};

//=============================================================================================================

struct RTPROCESINGSHARED_EXPORT DenoiserProcessDiagnostics
{
    Eigen::Index referenceRowCount;
    Eigen::Index targetRowCount;
    Eigen::Index featureCount;
    Eigen::Index warmupSamplesRemaining;
    std::uint64_t modelGeneration;
    std::uint64_t modelUpdatesAccepted;
    std::uint64_t modelUpdatesRejected;
    double inputRms;
    double outputRms;
    double estimatedNoiseRms;
};

//=============================================================================================================

struct RTPROCESINGSHARED_EXPORT DenoiserProcessResult
{
    DenoiserProcessStatus status;
    DenoiserProcessDiagnostics diagnostics;
};

//=============================================================================================================

/**
 * @brief Causal, block-recursive reference denoiser.
 *
 * @details Input rows are channels and columns are chronological time
 * samples. A configuration requires non-empty, individually unique reference
 * and target rows in `[0, channelCount)`, mutually disjoint, with
 * `R = referenceRows.size()` and `R * tapCount <= 256`. Only selected target
 * rows may be changed; every other row is preserved exactly.
 *
 * `configure()` is the sole allocation phase. It commits a complete new
 * state only on `Configured`; a returned `InvalidConfiguration`, or an
 * allocation exception that propagates from candidate-state construction,
 * preserves the previously committed configuration and state. The module is
 * single-stream and worker-thread owned, so calls are serialized by that
 * owner. After configuration, `process()` is `noexcept` and performs no heap
 * allocation, locking, string construction, Qt access or FIFF access. Each
 * call returns the fixed-size `DenoiserProcessResult` as the sole diagnostics
 * snapshot.
 */
class RTPROCESINGSHARED_EXPORT CausalReferenceDenoiser final
{
public:
    /**
     * @brief Construct an unconfigured denoiser without allocation.
     */
    CausalReferenceDenoiser() noexcept;

    /**
     * @brief Destroy the denoiser and its configured state.
     */
    ~CausalReferenceDenoiser();

    CausalReferenceDenoiser(const CausalReferenceDenoiser&) = delete;
    CausalReferenceDenoiser& operator=(const CausalReferenceDenoiser&) = delete;
    CausalReferenceDenoiser(CausalReferenceDenoiser&&) = delete;
    CausalReferenceDenoiser& operator=(CausalReferenceDenoiser&&) = delete;

    /**
     * @brief Validate, allocate and transactionally commit a configuration.
     * @return `Configured` on commit, otherwise `InvalidConfiguration` with
     *         the previous configuration and state retained.
     * @details Positive finite scalar settings, positive dimensions, non-empty
     *          row sets and the row-set invariants documented above are
     *          required. This function is intentionally not `noexcept`:
     *          allocation exceptions propagate while leaving the previous
     *          committed state unchanged.
     */
    DenoiserStatus configure(const CausalReferenceDenoiserConfig& config);

    /**
     * @brief Process one block in place using the selected denoising mode.
     * @details `BypassTrackHistory` preserves the block and advances causal
     *          history without learning. `ApplyOnly` applies the committed
     *          model and advances history without learning. `ApplyAndLearn`
     *          applies the committed model, then learns from the raw block
     *          for later samples. A shape error or non-finite selected input
     *          passes the whole block through and leaves state unchanged.
     *          Only target rows can be modified. After `configure()`, this
     *          operation performs no heap allocation, lock, string creation,
     *          Qt or FIFF access.
     */
    DenoiserProcessResult process(Eigen::Ref<Eigen::MatrixXd> block,
                                  DenoisingMode mode) noexcept;

    /**
     * @brief Clear history, model, pending epochs and diagnostics counters.
     * @details The committed configuration and its allocated storage remain;
     *          the next valid block starts with fresh causal/model state.
     */
    void reset() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

//=============================================================================================================

} // NAMESPACE

#endif // CAUSALREFERENCEDENOISER_RTPROCESSING_H

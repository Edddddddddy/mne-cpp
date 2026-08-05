//=============================================================================================================
/**
 * @file     causalreferencedenoiser.cpp
 * @brief    Causal exponentially weighted reference denoising implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "causalreferencedenoiser.h"

#include <Eigen/Cholesky>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace
{

constexpr double kMinimumRegularization = 1e-8;
constexpr double kMaximumRegularization = 1.0;
constexpr Eigen::Index kMaximumFeatureCount = 256;
constexpr double kLoadingScaleFloor = std::numeric_limits<double>::epsilon();

bool isValidRowSet(const Eigen::VectorXi& rows, Eigen::Index channelCount) noexcept
{
    if (rows.size() == 0) {
        return false;
    }

    for (Eigen::Index i = 0; i < rows.size(); ++i) {
        const Eigen::Index row = rows(i);
        if (row < 0 || row >= channelCount) {
            return false;
        }

        for (Eigen::Index j = 0; j < i; ++j) {
            if (rows(j) == row) {
                return false;
            }
        }
    }

    return true;
}

bool areDisjoint(const Eigen::VectorXi& first, const Eigen::VectorXi& second) noexcept
{
    for (Eigen::Index i = 0; i < first.size(); ++i) {
        for (Eigen::Index j = 0; j < second.size(); ++j) {
            if (first(i) == second(j)) {
                return false;
            }
        }
    }

    return true;
}

struct ScaledSumOfSquares
{
    void add(double value) noexcept
    {
        const double magnitude = std::abs(value);
        if (magnitude == 0.0) {
            return;
        }

        if (scale < magnitude) {
            const double ratio = scale / magnitude;
            sumOfSquares = 1.0 + sumOfSquares * ratio * ratio;
            scale = magnitude;
        } else {
            const double ratio = magnitude / scale;
            sumOfSquares += ratio * ratio;
        }
    }

    double rms(std::uint64_t valueCount) const noexcept
    {
        if (scale == 0.0) {
            return 0.0;
        }

        return scale * std::sqrt(sumOfSquares / static_cast<double>(valueCount));
    }

    double scale = 0.0;
    double sumOfSquares = 1.0;
};

} // namespace

//=============================================================================================================
// DEFINE PRIVATE IMPLEMENTATION
//=============================================================================================================

namespace RTPROCESSINGLIB
{

struct CausalReferenceDenoiser::Impl
{
    explicit Impl(const CausalReferenceDenoiserConfig& config)
    : channelCount(config.channelCount)
    , maxBlockSamples(config.maxBlockSamples)
    , referenceCount(config.referenceRows.size())
    , targetCount(config.targetRows.size())
    , tapCount(config.tapCount)
    , featureCount(referenceCount * tapCount)
    , adaptationIntervalSamples(config.adaptationIntervalSamples)
    , forgettingFactor(std::exp(-1.0
                                / (config.samplingFrequencyHz * config.memoryTimeSeconds)))
    , regularization(config.regularization)
    , referenceRows(config.referenceRows)
    , targetRows(config.targetRows)
    , currentReferences(referenceCount)
    , causalHistory(referenceCount, tapCount - 1)
    , feature(featureCount)
    , rawTargets(targetCount)
    , prediction(targetCount)
    , committedGram(featureCount, featureCount)
    , committedCross(targetCount, featureCount)
    , pendingGram(featureCount, featureCount)
    , pendingCross(targetCount, featureCount)
    , candidateGram(featureCount, featureCount)
    , candidateCross(targetCount, featureCount)
    , weights(targetCount, featureCount)
    , solveMatrix(featureCount, featureCount)
    , solveRightHandSide(featureCount, targetCount)
    , candidateWeightsTranspose(featureCount, targetCount)
    , decomposition(featureCount)
    {
        reset();
    }

    void reset() noexcept
    {
        currentReferences.setZero();
        causalHistory.setZero();
        feature.setZero();
        rawTargets.setZero();
        prediction.setZero();
        committedGram.setZero();
        committedCross.setZero();
        pendingGram.setZero();
        pendingCross.setZero();
        candidateGram.setZero();
        candidateCross.setZero();
        weights.setZero();
        solveMatrix.setZero();
        solveRightHandSide.setZero();
        candidateWeightsTranspose.setZero();
        decomposition.setZero();
        historySampleCount = 0;
        eligibleLearningSampleCount = 0;
        pendingEpochDecay = 1.0;
        modelGeneration = 0;
    }

    bool hasCompleteHistory() const noexcept
    {
        return historySampleCount == causalHistory.cols();
    }

    void advanceHistory() noexcept
    {
        if (causalHistory.cols() == 0) {
            return;
        }

        for (Eigen::Index lag = causalHistory.cols() - 1; lag > 0; --lag) {
            for (Eigen::Index reference = 0; reference < referenceCount; ++reference) {
                causalHistory(reference, lag) = causalHistory(reference, lag - 1);
            }
        }

        for (Eigen::Index reference = 0; reference < referenceCount; ++reference) {
            causalHistory(reference, 0) = currentReferences(reference);
        }

        if (historySampleCount < causalHistory.cols()) {
            ++historySampleCount;
        }
    }

    void buildFeature() noexcept
    {
        for (Eigen::Index reference = 0; reference < referenceCount; ++reference) {
            feature(reference) = currentReferences(reference);
        }

        for (Eigen::Index lag = 0; lag < causalHistory.cols(); ++lag) {
            const Eigen::Index featureOffset = (lag + 1) * referenceCount;
            for (Eigen::Index reference = 0; reference < referenceCount; ++reference) {
                feature(featureOffset + reference) = causalHistory(reference, lag);
            }
        }
    }

    void accumulatePendingStatistics() noexcept
    {
        // After n samples, pendingEpochDecay is lambda^n and the pending
        // matrices contain the recurrence initialized from zero.
        pendingEpochDecay *= forgettingFactor;

        pendingGram *= forgettingFactor;
        pendingGram.noalias() += feature * feature.transpose();

        pendingCross *= forgettingFactor;
        pendingCross.noalias() += rawTargets * feature.transpose();
    }

    bool solveAndCommitPendingEpoch() noexcept
    {
        // Compose the full recurrence without modifying the committed state.
        candidateGram.noalias() = committedGram;
        candidateGram *= pendingEpochDecay;
        candidateGram += pendingGram;

        candidateCross.noalias() = committedCross;
        candidateCross *= pendingEpochDecay;
        candidateCross += pendingCross;

        if (!candidateGram.allFinite() || !candidateCross.allFinite()) {
            return false;
        }

        double diagonalScale = 0.0;

        for (Eigen::Index row = 0; row < featureCount; ++row) {
            for (Eigen::Index column = 0; column <= row; ++column) {
                const double symmetricValue = 0.5 * candidateGram(row, column)
                                              + 0.5 * candidateGram(column, row);
                solveMatrix(row, column) = symmetricValue;
                solveMatrix(column, row) = symmetricValue;
            }

            diagonalScale = std::max(diagonalScale, std::abs(solveMatrix(row, row)));
        }

        // A = sym(G) + regularization * max(max_i |sym(G)_ii|, eps) * I.
        const double loadingScale = std::max(diagonalScale, kLoadingScaleFloor);
        const double diagonalLoading = regularization * loadingScale;
        for (Eigen::Index diagonal = 0; diagonal < featureCount; ++diagonal) {
            solveMatrix(diagonal, diagonal) += diagonalLoading;
        }

        if (!solveMatrix.allFinite()) {
            return false;
        }

        decomposition.compute(solveMatrix);
        if (decomposition.info() != Eigen::Success
            || !decomposition.vectorD().allFinite()) {
            return false;
        }

        solveRightHandSide.noalias() = candidateCross.transpose();
        candidateWeightsTranspose.noalias() = decomposition.solve(solveRightHandSide);
        if (decomposition.info() != Eigen::Success
            || !candidateWeightsTranspose.allFinite()) {
            return false;
        }

        // All numerical checks have passed; commit candidate G/H/W together.
        committedGram.noalias() = candidateGram;
        committedCross.noalias() = candidateCross;
        weights.noalias() = candidateWeightsTranspose.transpose();
        return true;
    }

    void rejectPendingEpoch() noexcept
    {
        committedGram *= pendingEpochDecay;
        committedCross *= pendingEpochDecay;
    }

    void clearPendingEpoch() noexcept
    {
        pendingGram.setZero();
        pendingCross.setZero();
        eligibleLearningSampleCount = 0;
        pendingEpochDecay = 1.0;
    }

    const Eigen::Index channelCount;
    const Eigen::Index maxBlockSamples;
    const Eigen::Index referenceCount;
    const Eigen::Index targetCount;
    const Eigen::Index tapCount;
    const Eigen::Index featureCount;
    const Eigen::Index adaptationIntervalSamples;
    const double forgettingFactor;
    const double regularization;

    Eigen::VectorXi referenceRows;
    Eigen::VectorXi targetRows;
    Eigen::VectorXd currentReferences;
    Eigen::MatrixXd causalHistory;
    Eigen::VectorXd feature;
    Eigen::VectorXd rawTargets;
    Eigen::VectorXd prediction;
    Eigen::MatrixXd committedGram;
    Eigen::MatrixXd committedCross;
    Eigen::MatrixXd pendingGram;
    Eigen::MatrixXd pendingCross;
    Eigen::MatrixXd candidateGram;
    Eigen::MatrixXd candidateCross;
    Eigen::MatrixXd weights;
    Eigen::MatrixXd solveMatrix;
    Eigen::MatrixXd solveRightHandSide;
    Eigen::MatrixXd candidateWeightsTranspose;
    Eigen::LDLT<Eigen::MatrixXd> decomposition;

    Eigen::Index historySampleCount = 0;
    Eigen::Index eligibleLearningSampleCount = 0;
    double pendingEpochDecay = 1.0;
    std::uint64_t modelGeneration = 0;
};

} // namespace RTPROCESSINGLIB

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CausalReferenceDenoiser::CausalReferenceDenoiser() noexcept = default;

//=============================================================================================================

CausalReferenceDenoiser::~CausalReferenceDenoiser() = default;

//=============================================================================================================

DenoiserStatus CausalReferenceDenoiser::configure(const CausalReferenceDenoiserConfig& config)
{
    if (config.channelCount <= 0
        || config.maxBlockSamples <= 0
        || !std::isfinite(config.samplingFrequencyHz)
        || config.samplingFrequencyHz <= 0.0
        || config.tapCount <= 0
        || config.adaptationIntervalSamples <= 0
        || !std::isfinite(config.memoryTimeSeconds)
        || config.memoryTimeSeconds <= 0.0
        || !std::isfinite(config.regularization)
        || config.regularization < kMinimumRegularization
        || config.regularization > kMaximumRegularization
        || !isValidRowSet(config.referenceRows, config.channelCount)
        || !isValidRowSet(config.targetRows, config.channelCount)
        || !areDisjoint(config.referenceRows, config.targetRows)
        || config.tapCount > kMaximumFeatureCount / config.referenceRows.size()) {
        return DenoiserStatus::InvalidConfiguration;
    }

    std::unique_ptr<Impl> candidate(new Impl(config));
    m_impl.swap(candidate);
    return DenoiserStatus::Configured;
}

//=============================================================================================================

DenoiserProcessResult CausalReferenceDenoiser::process(Eigen::Ref<Eigen::MatrixXd> block,
                                                       DenoisingMode mode) noexcept
{
    const double quietNaN = std::numeric_limits<double>::quiet_NaN();

    if (!m_impl) {
        return DenoiserProcessResult{
            DenoiserProcessStatus::NotConfigured,
            DenoiserProcessDiagnostics{0, 0, 0, 0, 0, 0, 0,
                                       quietNaN, quietNaN, quietNaN}};
    }

    Impl& state = *m_impl;
    const auto errorResult = [&state, quietNaN](DenoiserProcessStatus status) noexcept {
        return DenoiserProcessResult{
            status,
            DenoiserProcessDiagnostics{
                state.referenceCount,
                state.targetCount,
                state.featureCount,
                state.causalHistory.cols() - state.historySampleCount,
                state.modelGeneration,
                0,
                0,
                quietNaN,
                quietNaN,
                quietNaN}};
    };

    if (block.rows() != state.channelCount
        || block.cols() <= 0
        || block.cols() > state.maxBlockSamples) {
        return errorResult(DenoiserProcessStatus::InvalidShape);
    }

    for (Eigen::Index reference = 0; reference < state.referenceCount; ++reference) {
        const Eigen::Index row = state.referenceRows(reference);
        for (Eigen::Index sample = 0; sample < block.cols(); ++sample) {
            if (!std::isfinite(block(row, sample))) {
                return errorResult(DenoiserProcessStatus::NonFiniteInput);
            }
        }
    }

    for (Eigen::Index target = 0; target < state.targetCount; ++target) {
        const Eigen::Index row = state.targetRows(target);
        for (Eigen::Index sample = 0; sample < block.cols(); ++sample) {
            if (!std::isfinite(block(row, sample))) {
                return errorResult(DenoiserProcessStatus::NonFiniteInput);
            }
        }
    }

    const bool bypass = mode == DenoisingMode::BypassTrackHistory;
    const bool learn = mode == DenoisingMode::ApplyAndLearn;
    ScaledSumOfSquares inputSumOfSquares;
    ScaledSumOfSquares outputSumOfSquares;
    ScaledSumOfSquares estimatedNoiseSumOfSquares;
    std::uint64_t modelUpdatesAccepted = 0;
    std::uint64_t modelUpdatesRejected = 0;

    for (Eigen::Index sample = 0; sample < block.cols(); ++sample) {
        for (Eigen::Index target = 0; target < state.targetCount; ++target) {
            inputSumOfSquares.add(block(state.targetRows(target), sample));
        }

        for (Eigen::Index reference = 0; reference < state.referenceCount; ++reference) {
            state.currentReferences(reference) =
                block(state.referenceRows(reference), sample);
        }

        if (bypass || !state.hasCompleteHistory()) {
            state.advanceHistory();
            continue;
        }

        state.buildFeature();

        for (Eigen::Index target = 0; target < state.targetCount; ++target) {
            state.rawTargets(target) = block(state.targetRows(target), sample);
        }

        state.prediction.noalias() = state.weights * state.feature;
        for (Eigen::Index target = 0; target < state.targetCount; ++target) {
            estimatedNoiseSumOfSquares.add(state.prediction(target));
            block(state.targetRows(target), sample) =
                state.rawTargets(target) - state.prediction(target);
        }

        if (learn) {
            state.accumulatePendingStatistics();

            ++state.eligibleLearningSampleCount;
            if (state.eligibleLearningSampleCount == state.adaptationIntervalSamples) {
                if (state.solveAndCommitPendingEpoch()) {
                    ++modelUpdatesAccepted;
                    ++state.modelGeneration;
                } else {
                    state.rejectPendingEpoch();
                    ++modelUpdatesRejected;
                }
                state.clearPendingEpoch();
            }
        }

        state.advanceHistory();
    }

    for (Eigen::Index target = 0; target < state.targetCount; ++target) {
        const Eigen::Index row = state.targetRows(target);
        for (Eigen::Index sample = 0; sample < block.cols(); ++sample) {
            outputSumOfSquares.add(block(row, sample));
        }
    }

    const std::uint64_t valueCount = static_cast<std::uint64_t>(state.targetCount)
                                     * static_cast<std::uint64_t>(block.cols());

    return DenoiserProcessResult{
        bypass ? DenoiserProcessStatus::Bypassed : DenoiserProcessStatus::Processed,
        DenoiserProcessDiagnostics{
            state.referenceCount,
            state.targetCount,
            state.featureCount,
            state.causalHistory.cols() - state.historySampleCount,
            state.modelGeneration,
            modelUpdatesAccepted,
            modelUpdatesRejected,
            inputSumOfSquares.rms(valueCount),
            outputSumOfSquares.rms(valueCount),
            estimatedNoiseSumOfSquares.rms(valueCount)}};
}

//=============================================================================================================

void CausalReferenceDenoiser::reset() noexcept
{
    if (m_impl) {
        m_impl->reset();
    }
}

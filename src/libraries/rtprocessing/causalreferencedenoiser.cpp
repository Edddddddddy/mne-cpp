//=============================================================================================================
/**
 * @file     causalreferencedenoiser.cpp
 * @brief    Minimal public-interface implementation for causal reference denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "causalreferencedenoiser.h"

#include <cmath>

namespace
{

constexpr double kMinimumRegularization = 1e-8;
constexpr double kMaximumRegularization = 1.0;
constexpr Eigen::Index kMaximumFeatureCount = 256;

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

} // namespace

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
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

    m_channelCount = config.channelCount;
    m_maxBlockSamples = config.maxBlockSamples;
    return DenoiserStatus::Configured;
}

//=============================================================================================================

DenoiserProcessResult CausalReferenceDenoiser::process(Eigen::Ref<Eigen::MatrixXd> block,
                                                       DenoisingMode mode) noexcept
{
    (void)mode;

    if (block.rows() != m_channelCount
        || block.cols() <= 0
        || block.cols() > m_maxBlockSamples) {
        return DenoiserProcessResult{DenoiserProcessStatus::InvalidShape};
    }

    return DenoiserProcessResult{DenoiserProcessStatus::Bypassed};
}

//=============================================================================================================

void CausalReferenceDenoiser::reset() noexcept
{
}

//=============================================================================================================
/**
 * @file     causalreferencedenoiser.cpp
 * @brief    Minimal public-interface implementation for causal reference denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "causalreferencedenoiser.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DenoiserStatus CausalReferenceDenoiser::configure(const CausalReferenceDenoiserConfig& config)
{
    if (config.channelCount <= 0 || config.maxBlockSamples <= 0) {
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

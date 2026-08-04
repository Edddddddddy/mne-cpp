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
    (void)config;
    return DenoiserStatus::Configured;
}

//=============================================================================================================

DenoiserProcessResult CausalReferenceDenoiser::process(Eigen::Ref<Eigen::MatrixXd> block,
                                                       DenoisingMode mode) noexcept
{
    (void)block;
    (void)mode;
    return DenoiserProcessResult{DenoiserProcessStatus::Bypassed};
}

//=============================================================================================================

void CausalReferenceDenoiser::reset() noexcept
{
}

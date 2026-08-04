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

enum class DenoiserProcessStatus {
    Bypassed,
    InvalidShape
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

struct RTPROCESINGSHARED_EXPORT DenoiserProcessResult
{
    DenoiserProcessStatus status;
};

//=============================================================================================================

class RTPROCESINGSHARED_EXPORT CausalReferenceDenoiser final
{
public:
    DenoiserStatus configure(const CausalReferenceDenoiserConfig& config);

    DenoiserProcessResult process(Eigen::Ref<Eigen::MatrixXd> block,
                                  DenoisingMode mode) noexcept;

    void reset() noexcept;

private:
    Eigen::Index m_channelCount = 0;
    Eigen::Index m_maxBlockSamples = 0;
};

//=============================================================================================================

} // NAMESPACE

#endif // CAUSALREFERENCEDENOISER_RTPROCESSING_H

//=============================================================================================================
/**
 * @file     adaptivedenoisingprocessor.cpp
 * @brief    Data-only adapter for causal adaptive denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingprocessor.h"

#include <fiff/fiff_constants.h>

#include <cmath>
#include <limits>

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

AdaptiveDenoisingConfigureResult AdaptiveDenoisingProcessor::disarm(
    AdaptiveDenoisingConfigureStatus status) noexcept
{
    m_denoiser.reset();
    m_configuration = AdaptiveDenoisingConfigureResult{status, 0, 0, 0};
    return m_configuration;
}

//=============================================================================================================

AdaptiveDenoisingConfigureResult AdaptiveDenoisingProcessor::configure(
    const AdaptiveDenoisingStreamDescriptor& stream,
    Eigen::Index maxBlockSamples,
    const AdaptiveDenoisingSettings& settings)
{
    if (!std::isfinite(stream.samplingFrequencyHz)
        || stream.samplingFrequencyHz <= 0.0
        || stream.channels.empty()
        || maxBlockSamples <= 0) {
        return disarm(AdaptiveDenoisingConfigureStatus::InvalidMetadata);
    }

    if (settings.tapCount < 1
        || settings.tapCount > 32
        || settings.adaptationIntervalSamples < 16
        || settings.adaptationIntervalSamples > 2048
        || !std::isfinite(settings.memoryTimeSeconds)
        || settings.memoryTimeSeconds < 1.0
        || settings.memoryTimeSeconds > 300.0
        || !std::isfinite(settings.regularization)
        || settings.regularization < 1e-8
        || settings.regularization > 1.0) {
        return disarm(AdaptiveDenoisingConfigureStatus::InvalidSettings);
    }

    const Eigen::Index channelCount = static_cast<Eigen::Index>(stream.channels.size());
    Eigen::Index referenceCount = 0;
    Eigen::Index targetCount = 0;

    for (Eigen::Index row = 0; row < channelCount; ++row) {
        const AdaptiveDenoisingChannelDescriptor& channel =
            stream.channels[static_cast<std::size_t>(row)];

        if (channel.isBad) {
            continue;
        }

        if (channel.fiffKind == FIFFV_REF_MEG_CH) {
            ++referenceCount;
        } else if (channel.fiffKind == FIFFV_MEG_CH) {
            ++targetCount;
        }
    }

    if (referenceCount == 0) {
        return disarm(AdaptiveDenoisingConfigureStatus::MissingReferences);
    }

    if (targetCount == 0) {
        return disarm(AdaptiveDenoisingConfigureStatus::MissingTargets);
    }

    Eigen::VectorXi referenceRows(referenceCount);
    Eigen::VectorXi targetRows(targetCount);
    Eigen::Index referencePosition = 0;
    Eigen::Index targetPosition = 0;

    for (Eigen::Index row = 0; row < channelCount; ++row) {
        const AdaptiveDenoisingChannelDescriptor& channel =
            stream.channels[static_cast<std::size_t>(row)];

        if (channel.isBad) {
            continue;
        }

        if (channel.fiffKind == FIFFV_REF_MEG_CH) {
            referenceRows(referencePosition) = static_cast<int>(row);
            ++referencePosition;
        } else if (channel.fiffKind == FIFFV_MEG_CH) {
            targetRows(targetPosition) = static_cast<int>(row);
            ++targetPosition;
        }
    }

    RTPROCESSINGLIB::CausalReferenceDenoiserConfig numericalConfig{
        stream.samplingFrequencyHz,
        channelCount,
        maxBlockSamples,
        referenceRows,
        targetRows,
        settings.tapCount,
        settings.adaptationIntervalSamples,
        settings.memoryTimeSeconds,
        settings.regularization};

    std::unique_ptr<RTPROCESSINGLIB::CausalReferenceDenoiser> candidate(
        new RTPROCESSINGLIB::CausalReferenceDenoiser);

    if (candidate->configure(numericalConfig)
        != RTPROCESSINGLIB::DenoiserStatus::Configured) {
        return disarm(AdaptiveDenoisingConfigureStatus::InvalidSettings);
    }

    m_denoiser.swap(candidate);
    m_configuration = AdaptiveDenoisingConfigureResult{
        AdaptiveDenoisingConfigureStatus::Ready,
        referenceCount,
        targetCount,
        referenceCount * settings.tapCount};
    return m_configuration;
}

//=============================================================================================================

RTPROCESSINGLIB::DenoiserProcessResult AdaptiveDenoisingProcessor::process(
    Eigen::Ref<Eigen::MatrixXd> block,
    RTPROCESSINGLIB::DenoisingMode mode) noexcept
{
    if (m_denoiser) {
        return m_denoiser->process(block, mode);
    }

    const double quietNaN = std::numeric_limits<double>::quiet_NaN();
    return RTPROCESSINGLIB::DenoiserProcessResult{
        RTPROCESSINGLIB::DenoiserProcessStatus::NotConfigured,
        RTPROCESSINGLIB::DenoiserProcessDiagnostics{
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            quietNaN,
            quietNaN,
            quietNaN}};
}

//=============================================================================================================

void AdaptiveDenoisingProcessor::reset() noexcept
{
    if (m_denoiser) {
        m_denoiser->reset();
    }
}

//=============================================================================================================

AdaptiveDenoisingConfigureResult AdaptiveDenoisingProcessor::configuration() const noexcept
{
    return m_configuration;
}

//=============================================================================================================

} // NAMESPACE

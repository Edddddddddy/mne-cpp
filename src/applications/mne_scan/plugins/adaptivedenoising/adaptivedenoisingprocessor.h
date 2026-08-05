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

struct AdaptiveDenoisingChannelDescriptor
{
    int  fiffKind;
    bool isBad;
};

//=============================================================================================================

struct AdaptiveDenoisingStreamDescriptor
{
    double                                      samplingFrequencyHz;
    std::vector<AdaptiveDenoisingChannelDescriptor> channels;
};

//=============================================================================================================

struct AdaptiveDenoisingSettings
{
    Eigen::Index tapCount = 4;
    Eigen::Index adaptationIntervalSamples = 128;
    double       memoryTimeSeconds = 30.0;
    double       regularization = 1e-3;
};

//=============================================================================================================

enum class AdaptiveDenoisingConfigureStatus : std::uint8_t {
    Ready,
    InvalidMetadata,
    MissingReferences,
    MissingTargets,
    InvalidSettings
};

//=============================================================================================================

struct AdaptiveDenoisingConfigureResult
{
    AdaptiveDenoisingConfigureStatus status;
    Eigen::Index                     referenceCount;
    Eigen::Index                     targetCount;
    Eigen::Index                     featureCount;
};

//=============================================================================================================

class AdaptiveDenoisingProcessor final
{
public:
    AdaptiveDenoisingProcessor() noexcept = default;
    AdaptiveDenoisingProcessor(const AdaptiveDenoisingProcessor&) = delete;
    AdaptiveDenoisingProcessor& operator=(const AdaptiveDenoisingProcessor&) = delete;
    AdaptiveDenoisingProcessor(AdaptiveDenoisingProcessor&&) = delete;
    AdaptiveDenoisingProcessor& operator=(AdaptiveDenoisingProcessor&&) = delete;

    AdaptiveDenoisingConfigureResult configure(
        const AdaptiveDenoisingStreamDescriptor& stream,
        Eigen::Index maxBlockSamples,
        const AdaptiveDenoisingSettings& settings);

    RTPROCESSINGLIB::DenoiserProcessResult process(
        Eigen::Ref<Eigen::MatrixXd> block,
        RTPROCESSINGLIB::DenoisingMode mode) noexcept;

    void reset() noexcept;

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

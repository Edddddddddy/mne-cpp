//=============================================================================================================
/**
 * @file     adaptivedenoisingdiagnostics.h
 * @brief    Fixed GUI diagnostics value for the adaptive denoising plugin.
 */

#ifndef ADAPTIVEDENOISINGDIAGNOSTICS_ADAPTIVEDENOISINGPLUGIN_H
#define ADAPTIVEDENOISINGDIAGNOSTICS_ADAPTIVEDENOISINGPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingprocessor.h"

#include <QtCore/QMetaType>

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

enum class AdaptiveDenoisingPluginState : std::uint8_t {
    Stopped,
    WaitingForData,
    Processing,
    InvalidMetadata,
    ConfigurationException
};

//=============================================================================================================

struct AdaptiveDenoisingDiagnostics
{
    AdaptiveDenoisingPluginState              pluginState;
    AdaptiveDenoisingConfigureStatus          configureStatus;
    RTPROCESSINGLIB::DenoiserProcessStatus   processStatus;
    Eigen::Index                              referenceRowCount;
    Eigen::Index                              targetRowCount;
    Eigen::Index                              featureCount;
    Eigen::Index                              warmupSamplesRemaining;
    std::uint64_t                             modelGeneration;
    std::uint64_t                             modelUpdatesAccepted;
    std::uint64_t                             modelUpdatesRejected;
    double                                    inputRms;
    double                                    outputRms;
    double                                    estimatedNoiseRms;
    std::uint64_t                             droppedBlocks;
};

//=============================================================================================================

} // NAMESPACE

Q_DECLARE_METATYPE(ADAPTIVEDENOISINGPLUGIN::AdaptiveDenoisingDiagnostics)

#endif // ADAPTIVEDENOISINGDIAGNOSTICS_ADAPTIVEDENOISINGPLUGIN_H

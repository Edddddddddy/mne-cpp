//=============================================================================================================
/**
 * @file     adaptivedenoisingvisualizationmodel.h
 * @brief    Fixed worker-to-GUI visualization snapshot for adaptive denoising.
 */

#ifndef ADAPTIVEDENOISINGVISUALIZATIONMODEL_ADAPTIVEDENOISINGPLUGIN_H
#define ADAPTIVEDENOISINGVISUALIZATIONMODEL_ADAPTIVEDENOISINGPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingdiagnostics.h"

#include <Eigen/Core>

#include <array>
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

constexpr std::size_t kAdaptiveDenoisingTraceCapacity = 256u;
constexpr std::size_t kAdaptiveDenoisingRmsHistoryCapacity = 120u;

//=============================================================================================================
/**
 * Complete bounded value copied from the processing worker to the GUI timer.
 * Trace arrays use [0, traceSampleCount); RMS arrays use [0, rmsHistoryCount)
 * in chronological order.
 */
struct AdaptiveDenoisingVisualizationSnapshot
{
    std::uint64_t sequence = 0u;
    Eigen::Index selectedTargetOrdinal = 0;
    Eigen::Index selectedTargetRow = -1;
    Eigen::Index targetCount = 0;
    Eigen::Index sourceSampleCount = 0;
    Eigen::Index traceSampleCount = 0;
    Eigen::Index rmsHistoryCount = 0;
    std::array<double, kAdaptiveDenoisingTraceCapacity> raw{};
    std::array<double, kAdaptiveDenoisingTraceCapacity> denoised{};
    std::array<double, kAdaptiveDenoisingTraceCapacity> estimatedNoise{};
    std::array<double, kAdaptiveDenoisingRmsHistoryCapacity> inputRmsHistory{};
    std::array<double, kAdaptiveDenoisingRmsHistoryCapacity> outputRmsHistory{};
    std::array<double, kAdaptiveDenoisingRmsHistoryCapacity> estimatedNoiseRmsHistory{};
};

//=============================================================================================================
/**
 * Plugin-private deep module for bounded visualization publication.
 *
 * captureInput() and publishOutput() are called serially by the processing
 * worker around one numerical process call. snapshot() is called by the GUI
 * timer. All storage is fixed after construction; streaming calls do not
 * allocate and never retain an Eigen block.
 */
class AdaptiveDenoisingVisualizationModel final
{
public:
    AdaptiveDenoisingVisualizationModel();
    ~AdaptiveDenoisingVisualizationModel();

    AdaptiveDenoisingVisualizationModel(const AdaptiveDenoisingVisualizationModel&) = delete;
    AdaptiveDenoisingVisualizationModel& operator=(const AdaptiveDenoisingVisualizationModel&) = delete;
    AdaptiveDenoisingVisualizationModel(AdaptiveDenoisingVisualizationModel&&) = delete;
    AdaptiveDenoisingVisualizationModel& operator=(AdaptiveDenoisingVisualizationModel&&) = delete;

    void setSelectedTargetOrdinal(int targetOrdinal) noexcept;
    int selectedTargetOrdinal() const noexcept;

    void captureInput(const Eigen::MatrixXd& rawBlock,
                      const std::vector<Eigen::Index>& targetRows) noexcept;
    void publishOutput(const Eigen::MatrixXd& denoisedBlock,
                       const AdaptiveDenoisingDiagnostics& diagnostics) noexcept;

    AdaptiveDenoisingVisualizationSnapshot snapshot() const noexcept;
    void clear() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISINGVISUALIZATIONMODEL_ADAPTIVEDENOISINGPLUGIN_H


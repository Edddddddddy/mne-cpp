//=============================================================================================================
/**
 * @file     adaptivedenoisingvisualizationmodel.cpp
 * @brief    Fixed worker-to-GUI visualization snapshot implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingvisualizationmodel.h"

#include <QMutex>
#include <QMutexLocker>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>

//=============================================================================================================

using namespace ADAPTIVEDENOISINGPLUGIN;

//=============================================================================================================

static_assert(ATOMIC_INT_LOCK_FREE == 2,
              "Adaptive Denoising target selection requires an always-lock-free int atomic.");

//=============================================================================================================
// DEFINE PRIVATE IMPLEMENTATION
//=============================================================================================================

class AdaptiveDenoisingVisualizationModel::Impl
{
public:
    struct PendingTrace
    {
        bool valid = false;
        Eigen::Index selectedTargetOrdinal = 0;
        Eigen::Index selectedTargetRow = -1;
        Eigen::Index targetCount = 0;
        Eigen::Index sourceSampleCount = 0;
        Eigen::Index traceSampleCount = 0;
        std::array<Eigen::Index, kAdaptiveDenoisingTraceCapacity> sourceColumns{};
        std::array<double, kAdaptiveDenoisingTraceCapacity> raw{};
    };

    void appendRms(const AdaptiveDenoisingDiagnostics& diagnostics) noexcept
    {
        if(!std::isfinite(diagnostics.inputRms)
           || !std::isfinite(diagnostics.outputRms)
           || !std::isfinite(diagnostics.estimatedNoiseRms)) {
            return;
        }

        inputRmsRing[rmsWriteIndex] = diagnostics.inputRms;
        outputRmsRing[rmsWriteIndex] = diagnostics.outputRms;
        estimatedNoiseRmsRing[rmsWriteIndex] = diagnostics.estimatedNoiseRms;
        rmsWriteIndex = (rmsWriteIndex + 1u) % kAdaptiveDenoisingRmsHistoryCapacity;
        rmsCount = std::min(rmsCount + 1u, kAdaptiveDenoisingRmsHistoryCapacity);
    }

    void copyChronologicalRms(AdaptiveDenoisingVisualizationSnapshot& destination) const noexcept
    {
        destination.rmsHistoryCount = static_cast<Eigen::Index>(rmsCount);
        const std::size_t first = rmsCount < kAdaptiveDenoisingRmsHistoryCapacity
            ? 0u
            : rmsWriteIndex;

        for(std::size_t index = 0u; index < rmsCount; ++index) {
            const std::size_t ringIndex =
                (first + index) % kAdaptiveDenoisingRmsHistoryCapacity;
            destination.inputRmsHistory[index] = inputRmsRing[ringIndex];
            destination.outputRmsHistory[index] = outputRmsRing[ringIndex];
            destination.estimatedNoiseRmsHistory[index] = estimatedNoiseRmsRing[ringIndex];
        }
    }

    mutable QMutex mutex;
    std::atomic<int> selectedTarget{0};
    PendingTrace pendingTrace;
    AdaptiveDenoisingVisualizationSnapshot publishedSnapshot;
    std::array<double, kAdaptiveDenoisingRmsHistoryCapacity> inputRmsRing{};
    std::array<double, kAdaptiveDenoisingRmsHistoryCapacity> outputRmsRing{};
    std::array<double, kAdaptiveDenoisingRmsHistoryCapacity> estimatedNoiseRmsRing{};
    std::size_t rmsWriteIndex = 0u;
    std::size_t rmsCount = 0u;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AdaptiveDenoisingVisualizationModel::AdaptiveDenoisingVisualizationModel()
: m_impl(new Impl)
{
}

//=============================================================================================================

AdaptiveDenoisingVisualizationModel::~AdaptiveDenoisingVisualizationModel() = default;

//=============================================================================================================

void AdaptiveDenoisingVisualizationModel::setSelectedTargetOrdinal(int targetOrdinal) noexcept
{
    m_impl->selectedTarget.store(std::max(0, targetOrdinal), std::memory_order_release);
}

//=============================================================================================================

int AdaptiveDenoisingVisualizationModel::selectedTargetOrdinal() const noexcept
{
    return m_impl->selectedTarget.load(std::memory_order_acquire);
}

//=============================================================================================================

void AdaptiveDenoisingVisualizationModel::captureInput(
    const Eigen::MatrixXd& rawBlock,
    const std::vector<Eigen::Index>& targetRows) noexcept
{
    Impl::PendingTrace& pending = m_impl->pendingTrace;
    pending = Impl::PendingTrace{};

    if(rawBlock.cols() <= 0 || targetRows.empty()) {
        return;
    }

    const Eigen::Index targetCount = static_cast<Eigen::Index>(targetRows.size());
    const Eigen::Index requestedOrdinal = static_cast<Eigen::Index>(selectedTargetOrdinal());
    const Eigen::Index targetOrdinal = std::min(requestedOrdinal, targetCount - 1);
    const Eigen::Index targetRow = targetRows[static_cast<std::size_t>(targetOrdinal)];
    if(targetRow < 0 || targetRow >= rawBlock.rows()) {
        return;
    }

    pending.valid = true;
    pending.selectedTargetOrdinal = targetOrdinal;
    pending.selectedTargetRow = targetRow;
    pending.targetCount = targetCount;
    pending.sourceSampleCount = rawBlock.cols();
    pending.traceSampleCount = std::min<Eigen::Index>(
        rawBlock.cols(),
        static_cast<Eigen::Index>(kAdaptiveDenoisingTraceCapacity));

    for(Eigen::Index traceIndex = 0; traceIndex < pending.traceSampleCount; ++traceIndex) {
        const Eigen::Index sourceColumn = pending.traceSampleCount <= 1
            ? 0
            : (traceIndex * (rawBlock.cols() - 1)) / (pending.traceSampleCount - 1);
        pending.sourceColumns[static_cast<std::size_t>(traceIndex)] = sourceColumn;
        pending.raw[static_cast<std::size_t>(traceIndex)] = rawBlock(targetRow, sourceColumn);
    }
}

//=============================================================================================================

void AdaptiveDenoisingVisualizationModel::publishOutput(
    const Eigen::MatrixXd& denoisedBlock,
    const AdaptiveDenoisingDiagnostics& diagnostics) noexcept
{
    const Impl::PendingTrace& pending = m_impl->pendingTrace;
    if(!pending.valid
       || pending.selectedTargetRow < 0
       || pending.selectedTargetRow >= denoisedBlock.rows()
       || pending.sourceSampleCount != denoisedBlock.cols()) {
        return;
    }

    QMutexLocker locker(&m_impl->mutex);
    AdaptiveDenoisingVisualizationSnapshot next;
    next.sequence = m_impl->publishedSnapshot.sequence + 1u;
    next.selectedTargetOrdinal = pending.selectedTargetOrdinal;
    next.selectedTargetRow = pending.selectedTargetRow;
    next.targetCount = pending.targetCount;
    next.sourceSampleCount = pending.sourceSampleCount;
    next.traceSampleCount = pending.traceSampleCount;

    for(Eigen::Index traceIndex = 0; traceIndex < pending.traceSampleCount; ++traceIndex) {
        const std::size_t destinationIndex = static_cast<std::size_t>(traceIndex);
        const Eigen::Index sourceColumn = pending.sourceColumns[destinationIndex];
        const double raw = pending.raw[destinationIndex];
        const double denoised = denoisedBlock(pending.selectedTargetRow, sourceColumn);
        next.raw[destinationIndex] = raw;
        next.denoised[destinationIndex] = denoised;
        next.estimatedNoise[destinationIndex] = raw - denoised;
    }

    m_impl->appendRms(diagnostics);
    m_impl->copyChronologicalRms(next);
    m_impl->publishedSnapshot = next;
}

//=============================================================================================================

AdaptiveDenoisingVisualizationSnapshot AdaptiveDenoisingVisualizationModel::snapshot() const noexcept
{
    QMutexLocker locker(&m_impl->mutex);
    return m_impl->publishedSnapshot;
}

//=============================================================================================================

void AdaptiveDenoisingVisualizationModel::clear() noexcept
{
    QMutexLocker locker(&m_impl->mutex);
    m_impl->pendingTrace = Impl::PendingTrace{};
    m_impl->publishedSnapshot = AdaptiveDenoisingVisualizationSnapshot{};
    m_impl->inputRmsRing.fill(0.0);
    m_impl->outputRmsRing.fill(0.0);
    m_impl->estimatedNoiseRmsRing.fill(0.0);
    m_impl->rmsWriteIndex = 0u;
    m_impl->rmsCount = 0u;
}


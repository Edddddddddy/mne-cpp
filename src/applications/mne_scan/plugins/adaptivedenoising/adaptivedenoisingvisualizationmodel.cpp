//=============================================================================================================
/**
 * @file     adaptivedenoisingvisualizationmodel.cpp
 * @brief    Fixed worker-to-GUI visualization snapshot implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingvisualizationmodel.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <type_traits>

//=============================================================================================================

using namespace ADAPTIVEDENOISINGPLUGIN;

//=============================================================================================================

namespace
{

using MailboxCounter = unsigned int;
using MailboxEpoch = unsigned int;

constexpr std::size_t kVisualizationMailboxCapacity = 4u;
constexpr MailboxCounter kHalfMailboxCounterRange =
    std::numeric_limits<MailboxCounter>::max() / 2u + 1u;

static_assert(ATOMIC_INT_LOCK_FREE == 2,
              "Adaptive Denoising visualization requires always-lock-free int atomics.");
static_assert(std::numeric_limits<MailboxCounter>::is_modulo,
              "Adaptive Denoising visualization mailbox counters require unsigned modulo arithmetic.");
static_assert(kVisualizationMailboxCapacity < kHalfMailboxCounterRange,
              "Adaptive Denoising visualization mailbox capacity must be below half the counter range.");
static_assert(std::is_nothrow_copy_constructible<AdaptiveDenoisingVisualizationSnapshot>::value,
              "Adaptive Denoising visualization snapshots must be nothrow copy constructible.");
static_assert(std::is_nothrow_copy_assignable<AdaptiveDenoisingVisualizationSnapshot>::value,
              "Adaptive Denoising visualization snapshots must be nothrow copy assignable.");

MailboxEpoch nextNonZeroEpoch(MailboxEpoch epoch) noexcept
{
    ++epoch;
    if(epoch == 0u) {
        ++epoch;
    }
    return epoch;
}

std::uint64_t nextNonZeroSnapshotSequence(std::uint64_t sequence) noexcept
{
    ++sequence;
    if(sequence == 0u) {
        ++sequence;
    }
    return sequence;
}

} // NAMESPACE

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
        Eigen::Index sourceRowCount = 0;
        Eigen::Index sourceSampleCount = 0;
        Eigen::Index traceSampleCount = 0;
        std::array<Eigen::Index, kAdaptiveDenoisingTraceCapacity> sourceColumns{};
        std::array<double, kAdaptiveDenoisingTraceCapacity> raw{};
    };

    struct MailboxSlot
    {
        MailboxEpoch epoch = 0u;
        AdaptiveDenoisingVisualizationSnapshot value;
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

    void invalidate() noexcept
    {
        // Zero is exclusively the public-invalid marker. Publish it before
        // resetting worker-only staging so a racing GUI snapshot rejects its
        // cache and every queued slot from the preceding epoch.
        visibleEpoch.store(0u, std::memory_order_release);
        pendingTrace = PendingTrace{};
        rmsWriteIndex = 0u;
        rmsCount = 0u;
        publicSequence = 0u;

        // Repeated clears while already invalid do not consume epochs. Every
        // epoch advance is therefore paired with a successful publication,
        // which also prevents an unread four-slot backlog from being mistaken
        // for the current epoch when this unsigned counter wraps.
        if(epochHasPublication) {
            workerEpoch = nextNonZeroEpoch(workerEpoch);
            epochHasPublication = false;
        }
    }

    std::atomic<int> selectedTarget{0};
    PendingTrace pendingTrace;
    std::array<MailboxSlot, kVisualizationMailboxCapacity> mailbox{};

    // The worker is the sole writer of producerPosition/publishedPosition; the
    // GUI is the sole writer of consumerPosition/reusablePosition. Their
    // unsigned differences are always in [0, capacity], including at wrap.
    MailboxCounter producerPosition = 0u;
    std::atomic<MailboxCounter> publishedPosition{0u};
    mutable MailboxCounter consumerPosition = 0u;
    mutable std::atomic<MailboxCounter> reusablePosition{0u};

    MailboxEpoch workerEpoch = 1u;
    bool epochHasPublication = false;
    std::atomic<MailboxEpoch> visibleEpoch{0u};
    std::uint64_t publicSequence = 0u;

    // Only the GUI thread reads or writes this cache. Its epoch tag makes the
    // retained bytes harmless across clear and across rejected old slots.
    mutable AdaptiveDenoisingVisualizationSnapshot latestCache;
    mutable MailboxEpoch latestCacheEpoch = 0u;

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

    if(rawBlock.rows() <= 0
       || rawBlock.cols() <= 0
       || targetRows.empty()
       || targetRows.size()
              > static_cast<std::size_t>(std::numeric_limits<Eigen::Index>::max())) {
        m_impl->invalidate();
        return;
    }

    for(const Eigen::Index targetRow : targetRows) {
        if(targetRow < 0 || targetRow >= rawBlock.rows()) {
            m_impl->invalidate();
            return;
        }
    }

    const Eigen::Index targetCount = static_cast<Eigen::Index>(targetRows.size());
    const Eigen::Index requestedOrdinal = static_cast<Eigen::Index>(selectedTargetOrdinal());
    const Eigen::Index targetOrdinal = std::min(requestedOrdinal, targetCount - 1);
    const Eigen::Index targetRow = targetRows[static_cast<std::size_t>(targetOrdinal)];

    pending.valid = true;
    pending.selectedTargetOrdinal = targetOrdinal;
    pending.selectedTargetRow = targetRow;
    pending.targetCount = targetCount;
    pending.sourceRowCount = rawBlock.rows();
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
    Impl::PendingTrace& pending = m_impl->pendingTrace;
    if(!pending.valid
       || pending.selectedTargetRow < 0
       || pending.sourceRowCount != denoisedBlock.rows()
       || pending.sourceSampleCount != denoisedBlock.cols()) {
        m_impl->invalidate();
        return;
    }

    // History describes every finite processed block, including a visualization
    // update dropped because all fixed mailbox slots are still unread.
    m_impl->appendRms(diagnostics);
    pending.valid = false;

    const MailboxCounter reusable =
        m_impl->reusablePosition.load(std::memory_order_acquire);
    const MailboxCounter unread = m_impl->producerPosition - reusable;
    if(unread >= static_cast<MailboxCounter>(kVisualizationMailboxCapacity)) {
        return;
    }

    Impl::MailboxSlot& slot = m_impl->mailbox[
        static_cast<std::size_t>(m_impl->producerPosition)
        % kVisualizationMailboxCapacity];
    AdaptiveDenoisingVisualizationSnapshot& next = slot.value;
    next = AdaptiveDenoisingVisualizationSnapshot{};
    next.sequence = nextNonZeroSnapshotSequence(m_impl->publicSequence);
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

    m_impl->copyChronologicalRms(next);
    slot.epoch = m_impl->workerEpoch;

    m_impl->publicSequence = next.sequence;
    ++m_impl->producerPosition;
    m_impl->publishedPosition.store(m_impl->producerPosition,
                                    std::memory_order_release);

    if(!m_impl->epochHasPublication) {
        // Revalidation follows release-publication of the first complete slot
        // in this epoch. A full mailbox returns above and stays invalid.
        m_impl->epochHasPublication = true;
        m_impl->visibleEpoch.store(m_impl->workerEpoch,
                                   std::memory_order_release);
    }
}

//=============================================================================================================

AdaptiveDenoisingVisualizationSnapshot AdaptiveDenoisingVisualizationModel::snapshot() const noexcept
{
    // Reading validity first makes an observed nonzero epoch acquire the first
    // post-clear slot publication. A final acquire rejects a clear/revalidate
    // race while the fixed value is copied.
    const MailboxEpoch epochBefore =
        m_impl->visibleEpoch.load(std::memory_order_acquire);
    const MailboxCounter published =
        m_impl->publishedPosition.load(std::memory_order_acquire);
    const MailboxCounter available = published - m_impl->consumerPosition;
    const std::size_t drainCount = std::min<std::size_t>(
        static_cast<std::size_t>(available),
        kVisualizationMailboxCapacity);

    for(std::size_t drained = 0u; drained < drainCount; ++drained) {
        const MailboxCounter position = m_impl->consumerPosition;
        const Impl::MailboxSlot& slot = m_impl->mailbox[
            static_cast<std::size_t>(position) % kVisualizationMailboxCapacity];

        // Complete the value and epoch copies before release-reusing the slot;
        // this function never touches the slot again afterward.
        m_impl->latestCache = slot.value;
        m_impl->latestCacheEpoch = slot.epoch;
        m_impl->consumerPosition = position + 1u;
        m_impl->reusablePosition.store(m_impl->consumerPosition,
                                       std::memory_order_release);
    }

    const AdaptiveDenoisingVisualizationSnapshot result = m_impl->latestCache;
    const MailboxEpoch epochAfter =
        m_impl->visibleEpoch.load(std::memory_order_acquire);
    if(epochBefore == 0u
       || epochBefore != epochAfter
       || m_impl->latestCacheEpoch != epochAfter) {
        return AdaptiveDenoisingVisualizationSnapshot{};
    }

    return result;
}

//=============================================================================================================

void AdaptiveDenoisingVisualizationModel::clear() noexcept
{
    m_impl->invalidate();
}


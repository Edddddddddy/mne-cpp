//=============================================================================================================
/**
 * @file     test_adaptive_denoising_ui.cpp
 * @brief    Public-interface RED tracer for the adaptive denoising setup widget.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <adaptivedenoising/adaptivedenoisingdiagnostics.h>
#include <adaptivedenoising/adaptivedenoisingsetupwidget.h>
#include <adaptivedenoising/adaptivedenoisingvisualizationmodel.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

#include <Eigen/Core>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

//=============================================================================================================

namespace
{

thread_local bool g_countWorkerAllocations = false;
thread_local std::uint64_t g_workerAllocationCount = 0u;

void* allocateTestMemory(std::size_t size, bool throwing)
{
    void* const memory = std::malloc(size == 0u ? 1u : size);
    if(memory == nullptr) {
        if(throwing) {
            throw std::bad_alloc();
        }
        return nullptr;
    }

    if(g_countWorkerAllocations) {
        ++g_workerAllocationCount;
    }
    return memory;
}

} // namespace

void* operator new(std::size_t size)
{
    return allocateTestMemory(size, true);
}

void* operator new[](std::size_t size)
{
    return allocateTestMemory(size, true);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    return allocateTestMemory(size, false);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    return allocateTestMemory(size, false);
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

//=============================================================================================================

using namespace ADAPTIVEDENOISINGPLUGIN;

namespace
{

using Diagnostics = AdaptiveDenoisingDiagnostics;

static_assert(std::is_copy_constructible<Diagnostics>::value,
              "diagnostics must be copy constructible for queued delivery");
static_assert(std::is_copy_assignable<Diagnostics>::value,
              "diagnostics must be copy assignable for queued delivery");
static_assert(std::is_standard_layout<Diagnostics>::value,
              "diagnostics must remain a fixed standard-layout value");
static_assert(std::is_trivially_copyable<Diagnostics>::value,
              "diagnostics must contain only trivially copyable data");
static_assert(
    std::is_scalar<decltype(std::declval<Diagnostics>().pluginState)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().configureStatus)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().processStatus)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().referenceRowCount)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().targetRowCount)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().featureCount)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().warmupSamplesRemaining)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().modelGeneration)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().modelUpdatesAccepted)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().modelUpdatesRejected)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().inputRms)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().outputRms)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().estimatedNoiseRms)>::value
        && std::is_scalar<decltype(std::declval<Diagnostics>().droppedBlocks)>::value,
    "diagnostics must not contain strings, containers or owning pointers");

constexpr std::size_t kConcurrentVisualizationBlockCount = 4u;
constexpr Eigen::Index kConcurrentVisualizationSampleCount = 64;

struct PrecreatedVisualizationBlock
{
    Eigen::MatrixXd raw;
    Eigen::MatrixXd denoised;
    std::vector<Eigen::Index> targetRows;
    Diagnostics diagnostics{};
};

bool snapshotMatchesVisualizationBlock(
    const AdaptiveDenoisingVisualizationSnapshot& snapshot,
    const PrecreatedVisualizationBlock& block) noexcept
{
    if(snapshot.sequence == 0u
       || snapshot.selectedTargetOrdinal != 1
       || snapshot.selectedTargetRow != 1
       || snapshot.targetCount != 2
       || snapshot.sourceSampleCount != kConcurrentVisualizationSampleCount
       || snapshot.traceSampleCount != kConcurrentVisualizationSampleCount
       || snapshot.rmsHistoryCount <= 0
       || snapshot.rmsHistoryCount
              > static_cast<Eigen::Index>(kAdaptiveDenoisingRmsHistoryCapacity)) {
        return false;
    }

    for(Eigen::Index traceIndex = 0;
        traceIndex < snapshot.traceSampleCount;
        ++traceIndex) {
        const Eigen::Index sourceColumn =
            (traceIndex * (block.raw.cols() - 1))
            / (snapshot.traceSampleCount - 1);
        const std::size_t destinationIndex = static_cast<std::size_t>(traceIndex);
        const double expectedRaw = block.raw(1, sourceColumn);
        const double expectedDenoised = block.denoised(1, sourceColumn);
        if(snapshot.raw[destinationIndex] != expectedRaw
           || snapshot.denoised[destinationIndex] != expectedDenoised
           || snapshot.estimatedNoise[destinationIndex]
                  != expectedRaw - expectedDenoised) {
            return false;
        }
    }

    return true;
}

bool snapshotMatchesAnyVisualizationBlock(
    const AdaptiveDenoisingVisualizationSnapshot& snapshot,
    const std::array<PrecreatedVisualizationBlock,
                     kConcurrentVisualizationBlockCount>& blocks) noexcept
{
    const auto historyBlockIndex =
        [&blocks](const AdaptiveDenoisingVisualizationSnapshot& snapshot,
                  std::size_t historyIndex) {
            for(std::size_t blockIndex = 0u;
                blockIndex < kConcurrentVisualizationBlockCount;
                ++blockIndex) {
                const PrecreatedVisualizationBlock& block = blocks[blockIndex];
                if(std::isfinite(snapshot.inputRmsHistory[historyIndex])
                   && std::isfinite(snapshot.outputRmsHistory[historyIndex])
                   && std::isfinite(snapshot.estimatedNoiseRmsHistory[historyIndex])
                   && snapshot.inputRmsHistory[historyIndex]
                          == block.diagnostics.inputRms
                   && snapshot.outputRmsHistory[historyIndex]
                          == block.diagnostics.outputRms
                   && snapshot.estimatedNoiseRmsHistory[historyIndex]
                          == block.diagnostics.estimatedNoiseRms) {
                    return blockIndex;
                }
            }
            return kConcurrentVisualizationBlockCount;
        };

    if(snapshot.rmsHistoryCount <= 0
       || snapshot.rmsHistoryCount
              > static_cast<Eigen::Index>(kAdaptiveDenoisingRmsHistoryCapacity)) {
        return false;
    }

    std::array<std::size_t, kAdaptiveDenoisingRmsHistoryCapacity> historyBlocks{};
    for(std::size_t historyIndex = 0u;
        historyIndex < static_cast<std::size_t>(snapshot.rmsHistoryCount);
        ++historyIndex) {
        historyBlocks[historyIndex] = historyBlockIndex(snapshot, historyIndex);
        if(historyBlocks[historyIndex] >= kConcurrentVisualizationBlockCount) {
            return false;
        }
        if(historyIndex > 0u
           && historyBlocks[historyIndex]
                  != (historyBlocks[historyIndex - 1u] + 1u)
                         % kConcurrentVisualizationBlockCount) {
            return false;
        }
    }

    for(std::size_t blockIndex = 0u;
        blockIndex < kConcurrentVisualizationBlockCount;
        ++blockIndex) {
        if(snapshotMatchesVisualizationBlock(snapshot, blocks[blockIndex])
           && historyBlocks[static_cast<std::size_t>(snapshot.rmsHistoryCount) - 1u]
                  == blockIndex) {
            return true;
        }
    }
    return false;
}

} // namespace

//=============================================================================================================

class TestAdaptiveDenoisingUi : public QObject
{
    Q_OBJECT

private slots:
    void widgetControlsAndDiagnosticsExposePublicContract();
    void visualizationSnapshotIsBoundedAndChronological();
    void visualizationSnapshotInvalidatesStaleState();
    void visualizationSnapshotRemainsConsistentDuringConcurrentTraffic();
    void widgetTimerRendersSelectedTargetSnapshot();
};

//=============================================================================================================

void TestAdaptiveDenoisingUi::widgetControlsAndDiagnosticsExposePublicContract()
{
    AdaptiveDenoisingSetupWidget widget;

    QCheckBox* const enabled = widget.findChild<QCheckBox*>(QStringLiteral("enabledCheckBox"));
    QSpinBox* const taps = widget.findChild<QSpinBox*>(QStringLiteral("tapCountSpinBox"));
    QSpinBox* const interval =
        widget.findChild<QSpinBox*>(QStringLiteral("adaptationIntervalSpinBox"));
    QDoubleSpinBox* const memory =
        widget.findChild<QDoubleSpinBox*>(QStringLiteral("memoryTimeSecondsSpinBox"));
    QDoubleSpinBox* const regularization =
        widget.findChild<QDoubleSpinBox*>(QStringLiteral("regularizationSpinBox"));
    QCheckBox* const frozen = widget.findChild<QCheckBox*>(QStringLiteral("frozenCheckBox"));
    QPushButton* const reset = widget.findChild<QPushButton*>(QStringLiteral("resetButton"));

    QVERIFY(enabled != nullptr);
    QVERIFY(taps != nullptr);
    QVERIFY(interval != nullptr);
    QVERIFY(memory != nullptr);
    QVERIFY(regularization != nullptr);
    QVERIFY(frozen != nullptr);
    QVERIFY(reset != nullptr);

    QCOMPARE(enabled->isChecked(), true);
    QCOMPARE(taps->minimum(), 1);
    QCOMPARE(taps->maximum(), 32);
    QCOMPARE(taps->value(), 4);
    QCOMPARE(interval->minimum(), 16);
    QCOMPARE(interval->maximum(), 2048);
    QCOMPARE(interval->value(), 128);
    QCOMPARE(memory->minimum(), 1.0);
    QCOMPARE(memory->maximum(), 300.0);
    QCOMPARE(memory->value(), 30.0);
    QCOMPARE(regularization->minimum(), 1e-8);
    QCOMPARE(regularization->maximum(), 1.0);
    QCOMPARE(regularization->value(), 1e-3);
    QCOMPARE(frozen->isChecked(), false);

    QSignalSpy enabledSpy(&widget, &AdaptiveDenoisingSetupWidget::enabledChanged);
    QSignalSpy frozenSpy(&widget, &AdaptiveDenoisingSetupWidget::frozenChanged);
    QSignalSpy tapsSpy(&widget, &AdaptiveDenoisingSetupWidget::tapCountChanged);
    QSignalSpy intervalSpy(&widget, &AdaptiveDenoisingSetupWidget::adaptationIntervalChanged);
    QSignalSpy memorySpy(&widget, &AdaptiveDenoisingSetupWidget::memoryTimeSecondsChanged);
    QSignalSpy regularizationSpy(&widget, &AdaptiveDenoisingSetupWidget::regularizationChanged);
    QSignalSpy resetSpy(&widget, &AdaptiveDenoisingSetupWidget::resetRequested);

    enabled->setChecked(false);
    QCOMPARE(enabledSpy.count(), 1);
    QCOMPARE(enabledSpy.at(0).at(0).toBool(), false);

    frozen->setChecked(true);
    QCOMPARE(frozenSpy.count(), 1);
    QCOMPARE(frozenSpy.at(0).at(0).toBool(), true);

    taps->setValue(7);
    QCOMPARE(tapsSpy.count(), 1);
    QCOMPARE(tapsSpy.at(0).at(0).toInt(), 7);

    interval->setValue(256);
    QCOMPARE(intervalSpy.count(), 1);
    QCOMPARE(intervalSpy.at(0).at(0).toInt(), 256);

    memory->setValue(45.0);
    QCOMPARE(memorySpy.count(), 1);
    QCOMPARE(memorySpy.at(0).at(0).toDouble(), 45.0);

    regularization->setValue(0.01);
    QCOMPARE(regularizationSpy.count(), 1);
    QCOMPARE(regularizationSpy.at(0).at(0).toDouble(), 0.01);

    reset->click();
    QCOMPARE(resetSpy.count(), 1);

    const int diagnosticsTypeId = qRegisterMetaType<Diagnostics>();
    QVERIFY(diagnosticsTypeId != QMetaType::UnknownType);

    Diagnostics diagnostics{};
    diagnostics.pluginState = AdaptiveDenoisingPluginState::Processing;
    diagnostics.configureStatus = AdaptiveDenoisingConfigureStatus::Ready;
    diagnostics.processStatus = RTPROCESSINGLIB::DenoiserProcessStatus::Processed;
    diagnostics.referenceRowCount = 2;
    diagnostics.targetRowCount = 3;
    diagnostics.featureCount = 8;
    diagnostics.warmupSamplesRemaining = 7;
    diagnostics.modelGeneration = 11;
    diagnostics.modelUpdatesAccepted = 13;
    diagnostics.modelUpdatesRejected = 2;
    diagnostics.inputRms = 1.25;
    diagnostics.outputRms = 0.75;
    diagnostics.estimatedNoiseRms = 0.5;
    diagnostics.droppedBlocks = 4;

    widget.setDiagnostics(diagnostics);

    const auto valueLabel = [&widget](const QString& name) {
        return widget.findChild<QLabel*>(name);
    };

    QVERIFY(valueLabel(QStringLiteral("pluginStateValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("configureStatusValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("processStatusValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("referenceCountValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("targetCountValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("featureCountValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("warmupSamplesValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("modelGenerationValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("acceptedUpdatesValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("rejectedUpdatesValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("inputRmsValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("outputRmsValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("estimatedNoiseRmsValue")) != nullptr);
    QVERIFY(valueLabel(QStringLiteral("droppedBlocksValue")) != nullptr);

    QCOMPARE(valueLabel(QStringLiteral("pluginStateValue"))->text(), QStringLiteral("Processing"));
    QCOMPARE(valueLabel(QStringLiteral("configureStatusValue"))->text(), QStringLiteral("Ready"));
    QCOMPARE(valueLabel(QStringLiteral("processStatusValue"))->text(), QStringLiteral("Processed"));
    QCOMPARE(valueLabel(QStringLiteral("referenceCountValue"))->text(), QStringLiteral("2"));
    QCOMPARE(valueLabel(QStringLiteral("targetCountValue"))->text(), QStringLiteral("3"));
    QCOMPARE(valueLabel(QStringLiteral("featureCountValue"))->text(), QStringLiteral("8"));
    QCOMPARE(valueLabel(QStringLiteral("warmupSamplesValue"))->text(), QStringLiteral("7"));
    QCOMPARE(valueLabel(QStringLiteral("modelGenerationValue"))->text(), QStringLiteral("11"));
    QCOMPARE(valueLabel(QStringLiteral("acceptedUpdatesValue"))->text(), QStringLiteral("13"));
    QCOMPARE(valueLabel(QStringLiteral("rejectedUpdatesValue"))->text(), QStringLiteral("2"));
    QCOMPARE(valueLabel(QStringLiteral("inputRmsValue"))->text(), QStringLiteral("1.25"));
    QCOMPARE(valueLabel(QStringLiteral("outputRmsValue"))->text(), QStringLiteral("0.75"));
    QCOMPARE(valueLabel(QStringLiteral("estimatedNoiseRmsValue"))->text(), QStringLiteral("0.5"));
    QCOMPARE(valueLabel(QStringLiteral("droppedBlocksValue"))->text(), QStringLiteral("4"));
}

//=============================================================================================================

void TestAdaptiveDenoisingUi::visualizationSnapshotIsBoundedAndChronological()
{
    AdaptiveDenoisingVisualizationModel model;
    model.setSelectedTargetOrdinal(1);

    Eigen::MatrixXd raw(2, 300);
    Eigen::MatrixXd denoised(2, 300);
    for(Eigen::Index sample = 0; sample < raw.cols(); ++sample) {
        raw(0, sample) = -1000.0 - static_cast<double>(sample);
        raw(1, sample) = 1000.0 + static_cast<double>(sample);
        denoised(0, sample) = raw(0, sample);
        denoised(1, sample) = 250.0 + 0.25 * static_cast<double>(sample);
    }

    Diagnostics diagnostics{};
    diagnostics.inputRms = 10.0;
    diagnostics.outputRms = 4.0;
    diagnostics.estimatedNoiseRms = 6.0;

    const std::vector<Eigen::Index> targetRows{0, 1};
    model.captureInput(raw, targetRows);
    model.publishOutput(denoised, diagnostics);

    AdaptiveDenoisingVisualizationSnapshot snapshot = model.snapshot();
    QCOMPARE(snapshot.sequence, std::uint64_t(1));
    QCOMPARE(snapshot.selectedTargetOrdinal, Eigen::Index(1));
    QCOMPARE(snapshot.selectedTargetRow, Eigen::Index(1));
    QCOMPARE(snapshot.targetCount, Eigen::Index(2));
    QCOMPARE(snapshot.sourceSampleCount, Eigen::Index(300));
    QCOMPARE(snapshot.traceSampleCount, Eigen::Index(kAdaptiveDenoisingTraceCapacity));
    for(std::size_t traceIndex = 0u;
        traceIndex < kAdaptiveDenoisingTraceCapacity;
        ++traceIndex) {
        const Eigen::Index sourceColumn =
            (static_cast<Eigen::Index>(traceIndex) * (raw.cols() - 1))
            / static_cast<Eigen::Index>(kAdaptiveDenoisingTraceCapacity - 1u);
        QCOMPARE(snapshot.raw[traceIndex], raw(1, sourceColumn));
        QCOMPARE(snapshot.denoised[traceIndex], denoised(1, sourceColumn));
        QCOMPARE(snapshot.estimatedNoise[traceIndex],
                 raw(1, sourceColumn) - denoised(1, sourceColumn));
    }
    QCOMPARE(snapshot.rmsHistoryCount, Eigen::Index(1));
    QCOMPARE(snapshot.inputRmsHistory[0], 10.0);
    QCOMPARE(snapshot.outputRmsHistory[0], 4.0);
    QCOMPARE(snapshot.estimatedNoiseRmsHistory[0], 6.0);

    const AdaptiveDenoisingVisualizationSnapshot beforeNonFinite = snapshot;
    diagnostics.inputRms = std::numeric_limits<double>::quiet_NaN();
    diagnostics.outputRms = 404.0;
    diagnostics.estimatedNoiseRms = 505.0;
    model.captureInput(raw, targetRows);
    model.publishOutput(denoised, diagnostics);

    snapshot = model.snapshot();
    QCOMPARE(snapshot.rmsHistoryCount, beforeNonFinite.rmsHistoryCount);
    for(std::size_t historyIndex = 0u;
        historyIndex < kAdaptiveDenoisingRmsHistoryCapacity;
        ++historyIndex) {
        QCOMPARE(snapshot.inputRmsHistory[historyIndex],
                 beforeNonFinite.inputRmsHistory[historyIndex]);
        QCOMPARE(snapshot.outputRmsHistory[historyIndex],
                 beforeNonFinite.outputRmsHistory[historyIndex]);
        QCOMPARE(snapshot.estimatedNoiseRmsHistory[historyIndex],
                 beforeNonFinite.estimatedNoiseRmsHistory[historyIndex]);
    }

    raw.conservativeResize(Eigen::NoChange, 1);
    denoised.conservativeResize(Eigen::NoChange, 1);
    for(int observation = 1;
        observation <= static_cast<int>(kAdaptiveDenoisingRmsHistoryCapacity) + 5;
        ++observation) {
        diagnostics.inputRms = 1000.0 + static_cast<double>(observation);
        diagnostics.outputRms = 2000.0 + static_cast<double>(observation);
        diagnostics.estimatedNoiseRms = 3000.0 + static_cast<double>(observation);
        model.captureInput(raw, targetRows);
        model.publishOutput(denoised, diagnostics);
    }

    snapshot = model.snapshot();
    QCOMPARE(snapshot.rmsHistoryCount,
             Eigen::Index(kAdaptiveDenoisingRmsHistoryCapacity));
    for(std::size_t historyIndex = 0u;
        historyIndex < kAdaptiveDenoisingRmsHistoryCapacity;
        ++historyIndex) {
        const double observation = 6.0 + static_cast<double>(historyIndex);
        QCOMPARE(snapshot.inputRmsHistory[historyIndex], 1000.0 + observation);
        QCOMPARE(snapshot.outputRmsHistory[historyIndex], 2000.0 + observation);
        QCOMPARE(snapshot.estimatedNoiseRmsHistory[historyIndex],
                 3000.0 + observation);
        QVERIFY(std::isfinite(snapshot.inputRmsHistory[historyIndex]));
        QVERIFY(std::isfinite(snapshot.outputRmsHistory[historyIndex]));
        QVERIFY(std::isfinite(snapshot.estimatedNoiseRmsHistory[historyIndex]));
    }

    model.clear();
    snapshot = model.snapshot();
    QCOMPARE(snapshot.sequence, std::uint64_t(0u));
    QCOMPARE(snapshot.selectedTargetOrdinal, Eigen::Index(0));
    QCOMPARE(snapshot.selectedTargetRow, Eigen::Index(-1));
    QCOMPARE(snapshot.targetCount, Eigen::Index(0));
    QCOMPARE(snapshot.sourceSampleCount, Eigen::Index(0));
    QCOMPARE(snapshot.traceSampleCount, Eigen::Index(0));
    QCOMPARE(snapshot.rmsHistoryCount, Eigen::Index(0));
    for(std::size_t traceIndex = 0u;
        traceIndex < kAdaptiveDenoisingTraceCapacity;
        ++traceIndex) {
        QCOMPARE(snapshot.raw[traceIndex], 0.0);
        QCOMPARE(snapshot.denoised[traceIndex], 0.0);
        QCOMPARE(snapshot.estimatedNoise[traceIndex], 0.0);
    }
    for(std::size_t historyIndex = 0u;
        historyIndex < kAdaptiveDenoisingRmsHistoryCapacity;
        ++historyIndex) {
        QCOMPARE(snapshot.inputRmsHistory[historyIndex], 0.0);
        QCOMPARE(snapshot.outputRmsHistory[historyIndex], 0.0);
        QCOMPARE(snapshot.estimatedNoiseRmsHistory[historyIndex], 0.0);
    }
}

//=============================================================================================================

void TestAdaptiveDenoisingUi::visualizationSnapshotInvalidatesStaleState()
{
    const auto isEmptySnapshot = [](const AdaptiveDenoisingVisualizationSnapshot& snapshot) {
        if(snapshot.sequence != 0u
           || snapshot.selectedTargetOrdinal != 0
           || snapshot.selectedTargetRow != -1
           || snapshot.targetCount != 0
           || snapshot.sourceSampleCount != 0
           || snapshot.traceSampleCount != 0
           || snapshot.rmsHistoryCount != 0) {
            return false;
        }

        for(std::size_t index = 0u; index < kAdaptiveDenoisingTraceCapacity; ++index) {
            if(snapshot.raw[index] != 0.0
               || snapshot.denoised[index] != 0.0
               || snapshot.estimatedNoise[index] != 0.0) {
                return false;
            }
        }
        for(std::size_t index = 0u; index < kAdaptiveDenoisingRmsHistoryCapacity; ++index) {
            if(snapshot.inputRmsHistory[index] != 0.0
               || snapshot.outputRmsHistory[index] != 0.0
               || snapshot.estimatedNoiseRmsHistory[index] != 0.0) {
                return false;
            }
        }
        return true;
    };

    AdaptiveDenoisingVisualizationModel model;
    model.setSelectedTargetOrdinal(0);

    Eigen::MatrixXd raw(2, 8);
    Eigen::MatrixXd denoised(2, 8);
    for(Eigen::Index sample = 0; sample < raw.cols(); ++sample) {
        raw(0, sample) = 10.0 + static_cast<double>(sample);
        raw(1, sample) = 100.0 + static_cast<double>(sample);
        denoised(0, sample) = 7.0 + static_cast<double>(sample);
        denoised(1, sample) = 70.0 + static_cast<double>(sample);
    }

    Diagnostics diagnostics{};
    diagnostics.inputRms = 3.0;
    diagnostics.outputRms = 2.0;
    diagnostics.estimatedNoiseRms = 1.0;
    const std::vector<Eigen::Index> targetRows{1};

    model.captureInput(raw, targetRows);
    model.publishOutput(denoised, diagnostics);
    QVERIFY(model.snapshot().sequence > 0u);

    model.captureInput(raw, std::vector<Eigen::Index>());
    model.publishOutput(denoised, diagnostics);
    QVERIFY(isEmptySnapshot(model.snapshot()));

    model.clear();
    model.captureInput(raw, targetRows);
    model.publishOutput(denoised, diagnostics);
    QVERIFY(model.snapshot().sequence > 0u);

    Eigen::MatrixXd incompatibleRows(1, raw.cols());
    incompatibleRows.setZero();
    model.captureInput(raw, targetRows);
    model.publishOutput(incompatibleRows, diagnostics);
    QVERIFY(isEmptySnapshot(model.snapshot()));

    model.clear();
    model.captureInput(raw, targetRows);
    model.publishOutput(denoised, diagnostics);
    QVERIFY(model.snapshot().sequence > 0u);

    Eigen::MatrixXd incompatibleColumns(raw.rows(), raw.cols() - 1);
    incompatibleColumns.setZero();
    model.captureInput(raw, targetRows);
    model.publishOutput(incompatibleColumns, diagnostics);
    QVERIFY(isEmptySnapshot(model.snapshot()));
}

//=============================================================================================================

void TestAdaptiveDenoisingUi::visualizationSnapshotRemainsConsistentDuringConcurrentTraffic()
{
    std::array<PrecreatedVisualizationBlock,
               kConcurrentVisualizationBlockCount> blocks;
    for(std::size_t blockIndex = 0u;
        blockIndex < kConcurrentVisualizationBlockCount;
        ++blockIndex) {
        PrecreatedVisualizationBlock& block = blocks[blockIndex];
        block.raw.resize(2, kConcurrentVisualizationSampleCount);
        block.denoised.resize(2, kConcurrentVisualizationSampleCount);
        block.targetRows = std::vector<Eigen::Index>{0, 1};
        block.diagnostics.inputRms = 1000.0 + static_cast<double>(blockIndex);
        block.diagnostics.outputRms = 2000.0 + static_cast<double>(blockIndex);
        block.diagnostics.estimatedNoiseRms = 3000.0 + static_cast<double>(blockIndex);

        const double base = 10000.0 + 100.0 * static_cast<double>(blockIndex);
        const double noise = 10.0 + static_cast<double>(blockIndex);
        for(Eigen::Index sample = 0;
            sample < kConcurrentVisualizationSampleCount;
            ++sample) {
            block.raw(0, sample) = base + 0.5 * static_cast<double>(sample);
            block.raw(1, sample) = base + 1000.0 + static_cast<double>(sample);
            block.denoised(0, sample) = block.raw(0, sample) - noise;
            block.denoised(1, sample) = block.raw(1, sample) - noise;
        }
    }

    AdaptiveDenoisingVisualizationModel model;
    model.setSelectedTargetOrdinal(1);

    std::atomic<bool> start{false};
    std::atomic<bool> stop{false};
    std::atomic<bool> producerReady{false};
    std::atomic<bool> readerReady{false};
    std::atomic<bool> producerActive{false};
    std::atomic<bool> readerActive{false};
    std::atomic<bool> clearWindowRequested{false};
    std::atomic<bool> readerHoldingForClear{false};
    std::atomic<bool> workerClearCompleted{false};
    std::atomic<bool> overlapObserved{false};
    std::atomic<bool> workerClearOverlappedReader{false};
    std::atomic<bool> readerSawNonEmpty{false};
    std::atomic<bool> readerSawInvalidSnapshot{false};
    std::atomic<std::uint64_t> completeWorkerCalls{0u};
    std::atomic<std::uint64_t> workerAllocationCount{0u};
    std::atomic<std::uint64_t> maxWorkerCallNanoseconds{0u};
    std::atomic<std::uint64_t> readerIterations{0u};
    std::atomic<std::uint64_t> workerClearCalls{0u};

    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(750);

    std::thread producer([&]() {
        producerReady.store(true, std::memory_order_release);
        while(!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::uint64_t localCompleteCalls = 0u;
        std::uint64_t localMaxNanoseconds = 0u;
        std::uint64_t localClearCalls = 0u;
        std::size_t blockIndex = 0u;
        bool clearOverlapHandshakeUsed = false;
        g_workerAllocationCount = 0u;
        g_countWorkerAllocations = true;
        while(!stop.load(std::memory_order_acquire)
              && std::chrono::steady_clock::now() < deadline) {
            const std::chrono::steady_clock::time_point callStart =
                std::chrono::steady_clock::now();
            producerActive.store(true, std::memory_order_release);
            model.captureInput(blocks[blockIndex].raw, blocks[blockIndex].targetRows);
            model.publishOutput(blocks[blockIndex].denoised,
                                blocks[blockIndex].diagnostics);
            producerActive.store(false, std::memory_order_release);

            const std::uint64_t callNanoseconds = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - callStart)
                    .count());
            localMaxNanoseconds = std::max(localMaxNanoseconds, callNanoseconds);
            ++localCompleteCalls;
            blockIndex = (blockIndex + 1u) % kConcurrentVisualizationBlockCount;

            if(localCompleteCalls % 17u == 0u) {
                bool clearOverlappedReader = false;
                if(!clearOverlapHandshakeUsed) {
                    workerClearCompleted.store(false, std::memory_order_release);
                    clearWindowRequested.store(true, std::memory_order_release);
                    while(!readerHoldingForClear.load(std::memory_order_acquire)
                          && !stop.load(std::memory_order_acquire)
                          && std::chrono::steady_clock::now() < deadline) {
                        std::this_thread::yield();
                    }
                    clearOverlappedReader =
                        readerHoldingForClear.load(std::memory_order_acquire);
                }

                model.clear();
                ++localClearCalls;
                if(!clearOverlapHandshakeUsed) {
                    if(clearOverlappedReader
                       || readerActive.load(std::memory_order_acquire)) {
                        workerClearOverlappedReader.store(true,
                                                          std::memory_order_release);
                    }
                    workerClearCompleted.store(true, std::memory_order_release);
                    clearWindowRequested.store(false, std::memory_order_release);
                    clearOverlapHandshakeUsed = true;
                }
            }
        }
        producerActive.store(false, std::memory_order_release);
        g_countWorkerAllocations = false;
        completeWorkerCalls.store(localCompleteCalls, std::memory_order_release);
        workerAllocationCount.store(g_workerAllocationCount, std::memory_order_release);
        maxWorkerCallNanoseconds.store(localMaxNanoseconds, std::memory_order_release);
        workerClearCalls.store(localClearCalls, std::memory_order_release);
    });

    std::thread reader([&]() {
        readerReady.store(true, std::memory_order_release);
        while(!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::uint64_t localReaderIterations = 0u;
        while(!stop.load(std::memory_order_acquire)
              && std::chrono::steady_clock::now() < deadline) {
            readerActive.store(true, std::memory_order_release);
            const AdaptiveDenoisingVisualizationSnapshot snapshot = model.snapshot();
            if(producerActive.load(std::memory_order_acquire)) {
                overlapObserved.store(true, std::memory_order_release);
            }
            if(snapshot.traceSampleCount > 0) {
                readerSawNonEmpty.store(true, std::memory_order_release);
                if(!snapshotMatchesAnyVisualizationBlock(snapshot, blocks)) {
                    readerSawInvalidSnapshot.store(true, std::memory_order_release);
                }
            }

            if(clearWindowRequested.load(std::memory_order_acquire)) {
                readerHoldingForClear.store(true, std::memory_order_release);
                while(!workerClearCompleted.load(std::memory_order_acquire)
                      && !stop.load(std::memory_order_acquire)
                      && std::chrono::steady_clock::now() < deadline) {
                    std::this_thread::yield();
                }
                readerHoldingForClear.store(false, std::memory_order_release);
            }
            readerActive.store(false, std::memory_order_release);

            ++localReaderIterations;
        }
        readerIterations.store(localReaderIterations, std::memory_order_release);
        stop.store(true, std::memory_order_release);
    });

    const std::chrono::steady_clock::time_point readyDeadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    while((!producerReady.load(std::memory_order_acquire)
           || !readerReady.load(std::memory_order_acquire))
          && std::chrono::steady_clock::now() < readyDeadline) {
        std::this_thread::yield();
    }
    start.store(true, std::memory_order_release);

    producer.join();
    reader.join();

    const std::uint64_t completeCalls =
        completeWorkerCalls.load(std::memory_order_acquire);
    const std::uint64_t allocations =
        workerAllocationCount.load(std::memory_order_acquire);
    const std::uint64_t maxNanoseconds =
        maxWorkerCallNanoseconds.load(std::memory_order_acquire);
    const std::uint64_t observedReaderIterations =
        readerIterations.load(std::memory_order_acquire);
    const std::uint64_t observedWorkerClearCalls =
        workerClearCalls.load(std::memory_order_acquire);

    std::fprintf(stdout,
                 "\n[Adaptive Denoising] concurrent worker calls=%llu, "
                 "reader snapshots=%llu, worker clears=%llu, "
                 "max complete worker call=%llu ns, "
                 "worker allocations=%llu\n",
                 static_cast<unsigned long long>(completeCalls),
                 static_cast<unsigned long long>(observedReaderIterations),
                 static_cast<unsigned long long>(observedWorkerClearCalls),
                 static_cast<unsigned long long>(maxNanoseconds),
                 static_cast<unsigned long long>(allocations));
    std::fflush(stdout);

    QVERIFY(completeCalls > 0u);
    QVERIFY(observedReaderIterations > 0u);
    QVERIFY(observedWorkerClearCalls > 0u);
    QVERIFY(overlapObserved.load(std::memory_order_acquire));
    QVERIFY(workerClearOverlappedReader.load(std::memory_order_acquire));
    QVERIFY(readerSawNonEmpty.load(std::memory_order_acquire));
    QVERIFY(!readerSawInvalidSnapshot.load(std::memory_order_acquire));
    QCOMPARE(allocations, std::uint64_t(0u));
}

//=============================================================================================================

void TestAdaptiveDenoisingUi::widgetTimerRendersSelectedTargetSnapshot()
{
    const std::shared_ptr<AdaptiveDenoisingVisualizationModel> model =
        std::make_shared<AdaptiveDenoisingVisualizationModel>();
    AdaptiveDenoisingSetupWidget widget(model);
    widget.resize(1000, 760);

    QSpinBox* const targetSelector =
        widget.findChild<QSpinBox*>(QStringLiteral("visualizationTargetSpinBox"));
    QWidget* const traceWidget =
        widget.findChild<QWidget*>(QStringLiteral("visualizationTraceWidget"));
    QLabel* const targetValue =
        widget.findChild<QLabel*>(QStringLiteral("visualizationTargetValue"));
    QLabel* const samplesValue =
        widget.findChild<QLabel*>(QStringLiteral("visualizationSamplesValue"));
    QLabel* const sequenceValue =
        widget.findChild<QLabel*>(QStringLiteral("visualizationSequenceValue"));
    QTimer* const refreshTimer =
        widget.findChild<QTimer*>(QStringLiteral("visualizationRefreshTimer"));

    QVERIFY(targetSelector != nullptr);
    QVERIFY(traceWidget != nullptr);
    QVERIFY(targetValue != nullptr);
    QVERIFY(samplesValue != nullptr);
    QVERIFY(sequenceValue != nullptr);
    QVERIFY(refreshTimer != nullptr);
    QCOMPARE(refreshTimer->interval(), 50);
    QVERIFY(refreshTimer->isActive());

    widget.show();
    QApplication::processEvents();
    QVERIFY(traceWidget->width() > 0);
    QVERIFY(traceWidget->height() > 0);

    const QRectF content = QRectF(traceWidget->rect()).adjusted(12.0, 12.0, -12.0, -12.0);
    const double gap = 12.0;
    const double waveformHeight = (content.height() - gap) * 0.62;
    const QRectF waveformPanel(content.left(), content.top(),
                               content.width(), waveformHeight);
    const QRectF rmsPanel(content.left(), waveformPanel.bottom() + gap,
                          content.width(), content.bottom() - waveformPanel.bottom() - gap);
    const QRect waveformPlot = waveformPanel.adjusted(10.0, 28.0, -10.0, -10.0)
                                   .toAlignedRect();
    const QRect rmsPlot = rmsPanel.adjusted(10.0, 26.0, -10.0, -10.0).toAlignedRect();

    const auto exactColorCount = [](const QImage& image,
                                    const QRect& region,
                                    const QColor& color) {
        const QRect clipped = region.intersected(image.rect());
        int count = 0;
        for(int y = clipped.top(); y <= clipped.bottom(); ++y) {
            for(int x = clipped.left(); x <= clipped.right(); ++x) {
                count += QColor::fromRgba(image.pixel(x, y)) == color ? 1 : 0;
            }
        }
        return count;
    };

    const QColor cyan(QStringLiteral("#4fc3f7"));
    const QColor green(QStringLiteral("#66bb6a"));
    const QColor orange(QStringLiteral("#ffb74d"));

    QImage noData(traceWidget->size(), QImage::Format_ARGB32_Premultiplied);
    noData.fill(Qt::transparent);
    traceWidget->render(&noData);
    QCOMPARE(targetValue->text(), QStringLiteral("waiting"));
    QCOMPARE(samplesValue->text(), QStringLiteral("0 / 0"));
    QVERIFY(!targetSelector->isEnabled());
    QCOMPARE(exactColorCount(noData, waveformPlot, cyan), 0);
    QCOMPARE(exactColorCount(noData, waveformPlot, green), 0);
    QCOMPARE(exactColorCount(noData, waveformPlot, orange), 0);
    QCOMPARE(exactColorCount(noData, rmsPlot, cyan), 0);
    QCOMPARE(exactColorCount(noData, rmsPlot, green), 0);
    QCOMPARE(exactColorCount(noData, rmsPlot, orange), 0);

    Eigen::MatrixXd raw(2, 64);
    Eigen::MatrixXd denoised(2, 64);
    for(Eigen::Index sample = 0; sample < raw.cols(); ++sample) {
        const double phase = static_cast<double>(sample) / 8.0;
        raw(0, sample) = std::sin(phase);
        denoised(0, sample) = 0.4 * std::sin(phase);
        raw(1, sample) = std::cos(phase);
        denoised(1, sample) = 0.25 * std::cos(phase);
    }

    Diagnostics diagnostics{};
    diagnostics.inputRms = 1.0;
    diagnostics.outputRms = 0.4;
    diagnostics.estimatedNoiseRms = 0.6;
    const std::vector<Eigen::Index> targetRows{0, 1};

    model->captureInput(raw, targetRows);
    model->publishOutput(denoised, diagnostics);

    QTRY_COMPARE(sequenceValue->text(), QStringLiteral("1"));
    QCOMPARE(targetSelector->minimum(), 1);
    QCOMPARE(targetSelector->maximum(), 2);
    QCOMPARE(targetValue->text(), QStringLiteral("1 / row 0"));
    QCOMPARE(samplesValue->text(), QStringLiteral("64 / 64"));

    targetSelector->setValue(2);
    QCOMPARE(model->selectedTargetOrdinal(), 1);
    diagnostics.inputRms = 1.2;
    diagnostics.outputRms = 0.5;
    diagnostics.estimatedNoiseRms = 0.7;
    model->captureInput(raw, targetRows);
    model->publishOutput(denoised, diagnostics);
    QTRY_COMPARE(sequenceValue->text(), QStringLiteral("2"));
    QCOMPARE(targetValue->text(), QStringLiteral("2 / row 1"));

    QImage rendered(traceWidget->size(), QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    traceWidget->render(&rendered);

    QVERIFY(exactColorCount(rendered, waveformPlot, cyan) > 0);
    QVERIFY(exactColorCount(rendered, waveformPlot, green) > 0);
    QVERIFY(exactColorCount(rendered, waveformPlot, orange) > 0);
    QVERIFY(exactColorCount(rendered, rmsPlot, cyan) > 0);
    QVERIFY(exactColorCount(rendered, rmsPlot, green) > 0);
    QVERIFY(exactColorCount(rendered, rmsPlot, orange) > 0);
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    TestAdaptiveDenoisingUi testObject;
    const int result = QTest::qExec(&testObject, argc, argv);
    std::fprintf(stdout,
                 "\n[Adaptive Denoising] hosted visualization UI: %s (exit %d)\n",
                 result == 0 ? "PASS" : "FAIL",
                 result);
    std::fflush(stdout);
    return result;
}

#include "test_adaptive_denoising_ui.moc"

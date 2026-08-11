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
#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

#include <Eigen/Core>

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

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

} // namespace

//=============================================================================================================

class TestAdaptiveDenoisingUi : public QObject
{
    Q_OBJECT

private slots:
    void widgetControlsAndDiagnosticsExposePublicContract();
    void visualizationSnapshotIsBoundedAndChronological();
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
    QCOMPARE(snapshot.raw[0], 1000.0);
    QCOMPARE(snapshot.raw[kAdaptiveDenoisingTraceCapacity - 1], 1299.0);
    QCOMPARE(snapshot.denoised[0], 250.0);
    QCOMPARE(snapshot.estimatedNoise[0], 750.0);
    QCOMPARE(snapshot.rmsHistoryCount, Eigen::Index(1));
    QCOMPARE(snapshot.inputRmsHistory[0], 10.0);
    QCOMPARE(snapshot.outputRmsHistory[0], 4.0);
    QCOMPARE(snapshot.estimatedNoiseRmsHistory[0], 6.0);

    raw.conservativeResize(Eigen::NoChange, 1);
    denoised.conservativeResize(Eigen::NoChange, 1);
    for(int observation = 1;
        observation <= static_cast<int>(kAdaptiveDenoisingRmsHistoryCapacity) + 5;
        ++observation) {
        diagnostics.inputRms = static_cast<double>(observation);
        diagnostics.outputRms = 0.5 * static_cast<double>(observation);
        diagnostics.estimatedNoiseRms = 0.25 * static_cast<double>(observation);
        model.captureInput(raw, targetRows);
        model.publishOutput(denoised, diagnostics);
    }

    snapshot = model.snapshot();
    QCOMPARE(snapshot.rmsHistoryCount,
             Eigen::Index(kAdaptiveDenoisingRmsHistoryCapacity));
    QCOMPARE(snapshot.inputRmsHistory[0], 6.0);
    QCOMPARE(snapshot.inputRmsHistory[kAdaptiveDenoisingRmsHistoryCapacity - 1], 125.0);
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
    widget.show();

    QTRY_COMPARE(sequenceValue->text(), QStringLiteral("1"));
    QCOMPARE(targetSelector->minimum(), 1);
    QCOMPARE(targetSelector->maximum(), 2);
    QCOMPARE(targetValue->text(), QStringLiteral("1 / row 0"));
    QCOMPARE(samplesValue->text(), QStringLiteral("64 / 64"));

    targetSelector->setValue(2);
    QCOMPARE(model->selectedTargetOrdinal(), 1);
    model->captureInput(raw, targetRows);
    model->publishOutput(denoised, diagnostics);
    QTRY_COMPARE(sequenceValue->text(), QStringLiteral("2"));
    QCOMPARE(targetValue->text(), QStringLiteral("2 / row 1"));

    QImage rendered(traceWidget->size(), QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    traceWidget->render(&rendered);

    int cyanPixels = 0;
    int greenPixels = 0;
    int orangePixels = 0;
    for(int y = 0; y < rendered.height(); ++y) {
        for(int x = 0; x < rendered.width(); ++x) {
            const QColor color = QColor::fromRgba(rendered.pixel(x, y));
            cyanPixels += color == QColor(QStringLiteral("#4fc3f7")) ? 1 : 0;
            greenPixels += color == QColor(QStringLiteral("#66bb6a")) ? 1 : 0;
            orangePixels += color == QColor(QStringLiteral("#ffb74d")) ? 1 : 0;
        }
    }

    QVERIFY(cyanPixels > 0);
    QVERIFY(greenPixels > 0);
    QVERIFY(orangePixels > 0);
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

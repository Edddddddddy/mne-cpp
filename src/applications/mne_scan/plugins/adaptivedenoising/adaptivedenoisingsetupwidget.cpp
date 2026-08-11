//=============================================================================================================
/**
 * @file     adaptivedenoisingsetupwidget.cpp
 * @brief    Programmatic teaching controls and diagnostics for adaptive denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingsetupwidget.h"
#include "adaptivedenoisingtracewidget.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

#include <Eigen/Core>

#include <algorithm>
#include <utility>

//=============================================================================================================

namespace
{

using namespace ADAPTIVEDENOISINGPLUGIN;

QString pluginStateText(AdaptiveDenoisingPluginState state)
{
    switch(state) {
        case AdaptiveDenoisingPluginState::Stopped:
            return QStringLiteral("Stopped");
        case AdaptiveDenoisingPluginState::WaitingForData:
            return QStringLiteral("WaitingForData");
        case AdaptiveDenoisingPluginState::Processing:
            return QStringLiteral("Processing");
        case AdaptiveDenoisingPluginState::InvalidMetadata:
            return QStringLiteral("InvalidMetadata");
        case AdaptiveDenoisingPluginState::ConfigurationException:
            return QStringLiteral("ConfigurationException");
    }

    return QStringLiteral("Unknown");
}

QString configureStatusText(AdaptiveDenoisingConfigureStatus status)
{
    switch(status) {
        case AdaptiveDenoisingConfigureStatus::Ready:
            return QStringLiteral("Ready");
        case AdaptiveDenoisingConfigureStatus::InvalidMetadata:
            return QStringLiteral("InvalidMetadata");
        case AdaptiveDenoisingConfigureStatus::MissingReferences:
            return QStringLiteral("MissingReferences");
        case AdaptiveDenoisingConfigureStatus::MissingTargets:
            return QStringLiteral("MissingTargets");
        case AdaptiveDenoisingConfigureStatus::InvalidSettings:
            return QStringLiteral("InvalidSettings");
    }

    return QStringLiteral("Unknown");
}

QString processStatusText(RTPROCESSINGLIB::DenoiserProcessStatus status)
{
    switch(status) {
        case RTPROCESSINGLIB::DenoiserProcessStatus::NotConfigured:
            return QStringLiteral("NotConfigured");
        case RTPROCESSINGLIB::DenoiserProcessStatus::InvalidShape:
            return QStringLiteral("InvalidShape");
        case RTPROCESSINGLIB::DenoiserProcessStatus::NonFiniteInput:
            return QStringLiteral("NonFiniteInput");
        case RTPROCESSINGLIB::DenoiserProcessStatus::Bypassed:
            return QStringLiteral("Bypassed");
        case RTPROCESSINGLIB::DenoiserProcessStatus::Processed:
            return QStringLiteral("Processed");
    }

    return QStringLiteral("Unknown");
}

QString indexText(Eigen::Index value)
{
    return QString::number(static_cast<qlonglong>(value));
}

QString countText(std::uint64_t value)
{
    return QString::number(static_cast<qulonglong>(value));
}

QString rmsText(double value)
{
    return QString::number(value, 'g', 15);
}

} // namespace

//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

AdaptiveDenoisingSetupWidget::AdaptiveDenoisingSetupWidget(QWidget* parent)
: AdaptiveDenoisingSetupWidget(std::make_shared<AdaptiveDenoisingVisualizationModel>(), parent)
{
}

//=============================================================================================================

AdaptiveDenoisingSetupWidget::AdaptiveDenoisingSetupWidget(
    std::shared_ptr<AdaptiveDenoisingVisualizationModel> visualizationModel,
    QWidget* parent)
: QWidget(parent)
, m_visualizationModel(visualizationModel
      ? std::move(visualizationModel)
      : std::make_shared<AdaptiveDenoisingVisualizationModel>())
, m_enabledCheckBox(new QCheckBox(this))
, m_tapCountSpinBox(new QSpinBox(this))
, m_adaptationIntervalSpinBox(new QSpinBox(this))
, m_memoryTimeSecondsSpinBox(new QDoubleSpinBox(this))
, m_regularizationSpinBox(new QDoubleSpinBox(this))
, m_frozenCheckBox(new QCheckBox(this))
, m_resetButton(new QPushButton(this))
, m_visualizationTargetSpinBox(new QSpinBox(this))
, m_visualizationTraceWidget(new AdaptiveDenoisingTraceWidget(this))
, m_visualizationRefreshTimer(new QTimer(this))
, m_visualizationTargetValue(new QLabel(this))
, m_visualizationSamplesValue(new QLabel(this))
, m_visualizationSequenceValue(new QLabel(this))
, m_lastVisualizationSequence(0u)
, m_pluginStateValue(new QLabel(this))
, m_configureStatusValue(new QLabel(this))
, m_processStatusValue(new QLabel(this))
, m_referenceCountValue(new QLabel(this))
, m_targetCountValue(new QLabel(this))
, m_featureCountValue(new QLabel(this))
, m_warmupSamplesValue(new QLabel(this))
, m_modelGenerationValue(new QLabel(this))
, m_acceptedUpdatesValue(new QLabel(this))
, m_rejectedUpdatesValue(new QLabel(this))
, m_inputRmsValue(new QLabel(this))
, m_outputRmsValue(new QLabel(this))
, m_estimatedNoiseRmsValue(new QLabel(this))
, m_droppedBlocksValue(new QLabel(this))
{
    m_enabledCheckBox->setObjectName(QStringLiteral("enabledCheckBox"));
    m_enabledCheckBox->setChecked(true);

    m_tapCountSpinBox->setObjectName(QStringLiteral("tapCountSpinBox"));
    m_tapCountSpinBox->setRange(1, 32);
    m_tapCountSpinBox->setValue(4);

    m_adaptationIntervalSpinBox->setObjectName(QStringLiteral("adaptationIntervalSpinBox"));
    m_adaptationIntervalSpinBox->setRange(16, 2048);
    m_adaptationIntervalSpinBox->setValue(128);

    m_memoryTimeSecondsSpinBox->setObjectName(QStringLiteral("memoryTimeSecondsSpinBox"));
    m_memoryTimeSecondsSpinBox->setRange(1.0, 300.0);
    m_memoryTimeSecondsSpinBox->setDecimals(1);
    m_memoryTimeSecondsSpinBox->setValue(30.0);

    m_regularizationSpinBox->setObjectName(QStringLiteral("regularizationSpinBox"));
    m_regularizationSpinBox->setRange(1e-8, 1.0);
    m_regularizationSpinBox->setDecimals(8);
    m_regularizationSpinBox->setSingleStep(1e-3);
    m_regularizationSpinBox->setValue(1e-3);

    m_frozenCheckBox->setObjectName(QStringLiteral("frozenCheckBox"));
    m_frozenCheckBox->setChecked(false);

    m_resetButton->setObjectName(QStringLiteral("resetButton"));
    m_resetButton->setText(QStringLiteral("Reset model"));

    m_visualizationTargetSpinBox->setObjectName(QStringLiteral("visualizationTargetSpinBox"));
    m_visualizationTargetSpinBox->setRange(1, 1);
    m_visualizationTargetSpinBox->setValue(1);
    m_visualizationTargetSpinBox->setEnabled(false);

    m_visualizationTargetValue->setObjectName(QStringLiteral("visualizationTargetValue"));
    m_visualizationSamplesValue->setObjectName(QStringLiteral("visualizationSamplesValue"));
    m_visualizationSequenceValue->setObjectName(QStringLiteral("visualizationSequenceValue"));
    m_visualizationTargetValue->setText(QStringLiteral("waiting"));
    m_visualizationSamplesValue->setText(QStringLiteral("0 / 0"));
    m_visualizationSequenceValue->setText(QStringLiteral("0"));

    m_visualizationRefreshTimer->setObjectName(QStringLiteral("visualizationRefreshTimer"));
    m_visualizationRefreshTimer->setInterval(50);

    m_pluginStateValue->setObjectName(QStringLiteral("pluginStateValue"));
    m_configureStatusValue->setObjectName(QStringLiteral("configureStatusValue"));
    m_processStatusValue->setObjectName(QStringLiteral("processStatusValue"));
    m_referenceCountValue->setObjectName(QStringLiteral("referenceCountValue"));
    m_targetCountValue->setObjectName(QStringLiteral("targetCountValue"));
    m_featureCountValue->setObjectName(QStringLiteral("featureCountValue"));
    m_warmupSamplesValue->setObjectName(QStringLiteral("warmupSamplesValue"));
    m_modelGenerationValue->setObjectName(QStringLiteral("modelGenerationValue"));
    m_acceptedUpdatesValue->setObjectName(QStringLiteral("acceptedUpdatesValue"));
    m_rejectedUpdatesValue->setObjectName(QStringLiteral("rejectedUpdatesValue"));
    m_inputRmsValue->setObjectName(QStringLiteral("inputRmsValue"));
    m_outputRmsValue->setObjectName(QStringLiteral("outputRmsValue"));
    m_estimatedNoiseRmsValue->setObjectName(QStringLiteral("estimatedNoiseRmsValue"));
    m_droppedBlocksValue->setObjectName(QStringLiteral("droppedBlocksValue"));

    m_pluginStateValue->setText(QStringLiteral("Stopped"));
    m_configureStatusValue->setText(QStringLiteral("InvalidMetadata"));
    m_processStatusValue->setText(QStringLiteral("NotConfigured"));
    m_referenceCountValue->setText(QStringLiteral("0"));
    m_targetCountValue->setText(QStringLiteral("0"));
    m_featureCountValue->setText(QStringLiteral("0"));
    m_warmupSamplesValue->setText(QStringLiteral("0"));
    m_modelGenerationValue->setText(QStringLiteral("0"));
    m_acceptedUpdatesValue->setText(QStringLiteral("0"));
    m_rejectedUpdatesValue->setText(QStringLiteral("0"));
    m_inputRmsValue->setText(QStringLiteral("n/a"));
    m_outputRmsValue->setText(QStringLiteral("n/a"));
    m_estimatedNoiseRmsValue->setText(QStringLiteral("n/a"));
    m_droppedBlocksValue->setText(QStringLiteral("0"));

    QFormLayout* const controlsLayout = new QFormLayout;
    controlsLayout->addRow(QStringLiteral("Enabled"), m_enabledCheckBox);
    controlsLayout->addRow(QStringLiteral("Taps"), m_tapCountSpinBox);
    controlsLayout->addRow(QStringLiteral("Adaptation interval (samples)"),
                           m_adaptationIntervalSpinBox);
    controlsLayout->addRow(QStringLiteral("Memory (seconds)"), m_memoryTimeSecondsSpinBox);
    controlsLayout->addRow(QStringLiteral("Regularization"), m_regularizationSpinBox);
    controlsLayout->addRow(QStringLiteral("Freeze learning"), m_frozenCheckBox);
    controlsLayout->addRow(m_resetButton);
    controlsLayout->addRow(QStringLiteral("Plugin state"), m_pluginStateValue);
    controlsLayout->addRow(QStringLiteral("Configure status"), m_configureStatusValue);
    controlsLayout->addRow(QStringLiteral("Process status"), m_processStatusValue);
    controlsLayout->addRow(QStringLiteral("Reference rows"), m_referenceCountValue);
    controlsLayout->addRow(QStringLiteral("Target rows"), m_targetCountValue);
    controlsLayout->addRow(QStringLiteral("Features"), m_featureCountValue);
    controlsLayout->addRow(QStringLiteral("Warmup samples"), m_warmupSamplesValue);
    controlsLayout->addRow(QStringLiteral("Model generation"), m_modelGenerationValue);
    controlsLayout->addRow(QStringLiteral("Accepted updates"), m_acceptedUpdatesValue);
    controlsLayout->addRow(QStringLiteral("Rejected updates"), m_rejectedUpdatesValue);
    controlsLayout->addRow(QStringLiteral("Input RMS"), m_inputRmsValue);
    controlsLayout->addRow(QStringLiteral("Output RMS"), m_outputRmsValue);
    controlsLayout->addRow(QStringLiteral("Estimated noise RMS"), m_estimatedNoiseRmsValue);
    controlsLayout->addRow(QStringLiteral("Dropped blocks"), m_droppedBlocksValue);

    QFormLayout* const visualizationStatusLayout = new QFormLayout;
    visualizationStatusLayout->addRow(QStringLiteral("Displayed target"),
                                      m_visualizationTargetSpinBox);
    visualizationStatusLayout->addRow(QStringLiteral("Target / FIFF row"),
                                      m_visualizationTargetValue);
    visualizationStatusLayout->addRow(QStringLiteral("Displayed / source samples"),
                                      m_visualizationSamplesValue);
    visualizationStatusLayout->addRow(QStringLiteral("Snapshot sequence"),
                                      m_visualizationSequenceValue);

    QVBoxLayout* const visualizationLayout = new QVBoxLayout;
    visualizationLayout->addLayout(visualizationStatusLayout);
    visualizationLayout->addWidget(m_visualizationTraceWidget, 1);

    QHBoxLayout* const layout = new QHBoxLayout;
    layout->addLayout(controlsLayout);
    layout->addLayout(visualizationLayout, 1);
    setLayout(layout);

    connect(m_enabledCheckBox, &QCheckBox::toggled, this, &AdaptiveDenoisingSetupWidget::enabledChanged);
    connect(m_frozenCheckBox, &QCheckBox::toggled, this, &AdaptiveDenoisingSetupWidget::frozenChanged);
    connect(m_tapCountSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &AdaptiveDenoisingSetupWidget::tapCountChanged);
    connect(m_adaptationIntervalSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &AdaptiveDenoisingSetupWidget::adaptationIntervalChanged);
    connect(m_memoryTimeSecondsSpinBox,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,
            &AdaptiveDenoisingSetupWidget::memoryTimeSecondsChanged);
    connect(m_regularizationSpinBox,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,
            &AdaptiveDenoisingSetupWidget::regularizationChanged);
    connect(m_resetButton, &QPushButton::clicked, this, [this]() { emit resetRequested(); });
    connect(m_visualizationTargetSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int displayedTarget) {
                m_visualizationModel->setSelectedTargetOrdinal(displayedTarget - 1);
            });
    connect(m_visualizationRefreshTimer, &QTimer::timeout,
            this, &AdaptiveDenoisingSetupWidget::refreshVisualization);
    m_visualizationRefreshTimer->start();
}

//=============================================================================================================

void AdaptiveDenoisingSetupWidget::setDiagnostics(const AdaptiveDenoisingDiagnostics& diagnostics)
{
    m_pluginStateValue->setText(pluginStateText(diagnostics.pluginState));
    m_configureStatusValue->setText(configureStatusText(diagnostics.configureStatus));
    m_processStatusValue->setText(processStatusText(diagnostics.processStatus));
    m_referenceCountValue->setText(indexText(diagnostics.referenceRowCount));
    m_targetCountValue->setText(indexText(diagnostics.targetRowCount));
    m_featureCountValue->setText(indexText(diagnostics.featureCount));
    m_warmupSamplesValue->setText(indexText(diagnostics.warmupSamplesRemaining));
    m_modelGenerationValue->setText(countText(diagnostics.modelGeneration));
    m_acceptedUpdatesValue->setText(countText(diagnostics.modelUpdatesAccepted));
    m_rejectedUpdatesValue->setText(countText(diagnostics.modelUpdatesRejected));
    m_inputRmsValue->setText(rmsText(diagnostics.inputRms));
    m_outputRmsValue->setText(rmsText(diagnostics.outputRms));
    m_estimatedNoiseRmsValue->setText(rmsText(diagnostics.estimatedNoiseRms));
    m_droppedBlocksValue->setText(countText(diagnostics.droppedBlocks));
}

//=============================================================================================================

void AdaptiveDenoisingSetupWidget::refreshVisualization()
{
    const AdaptiveDenoisingVisualizationSnapshot snapshot = m_visualizationModel->snapshot();
    const int targetCount = static_cast<int>(snapshot.targetCount);
    m_visualizationTargetSpinBox->setEnabled(targetCount > 0);
    m_visualizationTargetSpinBox->setMaximum(std::max(1, targetCount));

    if(snapshot.sequence != m_lastVisualizationSequence) {
        const QSignalBlocker blocker(m_visualizationTargetSpinBox);
        m_visualizationTargetSpinBox->setValue(
            static_cast<int>(snapshot.selectedTargetOrdinal) + 1);
        m_lastVisualizationSequence = snapshot.sequence;
    }

    m_visualizationSequenceValue->setText(
        QString::number(static_cast<qulonglong>(snapshot.sequence)));
    m_visualizationSamplesValue->setText(
        QStringLiteral("%1 / %2")
            .arg(static_cast<qlonglong>(snapshot.traceSampleCount))
            .arg(static_cast<qlonglong>(snapshot.sourceSampleCount)));
    m_visualizationTargetValue->setText(snapshot.targetCount > 0
        ? QStringLiteral("%1 / row %2")
              .arg(static_cast<qlonglong>(snapshot.selectedTargetOrdinal + 1))
              .arg(static_cast<qlonglong>(snapshot.selectedTargetRow))
        : QStringLiteral("waiting"));
    m_visualizationTraceWidget->setSnapshot(snapshot);
}

//=============================================================================================================

} // NAMESPACE

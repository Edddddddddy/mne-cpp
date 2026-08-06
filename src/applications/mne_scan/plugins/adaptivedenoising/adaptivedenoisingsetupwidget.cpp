//=============================================================================================================
/**
 * @file     adaptivedenoisingsetupwidget.cpp
 * @brief    Programmatic teaching controls and diagnostics for adaptive denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingsetupwidget.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

#include <Eigen/Core>

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
: QWidget(parent)
, m_enabledCheckBox(new QCheckBox(this))
, m_tapCountSpinBox(new QSpinBox(this))
, m_adaptationIntervalSpinBox(new QSpinBox(this))
, m_memoryTimeSecondsSpinBox(new QDoubleSpinBox(this))
, m_regularizationSpinBox(new QDoubleSpinBox(this))
, m_frozenCheckBox(new QCheckBox(this))
, m_resetButton(new QPushButton(this))
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

    QFormLayout* const layout = new QFormLayout;
    layout->addRow(QStringLiteral("Enabled"), m_enabledCheckBox);
    layout->addRow(QStringLiteral("Taps"), m_tapCountSpinBox);
    layout->addRow(QStringLiteral("Adaptation interval (samples)"), m_adaptationIntervalSpinBox);
    layout->addRow(QStringLiteral("Memory (seconds)"), m_memoryTimeSecondsSpinBox);
    layout->addRow(QStringLiteral("Regularization"), m_regularizationSpinBox);
    layout->addRow(QStringLiteral("Freeze learning"), m_frozenCheckBox);
    layout->addRow(m_resetButton);
    layout->addRow(QStringLiteral("Plugin state"), m_pluginStateValue);
    layout->addRow(QStringLiteral("Configure status"), m_configureStatusValue);
    layout->addRow(QStringLiteral("Process status"), m_processStatusValue);
    layout->addRow(QStringLiteral("Reference rows"), m_referenceCountValue);
    layout->addRow(QStringLiteral("Target rows"), m_targetCountValue);
    layout->addRow(QStringLiteral("Features"), m_featureCountValue);
    layout->addRow(QStringLiteral("Warmup samples"), m_warmupSamplesValue);
    layout->addRow(QStringLiteral("Model generation"), m_modelGenerationValue);
    layout->addRow(QStringLiteral("Accepted updates"), m_acceptedUpdatesValue);
    layout->addRow(QStringLiteral("Rejected updates"), m_rejectedUpdatesValue);
    layout->addRow(QStringLiteral("Input RMS"), m_inputRmsValue);
    layout->addRow(QStringLiteral("Output RMS"), m_outputRmsValue);
    layout->addRow(QStringLiteral("Estimated noise RMS"), m_estimatedNoiseRmsValue);
    layout->addRow(QStringLiteral("Dropped blocks"), m_droppedBlocksValue);
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

} // NAMESPACE

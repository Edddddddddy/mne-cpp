//=============================================================================================================
/**
 * @file     aoemegstatuswidget.cpp
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Definition of the AOEMegStatusWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "aoemegstatuswidget.h"

#include "../aoemeg.h"

#include <QFileDialog>
#include <QFileInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AOEMEGPLUGIN;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AOEMegStatusWidget::AOEMegStatusWidget(AOEMeg* pAoeMeg, QWidget* parent)
: QWidget(parent)
, m_pAoeMeg(pAoeMeg)
, m_pStage1CheckBox(new QCheckBox("Enable Stage-1", this))
, m_pStage1LambdaSpinBox(new QDoubleSpinBox(this))
, m_pStage1CoeffClipSpinBox(new QDoubleSpinBox(this))
, m_pStage1PredClipSpinBox(new QDoubleSpinBox(this))
, m_pStage2CheckBox(new QCheckBox("Enable Stage-2", this))
, m_pStage2MagRankSpinBox(new QSpinBox(this))
, m_pStage2GradRankSpinBox(new QSpinBox(this))
, m_pStage2MagStrengthSpinBox(new QDoubleSpinBox(this))
, m_pStage2GradStrengthSpinBox(new QDoubleSpinBox(this))
, m_pStage2BandLowSpinBox(new QDoubleSpinBox(this))
, m_pStage2BandHighSpinBox(new QDoubleSpinBox(this))
, m_pStage2UpdateEverySpinBox(new QSpinBox(this))
, m_pStage2RegSpinBox(new QDoubleSpinBox(this))
, m_pStage2UsePriorCheckBox(new QCheckBox("Use empty-room prior", this))
, m_pStage2PriorMagRankSpinBox(new QSpinBox(this))
, m_pStage2PriorGradRankSpinBox(new QSpinBox(this))
, m_pStage2PriorLoadButton(new QPushButton("Load prior FIF", this))
, m_pStage2PriorClearButton(new QPushButton("Clear prior", this))
, m_pStage2PriorLabel(new QLabel("Stage-2 prior: none", this))
, m_pStage3CheckBox(new QCheckBox("Enable Stage-3", this))
, m_pStage3MeanfieldOrderSpinBox(new QSpinBox(this))
, m_pStage3MagKMaxSpinBox(new QSpinBox(this))
, m_pStage3GradKMaxSpinBox(new QSpinBox(this))
, m_pStage3MagStrengthSpinBox(new QDoubleSpinBox(this))
, m_pStage3GradStrengthSpinBox(new QDoubleSpinBox(this))
, m_pPipelineEdit(new QPlainTextEdit(this))
, m_pStage1StatusEdit(new QPlainTextEdit(this))
, m_pStage2StatusEdit(new QPlainTextEdit(this))
, m_pStage3StatusEdit(new QPlainTextEdit(this))
, m_pHpiEdit(new QPlainTextEdit(this))
, m_pSnrEdit(new QPlainTextEdit(this))
{
    QLabel* pTitleLabel = new QLabel("AOE-MEG Runtime", this);
    pTitleLabel->setWordWrap(true);

    m_pStage1LambdaSpinBox->setRange(0.900, 0.9999);
    m_pStage1LambdaSpinBox->setDecimals(4);
    m_pStage1LambdaSpinBox->setSingleStep(0.001);
    m_pStage1CoeffClipSpinBox->setRange(0.1, 10.0);
    m_pStage1CoeffClipSpinBox->setDecimals(2);
    m_pStage1CoeffClipSpinBox->setSingleStep(0.10);
    m_pStage1PredClipSpinBox->setRange(0.1, 10.0);
    m_pStage1PredClipSpinBox->setDecimals(2);
    m_pStage1PredClipSpinBox->setSingleStep(0.10);

    m_pStage2MagRankSpinBox->setRange(0, 8);
    m_pStage2GradRankSpinBox->setRange(0, 8);
    m_pStage2MagStrengthSpinBox->setRange(0.0, 1.0);
    m_pStage2MagStrengthSpinBox->setDecimals(2);
    m_pStage2MagStrengthSpinBox->setSingleStep(0.05);
    m_pStage2GradStrengthSpinBox->setRange(0.0, 1.0);
    m_pStage2GradStrengthSpinBox->setDecimals(2);
    m_pStage2GradStrengthSpinBox->setSingleStep(0.05);
    m_pStage2BandLowSpinBox->setRange(0.0, 50.0);
    m_pStage2BandLowSpinBox->setDecimals(1);
    m_pStage2BandLowSpinBox->setSingleStep(0.5);
    m_pStage2BandHighSpinBox->setRange(0.5, 100.0);
    m_pStage2BandHighSpinBox->setDecimals(1);
    m_pStage2BandHighSpinBox->setSingleStep(0.5);
    m_pStage2UpdateEverySpinBox->setRange(1, 16);
    m_pStage2RegSpinBox->setRange(1e-8, 1e-2);
    m_pStage2RegSpinBox->setDecimals(6);
    m_pStage2RegSpinBox->setSingleStep(1e-5);
    m_pStage2PriorMagRankSpinBox->setRange(0, 16);
    m_pStage2PriorGradRankSpinBox->setRange(0, 16);
    m_pStage3MeanfieldOrderSpinBox->setRange(0, 1);
    m_pStage3MagKMaxSpinBox->setRange(0, 12);
    m_pStage3GradKMaxSpinBox->setRange(0, 12);
    m_pStage3MagStrengthSpinBox->setRange(0.0, 1.0);
    m_pStage3MagStrengthSpinBox->setDecimals(2);
    m_pStage3MagStrengthSpinBox->setSingleStep(0.05);
    m_pStage3GradStrengthSpinBox->setRange(0.0, 1.0);
    m_pStage3GradStrengthSpinBox->setDecimals(2);
    m_pStage3GradStrengthSpinBox->setSingleStep(0.05);

    m_pPipelineEdit->setReadOnly(true);
    m_pPipelineEdit->setMaximumHeight(90);
    m_pStage1StatusEdit->setReadOnly(true);
    m_pStage1StatusEdit->setMaximumHeight(120);
    m_pStage2StatusEdit->setReadOnly(true);
    m_pStage2StatusEdit->setMaximumHeight(140);
    m_pStage3StatusEdit->setReadOnly(true);
    m_pStage3StatusEdit->setMaximumHeight(140);
    m_pHpiEdit->setReadOnly(true);
    m_pHpiEdit->setMaximumHeight(130);
    m_pSnrEdit->setReadOnly(true);
    m_pSnrEdit->setMaximumHeight(90);

    QGroupBox* pStage1Box = new QGroupBox("Stage-1 Motion Regression", this);
    QFormLayout* pStage1Layout = new QFormLayout;
    pStage1Layout->addRow(m_pStage1CheckBox);
    pStage1Layout->addRow("rls_lambda (default 0.995)", m_pStage1LambdaSpinBox);
    pStage1Layout->addRow("coeff_clip (default 2.0)", m_pStage1CoeffClipSpinBox);
    pStage1Layout->addRow("pred_clip_ratio (default 2.0)", m_pStage1PredClipSpinBox);
    pStage1Layout->addRow(m_pStage1StatusEdit);
    pStage1Box->setLayout(pStage1Layout);

    QGroupBox* pStage2Box = new QGroupBox("Stage-2 Residual SSP", this);
    QFormLayout* pStage2Layout = new QFormLayout;
    pStage2Layout->addRow(m_pStage2CheckBox);
    pStage2Layout->addRow("mag rank (default 1)", m_pStage2MagRankSpinBox);
    pStage2Layout->addRow("grad rank (default 2)", m_pStage2GradRankSpinBox);
    pStage2Layout->addRow("mag strength (default 0.18)", m_pStage2MagStrengthSpinBox);
    pStage2Layout->addRow("grad strength (default 0.28)", m_pStage2GradStrengthSpinBox);
    pStage2Layout->addRow("band low Hz (default 1.0)", m_pStage2BandLowSpinBox);
    pStage2Layout->addRow("band high Hz (default 20/30)", m_pStage2BandHighSpinBox);
    pStage2Layout->addRow("update every (default 1)", m_pStage2UpdateEverySpinBox);
    pStage2Layout->addRow("regularization (default 1e-5)", m_pStage2RegSpinBox);
    pStage2Layout->addRow(m_pStage2UsePriorCheckBox);
    pStage2Layout->addRow("prior mag rank (default 2)", m_pStage2PriorMagRankSpinBox);
    pStage2Layout->addRow("prior grad rank (default 4)", m_pStage2PriorGradRankSpinBox);
    pStage2Layout->addRow(m_pStage2PriorLoadButton, m_pStage2PriorClearButton);
    pStage2Layout->addRow(m_pStage2PriorLabel);
    pStage2Layout->addRow(m_pStage2StatusEdit);
    pStage2Box->setLayout(pStage2Layout);

    QGroupBox* pStage3Box = new QGroupBox("Stage-3 External Projection", this);
    QFormLayout* pStage3Layout = new QFormLayout;
    pStage3Layout->addRow(m_pStage3CheckBox);
    pStage3Layout->addRow(new QLabel("Stage-3 now runs mean-field physical basis plus statistical external-noise subspace projection.", this));
    pStage3Layout->addRow("meanfield order (default 1)", m_pStage3MeanfieldOrderSpinBox);
    pStage3Layout->addRow("mag kmax (default 2)", m_pStage3MagKMaxSpinBox);
    pStage3Layout->addRow("grad kmax (default 6)", m_pStage3GradKMaxSpinBox);
    pStage3Layout->addRow("mag ext strength (default 1.00)", m_pStage3MagStrengthSpinBox);
    pStage3Layout->addRow("grad ext strength (default 1.00)", m_pStage3GradStrengthSpinBox);
    pStage3Layout->addWidget(m_pStage3StatusEdit);
    pStage3Box->setLayout(pStage3Layout);

    QGroupBox* pPipelineBox = new QGroupBox("Pipeline Overview", this);
    QVBoxLayout* pPipelineLayout = new QVBoxLayout;
    pPipelineLayout->addWidget(m_pPipelineEdit);
    pPipelineBox->setLayout(pPipelineLayout);

    QGroupBox* pHpiBox = new QGroupBox("HPI / Motion Input", this);
    QVBoxLayout* pHpiLayout = new QVBoxLayout;
    pHpiLayout->addWidget(m_pHpiEdit);
    pHpiBox->setLayout(pHpiLayout);

    QGroupBox* pSnrBox = new QGroupBox("Runtime Effect Metrics", this);
    QVBoxLayout* pSnrLayout = new QVBoxLayout;
    pSnrLayout->addWidget(m_pSnrEdit);
    pSnrBox->setLayout(pSnrLayout);

    QVBoxLayout* pLayout = new QVBoxLayout;
    pLayout->addWidget(pTitleLabel);
    pLayout->addWidget(pPipelineBox);
    pLayout->addWidget(pStage1Box);
    pLayout->addWidget(pStage2Box);
    pLayout->addWidget(pStage3Box);
    pLayout->addWidget(pHpiBox);
    pLayout->addWidget(pSnrBox);
    pLayout->addStretch(1);
    setLayout(pLayout);

    if(m_pAoeMeg) {
        loadFromSettings(m_pAoeMeg->getCurrentSettings());
        setStatusText(m_pAoeMeg->getRuntimeStatusSummary());
        setSnrText(m_pAoeMeg->getSnrSummary());
        setStage2PriorText(m_pAoeMeg->getStage2PriorSummary());
        setSectionTexts(m_pAoeMeg->getPipelineStatusSummary(),
                        m_pAoeMeg->getStage1StatusSummary(),
                        m_pAoeMeg->getStage2StatusSummary(),
                        m_pAoeMeg->getStage3StatusSummary(),
                        m_pAoeMeg->getHpiStatusSummary());

        connect(m_pAoeMeg, &AOEMeg::runtimeStatusChanged,
                this, &AOEMegStatusWidget::setStatusText,
                Qt::QueuedConnection);
        connect(m_pAoeMeg, &AOEMeg::runtimeSectionsChanged,
                this, &AOEMegStatusWidget::setSectionTexts,
                Qt::QueuedConnection);
        connect(m_pAoeMeg, &AOEMeg::runtimeSnrChanged,
                this, &AOEMegStatusWidget::setSnrText,
                Qt::QueuedConnection);
        connect(m_pAoeMeg, &AOEMeg::stage2PriorChanged,
                this, &AOEMegStatusWidget::setStage2PriorText,
                Qt::QueuedConnection);
    }

    connect(m_pStage1CheckBox, &QCheckBox::toggled, this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage1LambdaSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage1CoeffClipSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage1PredClipSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2CheckBox, &QCheckBox::toggled, this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2MagRankSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2GradRankSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2MagStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2GradStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2BandLowSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2BandHighSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2UpdateEverySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2RegSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2UsePriorCheckBox, &QCheckBox::toggled, this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2PriorMagRankSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2PriorGradRankSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage2PriorLoadButton, &QPushButton::clicked, this, &AOEMegStatusWidget::onLoadStage2PriorClicked);
    connect(m_pStage2PriorClearButton, &QPushButton::clicked, this, &AOEMegStatusWidget::onClearStage2PriorClicked);
    connect(m_pStage3CheckBox, &QCheckBox::toggled, this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage3MeanfieldOrderSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage3MagKMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage3GradKMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage3MagStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
    connect(m_pStage3GradStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AOEMegStatusWidget::onControlsChanged);
}

//=============================================================================================================

AOEMegStatusWidget::~AOEMegStatusWidget()
{
}

//=============================================================================================================

void AOEMegStatusWidget::setStatusText(const QString& sStatusText)
{
    Q_UNUSED(sStatusText)
}

//=============================================================================================================

void AOEMegStatusWidget::setSectionTexts(const QString& sPipelineText,
                                         const QString& sStage1Text,
                                         const QString& sStage2Text,
                                         const QString& sStage3Text,
                                         const QString& sHpiText)
{
    m_pPipelineEdit->setPlainText(sPipelineText);
    m_pStage1StatusEdit->setPlainText(sStage1Text);
    m_pStage2StatusEdit->setPlainText(sStage2Text);
    m_pStage3StatusEdit->setPlainText(sStage3Text);
    m_pHpiEdit->setPlainText(sHpiText);
}

//=============================================================================================================

void AOEMegStatusWidget::setSnrText(const QString& sSnrText)
{
    m_pSnrEdit->setPlainText(sSnrText);
}

//=============================================================================================================

void AOEMegStatusWidget::setStage2PriorText(const QString& sPriorText)
{
    m_pStage2PriorLabel->setText(sPriorText);
}

//=============================================================================================================

void AOEMegStatusWidget::onControlsChanged()
{
    if(!m_pAoeMeg) {
        return;
    }

    m_settings.stage1.bEnabled = m_pStage1CheckBox->isChecked();
    m_settings.stage1.dRlsLambda = m_pStage1LambdaSpinBox->value();
    m_settings.stage1.dRlsCoeffClip = m_pStage1CoeffClipSpinBox->value();
    m_settings.stage1.dRlsPredClipRatio = m_pStage1PredClipSpinBox->value();
    m_settings.stage2.bEnabled = m_pStage2CheckBox->isChecked();
    m_settings.stage2.mag.iSspRank = m_pStage2MagRankSpinBox->value();
    m_settings.stage2.grad.iSspRank = m_pStage2GradRankSpinBox->value();
    m_settings.stage2.mag.dStrength = m_pStage2MagStrengthSpinBox->value();
    m_settings.stage2.grad.dStrength = m_pStage2GradStrengthSpinBox->value();
    m_settings.stage2.mag.dBandLowHz = m_pStage2BandLowSpinBox->value();
    m_settings.stage2.mag.dBandHighHz = m_pStage2BandHighSpinBox->value();
    m_settings.stage2.grad.dBandLowHz = m_pStage2BandLowSpinBox->value();
    m_settings.stage2.grad.dBandHighHz = m_pStage2BandHighSpinBox->value();
    m_settings.stage2.iUpdateEveryChunks = m_pStage2UpdateEverySpinBox->value();
    m_settings.stage2.dRegularization = m_pStage2RegSpinBox->value();
    m_settings.stage2.bUsePrior = m_pStage2UsePriorCheckBox->isChecked();
    m_settings.stage2.iPriorRankMag = m_pStage2PriorMagRankSpinBox->value();
    m_settings.stage2.iPriorRankGrad = m_pStage2PriorGradRankSpinBox->value();
    m_settings.stage3.bEnabled = m_pStage3CheckBox->isChecked();
    m_settings.stage3.iMeanfieldOrder = m_pStage3MeanfieldOrderSpinBox->value();
    m_settings.stage3.mag.iKMax = m_pStage3MagKMaxSpinBox->value();
    m_settings.stage3.grad.iKMax = m_pStage3GradKMaxSpinBox->value();
    m_settings.stage3.mag.dExtStrength = m_pStage3MagStrengthSpinBox->value();
    m_settings.stage3.grad.dExtStrength = m_pStage3GradStrengthSpinBox->value();

    m_pAoeMeg->setRuntimeSettings(m_settings);
}

//=============================================================================================================

void AOEMegStatusWidget::onLoadStage2PriorClicked()
{
    if(!m_pAoeMeg) {
        return;
    }

    const QString sFilePath = QFileDialog::getOpenFileName(this,
                                                           "Load Stage-2 prior FIF/proj file",
                                                           QString(),
                                                           "FIFF files (*.fif *.fiff);;All files (*.*)");
    if(sFilePath.isEmpty()) {
        return;
    }

    if(m_pAoeMeg->loadStage2PriorFromFile(sFilePath)) {
        setStage2PriorText(m_pAoeMeg->getStage2PriorSummary());
    } else {
        setStage2PriorText(QString("Stage-2 prior load failed: %1").arg(QFileInfo(sFilePath).fileName()));
    }
}

//=============================================================================================================

void AOEMegStatusWidget::onClearStage2PriorClicked()
{
    if(!m_pAoeMeg) {
        return;
    }

    m_pAoeMeg->clearStage2Prior();
    setStage2PriorText(m_pAoeMeg->getStage2PriorSummary());
}

//=============================================================================================================

void AOEMegStatusWidget::loadFromSettings(const AOEMegSettings& settings)
{
    m_settings = settings;

    m_pStage1CheckBox->setChecked(settings.stage1.bEnabled);
    m_pStage1LambdaSpinBox->setValue(settings.stage1.dRlsLambda);
    m_pStage1CoeffClipSpinBox->setValue(settings.stage1.dRlsCoeffClip);
    m_pStage1PredClipSpinBox->setValue(settings.stage1.dRlsPredClipRatio);
    m_pStage2CheckBox->setChecked(settings.stage2.bEnabled);
    m_pStage2MagRankSpinBox->setValue(settings.stage2.mag.iSspRank);
    m_pStage2GradRankSpinBox->setValue(settings.stage2.grad.iSspRank);
    m_pStage2MagStrengthSpinBox->setValue(settings.stage2.mag.dStrength);
    m_pStage2GradStrengthSpinBox->setValue(settings.stage2.grad.dStrength);
    m_pStage2BandLowSpinBox->setValue(settings.stage2.grad.dBandLowHz);
    m_pStage2BandHighSpinBox->setValue(settings.stage2.grad.dBandHighHz);
    m_pStage2UpdateEverySpinBox->setValue(settings.stage2.iUpdateEveryChunks);
    m_pStage2RegSpinBox->setValue(settings.stage2.dRegularization);
    m_pStage2UsePriorCheckBox->setChecked(settings.stage2.bUsePrior);
    m_pStage2PriorMagRankSpinBox->setValue(settings.stage2.iPriorRankMag);
    m_pStage2PriorGradRankSpinBox->setValue(settings.stage2.iPriorRankGrad);
    m_pStage3CheckBox->setChecked(settings.stage3.bEnabled);
    m_pStage3MeanfieldOrderSpinBox->setValue(settings.stage3.iMeanfieldOrder);
    m_pStage3MagKMaxSpinBox->setValue(settings.stage3.mag.iKMax);
    m_pStage3GradKMaxSpinBox->setValue(settings.stage3.grad.iKMax);
    m_pStage3MagStrengthSpinBox->setValue(settings.stage3.mag.dExtStrength);
    m_pStage3GradStrengthSpinBox->setValue(settings.stage3.grad.dExtStrength);
}

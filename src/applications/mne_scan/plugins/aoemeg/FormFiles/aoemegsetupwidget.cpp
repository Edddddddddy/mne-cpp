//=============================================================================================================
/**
 * @file     aoemegsetupwidget.cpp
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Definition of the AOEMegSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "aoemegsetupwidget.h"

#include "../aoemeg.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AOEMEGPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AOEMegSetupWidget::AOEMegSetupWidget(AOEMeg* pAoeMeg, QWidget* parent)
: QWidget(parent)
, m_pAoeMeg(pAoeMeg)
, m_pSummaryLabel(new QLabel(this))
{
    QLabel* pTitleLabel = new QLabel("AOE-MEG real-time denoising pipeline", this);
    QLabel* pPhaseLabel = new QLabel("Current implementation stage: Phase 3 is wired in. Stage-2 now applies a mild fullband residual SSP effect, and Stage-3 now performs external-basis plus statistical external-noise projection.", this);
    QLabel* pLayerLabel = new QLabel("Algorithm core lives in rtprocessing; this plugin owns I/O, RealTimeHpiResult mapping, worker, UI and result dispatch.", this);
    QLabel* pRuntimeLabel = new QLabel(this);

    pTitleLabel->setWordWrap(true);
    pPhaseLabel->setWordWrap(true);
    pLayerLabel->setWordWrap(true);
    pRuntimeLabel->setWordWrap(true);
    m_pSummaryLabel->setWordWrap(true);

    if(m_pAoeMeg) {
        m_pSummaryLabel->setText(m_pAoeMeg->getConfigurationSummary());
        pRuntimeLabel->setText(m_pAoeMeg->getRuntimeStatusSummary());
    }

    QVBoxLayout* pLayout = new QVBoxLayout;
    pLayout->addWidget(pTitleLabel);
    pLayout->addWidget(pPhaseLabel);
    pLayout->addWidget(pLayerLabel);
    pLayout->addWidget(m_pSummaryLabel);
    pLayout->addWidget(pRuntimeLabel);
    pLayout->addStretch(1);

    setLayout(pLayout);
}

//=============================================================================================================

AOEMegSetupWidget::~AOEMegSetupWidget()
{
}

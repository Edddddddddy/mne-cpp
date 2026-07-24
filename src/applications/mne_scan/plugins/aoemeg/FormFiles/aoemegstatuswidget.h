//=============================================================================================================
/**
 * @file     aoemegstatuswidget.h
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Contains the declaration of the AOEMegStatusWidget class.
 *
 */

#ifndef AOEMEGSTATUSWIDGET_H
#define AOEMEGSTATUSWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtprocessing/aoemegtypes.h>

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE AOEMEGPLUGIN
//=============================================================================================================

namespace AOEMEGPLUGIN
{

class AOEMeg;

class AOEMegStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AOEMegStatusWidget(AOEMeg* pAoeMeg, QWidget* parent = 0);
    ~AOEMegStatusWidget();

public slots:
    void setStatusText(const QString& sStatusText);
    void setSectionTexts(const QString& sPipelineText,
                         const QString& sStage1Text,
                         const QString& sStage2Text,
                         const QString& sStage3Text,
                         const QString& sHpiText);
    void setSnrText(const QString& sSnrText);
    void setStage2PriorText(const QString& sPriorText);

private slots:
    void onControlsChanged();
    void onLoadStage2PriorClicked();
    void onClearStage2PriorClicked();

private:
    void loadFromSettings(const RTPROCESSINGLIB::AOEMegSettings& settings);

    AOEMeg*                                             m_pAoeMeg;
    RTPROCESSINGLIB::AOEMegSettings                     m_settings;
    QCheckBox*                                          m_pStage1CheckBox;
    QDoubleSpinBox*                                     m_pStage1LambdaSpinBox;
    QDoubleSpinBox*                                     m_pStage1CoeffClipSpinBox;
    QDoubleSpinBox*                                     m_pStage1PredClipSpinBox;
    QCheckBox*                                          m_pStage2CheckBox;
    QSpinBox*                                           m_pStage2MagRankSpinBox;
    QSpinBox*                                           m_pStage2GradRankSpinBox;
    QDoubleSpinBox*                                     m_pStage2MagStrengthSpinBox;
    QDoubleSpinBox*                                     m_pStage2GradStrengthSpinBox;
    QDoubleSpinBox*                                     m_pStage2BandLowSpinBox;
    QDoubleSpinBox*                                     m_pStage2BandHighSpinBox;
    QSpinBox*                                           m_pStage2UpdateEverySpinBox;
    QDoubleSpinBox*                                     m_pStage2RegSpinBox;
    QCheckBox*                                          m_pStage2UsePriorCheckBox;
    QSpinBox*                                           m_pStage2PriorMagRankSpinBox;
    QSpinBox*                                           m_pStage2PriorGradRankSpinBox;
    QPushButton*                                        m_pStage2PriorLoadButton;
    QPushButton*                                        m_pStage2PriorClearButton;
    QLabel*                                             m_pStage2PriorLabel;
    QCheckBox*                                          m_pStage3CheckBox;
    QSpinBox*                                           m_pStage3MeanfieldOrderSpinBox;
    QSpinBox*                                           m_pStage3MagKMaxSpinBox;
    QSpinBox*                                           m_pStage3GradKMaxSpinBox;
    QDoubleSpinBox*                                     m_pStage3MagStrengthSpinBox;
    QDoubleSpinBox*                                     m_pStage3GradStrengthSpinBox;
    QPlainTextEdit*                                     m_pPipelineEdit;
    QPlainTextEdit*                                     m_pStage1StatusEdit;
    QPlainTextEdit*                                     m_pStage2StatusEdit;
    QPlainTextEdit*                                     m_pStage3StatusEdit;
    QPlainTextEdit*                                     m_pHpiEdit;
    QPlainTextEdit*                                     m_pSnrEdit;
};

} // namespace AOEMEGPLUGIN

#endif // AOEMEGSTATUSWIDGET_H

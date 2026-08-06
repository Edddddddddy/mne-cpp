//=============================================================================================================
/**
 * @file     adaptivedenoisingsetupwidget.h
 * @brief    Programmatic teaching controls and diagnostics for adaptive denoising.
 */

#ifndef ADAPTIVEDENOISINGSETUPWIDGET_ADAPTIVEDENOISINGPLUGIN_H
#define ADAPTIVEDENOISINGSETUPWIDGET_ADAPTIVEDENOISINGPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingdiagnostics.h"

#include <QtWidgets/QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

class AdaptiveDenoisingSetupWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit AdaptiveDenoisingSetupWidget(QWidget* parent = nullptr);

public slots:
    void setDiagnostics(const AdaptiveDenoisingDiagnostics& diagnostics);

signals:
    void enabledChanged(bool enabled);
    void frozenChanged(bool frozen);
    void tapCountChanged(int tapCount);
    void adaptationIntervalChanged(int samples);
    void memoryTimeSecondsChanged(double seconds);
    void regularizationChanged(double regularization);
    void resetRequested();

private:
    QCheckBox*      m_enabledCheckBox;
    QSpinBox*       m_tapCountSpinBox;
    QSpinBox*       m_adaptationIntervalSpinBox;
    QDoubleSpinBox* m_memoryTimeSecondsSpinBox;
    QDoubleSpinBox* m_regularizationSpinBox;
    QCheckBox*      m_frozenCheckBox;
    QPushButton*    m_resetButton;

    QLabel* m_pluginStateValue;
    QLabel* m_configureStatusValue;
    QLabel* m_processStatusValue;
    QLabel* m_referenceCountValue;
    QLabel* m_targetCountValue;
    QLabel* m_featureCountValue;
    QLabel* m_warmupSamplesValue;
    QLabel* m_modelGenerationValue;
    QLabel* m_acceptedUpdatesValue;
    QLabel* m_rejectedUpdatesValue;
    QLabel* m_inputRmsValue;
    QLabel* m_outputRmsValue;
    QLabel* m_estimatedNoiseRmsValue;
    QLabel* m_droppedBlocksValue;
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISINGSETUPWIDGET_ADAPTIVEDENOISINGPLUGIN_H

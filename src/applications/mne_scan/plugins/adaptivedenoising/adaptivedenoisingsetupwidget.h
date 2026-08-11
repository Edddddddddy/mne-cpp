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
#include "adaptivedenoisingvisualizationmodel.h"

#include <QtWidgets/QWidget>

#include <cstdint>
#include <memory>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimer;

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

class AdaptiveDenoisingTraceWidget;

//=============================================================================================================

class AdaptiveDenoisingSetupWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit AdaptiveDenoisingSetupWidget(QWidget* parent = nullptr);
    explicit AdaptiveDenoisingSetupWidget(
        std::shared_ptr<AdaptiveDenoisingVisualizationModel> visualizationModel,
        QWidget* parent = nullptr);

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
    void refreshVisualization();

    std::shared_ptr<AdaptiveDenoisingVisualizationModel> m_visualizationModel;

    QCheckBox*      m_enabledCheckBox;
    QSpinBox*       m_tapCountSpinBox;
    QSpinBox*       m_adaptationIntervalSpinBox;
    QDoubleSpinBox* m_memoryTimeSecondsSpinBox;
    QDoubleSpinBox* m_regularizationSpinBox;
    QCheckBox*      m_frozenCheckBox;
    QPushButton*    m_resetButton;
    QSpinBox*       m_visualizationTargetSpinBox;
    AdaptiveDenoisingTraceWidget* m_visualizationTraceWidget;
    QTimer*         m_visualizationRefreshTimer;
    QLabel*         m_visualizationTargetValue;
    QLabel*         m_visualizationSamplesValue;
    QLabel*         m_visualizationSequenceValue;
    std::uint64_t   m_lastVisualizationSequence;

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

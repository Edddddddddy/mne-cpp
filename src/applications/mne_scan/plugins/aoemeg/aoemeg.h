//=============================================================================================================
/**
 * @file     aoemeg.h
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Contains the declaration of the AOEMeg plugin class.
 *
 */

#ifndef AOEMEG_H
#define AOEMEG_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "aoemeg_global.h"

#include <scShared/Plugins/abstractalgorithm.h>
#include <utils/generics/circularbuffer.h>

#include <rtprocessing/rtaoemeg.h>

#include <QFutureWatcher>
#include <QMutex>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffInfo;
}

namespace SCMEASLIB
{
    class RealTimeHpiResult;
    class RealTimeMultiSampleArray;
}

namespace AOEMEGPLUGIN
{

class AOEMEGSHARED_EXPORT AOEMeg : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "aoemeg.json")
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

public:
    AOEMeg();
    ~AOEMeg();

    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    RTPROCESSINGLIB::AOEMegSettings getCurrentSettings() const;
    QString getConfigurationSummary() const;
    QString getRuntimeStatusSummary() const;
    QString getPipelineStatusSummary() const;
    QString getStage1StatusSummary() const;
    QString getStage2StatusSummary() const;
    QString getStage3StatusSummary() const;
    QString getHpiStatusSummary() const;
    QString getSnrSummary() const;
    QString getStage2PriorSummary() const;

public slots:
    void setRuntimeSettings(const RTPROCESSINGLIB::AOEMegSettings& settings);
    bool loadStage2PriorFromFile(const QString& sFilePath);
    void clearStage2Prior();

protected:
    virtual void initPluginControlWidgets();
    virtual void run();

private:
    QString composeRuntimeStatusSummaryUnlocked() const;
    QString composePipelineStatusSummaryUnlocked() const;
    QString composeStage1StatusSummaryUnlocked() const;
    QString composeStage2StatusSummaryUnlocked() const;
    QString composeStage3StatusSummaryUnlocked() const;
    QString composeHpiStatusSummaryUnlocked() const;
    QString composeSnrSummaryUnlocked() const;
    QString composeStage2PriorSummaryUnlocked() const;
    void emitRuntimeViews();

private slots:
    void onSnrComputationFinished();

private:
    mutable QMutex                                                              m_qMutex;
    QSharedPointer<FIFFLIB::FiffInfo>                                           m_pFiffInfo;
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                      m_pCircularBuffer;
    QSharedPointer<RTPROCESSINGLIB::RtAoeMeg>                                   m_pRtAoeMeg;
    qint64                                                                      m_iHpiPacketsReceived = 0;
    qint64                                                                      m_iHpiInitializedPackets = 0;
    bool                                                                        m_bLastHpiInitialized = false;
    bool                                                                        m_bLastDevHeadTransEmpty = true;
    bool                                                                        m_bLastLargeHeadMovement = false;
    double                                                                      m_dLastHeadMovementDistanceMm = 0.0;
    double                                                                      m_dLastHeadMovementAngleDeg = 0.0;
    double                                                                      m_dLastTransformTranslationNormMm = 0.0;
    double                                                                      m_dLastTransformRotationNormDeg = 0.0;
    bool                                                                        m_bStaticDevHeadTPresent = false;
    int                                                                         m_iStaticDigPointCount = 0;
    QString                                                                     m_sLastHpiStatus = "No HPI packet received";
    QFutureWatcher<QString>                                                     m_sSnrWatcher;
    QString                                                                     m_sSnrSummary = "Runtime effect metrics waiting";
    QString                                                                     m_sStage2PriorFilePath;
    int                                                                         m_iStage2PriorProjCount = 0;

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeHpiResult>::SPtr            m_pHpiInput;
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pInput;
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr    m_pOutput;

signals:
    void runtimeStatusChanged(const QString& sStatusText);
    void runtimeSectionsChanged(const QString& sPipelineText,
                                const QString& sStage1Text,
                                const QString& sStage2Text,
                                const QString& sStage3Text,
                                const QString& sHpiText);
    void runtimeSnrChanged(const QString& sSnrText);
    void stage2PriorChanged(const QString& sPriorText);
};

} // namespace AOEMEGPLUGIN

#endif // AOEMEG_H

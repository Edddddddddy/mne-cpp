//=============================================================================================================
/**
 * @file     aoemeg.cpp
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Definition of the AOEMeg class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "aoemeg.h"

#include "FormFiles/aoemegsetupwidget.h"
#include "FormFiles/aoemegstatuswidget.h"

#include <scMeas/realtimehpiresult.h>
#include <scMeas/realtimemultisamplearray.h>

#include <inverse/hpiFit/hpifit.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>

#include <Eigen/Geometry>

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QFileInfo>
#include <QtConcurrent>
#include <QMutexLocker>
#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AOEMEGPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace Eigen;

namespace
{

VectorXd devHeadTransToMotion6(const FiffCoordTrans& devHeadTrans)
{
    VectorXd motion(6);
    motion.setZero();

    if(devHeadTrans.isEmpty()) {
        return motion;
    }

    const Matrix3d rotation = devHeadTrans.trans.block<3,3>(0,0).cast<double>();
    const AngleAxisd angleAxis(rotation);
    Vector3d rotationVector = Vector3d::Zero();

    if(std::isfinite(angleAxis.angle()) && angleAxis.axis().allFinite()) {
        rotationVector = angleAxis.axis() * angleAxis.angle();
    }

    motion.segment<3>(0) = devHeadTrans.trans.block<3,1>(0,3).cast<double>();
    motion.segment<3>(3) = rotationVector;

    return motion;
}

QString formatDiagnostics(const AOEMegDiagnostics& diagnostics)
{
    return QString("%1\n%2\n%3\n%4")
            .arg(diagnostics.sStatusText)
            .arg(QString("Pipeline: blocks=%1 samples=%2 motion=%3")
                 .arg(diagnostics.iProcessedBlocks)
                 .arg(diagnostics.iReceivedSamples)
                 .arg(diagnostics.bMotionInputAvailable ? "ON" : "WAIT"))
            .arg(QString("Stage1: status=%1 freeze=%2 rho=%3 update=%4 accept=%5 residual=%6")
                 .arg(diagnostics.sStage1Status)
                 .arg(diagnostics.bStage1Frozen ? "YES" : "NO")
                 .arg(diagnostics.dRhoStage1, 0, 'f', 4)
                 .arg(diagnostics.dStage1UpdateRate, 0, 'f', 3)
                 .arg(diagnostics.dStage1ChunkAcceptRate, 0, 'f', 3)
                 .arg(diagnostics.dStage1ResidualAcceptRatio, 0, 'f', 3))
            .arg(QString("Stage2: status=%1 skipped=%2 rho=%3 mag(rho=%4 rank=%5/%6 cov=%7 apply=%8) grad(rho=%9 rank=%10/%11 cov=%12 apply=%13)")
                 .arg(diagnostics.sStage2Status)
                 .arg(diagnostics.bStage2UpdateSkipped ? "YES" : "NO")
                 .arg(diagnostics.dRhoStage2, 0, 'f', 4)
                 .arg(diagnostics.dRhoStage2Mag, 0, 'f', 4)
                 .arg(diagnostics.iStage2ActiveRankMag)
                 .arg(diagnostics.iStage2RequestedRankMag)
                 .arg(diagnostics.dStage2CovTraceMag, 0, 'e', 2)
                 .arg(diagnostics.bStage2AppliedMag ? "YES" : "NO")
                 .arg(diagnostics.dRhoStage2Grad, 0, 'f', 4)
                 .arg(diagnostics.iStage2ActiveRankGrad)
                 .arg(diagnostics.iStage2RequestedRankGrad)
                 .arg(diagnostics.dStage2CovTraceGrad, 0, 'e', 2)
                 .arg(diagnostics.bStage2AppliedGrad ? "YES" : "NO"));
}

QString formatPipelineSection(const AOEMegDiagnostics& diagnostics)
{
    return QString("Pipeline status=%1\nblocks=%2\nsamples=%3\nmotion input=%4")
            .arg(diagnostics.sStatusText)
            .arg(diagnostics.iProcessedBlocks)
            .arg(diagnostics.iReceivedSamples)
            .arg(diagnostics.bMotionInputAvailable ? "ON" : "WAIT");
}

QString formatStage1Section(const AOEMegDiagnostics& diagnostics)
{
    return QString("status=%1\nenabled=%2\nfreeze=%3\nrho=%4\nupdate rate=%5\naccept rate=%6\nresidual ratio=%7\nclip rate=%8\nu norm mean=%9")
            .arg(diagnostics.sStage1Status)
            .arg(diagnostics.bStage1Enabled ? "ON" : "OFF")
            .arg(diagnostics.bStage1Frozen ? "YES" : "NO")
            .arg(diagnostics.dRhoStage1, 0, 'f', 4)
            .arg(diagnostics.dStage1UpdateRate, 0, 'f', 3)
            .arg(diagnostics.dStage1ChunkAcceptRate, 0, 'f', 3)
            .arg(diagnostics.dStage1ResidualAcceptRatio, 0, 'f', 3)
            .arg(diagnostics.dStage1ClipRate, 0, 'f', 3)
            .arg(diagnostics.dStage1UNormMean, 0, 'f', 3);
}

QString formatStage2Section(const AOEMegDiagnostics& diagnostics)
{
    return QString("status=%1\nenabled=%2\nprior=%3 loaded=%4\nupdate skipped=%5\nrho=%6\nmag: rho=%7 prior=%8 rank=%9/%10 cov=%11 cov_update=%12 apply=%13\ngrad: rho=%14 prior=%15 rank=%16/%17 cov=%18 cov_update=%19 apply=%20")
            .arg(diagnostics.sStage2Status)
            .arg(diagnostics.bStage2Enabled ? "ON" : "OFF")
            .arg(diagnostics.sStage2PriorStatus)
            .arg(diagnostics.bStage2PriorLoaded ? "YES" : "NO")
            .arg(diagnostics.bStage2UpdateSkipped ? "YES" : "NO")
            .arg(diagnostics.dRhoStage2, 0, 'f', 4)
            .arg(diagnostics.dRhoStage2Mag, 0, 'f', 4)
            .arg(diagnostics.iStage2PriorRankMag)
            .arg(diagnostics.iStage2ActiveRankMag)
            .arg(diagnostics.iStage2RequestedRankMag)
            .arg(diagnostics.dStage2CovTraceMag, 0, 'e', 2)
            .arg(diagnostics.bStage2CovUpdatedMag ? "YES" : "NO")
            .arg(diagnostics.bStage2AppliedMag ? "YES" : "NO")
            .arg(diagnostics.dRhoStage2Grad, 0, 'f', 4)
            .arg(diagnostics.iStage2PriorRankGrad)
            .arg(diagnostics.iStage2ActiveRankGrad)
            .arg(diagnostics.iStage2RequestedRankGrad)
            .arg(diagnostics.dStage2CovTraceGrad, 0, 'e', 2)
            .arg(diagnostics.bStage2CovUpdatedGrad ? "YES" : "NO")
            .arg(diagnostics.bStage2AppliedGrad ? "YES" : "NO");
}

QString formatStage3Section(const AOEMegDiagnostics& diagnostics)
{
    return QString("status=%1\nenabled=%2\nupdate skipped=%3\nrho=%4\nmag: rho=%5 sel=%6 phys=%7 stat=%8 comb=%9 cov=%10 band=%11 cov_update=%12 apply=%13\ngrad: rho=%14 sel=%15 phys=%16 stat=%17 comb=%18 cov=%19 band=%20 cov_update=%21 apply=%22")
            .arg(diagnostics.sStage3Status)
            .arg(diagnostics.bStage3Enabled ? "ON" : "OFF")
            .arg(diagnostics.bStage3UpdateSkipped ? "YES" : "NO")
            .arg(diagnostics.dRhoStage3, 0, 'f', 4)
            .arg(diagnostics.dRhoStage3Mag, 0, 'f', 4)
            .arg(diagnostics.iStage3SelectedMag)
            .arg(diagnostics.iStage3PhysicalRankMag)
            .arg(diagnostics.iStage3StatRankMag)
            .arg(diagnostics.iStage3CombinedRankMag)
            .arg(diagnostics.dStage3CovTraceMag, 0, 'e', 2)
            .arg(diagnostics.dStage3BandPowerMag, 0, 'e', 2)
            .arg(diagnostics.bStage3CovUpdatedMag ? "YES" : "NO")
            .arg(diagnostics.bStage3AppliedMag ? "YES" : "NO")
            .arg(diagnostics.dRhoStage3Grad, 0, 'f', 4)
            .arg(diagnostics.iStage3SelectedGrad)
            .arg(diagnostics.iStage3PhysicalRankGrad)
            .arg(diagnostics.iStage3StatRankGrad)
            .arg(diagnostics.iStage3CombinedRankGrad)
            .arg(diagnostics.dStage3CovTraceGrad, 0, 'e', 2)
            .arg(diagnostics.dStage3BandPowerGrad, 0, 'e', 2)
            .arg(diagnostics.bStage3CovUpdatedGrad ? "YES" : "NO")
            .arg(diagnostics.bStage3AppliedGrad ? "YES" : "NO");
}

QString formatHpiSection(qint64 packetsReceived,
                         qint64 packetsInitialized,
                         const QString& lastStatus,
                         double lastMoveMm,
                         double lastMoveDeg,
                         double absTransMm,
                         double absRotDeg,
                         bool largeMovement,
                         bool hasStaticDevHeadT,
                         int digCount)
{
    return QString("packets=%1\ninitialized=%2\nstate=%3\nlast move=%4 mm / %5 deg\nabs trans=%6 mm\nabs rot=%7 deg\nlarge=%8\nstatic dev_head_t=%9\ndig points=%10\nhpi_meas field=C++ model N/A")
            .arg(packetsReceived)
            .arg(packetsInitialized)
            .arg(lastStatus)
            .arg(lastMoveMm, 0, 'f', 3)
            .arg(lastMoveDeg, 0, 'f', 3)
            .arg(absTransMm, 0, 'f', 3)
            .arg(absRotDeg, 0, 'f', 3)
            .arg(largeMovement ? "YES" : "NO")
            .arg(hasStaticDevHeadT ? "YES" : "NO")
            .arg(digCount);
}

double rotationVectorNormDeg(const VectorXd& motion6)
{
    if(motion6.size() < 6) {
        return 0.0;
    }

    return motion6.segment(3, 3).norm() * 180.0 / 3.14159265358979323846;
}

double percentileFromVector(VectorXd values, double q01)
{
    if(values.size() == 0) {
        return 0.0;
    }

    q01 = std::max(0.0, std::min(1.0, q01));
    const qint64 index = static_cast<qint64>(std::floor(q01 * static_cast<double>(values.size() - 1)));
    std::nth_element(values.data(), values.data() + index, values.data() + values.size());
    return values(index);
}

QString computeEffectSummary(const MatrixXd& before,
                             const MatrixXd& after,
                             const AOEMegDiagnostics& diagnostics)
{
    if(before.size() == 0 || after.size() == 0 || before.rows() != after.rows() || before.cols() != after.cols()) {
        return "Runtime effect metrics unavailable";
    }

    const MatrixXd removed = before - after;
    const double rmsBefore = std::sqrt(before.array().square().mean());
    const double rmsAfter = std::sqrt(after.array().square().mean());
    const double rmsRemoved = std::sqrt(removed.array().square().mean()) + 1e-18;
    const double totalRho = 1.0 - (after.squaredNorm() + 1e-18) / (before.squaredNorm() + 1e-18);
    const double retainedEnergy = 100.0 * (after.squaredNorm() + 1e-18) / (before.squaredNorm() + 1e-18);
    const double removedEnergy = 100.0 * (removed.squaredNorm() + 1e-18) / (before.squaredNorm() + 1e-18);
    const double corr = (before.array() * after.array()).mean() / ((rmsBefore * rmsAfter) + 1e-18);
    const MatrixXd matAbsRemoved = removed.cwiseAbs().matrix();
    const VectorXd absRemoved = Eigen::Map<const VectorXd>(matAbsRemoved.data(), matAbsRemoved.size());
    const double p95Removed = percentileFromVector(absRemoved, 0.95);
    const double p99Removed = percentileFromVector(absRemoved, 0.99);
    const double maxRemoved = absRemoved.size() > 0 ? absRemoved.maxCoeff() : 0.0;

    int changedChannels = 0;
    for(Index row = 0; row < before.rows(); ++row) {
        const double rowBefore = std::sqrt(before.row(row).array().square().mean());
        const double rowRemoved = std::sqrt(removed.row(row).array().square().mean());
        if(rowRemoved > 0.01 * rowBefore + 1e-18) {
            ++changedChannels;
        }
    }

    return QString("Effect: total rho=%1 %% | retain=%2 %% | removed=%3 %%\nDelta: rms=%4 | p95=%5 | p99=%6 | max=%7\nChanged channels (>1%% rms)=%8/%9 | corr=%10\nS1: %11 rho=%12 %% | S2: %13 rho=%14 %% apply(M/G)=%15/%16 rank(M/G)=%17/%18 | S3: %19 rho=%20 %% apply(M/G)=%21/%22 sel(M/G)=%23/%24")
            .arg(100.0 * totalRho, 0, 'f', 4)
            .arg(retainedEnergy, 0, 'f', 2)
            .arg(removedEnergy, 0, 'f', 2)
            .arg(rmsRemoved, 0, 'e', 2)
            .arg(p95Removed, 0, 'e', 2)
            .arg(p99Removed, 0, 'e', 2)
            .arg(maxRemoved, 0, 'e', 2)
            .arg(changedChannels)
            .arg(before.rows())
            .arg(corr, 0, 'f', 4)
            .arg(diagnostics.sStage1Status)
            .arg(100.0 * diagnostics.dRhoStage1, 0, 'f', 4)
            .arg(diagnostics.sStage2Status)
            .arg(100.0 * diagnostics.dRhoStage2, 0, 'f', 4)
            .arg(diagnostics.bStage2AppliedMag ? "Y" : "N")
            .arg(diagnostics.bStage2AppliedGrad ? "Y" : "N")
            .arg(diagnostics.iStage2ActiveRankMag)
            .arg(diagnostics.iStage2ActiveRankGrad)
            .arg(diagnostics.sStage3Status)
            .arg(100.0 * diagnostics.dRhoStage3, 0, 'f', 4)
            .arg(diagnostics.bStage3AppliedMag ? "Y" : "N")
            .arg(diagnostics.bStage3AppliedGrad ? "Y" : "N")
            .arg(diagnostics.iStage3SelectedMag)
            .arg(diagnostics.iStage3SelectedGrad);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AOEMeg::AOEMeg()
{
    connect(&m_sSnrWatcher, &QFutureWatcher<QString>::finished,
            this, &AOEMeg::onSnrComputationFinished);
}

//=============================================================================================================

AOEMeg::~AOEMeg()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> AOEMeg::clone() const
{
    QSharedPointer<AOEMeg> pClone(new AOEMeg);
    return pClone;
}

//=============================================================================================================

void AOEMeg::init()
{
    m_pCircularBuffer = QSharedPointer<CircularBuffer_Matrix_double>::create(40);
    m_pRtAoeMeg = QSharedPointer<RtAoeMeg>::create();

    m_pHpiInput = PluginInputData<RealTimeHpiResult>::create(this, "AOEMegHPIIn", "AOE-MEG real-time HPI input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &AOEMeg::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pHpiInput);

    m_pInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "AOEMegIn", "AOE-MEG input data");
    connect(m_pInput.data(), &PluginInputConnector::notify,
            this, &AOEMeg::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pInput);

    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "AOEMegOut", "AOE-MEG output data");
    m_pOutput->measurementData()->setName(this->getName());
    m_outputConnectors.append(m_pOutput);
}

//=============================================================================================================

void AOEMeg::unload()
{
}

//=============================================================================================================

bool AOEMeg::start()
{
    if(!this->isRunning()) {
        QThread::start();
    }

    return true;
}

//=============================================================================================================

bool AOEMeg::stop()
{
    requestInterruption();
    wait(500);

    if(m_pOutput) {
        m_pOutput->measurementData()->clear();
    }

    if(m_pCircularBuffer) {
        m_pCircularBuffer->clear();
    }

    if(m_pRtAoeMeg) {
        m_pRtAoeMeg->reset();
        if(m_pFiffInfo) {
            m_pRtAoeMeg->setFiffInfo(m_pFiffInfo);
        }
    }

    m_iHpiPacketsReceived = 0;
    m_iHpiInitializedPackets = 0;
    m_bLastHpiInitialized = false;
    m_bLastDevHeadTransEmpty = true;
    m_bLastLargeHeadMovement = false;
    m_dLastHeadMovementDistanceMm = 0.0;
    m_dLastHeadMovementAngleDeg = 0.0;
    m_dLastTransformTranslationNormMm = 0.0;
    m_dLastTransformRotationNormDeg = 0.0;
    m_bStaticDevHeadTPresent = false;
    m_iStaticDigPointCount = 0;
    m_sLastHpiStatus = "No HPI packet received";
    m_sSnrSummary = "Runtime effect metrics waiting";

    m_bPluginControlWidgetsInit = false;
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType AOEMeg::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString AOEMeg::getName() const
{
    return "AOE-MEG";
}

//=============================================================================================================

QWidget* AOEMeg::setupWidget()
{
    return new AOEMegSetupWidget(this);
}

//=============================================================================================================

QString AOEMeg::getBuildInfo()
{
    return QString(AOEMEGPLUGIN::buildDateTime()) + QString(" - ") + QString(AOEMEGPLUGIN::buildHash());
}

//=============================================================================================================

void AOEMeg::update(Measurement::SPtr pMeasurement)
{
    QString sStatusUpdate;

    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        if(!m_pFiffInfo) {
            QMutexLocker locker(&m_qMutex);

            if(!m_pFiffInfo) {
                m_pFiffInfo = pRTMSA->info();
                m_bStaticDevHeadTPresent = m_pFiffInfo && !m_pFiffInfo->dev_head_t.isEmpty();
                m_iStaticDigPointCount = m_pFiffInfo ? m_pFiffInfo->dig.size() : 0;

                m_pOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
                m_pOutput->measurementData()->setMultiArraySize(1);

                if(m_pRtAoeMeg) {
                    m_pRtAoeMeg->setFiffInfo(m_pFiffInfo);
                }
            }
        }

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                // Wait until the worker consumed enough data.
            }
        }
    } else if(QSharedPointer<RealTimeHpiResult> pRTHPI = pMeasurement.dynamicCast<RealTimeHpiResult>()) {
        QMutexLocker locker(&m_qMutex);
        ++m_iHpiPacketsReceived;
        m_bLastHpiInitialized = pRTHPI->isInitialized();

        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTHPI->getFiffInfo();
            if(m_pFiffInfo && m_pRtAoeMeg) {
                m_bStaticDevHeadTPresent = !m_pFiffInfo->dev_head_t.isEmpty();
                m_iStaticDigPointCount = m_pFiffInfo->dig.size();
                m_pRtAoeMeg->setFiffInfo(m_pFiffInfo);
            }
        }

        if(!m_bLastHpiInitialized) {
            if(m_pRtAoeMeg) {
                m_pRtAoeMeg->setMotionVector(VectorXd(), false);
            }
            m_sLastHpiStatus = "HPI packet received but measurement is not initialized yet";
        } else {
            ++m_iHpiInitializedPackets;

            QSharedPointer<HpiFitResult> pFitResult = pRTHPI->getValue();
            if(!pFitResult) {
                if(m_pRtAoeMeg) {
                    m_pRtAoeMeg->setMotionVector(VectorXd(), false);
                }
                m_sLastHpiStatus = "HPI measurement initialized but fit result pointer is null";
            } else {
                m_bLastDevHeadTransEmpty = pFitResult->devHeadTrans.isEmpty();
                m_bLastLargeHeadMovement = pFitResult->bIsLargeHeadMovement;
                m_dLastHeadMovementDistanceMm = pFitResult->fHeadMovementDistance;
                m_dLastHeadMovementAngleDeg = pFitResult->fHeadMovementAngle;

                const VectorXd motion6 = devHeadTransToMotion6(pFitResult->devHeadTrans);
                m_dLastTransformTranslationNormMm = motion6.size() >= 3 ? motion6.head(3).norm() * 1000.0 : 0.0;
                m_dLastTransformRotationNormDeg = rotationVectorNormDeg(motion6);

                if(m_bLastDevHeadTransEmpty) {
                    if(m_pRtAoeMeg) {
                        m_pRtAoeMeg->setMotionVector(VectorXd(), false);
                    }
                    m_sLastHpiStatus = "HPI fit exists but devHeadTrans is empty";
                } else {
                    if(m_pRtAoeMeg) {
                        m_pRtAoeMeg->setMotionVector(motion6, true);
                    }

                    if(std::abs(m_dLastHeadMovementDistanceMm) < 0.01
                            && std::abs(m_dLastHeadMovementAngleDeg) < 0.01) {
                        m_sLastHpiStatus = "HPI transform is valid, but relative movement is currently very small";
                    } else {
                        m_sLastHpiStatus = "HPI transform is valid and motion is available";
                    }
                }
            }
        }

        sStatusUpdate = composeRuntimeStatusSummaryUnlocked();

        if(!m_bPluginControlWidgetsInit && m_pFiffInfo) {
            initPluginControlWidgets();
        }
    }

    if(!sStatusUpdate.isEmpty()) {
        Q_UNUSED(sStatusUpdate)
        emitRuntimeViews();
    }
}

//=============================================================================================================

RTPROCESSINGLIB::AOEMegSettings AOEMeg::getCurrentSettings() const
{
    QMutexLocker locker(&m_qMutex);
    if(m_pRtAoeMeg) {
        return m_pRtAoeMeg->settings();
    }

    return AOEMegSettings();
}

//=============================================================================================================

QString AOEMeg::getConfigurationSummary() const
{
    const AOEMegSettings settings = getCurrentSettings();

    return QString("Chunk %1/%2, window=%3\nStage-1=%4 (lambda=%5, coeff_clip=%6, pred_clip=%7)\nStage-2=%8 (mag rank=%9, grad rank=%10, mag strength=%11, grad strength=%12, band=%13-%14 Hz, update=%15, reg=%16, prior=%17 mag/grad=%18/%19)\nStage-3=%20 (%21, meanfield order=%22, mag kmax=%23, grad kmax=%24)\nMotion input=RealTimeHpiResult")
            .arg(settings.iChunkSize)
            .arg(settings.iChunkOverlap)
            .arg(settings.sWindow)
            .arg(settings.stage1.bEnabled ? "ON" : "OFF")
            .arg(settings.stage1.dRlsLambda, 0, 'f', 4)
            .arg(settings.stage1.dRlsCoeffClip, 0, 'f', 2)
            .arg(settings.stage1.dRlsPredClipRatio, 0, 'f', 2)
            .arg(settings.stage2.bEnabled ? "ON" : "OFF")
            .arg(settings.stage2.mag.iSspRank)
            .arg(settings.stage2.grad.iSspRank)
            .arg(settings.stage2.mag.dStrength, 0, 'f', 2)
            .arg(settings.stage2.grad.dStrength, 0, 'f', 2)
            .arg(settings.stage2.grad.dBandLowHz, 0, 'f', 1)
            .arg(settings.stage2.grad.dBandHighHz, 0, 'f', 1)
            .arg(settings.stage2.iUpdateEveryChunks)
            .arg(settings.stage2.dRegularization, 0, 'e', 1)
            .arg(settings.stage2.bUsePrior ? "ON" : "OFF")
            .arg(settings.stage2.iPriorRankMag)
            .arg(settings.stage2.iPriorRankGrad)
            .arg(settings.stage3.bEnabled ? "ON" : "OFF")
            .arg(settings.stage3.sApplyMode)
            .arg(settings.stage3.iMeanfieldOrder)
            .arg(settings.stage3.mag.iKMax)
            .arg(settings.stage3.grad.iKMax);
}

//=============================================================================================================

QString AOEMeg::getRuntimeStatusSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeRuntimeStatusSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getPipelineStatusSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composePipelineStatusSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getStage1StatusSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeStage1StatusSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getStage2StatusSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeStage2StatusSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getStage3StatusSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeStage3StatusSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getHpiStatusSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeHpiStatusSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getSnrSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeSnrSummaryUnlocked();
}

//=============================================================================================================

QString AOEMeg::getStage2PriorSummary() const
{
    QMutexLocker locker(&m_qMutex);
    return composeStage2PriorSummaryUnlocked();
}

//=============================================================================================================

void AOEMeg::setRuntimeSettings(const AOEMegSettings& settings)
{
    {
        QMutexLocker locker(&m_qMutex);
        if(m_pRtAoeMeg) {
            m_pRtAoeMeg->setSettings(settings);
            if(m_pFiffInfo) {
                m_pRtAoeMeg->setFiffInfo(m_pFiffInfo);
            }
        }
    }

    emitRuntimeViews();
    emit stage2PriorChanged(getStage2PriorSummary());
}

//=============================================================================================================

bool AOEMeg::loadStage2PriorFromFile(const QString& sFilePath)
{
    if(sFilePath.isEmpty()) {
        return false;
    }

    QFile projFile(sFilePath);
    FiffStream::SPtr pStream(new FiffStream(&projFile));
    if(!pStream->open()) {
        return false;
    }

    QList<FiffProj> projs = pStream->read_proj(pStream->dirtree());
    pStream->close();

    if(projs.isEmpty()) {
        return false;
    }

    {
        QMutexLocker locker(&m_qMutex);
        m_sStage2PriorFilePath = sFilePath;
        m_iStage2PriorProjCount = projs.size();

        if(m_pRtAoeMeg) {
            m_pRtAoeMeg->setStage2PriorProjectors(projs);
        }
    }

    emitRuntimeViews();
    emit stage2PriorChanged(getStage2PriorSummary());
    return true;
}

//=============================================================================================================

void AOEMeg::clearStage2Prior()
{
    {
        QMutexLocker locker(&m_qMutex);
        m_sStage2PriorFilePath.clear();
        m_iStage2PriorProjCount = 0;

        if(m_pRtAoeMeg) {
            m_pRtAoeMeg->clearStage2PriorProjectors();
        }
    }

    emitRuntimeViews();
    emit stage2PriorChanged(getStage2PriorSummary());
}

//=============================================================================================================

QString AOEMeg::composeRuntimeStatusSummaryUnlocked() const
{
    QStringList parts;
    parts << composePipelineStatusSummaryUnlocked()
          << composeStage1StatusSummaryUnlocked()
          << composeStage2StatusSummaryUnlocked()
          << composeStage3StatusSummaryUnlocked()
          << composeHpiStatusSummaryUnlocked();
    return parts.join("\n\n");
}

//=============================================================================================================

QString AOEMeg::composePipelineStatusSummaryUnlocked() const
{
    if(!m_pRtAoeMeg) {
        return "Pipeline\nruntime core waiting";
    }

    return QString("Pipeline\n%1")
            .arg(formatPipelineSection(m_pRtAoeMeg->diagnostics()));
}

//=============================================================================================================

QString AOEMeg::composeStage1StatusSummaryUnlocked() const
{
    if(!m_pRtAoeMeg) {
        return "Stage-1\nruntime core waiting";
    }

    return QString("Stage-1\n%1")
            .arg(formatStage1Section(m_pRtAoeMeg->diagnostics()));
}

//=============================================================================================================

QString AOEMeg::composeStage2StatusSummaryUnlocked() const
{
    if(!m_pRtAoeMeg) {
        return "Stage-2\nruntime core waiting";
    }

    return QString("Stage-2\n%1")
            .arg(formatStage2Section(m_pRtAoeMeg->diagnostics()));
}

//=============================================================================================================

QString AOEMeg::composeStage3StatusSummaryUnlocked() const
{
    if(!m_pRtAoeMeg) {
        return "Stage-3\nruntime core waiting";
    }

    return QString("Stage-3\n%1")
            .arg(formatStage3Section(m_pRtAoeMeg->diagnostics()));
}

//=============================================================================================================

QString AOEMeg::composeHpiStatusSummaryUnlocked() const
{
    return QString("HPI\n%1")
            .arg(formatHpiSection(m_iHpiPacketsReceived,
                                  m_iHpiInitializedPackets,
                                  m_sLastHpiStatus,
                                  m_dLastHeadMovementDistanceMm,
                                  m_dLastHeadMovementAngleDeg,
                                  m_dLastTransformTranslationNormMm,
                                  m_dLastTransformRotationNormDeg,
                                  m_bLastLargeHeadMovement,
                                  m_bStaticDevHeadTPresent,
                                  m_iStaticDigPointCount));
}

//=============================================================================================================

QString AOEMeg::composeSnrSummaryUnlocked() const
{
    return m_sSnrSummary;
}

//=============================================================================================================

QString AOEMeg::composeStage2PriorSummaryUnlocked() const
{
    if(m_sStage2PriorFilePath.isEmpty()) {
        return "Stage-2 prior: none";
    }

    return QString("Stage-2 prior: %1 (%2 proj items)")
            .arg(QFileInfo(m_sStage2PriorFilePath).fileName())
            .arg(m_iStage2PriorProjCount);
}

//=============================================================================================================

void AOEMeg::emitRuntimeViews()
{
    const QString sRuntimeStatus = getRuntimeStatusSummary();
    const QString sPipeline = getPipelineStatusSummary();
    const QString sStage1 = getStage1StatusSummary();
    const QString sStage2 = getStage2StatusSummary();
    const QString sStage3 = getStage3StatusSummary();
    const QString sHpi = getHpiStatusSummary();

    emit runtimeStatusChanged(sRuntimeStatus);
    emit runtimeSectionsChanged(sPipeline, sStage1, sStage2, sStage3, sHpi);
}

//=============================================================================================================

void AOEMeg::onSnrComputationFinished()
{
    {
        QMutexLocker locker(&m_qMutex);
        m_sSnrSummary = m_sSnrWatcher.result();
    }

    emit runtimeSnrChanged(getSnrSummary());
}

//=============================================================================================================

void AOEMeg::initPluginControlWidgets()
{
    if(!m_pFiffInfo) {
        return;
    }

    QList<QWidget*> plControlWidgets;

    AOEMegStatusWidget* pStatusWidget = new AOEMegStatusWidget(this);
    pStatusWidget->setObjectName("group_tab_AOEMeg_Runtime");
    pStatusWidget->setStatusText(getRuntimeStatusSummary());
    plControlWidgets.append(pStatusWidget);

    emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
    m_bPluginControlWidgetsInit = true;
}

//=============================================================================================================

void AOEMeg::run()
{
    MatrixXd matData;

    while(!isInterruptionRequested()) {
        {
            QMutexLocker locker(&m_qMutex);
            if(m_pFiffInfo) {
                break;
            }
        }

        msleep(10);
    }

    while(!isInterruptionRequested()) {
        if(m_pCircularBuffer && m_pCircularBuffer->pop(matData)) {
            AOEMegProcessResult result;

            {
                QMutexLocker locker(&m_qMutex);
                if(!m_pRtAoeMeg) {
                    continue;
                }

                result = m_pRtAoeMeg->process(matData);
            }

            if(!isInterruptionRequested()) {
                m_pOutput->measurementData()->setValue(result.matData);
                emitRuntimeViews();

                if(!m_sSnrWatcher.isRunning()) {
                    const MatrixXd matBeforeSnr = matData;
                    const MatrixXd matAfterSnr = result.matData;
                    const AOEMegDiagnostics diagnostics = result.diagnostics;
                    m_sSnrWatcher.setFuture(QtConcurrent::run([matBeforeSnr, matAfterSnr, diagnostics]() {
                        return computeEffectSummary(matBeforeSnr, matAfterSnr, diagnostics);
                    }));
                }
            }
        } else {
            msleep(1);
        }
    }
}

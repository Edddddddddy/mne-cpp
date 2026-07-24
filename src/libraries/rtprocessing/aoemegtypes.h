//=============================================================================================================
/**
 * @file     aoemegtypes.h
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Shared settings and diagnostics for the AOE-MEG real-time pipeline.
 *
 */

#ifndef AOEMEGTYPES_RTPROCESSING_H
#define AOEMEGTYPES_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <QString>

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

struct RTPROCESINGSHARED_EXPORT AOEMegStage2TypeSettings
{
    int     iSspRank = 0;
    double  dBandLowHz = 0.5;
    double  dBandHighHz = 8.0;
    double  dStrength = 0.35;
};

struct RTPROCESINGSHARED_EXPORT AOEMegStage3TypeSettings
{
    double  dNoiseBandLowHz = 1.0;
    double  dNoiseBandHighHz = 15.0;
    double  dSelectThresholdMad = 2.5;
    int     iSelectMinCount = 12;
    double  dEnergyThresholdEta = 0.90;
    int     iKMax = 2;
    double  dExtStrength = 1.0;
    double  dScaleAlpha = 0.97;
    double  dScaleFloor = 1e-15;
};

struct RTPROCESINGSHARED_EXPORT AOEMegStage1Settings
{
    bool    bEnabled = true;
    double  dRlsLambda = 0.995;
    double  dRlsDelta = 10.0;
    double  dRlsCoeffClip = 2.0;
    double  dRlsPredClipRatio = 2.0;
    double  dRlsChunkPredClipRatio = 1.5;
    double  dRlsResidualAcceptRatio = 1.2;
    double  dRlsMinUpdateRate = 0.01;
    double  dRlsFreezeBadChunkRatio = 0.20;
    double  dMotionMinNorm = 1e-8;
    double  dMotionMaxAbsZ = 8.0;
    double  dMotionUNormCap = 4.0;
    bool    bFreezeOnBadMotion = true;
    bool    bMotionCenter = true;
    bool    bMotionStandardize = true;
    double  dMotionScaleAlpha = 0.98;
    double  dMotionScaleFloor = 1e-6;
    double  dRlsDenFloor = 1e-8;
    double  dRlsPMaxDiag = 1e4;
    double  dRlsPMaxAbs = 1e4;
    bool    bRlsResetBadP = true;
    bool    bRlsSymmetrizeP = true;
    bool    bNormalizeOutput = true;
    double  dScaleAlphaMag = 0.97;
    double  dScaleAlphaGrad = 0.97;
    double  dScaleFloorMag = 1e-15;
    double  dScaleFloorGrad = 1e-13;
    double  dAcceptEnergyRatio = 1.02;
    double  dMinRho = -0.002;
};

struct RTPROCESINGSHARED_EXPORT AOEMegStage2Settings
{
    bool                        bEnabled = false;
    bool                        bUsePrior = false;
    bool                        bApplyByType = true;
    bool                        bNormalizeByType = true;
    bool                        bNormalize = true;
    double                      dCovAlpha = 0.97;
    int                         iUpdateEveryChunks = 1;
    double                      dRegularization = 1e-5;
    int                         iPriorRankMag = 2;
    int                         iPriorRankGrad = 4;
    AOEMegStage2TypeSettings    mag;
    AOEMegStage2TypeSettings    grad;
};

struct RTPROCESINGSHARED_EXPORT AOEMegStage3Settings
{
    bool                        bEnabled = true;
    int                         iMeanfieldOrder = 1;
    QString                     sApplyMode = "fullband";
    bool                        bApplyByType = true;
    bool                        bNormalizeByType = true;
    bool                        bNormalize = true;
    double                      dCovAlpha = 0.98;
    int                         iUpdateEveryChunks = 1;
    double                      dTriggerEnergyDrop = 0.02;
    double                      dTriggerBandPowerGain = 1.05;
    int                         iTriggerCooldownChunks = 2;
    AOEMegStage3TypeSettings    mag;
    AOEMegStage3TypeSettings    grad;
};

struct RTPROCESINGSHARED_EXPORT AOEMegSettings
{
    int                     iChunkSize = 200;
    int                     iChunkOverlap = 100;
    QString                 sWindow = "hann";
    bool                    bNormalizeByType = true;
    AOEMegStage1Settings    stage1;
    AOEMegStage2Settings    stage2;
    AOEMegStage3Settings    stage3;

    AOEMegSettings()
    {
        stage2.bEnabled = true;
        stage2.bUsePrior = false;
        stage2.mag.iSspRank = 1;
        stage2.mag.dStrength = 0.18;
        stage2.mag.dBandLowHz = 1.0;
        stage2.mag.dBandHighHz = 20.0;
        stage2.grad.iSspRank = 2;
        stage2.grad.dStrength = 0.28;
        stage2.grad.dBandLowHz = 1.0;
        stage2.grad.dBandHighHz = 30.0;

        stage3.mag.dNoiseBandLowHz = 1.0;
        stage3.mag.dNoiseBandHighHz = 15.0;
        stage3.mag.dSelectThresholdMad = 2.5;
        stage3.mag.iSelectMinCount = 12;
        stage3.mag.dEnergyThresholdEta = 0.90;
        stage3.mag.iKMax = 2;
        stage3.mag.dExtStrength = 1.0;
        stage3.mag.dScaleAlpha = 0.97;
        stage3.mag.dScaleFloor = 1e-15;

        stage3.grad.dNoiseBandLowHz = 1.0;
        stage3.grad.dNoiseBandHighHz = 20.0;
        stage3.grad.dSelectThresholdMad = 2.0;
        stage3.grad.iSelectMinCount = 16;
        stage3.grad.dEnergyThresholdEta = 0.90;
        stage3.grad.iKMax = 6;
        stage3.grad.dExtStrength = 1.0;
        stage3.grad.dScaleAlpha = 0.97;
        stage3.grad.dScaleFloor = 1e-13;
    }
};

struct RTPROCESINGSHARED_EXPORT AOEMegDiagnostics
{
    qint64      iProcessedBlocks = 0;
    qint64      iReceivedSamples = 0;
    int         iChannelCount = 0;
    int         iLastBlockSamples = 0;
    bool        bStage1Enabled = true;
    bool        bStage2Enabled = false;
    bool        bStage3Enabled = true;
    bool        bStage1Frozen = false;
    bool        bStage2UpdateSkipped = false;
    bool        bStage3UpdateSkipped = false;
    bool        bMotionInputAvailable = false;
    double      dRhoStage1 = 0.0;
    double      dRhoStage2 = 0.0;
    double      dRhoStage3 = 0.0;
    double      dRhoStage2Mag = 0.0;
    double      dRhoStage2Grad = 0.0;
    double      dRhoStage3Mag = 0.0;
    double      dRhoStage3Grad = 0.0;
    double      dStage1BadURate = 0.0;
    double      dStage1PredRatioMean = 0.0;
    double      dStage1UpdateRate = 0.0;
    double      dStage1ClipRate = 0.0;
    double      dStage1UNormMean = 0.0;
    double      dStage1ChunkAcceptRate = 0.0;
    double      dStage1ResidualAcceptRatio = 1.0;
    int         iStage2RequestedRankMag = 0;
    int         iStage2RequestedRankGrad = 0;
    int         iStage2ActiveRankMag = 0;
    int         iStage2ActiveRankGrad = 0;
    int         iStage2PriorRankMag = 0;
    int         iStage2PriorRankGrad = 0;
    bool        bStage2AppliedMag = false;
    bool        bStage2AppliedGrad = false;
    bool        bStage2CovUpdatedMag = false;
    bool        bStage2CovUpdatedGrad = false;
    bool        bStage2PriorLoaded = false;
    double      dStage2CovTraceMag = 0.0;
    double      dStage2CovTraceGrad = 0.0;
    int         iStage3SelectedMag = 0;
    int         iStage3SelectedGrad = 0;
    int         iStage3PhysicalRankMag = 0;
    int         iStage3PhysicalRankGrad = 0;
    int         iStage3StatRankMag = 0;
    int         iStage3StatRankGrad = 0;
    int         iStage3CombinedRankMag = 0;
    int         iStage3CombinedRankGrad = 0;
    bool        bStage3AppliedMag = false;
    bool        bStage3AppliedGrad = false;
    bool        bStage3CovUpdatedMag = false;
    bool        bStage3CovUpdatedGrad = false;
    double      dStage3CovTraceMag = 0.0;
    double      dStage3CovTraceGrad = 0.0;
    double      dStage3BandPowerMag = 0.0;
    double      dStage3BandPowerGrad = 0.0;
    QString     sStage1Status = "idle";
    QString     sStage2Status = "off";
    QString     sStage2PriorStatus = "none";
    QString     sStage3Status = "off";
    QString     sStatusText = "Phase 3 Stage-3 ready";
};

struct RTPROCESINGSHARED_EXPORT AOEMegProcessResult
{
    Eigen::MatrixXd      matData;
    AOEMegDiagnostics    diagnostics;
};

} // namespace RTPROCESSINGLIB

#endif // AOEMEGTYPES_RTPROCESSING_H

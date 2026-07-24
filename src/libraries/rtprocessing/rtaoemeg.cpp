//=============================================================================================================
/**
 * @file     rtaoemeg.cpp
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Definition of the RtAoeMeg class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtaoemeg.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

#include <Eigen/Eigenvalues>
#include <unsupported/Eigen/FFT>

#include <algorithm>
#include <cmath>
#include <complex>
#include <utility>
#include <vector>

#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace
{

struct Stage1Context
{
    MatrixXd& B;
    MatrixXd& P;
    VectorXd& uMean;
    VectorXd& uScale;
};

struct PreparedMotion
{
    MatrixXd Uz;
    VectorXd uNorm;
    VectorXi badMask;
};

struct Stage1ChunkMetrics
{
    double badURate = 0.0;
    double predRatioMean = 0.0;
    double updateRate = 0.0;
    double clipRate = 0.0;
    double uNormMean = 0.0;
    double chunkPredRatio = 0.0;
    double chunkAccept = 1.0;
    bool freezeChunk = false;
};

struct Stage1ChunkOutcome
{
    MatrixXd residual;
    Stage1ChunkMetrics metrics;
};

struct Stage2TypeState
{
    MatrixXd& covariance;
    MatrixXd& basis;
    MatrixXd& priorBasis;
    VectorXd& scale;
};

struct Stage2TypeOutcome
{
    MatrixXd data;
    double rho = 0.0;
    int requestedRank = 0;
    int activeRank = 0;
    int priorRank = 0;
    bool covarianceUpdated = false;
    bool applied = false;
    double covarianceTrace = 0.0;
    QString status = "off";
};

struct Stage3TypeState
{
    MatrixXd& covariance;
    MatrixXd& physicalBasis;
    MatrixXd& statisticalBasis;
    VectorXd& scale;
    double& previousBandPower;
    double& previousRho;
    int& cooldownLeft;
};

struct Stage3TypeOutcome
{
    MatrixXd data;
    double rho = 0.0;
    double bandPower = 0.0;
    int selectedCount = 0;
    int physicalRank = 0;
    int statisticalRank = 0;
    int combinedRank = 0;
    bool covarianceUpdated = false;
    bool applied = false;
    double covarianceTrace = 0.0;
    QString status = "off";
};

double clampValue(double value, double low, double high)
{
    return std::max(low, std::min(value, high));
}

int clampInt(int value, int low, int high)
{
    return std::max(low, std::min(value, high));
}

AOEMegSettings sanitizeSettings(AOEMegSettings settings)
{
    settings.iChunkSize = clampInt(settings.iChunkSize, 16, 4096);
    settings.iChunkOverlap = clampInt(settings.iChunkOverlap, 0, settings.iChunkSize - 1);

    settings.stage1.dRlsLambda = clampValue(settings.stage1.dRlsLambda, 0.90, 0.99999);
    settings.stage1.dRlsDelta = clampValue(settings.stage1.dRlsDelta, 1e-3, 1e6);
    settings.stage1.dRlsCoeffClip = clampValue(settings.stage1.dRlsCoeffClip, 0.1, 10.0);
    settings.stage1.dRlsPredClipRatio = clampValue(settings.stage1.dRlsPredClipRatio, 0.1, 10.0);
    settings.stage1.dRlsChunkPredClipRatio = clampValue(settings.stage1.dRlsChunkPredClipRatio, 0.1, 10.0);
    settings.stage1.dRlsResidualAcceptRatio = clampValue(settings.stage1.dRlsResidualAcceptRatio, 0.5, 5.0);
    settings.stage1.dRlsMinUpdateRate = clampValue(settings.stage1.dRlsMinUpdateRate, 0.0, 1.0);
    settings.stage1.dRlsFreezeBadChunkRatio = clampValue(settings.stage1.dRlsFreezeBadChunkRatio, 0.0, 1.0);
    settings.stage1.dMotionMinNorm = clampValue(settings.stage1.dMotionMinNorm, 0.0, 1.0);
    settings.stage1.dMotionMaxAbsZ = clampValue(settings.stage1.dMotionMaxAbsZ, 0.5, 20.0);
    settings.stage1.dMotionUNormCap = clampValue(settings.stage1.dMotionUNormCap, 0.1, 20.0);
    settings.stage1.dMotionScaleAlpha = clampValue(settings.stage1.dMotionScaleAlpha, 0.0, 0.99999);
    settings.stage1.dMotionScaleFloor = clampValue(settings.stage1.dMotionScaleFloor, 1e-12, 1.0);
    settings.stage1.dRlsDenFloor = clampValue(settings.stage1.dRlsDenFloor, 1e-12, 1.0);
    settings.stage1.dRlsPMaxDiag = clampValue(settings.stage1.dRlsPMaxDiag, 1.0, 1e8);
    settings.stage1.dRlsPMaxAbs = clampValue(settings.stage1.dRlsPMaxAbs, 1.0, 1e8);
    settings.stage1.dScaleAlphaMag = clampValue(settings.stage1.dScaleAlphaMag, 0.0, 0.99999);
    settings.stage1.dScaleAlphaGrad = clampValue(settings.stage1.dScaleAlphaGrad, 0.0, 0.99999);
    settings.stage1.dScaleFloorMag = clampValue(settings.stage1.dScaleFloorMag, 1e-18, 1.0);
    settings.stage1.dScaleFloorGrad = clampValue(settings.stage1.dScaleFloorGrad, 1e-18, 1.0);
    settings.stage1.dAcceptEnergyRatio = clampValue(settings.stage1.dAcceptEnergyRatio, 0.8, 2.0);
    settings.stage1.dMinRho = clampValue(settings.stage1.dMinRho, -0.25, 0.25);

    settings.stage2.dCovAlpha = clampValue(settings.stage2.dCovAlpha, 0.0, 0.99999);
    settings.stage2.iUpdateEveryChunks = clampInt(settings.stage2.iUpdateEveryChunks, 1, 64);
    settings.stage2.dRegularization = clampValue(settings.stage2.dRegularization, 1e-10, 1e-2);
    settings.stage2.iPriorRankMag = clampInt(settings.stage2.iPriorRankMag, 0, 16);
    settings.stage2.iPriorRankGrad = clampInt(settings.stage2.iPriorRankGrad, 0, 16);
    settings.stage2.mag.iSspRank = clampInt(settings.stage2.mag.iSspRank, 0, 16);
    settings.stage2.grad.iSspRank = clampInt(settings.stage2.grad.iSspRank, 0, 16);
    settings.stage2.mag.dBandLowHz = clampValue(settings.stage2.mag.dBandLowHz, 0.0, 100.0);
    settings.stage2.grad.dBandLowHz = clampValue(settings.stage2.grad.dBandLowHz, 0.0, 100.0);
    settings.stage2.mag.dBandHighHz = clampValue(settings.stage2.mag.dBandHighHz, settings.stage2.mag.dBandLowHz + 0.1, 200.0);
    settings.stage2.grad.dBandHighHz = clampValue(settings.stage2.grad.dBandHighHz, settings.stage2.grad.dBandLowHz + 0.1, 200.0);
    settings.stage2.mag.dStrength = clampValue(settings.stage2.mag.dStrength, 0.0, 1.0);
    settings.stage2.grad.dStrength = clampValue(settings.stage2.grad.dStrength, 0.0, 1.0);

    settings.stage3.iMeanfieldOrder = clampInt(settings.stage3.iMeanfieldOrder, 0, 1);
    settings.stage3.dCovAlpha = clampValue(settings.stage3.dCovAlpha, 0.0, 0.99999);
    settings.stage3.iUpdateEveryChunks = clampInt(settings.stage3.iUpdateEveryChunks, 1, 64);
    settings.stage3.dTriggerEnergyDrop = clampValue(settings.stage3.dTriggerEnergyDrop, 0.0, 0.8);
    settings.stage3.dTriggerBandPowerGain = clampValue(settings.stage3.dTriggerBandPowerGain, 1.0, 10.0);
    settings.stage3.iTriggerCooldownChunks = clampInt(settings.stage3.iTriggerCooldownChunks, 0, 128);
    settings.stage3.mag.dNoiseBandLowHz = clampValue(settings.stage3.mag.dNoiseBandLowHz, 0.0, 100.0);
    settings.stage3.grad.dNoiseBandLowHz = clampValue(settings.stage3.grad.dNoiseBandLowHz, 0.0, 100.0);
    settings.stage3.mag.dNoiseBandHighHz = clampValue(settings.stage3.mag.dNoiseBandHighHz, settings.stage3.mag.dNoiseBandLowHz + 0.1, 200.0);
    settings.stage3.grad.dNoiseBandHighHz = clampValue(settings.stage3.grad.dNoiseBandHighHz, settings.stage3.grad.dNoiseBandLowHz + 0.1, 200.0);
    settings.stage3.mag.dSelectThresholdMad = clampValue(settings.stage3.mag.dSelectThresholdMad, 0.1, 10.0);
    settings.stage3.grad.dSelectThresholdMad = clampValue(settings.stage3.grad.dSelectThresholdMad, 0.1, 10.0);
    settings.stage3.mag.iSelectMinCount = clampInt(settings.stage3.mag.iSelectMinCount, 1, 512);
    settings.stage3.grad.iSelectMinCount = clampInt(settings.stage3.grad.iSelectMinCount, 1, 512);
    settings.stage3.mag.dEnergyThresholdEta = clampValue(settings.stage3.mag.dEnergyThresholdEta, 0.1, 0.9999);
    settings.stage3.grad.dEnergyThresholdEta = clampValue(settings.stage3.grad.dEnergyThresholdEta, 0.1, 0.9999);
    settings.stage3.mag.iKMax = clampInt(settings.stage3.mag.iKMax, 0, 16);
    settings.stage3.grad.iKMax = clampInt(settings.stage3.grad.iKMax, 0, 16);
    settings.stage3.mag.dExtStrength = clampValue(settings.stage3.mag.dExtStrength, 0.0, 1.0);
    settings.stage3.grad.dExtStrength = clampValue(settings.stage3.grad.dExtStrength, 0.0, 1.0);
    settings.stage3.mag.dScaleAlpha = clampValue(settings.stage3.mag.dScaleAlpha, 0.0, 0.99999);
    settings.stage3.grad.dScaleAlpha = clampValue(settings.stage3.grad.dScaleAlpha, 0.0, 0.99999);
    settings.stage3.mag.dScaleFloor = clampValue(settings.stage3.mag.dScaleFloor, 1e-18, 1.0);
    settings.stage3.grad.dScaleFloor = clampValue(settings.stage3.grad.dScaleFloor, 1e-18, 1.0);

    return settings;
}

double safeEnergy(const MatrixXd& matData)
{
    return matData.squaredNorm() + 1e-12;
}

double ratioRemoved(const MatrixXd& before, const MatrixXd& after)
{
    return 1.0 - safeEnergy(after) / safeEnergy(before);
}

bool projectionOutputAccepted(const MatrixXd& before,
                              const MatrixXd& after,
                              double maxEnergyRatio,
                              double minRho,
                              double maxRho)
{
    if(before.size() == 0 || after.size() == 0 || !after.allFinite()) {
        return false;
    }

    const double energyRatio = safeEnergy(after) / safeEnergy(before);
    const double rho = ratioRemoved(before, after);

    if(!std::isfinite(energyRatio) || !std::isfinite(rho)) {
        return false;
    }

    return energyRatio <= maxEnergyRatio && rho >= minRho && rho <= maxRho;
}

double medianOfVector(VectorXd values)
{
    if(values.size() == 0) {
        return 0.0;
    }

    const qint64 mid = values.size() / 2;
    std::nth_element(values.data(), values.data() + mid, values.data() + values.size());
    double med = values(mid);

    if(values.size() % 2 == 0) {
        std::nth_element(values.data(), values.data() + mid - 1, values.data() + mid);
        med = 0.5 * (med + values(mid - 1));
    }

    return med;
}

VectorXd robustChannelScale(const MatrixXd& matData, double floorValue)
{
    VectorXd scale(matData.rows());
    scale.setConstant(std::max(floorValue, 1e-18));

    for(Index i = 0; i < matData.rows(); ++i) {
        const VectorXd row = matData.row(i).transpose();
        const double med = medianOfVector(row);
        const VectorXd centered = row.array() - med;
        const double mad = medianOfVector(centered.cwiseAbs());
        const double sMad = 1.4826 * mad;
        const double sRms = std::sqrt(centered.array().square().mean());
        scale(i) = std::max(std::max(sMad, 0.25 * sRms), floorValue);
    }

    return scale;
}

VectorXd updateScale(const VectorXd& previous, const MatrixXd& matData, double alpha, double floorValue)
{
    const VectorXd current = robustChannelScale(matData, floorValue);

    if(previous.size() != current.size()) {
        return current;
    }

    return (alpha * previous.array() + (1.0 - alpha) * current.array()).max(floorValue).matrix();
}

MatrixXd pickRows(const MatrixXd& matData, const QVector<int>& rows)
{
    MatrixXd picked(rows.size(), matData.cols());

    for(int i = 0; i < rows.size(); ++i) {
        picked.row(i) = matData.row(rows.at(i));
    }

    return picked;
}

void assignRows(MatrixXd& target, const QVector<int>& rows, const MatrixXd& source)
{
    for(int i = 0; i < rows.size(); ++i) {
        target.row(rows.at(i)) = source.row(i);
    }
}

VectorXd clipByNorm(const VectorXd& vecIn, double maxNorm, bool* pClipped = 0)
{
    if(pClipped) {
        *pClipped = false;
    }

    const double normValue = vecIn.norm();
    if(!std::isfinite(normValue) || maxNorm <= 0.0) {
        if(pClipped) {
            *pClipped = true;
        }
        return VectorXd::Zero(vecIn.size());
    }

    if(normValue <= maxNorm || normValue <= 1e-15) {
        return vecIn;
    }

    if(pClipped) {
        *pClipped = true;
    }

    return vecIn * (maxNorm / normValue);
}

bool sanitizeCovariance(Stage1Context& state, const AOEMegStage1Settings& cfg)
{
    if(!state.P.allFinite()) {
        if(cfg.bRlsResetBadP) {
            state.P = MatrixXd::Identity(state.P.rows(), state.P.cols()) * cfg.dRlsDelta;
        }

        return false;
    }

    if(cfg.dRlsPMaxAbs > 0.0) {
        state.P = state.P.array().min(cfg.dRlsPMaxAbs).max(-cfg.dRlsPMaxAbs).matrix();
    }

    if(cfg.bRlsSymmetrizeP) {
        state.P = 0.5 * (state.P + state.P.transpose());
    }

    if(cfg.dRlsPMaxDiag > 0.0) {
        VectorXd diagonal = state.P.diagonal();
        diagonal = diagonal.array().min(cfg.dRlsPMaxDiag).max(1e-12).matrix();
        state.P.diagonal() = diagonal;
    }

    return state.P.allFinite();
}

MatrixXd robustZScoreRows(const MatrixXd& matData)
{
    MatrixXd zScores = MatrixXd::Zero(matData.rows(), matData.cols());

    for(Index i = 0; i < matData.rows(); ++i) {
        const VectorXd row = matData.row(i).transpose();
        const double med = medianOfVector(row);
        const double mad = medianOfVector((row.array() - med).abs().matrix()) + 1e-12;
        zScores.row(i) = (0.6745 * (row.array() - med) / mad).transpose();
    }

    return zScores;
}

PreparedMotion prepareMotionRegressors(const MatrixXd& matMotion,
                                       Stage1Context& state,
                                       const AOEMegStage1Settings& cfg)
{
    PreparedMotion prepared;

    if(cfg.bMotionCenter) {
        const VectorXd meanCurrent = matMotion.rowwise().mean();
        state.uMean = (cfg.dMotionScaleAlpha * state.uMean.array()
                     + (1.0 - cfg.dMotionScaleAlpha) * meanCurrent.array()).matrix();
    } else {
        state.uMean.setZero();
    }

    const MatrixXd centered = matMotion.colwise() - state.uMean;

    if(cfg.bMotionStandardize) {
        const VectorXd scaleCurrent = robustChannelScale(centered, cfg.dMotionScaleFloor);

        if(state.uScale.size() != scaleCurrent.size()) {
            state.uScale = scaleCurrent;
        } else {
            state.uScale = (cfg.dMotionScaleAlpha * state.uScale.array()
                          + (1.0 - cfg.dMotionScaleAlpha) * scaleCurrent.array()).max(cfg.dMotionScaleFloor).matrix();
        }

        prepared.Uz = centered.array().colwise() / state.uScale.array();
    } else {
        state.uScale = VectorXd::Ones(centered.rows());
        prepared.Uz = centered;
    }

    prepared.Uz = prepared.Uz.array().min(cfg.dMotionMaxAbsZ).max(-cfg.dMotionMaxAbsZ).matrix();
    prepared.uNorm = prepared.Uz.colwise().norm().transpose();

    if(cfg.dMotionUNormCap > 0.0) {
        for(Index t = 0; t < prepared.Uz.cols(); ++t) {
            const double normValue = prepared.uNorm(t);
            if(normValue > cfg.dMotionUNormCap && normValue > 1e-12) {
                prepared.Uz.col(t) *= cfg.dMotionUNormCap / normValue;
                prepared.uNorm(t) = prepared.Uz.col(t).norm();
            }
        }
    }

    prepared.badMask = VectorXi::Zero(prepared.Uz.cols());
    for(Index t = 0; t < prepared.Uz.cols(); ++t) {
        const bool badSmall = prepared.uNorm(t) < cfg.dMotionMinNorm;
        const bool badLarge = !prepared.Uz.col(t).allFinite();
        prepared.badMask(t) = (badSmall || badLarge) ? 1 : 0;
    }

    return prepared;
}

Stage1ChunkOutcome processStage1Chunk(const MatrixXd& matData,
                                      const MatrixXd& matMotion,
                                      bool freezeChunk,
                                      Stage1Context& state,
                                      const AOEMegStage1Settings& cfg)
{
    Stage1ChunkOutcome outcome;
    outcome.residual = MatrixXd::Zero(matData.rows(), matData.cols());

    const PreparedMotion prepared = prepareMotionRegressors(matMotion, state, cfg);
    const MatrixXd bPrevious = state.B;
    const MatrixXd pPrevious = state.P;
    double predRatioSum = 0.0;
    int clipCount = 0;
    int updateCount = 0;

    for(Index t = 0; t < matData.cols(); ++t) {
        const VectorXd y = matData.col(t);
        const VectorXd u = prepared.Uz.col(t);
        const bool badT = prepared.badMask(t) > 0;

        const VectorXd yHat = state.B * u;
        const double yNorm = y.norm() + 1e-12;
        const double predRatio = yHat.norm() / yNorm;
        predRatioSum += predRatio;

        bool localFreeze = freezeChunk || badT || predRatio > cfg.dRlsPredClipRatio || !yHat.allFinite();
        VectorXd yHatUse = yHat;

        if(predRatio > cfg.dRlsPredClipRatio) {
            bool clipped = false;
            yHatUse = clipByNorm(yHat, cfg.dRlsPredClipRatio * yNorm, &clipped);
            clipCount += clipped ? 1 : 0;
            localFreeze = true;
        }

        const VectorXd residual = y - yHatUse;
        outcome.residual.col(t) = residual;

        if(localFreeze) {
            continue;
        }

        if(!sanitizeCovariance(state, cfg)) {
            state.B = bPrevious;
            state.P = pPrevious;
            outcome.metrics.freezeChunk = true;
            outcome.metrics.chunkAccept = 0.0;
            return outcome;
        }

        const VectorXd pu = state.P * u;
        if(!pu.allFinite()) {
            state.B = bPrevious;
            state.P = pPrevious;
            outcome.metrics.freezeChunk = true;
            outcome.metrics.chunkAccept = 0.0;
            return outcome;
        }

        const double utPu = u.dot(pu);
        const double denom = cfg.dRlsLambda + utPu;

        if(!std::isfinite(utPu) || !std::isfinite(denom) || denom <= cfg.dRlsDenFloor) {
            continue;
        }

        const VectorXd k = pu / denom;
        if(!k.allFinite()) {
            state.B = bPrevious;
            state.P = pPrevious;
            outcome.metrics.freezeChunk = true;
            outcome.metrics.chunkAccept = 0.0;
            return outcome;
        }

        MatrixXd bNext = state.B + residual * k.transpose();

        if(cfg.dRlsCoeffClip > 0.0) {
            for(Index row = 0; row < bNext.rows(); ++row) {
                const double rowNorm = bNext.row(row).norm();
                if(rowNorm > cfg.dRlsCoeffClip && rowNorm > 1e-12) {
                    bNext.row(row) *= cfg.dRlsCoeffClip / rowNorm;
                }
            }

            const double bNorm = bNext.norm();
            const double globalLimit = cfg.dRlsCoeffClip
                    * std::sqrt(static_cast<double>(std::max<Index>(1, bNext.rows())));
            if(std::isfinite(bNorm) && bNorm > globalLimit) {
                bNext *= globalLimit / (bNorm + 1e-12);
            }
        }

        const RowVectorXd pRight = u.transpose() * state.P;
        if(!pRight.allFinite()) {
            state.B = bPrevious;
            state.P = pPrevious;
            outcome.metrics.freezeChunk = true;
            outcome.metrics.chunkAccept = 0.0;
            return outcome;
        }

        const MatrixXd pNext = (state.P - k * pRight) / cfg.dRlsLambda;
        if(!pNext.allFinite()) {
            state.B = bPrevious;
            state.P = pPrevious;
            outcome.metrics.freezeChunk = true;
            outcome.metrics.chunkAccept = 0.0;
            return outcome;
        }

        state.B = bNext;
        state.P = pNext;

        if(!sanitizeCovariance(state, cfg) || !state.B.allFinite()) {
            state.B = bPrevious;
            state.P = pPrevious;
            outcome.metrics.freezeChunk = true;
            outcome.metrics.chunkAccept = 0.0;
            return outcome;
        }

        ++updateCount;
    }

    outcome.metrics.badURate = prepared.badMask.size() > 0
            ? prepared.badMask.cast<double>().mean()
            : 0.0;
    outcome.metrics.predRatioMean = matData.cols() > 0
            ? predRatioSum / static_cast<double>(matData.cols())
            : 0.0;
    outcome.metrics.updateRate = matData.cols() > 0
            ? static_cast<double>(updateCount) / static_cast<double>(matData.cols())
            : 0.0;
    outcome.metrics.clipRate = matData.cols() > 0
            ? static_cast<double>(clipCount) / static_cast<double>(matData.cols())
            : 0.0;
    outcome.metrics.uNormMean = prepared.uNorm.size() > 0 ? prepared.uNorm.mean() : 0.0;
    outcome.metrics.chunkPredRatio = (matData - outcome.residual).norm() / (matData.norm() + 1e-12);

    if(outcome.metrics.chunkPredRatio > cfg.dRlsChunkPredClipRatio
            || outcome.metrics.updateRate < cfg.dRlsMinUpdateRate) {
        state.B = bPrevious;
        state.P = pPrevious;
        outcome.residual = matData;
        outcome.metrics.freezeChunk = true;
        outcome.metrics.chunkAccept = 0.0;
    }

    return outcome;
}

MatrixXd bandPassRowsFft(const MatrixXd& matData, double sfreq, double lowHz, double highHz)
{
    if(matData.size() == 0) {
        return matData;
    }

    if(sfreq <= 0.0 || highHz <= lowHz || matData.cols() < 4) {
        const VectorXd rowMeans = matData.rowwise().mean();
        return matData.colwise() - rowMeans;
    }

    Eigen::FFT<double> fft;
    MatrixXd filtered = MatrixXd::Zero(matData.rows(), matData.cols());
    const double nSamples = static_cast<double>(matData.cols());

    for(Index row = 0; row < matData.rows(); ++row) {
        std::vector<double> timeSignal(static_cast<size_t>(matData.cols()));
        for(Index col = 0; col < matData.cols(); ++col) {
            timeSignal[static_cast<size_t>(col)] = matData(row, col);
        }

        std::vector<std::complex<double> > spectrum;
        fft.fwd(spectrum, timeSignal);

        for(Index k = 0; k < static_cast<Index>(spectrum.size()); ++k) {
            const double freq = (k <= static_cast<Index>(spectrum.size() / 2) ? static_cast<double>(k)
                                                                               : static_cast<double>(spectrum.size() - k))
                              * sfreq / nSamples;
            if(freq < lowHz || freq > highHz) {
                spectrum[static_cast<size_t>(k)] = std::complex<double>(0.0, 0.0);
            }
        }

        std::vector<double> timeFiltered;
        fft.inv(timeFiltered, spectrum);

        for(Index col = 0; col < matData.cols(); ++col) {
            filtered(row, col) = timeFiltered[static_cast<size_t>(col)];
        }
    }

    filtered = filtered.colwise() - filtered.rowwise().mean();
    return filtered;
}

MatrixXd computeStage2Basis(const MatrixXd& covariance, int rank, double regularization)
{
    if(covariance.rows() == 0 || rank <= 0) {
        return MatrixXd(covariance.rows(), 0);
    }

    MatrixXd covarianceReg = covariance;
    covarianceReg.diagonal().array() += regularization;

    SelfAdjointEigenSolver<MatrixXd> solver(covarianceReg);
    if(solver.info() != Success) {
        return MatrixXd(covariance.rows(), 0);
    }

    const int activeRank = std::min<int>(rank, static_cast<int>(covariance.rows()));
    MatrixXd basis(covariance.rows(), activeRank);

    for(int i = 0; i < activeRank; ++i) {
        basis.col(i) = solver.eigenvectors().col(covariance.rows() - 1 - i);
    }

    return basis;
}

MatrixXd orthonormalizeColumns(const MatrixXd& matBasis);

MatrixXd buildPriorBasisFromProjectors(const QList<FiffProj>& projs,
                                       const QStringList& chNames,
                                       const QStringList& bads,
                                       int maxRank)
{
    if(projs.isEmpty() || chNames.isEmpty() || maxRank <= 0) {
        return MatrixXd(chNames.size(), 0);
    }

    QList<FiffProj> activeProjs = projs;
    FiffProj::activate_projs(activeProjs);

    MatrixXd projector;
    MatrixXd basis;
    const qint32 nproj = FiffProj::make_projector(activeProjs, chNames, projector, bads, basis);
    if(nproj <= 0 || basis.cols() == 0) {
        return MatrixXd(chNames.size(), 0);
    }

    const int rank = std::min<int>(maxRank, static_cast<int>(basis.cols()));
    return orthonormalizeColumns(basis.leftCols(rank));
}

MatrixXd orthonormalizeColumns(const MatrixXd& matBasis)
{
    if(matBasis.rows() == 0 || matBasis.cols() == 0) {
        return MatrixXd(matBasis.rows(), 0);
    }

    std::vector<VectorXd> vectors;
    vectors.reserve(static_cast<size_t>(matBasis.cols()));

    for(Index col = 0; col < matBasis.cols(); ++col) {
        VectorXd vec = matBasis.col(col);

        for(size_t i = 0; i < vectors.size(); ++i) {
            vec -= vectors[i] * (vectors[i].dot(vec));
        }

        const double normValue = vec.norm();
        if(std::isfinite(normValue) && normValue > 1e-10) {
            vectors.push_back(vec / normValue);
        }
    }

    MatrixXd basis(matBasis.rows(), static_cast<Index>(vectors.size()));
    for(size_t i = 0; i < vectors.size(); ++i) {
        basis.col(static_cast<Index>(i)) = vectors[i];
    }

    return basis;
}

VectorXd rowPower(const MatrixXd& matData)
{
    if(matData.rows() == 0) {
        return VectorXd(0);
    }

    return matData.array().square().rowwise().mean().matrix();
}

QVector<int> selectRowsByMad(const VectorXd& vecPower, double thresholdMad, int minCount)
{
    QVector<int> selected;

    if(vecPower.size() == 0) {
        return selected;
    }

    const double med = medianOfVector(vecPower);
    const double mad = medianOfVector((vecPower.array() - med).abs().matrix()) + 1e-12;
    const double threshold = med + thresholdMad * 1.4826 * mad;

    std::vector<std::pair<double, int> > sortedPower;
    sortedPower.reserve(static_cast<size_t>(vecPower.size()));

    for(Index i = 0; i < vecPower.size(); ++i) {
        if(vecPower(i) >= threshold) {
            selected.append(static_cast<int>(i));
        }

        sortedPower.push_back(std::make_pair(vecPower(i), static_cast<int>(i)));
    }

    std::sort(sortedPower.begin(),
              sortedPower.end(),
              [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
                  return a.first > b.first;
              });

    const int targetCount = std::min<int>(minCount, static_cast<int>(vecPower.size()));
    for(int i = 0; i < targetCount && selected.size() < targetCount; ++i) {
        if(!selected.contains(sortedPower[static_cast<size_t>(i)].second)) {
            selected.append(sortedPower[static_cast<size_t>(i)].second);
        }
    }

    std::sort(selected.begin(), selected.end());
    return selected;
}

MatrixXd buildWeightedSelection(const MatrixXd& matData, const QVector<int>& selectedRows)
{
    MatrixXd weighted = MatrixXd::Zero(matData.rows(), matData.cols());

    for(int i = 0; i < selectedRows.size(); ++i) {
        weighted.row(selectedRows.at(i)) = matData.row(selectedRows.at(i));
    }

    return weighted;
}

MatrixXd buildMeanFieldBasis(const QSharedPointer<FiffInfo>& pFiffInfo,
                             const QVector<int>& megPicks,
                             const QVector<int>& localRows,
                             int meanfieldOrder)
{
    if(!pFiffInfo || localRows.isEmpty()) {
        return MatrixXd(localRows.size(), 0);
    }

    const int nBaseCols = 3;
    const int nGradCols = meanfieldOrder >= 1 ? 9 : 0;
    MatrixXd basis = MatrixXd::Zero(localRows.size(), nBaseCols + nGradCols);

    for(int localIndex = 0; localIndex < localRows.size(); ++localIndex) {
        const int megLocalRow = localRows.at(localIndex);
        const int globalRow = megPicks.at(megLocalRow);
        const FiffChInfo& channelInfo = pFiffInfo->chs.at(globalRow);

        Vector3d normal = channelInfo.chpos.ez.cast<double>();
        const double normalNorm = normal.norm();
        if(!std::isfinite(normalNorm) || normalNorm <= 1e-12) {
            normal = Vector3d::UnitZ();
        } else {
            normal /= normalNorm;
        }

        const Vector3d position = channelInfo.chpos.r0.cast<double>();

        basis(localIndex, 0) = normal(0);
        basis(localIndex, 1) = normal(1);
        basis(localIndex, 2) = normal(2);

        if(meanfieldOrder >= 1) {
            int col = nBaseCols;
            for(int p = 0; p < 3; ++p) {
                for(int n = 0; n < 3; ++n) {
                    basis(localIndex, col) = position(p) * normal(n);
                    ++col;
                }
            }
        }
    }

    return orthonormalizeColumns(basis);
}

MatrixXd computeEnergyLimitedBasis(const MatrixXd& covariance,
                                   int kMax,
                                   double eta,
                                   double regularization,
                                   int* pActiveRank = 0)
{
    if(pActiveRank) {
        *pActiveRank = 0;
    }

    if(covariance.rows() == 0 || kMax <= 0) {
        return MatrixXd(covariance.rows(), 0);
    }

    MatrixXd covarianceReg = covariance;
    covarianceReg.diagonal().array() += regularization;

    SelfAdjointEigenSolver<MatrixXd> solver(covarianceReg);
    if(solver.info() != Success) {
        return MatrixXd(covariance.rows(), 0);
    }

    const VectorXd eigenValues = solver.eigenvalues().cwiseMax(0.0);
    const double totalEnergy = eigenValues.sum();
    if(totalEnergy <= 1e-12) {
        return MatrixXd(covariance.rows(), 0);
    }

    double cumulativeEnergy = 0.0;
    int activeRank = 0;
    const int rankLimit = std::min<int>(kMax, static_cast<int>(covariance.rows()));

    for(int i = 0; i < rankLimit; ++i) {
        cumulativeEnergy += eigenValues(eigenValues.size() - 1 - i);
        activeRank = i + 1;
        if(cumulativeEnergy / totalEnergy >= eta) {
            break;
        }
    }

    if(pActiveRank) {
        *pActiveRank = activeRank;
    }

    MatrixXd basis(covariance.rows(), activeRank);
    for(int i = 0; i < activeRank; ++i) {
        basis.col(i) = solver.eigenvectors().col(covariance.rows() - 1 - i);
    }

    return orthonormalizeColumns(basis);
}

Stage2TypeOutcome applyStage2Type(const MatrixXd& matInput,
                                  Stage2TypeState& state,
                                  const AOEMegStage2TypeSettings& typeSettings,
                                  const AOEMegStage2Settings& stage2Settings,
                                  bool usePrior,
                                  bool normalizeByType,
                                  double scaleAlpha,
                                  double scaleFloor,
                                  double sfreq,
                                  qint64 blockIndex)
{
    Stage2TypeOutcome outcome;
    outcome.data = matInput;
    outcome.requestedRank = typeSettings.iSspRank;

    if(matInput.rows() == 0) {
        outcome.status = "no channels";
        return outcome;
    }

    const bool hasPrior = usePrior && state.priorBasis.cols() > 0;
    const bool useOnlineRank = typeSettings.iSspRank > 0;
    if(!hasPrior && !useOnlineRank) {
        outcome.status = "rank=0";
        return outcome;
    }

    const MatrixXd matBand = bandPassRowsFft(matInput,
                                             sfreq,
                                             typeSettings.dBandLowHz,
                                             typeSettings.dBandHighHz);

    MatrixXd matFullNorm = matInput;
    MatrixXd matBandNorm = matBand;

    if(normalizeByType && stage2Settings.bNormalize) {
        state.scale = updateScale(state.scale, matInput, scaleAlpha, scaleFloor);
        matFullNorm = matInput.array().colwise() / state.scale.array();
        matBandNorm = matBand.array().colwise() / state.scale.array();
    }

    if(state.covariance.rows() != matInput.rows()) {
        state.covariance = MatrixXd::Identity(matInput.rows(), matInput.rows()) * 1e-12;
        state.basis = MatrixXd(matInput.rows(), 0);
    }

    const MatrixXd covChunk = (matBandNorm * matBandNorm.transpose()) / std::max<Index>(1, matBandNorm.cols());
    state.covariance = stage2Settings.dCovAlpha * state.covariance
                     + (1.0 - stage2Settings.dCovAlpha) * covChunk;
    outcome.covarianceUpdated = true;
    outcome.covarianceTrace = state.covariance.trace();

    const bool doBasisUpdate = useOnlineRank && (state.basis.cols() == 0
            || ((blockIndex - 1) % std::max(1, stage2Settings.iUpdateEveryChunks) == 0));

    if(doBasisUpdate) {
        state.basis = computeStage2Basis(state.covariance,
                                         typeSettings.iSspRank,
                                         stage2Settings.dRegularization);
    } else if(!useOnlineRank) {
        state.basis = MatrixXd(matInput.rows(), 0);
    }

    outcome.priorRank = usePrior ? static_cast<int>(state.priorBasis.cols()) : 0;

    const int priorCols = usePrior ? static_cast<int>(state.priorBasis.cols()) : 0;
    MatrixXd combinedBasis(matInput.rows(), priorCols + state.basis.cols());
    if(usePrior && state.priorBasis.cols() > 0) {
        combinedBasis.leftCols(state.priorBasis.cols()) = state.priorBasis;
    }
    if(state.basis.cols() > 0) {
        combinedBasis.rightCols(state.basis.cols()) = state.basis;
    }

    const MatrixXd orthBasis = orthonormalizeColumns(combinedBasis);
    outcome.activeRank = static_cast<int>(orthBasis.cols());

    if(orthBasis.cols() == 0 || typeSettings.dStrength <= 0.0) {
        outcome.status = "basis empty";
        return outcome;
    }

    const MatrixXd projectedFull = orthBasis * (orthBasis.transpose() * matFullNorm);
    MatrixXd matOutputNorm = matFullNorm - typeSettings.dStrength * projectedFull;

    if(normalizeByType && stage2Settings.bNormalize && state.scale.size() == matInput.rows()) {
        matOutputNorm = matOutputNorm.array().colwise() * state.scale.array();
    }

    if(!projectionOutputAccepted(matInput, matOutputNorm, 1.03, -0.01, 0.75)) {
        outcome.status = "rejected-by-guard";
        outcome.data = matInput;
        outcome.rho = 0.0;
        outcome.applied = false;
        return outcome;
    }

    outcome.data = matOutputNorm;
    outcome.rho = ratioRemoved(matInput, outcome.data);
    outcome.applied = true;
    outcome.status = QString("rank=%1 prior=%2 active=%3 strength=%4 rho=%5")
            .arg(typeSettings.iSspRank)
            .arg(outcome.priorRank)
            .arg(outcome.activeRank)
            .arg(typeSettings.dStrength, 0, 'f', 2)
            .arg(outcome.rho, 0, 'f', 4);

    return outcome;
}

Stage3TypeOutcome applyStage3Type(const MatrixXd& matInput,
                                  Stage3TypeState& state,
                                  const AOEMegStage3TypeSettings& typeSettings,
                                  const AOEMegStage3Settings& stage3Settings,
                                  bool normalizeByType,
                                  double sfreq,
                                  qint64 blockIndex)
{
    Stage3TypeOutcome outcome;
    outcome.data = matInput;
    outcome.physicalRank = static_cast<int>(state.physicalBasis.cols());

    if(matInput.rows() == 0) {
        outcome.status = "no channels";
        return outcome;
    }

    MatrixXd matFullNorm = matInput;
    MatrixXd matBandNorm = bandPassRowsFft(matInput,
                                           sfreq,
                                           typeSettings.dNoiseBandLowHz,
                                           typeSettings.dNoiseBandHighHz);

    if(normalizeByType && stage3Settings.bNormalize) {
        state.scale = updateScale(state.scale, matInput, typeSettings.dScaleAlpha, typeSettings.dScaleFloor);
        matFullNorm = matInput.array().colwise() / state.scale.array();
        matBandNorm = matBandNorm.array().colwise() / state.scale.array();
    }

    const VectorXd powerPerRow = rowPower(matBandNorm);
    const QVector<int> selectedRows = selectRowsByMad(powerPerRow,
                                                      typeSettings.dSelectThresholdMad,
                                                      typeSettings.iSelectMinCount);
    outcome.selectedCount = selectedRows.size();
    outcome.bandPower = powerPerRow.size() > 0 ? powerPerRow.mean() : 0.0;

    MatrixXd weightedBand = buildWeightedSelection(matBandNorm, selectedRows);

    if(state.covariance.rows() != matInput.rows()) {
        state.covariance = MatrixXd::Identity(matInput.rows(), matInput.rows()) * 1e-12;
        state.statisticalBasis = MatrixXd(matInput.rows(), 0);
    }

    const bool triggerByBand = state.previousBandPower > 1e-12
            && outcome.bandPower > state.previousBandPower * stage3Settings.dTriggerBandPowerGain
            && state.cooldownLeft <= 0;
    const bool triggerByWeakSuppression = state.previousRho < stage3Settings.dTriggerEnergyDrop
            && state.cooldownLeft <= 0;
    const bool periodicUpdate = state.statisticalBasis.cols() == 0
            || ((blockIndex - 1) % std::max(1, stage3Settings.iUpdateEveryChunks) == 0);
    const bool doUpdate = periodicUpdate || triggerByBand || triggerByWeakSuppression;

    if(doUpdate) {
        const MatrixXd covChunk = (weightedBand * weightedBand.transpose()) / std::max<Index>(1, weightedBand.cols());
        state.covariance = stage3Settings.dCovAlpha * state.covariance
                         + (1.0 - stage3Settings.dCovAlpha) * covChunk;
        outcome.covarianceUpdated = true;
        outcome.covarianceTrace = state.covariance.trace();
        state.statisticalBasis = computeEnergyLimitedBasis(state.covariance,
                                                           typeSettings.iKMax,
                                                           typeSettings.dEnergyThresholdEta,
                                                           1e-6,
                                                           &outcome.statisticalRank);

        if(triggerByBand || triggerByWeakSuppression) {
            state.cooldownLeft = std::max(0, stage3Settings.iTriggerCooldownChunks);
        }
    } else {
        outcome.covarianceTrace = state.covariance.size() > 0 ? state.covariance.trace() : 0.0;
        outcome.statisticalRank = static_cast<int>(state.statisticalBasis.cols());
    }

    if(state.cooldownLeft > 0) {
        --state.cooldownLeft;
    }

    MatrixXd combinedBasis(matInput.rows(),
                           state.physicalBasis.cols() + state.statisticalBasis.cols());
    if(state.physicalBasis.cols() > 0) {
        combinedBasis.leftCols(state.physicalBasis.cols()) = state.physicalBasis;
    }
    if(state.statisticalBasis.cols() > 0) {
        combinedBasis.rightCols(state.statisticalBasis.cols()) = state.statisticalBasis;
    }

    const MatrixXd orthBasis = orthonormalizeColumns(combinedBasis);
    outcome.combinedRank = static_cast<int>(orthBasis.cols());

    if(orthBasis.cols() == 0 || typeSettings.dExtStrength <= 0.0) {
        outcome.status = "basis empty";
        state.previousBandPower = outcome.bandPower;
        state.previousRho = 0.0;
        return outcome;
    }

    const MatrixXd projected = orthBasis * (orthBasis.transpose() * matFullNorm);
    MatrixXd matOutputNorm = matFullNorm - typeSettings.dExtStrength * projected;

    if(normalizeByType && stage3Settings.bNormalize && state.scale.size() == matInput.rows()) {
        matOutputNorm = matOutputNorm.array().colwise() * state.scale.array();
    }

    if(!projectionOutputAccepted(matInput, matOutputNorm, 1.03, -0.01, 0.90)) {
        outcome.status = "rejected-by-guard";
        outcome.data = matInput;
        outcome.rho = 0.0;
        outcome.applied = false;
        state.previousBandPower = outcome.bandPower;
        state.previousRho = 0.0;
        return outcome;
    }

    outcome.data = matOutputNorm;
    outcome.rho = ratioRemoved(matInput, outcome.data);
    outcome.applied = true;
    outcome.status = QString("sel=%1 phys=%2 stat=%3 comb=%4 ext=%5")
            .arg(outcome.selectedCount)
            .arg(outcome.physicalRank)
            .arg(outcome.statisticalRank)
            .arg(outcome.combinedRank)
            .arg(typeSettings.dExtStrength, 0, 'f', 2);

    state.previousBandPower = outcome.bandPower;
    state.previousRho = outcome.rho;

    return outcome;
}

QString makePipelineStatus(const AOEMegDiagnostics& diagnostics)
{
    return QString("AOE-MEG Phase 3 | S1=%1 rho=%2 | S2=%3 rho=%4 | S3=%5 rho=%6")
            .arg(diagnostics.sStage1Status)
            .arg(diagnostics.dRhoStage1, 0, 'f', 4)
            .arg(diagnostics.sStage2Status)
            .arg(diagnostics.dRhoStage2, 0, 'f', 4)
            .arg(diagnostics.sStage3Status)
            .arg(diagnostics.dRhoStage3, 0, 'f', 4);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtAoeMeg::RtAoeMeg()
{
    reset();
}

//=============================================================================================================

void RtAoeMeg::setSettings(const AOEMegSettings& settings)
{
    m_settings = sanitizeSettings(settings);
    m_diagnostics.bStage1Enabled = m_settings.stage1.bEnabled;
    m_diagnostics.bStage2Enabled = m_settings.stage2.bEnabled;
    m_diagnostics.bStage3Enabled = m_settings.stage3.bEnabled;
    resetStage1State();
    resetStage2State();
    resetStage3State();
}

//=============================================================================================================

AOEMegSettings RtAoeMeg::settings() const
{
    return m_settings;
}

//=============================================================================================================

AOEMegDiagnostics RtAoeMeg::diagnostics() const
{
    return m_diagnostics;
}

//=============================================================================================================

void RtAoeMeg::setStage2PriorProjectors(const QList<FiffProj>& projs)
{
    m_listStage2PriorProjs = projs;
    resetStage2State();
}

//=============================================================================================================

void RtAoeMeg::clearStage2PriorProjectors()
{
    m_listStage2PriorProjs.clear();
    resetStage2State();
}

//=============================================================================================================

void RtAoeMeg::setFiffInfo(const QSharedPointer<FiffInfo>& pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;

    if(m_pFiffInfo) {
        m_diagnostics.iChannelCount = m_pFiffInfo->nchan;
        rebuildChannelSelection();
        resetStage1State();
        resetStage2State();
        resetStage3State();
    }
}

//=============================================================================================================

void RtAoeMeg::setMotionVector(const VectorXd& vecMotion, bool bValid)
{
    if(!bValid || vecMotion.size() == 0 || !vecMotion.allFinite()) {
        m_vecLatestMotion.resize(0);
        m_bHasMotionVector = false;
        m_diagnostics.bMotionInputAvailable = false;
        return;
    }

    m_vecLatestMotion = vecMotion;
    m_bHasMotionVector = true;
    m_diagnostics.bMotionInputAvailable = true;
}

//=============================================================================================================

void RtAoeMeg::reset()
{
    m_settings = sanitizeSettings(AOEMegSettings());
    m_diagnostics = AOEMegDiagnostics();
    m_diagnostics.bStage1Enabled = m_settings.stage1.bEnabled;
    m_diagnostics.bStage2Enabled = m_settings.stage2.bEnabled;
    m_diagnostics.bStage3Enabled = m_settings.stage3.bEnabled;
    m_pFiffInfo.clear();
    m_vecMegPicks.clear();
    m_vecMagLocalRows.clear();
    m_vecGradLocalRows.clear();
    resetStage1State();
    resetStage2State();
    resetStage3State();
    m_vecLatestMotion.resize(0);
    m_bHasMotionVector = false;
}

//=============================================================================================================

void RtAoeMeg::resetStage1State()
{
    const int nMeg = m_vecMegPicks.size();
    const int nMotion = 6;

    if(nMeg <= 0) {
        m_matStage1B.resize(0, 0);
        m_matStage1P.resize(0, 0);
        m_vecStage1UMean.resize(0);
        m_vecStage1UScale.resize(0);
        m_vecStage1MagScale.resize(0);
        m_vecStage1GradScale.resize(0);
        return;
    }

    m_matStage1B = MatrixXd::Zero(nMeg, nMotion);
    m_matStage1P = MatrixXd::Identity(nMotion, nMotion) * m_settings.stage1.dRlsDelta;
    m_vecStage1UMean = VectorXd::Zero(nMotion);
    m_vecStage1UScale = VectorXd::Ones(nMotion);
    m_vecStage1MagScale.resize(0);
    m_vecStage1GradScale.resize(0);
}

//=============================================================================================================

void RtAoeMeg::resetStage2State()
{
    const int nMag = m_vecMagLocalRows.size();
    const int nGrad = m_vecGradLocalRows.size();

    m_matStage2CovMag = nMag > 0 ? MatrixXd::Identity(nMag, nMag) * 1e-12 : MatrixXd(0, 0);
    m_matStage2CovGrad = nGrad > 0 ? MatrixXd::Identity(nGrad, nGrad) * 1e-12 : MatrixXd(0, 0);
    m_matStage2BasisMag = MatrixXd(nMag, 0);
    m_matStage2BasisGrad = MatrixXd(nGrad, 0);
    m_matStage2PriorBasisMag = MatrixXd(nMag, 0);
    m_matStage2PriorBasisGrad = MatrixXd(nGrad, 0);
    m_vecStage2MagScale.resize(0);
    m_vecStage2GradScale.resize(0);

    if(!m_pFiffInfo || m_listStage2PriorProjs.isEmpty()) {
        return;
    }

    QStringList magChNames;
    QStringList gradChNames;

    for(int i = 0; i < m_vecMagLocalRows.size(); ++i) {
        const int megRow = m_vecMagLocalRows.at(i);
        magChNames << m_pFiffInfo->chs.at(m_vecMegPicks.at(megRow)).ch_name;
    }

    for(int i = 0; i < m_vecGradLocalRows.size(); ++i) {
        const int megRow = m_vecGradLocalRows.at(i);
        gradChNames << m_pFiffInfo->chs.at(m_vecMegPicks.at(megRow)).ch_name;
    }

    m_matStage2PriorBasisMag = buildPriorBasisFromProjectors(m_listStage2PriorProjs,
                                                             magChNames,
                                                             m_pFiffInfo->bads,
                                                             m_settings.stage2.iPriorRankMag);
    m_matStage2PriorBasisGrad = buildPriorBasisFromProjectors(m_listStage2PriorProjs,
                                                              gradChNames,
                                                              m_pFiffInfo->bads,
                                                              m_settings.stage2.iPriorRankGrad);
}

//=============================================================================================================

void RtAoeMeg::resetStage3State()
{
    const int nMag = m_vecMagLocalRows.size();
    const int nGrad = m_vecGradLocalRows.size();

    m_matStage3CovMag = nMag > 0 ? MatrixXd::Identity(nMag, nMag) * 1e-12 : MatrixXd(0, 0);
    m_matStage3CovGrad = nGrad > 0 ? MatrixXd::Identity(nGrad, nGrad) * 1e-12 : MatrixXd(0, 0);
    m_matStage3PhysicalBasisMag = buildMeanFieldBasis(m_pFiffInfo, m_vecMegPicks, m_vecMagLocalRows, m_settings.stage3.iMeanfieldOrder);
    m_matStage3PhysicalBasisGrad = buildMeanFieldBasis(m_pFiffInfo, m_vecMegPicks, m_vecGradLocalRows, m_settings.stage3.iMeanfieldOrder);
    m_matStage3StatBasisMag = MatrixXd(nMag, 0);
    m_matStage3StatBasisGrad = MatrixXd(nGrad, 0);
    m_vecStage3MagScale.resize(0);
    m_vecStage3GradScale.resize(0);
    m_dStage3PrevBandPowerMag = 0.0;
    m_dStage3PrevBandPowerGrad = 0.0;
    m_dStage3PrevRhoMag = 0.0;
    m_dStage3PrevRhoGrad = 0.0;
    m_iStage3CooldownMag = 0;
    m_iStage3CooldownGrad = 0;
}

//=============================================================================================================

void RtAoeMeg::rebuildChannelSelection()
{
    m_vecMegPicks.clear();
    m_vecMagLocalRows.clear();
    m_vecGradLocalRows.clear();

    if(!m_pFiffInfo) {
        return;
    }

    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        const FiffChInfo& channelInfo = m_pFiffInfo->chs.at(i);
        const bool isBad = m_pFiffInfo->bads.contains(channelInfo.ch_name);

        if(isBad || channelInfo.kind != FIFFV_MEG_CH) {
            continue;
        }

        if(channelInfo.unit == FIFF_UNIT_T) {
            m_vecMagLocalRows.append(m_vecMegPicks.size());
            m_vecMegPicks.append(i);
        } else if(channelInfo.unit == FIFF_UNIT_T_M) {
            m_vecGradLocalRows.append(m_vecMegPicks.size());
            m_vecMegPicks.append(i);
        }
    }
}

//=============================================================================================================

AOEMegProcessResult RtAoeMeg::process(const MatrixXd& matData)
{
    AOEMegProcessResult result;
    result.matData = matData;

    m_diagnostics.iProcessedBlocks += 1;
    m_diagnostics.iReceivedSamples += matData.cols();
    m_diagnostics.iLastBlockSamples = matData.cols();
    m_diagnostics.iChannelCount = matData.rows();
    m_diagnostics.bStage1Frozen = false;
    m_diagnostics.bStage2UpdateSkipped = true;
    m_diagnostics.bStage3UpdateSkipped = true;
    m_diagnostics.bMotionInputAvailable = m_bHasMotionVector;
    m_diagnostics.dRhoStage1 = 0.0;
    m_diagnostics.dRhoStage2 = 0.0;
    m_diagnostics.dRhoStage3 = 0.0;
    m_diagnostics.dRhoStage2Mag = 0.0;
    m_diagnostics.dRhoStage2Grad = 0.0;
    m_diagnostics.dRhoStage3Mag = 0.0;
    m_diagnostics.dRhoStage3Grad = 0.0;
    m_diagnostics.dStage1BadURate = 0.0;
    m_diagnostics.dStage1PredRatioMean = 0.0;
    m_diagnostics.dStage1UpdateRate = 0.0;
    m_diagnostics.dStage1ClipRate = 0.0;
    m_diagnostics.dStage1UNormMean = 0.0;
    m_diagnostics.dStage1ChunkAcceptRate = 0.0;
    m_diagnostics.dStage1ResidualAcceptRatio = 1.0;
    m_diagnostics.iStage2RequestedRankMag = m_settings.stage2.mag.iSspRank;
    m_diagnostics.iStage2RequestedRankGrad = m_settings.stage2.grad.iSspRank;
    m_diagnostics.iStage2ActiveRankMag = static_cast<int>(m_matStage2BasisMag.cols());
    m_diagnostics.iStage2ActiveRankGrad = static_cast<int>(m_matStage2BasisGrad.cols());
    m_diagnostics.iStage2PriorRankMag = static_cast<int>(m_matStage2PriorBasisMag.cols());
    m_diagnostics.iStage2PriorRankGrad = static_cast<int>(m_matStage2PriorBasisGrad.cols());
    m_diagnostics.bStage2AppliedMag = false;
    m_diagnostics.bStage2AppliedGrad = false;
    m_diagnostics.bStage2CovUpdatedMag = false;
    m_diagnostics.bStage2CovUpdatedGrad = false;
    m_diagnostics.bStage2PriorLoaded = !m_listStage2PriorProjs.isEmpty();
    m_diagnostics.dStage2CovTraceMag = m_matStage2CovMag.size() > 0 ? m_matStage2CovMag.trace() : 0.0;
    m_diagnostics.dStage2CovTraceGrad = m_matStage2CovGrad.size() > 0 ? m_matStage2CovGrad.trace() : 0.0;
    m_diagnostics.iStage3SelectedMag = 0;
    m_diagnostics.iStage3SelectedGrad = 0;
    m_diagnostics.iStage3PhysicalRankMag = static_cast<int>(m_matStage3PhysicalBasisMag.cols());
    m_diagnostics.iStage3PhysicalRankGrad = static_cast<int>(m_matStage3PhysicalBasisGrad.cols());
    m_diagnostics.iStage3StatRankMag = static_cast<int>(m_matStage3StatBasisMag.cols());
    m_diagnostics.iStage3StatRankGrad = static_cast<int>(m_matStage3StatBasisGrad.cols());
    m_diagnostics.iStage3CombinedRankMag = 0;
    m_diagnostics.iStage3CombinedRankGrad = 0;
    m_diagnostics.bStage3AppliedMag = false;
    m_diagnostics.bStage3AppliedGrad = false;
    m_diagnostics.bStage3CovUpdatedMag = false;
    m_diagnostics.bStage3CovUpdatedGrad = false;
    m_diagnostics.dStage3CovTraceMag = m_matStage3CovMag.size() > 0 ? m_matStage3CovMag.trace() : 0.0;
    m_diagnostics.dStage3CovTraceGrad = m_matStage3CovGrad.size() > 0 ? m_matStage3CovGrad.trace() : 0.0;
    m_diagnostics.dStage3BandPowerMag = 0.0;
    m_diagnostics.dStage3BandPowerGrad = 0.0;
    m_diagnostics.sStage1Status = "idle";
    m_diagnostics.sStage2Status = "off";
    m_diagnostics.sStage2PriorStatus = m_listStage2PriorProjs.isEmpty() ? "none" : "loaded";
    m_diagnostics.sStage3Status = "off";
    m_diagnostics.sStatusText = "AOE-MEG waiting";

    if(m_vecMegPicks.isEmpty()) {
        m_diagnostics.sStatusText = "AOE-MEG no MEG channels";
        result.diagnostics = m_diagnostics;
        return result;
    }

    if(m_matStage1B.rows() != m_vecMegPicks.size() || m_matStage1P.rows() != 6) {
        resetStage1State();
    }

    if(m_matStage2CovMag.rows() != m_vecMagLocalRows.size() || m_matStage2CovGrad.rows() != m_vecGradLocalRows.size()) {
        resetStage2State();
    }

    if(m_matStage3CovMag.rows() != m_vecMagLocalRows.size() || m_matStage3CovGrad.rows() != m_vecGradLocalRows.size()) {
        resetStage3State();
    }

    MatrixXd matMeg = pickRows(matData, m_vecMegPicks);
    MatrixXd matAfterStage1 = matMeg;

    if(m_settings.stage1.bEnabled && m_bHasMotionVector && m_vecLatestMotion.size() == 6 && m_vecLatestMotion.allFinite()) {
        const MatrixXd matMotion = m_vecLatestMotion.replicate(1, matMeg.cols());
        const VectorXd motionNormRaw = matMotion.colwise().norm().transpose();
        const MatrixXd motionZRaw = robustZScoreRows(matMotion).cwiseAbs();

        int badMotionCount = 0;
        for(Index t = 0; t < matMotion.cols(); ++t) {
            const bool badSmall = motionNormRaw(t) < m_settings.stage1.dMotionMinNorm;
            const bool badLarge = motionZRaw.col(t).maxCoeff() > m_settings.stage1.dMotionMaxAbsZ
                    || !matMotion.col(t).allFinite();
            badMotionCount += (badSmall || badLarge) ? 1 : 0;
        }

        bool freezeStage1 = m_settings.stage1.bFreezeOnBadMotion
                && matMotion.cols() > 0
                && (static_cast<double>(badMotionCount) / static_cast<double>(matMotion.cols()))
                    > m_settings.stage1.dRlsFreezeBadChunkRatio;

        MatrixXd matStage1Work = matMeg;

        if(m_settings.stage1.bNormalizeOutput && m_settings.bNormalizeByType) {
            if(!m_vecMagLocalRows.isEmpty()) {
                MatrixXd matMag = pickRows(matStage1Work, m_vecMagLocalRows);
                m_vecStage1MagScale = updateScale(m_vecStage1MagScale,
                                                  matMag,
                                                  m_settings.stage1.dScaleAlphaMag,
                                                  m_settings.stage1.dScaleFloorMag);
                matMag = matMag.array().colwise() / m_vecStage1MagScale.array();
                assignRows(matStage1Work, m_vecMagLocalRows, matMag);
            }

            if(!m_vecGradLocalRows.isEmpty()) {
                MatrixXd matGrad = pickRows(matStage1Work, m_vecGradLocalRows);
                m_vecStage1GradScale = updateScale(m_vecStage1GradScale,
                                                   matGrad,
                                                   m_settings.stage1.dScaleAlphaGrad,
                                                   m_settings.stage1.dScaleFloorGrad);
                matGrad = matGrad.array().colwise() / m_vecStage1GradScale.array();
                assignRows(matStage1Work, m_vecGradLocalRows, matGrad);
            }
        }

        Stage1Context stage1State{m_matStage1B, m_matStage1P, m_vecStage1UMean, m_vecStage1UScale};
        const Stage1ChunkOutcome outcome = processStage1Chunk(matStage1Work,
                                                              matMotion,
                                                              freezeStage1,
                                                              stage1State,
                                                              m_settings.stage1);

        MatrixXd matCandidate = outcome.residual;

        if(m_settings.stage1.bNormalizeOutput && m_settings.bNormalizeByType) {
            if(!m_vecMagLocalRows.isEmpty() && m_vecStage1MagScale.size() == m_vecMagLocalRows.size()) {
                for(int i = 0; i < m_vecMagLocalRows.size(); ++i) {
                    matCandidate.row(m_vecMagLocalRows.at(i)) *= m_vecStage1MagScale(i);
                }
            }

            if(!m_vecGradLocalRows.isEmpty() && m_vecStage1GradScale.size() == m_vecGradLocalRows.size()) {
                for(int i = 0; i < m_vecGradLocalRows.size(); ++i) {
                    matCandidate.row(m_vecGradLocalRows.at(i)) *= m_vecStage1GradScale(i);
                }
            }
        }

        const double residualAcceptRatio = safeEnergy(matCandidate) / safeEnergy(matMeg);
        const double rhoCandidate = ratioRemoved(matMeg, matCandidate);
        const bool stage1BadChunk = !std::isfinite(residualAcceptRatio)
                || residualAcceptRatio > m_settings.stage1.dAcceptEnergyRatio
                || rhoCandidate < m_settings.stage1.dMinRho;

        if(stage1BadChunk || residualAcceptRatio > m_settings.stage1.dRlsResidualAcceptRatio) {
            matCandidate = matMeg;
            freezeStage1 = true;
        } else {
            freezeStage1 = freezeStage1 || outcome.metrics.freezeChunk || outcome.metrics.chunkAccept < 0.5;
        }

        matAfterStage1 = matCandidate;
        m_diagnostics.bStage1Frozen = freezeStage1;
        m_diagnostics.dRhoStage1 = std::max(-5.0, std::min(1.0, ratioRemoved(matMeg, matCandidate)));
        m_diagnostics.dStage1BadURate = outcome.metrics.badURate;
        m_diagnostics.dStage1PredRatioMean = outcome.metrics.predRatioMean;
        m_diagnostics.dStage1UpdateRate = outcome.metrics.updateRate;
        m_diagnostics.dStage1ClipRate = outcome.metrics.clipRate;
        m_diagnostics.dStage1UNormMean = outcome.metrics.uNormMean;
        m_diagnostics.dStage1ChunkAcceptRate = freezeStage1 ? 0.0 : std::max(0.0, outcome.metrics.chunkAccept);
        m_diagnostics.dStage1ResidualAcceptRatio = residualAcceptRatio;
        m_diagnostics.sStage1Status = freezeStage1 ? "frozen" : "applied";
    } else if(m_settings.stage1.bEnabled) {
        m_diagnostics.sStage1Status = "waiting-motion";
    } else {
        m_diagnostics.sStage1Status = "off";
    }

    MatrixXd matAfterStage2 = matAfterStage1;
    const bool stage2PriorConfigured = m_settings.stage2.bUsePrior
            && ((!m_vecMagLocalRows.isEmpty() && m_matStage2PriorBasisMag.cols() > 0)
                || (!m_vecGradLocalRows.isEmpty() && m_matStage2PriorBasisGrad.cols() > 0));
    const bool stage2Configured = m_settings.stage2.bEnabled
            && (((m_settings.stage2.mag.iSspRank > 0 || (m_settings.stage2.bUsePrior && m_matStage2PriorBasisMag.cols() > 0)) && !m_vecMagLocalRows.isEmpty())
                || ((m_settings.stage2.grad.iSspRank > 0 || (m_settings.stage2.bUsePrior && m_matStage2PriorBasisGrad.cols() > 0)) && !m_vecGradLocalRows.isEmpty()));

    if(stage2Configured) {
        const double sfreq = m_pFiffInfo ? m_pFiffInfo->sfreq : 0.0;
        QStringList statusParts;
        m_diagnostics.bStage2UpdateSkipped = false;

        if(!m_vecMagLocalRows.isEmpty()) {
            Stage2TypeState state{m_matStage2CovMag, m_matStage2BasisMag, m_matStage2PriorBasisMag, m_vecStage2MagScale};
            const Stage2TypeOutcome outcome = applyStage2Type(pickRows(matAfterStage2, m_vecMagLocalRows),
                                                              state,
                                                              m_settings.stage2.mag,
                                                              m_settings.stage2,
                                                              m_settings.stage2.bUsePrior,
                                                              m_settings.stage2.bNormalizeByType,
                                                              m_settings.stage3.mag.dScaleAlpha,
                                                              m_settings.stage3.mag.dScaleFloor,
                                                              sfreq,
                                                              m_diagnostics.iProcessedBlocks);
            assignRows(matAfterStage2, m_vecMagLocalRows, outcome.data);
            m_diagnostics.dRhoStage2Mag = outcome.rho;
            m_diagnostics.iStage2ActiveRankMag = outcome.activeRank;
            m_diagnostics.iStage2PriorRankMag = outcome.priorRank;
            m_diagnostics.bStage2AppliedMag = outcome.applied;
            m_diagnostics.bStage2CovUpdatedMag = outcome.covarianceUpdated;
            m_diagnostics.dStage2CovTraceMag = outcome.covarianceTrace;
            statusParts << QString("mag:%1").arg(outcome.status);
        }

        if(!m_vecGradLocalRows.isEmpty()) {
            Stage2TypeState state{m_matStage2CovGrad, m_matStage2BasisGrad, m_matStage2PriorBasisGrad, m_vecStage2GradScale};
            const Stage2TypeOutcome outcome = applyStage2Type(pickRows(matAfterStage2, m_vecGradLocalRows),
                                                              state,
                                                              m_settings.stage2.grad,
                                                              m_settings.stage2,
                                                              m_settings.stage2.bUsePrior,
                                                              m_settings.stage2.bNormalizeByType,
                                                              m_settings.stage3.grad.dScaleAlpha,
                                                              m_settings.stage3.grad.dScaleFloor,
                                                              sfreq,
                                                              m_diagnostics.iProcessedBlocks);
            assignRows(matAfterStage2, m_vecGradLocalRows, outcome.data);
            m_diagnostics.dRhoStage2Grad = outcome.rho;
            m_diagnostics.iStage2ActiveRankGrad = outcome.activeRank;
            m_diagnostics.iStage2PriorRankGrad = outcome.priorRank;
            m_diagnostics.bStage2AppliedGrad = outcome.applied;
            m_diagnostics.bStage2CovUpdatedGrad = outcome.covarianceUpdated;
            m_diagnostics.dStage2CovTraceGrad = outcome.covarianceTrace;
            statusParts << QString("grad:%1").arg(outcome.status);
        }

        m_diagnostics.dRhoStage2 = ratioRemoved(matAfterStage1, matAfterStage2);
        m_diagnostics.sStage2Status = statusParts.join(" | ");
        m_diagnostics.sStage2PriorStatus = m_settings.stage2.bUsePrior
                ? QString("enabled mag=%1 grad=%2").arg(m_diagnostics.iStage2PriorRankMag).arg(m_diagnostics.iStage2PriorRankGrad)
                : (m_listStage2PriorProjs.isEmpty() ? "none" : "loaded-disabled");
    } else if(m_settings.stage2.bEnabled && stage2PriorConfigured) {
        m_diagnostics.sStage2Status = "prior-only";
    } else if(m_settings.stage2.bEnabled) {
        m_diagnostics.sStage2Status = "enabled-but-rank=0";
    } else {
        m_diagnostics.sStage2Status = "off";
    }

    MatrixXd matAfterStage3 = matAfterStage2;
    const bool stage3Configured = m_settings.stage3.bEnabled
            && ((!m_vecMagLocalRows.isEmpty() && m_matStage3PhysicalBasisMag.rows() == m_vecMagLocalRows.size())
                || (!m_vecGradLocalRows.isEmpty() && m_matStage3PhysicalBasisGrad.rows() == m_vecGradLocalRows.size()));

    if(stage3Configured) {
        const double sfreq = m_pFiffInfo ? m_pFiffInfo->sfreq : 0.0;
        QStringList statusParts;
        m_diagnostics.bStage3UpdateSkipped = false;

        if(!m_vecMagLocalRows.isEmpty()) {
            Stage3TypeState state{m_matStage3CovMag,
                                  m_matStage3PhysicalBasisMag,
                                  m_matStage3StatBasisMag,
                                  m_vecStage3MagScale,
                                  m_dStage3PrevBandPowerMag,
                                  m_dStage3PrevRhoMag,
                                  m_iStage3CooldownMag};
            const Stage3TypeOutcome outcome = applyStage3Type(pickRows(matAfterStage3, m_vecMagLocalRows),
                                                              state,
                                                              m_settings.stage3.mag,
                                                              m_settings.stage3,
                                                              m_settings.stage3.bNormalizeByType,
                                                              sfreq,
                                                              m_diagnostics.iProcessedBlocks);
            assignRows(matAfterStage3, m_vecMagLocalRows, outcome.data);
            m_diagnostics.dRhoStage3Mag = outcome.rho;
            m_diagnostics.iStage3SelectedMag = outcome.selectedCount;
            m_diagnostics.iStage3PhysicalRankMag = outcome.physicalRank;
            m_diagnostics.iStage3StatRankMag = outcome.statisticalRank;
            m_diagnostics.iStage3CombinedRankMag = outcome.combinedRank;
            m_diagnostics.bStage3AppliedMag = outcome.applied;
            m_diagnostics.bStage3CovUpdatedMag = outcome.covarianceUpdated;
            m_diagnostics.dStage3CovTraceMag = outcome.covarianceTrace;
            m_diagnostics.dStage3BandPowerMag = outcome.bandPower;
            statusParts << QString("mag:%1").arg(outcome.status);
        }

        if(!m_vecGradLocalRows.isEmpty()) {
            Stage3TypeState state{m_matStage3CovGrad,
                                  m_matStage3PhysicalBasisGrad,
                                  m_matStage3StatBasisGrad,
                                  m_vecStage3GradScale,
                                  m_dStage3PrevBandPowerGrad,
                                  m_dStage3PrevRhoGrad,
                                  m_iStage3CooldownGrad};
            const Stage3TypeOutcome outcome = applyStage3Type(pickRows(matAfterStage3, m_vecGradLocalRows),
                                                              state,
                                                              m_settings.stage3.grad,
                                                              m_settings.stage3,
                                                              m_settings.stage3.bNormalizeByType,
                                                              sfreq,
                                                              m_diagnostics.iProcessedBlocks);
            assignRows(matAfterStage3, m_vecGradLocalRows, outcome.data);
            m_diagnostics.dRhoStage3Grad = outcome.rho;
            m_diagnostics.iStage3SelectedGrad = outcome.selectedCount;
            m_diagnostics.iStage3PhysicalRankGrad = outcome.physicalRank;
            m_diagnostics.iStage3StatRankGrad = outcome.statisticalRank;
            m_diagnostics.iStage3CombinedRankGrad = outcome.combinedRank;
            m_diagnostics.bStage3AppliedGrad = outcome.applied;
            m_diagnostics.bStage3CovUpdatedGrad = outcome.covarianceUpdated;
            m_diagnostics.dStage3CovTraceGrad = outcome.covarianceTrace;
            m_diagnostics.dStage3BandPowerGrad = outcome.bandPower;
            statusParts << QString("grad:%1").arg(outcome.status);
        }

        m_diagnostics.dRhoStage3 = ratioRemoved(matAfterStage2, matAfterStage3);
        m_diagnostics.sStage3Status = statusParts.join(" | ");
    } else if(m_settings.stage3.bEnabled) {
        m_diagnostics.sStage3Status = "enabled-but-basis-missing";
    } else {
        m_diagnostics.sStage3Status = "off";
    }

    assignRows(result.matData, m_vecMegPicks, matAfterStage3);
    m_diagnostics.sStatusText = makePipelineStatus(m_diagnostics);

    result.diagnostics = m_diagnostics;
    return result;
}

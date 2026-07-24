//=============================================================================================================
/**
 * @file     adaptivetsss.cpp
 * @brief    Real-time adaptive tSSS-style processor definitions (SSS + trigger-aware temporal suppression).
 */

#include "adaptivetsss.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::RowVectorXi;

namespace RTPROCESSINGLIB
{
namespace
{
//-------------------------------------------------------------------------------------------------
static MatrixXd pickRows(const MatrixXd& M, const RowVectorXi& picks)
{
    MatrixXd out(picks.size(), M.cols());
    for(int i = 0; i < picks.size(); ++i) {
        out.row(i) = M.row(picks(i));
    }
    return out;
}

//-------------------------------------------------------------------------------------------------
static void setPickedRows(MatrixXd& dst, const RowVectorXi& picks, const MatrixXd& pickedData)
{
    for(int i = 0; i < picks.size(); ++i) {
        dst.row(picks(i)) = pickedData.row(i);
    }
}

//-------------------------------------------------------------------------------------------------
static bool loadMatrixCsv(const QString& path, MatrixXd& out)
{
    if(path.isEmpty()) {
        return false;
    }

    QFile f(path);
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream ts(&f);
    std::vector<std::vector<double>> rows;
    rows.reserve(512);

    while(!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if(line.isEmpty()) {
            continue;
        }

        QStringList parts = line.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        std::vector<double> vals;
        vals.reserve(parts.size());

        bool ok = true;
        for(const auto& p : parts) {
            double v = p.toDouble(&ok);
            if(!ok) {
                break;
            }
            vals.push_back(v);
        }

        if(ok && !vals.empty()) {
            rows.push_back(std::move(vals));
        }
    }

    if(rows.empty()) {
        return false;
    }

    const int r = static_cast<int>(rows.size());
    const int c = static_cast<int>(rows[0].size());
    for(int i = 1; i < r; ++i) {
        if(static_cast<int>(rows[i].size()) != c) {
            return false;
        }
    }

    out.resize(r, c);
    for(int i = 0; i < r; ++i) {
        for(int j = 0; j < c; ++j) {
            out(i, j) = rows[i][j];
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------------
static bool invSqrtAndSqrtSPD(const MatrixXd& C,
                             double epsFloor,
                             MatrixXd& invSqrt,
                             MatrixXd& sqrtM)
{
    if(C.rows() == 0 || C.rows() != C.cols()) {
        return false;
    }

    Eigen::SelfAdjointEigenSolver<MatrixXd> es(C);
    if(es.info() != Eigen::Success) {
        return false;
    }

    VectorXd eval = es.eigenvalues();
    MatrixXd evec = es.eigenvectors();

    const double maxev = std::max(1e-30, eval.maxCoeff());
    const double floorv = std::max(epsFloor, 1e-12 * maxev);

    VectorXd inv(eval.size());
    VectorXd sq(eval.size());
    for(int i = 0; i < eval.size(); ++i) {
        const double v = std::max(floorv, std::max(0.0, eval(i)));
        inv(i) = 1.0 / std::sqrt(v);
        sq(i)  = std::sqrt(v);
    }

    invSqrt = evec * inv.asDiagonal() * evec.transpose();
    sqrtM   = evec * sq.asDiagonal()  * evec.transpose();
    return true;
}

//-------------------------------------------------------------------------------------------------
// Clip eigenvalues of a symmetric matrix into [lo, hi] to keep it as a numerically safe projector.
// This is critical when smoothing multiple projectors, as linear interpolation is not guaranteed
// to preserve idempotency and can introduce gain > 1 ("爆振").
static void clipSymmetricEigenvalues(MatrixXd& M, double lo, double hi)
{
    if(M.rows() == 0 || M.rows() != M.cols()) {
        return;
    }
    M = 0.5 * (M + M.transpose());

    Eigen::SelfAdjointEigenSolver<MatrixXd> es(M);
    if(es.info() != Eigen::Success) {
        return;
    }

    VectorXd eval = es.eigenvalues();
    MatrixXd evec = es.eigenvectors();
    for(int i = 0; i < eval.size(); ++i) {
        double v = eval(i);
        if(!std::isfinite(v)) {
            v = 0.0;
        }
        v = std::min(hi, std::max(lo, v));
        eval(i) = v;
    }
    M = evec * eval.asDiagonal() * evec.transpose();
    M = 0.5 * (M + M.transpose());
}

} // namespace

//=============================================================================================================
AdaptiveTSSS::AdaptiveTSSS()
{
    reset();
}

//=============================================================================================================
void AdaptiveTSSS::reset()
{
    m_vecPicksLast.resize(0);
    m_iKernelVersionLast = std::numeric_limits<int>::min();

    m_matGIn.resize(0,0);
    m_matGOut.resize(0,0);
    m_matPinvW.resize(0,0);

    m_nPick = 0;
    m_nIn = 0;
    m_nOut = 0;

    m_bHasWeights = false;
    m_vecWFull.resize(0);
    m_vecWPick.resize(0);

    m_iChunkCounter = 0;

    m_iWinSamp = 0;
    m_iRingPos = 0;
    m_iRingFill = 0;
    m_ringAinLP.resize(0,0);
    m_ringAoutLP.resize(0,0);

    m_bUseCoeffLP = false;
    m_dCoeffLpHz = 0.0;
    m_dCoeffLpAlpha = 0.0;
    m_stateAinLP.resize(0);
    m_stateAoutLP.resize(0);

    m_bProjReady = false;
    m_matR.resize(0,0);
    m_matRSmoothed.resize(0,0);
    m_bHasRSmoothed = false;

    m_pendingTrigOffsets.clear();
    m_iProtectCarrySamp = 0;
    m_dFreezeLeftSec = 0.0;

    // External-noise tracking
    m_matCoo.resize(0,0);
    m_matUout.resize(0,0);
    m_iExtK = 0;
    m_bExtReady = false;
    m_dExtResScaleLast = 0.0;

    m_iKRemovedLast = 0;
    m_dCorrMaxLast = 0.0;
}

//=============================================================================================================
Eigen::MatrixXd AdaptiveTSSS::calculate(const Eigen::MatrixXd& matData,
                                       const RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel,
                                       const Eigen::RowVectorXi& vecPicks)
{
    return calculate(matData, kernel, vecPicks, nullptr, nullptr);
}

//=============================================================================================================
Eigen::MatrixXd AdaptiveTSSS::calculate(const Eigen::MatrixXd& matData,
                                       const RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel,
                                       const Eigen::RowVectorXi& vecPicks,
                                       Eigen::MatrixXd* pOutSSSOnly,
                                       RTPROCESSINGLIB::AdaptiveTSSSDebugInfo* pDbg)
{
    if(matData.rows() == 0 || matData.cols() == 0) {
        return matData;
    }

    // Resolve picks (if empty -> all channels)
    RowVectorXi picks = vecPicks;
    if(picks.size() == 0) {
        picks.resize(matData.rows());
        for(int i = 0; i < matData.rows(); ++i) {
            picks(i) = i;
        }
    }

    prepareIfNeeded(matData, kernel, picks);
    if(m_matPinvW.size() == 0 || m_matGIn.size() == 0 || m_matGOut.size() == 0) {
        if(pOutSSSOnly) {
            *pOutSSSOnly = matData;
        }
        if(pDbg) {
            *pDbg = AdaptiveTSSSDebugInfo();
        }
        return matData;
    }

    const int nSamp = static_cast<int>(matData.cols());
    const double chunkDurSec = (m_dFsHz > 1.0 ? static_cast<double>(nSamp) / m_dFsHz : 0.0);

    // If this chunk has triggers, extend freeze window (avoid learning evoked)
    if(!m_pendingTrigOffsets.empty() && m_dFreezeUpdateMs > 0.0) {
        m_dFreezeLeftSec = std::max(m_dFreezeLeftSec, m_dFreezeUpdateMs / 1000.0);
    }

    // Pick MEG data and apply weights for spatial solve
    MatrixXd Xpick = pickRows(matData, picks); // (nPick x nSamp)
    MatrixXd Xw = Xpick;
    if(m_vecWPick.size() == m_nPick) {
        for(int i = 0; i < m_nPick; ++i) {
            Xw.row(i) *= m_vecWPick(i);
        }
    }

    // Spatial coefficients
    const int nCoeff = static_cast<int>(m_matPinvW.rows());
    MatrixXd coeff = m_matPinvW * Xw; // (nCoeff x nSamp)
    MatrixXd Ain  = coeff.topRows(m_nIn);
    MatrixXd Aout = coeff.middleRows(m_nIn, m_nOut);

    // Optional sensor-residual “feature” coefficients (eSSS-style)
    const int nFeat = std::max(0, nCoeff - (m_nIn + m_nOut));
    MatrixXd Afeat;
    if(nFeat > 0) {
        Afeat = coeff.bottomRows(nFeat);
    }

    // Spatial reconstructions
    // SSS-only (internal) matches standard SSS output.
    MatrixXd XsssPick  = m_matGIn * Ain;
    // Maxwell external field reconstruction.
    MatrixXd YoutPick = (m_nOut > 0 ? (m_matGOut * Aout).eval() : MatrixXd::Zero(m_nPick, nSamp).eval());
    // Maxwell FULL-FIT (internal + external) is the best spatial fit *within the Maxwell basis*.
    // We use it as a protected-sample fallback base and for residual-feature learning.
    MatrixXd XfullPickMaxwell = XsssPick + YoutPick;
    // Keep the legacy name used throughout the file (Maxwell FULL-FIT only; does NOT include feature terms).
    MatrixXd XfullPick = XfullPickMaxwell;

    // Sensor-residual feature reconstruction (model-mismatch interference).
    MatrixXd XfeatPick = MatrixXd::Zero(m_nPick, nSamp);
    if(nFeat > 0 && m_bFeatCancel && m_bFeatReady && m_matUfeat.rows() == m_nPick && m_matUfeat.cols() >= nFeat) {
        XfeatPick = m_matUfeat.leftCols(nFeat) * Afeat;
    }

    // eSSS-inspired external noise cancellation on the external field:
    // Learn a dominant noise subspace in Aout (from past chunks, excluding protected samples),
    // remove it, and optionally add back the residual external field to preserve evoked components that leak into Aout.
    MatrixXd YresPick;
    bool extReadyNow = (m_bExtNoiseCancel && m_bExtReady && m_nOut > 0 && m_matUout.cols() > 0);
    double extResScale = 0.0;
    if(extReadyNow) {
        const MatrixXd AoutProj = m_matUout * (m_matUout.transpose() * Aout);
        const MatrixXd AoutRes = Aout - AoutProj;
        YresPick = m_matGOut * AoutRes;
        extResScale = 1.0;
        if(m_dExtMaxResidualRatio > 0.0) {
            const double Ein = std::max(1e-30, XsssPick.squaredNorm());
            const double Eres = YresPick.squaredNorm();
            const double Emax = m_dExtMaxResidualRatio * Ein;
            if(Eres > Emax) {
                extResScale = std::sqrt(Emax / std::max(1e-30, Eres));
                YresPick *= extResScale;
            }
        }
    } else {
        YresPick = MatrixXd::Zero(m_nPick, nSamp);
    }
    m_dExtResScaleLast = (extReadyNow ? extResScale : 0.0);

    // Protected fallback matrix (per chunk) for later blending
    MatrixXd XprotFallbackPick;
    if(m_bProtectFallbackFullFit) {
        // Use the Maxwell FULL-FIT as protected fallback. Do NOT add external residual here:
        // protected regions are dominated by artifacts; adding residual can inflate amplitude.
        XprotFallbackPick = XfullPick;
    } else {
        XprotFallbackPick = XsssPick;
    }

    MatrixXd outSSS = matData;
    setPickedRows(outSSS, picks, XsssPick);

    if(pOutSSSOnly) {
        *pOutSSSOnly = outSSS;
    }

    // Protection weights for blending (0..1). Must carry across chunks.
    const int carryInSamp = m_iProtectCarrySamp;
    std::vector<double> wProt;
    buildProtectionWeights(nSamp, wProt);
    // Optionally shrink over-wide protection windows: if the Maxwell residual
    // drops back to baseline, end protection early.
    if(m_bAdaptiveProtShrink && !wProt.empty() && m_dFsHz > 1.0) {
        const int stableS = static_cast<int>(std::round((m_dProtStableMs / 1000.0) * m_dFsHz));
        const int minHoldS = static_cast<int>(std::round((m_dProtMinHoldMs / 1000.0) * m_dFsHz));
        const int taperS = static_cast<int>(std::round((m_dTrigTaperMs / 1000.0) * m_dFsHz));

        if(stableS > 0) {
            // Residual ratio r(t) = ||x - x_fullfit(Maxwell)||^2 / ||x||^2
            const MatrixXd R = (Xpick - XfullPickMaxwell);
            VectorXd num = R.colwise().squaredNorm();
            VectorXd den = Xpick.colwise().squaredNorm();
            den.array() += 1e-30;
            VectorXd rr = num.array() / den.array();

            int t = 0;
            while(t < nSamp) {
                // find a protected segment
                while(t < nSamp && wProt[static_cast<size_t>(t)] <= 0.0) {
                    ++t;
                }
                if(t >= nSamp) break;
                const int segStart = t;
                while(t < nSamp && wProt[static_cast<size_t>(t)] > 0.0) {
                    ++t;
                }
                const int segEnd = t - 1;
                const int searchStart = std::min(segEnd, segStart + minHoldS);
                int cut = -1;
                int run = 0;
                for(int i = searchStart; i <= segEnd; ++i) {
                    if(rr(i) < m_dProtResidRatioThresh) {
                        ++run;
                        if(run >= stableS) {
                            cut = i - stableS + 1;
                            break;
                        }
                    } else {
                        run = 0;
                    }
                }
                if(cut >= 0 && cut < segEnd) {
                    // taper down from cut to cut+taperS then zero
                    for(int i = cut; i <= segEnd; ++i) {
                        if(taperS > 0) {
                            const int d = i - cut;
                            if(d <= taperS) {
                                const double w = 0.5 * (1.0 + std::cos(std::acos(-1.0) * static_cast<double>(d) / static_cast<double>(taperS)));
                                wProt[static_cast<size_t>(i)] = std::min(wProt[static_cast<size_t>(i)], w);
                            } else {
                                wProt[static_cast<size_t>(i)] = 0.0;
                            }
                        } else {
                            wProt[static_cast<size_t>(i)] = 0.0;
                        }
                    }
                }
            }
        }
    }

    // Recompute carry-out hard-protected samples based on (possibly) shrunk weights.
    int carryOutSamp = 0;
    if(!wProt.empty()) {
        for(int i = nSamp - 1; i >= 0; --i) {
            if(wProt[static_cast<size_t>(i)] >= 0.5) {
                ++carryOutSamp;
            } else {
                break;
            }
        }
    }
    m_iProtectCarrySamp = carryOutSamp;

    // Protected fallback safety: if the chosen fallback attenuates too much within hard-protected samples,
    // blend toward RAW to preserve evoked peaks (prevents GFP peak shrink).
    double protFullRawRatio = 1.0;
    double protRawMix = 0.0;
    double protGainScaleHard = 1.0;
    if(!wProt.empty()) {
        double Eraw = 0.0;
        double Efb  = 0.0;
        int nHard = 0;
        for(int t = 0; t < nSamp; ++t) {
            if(wProt[static_cast<size_t>(t)] >= 0.5) {
                Eraw += Xpick.col(t).squaredNorm();
                Efb  += XprotFallbackPick.col(t).squaredNorm();
                ++nHard;
            }
        }
        if(nHard > 0 && Eraw > 1e-30) {
            protFullRawRatio = std::sqrt(Efb / Eraw);

            // (1) Optional mixing toward RAW if FULL-FIT attenuates too much.
            // Default is disabled in the plugin configuration to avoid re-injecting artifacts.
            if(m_bProtRawSafety && protFullRawRatio < m_dProtTargetRatio) {
                protRawMix = std::min(m_dProtMaxRawMix,
                                     (m_dProtTargetRatio - protFullRawRatio) / std::max(1e-12, m_dProtTargetRatio));
            }

            // (2) Always prevent protected fallback from amplifying relative to RAW.
            if(m_bProtGainCap && protFullRawRatio > m_dProtGainMax) {
                protGainScaleHard = m_dProtGainMax / std::max(1e-12, protFullRawRatio);
            }
        }
    }


    // Update rings (exclude protected samples)
    if(m_iWinSamp > 0) {
        if(m_bUseCoeffLP) {
            if(m_stateAinLP.size() != m_nIn) {
                m_stateAinLP = VectorXd::Zero(m_nIn);
            }
            if(m_stateAoutLP.size() != m_nOut) {
                m_stateAoutLP = VectorXd::Zero(m_nOut);
            }
        }

        // External-noise covariance update (weighted, excludes hard-protected samples)
        const bool doExt = (m_bExtNoiseCancel && m_nOut > 0);
        // Sensor-residual covariance update (weighted, excludes hard-protected samples)
        // Residual here is defined as what the Maxwell bases cannot explain: R = X - (Gin*Ain + Gout*Aout).
        // This targets calibration/model-mismatch interference (eSSS “features”).
        const bool doFeat = (m_bFeatCancel);
        MatrixXd CooChunk;
        double denomCoo = 0.0;
        if(doExt) {
            CooChunk = MatrixXd::Zero(m_nOut, m_nOut);
        }

        // Residual-feature covariance is updated after the loop using a single GEMM.

        for(int t = 0; t < nSamp; ++t) {
            const double wp = (!wProt.empty() ? wProt[static_cast<size_t>(t)] : 0.0);

            // Hard exclusion for fully protected samples (pre/post window).
            if(wp >= 0.5) {
                continue;
            }

            VectorXd ain_t = Ain.col(t);
            VectorXd aout_t = Aout.col(t);

            if(m_bUseCoeffLP) {
                ain_t = lowpassStep(ain_t, m_stateAinLP);
                aout_t = lowpassStep(aout_t, m_stateAoutLP);
            }

            // Soft down-weighting for taper samples to reduce evoked leakage into the window
            // while not starving the ring buffer.
            const double wLearn = std::max(m_dLearnWeightFloor, 1.0 - wp);
            ain_t *= wLearn;
            aout_t *= wLearn;

            if(doExt) {
                denomCoo += wLearn * wLearn;
                CooChunk.noalias() += aout_t * aout_t.transpose();
            }

            // Residual-feature covariance is updated after the loop using a single GEMM:
            // CrrChunk = Rw * Rw^T, where Rw scales residual columns by wLearn.

            ringPush(ain_t, aout_t);
        }

        // Update external-noise covariance (EWMA)
        if(doExt && denomCoo > 1e-12) {
            if(m_matCoo.rows() != m_nOut || m_matCoo.cols() != m_nOut) {
                m_matCoo = MatrixXd::Zero(m_nOut, m_nOut);
            }
            const MatrixXd Cnorm = CooChunk / denomCoo;
            if(m_matCoo.size() == 0) {
                m_matCoo = Cnorm;
            } else {
                m_matCoo = m_dExtForget * m_matCoo + (1.0 - m_dExtForget) * Cnorm;
            }
        }

        // Update residual-feature covariance (EWMA)
        if(doFeat && m_nPick > 0) {
            if(m_matCrr.rows() != m_nPick || m_matCrr.cols() != m_nPick) {
                m_matCrr = MatrixXd::Zero(m_nPick, m_nPick);
            }

            VectorXd wLearnVec(nSamp);
            for(int t = 0; t < nSamp; ++t) {
                const double wp = (!wProt.empty() ? wProt[static_cast<size_t>(t)] : 0.0);
                wLearnVec(t) = std::max(m_dLearnWeightFloor, 1.0 - wp);
            }

            const double denom = wLearnVec.squaredNorm();
            if(denom > 1e-12) {
                // Residual in sensor space that Maxwell cannot explain: R = X - (Gin*Ain + Gout*Aout).
                // Update Crr via a single GEMM for performance.
                const MatrixXd R = (Xpick - XfullPickMaxwell);
                MatrixXd Rw = R.array().rowwise() * wLearnVec.transpose().array();
                const MatrixXd Crr = Rw * Rw.transpose();
                const MatrixXd Cnorm = Crr / denom;

                if(m_matCrr.size() == 0) {
                    m_matCrr = Cnorm;
                } else {
                    m_matCrr = m_dFeatForget * m_matCrr + (1.0 - m_dFeatForget) * Cnorm;
                }
            }
        }
    }

    // Update temporal projector if scheduled
    updateProjectorIfNeeded(chunkDurSec);

    // Update external-noise subspace (scheduled)
    if(m_bExtNoiseCancel && m_matCoo.size() > 0 && m_iExtUpdatePeriodChunks > 0 &&
       (m_iChunkCounter >= m_iWarmupChunks) && ((m_iChunkCounter % m_iExtUpdatePeriodChunks) == 0)) {
        Eigen::SelfAdjointEigenSolver<MatrixXd> es(m_matCoo);
        if(es.info() == Eigen::Success) {
            VectorXd vals = es.eigenvalues(); // ascending
            MatrixXd vecs = es.eigenvectors();
            const double total = vals.sum();
            const double vmax = (vals.size() > 0 ? vals.maxCoeff() : 0.0);
            int k = 0;
            if(total > 1e-30 && vmax > 1e-30 && m_iExtMaxDim > 0) {
                double cum = 0.0;
                for(int i = static_cast<int>(vals.size()) - 1; i >= 0; --i) {
                    if(vals(i) < 1e-12 * vmax) {
                        break;
                    }
                    cum += vals(i);
                    ++k;
                    if(cum / total >= m_dExtEnergyKeep) {
                        break;
                    }
                    if(k >= m_iExtMaxDim) {
                        break;
                    }
                }
            }
            k = std::min(k, std::max(0, m_iExtMaxDim));
            if(k > 0) {
                m_matUout = vecs.rightCols(k);
                m_iExtK = k;
                m_bExtReady = true;
            } else {
                m_matUout.resize(0,0);
                m_iExtK = 0;
                m_bExtReady = false;
            }
        } else {
            m_matUout.resize(0,0);
            m_iExtK = 0;
            m_bExtReady = false;
        }
    }

    // Update residual-feature basis (scheduled)
    if(m_bFeatCancel && m_matCrr.size() > 0 && m_iFeatUpdatePeriodChunks > 0 &&
       (m_iChunkCounter >= m_iWarmupChunks) && ((m_iChunkCounter % m_iFeatUpdatePeriodChunks) == 0)) {
        Eigen::SelfAdjointEigenSolver<MatrixXd> es(m_matCrr);
        if(es.info() == Eigen::Success) {
            VectorXd vals = es.eigenvalues(); // ascending
            MatrixXd vecs = es.eigenvectors();
            const double total = vals.sum();
            const double vmax = (vals.size() > 0 ? vals.maxCoeff() : 0.0);
            int k = 0;
            if(total > 1e-30 && vmax > 1e-30 && m_iFeatMaxDim > 0) {
                double cum = 0.0;
                for(int i = static_cast<int>(vals.size()) - 1; i >= 0; --i) {
                    if(vals(i) < 1e-12 * vmax) {
                        break;
                    }
                    cum += vals(i);
                    ++k;
                    if(cum / total >= m_dFeatEnergyKeep) {
                        break;
                    }
                    if(k >= m_iFeatMaxDim) {
                        break;
                    }
                }
            }

            k = std::min(k, std::max(0, m_iFeatMaxDim));
            if(k > 0) {
                m_matUfeat = vecs.rightCols(k);
                m_iFeatK = k;
                m_bFeatReady = true;
            } else {
                m_matUfeat.resize(0,0);
                m_iFeatK = 0;
                m_bFeatReady = false;
            }
        } else {
            m_matUfeat.resize(0,0);
            m_iFeatK = 0;
            m_bFeatReady = false;
        }

        // Rebuild spatial solver to include (or drop) feature nuisance regressors.
        rebuildSpatialSolver();
    }

    // Decide whether to apply temporal suppression
    const bool warmed = (m_iChunkCounter > m_iWarmupChunks);
    bool canApply = warmed && m_bProjReady;

    // If still warming and user wants spatial-only until warm, return SSS
    if(!warmed && m_bReturnSpatialUntilWarm) {
        if(pDbg) {
            AdaptiveTSSSDebugInfo d;
            d.e_total = Xpick.squaredNorm();
            d.e_out   = (m_matGOut * Aout).squaredNorm();
            d.R_out   = (d.e_total > 0.0 ? d.e_out / d.e_total : 0.0);
            d.gate_on = false;
            d.k_removed = m_iKRemovedLast;
            d.corr_max  = m_dCorrMaxLast;
            double pfSoft = 0.0;
            double pfHard = 0.0;
            if(!wProt.empty()) {
                for(double v : wProt) {
                    pfSoft += (v > 0.0 ? 1.0 : 0.0);
                    pfHard += (v >= 0.5 ? 1.0 : 0.0);
                }
                pfSoft /= static_cast<double>(wProt.size());
                pfHard /= static_cast<double>(wProt.size());
            }
            d.protect_frac_soft = pfSoft;
            d.protect_frac_hard = pfHard;
            d.protect_fullfit = m_bProtectFallbackFullFit;
            d.protect_carry_in_samp  = carryInSamp;
            d.protect_carry_out_samp = carryOutSamp;
            d.ext_reg_factor = m_dExtRegFactor;

            // External-noise subspace cancellation diagnostics
            d.ext_noise_ready = m_bExtReady;
            d.ext_noise_k = m_iExtK;
            d.ext_noise_energy_keep = m_dExtEnergyKeep;
            d.ext_res_scale = m_dExtResScaleLast;

            // Residual-feature nuisance regressors (eSSS-style basis augmentation)
            d.feat_ready = m_bFeatReady;
            d.feat_k = m_iFeatK;
            d.feat_energy_keep = m_dFeatEnergyKeep;
            d.feat_reg_factor = m_dFeatRegFactor;
            d.n_trig = static_cast<int>(m_pendingTrigOffsets.size());
            d.proj_ready = m_bProjReady;
            d.freeze_left_ms = m_dFreezeLeftSec * 1000.0;
            d.win_samp = m_iWinSamp;
            d.ring_fill = m_iRingFill;
            d.beta_energy = 1.0;
            d.aout_rel = Aout.norm() / (Ain.norm() + 1e-30);
        d.yout_field_rel = std::sqrt(d.e_out) / (std::sqrt(XsssPick.squaredNorm()) + 1e-30);
        d.prot_full_raw_ratio = protFullRawRatio;
        d.prot_raw_mix = protRawMix;
            d.hf_ratio_post = (firstDiffEnergy(XsssPick) + 1e-30) / (firstDiffEnergy(Xpick) + 1e-30);
            d.coeff_lp_hz = (m_bUseCoeffLP ? m_dCoeffLpHz : 0.0);
            *pDbg = d;
        }
        // During warmup we still apply trigger protection blending to avoid attenuating evoked.
        MatrixXd XwarmPick = XsssPick;
        if(!wProt.empty()) {
            for(int t = 0; t < nSamp; ++t) {
                const double w = wProt[static_cast<size_t>(t)];
                if(w <= 0.0) {
                    continue;
                }
                Eigen::VectorXd fb = (m_bProtectFallbackFullFit ? XfullPick.col(t) : XsssPick.col(t));
                if(m_bProtectFallbackFullFit && protRawMix > 0.0) {
                    fb.noalias() += protRawMix * (Xpick.col(t) - fb);
                }
                XwarmPick.col(t).noalias() = w * fb + (1.0 - w) * XwarmPick.col(t);
            }
        }
        MatrixXd outWarm = matData;
        setPickedRows(outWarm, picks, XwarmPick);

        m_pendingTrigOffsets.clear();
        ++m_iChunkCounter;
        return outWarm;
    }

    // Temporal suppression in coefficient space
    MatrixXd AinSupp = Ain;
    if(canApply) {
        const MatrixXd& Ruse = (m_bHasRSmoothed ? m_matRSmoothed : m_matR);
        if(Ruse.size() == m_nIn * m_nIn) {
            AinSupp.noalias() = Ain - (Ruse * Ain);
        }
    }

    MatrixXd XtsssPick = m_matGIn * AinSupp;// internal-only tSSS output


    // Safeguard: avoid over-suppressing internal signal outside protected samples.
    // Enforce ||Xout|| >= minKeep * ||Xsss|| (RMS) on unprotected samples by limiting
    // the temporal contribution: Xmix = Xsss + beta*(Xtsss - Xsss), beta in [0,1].
    double betaEnergy = 1.0;
    const double minKeep = 0.75; // tune: 0.7~0.9. Larger => better SNR preservation.
    bool badTemporalGain = false;

    if(canApply && (minKeep > 0.0 && minKeep < 1.0)) {
        double Eraw = 0.0;
        double Esss = 0.0;
        double Et   = 0.0;
        double dot  = 0.0;

        for(int ti = 0; ti < nSamp; ++ti) {
            const double wp = (!wProt.empty() ? wProt[static_cast<size_t>(ti)] : 0.0);
            if(wp >= 0.5) {
                continue; // hard-protected samples are handled by fallback blending
            }
            Eraw += Xpick.col(ti).squaredNorm();
            Esss += XsssPick.col(ti).squaredNorm();
            Et   += XtsssPick.col(ti).squaredNorm();
            dot  += XsssPick.col(ti).dot(XtsssPick.col(ti));
        }

        const double eps = 1e-30;
        const double rr_raw = std::sqrt((Et + eps) / (Eraw + eps)); // RMS(XtSSS) / RMS(raw)

        // (A) Robust blow-up detection: only treat as unstable if temporal branch explodes vs RAW.
        // Comparing vs Xsss can trigger false positives because Gin is not orthonormal.
        if(!(rr_raw == rr_raw) || !std::isfinite(rr_raw) || rr_raw > m_dBadProjMaxVsRaw) {
            betaEnergy = 0.0;
            badTemporalGain = true;
        }

        // (B) Lower-energy safeguard: avoid over-suppressing when XtSSS is too small vs Xsss.
        if(!badTemporalGain && Esss > 0.0) {
            const double rr_sss = std::sqrt((Et + eps) / (Esss + eps));
            if(rr_sss < minKeep) {
                const double d = std::max(0.0, Et + Esss - 2.0 * dot); // ||D||^2, D = Xt - Xsss
                const double s = dot - Esss;                            // <Xsss, D>

                if(d > 1e-30) {
                    const double k = minKeep * minKeep;                 // energy ratio target
                    const double A = d;
                    const double B = 2.0 * s;
                    const double C = (1.0 - k) * Esss;
                    const double disc = B * B - 4.0 * A * C;

                    if(disc > 0.0) {
                        const double sd = std::sqrt(disc);
                        const double r1 = (-B - sd) / (2.0 * A);
                        const double r2 = (-B + sd) / (2.0 * A);

                        double betaMax = 1.0;
                        const double c1 = (r1 > 0.0 ? r1 : 1e9);
                        const double c2 = (r2 > 0.0 ? r2 : 1e9);
                        betaMax = std::min(c1, c2);

                        betaEnergy = std::min(1.0, std::max(0.0, betaMax));
                    }
                }
            }
        }
    }

    // If we had to clamp the temporal branch due to blow-up, freeze projector updates briefly to avoid
    // feeding the instability back into the online estimator.
    if(badTemporalGain && m_dBadProjFreezeSec > 0.0) {
        m_dFreezeLeftSec = std::max(m_dFreezeLeftSec, m_dBadProjFreezeSec);
        canApply = false;
    }

// Start from tSSS output (outside protected samples), then optionally add back external residual field.
    // The residual is FULL-FIT external field with dominant external-noise subspace removed.
    MatrixXd XoutPick = XtsssPick;

    if(betaEnergy < 0.999) {
        for(int t = 0; t < nSamp; ++t) {
            const double wp = (!wProt.empty() ? wProt[static_cast<size_t>(t)] : 0.0);
            if(wp >= 0.5) {
                continue;
            }
            XoutPick.col(t).noalias() = XsssPick.col(t) + betaEnergy * (XtsssPick.col(t) - XsssPick.col(t));
        }
    }

    // Optionally add back the external residual field (FULL-FIT minus learned external-noise subspace).
    // This helps recover evoked components that leak into Aout, while keeping dominant external noise suppressed.
    if(extReadyNow && m_bExtApplyOutsideProtect && !badTemporalGain) {
        XoutPick.noalias() += YresPick;
    }

    if(!wProt.empty()) {
        for(int t = 0; t < nSamp; ++t) {
            const double w = wProt[static_cast<size_t>(t)];
            if(w <= 0.0) {
                continue;
            }
            // X = w*fallback + (1-w)*tSSS
            Eigen::VectorXd fb = XprotFallbackPick.col(t);
            if(protGainScaleHard < 0.999 && w >= 0.5) {
                fb *= protGainScaleHard;
            }
            if(protRawMix > 0.0) {
                fb.noalias() += protRawMix * (Xpick.col(t) - fb);
            }
            XoutPick.col(t).noalias() = w * fb + (1.0 - w) * XoutPick.col(t);
        }
    }

    // Global output gain clamp (avoid amplifying both signal and noise together).
    // Compute RMS ratio on non-protected samples and scale down if necessary.
    double outGainScale = 1.0;
    if(m_bOutGainClamp && m_dOutGainMax > 0.0) {
        double Eraw = 0.0;
        double Eout = 0.0;
        for(int t = 0; t < nSamp; ++t) {
            const double wp = (!wProt.empty() ? wProt[static_cast<size_t>(t)] : 0.0);
            if(wp >= 0.5) {
                continue;
            }
            Eraw += Xpick.col(t).squaredNorm();
            Eout += XoutPick.col(t).squaredNorm();
        }
        if(Eraw > 1e-30 && Eout > 0.0) {
            const double rr = std::sqrt(Eout / Eraw);
            if(rr > m_dOutGainMax) {
                outGainScale = m_dOutGainMax / std::max(1e-12, rr);
                XoutPick *= outGainScale;
            }
        }
    }

    MatrixXd out = matData;
    setPickedRows(out, picks, XoutPick);

    if(pDbg) {
        AdaptiveTSSSDebugInfo d;
        d.e_total = Xpick.squaredNorm();
        d.e_out   = (m_matGOut * Aout).squaredNorm();
        d.R_out   = (d.e_total > 0.0 ? d.e_out / d.e_total : 0.0);
        d.gate_on = canApply;
        d.k_removed = m_iKRemovedLast;
        d.corr_max  = m_dCorrMaxLast;

        double pfSoft = 0.0;
        double pfHard = 0.0;
        if(!wProt.empty()) {
            for(double v : wProt) {
                pfSoft += (v > 0.0 ? 1.0 : 0.0);
                pfHard += (v >= 0.5 ? 1.0 : 0.0);
            }
            pfSoft /= static_cast<double>(wProt.size());
            pfHard /= static_cast<double>(wProt.size());
        }
        d.protect_frac_soft = pfSoft;
        d.protect_frac_hard = pfHard;
        d.protect_fullfit = m_bProtectFallbackFullFit;
        d.protect_carry_in_samp  = carryInSamp;
        d.protect_carry_out_samp = carryOutSamp;
        d.ext_reg_factor = m_dExtRegFactor;

        // External-noise subspace cancellation diagnostics
        d.ext_noise_ready = m_bExtReady;
        d.ext_noise_k = m_iExtK;
        d.ext_noise_energy_keep = m_dExtEnergyKeep;
        d.ext_res_scale = m_dExtResScaleLast;

        // Residual-feature nuisance regressors (eSSS-style basis augmentation)
        d.feat_ready = m_bFeatReady;
        d.feat_k = m_iFeatK;
        d.feat_energy_keep = m_dFeatEnergyKeep;
        d.feat_reg_factor = m_dFeatRegFactor;

        d.n_trig = static_cast<int>(m_pendingTrigOffsets.size());
        d.proj_ready = m_bProjReady;
        d.freeze_left_ms = m_dFreezeLeftSec * 1000.0;
        d.win_samp = m_iWinSamp;
        d.ring_fill = m_iRingFill;
        d.beta_energy = betaEnergy;
        d.aout_rel = Aout.norm() / (Ain.norm() + 1e-30);
        d.yout_field_rel = std::sqrt(d.e_out) / (std::sqrt(XsssPick.squaredNorm()) + 1e-30);
        d.prot_full_raw_ratio = protFullRawRatio;
        d.prot_raw_mix = protRawMix;

        d.hf_ratio_post = (firstDiffEnergy(XoutPick) + 1e-30) / (firstDiffEnergy(Xpick) + 1e-30);
        d.coeff_lp_hz = (m_bUseCoeffLP ? m_dCoeffLpHz : 0.0);
        *pDbg = d;
    }

    m_pendingTrigOffsets.clear();
    ++m_iChunkCounter;

    return out;
}

//=============================================================================================================
void AdaptiveTSSS::setSamplingFrequency(double fsHz)
{
    if(std::isfinite(fsHz) && fsHz > 1.0) {
        m_dFsHz = fsHz;
        updateLowpassAlpha();
        if(!m_bTauLocked) {
            // window will be rebuilt on next prepare
            m_iKernelVersionLast = std::numeric_limits<int>::min();
        }
    }
}

//=============================================================================================================
void AdaptiveTSSS::setCoefficientLowpassHz(double fcHz)
{
    if(std::isfinite(fcHz) && fcHz > 0.0) {
        m_bUseCoeffLP = true;
        m_dCoeffLpHz = fcHz;
    } else {
        m_bUseCoeffLP = false;
        m_dCoeffLpHz = 0.0;
    }
    updateLowpassAlpha();

    // reset LP states
    m_stateAinLP.resize(0);
    m_stateAoutLP.resize(0);
}

//=============================================================================================================
void AdaptiveTSSS::setExternalRegFactor(double factor)
{
    if(std::isfinite(factor) && factor > 0.1) {
        m_dExtRegFactor = std::min(1e6, std::max(0.1, factor));
    }
}

//=============================================================================================================
void AdaptiveTSSS::setChannelWeights(const Eigen::VectorXd& w)
{
    m_bHasWeights = (w.size() > 0);
    if(m_bHasWeights) {
        m_vecWFull = w;
    } else {
        m_vecWFull.resize(0);
    }
    // force re-prepare
    m_iKernelVersionLast = std::numeric_limits<int>::min();
}

//=============================================================================================================
void AdaptiveTSSS::clearChannelWeights()
{
    m_bHasWeights = false;
    m_vecWFull.resize(0);
    m_vecWPick.resize(0);
    m_iKernelVersionLast = std::numeric_limits<int>::min();
}

//=============================================================================================================
void AdaptiveTSSS::setWarmupChunks(int warmupChunks, bool returnSpatialUntilWarm)
{
    m_iWarmupChunks = std::max(0, warmupChunks);
    m_bReturnSpatialUntilWarm = returnSpatialUntilWarm;
}

//=============================================================================================================
void AdaptiveTSSS::setTemporalMemorySeconds(double tauSeconds, bool lock)
{
    if(std::isfinite(tauSeconds) && tauSeconds > 0.05) {
        m_dTauSec = tauSeconds;
    }
    m_bTauLocked = lock;
    // force re-prepare to resize window
    m_iKernelVersionLast = std::numeric_limits<int>::min();
}

//=============================================================================================================
void AdaptiveTSSS::setTemporalCorrLimitBase(double corrLimit)
{
    if(std::isfinite(corrLimit)) {
        m_dCorrLimit = std::min(0.9999, std::max(0.0, corrLimit));
    }
}

//=============================================================================================================
void AdaptiveTSSS::setProjectorUpdatePeriodChunks(int periodChunks)
{
    m_iUpdatePeriodChunks = std::max(1, periodChunks);
}

//=============================================================================================================
void AdaptiveTSSS::setProjectorSmoothingAlpha(double alpha)
{
    if(!std::isfinite(alpha)) {
        return;
    }
    m_dProjSmoothAlpha = std::min(0.99, std::max(0.0, alpha));
}

//=============================================================================================================
void AdaptiveTSSS::setTemporalGainClamp(double maxGain, double freezeSec)
{
    if(std::isfinite(maxGain)) {
        // Values <= 1 are overly strict; use 1.0 to effectively disable the upper clamp.
        m_dMaxTemporalGain = std::min(100.0, std::max(1.0, maxGain));
    }
    if(std::isfinite(freezeSec)) {
        m_dBadProjFreezeSec = std::min(10.0, std::max(0.0, freezeSec));
    }
}

//=============================================================================================================
void AdaptiveTSSS::setTriggerProtectionMs(double preMs, double postMs, double taperMs)
{
    m_dTrigPreMs  = std::max(0.0, preMs);
    m_dTrigPostMs = std::max(0.0, postMs);
    m_dTrigTaperMs = std::max(0.0, taperMs);
}

//=============================================================================================================
void AdaptiveTSSS::setTriggerFreezeUpdateMs(double ms)
{
    m_dFreezeUpdateMs = std::max(0.0, ms);
}

//=============================================================================================================
void AdaptiveTSSS::setPendingTriggers(const std::vector<int>& sampleOffsets)
{
    m_pendingTrigOffsets = sampleOffsets;
}

//=============================================================================================================
void AdaptiveTSSS::setProtectFallbackFullFit(bool enabled)
{
    m_bProtectFallbackFullFit = enabled;
}

//=============================================================================================================
void AdaptiveTSSS::setProtectRawSafety(double targetRatio, double maxRawMix)
{
    // NOTE: Mixing toward RAW may re-inject high-amplitude stimulation artifacts.
    // Allow disabling by passing targetRatio<=0 or maxRawMix<=0.
    if(!(std::isfinite(targetRatio) && std::isfinite(maxRawMix)) || targetRatio <= 0.0 || maxRawMix <= 0.0) {
        m_bProtRawSafety = false;
        m_dProtTargetRatio = 0.0;
        m_dProtMaxRawMix = 0.0;
        return;
    }
    m_bProtRawSafety = true;
    m_dProtTargetRatio = std::min(1.0, std::max(0.1, targetRatio));
    m_dProtMaxRawMix = std::min(1.0, std::max(0.0, maxRawMix));
}

//=============================================================================================================
void AdaptiveTSSS::setOutputGainClamp(double maxGain)
{
    if(!std::isfinite(maxGain)) {
        return;
    }
    if(maxGain <= 0.0) {
        m_bOutGainClamp = false;
        return;
    }
    m_bOutGainClamp = true;
    m_dOutGainMax = std::min(10.0, std::max(0.5, maxGain));
}

//=============================================================================================================
void AdaptiveTSSS::setAdaptiveProtectionShrink(bool enabled, double residualRatioThresh, double stableMs, double minHoldMs)
{
    m_bAdaptiveProtShrink = enabled;
    if(std::isfinite(residualRatioThresh)) {
        m_dProtResidRatioThresh = std::min(0.5, std::max(0.0, residualRatioThresh));
    }
    if(std::isfinite(stableMs)) {
        m_dProtStableMs = std::min(200.0, std::max(0.0, stableMs));
    }
    if(std::isfinite(minHoldMs)) {
        m_dProtMinHoldMs = std::min(500.0, std::max(0.0, minHoldMs));
    }
}

//=============================================================================================================
void AdaptiveTSSS::setProtectFallbackGainCap(double maxGain)
{
    if(!std::isfinite(maxGain)) {
        return;
    }
    if(maxGain <= 0.0) {
        m_bProtGainCap = false;
        return;
    }
    m_bProtGainCap = true;
    m_dProtGainMax = std::min(10.0, std::max(0.5, maxGain));
}

//=============================================================================================================
void AdaptiveTSSS::setExternalNoiseCancel(bool enabled,
                                         double energyKeep,
                                         int maxDim,
                                         double forget,
                                         int updatePeriodChunks,
                                         bool applyOutsideProtect,
                                         double maxResidualRatio)
{
    m_bExtNoiseCancel = enabled;
    m_dExtEnergyKeep = std::min(0.999, std::max(0.5, energyKeep));
    m_iExtMaxDim = std::max(0, maxDim);
    m_dExtForget = std::min(0.9999, std::max(0.0, forget));
    m_iExtUpdatePeriodChunks = std::max(1, updatePeriodChunks);
    m_bExtApplyOutsideProtect = applyOutsideProtect;
    m_dExtMaxResidualRatio = std::min(1.0, std::max(0.0, maxResidualRatio));

    if(!enabled) {
        m_matCoo.resize(0,0);
        m_matUout.resize(0,0);
        m_iExtK = 0;
        m_bExtReady = false;
        m_dExtResScaleLast = 0.0;
    }
}

//=============================================================================================================
void AdaptiveTSSS::setResidualFeatureCancel(bool enabled,
                                           double energyKeep,
                                           int maxDim,
                                           double forget,
                                           int updatePeriodChunks,
                                           double regFactor)
{
    m_bFeatCancel = enabled;
    m_dFeatEnergyKeep = std::min(0.999, std::max(0.5, energyKeep));
    m_iFeatMaxDim = std::max(0, maxDim);
    m_dFeatForget = std::min(0.9999, std::max(0.0, forget));
    m_iFeatUpdatePeriodChunks = std::max(1, updatePeriodChunks);
    m_dFeatRegFactor = std::min(1e6, std::max(0.1, regFactor));

    if(!enabled) {
        m_matCrr.resize(0,0);
        m_matUfeat.resize(0,0);
        m_iFeatK = 0;
        m_bFeatReady = false;
        // Drop feature regressors from the solver immediately if we are already prepared.
        rebuildSpatialSolver();
    }
}

//=============================================================================================================
bool AdaptiveTSSS::loadBasisIfNeeded(RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel)
{
    bool changed = false;

    if(kernel.matSSSIn.size() == 0 && !kernel.sBasisInCsvPath.isEmpty()) {
        MatrixXd tmp;
        if(loadMatrixCsv(kernel.sBasisInCsvPath, tmp)) {
            kernel.matSSSIn = std::move(tmp);
            changed = true;
        }
    }

    if(kernel.matSSSOut.size() == 0 && !kernel.sBasisOutCsvPath.isEmpty()) {
        MatrixXd tmp;
        if(loadMatrixCsv(kernel.sBasisOutCsvPath, tmp)) {
            kernel.matSSSOut = std::move(tmp);
            changed = true;
        }
    }

    return changed;
}

//=============================================================================================================
void AdaptiveTSSS::rebuildSpatialSolver()
{
    // Requires bases + pick weights to be prepared.
    if(m_matGIn.size() == 0 || m_nPick <= 0 || m_nIn <= 0) {
        m_matPinvW.resize(0,0);
        return;
    }
    if(m_nOut > 0 && m_matGOut.size() == 0) {
        m_matPinvW.resize(0,0);
        return;
    }

    if(m_vecWPick.size() != m_nPick) {
        m_vecWPick = VectorXd::Ones(m_nPick);
    }

    // Decide whether to include residual-feature nuisance regressors.
    int nFeat = 0;
    const bool useFeat = (m_bFeatCancel && m_bFeatReady && m_iFeatK > 0 &&
                          m_matUfeat.rows() == m_nPick && m_matUfeat.cols() >= m_iFeatK);
    if(useFeat) {
        nFeat = m_iFeatK;
    }

    const int nCoeff = m_nIn + m_nOut + nFeat;
    if(nCoeff <= 0 || m_nPick <= 0) {
        m_matPinvW.resize(0,0);
        return;
    }

    // Build basis matrix A = [Gin, Gout, Ufeat]
    MatrixXd A(m_nPick, nCoeff);
    A.leftCols(m_nIn) = m_matGIn;
    if(m_nOut > 0) {
        A.middleCols(m_nIn, m_nOut) = m_matGOut;
    }
    if(nFeat > 0) {
        A.rightCols(nFeat) = m_matUfeat.leftCols(nFeat);
    }

    // Apply per-channel weights to basis
    MatrixXd Aw = A;
    for(int i = 0; i < m_nPick; ++i) {
        Aw.row(i) *= m_vecWPick(i);
    }

    // Column normalization for numerical stability.
    VectorXd colNorm(nCoeff);
    for(int j = 0; j < nCoeff; ++j) {
        const double n = Aw.col(j).norm();
        colNorm(j) = (std::isfinite(n) && n > 1e-20) ? n : 1.0;
    }
    const MatrixXd Dinv = colNorm.cwiseInverse().asDiagonal();
    const MatrixXd Awn = Aw * Dinv;

    MatrixXd AtA = Awn.transpose() * Awn;
    const double diagMean = std::max(1e-30, AtA.diagonal().array().abs().mean());
    const double regBase = std::max(1e-12, m_dRegSSS) * diagMean;
    const double regIn   = regBase;
    const double regOut  = regBase * std::max(0.1, m_dExtRegFactor);
    const double regFeat = regBase * std::max(0.1, m_dFeatRegFactor);

    AtA.diagonal().head(m_nIn).array() += regIn;
    if(m_nOut > 0) {
        AtA.diagonal().segment(m_nIn, m_nOut).array() += regOut;
    }
    if(nFeat > 0) {
        AtA.diagonal().tail(nFeat).array() += regFeat;
    }

    const MatrixXd pinvScaled = AtA.ldlt().solve(Awn.transpose());
    m_matPinvW = Dinv * pinvScaled;
}

//=============================================================================================================
void AdaptiveTSSS::prepareIfNeeded(const Eigen::MatrixXd& matData,
                                  const RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel,
                                  const Eigen::RowVectorXi& vecPicks)
{
    const int nChan = static_cast<int>(matData.rows());

    RowVectorXi picks = vecPicks;
    if(picks.size() == 0) {
        picks.resize(nChan);
        for(int i = 0; i < nChan; ++i) {
            picks(i) = i;
        }
    }

    const bool picksChanged =
        (m_vecPicksLast.size() != picks.size()) ||
        ((m_vecPicksLast.size() == picks.size()) && (m_vecPicksLast != picks));

    const bool kernelChanged = (m_iKernelVersionLast != kernel.iVersion);

    if(!picksChanged && !kernelChanged && m_matPinvW.size() != 0) {
        return;
    }

    // Validate bases
    if(kernel.matSSSIn.size() == 0 || kernel.matSSSOut.size() == 0) {
        m_matPinvW.resize(0,0);
        return;
    }

    m_nPick = static_cast<int>(picks.size());

    // Resolve bases (either already-picked or full channel)
    if(kernel.matSSSIn.rows() == m_nPick && kernel.matSSSOut.rows() == m_nPick) {
        m_matGIn = kernel.matSSSIn;
        m_matGOut = kernel.matSSSOut;
    } else if(kernel.matSSSIn.rows() == nChan && kernel.matSSSOut.rows() == nChan) {
        m_matGIn = pickRows(kernel.matSSSIn, picks);
        m_matGOut = pickRows(kernel.matSSSOut, picks);
    } else {
        m_matPinvW.resize(0,0);
        return;
    }

    m_nIn  = static_cast<int>(m_matGIn.cols());
    m_nOut = static_cast<int>(m_matGOut.cols());

    // Build pick weights
    m_vecWPick = VectorXd::Ones(m_nPick);
    if(m_bHasWeights && m_vecWFull.size() > 0) {
        if(m_vecWFull.size() == m_nPick) {
            m_vecWPick = m_vecWFull;
        } else if(m_vecWFull.size() == nChan) {
            for(int i = 0; i < m_nPick; ++i) {
                const int ch = picks(i);
                if(ch >= 0 && ch < nChan) {
                    m_vecWPick(i) = m_vecWFull(ch);
                }
            }
        }
        // sanitize
        for(int i = 0; i < m_vecWPick.size(); ++i) {
            if(!std::isfinite(m_vecWPick(i)) || m_vecWPick(i) <= 0.0) {
                m_vecWPick(i) = 1.0;
            }
        }
    }

    // Cache kernel ridge for solver rebuilds (e.g., when the feature basis changes).
    m_dRegSSS = std::max(1e-12, kernel.dRegSSS);

    // Residual-feature tracking reset (depends on nPick)
    if(m_bFeatCancel && m_nPick > 0) {
        m_matCrr = MatrixXd::Zero(m_nPick, m_nPick);
    } else {
        m_matCrr.resize(0,0);
    }
    m_matUfeat.resize(0,0);
    m_iFeatK = 0;
    m_bFeatReady = false;

    // Build WLS pseudo-inverse (Gin/Gout plus optional residual-feature nuisance regressors)
    rebuildSpatialSolver();
    if(m_matPinvW.size() == 0) {
        return;
    }

    // Resize ring for new dimensions
    m_iWinSamp = static_cast<int>(std::round(std::max(0.5, m_dTauSec) * m_dFsHz));
    m_iWinSamp = std::max(64, m_iWinSamp);

    m_ringAinLP = MatrixXd::Zero(m_nIn, m_iWinSamp);
    m_ringAoutLP = MatrixXd::Zero(m_nOut, m_iWinSamp);
    m_iRingPos = 0;
    m_iRingFill = 0;

    m_stateAinLP.resize(0);
    m_stateAoutLP.resize(0);

    m_bProjReady = false;
    m_matR = MatrixXd::Zero(m_nIn, m_nIn);
    m_matRSmoothed = MatrixXd::Zero(m_nIn, m_nIn);
    m_bHasRSmoothed = false;

    m_iChunkCounter = 0;
    m_dFreezeLeftSec = 0.0;
    m_iKRemovedLast = 0;
    m_dCorrMaxLast = 0.0;

    // External-noise tracking reset (depends on nOut)
    if(m_bExtNoiseCancel && m_nOut > 0) {
        m_matCoo = MatrixXd::Zero(m_nOut, m_nOut);
    } else {
        m_matCoo.resize(0,0);
    }
    m_matUout.resize(0,0);
    m_iExtK = 0;
    m_bExtReady = false;
    m_dExtResScaleLast = 0.0;

    m_vecPicksLast = picks;
    m_iKernelVersionLast = kernel.iVersion;

    updateLowpassAlpha();
}

//=============================================================================================================
void AdaptiveTSSS::updateLowpassAlpha()
{
    if(m_bUseCoeffLP && m_dCoeffLpHz > 0.0 && m_dFsHz > 1.0) {
        // one-pole per-sample alpha
        m_dCoeffLpAlpha = std::exp(-2.0 * std::acos(-1.0) * m_dCoeffLpHz / m_dFsHz);
    } else {
        m_dCoeffLpAlpha = 0.0;
    }
}

//=============================================================================================================
Eigen::VectorXd AdaptiveTSSS::lowpassStep(const Eigen::VectorXd& x,
                                         Eigen::VectorXd& state) const
{
    if(m_dCoeffLpAlpha <= 0.0) {
        return x;
    }
    if(state.size() != x.size()) {
        state = VectorXd::Zero(x.size());
    }

    const double a = m_dCoeffLpAlpha;
    const double b = 1.0 - a;
    state = a * state + b * x;
    return state;
}

//=============================================================================================================
void AdaptiveTSSS::ringPush(const Eigen::VectorXd& ain_lp,
                           const Eigen::VectorXd& aout_lp)
{
    if(m_iWinSamp <= 0) {
        return;
    }
    if(m_ringAinLP.rows() != ain_lp.size() || m_ringAoutLP.rows() != aout_lp.size()) {
        return;
    }

    m_ringAinLP.col(m_iRingPos) = ain_lp;
    m_ringAoutLP.col(m_iRingPos) = aout_lp;

    m_iRingPos = (m_iRingPos + 1) % m_iWinSamp;
    m_iRingFill = std::min(m_iWinSamp, m_iRingFill + 1);
}

//=============================================================================================================
void AdaptiveTSSS::ringWindow(Eigen::MatrixXd& AinWin,
                             Eigen::MatrixXd& AoutWin) const
{
    if(m_iRingFill < m_iWinSamp || m_iWinSamp <= 0) {
        AinWin.resize(0,0);
        AoutWin.resize(0,0);
        return;
    }

    // chronological order: oldest .. newest
    AinWin.resize(m_nIn, m_iWinSamp);
    AoutWin.resize(m_nOut, m_iWinSamp);

    const int pos = m_iRingPos; // next write => oldest is pos

    const int tail = m_iWinSamp - pos;
    if(pos == 0) {
        AinWin = m_ringAinLP;
        AoutWin = m_ringAoutLP;
        return;
    }

    AinWin.leftCols(tail) = m_ringAinLP.rightCols(tail);
    AinWin.rightCols(pos) = m_ringAinLP.leftCols(pos);

    AoutWin.leftCols(tail) = m_ringAoutLP.rightCols(tail);
    AoutWin.rightCols(pos) = m_ringAoutLP.leftCols(pos);
}

//=============================================================================================================
void AdaptiveTSSS::updateProjectorIfNeeded(double chunkDurSec)
{
    // Decay freeze window
    if(chunkDurSec > 0.0 && m_dFreezeLeftSec > 0.0) {
        m_dFreezeLeftSec = std::max(0.0, m_dFreezeLeftSec - chunkDurSec);
    }

    if(m_iWinSamp <= 0 || m_iRingFill < m_iWinSamp) {
        return;
    }

    if(m_iChunkCounter <= m_iWarmupChunks) {
        return;
    }

    if(m_iUpdatePeriodChunks <= 0) {
        return;
    }

    if((m_iChunkCounter % m_iUpdatePeriodChunks) != 0) {
        return;
    }

    // Freeze: do not update while within trigger-freeze window
    if(m_dFreezeLeftSec > 0.0) {
        return;
    }

    (void)computeProjectorFromWindow();
}

//=============================================================================================================
bool AdaptiveTSSS::computeProjectorFromWindow()
{
    MatrixXd AinWin, AoutWin;
    ringWindow(AinWin, AoutWin);
    if(AinWin.size() == 0 || AoutWin.size() == 0) {
        return false;
    }

    const int L = static_cast<int>(AinWin.cols());
    const double denom = static_cast<double>(std::max(1, L - 1));

    // Demean
    const VectorXd meanIn = AinWin.rowwise().mean();
    const VectorXd meanOut = AoutWin.rowwise().mean();

    AinWin.colwise() -= meanIn;
    AoutWin.colwise() -= meanOut;

    // Covariances
    MatrixXd Cii = (AinWin * AinWin.transpose()) / denom;
    MatrixXd Coo = (AoutWin * AoutWin.transpose()) / denom;
    MatrixXd Cio = (AinWin * AoutWin.transpose()) / denom;

    const double epsI = std::max(1e-30, m_dCovEps * Cii.diagonal().array().abs().mean());
    const double epsO = std::max(1e-30, m_dCovEps * Coo.diagonal().array().abs().mean());

    Cii.diagonal().array() += epsI;
    Coo.diagonal().array() += epsO;

    MatrixXd invSqrtI, sqrtI, invSqrtO, sqrtO;
    if(!invSqrtAndSqrtSPD(Cii, epsI, invSqrtI, sqrtI)) {
        return false;
    }
    if(!invSqrtAndSqrtSPD(Coo, epsO, invSqrtO, sqrtO)) {
        return false;
    }

    const MatrixXd K = invSqrtI * Cio * invSqrtO; // whitened cross-cov

    Eigen::JacobiSVD<MatrixXd> svd(K, Eigen::ComputeThinU | Eigen::ComputeThinV);
    if(svd.info() != Eigen::Success) {
        return false;
    }

    const VectorXd s = svd.singularValues();
    const MatrixXd U = svd.matrixU();

    int k = 0;
    double corrMax = 0.0;
    for(int i = 0; i < s.size(); ++i) {
        corrMax = std::max(corrMax, std::max(0.0, std::min(1.0, static_cast<double>(s(i)))));
        if(s(i) >= m_dCorrLimit) {
            ++k;
        }
    }

    k = std::min(k, std::min(m_iMaxRemove, m_nIn));

    m_iKRemovedLast = k;
    m_dCorrMaxLast = corrMax;

    if(k <= 0) {
        m_bProjReady = false;
        m_matR.setZero();
        return true;
    }

    const MatrixXd Usel = U.leftCols(k);

    // Removal projector: R = sqrt(Cii) * U U^T * invsqrt(Cii)
    MatrixXd R = sqrtI * Usel * Usel.transpose() * invSqrtI;
    R = 0.5 * (R + R.transpose());

    // Numerical safety: ensure projector spectrum stays within [0,1]
    clipSymmetricEigenvalues(R, 0.0, 1.0);

    m_matR = R;
    m_bProjReady = true;

    // Smooth projector across updates to reduce temporal jitter (improves baseline SNR)
    if(!m_bHasRSmoothed || m_matRSmoothed.size() != m_matR.size()) {
        m_matRSmoothed = m_matR;
        m_bHasRSmoothed = true;
    } else {
        m_matRSmoothed = m_dProjSmoothAlpha * m_matRSmoothed + (1.0 - m_dProjSmoothAlpha) * m_matR;
        // Interpolation of projectors is not necessarily a projector; clip to keep it safe.
        clipSymmetricEigenvalues(m_matRSmoothed, 0.0, 1.0);
    }

    return true;
}

//=============================================================================================================
void AdaptiveTSSS::buildProtectionWeights(int nSamp, std::vector<double>& w)
{
    w.assign(static_cast<size_t>(nSamp), 0.0);

    if(m_dFsHz <= 1.0 || nSamp <= 0) {
        m_iProtectCarrySamp = 0;
        return;
    }

    const int preS   = static_cast<int>(std::round((m_dTrigPreMs  / 1000.0) * m_dFsHz));
    const int postS  = static_cast<int>(std::round((m_dTrigPostMs / 1000.0) * m_dFsHz));
    const int taperS = static_cast<int>(std::round((m_dTrigTaperMs / 1000.0) * m_dFsHz));

    // ---- carry-in from previous chunk (handles triggers near end of chunk) ----
    const int carryIn = std::max(0, m_iProtectCarrySamp);
    if(carryIn > 0) {
        const int c = std::min(nSamp, carryIn);
        for(int t = 0; t < c; ++t) {
            w[static_cast<size_t>(t)] = 1.0;
        }
        // optional taper after the carried hard-protected region
        if(taperS > 0 && c < nSamp) {
            const int b = c - 1;
            const int b1 = std::min(nSamp - 1, b + taperS);
            for(int t = b + 1; t <= b1; ++t) {
                const int d = t - b;
                const double val = 0.5 * (1.0 + std::cos(std::acos(-1.0) * static_cast<double>(d) / static_cast<double>(taperS)));
                w[static_cast<size_t>(t)] = std::max(w[static_cast<size_t>(t)], val);
            }
        }
    }

    // Remaining carry if carryIn spans multiple chunks
    int carryRemain = std::max(0, carryIn - nSamp);

    // ---- triggers detected inside this chunk ----
    int carryOut = 0;

    for(int off : m_pendingTrigOffsets) {
        if(off < 0 || off >= nSamp) {
            continue;
        }

        const int a = off - preS;
        const int b = off + postS;

        const int aC = std::max(0, a);
        const int bC = std::min(nSamp - 1, b);

        for(int t = aC; t <= bC; ++t) {
            w[static_cast<size_t>(t)] = 1.0;
        }

        if(taperS > 0) {
            // pre taper
            const int a0 = std::max(0, aC - taperS);
            for(int t = a0; t < aC; ++t) {
                const int d = aC - t;
                const double val = 0.5 * (1.0 + std::cos(std::acos(-1.0) * static_cast<double>(d) / static_cast<double>(taperS)));
                w[static_cast<size_t>(t)] = std::max(w[static_cast<size_t>(t)], val);
            }
            // post taper (within chunk)
            const int b1 = std::min(nSamp - 1, bC + taperS);
            for(int t = bC + 1; t <= b1; ++t) {
                const int d = t - bC;
                const double val = 0.5 * (1.0 + std::cos(std::acos(-1.0) * static_cast<double>(d) / static_cast<double>(taperS)));
                w[static_cast<size_t>(t)] = std::max(w[static_cast<size_t>(t)], val);
            }
        }

        // carry beyond this chunk
        const int bExt = b + taperS;
        if(bExt >= nSamp) {
            carryOut = std::max(carryOut, bExt - (nSamp - 1));
        }
    }

    m_iProtectCarrySamp = std::max(carryRemain, carryOut);
}

//=============================================================================================================
double AdaptiveTSSS::firstDiffEnergy(const Eigen::MatrixXd& X)
{
    if(X.cols() < 2) {
        return 0.0;
    }
    double e = 0.0;
    for(int c = 1; c < X.cols(); ++c) {
        e += (X.col(c) - X.col(c - 1)).squaredNorm();
    }
    return e;
}

} // namespace RTPROCESSINGLIB

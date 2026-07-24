//=============================================================================================================
/**
 * @file     rtica.cpp
 * @brief    RtIca: real-time ICA-like artifact suppression (fast, bounded-time).
 */

#include "rtica.h"

#include <QtCore/QDebug>

#include <Eigen/QR>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace {
inline double clamp01(double v)
{
    return std::min(1.0, std::max(0.0, v));
}
}

using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================

RtIca::RtIca()
: m_iNChannels(0)
, m_iChunkCounter(0)
, m_bIsInitialized(false)
, m_iLastRemoved(-1)
{
}

RtIca::~RtIca() = default;

//=============================================================================================================

void RtIca::init(int iNChannels)
{
    init(iNChannels, Config());
}

void RtIca::init(int iNChannels, const Config& cfg)
{
    m_cfg = cfg;
    m_vecPicks.clear(); // Clear picks -> implies process all channels
    resetState(iNChannels);
    m_bIsInitialized = true;
}

void RtIca::init(const QVector<int>& vecPicks, const Config& cfg)
{
    m_cfg = cfg;
    m_vecPicks = vecPicks;
    // Internal state size is determined by number of picked channels
    resetState(m_vecPicks.size()); 
    m_bIsInitialized = true;
}

//=============================================================================================================

int RtIca::channelCount() const
{
    return m_iNChannels;
}

int RtIca::componentCount() const
{
    return static_cast<int>(m_matW.rows());
}

int RtIca::lastRemovedComponent() const
{
    // Return the first one (largest) for compatibility
    if (!m_vecLastRemoved.isEmpty()) {
        return m_vecLastRemoved.first();
    }
    return -1;
}

QVector<int> RtIca::lastRemovedComponents() const
{
    return m_vecLastRemoved;
}

VectorXd RtIca::lastComponentPowers() const
{
    return m_vecPowers;
}

//=============================================================================================================

void RtIca::resetState(int iNChannels)
{
    m_iNChannels = std::max(0, iNChannels);
    m_iChunkCounter = 0;
    m_vecLastRemoved.clear();

    if(m_iNChannels <= 0) {
        m_vecMean.resize(0);
        m_vecVar.resize(0);
        m_matW.resize(0, 0);
        return;
    }

    // Whitening state
    m_vecMean = VectorXd::Zero(m_iNChannels);
    m_vecVar  = VectorXd::Ones(m_iNChannels);

    // Reduced rank
    int nComp = std::min(std::max(1, m_cfg.iNumComponents), m_iNChannels);

    // Initialize W with small random values, then orthonormalize rows.
    m_matW.resize(nComp, m_iNChannels);
    m_matW.setRandom();
    m_matW *= 0.01;
    orthonormalizeRows();

    // Scratch buffers are allocated on demand based on nS.
    m_matZ.resize(m_iNChannels, 0);
    m_matY.resize(nComp, 0);
    m_matG.resize(nComp, 0);
    m_matGYt.resize(nComp, nComp);
    m_vecPowers = VectorXd::Zero(nComp);
}

//=============================================================================================================

void RtIca::orthonormalizeRows()
{
    // Enforce orthonormal rows for W (nComp x nCh) using thin QR on W^T.
    // Compute Q (nCh x nComp) and set W = Q^T.
    const int nComp = static_cast<int>(m_matW.rows());
    const int nCh   = static_cast<int>(m_matW.cols());

    if(nComp <= 0 || nCh <= 0) {
        return;
    }

    // QR on W^T: (nCh x nComp)
    MatrixXd WT = m_matW.transpose();
    HouseholderQR<MatrixXd> qr(WT);

    // Build thin Q: Q = H * I(:,0:nComp)
    MatrixXd I = MatrixXd::Identity(nCh, nComp);
    MatrixXd Q = qr.householderQ() * I;

    m_matW = Q.transpose();
}

//=============================================================================================================

MatrixXd RtIca::calculate(const MatrixXd& matData)
{
    if(matData.rows() <= 0 || matData.cols() <= 0) {
        return matData;
    }

    // Auto-init for FULL matrix if not initialized (backward compatibility)
    // Note: If using picks, user MUST call the init(picks, cfg) method explicitly.
    if(!m_bIsInitialized) {
        // Assume full matrix processing if not initialized
        init(matData.rows(), m_cfg);
        // If m_bIsInitialized became true, m_vecPicks is empty.
    }

    // Case 1: Process ALL channels (m_vecPicks is empty)
    if(m_vecPicks.isEmpty()) {
        if (matData.rows() != m_iNChannels) {
            // Re-init if dimension changed
            init(matData.rows(), m_cfg);
        }
        return computeStep(matData);
    } 
    
    // Case 2: Process ONLY picked channels
    // ------------------------------------
    // We treat the picked rows as the dataset for ICA
    
    // Validation
    // (internal check if size matches, though computeStep does not strictly check row count matches m_iNChannels,
    //  but logic inside depends on it)
    if (m_iNChannels != m_vecPicks.size()) {
         // Should hopefully not happen if init() was called correctly
         resetState(m_vecPicks.size());
    }

    // Extract submatrix
    int nS = matData.cols();
    MatrixXd matSub(m_iNChannels, nS);
    
    for(int i = 0; i < m_iNChannels; ++i) {
        int r = m_vecPicks[i];
        if(r >= 0 && r < matData.rows()) {
            matSub.row(i) = matData.row(r);
        } else {
            matSub.row(i).setZero();
        }
    }

    // Run ICA on submatrix
    MatrixXd matSubOut = computeStep(matSub);

    // Re-construct full output
    // We start with a copy of original data so unpicked channels pass through untouched
    MatrixXd matOut = matData; 
    
    for(int i = 0; i < m_iNChannels; ++i) {
         int r = m_vecPicks[i];
         if(r >= 0 && r < matOut.rows()) {
             matOut.row(r) = matSubOut.row(i);
         }
    }
    
    return matOut;
}

MatrixXd RtIca::computeStep(const MatrixXd& matData)
{
    // Existing logic moved here ...
    const int nCh = m_iNChannels;
    const int nS  = static_cast<int>(matData.cols());
    const int nComp = static_cast<int>(m_matW.rows());

    // Ensure scratch sizes
    m_matZ.resize(nCh, nS);
    m_matY.resize(nComp, nS);
    m_matG.resize(nComp, nS);
    m_matGYt.resize(nComp, nComp);

    // -----------------------------
    // 1) Online diagonal whitening
    // -----------------------------
    // Block mean
    VectorXd blockMean = matData.rowwise().mean();

    // SPECIAL INIT FOR FIRST CHUNK (or resets) to capture scale immediately
    // Otherwise EMA takes too long to decay from 1.0 to 1e-24 (MEG scale), 
    // causing underflow in ICA update during warmup.
    if (m_iChunkCounter == 0) {
        m_vecMean = blockMean;
        // Center temporary to get variance
        MatrixXd tempZ = matData.colwise() - m_vecMean;
        m_vecVar = tempZ.array().square().rowwise().mean();
        // Safety bound
        for(int i=0; i<m_vecVar.size(); ++i) { 
             if(m_vecVar(i) < m_cfg.dEps) m_vecVar(i) = m_cfg.dEps; 
        }
    } else {
        // Update mean EMA
        const double aMu  = clamp01(m_cfg.dMeanEmaAlpha);
        m_vecMean.noalias() = (1.0 - aMu) * m_vecMean + aMu * blockMean;
    }

    // Center data using updated mean
    m_matZ = matData.colwise() - m_vecMean;

    if (m_iChunkCounter > 0) {
        // Block variance (per channel)
        VectorXd blockVar = m_matZ.array().square().rowwise().mean();

        // Update variance EMA
        const double aVar = clamp01(m_cfg.dVarEmaAlpha);
        m_vecVar.noalias() = (1.0 - aVar) * m_vecVar + aVar * blockVar;
    }

    // stdInv
    const double eps = std::max(0.0, m_cfg.dEps);
    VectorXd stdInv = (m_vecVar.array() + eps).rsqrt().matrix();

    // Apply diagonal whitening: Z = (X - mean) .* stdInv
    m_matZ.array().colwise() *= stdInv.array();

    // DEBUG: Check whitening health
    /*
    if (m_iChunkCounter % 50 == 0) {
        qDebug() << "[RtIca] WhiteZ Max:" << m_matZ.cwiseAbs().maxCoeff() 
                 << " MeanVar:" << m_vecVar.mean();
    }
    */

    // -----------------------------
    // 2) Source estimation
    // -----------------------------
    m_matY.noalias() = m_matW * m_matZ; // (nComp x nS)

    // Component power (mean square)
    m_vecPowers = m_matY.rowwise().squaredNorm() / std::max(1, nS);

    // -----------------------------
    // 3) Online ICA update (natural gradient / infomax)
    // -----------------------------
    // G = tanh(scale * Y)
    const double s = m_cfg.dNonlinScale;
    m_matG = (s * m_matY.array()).tanh().matrix();

    // GYt = (G * Y^T) / nS  (nComp x nComp)
    m_matGYt.noalias() = (m_matG * m_matY.transpose()) / std::max(1, nS);

    // dW = (I - GYt) * W
    MatrixXd I = MatrixXd::Identity(nComp, nComp);
    MatrixXd dW = (I - m_matGYt) * m_matW;

    const double eta = m_cfg.dLearningRate;
    if(std::isfinite(eta) && eta > 0.0) {
        m_matW.noalias() += eta * dW;
    }

    // Normalize rows to avoid blow-up
    for(int r = 0; r < nComp; ++r) {
        const double nr = m_matW.row(r).norm();
        if(std::isfinite(nr) && nr > eps) {
            m_matW.row(r) /= nr;
        }
    }

    // Periodic orthonormalization
    ++m_iChunkCounter;
    if(m_cfg.iOrthoEveryNChunks > 0 && (m_iChunkCounter % m_cfg.iOrthoEveryNChunks) == 0) {
        orthonormalizeRows();
        // Recompute Y with orthonormalized W for consistent removal
        m_matY.noalias() = m_matW * m_matZ;
        m_vecPowers = m_matY.rowwise().squaredNorm() / std::max(1, nS);
    }

    // -----------------------------
    // 4) Remove largest component(s)
    // -----------------------------
    m_vecLastRemoved.clear();
    
    if(m_cfg.bRemoveLargest && m_iChunkCounter > m_cfg.iWarmupChunks) {
        const int kRemove = std::min(nComp, std::max(1, m_cfg.iNumRemove));

        // Sort indices by power (descending)
        std::vector<int> idxs(nComp);
        std::iota(idxs.begin(), idxs.end(), 0);
        
        std::sort(idxs.begin(), idxs.end(), [&](int a, int b) {
            return m_vecPowers(a) > m_vecPowers(b);
        });
        
        // Remove the top kRemove components
        for(int k = 0; k < kRemove; ++k) {
            int idx = idxs[k];
            
            // Remove contribution: Z -= w_idx^T * y_idx
            // Note: If W is orthogonal, this perfectly removes the component.
            // If not perfectly orthogonal, it might leave residues or affect others,
            // but for real-time artifact removal this is the standard subspace subtraction.
            m_matZ.noalias() -= m_matW.row(idx).transpose() * m_matY.row(idx);
            
            m_vecLastRemoved.push_back(idx);
        }
    }

    // -----------------------------
    // 5) Unwhiten (diagonal) and add mean
    // -----------------------------
    VectorXd std = (m_vecVar.array() + eps).sqrt().matrix();
    MatrixXd matOut = m_matZ;
    matOut.array().colwise() *= std.array();
    matOut.colwise() += m_vecMean;

    // DEBUG INFO (Updated)
    if(m_iChunkCounter % 10 == 0) {
        double inMax = matData.cwiseAbs().maxCoeff();
        double outMax = matOut.cwiseAbs().maxCoeff();
        bool hasNaN = !matOut.allFinite();
        
        QDebug dbg = qDebug();
        // Use scientific notation for small MEG values to be readable
        dbg.nospace() << "[RtIca] Chunk:" << m_iChunkCounter 
                      << " InMax:" << inMax // QString::number(inMax, 'g', 3)
                      << " OutMax:" << outMax; //QString::number(outMax, 'g', 3);

        if (hasNaN) dbg << " [NaN]";
            
        // Whitening stats
        double meanVar = m_vecVar.mean();
        double minVar  = m_vecVar.minCoeff();
        // dbg << " VarMean:" << meanVar;
        
        // Power Ratio (Out/In) - Should be < 1.0 for denoising
        double inPower = matData.rowwise().squaredNorm().sum();
        // Avoid recomputing full outPower if not needed, but for debug it is good
        double outPower = matOut.rowwise().squaredNorm().sum();
        double ratio = (inPower > 1e-40) ? (outPower / inPower) : 0.0;
        
        dbg << " Ratio:" << ratio; // QString::number(ratio, 'f', 4);

        // Artifact Removal Stats
        if(!m_vecLastRemoved.isEmpty()) {
            double totalRemPwr = 0.0;
            QString sRem = " Rm:[";
            for(int i=0; i<m_vecLastRemoved.size(); ++i) {
                int idx = m_vecLastRemoved[i];
                double p = m_vecPowers(idx);
                totalRemPwr += p;
                if(i < 8) sRem += QString("%1").arg(idx) + " "; 
                else if(i == 8) sRem += "... ";
            }
            sRem += "]";
            
            double totalPwr = m_vecPowers.sum();
            double pct = (totalPwr > 1e-40) ? (totalRemPwr/totalPwr)*100.0 : 0.0;
            
            dbg << sRem << " Pct:" << QString::number(pct, 'f', 1) << "%";
        }
    }

    return matOut;
}


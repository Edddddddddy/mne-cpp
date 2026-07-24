//=============================================================================================================
/**
 * @file     rtica.cpp
 * @author   Lcy
 * @since    0.1.0
 * @date     January, 2026
 *
 * @brief    RtIca class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtica.h"

#include <cmath>
#include <numeric>

#include <Eigen/Eigenvalues>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
// Helpers
//=============================================================================================================

static inline double clamp01(double v)
{
    if(v < 0.0) {
        return 0.0;
    }
    if(v > 1.0) {
        return 1.0;
    }
    return v;
}

static inline double clampOpen01(double v, double eps = 1e-9)
{
    if(v < eps) {
        return eps;
    }
    if(v > 1.0 - eps) {
        return 1.0 - eps;
    }
    return v;
}

//=============================================================================================================
// RtIca
//=============================================================================================================

RtIca::RtIca()
: m_cfg()
, m_iNChannels(0)
, m_bIsInitialized(false)
, m_lCounter(0)
, m_iBlockCounter(0)
{
    normalizeConfig();
}

//=============================================================================================================

RtIca::RtIca(const Config& cfg)
: m_cfg(cfg)
, m_iNChannels(0)
, m_bIsInitialized(false)
, m_lCounter(0)
, m_iBlockCounter(0)
{
    normalizeConfig();
}

//=============================================================================================================

RtIca::~RtIca()
{
}

//=============================================================================================================

void RtIca::normalizeConfig()
{
    // Block sizes
    if(m_cfg.iBlockSizeIca <= 0) {
        m_cfg.iBlockSizeIca = 0;
    }
    if(m_cfg.iBlockSizeWhite <= 0) {
        m_cfg.iBlockSizeWhite = 0;
    }

    // Safety clamps
    if(m_cfg.iOrthoEveryNBlocks < 1) {
        m_cfg.iOrthoEveryNBlocks = 1;
    }
    if(m_cfg.iNumRemove < 0) {
        m_cfg.iNumRemove = 0;
    }
    if(m_cfg.iWarmupBlocks < 0) {
        m_cfg.iWarmupBlocks = 0;
    }
    m_cfg.dPowerEmaAlpha = clamp01(m_cfg.dPowerEmaAlpha);

    if(m_cfg.dGamma < 0.0) {
        m_cfg.dGamma = 0.0;
    }
    if(m_cfg.dLambda0 < 0.0) {
        m_cfg.dLambda0 = 0.0;
    }
}

//=============================================================================================================

void RtIca::setConfig(const Config& cfg)
{
    m_cfg = cfg;
    normalizeConfig();
    updateKurtosisSign();
}

//=============================================================================================================

const RtIca::Config& RtIca::config() const
{
    return m_cfg;
}

//=============================================================================================================

void RtIca::setGamma(double dGamma)
{
    m_cfg.dGamma = dGamma;
    normalizeConfig();
}

void RtIca::setLambda0(double dLambda0)
{
    m_cfg.dLambda0 = dLambda0;
    normalizeConfig();
}

void RtIca::setRemoveLargestComponent(bool bEnable)
{
    m_cfg.bRemoveLargest = bEnable;
}

void RtIca::setNumRemove(int iNumRemove)
{
    m_cfg.iNumRemove = iNumRemove;
    normalizeConfig();
}

int RtIca::lastRemovedComponent() const
{
    return m_vecLastRemoved.empty() ? -1 : m_vecLastRemoved.front();
}

//=============================================================================================================

int RtIca::channelCount() const
{
    return m_iNChannels;
}

long RtIca::sampleCounter() const
{
    return m_lCounter;
}

//=============================================================================================================

const MatrixXd& RtIca::weights() const
{
    return m_matWeights;
}

const MatrixXd& RtIca::sphere() const
{
    return m_matSphere;
}

MatrixXd RtIca::unmixingMatrix() const
{
    return m_matWeights * m_matSphere;
}

//=============================================================================================================

const VectorXd& RtIca::componentPowerEma() const
{
    return m_vecPowerEma;
}

const std::vector<int>& RtIca::lastRemovedComponents() const
{
    return m_vecLastRemoved;
}

//=============================================================================================================

void RtIca::ensureInitialized(int nCh)
{
    if(!m_bIsInitialized || m_iNChannels != nCh) {
        init(nCh, m_cfg);
    }
}

//=============================================================================================================

void RtIca::init(int iNChannels, const Config& cfg)
{
    m_cfg = cfg;
    normalizeConfig();

    m_iNChannels = iNChannels;
    m_bIsInitialized = true;

    m_matWeights = MatrixXd::Identity(m_iNChannels, m_iNChannels);
    m_matSphere  = MatrixXd::Identity(m_iNChannels, m_iNChannels);

    m_lCounter = 0;
    m_iBlockCounter = 0;

    m_vecPowerEma = VectorXd::Zero(m_iNChannels);
    m_vecLastRemoved.clear();

    m_vecKurtSign.resize(m_iNChannels);
    updateKurtosisSign();

    // Pre-allocate temporaries (resized again as needed)
    m_matCov.resize(m_iNChannels, m_iNChannels);
    m_matY.resize(m_iNChannels, 0);
    m_matF.resize(m_iNChannels, 0);
    m_vecLambdaK.resize(0);
    m_vecDotFy.resize(0);
    m_vecQ.resize(0);
    m_vecScales.resize(0);
    m_matYScaled.resize(m_iNChannels, 0);
}

//=============================================================================================================

void RtIca::init(int iNChannels)
{
    init(iNChannels, m_cfg);
}

//=============================================================================================================

void RtIca::reset()
{
    if(!m_bIsInitialized) {
        return;
    }
    init(m_iNChannels, m_cfg);
}

//=============================================================================================================

double RtIca::lambdaConst() const
{
    if(!std::isfinite(m_cfg.dTauConstSamples) || m_cfg.dTauConstSamples <= 0.0) {
        return 0.0;
    }
    return 1.0 - std::exp(-1.0 / m_cfg.dTauConstSamples);
}

//=============================================================================================================

void RtIca::updateKurtosisSign()
{
    if(m_iNChannels <= 0) {
        return;
    }

    m_vecKurtSign.setConstant(true);
    const int nSub = std::max(0, std::min(m_cfg.iNumSubgaussian, m_iNChannels));
    for(int i = 0; i < nSub; ++i) {
        m_vecKurtSign(i) = false;
    }
}

//=============================================================================================================

void RtIca::updateWhiteningBlock(const MatrixXd& matDataRaw)
{
    if(!m_cfg.bOnlineWhitening) {
        return;
    }

    const int nPts = static_cast<int>(matDataRaw.cols());
    if(nPts <= 0) {
        return;
    }

    // Matlab reference: lambda(t) = lambda0 / t^gamma ; lambda_avg = 1 - lambda(median)
    const double tMid = static_cast<double>(m_lCounter) + 0.5 * static_cast<double>(nPts);
    const double tSafe = std::max(1.0, tMid);

    double lambda = (tSafe > 0.0) ? (m_cfg.dLambda0 / std::pow(tSafe, m_cfg.dGamma)) : m_cfg.dLambda0;
    const double lConst = lambdaConst();
    if(lambda < lConst) {
        lambda = lConst;
    }

    double lambdaAvg = lambda;
    if(m_cfg.bMatlabWhiteningLambdaOneMinus) {
        lambdaAvg = 1.0 - lambda;
    }
    lambdaAvg = clampOpen01(lambdaAvg);

    // v = sphere * X
    MatrixXd v = m_matSphere * matDataRaw;

    const double vEnergy = v.squaredNorm();
    const double QWhite = (lambdaAvg / (1.0 - lambdaAvg)) + (vEnergy / static_cast<double>(nPts));

    // sphere = 1/lambdaAvg * (sphere - v*v'/(nPts*QWhite) * sphere)
    // Rearranged: sphere - factor * v * (v' * sphere)
    const double factor = 1.0 / (static_cast<double>(nPts) * QWhite);
    m_matSphere = (1.0 / lambdaAvg) * (m_matSphere - factor * v * (v.transpose() * m_matSphere));
}

//=============================================================================================================

void RtIca::updateOricaBlock(const MatrixXd& matDataWhitened)
{
    const int nPts = static_cast<int>(matDataWhitened.cols());
    if(nPts <= 0) {
        return;
    }

    // Compute source activation using current weights (pre-update)
    m_matY.noalias() = m_matWeights * matDataWhitened;

    // Choose nonlinearities (super- vs. sub-gaussian)
    m_matF.resizeLike(m_matY);
    m_matF.setZero();

    // f(super)  = -2*tanh(y)
    // f(sub)    =  2*tanh(y)
    // (matches orica.m dynamicOrica)
    const ArrayXXd Yarr = m_matY.array();
    ArrayXXd Farr = (-2.0 * Yarr.tanh());
    for(int r = 0; r < m_iNChannels; ++r) {
        if(!m_vecKurtSign(r)) {
            Farr.row(r) *= -1.0;
        }
    }
    m_matF = Farr.matrix();

    // Forgetting / learning schedule lambda_k (vector)
    m_vecLambdaK.resize(nPts);
    const double lConst = lambdaConst();
    for(int i = 0; i < nPts; ++i) {
        const double t = std::max(1.0, static_cast<double>(m_lCounter) + 1.0 + static_cast<double>(i));
        double lk = (t > 0.0) ? (m_cfg.dLambda0 / std::pow(t, m_cfg.dGamma)) : m_cfg.dLambda0;
        if(lk < lConst) {
            lk = lConst;
        }
        m_vecLambdaK(i) = lk;
    }

    // Update sample counter (same timing as Matlab: after lambda_k computed)
    m_lCounter += nPts;

    // ORICA block update
    // lambda_prod = prod(1./(1-lambda_k))
    double lambdaProd = 1.0;
    for(int i = 0; i < nPts; ++i) {
        lambdaProd *= (1.0 / (1.0 - clampOpen01(m_vecLambdaK(i))));
    }

    // dot(f,y,1) -> per-sample inner product across channels
    m_vecDotFy.resize(nPts);
    m_vecDotFy.setZero();
    m_vecDotFy.noalias() = (m_matF.array() * m_matY.array()).colwise().sum().matrix();

    // Q = 1 + lambda_k .* (dot(f,y,1)-1)
    m_vecQ.resize(nPts);
    m_vecQ = (RowVectorXd::Ones(nPts).array()
            + m_vecLambdaK.array() * (m_vecDotFy.array() - 1.0)).matrix();
    // scales = lambda_k ./ Q
    m_vecScales.resize(nPts);
    m_vecScales = (m_vecLambdaK.array() / m_vecQ.array()).matrix();

    // W = lambdaProd * (W - Y * diag(scales) * F' * W)
    // Avoid explicit diag: YScaled = Y .* scales (colwise)
    m_matYScaled.resizeLike(m_matY);
    m_matYScaled = m_matY.array().rowwise() * m_vecScales.array();

    // temp = F' * W
    const MatrixXd FW = m_matF.transpose() * m_matWeights;
    m_matWeights = lambdaProd * (m_matWeights - m_matYScaled * FW);

    // Orthogonalize periodically (symmetric decorrelation)
    ++m_iBlockCounter;
    if(m_cfg.iOrthoEveryNBlocks > 0 && (m_iBlockCounter % m_cfg.iOrthoEveryNBlocks) == 0) {
        symmetricDecorrelate();
    }
}

//=============================================================================================================

void RtIca::symmetricDecorrelate()
{
    // cov = W*W'  (self-adjoint rank update)
    m_matCov.setZero(m_iNChannels, m_iNChannels);
    m_matCov.selfadjointView<Lower>().rankUpdate(m_matWeights);

    SelfAdjointEigenSolver<MatrixXd> es(m_matCov);
    const MatrixXd V = es.eigenvectors();
    VectorXd D = es.eigenvalues();

    // Numerical safeguard
    D = D.array().max(1e-12);

    // invSqrt = V * diag(1/sqrt(D)) * V'
    const VectorXd invSqrtD = D.array().inverse().sqrt();
    const MatrixXd invSqrtMat = V * invSqrtD.asDiagonal() * V.transpose();
    m_matWeights = invSqrtMat * m_matWeights;
}

//=============================================================================================================

void RtIca::updatePowerEstimate(const MatrixXd& sources)
{
    if(m_cfg.dPowerEmaAlpha <= 0.0) {
        return;
    }
    if(sources.cols() <= 0) {
        return;
    }

    const VectorXd blockPow = sources.rowwise().squaredNorm() / static_cast<double>(sources.cols());
    if(m_vecPowerEma.size() != blockPow.size()) {
        m_vecPowerEma = blockPow;
        return;
    }

    const double a = m_cfg.dPowerEmaAlpha;
    m_vecPowerEma = (1.0 - a) * m_vecPowerEma + a * blockPow;
}

//=============================================================================================================

void RtIca::removeLargestComponents(MatrixXd& sources)
{
    m_vecLastRemoved.clear();
    const int k = std::max(0, std::min(m_cfg.iNumRemove, m_iNChannels));
    if(k <= 0 || sources.cols() <= 0) {
        return;
    }

    // Rank components by running power estimate. If EMA is disabled, fall back to current block power.
    VectorXd score;
    if(m_cfg.dPowerEmaAlpha > 0.0 && m_vecPowerEma.size() == m_iNChannels) {
        score = m_vecPowerEma;
    } else {
        score = sources.rowwise().squaredNorm() / static_cast<double>(sources.cols());
    }

    std::vector<int> idx(m_iNChannels);
    std::iota(idx.begin(), idx.end(), 0);
    std::partial_sort(idx.begin(), idx.begin() + k, idx.end(),
                      [&score](int a, int b) {
                          return score(a) > score(b);
                      });

    for(int i = 0; i < k; ++i) {
        const int comp = idx[i];
        sources.row(comp).setZero();
        m_vecLastRemoved.push_back(comp);
    }
}

//=============================================================================================================

MatrixXd RtIca::calculate(const MatrixXd& matData)
{
    if(matData.rows() <= 0 || matData.cols() <= 0) {
        return matData;
    }

    ensureInitialized(static_cast<int>(matData.rows()));

    const int nPtsTotal = static_cast<int>(matData.cols());
    MatrixXd matOut(m_iNChannels, nPtsTotal);

    const int L = (m_cfg.iBlockSizeIca > 0) ? m_cfg.iBlockSizeIca : nPtsTotal;
    const int Lw = (m_cfg.iBlockSizeWhite > 0)
            ? m_cfg.iBlockSizeWhite
            : ((m_cfg.iBlockSizeIca > 0) ? m_cfg.iBlockSizeIca : nPtsTotal);

    for(int start = 0; start < nPtsTotal; start += L) {
        const int len = std::min(L, nPtsTotal - start);
        const MatrixXd blockRaw = matData.middleCols(start, len);

        // Online whitening update (optionally in smaller sub-blocks)
        if(m_cfg.bOnlineWhitening && Lw > 0 && Lw < len) {
            for(int s = 0; s < len; s += Lw) {
                const int l2 = std::min(Lw, len - s);
                updateWhiteningBlock(blockRaw.middleCols(s, l2));
            }
        } else {
            updateWhiteningBlock(blockRaw);
        }

        // Whiten current block with the *updated* sphere
        const MatrixXd blockZ = m_matSphere * blockRaw;

        // Update weights using ORICA block update
        updateOricaBlock(blockZ);

        // Sources using updated weights
        const MatrixXd blockY = m_matWeights * blockZ;

        // Update running power estimate
        updatePowerEstimate(blockY);

        // Remove largest components (after warmup)
        MatrixXd blockYClean = blockY;
        if(m_cfg.bRemoveLargest && (m_iBlockCounter > m_cfg.iWarmupBlocks)) {
            removeLargestComponents(blockYClean);
        } else {
            m_vecLastRemoved.clear();
        }

        // Reconstruct
        const MatrixXd blockZClean = m_matWeights.transpose() * blockYClean;

        if(m_cfg.bReturnWhitened) {
            matOut.middleCols(start, len) = blockZClean;
        } else {
            // Dewhiten: solve Sphere * X = ZClean
            matOut.middleCols(start, len) = m_matSphere.colPivHouseholderQr().solve(blockZClean);
        }
    }

    return matOut;
}

//=============================================================================================================

MatrixXd RtIca::calculateSources(const MatrixXd& matData)
{
    if(matData.rows() <= 0 || matData.cols() <= 0) {
        return matData;
    }

    ensureInitialized(static_cast<int>(matData.rows()));

    const int nPtsTotal = static_cast<int>(matData.cols());
    MatrixXd matSources(m_iNChannels, nPtsTotal);

    const int L = (m_cfg.iBlockSizeIca > 0) ? m_cfg.iBlockSizeIca : nPtsTotal;
    const int Lw = (m_cfg.iBlockSizeWhite > 0)
            ? m_cfg.iBlockSizeWhite
            : ((m_cfg.iBlockSizeIca > 0) ? m_cfg.iBlockSizeIca : nPtsTotal);

    for(int start = 0; start < nPtsTotal; start += L) {
        const int len = std::min(L, nPtsTotal - start);
        const MatrixXd blockRaw = matData.middleCols(start, len);

        if(m_cfg.bOnlineWhitening && Lw > 0 && Lw < len) {
            for(int s = 0; s < len; s += Lw) {
                const int l2 = std::min(Lw, len - s);
                updateWhiteningBlock(blockRaw.middleCols(s, l2));
            }
        } else {
            updateWhiteningBlock(blockRaw);
        }

        const MatrixXd blockZ = m_matSphere * blockRaw;
        updateOricaBlock(blockZ);

        const MatrixXd blockY = m_matWeights * blockZ;
        updatePowerEstimate(blockY);
        m_vecLastRemoved.clear();

        matSources.middleCols(start, len) = blockY;
    }

    return matSources;
}

//=============================================================================================================
/**
 * @file     rtica.h
 * @brief    RtIca: real-time ICA-like artifact suppression (fast, bounded-time).
 *
 * This implementation is intended for real-time pipelines (MNESCAN) where the
 * original ORICA-style NxN update + NxN eigen-decomposition per chunk is too
 * expensive and can stall the processing thread.
 *
 * Key design choices:
 *  - Diagonal (per-channel) online whitening (EMA mean/variance) for speed.
 *  - Reduced-rank online ICA update (natural-gradient / infomax style) on
 *    M components (M << Nch), with periodic orthonormalization using thin QR.
 *  - Remove the highest-power component (after warmup) and reconstruct the
 *    cleaned sensor data.
 */

#ifndef RTICA_H
#define RTICA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

namespace RTPROCESSINGLIB
{

class RTPROCESINGSHARED_EXPORT RtIca
{
public:
    typedef QSharedPointer<RtIca> SPtr;
    typedef QSharedPointer<const RtIca> ConstSPtr;

    struct Config
    {
        // Reduced rank (number of ICA components to track). For MEG 300-400ch,
        // 32/64/96 are typical real-time values.
        int    iNumComponents      = 64;

        // Learning rate for W update (natural gradient).
        double dLearningRate       = 0.05;

        // Nonlinearity scaling for tanh (y_scaled = dNonlinScale * y).
        double dNonlinScale        = 1.0;

        // Per-channel mean / variance EMA factors.
        double dMeanEmaAlpha       = 0.02;
        double dVarEmaAlpha        = 0.02;

        // Numerical stabilization.
        double dEps                = 1e-8;

        // Orthonormalize rows of W every N chunks (thin QR). 10-50 recommended.
        int    iOrthoEveryNChunks  = 20;

        // Warmup chunks before removing components.
        int    iWarmupChunks       = 30;

        // Artifact removal settings.
        bool   bRemoveLargest      = true;
        int    iNumRemove          = 1;   // remove top-K by power among tracked components
    };

    RtIca();
    ~RtIca();

    void init(int iNChannels);
    void init(int iNChannels, const Config& cfg);
    void init(const QVector<int>& vecPicks, const Config& cfg);

    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData);

    int channelCount() const;
    int componentCount() const;

    int lastRemovedComponent() const;
    QVector<int> lastRemovedComponents() const;
    Eigen::VectorXd lastComponentPowers() const;

private:
    void resetState(int iNChannels);
    void orthonormalizeRows();
    Eigen::MatrixXd computeStep(const Eigen::MatrixXd& matData);

private:
    int m_iNChannels;
    int m_iChunkCounter;
    bool m_bIsInitialized;
    
    QVector<int> m_vecPicks; // Indices of channels to process (if empty, process all)
    mutable QVector<int> m_vecLastRemoved; // All removed indices in current step

    Config m_cfg;

    // Online per-channel whitening state
    Eigen::VectorXd m_vecMean;     // (nCh)
    Eigen::VectorXd m_vecVar;      // (nCh)

    // Unmixing matrix for reduced-rank ICA (nComp x nCh)
    Eigen::MatrixXd m_matW;

    // Scratch buffers
    Eigen::MatrixXd m_matZ;        // (nCh x nS)
    Eigen::MatrixXd m_matY;        // (nComp x nS)
    Eigen::MatrixXd m_matG;        // (nComp x nS)
    Eigen::MatrixXd m_matGYt;      // (nComp x nComp)
    Eigen::VectorXd m_vecPowers;   // (nComp)

    int m_iLastRemoved;
};

} // namespace RTPROCESSINGLIB

#endif // RTICA_H

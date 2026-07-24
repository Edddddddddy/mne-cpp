//=============================================================================================================
/**
 * @file     adaptivetssskernel.h
 * @author   Generated for real-time adaptive tSSS-style processing (streaming).
 * @since    0.1.0
 * @date     Jan, 2026
 *
 * @brief    Adaptive tSSS kernel configuration and precomputed bases.
 *
 * Notes
 * -----
 * This kernel expects precomputed SSS bases (internal and external) for the MEG channel set you process.
 * In practice, these bases are computed offline from sensor geometry (coil definitions + device->head transform).
 * The real-time processor uses them to:
 *   1) Estimate internal/external multipole coefficients by ridge regression
 *   2) Suppress temporally correlated interference by a sliding-window CCA/SVD step (online tSSS-style)
 *
 */

#ifndef ADAPTIVETSSSKERNEL_RTPROCESSING_H
#define ADAPTIVETSSSKERNEL_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <QString>

namespace RTPROCESSINGLIB
{

//=========================================================================================================
/**
 * @brief Kernel/config for AdaptiveTSSS.
 *
 * Required:
 *  - matSSSIn  : (nChanTotal x nIn)  internal SSS basis
 *  - matSSSOut : (nChanTotal x nOut) external SSS basis
 *
 * Typical defaults for real-time:
 *  - iStDurationSamp: 1024 (≈1.0 s @ 1024 Hz)
 *  - dStCorrLimit: 0.98
 *  - iUpdatePeriodChunks: 5
 */
struct AdaptiveTSSSKernel
{
    // -------------------------
    // Precomputed SSS bases
    // -------------------------
    Eigen::MatrixXd matSSSIn;      /**< Internal SSS basis (G_in). */
    Eigen::MatrixXd matSSSOut;     /**< External SSS basis (G_out). */

    // -------------------------
    // SSS coefficient estimation
    // -------------------------
    double dRegSSS = 1e-4;         /**< Ridge regularization for [G_in G_out] inversion. */

    // -------------------------
    // Temporal suppression (tSSS-style)
    // -------------------------
    int    iStDurationSamp = 1024; /**< Sliding window length in samples for temporal correlation estimation. */
    double dStCorrLimit = 0.98;    /**< Canonical correlation threshold; components above are removed. */
    double dCovEps = 1e-10;        /**< Diagonal loading for covariance matrices. */
    int    iMaxRemoveComp = -1;    /**< Max #components to remove; -1 = no cap. */
    int    iUpdatePeriodChunks = 5;/**< Recompute temporal projector every N chunks (stability + CPU). */

    // -------------------------
    // Runtime behavior
    // -------------------------
    bool   bApplySpatialSSS = true;   /**< If true, always reconstruct using internal basis (SSS). */
    bool   bApplyTemporal = true;     /**< If true, run the temporal (tSSS-style) suppression once warm. */
    bool   bReturnRawUntilWarm = true;/**< If true, output raw until the temporal window is filled. */

    // -------------------------
    // Change tracking (optional)
    // -------------------------
    int    iVersion = 0;              /**< Increment when any matrix/parameter changes to force re-prepare(). */

    // -------------------------
    // Convenience (optional) paths for offline-exported bases
    // -------------------------
    QString sBasisInCsvPath;          /**< Optional CSV path for matSSSIn. */
    QString sBasisOutCsvPath;         /**< Optional CSV path for matSSSOut. */
};

} // namespace RTPROCESSINGLIB

#endif // ADAPTIVETSSSKERNEL_RTPROCESSING_H

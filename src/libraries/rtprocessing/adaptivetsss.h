//=============================================================================================================
/**
 * @file     adaptivetsss.h
 * @brief    Real-time adaptive tSSS-style processor (SSS + trigger-aware temporal suppression).
 *
 * Processing (per chunk):
 *   1) Spatial SSS (always enabled on MEG picks):
 *        c = argmin ||W( [G_in G_out] c - x )||^2 + reg ||c||^2
 *      where W is optional per-channel weights (e.g., mag/grad balancing) and reg is ridge.
 *   2) Temporal suppression (tSSS-like): estimate internal/external canonical correlations from a
 *      sliding coefficient window (duration tau), then remove highly correlated internal modes.
 *
 * Trigger-aware SNR protection (key for evoked):
 *   - Samples in a trigger protection window are down-weighted for window statistics.
 *   - Output is blended: protected samples -> fallback (FULL-FIT or SSS-only), others -> tSSS output.
 *   - Projector updates can be frozen for some time after a trigger to avoid learning evoked.
 */

#ifndef ADAPTIVETSSS_RTPROCESSING_H
#define ADAPTIVETSSS_RTPROCESSING_H

#include "rtprocessing_global.h"
#include "helpers/adaptivetssskernel.h"

#include <Eigen/Core>
#include <QSharedPointer>

#include <limits>
#include <vector>

namespace RTPROCESSINGLIB
{

struct RTPROCESINGSHARED_EXPORT AdaptiveTSSSDebugInfo
{
    // Energies
    double e_total = 0.0;       //!< ||X_pick||_F^2
    double e_out   = 0.0;       //!< ||Y_out||_F^2, Y_out = G_out * A_out
    double R_out   = 0.0;       //!< e_out / e_total

    // Temporal status
    bool   gate_on = false;     //!< temporal applied outside protection
    bool   proj_ready = false;  //!< projector available
    int    k_removed = 0;       //!< removed rank at last update
    double corr_max  = 0.0;     //!< max canonical correlation at last update

    // Trigger protection
    int    n_trig = 0;          //!< number of trigger onsets in this chunk
    double protect_frac_soft = 0.0; //!< fraction of samples with protection weight > 0
    double protect_frac_hard = 0.0; //!< fraction of samples with protection weight >= 0.5
    bool   protect_fullfit = true;  //!< protected samples fallback is FULL-FIT (else SSS-only)

    int    protect_carry_in_samp  = 0; //!< carry-in protected samples at chunk start
    int    protect_carry_out_samp = 0; //!< carry-out protected samples into next chunk

    // Update gating
    double freeze_left_ms = 0.0; //!< remaining freeze window
    int    win_samp = 0;         //!< tau window length (samples)
    int    ring_fill = 0;        //!< ring fill level

    // Output shaping
    double beta_energy = 1.0;    //!< energy safeguard beta (1=no clamp)
    double hf_ratio_post = 1.0;  //!< ||dX_post||^2 / ||dX_raw||^2 (high-freq proxy)

    // Stats / diagnostics
    double coeff_lp_hz = 0.0;    //!< coefficient stats LPF cutoff
    double aout_rel = 0.0;       //!< ||Aout||_F / (||Ain||_F + eps)
    double yout_field_rel = 0.0; //!< ||Yout||_F / (||Yin||_F + eps), Yin=Gin*Ain
    double prot_full_raw_ratio = 1.0; //!< RMS(FULL-FIT) / RMS(RAW) within hard-protected samples
    double prot_raw_mix = 0.0;   //!< actual RAW mix used within protected fallback (0..1)
    double ext_reg_factor = 0.0; //!< external ridge multiplier vs internal

    // External-noise subspace cancellation (eSSS-inspired)
    bool   ext_noise_ready = false; //!< external-noise subspace available
    int    ext_noise_k = 0;         //!< dimension of external-noise subspace
    double ext_noise_energy_keep = 0.0; //!< target retained energy for noise subspace
    double ext_res_scale = 0.0;     //!< scale applied to external residual add-back (0..1)

    // Residual (model-mismatch) feature cancellation (eSSS-style “feature” basis)
    bool   feat_ready = false;      //!< sensor-residual feature basis available
    int    feat_k = 0;              //!< feature basis dimension
    double feat_energy_keep = 0.0;  //!< target retained energy for feature basis
    double feat_reg_factor = 0.0;   //!< ridge multiplier vs internal for feature coefficients
};

class RTPROCESINGSHARED_EXPORT AdaptiveTSSS
{
public:
    typedef QSharedPointer<AdaptiveTSSS> SPtr;
    typedef QSharedPointer<const AdaptiveTSSS> ConstSPtr;

    AdaptiveTSSS();
    ~AdaptiveTSSS() = default;

    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData,
                              const RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel,
                              const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData,
                              const RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel,
                              const Eigen::RowVectorXi& vecPicks,
                              Eigen::MatrixXd* pOutSSSOnly,
                              RTPROCESSINGLIB::AdaptiveTSSSDebugInfo* pDbg);

    // ---- configuration ----
    void reset();

    void setSamplingFrequency(double fsHz);
    void setCoefficientLowpassHz(double fcHz); // stats-only

    void setChannelWeights(const Eigen::VectorXd& w);
    void clearChannelWeights();

    void setWarmupChunks(int warmupChunks, bool returnSpatialUntilWarm);
    void setTemporalMemorySeconds(double tauSeconds, bool lock);
    void setTemporalCorrLimitBase(double corrLimit);
    void setProjectorUpdatePeriodChunks(int periodChunks);

    // Exponential smoothing on the temporal removal projector.
    // Smoothing improves baseline stability but must remain numerically safe.
    // alpha in [0, 0.99]. 0 disables smoothing.
    void setProjectorSmoothingAlpha(double alpha);

    // Clamp temporal branch gain to avoid numerical blow-ups ("爆振") on simulated
    // or ill-conditioned data. This limits the energy of the temporal output relative
    // to the spatial SSS output (unprotected samples).
    // maxGain: upper bound on ||XtSSS|| / ||Xsss|| (energy proxy); typical 1.1~1.5.
    // freezeSec: freeze projector updates for this duration after a clamp event.
    void setTemporalGainClamp(double maxGain, double freezeSec = 1.0);

    // Penalize external coefficients in the spatial solve (prevents internal evoked energy leaking into Aout)
    void setExternalRegFactor(double factor);

    // Trigger-aware protection
    void setTriggerProtectionMs(double preMs, double postMs, double taperMs = 20.0);
    void setTriggerFreezeUpdateMs(double ms);
    void setPendingTriggers(const std::vector<int>& sampleOffsets);

    // Protected-sample fallback: FULL-FIT keeps internal+external spatial fit (recommended for evoked preservation)
    void setProtectFallbackFullFit(bool enabled);

    // Protected fallback amplitude safeguard: if FULL-FIT attenuates too much, blend toward RAW.
    // targetRatio: ensure RMS(FULL-FIT) >= targetRatio*RMS(RAW) within hard-protected samples by mixing RAW.
    void setProtectRawSafety(double targetRatio, double maxRawMix = 1.0);

    // Clamp overall output gain (relative to RAW) for non-protected samples.
    // This prevents global amplitude inflation where noise and signal are amplified together.
    // maxGain <= 1.0 means "never amplify" (strict). Typical 1.02~1.10.
    void setOutputGainClamp(double maxGain);

    // Adaptively shrink trigger protection windows based on the residual energy
    // r(t)=||x(t)-x_sss(t)||^2 / ||x(t)||^2. When r(t) stays below threshold
    // for stableMs, protection is ended early (with a taper).
    // This reduces over-wide protected regions that can otherwise dominate the chunk.
    void setAdaptiveProtectionShrink(bool enabled,
                                    double residualRatioThresh = 0.03,
                                    double stableMs = 8.0,
                                    double minHoldMs = 10.0);

    // Clamp protected fallback gain relative to RAW within hard-protected samples.
    // If the fallback RMS exceeds maxGain*RAW RMS, the fallback is scaled down.
    // Typical 1.02~1.10.
    void setProtectFallbackGainCap(double maxGain);

    // eSSS-inspired: learn dominant external-noise subspace online from Aout (excluding protected samples)
    // and cancel it while adding back the residual external field to preserve evoked components that leak into Aout.
    void setExternalNoiseCancel(bool enabled,
                                double energyKeep = 0.95,
                                int maxDim = 12,
                                double forget = 0.98,
                                int updatePeriodChunks = 50,
                                bool applyOutsideProtect = true,
                                double maxResidualRatio = 0.15);

    // eSSS-style “feature” basis: learn dominant *sensor-space* residual patterns that are NOT
    // explained by the Maxwell bases (Gin/Gout), then include them as nuisance regressors in the
    // spatial WLS solve. This targets calibration/model mismatch interference that classic SSS/tSSS
    // cannot represent, and typically yields a tangible SNR lift on real SQUID data.
    void setResidualFeatureCancel(bool enabled,
                                  double energyKeep = 0.95,
                                  int maxDim = 8,
                                  double forget = 0.98,
                                  int updatePeriodChunks = 100,
                                  double regFactor = 10.0);

    // Optional basis loading from kernel CSV paths
    bool loadBasisIfNeeded(RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel);

private:
    void rebuildSpatialSolver();

    void prepareIfNeeded(const Eigen::MatrixXd& matData,
                         const RTPROCESSINGLIB::AdaptiveTSSSKernel& kernel,
                         const Eigen::RowVectorXi& vecPicks);

    void updateLowpassAlpha();
    Eigen::VectorXd lowpassStep(const Eigen::VectorXd& x,
                                Eigen::VectorXd& state) const;

    void ringPush(const Eigen::VectorXd& ain_lp,
                  const Eigen::VectorXd& aout_lp);

    void ringWindow(Eigen::MatrixXd& AinWin,
                    Eigen::MatrixXd& AoutWin) const;

    void updateProjectorIfNeeded(double chunkDurSec);
    bool computeProjectorFromWindow();

    void buildProtectionWeights(int nSamp, std::vector<double>& w);

    static double firstDiffEnergy(const Eigen::MatrixXd& X);

private:
    // Cached picks and kernel version
    Eigen::RowVectorXi m_vecPicksLast;
    int                m_iKernelVersionLast = std::numeric_limits<int>::min();

    // Bases for picked channels
    Eigen::MatrixXd    m_matGIn;
    Eigen::MatrixXd    m_matGOut;

    // Learned sensor-residual feature basis (picked-channel space)
    Eigen::MatrixXd    m_matFeat; // (nPick x nFeat)
    int                m_nFeat = 0;

    // Spatial inverse (weighted, column-normalized)
    Eigen::MatrixXd    m_matPinvW;

    int                m_nPick = 0;
    int                m_nIn   = 0;
    int                m_nOut  = 0;

    // Weights
    bool               m_bHasWeights = false;
    Eigen::VectorXd    m_vecWFull;
    Eigen::VectorXd    m_vecWPick;

    // Temporal window / ring
    double             m_dFsHz = 1024.0;
    double             m_dTauSec = 10.0;
    bool               m_bTauLocked = true;

    int                m_iWinSamp = 0;
    int                m_iRingPos = 0;
    int                m_iRingFill = 0;
    Eigen::MatrixXd    m_ringAinLP;
    Eigen::MatrixXd    m_ringAoutLP;

    // Stats LPF
    bool               m_bUseCoeffLP = false;
    double             m_dCoeffLpHz = 0.0;
    double             m_dCoeffLpAlpha = 0.0;
    Eigen::VectorXd    m_stateAinLP;
    Eigen::VectorXd    m_stateAoutLP;

    // Projector
    bool               m_bProjReady = false;
    Eigen::MatrixXd    m_matR;
    Eigen::MatrixXd    m_matRSmoothed;
    bool               m_bHasRSmoothed = false;
    double             m_dProjSmoothAlpha = 0.85;

    // Temporal safety: clamp excessive temporal gain (prevents "爆振")
    double             m_dMaxTemporalGain = 1.25; // upper bound on temporal/spatial energy ratio
    double             m_dBadProjFreezeSec = 1.0; // freeze projector updates after clamp
    double             m_dBadProjMaxVsRaw  = 5.0; // consider temporal branch unstable if RMS(XtSSS)/RMS(raw) exceeds this

    // Update scheduling
    int                m_iChunkCounter = 0;
    int                m_iWarmupChunks = 50;
    bool               m_bReturnSpatialUntilWarm = true;

    int                m_iUpdatePeriodChunks = 5;
    double             m_dCorrLimit = 0.98;
    double             m_dCovEps = 1e-6;
    double             m_dExtRegFactor = 100.0; // external ridge multiplier vs internal
    double             m_dRegSSS = 1e-12;       // base ridge from kernel (kernel.dRegSSS)
    int                m_iMaxRemove = 20;

    // Trigger-aware protection
    std::vector<int>   m_pendingTrigOffsets;
    double             m_dTrigPreMs  = 0.0;
    double             m_dTrigPostMs = 140.0;
    double             m_dTrigTaperMs = 10.0;

    double             m_dFreezeUpdateMs = 0.0;
    double             m_dFreezeLeftSec = 0.0;

    // How much protected samples affect learning (0 => exclude fully)
    double             m_dLearnWeightFloor = 0.05;

    // Cross-chunk protection carry (samples at next chunk start)
    int                m_iProtectCarrySamp = 0;

    // Protected-sample fallback
    bool               m_bProtectFallbackFullFit = true;

    // RAW safety mix inside protected fallback (prefers FULL-FIT, blends toward RAW if FULL-FIT attenuates)
    bool               m_bProtRawSafety = true;
    double             m_dProtTargetRatio = 0.90;
    double             m_dProtMaxRawMix = 1.0;

    // Clamp protected fallback gain relative to RAW (hard-protected samples)
    bool               m_bProtGainCap = true;
    double             m_dProtGainMax = 1.05;

    // Prevent global amplitude inflation: clamp unprotected output RMS relative to RAW
    bool               m_bOutGainClamp = true;
    double             m_dOutGainMax = 1.05;

    // Adaptive shrink of trigger protection windows based on Maxwell residual ratio
    bool               m_bAdaptiveProtShrink = true;
    double             m_dProtResidRatioThresh = 0.03;
    double             m_dProtStableMs = 8.0;
    double             m_dProtMinHoldMs = 10.0;

    // External-noise subspace cancellation (eSSS-inspired)
    bool               m_bExtNoiseCancel = false;
    double             m_dExtEnergyKeep = 0.95;
    int                m_iExtMaxDim = 12;
    double             m_dExtForget = 0.98;
    int                m_iExtUpdatePeriodChunks = 50;
    bool               m_bExtApplyOutsideProtect = true;
    double             m_dExtMaxResidualRatio = 0.15;

    Eigen::MatrixXd    m_matCoo;   // covariance of Aout (nOut x nOut)
    Eigen::MatrixXd    m_matUout;  // noise subspace basis (nOut x k)
    int                m_iExtK = 0;
    bool               m_bExtReady = false;
    double             m_dExtResScaleLast = 0.0;

    // Residual (model-mismatch) feature cancellation (eSSS-style feature basis)
    bool               m_bFeatCancel = false;
    double             m_dFeatEnergyKeep = 0.95;
    int                m_iFeatMaxDim = 8;
    double             m_dFeatForget = 0.98;
    int                m_iFeatUpdatePeriodChunks = 100;
    double             m_dFeatRegFactor = 10.0;

    Eigen::MatrixXd    m_matCrr;    // covariance of sensor residual (nPick x nPick)
    Eigen::MatrixXd    m_matUfeat;  // feature basis (nPick x k)
    int                m_iFeatK = 0;
    bool               m_bFeatReady = false;

    // Stats
    int                m_iKRemovedLast = 0;
    double             m_dCorrMaxLast  = 0.0;
};

} // namespace RTPROCESSINGLIB

#endif // ADAPTIVETSSS_RTPROCESSING_H

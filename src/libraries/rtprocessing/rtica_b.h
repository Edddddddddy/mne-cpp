//=============================================================================================================
/**
 * @file     rtica.h
 * @author   Lcy
 * @since    0.1.0
 * @date     January, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Lcy. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    RtIca class declaration.
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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// STL
//=============================================================================================================

#include <algorithm>
#include <limits>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * DECLARE CLASS RtIca
 *
 * @brief Real-time ICA processor using ORICA-style block updates.
 *
 * 本类参考 FilterOverlapAdd 的“流式 + 状态持久化”设计：
 *  - weights / sphere / counter 在多次 calculate() 调用间保持
 *  - 支持把任意长度的数据块拆成小 block 进行在线更新
 *  - 可选：去除能量最大的 ICA 组分（Top-k by running power）并重构回传感器空间
 */
class RTPROCESINGSHARED_EXPORT RtIca
{
public:
    typedef QSharedPointer<RtIca> SPtr;             /**< Shared pointer type for RtIca. */
    typedef QSharedPointer<const RtIca> ConstSPtr;  /**< Const shared pointer type for RtIca. */

    //=========================================================================================================
    /**
     * @brief Runtime configuration.
     */
    struct Config
    {
        // Block sizes (samples). If <= 0, treat the whole incoming chunk as one block.
        int     iBlockSizeIca      = 8;    /**< ORICA update block size. */
        int     iBlockSizeWhite    = 8;    /**< Whitening update block size. If <=0, fallback to iBlockSizeIca. */

        // Whitening / ORICA
        bool    bOnlineWhitening   = true; /**< Enable online RLS whitening update. */
        double  dGamma             = 0.6;  /**< Cooling exponent gamma. */
        double  dLambda0           = 0.995;/**< Cooling parameter lambda_0. */

        // ORICA Matlab reference: lambda_const = 1 - exp(-1/tau_const)
        // Unit: samples. If +Inf, floor is 0.
        double  dTauConstSamples   = std::numeric_limits<double>::infinity();

        // Kurtosis sign config: by default treat all components as supergaussian.
        // If you expect subgaussian components, set iNumSubgaussian > 0 (the first N comps).
        int     iNumSubgaussian    = 0;

        // Artifact removal
        bool    bRemoveLargest     = true; /**< Remove largest component(s) after separation. */
        int     iNumRemove         = 1;    /**< Remove top-k components by running power. */
        double  dPowerEmaAlpha     = 0.05; /**< EMA smoothing for component power [0..1]. */
        int     iWarmupBlocks      = 10;   /**< Do not remove components during first N blocks. */

        // Orthogonalization
        int     iOrthoEveryNBlocks = 1;    /**< Symmetric decorrelation every N blocks (>=1). */

        // Conventions / outputs
        bool    bMatlabWhiteningLambdaOneMinus = true;
        /**< If true, whitening uses lambda_avg = 1 - lambda (as in orica.m dynamicWhitening). */

        bool    bReturnWhitened    = false;
        /**< If true, calculate() returns cleaned Z (whitened sensor data) instead of de-whitened X. */
    };

    RtIca();
    explicit RtIca(const Config& cfg);
    ~RtIca();

    //=========================================================================================================
    /**
     * @brief Initialize (or re-initialize) for a given channel count.
     */
    void init(int iNChannels, const Config& cfg = Config());

    //=========================================================================================================
    /**
     * @brief Backwards-compatible init.
     */
    void init(int iNChannels);

    //=========================================================================================================
    /**
     * @brief Reset learning state (weights/sphere/counter/power). Config is kept.
     */
    void reset();

    //=========================================================================================================
    /**
     * @brief Set configuration. Does not reset the learning state automatically.
     */
    void setConfig(const Config& cfg);

    //=========================================================================================================
    const Config& config() const;

    //=========================================================================================================
    /**
     * @brief Run ORICA + optional removal and return cleaned data.
     *
     * Input/Output shape: (nChannels x nSamples).
     */
    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * @brief Run ORICA update and return the ICA sources (no removal).
     */
    Eigen::MatrixXd calculateSources(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * @brief Backwards-compatible setters.
     */
    void setGamma(double dGamma);
    void setLambda0(double dLambda0);
    void setRemoveLargestComponent(bool bEnable);
    void setNumRemove(int iNumRemove);

    //=========================================================================================================
    /**
     * @brief Convenience: the strongest removed component index from the last processed block.
     * @return -1 if nothing removed.
     */
    int lastRemovedComponent() const;

    //=========================================================================================================
    int channelCount() const;
    long sampleCounter() const;

    //=========================================================================================================
    const Eigen::MatrixXd& weights() const;
    const Eigen::MatrixXd& sphere() const;

    //=========================================================================================================
    /**
     * @brief Unmixing matrix from raw X to sources Y:  W * Sphere.
     */
    Eigen::MatrixXd unmixingMatrix() const;

    //=========================================================================================================
    const Eigen::VectorXd& componentPowerEma() const;
    const std::vector<int>& lastRemovedComponents() const;

private:
    void ensureInitialized(int nCh);
    void normalizeConfig();
    double lambdaConst() const;
    void updateKurtosisSign();

    void updateWhiteningBlock(const Eigen::MatrixXd& matDataRaw);
    void updateOricaBlock(const Eigen::MatrixXd& matDataWhitened);
    void symmetricDecorrelate();

    void updatePowerEstimate(const Eigen::MatrixXd& sources);
    void removeLargestComponents(Eigen::MatrixXd& sources);

private:
    Config              m_cfg;

    int                 m_iNChannels;
    bool                m_bIsInitialized;

    Eigen::MatrixXd     m_matWeights;
    Eigen::MatrixXd     m_matSphere;

    long                m_lCounter;      /**< Absolute sample counter (like state.counter). */
    int                 m_iBlockCounter; /**< Number of ICA blocks processed. */

    // Component statistics
    Eigen::VectorXd     m_vecPowerEma;
    std::vector<int>    m_vecLastRemoved;
    Eigen::Array<bool, Eigen::Dynamic, 1> m_vecKurtSign;

    // Temporaries to reduce allocations
    Eigen::MatrixXd     m_matCov;
    Eigen::MatrixXd     m_matY;
    Eigen::MatrixXd     m_matF;
    Eigen::RowVectorXd  m_vecLambdaK;
    Eigen::RowVectorXd  m_vecDotFy;
    Eigen::RowVectorXd  m_vecQ;
    Eigen::RowVectorXd  m_vecScales;
    Eigen::MatrixXd     m_matYScaled;
};

} // namespace RTPROCESSINGLIB

#endif // RTICA_H

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
 *
 * @brief    RtIca class declaration.
 *
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

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * DECLARE CLASS RtIca
 *
 * @brief The RtIca class provides Real-Time Independent Component Analysis (ORICA).
 */
class RTPROCESINGSHARED_EXPORT RtIca
{
public:
    typedef QSharedPointer<RtIca> SPtr;             /**< Shared pointer type for RtIca. */
    typedef QSharedPointer<const RtIca> ConstSPtr;  /**< Const shared pointer type for RtIca. */

    //=========================================================================================================
    /**
     * Constructs a RtIca object.
     */
    RtIca();

    //=========================================================================================================
    /**
     * Destroys the RtIca object.
     */
    ~RtIca();

    //=========================================================================================================
    /**
     * Initializes the RtIca object.
     *
     * @param[in] iNChannels    Number of channels.
     */
    void init(int iNChannels);

    //=========================================================================================================
    /**
     * Calculates the ICA of the input data.
     *
     * @param[in] matData    Input data matrix (channels x samples).
     *
     * @return The ICA transformed data.
     */
    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData);

private:
    //=========================================================================================================
    /**
     * Updates the whitening matrix based on the input data block.
     *
     * @param[in] matData    Input data block.
     */
    void updateWhitening(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Updates the ICA weights based on the input data block.
     *
     * @param[in] matData    Input data block (whitened).
     */
    void updateOrica(const Eigen::MatrixXd& matData);

    int m_iNChannels;               /**< Number of channels. */
    Eigen::MatrixXd m_matWeights;   /**< ICA weight matrix. */
    Eigen::MatrixXd m_matSphere;    /**< Sphere matrix. */
    
    // Forgetting factor parameters (cooling)
    double m_dGamma;                /**< Gamma parameter for cooling forgetting factor. */
    double m_dLambda0;              /**< Initial lambda. */
    long m_lCounter;                /**< Sample counter. */

    bool m_bIsInitialized;          /**< Initialization flag. */

    // Temporaries to avoid reallocation
    Eigen::MatrixXd m_matCov;
    Eigen::MatrixXd m_matY;
    Eigen::MatrixXd m_matF;
    Eigen::RowVectorXd m_vecLambdaK;
    Eigen::RowVectorXd m_vecQ;
    Eigen::RowVectorXd m_vecScales;
    Eigen::MatrixXd m_matYScaled;
    Eigen::RowVectorXd m_vecDotFy;
};

} // NAMESPACE RTPROCESSINGLIB

#endif // RTICA_H

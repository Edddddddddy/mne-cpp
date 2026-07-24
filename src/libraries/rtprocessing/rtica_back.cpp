//=============================================================================================================
/**
 * @file     rtica.cpp
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
 * @brief    RtIca class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtica.h"
#include <iostream>
#include <cmath>
#include <QDebug>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtIca::RtIca()
: m_iNChannels(0)
, m_dGamma(0.6)
, m_dLambda0(0.995)
, m_lCounter(0)
, m_bIsInitialized(false)
{
}

//=============================================================================================================

RtIca::~RtIca()
{
}

//=============================================================================================================

void RtIca::init(int iNChannels)
{
    m_iNChannels = iNChannels;
    m_matWeights = MatrixXd::Identity(m_iNChannels, m_iNChannels);
    m_matSphere = MatrixXd::Identity(m_iNChannels, m_iNChannels);
    m_lCounter = 0;
    m_bIsInitialized = true;

    // Pre-allocate temporaries
    // Note: Some dimensions depend on block size, which might vary if we clamp/min it.
    // But m_matCov is fixed size.
    m_matCov.resize(m_iNChannels, m_iNChannels);
}

//=============================================================================================================

Eigen::MatrixXd RtIca::calculate(const Eigen::MatrixXd& matData)
{
    if(!m_bIsInitialized || matData.rows() != m_iNChannels) {
        if(matData.rows() > 0) {
             init(matData.rows());
        } else {
             return matData;
        }
    }

    // 1. Update Whitening Matrix
    updateWhitening(matData);
    
    // 2. Apply Whitening
    MatrixXd matWhitened = m_matSphere * matData;
    
    // 3. Update ICA Weights
    updateOrica(matWhitened);
    
    // 4. Artifact Removal
    // Calculate sources S = W * Z (where Z is whitened data)
    // Note: We use the current m_matSphere and m_matWeights
    MatrixXd matSources = m_matWeights * matWhitened;

    // Find component with maximum variance
    VectorXd variances = matSources.rowwise().squaredNorm();
    int maxIdx = 0;
    variances.maxCoeff(&maxIdx);

    // Remove the largest component
    matSources.row(maxIdx).setZero();

    // Reconstruct data
    // X_clean = Q^-1 * W^T * S_clean
    // 1. Z_clean = W^T * S_clean (since W is orthogonal)
    MatrixXd matZClean = m_matWeights.transpose() * matSources;

    // 2. X_clean = Q^-1 * Z_clean
    // Solve Q * X_clean = Z_clean
    // Using LLT decomposition since Sphere matrix should be symmetric positive definite
    // If not, use LDLT or ColPivHouseholderQR
    MatrixXd matXClean = m_matSphere.llt().solve(matZClean);

    return matXClean;
}

//=============================================================================================================

void RtIca::updateWhitening(const Eigen::MatrixXd& matData)
{
    int nPts = matData.cols();
    
    long t = m_lCounter + nPts / 2; 
    if (t == 0) t = 1;
    
    double lambda = m_dLambda0 / std::pow((double)t, m_dGamma);
    
    // MatrixXd v = m_matSphere * matData;
    // Use member if possible, but matData size varies.
    // Just use local for v as it depends on nPts.
    MatrixXd v = m_matSphere * matData;

    // double trace_vv = (v.transpose() * v).trace();
    double trace_vv = v.squaredNorm();
    
    double QWhite = lambda / (1.0 - lambda) + trace_vv / nPts;
    
    // m_matSphere = (1.0 / lambda) * (m_matSphere - v * v.transpose() / nPts / QWhite * m_matSphere);
    // Optimize associativity: (v * v') * S -> v * (v' * S)
    // v is nCh x nPts. S is nCh x nCh.
    // v' * S is nPts x nCh.
    // v * (v' * S) is nCh x nCh.
    // Cost: nPts * nCh^2 + nCh * nPts * nCh = 2 * nPts * nCh^2.
    // Original: nCh^2 * nPts + nCh^3.
    // Since nPts << nCh, this is much faster.
    
    double factor = 1.0 / (nPts * QWhite);
    m_matSphere = (1.0 / lambda) * (m_matSphere - factor * v * (v.transpose() * m_matSphere));
}

//=============================================================================================================

void RtIca::updateOrica(const Eigen::MatrixXd& matData)
{
    int nPts = matData.cols();
    
    // MatrixXd y = m_matWeights * matData;
    m_matY = m_matWeights * matData;
    
    // Nonlinearity
    // f(y) = -2 * tanh(y) for supergaussian
    // MatrixXd f = -2.0 * y.array().tanh();
    m_matF = -2.0 * m_matY.array().tanh();
    
    // Update weights
    // RowVectorXd lambda_k(nPts);
    m_vecLambdaK.resize(nPts);
    for(int i=0; i<nPts; ++i) {
        long t = m_lCounter + i + 1;
        m_vecLambdaK(i) = m_dLambda0 / std::pow((double)t, m_dGamma);
    }
    
    // Update counter
    m_lCounter += nPts;
    
    double lambda_prod = 1.0;
    for(int i=0; i<nPts; ++i) {
        lambda_prod *= (1.0 / (1.0 - m_vecLambdaK(i)));
    }
    
    // RowVectorXd dot_fy = (f.array() * y.array()).colwise().sum();
    m_vecDotFy = (m_matF.array() * m_matY.array()).colwise().sum();
    
    // RowVectorXd Q = 1.0 + lambda_k.array() * (dot_fy.array() - 1.0);
    m_vecQ = 1.0 + m_vecLambdaK.array() * (m_vecDotFy.array() - 1.0);
    
    // MatrixXd diag_lambda_Q = (lambda_k.array() / Q.array()).matrix().asDiagonal();
    // m_matWeights = lambda_prod * (m_matWeights - y * diag_lambda_Q * f.transpose() * m_matWeights);

    // Optimize:
    // 1. Avoid diag matrix. Scale y columns.
    // 2. Associativity: (y * f') * W -> y * (f' * W)

    // RowVectorXd scales = lambda_k.array() / Q.array();
    m_vecScales = m_vecLambdaK.array() / m_vecQ.array();
    // MatrixXd y_scaled = y.array().rowwise() * scales.array(); // nCh x nPts
    m_matYScaled = m_matY.array().rowwise() * m_vecScales.array();

    // f.transpose() * m_matWeights -> nPts x nCh
    // y_scaled * (f.transpose() * m_matWeights) -> nCh x nCh
    // Cost: nPts * nCh^2 + nCh * nPts * nCh = 2 * nPts * nCh^2.
    // Original: nCh^2 * nPts + nCh^3.

    m_matWeights = lambda_prod * (m_matWeights - m_matYScaled * (m_matF.transpose() * m_matWeights));
    
    // Orthogonalize
    // SelfAdjointEigenSolver<MatrixXd> es(m_matWeights * m_matWeights.transpose());
    // Use rankUpdate for W * W'
    // MatrixXd cov(m_iNChannels, m_iNChannels);
    m_matCov.setZero();
    m_matCov.selfadjointView<Lower>().rankUpdate(m_matWeights);

    SelfAdjointEigenSolver<MatrixXd> es(m_matCov);
    MatrixXd V = es.eigenvectors();
    VectorXd D = es.eigenvalues();
    
    // (WW')^{-1/2} * W
    VectorXd invSqrtD = D.array().inverse().sqrt();
    MatrixXd invSqrtMat = V * invSqrtD.asDiagonal() * V.transpose();
    
    m_matWeights = invSqrtMat * m_matWeights;
}

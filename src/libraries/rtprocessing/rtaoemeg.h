//=============================================================================================================
/**
 * @file     rtaoemeg.h
 * @author   OpenAI Codex
 * @date     March, 2026
 *
 * @brief    Real-time AOE-MEG pipeline core.
 *
 */

#ifndef RTAOEMEG_RTPROCESSING_H
#define RTAOEMEG_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"
#include "aoemegtypes.h"

#include <fiff/fiff_proj.h>

#include <QVector>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffInfo;
}

namespace RTPROCESSINGLIB
{

class RTPROCESINGSHARED_EXPORT RtAoeMeg
{
public:
    typedef QSharedPointer<RtAoeMeg> SPtr;
    typedef QSharedPointer<const RtAoeMeg> ConstSPtr;

    RtAoeMeg();
    ~RtAoeMeg() = default;

    void setSettings(const AOEMegSettings& settings);
    AOEMegSettings settings() const;
    AOEMegDiagnostics diagnostics() const;

    void setFiffInfo(const QSharedPointer<FIFFLIB::FiffInfo>& pFiffInfo);
    void setMotionVector(const Eigen::VectorXd& vecMotion, bool bValid = true);
    void setStage2PriorProjectors(const QList<FIFFLIB::FiffProj>& projs);
    void clearStage2PriorProjectors();
    void reset();

    AOEMegProcessResult process(const Eigen::MatrixXd& matData);

private:
    void resetStage1State();
    void resetStage2State();
    void resetStage3State();
    void rebuildChannelSelection();

    AOEMegSettings                       m_settings;
    AOEMegDiagnostics                    m_diagnostics;
    QSharedPointer<FIFFLIB::FiffInfo>    m_pFiffInfo;
    QVector<int>                         m_vecMegPicks;
    QVector<int>                         m_vecMagLocalRows;
    QVector<int>                         m_vecGradLocalRows;
    Eigen::MatrixXd                      m_matStage1B;
    Eigen::MatrixXd                      m_matStage1P;
    Eigen::VectorXd                      m_vecStage1UMean;
    Eigen::VectorXd                      m_vecStage1UScale;
    Eigen::VectorXd                      m_vecStage1MagScale;
    Eigen::VectorXd                      m_vecStage1GradScale;
    Eigen::MatrixXd                      m_matStage2CovMag;
    Eigen::MatrixXd                      m_matStage2CovGrad;
    Eigen::MatrixXd                      m_matStage2BasisMag;
    Eigen::MatrixXd                      m_matStage2BasisGrad;
    Eigen::MatrixXd                      m_matStage2PriorBasisMag;
    Eigen::MatrixXd                      m_matStage2PriorBasisGrad;
    Eigen::VectorXd                      m_vecStage2MagScale;
    Eigen::VectorXd                      m_vecStage2GradScale;
    QList<FIFFLIB::FiffProj>             m_listStage2PriorProjs;
    Eigen::MatrixXd                      m_matStage3CovMag;
    Eigen::MatrixXd                      m_matStage3CovGrad;
    Eigen::MatrixXd                      m_matStage3PhysicalBasisMag;
    Eigen::MatrixXd                      m_matStage3PhysicalBasisGrad;
    Eigen::MatrixXd                      m_matStage3StatBasisMag;
    Eigen::MatrixXd                      m_matStage3StatBasisGrad;
    Eigen::VectorXd                      m_vecStage3MagScale;
    Eigen::VectorXd                      m_vecStage3GradScale;
    double                               m_dStage3PrevBandPowerMag = 0.0;
    double                               m_dStage3PrevBandPowerGrad = 0.0;
    double                               m_dStage3PrevRhoMag = 0.0;
    double                               m_dStage3PrevRhoGrad = 0.0;
    int                                  m_iStage3CooldownMag = 0;
    int                                  m_iStage3CooldownGrad = 0;
    Eigen::VectorXd                      m_vecLatestMotion;
    bool                                 m_bHasMotionVector = false;
};

} // namespace RTPROCESSINGLIB

#endif // RTAOEMEG_RTPROCESSING_H

//=============================================================================================================
/**
 * @file     noisereduction.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the NoiseReduction class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction.h"

#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/spharasettingsview.h>

#include <rtprocessing/filter.h>
#include <rtprocessing/sphara.h>
#include <rtprocessing/adaptivetsss.h>
#include <rtprocessing/detecttrigger.h>

#include <utils/ioutils.h>

#include <scMeas/realtimemultisamplearray.h>

#include "FormFiles/noisereductionsetupwidget.h"

#include <QElapsedTimer>
#include <QString>
#include <cmath>
#include <algorithm>
#include <vector>
#include <set>

namespace {

static Eigen::MatrixXd pickRows(const Eigen::MatrixXd& M, const Eigen::RowVectorXi& picks)
{
    Eigen::MatrixXd out(picks.size(), M.cols());
    for(int i = 0; i < picks.size(); ++i) {
        out.row(i) = M.row(picks(i));
    }
    return out;
}

static double rmsAll(const Eigen::MatrixXd& X)
{
    if(X.size() == 0) {
        return 0.0;
    }
    return std::sqrt(X.array().square().mean());
}

static double goertzelAvgPower(const Eigen::MatrixXd& X, double sfreq, double freq)
{
    const int nChan = static_cast<int>(X.rows());
    const int N = static_cast<int>(X.cols());
    if(nChan == 0 || N < 2 || sfreq <= 0.0 || freq <= 0.0) {
        return 0.0;
    }

    const double k = 0.5 + (static_cast<double>(N) * freq / sfreq);
    const double w = 2.0 * std::acos(-1.0) * k / static_cast<double>(N);
    const double coeff = 2.0 * std::cos(w);

    double acc = 0.0;

    for(int ch = 0; ch < nChan; ++ch) {
        double s_prev = 0.0;
        double s_prev2 = 0.0;

        for(int n = 0; n < N; ++n) {
            const double x = X(ch, n);
            const double s = x + coeff * s_prev - s_prev2;
            s_prev2 = s_prev;
            s_prev = s;
        }

        const double power = s_prev2*s_prev2 + s_prev*s_prev - coeff*s_prev*s_prev2;
        acc += power / static_cast<double>(N);
    }

    return acc / static_cast<double>(nChan);
}

} // anonymous namespace


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NOISEREDUCTIONPLUGIN;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace UTILSLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace SCSHAREDLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReduction::NoiseReduction()
: m_bCompActivated(false)
, m_bSpharaActive(false)
, m_bProjActivated(false)
, m_bFilterActivated(false)
, m_iMaxFilterLength(1)
, m_iMaxFilterTapSize(-1)
, m_sCurrentSystem("VectorView")
, m_pCircularBuffer(QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>::create(40))
, m_pNoiseReductionInput(Q_NULLPTR)
, m_pNoiseReductionOutput(Q_NULLPTR)
{
    if(m_sCurrentSystem == "BabyMEG") {
        m_iNBaseFctsFirst = 270;
        m_iNBaseFctsSecond = 105;
    } else if(m_sCurrentSystem == "VectorView") {
        m_iNBaseFctsFirst = 102;
        m_iNBaseFctsSecond = 102;
    } else {
        m_iNBaseFctsFirst = 0;
        m_iNBaseFctsSecond = 0;
        qDebug() << "[NoiseReduction::NoiseReduction] Current system type not recognized.";
    }
}

//=============================================================================================================

NoiseReduction::~NoiseReduction()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> NoiseReduction::clone() const
{
    QSharedPointer<NoiseReduction> pNoiseReductionClone(new NoiseReduction);
    return pNoiseReductionClone;
}

//=============================================================================================================

void NoiseReduction::init()
{
    // Input
    m_pNoiseReductionInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "NoiseReductionIn", "NoiseReduction input data");
    connect(m_pNoiseReductionInput.data(), &PluginInputConnector::notify,
            this, &NoiseReduction::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pNoiseReductionInput);

    // Output
    m_pNoiseReductionOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "NoiseReductionOut", "NoiseReduction output data");
    m_pNoiseReductionOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pNoiseReductionOutput);
}

//=============================================================================================================

void NoiseReduction::unload()
{
}

//=============================================================================================================

bool NoiseReduction::start()
{
    //Start thread as soon as we have received the first data block. See update().

    return true;
}

//=============================================================================================================

bool NoiseReduction::stop()
{
    requestInterruption();
    wait(500);

    m_iMaxFilterTapSize = -1;

    m_pNoiseReductionOutput->measurementData()->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType NoiseReduction::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString NoiseReduction::getName() const
{
    return "Filter";
}

//=============================================================================================================

QWidget* NoiseReduction::setupWidget()
{
    NoiseReductionSetupWidget* setupWidget = new NoiseReductionSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void NoiseReduction::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Check if the fiff info was inititalized
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init the multiplication matrices
            m_matSparseProjMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseSpharaMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseProjCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
            m_matSparseFull = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

            m_matSparseProjMult.setIdentity();
            m_matSparseCompMult.setIdentity();
            m_matSparseSpharaMult.setIdentity();
            m_matSparseProjCompMult.setIdentity();
            m_matSparseFull.setIdentity();

            //Init output
            m_pNoiseReductionOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
            m_pNoiseReductionOutput->measurementData()->setMultiArraySize(1);

            // ---------------------------------------------------------------------------------
            // Adaptive tSSS: initialize MEG picks (exclude STIM/misc). This must be done once we
            // have valid FiffInfo; otherwise AdaptiveTSSS will default to "all channels" which
            // includes STIM and causes basis dimension mismatch (e.g., 344 vs 306).
            // ---------------------------------------------------------------------------------
            // Build MEG picks for tSSS. IMPORTANT: never include trigger/STIM channels (e.g., STI014)
            // even if their FIFF kind is incorrectly marked as MEG in simulated data.
            m_lTsssChannelList.resize(0);
            int nExcludedStimLike = 0;
            for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
                const auto& ch = m_pFiffInfo->chs.at(i);
                const QString nm = ch.ch_name;
                const bool stimLikeByName = nm.startsWith("STI", Qt::CaseInsensitive)
                                         || nm.contains("STI", Qt::CaseInsensitive)
                                         || nm.contains("TRIG", Qt::CaseInsensitive);
                const bool stimLikeByKind = (ch.kind == FIFFV_STIM_CH);

                if(ch.kind == FIFFV_MEG_CH && !stimLikeByName && !stimLikeByKind) {
                    m_lTsssChannelList.conservativeResize(m_lTsssChannelList.cols() + 1);
                    m_lTsssChannelList[m_lTsssChannelList.cols() - 1] = i;
                } else {
                    if(stimLikeByName || stimLikeByKind) {
                        ++nExcludedStimLike;
                    }
                }
            }

            qDebug() << "[tSSS-PICKS]" << "n_meg=" << m_lTsssChannelList.cols()
                     << "nchan_total=" << m_pFiffInfo->chs.size()
                     << "excluded_stim_like=" << nExcludedStimLike;
        }

        // Check if data is present
        if(pRTMSA->getMultiSampleArray().size() > 0) {
            //Init widgets
            if(m_iMaxFilterTapSize == -1) {
                m_iMaxFilterTapSize = pRTMSA->getMultiSampleArray().first().cols();
                initPluginControlWidgets();
                QThread::start();
            }

            for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                // Please note that we do not need a copy here since this function will block until
                // the buffer accepts new data again. Hence, the data is not deleted in the actual
                // Measurement function after it emitted the notify signal.
                while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            }
        }
    }
}

//=============================================================================================================

void NoiseReduction::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Projectors
        ProjectorsView* pProjectorsView = new ProjectorsView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pProjectorsView, &ProjectorsView::setGuiMode);
        pProjectorsView->setObjectName("group_tab_Settings_SSP");
        plControlWidgets.append(pProjectorsView);

        connect(pProjectorsView, &ProjectorsView::projSelectionChanged,
                this, &NoiseReduction::updateProjection);

        pProjectorsView->setProjectors(m_pFiffInfo->projs);

        // Compensators
        CompensatorView* pCompensatorView = new CompensatorView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pCompensatorView, &CompensatorView::setGuiMode);
        pCompensatorView->setObjectName("group_tab_Settings_Comp");
        plControlWidgets.append(pCompensatorView);

        connect(pCompensatorView, &CompensatorView::compSelectionChanged,
                this, &NoiseReduction::updateCompensator);

        pCompensatorView->setCompensators(m_pFiffInfo->comps);

        // Filter
        FilterSettingsView* pFilterSettingsView = new FilterSettingsView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pFilterSettingsView, &FilterSettingsView::setGuiMode);
        pFilterSettingsView->setObjectName("group_tab_Settings_Filter");
        plControlWidgets.append(pFilterSettingsView);

        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChannelTypeChanged,
                this, &NoiseReduction::setFilterChannelType);

        connect(pFilterSettingsView->getFilterView().data(), &FilterDesignView::filterChanged,
                this, &NoiseReduction::setFilter);

        connect(pFilterSettingsView, &FilterSettingsView::filterActivationChanged,
                this, &NoiseReduction::setFilterActive);

        pFilterSettingsView->setSamplingRate(m_pFiffInfo->sfreq);
        pFilterSettingsView->getFilterView()->setMaxAllowedFilterTaps(m_iMaxFilterTapSize);

        this->setFilterActive(pFilterSettingsView->getFilterActive());
        this->setFilterChannelType(pFilterSettingsView->getFilterView()->getChannelType());

        // SPHARA settings
        SpharaSettingsView* pSpharaSettingsView = new SpharaSettingsView(QString("MNESCAN/%1").arg(this->getName()));
        connect(this, &NoiseReduction::guiModeChanged,
                pSpharaSettingsView, &SpharaSettingsView::setGuiMode);
        pSpharaSettingsView->setObjectName("group_tab_Settings_SPHARA");
        plControlWidgets.append(pSpharaSettingsView);

        connect(pSpharaSettingsView, &SpharaSettingsView::spharaActivationChanged,
                this, &NoiseReduction::setSpharaActive);

        connect(pSpharaSettingsView, &SpharaSettingsView::spharaOptionsChanged,
                this, &NoiseReduction::setSpharaOptions);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
    }
}

//=============================================================================================================

void NoiseReduction::setSpharaActive(bool state)
{
    m_mutex.lock();
    m_bSpharaActive = state;

    m_mutex.unlock();
}

//=============================================================================================================

void NoiseReduction::setSpharaOptions(const QString& sSytemType,
                                      int nBaseFctsFirst,
                                      int nBaseFctsSecond)
{
    m_mutex.lock();
    m_iNBaseFctsFirst = nBaseFctsFirst;
    m_iNBaseFctsSecond = nBaseFctsSecond;
    m_sCurrentSystem = sSytemType;
    m_mutex.unlock();

    createSpharaOperator();
}

//=============================================================================================================

void NoiseReduction::run()
{
    // Read and create SPHARA operator for the first time
    initSphara();
    createSpharaOperator();

    // Init
    MatrixXd matData;
    QScopedPointer<RTPROCESSINGLIB::FilterOverlapAdd> pRtFilter(new RTPROCESSINGLIB::FilterOverlapAdd());
    QScopedPointer<RTPROCESSINGLIB::AdaptiveTSSS> pRtTsss(new RTPROCESSINGLIB::AdaptiveTSSS());
    bool m_bTsssActivated = true;

    // ---------------------------------------------------------------------------------
    // Adaptive tSSS: safety net. Ensure MEG picks are initialized before processing loop.
    // ---------------------------------------------------------------------------------
    if(m_lTsssChannelList.size() == 0 && m_pFiffInfo) {
        m_lTsssChannelList.resize(0);
        for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
            if(m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH) {
                m_lTsssChannelList.conservativeResize(m_lTsssChannelList.cols() + 1);
                m_lTsssChannelList[m_lTsssChannelList.cols() - 1] = i;
            }
        }

        qDebug() << "[tSSS-PICKS-RUN]" << "n_meg=" << m_lTsssChannelList.cols()
                 << "nchan_total=" << m_pFiffInfo->chs.size();
    }
    m_tsssKernel.sBasisInCsvPath = "D:/tsss_basis/sss_gin.csv";
    m_tsssKernel.sBasisOutCsvPath = "D:/tsss_basis/sss_gout.csv";
    m_tsssKernel.iVersion = 1;
    // Regularization for SSS inversion (G_in, G_out). 
    // 1e-4 is standard. Too small values (like 1e-10) can cause numerical instability/crashes.
    m_tsssKernel.dRegSSS = 1e-4;  

    // Optional: warn if basis rows don't match MEG picks (common reason for Gin/Gout == 0)
    if(m_tsssKernel.matSSSIn.size() > 0 && m_lTsssChannelList.size() > 0 &&
       m_tsssKernel.matSSSIn.rows() != m_lTsssChannelList.size()) {
        qDebug() << "[tSSS-WARN] basis_rows=" << m_tsssKernel.matSSSIn.rows()
                 << "!= n_meg_picks=" << m_lTsssChannelList.size()
                 << "(basis must be generated for exactly these picked channels/order)";
    }
    static Eigen::VectorXd w;
    static bool bWeightsLoaded = false;
    if(!bWeightsLoaded) {
        Eigen::MatrixXd matWeights;
        UTILSLIB::IOUtils::read_eigen_matrix(matWeights, QString("D:/tsss_basis/meg_weights.csv"));
        if(matWeights.size() > 0) {
            w = matWeights.col(0);
            qDebug() << "[tSSS] Loaded weights from meg_weights.csv. Size:" << w.size();
        }
        bWeightsLoaded = true;
    }
    if(w.size() > 0) {
        pRtTsss->setChannelWeights(w);
    }

    // ----------------------------------------------------------------------------
    // Adaptive tSSS configuration
    // ----------------------------------------------------------------------------
    const double fs = (m_pFiffInfo ? m_pFiffInfo->sfreq : 1024.0);
    pRtTsss->setSamplingFrequency(fs);

    // Temporal part (tSSS-like)
    pRtTsss->setWarmupChunks(50, true);
    pRtTsss->setTemporalMemorySeconds(10.0, true);    // tau ~ 10 s
    pRtTsss->setTemporalCorrLimitBase(0.98);
    pRtTsss->setProjectorUpdatePeriodChunks(50);      // reduce update rate to avoid stalls
    pRtTsss->setCoefficientLowpassHz(20.0);

    // Trigger-aware protection: keep evoked response
    pRtTsss->setTriggerProtectionMs(0.0, 140.0, 10.0);  // 0..140 ms (+10 ms taper)
    pRtTsss->setTriggerFreezeUpdateMs(0.0);
    pRtTsss->setProtectFallbackFullFit(true);           // keep stimulus components

    // Conservative RAW safety (avoid re-injecting noise; only used if fallback attenuates too much)
    pRtTsss->setProtectRawSafety(0.85, 0.02);

    // eSSS-inspired external-noise subspace cancellation:
    // FULL-FIT fallback is preserved, but dominant external-noise subspace is removed from Aout.
    pRtTsss->setExternalNoiseCancel(true, 0.95, 12, 0.98, 50, true, 0.15);
    // tsss.setTemporalMemorySeconds(10.0, true); // 新增：锁定 tau≈10s，对齐离线窗口
    // tsss.setTemporalLambda(1e-4, true);  

    while(!isInterruptionRequested()) {
        // Get the current data
        if(m_pCircularBuffer->pop(matData)) {
            m_mutex.lock();

            // Keep an unmodified copy for trigger detection and for restoring STIM channels.
            const Eigen::MatrixXd matDataRaw = matData;

            // Identify STIM-like channel indices once (by FIFF kind or name), used to prevent
            // leakage of STI014 (high-amplitude) into MEG channels through any spatial mixing.
            static std::vector<int> s_stimLikeIdx;
            if(s_stimLikeIdx.empty() && m_pFiffInfo) {
                for(int ci = 0; ci < m_pFiffInfo->chs.size(); ++ci) {
                    const auto& ch = m_pFiffInfo->chs.at(ci);
                    const QString nm = ch.ch_name;
                    const bool stimLikeByName = nm.startsWith("STI", Qt::CaseInsensitive)
                                             || nm.contains("STI", Qt::CaseInsensitive)
                                             || nm.contains("TRIG", Qt::CaseInsensitive);
                    const bool stimLikeByKind = (ch.kind == FIFFV_STIM_CH);
                    if(stimLikeByName || stimLikeByKind) {
                        s_stimLikeIdx.push_back(ci);
                    }
                }
                if(!s_stimLikeIdx.empty()) {
                    qDebug() << "[STIM-EXCLUDE]" << "n_stim_like=" << static_cast<int>(s_stimLikeIdx.size());
                }
            }

            // Prevent STIM leakage: zero STIM-like inputs before any processing (SSP/comp/tSSS/etc.),
            // then restore them untouched before output.
            if(!s_stimLikeIdx.empty()) {
                matData = matDataRaw;
                for(int ci : s_stimLikeIdx) {
                    if(ci >= 0 && ci < matData.rows()) {
                        matData.row(ci).setZero();
                    }
                }
            }
            //Do SSP's and compensators here
            // if(m_bCompActivated) {
            //     if(m_bProjActivated) {
            //         //Comp + Proj
            //         matData = m_matSparseProjCompMult * matData;
            //     } else {
            //         //Comp
            //         matData = m_matSparseCompMult * matData;
            //     }
            // } else 
            // {
            //     if(m_bProjActivated) {
            //         //Proj
            //         matData = m_matSparseProjMult * matData;
            //     } else {
            //         //None - Raw
            //     }
            // }

            //Do temporal filtering here
            if(m_bFilterActivated) {
                matData = pRtFilter->calculate(matData,
                                               m_filterKernel,
                                               m_lFilterChannelList);
            }

            // Detect trigger (STI014 / STI 014): onset detection (0 -> non-zero) with refractory.
            // Additionally, shift the trigger forward by a fixed lag (default 50 ms) to match pipeline latency,
            // and automatically collect/print newly observed event codes.
            static int s_iTriggerChIndex = -1;
            static bool s_prevHigh = false;
            static long long s_globalSample = 0;
            static long long s_lastOnsetGlobal = -(1LL<<60);
            static std::vector<int> s_carryOffsets; // offsets (samples) carried into next chunk (after lag shift)
            static std::set<int> s_seenIds;

            if(s_iTriggerChIndex == -1 && m_pFiffInfo) {
                s_iTriggerChIndex = m_pFiffInfo->ch_names.indexOf("STI014");
                if(s_iTriggerChIndex == -1) {
                    s_iTriggerChIndex = m_pFiffInfo->ch_names.indexOf("STI 014");
                }
            }

            std::vector<int> trigOffsets;
            trigOffsets.reserve(8);

            // Pull carried offsets first
            if(!s_carryOffsets.empty()) {
                trigOffsets.insert(trigOffsets.end(), s_carryOffsets.begin(), s_carryOffsets.end());
                s_carryOffsets.clear();
            }

            if(s_iTriggerChIndex != -1) {
                const int nSamp = static_cast<int>(matData.cols());
                const double sf = (m_pFiffInfo ? m_pFiffInfo->sfreq : 0.0);
                const int refractorySamp = std::max(1, static_cast<int>(std::round(0.05 * (sf > 0.0 ? sf : 1024.0)))); // 50 ms
                // No trigger lag: protection windows are handled inside AdaptiveTSSS via carry-out.
                const int lagSamp = 0;

                for(int t = 0; t < nSamp; ++t) {
                    const double v = matDataRaw(s_iTriggerChIndex, t);
                    const int iv = static_cast<int>(std::llround(v));
                    const bool high = (iv != 0);

                    if(!s_prevHigh && high) {
                        const long long g = s_globalSample + t;
                        if(g - s_lastOnsetGlobal >= refractorySamp) {
                            // Any non-zero code is accepted. Bit-coded paradigms are also supported.
                            if(s_seenIds.insert(iv).second) {
                                qDebug() << "[TRIG] new_id=" << iv;
                            }

                            const int tLag = t + lagSamp;
                            if(tLag < nSamp) {
                                trigOffsets.push_back(tLag);
                            } else {
                                // carry to next chunk after applying lag
                                s_carryOffsets.push_back(tLag - nSamp);
                            }
                            s_lastOnsetGlobal = g;
                        }
                    }
                    s_prevHigh = high;
                }

                s_globalSample += nSamp;
            }

            // Zero out STIM-like channels in the processing buffer to avoid any spatial mixing
            // (projectors/compensators/sphara) injecting trigger waveforms into MEG channels.
            // We will restore these channels from matDataRaw right before output.
            if(!s_stimLikeIdx.empty()) {
                for(int idx : s_stimLikeIdx) {
                    if(idx >= 0 && idx < matData.rows()) {
                        matData.row(idx).setZero();
                    }
                }
            }

            // Provide trigger offsets (within this chunk) to the adaptive tSSS processor
            pRtTsss->setPendingTriggers(trigOffsets);
            if(m_bTsssActivated) {
                // pRtTsss->setCoefficientLowpassHz(25.0);
                
                
                // --- Load bases on first use (if configured via kernel CSV paths) ---
                static bool s_basisChecked = false;
                if(!s_basisChecked) {
                    bool loaded = pRtTsss->loadBasisIfNeeded(m_tsssKernel);
                    if(loaded) {
                        m_tsssKernel.iVersion += 1;
                        qDebug() << "[tSSS-BASIS-LOAD]"
                                << "loaded=" << true
                                << "Gin=" << m_tsssKernel.matSSSIn.rows() << "x" << m_tsssKernel.matSSSIn.cols()
                                << "Gout=" << m_tsssKernel.matSSSOut.rows() << "x" << m_tsssKernel.matSSSOut.cols();
                    }
                    s_basisChecked = true;
                }

                // --- Pre/post QC (MEG-only picks) ---
                const double sfreq = (m_pFiffInfo ? m_pFiffInfo->sfreq : 0.0);
                const double lineFreq = 50.0; // adjust if needed

                Eigen::MatrixXd XprePicked = pickRows(matData, m_lTsssChannelList);

                Eigen::MatrixXd matSSS; // spatial-only reconstruction (full)
                RTPROCESSINGLIB::AdaptiveTSSSDebugInfo dbg;

                QElapsedTimer tElapsed;
                tElapsed.start();

                Eigen::MatrixXd matOut = pRtTsss->calculate(matData,
                                                        m_tsssKernel,
                                                        m_lTsssChannelList,
                                                        &matSSS,
                                                        &dbg);

                const double last_ms = static_cast<double>(tElapsed.nsecsElapsed()) / 1e6;

                Eigen::MatrixXd XsssPicked  = pickRows(matSSS,  m_lTsssChannelList);
                Eigen::MatrixXd XpostPicked = pickRows(matOut,  m_lTsssChannelList);

                const double rms_pre   = rmsAll(XprePicked);
                const double rms_post  = rmsAll(XpostPicked);
                const double rms_ratio = (rms_post + 1e-25) / (rms_pre + 1e-25);

                const double line_pre  = goertzelAvgPower(XprePicked,  sfreq, lineFreq) + 1e-35;
                const double line_post = goertzelAvgPower(XpostPicked, sfreq, lineFreq) + 1e-35;
                const double lineRatio = line_post / line_pre;

                const double relSSS = rmsAll(XsssPicked - XprePicked) / (rms_pre + 1e-25);
                const double rms_sss = rmsAll(XsssPicked);
                const double relT  = rmsAll(XpostPicked - XsssPicked) / (rms_sss + 1e-25);

                const double driftIndex = (rms_pre + 1e-25) / (rms_post + 1e-25);

                // Update output
                matData = matOut;

                // Throttle logs (set to 1 for every chunk)
                static int s_logCounter = 0;
                const int LOG_EVERY = 10;
                if((++s_logCounter % LOG_EVERY) == 0) {
                    qDebug() << "[tSSS-RT-QC2]"
                            << "R_out=" << dbg.R_out
                            << "protect_frac=" << dbg.protect_frac_hard
                            << "protect_soft=" << dbg.protect_frac_soft
                            << "carry_in=" << dbg.protect_carry_in_samp
                            << "carry_out=" << dbg.protect_carry_out_samp
                            << "extRegF=" << dbg.ext_reg_factor
                            << "gate_on=" << dbg.gate_on
                            << "proj_ready=" << dbg.proj_ready
                            << "nTrig=" << dbg.n_trig
                            << "freeze_ms=" << dbg.freeze_left_ms
                            << "betaE=" << dbg.beta_energy
                            << "aout_rel=" << dbg.aout_rel
                            << "yout_rel=" << dbg.yout_field_rel
                            << "prot_fullraw=" << dbg.prot_full_raw_ratio
                            << "prot_rawmix=" << dbg.prot_raw_mix
                            << "hf_ratio_post=" << dbg.hf_ratio_post
                            << "k_removed=" << dbg.k_removed
                            << "corr_max=" << dbg.corr_max
                            << "lineRatio=" << lineRatio
                            << "driftIndex=" << driftIndex
                            << "relSSS=" << relSSS
                            << "relT=" << relT
                            << "rms_ratio=" << rms_ratio
                            << "ring=" << dbg.ring_fill << "/" << dbg.win_samp
                            << "coeffLP_Hz=" << dbg.coeff_lp_hz
                            << "fullfit=" << dbg.protect_fullfit;

                    // PERF
                    static std::vector<double> s_msRing;
                    static size_t s_msPos = 0;
                    const size_t RING_N = 200;

                    if(s_msRing.size() < RING_N) {
                        s_msRing.push_back(last_ms);
                        s_msPos = s_msRing.size() % RING_N;
                    } else {
                        s_msRing[s_msPos] = last_ms;
                        s_msPos = (s_msPos + 1) % RING_N;
                    }

                    double mean_ms = 0.0;
                    for(double v : s_msRing) mean_ms += v;
                    mean_ms = (s_msRing.empty() ? 0.0 : mean_ms / static_cast<double>(s_msRing.size()));

                    double p95_ms = 0.0;
                    if(!s_msRing.empty()) {
                        std::vector<double> tmp = s_msRing;
                        const size_t k95 = static_cast<size_t>(std::floor(0.95 * (tmp.size() - 1)));
                        std::nth_element(tmp.begin(), tmp.begin() + k95, tmp.end());
                        p95_ms = tmp[k95];
                    }

                    qDebug() << "[tSSS-RT-PERF]"
                            << "last_ms=" << last_ms
                            << "mean_ms=" << mean_ms
                            << "p95_ms=" << p95_ms;
                }
            }
            
            //Do SPHARA here
            // if(m_bSpharaActive) {
            //     //Set bad channels to zero so they do not get smeared into
            //     for(int i = 0; i < m_pFiffInfo->bads.size(); ++i) {
            //         matData.row(m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(i))).setZero();
            //     }

            //     matData = m_matSparseSpharaMult * matData;
            // }

    //        //Common average
    //        MatrixXd commonAvr = MatrixXd(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
    //        commonAvr.setZero();

    //        int nEEGCh = 0;

    //        for(int i = 0; i <m_pFiffInfo->chs.size(); ++i) {
    //            if(m_pFiffInfo->chs.at(i).ch_name.contains("EEG") && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
    //                nEEGCh++;
    //            }
    //        }

    //        for(int i = 0; i <m_pFiffInfo->chs.size(); ++i) {
    //            for(int j = 0; j < m_pFiffInfo->chs.size(); ++j) {
    //                if(m_pFiffInfo->chs.at(j).ch_name.contains("EEG") && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(j).ch_name)) {
    //                    commonAvr(i,j) = 1/nEEGCh;
    //                }
    //            }
    //        }

    //        UTILSLIB::IOUtils::write_eigen_matrix(commonAvr, "commonAvr.txt", "common vaergae matrix");

            // Restore STIM-like channels (STI014 etc.) from the raw buffer so they are not altered
            // by filtering/tSSS and also do not contaminate MEG processing.
            if(!s_stimLikeIdx.empty()) {
                for(int idx : s_stimLikeIdx) {
                    if(idx >= 0 && idx < matData.rows() && idx < matDataRaw.rows()) {
                        matData.row(idx) = matDataRaw.row(idx);
                    }
                }
            }

    m_mutex.unlock();

            //Send the data to the connected plugins and the display
            if(!isInterruptionRequested()) {
                m_pNoiseReductionOutput->measurementData()->setValue(matData);
            }
        }
    }
}

//=============================================================================================================

void NoiseReduction::updateProjection(const QList<FIFFLIB::FiffProj>& projs)
{
    //  Update the SSP projector
    if(m_pFiffInfo) {
        m_mutex.lock();
        //If a minimum of one projector is active set m_bProjActivated to true so that this model applies the ssp to the incoming data
        m_bProjActivated = false;
        for(qint32 i = 0; i < projs.size(); ++i) {
            if(projs[i].active) {
                m_bProjActivated = true;
                break;
            }
        }

        MatrixXd matProj;
        FiffProj::make_projector(projs, m_pFiffInfo->ch_names, matProj, m_pFiffInfo->bads);

        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < m_pFiffInfo->bads.size(); ++j) {
            int index = m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(j));
            if(index >= 0 && index<m_pFiffInfo->ch_names.size()) {
                matProj.col(index).setZero();
            }
        }

        // Make proj sparse
        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        tripletList.clear();
        tripletList.reserve(matProj.rows()*matProj.cols());
        for(i = 0; i < matProj.rows(); ++i) {
            for(k = 0; k < matProj.cols(); ++k) {
                if(matProj(i,k) != 0) {
                    tripletList.push_back(T(i, k, matProj(i,k)));
                }
            }
        }

        m_matSparseProjMult = SparseMatrix<double>(matProj.rows(),matProj.cols());
        if(tripletList.size() > 0)
            m_matSparseProjMult.setFromTriplets(tripletList.begin(), tripletList.end());

        //Create full multiplication matrix
        m_matSparseProjCompMult = m_matSparseProjMult * m_matSparseCompMult;

        m_matSparseFull = m_matSparseProjMult * m_matSparseCompMult;
        m_mutex.unlock();
    }
}

//=============================================================================================================

void NoiseReduction::updateCompensator(int to)
{
    // Update the compensator
    if(m_pFiffInfo) {
        if(to == 0) {
            m_bCompActivated = false;
        } else {
            m_bCompActivated = true;
        }

//        qDebug()<<"to"<<to;
//        qDebug()<<"from"<<from;
//        qDebug()<<"m_bCompActivated"<<m_bCompActivated;

        FiffCtfComp newComp;
        this->m_pFiffInfo->make_compensator(0, to, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data

        //this->m_pFiffInfo->set_current_comp(to);
        MatrixXd matComp = newComp.data->data;

        //
        // Make proj sparse
        //
        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        tripletList.clear();
        tripletList.reserve(matComp.rows()*matComp.cols());
        for(i = 0; i < matComp.rows(); ++i) {
            for(k = 0; k < matComp.cols(); ++k) {
                if(matComp(i,k) != 0) {
                    tripletList.push_back(T(i, k, matComp(i,k)));
                }
            }
        }

        m_matSparseCompMult = SparseMatrix<double>(matComp.rows(),matComp.cols());
        if(tripletList.size() > 0) {
            m_matSparseCompMult.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        //Create full multiplication matrix
        m_matSparseProjCompMult = m_matSparseProjMult * m_matSparseCompMult;

        m_matSparseFull = m_matSparseProjMult * m_matSparseCompMult;
    }
}

//=============================================================================================================

void NoiseReduction::setFilterChannelType(QString sType)
{
    m_sFilterChannelType = sType;

    m_mutex.lock();
    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_lFilterChannelList.resize(0);

    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH)/* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {

            if(m_sFilterChannelType == "All") {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols()-1] = i;
            } else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols()-1] = i;
            }
        }
    }
    m_mutex.unlock();
}

//=============================================================================================================

void NoiseReduction::setFilter(const FilterKernel& filterData)
{
    m_mutex.lock();
    m_filterKernel = filterData;

    m_iMaxFilterLength = 1;
    if(m_iMaxFilterLength < m_filterKernel.getFilterOrder()) {
        m_iMaxFilterLength = m_filterKernel.getFilterOrder();
    }
    m_mutex.unlock();
}

//=============================================================================================================

void NoiseReduction::setFilterActive(bool state)
{
    m_bFilterActivated = state;
}

//=============================================================================================================

void NoiseReduction::initSphara()
{
    //Load SPHARA matrix
    IOUtils::read_eigen_matrix(m_matSpharaVVGradLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaVVMagLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGInnerLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Inner.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGOuterLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Outer.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaEEGLoaded, QString(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/noisereduction/SPHARA/Current_SPHARA_EEG.txt"));

    //Generate indices used to create the SPHARA operators for VectorView
    m_vecIndicesFirstVV.resize(0);
    m_vecIndicesSecondVV.resize(0);

    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find gardiometers
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3012) {
            m_vecIndicesFirstVV.conservativeResize(m_vecIndicesFirstVV.rows()+1);
            m_vecIndicesFirstVV(m_vecIndicesFirstVV.rows()-1) = r;
        }

        //Find magnetometers
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3024) {
            m_vecIndicesSecondVV.conservativeResize(m_vecIndicesSecondVV.rows()+1);
            m_vecIndicesSecondVV(m_vecIndicesSecondVV.rows()-1) = r;
        }
    }

    //Generate indices used to create the SPHARA operators for babyMEG
    m_vecIndicesFirstBabyMEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find inner layer
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 7002) {
            m_vecIndicesFirstBabyMEG.conservativeResize(m_vecIndicesFirstBabyMEG.rows()+1);
            m_vecIndicesFirstBabyMEG(m_vecIndicesFirstBabyMEG.rows()-1) = r;
        }

        //TODO: Find outer layer
    }

    //Generate indices used to create the SPHARA operators for EEG layouts
    m_vecIndicesFirstEEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find EEG
        if(m_pFiffInfo->chs.at(r).kind == FIFFV_EEG_CH) {
            m_vecIndicesFirstEEG.conservativeResize(m_vecIndicesFirstEEG.rows()+1);
            m_vecIndicesFirstEEG(m_vecIndicesFirstEEG.rows()-1) = r;
        }
    }

//    qDebug()<<"NoiseReduction::createSpharaOperator - Read VectorView mag matrix "<<m_matSpharaVVMagLoaded.rows()<<m_matSpharaVVMagLoaded.cols()<<"and grad matrix"<<m_matSpharaVVGradLoaded.rows()<<m_matSpharaVVGradLoaded.cols();
//    qDebug()<<"NoiseReduction::createSpharaOperator - Read BabyMEG inner layer matrix "<<m_matSpharaBabyMEGInnerLoaded.rows()<<m_matSpharaBabyMEGInnerLoaded.cols()<<"and outer layer matrix"<<m_matSpharaBabyMEGOuterFull.rows()<<m_matSpharaBabyMEGOuterFull.cols();
}

//=============================================================================================================

void NoiseReduction::createSpharaOperator()
{
    qDebug()<<"NoiseReduction::createSpharaOperator - Creating SPHARA oerpator for"<<m_sCurrentSystem;

    m_mutex.lock();

    MatrixXd matSpharaMultFirst = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
    MatrixXd matSpharaMultSecond = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

    if(m_sCurrentSystem == "VectorView") {
        matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaVVGradLoaded, m_vecIndicesFirstVV, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 1); //GRADIOMETERS
        matSpharaMultSecond = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaVVMagLoaded, m_vecIndicesSecondVV, m_pFiffInfo->nchan, m_iNBaseFctsSecond, 0); //Magnetometers
    }

    if(m_sCurrentSystem == "BabyMEG") {
        matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaBabyMEGInnerLoaded, m_vecIndicesFirstBabyMEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    if(m_sCurrentSystem == "EEG") {
        matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaEEGLoaded, m_vecIndicesFirstEEG, m_pFiffInfo->nchan, m_iNBaseFctsFirst, 0); //InnerLayer
    }

    //Write final operator matrices to file
//    IOUtils::write_eigen_matrix(matSpharaMultFirst, QString(QCoreApplication::applicationDirPath() + "../resources/mne_scan/plugins/noisereduction/SPHARA/matSpharaMultFirst.txt"));
//    IOUtils::write_eigen_matrix(matSpharaMultSecond, QString(QCoreApplication::applicationDirPath() + "../resources/mne_scan/plugins/noisereduction/SPHARA/matSpharaMultSecond.txt"));

    //
    // Make operators sparse
    //
    qint32 nchan = this->m_pFiffInfo->nchan;
    qint32 i, k;

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(nchan);

    //First operator
    tripletList.clear();
    tripletList.reserve(matSpharaMultFirst.rows()*matSpharaMultFirst.cols());
    for(i = 0; i < matSpharaMultFirst.rows(); ++i) {
        for(k = 0; k < matSpharaMultFirst.cols(); ++k) {
            if(matSpharaMultFirst(i,k) != 0) {
                tripletList.push_back(T(i, k, matSpharaMultFirst(i,k)));
            }
        }
    }

    SparseMatrix<double> matSparseSpharaMultFirst = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

    if(tripletList.size() > 0) {
        matSparseSpharaMultFirst.setFromTriplets(tripletList.begin(), tripletList.end());
    }

    //Second operator
    tripletList.clear();
    tripletList.reserve(matSpharaMultSecond.rows()*matSpharaMultSecond.cols());

    for(i = 0; i < matSpharaMultSecond.rows(); ++i) {
        for(k = 0; k < matSpharaMultSecond.cols(); ++k) {
            if(matSpharaMultSecond(i,k) != 0) {
                tripletList.push_back(T(i, k, matSpharaMultSecond(i,k)));
            }
        }
    }

    SparseMatrix<double>matSparseSpharaMultSecond = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

    if(tripletList.size() > 0) {
        matSparseSpharaMultSecond.setFromTriplets(tripletList.begin(), tripletList.end());
    }

    //Create full multiplication matrix
    m_matSparseSpharaMult = matSparseSpharaMultFirst * matSparseSpharaMultSecond;

    m_matSparseFull = m_matSparseProjMult * m_matSparseCompMult;

    m_mutex.unlock();
}

//=============================================================================================================

QString NoiseReduction::getBuildInfo()
{
    return QString(NOISEREDUCTIONPLUGIN::buildDateTime()) + QString(" - ")  + QString(NOISEREDUCTIONPLUGIN::buildHash());
}

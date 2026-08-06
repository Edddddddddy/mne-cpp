//=============================================================================================================
/**
 * @file     adaptivedenoising.h
 * @brief    Adaptive Denoising mne_scan algorithm plugin.
 */

#ifndef ADAPTIVEDENOISING_H
#define ADAPTIVEDENOISING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoising_global.h"

#include <scShared/Plugins/abstractalgorithm.h>
#include <scMeas/measurement.h>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================
/**
 * Concrete mne_scan adapter for causal reference-channel adaptive denoising.
 * Acquisition only enters the preallocated queue; FIFF mapping, numerical ownership and output all stay on the
 * processing worker.
 */
class ADAPTIVEDENOISINGSHARED_EXPORT AdaptiveDenoising final
    : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "adaptivedenoising.json")
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

public:
    AdaptiveDenoising();
    ~AdaptiveDenoising() override;

    QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const override;
    void init() override;
    void unload() override;
    bool start() override;
    bool stop() override;
    SCSHAREDLIB::AbstractPlugin::PluginType getType() const override;
    QString getName() const override;
    QWidget* setupWidget() override;
    QString getBuildInfo() override;

    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

protected:
    void run() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISING_H

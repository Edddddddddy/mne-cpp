//=============================================================================================================
/**
 * @file     adaptivedenoisingtracewidget.h
 * @brief    Lightweight waveform and RMS painter for adaptive denoising.
 */

#ifndef ADAPTIVEDENOISINGTRACEWIDGET_ADAPTIVEDENOISINGPLUGIN_H
#define ADAPTIVEDENOISINGTRACEWIDGET_ADAPTIVEDENOISINGPLUGIN_H

//=============================================================================================================

#include "adaptivedenoisingvisualizationmodel.h"

#include <QtWidgets/QWidget>

//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

class AdaptiveDenoisingTraceWidget final : public QWidget
{
public:
    explicit AdaptiveDenoisingTraceWidget(QWidget* parent = nullptr);

    void setSnapshot(const AdaptiveDenoisingVisualizationSnapshot& snapshot);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    AdaptiveDenoisingVisualizationSnapshot m_snapshot;
};

//=============================================================================================================

} // NAMESPACE

#endif // ADAPTIVEDENOISINGTRACEWIDGET_ADAPTIVEDENOISINGPLUGIN_H


//=============================================================================================================
/**
 * @file     adaptivedenoisingtracewidget.cpp
 * @brief    Lightweight waveform and RMS painter for adaptive denoising.
 */

//=============================================================================================================

#include "adaptivedenoisingtracewidget.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

//=============================================================================================================

using namespace ADAPTIVEDENOISINGPLUGIN;

namespace
{

const QColor kBackgroundColor(QStringLiteral("#101820"));
const QColor kPanelColor(QStringLiteral("#17232d"));
const QColor kGridColor(QStringLiteral("#32424f"));
const QColor kTextColor(QStringLiteral("#d9e2e8"));
const QColor kRawColor(QStringLiteral("#4fc3f7"));
const QColor kDenoisedColor(QStringLiteral("#66bb6a"));
const QColor kNoiseColor(QStringLiteral("#ffb74d"));

template<std::size_t Capacity>
void includeFiniteRange(const std::array<double, Capacity>& values,
                        Eigen::Index count,
                        double& minimum,
                        double& maximum)
{
    for(Eigen::Index index = 0; index < count; ++index) {
        const double value = values[static_cast<std::size_t>(index)];
        if(std::isfinite(value)) {
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
    }
}

template<std::size_t Capacity>
void drawSeries(QPainter& painter,
                const QRectF& plot,
                const std::array<double, Capacity>& values,
                Eigen::Index count,
                double minimum,
                double maximum,
                const QColor& color)
{
    if(count <= 0 || !(maximum > minimum)) {
        return;
    }

    QPainterPath path;
    bool pathStarted = false;
    for(Eigen::Index index = 0; index < count; ++index) {
        const double value = values[static_cast<std::size_t>(index)];
        if(!std::isfinite(value)) {
            pathStarted = false;
            continue;
        }

        const double normalizedX = count <= 1
            ? 0.5
            : static_cast<double>(index) / static_cast<double>(count - 1);
        const double normalizedY = (value - minimum) / (maximum - minimum);
        const QPointF point(plot.left() + normalizedX * plot.width(),
                            plot.bottom() - normalizedY * plot.height());
        if(!pathStarted) {
            path.moveTo(point);
            pathStarted = true;
        } else {
            path.lineTo(point);
        }
    }

    painter.setPen(QPen(color, 2.0));
    painter.drawPath(path);
}

void drawPanelFrame(QPainter& painter, const QRectF& panel, const QString& title)
{
    painter.fillRect(panel, kPanelColor);
    painter.setPen(QPen(kGridColor, 1.0));
    painter.drawRect(panel);
    for(int division = 1; division < 4; ++division) {
        const double y = panel.top() + panel.height() * static_cast<double>(division) / 4.0;
        painter.drawLine(QPointF(panel.left(), y), QPointF(panel.right(), y));
    }
    painter.setPen(kTextColor);
    painter.drawText(panel.adjusted(8.0, 4.0, -8.0, -4.0),
                     Qt::AlignLeft | Qt::AlignTop,
                     title);
}

void drawLegend(QPainter& painter, const QRectF& panel)
{
    const std::array<QString, 3> labels{
        QStringLiteral("Raw / Input RMS"),
        QStringLiteral("Denoised / Output RMS"),
        QStringLiteral("Estimated noise")};
    const std::array<QColor, 3> colors{kRawColor, kDenoisedColor, kNoiseColor};

    double x = panel.left() + 180.0;
    const double y = panel.top() + 14.0;
    for(std::size_t index = 0u; index < labels.size(); ++index) {
        painter.setPen(QPen(colors[index], 3.0));
        painter.drawLine(QPointF(x, y), QPointF(x + 18.0, y));
        painter.setPen(kTextColor);
        painter.drawText(QPointF(x + 24.0, y + 5.0), labels[index]);
        x += 155.0;
    }
}

} // namespace

//=============================================================================================================

AdaptiveDenoisingTraceWidget::AdaptiveDenoisingTraceWidget(QWidget* parent)
: QWidget(parent)
{
    setObjectName(QStringLiteral("visualizationTraceWidget"));
    setMinimumSize(640, 360);
}

//=============================================================================================================

void AdaptiveDenoisingTraceWidget::setSnapshot(
    const AdaptiveDenoisingVisualizationSnapshot& snapshot)
{
    m_snapshot = snapshot;
    update();
}

//=============================================================================================================

void AdaptiveDenoisingTraceWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), kBackgroundColor);

    const QRectF content = QRectF(rect()).adjusted(12.0, 12.0, -12.0, -12.0);
    const double gap = 12.0;
    const double waveformHeight = (content.height() - gap) * 0.62;
    const QRectF waveformPanel(content.left(), content.top(), content.width(), waveformHeight);
    const QRectF rmsPanel(content.left(), waveformPanel.bottom() + gap,
                          content.width(), content.bottom() - waveformPanel.bottom() - gap);

    drawPanelFrame(painter, waveformPanel, QStringLiteral("Selected target waveform"));
    drawPanelFrame(painter, rmsPanel, QStringLiteral("Target-set RMS history"));
    drawLegend(painter, waveformPanel);

    if(m_snapshot.traceSampleCount <= 0) {
        painter.setPen(kTextColor);
        painter.drawText(waveformPanel, Qt::AlignCenter, QStringLiteral("Waiting for MEG data"));
        return;
    }

    const QRectF waveformPlot = waveformPanel.adjusted(10.0, 28.0, -10.0, -10.0);
    double waveformMinimum = std::numeric_limits<double>::infinity();
    double waveformMaximum = -std::numeric_limits<double>::infinity();
    includeFiniteRange(m_snapshot.raw, m_snapshot.traceSampleCount,
                       waveformMinimum, waveformMaximum);
    includeFiniteRange(m_snapshot.denoised, m_snapshot.traceSampleCount,
                       waveformMinimum, waveformMaximum);
    includeFiniteRange(m_snapshot.estimatedNoise, m_snapshot.traceSampleCount,
                       waveformMinimum, waveformMaximum);
    if(!(waveformMaximum > waveformMinimum)) {
        waveformMinimum -= 1.0;
        waveformMaximum += 1.0;
    }

    drawSeries(painter, waveformPlot, m_snapshot.raw, m_snapshot.traceSampleCount,
               waveformMinimum, waveformMaximum, kRawColor);
    drawSeries(painter, waveformPlot, m_snapshot.denoised, m_snapshot.traceSampleCount,
               waveformMinimum, waveformMaximum, kDenoisedColor);
    drawSeries(painter, waveformPlot, m_snapshot.estimatedNoise, m_snapshot.traceSampleCount,
               waveformMinimum, waveformMaximum, kNoiseColor);

    const QRectF rmsPlot = rmsPanel.adjusted(10.0, 26.0, -10.0, -10.0);
    double rmsMinimum = 0.0;
    double rmsMaximum = 0.0;
    includeFiniteRange(m_snapshot.inputRmsHistory, m_snapshot.rmsHistoryCount,
                       rmsMinimum, rmsMaximum);
    includeFiniteRange(m_snapshot.outputRmsHistory, m_snapshot.rmsHistoryCount,
                       rmsMinimum, rmsMaximum);
    includeFiniteRange(m_snapshot.estimatedNoiseRmsHistory, m_snapshot.rmsHistoryCount,
                       rmsMinimum, rmsMaximum);
    if(!(rmsMaximum > rmsMinimum)) {
        rmsMaximum = rmsMinimum + 1.0;
    }

    drawSeries(painter, rmsPlot, m_snapshot.inputRmsHistory, m_snapshot.rmsHistoryCount,
               rmsMinimum, rmsMaximum, kRawColor);
    drawSeries(painter, rmsPlot, m_snapshot.outputRmsHistory, m_snapshot.rmsHistoryCount,
               rmsMinimum, rmsMaximum, kDenoisedColor);
    drawSeries(painter, rmsPlot, m_snapshot.estimatedNoiseRmsHistory,
               m_snapshot.rmsHistoryCount, rmsMinimum, rmsMaximum, kNoiseColor);
}


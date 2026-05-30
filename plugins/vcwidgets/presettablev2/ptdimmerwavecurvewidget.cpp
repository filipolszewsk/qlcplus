/*
  Dimmer wave curve preview widget.
*/

#include "ptdimmerwavecurvewidget.h"

#include <QPainter>
#include <QPaintEvent>

PTDimmerWaveCurveWidget::PTDimmerWaveCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(100);
    setAutoFillBackground(true);
}

void PTDimmerWaveCurveWidget::setParams(const PTDimmerWaveParams& params)
{
    m_params = params;
    update();
}

void PTDimmerWaveCurveWidget::setCycleDurationMs(quint32 ms)
{
    m_cycleDurationMs = qMax(quint32(20), ms);
    update();
}

void PTDimmerWaveCurveWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect().adjusted(8, 8, -8, -20);
    p.fillRect(r, palette().color(QPalette::Base).darker(110));

    const int waveW = qBound(0, m_params.waveWidth, 360);
    const int packetPx = waveW > 0 ? qMax(1, r.width() * waveW / 360) : 0;
    if (packetPx > 0)
    {
        QRect packetRect(r.left(), r.top(), packetPx, r.height());
        p.fillRect(packetRect, palette().color(QPalette::Highlight).lighter(160));
    }

    // Fade zone guides inside packet
    if (packetPx > 0 && m_params.waveShape != 1)
    {
        const float fadeIn = float(qBound(0, m_params.waveFadeIn, 100)) / 100.0f;
        const float fadeOut = float(qBound(0, m_params.waveFadeOut, 100)) / 100.0f;
        p.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DashLine));
        if (fadeIn > 0.0f)
        {
            const int x = r.left() + int(packetPx * fadeIn);
            p.drawLine(x, r.top(), x, r.bottom());
        }
        if (fadeOut > 0.0f)
        {
            const int x = r.left() + packetPx - int(packetPx * fadeOut);
            p.drawLine(x, r.top(), x, r.bottom());
        }
    }

    QPolygonF poly;
    const int samples = qMax(64, r.width());
    for (int i = 0; i <= samples; ++i)
    {
        const float deg = 360.0f * float(i) / float(samples);
        const float dimmer01 = PTDimmerWaveEngine::sampleDimmerCycle01(deg, m_params);
        const qreal x = r.left() + (r.width() * i) / samples;
        const qreal y = r.bottom() - dimmer01 * r.height();
        poly.append(QPointF(x, y));
    }

    p.setPen(QPen(palette().color(QPalette::BrightText), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(poly);

    p.setPen(palette().color(QPalette::Text));
    p.drawText(QRect(8, height() - 18, width() - 16, 16), Qt::AlignLeft,
               tr("X: 0–360° (1 cycle = %1 ms)   Y: dimmer   Width: %2°")
                       .arg(m_cycleDurationMs)
                       .arg(waveW));
}

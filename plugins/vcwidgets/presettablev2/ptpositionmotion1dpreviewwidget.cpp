/*
  ptpositionmotion1dpreviewwidget.cpp
*/

#include "ptpositionmotion1dpreviewwidget.h"
#include "ptpositionfxengine.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PTPositionMotion1DPreviewWidget::PTPositionMotion1DPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(90);
    setAutoFillBackground(true);
    setToolTip(tr("Double-click to edit wave curve (same as row morph)"));
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        const double ms = qMax(200.0, double(m_cycleMs));
        m_animPhase01 += double(m_timer.interval()) / ms;
        if (m_animPhase01 >= 1.0)
            m_animPhase01 -= 1.0;
        update();
    });
}

bool PTPositionMotion1DPreviewWidget::is1DMotion(PTPositionMotion motion)
{
    return PTPositionFxEngine::motionIs1D(motion);
}

void PTPositionMotion1DPreviewWidget::stopAnimation()
{
    m_timer.stop();
    m_animPhase01 = 0;
}

void PTPositionMotion1DPreviewWidget::setMotionPreview(PTPositionMotion motion, qreal panSizeDeg,
                                                         qreal tiltSizeDeg,
                                                         const QVector<PhaseMarker>& markers,
                                                         quint32 cycleMs)
{
    m_motion = motion;
    m_usePresetMotion = false;
    m_dualAxisPreview = false;
    m_panSize = panSizeDeg;
    m_tiltSize = tiltSizeDeg;
    m_markers = markers;
    m_cycleMs = qMax(quint32(200), cycleMs);
    m_waveParams = PTDimmerWaveParams();
    m_waveParams.waveWidth = 360;
    stopAnimation();
    if (m_motion != PTPositionMotion::Off)
    {
        m_timer.setInterval(qMax(16, int(m_cycleMs) / 60));
        m_timer.start();
    }
    update();
}

void PTPositionMotion1DPreviewWidget::setMotionPreviewFromPreset(const PTTransitionPreset& preset,
                                                                 const PTDimmerWaveParams& waveParams,
                                                                 const QVector<PhaseMarker>& markers,
                                                                 quint32 cycleMs)
{
    m_preset = preset;
    m_waveParams = waveParams;
    m_motion = PTPositionMotion(preset.positionMotion);
    m_usePresetMotion = true;
    m_dualAxisPreview = false;
    m_panSize = 1.0;
    m_tiltSize = 1.0;
    m_markers = markers;
    m_cycleMs = qMax(quint32(200), cycleMs);
    stopAnimation();
    if (m_motion != PTPositionMotion::Off)
    {
        m_timer.setInterval(qMax(16, int(m_cycleMs) / 60));
        m_timer.start();
    }
    update();
}

void PTPositionMotion1DPreviewWidget::setMotionPreview2DFromPreset(
        const PTTransitionPreset& preset,
        const QVector<PhaseMarker>& markers,
        quint32 cycleMs)
{
    m_preset = preset;
    m_waveParams = PTDimmerWaveParams();
    m_waveParams.waveWidth = qBound(1, preset.waveWidth, 360);
    m_motion = PTPositionMotion(preset.positionMotion);
    m_usePresetMotion = true;
    m_dualAxisPreview = true;
    m_panSize = 1.0;
    m_tiltSize = 1.0;
    m_markers = markers;
    m_cycleMs = qMax(quint32(200), cycleMs);
    stopAnimation();
    if (m_motion != PTPositionMotion::Off)
    {
        m_timer.setInterval(qMax(16, int(m_cycleMs) / 60));
        m_timer.start();
    }
    update();
}

void PTPositionMotion1DPreviewWidget::clear()
{
    m_motion = PTPositionMotion::Off;
    m_usePresetMotion = false;
    m_dualAxisPreview = false;
    m_panSize = 0;
    m_tiltSize = 0;
    m_markers.clear();
    stopAnimation();
    update();
}

void PTPositionMotion1DPreviewWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_motion != PTPositionMotion::Off)
        emit motionCurveEditRequested();
    QWidget::mouseDoubleClickEvent(event);
}

QRectF PTPositionMotion1DPreviewWidget::plotRect() const
{
    return rect().adjusted(28, 18, -8, -16);
}

QPointF PTPositionMotion1DPreviewWidget::mapSample(double cycle01, qreal normValue,
                                                     const QRectF& plot) const
{
    const qreal x = plot.left() + qBound(0.0, cycle01, 1.0) * plot.width();
    const qreal y = plot.bottom() - (normValue + 1.0) * 0.5 * plot.height();
    return QPointF(x, y);
}

qreal PTPositionMotion1DPreviewWidget::sampleAtCycle01(double cycle01) const
{
    if (m_dualAxisPreview)
        return sampleAxisAtCycle01(cycle01, false);

    if (!m_usePresetMotion)
    {
        const double phaseRadians = cycle01 * 2.0 * M_PI;
        qreal panOff = 0;
        qreal tiltOff = 0;
        const auto shape = PTPositionFxEngine::shapeFromPositionMotion(m_motion);
        PTPositionFxEngine::relativeOffset(shape, phaseRadians, m_panSize, m_tiltSize,
                                           panOff, tiltOff);
        const bool useTilt = m_motion == PTPositionMotion::Tilt1D;
        const qreal value = useTilt ? tiltOff : panOff;
        const qreal amp = qMax(qreal(0.001), useTilt ? m_tiltSize : m_panSize);
        return value / amp;
    }

    const float deg = float(cycle01 * 360.0);
    return qreal(PTPositionFxEngine::sampleMotionAtCycleDeg(deg, m_preset, m_waveParams));
}

qreal PTPositionMotion1DPreviewWidget::sampleAxisAtCycle01(double cycle01, bool tiltAxis) const
{
    if (!m_dualAxisPreview)
        return sampleAtCycle01(cycle01);

    const double start01 = qBound(0, m_preset.startOffset, 359) / 360.0;
    const double width01 = qBound(1, m_preset.waveWidth, 360) / 360.0;
    double window01 = cycle01 - start01;
    window01 -= qFloor(window01);
    const double motionPhase01 = window01 > width01 ? 1.0 : window01 / width01;
    const double phaseRadians = motionPhase01 * 2.0 * M_PI;
    qreal panOff = 0;
    qreal tiltOff = 0;
    PTPositionFxEngine::relativeOffsetForPreset(m_preset, phaseRadians, 1.0, 1.0,
                                                panOff, tiltOff);
    return tiltAxis ? tiltOff : panOff;
}

void PTPositionMotion1DPreviewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.fillRect(rect(), palette().base());

    const QRectF plot = plotRect();
    p.setPen(QPen(palette().mid(), 1));
    p.drawRect(plot);

    const qreal zeroY = mapSample(0.0, 0.0, plot).y();
    p.setPen(QPen(palette().mid(), 1, Qt::DashLine));
    p.drawLine(plot.left(), zeroY, plot.right(), zeroY);

    p.setPen(palette().text().color());
    const bool useTilt = m_motion == PTPositionMotion::Tilt1D
            || m_motion == PTPositionMotion::CustomTilt1D;
    const QString axisLabel = useTilt ? tr("Tilt") : tr("Pan");
    QString shapeLabel;
    if (m_usePresetMotion)
    {
        if (m_preset.customCurveEnabled)
            shapeLabel = tr("Custom");
        else
        {
            switch (m_preset.waveShape)
            {
                case 1: shapeLabel = tr("[Legacy] Square"); break;
                case 2: shapeLabel = tr("Triangle"); break;
                default: shapeLabel = tr("Sine"); break;
            }
        }
    }
    p.drawText(QRectF(rect().left(), 2, rect().width(), 14),
               Qt::AlignLeft | Qt::AlignVCenter,
               m_dualAxisPreview
                       ? tr("Pan + Tilt offset · orbit %1°").arg(m_preset.waveWidth)
                       : m_usePresetMotion
                       ? tr("%1 offset · %2 · orbit %3°")
                               .arg(axisLabel, shapeLabel).arg(m_preset.waveWidth)
                       : tr("%1 offset vs time (unit shape)").arg(axisLabel));

    if (m_usePresetMotion)
    {
        const int waveW = qBound(1, m_preset.waveWidth, 360);
        const int startOff = qBound(0, m_preset.startOffset, 359);
        const qreal plotW = plot.width();
        const qreal packetW = plotW * qreal(waveW) / 360.0;
        const qreal packetLeft = plot.left() + plotW * qreal(startOff) / 360.0;
        const QColor idleTint(0, 0, 0, 28);

        if (waveW < 360)
        {
            if (startOff > 0)
            {
                QRectF preIdle(plot.left(), plot.top(), packetLeft - plot.left(), plot.height());
                if (preIdle.width() > 0)
                    p.fillRect(preIdle, idleTint);
            }
            const qreal packetRight = packetLeft + packetW;
            if (packetRight < plot.right())
            {
                QRectF postIdle(packetRight, plot.top(), plot.right() - packetRight, plot.height());
                if (postIdle.width() > 0)
                    p.fillRect(postIdle, idleTint);
            }
            if (packetRight > plot.right())
            {
                const qreal wrappedW = packetRight - plot.right();
                const qreal gapW = packetLeft - plot.left();
                if (gapW > 0)
                    p.fillRect(QRectF(plot.left(), plot.top(), gapW, plot.height()), idleTint);
                Q_UNUSED(wrappedW)
            }
        }

        if (packetW > 0)
        {
            if (packetLeft + packetW <= plot.right())
            {
                p.fillRect(QRectF(packetLeft, plot.top(), packetW, plot.height()),
                           palette().color(QPalette::Highlight).lighter(175));
            }
            else
            {
                const qreal firstW = plot.right() - packetLeft;
                p.fillRect(QRectF(packetLeft, plot.top(), firstW, plot.height()),
                           palette().color(QPalette::Highlight).lighter(175));
                const qreal secondW = packetW - firstW;
                p.fillRect(QRectF(plot.left(), plot.top(), secondW, plot.height()),
                           palette().color(QPalette::Highlight).lighter(175));
            }

            if (m_preset.waveShape != 1)
            {
                const float fadeIn = float(qBound(0, m_preset.waveFadeIn, 100)) / 100.0f;
                const float fadeOut = float(qBound(0, m_preset.waveFadeOut, 100)) / 100.0f;
                p.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DashLine));
                if (fadeIn > 0.0f)
                {
                    const qreal x = packetLeft + packetW * fadeIn;
                    if (x <= plot.right())
                        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
                }
                if (fadeOut > 0.0f)
                {
                    const qreal x = packetLeft + packetW - packetW * fadeOut;
                    if (x >= plot.left())
                        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
                }
            }
        }
    }

    p.setPen(palette().mid().color());
    QFont small = p.font();
    small.setPointSize(qMax(7, small.pointSize() - 1));
    p.setFont(small);
    p.drawText(QRectF(rect().left(), plot.top(), 24, plot.height() / 2),
               Qt::AlignRight | Qt::AlignTop, tr("max"));
    p.drawText(QRectF(rect().left(), plot.center().y() - 6, 24, 12),
               Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("0"));
    p.drawText(QRectF(rect().left(), plot.bottom() - plot.height() / 2, 24, plot.height() / 2),
               Qt::AlignRight | Qt::AlignBottom, tr("min"));
    p.drawText(QRectF(plot.left(), plot.bottom() + 2, plot.width() / 2, 12),
               Qt::AlignLeft | Qt::AlignTop, QStringLiteral("0°"));
    p.drawText(QRectF(plot.center().x(), plot.bottom() + 2, plot.width() / 2, 12),
               Qt::AlignRight | Qt::AlignTop, QStringLiteral("360°"));

    if (m_motion == PTPositionMotion::Off)
    {
        p.setPen(palette().mid().color());
        p.drawText(plot, Qt::AlignCenter, tr("Motion off"));
        return;
    }

    auto buildPath = [&](bool tiltAxis) {
        QPainterPath path;
        for (int i = 0; i <= 256; ++i)
        {
            const double cycle01 = double(i) / 256.0;
            const qreal norm = m_dualAxisPreview
                    ? sampleAxisAtCycle01(cycle01, tiltAxis)
                    : sampleAtCycle01(cycle01);
            const QPointF pt = mapSample(cycle01, norm, plot);
            if (i == 0)
                path.moveTo(pt);
            else
                path.lineTo(pt);
        }
        return path;
    };

    p.setBrush(Qt::NoBrush);
    if (m_dualAxisPreview)
    {
        const QColor panColor(80, 160, 255);
        const QColor tiltColor(255, 110, 180);
        p.setPen(QPen(panColor, 2));
        p.drawPath(buildPath(false));
        p.setPen(QPen(tiltColor, 2));
        p.drawPath(buildPath(true));

        QFont smallLegend = p.font();
        smallLegend.setPointSize(qMax(7, smallLegend.pointSize() - 1));
        p.setFont(smallLegend);
        p.setPen(panColor);
        p.drawText(QRectF(plot.right() - 82, plot.top() + 2, 38, 14),
                   Qt::AlignLeft | Qt::AlignVCenter, tr("Pan"));
        p.setPen(tiltColor);
        p.drawText(QRectF(plot.right() - 42, plot.top() + 2, 38, 14),
                   Qt::AlignLeft | Qt::AlignVCenter, tr("Tilt"));

        for (const PhaseMarker& marker : m_markers)
        {
            const double cycle01 = m_animPhase01 + marker.phaseOffset01;
            const double wrapped = cycle01 - qFloor(cycle01);
            const QPointF panPt = mapSample(wrapped, sampleAxisAtCycle01(wrapped, false), plot);
            const QPointF tiltPt = mapSample(wrapped, sampleAxisAtCycle01(wrapped, true), plot);
            p.setPen(QPen(panColor.darker(130), 1));
            p.setBrush(panColor);
            p.drawEllipse(panPt, 4, 4);
            p.setPen(QPen(tiltColor.darker(130), 1));
            p.setBrush(tiltColor);
            p.drawEllipse(tiltPt, 4, 4);
        }
    }
    else
    {
        p.setPen(QPen(QColor(80, 160, 255), 2));
        p.drawPath(buildPath(false));

        for (const PhaseMarker& marker : m_markers)
        {
            const double cycle01 = m_animPhase01 + marker.phaseOffset01;
            const double wrapped = cycle01 - qFloor(cycle01);
            const qreal norm = sampleAtCycle01(wrapped);
            const QPointF pt = mapSample(wrapped, norm, plot);
            p.setPen(QPen(marker.color.darker(120), 1));
            p.setBrush(marker.color);
            p.drawEllipse(pt, 5, 5);
        }
    }
}

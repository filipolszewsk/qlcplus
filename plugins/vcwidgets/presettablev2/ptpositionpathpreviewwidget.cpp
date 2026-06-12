/*
  ptpositionpathpreviewwidget.cpp
*/

#include "ptpositionpathpreviewwidget.h"
#include "ptpositionfxengine.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PTPositionPathPreviewWidget::PTPositionPathPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(90);
    setAutoFillBackground(true);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        const double ms = qMax(200.0, double(m_cycleMs));
        m_animPhase01 += double(m_timer.interval()) / ms;
        if (m_animPhase01 >= 1.0)
            m_animPhase01 -= 1.0;
        update();
    });
}

void PTPositionPathPreviewWidget::stopAnimation()
{
    m_timer.stop();
    m_animPhase01 = 0;
}

void PTPositionPathPreviewWidget::setOrbitPreview(PTPositionMotion motion, qreal panSizeDeg,
                                                  qreal tiltSizeDeg,
                                                  const QVector<OrbitBall>& balls,
                                                  quint32 cycleMs)
{
    m_motion = motion;
    m_usePresetMotion = false;
    m_panSize = panSizeDeg;
    m_tiltSize = tiltSizeDeg;
    m_balls = balls;
    m_cycleMs = qMax(quint32(200), cycleMs);
    stopAnimation();
    if (m_motion != PTPositionMotion::Off)
    {
        m_timer.setInterval(qMax(16, int(m_cycleMs) / 60));
        m_timer.start();
    }
    update();
}

void PTPositionPathPreviewWidget::setOrbitPreviewFromPreset(const PTTransitionPreset& preset,
                                                            const QVector<OrbitBall>& balls,
                                                            quint32 cycleMs)
{
    m_preset = preset;
    m_motion = PTPositionMotion(preset.positionMotion);
    m_usePresetMotion = true;
    m_panSize = 1.0;
    m_tiltSize = 1.0;
    m_balls = balls;
    m_cycleMs = qMax(quint32(200), cycleMs);
    stopAnimation();
    if (m_motion != PTPositionMotion::Off)
    {
        m_timer.setInterval(qMax(16, int(m_cycleMs) / 60));
        m_timer.start();
    }
    update();
}

void PTPositionPathPreviewWidget::clear()
{
    m_motion = PTPositionMotion::Off;
    m_usePresetMotion = false;
    m_panSize = 0;
    m_tiltSize = 0;
    m_balls.clear();
    stopAnimation();
    update();
}

QRectF PTPositionPathPreviewWidget::plotRect() const
{
    return rect().adjusted(8, 18, -8, -8);
}

QPointF PTPositionPathPreviewWidget::mapRelative(qreal panOffDeg, qreal tiltOffDeg,
                                                 const QPointF& center, qreal scale) const
{
    return QPointF(center.x() + panOffDeg * scale, center.y() - tiltOffDeg * scale);
}

void PTPositionPathPreviewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.fillRect(rect(), palette().base());

    const QRectF plot = plotRect();
    p.setPen(QPen(palette().mid(), 1));
    p.drawRect(plot);
    p.drawLine(plot.center().x(), plot.top(), plot.center().x(), plot.bottom());
    p.drawLine(plot.left(), plot.center().y(), plot.right(), plot.center().y());

    p.setPen(palette().text().color());
    p.drawText(QRectF(rect().left(), 2, rect().width(), 14),
               Qt::AlignLeft | Qt::AlignVCenter,
               tr("Position motion (unit shape — global size at runtime)"));

    if (m_motion == PTPositionMotion::Off)
    {
        p.setPen(palette().mid().color());
        p.drawText(plot, Qt::AlignCenter, tr("Motion off"));
        return;
    }

    const QPointF center = plot.center();
    const qreal maxPan = qMax(qreal(1), m_panSize);
    const qreal maxTilt = qMax(qreal(1), m_tiltSize);
    const qreal scale = qMin(plot.width() / (2.0 * maxPan), plot.height() / (2.0 * maxTilt))
            * 0.9;

    QPainterPath path;
    for (int i = 0; i <= 128; ++i)
    {
        const double phase = double(i) / 128.0 * 2.0 * M_PI;
        qreal panOff = 0;
        qreal tiltOff = 0;
        if (m_usePresetMotion)
        {
            PTPositionFxEngine::relativeOffsetForPreset(m_preset, phase,
                                                        m_panSize, m_tiltSize, panOff, tiltOff);
        }
        else
        {
            const auto shape = PTPositionFxEngine::shapeFromPositionMotion(m_motion);
            PTPositionFxEngine::relativeOffset(shape, phase, m_panSize, m_tiltSize, panOff, tiltOff);
        }
        const QPointF pt = mapRelative(panOff, tiltOff, center, scale);
        if (i == 0)
            path.moveTo(pt);
        else
            path.lineTo(pt);
    }

    p.setPen(QPen(QColor(80, 160, 255), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    p.setPen(QPen(QColor(120, 220, 120), 6));
    p.drawPoint(center);

    for (const OrbitBall& ball : m_balls)
    {
        const double phase = (m_animPhase01 + ball.phaseOffset01) * 2.0 * M_PI;
        qreal panOff = 0;
        qreal tiltOff = 0;
        if (m_usePresetMotion)
        {
            PTPositionFxEngine::relativeOffsetForPreset(m_preset, phase,
                                                        m_panSize, m_tiltSize, panOff, tiltOff);
        }
        else
        {
            const auto shape = PTPositionFxEngine::shapeFromPositionMotion(m_motion);
            PTPositionFxEngine::relativeOffset(shape, phase, m_panSize, m_tiltSize, panOff, tiltOff);
        }
        const QPointF pt = mapRelative(panOff, tiltOff, center, scale);
        p.setPen(QPen(ball.color.darker(120), 1));
        p.setBrush(ball.color);
        p.drawEllipse(pt, 5, 5);
    }
}

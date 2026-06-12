/*
  ptpositionmotion1dpreviewwidget.cpp
*/

#include "ptpositionmotion1dpreviewwidget.h"
#include "ptpositionfxengine.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PTPositionMotion1DPreviewWidget::PTPositionMotion1DPreviewWidget(QWidget* parent)
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

bool PTPositionMotion1DPreviewWidget::is1DMotion(PTPositionMotion motion)
{
    return motion == PTPositionMotion::Pan1D
            || motion == PTPositionMotion::Tilt1D
            || motion == PTPositionMotion::CustomPan1D
            || motion == PTPositionMotion::CustomTilt1D;
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
    m_panSize = panSizeDeg;
    m_tiltSize = tiltSizeDeg;
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

void PTPositionMotion1DPreviewWidget::setMotionPreviewFromPreset(const PTTransitionPreset& preset,
                                                                 const QVector<PhaseMarker>& markers,
                                                                 quint32 cycleMs)
{
    m_preset = preset;
    m_motion = PTPositionMotion(preset.positionMotion);
    m_usePresetMotion = true;
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
    m_panSize = 0;
    m_tiltSize = 0;
    m_markers.clear();
    stopAnimation();
    update();
}

QRectF PTPositionMotion1DPreviewWidget::plotRect() const
{
    return rect().adjusted(28, 18, -8, -16);
}

qreal PTPositionMotion1DPreviewWidget::sampleNormalizedOffset(double phaseRadians) const
{
    qreal panOff = 0;
    qreal tiltOff = 0;
    if (m_usePresetMotion)
    {
        PTPositionFxEngine::relativeOffsetForPreset(m_preset, phaseRadians,
                                                    m_panSize, m_tiltSize, panOff, tiltOff);
    }
    else
    {
        const auto shape = PTPositionFxEngine::shapeFromPositionMotion(m_motion);
        PTPositionFxEngine::relativeOffset(shape, phaseRadians, m_panSize, m_tiltSize,
                                           panOff, tiltOff);
    }

    const bool useTilt = m_motion == PTPositionMotion::Tilt1D
            || m_motion == PTPositionMotion::CustomTilt1D;
    const qreal value = useTilt ? tiltOff : panOff;
    const qreal amp = qMax(qreal(0.001), useTilt ? m_tiltSize : m_panSize);
    return value / amp;
}

QPointF PTPositionMotion1DPreviewWidget::mapSample(double phase01, qreal normValue,
                                                   const QRectF& plot) const
{
    const qreal x = plot.left() + qBound(0.0, phase01, 1.0) * plot.width();
    const qreal y = plot.bottom() - (normValue + 1.0) * 0.5 * plot.height();
    return QPointF(x, y);
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
    p.drawText(QRectF(rect().left(), 2, rect().width(), 14),
               Qt::AlignLeft | Qt::AlignVCenter,
               tr("%1 offset vs time (unit shape)").arg(axisLabel));

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
               Qt::AlignLeft | Qt::AlignTop, QStringLiteral("0%"));
    p.drawText(QRectF(plot.center().x(), plot.bottom() + 2, plot.width() / 2, 12),
               Qt::AlignRight | Qt::AlignTop, QStringLiteral("100%"));

    if (m_motion == PTPositionMotion::Off)
    {
        p.setPen(palette().mid().color());
        p.drawText(plot, Qt::AlignCenter, tr("Motion off"));
        return;
    }

    QPainterPath path;
    for (int i = 0; i <= 128; ++i)
    {
        const double phase01 = double(i) / 128.0;
        const double phaseRadians = phase01 * 2.0 * M_PI;
        const qreal norm = sampleNormalizedOffset(phaseRadians);
        const QPointF pt = mapSample(phase01, norm, plot);
        if (i == 0)
            path.moveTo(pt);
        else
            path.lineTo(pt);
    }

    p.setPen(QPen(QColor(80, 160, 255), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    for (const PhaseMarker& marker : m_markers)
    {
        const double phase01 = m_animPhase01 + marker.phaseOffset01;
        const double wrapped = phase01 - qFloor(phase01);
        const qreal norm = sampleNormalizedOffset(wrapped * 2.0 * M_PI);
        const QPointF pt = mapSample(wrapped, norm, plot);
        p.setPen(QPen(marker.color.darker(120), 1));
        p.setBrush(marker.color);
        p.drawEllipse(pt, 5, 5);
    }
}

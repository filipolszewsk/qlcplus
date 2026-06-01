/*
  Dimmer wave curve preview widget.
*/

#include "ptdimmerwavecurvewidget.h"

#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <algorithm>

namespace {

QVector<PTCustomCurvePoint> defaultCustomCurve()
{
    QVector<PTCustomCurvePoint> pts;
    PTCustomCurvePoint a;
    a.xDeg = 0.0; a.yValue = 0.0;
    a.leftHandleXDeg = 0.0; a.leftHandleYValue = 0.0;
    a.rightHandleXDeg = 60.0; a.rightHandleYValue = 0.0;
    PTCustomCurvePoint b;
    b.xDeg = 180.0; b.yValue = 255.0;
    b.leftHandleXDeg = 120.0; b.leftHandleYValue = 255.0;
    b.rightHandleXDeg = 240.0; b.rightHandleYValue = 255.0;
    PTCustomCurvePoint c;
    c.xDeg = 360.0; c.yValue = 0.0;
    c.leftHandleXDeg = 300.0; c.leftHandleYValue = 0.0;
    c.rightHandleXDeg = 360.0; c.rightHandleYValue = 0.0;
    pts << a << b << c;
    return pts;
}

double snapValue(double value, const QVector<double>& snaps, double threshold)
{
    for (double snap : snaps)
    {
        if (qAbs(value - snap) <= threshold)
            return snap;
    }
    return value;
}

} // namespace

PTDimmerWaveCurveWidget::PTDimmerWaveCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(100);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
}

void PTDimmerWaveCurveWidget::setParams(const PTDimmerWaveParams& params)
{
    m_params = params;
    if (m_params.customCurveEnabled)
        setCustomCurve(m_params.customCurve);
    update();
}

void PTDimmerWaveCurveWidget::setCycleDurationMs(quint32 ms)
{
    m_cycleDurationMs = qMax(quint32(20), ms);
    update();
}

void PTDimmerWaveCurveWidget::setEditable(bool editable)
{
    m_editable = editable;
    update();
}

void PTDimmerWaveCurveWidget::setCustomCurve(const QVector<PTCustomCurvePoint>& points)
{
    m_customCurve = points.isEmpty() ? defaultCustomCurve() : points;
    normalizeCustomCurve();
    m_params.customCurve = m_customCurve;
    update();
}

bool PTDimmerWaveCurveWidget::deleteSelectedPoint()
{
    if (m_selectedIndex <= 0 || m_selectedIndex >= m_customCurve.size() - 1)
        return false;
    m_customCurve.remove(m_selectedIndex);
    m_selectedIndex = -1;
    emitCustomCurveChanged();
    return true;
}

void PTDimmerWaveCurveWidget::resetCustomCurve()
{
    m_customCurve = defaultCustomCurve();
    m_selectedIndex = -1;
    emitCustomCurveChanged();
}

QRectF PTDimmerWaveCurveWidget::plotRect() const
{
    return rect().adjusted(28, 8, -10, -28);
}

QPointF PTDimmerWaveCurveWidget::curveToScreen(double xDeg, double yValue) const
{
    const QRectF r = plotRect();
    return QPointF(r.left() + qBound(0.0, xDeg, 360.0) / 360.0 * r.width(),
                   r.bottom() - qBound(0.0, yValue, 255.0) / 255.0 * r.height());
}

QPointF PTDimmerWaveCurveWidget::screenToCurve(const QPointF& pt) const
{
    const QRectF r = plotRect();
    return QPointF(qBound(0.0, (pt.x() - r.left()) / qMax(1.0, r.width()) * 360.0, 360.0),
                   qBound(0.0, (r.bottom() - pt.y()) / qMax(1.0, r.height()) * 255.0, 255.0));
}

QPointF PTDimmerWaveCurveWidget::snappedCurvePoint(const QPointF& pt, int movingIndex) const
{
    QVector<double> xSnaps {0.0, 90.0, 180.0, 270.0, 360.0};
    QVector<double> ySnaps {0.0, 64.0, 128.0, 192.0, 255.0};
    for (int i = 0; i < m_customCurve.size(); ++i)
    {
        if (i == movingIndex)
            continue;
        xSnaps.append(m_customCurve.at(i).xDeg);
        ySnaps.append(m_customCurve.at(i).yValue);
    }
    return QPointF(snapValue(pt.x(), xSnaps, 4.0),
                   snapValue(pt.y(), ySnaps, 5.0));
}

QPainterPath PTDimmerWaveCurveWidget::customCurvePath(const QVector<PTCustomCurvePoint>& points) const
{
    QPainterPath path;
    if (points.isEmpty())
        return path;

    path.moveTo(curveToScreen(points.first().xDeg, points.first().yValue));
    for (int i = 0; i < points.size() - 1; ++i)
    {
        const PTCustomCurvePoint& a = points.at(i);
        const PTCustomCurvePoint& b = points.at(i + 1);
        path.cubicTo(curveToScreen(a.rightHandleXDeg, a.rightHandleYValue),
                     curveToScreen(b.leftHandleXDeg, b.leftHandleYValue),
                     curveToScreen(b.xDeg, b.yValue));
    }
    return path;
}

void PTDimmerWaveCurveWidget::normalizeCustomCurve()
{
    if (m_customCurve.size() < 2)
        m_customCurve = defaultCustomCurve();
    std::sort(m_customCurve.begin(), m_customCurve.end(),
              [](const PTCustomCurvePoint& a, const PTCustomCurvePoint& b) {
                  return a.xDeg < b.xDeg;
              });
    m_customCurve.first().xDeg = 0.0;
    m_customCurve.last().xDeg = 360.0;
    for (PTCustomCurvePoint& p : m_customCurve)
    {
        p.xDeg = qBound(0.0, p.xDeg, 360.0);
        p.yValue = qBound(0.0, p.yValue, 255.0);
        p.leftHandleXDeg = qBound(0.0, p.leftHandleXDeg, 360.0);
        p.leftHandleYValue = qBound(0.0, p.leftHandleYValue, 255.0);
        p.rightHandleXDeg = qBound(0.0, p.rightHandleXDeg, 360.0);
        p.rightHandleYValue = qBound(0.0, p.rightHandleYValue, 255.0);
    }
    m_params.customCurve = m_customCurve;
}

void PTDimmerWaveCurveWidget::emitCustomCurveChanged()
{
    normalizeCustomCurve();
    m_params.customCurve = m_customCurve;
    emit customCurveChanged(m_customCurve);
    update();
}

int PTDimmerWaveCurveWidget::hitPoint(const QPointF& pos, DragTarget* target) const
{
    if (target)
        *target = DragTarget::None;
    if (!m_editable)
        return -1;
    for (int i = 0; i < m_customCurve.size(); ++i)
    {
        const PTCustomCurvePoint& c = m_customCurve.at(i);
        if (QLineF(pos, curveToScreen(c.xDeg, c.yValue)).length() <= 7.0)
        {
            if (target)
                *target = DragTarget::Point;
            return i;
        }
        if (QLineF(pos, curveToScreen(c.leftHandleXDeg, c.leftHandleYValue)).length() <= 6.0)
        {
            if (target)
                *target = DragTarget::LeftHandle;
            return i;
        }
        if (QLineF(pos, curveToScreen(c.rightHandleXDeg, c.rightHandleYValue)).length() <= 6.0)
        {
            if (target)
                *target = DragTarget::RightHandle;
            return i;
        }
    }
    return -1;
}

void PTDimmerWaveCurveWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = plotRect();
    p.fillRect(r, palette().color(QPalette::Base).darker(110));

    p.setPen(QPen(palette().color(QPalette::Mid), 1));
    for (int deg : {0, 90, 180, 270, 360})
    {
        const qreal x = curveToScreen(deg, 0).x();
        p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
        p.drawText(QRectF(x - 18, r.bottom() + 2, 36, 14), Qt::AlignCenter,
                   QString::number(deg));
    }
    for (int value : {0, 64, 128, 192, 255})
    {
        const qreal y = curveToScreen(0, value).y();
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        p.drawText(QRectF(0, y - 7, 24, 14), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(value));
    }

    const int waveW = qBound(0, m_params.waveWidth, 360);
    const int packetPx = waveW > 0 ? qMax(1, int(r.width() * waveW / 360.0)) : 0;
    if (packetPx > 0)
    {
        QRectF packetRect(r.left(), r.top(), packetPx, r.height());
        p.fillRect(packetRect, palette().color(QPalette::Highlight).lighter(160));
    }

    // Fade zone guides inside packet
    if (packetPx > 0 && m_params.waveShape != 1 && !m_params.customCurveEnabled)
    {
        const float fadeIn = float(qBound(0, m_params.waveFadeIn, 100)) / 100.0f;
        const float fadeOut = float(qBound(0, m_params.waveFadeOut, 100)) / 100.0f;
        p.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DashLine));
        if (fadeIn > 0.0f)
        {
            const qreal x = r.left() + packetPx * fadeIn;
            p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
        }
        if (fadeOut > 0.0f)
        {
            const qreal x = r.left() + packetPx - packetPx * fadeOut;
            p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
        }
    }

    QVector<PTCustomCurvePoint> pts;
    if (m_params.customCurveEnabled)
        pts = m_customCurve.isEmpty() ? defaultCustomCurve() : m_customCurve;

    if (m_params.customCurveEnabled)
    {
        p.setPen(QPen(palette().color(QPalette::BrightText), 3));
        p.setBrush(Qt::NoBrush);
        p.drawPath(customCurvePath(pts));
    }
    else
    {
        QPolygonF poly;
        const int samples = qMax(64, int(r.width()));
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
    }

    if (m_params.customCurveEnabled && m_editable)
    {
        for (int i = 0; i < pts.size(); ++i)
        {
            const PTCustomCurvePoint& c = pts.at(i);
            const QPointF point = curveToScreen(c.xDeg, c.yValue);
            const QPointF lh = curveToScreen(c.leftHandleXDeg, c.leftHandleYValue);
            const QPointF rh = curveToScreen(c.rightHandleXDeg, c.rightHandleYValue);
            const bool selected = (i == m_selectedIndex);
            p.setPen(QPen(selected ? palette().color(QPalette::BrightText)
                                   : palette().color(QPalette::Mid),
                         selected ? 2 : 1, Qt::DashLine));
            p.drawLine(point, lh);
            p.drawLine(point, rh);
            p.setPen(QPen(palette().color(QPalette::BrightText), 1));
            p.setBrush(palette().color(QPalette::Mid));
            p.drawEllipse(lh, selected ? 4 : 3, selected ? 4 : 3);
            p.drawEllipse(rh, selected ? 4 : 3, selected ? 4 : 3);
            p.setBrush(selected ? palette().color(QPalette::Highlight)
                                : palette().color(QPalette::Button));
            p.drawEllipse(point, selected ? 7 : 5, selected ? 7 : 5);
        }
    }

    p.setPen(palette().color(QPalette::Text));
    p.drawText(QRect(8, height() - 18, width() - 16, 16), Qt::AlignLeft,
               tr("X: 0–360° (1 cycle = %1 ms)   Y: 0–255   Width: %2°")
                       .arg(m_cycleDurationMs)
                       .arg(waveW));
}

void PTDimmerWaveCurveWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_editable && m_params.customCurveEnabled && event->button() == Qt::LeftButton)
    {
        emit customCurveEditRequested();
        return;
    }
    if (!m_editable || !m_params.customCurveEnabled)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    setFocus();
    DragTarget target;
    const int idx = hitPoint(event->pos(), &target);
    if (idx >= 0)
    {
        m_selectedIndex = idx;
        m_dragTarget = target;
        update();
        return;
    }
    QWidget::mousePressEvent(event);
}

void PTDimmerWaveCurveWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_editable || m_selectedIndex < 0 || m_selectedIndex >= m_customCurve.size()
            || m_dragTarget == DragTarget::None)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }

    QPointF pt = snappedCurvePoint(screenToCurve(event->pos()), m_selectedIndex);
    PTCustomCurvePoint& c = m_customCurve[m_selectedIndex];
    if (m_dragTarget == DragTarget::Point)
    {
        if (m_selectedIndex == 0)
            pt.setX(0.0);
        else if (m_selectedIndex == m_customCurve.size() - 1)
            pt.setX(360.0);
        const double appliedDx = pt.x() - c.xDeg;
        const double appliedDy = pt.y() - c.yValue;
        c.xDeg = pt.x();
        c.yValue = pt.y();
        c.leftHandleXDeg = qBound(0.0, c.leftHandleXDeg + appliedDx, 360.0);
        c.leftHandleYValue = qBound(0.0, c.leftHandleYValue + appliedDy, 255.0);
        c.rightHandleXDeg = qBound(0.0, c.rightHandleXDeg + appliedDx, 360.0);
        c.rightHandleYValue = qBound(0.0, c.rightHandleYValue + appliedDy, 255.0);
    }
    else if (m_dragTarget == DragTarget::LeftHandle)
    {
        c.leftHandleXDeg = pt.x();
        c.leftHandleYValue = pt.y();
    }
    else if (m_dragTarget == DragTarget::RightHandle)
    {
        c.rightHandleXDeg = pt.x();
        c.rightHandleYValue = pt.y();
    }
    emitCustomCurveChanged();
}

void PTDimmerWaveCurveWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_dragTarget = DragTarget::None;
}

void PTDimmerWaveCurveWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_editable && m_params.customCurveEnabled && event->button() == Qt::LeftButton)
    {
        emit customCurveEditRequested();
        return;
    }
    if (!m_editable || !m_params.customCurveEnabled)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    QPointF pt = snappedCurvePoint(screenToCurve(event->pos()), -1);
    PTCustomCurvePoint c;
    c.xDeg = pt.x();
    c.yValue = pt.y();
    c.leftHandleXDeg = qMax(0.0, c.xDeg - 30.0);
    c.leftHandleYValue = c.yValue;
    c.rightHandleXDeg = qMin(360.0, c.xDeg + 30.0);
    c.rightHandleYValue = c.yValue;
    m_customCurve.append(c);
    normalizeCustomCurve();
    for (int i = 0; i < m_customCurve.size(); ++i)
    {
        if (qAbs(m_customCurve.at(i).xDeg - c.xDeg) < 0.001)
        {
            m_selectedIndex = i;
            break;
        }
    }
    emitCustomCurveChanged();
}

void PTDimmerWaveCurveWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_editable && (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace))
    {
        if (deleteSelectedPoint())
            return;
    }
    QWidget::keyPressEvent(event);
}

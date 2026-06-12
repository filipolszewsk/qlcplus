/*
  ptpositionpath2dwidget.cpp
*/

#include "ptpositionpath2dwidget.h"
#include "ptshapesgallery.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

PTPositionPath2DWidget::PTPositionPath2DWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(320, 220);
    setAutoFillBackground(true);
    m_points = PTShapesGallery::defaultMotionPath2D();
}

void PTPositionPath2DWidget::setPath(const QVector<PTPositionPath2DPoint>& points, bool closed)
{
    m_points = points.size() >= 2 ? points : PTShapesGallery::defaultMotionPath2D();
    m_closed = closed;
    update();
}

void PTPositionPath2DWidget::setPathClosed(bool closed)
{
    if (m_closed == closed)
        return;
    m_closed = closed;
    emitPathChanged();
    update();
}

QRectF PTPositionPath2DWidget::plotRect() const
{
    return rect().adjusted(24, 20, -12, -12);
}

QPointF PTPositionPath2DWidget::screenToUnit(const QPointF& pt) const
{
    const QRectF plot = plotRect();
    const QPointF c = plot.center();
    const double pan = (pt.x() - c.x()) / (plot.width() * 0.45);
    const double tilt = -(pt.y() - c.y()) / (plot.height() * 0.45);
    return QPointF(qBound(-1.0, pan, 1.0), qBound(-1.0, tilt, 1.0));
}

QPointF PTPositionPath2DWidget::unitToScreen(double pan01, double tilt01) const
{
    const QRectF plot = plotRect();
    const QPointF c = plot.center();
    return QPointF(c.x() + pan01 * plot.width() * 0.45,
                   c.y() - tilt01 * plot.height() * 0.45);
}

int PTPositionPath2DWidget::hitPoint(const QPointF& pos) const
{
    for (int i = 0; i < m_points.size(); ++i)
    {
        const QPointF pt = unitToScreen(m_points.at(i).pan01, m_points.at(i).tilt01);
        if (QLineF(pos, pt).length() <= 8.0)
            return i;
    }
    return -1;
}

void PTPositionPath2DWidget::emitPathChanged()
{
    emit pathChanged(m_points, m_closed);
}

void PTPositionPath2DWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.fillRect(rect(), palette().base());

    const QRectF plot = plotRect();
    p.setPen(palette().text().color());
    p.drawText(QRectF(rect().left(), 2, rect().width(), 16), Qt::AlignLeft | Qt::AlignVCenter,
               tr("2D path (pan/tilt unit space) — double-click empty area to add point"));

    p.setPen(QPen(palette().mid(), 1));
    p.drawRect(plot);
    p.drawLine(plot.center().x(), plot.top(), plot.center().x(), plot.bottom());
    p.drawLine(plot.left(), plot.center().y(), plot.right(), plot.center().y());

    if (m_points.size() >= 2)
    {
        QPainterPath path;
        path.moveTo(unitToScreen(m_points.first().pan01, m_points.first().tilt01));
        for (int i = 1; i < m_points.size(); ++i)
            path.lineTo(unitToScreen(m_points.at(i).pan01, m_points.at(i).tilt01));
        if (m_closed)
            path.closeSubpath();
        p.setPen(QPen(QColor(80, 160, 255), 2));
        p.drawPath(path);
    }

    p.setBrush(QColor(120, 220, 120));
    p.setPen(QPen(palette().text().color(), 1));
    for (const PTPositionPath2DPoint& point : m_points)
    {
        const QPointF pt = unitToScreen(point.pan01, point.tilt01);
        p.drawEllipse(pt, 5, 5);
    }
}

void PTPositionPath2DWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_dragIndex = hitPoint(event->position());
    if (m_dragIndex < 0)
        return;
    event->accept();
}

void PTPositionPath2DWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragIndex < 0 || m_dragIndex >= m_points.size())
        return;
    const QPointF unit = screenToUnit(event->position());
    PTPositionPath2DPoint& point = m_points[m_dragIndex];
    point.pan01 = unit.x();
    point.tilt01 = unit.y();
    point.leftHandlePan01 = point.pan01;
    point.leftHandleTilt01 = point.tilt01;
    point.rightHandlePan01 = point.pan01;
    point.rightHandleTilt01 = point.tilt01;
    emitPathChanged();
    update();
}

void PTPositionPath2DWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    m_dragIndex = -1;
}

void PTPositionPath2DWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (hitPoint(event->position()) >= 0)
    {
        if (m_points.size() > 2)
        {
            const int idx = hitPoint(event->position());
            m_points.remove(idx);
            emitPathChanged();
            update();
        }
        return;
    }

    const QPointF unit = screenToUnit(event->position());
    PTPositionPath2DPoint point;
    point.pan01 = unit.x();
    point.tilt01 = unit.y();
    point.leftHandlePan01 = point.pan01;
    point.leftHandleTilt01 = point.tilt01;
    point.rightHandlePan01 = point.pan01;
    point.rightHandleTilt01 = point.tilt01;
    point.segmentMode = PTCustomCurvePoint::Linear;
    m_points.append(point);
    emitPathChanged();
    update();
}

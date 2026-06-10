/*
  ptpositionxypadwidget.cpp
*/

#include "ptpositionxypadwidget.h"
#include "ptpositionconverter.h"

#include "fixture.h"

#include <QPainter>
#include <QMouseEvent>

PTPositionXYPadWidget::PTPositionXYPadWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(160, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void PTPositionXYPadWidget::setFixture(Fixture* fxi, int head)
{
    m_fixture = fxi;
    m_head = head;
    update();
}

void PTPositionXYPadWidget::setNormalizedPosition(qreal xNorm, qreal yNorm)
{
    m_xNorm = qBound(0.0, xNorm, 1.0);
    m_yNorm = qBound(0.0, yNorm, 1.0);
    update();
}

QRect PTPositionXYPadWidget::padRect() const
{
    const int margin = 8;
    const int side = qMin(width(), height()) - margin * 2;
    const int x = (width() - side) / 2;
    const int y = (height() - side) / 2;
    return QRect(x, y, side, side);
}

void PTPositionXYPadWidget::updateFromMouse(const QPoint& pos)
{
    const QRect r = padRect();
    const int cx = qBound(r.left(), pos.x(), r.right());
    const int cy = qBound(r.top(), pos.y(), r.bottom());
    const qreal spanX = qreal(qMax(1, r.width() - 1));
    const qreal spanY = qreal(qMax(1, r.height() - 1));
    const qreal xNorm = qreal(cx - r.left()) / spanX;
    const qreal yNorm = qreal(cy - r.top()) / spanY;
    m_xNorm = qBound(0.0, xNorm, 1.0);
    m_yNorm = qBound(0.0, yNorm, 1.0);
    update();
    emit positionChanged(m_xNorm, m_yNorm);
}

void PTPositionXYPadWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Base));

    const QRect r = padRect();
    p.setPen(QPen(palette().color(QPalette::Mid), 1));
    p.drawRect(r);

    p.setPen(QPen(palette().color(QPalette::Midlight), 1, Qt::DashLine));
    p.drawLine(r.center().x(), r.top(), r.center().x(), r.bottom());
    p.drawLine(r.left(), r.center().y(), r.right(), r.center().y());

    const qreal spanX = qreal(qMax(1, r.width() - 1));
    const qreal spanY = qreal(qMax(1, r.height() - 1));
    const int hx = r.left() + qRound(m_xNorm * spanX);
    const int hy = r.top() + qRound(m_yNorm * spanY);
    p.setBrush(QColor(60, 140, 240));
    p.setPen(QPen(Qt::white, 2));
    p.drawEllipse(QPoint(hx, hy), 7, 7);

    if (m_fixture)
    {
        qreal panDeg = 0;
        qreal tiltDeg = 0;
        PTPositionConverter::normalizedToDegrees(m_fixture, m_head, m_xNorm, m_yNorm,
                                                 panDeg, tiltDeg);
        p.setPen(palette().color(QPalette::Text));
        p.drawText(r.adjusted(4, 4, -4, -4), Qt::AlignBottom | Qt::AlignHCenter,
                   QStringLiteral("%1° / %2°")
                           .arg(panDeg, 0, 'f', 1)
                           .arg(tiltDeg, 0, 'f', 1));
    }
    else
    {
        p.setPen(palette().color(QPalette::Disabled, QPalette::Text));
        p.drawText(rect(), Qt::AlignCenter, tr("Select a fixture cell"));
    }
}

void PTPositionXYPadWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_fixture)
    {
        m_dragging = true;
        grabMouse();
        updateFromMouse(event->pos());
    }
}

void PTPositionXYPadWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging)
        updateFromMouse(event->pos());
}

void PTPositionXYPadWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging)
    {
        m_dragging = false;
        releaseMouse();
    }
}

QSize PTPositionXYPadWidget::sizeHint() const
{
    return QSize(200, 200);
}

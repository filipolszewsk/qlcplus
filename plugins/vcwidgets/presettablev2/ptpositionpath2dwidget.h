/*
  ptpositionpath2dwidget.h — editable 2D pan/tilt motion path
*/

#pragma once

#include "presettablev2effectengine.h"

#include <QVector>
#include <QWidget>

class PTPositionPath2DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PTPositionPath2DWidget(QWidget* parent = nullptr);

    void setPath(const QVector<PTPositionPath2DPoint>& points, bool closed);
    QVector<PTPositionPath2DPoint> path() const { return m_points; }
    bool pathClosed() const { return m_closed; }
    void setPathClosed(bool closed);

signals:
    void pathChanged(const QVector<PTPositionPath2DPoint>& points, bool closed);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QRectF plotRect() const;
    QPointF screenToUnit(const QPointF& pt) const;
    QPointF unitToScreen(double pan01, double tilt01) const;
    int hitPoint(const QPointF& pos) const;
    void emitPathChanged();

    QVector<PTPositionPath2DPoint> m_points;
    bool m_closed = true;
    int m_dragIndex = -1;
};

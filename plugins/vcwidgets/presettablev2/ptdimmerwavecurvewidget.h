/*
  Dimmer wave preview: X = 0–360° cycle, Y = dimmer 0–255.
*/

#pragma once

#include "ptdimmerwaveengine.h"

#include <QWidget>

class PTDimmerWaveCurveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PTDimmerWaveCurveWidget(QWidget* parent = nullptr);

    void setParams(const PTDimmerWaveParams& params);
    void setCycleDurationMs(quint32 ms);
    void setEditable(bool editable);
    void setCustomCurve(const QVector<PTCustomCurvePoint>& points);
    QVector<PTCustomCurvePoint> customCurve() const { return m_customCurve; }
    int selectedPointIndex() const { return m_selectedIndex; }
    bool deleteSelectedPoint();
    void resetCustomCurve();

signals:
    void customCurveChanged(const QVector<PTCustomCurvePoint>& points);
    void customCurveEditRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    QSize sizeHint() const override { return QSize(400, 120); }

private:
    enum class DragTarget { None, Point, LeftHandle, RightHandle };

    QRectF plotRect() const;
    QPointF curveToScreen(double xDeg, double yValue) const;
    QPointF screenToCurve(const QPointF& pt) const;
    QPointF snappedCurvePoint(const QPointF& pt, int movingIndex) const;
    QPainterPath customCurvePath(const QVector<PTCustomCurvePoint>& points) const;
    void normalizeCustomCurve();
    void emitCustomCurveChanged();
    int hitPoint(const QPointF& pos, DragTarget* target) const;

    PTDimmerWaveParams m_params;
    quint32 m_cycleDurationMs = 5000;
    QVector<PTCustomCurvePoint> m_customCurve;
    bool m_editable = false;
    int m_selectedIndex = -1;
    DragTarget m_dragTarget = DragTarget::None;
};

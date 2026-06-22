/*
  Dimmer wave preview: X = 0–360° cycle, Y = dimmer 0–255.
*/

#pragma once

#include "ptdimmerwaveengine.h"

#include <QColor>
#include <QTimer>
#include <QWidget>

class PTDimmerWaveCurveWidget : public QWidget
{
    Q_OBJECT

public:
    enum class MarkerMode
    {
        Cyclic,
        OneShot
    };

    struct PhaseMarker
    {
        qreal phaseOffset01 = 0.0;
        QColor color;
    };

    explicit PTDimmerWaveCurveWidget(QWidget* parent = nullptr);

    void setParams(const PTDimmerWaveParams& params);
    void setCycleDurationMs(quint32 ms);
    void setPhaseMarkers(const QVector<PhaseMarker>& markers,
                         MarkerMode mode = MarkerMode::Cyclic);
    void setOneShotProgress(qreal progress01);
    void setEditable(bool editable);
    void setStatusTextVisible(bool visible);
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
    QRectF customCurveRect() const;
    QPointF snappedCurvePoint(const QPointF& pt, int movingIndex) const;
    QPointF constrainedCurvePoint(const QPointF& pt) const;
    QPainterPath customCurvePath(const QVector<PTCustomCurvePoint>& points) const;
    void normalizeCustomCurve();
    void emitCustomCurveChanged();
    int hitPoint(const QPointF& pos, DragTarget* target) const;
    void updateAnimationState();

    PTDimmerWaveParams m_params;
    quint32 m_cycleDurationMs = 5000;
    QVector<PhaseMarker> m_phaseMarkers;
    MarkerMode m_markerMode = MarkerMode::Cyclic;
    qreal m_oneShotProgress01 = 0.0;
    QTimer m_timer;
    qreal m_animPhase01 = 0.0;
    QVector<PTCustomCurvePoint> m_customCurve;
    bool m_editable = false;
    bool m_statusTextVisible = true;
    int m_selectedIndex = -1;
    DragTarget m_dragTarget = DragTarget::None;
    QPointF m_dragStartCurvePoint;
};

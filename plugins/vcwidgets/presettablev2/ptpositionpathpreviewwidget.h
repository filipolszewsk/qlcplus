/*
  ptpositionpathpreviewwidget.h — 2D pan/tilt orbit preview for Position Mode FX
*/

#pragma once

#include "presettablev2effectengine.h"

#include <QColor>
#include <QTimer>
#include <QVector>
#include <QWidget>

class PTPositionPathPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    struct OrbitBall
    {
        qreal phaseOffset01 = 0;
        QColor color = QColor(80, 160, 255);
    };

    explicit PTPositionPathPreviewWidget(QWidget* parent = nullptr);

    void setOrbitPreview(PTPositionMotion motion, qreal panSizeDeg, qreal tiltSizeDeg,
                         const QVector<OrbitBall>& balls, quint32 cycleMs);
    void setOrbitPreviewFromPreset(const PTTransitionPreset& preset,
                                   const QVector<OrbitBall>& balls, quint32 cycleMs);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override { return QSize(400, 130); }

private:
    void stopAnimation();
    QRectF plotRect() const;
    QPointF mapRelative(qreal panOffDeg, qreal tiltOffDeg, const QPointF& center,
                        qreal scale) const;

    PTPositionMotion m_motion = PTPositionMotion::Off;
    PTTransitionPreset m_preset;
    bool m_usePresetMotion = false;
    qreal m_panSize = 0;
    qreal m_tiltSize = 0;
    QVector<OrbitBall> m_balls;
    QTimer m_timer;
    quint32 m_cycleMs = 5000;
    double m_animPhase01 = 0;
};

/*
  ptpositionmotion1dpreviewwidget.h — 1D pan/tilt offset vs time preview for Position Mode FX
*/

#pragma once

#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"

#include <QColor>
#include <QTimer>
#include <QVector>
#include <QWidget>

class PTPositionMotion1DPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    struct PhaseMarker
    {
        qreal phaseOffset01 = 0;
        QColor color = QColor(80, 160, 255);
    };

    explicit PTPositionMotion1DPreviewWidget(QWidget* parent = nullptr);

    static bool is1DMotion(PTPositionMotion motion);

    void setMotionPreview(PTPositionMotion motion, qreal panSizeDeg, qreal tiltSizeDeg,
                          const QVector<PhaseMarker>& markers, quint32 cycleMs);
    void setMotionPreviewFromPreset(const PTTransitionPreset& preset,
                                    const PTDimmerWaveParams& waveParams,
                                    const QVector<PhaseMarker>& markers, quint32 cycleMs);
    void setMotionPreview2DFromPreset(const PTTransitionPreset& preset,
                                      const QVector<PhaseMarker>& markers, quint32 cycleMs);
    void clear();

signals:
    void motionCurveEditRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    QSize sizeHint() const override { return QSize(400, 130); }

private:
    void stopAnimation();
    QRectF plotRect() const;
    QPointF mapSample(double cycle01, qreal normValue, const QRectF& plot) const;
    qreal sampleAtCycle01(double cycle01) const;
    qreal sampleAxisAtCycle01(double cycle01, bool tiltAxis) const;

    PTPositionMotion m_motion = PTPositionMotion::Off;
    PTTransitionPreset m_preset;
    PTDimmerWaveParams m_waveParams;
    bool m_usePresetMotion = false;
    bool m_dualAxisPreview = false;
    qreal m_panSize = 0;
    qreal m_tiltSize = 0;
    QVector<PhaseMarker> m_markers;
    QTimer m_timer;
    quint32 m_cycleMs = 5000;
    double m_animPhase01 = 0;
};

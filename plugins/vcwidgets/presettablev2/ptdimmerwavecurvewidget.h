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

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override { return QSize(400, 120); }

private:
    PTDimmerWaveParams m_params;
    quint32 m_cycleDurationMs = 5000;
};

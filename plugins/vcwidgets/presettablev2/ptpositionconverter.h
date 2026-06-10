/*
  ptpositionconverter.h — Pan/Tilt degrees ↔ DMX for Position Mode
*/

#pragma once

#include "presettablev2widget.h"

#include <QRectF>

class Fixture;

class PTPositionConverter
{
public:
    static QRectF degreesRange(Fixture* fxi, int head);
    static PTPositionValue centerPosition(Fixture* fxi, int head);

    static qreal clampPanDeg(Fixture* fxi, int head, qreal panDeg);
    static qreal clampTiltDeg(Fixture* fxi, int head, qreal tiltDeg);
    static PTPositionValue clampPosition(Fixture* fxi, int head, const PTPositionValue& pos);

    static quint16 panDegToPan16(Fixture* fxi, int head, qreal panDeg);
    static quint16 tiltDegToTilt16(Fixture* fxi, int head, qreal tiltDeg);
    static qreal pan16ToPanDeg(Fixture* fxi, int head, quint16 pan16);
    static qreal tilt16ToTiltDeg(Fixture* fxi, int head, quint16 tilt16);

    static void degreesToNormalized(Fixture* fxi, int head, qreal panDeg, qreal tiltDeg,
                                    qreal& xNorm, qreal& yNorm);
    static void normalizedToDegrees(Fixture* fxi, int head, qreal xNorm, qreal yNorm,
                                    qreal& panDeg, qreal& tiltDeg);

    static QString formatPosition(const PTPositionValue& pos, int decimals = 1);
    static PTPositionValue parsePositionText(const QString& raw, Fixture* fxi, int head);
    static PTPositionValue blendPositions(const PTPositionValue& a, const PTPositionValue& b,
                                          double t);
    static PTPositionValue addRelativeOffset(const PTPositionValue& base, Fixture* fxi, int head,
                                             qreal panOffsetDeg, qreal tiltOffsetDeg);
};

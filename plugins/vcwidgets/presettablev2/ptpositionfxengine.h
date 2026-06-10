/*
  ptpositionfxengine.h — relative 2D position offsets for Position Mode FX
*/

#pragma once

#include "presettablev2effectengine.h"

class Fixture;

class PTPositionFxEngine
{
public:
    enum class Shape
    {
        Circle = 0,
        Line,
        PanOnly,
        TiltOnly,
        Figure8
    };

    static Shape shapeFromPositionMotion(PTPositionMotion motion);
    static void relativeOffset(Shape shape, double phaseRadians,
                               qreal panSizeDeg, qreal tiltSizeDeg,
                               qreal& panOffDeg, qreal& tiltOffDeg);
    /** Smart compressor: pivot shifts toward axis center only when amplitude would clip. */
    static PTPositionValue applySmartMotion(const PTPositionValue& base, Fixture* fxi, int head,
                                            Shape shape, double phaseRadians, qreal size01);
};

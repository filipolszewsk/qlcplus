/*
  ptpositionfxengine.h — relative 2D position offsets for Position Mode FX
*/

#pragma once

#include "presettablev2effectengine.h"

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
};

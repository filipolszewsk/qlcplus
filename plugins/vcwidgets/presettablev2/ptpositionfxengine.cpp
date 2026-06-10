/*
  ptpositionfxengine.cpp
*/

#include "ptpositionfxengine.h"

#include <QtMath>

PTPositionFxEngine::Shape PTPositionFxEngine::shapeFromPositionMotion(PTPositionMotion motion)
{
    switch (motion)
    {
        case PTPositionMotion::Pan1D:     return Shape::PanOnly;
        case PTPositionMotion::Tilt1D:    return Shape::TiltOnly;
        case PTPositionMotion::Line2D:    return Shape::Line;
        case PTPositionMotion::Figure8_2D: return Shape::Figure8;
        case PTPositionMotion::Circle2D:  return Shape::Circle;
        default:                          return Shape::Circle;
    }
}

void PTPositionFxEngine::relativeOffset(Shape shape, double phaseRadians,
                                        qreal panSizeDeg, qreal tiltSizeDeg,
                                        qreal& panOffDeg, qreal& tiltOffDeg)
{
    panOffDeg = 0;
    tiltOffDeg = 0;
    const double s = qSin(phaseRadians);
    const double c = qCos(phaseRadians);

    switch (shape)
    {
        case Shape::Circle:
            panOffDeg = qreal(s) * panSizeDeg;
            tiltOffDeg = qreal(c) * tiltSizeDeg;
            break;
        case Shape::Line:
            panOffDeg = qreal(s) * panSizeDeg;
            tiltOffDeg = 0;
            break;
        case Shape::PanOnly:
            panOffDeg = qreal(s) * panSizeDeg;
            break;
        case Shape::TiltOnly:
            tiltOffDeg = qreal(s) * tiltSizeDeg;
            break;
        case Shape::Figure8:
            panOffDeg = qreal(s) * panSizeDeg;
            tiltOffDeg = qreal(qSin(phaseRadians * 2.0)) * tiltSizeDeg;
            break;
    }
}

/*
  ptpositionfxengine.cpp
*/

#include "ptpositionfxengine.h"
#include "ptpositionconverter.h"
#include "ptdimmerwaveengine.h"

#include "fixture.h"

#include <QtMath>

namespace {

bool shapeUsesPan(PTPositionFxEngine::Shape shape)
{
    return shape != PTPositionFxEngine::Shape::TiltOnly;
}

bool shapeUsesTilt(PTPositionFxEngine::Shape shape)
{
    return shape == PTPositionFxEngine::Shape::Circle
            || shape == PTPositionFxEngine::Shape::Figure8
            || shape == PTPositionFxEngine::Shape::TiltOnly;
}

qreal pivotAndAmplitude(qreal baseDeg, qreal minDeg, qreal maxDeg, qreal size01, qreal& amplitude)
{
    amplitude = 0;
    if (size01 <= 0)
        return baseDeg;

    const qreal half = (maxDeg - minDeg) / 2.0;
    const qreal center = minDeg + half;
    const qreal A = size01 * half;
    const qreal pivotLo = minDeg + A;
    const qreal pivotHi = maxDeg - A;
    qreal pivot = baseDeg;
    if (pivotLo <= pivotHi)
        pivot = qBound(pivotLo, baseDeg, pivotHi);
    else
        pivot = center;
    amplitude = A;
    return pivot;
}

} // namespace

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

bool PTPositionFxEngine::orbitPhaseFromIterator(float iteratorRad, int waveWidthDeg,
                                                double& outPhaseRad)
{
    const float widthRad = (float(qBound(1, waveWidthDeg, 360)) / 360.0f) * float(M_PI * 2.0);
    if (widthRad <= 0.0f || iteratorRad >= widthRad)
        return false;

    outPhaseRad = double(iteratorRad / widthRad) * 2.0 * M_PI;
    return true;
}

double PTPositionFxEngine::applyMotionDirection(double phaseRad,
                                                PTPositionMotionDirection direction,
                                                const PTDimmerWaveOffsetInfo& spatial)
{
    switch (direction)
    {
        case PTPositionMotionDirection::Reverse:
            return -phaseRad;
        case PTPositionMotionDirection::AlternateWings:
            return (spatial.wingIndex % 2 == 1) ? -phaseRad : phaseRad;
        case PTPositionMotionDirection::SymmetricPairs:
            return (spatial.localIndex % 2 == 1) ? -phaseRad : phaseRad;
        default:
            return phaseRad;
    }
}

qreal PTPositionFxEngine::orbitAmplitude01(float iteratorRad, int waveWidthDeg,
                                           const PTDimmerWaveParams& waveParams, Shape shape)
{
    if (shape != Shape::PanOnly && shape != Shape::TiltOnly)
        return 1.0;

    const float widthRad = (float(qBound(1, waveWidthDeg, 360)) / 360.0f) * float(M_PI * 2.0);
    if (widthRad <= 0.0f || iteratorRad >= widthRad)
        return 0.0;

    PTDimmerWaveParams envParams = waveParams;
    envParams.waveLevel = 255;
    return qreal(PTDimmerWaveEngine::dimmerAtPhaseInWidth(iteratorRad / widthRad, envParams));
}

PTPositionValue PTPositionFxEngine::applySmartMotion(const PTPositionValue& base, Fixture* fxi,
                                                     int head, Shape shape, double phaseRadians,
                                                     qreal size01)
{
    if (!base.valid || !fxi)
        return base;

    size01 = qBound(0.0, size01, 1.0);
    if (size01 <= 0)
        return PTPositionConverter::clampPosition(fxi, head, base);

    const PTPositionValue clampedBase = PTPositionConverter::clampPosition(fxi, head, base);
    const QRectF range = PTPositionConverter::degreesRange(fxi, head);
    const qreal panMin = range.left();
    const qreal panMax = range.left() + range.width();
    const qreal tiltMin = range.top();
    const qreal tiltMax = range.top() + range.height();

    qreal panUnit = 0;
    qreal tiltUnit = 0;
    relativeOffset(shape, phaseRadians, 1.0, 1.0, panUnit, tiltUnit);

    qreal panAmp = 0;
    qreal tiltAmp = 0;
    qreal pivotPan = clampedBase.panDeg;
    qreal pivotTilt = clampedBase.tiltDeg;
    if (shapeUsesPan(shape))
        pivotPan = pivotAndAmplitude(clampedBase.panDeg, panMin, panMax, size01, panAmp);
    if (shapeUsesTilt(shape))
        pivotTilt = pivotAndAmplitude(clampedBase.tiltDeg, tiltMin, tiltMax, size01, tiltAmp);

    PTPositionValue out;
    out.valid = true;
    out.panDeg = PTPositionConverter::clampPanDeg(fxi, head, pivotPan + panAmp * panUnit);
    out.tiltDeg = PTPositionConverter::clampTiltDeg(fxi, head, pivotTilt + tiltAmp * tiltUnit);
    return out;
}

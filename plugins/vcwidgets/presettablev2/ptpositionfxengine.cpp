/*
  ptpositionfxengine.cpp
*/

#include "ptpositionfxengine.h"
#include "ptpositionconverter.h"
#include "ptdimmerwaveengine.h"

#include "fixture.h"

#include <QtMath>

namespace {

bool shapeUsesTilt(PTPositionFxEngine::Shape shape)
{
    return shape == PTPositionFxEngine::Shape::Circle
            || shape == PTPositionFxEngine::Shape::Figure8
            || shape == PTPositionFxEngine::Shape::TiltOnly
            || shape == PTPositionFxEngine::Shape::Custom2D
            || shape == PTPositionFxEngine::Shape::CustomTilt1D;
}

bool shapeUsesPan(PTPositionFxEngine::Shape shape)
{
    return shape != PTPositionFxEngine::Shape::TiltOnly
            && shape != PTPositionFxEngine::Shape::CustomTilt1D;
}

QPointF samplePath2D01(double t01, const QVector<PTPositionPath2DPoint>& points, bool closed)
{
    if (points.isEmpty())
        return QPointF(0, 0);
    if (points.size() == 1)
        return QPointF(points.first().pan01, points.first().tilt01);

    const int segmentCount = closed ? points.size() : points.size() - 1;
    if (segmentCount <= 0)
        return QPointF(points.first().pan01, points.first().tilt01);

    const double t = qBound(0.0, t01, 1.0) * segmentCount;
    const int seg = qMin(int(t), segmentCount - 1);
    const double localT = t - seg;
    const PTPositionPath2DPoint& a = points.at(seg);
    const PTPositionPath2DPoint& b = points.at((seg + 1) % points.size());

    if (a.segmentMode == PTCustomCurvePoint::Linear || localT <= 0.0)
    {
        if (localT >= 1.0)
            return QPointF(b.pan01, b.tilt01);
        return QPointF(a.pan01 + (b.pan01 - a.pan01) * localT,
                       a.tilt01 + (b.tilt01 - a.tilt01) * localT);
    }

    const double u = 1.0 - localT;
    const double pan = u * u * u * a.pan01
            + 3.0 * u * u * localT * a.rightHandlePan01
            + 3.0 * u * localT * localT * b.leftHandlePan01
            + localT * localT * localT * b.pan01;
    const double tilt = u * u * u * a.tilt01
            + 3.0 * u * u * localT * a.rightHandleTilt01
            + 3.0 * u * localT * localT * b.leftHandleTilt01
            + localT * localT * localT * b.tilt01;
    return QPointF(pan, tilt);
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
        case PTPositionMotion::CustomPan1D: return Shape::CustomPan1D;
        case PTPositionMotion::CustomTilt1D: return Shape::CustomTilt1D;
        case PTPositionMotion::Custom2D:  return Shape::Custom2D;
        default:                          return Shape::Circle;
    }
}

bool PTPositionFxEngine::motionUsesCustomData(PTPositionMotion motion)
{
    return motion == PTPositionMotion::CustomPan1D
            || motion == PTPositionMotion::CustomTilt1D
            || motion == PTPositionMotion::Custom2D;
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
            tiltOffDeg = qreal(s) * tiltSizeDeg;
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
        case Shape::CustomPan1D:
        case Shape::CustomTilt1D:
        case Shape::Custom2D:
            break;
    }
}

void PTPositionFxEngine::relativeOffsetForPreset(const PTTransitionPreset& preset,
                                                 double phaseRadians,
                                                 qreal panSizeDeg, qreal tiltSizeDeg,
                                                 qreal& panOffDeg, qreal& tiltOffDeg)
{
    panOffDeg = 0;
    tiltOffDeg = 0;
    const PTPositionMotion motion = PTPositionMotion(preset.positionMotion);
    const double phase01 = phaseRadians / (2.0 * M_PI);

    switch (motion)
    {
        case PTPositionMotion::CustomPan1D:
        {
            const QVector<PTCustomCurvePoint>& curve = preset.positionMotionCurve.size() >= 2
                    ? preset.positionMotionCurve : QVector<PTCustomCurvePoint>();
            if (curve.size() < 2)
            {
                relativeOffset(Shape::PanOnly, phaseRadians, panSizeDeg, tiltSizeDeg,
                               panOffDeg, tiltOffDeg);
                return;
            }
            const float sample = PTDimmerWaveEngine::sampleCustomCurve01(float(phase01), curve);
            panOffDeg = qreal(sample * 2.0 - 1.0) * panSizeDeg;
            break;
        }
        case PTPositionMotion::CustomTilt1D:
        {
            const QVector<PTCustomCurvePoint>& curve = preset.positionMotionCurve.size() >= 2
                    ? preset.positionMotionCurve : QVector<PTCustomCurvePoint>();
            if (curve.size() < 2)
            {
                relativeOffset(Shape::TiltOnly, phaseRadians, panSizeDeg, tiltSizeDeg,
                               panOffDeg, tiltOffDeg);
                return;
            }
            const float sample = PTDimmerWaveEngine::sampleCustomCurve01(float(phase01), curve);
            tiltOffDeg = qreal(sample * 2.0 - 1.0) * tiltSizeDeg;
            break;
        }
        case PTPositionMotion::Custom2D:
        {
            const QVector<PTPositionPath2DPoint>& path = preset.positionPath2D.size() >= 2
                    ? preset.positionPath2D : QVector<PTPositionPath2DPoint>();
            if (path.size() < 2)
            {
                relativeOffset(Shape::Circle, phaseRadians, panSizeDeg, tiltSizeDeg,
                               panOffDeg, tiltOffDeg);
                return;
            }
            const QPointF unit = samplePath2D01(phase01, path, preset.positionPath2DClosed);
            panOffDeg = unit.x() * panSizeDeg;
            tiltOffDeg = unit.y() * tiltSizeDeg;
            break;
        }
        default:
        {
            const Shape shape = shapeFromPositionMotion(motion);
            relativeOffset(shape, phaseRadians, panSizeDeg, tiltSizeDeg, panOffDeg, tiltOffDeg);
            break;
        }
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
    if (shape != Shape::PanOnly && shape != Shape::TiltOnly
            && shape != Shape::CustomPan1D && shape != Shape::CustomTilt1D)
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
    if (shape == Shape::CustomPan1D || shape == Shape::CustomTilt1D || shape == Shape::Custom2D)
        return base;

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

PTPositionValue PTPositionFxEngine::applySmartMotionFromPreset(const PTPositionValue& base,
                                                               Fixture* fxi, int head,
                                                               const PTTransitionPreset& preset,
                                                               double phaseRadians, qreal size01)
{
    if (!base.valid || !fxi)
        return base;

    size01 = qBound(0.0, size01, 1.0);
    if (size01 <= 0)
        return PTPositionConverter::clampPosition(fxi, head, base);

    const PTPositionMotion motion = PTPositionMotion(preset.positionMotion);
    if (!motionUsesCustomData(motion))
    {
        return applySmartMotion(base, fxi, head, shapeFromPositionMotion(motion),
                                phaseRadians, size01);
    }

    const PTPositionValue clampedBase = PTPositionConverter::clampPosition(fxi, head, base);
    const QRectF range = PTPositionConverter::degreesRange(fxi, head);
    const qreal panMin = range.left();
    const qreal panMax = range.left() + range.width();
    const qreal tiltMin = range.top();
    const qreal tiltMax = range.top() + range.height();

    qreal panUnit = 0;
    qreal tiltUnit = 0;
    relativeOffsetForPreset(preset, phaseRadians, 1.0, 1.0, panUnit, tiltUnit);

    qreal panAmp = 0;
    qreal tiltAmp = 0;
    qreal pivotPan = clampedBase.panDeg;
    qreal pivotTilt = clampedBase.tiltDeg;
    const Shape shape = shapeFromPositionMotion(motion);
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

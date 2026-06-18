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
    return shape != PTPositionFxEngine::Shape::None
            && shape != PTPositionFxEngine::Shape::TiltOnly
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
        default:                          return Shape::None;
    }
}

bool PTPositionFxEngine::motionUsesCustomData(PTPositionMotion motion)
{
    return motion == PTPositionMotion::CustomPan1D
            || motion == PTPositionMotion::CustomTilt1D
            || motion == PTPositionMotion::Custom2D;
}

bool PTPositionFxEngine::motionIs1D(PTPositionMotion motion)
{
    return motion == PTPositionMotion::Pan1D
            || motion == PTPositionMotion::Tilt1D
            || motion == PTPositionMotion::CustomPan1D
            || motion == PTPositionMotion::CustomTilt1D;
}

namespace {

float oscillateBuiltinBipolar(float phase01, int waveShape)
{
    const float t = qBound(0.0f, phase01, 1.0f);
    const double phase = double(t) * 2.0 * M_PI;
    switch (waveShape)
    {
        case 1:
            return t < 0.5f ? 1.0f : -1.0f;
        case 2:
            if (t < 0.5f)
                return float(t * 4.0 - 1.0);
            return float(3.0 - t * 4.0);
        default:
            return float(qSin(phase));
    }
}

float envelope01(float phaseInWidth, const PTDimmerWaveParams& waveParams)
{
    PTDimmerWaveParams envParams = waveParams;
    envParams.waveLevel = 255;
    return PTDimmerWaveEngine::dimmerAtPhaseInWidth(phaseInWidth, envParams);
}

float customCurveBipolar(float phase01, const PTDimmerWaveParams& waveParams)
{
    const float sample = PTDimmerWaveEngine::sampleCustomCurve01(
            qBound(0.0f, phase01, 1.0f), waveParams.customCurve);
    const float center = 128.0f / 255.0f;
    const float bipolar = sample <= center
            ? (sample / center) - 1.0f
            : (sample - center) / (1.0f - center);
    const float level = float(qBound(0, waveParams.waveLevel, 255)) / 255.0f;
    return qBound(-1.0f, bipolar, 1.0f) * level;
}

float applyDirectionToUnitOffset(float unitOffset, PTPositionMotionDirection direction,
                                 const PTDimmerWaveOffsetInfo& spatial)
{
    switch (direction)
    {
        case PTPositionMotionDirection::Reverse:
            return -unitOffset;
        case PTPositionMotionDirection::AlternateWings:
            return (spatial.wingIndex % 2 == 1) ? -unitOffset : unitOffset;
        case PTPositionMotionDirection::SymmetricPairs:
            return (spatial.localIndex % 2 == 1) ? -unitOffset : unitOffset;
        default:
            return unitOffset;
    }
}

float normalizeRad(float radians)
{
    const float twoPi = float(M_PI * 2.0);
    float r = radians;
    while (r < 0.0f)
        r += twoPi;
    while (r >= twoPi)
        r -= twoPi;
    return r;
}

/** 1D orbit morph packet: fade out 0% = instant step down at packet end; fade in 0% = instant step up. */
float sample1DMorphPacketUnipolar01(float phase01, const PTDimmerWaveParams& waveParams)
{
    const float maxValue = float(waveParams.waveLevel) / 255.0f;
    const float phase = qBound(0.0f, phase01, 1.0f);

    const int fadeIn = qBound(0, waveParams.waveFadeIn, 100);
    const int fadeOut = qBound(0, waveParams.waveFadeOut, 100);
    const float fadeInZone = float(fadeIn) / 100.0f;
    const float fadeOutZone = float(fadeOut) / 100.0f;
    const float sustainZone = qMax(0.0f, 1.0f - fadeInZone - fadeOutZone);

    if (fadeOut == 0 && phase >= 1.0f)
        return 0.0f;

    if (fadeIn == 0 && phase <= 0.0f)
        return 0.0f;

    if (waveParams.customCurveEnabled && waveParams.customCurve.size() >= 2)
    {
        if (fadeInZone > 0.0f && phase < fadeInZone)
        {
            const float fadeProgress = phase / fadeInZone;
            return PTDimmerWaveEngine::applyWaveShape(fadeProgress, waveParams.waveShape) * maxValue;
        }
        if (fadeOutZone > 0.0f && phase >= fadeInZone + sustainZone)
        {
            const float fadeProgress = (phase - fadeInZone - sustainZone) / fadeOutZone;
            return PTDimmerWaveEngine::applyWaveShape(1.0f - fadeProgress, waveParams.waveShape)
                    * maxValue;
        }
        if (sustainZone > 0.0f)
        {
            const float localPhase = (phase - fadeInZone) / sustainZone;
            return PTDimmerWaveEngine::sampleCustomCurve01(localPhase, waveParams.customCurve)
                    * maxValue;
        }
        return PTDimmerWaveEngine::sampleCustomCurve01(phase, waveParams.customCurve) * maxValue;
    }

    if (waveParams.waveShape == 1)
        return maxValue;

    if (fadeInZone > 0.0f && phase < fadeInZone)
    {
        const float fadeProgress = phase / fadeInZone;
        return PTDimmerWaveEngine::applyWaveShape(fadeProgress, waveParams.waveShape) * maxValue;
    }

    if (fadeOutZone > 0.0f && phase >= fadeInZone + sustainZone)
    {
        const float fadeProgress = (phase - fadeInZone - sustainZone) / fadeOutZone;
        return PTDimmerWaveEngine::applyWaveShape(1.0f - fadeProgress, waveParams.waveShape)
                * maxValue;
    }

    return maxValue;
}

float samplePosition1DAtPhase(float phase01, const PTTransitionPreset& preset,
                            const PTDimmerWaveParams& waveParams)
{
    const float phase = qBound(0.0f, phase01, 1.0f);

    if (waveParams.customCurveEnabled && waveParams.customCurve.size() >= 2)
        return customCurveBipolar(phase, waveParams);

    if (preset.position1DBuiltinMode == 1)
    {
        const float osc = oscillateBuiltinBipolar(phase, waveParams.waveShape);
        return osc * envelope01(phase, waveParams);
    }

    const float unipolar = sample1DMorphPacketUnipolar01(phase, waveParams);
    return unipolar * 2.0f - 1.0f;
}

float position1DOffsetForCycleProgress(float cycleProgressRad,
                                       const PTTransitionPreset& preset,
                                       const PTDimmerWaveParams& waveParams)
{
    const float twoPi = float(M_PI * 2.0);
    const int waveWidth = qBound(1, waveParams.waveWidth, 360);
    if (waveWidth >= 360)
        return samplePosition1DAtPhase(cycleProgressRad / twoPi, preset, waveParams);

    const float widthRad = (float(waveWidth) / 360.0f) * twoPi;
    const float windowStartRad = PTDimmerWaveEngine::convertOffsetDegrees(waveParams.startOffset);
    const float cycleRad = normalizeRad(cycleProgressRad);
    const float packetEndRad = windowStartRad + widthRad;
    const bool packetWraps = packetEndRad >= twoPi;
    const float packetEndMod = packetWraps ? packetEndRad - twoPi : packetEndRad;

    bool inPacket = false;
    if (!packetWraps)
        inPacket = cycleRad >= windowStartRad && cycleRad < packetEndRad;
    else
        inPacket = cycleRad >= windowStartRad || cycleRad < packetEndMod;

    if (inPacket)
    {
        float rel = cycleRad - windowStartRad;
        if (rel < 0.0f)
            rel += twoPi;
        return samplePosition1DAtPhase(rel / widthRad, preset, waveParams);
    }

    bool beforePacket = false;
    if (!packetWraps)
        beforePacket = cycleRad < windowStartRad;
    else
        beforePacket = cycleRad >= packetEndMod && cycleRad < windowStartRad;

    return beforePacket
            ? samplePosition1DAtPhase(0.0f, preset, waveParams)
            : samplePosition1DAtPhase(1.0f, preset, waveParams);
}

/** DMX path: same iterator window as dimmer wave / orbitPhaseFromIterator [0, widthRad). */
float position1DOffsetForIterator(float iteratorRad,
                                const PTTransitionPreset& preset,
                                const PTDimmerWaveParams& waveParams)
{
    const float twoPi = float(M_PI * 2.0);
    const float iter = normalizeRad(iteratorRad);
    const int waveWidth = qBound(1, waveParams.waveWidth, 360);
    if (waveWidth >= 360)
        return samplePosition1DAtPhase(iter / twoPi, preset, waveParams);

    const float widthRad = (float(waveWidth) / 360.0f) * twoPi;
    if (iter < widthRad)
        return samplePosition1DAtPhase(iter / widthRad, preset, waveParams);

    return samplePosition1DAtPhase(1.0f, preset, waveParams);
}

} // namespace

float PTPositionFxEngine::samplePosition1DOffset(float iteratorRad,
                                                 const PTTransitionPreset& preset,
                                                 const PTDimmerWaveParams& waveParams,
                                                 int headOffsetDeg)
{
    Q_UNUSED(headOffsetDeg)
    return position1DOffsetForIterator(iteratorRad, preset, waveParams);
}

float PTPositionFxEngine::sampleMotionAtCycleDeg(float cycleDeg, const PTTransitionPreset& preset,
                                                 const PTDimmerWaveParams& waveParams)
{
    const float deg = cycleDeg - std::floor(cycleDeg / 360.0f) * 360.0f;
    const float cycleProgressRad = deg / 360.0f * float(M_PI * 2.0);
    return position1DOffsetForCycleProgress(cycleProgressRad, preset, waveParams);
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
        case Shape::None:
            break;
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
        case PTPositionMotion::Pan1D:
        case PTPositionMotion::CustomPan1D:
        case PTPositionMotion::Tilt1D:
        case PTPositionMotion::CustomTilt1D:
            break;
        case PTPositionMotion::Custom2D:
        {
            const QVector<PTPositionPath2DPoint>& path = preset.positionPath2D.size() >= 2
                    ? preset.positionPath2D : QVector<PTPositionPath2DPoint>();
            if (path.size() < 2)
                return;
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

PTPositionValue PTPositionFxEngine::applySmartMotion(const PTPositionValue& base, Fixture* fxi,
                                                     int head, Shape shape, double phaseRadians,
                                                     qreal size01)
{
    if (!base.valid || !fxi)
        return base;

    size01 = qBound(0.0, size01, 1.0);
    if (size01 <= 0)
        return PTPositionConverter::clampPosition(fxi, head, base);
    if (shape == Shape::None)
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
                                                               double phaseRadians, qreal size01,
                                                               float iteratorRad,
                                                               const PTDimmerWaveParams* waveParams,
                                                               const PTDimmerWaveOffsetInfo* spatial,
                                                               int headOffsetDeg)
{
    if (!base.valid || !fxi)
        return base;

    size01 = qBound(0.0, size01, 1.0);
    if (size01 <= 0)
        return PTPositionConverter::clampPosition(fxi, head, base);

    const PTPositionMotion motion = PTPositionMotion(preset.positionMotion);
    const PTPositionValue clampedBase = PTPositionConverter::clampPosition(fxi, head, base);
    if (motion == PTPositionMotion::Off)
        return clampedBase;
    const QRectF range = PTPositionConverter::degreesRange(fxi, head);
    const qreal panMin = range.left();
    const qreal panMax = range.left() + range.width();
    const qreal tiltMin = range.top();
    const qreal tiltMax = range.top() + range.height();

    qreal panUnit = 0;
    qreal tiltUnit = 0;
    if (motionIs1D(motion) && waveParams != nullptr && iteratorRad >= 0.0f)
    {
        float unit = samplePosition1DOffset(iteratorRad, preset, *waveParams, headOffsetDeg);
        if (spatial != nullptr)
        {
            unit = applyDirectionToUnitOffset(unit,
                    PTPositionMotionDirection(preset.positionMotionDirection), *spatial);
        }
        if (motion == PTPositionMotion::Pan1D || motion == PTPositionMotion::CustomPan1D)
            panUnit = qreal(unit);
        else
            tiltUnit = qreal(unit);
    }
    else
    {
        relativeOffsetForPreset(preset, phaseRadians, 1.0, 1.0, panUnit, tiltUnit);
    }

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

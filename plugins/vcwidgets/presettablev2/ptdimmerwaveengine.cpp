/*
  Port of QLC+ EFX::DimmerWave math for Preset Table v2 plugin.
*/

#include "ptdimmerwaveengine.h"
#include "ptparammatrixengine.h"

#include <QPointF>
#include <QtMath>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int ceilDivPositive(int numerator, int denominator)
{
    return int(std::ceil(double(qMax(1, numerator)) / double(qMax(1, denominator))));
}

PTDimmerWaveParams PTDimmerWaveEngine::paramsFromPreset(const PTTransitionPreset& preset,
                                                        const PTGlobalEffectSettings* global)
{
    PTDimmerWaveParams p;
    p.durationMs = int(preset.durationMs);
    p.waveWidth = preset.waveWidth;
    p.waveShape = preset.waveShape;
    p.waveFadeIn = preset.waveFadeIn;
    p.waveFadeOut = preset.waveFadeOut;
    p.waveLevel = preset.waveLevel;
    p.startOffset = preset.startOffset;
    p.customCurveEnabled = preset.customCurveEnabled;
    p.customCurve = preset.customCurve;
    p.offsetStep = preset.offsetStep;
    p.offsetStepMode = preset.offsetStepMode;
    p.offsetCoverage = preset.offsetCoverage;
    p.wings = preset.wings;
    p.blocks = preset.blocks > 0 ? preset.blocks : 1;
    p.wingsSymmetry = preset.wingsSymmetry;
    p.axis = preset.axis;
    p.offsetDirection = preset.offsetDirection;
    p.propagation = preset.propagation;

    if (global && preset.wingsSymmetry == 0 && global->fxWingsSymmetry != 0)
        p.wingsSymmetry = global->fxWingsSymmetry;

    if (global)
    {
        if (preset.blocks <= 1 && global->fxBlocks > 1)
            p.blocks = qMax(1, global->fxBlocks);

        const int phaseOffsetDeg = int(std::floor((double(global->fxPhaseOffset) / 255.0) * 360.0));
        p.startOffset = (p.startOffset + phaseOffsetDeg) % 360;

        const double mult = PTParamMatrixEngine::fxMultiplierValue(global->fxMultiplier);
        if (p.offsetStepMode == PTOffsetStepMode::FixedDegrees && p.offsetStep != 0)
            p.offsetStep = qMax(1, int(std::round(double(p.offsetStep) * mult)));
    }

    return p;
}

int PTDimmerWaveEngine::evenOffsetStepForSpan(int span)
{
    if (span <= 1)
        return 1;
    return qMax(1, 360 / span);
}

static int scalableOffsetStepForSlots(
        int slotsPerWing, PTDimmerWaveEngine::OffsetDistributionPolicy policy)
{
    if (slotsPerWing <= 1)
        return 0;
    if (policy == PTDimmerWaveEngine::OffsetDistributionPolicy::NonWrappingSweep)
        return qMax(1, 360 / (slotsPerWing - 1));
    return qMax(1, 360 / slotsPerWing);
}

int PTDimmerWaveEngine::gridSpanAlongAxis(int gridWidth, int gridHeight, PTTransitionAxis axis,
                                          int fxOrientation)
{
    PTTransitionAxis useAxis = axis;
    if (fxOrientation == 1)
        useAxis = PTTransitionAxis::Y;

    const PTDimmerWaveSpatialSpan s = spatialSpanForPoint(
            qMax(0, gridWidth - 1), qMax(0, gridHeight - 1),
            qMax(1, gridWidth), qMax(1, gridHeight), useAxis);
    return qMax(1, s.span);
}

int PTDimmerWaveEngine::effectiveOffsetSlotCount(int gridSpanAlongAxis,
                                                 const PTTransitionPreset& preset)
{
    const int span = qMax(1, gridSpanAlongAxis);
    const int wings = qBound(1, preset.wings, span);
    return qMax(1, wings * offsetSlotCountForWing(span, preset));
}

int PTDimmerWaveEngine::offsetSlotCountForWing(int gridSpanAlongAxis,
                                               const PTTransitionPreset& preset)
{
    const int span = qMax(1, gridSpanAlongAxis);
    const int wings = qBound(1, preset.wings, span);
    const int blocks = qMax(1, preset.blocks);
    const int positionsPerWing = ceilDivPositive(span, wings);
    return qMax(1, ceilDivPositive(positionsPerWing, blocks));
}

int PTDimmerWaveEngine::maxOffsetStepForGrid(int gridSpanAlongAxis,
                                             const PTTransitionPreset& preset,
                                             OffsetDistributionPolicy policy)
{
    const int slotsPerWing = offsetSlotCountForWing(gridSpanAlongAxis, preset);
    const int step = scalableOffsetStepForSlots(slotsPerWing, policy);
    return step == 0 ? 360 : step;
}

int PTDimmerWaveEngine::effectiveOffsetStepForSpan(int gridSpanAlongAxis,
                                                   const PTTransitionPreset& preset,
                                                   OffsetDistributionPolicy policy)
{
    const int slotsPerWing = offsetSlotCountForWing(gridSpanAlongAxis, preset);
    const int scalableMax = scalableOffsetStepForSlots(slotsPerWing, policy);
    switch (preset.offsetStepMode)
    {
        case PTOffsetStepMode::Off:
            return 0;
        case PTOffsetStepMode::AutoFit:
            return scalableMax;
        case PTOffsetStepMode::CoveragePercent:
            return qBound(0, int(std::round(double(scalableMax)
                                            * double(qBound(0, preset.offsetCoverage, 100))
                                            / 100.0)), scalableMax);
        case PTOffsetStepMode::FixedDegrees:
            if (preset.offsetStep == 0)
                return 0;
            return qBound(0, preset.offsetStep, maxOffsetStepForGrid(gridSpanAlongAxis, preset));
    }
    return 0;
}

PTTransitionPreset PTDimmerWaveEngine::normalizedTransitionSweepPreset(
        PTTransitionPreset preset)
{
    preset.offsetStepMode = PTOffsetStepMode::AutoFit;
    preset.offsetCoverage = 100;
    preset.offsetStep = 0;
    return preset;
}

void PTDimmerWaveEngine::clampOffsetStep(PTTransitionPreset& preset, int gridSpanAlongAxis)
{
    preset.offsetCoverage = qBound(0, preset.offsetCoverage, 100);
    if (gridSpanAlongAxis <= 0)
        return;
    if (preset.offsetStepMode != PTOffsetStepMode::FixedDegrees)
        return;
    if (preset.offsetStep == 0)
        return;
    preset.offsetStep = qBound(0, preset.offsetStep, maxOffsetStepForGrid(gridSpanAlongAxis, preset));
}

PTDimmerWaveSpatialSpan PTDimmerWaveEngine::spatialSpanForPoint(int col, int row, int gridWidth,
                                                                int gridHeight, PTTransitionAxis axis)
{
    PTDimmerWaveSpatialSpan s;
    s.span = qMax(1, gridWidth);
    s.position = col;
    if (axis == PTTransitionAxis::Y)
    {
        s.span = qMax(1, gridHeight);
        s.position = row;
    }
    else if (axis == PTTransitionAxis::XY)
    {
        const int w = qMax(1, gridWidth);
        s.span = qMax(1, w * qMax(1, gridHeight));
        s.position = row * w + col;
    }
    return s;
}

static int spatialIndexFromOffsetDirection(PTOffsetDirection dir)
{
    switch (dir)
    {
        case PTOffsetDirection::RightToLeft:    return 1;
        case PTOffsetDirection::CenterToSides: return 2;
        case PTOffsetDirection::SidesToCenter: return 3;
        case PTOffsetDirection::Alternate:    return 4;
        case PTOffsetDirection::Symmetric:    return 5;
        case PTOffsetDirection::Random:       return 6;
        default:                              return 0;
    }
}

static PTOffsetDirection offsetDirectionFromSpatialIndex(int index)
{
    switch (index)
    {
        case 1:  return PTOffsetDirection::RightToLeft;
        case 2:  return PTOffsetDirection::CenterToSides;
        case 3:  return PTOffsetDirection::SidesToCenter;
        case 4:  return PTOffsetDirection::Alternate;
        case 5:  return PTOffsetDirection::Symmetric;
        case 6:  return PTOffsetDirection::Random;
        default: return PTOffsetDirection::LeftToRight;
    }
}

PTOffsetDirection PTDimmerWaveEngine::wingOffsetDirection(int wingIndex, PTOffsetDirection baseDirection,
                                                          int wingsSymmetry, int totalWings)
{
    if (wingsSymmetry == 0 || totalWings <= 0)
        return baseDirection;
    if (baseDirection == PTOffsetDirection::Random)
        return baseDirection;

    int dir = spatialIndexFromOffsetDirection(baseDirection);

    if (wingsSymmetry == 1)
    {
        if (wingIndex % 2 == 1)
            dir = PTParamMatrixEngine::reverseSpatialDirection(dir);
    }
    else if (wingsSymmetry == 2)
    {
        const int half = int(std::ceil(double(totalWings) / 2.0));
        if (wingIndex >= half)
            dir = PTParamMatrixEngine::reverseSpatialDirection(dir);
    }

    return offsetDirectionFromSpatialIndex(dir);
}

float PTDimmerWaveEngine::applyWaveShape(float input, int shape)
{
    switch (shape)
    {
        case 0:
            return float((1.0 - cos(double(input) * M_PI)) / 2.0);
        case 1:
            return input > 0.5f ? 1.0f : 0.0f;
        case 2:
        default:
            return input;
    }
}

static QPointF cubicPoint(const QPointF& p0, const QPointF& c1,
                          const QPointF& c2, const QPointF& p1, double t)
{
    const double mt = 1.0 - t;
    return p0 * (mt * mt * mt)
            + c1 * (3.0 * mt * mt * t)
            + c2 * (3.0 * mt * t * t)
            + p1 * (t * t * t);
}

float PTDimmerWaveEngine::sampleCustomCurve01(float phase01,
                                              const QVector<PTCustomCurvePoint>& points)
{
    if (points.size() < 2)
        return 0.0f;

    QVector<PTCustomCurvePoint> sorted = points;
    std::sort(sorted.begin(), sorted.end(),
              [](const PTCustomCurvePoint& a, const PTCustomCurvePoint& b) {
                  return a.xDeg < b.xDeg;
              });

    const double x = qBound(0.0, phase01, 1.0) * 360.0;
    if (x <= sorted.first().xDeg)
        return float(qBound(0.0, sorted.first().yValue / 255.0, 1.0));
    if (x >= sorted.last().xDeg)
        return float(qBound(0.0, sorted.last().yValue / 255.0, 1.0));

    for (int i = 0; i < sorted.size() - 1; ++i)
    {
        const PTCustomCurvePoint& a = sorted.at(i);
        const PTCustomCurvePoint& b = sorted.at(i + 1);
        if (x < a.xDeg || x > b.xDeg)
            continue;

        const QPointF p0(a.xDeg, a.yValue);
        const QPointF p1(b.xDeg, b.yValue);

        if (a.segmentMode == PTCustomCurvePoint::Linear)
        {
            const double span = qMax(0.001, b.xDeg - a.xDeg);
            const double t = qBound(0.0, (x - a.xDeg) / span, 1.0);
            const double y = a.yValue + (b.yValue - a.yValue) * t;
            return float(qBound(0.0, y / 255.0, 1.0));
        }

        const QPointF c1(a.rightHandleXDeg, a.rightHandleYValue);
        const QPointF c2(b.leftHandleXDeg, b.leftHandleYValue);

        double lo = 0.0;
        double hi = 1.0;
        for (int n = 0; n < 18; ++n)
        {
            const double mid = (lo + hi) * 0.5;
            if (cubicPoint(p0, c1, c2, p1, mid).x() < x)
                lo = mid;
            else
                hi = mid;
        }
        const QPointF pt = cubicPoint(p0, c1, c2, p1, (lo + hi) * 0.5);
        return float(qBound(0.0, pt.y() / 255.0, 1.0));
    }
    return 0.0f;
}

float PTDimmerWaveEngine::dimmerAtPhaseInWidth(float phaseInWidth, const PTDimmerWaveParams& params)
{
    const float maxValue = float(params.waveLevel) / 255.0f;
    const float phase = qBound(0.0f, phaseInWidth, 1.0f);

    if (params.customCurveEnabled && params.customCurve.size() >= 2)
        return sampleCustomCurve01(phase, params.customCurve) * maxValue;

    // Square: hard on/off inside wave width (no fade-in/out ramp on packet edges).
    if (params.waveShape == 1)
        return maxValue;

    // Match engine/src/efx.cpp DimmerWave: 0% fade zone = sharp edge, not "blocked".
    const float fadeInZone = float(qBound(0, params.waveFadeIn, 100)) / 100.0f;
    const float fadeOutZone = float(qBound(0, params.waveFadeOut, 100)) / 100.0f;
    const float sustainZone = qMax(0.0f, 1.0f - fadeInZone - fadeOutZone);

    if (fadeInZone > 0.0f && phase < fadeInZone)
    {
        const float fadeProgress = phase / fadeInZone;
        return applyWaveShape(fadeProgress, params.waveShape) * maxValue;
    }
    if (phase < fadeInZone + sustainZone)
        return maxValue;
    if (fadeOutZone > 0.0f)
    {
        const float fadeProgress = (phase - fadeInZone - sustainZone) / fadeOutZone;
        return applyWaveShape(1.0f - fadeProgress, params.waveShape) * maxValue;
    }
    // No fade-out zone — sustain to end of packet (QLC else branch).
    return maxValue;
}

float PTDimmerWaveEngine::dimmerSweepAttack01(float phaseInWindow01, const PTDimmerWaveParams& params)
{
    PTDimmerWaveParams attack = params;
    attack.waveFadeOut = 0;
    return dimmerAtPhaseInWidth(phaseInWindow01, attack);
}

float PTDimmerWaveEngine::calculateDimmerWave(float iteratorRad, const PTDimmerWaveParams& params)
{
    const float widthRadians = (float(params.waveWidth) / 360.0f) * float(M_PI * 2.0);
    if (widthRadians <= 0.0f || iteratorRad >= widthRadians)
        return 0.0f;

    const float phaseInWidth = iteratorRad / widthRadians;
    return dimmerAtPhaseInWidth(phaseInWidth, params);
}

float PTDimmerWaveEngine::sampleDimmerCycle01(float degrees, const PTDimmerWaveParams& params)
{
    const float deg = degrees - std::floor(degrees / 360.0f) * 360.0f;
    const float iteratorRad = (deg / 360.0f) * float(M_PI * 2.0);
    return calculateDimmerWave(iteratorRad, params);
}

float PTDimmerWaveEngine::convertOffsetDegrees(int offsetDegrees)
{
    return float(M_PI / 180.0) * float(offsetDegrees % 360);
}

float PTDimmerWaveEngine::iteratorFromElapsed(quint32 elapsedMs, quint32 durationMs,
                                              int startOffsetDeg, int headOffsetDeg,
                                              quint32 serialTimeOffsetMs)
{
    if (durationMs == 0)
        return 0.0f;

    const quint32 pos = (elapsedMs + serialTimeOffsetMs) % durationMs;
    float iterator = float(pos) / float(durationMs) * float(M_PI * 2.0);
    iterator += convertOffsetDegrees(headOffsetDeg + startOffsetDeg);
    if (iterator >= float(M_PI * 2.0))
        iterator -= float(M_PI * 2.0);
    return iterator;
}

static int positiveGcd(int a, int b)
{
    a = qAbs(a);
    b = qAbs(b);
    while (b != 0)
    {
        const int t = a % b;
        a = b;
        b = t;
    }
    return qMax(1, a);
}

static int stableRandomOffsetIndex(int position, int span)
{
    if (span <= 1)
        return 0;

    int step = qMax(1, span / 2 + 1);
    while (step < span && positiveGcd(step, span) != 1)
        ++step;
    if (step >= span)
        step = span - 1;

    const int offset = (span * 37 + 11) % span;
    return (qBound(0, position, span - 1) * step + offset) % span;
}

static int templateOffsetIndex(int position, int span, PTOffsetDirection direction)
{
    if (span <= 0)
        return 0;

    int index = 0;
    switch (direction)
    {
        case PTOffsetDirection::LeftToRight:
            index = position;
            break;
        case PTOffsetDirection::RightToLeft:
            index = (span - 1) - position;
            break;
        case PTOffsetDirection::CenterToSides:
            if (span % 2 == 0)
            {
                const int centerRight = span / 2;
                const int centerLeft = centerRight - 1;
                index = qMin(qAbs(position - centerLeft), qAbs(position - centerRight));
            }
            else
                index = qAbs(position - span / 2);
            break;
        case PTOffsetDirection::SidesToCenter:
            index = qMin(position, (span - 1) - position);
            break;
        case PTOffsetDirection::Alternate:
        {
            const int half = (span + 1) / 2;
            index = (position % 2 == 0) ? (position / 2) : (half + (position / 2));
        }
        break;
        case PTOffsetDirection::Symmetric:
        {
            const int center = span / 2;
            index = (position <= center) ? position : (span - 1 - position);
        }
        break;
        case PTOffsetDirection::Random:
            index = stableRandomOffsetIndex(position, span);
            break;
    }
    return index;
}

int PTDimmerWaveEngine::calculateHeadStartOffsetExtended(int col, int row, int gridWidth, int gridHeight,
                                                         const PTDimmerWaveParams& params)
{
    return offsetInfoForPoint(col, row, gridWidth, gridHeight, params).headOffsetDeg;
}

PTDimmerWaveOffsetInfo PTDimmerWaveEngine::offsetInfoForPoint(
        int col, int row, int gridWidth, int gridHeight, const PTDimmerWaveParams& params,
        OffsetDistributionPolicy policy)
{
    PTDimmerWaveOffsetInfo info;
    const PTDimmerWaveSpatialSpan spatial = spatialSpanForPoint(col, row, gridWidth, gridHeight, params.axis);
    const int span = qMax(1, spatial.span);
    const int position = qBound(0, spatial.position, span - 1);

    const int wings = qBound(1, params.wings, span);
    const int blocks = qMax(1, params.blocks);

    const int positionsPerWing = ceilDivPositive(span, wings);
    const int wingIndex = qMin(wings - 1, position / positionsPerWing);
    const int localIndex = qMax(0, position - wingIndex * positionsPerWing);
    const int blocksPerWing = qMax(1, ceilDivPositive(positionsPerWing, blocks));
    const int blockIndex = qMin(blocksPerWing - 1, localIndex / blocks);

    const PTOffsetDirection wingDir = wingOffsetDirection(wingIndex, params.offsetDirection,
                                                          params.wingsSymmetry, wings);
    const int index = templateOffsetIndex(blockIndex, qMax(1, blocksPerWing), wingDir);
    info.wingIndex = wingIndex;
    info.localIndex = localIndex;
    info.blockIndex = blockIndex;
    info.localOrder = localIndex + 1;
    info.offsetSlot = index;
    info.slotsPerWing = blocksPerWing;
    int step = params.offsetStep;
    switch (params.offsetStepMode)
    {
        case PTOffsetStepMode::Off:
            step = 0;
            break;
        case PTOffsetStepMode::AutoFit:
            step = scalableOffsetStepForSlots(blocksPerWing, policy);
            break;
        case PTOffsetStepMode::CoveragePercent:
        {
            const int maxStep = scalableOffsetStepForSlots(blocksPerWing, policy);
            step = qBound(0, int(std::round(double(maxStep)
                                            * double(qBound(0, params.offsetCoverage, 100))
                                            / 100.0)), maxStep);
            break;
        }
        case PTOffsetStepMode::FixedDegrees:
            if (step < 0)
                step = evenOffsetStepForSpan(span);
            break;
    }

    if (step == 0)
    {
        info.headOffsetDeg = 0;
        info.phaseStart01 = 0.0;
    }
    else if (policy == OffsetDistributionPolicy::NonWrappingSweep)
    {
        const double phase = blocksPerWing <= 1
                ? 0.0
                : qBound(0.0, double(index) / double(blocksPerWing - 1), 1.0);
        info.phaseStart01 = phase;
        info.headOffsetDeg = int(std::round(phase * 360.0));
    }
    else
    {
        info.headOffsetDeg = (step * index) % 360;
        info.phaseStart01 = double(info.headOffsetDeg) / 360.0;
    }
    return info;
}

int PTDimmerWaveEngine::calculateHeadStartOffset(int col, int row, int gridWidth, int gridHeight,
                                                 PTTransitionAxis axis, int wings, int offsetStep,
                                                 PTOffsetDirection direction)
{
    PTDimmerWaveParams p;
    p.axis = axis;
    p.wings = wings;
    p.offsetStep = offsetStep;
    p.offsetStepMode = PTOffsetStepMode::FixedDegrees;
    p.offsetDirection = direction;
    p.blocks = 1;
    return calculateHeadStartOffsetExtended(col, row, gridWidth, gridHeight, p);
}

quint32 PTDimmerWaveEngine::serialTimeOffsetMs(int serialIndex, int fixtureCount,
                                               quint32 durationMs, PTPropagationMode propagation)
{
    if (propagation != PTPropagationMode::Serial || fixtureCount <= 0 || durationMs == 0)
        return 0;
    return durationMs / quint32(fixtureCount + 1) * quint32(serialIndex);
}

double PTDimmerWaveEngine::phase01AtCycleStart(quint32 cycleMs, const PTDimmerWaveParams& params,
                                               int headOffsetDeg, int serialIndex, int serialCount)
{
    cycleMs = qMax(quint32(1), cycleMs);
    const quint32 timeOffset = serialTimeOffsetMs(serialIndex, serialCount, cycleMs,
                                                  params.propagation);
    const float iterator = iteratorFromElapsed(0, cycleMs, params.startOffset, headOffsetDeg,
                                               timeOffset);
    return double(iterator) / (2.0 * M_PI);
}

QString PTDimmerWaveEngine::waveShapeToString(int shape)
{
    switch (shape)
    {
        case 1:  return QStringLiteral("Square");
        case 2:  return QStringLiteral("Triangle");
        default: return QStringLiteral("Sine");
    }
}

int PTDimmerWaveEngine::waveShapeFromString(const QString& s)
{
    if (s == QLatin1String("Square"))   return 1;
    if (s == QLatin1String("Triangle")) return 2;
    return 0;
}

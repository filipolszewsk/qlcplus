/*
  Port of QLC+ EFX::DimmerWave math for Preset Table v2 plugin.
*/

#include "ptdimmerwaveengine.h"
#include "ptparammatrixengine.h"

#include <QtMath>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    p.offsetStep = preset.offsetStep;
    p.wings = preset.wings;
    p.blocks = preset.blocks > 0 ? preset.blocks : 1;
    p.wingsSymmetry = preset.wingsSymmetry;
    p.axis = preset.axis;
    p.offsetDirection = preset.offsetDirection;
    p.propagation = preset.propagation;

    if (global && preset.wingsSymmetry == 0 && global->fxWingsSymmetry != 0)
        p.wingsSymmetry = global->fxWingsSymmetry;

    return p;
}

int PTDimmerWaveEngine::evenOffsetStepForSpan(int span)
{
    if (span <= 1)
        return 1;
    return 360 / (span - 1);
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
    const int blocks = qMax(1, preset.blocks);
    const int ppw = qMax(1, span / wings);
    const int blocksPerWing = int(std::ceil(double(ppw) / double(blocks)));
    return qMax(1, wings * blocksPerWing);
}

int PTDimmerWaveEngine::maxOffsetStepForGrid(int gridSpanAlongAxis,
                                             const PTTransitionPreset& preset)
{
    const int slotCount = effectiveOffsetSlotCount(gridSpanAlongAxis, preset);
    return qMax(1, 360 / slotCount);
}

void PTDimmerWaveEngine::clampOffsetStep(PTTransitionPreset& preset, int gridSpanAlongAxis)
{
    if (gridSpanAlongAxis <= 0)
        return;
    preset.offsetStep = qBound(1, preset.offsetStep, maxOffsetStepForGrid(gridSpanAlongAxis, preset));
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
        default: return PTOffsetDirection::LeftToRight;
    }
}

PTOffsetDirection PTDimmerWaveEngine::wingOffsetDirection(int wingIndex, PTOffsetDirection baseDirection,
                                                          int wingsSymmetry, int totalWings)
{
    if (wingsSymmetry == 0 || totalWings <= 0)
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

float PTDimmerWaveEngine::dimmerAtPhaseInWidth(float phaseInWidth, const PTDimmerWaveParams& params)
{
    const float maxValue = float(params.waveLevel) / 255.0f;
    const float phase = qBound(0.0f, phaseInWidth, 1.0f);

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
    }
    return index;
}

int PTDimmerWaveEngine::calculateHeadStartOffsetExtended(int col, int row, int gridWidth, int gridHeight,
                                                         const PTDimmerWaveParams& params)
{
    const PTDimmerWaveSpatialSpan spatial = spatialSpanForPoint(col, row, gridWidth, gridHeight, params.axis);
    const int span = qMax(1, spatial.span);
    const int position = qBound(0, spatial.position, span - 1);

    const int wings = qBound(1, params.wings, span);
    const int blocks = qMax(1, params.blocks);

    int step = params.offsetStep;
    if (step <= 0)
        step = evenOffsetStepForSpan(span);

    const int ppw = qMax(1, span / wings);
    const int wingIndex = qMin(wings - 1, position / ppw);
    const int localIndex = position - wingIndex * ppw;
    const int blocksPerWing = int(std::ceil(double(ppw) / double(blocks)));
    const int blockIndex = qMin(blocksPerWing - 1, localIndex / blocks);

    const PTOffsetDirection wingDir = wingOffsetDirection(wingIndex, params.offsetDirection,
                                                          params.wingsSymmetry, wings);
    const int index = templateOffsetIndex(blockIndex, qMax(1, blocksPerWing), wingDir);
    return (step * index) % 360;
}

int PTDimmerWaveEngine::calculateHeadStartOffset(int col, int row, int gridWidth, int gridHeight,
                                                 PTTransitionAxis axis, int wings, int offsetStep,
                                                 PTOffsetDirection direction)
{
    PTDimmerWaveParams p;
    p.axis = axis;
    p.wings = wings;
    p.offsetStep = offsetStep;
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

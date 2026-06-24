/*
  QLC+ VC Widget Plugin — Preset Table v2
  ptparammatrixengine.cpp
*/

#include "ptparammatrixengine.h"

#include "ptdimmerwaveengine.h"
#include "ptspatialfixtureplan.h"
#include "mastertimer.h"

#include <QtMath>
#include <algorithm>

static double smoothStep01(double x)
{
    x = qBound(0.0, x, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

double PTParamMatrixEngine::speedMultiplierValue(int index)
{
    static const double values[] = { 0.5, 1.0, 2.0, 3.0, 4.0, 5.0 };
    if (index < 0)
        return 1.0;
    if (index >= 6)
        return 5.0;
    return values[index];
}

double PTParamMatrixEngine::fxMultiplierValue(int index)
{
    static const double values[] = { 0.5, 1.0, 2.0 };
    if (index < 0)
        return 1.0;
    if (index >= 3)
        return 2.0;
    return values[index];
}

quint32 PTParamMatrixEngine::effectiveDurationMs(const PTGlobalEffectSettings& global,
                                                const PTTransitionPreset& preset,
                                                bool honorPresetDuration)
{
    if (honorPresetDuration && preset.durationMs >= 20)
        return preset.durationMs;

    const double speedProgress = double(global.speed) / 255.0;
    const quint32 minMs = qMin(global.minDurationMs, global.maxDurationMs);
    const quint32 maxMs = qMax(global.minDurationMs, global.maxDurationMs);
    // speed 255 = min (fast), speed 0 = max (slow) — one full 360° phase
    const double baseMs = double(minMs) + (1.0 - speedProgress) * double(maxMs - minMs);
    double cycleMs = qMax(20.0, baseMs);
    if (global.sizeSpeedCeilingEnabled)
    {
        const int knee = qBound(1, global.speedOverdriveKnee, 254);
        if (int(global.speed) > knee)
        {
            const double knee01 = double(knee) / 255.0;
            const double overdrive01 = smoothStep01((speedProgress - knee01) / (1.0 - knee01));
            const double sizeUnlock01 = 1.0 - (double(global.positionSize) / 255.0);
            const double smallMinMs = double(qMax(quint32(20), global.smallSizeMinDurationMs));
            const double fullMinMs = double(qMax(quint32(20), minMs));
            const double overdriveMinMs = qMin(fullMinMs,
                    fullMinMs + (smallMinMs - fullMinMs) * qBound(0.0, sizeUnlock01, 1.0));
            cycleMs = qMax(20.0, baseMs + (overdriveMinMs - baseMs) * overdrive01);
        }
    }
    const double mult = speedMultiplierValue(preset.speedMultiplier);
    if (mult > 0.0)
        cycleMs /= mult;
    return quint32(cycleMs);
}

double PTParamMatrixEngine::transitionIncrement(const PTGlobalEffectSettings& global,
                                              const PTTransitionPreset& preset)
{
    const quint32 durationMs = qMax(quint32(1), effectiveDurationMs(global, preset));
    const double stepDurationS = double(MasterTimer::tick()) / 1000.0;
    const double targetDurationS = double(durationMs) / 1000.0;
    const double baseIncrement = stepDurationS / targetDurationS;
    return baseIncrement * speedMultiplierValue(preset.speedMultiplier);
}

double PTParamMatrixEngine::fixturePhaseOffset(int fixtureIndex, int fixtureCount, int direction)
{
    if (fixtureCount <= 0)
        return 0.0;

    const double count = double(fixtureCount);
    switch (direction)
    {
        case 1:
            return double(fixtureCount - 1 - fixtureIndex) / count;
        case 2:
        {
            const double center = count / 2.0;
            const double dist = qAbs(double(fixtureIndex) - center + 0.5);
            return dist / count;
        }
        case 3:
        {
            const double center = count / 2.0;
            const double dist = qAbs(double(fixtureIndex) - center + 0.5);
            return (center - dist) / count;
        }
        default:
            return double(fixtureIndex) / count;
    }
}

bool PTParamMatrixEngine::waveShowNew(int fixtureIndex, int fixtureCount, double progress,
                                      int transitionDirection)
{
    if (transitionDirection <= 0)
        return true;

    const int fxDir = transitionDirection - 1;
    const double phaseOffset = fixturePhaseOffset(fixtureIndex, fixtureCount, fxDir);
    return phaseOffset <= progress;
}

double PTParamMatrixEngine::fxBrightness(int phase, int totalSteps, int fadeIn255, int fadeOut255,
                                         int wavelength255)
{
    if (totalSteps <= 0 || phase < 0)
        return 0.0;

    const double wavelengthFraction = 0.05 + (double(wavelength255) / 255.0) * 0.95;
    const int activeWindowSteps = int(std::floor(double(totalSteps) * wavelengthFraction));
    if (phase >= activeWindowSteps)
        return 0.0;

    const int windowDuration = activeWindowSteps;
    int fadeInSteps = int(std::floor((double(fadeIn255) / 255.0) * windowDuration));
    int fadeOutSteps = int(std::floor((double(fadeOut255) / 255.0) * windowDuration));

    const int totalFade = fadeInSteps + fadeOutSteps;
    if (totalFade > windowDuration && totalFade > 0)
    {
        const double scale = double(windowDuration) / double(totalFade);
        fadeInSteps = int(std::floor(fadeInSteps * scale));
        fadeOutSteps = int(std::floor(fadeOutSteps * scale));
    }

    const int sustainEnd = windowDuration - fadeOutSteps;

    if (fadeInSteps > 0 && phase < fadeInSteps)
        return (double(phase) / double(fadeInSteps)) * 255.0;
    if (fadeOutSteps > 0 && phase >= sustainEnd)
    {
        const int fadePhase = phase - sustainEnd;
        return 255.0 * (1.0 - double(fadePhase) / double(fadeOutSteps));
    }
    return 255.0;
}

int PTParamMatrixEngine::reverseSpatialDirection(int direction)
{
    switch (direction)
    {
        case 0: return 1;
        case 1: return 0;
        case 2: return 3;
        case 3: return 2;
        default: return direction;
    }
}

int PTParamMatrixEngine::wingSpatialDirection(int wingIndex, int baseDirection, int symmetry,
                                              int totalWings)
{
    switch (symmetry)
    {
        case 1:
            if (wingIndex % 2 == 1)
                return reverseSpatialDirection(baseDirection);
            return baseDirection;
        case 2:
        {
            const int half = int(std::ceil(double(totalWings) / 2.0));
            if (wingIndex < half)
                return reverseSpatialDirection(baseDirection);
            return baseDirection;
        }
        default:
            return baseDirection;
    }
}

void PTParamMatrixEngine::buildFxContext(PTMatrixFxFrameContext& ctx,
                                         const PTGlobalEffectSettings& global,
                                         const PTTransitionPreset& preset,
                                         int gridWidth, int gridHeight, quint32 fxStep)
{
    const quint32 cycleMs = effectiveDurationMs(global, preset);
    ctx.stepsPerCycle = int(std::round(double(cycleMs) / double(kPTEfxStepMs))) + 1;
    if (ctx.stepsPerCycle < 1)
        ctx.stepsPerCycle = 1;

    ctx.vertical = (global.fxOrientation == 1);
    if (ctx.vertical)
        ctx.fixtureCount = qMax(1, gridHeight);
    else if (preset.axis == PTTransitionAxis::Y)
        ctx.fixtureCount = qMax(1, gridHeight);
    else if (preset.axis == PTTransitionAxis::XY)
        ctx.fixtureCount = qMax(1, gridWidth * gridHeight);
    else
        ctx.fixtureCount = qMax(1, gridWidth);

    ctx.effectiveWings = qBound(1, preset.wings, ctx.fixtureCount);
    ctx.effectiveBlocks = qBound(1, preset.blocks, ctx.fixtureCount);
    ctx.positionsPerWing = qMax(1, ctx.fixtureCount / ctx.effectiveWings);
    ctx.blocksPerWing = int(std::ceil(double(ctx.positionsPerWing) / double(ctx.effectiveBlocks)));
    if (ctx.blocksPerWing < 1)
        ctx.blocksPerWing = 1;

    ctx.fxMultiplier = fxMultiplierValue(global.fxMultiplier);
    const int phaseOffsetSteps = int(std::floor((double(global.fxPhaseOffset) / 255.0)
                                                * ctx.stepsPerCycle));
    const int scaledStep = int(fxStep * ctx.fxMultiplier) + phaseOffsetSteps;
    ctx.stepOffset = ctx.stepsPerCycle > 0 ? scaledStep % ctx.stepsPerCycle : 0;

    switch (preset.offsetDirection)
    {
        case PTOffsetDirection::RightToLeft:
            ctx.fxDirection = 1;
            break;
        case PTOffsetDirection::CenterToSides:
            ctx.fxDirection = 2;
            break;
        case PTOffsetDirection::SidesToCenter:
            ctx.fxDirection = 3;
            break;
        default:
            ctx.fxDirection = 0;
            break;
    }
    ctx.wingsSymmetry = global.fxWingsSymmetry;
}

int PTParamMatrixEngine::linearPosition(const QLCPoint& pt, PTTransitionAxis axis, int gridWidth)
{
    switch (axis)
    {
        case PTTransitionAxis::Y:
            return pt.y();
        case PTTransitionAxis::XY:
            return pt.y() * qMax(1, gridWidth) + pt.x();
        default:
            return pt.x();
    }
}

int PTParamMatrixEngine::colorFxBrightnessAt(const PTMatrixFxFrameContext& ctx,
                                             const PTGlobalEffectSettings& global,
                                             const PTTransitionPreset& preset,
                                             int positionIndex)
{
    if (positionIndex >= ctx.fixtureCount)
        return 0;

    const int wingIndex = positionIndex / ctx.positionsPerWing;
    const int localIndex = positionIndex - wingIndex * ctx.positionsPerWing;
    const int wingDirection = wingSpatialDirection(wingIndex, ctx.fxDirection,
                                                   ctx.wingsSymmetry, ctx.effectiveWings);
    const int blockIndex = localIndex / qMax(1, ctx.effectiveBlocks);
    const double blockPhase = fixturePhaseOffset(blockIndex, ctx.blocksPerWing, wingDirection)
            * ctx.stepsPerCycle;
    int fixturePhase = ctx.stepOffset - int(blockPhase) + ctx.stepsPerCycle;
    fixturePhase %= ctx.stepsPerCycle;
    if (fixturePhase < 0)
        fixturePhase += ctx.stepsPerCycle;

    const int fadeIn = qBound(0, preset.waveFadeIn * 255 / 100, 255);
    const int fadeOut = qBound(0, preset.waveFadeOut * 255 / 100, 255);
    const int wavelength = qBound(0, preset.waveWidth * 255 / 360, 255);

    return int(fxBrightness(fixturePhase, ctx.stepsPerCycle, fadeIn, fadeOut, wavelength));
}

QVector<uchar> PTParamMatrixEngine::blendWithIntensity(const QVector<uchar>& values, uchar intensity)
{
    if (intensity >= 255)
        return values;

    const double factor = double(intensity) / 255.0;
    QVector<uchar> out = values;
    for (int i = 0; i < out.size(); ++i)
        out[i] = uchar(qBound(0, int(std::lround(out[i] * factor)), 255));
    return out;
}

PTOffsetDirection PTParamMatrixEngine::offsetFromGlobalDirection(int transitionDirection)
{
    switch (transitionDirection)
    {
        case 2:  return PTOffsetDirection::RightToLeft;
        case 3:  return PTOffsetDirection::CenterToSides;
        case 4:  return PTOffsetDirection::SidesToCenter;
        default: return PTOffsetDirection::LeftToRight;
    }
}

bool PTParamMatrixEngine::sweepInstantForOffset(PTOffsetDirection dir)
{
    return dir == PTOffsetDirection::Alternate || dir == PTOffsetDirection::Symmetric;
}

int PTParamMatrixEngine::waveFrontFromOffset(PTOffsetDirection dir)
{
    switch (dir)
    {
        case PTOffsetDirection::RightToLeft:    return 2;
        case PTOffsetDirection::CenterToSides: return 3;
        case PTOffsetDirection::SidesToCenter: return 4;
        case PTOffsetDirection::Alternate:
        case PTOffsetDirection::Symmetric:      return 0;
        case PTOffsetDirection::Random:         return 1;
        default:                                return 1;
    }
}

float PTParamMatrixEngine::crossfadeSweepBlend01(double globalProgress, double phaseStart01,
                                               const PTTransitionPreset& preset,
                                               const PTGlobalEffectSettings& global)
{
    return PTSpatialFixturePlan::sweepBlend01AtPhaseStart(
            globalProgress, phaseStart01, preset, global);
}

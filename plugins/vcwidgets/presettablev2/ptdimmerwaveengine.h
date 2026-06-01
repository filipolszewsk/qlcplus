/*
  Port of QLC+ EFX::DimmerWave math for Preset Table v2 plugin (no core EFX dependency).
*/

#pragma once

#include <QtGlobal>
#include <QString>
#include "presettablev2effectengine.h"

struct PTGlobalEffectSettings;

struct PTDimmerWaveParams
{
    int durationMs = 5000;
    int waveWidth = 180;
    int waveShape = 0;
    int waveFadeIn = 25;
    int waveFadeOut = 25;
    int waveLevel = 255;
    int startOffset = 0;
    int offsetStep = 20;
    int wings = 1;
    int blocks = 1;
    int wingsSymmetry = 0;
    PTTransitionAxis axis = PTTransitionAxis::X;
    PTOffsetDirection offsetDirection = PTOffsetDirection::LeftToRight;
    PTPropagationMode propagation = PTPropagationMode::Parallel;
};

struct PTDimmerWaveSpatialSpan
{
    int position = 0;
    int span     = 1;
};

struct PTDimmerWaveOffsetInfo
{
    int wingIndex = 0;
    int localIndex = 0;
    int blockIndex = 0;
    int localOrder = 1;
    int offsetSlot = 0;
    int slotsPerWing = 1;
    int headOffsetDeg = 0;
};

class PTDimmerWaveEngine
{
public:
    static PTDimmerWaveParams paramsFromPreset(const PTTransitionPreset& preset,
                                               const PTGlobalEffectSettings* global = nullptr);

    static int evenOffsetStepForSpan(int span);

    /** Span along preset axis (after global fxOrientation). 0 if grid invalid. */
    static int gridSpanAlongAxis(int gridWidth, int gridHeight, PTTransitionAxis axis,
                                 int fxOrientation);

    static int effectiveOffsetSlotCount(int gridSpanAlongAxis, const PTTransitionPreset& preset);
    static int offsetSlotCountForWing(int gridSpanAlongAxis, const PTTransitionPreset& preset);
    static int maxOffsetStepForGrid(int gridSpanAlongAxis, const PTTransitionPreset& preset);
    static void clampOffsetStep(PTTransitionPreset& preset, int gridSpanAlongAxis);

    static PTDimmerWaveSpatialSpan spatialSpanForPoint(int col, int row, int gridWidth, int gridHeight,
                                                       PTTransitionAxis axis);

    static PTOffsetDirection wingOffsetDirection(int wingIndex, PTOffsetDirection baseDirection,
                                                 int wingsSymmetry, int totalWings);

    static int calculateHeadStartOffsetExtended(int col, int row, int gridWidth, int gridHeight,
                                                const PTDimmerWaveParams& params);

    static PTDimmerWaveOffsetInfo offsetInfoForPoint(int col, int row, int gridWidth, int gridHeight,
                                                     const PTDimmerWaveParams& params);

    static float applyWaveShape(float input, int shape);

    /** Dimmer 0…1 inside wave packet; phase01 is 0…1 within active width (QLC DimmerWave). */
    static float dimmerAtPhaseInWidth(float phase01, const PTDimmerWaveParams& params);

    /** Sweep spatial: monotonic attack only (no fade-out dip on globalProgress). */
    static float dimmerSweepAttack01(float phaseInWindow01, const PTDimmerWaveParams& params);

    static float calculateDimmerWave(float iteratorRad, const PTDimmerWaveParams& params);

    /** Sample full 360° cycle: X = degrees 0…360, returns 0…1 dimmer. */
    static float sampleDimmerCycle01(float degrees, const PTDimmerWaveParams& params);

    static float convertOffsetDegrees(int offsetDegrees);
    static float iteratorFromElapsed(quint32 elapsedMs, quint32 durationMs,
                                     int startOffsetDeg, int headOffsetDeg,
                                     quint32 serialTimeOffsetMs);

    static int calculateHeadStartOffset(int col, int row, int gridWidth, int gridHeight,
                                        PTTransitionAxis axis, int wings, int offsetStep,
                                        PTOffsetDirection direction);

    static quint32 serialTimeOffsetMs(int serialIndex, int fixtureCount,
                                      quint32 durationMs, PTPropagationMode propagation);

    static QString waveShapeToString(int shape);
    static int waveShapeFromString(const QString& s);
};

/*
  EFX Engine: stable VCWidget input source IDs (independent of table column order).
*/

#pragma once

#include <QtGlobal>

namespace PTEfxCol
{
/** Table column indices — PresetTableV2TransitionWidget::PresetColumn (no PlaybackMode column). */
enum Column : quint8
{
    Name = 0,
    Axis,
    OffsetDir,
    Wings,
    Blocks,
    WingsSymmetry,
    OffsetStepMode,
    OffsetStep,
    Duration,
    WaveWidth,
    WaveShape,
    FadeIn,
    FadeOut,
    WaveLevel,
    StartOffset,
    Propagation,
    SpeedMult,
    PositionMotion,
    PositionMotionDir,
    Position1DBuiltinMode,
    PositionPanSize,
    PositionTiltSize,
    Channel1DTarget,
    Channel1DTargetMode,
    Channel1DApplyMode,
    Channel1DLow,
    Channel1DHigh,
    Channel1DAmount,
    Channel1DCustomColumn,
    Count
};

/** Stable VC input IDs (32+) — never use column index as input id. */
enum Input : quint8
{
    InputAxis = 32,
    InputOffsetDir = 33,
    InputWings = 34,
    InputBlocks = 35,
    InputWingsSymmetry = 36,
    InputOffsetStep = 37,
    InputDuration = 38,
    InputWaveWidth = 39,
    InputWaveShape = 40,
    InputFadeIn = 41,
    InputFadeOut = 42,
    InputWaveLevel = 43,
    InputStartOffset = 44,
    InputPropagation = 45,
    /** @deprecated No table column; legacy projects / live merge only. */
    InputPlaybackMode = 46,
    InputSpeedMult = 53,
    InputPositionMotion = 54,
    InputPositionPanSize = 55,
    InputPositionTiltSize = 56,
    InputOffsetStepMode = 58,
    InputPositionMotionDir = 59,
    InputPosition1DBuiltinMode = 60,
    InputChannel1DTarget = 61,
    InputChannel1DTargetMode = 62,
    InputChannel1DApplyMode = 63,
    InputChannel1DLow = 64,
    InputChannel1DHigh = 65,
    InputChannel1DAmount = 66,
    InputChannel1DCustomColumn = 67,

    InputGlobalSpeed = 48,
    InputGlobalDirection = 49,
    InputGlobalIntensity = 50,
    InputGlobalBlocks = 51,

    /** >127 = crossfade driven by table fader (manual); <=127 = global speed/min/max clock. */
    InputCrossfadeManual = 52,

    InputGlobalPositionSize = 57
};

inline bool isStableInputId(quint8 id)
{
    return (id >= InputAxis && id <= InputPropagation)
            || id == InputSpeedMult
            || (id >= InputPositionMotion && id <= InputPositionTiltSize)
            || (id >= InputGlobalSpeed && id <= InputGlobalIntensity)
            || id == InputGlobalBlocks
            || id == InputGlobalPositionSize
            || id == InputCrossfadeManual
            || id == InputOffsetStepMode
            || id == InputPositionMotionDir
            || id == InputPosition1DBuiltinMode
            || (id >= InputChannel1DTarget && id <= InputChannel1DCustomColumn);
}

inline quint8 inputIdForColumn(int col)
{
    switch (col)
    {
        case Axis:          return InputAxis;
        case OffsetDir:     return InputOffsetDir;
        case Wings:         return InputWings;
        case Blocks:        return InputBlocks;
        case WingsSymmetry: return InputWingsSymmetry;
        case OffsetStepMode: return InputOffsetStepMode;
        case OffsetStep:    return InputOffsetStep;
        case Duration:      return InputDuration;
        case WaveWidth:     return InputWaveWidth;
        case WaveShape:     return InputWaveShape;
        case FadeIn:        return InputFadeIn;
        case FadeOut:       return InputFadeOut;
        case WaveLevel:     return InputWaveLevel;
        case StartOffset:   return InputStartOffset;
        case Propagation:   return InputPropagation;
        case SpeedMult:         return InputSpeedMult;
        case PositionMotion:    return InputPositionMotion;
        case PositionMotionDir: return InputPositionMotionDir;
        case Position1DBuiltinMode: return InputPosition1DBuiltinMode;
        case PositionPanSize:   return InputPositionPanSize;
        case PositionTiltSize:  return InputPositionTiltSize;
        case Channel1DTarget: return InputChannel1DTarget;
        case Channel1DTargetMode: return InputChannel1DTargetMode;
        case Channel1DApplyMode: return InputChannel1DApplyMode;
        case Channel1DLow: return InputChannel1DLow;
        case Channel1DHigh: return InputChannel1DHigh;
        case Channel1DAmount: return InputChannel1DAmount;
        case Channel1DCustomColumn: return InputChannel1DCustomColumn;
        default:                return 0;
    }
}

inline bool hasExternalInput(int col) { return col > Name && col < Count; }

/** @deprecated Use inputIdForColumn. */
inline quint8 inputId(int col) { return inputIdForColumn(col); }

/** Remap legacy keys (column index 1…14) to stable Input ids. */
inline quint8 migrateLegacyInputKey(quint8 legacyKey)
{
    if (isStableInputId(legacyKey))
        return legacyKey;

    return inputIdForColumn(int(legacyKey));
}

} // namespace PTEfxCol

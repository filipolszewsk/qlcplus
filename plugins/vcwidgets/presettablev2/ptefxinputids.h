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

    InputGlobalSpeed = 48,
    InputGlobalDirection = 49,
    InputGlobalIntensity = 50,
    InputGlobalBlocks = 51,

    /** >127 = crossfade driven by table fader (manual); <=127 = global speed/min/max clock. */
    InputCrossfadeManual = 52
};

inline bool isStableInputId(quint8 id)
{
    return (id >= InputAxis && id <= InputPropagation)
            || id == InputSpeedMult
            || (id >= InputGlobalSpeed && id <= InputGlobalIntensity)
            || id == InputCrossfadeManual;
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
        case OffsetStep:    return InputOffsetStep;
        case Duration:      return InputDuration;
        case WaveWidth:     return InputWaveWidth;
        case WaveShape:     return InputWaveShape;
        case FadeIn:        return InputFadeIn;
        case FadeOut:       return InputFadeOut;
        case WaveLevel:     return InputWaveLevel;
        case StartOffset:   return InputStartOffset;
        case Propagation:   return InputPropagation;
        case SpeedMult:     return InputSpeedMult;
        default:            return 0;
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

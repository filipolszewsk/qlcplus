/*
  ptpositionfxengine.h — relative 2D position offsets for Position Mode FX
*/

#pragma once

#include "presettablev2effectengine.h"

class Fixture;
struct PTDimmerWaveOffsetInfo;
struct PTDimmerWaveParams;

class PTPositionFxEngine
{
public:
    enum class Shape
    {
        None = -1,
        Circle = 0,
        Line,
        PanOnly,
        TiltOnly,
        Figure8,
        CustomPan1D,
        CustomTilt1D,
        Custom2D
    };

    static Shape shapeFromPositionMotion(PTPositionMotion motion);
    static bool motionUsesCustomData(PTPositionMotion motion);
    static bool motionIs1D(PTPositionMotion motion);
    /** Bipolar offset -1..+1 from shared morph wave packet (waveShape/customCurve/waveWidth/fade).
        iteratorRad must come from iteratorFromElapsed (includes startOffset + headOffset).
        Active window matches orbitPhaseFromIterator: [0, waveWidth). Outside: hold curve end. */
    static float samplePosition1DOffset(float iteratorRad, const PTTransitionPreset& preset,
                                        const PTDimmerWaveParams& waveParams,
                                        int headOffsetDeg = 0);
    /** Effective offset over full 0..360° cycle (hold start/end outside waveWidth window). */
    static float sampleMotionAtCycleDeg(float cycleDeg, const PTTransitionPreset& preset,
                                        const PTDimmerWaveParams& waveParams);
    static void relativeOffset(Shape shape, double phaseRadians,
                               qreal panSizeDeg, qreal tiltSizeDeg,
                               qreal& panOffDeg, qreal& tiltOffDeg);
    static void relativeOffsetForPreset(const PTTransitionPreset& preset, double phaseRadians,
                                        qreal panSizeDeg, qreal tiltSizeDeg,
                                        qreal& panOffDeg, qreal& tiltOffDeg);
    /** Smart compressor: pivot shifts toward axis center only when amplitude would clip. */
    static PTPositionValue applySmartMotion(const PTPositionValue& base, Fixture* fxi, int head,
                                            Shape shape, double phaseRadians, qreal size01);
    static PTPositionValue applySmartMotionFromPreset(const PTPositionValue& base, Fixture* fxi,
                                                      int head, const PTTransitionPreset& preset,
                                                      double phaseRadians, qreal size01,
                                                      float iteratorRad = -1.0f,
                                                      const PTDimmerWaveParams* waveParams = nullptr,
                                                      const PTDimmerWaveOffsetInfo* spatial = nullptr,
                                                      int headOffsetDeg = 0);

    /**
     * Map dimmer-wave iterator (rad) into orbit phase.
     * Outside waveWidth window, hold the loop start/end phase instead of falling back to base.
     */
    static bool orbitPhaseFromIterator(float iteratorRad, int waveWidthDeg, double& outPhaseRad);

    static double applyMotionDirection(double phaseRad, PTPositionMotionDirection direction,
                                       const PTDimmerWaveOffsetInfo& spatial);
};

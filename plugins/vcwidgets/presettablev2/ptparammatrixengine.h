/*
  QLC+ VC Widget Plugin — Preset Table v2
  Port of GRIDqlc DMX Param Matrix Pro Sync wave / FX math.
*/

#pragma once

#include <QHash>
#include <QSize>
#include <QVector>
#include "qlcpoint.h"
#include "presettablev2effectengine.h"

/** Global timing / direction — shared by sweep, flash, and continuous FX. */
struct PTGlobalEffectSettings
{
    uchar     speed            = 128;
    quint32   minDurationMs    = 100;
    quint32   maxDurationMs    = 5000;
    int       speedMultiplier  = 1;   /**< 0=0.5x … 5=5.0x */
    int       transitionDirection = 0; /**< 0=All, 1=LR, 2=RL, 3=CenterOut, 4=OutsideIn */
    uchar     intensity        = 255;
    int       fxBlocks         = 1;
    int       fxPhaseOffset    = 0;   /**< 0–255 */
    int       fxWingsSymmetry  = 0;   /**< 0=Normal, 1=Alternate, 2=Mirror */
    int       fxOrientation    = 0;   /**< 0=Horizontal (X), 1=Vertical (Y) */
    int       fxMultiplier     = 1;   /**< 0=0.5x, 1=1x, 2=2x */
};

enum class PTFlashPhase
{
    Idle = 0,
    WaveIn,
    Hold,
    WaveOut
};

enum class PTPreFlashState
{
    Idle = 0,
    Sweep,
    ColorFx
};

struct PTOutputMatrixState
{
    bool     sweepRunning  = false;
    /** Crossfade fader drives sweep progress (no auto timer). */
    bool     sweepManualCrossfade = false;
    double   sweepManualPhase     = 0.0;
    double   sweepManualPhasePrev = 0.0;
    double   sweepProgress = 0.0;
    int      sweepFromRow  = -1;
    int      sweepToRow    = -1;
    quint32  sweepElapsedMs = 0;
    quint32  sweepLastCycleMs = 0;
    QHash<QLCPoint, float> sweepPeakDimmer;
    QHash<QLCPoint, QVector<uchar>> sweepHeldValues;

    bool           flashActive       = false;
    PTFlashPhase   flashPhase        = PTFlashPhase::Idle;
    double         flashWaveProgress = 0.0;
    /** WaveIn progress captured at release; 1.0 = full hold release. */
    double         flashReleaseProgress = 1.0;
    quint32        flashElapsedMs    = 0;
    quint32        flashLastCycleMs  = 0;
    quint32        flashSourceWidgetId = 0;
    quint64        flashToken        = 0;
    int            flashRow          = -1;
    int            flashReturnRow    = -1;
    QVector<uchar> flashValues;
    QVector<uchar> flashReturnValues;
    PTTransitionPreset flashPreset;
    double flashTimeMultiplier = 1.0;
    PTPreFlashState preFlashState    = PTPreFlashState::Idle;

    quint32 fxStep = 0;
    int     appliedRow = -1;
};

struct PTMatrixFxFrameContext
{
    int   stepsPerCycle = 256;
    bool  vertical      = false;
    int   fixtureCount  = 1;
    int   effectiveWings = 1;
    int   effectiveBlocks = 1;
    int   positionsPerWing = 1;
    int   blocksPerWing = 1;
    double fxMultiplier = 1.0;
    int   stepOffset = 0;
    int   fxDirection = 0;
    int   wingsSymmetry = 0;
};

class PTParamMatrixEngine
{
public:
    static double speedMultiplierValue(int index);
    static double fxMultiplierValue(int index);

    static quint32 effectiveDurationMs(const PTGlobalEffectSettings& global,
                                       const PTTransitionPreset& preset,
                                       bool honorPresetDuration = false);

    static double transitionIncrement(const PTGlobalEffectSettings& global,
                                      const PTTransitionPreset& preset);

    static double fixturePhaseOffset(int fixtureIndex, int fixtureCount, int direction);

    static bool waveShowNew(int fixtureIndex, int fixtureCount, double progress,
                            int transitionDirection);

    static double fxBrightness(int phase, int totalSteps, int fadeIn255, int fadeOut255,
                               int wavelength255);

    static int reverseSpatialDirection(int direction);
    static int wingSpatialDirection(int wingIndex, int baseDirection, int symmetry,
                                    int totalWings);

    static void buildFxContext(PTMatrixFxFrameContext& ctx,
                               const PTGlobalEffectSettings& global,
                               const PTTransitionPreset& preset,
                               int gridWidth, int gridHeight, quint32 fxStep);

    static int linearPosition(const QLCPoint& pt, PTTransitionAxis axis, int gridWidth);

    static int colorFxBrightnessAt(const PTMatrixFxFrameContext& ctx,
                                   const PTGlobalEffectSettings& global,
                                   const PTTransitionPreset& preset,
                                   int positionIndex);

    static QVector<uchar> blendWithIntensity(const QVector<uchar>& values, uchar intensity);

    static PTOffsetDirection offsetFromGlobalDirection(int transitionDirection);

    /** Alternate / Symmetric → instant row change (no spatial sweep wave). */
    static bool sweepInstantForOffset(PTOffsetDirection dir);

    /** Maps offset dir to waveShowNew front: 0=All, 1=LR, 2=RL, 3=center out, 4=outside in. */
    static int waveFrontFromOffset(PTOffsetDirection dir);

    /** Crossfade sweep: blend A→B from spatial phaseStart01 (head offset / 360). */
    static float crossfadeSweepBlend01(double globalProgress, double phaseStart01,
                                       const PTTransitionPreset& preset,
                                       const PTGlobalEffectSettings& global);
};

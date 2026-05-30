/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2effectengine.h — spatial / transition engine
*/

#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <QVector>
#include <QString>
#include "qlcpoint.h"

static const int kPTEfxStepMs = 20;

enum class PTSpatialOrder
{
    RowMajor = 0,
    ByColumn,
    ByRow
};

struct PTSpatialEffectSettings
{
    bool           enabled     = false;
    PTSpatialOrder order       = PTSpatialOrder::RowMajor;
    bool           reverse     = false;
    quint32        stepDelayMs = 50;
    quint32        fadeMs      = 150;
};

enum class PTTransitionAxis
{
    X = 0,
    Y,
    XY
};

/** Legacy chase sort — mapped from PTOffsetDirection where needed. */
enum class PTTransitionDirection
{
    LR = 0,
    RL,
    In,
    Out,
    OutIn,
    Even,
    Odd
};

enum class PTOffsetDirection
{
    LeftToRight = 0,
    RightToLeft,
    CenterToSides,
    SidesToCenter,
    Alternate,
    Symmetric
};

enum class PTPropagationMode
{
    Parallel = 0,
    Serial
};

enum class PTTransitionMode
{
    Off = 0,
    SweepOnly,
    Continuous
};

/** EFX DimmerWave-aligned transition preset (per spatial preset row). */
struct PTTransitionPreset
{
    QString            name;
    bool               enabled = true;
    PTTransitionAxis   axis = PTTransitionAxis::X;
    PTOffsetDirection  offsetDirection = PTOffsetDirection::LeftToRight;
    int                offsetStep = 20;
    int                wings = 1;
    int                blocks = 1;
    int                wingsSymmetry = 0; /**< 0=Normal, 1=Alternate, 2=Mirror */
    quint32            durationMs = 5000;
    int                waveWidth = 180;
    int                waveShape = 0;
    int                waveFadeIn = 25;
    int                waveFadeOut = 25;
    int                waveLevel = 255;
    int                startOffset = 0;
    PTPropagationMode  propagation = PTPropagationMode::Parallel;
    PTTransitionMode   playbackMode = PTTransitionMode::SweepOnly;
    /** Sweep / flash wave front: 0=All, 1=LR, 2=RL, 3=CenterOut, 4=OutsideIn */
    int                transitionDirection = 0;
    /** 0=0.5x … 5=5.0x — cycle speed for this bank preset */
    int                speedMultiplier = 1;
    quint32            stepDelayMs = 50;
    quint32            fadeMs = 150;
};

struct PTSpatialChaseOutput
{
    bool               active = false;
    int                targetRow = -1;
    double             progress = 0.0;
    PTTransitionPreset spatialPreset;
    QList<QLCPoint>    order;
    QSet<QLCPoint>     armed;
};

class PresetTableV2SpatialEngine
{
public:
    static QList<QLCPoint> sortedPoints(const QList<QLCPoint>& points,
                                        PTSpatialOrder order,
                                        bool reverse);

    static QString orderToString(PTSpatialOrder order);
    static PTSpatialOrder orderFromString(const QString& s);

    static QString axisToString(PTTransitionAxis axis);
    static PTTransitionAxis axisFromString(const QString& s);

    static QString offsetDirectionToString(PTOffsetDirection dir);
    static PTOffsetDirection offsetDirectionFromString(const QString& s);

    static PTTransitionDirection chaseDirectionFromOffset(PTOffsetDirection dir);

    static PTTransitionPreset presetFromLegacySpatial(const PTSpatialEffectSettings& fx);

    static QList<QLCPoint> buildChaseOrder(const QList<QLCPoint>& points,
                                           const PTTransitionPreset& preset,
                                           int gridWidth,
                                           int gridHeight);

    static quint32 totalDurationMs(quint32 stepDelayMs, quint32 fadeMs, int pointCount);
    static quint32 totalDurationMs(const PTSpatialEffectSettings& fx, int pointCount);

    static int transitionPresetIndexFromInput(uchar value, int presetCount);
    static int tableRowIndexFromInput(uchar value, int rowCount);

    static PTTransitionPreset mergePreset(const PTTransitionPreset& base,
                                          const QHash<quint8, uchar>& liveByColumn);

    static QVector<uchar> blendValues(const QVector<uchar>& primary,
                                      const QVector<uchar>& secondary,
                                      double brightness,
                                      bool snapBlend = false);

    static quint32 durationMsFromInputByte(uchar value);

    static int fixtureSpanAlongAxis(const QList<QLCPoint>& points,
                                    PTTransitionAxis axis,
                                    int gridWidth);

    static void applySweepPresetConstraints(PTTransitionPreset& preset);
};

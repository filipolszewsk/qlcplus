/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2effectengine.h — spatial / transition engine
*/

#pragma once

#include <QtGlobal>
#include <QHash>
#include <QList>
#include <QSet>
#include <QVector>
#include <QString>
#include "qlcpoint.h"

struct PTGlobalEffectSettings;

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

enum class PTOffsetStepMode : int
{
    Off = 0,
    AutoFit,
    CoveragePercent,
    FixedDegrees
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
    Continuous,
    MultiFx = 3,
    PositionMotion = 4,
    Channel1D = 5
};

enum class PTMultiFxInterpolationSourceMode : int
{
    Dynamic = 0,
    Static = 1
};

enum class PTChannel1DTarget : int
{
    Dimmer = 0,
    AllIntensity,
    Zoom,
    Focus,
    Iris,
    Prism,
    GoboIndex,
    ShutterStrobe,
    Speed,
    Color,
    CustomColumn
};

enum class PTChannel1DTargetMode : int
{
    First = 0,
    All
};

enum class PTChannel1DApplyMode : int
{
    AbsoluteRange = 0,
    RelativeAroundBase,
    MultiplyBase,
    BumpAdd
};

/** Relative position orbit type (Position Mode only — separate from dimmer waveShape). */
enum class PTPositionMotion : int
{
    Off = 0,
    Pan1D = 1,
    Tilt1D = 2,
    Circle2D = 3,
    Line2D = 4,
    Figure8_2D = 5,
    CustomPan1D = 6,
    CustomTilt1D = 7,
    Custom2D = 8
};

/** Orbit winding / symmetry (Position Mode). */
enum class PTPositionMotionDirection : int
{
    Forward = 0,
    Reverse,
    AlternateWings,
    SymmetricPairs,
    ReverseAlternateWings
};

struct PTCustomCurvePoint
{
    enum SegmentMode
    {
        Bezier = 0,
        Linear = 1
    };

    double xDeg = 0.0;
    double yValue = 0.0;
    double leftHandleXDeg = 0.0;
    double leftHandleYValue = 0.0;
    double rightHandleXDeg = 0.0;
    double rightHandleYValue = 0.0;
    int segmentMode = Bezier;
};

struct PTCustomCurveGalleryItem
{
    QString name;
    QVector<PTCustomCurvePoint> points;
};

struct PTPositionPath2DPoint
{
    double pan01 = 0.0;
    double tilt01 = 0.0;
    double leftHandlePan01 = 0.0;
    double leftHandleTilt01 = 0.0;
    double rightHandlePan01 = 0.0;
    double rightHandleTilt01 = 0.0;
    int segmentMode = PTCustomCurvePoint::Bezier;
};

enum class PTShapeGalleryKind
{
    Curve1D = 0,
    Path2D = 1
};

struct PTShapeGalleryItem
{
    QString name;
    PTShapeGalleryKind kind = PTShapeGalleryKind::Curve1D;
    QVector<PTCustomCurvePoint> curve1D;
    QVector<PTPositionPath2DPoint> path2D;
    bool path2DClosed = true;
};

struct PTPositionValue
{
    bool  valid = false;
    qreal panDeg = 0;
    qreal tiltDeg = 0;
};

/** EFX DimmerWave-aligned transition preset (per spatial preset row). */
struct PTTransitionPreset
{
    QString            name;
    bool               enabled = true;
    PTTransitionAxis   axis = PTTransitionAxis::X;
    PTOffsetDirection  offsetDirection = PTOffsetDirection::LeftToRight;
    int                offsetStep = 20;
    PTOffsetStepMode   offsetStepMode = PTOffsetStepMode::AutoFit;
    int                offsetCoverage = 100;
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
    bool               customCurveEnabled = false;
    QVector<PTCustomCurvePoint> customCurve;
    PTPropagationMode  propagation = PTPropagationMode::Parallel;
    PTTransitionMode   playbackMode = PTTransitionMode::SweepOnly;
    /** Sweep / flash wave front: 0=All, 1=LR, 2=RL, 3=CenterOut, 4=OutsideIn */
    int                transitionDirection = 0;
    /** 0=0.5x … 5=5.0x — cycle speed for this bank preset */
    int                speedMultiplier = 1;
    quint32            stepDelayMs = 50;
    quint32            fadeMs = 150;
    /** Position Mode: relative orbit around base row (independent of dimmer wave fields). */
    int                positionMotion = int(PTPositionMotion::Off);
    int                positionMotionDirection = int(PTPositionMotionDirection::Forward);
    int                positionPanSize = 45;
    int                positionTiltSize = 30;
    /** Pan/Tilt 1D builtin: 0=morph packet (calculateDimmerWave), 1=oscillate in window. */
    int                position1DBuiltinMode = 0;
    QVector<PTPositionPath2DPoint> positionPath2D;
    bool               positionPath2DClosed = true;
    /** Fixture Group only: generic 1D channel FX target/value operation. */
    int                channel1DTarget = int(PTChannel1DTarget::Dimmer);
    int                channel1DTargetMode = int(PTChannel1DTargetMode::First);
    int                channel1DApplyMode = int(PTChannel1DApplyMode::MultiplyBase);
    int                channel1DLow = 0;
    int                channel1DHigh = 255;
    int                channel1DAmount = 255;
    int                channel1DCustomColumn = 0;
    /** MultiFX Interpolation route only: 0=target table live primary/secondary, 1=manual rows below. */
    int                multiFxInterpolationSourceMode =
            int(PTMultiFxInterpolationSourceMode::Dynamic);
    int                multiFxInterpolationPrimaryRow = -1;
    int                multiFxInterpolationSecondaryRow = -1;
};

struct PTSpatialChaseOutput
{
    bool               active = false;
    int                targetRow = -1;
    double             progress = 0.0;
    PTTransitionPreset spatialPreset;
    QList<QLCPoint>    order;
    QSet<QLCPoint>     armed;
    QSet<quint32>      armedFixtures;
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
                                           int gridHeight,
                                           const PTGlobalEffectSettings* global = nullptr);

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

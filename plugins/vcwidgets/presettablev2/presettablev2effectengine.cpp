/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2effectengine.cpp
*/

#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"
#include "ptparammatrixengine.h"
#include "ptefxinputids.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace {

int linearIndex(const QLCPoint& pt, PTTransitionAxis axis, int gridWidth)
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

struct PointOffset
{
    QLCPoint pt;
    int      offset = 0;
};

} // namespace

QList<QLCPoint> PresetTableV2SpatialEngine::sortedPoints(const QList<QLCPoint>& points,
                                                          PTSpatialOrder order,
                                                          bool reverse)
{
    QList<QLCPoint> sorted = points;
    switch (order)
    {
        case PTSpatialOrder::ByColumn:
            std::sort(sorted.begin(), sorted.end(),
                      [](const QLCPoint& a, const QLCPoint& b) {
                          if (a.x() != b.x()) return a.x() < b.x();
                          return a.y() < b.y();
                      });
            break;
        case PTSpatialOrder::ByRow:
            std::sort(sorted.begin(), sorted.end(),
                      [](const QLCPoint& a, const QLCPoint& b) {
                          if (a.y() != b.y()) return a.y() < b.y();
                          return a.x() < b.x();
                      });
            break;
        default:
            std::sort(sorted.begin(), sorted.end(),
                      [](const QLCPoint& a, const QLCPoint& b) {
                          if (a.y() != b.y()) return a.y() < b.y();
                          return a.x() < b.x();
                      });
            break;
    }
    if (reverse)
        std::reverse(sorted.begin(), sorted.end());
    return sorted;
}

QString PresetTableV2SpatialEngine::orderToString(PTSpatialOrder order)
{
    switch (order)
    {
        case PTSpatialOrder::ByColumn: return QStringLiteral("Column");
        case PTSpatialOrder::ByRow:    return QStringLiteral("Row");
        default:                       return QStringLiteral("RowMajor");
    }
}

PTSpatialOrder PresetTableV2SpatialEngine::orderFromString(const QString& s)
{
    if (s == QLatin1String("Column")) return PTSpatialOrder::ByColumn;
    if (s == QLatin1String("Row"))    return PTSpatialOrder::ByRow;
    return PTSpatialOrder::RowMajor;
}

QString PresetTableV2SpatialEngine::axisToString(PTTransitionAxis axis)
{
    switch (axis)
    {
        case PTTransitionAxis::Y:  return QStringLiteral("Y");
        case PTTransitionAxis::XY: return QStringLiteral("XY");
        default:                   return QStringLiteral("X");
    }
}

PTTransitionAxis PresetTableV2SpatialEngine::axisFromString(const QString& s)
{
    if (s == QLatin1String("Y"))  return PTTransitionAxis::Y;
    if (s == QLatin1String("XY")) return PTTransitionAxis::XY;
    return PTTransitionAxis::X;
}

QString PresetTableV2SpatialEngine::offsetDirectionToString(PTOffsetDirection dir)
{
    switch (dir)
    {
        case PTOffsetDirection::RightToLeft:    return QStringLiteral("RL");
        case PTOffsetDirection::CenterToSides: return QStringLiteral("IN");
        case PTOffsetDirection::SidesToCenter: return QStringLiteral("OUT");
        case PTOffsetDirection::Alternate:    return QStringLiteral("ALT");
        case PTOffsetDirection::Symmetric:      return QStringLiteral("SYM");
        default:                              return QStringLiteral("LR");
    }
}

PTOffsetDirection PresetTableV2SpatialEngine::offsetDirectionFromString(const QString& s)
{
    if (s == QLatin1String("RL") || s == QLatin1String("RightToLeft"))
        return PTOffsetDirection::RightToLeft;
    if (s == QLatin1String("IN") || s == QLatin1String("CenterToSides"))
        return PTOffsetDirection::CenterToSides;
    if (s == QLatin1String("OUT") || s == QLatin1String("SidesToCenter"))
        return PTOffsetDirection::SidesToCenter;
    if (s == QLatin1String("ALT") || s == QLatin1String("Alternate") || s == QLatin1String("EVEN"))
        return PTOffsetDirection::Alternate;
    if (s == QLatin1String("SYM") || s == QLatin1String("Symmetric") || s == QLatin1String("Mirror"))
        return PTOffsetDirection::Symmetric;
    if (s == QLatin1String("OUTIN"))
        return PTOffsetDirection::CenterToSides;
    return PTOffsetDirection::LeftToRight;
}

PTTransitionDirection PresetTableV2SpatialEngine::chaseDirectionFromOffset(PTOffsetDirection dir)
{
    switch (dir)
    {
        case PTOffsetDirection::RightToLeft:    return PTTransitionDirection::RL;
        case PTOffsetDirection::CenterToSides: return PTTransitionDirection::In;
        case PTOffsetDirection::SidesToCenter: return PTTransitionDirection::Out;
        case PTOffsetDirection::Alternate:    return PTTransitionDirection::Even;
        case PTOffsetDirection::Symmetric:      return PTTransitionDirection::OutIn;
        default:                              return PTTransitionDirection::LR;
    }
}

PTTransitionPreset PresetTableV2SpatialEngine::presetFromLegacySpatial(const PTSpatialEffectSettings& fx)
{
    PTTransitionPreset p;
    p.name = QStringLiteral("Legacy");
    p.enabled = fx.enabled;
    p.stepDelayMs = fx.stepDelayMs;
    p.fadeMs = fx.fadeMs;
    p.offsetDirection = fx.reverse ? PTOffsetDirection::RightToLeft : PTOffsetDirection::LeftToRight;
    switch (fx.order)
    {
        case PTSpatialOrder::ByColumn:
            p.axis = PTTransitionAxis::X;
            break;
        case PTSpatialOrder::ByRow:
            p.axis = PTTransitionAxis::Y;
            break;
        default:
            p.axis = PTTransitionAxis::XY;
            break;
    }
    return p;
}

QList<QLCPoint> PresetTableV2SpatialEngine::buildChaseOrder(const QList<QLCPoint>& points,
                                                            const PTTransitionPreset& preset,
                                                            int gridWidth,
                                                            int gridHeight,
                                                            const PTGlobalEffectSettings* global)
{
    if (points.isEmpty())
        return {};

    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(preset, global);
    if (global && global->fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    std::vector<PointOffset> items;
    items.reserve(size_t(points.size()));

    for (const QLCPoint& pt : points)
    {
        const int off = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                pt.x(), pt.y(), gridWidth, gridHeight, waveParams);
        items.push_back({pt, off});
    }

    std::sort(items.begin(), items.end(),
              [](const PointOffset& a, const PointOffset& b) {
                  if (a.offset != b.offset) return a.offset < b.offset;
                  if (a.pt.y() != b.pt.y()) return a.pt.y() < b.pt.y();
                  return a.pt.x() < b.pt.x();
              });

    QList<QLCPoint> result;
    result.reserve(int(items.size()));
    for (const PointOffset& po : items)
        result.append(po.pt);
    return result;
}

quint32 PresetTableV2SpatialEngine::totalDurationMs(quint32 stepDelayMs, quint32 fadeMs, int pointCount)
{
    if (pointCount <= 0)
        return 0;
    const quint32 lastStart = quint32(pointCount - 1) * stepDelayMs;
    return lastStart + fadeMs;
}

quint32 PresetTableV2SpatialEngine::totalDurationMs(const PTSpatialEffectSettings& fx, int pointCount)
{
    return totalDurationMs(fx.stepDelayMs, fx.fadeMs, pointCount);
}

int PresetTableV2SpatialEngine::transitionPresetIndexFromInput(uchar value, int presetCount)
{
    if (value == 0 || presetCount <= 0)
        return -1;
    const int idx = int(value) - 1;
    return qBound(0, idx, presetCount - 1);
}

int PresetTableV2SpatialEngine::tableRowIndexFromInput(uchar value, int rowCount)
{
    if (value == 0 || rowCount <= 0)
        return -1;
    const int idx = int(value) - 1;
    return qBound(0, idx, rowCount - 1);
}

quint32 PresetTableV2SpatialEngine::durationMsFromInputByte(uchar value)
{
    const double t = double(value) / 255.0;
    return quint32(20.0 + t * (60000.0 - 20.0));
}

int PresetTableV2SpatialEngine::fixtureSpanAlongAxis(const QList<QLCPoint>& points,
                                                     PTTransitionAxis axis,
                                                     int gridWidth)
{
    int maxIdx = 0;
    for (const QLCPoint& pt : points)
        maxIdx = qMax(maxIdx, linearIndex(pt, axis, gridWidth));
    return maxIdx + 1;
}

PTTransitionPreset PresetTableV2SpatialEngine::mergePreset(const PTTransitionPreset& base,
                                                         const QHash<quint8, uchar>& liveByColumn)
{
    PTTransitionPreset p = base;
    auto val = [&](quint8 col) -> uchar {
        return liveByColumn.value(col);
    };
    if (liveByColumn.contains(PTEfxCol::InputAxis))
    {
        const int v = int(val(PTEfxCol::InputAxis)) % 3;
        p.axis = PTTransitionAxis(v);
    }
    if (liveByColumn.contains(PTEfxCol::InputOffsetDir))
    {
        const int v = int(val(PTEfxCol::InputOffsetDir)) % 6;
        p.offsetDirection = PTOffsetDirection(v);
    }
    if (liveByColumn.contains(PTEfxCol::InputWings))
        p.wings = qBound(1, 1 + int(val(PTEfxCol::InputWings)) * 63 / 255, 64);
    if (liveByColumn.contains(PTEfxCol::InputBlocks))
        p.blocks = qBound(1, 1 + int(val(PTEfxCol::InputBlocks)) * 63 / 255, 64);
    if (liveByColumn.contains(PTEfxCol::InputWingsSymmetry))
        p.wingsSymmetry = int(val(PTEfxCol::InputWingsSymmetry)) % 3;
    if (liveByColumn.contains(PTEfxCol::InputOffsetStep))
    {
        const int v = int(val(PTEfxCol::InputOffsetStep));
        p.offsetStep = (v == 0) ? 0 : qBound(1, v * 360 / 255, 360);
    }
    if (liveByColumn.contains(PTEfxCol::InputDuration))
        p.durationMs = durationMsFromInputByte(val(PTEfxCol::InputDuration));
    if (liveByColumn.contains(PTEfxCol::InputWaveWidth))
        p.waveWidth = qBound(1, int(val(PTEfxCol::InputWaveWidth)) * 360 / 255, 360);
    if (liveByColumn.contains(PTEfxCol::InputWaveShape))
    {
        p.waveShape = int(val(PTEfxCol::InputWaveShape)) % 3;
        p.customCurveEnabled = false;
    }
    if (liveByColumn.contains(PTEfxCol::InputFadeIn))
        p.waveFadeIn = qBound(0, int(val(PTEfxCol::InputFadeIn)) * 100 / 255, 100);
    if (liveByColumn.contains(PTEfxCol::InputFadeOut))
        p.waveFadeOut = qBound(0, int(val(PTEfxCol::InputFadeOut)) * 100 / 255, 100);
    if (liveByColumn.contains(PTEfxCol::InputWaveLevel))
        p.waveLevel = int(val(PTEfxCol::InputWaveLevel));
    if (liveByColumn.contains(PTEfxCol::InputStartOffset))
        p.startOffset = int(val(PTEfxCol::InputStartOffset)) * 360 / 255;
    if (liveByColumn.contains(PTEfxCol::InputPropagation))
        p.propagation = (int(val(PTEfxCol::InputPropagation)) % 2 == 0)
                ? PTPropagationMode::Parallel : PTPropagationMode::Serial;
    if (liveByColumn.contains(PTEfxCol::InputSpeedMult))
        p.speedMultiplier = qBound(0, int(val(PTEfxCol::InputSpeedMult)) * 5 / 255, 5);
    if (liveByColumn.contains(PTEfxCol::InputPositionMotion))
        p.positionMotion = int(val(PTEfxCol::InputPositionMotion)) % 9;
    return p;
}

void PresetTableV2SpatialEngine::applySweepPresetConstraints(PTTransitionPreset& preset)
{
    if (preset.playbackMode != PTTransitionMode::SweepOnly)
        return;

    preset.waveWidth = 360;
    preset.waveLevel = 255;
    preset.waveFadeIn = qBound(0, preset.waveFadeIn, 50);
    preset.waveFadeOut = qBound(0, preset.waveFadeOut, 50);
}

QVector<uchar> PresetTableV2SpatialEngine::blendValues(const QVector<uchar>& primary,
                                                       const QVector<uchar>& secondary,
                                                       double brightness,
                                                       bool snapBlend)
{
    const int n = qMax(primary.size(), secondary.size());
    QVector<uchar> out(n, 0);
    double t = brightness;
    if (t > 1.0)
        t /= 255.0;
    t = qBound(0.0, t, 1.0);

    if (snapBlend)
    {
        for (int i = 0; i < n; ++i)
        {
            const uchar a = (i < primary.size()) ? primary[i] : 0;
            const uchar b = (i < secondary.size()) ? secondary[i] : 0;
            out[i] = (t >= 0.5) ? b : a;
        }
        return out;
    }

    for (int i = 0; i < n; ++i)
    {
        const uchar a = (i < primary.size()) ? primary[i] : 0;
        const uchar b = (i < secondary.size()) ? secondary[i] : 0;
        out[i] = uchar(qBound(0, int(std::lround(double(a) + (double(b) - double(a)) * t)), 255));
    }
    return out;
}

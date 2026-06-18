/*
  Preset Table v2 — ptspatialfixtureplan.cpp
*/

#include "ptspatialfixtureplan.h"

#include "ptdimmerwaveengine.h"
#include "presettablev2effectengine.h"

#include <QtMath>
#include <QSet>
#include <cmath>

const PTSpatialFixtureEntry* PTSpatialFixturePlan::entryFor(const QLCPoint& pt) const
{
    const auto it = indexByPoint.constFind(pt);
    if (it == indexByPoint.constEnd())
        return nullptr;
    const int idx = it.value();
    if (idx < 0 || idx >= entries.size())
        return nullptr;
    return &entries.at(idx);
}

double PTSpatialFixturePlan::windowWidth01(const PTTransitionPreset& preset)
{
    return qMax(0.02, double(qBound(1, preset.waveWidth, 360)) / 360.0);
}

double PTSpatialFixturePlan::sweepWindowWidth01(const PTTransitionPreset& preset)
{
    const int sweepWidth = qMin(180, qBound(1, preset.waveWidth, 360));
    return qMax(0.02, double(sweepWidth) / 360.0);
}

PTSpatialFixturePlan PTSpatialFixturePlan::build(const QList<QLCPoint>& scopePoints,
                                                 const PTTransitionPreset& preset,
                                                 const PTGlobalEffectSettings& global,
                                                 int gridWidth, int gridHeight)
{
    PTSpatialFixturePlan plan;
    if (scopePoints.isEmpty())
        return plan;

    const bool sweepMode = preset.playbackMode == PTTransitionMode::SweepOnly;
    const PTTransitionPreset effectivePreset = sweepMode
            ? PTDimmerWaveEngine::normalizedTransitionSweepPreset(preset) : preset;
    const PTDimmerWaveEngine::OffsetDistributionPolicy offsetPolicy = sweepMode
            ? PTDimmerWaveEngine::OffsetDistributionPolicy::NonWrappingSweep
            : PTDimmerWaveEngine::OffsetDistributionPolicy::CyclicNoDuplicate;
    const QList<QLCPoint> chaseOrder = PresetTableV2SpatialEngine::buildChaseOrder(
            scopePoints, effectivePreset, gridWidth, gridHeight, &global);
    const int count = qMax(1, chaseOrder.size());
    const double width01 = windowWidth01(effectivePreset);

    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(effectivePreset, &global);
    if (global.fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    plan.entries.reserve(chaseOrder.size());
    for (int i = 0; i < chaseOrder.size(); ++i)
    {
        const QLCPoint& pt = chaseOrder.at(i);
        PTSpatialFixtureEntry e;
        e.pt = pt;
        e.serialIndex = i;
        const PTDimmerWaveOffsetInfo info = PTDimmerWaveEngine::offsetInfoForPoint(
                pt.x(), pt.y(), gridWidth, gridHeight, waveParams, offsetPolicy);
        if (sweepMode)
        {
            const bool pairedSweep = effectivePreset.offsetDirection == PTOffsetDirection::CenterToSides
                    || effectivePreset.offsetDirection == PTOffsetDirection::SidesToCenter
                    || effectivePreset.offsetDirection == PTOffsetDirection::Symmetric;
            e.phaseStart01 = pairedSweep
                    ? qBound(0.0, info.phaseStart01, 1.0)
                    : (count <= 1 ? 0.0 : double(i) / double(count - 1));
            e.headOffsetDeg = int(std::round(e.phaseStart01 * 360.0));
        }
        else
        {
            e.headOffsetDeg = info.headOffsetDeg;
            e.phaseStart01 = qBound(0.0, info.phaseStart01, 1.0);
        }

        if (!sweepMode && effectivePreset.propagation == PTPropagationMode::Serial && count > 1)
        {
            const double serialSpread = double(i) / double(count - 1) * qMax(0.0, 1.0 - width01);
            e.phaseStart01 = qMin(1.0, e.phaseStart01 + serialSpread);
        }

        plan.indexByPoint.insert(pt, plan.entries.size());
        plan.entries.append(e);
    }
    return plan;
}

float PTSpatialFixturePlan::sweepBlend01AtPhaseStart(double globalProgress, double phaseStart01,
                                                     const PTTransitionPreset& preset,
                                                     const PTGlobalEffectSettings& global)
{
    PTSpatialFixtureEntry entry;
    entry.phaseStart01 = qBound(0.0, phaseStart01, 1.0);
    PTSpatialFixturePlan plan;
    return plan.sweepBlend01(globalProgress, entry, preset, global);
}

float PTSpatialFixturePlan::sweepBlend01(double globalProgress,
                                         const PTSpatialFixtureEntry& entry,
                                         const PTTransitionPreset& preset,
                                         const PTGlobalEffectSettings& global) const
{
    globalProgress = qBound(0.0, globalProgress, 1.0);
    if (globalProgress <= 0.0)
        return 0.0f;
    if (globalProgress >= 1.0)
        return 1.0f;

    const PTTransitionPreset effectivePreset = preset.playbackMode == PTTransitionMode::SweepOnly
            ? PTDimmerWaveEngine::normalizedTransitionSweepPreset(preset) : preset;

    if (PTParamMatrixEngine::sweepInstantForOffset(effectivePreset.offsetDirection))
        return float(globalProgress);

    const double start01 = entry.phaseStart01;
    const double width01 = windowWidth01(effectivePreset);
    const double end01 = qMin(1.0, start01 + width01);

    if (globalProgress <= start01)
        return 0.0f;
    if (globalProgress >= end01)
        return 1.0f;

    const double localPhase = (globalProgress - start01) / qMax(0.001, end01 - start01);
    const PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(effectivePreset, &global);
    return PTDimmerWaveEngine::dimmerSweepAttack01(float(localPhase), waveParams);
}

float PTSpatialFixturePlan::sweepBlend01(double globalProgress, const QLCPoint& pt,
                                         const PTTransitionPreset& preset,
                                         const PTGlobalEffectSettings& global) const
{
    const PTSpatialFixtureEntry* e = entryFor(pt);
    if (!e)
        return globalProgress >= 1.0 ? 1.0f : 0.0f;
    return sweepBlend01(globalProgress, *e, preset, global);
}

PTSpatialGridPreview PTSpatialFixturePlan::buildGridPreview(const QList<QLCPoint>& scopePoints,
                                                            const PTTransitionPreset& preset,
                                                            const PTGlobalEffectSettings& global,
                                                            int gridWidth, int gridHeight)
{
    PTSpatialGridPreview preview;
    preview.gridSize = QSize(qMax(1, gridWidth), qMax(1, gridHeight));
    if (scopePoints.isEmpty() || gridWidth <= 0 || gridHeight <= 0)
        return preview;

    const bool sweepMode = preset.playbackMode == PTTransitionMode::SweepOnly;
    const PTTransitionPreset effectivePreset = sweepMode
            ? PTDimmerWaveEngine::normalizedTransitionSweepPreset(preset) : preset;
    const PTDimmerWaveEngine::OffsetDistributionPolicy offsetPolicy = sweepMode
            ? PTDimmerWaveEngine::OffsetDistributionPolicy::NonWrappingSweep
            : PTDimmerWaveEngine::OffsetDistributionPolicy::CyclicNoDuplicate;
    const int span = PTDimmerWaveEngine::gridSpanAlongAxis(
            gridWidth, gridHeight, effectivePreset.axis, global.fxOrientation);
    preview.effectiveOffsetSlots = PTDimmerWaveEngine::effectiveOffsetSlotCount(span, effectivePreset);
    preview.wings = qBound(1, effectivePreset.wings, qMax(1, span));
    preview.blocks = qMax(1, effectivePreset.blocks);
    preview.slotsPerWing = PTDimmerWaveEngine::offsetSlotCountForWing(span, effectivePreset);
    preview.maxOffsetStep = PTDimmerWaveEngine::maxOffsetStepForGrid(
            span, effectivePreset, offsetPolicy);
    preview.effectiveOffsetStep = PTDimmerWaveEngine::effectiveOffsetStepForSpan(
            span, effectivePreset, offsetPolicy);
    preview.offsetStepMode = effectivePreset.offsetStepMode;
    preview.offsetCoverage = qBound(0, effectivePreset.offsetCoverage, 100);
    preview.offsetStepOk = effectivePreset.offsetStepMode != PTOffsetStepMode::FixedDegrees
            || effectivePreset.offsetStep == 0
            || effectivePreset.offsetStep <= preview.maxOffsetStep;

    const PTSpatialFixturePlan plan = build(scopePoints, effectivePreset, global, gridWidth, gridHeight);
    const quint32 cycleMs = qMax(quint32(1),
                                 PTParamMatrixEngine::effectiveDurationMs(global, effectivePreset));

    QHash<int, QSet<int>> offsetSlotsByDegree;
    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(effectivePreset, &global);
    if (global.fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    for (const PTSpatialFixtureEntry& e : plan.entries)
    {
        const PTDimmerWaveOffsetInfo info = PTDimmerWaveEngine::offsetInfoForPoint(
                e.pt.x(), e.pt.y(), gridWidth, gridHeight, waveParams, offsetPolicy);
        const int finalOffset = sweepMode
                ? qBound(0, int(std::round(e.phaseStart01 * 360.0)), 360)
                : ((info.headOffsetDeg + effectivePreset.startOffset) % 360 + 360) % 360;
        offsetSlotsByDegree[info.wingIndex * 10000 + finalOffset].insert(info.offsetSlot);
    }

    for (int y = 0; y < gridHeight; ++y)
    {
        for (int x = 0; x < gridWidth; ++x)
        {
            const QLCPoint pt(x, y);
            PTSpatialGridCellData cell;
            const PTSpatialFixtureEntry* e = plan.entryFor(pt);
            if (e)
            {
                cell.occupied = true;
                cell.chaseOrder = e->serialIndex + 1;
                const PTDimmerWaveOffsetInfo info = PTDimmerWaveEngine::offsetInfoForPoint(
                        x, y, gridWidth, gridHeight, waveParams, offsetPolicy);
                cell.wingIndex = info.wingIndex;
                cell.localIndex = info.localIndex;
                cell.blockIndex = info.blockIndex;
                cell.localOrder = info.localOrder;
                cell.offsetSlot = info.offsetSlot;
                cell.headOffsetDeg = sweepMode
                        ? qBound(0, int(std::round(e->phaseStart01 * 360.0)), 360)
                        : ((e->headOffsetDeg + effectivePreset.startOffset) % 360 + 360) % 360;
                cell.phaseStart01 = qBound(0.0,
                        sweepMode ? e->phaseStart01
                                  : PTDimmerWaveEngine::phase01AtCycleStart(cycleMs, waveParams,
                                                                            e->headOffsetDeg,
                                                                            e->serialIndex,
                                                                            plan.count()),
                        1.0);
                if (!sweepMode
                        && offsetSlotsByDegree.value(info.wingIndex * 10000 + cell.headOffsetDeg).size() > 1)
                {
                    cell.offsetCollision = true;
                    preview.hasOffsetCollisions = true;
                }
            }
            preview.cells.insert(pt, cell);
        }
    }

    preview.valid = true;
    return preview;
}

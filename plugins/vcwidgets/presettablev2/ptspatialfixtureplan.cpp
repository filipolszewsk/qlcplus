/*
  Preset Table v2 — ptspatialfixtureplan.cpp
*/

#include "ptspatialfixtureplan.h"

#include "ptdimmerwaveengine.h"
#include "presettablev2effectengine.h"

#include <QtMath>
#include <QSet>

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

    const QList<QLCPoint> chaseOrder = PresetTableV2SpatialEngine::buildChaseOrder(
            scopePoints, preset, gridWidth, gridHeight);
    const int count = qMax(1, chaseOrder.size());
    const double width01 = windowWidth01(preset);

    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(preset, &global);
    if (global.fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    plan.entries.reserve(chaseOrder.size());
    for (int i = 0; i < chaseOrder.size(); ++i)
    {
        const QLCPoint& pt = chaseOrder.at(i);
        PTSpatialFixtureEntry e;
        e.pt = pt;
        e.serialIndex = i;
        e.headOffsetDeg = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                pt.x(), pt.y(), gridWidth, gridHeight, waveParams);
        e.phaseStart01 = qBound(0.0, double(e.headOffsetDeg) / 360.0, 1.0);

        if (preset.propagation == PTPropagationMode::Serial && count > 1)
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

    if (PTParamMatrixEngine::sweepInstantForOffset(preset.offsetDirection))
        return float(globalProgress);

    const double start01 = entry.phaseStart01;
    const double width01 = windowWidth01(preset);
    const double end01 = qMin(1.0, start01 + width01);

    if (globalProgress <= start01)
        return 0.0f;
    if (globalProgress >= end01)
        return 1.0f;

    const double localPhase = (globalProgress - start01) / qMax(0.001, end01 - start01);
    const PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(preset, &global);
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

    const int span = PTDimmerWaveEngine::gridSpanAlongAxis(
            gridWidth, gridHeight, preset.axis, global.fxOrientation);
    preview.effectiveOffsetSlots = PTDimmerWaveEngine::effectiveOffsetSlotCount(span, preset);
    preview.wings = qBound(1, preset.wings, qMax(1, span));
    preview.blocks = qMax(1, preset.blocks);
    preview.slotsPerWing = PTDimmerWaveEngine::offsetSlotCountForWing(span, preset);
    preview.maxOffsetStep = PTDimmerWaveEngine::maxOffsetStepForGrid(span, preset);
    preview.offsetStepOk = preset.offsetStep <= preview.maxOffsetStep;

    const PTSpatialFixturePlan plan = build(scopePoints, preset, global, gridWidth, gridHeight);

    QHash<int, QSet<int>> offsetSlotsByDegree;
    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(preset, &global);
    if (global.fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    for (const PTSpatialFixtureEntry& e : plan.entries)
    {
        const PTDimmerWaveOffsetInfo info = PTDimmerWaveEngine::offsetInfoForPoint(
                e.pt.x(), e.pt.y(), gridWidth, gridHeight, waveParams);
        offsetSlotsByDegree[info.wingIndex * 10000 + info.headOffsetDeg].insert(info.offsetSlot);
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
                        x, y, gridWidth, gridHeight, waveParams);
                cell.wingIndex = info.wingIndex;
                cell.localIndex = info.localIndex;
                cell.blockIndex = info.blockIndex;
                cell.localOrder = info.localOrder;
                cell.offsetSlot = info.offsetSlot;
                cell.headOffsetDeg = e->headOffsetDeg;
                cell.phaseStart01 = e->phaseStart01;
                if (offsetSlotsByDegree.value(info.wingIndex * 10000 + info.headOffsetDeg).size() > 1)
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

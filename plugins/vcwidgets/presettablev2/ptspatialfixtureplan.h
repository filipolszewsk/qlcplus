/*
  Preset Table v2 — spatial fixture queue + phase starts for sweep / crossfade.
*/

#pragma once

#include <QHash>
#include <QList>
#include <QSize>
#include "qlcpoint.h"
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"

struct PTSpatialFixtureEntry
{
    QLCPoint pt;
    int      serialIndex    = 0;
    int      headOffsetDeg  = 0;
    double   phaseStart01   = 0.0;
};

struct PTSpatialGridCellData
{
    bool   occupied = false;
    int    chaseOrder = 0;
    int    headOffsetDeg = 0;
    double phaseStart01 = 0.0;
    bool   offsetCollision = false;
};

struct PTSpatialGridPreview
{
    bool  valid = false;
    QSize gridSize;
    QHash<QLCPoint, PTSpatialGridCellData> cells;
    int   effectiveOffsetSlots = 1;
    int   maxOffsetStep = 360;
    bool  offsetStepOk = true;
    bool  hasOffsetCollisions = false;
};

/** Axis → direction → wings/blocks → chase order → 360° phase → blend curve. */
class PTSpatialFixturePlan
{
public:
    QList<PTSpatialFixtureEntry> entries;
    QHash<QLCPoint, int>         indexByPoint;

    bool isEmpty() const { return entries.isEmpty(); }
    int count() const { return entries.size(); }

    const PTSpatialFixtureEntry* entryFor(const QLCPoint& pt) const;

    static PTSpatialFixturePlan build(const QList<QLCPoint>& scopePoints,
                                      const PTTransitionPreset& preset,
                                      const PTGlobalEffectSettings& global,
                                      int gridWidth, int gridHeight);

    static double windowWidth01(const PTTransitionPreset& preset);

    /** Sweep on globalProgress: max 180° packet (not full 360°). */
    static double sweepWindowWidth01(const PTTransitionPreset& preset);

    /** Ordered sweep / crossfade: globalProgress 0 = row A, 1 = row B. */
    static float sweepBlend01AtPhaseStart(double globalProgress, double phaseStart01,
                                        const PTTransitionPreset& preset,
                                        const PTGlobalEffectSettings& global);

    float sweepBlend01(double globalProgress, const QLCPoint& pt,
                       const PTTransitionPreset& preset,
                       const PTGlobalEffectSettings& global) const;

    float sweepBlend01(double globalProgress, const PTSpatialFixtureEntry& entry,
                       const PTTransitionPreset& preset,
                       const PTGlobalEffectSettings& global) const;

    static PTSpatialGridPreview buildGridPreview(const QList<QLCPoint>& scopePoints,
                                                 const PTTransitionPreset& preset,
                                                 const PTGlobalEffectSettings& global,
                                                 int gridWidth, int gridHeight);
};

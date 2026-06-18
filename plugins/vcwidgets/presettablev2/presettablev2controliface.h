/*
  Cross-plugin interface for Preset Table v2 (no link to PresetTableV2Widget class).
*/

#pragma once

#include <QtPlugin>
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"
#include "ptspatialfixtureplan.h"

class PresetTableV2ControlIface
{
public:
    virtual ~PresetTableV2ControlIface() = default;

    virtual bool spatialEffectsEnabled() const = 0;
    virtual void setSpatialEffectsEnabled(bool enabled) = 0;

    virtual quint32 linkedTransitionWidgetId() const = 0;
    virtual void setLinkedTransitionWidgetId(quint32 id) = 0;

    /** Copy presets from linked Transition widget (GUI thread only). */
    virtual void refreshTransitionPresetCache() = 0;

    /** Flash table row across outputs (Operate, MasterTimer thread safe). */
    virtual void requestTableFlash(int tableRowIndex, int transitionPresetIndex) = 0;

    /** True when Continuous FX selection edits go to staged buffers (fader ≤127). */
    virtual bool continuousCrossfadeStagedEditing() const = 0;

    virtual int outputCountForPresetOverrides() const { return 0; }
    virtual QString outputNameForPresetOverride(int outputIdx) const
    {
        Q_UNUSED(outputIdx);
        return QString();
    }
    virtual QList<QLCPoint> outputPointsForPresetOverride(int outputIdx) const
    {
        Q_UNUSED(outputIdx);
        return QList<QLCPoint>();
    }

    /** Span along preset axis from linked fixture group; 0 if unavailable. */
    virtual int fixtureGroupSpanAlongAxis(const PTTransitionPreset& preset,
                                          const PTGlobalEffectSettings& global) const = 0;

    virtual bool spatialGridPreview(const PTTransitionPreset& preset,
                                    const PTGlobalEffectSettings& global,
                                    PTSpatialGridPreview& out) const = 0;
    virtual bool spatialGridPreviewForOutput(int outputIdx,
                                             const PTTransitionPreset& preset,
                                             const PTGlobalEffectSettings& global,
                                             PTSpatialGridPreview& out) const
    {
        Q_UNUSED(outputIdx);
        return spatialGridPreview(preset, global, out);
    }

    /** @deprecated Use transition presets on linked Transition widget. */
    virtual PTSpatialEffectSettings spatialEffectSettings() const = 0;
    virtual void setSpatialEffectSettings(const PTSpatialEffectSettings& settings) = 0;

    /** True when linked table stores pan/tilt position presets (Position Mode). */
    virtual bool tableUsesPositionMode() const { return false; }

    /** All fixture-group grid points (Position / FixtureGroup mode). */
    virtual QList<QLCPoint> fixtureGroupPoints() const { return QList<QLCPoint>(); }

    /** Position stored on table row for preview (output/selection layer aware). */
    virtual PTPositionValue positionForPreview(int tableRow, int outputIdx,
                                               int selectionIdx,
                                               const QLCPoint& pt) const
    {
        Q_UNUSED(tableRow);
        Q_UNUSED(outputIdx);
        Q_UNUSED(selectionIdx);
        Q_UNUSED(pt);
        return PTPositionValue();
    }
};

Q_DECLARE_INTERFACE(PresetTableV2ControlIface, "org.qlcplus.PresetTableV2ControlIface/1.5")

class PresetTableV2PreviewStateIface
{
public:
    virtual ~PresetTableV2PreviewStateIface() = default;

    /** Read-only crossfade progress for EFX preview; active=false means static preview. */
    virtual double crossfadePreviewProgress01(bool* active) const = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2PreviewStateIface,
                    "org.qlcplus.PresetTableV2PreviewStateIface/1.0")

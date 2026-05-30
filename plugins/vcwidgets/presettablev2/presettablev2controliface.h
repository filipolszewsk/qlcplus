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

    /** True when Continuous+crossfade edits go to staged buffers (fader ≤127). */
    virtual bool continuousCrossfadeStagedEditing() const = 0;

    /** Span along preset axis from linked fixture group; 0 if unavailable. */
    virtual int fixtureGroupSpanAlongAxis(const PTTransitionPreset& preset,
                                          const PTGlobalEffectSettings& global) const = 0;

    virtual bool spatialGridPreview(const PTTransitionPreset& preset,
                                    const PTGlobalEffectSettings& global,
                                    PTSpatialGridPreview& out) const = 0;

    /** @deprecated Use transition presets on linked Transition widget. */
    virtual PTSpatialEffectSettings spatialEffectSettings() const = 0;
    virtual void setSpatialEffectSettings(const PTSpatialEffectSettings& settings) = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2ControlIface, "org.qlcplus.PresetTableV2ControlIface/1.4")

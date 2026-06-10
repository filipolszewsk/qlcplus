/*
  Cross-plugin interface for EFX Engine widget preset library (no table class link).
*/

#pragma once

#include <QtPlugin>
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"
#include "qlcpoint.h"

class PresetTableV2TransitionProviderIface
{
public:
    virtual ~PresetTableV2TransitionProviderIface() = default;

    virtual int transitionPresetCount(PTTransitionMode mode) const = 0;
    virtual PTTransitionPreset transitionPreset(PTTransitionMode mode, int index) const = 0;
    virtual PTTransitionPreset effectiveTransitionPreset(PTTransitionMode mode, int index) const = 0;
    virtual PTTransitionPreset effectiveTransitionPresetForOutput(PTTransitionMode mode,
                                                                  int index,
                                                                  int outputIdx) const
    {
        Q_UNUSED(outputIdx);
        return effectiveTransitionPreset(mode, index);
    }
    virtual PTTransitionPreset effectiveTransitionPresetForPoint(PTTransitionMode mode,
                                                                 int index,
                                                                 int outputIdx,
                                                                 const QLCPoint& point) const
    {
        Q_UNUSED(point);
        return effectiveTransitionPresetForOutput(mode, index, outputIdx);
    }
    virtual int transitionSelectionKeyForPoint(PTTransitionMode mode,
                                               int index,
                                               int outputIdx,
                                               const QLCPoint& point) const
    {
        Q_UNUSED(mode);
        Q_UNUSED(index);
        Q_UNUSED(outputIdx);
        Q_UNUSED(point);
        return -1;
    }
    virtual PTTransitionPreset effectiveTransitionPresetForSelection(PTTransitionMode mode,
                                                                     int index,
                                                                     int outputIdx,
                                                                     int selectionKey) const
    {
        if (selectionKey < 0)
            return effectiveTransitionPresetForOutput(mode, index, outputIdx);
        return effectiveTransitionPresetForOutput(mode, index, outputIdx);
    }
    virtual QString transitionPresetName(PTTransitionMode mode, int index) const = 0;

    virtual PTTransitionMode transitionMode() const = 0;
    virtual PTGlobalEffectSettings globalEffectSettings() const = 0;
    virtual bool hasLiveColumnOverride(quint8 inputId) const = 0;
    virtual void requestFlash(int tableRowIndex, int transitionPresetIndex) = 0;

    /** @deprecated EFX parameter overrides are always live; kept for ABI/IID compatibility. */
    virtual void promoteStagedColumnOverrides() = 0;

    /** True when crossfade progress follows table fader; false = global speed/min/max clock. */
    virtual bool crossfadeManualControlEnabled() const = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2TransitionProviderIface,
                    "org.qlcplus.PresetTableV2TransitionProvider/2.5")

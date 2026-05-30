/*
  Cross-plugin interface for EFX Engine widget preset library (no table class link).
*/

#pragma once

#include <QtPlugin>
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"

class PresetTableV2TransitionProviderIface
{
public:
    virtual ~PresetTableV2TransitionProviderIface() = default;

    virtual int transitionPresetCount(PTTransitionMode mode) const = 0;
    virtual PTTransitionPreset transitionPreset(PTTransitionMode mode, int index) const = 0;
    virtual PTTransitionPreset effectiveTransitionPreset(PTTransitionMode mode, int index) const = 0;
    virtual QString transitionPresetName(PTTransitionMode mode, int index) const = 0;

    virtual PTTransitionMode transitionMode() const = 0;
    virtual PTGlobalEffectSettings globalEffectSettings() const = 0;
    virtual bool hasLiveColumnOverride(quint8 inputId) const = 0;
    virtual void requestFlash(int tableRowIndex, int transitionPresetIndex) = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2TransitionProviderIface,
                    "org.qlcplus.PresetTableV2TransitionProvider/2.3")

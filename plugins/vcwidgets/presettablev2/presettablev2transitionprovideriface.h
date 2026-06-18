/*
  Cross-plugin interface for EFX Engine widget preset library (no table class link).
*/

#pragma once

#include <QtPlugin>
#include <QHash>
#include <QSet>
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"
#include "qlcpoint.h"

struct PTTransitionProviderPresetOverride
{
    PTTransitionPreset values;
    QSet<int> columns;
};

struct PTTransitionProviderSelection
{
    QString name;
    QVector<QLCPoint> cells;
    PTTransitionProviderPresetOverride overrides;
};

struct PTTransitionProviderOutputLayer
{
    PTTransitionProviderPresetOverride all;
    QVector<PTTransitionProviderSelection> selections;
};

struct PTTransitionProviderSnapshot
{
    QVector<PTTransitionPreset> sweepPresets;
    QVector<PTTransitionPreset> continuousPresets;
    QVector<PTTransitionPreset> multiFxPresets;
    QVector<PTTransitionPreset> positionMotionPresets;
    QVector<PTTransitionPreset> channel1DPresets;
    QVector<QHash<int, PTTransitionProviderOutputLayer>> sweepOutputOverrides;
    QVector<QHash<int, PTTransitionProviderOutputLayer>> continuousOutputOverrides;
    QVector<QHash<int, PTTransitionProviderOutputLayer>> multiFxOutputOverrides;
    QVector<QHash<int, PTTransitionProviderOutputLayer>> positionMotionOutputOverrides;
    QVector<QHash<int, PTTransitionProviderOutputLayer>> channel1DOutputOverrides;
    QHash<quint8, uchar> liveColumnOverrides;
    PTGlobalEffectSettings globalSettings;
    PTTransitionMode activeMode = PTTransitionMode::SweepOnly;
    bool enabled = true;
    bool crossfadeManualControl = true;
    int spanX = 0;
    int spanY = 0;
    int spanXY = 0;
    quint64 revision = 0;
};

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

    /** Immutable copy for DMX/render paths. GUI code may still use the legacy getters. */
    virtual PTTransitionProviderSnapshot transitionProviderSnapshot() const = 0;

    /** Optional master/slave bank source. invalidId means local bank. */
    virtual quint32 bankSourceEngineId(PTTransitionMode mode) const
    {
        Q_UNUSED(mode);
        return quint32(-1);
    }

    /** Linked Preset Table id owned by this provider. invalidId means not resolved yet. */
    virtual quint32 targetTableId() const
    {
        return quint32(-1);
    }
};

Q_DECLARE_INTERFACE(PresetTableV2TransitionProviderIface,
                    "org.qlcplus.PresetTableV2TransitionProvider/2.6")

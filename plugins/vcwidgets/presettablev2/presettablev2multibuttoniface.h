#pragma once

#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

class QLCInputSource;

struct PresetTableV2MultiButtonLinkedAction
{
    quint32 widgetId = 0;
    int outputIndex = 0;
    int parameter = 0;
    quint32 sourceEngineId = quint32(-1);
    quint64 phaseAnchorMs = 0;
};

class PresetTableV2MultiButtonTargetIface
{
public:
    enum Parameter
    {
        TransitionPreset = 0,
        ContinuousPreset = 1,
        MultiFxPreset = 2,
        PrimaryRow = 3,
        SecondaryRow = 4,
        PositionMotionPreset = 5,
        Channel1DPreset = 6
    };

    virtual ~PresetTableV2MultiButtonTargetIface() = default;

    virtual int multiButtonOutputCount() const = 0;
    virtual QString multiButtonOutputName(int outputIdx) const = 0;
    virtual int multiButtonParameterCount() const = 0;
    virtual QString multiButtonParameterName(int parameter) const = 0;
    virtual int multiButtonEntryCount(int outputIdx, int parameter) const = 0;
    virtual QString multiButtonEntryName(int outputIdx, int parameter, int index) const = 0;
    virtual int multiButtonCurrentIndex(int outputIdx, int parameter) const = 0;
    virtual int multiButtonLiveIndex(int outputIdx, int parameter) const = 0;
    virtual bool multiButtonStagingAvailable(int outputIdx, int parameter) const = 0;
    virtual quint64 multiButtonStateRevision(int outputIdx, int parameter) const = 0;
    virtual bool multiButtonHasStagedIndex(int outputIdx, int parameter) const = 0;
    virtual int multiButtonStagedIndex(int outputIdx, int parameter) const = 0;
    virtual bool multiButtonActivate(int outputIdx, int parameter, int index) = 0;
    virtual bool multiButtonActivateStaged(int outputIdx, int parameter, int index) = 0;
    virtual bool multiButtonActivateFromSource(int outputIdx, int parameter, int index,
                                               quint32 sourceEngineId)
    {
        return multiButtonActivateFromSourceAndPhase(outputIdx, parameter, index,
                                                     sourceEngineId, 0);
    }
    virtual bool multiButtonActivateStagedFromSource(int outputIdx, int parameter, int index,
                                                     quint32 sourceEngineId)
    {
        return multiButtonActivateStagedFromSourceAndPhase(outputIdx, parameter, index,
                                                           sourceEngineId, 0);
    }
    virtual bool multiButtonActivateFromSourceAndPhase(int outputIdx, int parameter, int index,
                                                       quint32 sourceEngineId,
                                                       quint64 phaseAnchorMs)
    {
        Q_UNUSED(sourceEngineId);
        Q_UNUSED(phaseAnchorMs);
        return multiButtonActivate(outputIdx, parameter, index);
    }
    virtual bool multiButtonActivateStagedFromSourceAndPhase(int outputIdx, int parameter,
                                                             int index,
                                                             quint32 sourceEngineId,
                                                             quint64 phaseAnchorMs)
    {
        Q_UNUSED(sourceEngineId);
        Q_UNUSED(phaseAnchorMs);
        return multiButtonActivateStaged(outputIdx, parameter, index);
    }
    virtual QSharedPointer<QLCInputSource> multiButtonLiveInputSource(int outputIdx, int parameter) const = 0;
    virtual bool multiButtonSetLiveInputSource(int outputIdx, int parameter,
                                               QSharedPointer<QLCInputSource> src) = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2MultiButtonTargetIface,
                    "org.qlcplus.PresetTableV2MultiButtonTargetIface/1.3")

class PresetTableV2MultiButtonTargetExtrasIface
{
public:
    virtual ~PresetTableV2MultiButtonTargetExtrasIface() = default;
    virtual bool multiButtonSupportsAllOutputs() const = 0;
    virtual bool multiButtonOutputControlsParameter(int outputIdx, int parameter) const = 0;
    virtual QList<PresetTableV2MultiButtonLinkedAction> multiButtonLinkedSlaveActions(
            int outputIdx, int parameter) const
    {
        Q_UNUSED(outputIdx);
        Q_UNUSED(parameter);
        return {};
    }
    virtual QList<PresetTableV2MultiButtonLinkedAction> multiButtonLinkedSlaveActionsForIndex(
            int outputIdx, int parameter, int index) const
    {
        Q_UNUSED(index);
        return multiButtonLinkedSlaveActions(outputIdx, parameter);
    }
};

Q_DECLARE_INTERFACE(PresetTableV2MultiButtonTargetExtrasIface,
                    "org.qlcplus.PresetTableV2MultiButtonTargetExtrasIface/1.4")

class PresetTableV2MultiButtonFlashIface
{
public:
    virtual ~PresetTableV2MultiButtonFlashIface() = default;

    virtual bool multiButtonBeginFlash(int outputIdx, int parameter, int index,
                                       quint32 sourceWidgetId, quint64 token,
                                       double timeMultiplier = 1.0) = 0;
    virtual bool multiButtonBeginFlashFromSourceAndPhase(int outputIdx, int parameter,
                                                         int index,
                                                         quint32 sourceWidgetId,
                                                         quint64 token,
                                                         quint32 sourceEngineId,
                                                         quint64 phaseAnchorMs,
                                                         double timeMultiplier = 1.0)
    {
        Q_UNUSED(sourceEngineId);
        Q_UNUSED(phaseAnchorMs);
        return multiButtonBeginFlash(outputIdx, parameter, index,
                                     sourceWidgetId, token, timeMultiplier);
    }
    virtual bool multiButtonEndFlash(int outputIdx, int parameter, int index,
                                     quint32 sourceWidgetId, quint64 token) = 0;
    virtual bool multiButtonEndFlashFromSource(int outputIdx, int parameter, int index,
                                               quint32 sourceWidgetId, quint64 token,
                                               quint32 sourceEngineId)
    {
        Q_UNUSED(sourceEngineId);
        return multiButtonEndFlash(outputIdx, parameter, index, sourceWidgetId, token);
    }
    virtual bool multiButtonFlashGateActive() const = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2MultiButtonFlashIface,
                    "org.qlcplus.PresetTableV2MultiButtonFlashIface/1.3")

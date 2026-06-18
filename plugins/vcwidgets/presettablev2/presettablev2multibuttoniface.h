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
    virtual QSharedPointer<QLCInputSource> multiButtonLiveInputSource(int outputIdx, int parameter) const = 0;
    virtual bool multiButtonSetLiveInputSource(int outputIdx, int parameter,
                                               QSharedPointer<QLCInputSource> src) = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2MultiButtonTargetIface,
                    "org.qlcplus.PresetTableV2MultiButtonTargetIface/1.1")

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
};

Q_DECLARE_INTERFACE(PresetTableV2MultiButtonTargetExtrasIface,
                    "org.qlcplus.PresetTableV2MultiButtonTargetExtrasIface/1.2")

class PresetTableV2MultiButtonFlashIface
{
public:
    virtual ~PresetTableV2MultiButtonFlashIface() = default;

    virtual bool multiButtonBeginFlash(int outputIdx, int parameter, int index,
                                       quint32 sourceWidgetId, quint64 token,
                                       double timeMultiplier = 1.0) = 0;
    virtual bool multiButtonEndFlash(int outputIdx, int parameter, int index,
                                     quint32 sourceWidgetId, quint64 token) = 0;
    virtual bool multiButtonFlashGateActive() const = 0;
};

Q_DECLARE_INTERFACE(PresetTableV2MultiButtonFlashIface,
                    "org.qlcplus.PresetTableV2MultiButtonFlashIface/1.2")

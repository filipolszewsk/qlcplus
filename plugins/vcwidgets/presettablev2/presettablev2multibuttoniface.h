#pragma once

#include <QSharedPointer>
#include <QString>

class QLCInputSource;

class PresetTableV2MultiButtonTargetIface
{
public:
    enum Parameter
    {
        TransitionPreset = 0,
        ContinuousPreset = 1,
        MultiFxPreset = 2,
        PrimaryRow = 3,
        SecondaryRow = 4
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
                    "org.qlcplus.PresetTableV2MultiButtonTargetIface/1.0")

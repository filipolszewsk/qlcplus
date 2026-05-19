/*
  QLC+ VC Widget Plugin — Infinity Encoder
  infinityencoderplugin.h — Apache 2.0 / public domain
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class InfinityEncoderPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.vcwidgets.infinityencoder"); }
    QString name()        const override { return QStringLiteral("Infinity Encoder"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override { return QStringLiteral("Rotary infinity encoder — controls DMX channels relative (like grandMA encoders). Supports A/B/C/D channel banks, 3 sensitivity levels and MIDI/OSC external input."); }
    QString category()    const override { return QStringLiteral("DMX Control"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("media-record"),
                                QIcon::fromTheme(QStringLiteral("input-gaming")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

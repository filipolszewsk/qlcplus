/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericplugin.h — Apache 2.0 / public domain
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class DMXNumericPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.dmxnumeric"); }
    QString name()        const override { return QStringLiteral("DMX Numeric"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override { return QStringLiteral("Type a DMX value (0-255) directly to a configured universe/channel."); }
    QString category()    const override { return QStringLiteral("DMX"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("input-gaming"),
                                QIcon::fromTheme(QStringLiteral("utilities-terminal")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

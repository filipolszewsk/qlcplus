/*
  QLC+ VC Widget Plugin — Preset Table
  presettableplugin.h — Apache 2.0 / public domain
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class PresetTablePlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.vcwidgets.presettable"); }
    QString name()        const override { return QStringLiteral("Preset Table"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override { return QStringLiteral("A DMX preset table: rows of named presets, columns of channels, multiple outputs selectable via external input."); }
    QString category()    const override { return QStringLiteral("DMX"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("view-list-details"),
                                QIcon::fromTheme(QStringLiteral("utilities-terminal")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

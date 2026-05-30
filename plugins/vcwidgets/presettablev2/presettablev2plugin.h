/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2plugin.h — Apache 2.0 / public domain
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class PresetTableV2Plugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.vcwidgets.presettablev2"); }
    QString name()        const override { return QStringLiteral("Preset Table v2"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override {
        return QStringLiteral("Preset table with mask-aware outputs and spatial stagger on row recall (Fixture Group mode).");
    }
    QString category()    const override { return QStringLiteral("DMX"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("view-list-details"),
                                QIcon::fromTheme(QStringLiteral("utilities-terminal")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

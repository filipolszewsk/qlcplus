/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutplugin.h — Apache 2.0 / public domain
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class FixtureGroupLayoutPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.vcwidgets.fixturegrouplayout"); }
    QString name()        const override { return QStringLiteral("Fixture Group Layout"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override
    {
        return QStringLiteral("Display a fixture group grid and rearrange fixture positions.");
    }
    QString category()    const override { return QStringLiteral("Fixtures"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("view-grid"),
                                QIcon::fromTheme(QStringLiteral("format-indent-more")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

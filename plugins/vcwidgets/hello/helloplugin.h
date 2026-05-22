/*
  QLC+ VC Widget Plugin Example
  helloplugin.h — public domain (CC0 1.0)
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

/**
 * HelloPlugin — the factory object loaded by QPluginLoader.
 *
 * This is the ONLY class that needs Q_PLUGIN_METADATA.
 * The widget itself (HelloWidget) is a plain VCWidget subclass.
 */
class HelloPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()   const override { return QStringLiteral("org.qlcplus.example.hellowidget"); }
    QString name()       const override { return QStringLiteral("Hello Widget"); }
    QString version()    const override { return QStringLiteral("1.0.0"); }
    QString author()     const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override { return QStringLiteral("Example VC widget plugin for QLC+."); }
    QString homepage()   const override { return QStringLiteral("https://github.com/gridqlc/qlcplus"); }
    QString category()   const override { return QStringLiteral("Example"); }

    QIcon icon() const override
    {
        // Return a default icon; replace with your own resource.
        return QIcon::fromTheme(QStringLiteral("help-about"));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

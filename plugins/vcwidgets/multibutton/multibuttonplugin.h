/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonplugin.h — Apache 2.0 / public domain
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class MultiButtonPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.vcwidgets.multibutton"); }
    QString name()        const override { return QStringLiteral("Multi Button"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override { return QStringLiteral("Cyclic multi-function button — short click cycles through assigned scenes/functions; long press or right-click opens a direct-pick popup menu. Dot indicators show current position."); }
    QString category()    const override { return QStringLiteral("Function Control"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("media-skip-forward"),
                                QIcon::fromTheme(QStringLiteral("go-next")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

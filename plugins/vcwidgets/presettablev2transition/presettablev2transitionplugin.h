/*
  QLC+ VC Widget Plugin — Preset Table v2 Transition
*/

#pragma once

#include <QObject>
#include "vcwidgetplugininterface.h"

class PresetTableV2TransitionPlugin : public QObject, public VCWidgetPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
    Q_INTERFACES(VCWidgetPluginInterface)

public:
    QString pluginId()    const override { return QStringLiteral("org.qlcplus.vcwidgets.presettablev2transition"); }
    QString name()        const override { return QStringLiteral("Preset Table v2 Transition"); }
    QString version()     const override { return QStringLiteral("1.0.0"); }
    QString author()      const override { return QStringLiteral("QLC+ Community"); }
    QString description() const override
    {
        return QStringLiteral("Transition panel for Preset Table v2: direction, delay, fade (no DMX).");
    }
    QString category()    const override { return QStringLiteral("DMX"); }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("transform-move"),
                                QIcon::fromTheme(QStringLiteral("view-list-details")));
    }

    VCWidget* createWidget(QWidget* parent, Doc* doc) override;
};

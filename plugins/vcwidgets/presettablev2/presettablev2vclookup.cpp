#include "presettablev2vclookup.h"
#include "presettablev2widget.h"
#include "presettablev2controliface.h"
#include "presettablev2transitionprovideriface.h"

#include "virtualconsole.h"
#include "vcframe.h"
#include "vcwidget.h"

static const char kTransitionClassName[] = "PresetTableV2TransitionWidget";

QList<VCWidget*> PresetTableV2VCLookup::allVcWidgets()
{
    QList<VCWidget*> result;
    VirtualConsole* vc = VirtualConsole::instance();
    VCFrame* root = vc ? vc->contents() : nullptr;
    if (!root)
        return result;

    return root->findChildren<VCWidget*>(QString(), Qt::FindChildrenRecursively);
}

PresetTableV2ControlIface* PresetTableV2VCLookup::controlIfaceByVcId(quint32 id)
{
    if (id == VCWidget::invalidId())
        return nullptr;

    for (VCWidget* candidate : allVcWidgets())
    {
        if (!candidate || candidate->id() != id)
            continue;
        return qobject_cast<PresetTableV2ControlIface*>(candidate);
    }
    return nullptr;
}

PresetTableV2TransitionProviderIface* PresetTableV2VCLookup::transitionProviderByVcId(quint32 id)
{
    if (id == VCWidget::invalidId())
        return nullptr;

    for (VCWidget* candidate : allVcWidgets())
    {
        if (!candidate || candidate->id() != id)
            continue;
        if (QString::fromLatin1(candidate->metaObject()->className())
                != QLatin1String(kTransitionClassName))
            continue;
        return qobject_cast<PresetTableV2TransitionProviderIface*>(candidate);
    }
    return nullptr;
}

QList<PresetTableV2Widget*> PresetTableV2VCLookup::allTables()
{
    QList<PresetTableV2Widget*> tables;
    for (VCWidget* w : allVcWidgets())
    {
        if (auto* iface = qobject_cast<PresetTableV2ControlIface*>(w))
            tables.append(static_cast<PresetTableV2Widget*>(iface));
    }
    return tables;
}

QList<VCWidget*> PresetTableV2VCLookup::allTransitionWidgets()
{
    QList<VCWidget*> list;
    for (VCWidget* w : allVcWidgets())
    {
        if (w && QString::fromLatin1(w->metaObject()->className()) == QLatin1String(kTransitionClassName))
            list.append(w);
    }
    return list;
}

QString PresetTableV2VCLookup::tableLabel(PresetTableV2Widget* table)
{
    if (!table)
        return QString();
    return QStringLiteral("#%1 %2").arg(table->id()).arg(table->caption());
}

QString PresetTableV2VCLookup::vcWidgetLabel(VCWidget* widget)
{
    if (!widget)
        return QString();
    return QStringLiteral("#%1 %2").arg(widget->id()).arg(widget->caption());
}

/*
  QLC+ VC Widget Plugin — Preset Table v2
  Helpers to find linked VC widgets (GUI thread).
*/

#pragma once

#include <QList>
#include <QString>

class VCWidget;
class PresetTableV2Widget;
class PresetTableV2ControlIface;
class PresetTableV2TransitionProviderIface;

namespace PresetTableV2VCLookup
{
QList<VCWidget*> allVcWidgets();
/** Live VC widget tree lookup by id (avoids stale VirtualConsole::widget map entries). */
PresetTableV2ControlIface* controlIfaceByVcId(quint32 id);
PresetTableV2TransitionProviderIface* transitionProviderByVcId(quint32 id);
QList<PresetTableV2Widget*> allTables();
QList<VCWidget*> allTransitionWidgets();

QString tableLabel(PresetTableV2Widget* table);
QString vcWidgetLabel(VCWidget* widget);
}

/*
  QLC+ VC Widget Plugin — Preset Table v2
  Helpers to find linked VC widgets (GUI thread).
*/

#pragma once

#include <QList>
#include <QString>

class VCWidget;
class PresetTableV2Widget;

namespace PresetTableV2VCLookup
{
QList<VCWidget*> allVcWidgets();
QList<PresetTableV2Widget*> allTables();
QList<VCWidget*> allTransitionWidgets();

QString tableLabel(PresetTableV2Widget* table);
QString vcWidgetLabel(VCWidget* widget);
}

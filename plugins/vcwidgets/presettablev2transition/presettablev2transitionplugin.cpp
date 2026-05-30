/*
  QLC+ VC Widget Plugin — Preset Table v2 Transition
*/

#include "presettablev2transitionplugin.h"
#include "presettablev2transitionwidget.h"

VCWidget* PresetTableV2TransitionPlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new PresetTableV2TransitionWidget(parent, doc);
}

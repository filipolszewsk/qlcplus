/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2plugin.cpp — Apache 2.0 / public domain
*/

#include "presettablev2plugin.h"
#include "presettablev2widget.h"

VCWidget* PresetTableV2Plugin::createWidget(QWidget* parent, Doc* doc)
{
    return new PresetTableV2Widget(parent, doc);
}

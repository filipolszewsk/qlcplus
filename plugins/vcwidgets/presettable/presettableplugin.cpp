/*
  QLC+ VC Widget Plugin — Preset Table
  presettableplugin.cpp — Apache 2.0 / public domain
*/

#include "presettableplugin.h"
#include "presettablewidget.h"

VCWidget* PresetTablePlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new PresetTableWidget(parent, doc);
}

/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericplugin.cpp — Apache 2.0 / public domain
*/

#include "dmxnumericplugin.h"
#include "dmxnumericwidget.h"

VCWidget* DMXNumericPlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new DMXNumericWidget(parent, doc);
}

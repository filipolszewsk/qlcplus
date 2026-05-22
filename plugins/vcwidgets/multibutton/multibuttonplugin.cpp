/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonplugin.cpp — Apache 2.0 / public domain
*/

#include "multibuttonplugin.h"
#include "multibuttonwidget.h"

VCWidget* MultiButtonPlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new MultiButtonWidget(parent, doc);
}

/*
  QLC+ VC Widget Plugin Example
  helloplugin.cpp — public domain (CC0 1.0)
*/

#include "helloplugin.h"
#include "hellowidget.h"

VCWidget* HelloPlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new HelloWidget(parent, doc);
}

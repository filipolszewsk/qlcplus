/*
  QLC+ VC Widget Plugin — Infinity Encoder
  infinityencoderplugin.cpp — Apache 2.0 / public domain
*/

#include "infinityencoderplugin.h"
#include "infinityencoderwidget.h"

VCWidget* InfinityEncoderPlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new InfinityEncoderWidget(parent, doc);
}

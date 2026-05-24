/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutplugin.cpp — Apache 2.0 / public domain
*/

#include "fixturegrouplayoutplugin.h"
#include "fixturegrouplayoutwidget.h"

VCWidget* FixtureGroupLayoutPlugin::createWidget(QWidget* parent, Doc* doc)
{
    return new FixtureGroupLayoutWidget(parent, doc);
}

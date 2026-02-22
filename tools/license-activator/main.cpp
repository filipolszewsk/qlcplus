/*
  QLC+ License Activator
  main.cpp

  Copyright (c) Filip Olszewski
  PROPRIETARY - NOT OPEN SOURCE
*/

#include <QApplication>
#include "activatorwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QLC+ License Activator");

    ActivatorWindow window;
    window.show();

    return app.exec();
}

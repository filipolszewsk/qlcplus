/*
  vcplugindiagnostics.h
*/

#pragma once

#include <QtGlobal>
#include <QString>

class VCPluginDiagnostics
{
public:
    static void install(const QString& pluginId, quint32 widgetId,
                        const QString& caption);
    static void breadcrumb(const QString& pluginId, quint32 widgetId,
                           const QString& caption, const QString& message,
                           bool immediateFlush = false);
    static void breadcrumbRateLimited(const QString& pluginId, quint32 widgetId,
                                      const QString& caption, const QString& key,
                                      int intervalMs, const QString& message,
                                      bool immediateFlush = false);
    static QString logFilePath();
    static QString crashLogFilePath();
};

/*
  Q Light Controller Plus
  vcwidgetpluginmanager.h

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef VCWIDGETPLUGINMANAGER_H
#define VCWIDGETPLUGINMANAGER_H

#include <QObject>
#include <QDir>
#include <QList>
#include <QMap>

class VCWidgetPluginInterface;

/**
 * Holds metadata about a scanned plugin file, including load errors.
 * Used by the Plugin Manager UI to show status for each file.
 */
struct VCWidgetPluginEntry
{
    QString filePath;
    VCWidgetPluginInterface* plugin = nullptr;   ///< null if load failed
    QString errorString;                         ///< populated when plugin == nullptr
};

/**
 * VCWidgetPluginManager scans configured directories for VC widget plugins
 * and makes them available to the rest of the application.
 *
 * Usage:
 *   VCWidgetPluginManager::instance()->load(VCWidgetPluginManager::systemPluginDirectory());
 *   VCWidgetPluginManager::instance()->load(VCWidgetPluginManager::userPluginDirectory());
 *
 * After loading, plugins() returns the successfully loaded plugins and
 * entries() returns all scanned files including failed ones (for UI display).
 */
class VCWidgetPluginManager : public QObject
{
    Q_OBJECT

public:
    ~VCWidgetPluginManager();

    /** Returns the application-wide singleton instance. */
    static VCWidgetPluginManager* instance();

    /**
     * Scan the given directory and attempt to load each file as a
     * VCWidgetPlugin. Already-loaded plugin IDs are skipped (dedup).
     * Safe to call multiple times with different directories.
     */
    void load(const QDir& dir);

    /** List of successfully loaded plugins, in load order. */
    QList<VCWidgetPluginInterface*> plugins() const;

    /**
     * Find a loaded plugin by its unique pluginId().
     * Returns nullptr if no matching plugin is loaded.
     */
    VCWidgetPluginInterface* pluginById(const QString& id) const;

    /**
     * All scanned entries (loaded + failed), for the Plugin Manager dialog.
     * Entries with plugin == nullptr have a non-empty errorString.
     */
    QList<VCWidgetPluginEntry> entries() const;

    /**
     * System-wide plugin directory (installed alongside QLC+ binary).
     * Corresponds to VCWIDGETPLUGINDIR from qlcconfig.h.
     */
    static QDir systemPluginDirectory();

    /**
     * Per-user plugin directory (user-writable, no admin required).
     * Corresponds to USERVCWIDGETPLUGINDIR from qlcconfig.h.
     *
     *   macOS:   ~/Library/Application Support/QLC+/VCWidgets/
     *   Linux:   ~/.qlcplus/vcwidgets/
     *   Windows: %UserProfile%\QLC+\VCWidgets\
     */
    static QDir userPluginDirectory();

signals:
    /** Emitted after load() adds one or more new plugins. */
    void pluginsChanged();

private:
    explicit VCWidgetPluginManager(QObject* parent = nullptr);

    static VCWidgetPluginManager* s_instance;

    QList<VCWidgetPluginInterface*> m_plugins;
    QList<VCWidgetPluginEntry>      m_entries;
    QMap<QString, VCWidgetPluginInterface*> m_pluginById;
};

#endif // VCWIDGETPLUGINMANAGER_H

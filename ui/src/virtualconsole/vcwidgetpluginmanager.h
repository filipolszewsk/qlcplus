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
#include <QTimer>

class VCWidgetPluginInterface;
class QPluginLoader;
class QFileSystemWatcher;

/**
 * Holds metadata about a scanned plugin file, including load errors.
 * Used by the Plugin Manager UI to show status for each file.
 */
struct VCWidgetPluginEntry
{
    QString filePath;
    VCWidgetPluginInterface* plugin = nullptr;   ///< null if load failed
    QString errorString;                         ///< populated when plugin == nullptr
    QPluginLoader* loader = nullptr;             ///< kept alive so we can unload/reload
};

/**
 * VCWidgetPluginManager scans configured directories for VC widget plugins
 * and makes them available to the rest of the application.
 *
 * Supports hot-reload: file system changes in the user directory are detected
 * automatically and trigger a reload of new/changed plugins.
 *
 * Usage:
 *   VCWidgetPluginManager::instance()->load(VCWidgetPluginManager::systemPluginDirectory());
 *   VCWidgetPluginManager::instance()->load(VCWidgetPluginManager::userPluginDirectory());
 *   VCWidgetPluginManager::instance()->startFileWatcher();
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

    /**
     * Reload a single plugin file (e.g. after it has been updated on disk).
     * If the plugin was previously loaded, it is unloaded first.
     * Emits pluginsChanged() if the set of loaded plugins changes.
     * Returns true if the plugin was successfully loaded after the reload.
     */
    bool reloadFile(const QString& filePath);

    /**
     * Unload and remove a plugin entry by its file path.
     * Does NOT delete the file from disk.
     * Emits pluginsChanged() if the plugin was loaded.
     */
    void removeEntry(const QString& filePath);

    /**
     * Unload and remove a plugin by its ID.
     * Returns false if no plugin with that ID is loaded.
     */
    bool unloadById(const QString& id);

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

    /**
     * Start watching the user plugin directory for new/changed plugin files.
     * When a new file appears or an existing one is modified, the plugin is
     * automatically loaded or reloaded.
     *
     * This is safe to call multiple times; it will only start the watcher once.
     */
    void startFileWatcher();

    /** Stop the file system watcher. */
    void stopFileWatcher();

signals:
    /** Emitted after load() adds one or more new plugins. */
    void pluginsChanged();

    /**
     * Emitted BEFORE unloading the old plugin library during a hot-reload.
     * Any observer that holds live instances of widgets created by this plugin
     * MUST destroy them synchronously in response to this signal (connect with
     * Qt::DirectConnection) before the library is actually unloaded.
     *
     * @p pluginId  the plugin ID that is about to be reloaded.
     */
    void aboutToReloadPlugin(const QString& pluginId);

    /**
     * Emitted when a specific plugin has been reloaded (hot-reload).
     * The new plugin is fully loaded and ready when this signal is received.
     * @p pluginId  the plugin's pluginId(), or empty string if the plugin
     *              failed to load after the reload.
     */
    void pluginReloaded(const QString& pluginId);

private slots:
    void slotDirectoryChanged(const QString& path);
    void slotFileChanged(const QString& path);
    void slotReloadDebounced();

private:
    explicit VCWidgetPluginManager(QObject* parent = nullptr);

    /** Load a single file. Returns true if the plugin was newly added. */
    bool loadFile(const QString& filePath);

    /** Unload a single entry (does not remove from lists). */
    void unloadEntry(VCWidgetPluginEntry& entry);

    static VCWidgetPluginManager* s_instance;

    QList<VCWidgetPluginEntry>      m_entries;
    QMap<QString, VCWidgetPluginInterface*> m_pluginById;

    QFileSystemWatcher* m_watcher = nullptr;

    // Debounce timer: linkers write files in multiple phases, so we wait
    // a short time after the last file event before actually reloading.
    QTimer  m_debounceTimer;
    QStringList m_pendingReload;  ///< files queued for reload after debounce
};

#endif // VCWIDGETPLUGINMANAGER_H

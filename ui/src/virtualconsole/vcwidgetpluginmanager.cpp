/*
  Q Light Controller Plus
  vcwidgetpluginmanager.cpp

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

#include <QPluginLoader>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDebug>

#include "vcwidgetpluginmanager.h"
#include "vcwidgetplugininterface.h"
#include "qlcconfig.h"
#include "qlcfile.h"

VCWidgetPluginManager* VCWidgetPluginManager::s_instance = nullptr;

VCWidgetPluginManager::VCWidgetPluginManager(QObject* parent)
    : QObject(parent)
{
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(600);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &VCWidgetPluginManager::slotReloadDebounced);
}

VCWidgetPluginManager::~VCWidgetPluginManager()
{
    stopFileWatcher();

    // Unload all loaders. Qt plugin instances are owned by QPluginLoader,
    // so calling unload() releases the library.
    for (VCWidgetPluginEntry& entry : m_entries)
        unloadEntry(entry);

    s_instance = nullptr;
}

VCWidgetPluginManager* VCWidgetPluginManager::instance()
{
    if (s_instance == nullptr)
        s_instance = new VCWidgetPluginManager();
    return s_instance;
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

void VCWidgetPluginManager::load(const QDir& dir)
{
    qDebug() << Q_FUNC_INFO << dir.path();

    if (!dir.exists() || !dir.isReadable())
        return;

    bool anyAdded = false;

    const QStringList fileNames = dir.entryList();
    for (const QString& fileName : fileNames)
    {
        const QString path = dir.absoluteFilePath(fileName);
        if (loadFile(path))
            anyAdded = true;
    }

    if (anyAdded)
        emit pluginsChanged();
}

bool VCWidgetPluginManager::loadFile(const QString& filePath)
{
    // Skip directories and non-plugin files (e.g. manifest.json, bundle dirs)
    QFileInfo fi(filePath);
    if (!fi.isFile() || !fi.fileName().endsWith(KExtPlugin, Qt::CaseInsensitive))
        return false;

    // Skip if this path is already tracked (loaded or failed)
    for (const VCWidgetPluginEntry& e : m_entries)
    {
        if (e.filePath == filePath)
            return false;
    }

    QPluginLoader* loader = new QPluginLoader(filePath, this);
    QObject* obj = loader->instance();

    VCWidgetPluginEntry entry;
    entry.filePath = filePath;
    entry.loader   = loader;

    if (obj == nullptr)
    {
        entry.errorString = loader->errorString();
        qWarning() << Q_FUNC_INFO << QFileInfo(filePath).fileName()
                   << "failed to load:" << entry.errorString;
        m_entries.append(entry);
        loader->unload();
        return false;
    }

    VCWidgetPluginInterface* plugin =
        qobject_cast<VCWidgetPluginInterface*>(obj);

    if (plugin == nullptr)
    {
        // Valid Qt plugin but not a VCWidgetPlugin — silently skip.
        loader->unload();
        delete loader;
        return false;
    }

    const QString id = plugin->pluginId();

    if (m_pluginById.contains(id))
    {
        qWarning() << Q_FUNC_INFO << "Duplicate VC widget plugin id"
                   << id << "in" << filePath << "— discarded";
        entry.errorString = QString("Duplicate plugin id: %1").arg(id);
        m_entries.append(entry);
        loader->unload();
        return false;
    }

    qDebug() << "Loaded VC widget plugin" << plugin->name()
             << "v" << plugin->version()
             << "by" << plugin->author()
             << "from" << QFileInfo(filePath).fileName();

    entry.plugin = plugin;
    m_entries.append(entry);
    m_pluginById.insert(id, plugin);
    return true;
}

// ---------------------------------------------------------------------------
// Reload / unload
// ---------------------------------------------------------------------------

void VCWidgetPluginManager::unloadEntry(VCWidgetPluginEntry& entry)
{
    if (entry.plugin != nullptr)
        m_pluginById.remove(entry.plugin->pluginId());

    entry.plugin = nullptr;

    if (entry.loader != nullptr)
    {
        entry.loader->unload();
        entry.loader->deleteLater();
        entry.loader = nullptr;
    }
}

bool VCWidgetPluginManager::reloadFile(const QString& filePath)
{
    // Find existing entry for this path (if any)
    int idx = -1;
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].filePath == filePath)
        {
            idx = i;
            break;
        }
    }

    QString previousId;

    if (idx >= 0)
    {
        VCWidgetPluginEntry& existing = m_entries[idx];
        if (existing.plugin)
            previousId = existing.plugin->pluginId();

        // Notify observers BEFORE unloading so they can destroy live instances.
        // Must be a synchronous (direct) connection — library unload follows immediately.
        if (!previousId.isEmpty())
            emit aboutToReloadPlugin(previousId);

        unloadEntry(existing);
        m_entries.removeAt(idx);
    }

    // Re-load
    QPluginLoader* loader = new QPluginLoader(filePath, this);
    QObject* obj = loader->instance();

    VCWidgetPluginEntry entry;
    entry.filePath = filePath;
    entry.loader   = loader;

    if (obj == nullptr)
    {
        entry.errorString = loader->errorString();
        qWarning() << Q_FUNC_INFO << QFileInfo(filePath).fileName()
                   << "failed to reload:" << entry.errorString;
        m_entries.append(entry);
        loader->unload();
        emit pluginsChanged();
        emit pluginReloaded(QString());
        return false;
    }

    VCWidgetPluginInterface* plugin =
        qobject_cast<VCWidgetPluginInterface*>(obj);

    if (plugin == nullptr)
    {
        loader->unload();
        delete loader;
        emit pluginsChanged();
        return false;
    }

    const QString id = plugin->pluginId();

    if (m_pluginById.contains(id) && id != previousId)
    {
        qWarning() << Q_FUNC_INFO << "Reload: duplicate plugin id" << id;
        entry.errorString = QString("Duplicate plugin id: %1").arg(id);
        m_entries.append(entry);
        loader->unload();
        emit pluginsChanged();
        return false;
    }

    entry.plugin = plugin;
    m_entries.append(entry);
    m_pluginById.insert(id, plugin);

    qDebug() << "Reloaded VC widget plugin" << plugin->name()
             << "v" << plugin->version()
             << "from" << QFileInfo(filePath).fileName();

    emit pluginsChanged();
    emit pluginReloaded(id);
    return true;
}

void VCWidgetPluginManager::removeEntry(const QString& filePath)
{
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].filePath == filePath)
        {
            bool wasLoaded = (m_entries[i].plugin != nullptr);
            unloadEntry(m_entries[i]);
            m_entries.removeAt(i);
            if (wasLoaded)
                emit pluginsChanged();
            return;
        }
    }
}

bool VCWidgetPluginManager::unloadById(const QString& id)
{
    VCWidgetPluginInterface* plugin = m_pluginById.value(id, nullptr);
    if (!plugin)
        return false;

    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].plugin == plugin)
        {
            unloadEntry(m_entries[i]);
            m_entries.removeAt(i);
            emit pluginsChanged();
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

QList<VCWidgetPluginInterface*> VCWidgetPluginManager::plugins() const
{
    QList<VCWidgetPluginInterface*> result;
    for (const VCWidgetPluginEntry& e : m_entries)
    {
        if (e.plugin)
            result.append(e.plugin);
    }
    return result;
}

VCWidgetPluginInterface* VCWidgetPluginManager::pluginById(const QString& id) const
{
    return m_pluginById.value(id, nullptr);
}

QList<VCWidgetPluginEntry> VCWidgetPluginManager::entries() const
{
    return m_entries;
}

// ---------------------------------------------------------------------------
// Directories
// ---------------------------------------------------------------------------

QDir VCWidgetPluginManager::systemPluginDirectory()
{
    return QLCFile::systemDirectory(VCWIDGETPLUGINDIR, KExtPlugin);
}

QDir VCWidgetPluginManager::userPluginDirectory()
{
    return QLCFile::userDirectory(USERVCWIDGETPLUGINDIR,
                                  USERVCWIDGETPLUGINDIR,
                                  QStringList() << QString("*%1").arg(KExtPlugin));
}

// ---------------------------------------------------------------------------
// File watcher
// ---------------------------------------------------------------------------

void VCWidgetPluginManager::startFileWatcher()
{
    if (m_watcher)
        return;

    m_watcher = new QFileSystemWatcher(this);

    QDir userDir = userPluginDirectory();
    if (!userDir.exists())
        userDir.mkpath(userDir.absolutePath());

    m_watcher->addPath(userDir.absolutePath());

    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &VCWidgetPluginManager::slotDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &VCWidgetPluginManager::slotFileChanged);

    // Also watch individual plugin files already loaded from the user dir
    const QString userPath = userDir.absolutePath();
    for (const VCWidgetPluginEntry& e : m_entries)
    {
        if (e.filePath.startsWith(userPath) && !e.filePath.isEmpty())
            m_watcher->addPath(e.filePath);
    }
}

void VCWidgetPluginManager::stopFileWatcher()
{
    if (m_watcher)
    {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    m_debounceTimer.stop();
}

void VCWidgetPluginManager::slotDirectoryChanged(const QString& path)
{
    // A file was added, removed, or renamed in the watched directory.
    // Diff the current disk contents against our entry list.
    QDir dir(path);
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << QString("*%1").arg(KExtPlugin));

    const QStringList diskFiles = dir.entryList();

    // Collect currently tracked files in this directory
    QStringList trackedFiles;
    for (const VCWidgetPluginEntry& e : m_entries)
    {
        if (QFileInfo(e.filePath).absolutePath() == path)
            trackedFiles.append(e.filePath);
    }

    bool anyChange = false;

    // New files on disk that we haven't seen
    for (const QString& name : diskFiles)
    {
        const QString fullPath = dir.absoluteFilePath(name);
        if (!trackedFiles.contains(fullPath))
        {
            if (loadFile(fullPath))
            {
                if (m_watcher)
                    m_watcher->addPath(fullPath);
                anyChange = true;
            }
        }
    }

    // Tracked files that disappeared from disk
    const QStringList diskFullPaths = [&]() {
        QStringList r;
        for (const QString& n : diskFiles)
            r << dir.absoluteFilePath(n);
        return r;
    }();

    for (const QString& tracked : trackedFiles)
    {
        if (!diskFullPaths.contains(tracked))
        {
            removeEntry(tracked);
            anyChange = true;
        }
    }

    if (anyChange)
        emit pluginsChanged();
}

void VCWidgetPluginManager::slotFileChanged(const QString& path)
{
    // An existing plugin file was modified (e.g. developer rebuilt it).
    // Debounce: linkers/copy tools write files in phases, so wait a moment.
    if (!m_pendingReload.contains(path))
        m_pendingReload.append(path);

    m_debounceTimer.start();
}

void VCWidgetPluginManager::slotReloadDebounced()
{
    const QStringList toReload = m_pendingReload;
    m_pendingReload.clear();

    for (const QString& path : toReload)
    {
        // Re-add to watcher — on some platforms file-changed removes the watch
        if (m_watcher && !m_watcher->files().contains(path)
            && QFile::exists(path))
        {
            m_watcher->addPath(path);
        }

        if (QFile::exists(path))
            reloadFile(path);
    }
}

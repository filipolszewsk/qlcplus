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
#include <QDebug>

#include "vcwidgetpluginmanager.h"
#include "vcwidgetplugininterface.h"
#include "qlcconfig.h"
#include "qlcfile.h"

VCWidgetPluginManager* VCWidgetPluginManager::s_instance = nullptr;

VCWidgetPluginManager::VCWidgetPluginManager(QObject* parent)
    : QObject(parent)
{
}

VCWidgetPluginManager::~VCWidgetPluginManager()
{
    // Plugins are owned by QPluginLoader instances (as Qt plugin instances),
    // so we don't delete them manually.
    s_instance = nullptr;
}

VCWidgetPluginManager* VCWidgetPluginManager::instance()
{
    if (s_instance == nullptr)
        s_instance = new VCWidgetPluginManager();
    return s_instance;
}

void VCWidgetPluginManager::load(const QDir& dir)
{
    qDebug() << Q_FUNC_INFO << dir.path();

    if (!dir.exists() || !dir.isReadable())
        return;

    bool anyAdded = false;

    const QStringList entries = dir.entryList();
    for (const QString& fileName : entries)
    {
        const QString path = dir.absoluteFilePath(fileName);

        QPluginLoader loader(path, this);
        QObject* obj = loader.instance();

        VCWidgetPluginEntry entry;
        entry.filePath = path;

        if (obj == nullptr)
        {
            entry.errorString = loader.errorString();
            qWarning() << Q_FUNC_INFO << fileName
                       << "failed to load:" << entry.errorString;
            m_entries.append(entry);
            loader.unload();
            continue;
        }

        VCWidgetPluginInterface* plugin =
            qobject_cast<VCWidgetPluginInterface*>(obj);

        if (plugin == nullptr)
        {
            // Valid Qt plugin but not a VCWidgetPlugin — silently skip,
            // it may be an I/O plugin or audio plugin in the same dir.
            loader.unload();
            continue;
        }

        const QString id = plugin->pluginId();

        if (m_pluginById.contains(id))
        {
            qWarning() << Q_FUNC_INFO << "Duplicate VC widget plugin id"
                       << id << "in" << path << "— discarded";
            entry.errorString = QString("Duplicate plugin id: %1").arg(id);
            m_entries.append(entry);
            loader.unload();
            continue;
        }

        qDebug() << "Loaded VC widget plugin" << plugin->name()
                 << "v" << plugin->version()
                 << "by" << plugin->author()
                 << "from" << fileName;

        entry.plugin = plugin;
        m_entries.append(entry);
        m_plugins.append(plugin);
        m_pluginById.insert(id, plugin);
        anyAdded = true;
    }

    if (anyAdded)
        emit pluginsChanged();
}

QList<VCWidgetPluginInterface*> VCWidgetPluginManager::plugins() const
{
    return m_plugins;
}

VCWidgetPluginInterface* VCWidgetPluginManager::pluginById(const QString& id) const
{
    return m_pluginById.value(id, nullptr);
}

QList<VCWidgetPluginEntry> VCWidgetPluginManager::entries() const
{
    return m_entries;
}

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

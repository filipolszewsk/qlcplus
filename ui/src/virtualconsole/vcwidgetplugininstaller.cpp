/*
  Q Light Controller Plus
  vcwidgetplugininstaller.cpp

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
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDebug>

#include "vcwidgetplugininstaller.h"
#include "vcwidgetpluginmanager.h"
#include "vcwidgetplugininterface.h"

VCWidgetPluginInstaller::VCWidgetPluginInstaller()
{
}

QString VCWidgetPluginInstaller::lastError() const
{
    return m_lastError;
}

VCWidgetPluginInstaller::Result VCWidgetPluginInstaller::install(
    const QString& filePath)
{
    m_lastError.clear();

    const QFileInfo fi(filePath);

    if (fi.suffix().toLower() == QStringLiteral("qlcvcw"))
    {
        // .qlcvcw package format — future implementation
        m_lastError = QStringLiteral(
            ".qlcvcw package installation is not yet implemented. "
            "Extract the binary for your platform and install it directly.");
        return InvalidPackage;
    }

    // Treat as bare binary
    return installBinary(filePath);
}

VCWidgetPluginInstaller::Result VCWidgetPluginInstaller::installBinary(
    const QString& filePath)
{
    const QFileInfo fi(filePath);

    if (!fi.exists())
    {
        m_lastError = QStringLiteral("File not found: ") + filePath;
        return InvalidPackage;
    }

    // Test-load the plugin to validate Qt ABI and interface
    QPluginLoader loader(filePath);
    QObject* obj = loader.instance();

    if (obj == nullptr)
    {
        m_lastError = loader.errorString();
        loader.unload();
        return IncompatibleQt;
    }

    VCWidgetPluginInterface* plugin = qobject_cast<VCWidgetPluginInterface*>(obj);
    if (plugin == nullptr)
    {
        m_lastError = QStringLiteral(
            "The file is a valid Qt plugin but does not implement "
            "VCWidgetPluginInterface. It may be an I/O or audio plugin.");
        loader.unload();
        return InvalidPackage;
    }

    const QString pluginId = plugin->pluginId();

    // Check for duplicate
    if (VCWidgetPluginManager::instance()->pluginById(pluginId) != nullptr)
    {
        loader.unload();
        return AlreadyInstalled;
    }

    loader.unload();

    // Ensure user plugin directory exists
    QDir destDir = VCWidgetPluginManager::userPluginDirectory();
    if (!destDir.exists())
    {
        if (!destDir.mkpath(destDir.absolutePath()))
        {
            m_lastError = QStringLiteral("Cannot create plugins directory: ")
                          + destDir.absolutePath();
            return CopyFailed;
        }
    }

    const QString destPath = destDir.absoluteFilePath(fi.fileName());

    if (QFile::exists(destPath))
    {
        // Overwrite existing file with same name
        if (!QFile::remove(destPath))
        {
            m_lastError = QStringLiteral("Cannot overwrite existing file: ")
                          + destPath;
            return CopyFailed;
        }
    }

    if (!QFile::copy(filePath, destPath))
    {
        m_lastError = QStringLiteral("Failed to copy file to: ") + destPath;
        return CopyFailed;
    }

    qDebug() << Q_FUNC_INFO << "Installed VC widget plugin" << pluginId
             << "to" << destPath;

    return Ok;
}

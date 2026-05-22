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
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

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

static QString currentPlatformKey()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_DARWIN)
#  if defined(Q_PROCESSOR_ARM)
    return QStringLiteral("macos_arm64");
#  else
    return QStringLiteral("macos_x64");
#  endif
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows_x64");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux_x64");
#else
    return QString();
#endif
}

VCWidgetPluginInstaller::Result VCWidgetPluginInstaller::install(
    const QString& filePath)
{
    m_lastError.clear();

    const QFileInfo fi(filePath);

    if (fi.suffix().toLower() == QStringLiteral("qlcvcw"))
        return installPackage(filePath);

    // Treat as bare binary
    return installBinary(filePath);
}

VCWidgetPluginInstaller::Result VCWidgetPluginInstaller::installPackage(
    const QString& filePath)
{
    // Create temp directory and extract the ZIP
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid())
    {
        m_lastError = QStringLiteral("Cannot create temporary directory.");
        return CopyFailed;
    }

    QProcess proc;
    proc.setProgram(QStringLiteral("unzip"));
    proc.setArguments({ QStringLiteral("-o"), filePath,
                        QStringLiteral("-d"), tmpDir.path() });
    proc.start();
    if (!proc.waitForFinished(15000) || proc.exitCode() != 0)
    {
        m_lastError = QStringLiteral("Failed to extract package: ")
                      + proc.readAllStandardError();
        return InvalidPackage;
    }

    // Read manifest.json
    QFile mf(tmpDir.filePath(QStringLiteral("manifest.json")));
    if (!mf.open(QIODevice::ReadOnly))
    {
        m_lastError = QStringLiteral("Package is missing manifest.json.");
        return InvalidPackage;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(mf.readAll());
    mf.close();

    if (doc.isNull() || !doc.isObject())
    {
        m_lastError = QStringLiteral("manifest.json is not valid JSON.");
        return InvalidPackage;
    }

    const QJsonObject root = doc.object();

    // Validate required fields
    if (!root.contains(QStringLiteral("plugin_id"))
        || !root.contains(QStringLiteral("platforms")))
    {
        m_lastError = QStringLiteral(
            "manifest.json is missing required fields (plugin_id, platforms).");
        return InvalidPackage;
    }

    const QString platformKey = currentPlatformKey();
    const QJsonObject platforms = root.value(QStringLiteral("platforms")).toObject();

    if (platformKey.isEmpty() || !platforms.contains(platformKey))
    {
        const QStringList available = platforms.keys();
        m_lastError = QStringLiteral("No binary for the current platform (%1). "
                                     "Available: %2")
                      .arg(platformKey.isEmpty() ? QStringLiteral("unknown")
                                                 : platformKey,
                           available.join(QStringLiteral(", ")));
        return InvalidPackage;
    }

    const QString binaryName = platforms.value(platformKey).toString();
    const QString binaryPath = tmpDir.filePath(binaryName);

    if (!QFile::exists(binaryPath))
    {
        m_lastError = QStringLiteral("Binary listed in manifest not found in package: ")
                      + binaryName;
        return InvalidPackage;
    }

    return installBinary(binaryPath);
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

    // Basic sanity: must be a loadable library extension
    const QString suf = fi.suffix().toLower();
    if (suf != QLatin1String("so")
        && suf != QLatin1String("dylib")
        && suf != QLatin1String("dll"))
    {
        m_lastError = QStringLiteral(
            "Not a shared library (expected .so / .dylib / .dll): ") + fi.fileName();
        return InvalidPackage;
    }

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

    qDebug() << Q_FUNC_INFO << "Installed VC widget plugin to" << destPath;
    return Ok;
}

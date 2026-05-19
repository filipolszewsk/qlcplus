/*
  Q Light Controller Plus
  pluginregistry.h

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

#ifndef PLUGINREGISTRY_H
#define PLUGINREGISTRY_H

#include <QObject>
#include <QString>
#include <QMap>

/**
 * Metadata about a single plugin available in an online registry.
 */
struct RegistryEntry
{
    QString pluginId;
    QString name;
    QString version;
    QString author;
    QString description;
    QString category;
    QString homepage;
    QString minQlcVersion;

    /** Download URLs per platform key (same keys as manifest.json: macos_arm64, etc.) */
    QMap<QString, QString> platformUrls;

    /** Returns the download URL for the current platform, or an empty string. */
    QString downloadUrlForCurrentPlatform() const;
};

/**
 * Abstract base for plugin registry backends.
 *
 * Implementations are responsible for fetching a list of available plugins
 * and downloading individual packages.
 *
 * Current implementations:
 *   - GitHubPluginRegistry: reads index.json from a GitHub raw URL
 *
 * The interface is designed so that an HTTP/REST backend can be added later
 * without changing any call sites in the dialog.
 */
class PluginRegistry : public QObject
{
    Q_OBJECT

public:
    explicit PluginRegistry(QObject* parent = nullptr)
        : QObject(parent) {}

    virtual ~PluginRegistry() {}

    /**
     * Asynchronously fetch the list of available plugins.
     * Emits indexReady() on success or indexFailed() on error.
     */
    virtual void fetchIndex() = 0;

    /**
     * Asynchronously download the package for @p entry to @p destPath.
     * @p destPath should be a full file path (including filename).
     * Emits downloadReady() on success or downloadFailed() on error.
     */
    virtual void download(const RegistryEntry& entry,
                          const QString& destPath) = 0;

    /** Cancel any in-progress fetch or download. */
    virtual void cancel() {}

signals:
    /** Emitted when fetchIndex() completes successfully. */
    void indexReady(QList<RegistryEntry> entries);

    /** Emitted when fetchIndex() fails. */
    void indexFailed(QString errorMessage);

    /**
     * Emitted periodically during download.
     * @p pluginId  identifies which download this belongs to.
     * @p percent   0–100.
     */
    void downloadProgress(QString pluginId, int percent);

    /** Emitted when download() completes. @p localPath is the saved file. */
    void downloadReady(QString pluginId, QString localPath);

    /** Emitted when download() fails. */
    void downloadFailed(QString pluginId, QString errorMessage);
};

#endif // PLUGINREGISTRY_H

/*
  Q Light Controller Plus
  githubpluginregistry.h

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

#ifndef GITHUBPLUGINREGISTRY_H
#define GITHUBPLUGINREGISTRY_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryFile>

#include "pluginregistry.h"

/**
 * GitHub-backed plugin registry.
 *
 * Fetches `index.json` from a GitHub raw URL (default:
 * https://raw.githubusercontent.com/qlcplus/vcwidget-registry/main/index.json)
 * and parses it into a list of RegistryEntry objects.
 *
 * Expected index.json schema:
 * {
 *   "schema_version": 1,
 *   "plugins": [
 *     {
 *       "plugin_id": "org.qlcplus.vcwidgets.mywidget",
 *       "name": "My Widget",
 *       "version": "1.2.0",
 *       "author": "Author Name",
 *       "category": "DMX Control",
 *       "description": "Short description.",
 *       "homepage": "https://github.com/...",
 *       "min_qlc_version": "4.14.0",
 *       "platforms": {
 *         "macos_arm64": "https://github.com/.../releases/.../mywidget.qlcvcw",
 *         "linux_x64":   "https://...",
 *         "windows_x64": "https://..."
 *       }
 *     }
 *   ]
 * }
 */
class GitHubPluginRegistry : public PluginRegistry
{
    Q_OBJECT

public:
    static const QString defaultIndexUrl();

    explicit GitHubPluginRegistry(
        const QString& indexUrl = defaultIndexUrl(),
        QObject* parent = nullptr);

    ~GitHubPluginRegistry() override;

    void fetchIndex() override;
    void download(const RegistryEntry& entry, const QString& destPath) override;
    void cancel() override;

private slots:
    void slotIndexFinished();
    void slotDownloadProgress(qint64 received, qint64 total);
    void slotDownloadFinished();

private:
    QNetworkAccessManager m_nam;
    QNetworkReply*        m_indexReply    = nullptr;
    QNetworkReply*        m_downloadReply = nullptr;
    QString               m_indexUrl;

    QString               m_downloadPluginId;
    QString               m_downloadDestPath;
    QTemporaryFile*       m_downloadFile  = nullptr;
};

#endif // GITHUBPLUGINREGISTRY_H

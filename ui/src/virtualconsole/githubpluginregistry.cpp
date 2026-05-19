/*
  Q Light Controller Plus
  githubpluginregistry.cpp

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

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

#include "githubpluginregistry.h"
#include "vcwidgetplugininstaller.h"

// ---------------------------------------------------------------------------
// RegistryEntry helpers
// ---------------------------------------------------------------------------

static QString platformKey()
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

QString RegistryEntry::downloadUrlForCurrentPlatform() const
{
    const QString key = platformKey();
    if (key.isEmpty())
        return QString();
    return platformUrls.value(key);
}

// ---------------------------------------------------------------------------
// GitHubPluginRegistry
// ---------------------------------------------------------------------------

const QString GitHubPluginRegistry::defaultIndexUrl()
{
    return QStringLiteral(
        "https://raw.githubusercontent.com/"
        "qlcplus/vcwidget-registry/main/index.json");
}

GitHubPluginRegistry::GitHubPluginRegistry(const QString& indexUrl,
                                            QObject* parent)
    : PluginRegistry(parent)
    , m_indexUrl(indexUrl)
{
}

GitHubPluginRegistry::~GitHubPluginRegistry()
{
    cancel();
}

void GitHubPluginRegistry::fetchIndex()
{
    if (m_indexReply)
    {
        m_indexReply->abort();
        m_indexReply->deleteLater();
        m_indexReply = nullptr;
    }

    qDebug() << Q_FUNC_INFO << "Fetching" << m_indexUrl;

    const QUrl indexEndpoint(m_indexUrl);
    QNetworkRequest req(indexEndpoint);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("QLC+/4.14 vcwidgets-registry"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_indexReply = m_nam.get(req);
    connect(m_indexReply, &QNetworkReply::finished,
            this, &GitHubPluginRegistry::slotIndexFinished);
}

void GitHubPluginRegistry::slotIndexFinished()
{
    QNetworkReply* reply = m_indexReply;
    m_indexReply = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        const QString err = reply->errorString();
        reply->deleteLater();
        emit indexFailed(tr("Network error: %1").arg(err));
        return;
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);

    if (doc.isNull())
    {
        emit indexFailed(tr("JSON parse error: %1").arg(parseErr.errorString()));
        return;
    }

    if (!doc.isObject())
    {
        emit indexFailed(tr("Unexpected JSON format (expected object at root)."));
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonArray  pluginsArr = root.value(QStringLiteral("plugins")).toArray();

    QList<RegistryEntry> entries;

    for (const QJsonValue& val : pluginsArr)
    {
        if (!val.isObject())
            continue;

        const QJsonObject obj = val.toObject();

        RegistryEntry entry;
        entry.pluginId       = obj.value(QStringLiteral("plugin_id")).toString();
        entry.name           = obj.value(QStringLiteral("name")).toString();
        entry.version        = obj.value(QStringLiteral("version")).toString();
        entry.author         = obj.value(QStringLiteral("author")).toString();
        entry.description    = obj.value(QStringLiteral("description")).toString();
        entry.category       = obj.value(QStringLiteral("category")).toString();
        entry.homepage       = obj.value(QStringLiteral("homepage")).toString();
        entry.minQlcVersion  = obj.value(QStringLiteral("min_qlc_version")).toString();

        const QJsonObject platforms = obj.value(QStringLiteral("platforms")).toObject();
        for (auto it = platforms.begin(); it != platforms.end(); ++it)
            entry.platformUrls.insert(it.key(), it.value().toString());

        if (entry.pluginId.isEmpty() || entry.name.isEmpty())
        {
            qWarning() << Q_FUNC_INFO
                       << "Skipping registry entry with missing plugin_id or name";
            continue;
        }

        entries.append(entry);
    }

    qDebug() << Q_FUNC_INFO << "Registry loaded:" << entries.size() << "plugins";
    emit indexReady(entries);
}

void GitHubPluginRegistry::download(const RegistryEntry& entry,
                                     const QString& destPath)
{
    const QString url = entry.downloadUrlForCurrentPlatform();
    if (url.isEmpty())
    {
        emit downloadFailed(entry.pluginId,
            tr("No download URL for the current platform."));
        return;
    }

    if (m_downloadReply)
    {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }

    m_downloadPluginId = entry.pluginId;
    m_downloadDestPath = destPath;

    // Use a temp file alongside the destination to avoid partial downloads
    const QString tmpPath = destPath + QStringLiteral(".part");
    delete m_downloadFile;
    m_downloadFile = new QTemporaryFile(this);
    m_downloadFile->setFileTemplate(tmpPath);

    if (!m_downloadFile->open())
    {
        delete m_downloadFile;
        m_downloadFile = nullptr;
        emit downloadFailed(entry.pluginId,
            tr("Cannot create temporary download file."));
        return;
    }

    qDebug() << Q_FUNC_INFO << "Downloading" << url << "→" << destPath;

    const QUrl downloadEndpoint(url);
    QNetworkRequest req(downloadEndpoint);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("QLC+/4.14 vcwidgets-registry"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = m_nam.get(req);
    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &GitHubPluginRegistry::slotDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &GitHubPluginRegistry::slotDownloadFinished);
    connect(m_downloadReply, &QNetworkReply::readyRead,
            this, [this]() {
                if (m_downloadFile)
                    m_downloadFile->write(m_downloadReply->readAll());
            });
}

void GitHubPluginRegistry::slotDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
    {
        const int percent = static_cast<int>(received * 100 / total);
        emit downloadProgress(m_downloadPluginId, percent);
    }
}

void GitHubPluginRegistry::slotDownloadFinished()
{
    QNetworkReply* reply = m_downloadReply;
    m_downloadReply = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        const QString err = reply->errorString();
        reply->deleteLater();
        delete m_downloadFile;
        m_downloadFile = nullptr;
        emit downloadFailed(m_downloadPluginId,
            tr("Download error: %1").arg(err));
        return;
    }

    reply->deleteLater();

    if (!m_downloadFile)
    {
        emit downloadFailed(m_downloadPluginId,
            tr("Internal error: download file is null."));
        return;
    }

    m_downloadFile->flush();
    m_downloadFile->close();

    // Move temp file to final destination
    if (QFile::exists(m_downloadDestPath))
        QFile::remove(m_downloadDestPath);

    const bool moved = m_downloadFile->rename(m_downloadDestPath);
    delete m_downloadFile;
    m_downloadFile = nullptr;

    if (!moved)
    {
        emit downloadFailed(m_downloadPluginId,
            tr("Could not save downloaded file to: %1").arg(m_downloadDestPath));
        return;
    }

    qDebug() << Q_FUNC_INFO << "Downloaded" << m_downloadPluginId
             << "to" << m_downloadDestPath;

    emit downloadProgress(m_downloadPluginId, 100);
    emit downloadReady(m_downloadPluginId, m_downloadDestPath);
}

void GitHubPluginRegistry::cancel()
{
    if (m_indexReply)
    {
        m_indexReply->abort();
        m_indexReply->deleteLater();
        m_indexReply = nullptr;
    }
    if (m_downloadReply)
    {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }
    delete m_downloadFile;
    m_downloadFile = nullptr;
}

/*
  Q Light Controller Plus
  licensemanager.cpp

  Copyright (c) Filip Olszewski

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

#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QDir>

#include "licensemanager.h"
#include "qlccrypto.h"

LicenseManager::LicenseManager(QObject *parent)
    : QObject(parent)
    , m_licensed(false)
{
}

QString LicenseManager::licenseFilePath()
{
#if defined(__APPLE__) || defined(Q_OS_MAC)
    QString path = QDir::homePath() + "/Library/Application Support/QLC+";
#elif defined(WIN32) || defined(Q_OS_WIN)
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString path = QDir::homePath() + "/.qlcplus";
#endif
    return path + QDir::separator() + QLCKEY_FILENAME;
}

bool LicenseManager::loadLicense()
{
    m_licensed = false;
    m_customerName.clear();
    m_customerEmail.clear();
    m_licenseKey.clear();
    m_instanceId.clear();
    m_contentKey.clear();

    QString keyPath = licenseFilePath();
    QFile keyFile(keyPath);
    if (!keyFile.exists())
    {
        qDebug() << "LicenseManager: no license file found at" << keyPath;
        return false;
    }

    if (!keyFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "LicenseManager: cannot open license file" << keyPath;
        return false;
    }

    QByteArray encrypted = keyFile.readAll();
    keyFile.close();

    if (encrypted.isEmpty())
    {
        qWarning() << "LicenseManager: license file is empty";
        return false;
    }

    QString hwFingerprint = QLCCrypto::generateHardwareFingerprint();
    QByteArray decryptionKey = QLCCrypto::deriveKey(hwFingerprint);
    QByteArray decrypted = QLCCrypto::aesDecrypt(encrypted, decryptionKey);

    if (decrypted.isEmpty())
    {
        // Fallback: try the legacy fingerprint (which included localHostName).
        // If it works, re-encrypt with the new stable fingerprint so the user
        // never hits this again, even after future hostname changes.
        QString legacyFingerprint = QLCCrypto::generateLegacyHardwareFingerprint();
        if (legacyFingerprint != hwFingerprint)
        {
            QByteArray legacyKey = QLCCrypto::deriveKey(legacyFingerprint);
            decrypted = QLCCrypto::aesDecrypt(encrypted, legacyKey);
            if (!decrypted.isEmpty())
            {
                qDebug() << "LicenseManager: migrating license file to stable fingerprint";
                QByteArray reEncrypted = QLCCrypto::aesEncrypt(decrypted, decryptionKey);
                QFile rewriteFile(keyPath);
                if (rewriteFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    rewriteFile.write(reEncrypted);
                    rewriteFile.close();
                }
            }
        }
    }

    if (decrypted.isEmpty())
    {
        qWarning() << "LicenseManager: failed to decrypt license file (hardware mismatch?)";
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(decrypted, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        qWarning() << "LicenseManager: license file contains invalid data";
        return false;
    }

    QJsonObject obj = jsonDoc.object();
    if (obj.value("magic").toString() != QLCKEY_MAGIC)
    {
        qWarning() << "LicenseManager: invalid license magic (wrong hardware?)";
        return false;
    }

    m_customerName = obj.value("customer_name").toString();
    m_customerEmail = obj.value("customer_email").toString();
    m_licenseKey = obj.value("license_key").toString();
    m_instanceId = obj.value("instance_id").toString();

    QString contentKeyHex = obj.value("content_key").toString();
    m_contentKey = QByteArray::fromHex(contentKeyHex.toUtf8());

    if (m_contentKey.size() != QLCCrypto::KEY_LEN)
    {
        qWarning() << "LicenseManager: invalid content key in license file";
        m_contentKey.clear();
        return false;
    }

    m_licensed = true;
    qDebug() << "LicenseManager: license loaded for" << m_customerName;
    emit licenseChanged();
    return true;
}

bool LicenseManager::isLicensed() const
{
    return m_licensed;
}

QString LicenseManager::customerName() const
{
    return m_customerName;
}

QString LicenseManager::customerEmail() const
{
    return m_customerEmail;
}

QString LicenseManager::licenseKey() const
{
    return m_licenseKey;
}

QString LicenseManager::instanceId() const
{
    return m_instanceId;
}

QByteArray LicenseManager::contentKey() const
{
    return m_contentKey;
}

QByteArray LicenseManager::decryptPremiumFile(const QByteArray &fileData) const
{
    if (!m_licensed || m_contentKey.isEmpty())
        return QByteArray();

    if (fileData.size() < 5 + QLCCrypto::BLOCK_LEN)
        return QByteArray();

    if (fileData.left(4) != PREMIUM_FILE_MAGIC)
    {
        qWarning() << "LicenseManager: invalid premium file magic";
        return QByteArray();
    }

    quint8 version = static_cast<quint8>(fileData.at(4));
    if (version != PREMIUM_FILE_VERSION)
    {
        qWarning() << "LicenseManager: unsupported premium file version" << version;
        return QByteArray();
    }

    QByteArray payload = fileData.mid(5);
    return QLCCrypto::aesDecrypt(payload, m_contentKey);
}

bool LicenseManager::isPremiumFile(const QString &fileName)
{
    return fileName.endsWith(PREMIUM_SCRIPT_EXT, Qt::CaseInsensitive);
}

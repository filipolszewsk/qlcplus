/*
  Q Light Controller Plus
  licensemanager.h

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

#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <QByteArray>
#include <QObject>
#include <QString>

#define QLCKEY_MAGIC "QLCPLUS_LICENSE"
#define QLCKEY_FILENAME "license.qlckey"

#define PREMIUM_SCRIPT_EXT ".qlcscript"

#define PREMIUM_FILE_MAGIC "QLCP"
#define PREMIUM_FILE_VERSION 1

class LicenseManager : public QObject
{
    Q_OBJECT

public:
    explicit LicenseManager(QObject *parent = nullptr);

    bool loadLicense();
    bool isLicensed() const;

    QString customerName() const;
    QString customerEmail() const;
    QString licenseKey() const;
    QString instanceId() const;
    QByteArray contentKey() const;

    QByteArray decryptPremiumFile(const QByteArray &fileData) const;

    static QString licenseFilePath();
    static bool isPremiumFile(const QString &fileName);

signals:
    void licenseChanged();

private:
    bool m_licensed;
    QString m_customerName;
    QString m_customerEmail;
    QString m_licenseKey;
    QString m_instanceId;
    QByteArray m_contentKey;
};

#endif // LICENSEMANAGER_H

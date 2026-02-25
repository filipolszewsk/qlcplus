/*
  Q Light Controller Plus
  licensedialog.cpp

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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFont>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include "licensedialog.h"
#include "licensemanager.h"
#include "qlccrypto.h"
#include "doc.h"

LicenseDialog::LicenseDialog(Doc *doc, QWidget *parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_nam(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("GRIDqlc - Premium License"));
    setMinimumWidth(500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- Status section ---
    QGroupBox *statusGroup = new QGroupBox(tr("License Status"), this);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);

    m_statusLabel = new QLabel(this);
    QFont statusFont = m_statusLabel->font();
    statusFont.setPointSize(statusFont.pointSize() + 2);
    statusFont.setBold(true);
    m_statusLabel->setFont(statusFont);
    statusLayout->addWidget(m_statusLabel);

    m_nameLabel = new QLabel(this);
    statusLayout->addWidget(m_nameLabel);

    m_emailLabel = new QLabel(this);
    statusLayout->addWidget(m_emailLabel);

    m_buyLabel = new QLabel(
        tr("<a href=\"https://gridqlc.com/#pricing\">Buy a premium license</a>"), this);
    m_buyLabel->setOpenExternalLinks(true);
    m_buyLabel->setTextFormat(Qt::RichText);
    statusLayout->addWidget(m_buyLabel);

    mainLayout->addWidget(statusGroup);

    // --- Hardware ID section ---
    QGroupBox *hwGroup = new QGroupBox(tr("Hardware ID"), this);
    QVBoxLayout *hwLayout = new QVBoxLayout(hwGroup);

    m_hwIdLabel = new QLabel(this);
    m_hwIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont monoFont = m_hwIdLabel->font();
    monoFont.setFamily("Courier New");
    m_hwIdLabel->setFont(monoFont);
    hwLayout->addWidget(m_hwIdLabel);

    QLabel *hwNote = new QLabel(
        tr("Need to move your license to a new computer? Contact support and provide this ID."), this);
    hwNote->setWordWrap(true);
    hwNote->setStyleSheet("color: gray; font-size: 11px;");
    hwLayout->addWidget(hwNote);

    QPushButton *copyHwBtn = new QPushButton(tr("Copy Hardware ID"), this);
    connect(copyHwBtn, &QPushButton::clicked, this, &LicenseDialog::slotCopyHwId);
    hwLayout->addWidget(copyHwBtn);

    mainLayout->addWidget(hwGroup);

    // --- Activation section ---
    QGroupBox *activateGroup = new QGroupBox(tr("Activate License"), this);
    QVBoxLayout *activateLayout = new QVBoxLayout(activateGroup);

    QLabel *keyLabel = new QLabel(tr("Paste your license key from LemonSqueezy:"), this);
    activateLayout->addWidget(keyLabel);

    QHBoxLayout *keyRow = new QHBoxLayout();
    m_keyInput = new QLineEdit(this);
    m_keyInput->setPlaceholderText(tr("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"));
    keyRow->addWidget(m_keyInput);

    m_activateBtn = new QPushButton(tr("Activate"), this);
    m_activateBtn->setMinimumWidth(90);
    connect(m_activateBtn, &QPushButton::clicked, this, &LicenseDialog::slotActivate);
    keyRow->addWidget(m_activateBtn);
    activateLayout->addLayout(keyRow);

    m_activationStatus = new QLabel(this);
    m_activationStatus->setWordWrap(true);
    m_activationStatus->setVisible(false);
    activateLayout->addWidget(m_activationStatus);

    mainLayout->addWidget(activateGroup);

    // --- Buttons ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *refreshBtn = new QPushButton(tr("Refresh License"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &LicenseDialog::slotRefresh);
    btnLayout->addWidget(refreshBtn);

    m_deactivateBtn = new QPushButton(tr("Deactivate on this machine"), this);
    m_deactivateBtn->setStyleSheet("color: #b71c1c;");
    connect(m_deactivateBtn, &QPushButton::clicked, this, &LicenseDialog::slotDeactivate);
    btnLayout->addWidget(m_deactivateBtn);

    btnLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);

    updateDisplay();
}

void LicenseDialog::slotRefresh()
{
    if (m_doc && m_doc->licenseManager())
    {
        m_doc->licenseManager()->loadLicense();
        updateDisplay();
    }
}

void LicenseDialog::slotCopyHwId()
{
    QApplication::clipboard()->setText(m_fullHwId);
}

void LicenseDialog::slotActivate()
{
    QString key = m_keyInput->text().trimmed();
    if (key.isEmpty())
    {
        setActivationStatus(tr("Please enter your license key."), true);
        return;
    }

    m_activateBtn->setEnabled(false);
    setActivationStatus(tr("Activating..."), false);

    QNetworkRequest req(QUrl("https://gridqlc-activator.vercel.app/api/activate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("X-QLC-Client", "1");

    QByteArray body = "licenseKey=" + QUrl::toPercentEncoding(key)
                    + "&hardwareId=" + QUrl::toPercentEncoding(m_fullHwId);

    QNetworkReply *reply = m_nam->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        slotActivateReply(reply);
    });
}

void LicenseDialog::slotActivateReply(QNetworkReply *reply)
{
    reply->deleteLater();
    m_activateBtn->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError)
    {
        setActivationStatus(tr("Network error: %1").arg(reply->errorString()), true);
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    if (!obj.value("success").toBool())
    {
        QString errMsg = obj.value("error").toString(tr("Unknown error."));
        setActivationStatus(errMsg, true);
        return;
    }

    QString b64 = obj.value("licenseData").toString();
    if (b64.isEmpty())
    {
        setActivationStatus(tr("Server returned empty license data."), true);
        return;
    }

    QByteArray licenseBytes = QByteArray::fromBase64(b64.toUtf8());
    QString filePath = LicenseManager::licenseFilePath();

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        setActivationStatus(tr("Could not write license file: %1").arg(filePath), true);
        return;
    }
    f.write(licenseBytes);
    f.close();

    if (m_doc && m_doc->licenseManager())
    {
        m_doc->licenseManager()->loadLicense();
        if (m_doc->licenseManager()->isLicensed())
            emit m_doc->licenseManager()->licenseChanged();
    }

    m_keyInput->clear();
    setActivationStatus(tr("✔ Activated successfully!"), false);
    updateDisplay();
}

void LicenseDialog::slotDeactivate()
{
    LicenseManager *lm = m_doc ? m_doc->licenseManager() : nullptr;
    if (!lm || !lm->isLicensed())
        return;

    QString instanceId = lm->instanceId();
    QString licenseKey = lm->licenseKey();

    if (instanceId.isEmpty() || licenseKey.isEmpty())
    {
        setActivationStatus(tr("Cannot deactivate: missing license data."), true);
        return;
    }

    m_deactivateBtn->setEnabled(false);
    setActivationStatus(tr("Deactivating..."), false);

    QNetworkRequest req(QUrl("https://gridqlc-activator.vercel.app/api/deactivate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["licenseKey"] = licenseKey;
    payload["instanceId"] = instanceId;

    QNetworkReply *reply = m_nam->post(req, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_deactivateBtn->setEnabled(true);

        if (reply->error() != QNetworkReply::NoError)
        {
            setActivationStatus(tr("Network error: %1").arg(reply->errorString()), true);
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (!obj.value("success").toBool())
        {
            setActivationStatus(obj.value("error").toString(tr("Deactivation failed.")), true);
            return;
        }

        // Remove local license file and reset state
        QFile::remove(LicenseManager::licenseFilePath());
        if (m_doc && m_doc->licenseManager())
        {
            m_doc->licenseManager()->loadLicense();
            emit m_doc->licenseManager()->licenseChanged();
        }

        setActivationStatus(tr("✔ Deactivated. License slot is now free."), false);
        updateDisplay();
    });
}

void LicenseDialog::setActivationStatus(const QString &msg, bool isError)
{
    m_activationStatus->setText(msg);
    m_activationStatus->setStyleSheet(isError ? "color: #b71c1c;" : "color: #2e7d32;");
    m_activationStatus->setVisible(true);
}

void LicenseDialog::updateDisplay()
{
    m_fullHwId = QLCCrypto::generateHardwareFingerprint();
    QString shortId = m_fullHwId.left(16) + "...";
    m_hwIdLabel->setText(shortId);

    LicenseManager *lm = m_doc ? m_doc->licenseManager() : nullptr;
    if (lm && lm->isLicensed())
    {
        m_statusLabel->setText(tr("✔  ACTIVE"));
        m_statusLabel->setStyleSheet("color: #2e7d32;");
        m_nameLabel->setText(tr("Licensed to: %1").arg(lm->customerName()));
        m_emailLabel->setText(tr("Email: %1").arg(lm->customerEmail()));
        m_buyLabel->setVisible(false);
        m_deactivateBtn->setVisible(true);
    }
    else
    {
        m_statusLabel->setText(tr("✘  NOT ACTIVATED"));
        m_statusLabel->setStyleSheet("color: #b71c1c;");
        m_nameLabel->setText(tr("No premium license detected."));
        m_emailLabel->setText(
            tr("Place a valid .qlckey file in: %1").arg(LicenseManager::licenseFilePath()));
        m_buyLabel->setVisible(true);
        m_deactivateBtn->setVisible(false);
    }
}

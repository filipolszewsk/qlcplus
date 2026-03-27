/*
  QLC+ License Activator
  activatorwindow.cpp

  Copyright (c) Filip Olszewski
  PROPRIETARY - NOT OPEN SOURCE
*/

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QHostInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include "activatorwindow.h"
#include "qlccrypto.h"

/*
 * IMPORTANT: Replace this with your actual master content key.
 * This is the AES-256 key used to encrypt premium files.
 * Generate with: openssl rand -hex 32
 * Keep this SECRET - this binary should NOT be open source.
 */
const QString ActivatorWindow::MASTER_CONTENT_KEY =
    "3bc2ddf5148fdf2bff59e14935675cf4e525752cbd95c4a8a882274ee14afa6d";

ActivatorWindow::ActivatorWindow(QWidget *parent)
    : QWidget(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("GRIDqlc License Activator"));
    setMinimumWidth(520);
    setMaximumWidth(600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- Branding header ---
    QLabel *titleLabel = new QLabel(tr("GRIDqlc License Activator"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(tr("Activate your premium content license"), this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("color: gray; margin-bottom: 8px;");
    mainLayout->addWidget(subtitleLabel);

    // --- Machine Information ---
    QGroupBox *hwGroup = new QGroupBox(tr("This Machine"), this);
    QVBoxLayout *hwLayout = new QVBoxLayout(hwGroup);

    QHBoxLayout *hwRow = new QHBoxLayout();
    QLabel *hwTitleLabel = new QLabel(tr("Hardware ID:"), this);
    hwRow->addWidget(hwTitleLabel);

    m_hwShortLabel = new QLabel(this);
    QFont monoFont = m_hwShortLabel->font();
    monoFont.setFamily("Courier New");
    m_hwShortLabel->setFont(monoFont);
    hwRow->addWidget(m_hwShortLabel, 1);

    QPushButton *copyBtn = new QPushButton(tr("Copy"), this);
    copyBtn->setFixedWidth(60);
    connect(copyBtn, &QPushButton::clicked, this, &ActivatorWindow::slotCopyHwId);
    hwRow->addWidget(copyBtn);

    hwLayout->addLayout(hwRow);
    mainLayout->addWidget(hwGroup);

    // --- License Key ---
    QGroupBox *keyGroup = new QGroupBox(tr("License Key"), this);
    QVBoxLayout *keyLayout = new QVBoxLayout(keyGroup);

    m_keyInput = new QLineEdit(this);
    m_keyInput->setPlaceholderText(tr("Paste your license key here..."));
    m_keyInput->setMinimumHeight(32);
    keyLayout->addWidget(m_keyInput);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_activateBtn = new QPushButton(tr("Activate"), this);
    m_activateBtn->setMinimumHeight(36);
    m_activateBtn->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1976d2; }"
        "QPushButton:disabled { background-color: #aaa; }");
    connect(m_activateBtn, &QPushButton::clicked, this, &ActivatorWindow::slotActivate);
    btnLayout->addWidget(m_activateBtn);

    m_deactivateBtn = new QPushButton(tr("Deactivate"), this);
    m_deactivateBtn->setMinimumHeight(36);
    m_deactivateBtn->setStyleSheet(
        "QPushButton { background-color: #c62828; color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
        "QPushButton:disabled { background-color: #aaa; }");
    connect(m_deactivateBtn, &QPushButton::clicked, this, &ActivatorWindow::slotDeactivate);
    btnLayout->addWidget(m_deactivateBtn);

    keyLayout->addLayout(btnLayout);
    mainLayout->addWidget(keyGroup);

    // --- Status ---
    m_statusLabel = new QLabel(tr("Enter your license key and click Activate."), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: gray; padding: 4px;");
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    updateStatus();
}

QString ActivatorWindow::keyFilePath() const
{
#if defined(__APPLE__) || defined(Q_OS_MAC)
    QString path = QDir::homePath() + "/Library/Application Support/QLC+";
#elif defined(WIN32) || defined(Q_OS_WIN)
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString path = QDir::homePath() + "/.qlcplus";
#endif
    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(".");
    return path + QDir::separator() + "license.qlckey";
}

void ActivatorWindow::updateStatus()
{
    m_fullHwId = QLCCrypto::generateHardwareFingerprint();
    m_hwShortLabel->setText(m_fullHwId.left(20) + "...");

    QFile keyFile(keyFilePath());
    if (keyFile.exists())
        setStatus(tr("License file found. This machine is activated."), true);
}

void ActivatorWindow::setStatus(const QString &msg, bool success)
{
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(
        success ? "color: #2e7d32; font-weight: bold; padding: 4px;"
                : "color: #b71c1c; font-weight: bold; padding: 4px;");
}

void ActivatorWindow::slotCopyHwId()
{
    QApplication::clipboard()->setText(m_fullHwId);
}

void ActivatorWindow::slotActivate()
{
    QString licenseKey = m_keyInput->text().trimmed();
    if (licenseKey.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a license key."));
        return;
    }

    setStatus(tr("Contacting activation server..."), true);
    m_activateBtn->setEnabled(false);

    QNetworkRequest req(QUrl("https://api.lemonsqueezy.com/v1/licenses/activate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Accept", "application/json");

    QUrlQuery params;
    params.addQueryItem("license_key", licenseKey);
    params.addQueryItem("instance_name", QHostInfo::localHostName());

    QNetworkReply *reply = m_nam->post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        slotActivateReply(reply);
    });
}

void ActivatorWindow::slotActivateReply(QNetworkReply *reply)
{
    m_activateBtn->setEnabled(true);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        QByteArray body = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(body);
        QString errorMsg = doc.object().value("error").toString();
        if (errorMsg.isEmpty())
            errorMsg = reply->errorString();
        setStatus(tr("Activation failed: %1").arg(errorMsg), false);
        return;
    }

    QByteArray body = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(body);
    QJsonObject root = doc.object();

    bool activated = root.value("activated").toBool();
    if (!activated)
    {
        QString error = root.value("error").toString();
        setStatus(tr("Activation failed: %1").arg(error), false);
        return;
    }

    QJsonObject meta = root.value("meta").toObject();
    QString customerName = meta.value("customer_name").toString();
    QString customerEmail = meta.value("customer_email").toString();

    QJsonObject instance = root.value("instance").toObject();
    QString instanceId = instance.value("id").toString();

    if (customerName.isEmpty())
        customerName = "Licensed User";

    generateKeyFile(customerName, customerEmail, m_keyInput->text().trimmed(), instanceId);

    setStatus(tr("✔  Activated for: %1 (%2)\nYou can now use premium content in QLC+.")
              .arg(customerName, customerEmail), true);
}

void ActivatorWindow::slotDeactivate()
{
    QString licenseKey = m_keyInput->text().trimmed();
    if (licenseKey.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a license key."));
        return;
    }

    QString instanceId = readInstanceIdFromKeyFile();
    if (instanceId.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
            tr("No active license found on this machine. Cannot deactivate."));
        return;
    }

    setStatus(tr("Deactivating..."), true);
    m_deactivateBtn->setEnabled(false);

    QNetworkRequest req(QUrl("https://api.lemonsqueezy.com/v1/licenses/deactivate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Accept", "application/json");

    QUrlQuery params;
    params.addQueryItem("license_key", licenseKey);
    params.addQueryItem("instance_id", instanceId);

    QNetworkReply *reply = m_nam->post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        slotDeactivateReply(reply);
    });
}

void ActivatorWindow::slotDeactivateReply(QNetworkReply *reply)
{
    m_deactivateBtn->setEnabled(true);
    reply->deleteLater();

    QFile keyFile(keyFilePath());
    if (keyFile.exists())
        keyFile.remove();

    setStatus(tr("License deactivated. This machine is now free."), false);
}


QString ActivatorWindow::readInstanceIdFromKeyFile() const
{
    QFile keyFile(keyFilePath());
    if (!keyFile.exists() || !keyFile.open(QIODevice::ReadOnly))
        return QString();

    QByteArray encrypted = keyFile.readAll();
    keyFile.close();

    QString hwFingerprint = QLCCrypto::generateHardwareFingerprint();
    QByteArray decryptionKey = QLCCrypto::deriveKey(hwFingerprint);
    QByteArray decrypted = QLCCrypto::aesDecrypt(encrypted, decryptionKey);

    if (decrypted.isEmpty())
    {
        QString legacyFingerprint = QLCCrypto::generateLegacyHardwareFingerprint();
        if (legacyFingerprint != hwFingerprint)
        {
            QByteArray legacyKey = QLCCrypto::deriveKey(legacyFingerprint);
            decrypted = QLCCrypto::aesDecrypt(encrypted, legacyKey);
        }
    }

    if (decrypted.isEmpty())
        return QString();

    QJsonDocument doc = QJsonDocument::fromJson(decrypted);
    QJsonObject obj = doc.object();
    if (obj.value("magic").toString() != "QLCPLUS_LICENSE")
        return QString();

    return obj.value("instance_id").toString();
}

void ActivatorWindow::generateKeyFile(const QString &customerName,
                                       const QString &customerEmail,
                                       const QString &licenseKey,
                                       const QString &instanceId)
{
    QString hwFingerprint = QLCCrypto::generateHardwareFingerprint();

    QJsonObject payload;
    payload["magic"] = QString("QLCPLUS_LICENSE");
    payload["content_key"] = MASTER_CONTENT_KEY;
    payload["customer_name"] = customerName;
    payload["customer_email"] = customerEmail;
    payload["license_key"] = licenseKey;
    payload["instance_id"] = instanceId;
    payload["activated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray encryptionKey = QLCCrypto::deriveKey(hwFingerprint);
    QByteArray encrypted = QLCCrypto::aesEncrypt(jsonData, encryptionKey);

    QFile keyFile(keyFilePath());
    if (keyFile.open(QIODevice::WriteOnly))
    {
        keyFile.write(encrypted);
        keyFile.close();
    }
}

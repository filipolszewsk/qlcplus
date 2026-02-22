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
#include <QFont>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>

#include "licensedialog.h"
#include "licensemanager.h"
#include "qlccrypto.h"
#include "doc.h"

LicenseDialog::LicenseDialog(Doc *doc, QWidget *parent)
    : QDialog(parent)
    , m_doc(doc)
{
    setWindowTitle(tr("GRIDqlc - Premium License"));
    setMinimumWidth(480);

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

    // "Buy now" link - shown only when not licensed
    m_buyLabel = new QLabel(
        tr("<a href=\"https://gridqlc.com/buy\">Buy a premium license</a>"), this);
    m_buyLabel->setOpenExternalLinks(true);
    m_buyLabel->setTextFormat(Qt::RichText);
    statusLayout->addWidget(m_buyLabel);

    mainLayout->addWidget(statusGroup);

    // --- Hardware ID section (always visible so user can copy for support) ---
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

    // --- Buttons ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *refreshBtn = new QPushButton(tr("Refresh License"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &LicenseDialog::slotRefresh);
    btnLayout->addWidget(refreshBtn);

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
    }
    else
    {
        m_statusLabel->setText(tr("✘  NOT ACTIVATED"));
        m_statusLabel->setStyleSheet("color: #b71c1c;");
        m_nameLabel->setText(tr("No premium license detected."));
        m_emailLabel->setText(
            tr("Place a valid .qlckey file in: %1").arg(LicenseManager::licenseFilePath()));
        m_buyLabel->setVisible(true);
    }
}

/*
  Q Light Controller Plus
  copyfunctionsettingsdialog.cpp

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

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>

#include "copyfunctionsettingsdialog.h"
#include "function.h"
#include "doc.h"

#define SETTINGS_KEY "CopyFunctionSettingsDialog/flags/%1"

CopyFunctionSettingsDialog::CopyFunctionSettingsDialog(Function *function, const Doc *doc, QWidget *parent)
    : QDialog(parent)
    , m_function(function)
    , m_doc(doc)
    , m_selectAllBox(nullptr)
    , m_blockSignals(false)
{
    Q_ASSERT(function != nullptr);
    setWindowTitle(tr("Copy Function Settings to Clipboard"));
    buildUI();
}

void CopyFunctionSettingsDialog::buildUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *infoLabel = new QLabel(
        tr("Select which parameter groups to copy from <b>%1</b>:")
            .arg(m_function->name()), this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    m_selectAllBox = new QCheckBox(tr("Select All"), this);
    m_selectAllBox->setChecked(true);
    connect(m_selectAllBox, SIGNAL(toggled(bool)), this, SLOT(slotSelectAllToggled(bool)));
    layout->addWidget(m_selectAllBox);

    /* Read persisted flags for this function type */
    QSettings settings;
    QString settingsKey = QString(SETTINGS_KEY).arg(m_function->type());
    int savedFlags = settings.value(settingsKey, -1).toInt();

    const QList<QPair<int, QString>> groups = m_function->copyableParameterGroups();
    for (const QPair<int, QString> &group : groups)
    {
        QCheckBox *cb = new QCheckBox(group.second, this);
        bool checked = (savedFlags == -1) ? true : (savedFlags & group.first);
        cb->setChecked(checked);
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(slotCheckBoxToggled(bool)));
        layout->addWidget(cb);
        m_entries << QPair<int, QCheckBox*>(group.first, cb);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);
}

int CopyFunctionSettingsDialog::selectedFlags() const
{
    int flags = 0;
    for (const QPair<int, QCheckBox*> &entry : m_entries)
    {
        if (entry.second->isChecked())
            flags |= entry.first;
    }
    return flags;
}

bool CopyFunctionSettingsDialog::copyToClipboard()
{
    int flags = selectedFlags();
    if (flags == 0)
        return false;

    QJsonObject obj;
    obj["type"]         = QString("qlc_function_settings");
    obj["version"]      = 1;
    obj["functionType"] = static_cast<int>(m_function->type());
    obj["functionName"] = m_function->name();
    obj["flags"]        = flags;

    m_function->settingsToJson(obj, flags, m_doc);

    QJsonDocument doc(obj);
    QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Compact));
    return true;
}

void CopyFunctionSettingsDialog::accept()
{
    /* Persist selected flags for this function type */
    QSettings settings;
    settings.setValue(QString(SETTINGS_KEY).arg(m_function->type()), selectedFlags());

    if (copyToClipboard())
        QDialog::accept();
}

void CopyFunctionSettingsDialog::slotSelectAllToggled(bool checked)
{
    if (m_blockSignals)
        return;
    m_blockSignals = true;
    for (const QPair<int, QCheckBox*> &entry : m_entries)
        entry.second->setChecked(checked);
    m_blockSignals = false;
}

void CopyFunctionSettingsDialog::slotCheckBoxToggled(bool /*checked*/)
{
    if (m_blockSignals)
        return;
    /* Update "Select All" state */
    bool allChecked = true;
    bool anyChecked = false;
    for (const QPair<int, QCheckBox*> &entry : m_entries)
    {
        if (entry.second->isChecked())
            anyChecked = true;
        else
            allChecked = false;
    }
    m_blockSignals = true;
    m_selectAllBox->setCheckState(allChecked ? Qt::Checked
                                  : anyChecked ? Qt::PartiallyChecked
                                               : Qt::Unchecked);
    m_blockSignals = false;
}

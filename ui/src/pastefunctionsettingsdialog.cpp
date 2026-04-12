/*
  Q Light Controller Plus
  pastefunctionsettingsdialog.cpp

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
#include <QPushButton>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

#include "pastefunctionsettingsdialog.h"
#include "function.h"
#include "doc.h"

PasteFunctionSettingsDialog::PasteFunctionSettingsDialog(const QJsonObject &clipboardJson,
                                                         const QList<Function*> &targets,
                                                         Doc *doc,
                                                         QWidget *parent)
    : QDialog(parent)
    , m_json(clipboardJson)
    , m_targets(targets)
    , m_doc(doc)
    , m_storedFlags(clipboardJson["flags"].toInt(-1))
    , m_selectAllBox(nullptr)
    , m_blockSignals(false)
{
    setWindowTitle(tr("Paste Function Settings from Clipboard"));
    buildUI();
}

void PasteFunctionSettingsDialog::buildUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    /* Info: source */
    QString srcName = m_json["functionName"].toString();
    int srcType     = m_json["functionType"].toInt(-1);
    QString srcTypeStr = (srcType >= 0)
        ? Function::typeToString(static_cast<Function::Type>(srcType))
        : tr("Unknown");

    QLabel *srcLabel = new QLabel(
        tr("<b>Source:</b> %1 (%2)").arg(srcName).arg(srcTypeStr), this);
    layout->addWidget(srcLabel);

    /* Info: targets */
    QString targetDesc;
    if (m_targets.isEmpty())
        targetDesc = tr("<i>No target functions selected.</i>");
    else if (m_targets.size() == 1)
        targetDesc = tr("<b>Target:</b> %1").arg(m_targets.first()->name());
    else
        targetDesc = tr("<b>Targets:</b> %1 functions selected").arg(m_targets.size());
    QLabel *tgtLabel = new QLabel(targetDesc, this);
    layout->addWidget(tgtLabel);

    /* Separator */
    QFrame *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    layout->addWidget(sep);

    QLabel *groupLabel = new QLabel(tr("Select parameter groups to apply:"), this);
    layout->addWidget(groupLabel);

    m_selectAllBox = new QCheckBox(tr("Select All"), this);
    connect(m_selectAllBox, SIGNAL(toggled(bool)), this, SLOT(slotSelectAllToggled(bool)));
    layout->addWidget(m_selectAllBox);

    /* Get parameter group names from the first target (or fallback to stored flags only) */
    QList<QPair<int, QString>> groups;
    if (!m_targets.isEmpty())
        groups = m_targets.first()->copyableParameterGroups();

    bool allChecked = true;
    for (const QPair<int, QString> &group : groups)
    {
        bool present  = (m_storedFlags == -1) || (m_storedFlags & group.first);
        QCheckBox *cb = new QCheckBox(group.second, this);
        cb->setChecked(present);
        cb->setEnabled(present);
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(slotCheckBoxToggled(bool)));
        layout->addWidget(cb);
        m_entries << QPair<int, QCheckBox*>(group.first, cb);
        if (!present)
            allChecked = false;
    }

    m_blockSignals = true;
    m_selectAllBox->setChecked(allChecked);
    m_blockSignals = false;

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okBtn = buttons->button(QDialogButtonBox::Ok);
    okBtn->setText(tr("Apply"));
    okBtn->setEnabled(!m_targets.isEmpty());
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);
}

int PasteFunctionSettingsDialog::selectedFlags() const
{
    int flags = 0;
    for (const QPair<int, QCheckBox*> &entry : m_entries)
    {
        if (entry.second->isChecked())
            flags |= entry.first;
    }
    return flags;
}

void PasteFunctionSettingsDialog::accept()
{
    int flags = selectedFlags();
    for (Function *f : m_targets)
        f->applySettingsFromJson(m_json, flags, m_doc);
    QDialog::accept();
}

bool PasteFunctionSettingsDialog::clipboardHasFunctionSettings()
{
    QJsonObject obj = clipboardJson();
    return !obj.isEmpty();
}

QJsonObject PasteFunctionSettingsDialog::clipboardJson()
{
    QString text = QApplication::clipboard()->text();
    if (text.isEmpty())
        return QJsonObject();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return QJsonObject();

    QJsonObject root = doc.object();
    /* Accept both new cross-project format and legacy formats */
    QString type = root["type"].toString();
    if (type != "qlc_function_settings" &&
        type != "qlc_rgb_matrix_settings" &&
        type != "qlc_efx_settings")
        return QJsonObject();

    return root;
}

void PasteFunctionSettingsDialog::slotSelectAllToggled(bool checked)
{
    if (m_blockSignals)
        return;
    m_blockSignals = true;
    for (const QPair<int, QCheckBox*> &entry : m_entries)
    {
        if (entry.second->isEnabled())
            entry.second->setChecked(checked);
    }
    m_blockSignals = false;
}

void PasteFunctionSettingsDialog::slotCheckBoxToggled(bool /*checked*/)
{
    if (m_blockSignals)
        return;
    bool allChecked = true;
    bool anyChecked = false;
    for (const QPair<int, QCheckBox*> &entry : m_entries)
    {
        if (!entry.second->isEnabled())
            continue;
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

/*
  Q Light Controller Plus
  vcpastepropertiesdialog.cpp

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

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

#include "vcpastepropertiesdialog.h"

VCPastePropertiesDialog::VCPastePropertiesDialog(VCWidget* source,
                                                  VCWidget* target,
                                                  QWidget* parent)
    : QDialog(parent)
    , m_selectAllBox(nullptr)
    , m_blockSignals(false)
{
    buildUI(source, target);
}

void VCPastePropertiesDialog::buildUI(VCWidget* source, VCWidget* target)
{
    setWindowTitle(tr("Paste Properties"));
    setMinimumWidth(320);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(12, 12, 12, 8);

    // Source / target labels
    QLabel* srcLabel = new QLabel(
        tr("Source: %1 \"%2\"")
            .arg(VCWidget::typeToString(source->type()))
            .arg(source->caption()), this);
    QLabel* tgtLabel = new QLabel(
        tr("Target: %1 \"%2\"")
            .arg(VCWidget::typeToString(target->type()))
            .arg(target->caption()), this);
    mainLayout->addWidget(srcLabel);
    mainLayout->addWidget(tgtLabel);

    // Horizontal separator
    QFrame* topLine = new QFrame(this);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(topLine);

    // "Select All" master checkbox
    m_selectAllBox = new QCheckBox(tr("Select All"), this);
    m_selectAllBox->setTristate(true);
    m_selectAllBox->setCheckState(Qt::Checked);
    mainLayout->addWidget(m_selectAllBox);
    connect(m_selectAllBox, &QCheckBox::toggled,
            this, &VCPastePropertiesDialog::slotSelectAllToggled);

    // Build property group checkboxes from the source widget
    QList<QPair<VCWidget::PastePropertyGroup, QString>> groups =
        source->pasteablePropertyGroups();

    // Split into common (bits 0-7) and widget-specific (bits 8+)
    QList<QPair<VCWidget::PastePropertyGroup, QString>> commonGroups;
    QList<QPair<VCWidget::PastePropertyGroup, QString>> specificGroups;
    for (const auto& g : groups)
    {
        if (static_cast<int>(g.first) < (1 << 8))
            commonGroups << g;
        else
            specificGroups << g;
    }

    if (!commonGroups.isEmpty())
    {
        mainLayout->addWidget(makeSeparator(tr("Common")));
        for (const auto& g : commonGroups)
        {
            QCheckBox* cb = new QCheckBox(g.second, this);
            cb->setChecked(true);
            mainLayout->addWidget(cb);
            m_entries << qMakePair(g.first, cb);
            connect(cb, &QCheckBox::toggled,
                    this, &VCPastePropertiesDialog::slotCheckBoxToggled);
        }
    }

    if (!specificGroups.isEmpty())
    {
        mainLayout->addWidget(makeSeparator(VCWidget::typeToString(source->type())));
        for (const auto& g : specificGroups)
        {
            QCheckBox* cb = new QCheckBox(g.second, this);
            cb->setChecked(true);
            mainLayout->addWidget(cb);
            m_entries << qMakePair(g.first, cb);
            connect(cb, &QCheckBox::toggled,
                    this, &VCPastePropertiesDialog::slotCheckBoxToggled);
        }
    }

    // OK / Cancel buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QFrame* VCPastePropertiesDialog::makeSeparator(const QString& title)
{
    QFrame* frame = new QFrame(this);
    QHBoxLayout* lay = new QHBoxLayout(frame);
    lay->setContentsMargins(0, 4, 0, 0);
    lay->setSpacing(4);

    QFrame* line1 = new QFrame(frame);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    line1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    line1->setFixedWidth(12);

    QLabel* lbl = new QLabel(QString("<b>%1</b>").arg(title), frame);

    QFrame* line2 = new QFrame(frame);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    lay->addWidget(line1);
    lay->addWidget(lbl);
    lay->addWidget(line2);

    return frame;
}

VCWidget::PastePropertyGroups VCPastePropertiesDialog::selectedFlags() const
{
    VCWidget::PastePropertyGroups flags;
    for (const auto& entry : m_entries)
    {
        if (entry.second->isChecked())
            flags |= entry.first;
    }
    return flags;
}

void VCPastePropertiesDialog::slotSelectAllToggled(bool checked)
{
    if (m_blockSignals)
        return;
    m_blockSignals = true;
    for (const auto& entry : m_entries)
        entry.second->setChecked(checked);
    m_blockSignals = false;
}

void VCPastePropertiesDialog::slotCheckBoxToggled(bool /*checked*/)
{
    if (m_blockSignals)
        return;

    bool allChecked = true;
    bool anyChecked = false;
    for (const auto& entry : m_entries)
    {
        if (entry.second->isChecked())
            anyChecked = true;
        else
            allChecked = false;
    }

    m_blockSignals = true;
    if (allChecked)
        m_selectAllBox->setCheckState(Qt::Checked);
    else if (anyChecked)
        m_selectAllBox->setCheckState(Qt::PartiallyChecked);
    else
        m_selectAllBox->setCheckState(Qt::Unchecked);
    m_blockSignals = false;
}

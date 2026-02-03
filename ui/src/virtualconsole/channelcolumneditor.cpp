/*
  Q Light Controller Plus
  channelcolumneditor.cpp

  Copyright (c) Massimo Callegari

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
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLabel>

#include "channelcolumneditor.h"
#include "vccuelist.h"

ChannelColumnEditor::ChannelColumnEditor(ChannelColumnInfo &info, QWidget *parent)
    : QDialog(parent)
    , m_info(info)
{
    setupUi();
    loadFromInfo();
}

ChannelColumnEditor::~ChannelColumnEditor()
{
}

void ChannelColumnEditor::setupUi()
{
    setWindowTitle(tr("Channel Column Settings"));
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Name section
    QFormLayout *nameLayout = new QFormLayout();
    m_nameEdit = new QLineEdit(this);
    nameLayout->addRow(tr("Name:"), m_nameEdit);
    mainLayout->addLayout(nameLayout);

    // Hidden checkbox
    m_hiddenCheck = new QCheckBox(tr("Hide this column"), this);
    mainLayout->addWidget(m_hiddenCheck);

    // Display mode section
    QFormLayout *modeLayout = new QFormLayout();
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(tr("Raw DMX (0-255)"), DisplayRaw);
    m_modeCombo->addItem(tr("Scaled Values"), DisplayScaled);
    m_modeCombo->addItem(tr("Dropdown Selection"), DisplayDropdown);
    modeLayout->addRow(tr("Display Mode:"), m_modeCombo);
    mainLayout->addLayout(modeLayout);

    // Stacked widget for mode-specific settings
    m_settingsStack = new QStackedWidget(this);

    // Page 0: Raw mode (empty placeholder)
    QWidget *rawPage = new QWidget(this);
    QVBoxLayout *rawLayout = new QVBoxLayout(rawPage);
    rawLayout->addWidget(new QLabel(tr("Values displayed as raw DMX (0-255)"), this));
    rawLayout->addStretch();
    m_settingsStack->addWidget(rawPage);

    // Page 1: Scaled mode
    QWidget *scaledPage = new QWidget(this);
    QVBoxLayout *scaledLayout = new QVBoxLayout(scaledPage);

    QGroupBox *scaleGroup = new QGroupBox(tr("Scale Settings"), this);
    QFormLayout *scaleFormLayout = new QFormLayout(scaleGroup);

    m_scaleMinSpin = new QDoubleSpinBox(this);
    m_scaleMinSpin->setRange(-99999.0, 99999.0);
    m_scaleMinSpin->setDecimals(1);
    m_scaleMinSpin->setValue(0.0);
    scaleFormLayout->addRow(tr("Minimum:"), m_scaleMinSpin);

    m_scaleMaxSpin = new QDoubleSpinBox(this);
    m_scaleMaxSpin->setRange(-99999.0, 99999.0);
    m_scaleMaxSpin->setDecimals(1);
    m_scaleMaxSpin->setValue(255.0);
    scaleFormLayout->addRow(tr("Maximum:"), m_scaleMaxSpin);

    m_scaleSuffixEdit = new QLineEdit(this);
    m_scaleSuffixEdit->setPlaceholderText(tr("e.g., °, %, rpm"));
    m_scaleSuffixEdit->setMaxLength(10);
    scaleFormLayout->addRow(tr("Suffix:"), m_scaleSuffixEdit);

    scaledLayout->addWidget(scaleGroup);

    // Examples label
    QLabel *examplesLabel = new QLabel(tr("Examples:\n"
        "  Pan: Min=0, Max=540, Suffix=°\n"
        "  Dimmer: Min=0, Max=100, Suffix=%\n"
        "  Tilt: Min=-135, Max=135, Suffix=°"), this);
    examplesLabel->setStyleSheet("color: gray; font-size: 10px;");
    scaledLayout->addWidget(examplesLabel);
    scaledLayout->addStretch();

    m_settingsStack->addWidget(scaledPage);

    // Page 2: Dropdown mode
    QWidget *dropdownPage = new QWidget(this);
    QVBoxLayout *dropdownLayout = new QVBoxLayout(dropdownPage);

    QGroupBox *mappingGroup = new QGroupBox(tr("Value Mappings"), this);
    QVBoxLayout *mappingLayout = new QVBoxLayout(mappingGroup);

    m_mappingTable = new QTableWidget(this);
    m_mappingTable->setColumnCount(2);
    m_mappingTable->setHorizontalHeaderLabels(QStringList() << tr("DMX Value") << tr("Label"));
    m_mappingTable->horizontalHeader()->setStretchLastSection(true);
    m_mappingTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_mappingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mappingTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mappingLayout->addWidget(m_mappingTable);

    QHBoxLayout *mappingBtnLayout = new QHBoxLayout();
    m_addMappingBtn = new QToolButton(this);
    m_addMappingBtn->setIcon(QIcon(":/edit_add.png"));
    m_addMappingBtn->setToolTip(tr("Add mapping"));
    mappingBtnLayout->addWidget(m_addMappingBtn);

    m_removeMappingBtn = new QToolButton(this);
    m_removeMappingBtn->setIcon(QIcon(":/edit_remove.png"));
    m_removeMappingBtn->setToolTip(tr("Remove mapping"));
    mappingBtnLayout->addWidget(m_removeMappingBtn);

    mappingBtnLayout->addStretch();
    mappingLayout->addLayout(mappingBtnLayout);

    dropdownLayout->addWidget(mappingGroup);

    QLabel *dropdownHint = new QLabel(tr("Define DMX values and their labels.\n"
        "Values between mappings will show the nearest lower mapping."), this);
    dropdownHint->setStyleSheet("color: gray; font-size: 10px;");
    dropdownLayout->addWidget(dropdownHint);

    m_settingsStack->addWidget(dropdownPage);

    mainLayout->addWidget(m_settingsStack);

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    // Connections
    connect(m_modeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotDisplayModeChanged(int)));
    connect(m_addMappingBtn, SIGNAL(clicked()), this, SLOT(slotAddMapping()));
    connect(m_removeMappingBtn, SIGNAL(clicked()), this, SLOT(slotRemoveMapping()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ChannelColumnEditor::loadFromInfo()
{
    m_nameEdit->setText(m_info.customName);
    m_hiddenCheck->setChecked(m_info.hidden);

    // Set display mode
    int modeIndex = m_modeCombo->findData(m_info.displayMode);
    if (modeIndex >= 0)
        m_modeCombo->setCurrentIndex(modeIndex);

    // Load scale settings
    m_scaleMinSpin->setValue(m_info.scaleMin);
    m_scaleMaxSpin->setValue(m_info.scaleMax);
    m_scaleSuffixEdit->setText(m_info.scaleSuffix);

    // Load dropdown mappings
    m_mappingTable->setRowCount(0);
    QMapIterator<int, QString> it(m_info.dropdownMappings);
    while (it.hasNext())
    {
        it.next();
        int row = m_mappingTable->rowCount();
        m_mappingTable->insertRow(row);

        QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(it.key()));
        QTableWidgetItem *labelItem = new QTableWidgetItem(it.value());
        m_mappingTable->setItem(row, 0, valueItem);
        m_mappingTable->setItem(row, 1, labelItem);
    }

    // Update stack visibility
    slotDisplayModeChanged(m_modeCombo->currentIndex());
}

void ChannelColumnEditor::saveToInfo()
{
    m_info.customName = m_nameEdit->text();
    m_info.hidden = m_hiddenCheck->isChecked();
    m_info.displayMode = static_cast<ChannelDisplayMode>(m_modeCombo->currentData().toInt());

    // Save scale settings
    m_info.scaleMin = m_scaleMinSpin->value();
    m_info.scaleMax = m_scaleMaxSpin->value();
    m_info.scaleSuffix = m_scaleSuffixEdit->text();

    // Save dropdown mappings
    m_info.dropdownMappings.clear();
    for (int row = 0; row < m_mappingTable->rowCount(); ++row)
    {
        QTableWidgetItem *valueItem = m_mappingTable->item(row, 0);
        QTableWidgetItem *labelItem = m_mappingTable->item(row, 1);
        if (valueItem && labelItem)
        {
            int dmxValue = valueItem->text().toInt();
            QString label = labelItem->text();
            if (!label.isEmpty() && dmxValue >= 0 && dmxValue <= 255)
                m_info.dropdownMappings[dmxValue] = label;
        }
    }
}

ChannelColumnInfo ChannelColumnEditor::columnInfo() const
{
    return m_info;
}

void ChannelColumnEditor::slotDisplayModeChanged(int index)
{
    ChannelDisplayMode mode = static_cast<ChannelDisplayMode>(m_modeCombo->itemData(index).toInt());
    switch (mode)
    {
        case DisplayRaw:
            m_settingsStack->setCurrentIndex(0);
            break;
        case DisplayScaled:
            m_settingsStack->setCurrentIndex(1);
            break;
        case DisplayDropdown:
            m_settingsStack->setCurrentIndex(2);
            break;
    }
}

void ChannelColumnEditor::slotAddMapping()
{
    int row = m_mappingTable->rowCount();
    m_mappingTable->insertRow(row);

    // Default to next available value
    int nextValue = 0;
    if (row > 0)
    {
        QTableWidgetItem *prevItem = m_mappingTable->item(row - 1, 0);
        if (prevItem)
            nextValue = qMin(255, prevItem->text().toInt() + 10);
    }

    QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(nextValue));
    QTableWidgetItem *labelItem = new QTableWidgetItem(tr("Label %1").arg(row + 1));
    m_mappingTable->setItem(row, 0, valueItem);
    m_mappingTable->setItem(row, 1, labelItem);

    m_mappingTable->selectRow(row);
    m_mappingTable->editItem(labelItem);
}

void ChannelColumnEditor::slotRemoveMapping()
{
    int row = m_mappingTable->currentRow();
    if (row >= 0)
        m_mappingTable->removeRow(row);
}

void ChannelColumnEditor::accept()
{
    saveToInfo();
    QDialog::accept();
}

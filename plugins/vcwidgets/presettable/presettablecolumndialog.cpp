/*
  QLC+ VC Widget Plugin — Preset Table
  presettablecolumndialog.cpp — Apache 2.0 / public domain
*/

#include "presettablecolumndialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSpinBox>

PresetTableColumnDialog::PresetTableColumnDialog(const PTColumn& column, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Column"));
    setMinimumWidth(360);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ---- Name -------------------------------------------------------
    QGroupBox* nameGrp = new QGroupBox(tr("Column"), this);
    QFormLayout* form = new QFormLayout(nameGrp);
    m_nameEdit = new QLineEdit(column.name, nameGrp);
    form->addRow(tr("Name:"), m_nameEdit);
    root->addWidget(nameGrp);

    // ---- Type -------------------------------------------------------
    QGroupBox* typeGrp = new QGroupBox(tr("Value type"), this);
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGrp);
    m_rbNumeric  = new QRadioButton(tr("Numeric  (0 – 255 spinbox)"), typeGrp);
    m_rbDropdown = new QRadioButton(tr("Dropdown (named options mapped to values)"), typeGrp);
    typeLayout->addWidget(m_rbNumeric);
    typeLayout->addWidget(m_rbDropdown);
    root->addWidget(typeGrp);

    // ---- Dropdown options -------------------------------------------
    QGroupBox* optGrp = new QGroupBox(tr("Dropdown options"), this);
    QVBoxLayout* optLayout = new QVBoxLayout(optGrp);

    m_optTable = new QTableWidget(0, 2, optGrp);
    m_optTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("DMX value (0–255)"));
    m_optTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_optTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_optTable->verticalHeader()->hide();
    m_optTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_optTable->setSelectionMode(QAbstractItemView::SingleSelection);
    optLayout->addWidget(m_optTable);

    QHBoxLayout* btnRow = new QHBoxLayout;
    m_addOptBtn = new QPushButton(tr("+ Option"), optGrp);
    m_remOptBtn = new QPushButton(tr("- Option"), optGrp);
    btnRow->addWidget(m_addOptBtn);
    btnRow->addWidget(m_remOptBtn);
    btnRow->addStretch();
    optLayout->addLayout(btnRow);
    root->addWidget(optGrp);

    // ---- Crossfade behavior -----------------------------------------
    QGroupBox* xfGrp = new QGroupBox(tr("Crossfade behavior"), this);
    QVBoxLayout* xfLayout = new QVBoxLayout(xfGrp);
    m_rbFade = new QRadioButton(tr("Fade  (linear interpolation between A and B)"), xfGrp);
    m_rbSnap = new QRadioButton(tr("Snap  (switch at 50%: pos \u2264 127 \u2192 A, pos > 127 \u2192 B)"), xfGrp);
    xfLayout->addWidget(m_rbFade);
    xfLayout->addWidget(m_rbSnap);
    root->addWidget(xfGrp);

    // ---- Buttons ----------------------------------------------------
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    // ---- Populate ---------------------------------------------------
    if (column.type == PTColumn::Dropdown)
        m_rbDropdown->setChecked(true);
    else
        m_rbNumeric->setChecked(true);

    if (column.fade)
        m_rbFade->setChecked(true);
    else
        m_rbSnap->setChecked(true);

    m_optTable->blockSignals(true);
    for (const PTOption& opt : column.options)
    {
        int r = m_optTable->rowCount();
        m_optTable->insertRow(r);
        m_optTable->setItem(r, 0, new QTableWidgetItem(opt.name));
        QTableWidgetItem* valItem = new QTableWidgetItem(QString::number(opt.value));
        valItem->setData(Qt::EditRole, int(opt.value));
        m_optTable->setItem(r, 1, valItem);
    }
    m_optTable->blockSignals(false);

    updateOptionsEnabled();

    // ---- Connections ------------------------------------------------
    connect(m_rbNumeric,  &QRadioButton::toggled, this, &PresetTableColumnDialog::slotTypeChanged);
    connect(m_rbDropdown, &QRadioButton::toggled, this, &PresetTableColumnDialog::slotTypeChanged);
    connect(m_addOptBtn,  &QPushButton::clicked,  this, &PresetTableColumnDialog::slotAddOption);
    connect(m_remOptBtn,  &QPushButton::clicked,  this, &PresetTableColumnDialog::slotRemoveOption);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

PTColumn PresetTableColumnDialog::column() const
{
    PTColumn col;
    col.name = m_nameEdit->text().trimmed();
    col.type = m_rbDropdown->isChecked() ? PTColumn::Dropdown : PTColumn::Numeric;
    col.fade = m_rbFade->isChecked();

    if (col.type == PTColumn::Dropdown)
    {
        for (int r = 0; r < m_optTable->rowCount(); ++r)
        {
            PTOption opt;
            QTableWidgetItem* nameItem = m_optTable->item(r, 0);
            QTableWidgetItem* valItem  = m_optTable->item(r, 1);
            opt.name  = nameItem ? nameItem->text() : QString();
            opt.value = valItem ? uchar(valItem->data(Qt::EditRole).toInt()) : 0;
            if (!opt.name.isEmpty())
                col.options.append(opt);
        }
    }

    return col;
}

void PresetTableColumnDialog::slotTypeChanged()
{
    updateOptionsEnabled();
}

void PresetTableColumnDialog::slotAddOption()
{
    int r = m_optTable->rowCount();
    m_optTable->insertRow(r);
    m_optTable->setItem(r, 0, new QTableWidgetItem(tr("Option %1").arg(r + 1)));
    QTableWidgetItem* valItem = new QTableWidgetItem(QString::number(0));
    valItem->setData(Qt::EditRole, 0);
    m_optTable->setItem(r, 1, valItem);
    m_optTable->scrollToBottom();
    m_optTable->editItem(m_optTable->item(r, 0));
}

void PresetTableColumnDialog::slotRemoveOption()
{
    int row = m_optTable->currentRow();
    if (row >= 0)
        m_optTable->removeRow(row);
}

void PresetTableColumnDialog::updateOptionsEnabled()
{
    bool dropdown = m_rbDropdown->isChecked();
    m_optTable->setEnabled(dropdown);
    m_addOptBtn->setEnabled(dropdown);
    m_remOptBtn->setEnabled(dropdown);
}

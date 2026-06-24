/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2columndialog.cpp — Apache 2.0 / public domain
*/

#include "presettablev2columndialog.h"

#include "doc.h"
#include "fixture.h"
#include "fixturegroup.h"
#include "grouphead.h"
#include "qlcchannel.h"
#include "qlccapability.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "qlcfixturehead.h"
#include "inputselectionwidget.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QListWidget>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QColor>
#include <QSet>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QIcon PresetTableV2ColumnDialog::makeResourceIcon(const QString& resource)
{
    if (resource.isEmpty())
        return QIcon();
    if (resource.startsWith('#'))
    {
        QPixmap pm(16, 16);
        pm.fill(QColor(resource));
        return QIcon(pm);
    }
    QPixmap pm;
    pm.load(resource);
    if (!pm.isNull())
        pm = pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return pm.isNull() ? QIcon() : QIcon(pm);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

PresetTableV2ColumnDialog::PresetTableV2ColumnDialog(Doc* doc,
                                                   const PTColumn& column,
                                                   PTMode mode,
                                                   FixtureGroup* group,
                                                   const QVector<PTOutput>& outputs,
                                                   int widgetPage,
                                                   QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_mode(mode)
    , m_group(group)
    , m_outputs(outputs)
    , m_widgetPage(widgetPage)
{
    setWindowTitle(tr("Edit Column"));
    setMinimumWidth(400);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ---- Name -------------------------------------------------------
    QGroupBox* nameGrp = new QGroupBox(tr("Column"), this);
    QFormLayout* form = new QFormLayout(nameGrp);
    m_nameEdit = new QLineEdit(column.name, nameGrp);
    form->addRow(tr("Name:"), m_nameEdit);
    root->addWidget(nameGrp);

    // ---- Fixture Binding (FixtureGroup mode only) -------------------
    m_bindGrp = new QGroupBox(tr("Fixture Binding"), this);
    m_bindGrp->setVisible(mode == PTMode::FixtureGroup);
    QVBoxLayout* bindLayout = new QVBoxLayout(m_bindGrp);

    m_bindTable = new QTableWidget(0, 2, m_bindGrp);
    m_bindTable->setHorizontalHeaderLabels(QStringList()
            << tr("Fixture type") << tr("Channel"));
    m_bindTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_bindTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_bindTable->verticalHeader()->hide();
    m_bindTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bindTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_bindTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_bindTable->setMinimumHeight(120);
    bindLayout->addWidget(m_bindTable);

    QHBoxLayout* bindBtnRow = new QHBoxLayout;
    m_addBindBtn = new QPushButton(tr("+ Add channels..."), m_bindGrp);
    m_remBindBtn = new QPushButton(tr("- Remove"), m_bindGrp);
    bindBtnRow->addWidget(m_addBindBtn);
    bindBtnRow->addWidget(m_remBindBtn);
    bindBtnRow->addStretch();
    bindLayout->addLayout(bindBtnRow);

    root->addWidget(m_bindGrp);

    // ---- Per-output final multiplier inputs -------------------------
    m_intensityGrp = new QGroupBox(tr("Column multiplier inputs"), this);
    m_intensityGrp->setVisible(mode == PTMode::FixtureGroup);
    QFormLayout* intensityForm = new QFormLayout(m_intensityGrp);
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const QString outLabel = m_outputs.at(o).name.isEmpty()
                ? tr("Output %1").arg(o + 1) : m_outputs.at(o).name;
        InputSelectionWidget* sel = new InputSelectionWidget(m_doc, m_intensityGrp);
        sel->setWidgetPage(m_widgetPage);
        sel->setKeyInputVisibility(false);
        if (o < column.intensityInputSources.size())
            sel->setInputSource(column.intensityInputSources.at(o));
        m_intensityInputSels.append(sel);
        intensityForm->addRow(outLabel, sel);
    }
    root->addWidget(m_intensityGrp);

    // ---- Type -------------------------------------------------------
    QGroupBox* typeGrp = new QGroupBox(tr("Value type"), this);
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGrp);
    m_rbNumeric  = new QRadioButton(tr("Numeric  (0 - 255 spinbox)"), typeGrp);
    m_rbDropdown = new QRadioButton(tr("Dropdown (named options mapped to values)"), typeGrp);
    m_rbScaler   = new QRadioButton(tr("Scaler   (range mapped to DMX 0-255)"), typeGrp);
    typeLayout->addWidget(m_rbNumeric);
    typeLayout->addWidget(m_rbDropdown);
    typeLayout->addWidget(m_rbScaler);
    root->addWidget(typeGrp);

    // ---- Dropdown options -------------------------------------------
    QGroupBox* optGrp = new QGroupBox(tr("Dropdown options"), this);
    QVBoxLayout* optLayout = new QVBoxLayout(optGrp);

    m_optTable = new QTableWidget(0, 2, optGrp);
    m_optTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("DMX value (0-255)"));
    m_optTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_optTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_optTable->verticalHeader()->hide();
    m_optTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_optTable->setSelectionMode(QAbstractItemView::SingleSelection);
    optLayout->addWidget(m_optTable);

    QHBoxLayout* btnRow = new QHBoxLayout;
    m_addOptBtn  = new QPushButton(tr("+ Option"), optGrp);
    m_remOptBtn  = new QPushButton(tr("- Option"), optGrp);
    m_importBtn  = new QPushButton(tr("Import from channel..."), optGrp);
    btnRow->addWidget(m_addOptBtn);
    btnRow->addWidget(m_remOptBtn);
    btnRow->addSpacing(8);
    btnRow->addWidget(m_importBtn);
    btnRow->addStretch();
    optLayout->addLayout(btnRow);
    root->addWidget(optGrp);

    // ---- Scaler options ---------------------------------------------
    m_scalerGrp = new QGroupBox(tr("Scaler options"), this);
    QFormLayout* scalerForm = new QFormLayout(m_scalerGrp);
    m_scalerMin    = new QSpinBox(m_scalerGrp);
    m_scalerMin->setRange(-100000, 100000);
    m_scalerMin->setValue(0);
    m_scalerMax    = new QSpinBox(m_scalerGrp);
    m_scalerMax->setRange(-100000, 100000);
    m_scalerMax->setValue(360);
    m_scalerSuffix = new QLineEdit(m_scalerGrp);
    m_scalerSuffix->setPlaceholderText(tr("e.g. °"));
    scalerForm->addRow(tr("Min:"),    m_scalerMin);
    scalerForm->addRow(tr("Max:"),    m_scalerMax);
    scalerForm->addRow(tr("Suffix:"), m_scalerSuffix);
    root->addWidget(m_scalerGrp);

    // ---- Crossfade behavior -----------------------------------------
    QGroupBox* xfGrp = new QGroupBox(tr("Crossfade behavior"), this);
    QVBoxLayout* xfLayout = new QVBoxLayout(xfGrp);
    m_rbFade = new QRadioButton(tr("Fade  (linear interpolation between A and B)"), xfGrp);
    m_rbSnap = new QRadioButton(tr("Snap  (switch at 50%: pos <= 127 -> A, pos > 127 -> B)"), xfGrp);
    xfLayout->addWidget(m_rbFade);
    xfLayout->addWidget(m_rbSnap);
    root->addWidget(xfGrp);

    // ---- Buttons ----------------------------------------------------
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    // ---- Populate ---------------------------------------------------
    if (column.type == PTColumn::Dropdown)
        m_rbDropdown->setChecked(true);
    else if (column.type == PTColumn::Scaler)
        m_rbScaler->setChecked(true);
    else
        m_rbNumeric->setChecked(true);

    // Scaler fields
    m_scalerMin->setValue(column.scalerMin);
    m_scalerMax->setValue(column.scalerMax);
    m_scalerSuffix->setText(column.scalerSuffix);

    if (column.fade)
        m_rbFade->setChecked(true);
    else
        m_rbSnap->setChecked(true);

    m_optTable->blockSignals(true);
    for (const PTOption& opt : column.options)
    {
        int r = m_optTable->rowCount();
        m_optTable->insertRow(r);

        auto* nameItem = new QTableWidgetItem(opt.name);
        nameItem->setData(Qt::UserRole, opt.resource);
        if (!opt.resource.isEmpty())
            nameItem->setIcon(makeResourceIcon(opt.resource));
        m_optTable->setItem(r, 0, nameItem);

        auto* valItem = new QTableWidgetItem(QString::number(opt.value));
        valItem->setData(Qt::EditRole, int(opt.value));
        m_optTable->setItem(r, 1, valItem);
    }
    m_optTable->blockSignals(false);

    // Populate fixture bindings (FG mode)
    if (mode == PTMode::FixtureGroup && group)
    {
        populateFixtureTypeEntries();
        m_bindings = column.bindings;
        rebuildBindTable();
    }

    updateOptionsEnabled();

    // ---- Connections ------------------------------------------------
    connect(m_rbNumeric,  &QRadioButton::toggled, this, &PresetTableV2ColumnDialog::slotTypeChanged);
    connect(m_rbDropdown, &QRadioButton::toggled, this, &PresetTableV2ColumnDialog::slotTypeChanged);
    connect(m_rbScaler,   &QRadioButton::toggled, this, &PresetTableV2ColumnDialog::slotTypeChanged);
    connect(m_addOptBtn,  &QPushButton::clicked,  this, &PresetTableV2ColumnDialog::slotAddOption);
    connect(m_remOptBtn,  &QPushButton::clicked,  this, &PresetTableV2ColumnDialog::slotRemoveOption);
    connect(m_importBtn,  &QPushButton::clicked,  this, &PresetTableV2ColumnDialog::slotImportFromChannel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_addBindBtn, &QPushButton::clicked, this, &PresetTableV2ColumnDialog::slotAddBindings);
    connect(m_remBindBtn, &QPushButton::clicked, this, &PresetTableV2ColumnDialog::slotRemoveBindings);

    // If opened with Dropdown already selected but no options yet (e.g. type was
    // changed inline in the table before opening the dialog), auto-import now that
    // the binding combos are fully populated.
    if (column.type == PTColumn::Dropdown && column.options.isEmpty())
        autoImportFromBinding(/*onlyIfEmpty=*/true);
}

// ---------------------------------------------------------------------------
// Fixture binding population
// ---------------------------------------------------------------------------

void PresetTableV2ColumnDialog::populateFixtureTypeEntries()
{
    m_fxTypeEntries.clear();

    if (!m_group || !m_doc)
        return;

    for (quint32 fxiId : m_group->fixtureList())
    {
        Fixture* fxi = m_doc->fixture(fxiId);
        if (!fxi) continue;

        QLCFixtureDef*  def  = fxi->fixtureDef();
        QLCFixtureMode* mode = fxi->fixtureMode();
        if (!def || !mode) continue;

        const QString mfg   = def->manufacturer();
        const QString model = def->model();
        const QString mName = mode->name();

        bool found = false;
        for (FxTypeEntry& e : m_fxTypeEntries)
        {
            if (e.manufacturer == mfg && e.model == model && e.modeName == mName)
            {
                e.count++;
                found = true;
                break;
            }
        }
        if (!found)
        {
            FxTypeEntry e;
            e.manufacturer = mfg;
            e.model        = model;
            e.modeName     = mName;
            e.count        = 1;
            m_fxTypeEntries.append(e);
        }
    }
}

QLCFixtureMode* PresetTableV2ColumnDialog::representativeMode(const QString& mfg,
                                                              const QString& model,
                                                              const QString& modeName) const
{
    if (!m_group || !m_doc)
        return nullptr;

    for (quint32 fxiId : m_group->fixtureList())
    {
        Fixture* fxi = m_doc->fixture(fxiId);
        if (!fxi) continue;
        QLCFixtureDef*  def  = fxi->fixtureDef();
        QLCFixtureMode* mode = fxi->fixtureMode();
        if (!def || !mode) continue;
        if (def->manufacturer() == mfg && def->model() == model && mode->name() == modeName)
            return mode;
    }
    return nullptr;
}

QString PresetTableV2ColumnDialog::bindingTypeLabel(const PTColumnTypeBinding& binding) const
{
    return QString("%1 %2 — %3")
            .arg(binding.manufacturer, binding.model, binding.modeName);
}

QString PresetTableV2ColumnDialog::bindingChannelLabel(const PTColumnTypeBinding& binding) const
{
    QLCFixtureMode* fxMode = representativeMode(binding.manufacturer,
                                                binding.model,
                                                binding.modeName);
    if (!fxMode)
        return tr("ch%1").arg(binding.channelIndex + 1);

    QLCChannel* ch = fxMode->channel(quint32(binding.channelIndex));
    const QVector<QLCFixtureHead>& heads = fxMode->heads();
    int headIdx = -1;
    for (int hi = 0; hi < heads.size(); ++hi)
    {
        if (heads[hi].channels().contains(quint32(binding.channelIndex)))
        {
            headIdx = hi;
            break;
        }
    }

    QString prefix;
    if (!heads.isEmpty())
        prefix = (headIdx >= 0) ? QString("[H%1] ").arg(headIdx) : tr("[shared] ");

    if (ch)
        return prefix + QString("%1: %2").arg(binding.channelIndex + 1).arg(ch->name());
    return prefix + tr("Ch %1").arg(binding.channelIndex + 1);
}

bool PresetTableV2ColumnDialog::bindingsContain(const PTColumnTypeBinding& binding) const
{
    for (const PTColumnTypeBinding& existing : m_bindings)
    {
        if (existing == binding)
            return true;
    }
    return false;
}

void PresetTableV2ColumnDialog::rebuildBindTable()
{
    if (!m_bindTable)
        return;

    m_bindTable->setRowCount(0);
    for (int i = 0; i < m_bindings.size(); ++i)
    {
        const PTColumnTypeBinding& binding = m_bindings[i];
        if (!binding.isValid())
            continue;

        const int row = m_bindTable->rowCount();
        m_bindTable->insertRow(row);
        auto* typeItem = new QTableWidgetItem(bindingTypeLabel(binding));
        typeItem->setData(Qt::UserRole, i);
        m_bindTable->setItem(row, 0, typeItem);
        m_bindTable->setItem(row, 1, new QTableWidgetItem(bindingChannelLabel(binding)));
    }
}

// ---------------------------------------------------------------------------
// column() getter
// ---------------------------------------------------------------------------

PTColumn PresetTableV2ColumnDialog::column() const
{
    PTColumn col;
    col.name = m_nameEdit->text().trimmed();
    col.useFor1DFx = false;
    col.type = m_rbDropdown->isChecked() ? PTColumn::Dropdown :
               m_rbScaler->isChecked()   ? PTColumn::Scaler   :
                                           PTColumn::Numeric;
    col.fade = m_rbFade->isChecked();

    if (col.type == PTColumn::Scaler)
    {
        col.scalerMin    = m_scalerMin->value();
        col.scalerMax    = m_scalerMax->value();
        col.scalerSuffix = m_scalerSuffix->text();
    }

    if (col.type == PTColumn::Dropdown)
    {
        for (int r = 0; r < m_optTable->rowCount(); ++r)
        {
            PTOption opt;
            auto* nameItem = m_optTable->item(r, 0);
            auto* valItem  = m_optTable->item(r, 1);
            opt.name     = nameItem ? nameItem->text() : QString();
            opt.value    = valItem  ? uchar(valItem->data(Qt::EditRole).toInt()) : 0;
            opt.resource = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
            if (!opt.name.isEmpty())
                col.options.append(opt);
        }
    }

    if (m_mode == PTMode::FixtureGroup)
    {
        col.bindings = m_bindings;
        col.intensityInputSources.resize(m_intensityInputSels.size());
        for (int i = 0; i < m_intensityInputSels.size(); ++i)
        {
            if (m_intensityInputSels.at(i))
                col.intensityInputSources[i] = m_intensityInputSels.at(i)->inputSource();
        }
    }

    return col;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PresetTableV2ColumnDialog::slotTypeChanged()
{
    updateOptionsEnabled();
    autoImportFromBinding(/*onlyIfEmpty=*/true);
}

void PresetTableV2ColumnDialog::autoImportFromBinding(bool onlyIfEmpty)
{
    if (!m_rbDropdown->isChecked()) return;
    if (onlyIfEmpty && m_optTable->rowCount() > 0) return;

    if (m_bindings.isEmpty() || !m_group || !m_doc)
        return;

    const PTColumnTypeBinding& binding = m_bindings.first();
    if (!binding.isValid())
        return;

    QLCFixtureMode* fxMode = representativeMode(binding.manufacturer,
                                                binding.model,
                                                binding.modeName);
    if (!fxMode)
        return;

    const QLCChannel* chan = fxMode->channel(quint32(binding.channelIndex));
    if (!chan || chan->capabilities().isEmpty()) return;

    // Clear and repopulate options from channel capabilities
    m_optTable->setRowCount(0);
    for (QLCCapability* cap : chan->capabilities())
    {
        int r = m_optTable->rowCount();
        m_optTable->insertRow(r);

        QString resource;
        if (cap->presetType() == QLCCapability::Picture)
            resource = cap->resource(0).toString();
        else if (cap->presetType() == QLCCapability::SingleColor)
            resource = cap->resource(0).value<QColor>().name();

        auto* nameItem = new QTableWidgetItem(cap->name());
        nameItem->setData(Qt::UserRole, resource);
        if (!resource.isEmpty())
            nameItem->setIcon(makeResourceIcon(resource));
        m_optTable->setItem(r, 0, nameItem);

        auto* valItem = new QTableWidgetItem(QString::number(cap->min()));
        valItem->setData(Qt::EditRole, int(cap->min()));
        m_optTable->setItem(r, 1, valItem);
    }
}

void PresetTableV2ColumnDialog::slotAddBindings()
{
    if (!m_group || !m_doc || m_fxTypeEntries.isEmpty())
        return;

    QDialog picker(this);
    picker.setWindowTitle(tr("Add fixture channels"));
    picker.resize(520, 480);
    QVBoxLayout* pl = new QVBoxLayout(&picker);

    QScrollArea* scroll = new QScrollArea(&picker);
    scroll->setWidgetResizable(true);
    QWidget* scrollBody = new QWidget(scroll);
    QVBoxLayout* bodyLayout = new QVBoxLayout(scrollBody);

    QList<QListWidget*> channelLists;
    for (const FxTypeEntry& entry : m_fxTypeEntries)
    {
        QLCFixtureMode* fxMode = representativeMode(entry.manufacturer,
                                                    entry.model,
                                                    entry.modeName);
        if (!fxMode)
            continue;

        QLabel* typeLabel = new QLabel(QString("%1 %2 — %3 (%4×)")
                .arg(entry.manufacturer, entry.model, entry.modeName)
                .arg(entry.count), scrollBody);
        typeLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
        bodyLayout->addWidget(typeLabel);

        QListWidget* chanList = new QListWidget(scrollBody);
        channelLists.append(chanList);
        const QVector<QLCFixtureHead>& heads = fxMode->heads();
        const int nChannels = fxMode->channels().size();

        for (int ci = 0; ci < nChannels; ++ci)
        {
            PTColumnTypeBinding candidate;
            candidate.manufacturer = entry.manufacturer;
            candidate.model        = entry.model;
            candidate.modeName     = entry.modeName;
            candidate.channelIndex = ci;

            int headIdx = -1;
            for (int hi = 0; hi < heads.size(); ++hi)
            {
                if (heads[hi].channels().contains(quint32(ci)))
                {
                    headIdx = hi;
                    break;
                }
            }

            QString prefix;
            if (!heads.isEmpty())
                prefix = (headIdx >= 0) ? QString("[H%1] ").arg(headIdx) : tr("[shared] ");

            QLCChannel* ch = fxMode->channel(ci);
            const QString label = prefix + (ch
                    ? QString("%1: %2").arg(ci + 1).arg(ch->name())
                    : tr("Ch %1").arg(ci + 1));

            QListWidgetItem* item = new QListWidgetItem(label, chanList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setData(Qt::UserRole, ci);
            item->setData(Qt::UserRole + 1, entry.manufacturer);
            item->setData(Qt::UserRole + 2, entry.model);
            item->setData(Qt::UserRole + 3, entry.modeName);

            if (bindingsContain(candidate))
            {
                item->setCheckState(Qt::Checked);
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            }
            else
            {
                item->setCheckState(Qt::Unchecked);
            }
        }

        bodyLayout->addWidget(chanList);
    }

    bodyLayout->addStretch();
    scroll->setWidget(scrollBody);
    pl->addWidget(scroll, 1);

    QHBoxLayout* selRow = new QHBoxLayout;
    QPushButton* selAll = new QPushButton(tr("Select all"), &picker);
    QPushButton* selNone = new QPushButton(tr("Clear"), &picker);
    selRow->addWidget(selAll);
    selRow->addWidget(selNone);
    selRow->addStretch();
    pl->addLayout(selRow);

    connect(selAll, &QPushButton::clicked, &picker, [&channelLists]() {
        for (QListWidget* list : channelLists)
        {
            for (int i = 0; i < list->count(); ++i)
            {
                QListWidgetItem* item = list->item(i);
                if (item->flags() & Qt::ItemIsEnabled)
                    item->setCheckState(Qt::Checked);
            }
        }
    });
    connect(selNone, &QPushButton::clicked, &picker, [&channelLists]() {
        for (QListWidget* list : channelLists)
        {
            for (int i = 0; i < list->count(); ++i)
            {
                QListWidgetItem* item = list->item(i);
                if (item->flags() & Qt::ItemIsEnabled)
                    item->setCheckState(Qt::Unchecked);
            }
        }
    });

    QDialogButtonBox* pb = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &picker);
    pl->addWidget(pb);
    connect(pb, &QDialogButtonBox::accepted, &picker, &QDialog::accept);
    connect(pb, &QDialogButtonBox::rejected, &picker, &QDialog::reject);

    if (picker.exec() != QDialog::Accepted)
        return;

    bool addedAny = false;
    for (QListWidget* list : channelLists)
    {
        for (int i = 0; i < list->count(); ++i)
        {
            QListWidgetItem* item = list->item(i);
            if (item->checkState() != Qt::Checked)
                continue;
            if (!(item->flags() & Qt::ItemIsEnabled))
                continue;

            PTColumnTypeBinding binding;
            binding.manufacturer = item->data(Qt::UserRole + 1).toString();
            binding.model        = item->data(Qt::UserRole + 2).toString();
            binding.modeName     = item->data(Qt::UserRole + 3).toString();
            binding.channelIndex = item->data(Qt::UserRole).toInt();
            if (!binding.isValid() || bindingsContain(binding))
                continue;

            m_bindings.append(binding);
            addedAny = true;
        }
    }

    if (addedAny)
    {
        rebuildBindTable();
        if (m_bindings.size() == 1 && m_nameEdit)
        {
            const PTColumnTypeBinding& first = m_bindings.first();
            QLCFixtureMode* fxMode = representativeMode(first.manufacturer,
                                                        first.model,
                                                        first.modeName);
            if (fxMode)
            {
                const QLCChannel* ch = fxMode->channel(quint32(first.channelIndex));
                if (ch)
                    m_nameEdit->setText(ch->name());
            }
        }
        autoImportFromBinding(/*onlyIfEmpty=*/true);
    }
}

void PresetTableV2ColumnDialog::slotRemoveBindings()
{
    if (!m_bindTable)
        return;

    QSet<int> removeIndices;
    for (QTableWidgetItem* item : m_bindTable->selectedItems())
    {
        if (item->column() != 0)
            continue;
        removeIndices.insert(item->data(Qt::UserRole).toInt());
    }

    QVector<PTColumnTypeBinding> kept;
    kept.reserve(m_bindings.size());
    for (int i = 0; i < m_bindings.size(); ++i)
    {
        if (removeIndices.contains(i))
            continue;
        kept.append(m_bindings[i]);
    }
    m_bindings = kept;
    rebuildBindTable();
}

void PresetTableV2ColumnDialog::slotAddOption()
{
    int r = m_optTable->rowCount();
    m_optTable->insertRow(r);
    auto* nameItem = new QTableWidgetItem(tr("Option %1").arg(r + 1));
    nameItem->setData(Qt::UserRole, QString());
    m_optTable->setItem(r, 0, nameItem);
    auto* valItem = new QTableWidgetItem(QString::number(0));
    valItem->setData(Qt::EditRole, 0);
    m_optTable->setItem(r, 1, valItem);
    m_optTable->scrollToBottom();
    m_optTable->editItem(m_optTable->item(r, 0));
}

void PresetTableV2ColumnDialog::slotRemoveOption()
{
    int row = m_optTable->currentRow();
    if (row >= 0)
        m_optTable->removeRow(row);
}

void PresetTableV2ColumnDialog::slotImportFromChannel()
{
    if (!m_doc)
        return;

    const QList<Fixture*>& fixtures = m_doc->fixtures();
    if (fixtures.isEmpty())
        return;

    // ---- Mini-dialog: pick fixture + channel via tree ---------------
    QDialog picker(this);
    picker.setWindowTitle(tr("Import from fixture channel"));
    picker.resize(420, 420);
    QVBoxLayout* pl = new QVBoxLayout(&picker);

    QTreeWidget* tree = new QTreeWidget(&picker);
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList() << tr("Name") << tr("Type"));
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setRootIsDecorated(true);
    pl->addWidget(tree);

    // Populate — skip hidden fixtures
    for (Fixture* fxi : m_doc->fixtures())
    {
        if (fxi->isHidden()) continue;

        QTreeWidgetItem* fxiItem = new QTreeWidgetItem(tree);
        fxiItem->setText(0, fxi->name());
        fxiItem->setText(1, fxi->typeString());
        fxiItem->setIcon(0, fxi->getIconFromType());
        fxiItem->setData(0, Qt::UserRole,     fxi->id());
        fxiItem->setData(0, Qt::UserRole + 1, -1);  // fixture node marker

        for (quint32 c = 0; c < fxi->channels(); ++c)
        {
            const QLCChannel* ch = fxi->channel(c);
            if (!ch) continue;
            QTreeWidgetItem* chItem = new QTreeWidgetItem(fxiItem);
            chItem->setText(0, QString("%1:%2").arg(c + 1).arg(ch->name()));
            chItem->setIcon(0, ch->getIcon());
            if (ch->group() == QLCChannel::Intensity && ch->colour() != QLCChannel::NoColour)
                chItem->setText(1, QLCChannel::colourToString(ch->colour()));
            else
                chItem->setText(1, QLCChannel::groupToString(ch->group()));
            chItem->setData(0, Qt::UserRole,     fxi->id());
            chItem->setData(0, Qt::UserRole + 1, int(c));
        }
    }
    tree->header()->resizeSections(QHeaderView::ResizeToContents);

    QDialogButtonBox* pb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &picker);
    pb->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(tree, &QTreeWidget::itemSelectionChanged, [&]() {
        auto sel = tree->selectedItems();
        bool valid = !sel.isEmpty() && sel.first()->data(0, Qt::UserRole + 1).toInt() >= 0;
        pb->button(QDialogButtonBox::Ok)->setEnabled(valid);
    });
    pl->addWidget(pb);
    connect(pb, &QDialogButtonBox::accepted, &picker, &QDialog::accept);
    connect(pb, &QDialogButtonBox::rejected, &picker, &QDialog::reject);

    if (picker.exec() != QDialog::Accepted)
        return;

    auto selItems = tree->selectedItems();
    if (selItems.isEmpty()) return;
    QTreeWidgetItem* selItem = selItems.first();
    int chanIdx = selItem->data(0, Qt::UserRole + 1).toInt();
    if (chanIdx < 0) return;  // fixture node, not a channel
    quint32 fxiId = selItem->data(0, Qt::UserRole).toUInt();

    Fixture* fxi = m_doc->fixture(fxiId);
    if (!fxi) return;
    const QLCChannel* chan = fxi->channel(quint32(chanIdx));
    if (!chan) return;

    // ---- Populate options from capabilities -------------------------
    m_optTable->setRowCount(0);
    for (QLCCapability* cap : chan->capabilities())
    {
        int r = m_optTable->rowCount();
        m_optTable->insertRow(r);

        QString resource;
        if (cap->presetType() == QLCCapability::Picture)
            resource = cap->resource(0).toString();
        else if (cap->presetType() == QLCCapability::SingleColor)
            resource = cap->resource(0).value<QColor>().name();   // "#rrggbb"

        auto* nameItem = new QTableWidgetItem(cap->name());
        nameItem->setData(Qt::UserRole, resource);
        if (!resource.isEmpty())
            nameItem->setIcon(makeResourceIcon(resource));
        m_optTable->setItem(r, 0, nameItem);

        auto* valItem = new QTableWidgetItem(QString::number(cap->min()));
        valItem->setData(Qt::EditRole, int(cap->min()));
        m_optTable->setItem(r, 1, valItem);
    }

    m_rbDropdown->setChecked(true);
}

void PresetTableV2ColumnDialog::updateOptionsEnabled()
{
    bool dropdown = m_rbDropdown->isChecked();
    bool scaler   = m_rbScaler   && m_rbScaler->isChecked();
    m_optTable->setEnabled(dropdown);
    m_addOptBtn->setEnabled(dropdown);
    m_remOptBtn->setEnabled(dropdown);
    m_importBtn->setEnabled(dropdown && m_doc != nullptr);
    if (m_scalerGrp)
        m_scalerGrp->setVisible(scaler);
}

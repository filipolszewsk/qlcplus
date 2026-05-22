/*
  QLC+ VC Widget Plugin — Preset Table
  presettablecolumndialog.cpp — Apache 2.0 / public domain
*/

#include "presettablecolumndialog.h"

#include "doc.h"
#include "fixture.h"
#include "fixturegroup.h"
#include "grouphead.h"
#include "qlcchannel.h"
#include "qlccapability.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QColor>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QIcon PresetTableColumnDialog::makeResourceIcon(const QString& resource)
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

PresetTableColumnDialog::PresetTableColumnDialog(Doc* doc,
                                                   const PTColumn& column,
                                                   PTMode mode,
                                                   FixtureGroup* group,
                                                   QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_mode(mode)
    , m_group(group)
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
    QFormLayout* bindForm = new QFormLayout(m_bindGrp);

    m_fxTypeLabel  = new QLabel(tr("Fixture type:"), m_bindGrp);
    m_fxTypeCombo  = new QComboBox(m_bindGrp);
    m_fxTypeCombo->setMinimumWidth(240);
    bindForm->addRow(m_fxTypeLabel, m_fxTypeCombo);

    m_channelLabel = new QLabel(tr("Channel:"), m_bindGrp);
    m_channelCombo = new QComboBox(m_bindGrp);
    m_channelCombo->setMinimumWidth(240);
    bindForm->addRow(m_channelLabel, m_channelCombo);

    root->addWidget(m_bindGrp);

    // ---- Type -------------------------------------------------------
    QGroupBox* typeGrp = new QGroupBox(tr("Value type"), this);
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGrp);
    m_rbNumeric  = new QRadioButton(tr("Numeric  (0 - 255 spinbox)"), typeGrp);
    m_rbDropdown = new QRadioButton(tr("Dropdown (named options mapped to values)"), typeGrp);
    typeLayout->addWidget(m_rbNumeric);
    typeLayout->addWidget(m_rbDropdown);
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

    // Populate fixture binding combos (FG mode)
    if (mode == PTMode::FixtureGroup && group)
    {
        populateFixtureTypeCombo();

        // Pre-select current binding if valid
        if (column.binding.isValid())
        {
            for (int i = 0; i < m_fxTypeEntries.size(); ++i)
            {
                const FxTypeEntry& e = m_fxTypeEntries[i];
                if (e.manufacturer == column.binding.manufacturer &&
                    e.model        == column.binding.model        &&
                    e.modeName     == column.binding.modeName)
                {
                    m_fxTypeCombo->setCurrentIndex(i);
                    populateChannelCombo(i);

                    // Select the matching channel
                    for (int ci = 0; ci < m_channelCombo->count(); ++ci)
                    {
                        if (m_channelCombo->itemData(ci).toInt() == column.binding.channelIndex)
                        {
                            m_channelCombo->setCurrentIndex(ci);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        else if (m_fxTypeCombo->count() > 0)
        {
            populateChannelCombo(0);
        }
    }

    updateOptionsEnabled();

    // ---- Connections ------------------------------------------------
    connect(m_rbNumeric,  &QRadioButton::toggled, this, &PresetTableColumnDialog::slotTypeChanged);
    connect(m_rbDropdown, &QRadioButton::toggled, this, &PresetTableColumnDialog::slotTypeChanged);
    connect(m_addOptBtn,  &QPushButton::clicked,  this, &PresetTableColumnDialog::slotAddOption);
    connect(m_remOptBtn,  &QPushButton::clicked,  this, &PresetTableColumnDialog::slotRemoveOption);
    connect(m_importBtn,  &QPushButton::clicked,  this, &PresetTableColumnDialog::slotImportFromChannel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_fxTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresetTableColumnDialog::slotFixtureTypeChanged);
}

// ---------------------------------------------------------------------------
// Fixture binding population
// ---------------------------------------------------------------------------

void PresetTableColumnDialog::populateFixtureTypeCombo()
{
    m_fxTypeCombo->clear();
    m_fxTypeEntries.clear();

    if (!m_group || !m_doc) return;

    // Build a deduplicated list of (manufacturer, model, modeName) in the group
    for (quint32 fxiId : m_group->fixtureList())
    {
        Fixture* fxi = m_doc->fixture(fxiId);
        if (!fxi) continue;

        QLCFixtureDef*  def  = fxi->fixtureDef();
        QLCFixtureMode* mode = fxi->fixtureMode();
        if (!def || !mode) continue;

        QString mfg    = def->manufacturer();
        QString model  = def->model();
        QString mName  = mode->name();

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

    for (const FxTypeEntry& e : m_fxTypeEntries)
    {
        QString label = QString("%1 %2 — %3 (%4×)")
            .arg(e.manufacturer, e.model, e.modeName)
            .arg(e.count);
        m_fxTypeCombo->addItem(label);
    }
}

void PresetTableColumnDialog::populateChannelCombo(int fixtureTypeIndex)
{
    m_channelCombo->clear();

    if (fixtureTypeIndex < 0 || fixtureTypeIndex >= m_fxTypeEntries.size()) return;
    if (!m_group || !m_doc) return;

    const FxTypeEntry& entry = m_fxTypeEntries[fixtureTypeIndex];

    // Find a representative fixture of this type in the group
    QLCFixtureMode* fxMode = nullptr;
    for (quint32 fxiId : m_group->fixtureList())
    {
        Fixture* fxi = m_doc->fixture(fxiId);
        if (!fxi) continue;
        QLCFixtureDef*  def  = fxi->fixtureDef();
        QLCFixtureMode* mode = fxi->fixtureMode();
        if (!def || !mode) continue;
        if (def->manufacturer() == entry.manufacturer &&
            def->model()        == entry.model        &&
            mode->name()        == entry.modeName)
        {
            fxMode = mode;
            break;
        }
    }

    if (!fxMode) return;

    // Populate combo with channel names
    int nChannels = fxMode->channels().size();

    // For multi-head fixtures, show only channels of one head
    int nHeads = fxMode->heads().size();
    if (nHeads > 1)
        nChannels = nChannels / nHeads;

    for (int ci = 0; ci < nChannels; ++ci)
    {
        QLCChannel* ch = fxMode->channel(ci);
        QString label = ch
            ? QString("%1: %2").arg(ci + 1).arg(ch->name())
            : tr("Ch %1").arg(ci + 1);
        m_channelCombo->addItem(label, ci);
    }
}

// ---------------------------------------------------------------------------
// column() getter
// ---------------------------------------------------------------------------

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
            auto* nameItem = m_optTable->item(r, 0);
            auto* valItem  = m_optTable->item(r, 1);
            opt.name     = nameItem ? nameItem->text() : QString();
            opt.value    = valItem  ? uchar(valItem->data(Qt::EditRole).toInt()) : 0;
            opt.resource = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
            if (!opt.name.isEmpty())
                col.options.append(opt);
        }
    }

    // Read binding (FG mode)
    if (m_mode == PTMode::FixtureGroup && m_bindGrp && m_bindGrp->isVisible())
    {
        int typeIdx = m_fxTypeCombo ? m_fxTypeCombo->currentIndex() : -1;
        int chanIdx = (m_channelCombo && m_channelCombo->currentIndex() >= 0)
            ? m_channelCombo->currentData().toInt() : -1;

        if (typeIdx >= 0 && typeIdx < m_fxTypeEntries.size() && chanIdx >= 0)
        {
            const FxTypeEntry& e = m_fxTypeEntries[typeIdx];
            col.binding.manufacturer = e.manufacturer;
            col.binding.model        = e.model;
            col.binding.modeName     = e.modeName;
            col.binding.channelIndex = chanIdx;
        }
    }

    return col;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PresetTableColumnDialog::slotTypeChanged()
{
    updateOptionsEnabled();
}

void PresetTableColumnDialog::slotFixtureTypeChanged(int index)
{
    populateChannelCombo(index);
}

void PresetTableColumnDialog::slotAddOption()
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

void PresetTableColumnDialog::slotRemoveOption()
{
    int row = m_optTable->currentRow();
    if (row >= 0)
        m_optTable->removeRow(row);
}

void PresetTableColumnDialog::slotImportFromChannel()
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

void PresetTableColumnDialog::updateOptionsEnabled()
{
    bool dropdown = m_rbDropdown->isChecked();
    m_optTable->setEnabled(dropdown);
    m_addOptBtn->setEnabled(dropdown);
    m_remOptBtn->setEnabled(dropdown);
    m_importBtn->setEnabled(dropdown && m_doc != nullptr);
}

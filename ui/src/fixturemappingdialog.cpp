/*
  Q Light Controller Plus
  fixturemappingdialog.cpp

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

#include <QDialogButtonBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>

#include "fixturemappingdialog.h"

#include "qlcfixturedefcache.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "fixturegroup.h"
#include "fixture.h"
#include "doc.h"

#define COL_NAME      0
#define COL_TYPE      1
#define COL_SRC_ADDR  2
#define COL_ACTION    3
#define COL_TARGET    4

FixtureMappingDialog::FixtureMappingDialog(QWidget *parent, Doc *doc, Doc *importDoc,
                                           const QList<quint32> &fixtureIDList,
                                           const QList<quint32> &fixtureGroupIDList)
    : QDialog(parent)
    , m_doc(doc)
    , m_importDoc(importDoc)
    , m_fixtureIDList(fixtureIDList)
    , m_fixtureGroupIDList(fixtureGroupIDList)
{
    setWindowTitle(tr("Fixture Mapping"));
    setMinimumSize(900, 400);
    resize(1000, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *infoLabel = new QLabel(
        tr("The selected functions require the fixtures listed below. "
           "Choose how each fixture should be handled:"), this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    /* Table */
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(QStringList()
        << tr("Imported Fixture")
        << tr("Type")
        << tr("Source Addr")
        << tr("Action")
        << tr("Target"));
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(COL_TYPE, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_SRC_ADDR, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_ACTION, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_TARGET, QHeaderView::Interactive);
    m_table->horizontalHeader()->resizeSection(COL_TARGET, 250);
    m_table->horizontalHeader()->setMinimumSectionSize(80);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table, 1);

    /* Button row */
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_autoNameButton = new QPushButton(tr("Auto-map by Name"), this);
    m_autoAddrButton = new QPushButton(tr("Auto-map by Address"), this);
    m_statusLabel = new QLabel(this);
    btnLayout->addWidget(m_autoNameButton);
    btnLayout->addWidget(m_autoAddrButton);
    btnLayout->addStretch();
    btnLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(btnLayout);

    /* Dialog buttons */
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(m_autoNameButton, SIGNAL(clicked()), this, SLOT(slotAutoMapByName()));
    connect(m_autoAddrButton, SIGNAL(clicked()), this, SLOT(slotAutoMapByAddress()));
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        applyMapping();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    populateTable();
    /* Auto-map by name as initial default */
    slotAutoMapByName();
    m_table->resizeRowsToContents();
}

FixtureMappingDialog::~FixtureMappingDialog()
{
}

QMap<quint32, quint32> FixtureMappingDialog::fixtureIDRemap() const
{
    return m_fixtureIDRemap;
}

QMap<quint32, quint32> FixtureMappingDialog::fixtureGroupIDRemap() const
{
    return m_fixtureGroupIDRemap;
}

/*****************************************************************************
 * Table population
 *****************************************************************************/

QString FixtureMappingDialog::fixtureDisplayString(Fixture *fxi) const
{
    if (fxi == nullptr)
        return QString();
    return QString("%1 (%2/%3)")
        .arg(fxi->name())
        .arg(fxi->universe() + 1)
        .arg(QString::number(fxi->address() + 1).rightJustified(3, '0'));
}

QList<Fixture *> FixtureMappingDialog::findCompatibleFixtures(Fixture *importFixture)
{
    QList<Fixture *> result;
    if (importFixture == nullptr)
        return result;

    QLCFixtureDef *importDef = importFixture->fixtureDef();
    QLCFixtureMode *importMode = importFixture->fixtureMode();

    for (Fixture *docFxi : m_doc->fixtures())
    {
        /* Show all fixtures, but put same-type ones first */
        result.append(docFxi);
    }

    /* Sort: same manufacturer+model+mode first, then by name */
    std::sort(result.begin(), result.end(),
        [importDef, importMode](Fixture *a, Fixture *b) -> bool
        {
            bool aMatch = (a->fixtureDef() != nullptr && importDef != nullptr &&
                           a->fixtureDef()->manufacturer() == importDef->manufacturer() &&
                           a->fixtureDef()->model() == importDef->model());
            bool bMatch = (b->fixtureDef() != nullptr && importDef != nullptr &&
                           b->fixtureDef()->manufacturer() == importDef->manufacturer() &&
                           b->fixtureDef()->model() == importDef->model());
            if (aMatch != bMatch)
                return aMatch; /* matching type comes first */
            return a->name() < b->name();
        });

    return result;
}

void FixtureMappingDialog::populateTable()
{
    m_entries.clear();

    /* Sort fixture IDs for consistent ordering */
    QList<quint32> sortedList = m_fixtureIDList;
    std::sort(sortedList.begin(), sortedList.end());

    for (quint32 importID : sortedList)
    {
        Fixture *importFxi = m_importDoc->fixture(importID);
        if (importFxi == nullptr)
            continue;

        FixtureMappingEntry entry;
        entry.importID = importID;
        entry.name = importFxi->name();
        entry.channels = importFxi->channels();
        entry.srcUniverse = importFxi->universe();
        entry.srcAddress = importFxi->address();
        entry.action = FixtureMappingEntry::CreateNew;
        entry.targetFixtureID = Fixture::invalidId();
        entry.targetUniverse = importFxi->universe();
        entry.targetAddress = importFxi->address();

        QLCFixtureDef *def = importFxi->fixtureDef();
        if (def != nullptr)
        {
            entry.manufacturer = def->manufacturer();
            entry.model = def->model();
        }
        QLCFixtureMode *mode = importFxi->fixtureMode();
        if (mode != nullptr)
            entry.modeName = mode->name();

        m_entries.append(entry);
    }

    m_table->setRowCount(m_entries.count());

    for (int row = 0; row < m_entries.count(); row++)
    {
        const FixtureMappingEntry &entry = m_entries.at(row);

        /* Col 0: Name */
        QTableWidgetItem *nameItem = new QTableWidgetItem(entry.name);
        nameItem->setIcon(QIcon(":/fixture.png"));
        m_table->setItem(row, COL_NAME, nameItem);

        /* Col 1: Type (manufacturer + model) */
        QString typeStr = entry.manufacturer;
        if (!entry.model.isEmpty())
        {
            if (!typeStr.isEmpty()) typeStr += " ";
            typeStr += entry.model;
        }
        if (!entry.modeName.isEmpty())
            typeStr += " [" + entry.modeName + "]";
        m_table->setItem(row, COL_TYPE, new QTableWidgetItem(typeStr));

        /* Col 2: Source Address */
        QString addrStr = QString("%1/%2")
            .arg(entry.srcUniverse + 1)
            .arg(QString::number(entry.srcAddress + 1).rightJustified(3, '0'));
        m_table->setItem(row, COL_SRC_ADDR, new QTableWidgetItem(addrStr));

        /* Col 3: Action combo */
        QComboBox *actionCombo = new QComboBox();
        actionCombo->addItem(tr("Map to existing"), FixtureMappingEntry::MapToExisting);
        actionCombo->addItem(tr("Create new"), FixtureMappingEntry::CreateNew);
        actionCombo->addItem(tr("Skip"), FixtureMappingEntry::Skip);
        actionCombo->setCurrentIndex(1); /* Default: Create new */
        m_table->setCellWidget(row, COL_ACTION, actionCombo);

        connect(actionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, row](int) { slotActionChanged(row); });

        /* Col 4: Target -- will be set by slotAutoMapByName called after populateTable */
    }

    updateStatusLabel();
}

/*****************************************************************************
 * Target widget management
 *****************************************************************************/

void FixtureMappingDialog::updateTargetWidget(int row)
{
    if (row < 0 || row >= m_entries.count())
        return;

    FixtureMappingEntry &entry = m_entries[row];

    QComboBox *actionCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, COL_ACTION));
    if (actionCombo == nullptr)
        return;

    FixtureMappingEntry::Action action =
        static_cast<FixtureMappingEntry::Action>(actionCombo->currentData().toInt());
    entry.action = action;

    /* Remove old target widget */
    m_table->removeCellWidget(row, COL_TARGET);
    m_table->setItem(row, COL_TARGET, nullptr);

    switch (action)
    {
        case FixtureMappingEntry::MapToExisting:
        {
            QComboBox *targetCombo = new QComboBox();
            Fixture *importFxi = m_importDoc->fixture(entry.importID);
            QList<Fixture *> compatibles = findCompatibleFixtures(importFxi);

            int bestIndex = -1;
            for (int i = 0; i < compatibles.count(); i++)
            {
                Fixture *fxi = compatibles.at(i);
                targetCombo->addItem(fixtureDisplayString(fxi), fxi->id());

                /* Pre-select by name match */
                if (bestIndex < 0 && fxi->name() == entry.name)
                    bestIndex = i;
            }

            if (compatibles.isEmpty())
            {
                targetCombo->addItem(tr("(no fixtures in project)"));
                targetCombo->setEnabled(false);
            }
            else
            {
                if (bestIndex >= 0)
                    targetCombo->setCurrentIndex(bestIndex);

                connect(targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                        this, [this, row](int) {
                    if (row < m_entries.count())
                    {
                        QComboBox *combo = qobject_cast<QComboBox *>(m_table->cellWidget(row, COL_TARGET));
                        if (combo)
                            m_entries[row].targetFixtureID = combo->currentData().toUInt();
                    }
                });

                entry.targetFixtureID = targetCombo->currentData().toUInt();
            }

            m_table->setCellWidget(row, COL_TARGET, targetCombo);
        }
        break;

        case FixtureMappingEntry::CreateNew:
        {
            QWidget *container = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(container);
            layout->setContentsMargins(2, 0, 2, 0);
            layout->setSpacing(4);

            QLabel *uniLabel = new QLabel(tr("Uni:"));
            QSpinBox *uniSpin = new QSpinBox();
            uniSpin->setMinimum(1);
            uniSpin->setMaximum(64);
            uniSpin->setValue(entry.targetUniverse + 1);

            QLabel *addrLabel = new QLabel(tr("Addr:"));
            QSpinBox *addrSpin = new QSpinBox();
            addrSpin->setMinimum(1);
            addrSpin->setMaximum(512);
            addrSpin->setValue(entry.targetAddress + 1);

            layout->addWidget(uniLabel);
            layout->addWidget(uniSpin);
            layout->addWidget(addrLabel);
            layout->addWidget(addrSpin);
            layout->addStretch();

            connect(uniSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this, row](int val) {
                if (row < m_entries.count())
                    m_entries[row].targetUniverse = val - 1;
            });

            connect(addrSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this, row](int val) {
                if (row < m_entries.count())
                    m_entries[row].targetAddress = val - 1;
            });

            m_table->setCellWidget(row, COL_TARGET, container);
        }
        break;

        case FixtureMappingEntry::Skip:
        {
            QTableWidgetItem *skipItem = new QTableWidgetItem(tr("-- skipped --"));
            skipItem->setForeground(Qt::gray);
            m_table->setItem(row, COL_TARGET, skipItem);
        }
        break;
    }
}

/*****************************************************************************
 * Auto-map helpers
 *****************************************************************************/

void FixtureMappingDialog::slotAutoMapByName()
{
    for (int row = 0; row < m_entries.count(); row++)
    {
        FixtureMappingEntry &entry = m_entries[row];
        bool found = false;

        for (Fixture *docFxi : m_doc->fixtures())
        {
            if (docFxi->name() == entry.name)
            {
                entry.action = FixtureMappingEntry::MapToExisting;
                entry.targetFixtureID = docFxi->id();
                found = true;
                break;
            }
        }

        if (!found)
        {
            entry.action = FixtureMappingEntry::CreateNew;
            entry.targetUniverse = entry.srcUniverse;
            entry.targetAddress = entry.srcAddress;
        }

        /* Update the Action combo without triggering slotActionChanged */
        QComboBox *actionCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, COL_ACTION));
        if (actionCombo)
        {
            actionCombo->blockSignals(true);
            int idx = actionCombo->findData(entry.action);
            if (idx >= 0)
                actionCombo->setCurrentIndex(idx);
            actionCombo->blockSignals(false);
        }

        updateTargetWidget(row);
    }

    updateStatusLabel();
}

void FixtureMappingDialog::slotAutoMapByAddress()
{
    for (int row = 0; row < m_entries.count(); row++)
    {
        FixtureMappingEntry &entry = m_entries[row];
        bool found = false;

        for (Fixture *docFxi : m_doc->fixtures())
        {
            if (docFxi->universe() == entry.srcUniverse &&
                docFxi->address() == entry.srcAddress)
            {
                entry.action = FixtureMappingEntry::MapToExisting;
                entry.targetFixtureID = docFxi->id();
                found = true;
                break;
            }
        }

        if (!found)
        {
            entry.action = FixtureMappingEntry::CreateNew;
            entry.targetUniverse = entry.srcUniverse;
            entry.targetAddress = entry.srcAddress;
        }

        /* Update the Action combo without triggering slotActionChanged */
        QComboBox *actionCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, COL_ACTION));
        if (actionCombo)
        {
            actionCombo->blockSignals(true);
            int idx = actionCombo->findData(entry.action);
            if (idx >= 0)
                actionCombo->setCurrentIndex(idx);
            actionCombo->blockSignals(false);
        }

        updateTargetWidget(row);
    }

    updateStatusLabel();
}

/*****************************************************************************
 * Slots
 *****************************************************************************/

void FixtureMappingDialog::slotActionChanged(int row)
{
    if (row < 0 || row >= m_entries.count())
        return;

    updateTargetWidget(row);
    updateStatusLabel();
}

/*****************************************************************************
 * Status
 *****************************************************************************/

void FixtureMappingDialog::updateStatusLabel()
{
    int mapped = 0, created = 0, skipped = 0;

    for (const FixtureMappingEntry &entry : m_entries)
    {
        switch (entry.action)
        {
            case FixtureMappingEntry::MapToExisting: mapped++; break;
            case FixtureMappingEntry::CreateNew: created++; break;
            case FixtureMappingEntry::Skip: skipped++; break;
        }
    }

    m_statusLabel->setText(tr("%1 mapped, %2 new, %3 skipped")
                           .arg(mapped).arg(created).arg(skipped));
}

/*****************************************************************************
 * Address helper
 *****************************************************************************/

void FixtureMappingDialog::getAvailableFixtureAddress(int channels, int &universe, int &address)
{
    int freeCounter = 0;
    quint32 absAddress = (universe << 9) + address;

    while (1)
    {
        if (m_doc->fixtureForAddress(absAddress) == Fixture::invalidId())
            freeCounter++;
        else
            freeCounter = 0;

        if (freeCounter == channels)
        {
            universe = (absAddress >> 9);
            address = absAddress - (universe * 512) - (channels - 1);
            return;
        }

        absAddress++;
    }
}

/*****************************************************************************
 * Apply mapping -- create fixtures and build remap maps
 *****************************************************************************/

void FixtureMappingDialog::applyMapping()
{
    m_fixtureIDRemap.clear();
    m_fixtureGroupIDRemap.clear();

    /* ======================== Fixtures ======================== */
    for (const FixtureMappingEntry &entry : m_entries)
    {
        switch (entry.action)
        {
            case FixtureMappingEntry::MapToExisting:
            {
                m_fixtureIDRemap[entry.importID] = entry.targetFixtureID;
                qDebug() << "Mapped fixture" << entry.name
                         << "importID" << entry.importID
                         << "->" << entry.targetFixtureID;
            }
            break;

            case FixtureMappingEntry::CreateNew:
            {
                Fixture *importFxi = m_importDoc->fixture(entry.importID);
                if (importFxi == nullptr)
                    continue;

                QLCFixtureDef *importDef = importFxi->fixtureDef();
                QLCFixtureMode *importMode = importFxi->fixtureMode();
                QLCFixtureDef *fxiDef = m_doc->fixtureDefCache()->fixtureDef(
                    importDef->manufacturer(), importDef->model());
                QLCFixtureMode *fxiMode = nullptr;

                if (fxiDef != nullptr && importMode != nullptr)
                    fxiMode = fxiDef->mode(importMode->name());

                Fixture *fxi = new Fixture(m_doc);
                fxi->setName(entry.name);
                fxi->setUniverse(entry.targetUniverse);
                fxi->setAddress(entry.targetAddress);

                if (fxiDef == nullptr && fxiMode == nullptr)
                {
                    if (importDef->model() == "Generic")
                    {
                        fxiDef = fxi->genericDimmerDef(entry.channels);
                        fxiMode = fxi->genericDimmerMode(fxiDef, entry.channels);
                    }
                    else
                    {
                        qWarning() << "Cannot find fixture definition for"
                                   << importDef->manufacturer() << importDef->model();
                        delete fxi;
                        continue;
                    }
                }

                fxi->setFixtureDefinition(fxiDef, fxiMode);

                if (m_doc->addFixture(fxi))
                {
                    m_fixtureIDRemap[entry.importID] = fxi->id();
                    qDebug() << "Created fixture" << fxi->name()
                             << "ID:" << fxi->id()
                             << "at" << (entry.targetUniverse + 1)
                             << "/" << (entry.targetAddress + 1);
                }
                else
                {
                    qWarning() << "Failed to add fixture" << entry.name;
                    delete fxi;
                }
            }
            break;

            case FixtureMappingEntry::Skip:
                /* No remap entry -- references to this fixture will be dropped */
                qDebug() << "Skipped fixture" << entry.name;
            break;
        }
    }

    /* =================== Fixture Groups =================== */
    for (quint32 groupID : m_fixtureGroupIDList)
    {
        FixtureGroup *importGroup = m_importDoc->fixtureGroup(groupID);
        if (importGroup == nullptr)
            continue;

        bool matchFound = false;

        for (FixtureGroup *docGroup : m_doc->fixtureGroups())
        {
            if (docGroup->name() == importGroup->name())
            {
                m_fixtureGroupIDRemap[groupID] = docGroup->id();
                matchFound = true;
                break;
            }
        }

        if (!matchFound)
        {
            FixtureGroup *newGroup = new FixtureGroup(m_doc);
            newGroup->setName(importGroup->name());
            newGroup->setSize(importGroup->size());

            QMap<QLCPoint, GroupHead> headsMap = importGroup->headsMap();
            QMap<QLCPoint, GroupHead>::const_iterator it = headsMap.constBegin();
            while (it != headsMap.constEnd())
            {
                QLCPoint p = it.key();
                GroupHead head = it.value();

                if (m_fixtureIDRemap.contains(head.fxi))
                {
                    head.fxi = m_fixtureIDRemap[head.fxi];
                    newGroup->assignHead(p, head);
                }

                ++it;
            }

            if (m_doc->addFixtureGroup(newGroup))
            {
                m_fixtureGroupIDRemap[groupID] = newGroup->id();
            }
            else
            {
                qWarning() << "Failed to add fixture group" << newGroup->name();
                delete newGroup;
            }
        }
    }
}

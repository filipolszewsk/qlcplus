/*
  QLC+ VC Widget Plugin — Preset Table
  presettableconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "presettableconfigdialog.h"
#include "presettablecolumndialog.h"

#include "inputselectionwidget.h"
#include "fixtureselection.h"
#include "fixturegroup.h"
#include "qlcpoint.h"
#include "grouphead.h"
#include "doc.h"
#include "fixture.h"
#include "qlcchannel.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "qlcfixturehead.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QHeaderView>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <algorithm>

// ==========================================================================
// OutputEditorRow (Legacy mode)
// ==========================================================================

OutputEditorRow::OutputEditorRow(Doc* doc,
                                  const PTOutput& output,
                                  QSharedPointer<QLCInputSource> src,
                                  int requiredChannels,
                                  int widgetPage,
                                  QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_requiredChannels(requiredChannels)
    , m_fixtureId(output.fixtureId)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(4, 2, 4, 2);
    lay->setSpacing(6);

    // Name edit
    m_nameEdit = new QLineEdit(output.name, this);
    m_nameEdit->setPlaceholderText(tr("Output name"));
    m_nameEdit->setMinimumWidth(80);
    m_nameEdit->setMaximumWidth(120);
    lay->addWidget(m_nameEdit);

    // Fixture label + choose button
    m_fixtureLabel = new QLabel(this);
    m_fixtureLabel->setMinimumWidth(160);
    lay->addWidget(m_fixtureLabel, 1);

    m_chooseFixBtn = new QPushButton(tr("Choose fixture..."), this);
    m_chooseFixBtn->setMaximumWidth(130);
    lay->addWidget(m_chooseFixBtn);

    // Row selector input
    m_inputSel = new InputSelectionWidget(doc, this);
    m_inputSel->setKeyInputVisibility(false);
    m_inputSel->setWidgetPage(widgetPage);
    m_inputSel->setInputSource(src);
    lay->addWidget(m_inputSel);

    updateFixtureLabel();

    connect(m_chooseFixBtn, &QPushButton::clicked, this, &OutputEditorRow::slotChooseFixture);
    connect(m_nameEdit, &QLineEdit::textEdited, this, &OutputEditorRow::slotNameEdited);
}

PTOutput OutputEditorRow::output() const
{
    PTOutput out;
    out.name      = m_nameEdit->text().trimmed();
    out.fixtureId = m_fixtureId;
    return out;
}

QSharedPointer<QLCInputSource> OutputEditorRow::inputSource() const
{
    return m_inputSel ? m_inputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

void OutputEditorRow::slotChooseFixture()
{
    FixtureSelection fs(this, m_doc);
    fs.setMultiSelection(false);

    // Filter to only show fixtures with the required channel count
    if (m_requiredChannels > 0)
    {
        QList<quint32> disabled;
        for (Fixture* fx : m_doc->fixtures())
            if ((int)fx->channels() != m_requiredChannels)
                disabled.append(fx->id());
        fs.setDisabledFixtures(disabled);
    }

    if (fs.exec() == QDialog::Accepted)
    {
        QList<quint32> sel = fs.selection();
        if (!sel.isEmpty())
        {
            m_fixtureId = sel.first();
            updateFixtureLabel();
            emit changed();
            emit fixtureSelected(m_fixtureId);
        }
    }
}

void OutputEditorRow::setRequiredChannels(int n)
{
    m_requiredChannels = n;
    updateFixtureLabel();
}

void OutputEditorRow::slotNameEdited(const QString&)
{
    emit changed();
}

void OutputEditorRow::updateFixtureLabel()
{
    if (m_fixtureId == UINT_MAX)
    {
        m_fixtureLabel->setText(tr("<not assigned>"));
        m_fixtureLabel->setStyleSheet(QStringLiteral("color: #888;"));
        return;
    }

    Fixture* fx = m_doc->fixture(m_fixtureId);
    if (!fx)
    {
        m_fixtureLabel->setText(tr("<missing fixture>"));
        m_fixtureLabel->setStyleSheet(QStringLiteral("color: #ff4444;"));
        return;
    }

    bool ok = (m_requiredChannels <= 0 || (int)fx->channels() == m_requiredChannels);
    QString label = QString("U%1 CH%2 — %3 (%4ch)")
        .arg(fx->universe() + 1)
        .arg(fx->address() + 1)
        .arg(fx->name())
        .arg(fx->channels());
    m_fixtureLabel->setText(label);
    m_fixtureLabel->setStyleSheet(ok ? QString() : QStringLiteral("color: #ff4444;"));
}

// ==========================================================================
// FGOutputEditorRow (FixtureGroup mode)
// ==========================================================================

FGOutputEditorRow::FGOutputEditorRow(Doc* doc,
                                       const PTOutput& output,
                                       QSharedPointer<QLCInputSource> src,
                                       FixtureGroup* group,
                                       int widgetPage,
                                       QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_group(group)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(4, 2, 4, 2);
    lay->setSpacing(6);

    // Name edit
    m_nameEdit = new QLineEdit(output.name, this);
    m_nameEdit->setPlaceholderText(tr("Output name"));
    m_nameEdit->setMinimumWidth(80);
    m_nameEdit->setMaximumWidth(120);
    lay->addWidget(m_nameEdit);

    // Checkbox container (rows)
    m_cbWidget = new QWidget(this);
    m_cbLayout = new QVBoxLayout(m_cbWidget);
    m_cbLayout->setContentsMargins(0, 0, 0, 0);
    m_cbLayout->setSpacing(1);
    m_cbWidget->setLayout(m_cbLayout);
    lay->addWidget(m_cbWidget, 1);

    // Input selector
    m_inputSel = new InputSelectionWidget(doc, this);
    m_inputSel->setKeyInputVisibility(false);
    m_inputSel->setWidgetPage(widgetPage);
    m_inputSel->setInputSource(src);
    lay->addWidget(m_inputSel);

    // Build checkboxes, then set initially-checked rows from output.groupRows
    rebuildRowCheckboxes();
    for (QCheckBox* cb : m_rowCBs)
    {
        int y = cb->property("rowY").toInt();
        if (output.groupRows.contains(y))
            cb->setChecked(true);
    }

    connect(m_nameEdit, &QLineEdit::textEdited, this, &FGOutputEditorRow::changed);
}

PTOutput FGOutputEditorRow::output() const
{
    PTOutput out;
    out.name = m_nameEdit ? m_nameEdit->text().trimmed() : QString();
    for (const QCheckBox* cb : m_rowCBs)
    {
        if (cb->isChecked())
            out.groupRows.append(cb->property("rowY").toInt());
    }
    return out;
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::inputSource() const
{
    return m_inputSel ? m_inputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

void FGOutputEditorRow::setFixtureGroup(FixtureGroup* group)
{
    m_group = group;
    // Preserve checked rows before rebuild
    QList<int> wasChecked;
    for (const QCheckBox* cb : m_rowCBs)
        if (cb->isChecked())
            wasChecked.append(cb->property("rowY").toInt());

    rebuildRowCheckboxes();

    for (QCheckBox* cb : m_rowCBs)
    {
        int y = cb->property("rowY").toInt();
        cb->setChecked(wasChecked.contains(y));
    }
}

void FGOutputEditorRow::rebuildRowCheckboxes()
{
    // Clear existing checkboxes
    for (QCheckBox* cb : m_rowCBs)
        delete cb;
    m_rowCBs.clear();

    if (!m_group) return;

    int gridH = m_group->size().height();
    if (gridH <= 0) return;

    // Count fixtures per row
    QMap<int, int> countPerRow;
    for (const GroupHead& head : m_group->headList())
    {
        // Find the y-coordinate for this head
        const auto& headsMap = m_group->headsMap();
        for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
        {
            if (it.value().fxi == head.fxi && it.value().head == head.head)
            {
                countPerRow[it.key().y()]++;
                break;
            }
        }
    }

    for (int y = 0; y < gridH; ++y)
    {
        int cnt = countPerRow.value(y, 0);
        QString label = tr("Row %1 (%2 fixture(s))").arg(y).arg(cnt);
        QCheckBox* cb = new QCheckBox(label, m_cbWidget);
        cb->setProperty("rowY", y);
        m_cbLayout->addWidget(cb);
        m_rowCBs.append(cb);
        connect(cb, &QCheckBox::toggled, this, &FGOutputEditorRow::changed);
    }
}

// ==========================================================================
// PresetTableConfigDialog
// ==========================================================================

PresetTableConfigDialog::PresetTableConfigDialog(Doc* doc,
                                                   const QVector<PTColumn>& columns,
                                                   const QVector<PTOutput>&  outputs,
                                                   const QVector<QSharedPointer<QLCInputSource>>& sources,
                                                   bool crossfadeEnabled,
                                                   QSharedPointer<QLCInputSource> crossfadeSrc,
                                                   int widgetPage,
                                                   PTMode mode,
                                                   quint32 fixtureGroupId,
                                                   QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_widgetPage(widgetPage)
    , m_columns(columns)
    , m_initSources(sources)
{
    setWindowTitle(tr("Preset Table — Properties"));
    setMinimumSize(600, 520);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ============================
    // Mode + Group selection row
    // ============================
    QWidget* modeWidget = new QWidget(this);
    QHBoxLayout* modeRow = new QHBoxLayout(modeWidget);
    modeRow->setContentsMargins(0, 0, 0, 0);

    modeRow->addWidget(new QLabel(tr("Mode:"), modeWidget));
    m_modeCombo = new QComboBox(modeWidget);
    m_modeCombo->addItem(tr("Single Fixture (Legacy)"), int(PTMode::Legacy));
    m_modeCombo->addItem(tr("Fixture Group"),           int(PTMode::FixtureGroup));
    m_modeCombo->setCurrentIndex(mode == PTMode::FixtureGroup ? 1 : 0);
    modeRow->addWidget(m_modeCombo);

    // Fixture Group selector (hidden when Legacy)
    m_groupRow = new QWidget(modeWidget);
    QHBoxLayout* groupRowLay = new QHBoxLayout(m_groupRow);
    groupRowLay->setContentsMargins(12, 0, 0, 0);
    groupRowLay->addWidget(new QLabel(tr("Group:"), m_groupRow));
    m_groupCombo = new QComboBox(m_groupRow);
    m_groupCombo->setMinimumWidth(200);
    groupRowLay->addWidget(m_groupCombo, 1);
    modeRow->addWidget(m_groupRow, 1);

    modeRow->addStretch();
    root->addWidget(modeWidget);

    // Populate group combo
    rebuildGroupCombo();
    // Select the group that was active
    if (fixtureGroupId != UINT_MAX)
    {
        for (int i = 0; i < m_groupCombo->count(); ++i)
        {
            if (m_groupCombo->itemData(i).toUInt() == fixtureGroupId)
            {
                m_groupCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    m_groupRow->setVisible(mode == PTMode::FixtureGroup);

    // ============================
    // Tabs
    // ============================
    QTabWidget* tabs = new QTabWidget(this);

    // ---- TAB: Outputs -------------------------------------------------------
    QWidget* outTab = new QWidget(tabs);
    QVBoxLayout* outLayout = new QVBoxLayout(outTab);

    m_outHintLabel = new QLabel(outTab);
    m_outHintLabel->setWordWrap(true);
    {
        QFont hf = m_outHintLabel->font();
        hf.setItalic(true);
        m_outHintLabel->setFont(hf);
    }
    outLayout->addWidget(m_outHintLabel);

    // Scrollable list of output editor rows
    m_outputList = new QListWidget(outTab);
    m_outputList->setUniformItemSizes(false);
    m_outputList->setSpacing(2);
    outLayout->addWidget(m_outputList, 1);

    QHBoxLayout* outBtnRow = new QHBoxLayout;
    m_addOutBtn = new QPushButton(tr("Add Output"), outTab);
    m_remOutBtn = new QPushButton(tr("Remove Output"), outTab);
    outBtnRow->addWidget(m_addOutBtn);
    outBtnRow->addWidget(m_remOutBtn);
    outBtnRow->addStretch();
    outLayout->addLayout(outBtnRow);

    tabs->addTab(outTab, tr("Outputs"));

    // ---- TAB: Columns -------------------------------------------------------
    QWidget* colTab = new QWidget(tabs);
    QVBoxLayout* colLayout = new QVBoxLayout(colTab);

    m_colTable = new QTableWidget(0, 4, colTab);
    m_colTable->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Fade"), tr("Binding")});
    m_colTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_colTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_colTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_colTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_colTable->verticalHeader()->setVisible(false);
    m_colTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_colTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_colTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    colLayout->addWidget(m_colTable, 1);

    QHBoxLayout* colBtnRow = new QHBoxLayout;
    m_editColBtn        = new QPushButton(tr("Edit..."), colTab);
    m_addColBtn         = new QPushButton(tr("+ Column"), colTab);
    m_remColsBtn        = new QPushButton(tr("- Column"), colTab);
    m_addChFromGrpBtn   = new QPushButton(tr("+ Channels from Group..."), colTab);
    colBtnRow->addWidget(m_editColBtn);
    colBtnRow->addWidget(m_addColBtn);
    colBtnRow->addWidget(m_remColsBtn);
    colBtnRow->addWidget(m_addChFromGrpBtn);
    colBtnRow->addStretch();
    colLayout->addLayout(colBtnRow);

    tabs->addTab(colTab, tr("Columns"));
    root->addWidget(tabs);

    // ---- Crossfade section --------------------------------------------------
    QGroupBox* xfGrp = new QGroupBox(tr("Crossfade"), this);
    QVBoxLayout* xfLayout = new QVBoxLayout(xfGrp);

    m_crossfadeChk = new QCheckBox(tr("Enable crossfade (row selector sets staged, not current)"), xfGrp);
    m_crossfadeChk->setChecked(crossfadeEnabled);
    xfLayout->addWidget(m_crossfadeChk);

    m_xfadeInputWidget = new QWidget(xfGrp);
    QHBoxLayout* xfInputRow = new QHBoxLayout(m_xfadeInputWidget);
    xfInputRow->setContentsMargins(0, 0, 0, 0);
    xfInputRow->addWidget(new QLabel(tr("Global crossfade input (0–255):"), m_xfadeInputWidget));
    m_xfadeInputSel = new InputSelectionWidget(doc, m_xfadeInputWidget);
    m_xfadeInputSel->setKeyInputVisibility(false);
    m_xfadeInputSel->setWidgetPage(widgetPage);
    m_xfadeInputSel->setInputSource(crossfadeSrc);
    xfInputRow->addWidget(m_xfadeInputSel, 1);
    xfLayout->addWidget(m_xfadeInputWidget);

    m_xfadeInputWidget->setVisible(crossfadeEnabled);
    root->addWidget(xfGrp);

    connect(m_crossfadeChk, &QCheckBox::toggled, m_xfadeInputWidget, &QWidget::setVisible);

    // ---- Error label --------------------------------------------------------
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #ff4444;"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    root->addWidget(m_errorLabel);

    // ---- Buttons ------------------------------------------------------------
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    // ---- Populate output rows -----------------------------------------------
    bool isFG = (mode == PTMode::FixtureGroup);
    FixtureGroup* grp = isFG ? m_doc->fixtureGroup(fixtureGroupId) : nullptr;

    for (int i = 0; i < outputs.size(); ++i)
    {
        QSharedPointer<QLCInputSource> src = (i < sources.size()) ? sources[i] : QSharedPointer<QLCInputSource>();

        if (!isFG)
        {
            auto* row = new OutputEditorRow(m_doc, outputs[i], src, m_columns.size(), m_widgetPage, this);
            m_outputRows.append(row);
            connect(row, &OutputEditorRow::fixtureSelected,
                    this, &PresetTableConfigDialog::slotAutoCreateColumns);
            QListWidgetItem* item = new QListWidgetItem(m_outputList);
            item->setSizeHint(row->sizeHint());
            m_outputList->addItem(item);
            m_outputList->setItemWidget(item, row);
        }
        else
        {
            auto* row = new FGOutputEditorRow(m_doc, outputs[i], src, grp, m_widgetPage, this);
            m_fgOutputRows.append(row);
            QListWidgetItem* item = new QListWidgetItem(m_outputList);
            item->setSizeHint(row->sizeHint());
            m_outputList->addItem(item);
            m_outputList->setItemWidget(item, row);
        }
    }

    updateHintLabel();

    // ---- Populate column table ----------------------------------------------
    rebuildColumnTable();
    updateColumnButtons();

    // ---- Connections --------------------------------------------------------
    connect(m_addOutBtn,  &QPushButton::clicked, this, &PresetTableConfigDialog::slotAddOutput);
    connect(m_remOutBtn,  &QPushButton::clicked, this, &PresetTableConfigDialog::slotRemoveOutput);

    // Column tab
    connect(m_editColBtn,       &QPushButton::clicked,
            this, &PresetTableConfigDialog::slotEditColumn);
    connect(m_addColBtn,        &QPushButton::clicked,
            this, &PresetTableConfigDialog::slotColumnsAddBlank);
    connect(m_remColsBtn,       &QPushButton::clicked,
            this, &PresetTableConfigDialog::slotColumnsRemoveSelected);
    connect(m_addChFromGrpBtn,  &QPushButton::clicked,
            this, &PresetTableConfigDialog::slotColumnsAddFromChannels);
    connect(m_colTable, &QTableWidget::cellDoubleClicked,
            this, [this](int /*row*/, int /*col*/) { slotEditColumn(); });
    connect(m_colTable, &QTableWidget::cellChanged,
            this, &PresetTableConfigDialog::slotColTableCellChanged);
    connect(m_colTable, &QTableWidget::itemChanged,
            this, &PresetTableConfigDialog::slotColTableItemChanged);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &PresetTableConfigDialog::slotValidate);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_modeCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresetTableConfigDialog::slotModeComboChanged);
    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresetTableConfigDialog::slotGroupComboChanged);
}

// ==========================================================================
// Accessors
// ==========================================================================

QVector<PTOutput> PresetTableConfigDialog::outputs() const
{
    QVector<PTOutput> result;
    PTMode curMode = widgetMode();

    if (curMode == PTMode::Legacy)
    {
        for (const auto* row : m_outputRows)
            result.append(row->output());
    }
    else
    {
        for (const auto* row : m_fgOutputRows)
            result.append(row->output());
    }
    return result;
}

QSharedPointer<QLCInputSource> PresetTableConfigDialog::inputSource(int outputIdx) const
{
    PTMode curMode = widgetMode();

    if (curMode == PTMode::Legacy)
    {
        if (outputIdx < 0 || outputIdx >= m_outputRows.size())
            return QSharedPointer<QLCInputSource>();
        return m_outputRows[outputIdx]->inputSource();
    }
    else
    {
        if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
            return QSharedPointer<QLCInputSource>();
        return m_fgOutputRows[outputIdx]->inputSource();
    }
}

bool PresetTableConfigDialog::crossfadeEnabled() const
{
    return m_crossfadeChk ? m_crossfadeChk->isChecked() : false;
}

QSharedPointer<QLCInputSource> PresetTableConfigDialog::crossfadeInputSource() const
{
    return m_xfadeInputSel ? m_xfadeInputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

PTMode PresetTableConfigDialog::widgetMode() const
{
    if (!m_modeCombo) return PTMode::Legacy;
    return (m_modeCombo->currentIndex() == 1) ? PTMode::FixtureGroup : PTMode::Legacy;
}

quint32 PresetTableConfigDialog::selectedFixtureGroupId() const
{
    if (!m_groupCombo || m_groupCombo->count() == 0) return UINT_MAX;
    return m_groupCombo->currentData().toUInt();
}

// ==========================================================================
// Private helpers
// ==========================================================================

void PresetTableConfigDialog::updateHintLabel()
{
    if (!m_outHintLabel) return;

    if (widgetMode() == PTMode::Legacy)
    {
        m_outHintLabel->setText(
            tr("Each output must be assigned to a fixture with exactly %n channel(s). "
               "External input: value 0 = off, value N = row N.", "", m_columns.size()));
    }
    else
    {
        FixtureGroup* grp = currentFixtureGroup();
        QString grpName = grp ? grp->name() : tr("<none selected>");
        m_outHintLabel->setText(
            tr("Fixture Group: %1. Each output covers one or more grid rows. "
               "External input: value 0 = off, value N = row N.").arg(grpName));
    }
}

void PresetTableConfigDialog::rebuildGroupCombo()
{
    if (!m_groupCombo) return;
    m_groupCombo->clear();
    for (FixtureGroup* grp : m_doc->fixtureGroups())
        m_groupCombo->addItem(grp->name(), grp->id());
    if (m_groupCombo->count() == 0)
        m_groupCombo->addItem(tr("<no fixture groups>"), QVariant(UINT_MAX));
}

FixtureGroup* PresetTableConfigDialog::currentFixtureGroup() const
{
    if (!m_groupCombo || m_groupCombo->count() == 0) return nullptr;
    quint32 id = m_groupCombo->currentData().toUInt();
    return (id != UINT_MAX) ? m_doc->fixtureGroup(id) : nullptr;
}

void PresetTableConfigDialog::rebuildOutputList()
{
    // Clear all rows from the list widget
    m_outputList->clear();
    qDeleteAll(m_outputRows);
    m_outputRows.clear();
    qDeleteAll(m_fgOutputRows);
    m_fgOutputRows.clear();
}

// ==========================================================================
// Slots
// ==========================================================================

void PresetTableConfigDialog::slotModeComboChanged(int /*index*/)
{
    PTMode newMode = widgetMode();
    bool isFG = (newMode == PTMode::FixtureGroup);

    m_groupRow->setVisible(isFG);
    updateHintLabel();
    updateColumnButtons();

    // Warn about clearing outputs when switching modes
    if (m_outputList->count() > 0)
    {
        int ret = QMessageBox::question(this,
            tr("Change Mode"),
            tr("Switching mode will clear all current output assignments. Continue?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
        {
            // Revert combo silently
            QSignalBlocker sb(m_modeCombo);
            m_modeCombo->setCurrentIndex(isFG ? 0 : 1);
            m_groupRow->setVisible(!isFG);
            updateHintLabel();
            return;
        }
    }

    rebuildOutputList();
}

void PresetTableConfigDialog::slotGroupComboChanged(int /*index*/)
{
    updateHintLabel();
    updateColumnButtons();

    // Update existing FG output rows with new group
    FixtureGroup* grp = currentFixtureGroup();
    for (FGOutputEditorRow* row : m_fgOutputRows)
        row->setFixtureGroup(grp);

    // Resize list items to fit new row heights
    for (int i = 0; i < m_outputList->count(); ++i)
    {
        QListWidgetItem* item = m_outputList->item(i);
        QWidget* w = m_outputList->itemWidget(item);
        if (w) item->setSizeHint(w->sizeHint());
    }
}

void PresetTableConfigDialog::slotAddOutput()
{
    PTOutput out;
    PTMode curMode = widgetMode();

    if (curMode == PTMode::Legacy)
    {
        out.name = tr("Output %1").arg(m_outputRows.size() + 1);
        auto* row = new OutputEditorRow(m_doc, out, QSharedPointer<QLCInputSource>(),
                                         m_columns.size(), m_widgetPage, this);
        m_outputRows.append(row);
        connect(row, &OutputEditorRow::fixtureSelected,
                this, &PresetTableConfigDialog::slotAutoCreateColumns);

        QListWidgetItem* item = new QListWidgetItem(m_outputList);
        item->setSizeHint(row->sizeHint());
        m_outputList->addItem(item);
        m_outputList->setItemWidget(item, row);
    }
    else
    {
        out.name = tr("Output %1").arg(m_fgOutputRows.size() + 1);
        FixtureGroup* grp = currentFixtureGroup();
        auto* row = new FGOutputEditorRow(m_doc, out, QSharedPointer<QLCInputSource>(),
                                           grp, m_widgetPage, this);
        m_fgOutputRows.append(row);

        QListWidgetItem* item = new QListWidgetItem(m_outputList);
        item->setSizeHint(row->sizeHint());
        m_outputList->addItem(item);
        m_outputList->setItemWidget(item, row);
    }

    m_outputList->scrollToBottom();
}

void PresetTableConfigDialog::slotRemoveOutput()
{
    int idx = m_outputList->currentRow();
    if (idx < 0) return;

    PTMode curMode = widgetMode();
    if (curMode == PTMode::Legacy)
    {
        if (idx >= m_outputRows.size()) return;
        m_outputRows.removeAt(idx);
    }
    else
    {
        if (idx >= m_fgOutputRows.size()) return;
        m_fgOutputRows.removeAt(idx);
    }
    delete m_outputList->takeItem(idx);
}

void PresetTableConfigDialog::slotEditColumn()
{
    int idx = m_colTable->currentRow();
    if (idx < 0 || idx >= m_columns.size()) return;

    PTMode   curMode = widgetMode();
    FixtureGroup* grp = (curMode == PTMode::FixtureGroup) ? currentFixtureGroup() : nullptr;

    PresetTableColumnDialog dlg(m_doc, m_columns[idx], curMode, grp, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_columns[idx] = dlg.column();
    rebuildColumnTable();

    // Keep same row selected
    m_colTable->selectRow(idx);

    updateHintLabel();
    if (curMode == PTMode::Legacy)
    {
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
    }
}

// ---------------------------------------------------------------------------
// Column table helpers
// ---------------------------------------------------------------------------

void PresetTableConfigDialog::rebuildColumnTable()
{
    if (!m_colTable) return;
    m_updatingColTable = true;
    m_colTable->blockSignals(true);

    m_colTable->setRowCount(0);
    for (int r = 0; r < m_columns.size(); ++r)
    {
        const PTColumn& col = m_columns[r];
        m_colTable->insertRow(r);

        // Col 0: Name — editable text
        QTableWidgetItem* nameItem = new QTableWidgetItem(col.name);
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        m_colTable->setItem(r, 0, nameItem);

        // Col 1: Type — inline QComboBox
        QComboBox* typeCombo = new QComboBox(m_colTable);
        typeCombo->addItem(tr("Numeric"),  int(PTColumn::Numeric));
        typeCombo->addItem(tr("Dropdown"), int(PTColumn::Dropdown));
        typeCombo->addItem(tr("Scaler"),   int(PTColumn::Scaler));
        typeCombo->setCurrentIndex(int(col.type));
        int capturedRow = r;
        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, capturedRow](int comboIdx) {
                    if (m_updatingColTable) return;
                    if (capturedRow < 0 || capturedRow >= m_columns.size()) return;
                    m_columns[capturedRow].type = PTColumn::Type(comboIdx);
                    // Refresh the Binding cell summary
                    if (auto* bi = m_colTable->item(capturedRow, 3))
                        bi->setText(bindingSummary(m_columns[capturedRow]));
                });
        m_colTable->setCellWidget(r, 1, typeCombo);

        // Col 2: Fade — checkable item
        QTableWidgetItem* fadeItem = new QTableWidgetItem();
        fadeItem->setFlags((fadeItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        fadeItem->setCheckState(col.fade ? Qt::Checked : Qt::Unchecked);
        m_colTable->setItem(r, 2, fadeItem);

        // Col 3: Binding — read-only summary
        QTableWidgetItem* bindItem = new QTableWidgetItem(bindingSummary(col));
        bindItem->setFlags(bindItem->flags() & ~Qt::ItemIsEditable);
        m_colTable->setItem(r, 3, bindItem);
    }

    m_colTable->blockSignals(false);
    m_updatingColTable = false;
}

QString PresetTableConfigDialog::bindingSummary(const PTColumn& col) const
{
    if (!col.binding.isValid()) return tr("—");

    // Try to resolve the channel name from the group
    if (m_doc)
    {
        FixtureGroup* grp = currentFixtureGroup();
        if (grp)
        {
            for (quint32 fxiId : grp->fixtureList())
            {
                Fixture* fxi = m_doc->fixture(fxiId);
                if (!fxi) continue;
                QLCFixtureDef*  def  = fxi->fixtureDef();
                QLCFixtureMode* mode = fxi->fixtureMode();
                if (!def || !mode) continue;
                if (def->manufacturer() == col.binding.manufacturer &&
                    def->model()        == col.binding.model &&
                    mode->name()        == col.binding.modeName)
                {
                    QLCChannel* ch = mode->channel(quint32(col.binding.channelIndex));
                    if (ch)
                        return QString("ch%1: %2").arg(col.binding.channelIndex + 1).arg(ch->name());
                    break;
                }
            }
        }
    }
    return QString("%1 ch%2").arg(col.binding.model).arg(col.binding.channelIndex + 1);
}

void PresetTableConfigDialog::updateColumnButtons()
{
    bool fgMode = (widgetMode() == PTMode::FixtureGroup);
    bool hasGrp = (currentFixtureGroup() != nullptr);
    if (m_addChFromGrpBtn)
        m_addChFromGrpBtn->setEnabled(fgMode && hasGrp);
}

// ---------------------------------------------------------------------------
// Column table inline-edit slots
// ---------------------------------------------------------------------------

void PresetTableConfigDialog::slotColTableCellChanged(int row, int col)
{
    if (m_updatingColTable) return;
    if (row < 0 || row >= m_columns.size()) return;
    if (col == 0)
    {
        auto* item = m_colTable->item(row, 0);
        if (item) m_columns[row].name = item->text();
    }
}

void PresetTableConfigDialog::slotColTableItemChanged(QTableWidgetItem* item)
{
    if (m_updatingColTable) return;
    if (!item || item->column() != 2) return;
    int row = item->row();
    if (row < 0 || row >= m_columns.size()) return;
    m_columns[row].fade = (item->checkState() == Qt::Checked);
}

// ---------------------------------------------------------------------------
// Column toolbar button slots
// ---------------------------------------------------------------------------

void PresetTableConfigDialog::slotColumnsAddBlank()
{
    PTColumn col;
    col.name = tr("Column %1").arg(m_columns.size() + 1);
    col.type = PTColumn::Numeric;
    col.fade = true;
    m_columns.append(col);
    rebuildColumnTable();
    m_colTable->selectRow(m_columns.size() - 1);
    if (widgetMode() == PTMode::Legacy)
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
    updateHintLabel();
}

void PresetTableConfigDialog::slotColumnsRemoveSelected()
{
    QList<int> rows;
    const auto selected = m_colTable->selectionModel()->selectedRows();
    for (const QModelIndex& idx : selected)
        rows.append(idx.row());
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        if (r >= 0 && r < m_columns.size())
            m_columns.removeAt(r);
    rebuildColumnTable();
    if (widgetMode() == PTMode::Legacy)
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
    updateHintLabel();
}

void PresetTableConfigDialog::slotColumnsAddFromChannels()
{
    FixtureGroup* grp = currentFixtureGroup();
    if (!grp || !m_doc) return;

    // Build deduplicated fixture-type list from the group
    struct FxEntry {
        QString manufacturer, model, modeName;
        QLCFixtureMode* mode = nullptr;
    };
    QList<FxEntry> fxTypes;
    for (quint32 fxiId : grp->fixtureList())
    {
        Fixture* fxi = m_doc->fixture(fxiId);
        if (!fxi) continue;
        QLCFixtureDef*  def  = fxi->fixtureDef();
        QLCFixtureMode* mode = fxi->fixtureMode();
        if (!def || !mode) continue;
        bool found = false;
        for (const FxEntry& e : fxTypes)
            if (e.manufacturer == def->manufacturer() &&
                e.model        == def->model() &&
                e.modeName     == mode->name())
            { found = true; break; }
        if (!found)
            fxTypes.append({def->manufacturer(), def->model(), mode->name(), mode});
    }
    if (fxTypes.isEmpty())
    {
        QMessageBox::information(this, tr("No fixtures"),
            tr("The selected fixture group contains no fixture types with definitions."));
        return;
    }

    // ---- Build inline picker dialog ----------------------------------------
    QDialog picker(this);
    picker.setWindowTitle(tr("Add columns from group channels"));
    picker.resize(520, 440);
    QVBoxLayout* pl = new QVBoxLayout(&picker);

    // Fixture type row
    QHBoxLayout* typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel(tr("Fixture type:"), &picker));
    QComboBox* typeCombo = new QComboBox(&picker);
    for (const FxEntry& e : fxTypes)
        typeCombo->addItem(QString("%1 %2 — %3").arg(e.manufacturer, e.model, e.modeName));
    typeRow->addWidget(typeCombo, 1);
    pl->addLayout(typeRow);

    // Channel list
    QListWidget* chanList = new QListWidget(&picker);
    chanList->setSelectionMode(QAbstractItemView::NoSelection);
    pl->addWidget(chanList, 1);

    // Lambda: rebuild channel list for the chosen fixture type
    auto populateChannels = [&](int typeIdx)
    {
        chanList->clear();
        if (typeIdx < 0 || typeIdx >= fxTypes.size()) return;
        QLCFixtureMode* fxMode = fxTypes[typeIdx].mode;
        if (!fxMode) return;

        const QVector<QLCFixtureHead>& heads = fxMode->heads();
        int nCh = fxMode->channels().size();
        for (int ci = 0; ci < nCh; ++ci)
        {
            QLCChannel* ch = fxMode->channel(quint32(ci));

            // Determine head label
            int headIdx = -1;
            for (int hi = 0; hi < heads.size(); ++hi)
                if (heads[hi].channels().contains(quint32(ci))) { headIdx = hi; break; }

            QString prefix;
            if (!heads.isEmpty())
                prefix = (headIdx >= 0) ? QString("[H%1] ").arg(headIdx) : tr("[shared] ");

            QString label = prefix + (ch
                ? QString("%1: %2").arg(ci + 1).arg(ch->name())
                : tr("Ch %1").arg(ci + 1));

            QListWidgetItem* item = new QListWidgetItem(label, chanList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            item->setData(Qt::UserRole, ci);
        }
    };
    populateChannels(0);

    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &picker, [&](int idx) { populateChannels(idx); });

    // Select-all / Clear row
    QHBoxLayout* selRow = new QHBoxLayout;
    QPushButton* selAll  = new QPushButton(tr("Select all"), &picker);
    QPushButton* selNone = new QPushButton(tr("Clear"),      &picker);
    selRow->addWidget(selAll);
    selRow->addWidget(selNone);
    selRow->addStretch();
    pl->addLayout(selRow);

    connect(selAll,  &QPushButton::clicked, &picker, [&] {
        for (int i = 0; i < chanList->count(); ++i)
            chanList->item(i)->setCheckState(Qt::Checked);
    });
    connect(selNone, &QPushButton::clicked, &picker, [&] {
        for (int i = 0; i < chanList->count(); ++i)
            chanList->item(i)->setCheckState(Qt::Unchecked);
    });

    QDialogButtonBox* pb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &picker);
    pl->addWidget(pb);
    connect(pb, &QDialogButtonBox::accepted, &picker, &QDialog::accept);
    connect(pb, &QDialogButtonBox::rejected, &picker, &QDialog::reject);

    if (picker.exec() != QDialog::Accepted) return;

    // ---- Collect selected channels and append PTColumns --------------------
    int typeIdx = typeCombo->currentIndex();
    if (typeIdx < 0 || typeIdx >= fxTypes.size()) return;
    const FxEntry& fxEntry = fxTypes[typeIdx];
    QLCFixtureMode* fxMode = fxEntry.mode;

    bool anyAdded = false;
    for (int i = 0; i < chanList->count(); ++i)
    {
        QListWidgetItem* item = chanList->item(i);
        if (item->checkState() != Qt::Checked) continue;

        int chanIdx = item->data(Qt::UserRole).toInt();
        QLCChannel* ch = fxMode ? fxMode->channel(quint32(chanIdx)) : nullptr;

        PTColumn col;
        col.name               = ch ? ch->name() : tr("Ch %1").arg(chanIdx + 1);
        col.type               = PTColumn::Numeric;
        col.fade               = true;
        col.binding.manufacturer = fxEntry.manufacturer;
        col.binding.model        = fxEntry.model;
        col.binding.modeName     = fxEntry.modeName;
        col.binding.channelIndex = chanIdx;
        m_columns.append(col);
        anyAdded = true;
    }

    if (anyAdded)
    {
        rebuildColumnTable();
        if (widgetMode() == PTMode::Legacy)
            for (auto* row : m_outputRows)
                row->setRequiredChannels(m_columns.size());
        updateHintLabel();
    }
}

void PresetTableConfigDialog::slotAutoCreateColumns(quint32 fixtureId)
{
    // Only auto-create when in Legacy mode and there are no columns yet
    if (widgetMode() != PTMode::Legacy) return;
    if (!m_columns.isEmpty()) return;

    Fixture* fx = m_doc->fixture(fixtureId);
    if (!fx || fx->channels() == 0) return;

    m_columns.clear();
    for (quint32 ch = 0; ch < fx->channels(); ++ch)
    {
        PTColumn col;
        const QLCChannel* qlcCh = fx->channel(ch);
        col.name = qlcCh ? qlcCh->name() : tr("Ch %1").arg(ch + 1);
        col.type = PTColumn::Numeric;
        col.fade = true;
        m_columns.append(col);
    }

    rebuildColumnTable();
    updateHintLabel();

    int n = m_columns.size();
    for (auto* row : m_outputRows)
        row->setRequiredChannels(n);
}

void PresetTableConfigDialog::slotValidate()
{
    PTMode curMode = widgetMode();
    QStringList errors;

    if (curMode == PTMode::Legacy)
    {
        int required = m_columns.size();
        for (int i = 0; i < m_outputRows.size(); ++i)
        {
            PTOutput out = m_outputRows[i]->output();
            if (out.fixtureId == UINT_MAX) continue;  // unassigned is ok

            Fixture* fx = m_doc->fixture(out.fixtureId);
            if (!fx)
            {
                errors.append(tr("Output \"%1\": fixture not found.").arg(out.name));
                continue;
            }
            if ((int)fx->channels() != required)
            {
                errors.append(tr("Output \"%1\": fixture has %2 channels but table has %3 value columns.")
                    .arg(out.name).arg(fx->channels()).arg(required));
            }
        }
    }
    else
    {
        // FixtureGroup mode validation
        FixtureGroup* grp = currentFixtureGroup();
        if (!grp)
        {
            errors.append(tr("No Fixture Group selected."));
        }
        else
        {
            for (int i = 0; i < m_fgOutputRows.size(); ++i)
            {
                PTOutput out = m_fgOutputRows[i]->output();
                if (out.groupRows.isEmpty())
                    errors.append(tr("Output \"%1\": no grid rows selected.").arg(out.name));
            }

            // Warn if any column has no binding
            for (const PTColumn& col : m_columns)
            {
                if (!col.binding.isValid())
                    errors.append(tr("Column \"%1\" has no fixture binding set.").arg(col.name));
            }
        }
    }

    if (!errors.isEmpty())
    {
        m_errorLabel->setText(errors.join(QLatin1Char('\n')));
        m_errorLabel->setVisible(true);
        return;
    }

    m_errorLabel->setVisible(false);
    accept();
}

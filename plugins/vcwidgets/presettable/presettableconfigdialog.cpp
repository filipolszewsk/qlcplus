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

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
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

    {
        QFont hf;
        hf.setItalic(true);
        QLabel* colHint = new QLabel(
            tr("Double-click a column header in the widget to edit it directly, "
               "or select a column here and click Edit."), colTab);
        colHint->setWordWrap(true);
        colHint->setFont(hf);
        colLayout->addWidget(colHint);
    }

    m_colList = new QListWidget(colTab);
    m_colList->setSelectionMode(QAbstractItemView::SingleSelection);
    colLayout->addWidget(m_colList, 1);

    QHBoxLayout* colBtnRow = new QHBoxLayout;
    m_editColBtn = new QPushButton(tr("Edit Column..."), colTab);
    colBtnRow->addWidget(m_editColBtn);
    colBtnRow->addStretch();
    colLayout->addLayout(colBtnRow);

    for (const PTColumn& c : m_columns)
    {
        QString typeStr = (c.type == PTColumn::Dropdown)
            ? tr("Dropdown (%1 options)").arg(c.options.size())
            : tr("Numeric");
        QString bindStr;
        if (c.binding.isValid())
            bindStr = QString(" [%1 ch%2]").arg(c.binding.model).arg(c.binding.channelIndex + 1);
        m_colList->addItem(QString("%1  [%2]%3").arg(c.name, typeStr, bindStr));
    }

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

    // ---- Connections --------------------------------------------------------
    connect(m_addOutBtn,  &QPushButton::clicked, this, &PresetTableConfigDialog::slotAddOutput);
    connect(m_remOutBtn,  &QPushButton::clicked, this, &PresetTableConfigDialog::slotRemoveOutput);
    connect(m_editColBtn, &QPushButton::clicked, this, &PresetTableConfigDialog::slotEditColumn);
    connect(m_colList, &QListWidget::itemDoubleClicked, this, &PresetTableConfigDialog::slotEditColumn);
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
    int idx = m_colList->currentRow();
    if (idx < 0 || idx >= m_columns.size()) return;

    PTMode   curMode = widgetMode();
    FixtureGroup* grp = (curMode == PTMode::FixtureGroup) ? currentFixtureGroup() : nullptr;

    PresetTableColumnDialog dlg(m_doc, m_columns[idx], curMode, grp, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_columns[idx] = dlg.column();

    // Refresh column list display
    const PTColumn& c = m_columns[idx];
    QString typeStr = (c.type == PTColumn::Dropdown)
        ? tr("Dropdown (%1 options)").arg(c.options.size())
        : tr("Numeric");
    QString bindStr;
    if (c.binding.isValid())
        bindStr = QString(" [%1 ch%2]").arg(c.binding.model).arg(c.binding.channelIndex + 1);
    m_colList->item(idx)->setText(QString("%1  [%2]%3").arg(c.name, typeStr, bindStr));

    updateHintLabel();

    // Update required channel count in legacy output rows
    if (curMode == PTMode::Legacy)
    {
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
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
    m_colList->clear();

    for (quint32 ch = 0; ch < fx->channels(); ++ch)
    {
        PTColumn col;
        const QLCChannel* qlcCh = fx->channel(ch);
        col.name = qlcCh ? qlcCh->name() : tr("Ch %1").arg(ch + 1);
        col.type = PTColumn::Numeric;
        m_columns.append(col);
        m_colList->addItem(QString("%1  [Numeric]").arg(col.name));
    }

    int n = m_columns.size();
    updateHintLabel();

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

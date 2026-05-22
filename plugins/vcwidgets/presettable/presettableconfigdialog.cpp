/*
  QLC+ VC Widget Plugin — Preset Table
  presettableconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "presettableconfigdialog.h"
#include "presettablecolumndialog.h"

#include "inputselectionwidget.h"
#include "fixtureselection.h"
#include "doc.h"
#include "fixture.h"
#include "qlcchannel.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QFrame>
#include <algorithm>

// ==========================================================================
// OutputEditorRow
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
// PresetTableConfigDialog
// ==========================================================================

PresetTableConfigDialog::PresetTableConfigDialog(Doc* doc,
                                                   const QVector<PTColumn>& columns,
                                                   const QVector<PTOutput>&  outputs,
                                                   const QVector<QSharedPointer<QLCInputSource>>& sources,
                                                   bool crossfadeEnabled,
                                                   QSharedPointer<QLCInputSource> crossfadeSrc,
                                                   int widgetPage,
                                                   QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_widgetPage(widgetPage)
    , m_columns(columns)
    , m_initSources(sources)
{
    setWindowTitle(tr("Preset Table — Properties"));
    setMinimumSize(560, 480);

    QVBoxLayout* root = new QVBoxLayout(this);

    QTabWidget* tabs = new QTabWidget(this);

    // ============================
    // TAB: Outputs
    // ============================
    QWidget* outTab = new QWidget(tabs);
    QVBoxLayout* outLayout = new QVBoxLayout(outTab);

    m_outHintLabel = new QLabel(
        tr("Each output must be assigned to a fixture with exactly %n channel(s). "
           "External input: value 0 = off, value N = row N.", "", m_columns.size()),
        outTab);
    m_outHintLabel->setWordWrap(true);
    QFont hf = m_outHintLabel->font();
    hf.setItalic(true);
    m_outHintLabel->setFont(hf);
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

    // ============================
    // TAB: Columns
    // ============================
    QWidget* colTab = new QWidget(tabs);
    QVBoxLayout* colLayout = new QVBoxLayout(colTab);

    QLabel* colHint = new QLabel(
        tr("Double-click a column header in the widget to edit it directly, "
           "or select a column here and click Edit."), colTab);
    colHint->setWordWrap(true);
    colHint->setFont(hf);
    colLayout->addWidget(colHint);

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
        m_colList->addItem(QString("%1  [%2]").arg(c.name, typeStr));
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
    for (int i = 0; i < outputs.size(); ++i)
    {
        QSharedPointer<QLCInputSource> src = (i < sources.size()) ? sources[i] : QSharedPointer<QLCInputSource>();
        auto* row = new OutputEditorRow(m_doc, outputs[i], src, m_columns.size(), m_widgetPage, this);
        m_outputRows.append(row);
        connect(row, &OutputEditorRow::fixtureSelected,
                this, &PresetTableConfigDialog::slotAutoCreateColumns);
        // Embed in QListWidget item
        QListWidgetItem* item = new QListWidgetItem(m_outputList);
        item->setSizeHint(row->sizeHint());
        m_outputList->addItem(item);
        m_outputList->setItemWidget(item, row);
    }

    // ---- Connections --------------------------------------------------------
    connect(m_addOutBtn,  &QPushButton::clicked, this, &PresetTableConfigDialog::slotAddOutput);
    connect(m_remOutBtn,  &QPushButton::clicked, this, &PresetTableConfigDialog::slotRemoveOutput);
    connect(m_editColBtn, &QPushButton::clicked, this, &PresetTableConfigDialog::slotEditColumn);
    connect(m_colList, &QListWidget::itemDoubleClicked, this, &PresetTableConfigDialog::slotEditColumn);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &PresetTableConfigDialog::slotValidate);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QVector<PTOutput> PresetTableConfigDialog::outputs() const
{
    QVector<PTOutput> result;
    for (const auto* row : m_outputRows)
        result.append(row->output());
    return result;
}

QSharedPointer<QLCInputSource> PresetTableConfigDialog::inputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_outputRows[outputIdx]->inputSource();
}

bool PresetTableConfigDialog::crossfadeEnabled() const
{
    return m_crossfadeChk ? m_crossfadeChk->isChecked() : false;
}

QSharedPointer<QLCInputSource> PresetTableConfigDialog::crossfadeInputSource() const
{
    return m_xfadeInputSel ? m_xfadeInputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

void PresetTableConfigDialog::slotAddOutput()
{
    PTOutput out;
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
    m_outputList->scrollToBottom();
}

void PresetTableConfigDialog::slotRemoveOutput()
{
    int idx = m_outputList->currentRow();
    if (idx < 0 || idx >= m_outputRows.size()) return;

    m_outputRows.removeAt(idx);
    delete m_outputList->takeItem(idx);
}

void PresetTableConfigDialog::slotEditColumn()
{
    int idx = m_colList->currentRow();
    if (idx < 0 || idx >= m_columns.size()) return;

    PresetTableColumnDialog dlg(m_doc, m_columns[idx], this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_columns[idx] = dlg.column();

    // Refresh column list display
    const PTColumn& c = m_columns[idx];
    QString typeStr = (c.type == PTColumn::Dropdown)
        ? tr("Dropdown (%1 options)").arg(c.options.size())
        : tr("Numeric");
    m_colList->item(idx)->setText(QString("%1  [%2]").arg(c.name, typeStr));

    // Update hint label with new column count (in case future column ops change it)
    m_outHintLabel->setText(
        tr("Each output must be assigned to a fixture with exactly %n channel(s). "
           "External input: value 0 = off, value N = row N.", "", m_columns.size()));
}

void PresetTableConfigDialog::slotAutoCreateColumns(quint32 fixtureId)
{
    // Only auto-create when there are no columns yet (first fixture assignment)
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

    // Update hint label
    m_outHintLabel->setText(
        tr("Each output must be assigned to a fixture with exactly %n channel(s). "
           "External input: value 0 = off, value N = row N.", "", n));

    // Update required channel count in all existing output rows so their
    // fixture labels refresh their validation color
    for (auto* row : m_outputRows)
        row->setRequiredChannels(n);
}

void PresetTableConfigDialog::slotValidate()
{
    // Check that all assigned fixtures have the correct channel count
    int required = m_columns.size();
    QStringList errors;

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

    if (!errors.isEmpty())
    {
        m_errorLabel->setText(errors.join(QLatin1Char('\n')));
        m_errorLabel->setVisible(true);
        return;
    }

    m_errorLabel->setVisible(false);
    accept();
}

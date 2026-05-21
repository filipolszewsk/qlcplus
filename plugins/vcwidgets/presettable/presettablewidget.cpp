/*
  QLC+ VC Widget Plugin — Preset Table
  presettablewidget.cpp — Apache 2.0 / public domain
*/

#include "presettablewidget.h"
#include "presettableconfigdialog.h"
#include "presettablecolumndialog.h"

#include "genericfader.h"
#include "fadechannel.h"
#include "mastertimer.h"
#include "universe.h"
#include "fixture.h"
#include "qlcchannel.h"
#include "qlcinputsource.h"
#include "doc.h"

#include <QPainter>
#include <QMutexLocker>
#include <QSpinBox>
#include <QComboBox>
#include <QStyleOptionViewItem>
#include <QHeaderView>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>
#include <algorithm>
#include <climits>

// ---- Static colors for output badges -------------------------------------

const QColor PresetTableWidget::s_outputColors[8] = {
    QColor("#3a86ff"),   // 0 — blue
    QColor("#ff006e"),   // 1 — pink
    QColor("#ffbe0b"),   // 2 — amber
    QColor("#06d6a0"),   // 3 — teal
    QColor("#fb5607"),   // 4 — orange
    QColor("#8338ec"),   // 5 — purple
    QColor("#ef233c"),   // 6 — red
    QColor("#80b918"),   // 7 — lime
};

// ---- XML tag constants ---------------------------------------------------

static const QString KXMLRoot       = QStringLiteral("PluginWidget");
static const QString KXMLPluginId   = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal= QStringLiteral("org.qlcplus.vcwidgets.presettable");
static const QString KXMLColumn     = QStringLiteral("Column");
static const QString KXMLColIndex   = QStringLiteral("Index");
static const QString KXMLColName    = QStringLiteral("Name");
static const QString KXMLColType    = QStringLiteral("Type");
static const QString KXMLColFade    = QStringLiteral("Fade");
static const QString KXMLOption     = QStringLiteral("Option");
static const QString KXMLOptName    = QStringLiteral("Name");
static const QString KXMLOptValue   = QStringLiteral("Value");
static const QString KXMLRow        = QStringLiteral("Row");
static const QString KXMLRowIndex   = QStringLiteral("Index");
static const QString KXMLRowName    = QStringLiteral("Name");
static const QString KXMLV          = QStringLiteral("V");
static const QString KXMLOutput     = QStringLiteral("Output");
static const QString KXMLOutIndex   = QStringLiteral("Index");
static const QString KXMLOutName    = QStringLiteral("Name");
static const QString KXMLOutFxId    = QStringLiteral("FixtureID");
static const QString KXMLOutInput        = QStringLiteral("OutInput");
static const QString KXMLCrossfadeEn     = QStringLiteral("CrossfadeEnabled");
static const QString KXMLCrossfadeInput  = QStringLiteral("CrossfadeInput");

// ==========================================================================
// PresetTableDelegate
// ==========================================================================

PresetTableDelegate::PresetTableDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void PresetTableDelegate::setColumns(const QVector<PTColumn>* columns)
{
    m_columns = columns;
}

QWidget* PresetTableDelegate::createEditor(QWidget* parent,
                                            const QStyleOptionViewItem& /*option*/,
                                            const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
    {
        // Row name — plain line edit provided by Qt default delegate is fine,
        // but we handle it ourselves with QTableWidgetItem editable flag.
        return nullptr;
    }

    int valCol = col - 1;  // value column index
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return nullptr;

    const PTColumn& ptcol = (*m_columns)[valCol];

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = new QComboBox(parent);
        for (const PTOption& opt : ptcol.options)
            cb->addItem(opt.name, int(opt.value));
        return cb;
    }
    else
    {
        QSpinBox* sb = new QSpinBox(parent);
        sb->setRange(0, 255);
        sb->setFrame(false);
        return sb;
    }
}

void PresetTableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0) return;

    int valCol = col - 1;
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return;

    const PTColumn& ptcol = (*m_columns)[valCol];
    int stored = index.data(Qt::UserRole).toInt();

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        // Select entry matching the stored DMX value
        for (int i = 0; i < cb->count(); ++i)
        {
            if (cb->itemData(i).toInt() == stored)
            {
                cb->setCurrentIndex(i);
                return;
            }
        }
        cb->setCurrentIndex(0);
    }
    else
    {
        QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
        if (sb) sb->setValue(stored);
    }
}

void PresetTableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                        const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0) return;

    int valCol = col - 1;
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return;

    const PTColumn& ptcol = (*m_columns)[valCol];

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        int dmxVal = cb->currentData().toInt();
        // Set UserRole (DMX int) FIRST so slotCellChanged reads the correct value
        model->setData(index, dmxVal, Qt::UserRole);
        model->setData(index, cb->currentText(), Qt::DisplayRole);
    }
    else
    {
        QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
        if (!sb) return;
        // Set UserRole (DMX int) FIRST so slotCellChanged reads the correct value
        model->setData(index, sb->value(), Qt::UserRole);
        model->setData(index, QString::number(sb->value()), Qt::DisplayRole);
    }
}

void PresetTableDelegate::updateEditorGeometry(QWidget* editor,
                                                const QStyleOptionViewItem& option,
                                                const QModelIndex& /*index*/) const
{
    if (editor)
        editor->setGeometry(option.rect);
}

void PresetTableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    // Draw background selection
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    else
        painter->fillRect(option.rect, option.backgroundBrush);

    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(option.palette.color(QPalette::Text));
    painter->drawText(option.rect.adjusted(4, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
}

QSize PresetTableDelegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& /*index*/) const
{
    return QSize(60, option.fontMetrics.height() + 8);
}

// ==========================================================================
// PresetTableWidget — construction
// ==========================================================================

PresetTableWidget::PresetTableWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(PresetTableWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Preset Table"));
    resize(QSize(400, 260));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);

    // ---- Toolbar (Design mode only) --------------------------------------
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setMovable(false);

    QAction* actAddRow = m_toolbar->addAction(tr("+ Row"));
    QAction* actRemRow = m_toolbar->addAction(tr("- Row"));
    m_toolbar->addSeparator();
    QAction* actAddCol = m_toolbar->addAction(tr("+ Col"));
    QAction* actRemCol = m_toolbar->addAction(tr("- Col"));
    m_toolbar->addSeparator();
    QAction* actProps  = m_toolbar->addAction(tr("Properties..."));

    connect(actAddRow, &QAction::triggered, this, &PresetTableWidget::slotAddRow);
    connect(actRemRow, &QAction::triggered, this, &PresetTableWidget::slotRemoveRow);
    connect(actAddCol, &QAction::triggered, this, &PresetTableWidget::slotAddColumn);
    connect(actRemCol, &QAction::triggered, this, &PresetTableWidget::slotRemoveColumn);
    connect(actProps,  &QAction::triggered, this, &PresetTableWidget::slotProperties);

    m_layout->addWidget(m_toolbar);

    // ---- Table -----------------------------------------------------------
    m_delegate = new PresetTableDelegate(this);
    m_delegate->setColumns(&m_columns);

    m_table = new QTableWidget(0, 1, this);  // start: 0 rows, 1 col (Name)
    m_table->setHorizontalHeaderLabels(QStringList() << tr("Name"));
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->verticalHeader()->setDefaultSectionSize(22);
    m_table->verticalHeader()->hide();
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setAlternatingRowColors(true);
    m_table->setItemDelegate(m_delegate);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    connect(m_table, &QTableWidget::cellChanged,
            this, &PresetTableWidget::slotCellChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &PresetTableWidget::slotColumnHeaderDoubleClicked);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized,
            this, &PresetTableWidget::slotHeaderSectionResized);

    m_layout->addWidget(m_table, 1);

    // ---- Status bar (Operate mode only) ----------------------------------
    m_statusBar = new QLabel(this);
    m_statusBar->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont sf = m_statusBar->font();
    sf.setPointSize(7);
    m_statusBar->setFont(sf);
    m_statusBar->setVisible(false);
    m_layout->addWidget(m_statusBar);

    setLayout(m_layout);
}

PresetTableWidget::~PresetTableWidget()
{
    if (m_doc && m_doc->masterTimer())
        m_doc->masterTimer()->unregisterDMXSource(this);

    QMutexLocker lk(&m_stateMutex);
    for (auto& fader : m_faders)
        if (!fader.isNull()) fader->requestDelete();
    m_faders.clear();
}

// ==========================================================================
// Data setters
// ==========================================================================

void PresetTableWidget::setColumns(const QVector<PTColumn>& cols)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_columns = cols;
        // Resize row value vectors
        for (PTRow& row : m_rows)
            row.values.resize(m_columns.size(), 0);
    }
    rebuildTable();
}

void PresetTableWidget::setRows(const QVector<PTRow>& rows)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_rows = rows;
        for (PTRow& row : m_rows)
            row.values.resize(m_columns.size(), 0);
        // Reset active rows
        m_activeRow.fill(-1, m_outputs.size());
        m_stagedRow.fill(-1, m_outputs.size());
    }
    rebuildTable();
}

void PresetTableWidget::setOutputs(const QVector<PTOutput>& outs)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_outputs = outs;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
    }
    refreshRowHighlights();
}

// ==========================================================================
// Mode
// ==========================================================================

void PresetTableWidget::slotModeChanged(Doc::Mode newMode)
{
    if (newMode == Doc::Operate)
    {
        m_toolbar->setVisible(false);
        m_statusBar->setVisible(true);
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else
    {
        m_toolbar->setVisible(true);
        m_statusBar->setVisible(false);
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_doc->masterTimer()->unregisterDMXSource(this);

        {
            QMutexLocker lk(&m_stateMutex);
            for (auto& fader : m_faders)
                if (!fader.isNull()) fader->requestDelete();
            m_faders.clear();
            m_activeRow.fill(-1, m_activeRow.size());
            m_stagedRow.fill(-1, m_stagedRow.size());
            m_crossfadeGlobalPos = 0;
            m_crossfadeStartPos  = 0;
        }
    }

    VCWidget::slotModeChanged(newMode);

    if (newMode == Doc::Design)
    {
        // Rebuild AFTER mode is Design so rebuildTable/refreshRowHighlights
        // won't add badge-prefixed text back into cells
        rebuildTable();
    }

    refreshRowHighlights();
    update();
}

// ==========================================================================
// Toolbar actions
// ==========================================================================

void PresetTableWidget::slotAddRow()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Row"),
                                         tr("Preset name:"), QLineEdit::Normal,
                                         tr("Preset %1").arg(m_rows.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    PTRow row;
    row.name = name.trimmed();
    row.values.resize(m_columns.size(), 0);

    {
        QMutexLocker lk(&m_stateMutex);
        m_rows.append(row);
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableWidget::slotRemoveRow()
{
    int selRow = m_table->currentRow();
    if (selRow < 0 || selRow >= m_rows.size()) return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_rows.remove(selRow);
        // Clamp active rows
        for (int& ar : m_activeRow)
            if (ar >= m_rows.size()) ar = -1;
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableWidget::slotAddColumn()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Column"),
                                         tr("Channel name:"), QLineEdit::Normal,
                                         tr("Ch %1").arg(m_columns.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    PTColumn col;
    col.name = name.trimmed();
    col.type = PTColumn::Numeric;

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns.append(col);
        for (PTRow& row : m_rows)
            row.values.append(0);
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableWidget::slotRemoveColumn()
{
    int selCol = m_table->currentColumn() - 1;  // -1 because col 0 is Name
    if (selCol < 0 || selCol >= m_columns.size()) return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns.remove(selCol);
        for (PTRow& row : m_rows)
            if (selCol < row.values.size()) row.values.remove(selCol);
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableWidget::slotProperties()
{
    editProperties();
}

// ==========================================================================
// Column header double-click → open column config dialog
// ==========================================================================

void PresetTableWidget::slotColumnHeaderDoubleClicked(int logicalIndex)
{
    if (mode() != Doc::Design) return;
    if (logicalIndex == 0) return;  // Name column — no config

    int valCol = logicalIndex - 1;
    if (valCol < 0 || valCol >= m_columns.size()) return;

    PresetTableColumnDialog dlg(m_columns[valCol], this);
    if (dlg.exec() != QDialog::Accepted) return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns[valCol] = dlg.column();
    }
    rebuildTable();
    m_doc->setModified();
}

// ==========================================================================
// Event filter — multi-column header selection (Ctrl+click)
// ==========================================================================

bool PresetTableWidget::eventFilter(QObject* obj, QEvent* ev)
{
    return VCWidget::eventFilter(obj, ev);
}

// ==========================================================================
// Multi-column resize — propagate width to all selected columns
// ==========================================================================

void PresetTableWidget::slotHeaderSectionResized(int logicalIndex, int /*oldSize*/, int newSize)
{
    if (m_resizingColumns) return;
    QList<int> selected;
    for (const QModelIndex& idx : m_table->selectionModel()->selectedColumns())
        selected.append(idx.column());
    if (!selected.contains(logicalIndex) || selected.size() < 2) return;
    m_resizingColumns = true;
    for (int col : selected)
        if (col != logicalIndex)
            m_table->setColumnWidth(col, newSize);
    m_resizingColumns = false;
}

// ==========================================================================
// keyPressEvent — Ctrl+C / Ctrl+V
// ==========================================================================

void PresetTableWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->matches(QKeySequence::Copy))  { slotCopySelection(); e->accept(); return; }
    if (e->matches(QKeySequence::Paste)) { slotPasteSelection(); e->accept(); return; }
    VCWidget::keyPressEvent(e);
}

// ==========================================================================
// Copy selection → clipboard as TSV
// ==========================================================================

void PresetTableWidget::slotCopySelection()
{
    QList<QTableWidgetItem*> items = m_table->selectedItems();
    if (items.isEmpty()) return;

    int minRow = INT_MAX, maxRow = INT_MIN, minCol = INT_MAX, maxCol = INT_MIN;
    for (auto* it : items)
    {
        minRow = qMin(minRow, it->row());    maxRow = qMax(maxRow, it->row());
        minCol = qMin(minCol, it->column()); maxCol = qMax(maxCol, it->column());
    }

    QStringList lines;
    for (int r = minRow; r <= maxRow; ++r)
    {
        QStringList parts;
        for (int c = minCol; c <= maxCol; ++c)
        {
            auto* it = m_table->item(r, c);
            parts << (it ? it->text() : QString());
        }
        lines << parts.join(QLatin1Char('\t'));
    }
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

// ==========================================================================
// Paste from clipboard → selected cells / rows
// ==========================================================================

void PresetTableWidget::pasteValueToItem(QTableWidgetItem* item, const QString& raw)
{
    int valCol = item->column() - 1;
    if (valCol < 0 || valCol >= m_columns.size()) return;

    const PTColumn& col = m_columns[valCol];
    int dmxVal = 0;

    if (col.type == PTColumn::Dropdown && !col.options.isEmpty())
    {
        // Try name match first, then numeric fallback
        bool found = false;
        for (const PTOption& opt : col.options)
        {
            if (opt.name.compare(raw, Qt::CaseInsensitive) == 0)
                { dmxVal = opt.value; found = true; break; }
        }
        if (!found) dmxVal = qBound(0, raw.toInt(), 255);

        QString disp = col.options[0].name;
        for (const PTOption& opt : col.options)
            if (opt.value == uchar(dmxVal)) { disp = opt.name; break; }
        item->setText(disp);
    }
    else
    {
        dmxVal = qBound(0, raw.toInt(), 255);
        item->setText(QString::number(dmxVal));
    }

    item->setData(Qt::UserRole, dmxVal);

    // Sync directly into m_rows (already under rebuildingTable guard)
    int row = item->row();
    QMutexLocker lk(&m_stateMutex);
    if (row < m_rows.size() && valCol < m_rows[row].values.size())
        m_rows[row].values[valCol] = uchar(dmxVal);
}

void PresetTableWidget::syncAllDataFromTable()
{
    QMutexLocker lk(&m_stateMutex);
    bool syncNames = (mode() == Doc::Design);
    for (int r = 0; r < m_table->rowCount() && r < m_rows.size(); ++r)
    {
        if (syncNames)
        {
            auto* nameItem = m_table->item(r, 0);
            if (nameItem) m_rows[r].name = nameItem->text();
        }

        for (int c = 0; c < m_columns.size(); ++c)
        {
            auto* item = m_table->item(r, c + 1);
            if (item && c < m_rows[r].values.size())
                m_rows[r].values[c] = uchar(item->data(Qt::UserRole).toInt());
        }
    }
}

void PresetTableWidget::slotPasteSelection()
{
    QString text = QApplication::clipboard()->text().trimmed();
    if (text.isEmpty()) return;

    QList<QTableWidgetItem*> sel = m_table->selectedItems();
    if (sel.isEmpty()) return;

    QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    m_table->blockSignals(true);
    m_rebuildingTable = true;

    // Single value → paste to ALL selected non-name cells
    bool singleValue = (lines.size() == 1 && !lines[0].contains(QLatin1Char('\t')));
    if (singleValue)
    {
        QString raw = lines[0].trimmed();
        for (auto* item : sel)
        {
            if (item->column() == 0) continue;  // never overwrite row name
            pasteValueToItem(item, raw);
        }
    }
    else
    {
        // Multi-row paste: anchor at top-left of selection
        int anchorRow = sel.first()->row();
        int anchorCol = sel.first()->column();
        for (auto* it : sel)
        {
            anchorRow = qMin(anchorRow, it->row());
            anchorCol = qMin(anchorCol, it->column());
        }

        for (int li = 0; li < lines.size(); ++li)
        {
            int r = anchorRow + li;
            if (r >= m_table->rowCount()) break;
            QStringList cols = lines[li].split(QLatin1Char('\t'));
            for (int ci = 0; ci < cols.size(); ++ci)
            {
                int c = anchorCol + ci;
                if (c >= m_table->columnCount()) break;
                if (c == 0) continue;  // skip Name column
                auto* item = m_table->item(r, c);
                if (item) pasteValueToItem(item, cols[ci].trimmed());
            }
        }
    }

    m_rebuildingTable = false;
    m_table->blockSignals(false);
    m_doc->setModified();
}

// ==========================================================================
// Table rebuild from data
// ==========================================================================

void PresetTableWidget::rebuildTable()
{
    m_rebuildingTable = true;

    m_table->blockSignals(true);

    int numCols = 1 + m_columns.size();
    int numRows = m_rows.size();

    m_table->setRowCount(0);
    m_table->setColumnCount(numCols);

    // Header
    QStringList headers;
    headers << tr("Name");
    for (const PTColumn& c : m_columns)
        headers << c.name;
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setRowCount(numRows);

    for (int r = 0; r < numRows; ++r)
    {
        const PTRow& row = m_rows[r];

        // Name column — always plain text item
        QTableWidgetItem* nameItem = new QTableWidgetItem(row.name);
        if (mode() == Doc::Design)
            nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        else
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(r, 0, nameItem);

        // Value columns
        for (int c = 0; c < m_columns.size(); ++c)
        {
            uchar dmxVal = (c < row.values.size()) ? row.values[c] : 0;
            const PTColumn& ptcol = m_columns[c];

            QString displayText;
            if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
            {
                // Find matching option name
                displayText = ptcol.options[0].name;
                for (const PTOption& opt : ptcol.options)
                    if (opt.value == dmxVal) { displayText = opt.name; break; }
            }
            else
            {
                displayText = QString::number(dmxVal);
            }

            QTableWidgetItem* item = new QTableWidgetItem(displayText);
            item->setData(Qt::UserRole, int(dmxVal));
            m_table->setItem(r, c + 1, item);
        }
    }

    m_table->blockSignals(false);
    m_rebuildingTable = false;

    refreshRowHighlights();
}

// ==========================================================================
// Cell changed — sync back to m_rows
// ==========================================================================

void PresetTableWidget::slotCellChanged(int row, int col)
{
    if (m_rebuildingTable) return;
    if (row < 0 || row >= m_rows.size()) return;

    QTableWidgetItem* item = m_table->item(row, col);
    if (!item) return;

    QMutexLocker lk(&m_stateMutex);

    if (col == 0)
    {
        if (mode() != Doc::Design) return;
        m_rows[row].name = item->text();
    }
    else
    {
        int valCol = col - 1;
        if (valCol < m_rows[row].values.size())
            m_rows[row].values[valCol] = uchar(item->data(Qt::UserRole).toInt());
    }
}

// ==========================================================================
// Row highlights (Operate: badge with output number)
// ==========================================================================

void PresetTableWidget::setActiveRow(int outputIdx, int rowIdx)
{
    {
        QMutexLocker lk(&m_stateMutex);
        if (outputIdx < 0 || outputIdx >= m_activeRow.size()) return;
        m_activeRow[outputIdx] = rowIdx;
    }
    refreshRowHighlights();
    sendFeedback(rowIdx + 1, quint8(outputIdx));   // 0 = off when rowIdx == -1 → gives 0
    update();
}

void PresetTableWidget::refreshRowHighlights()
{
    if (mode() != Doc::Operate) return;

    m_table->blockSignals(true);

    int numRows = m_table->rowCount();

    // Clear all Name-column items' background first
    for (int r = 0; r < numRows; ++r)
    {
        QTableWidgetItem* it = m_table->item(r, 0);
        if (it)
        {
            it->setBackground(QBrush());
            it->setForeground(QBrush());
            it->setText(m_rows[r].name);
        }
    }

    // Build per-row display: collect outputs active on each row
    QMap<int, QStringList> rowOutputLabels;

    QMutexLocker lk(&m_stateMutex);
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        if (ar >= 0 && ar < numRows)
            rowOutputLabels[ar].append(QString::number(o + 1));
    }
    lk.unlock();

    for (auto it = rowOutputLabels.constBegin(); it != rowOutputLabels.constEnd(); ++it)
    {
        int r = it.key();
        QTableWidgetItem* item = m_table->item(r, 0);
        if (!item) continue;

        // Use first output's color as row tint
        QColor tint = s_outputColors[0];
        // Find first output index mapped to this row
        QMutexLocker lk2(&m_stateMutex);
        for (int o = 0; o < m_activeRow.size(); ++o)
            if (m_activeRow[o] == r) { tint = s_outputColors[o % 8]; break; }
        lk2.unlock();

        QColor bg = tint;
        bg.setAlpha(60);
        item->setBackground(bg);

        QString badges = it.value().join(QLatin1Char(' '));
        item->setText(QString("[%1] %2").arg(badges, m_rows[r].name));
    }

    // Update status bar
    QStringList parts;
    QMutexLocker lk3(&m_stateMutex);
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        QString outName = (o < m_outputs.size()) ? m_outputs[o].name : tr("Out%1").arg(o+1);
        if (ar < 0)
            parts.append(QString("%1: off").arg(outName));
        else if (ar < m_rows.size())
            parts.append(QString("%1: %2").arg(outName, m_rows[ar].name));
    }
    lk3.unlock();
    if (m_statusBar)
        m_statusBar->setText(parts.join(QLatin1String("    ")));

    m_table->blockSignals(false);
}

// ==========================================================================
// writeDMX (MasterTimer thread)
// ==========================================================================

void PresetTableWidget::writeDMX(MasterTimer* /*timer*/, QList<Universe*> universes)
{
    QMutexLocker lk(&m_stateMutex);

    const bool  xfEnabled  = m_crossfadeEnabled;
    const uchar xfPos      = m_crossfadeGlobalPos;
    const uchar xfStartPos = m_crossfadeStartPos;

    // Direction-locked: compute effectivePos (0=A, 255=B) from how far fader
    // has traveled from startPos toward the target (furthest extreme)
    const uchar xfTarget = (xfStartPos < 128) ? 255 : 0;
    qint16 xfTraveled, xfMaxTravel;
    if (xfTarget == 255) {
        xfTraveled   = qint16(xfPos) - qint16(xfStartPos);
        xfMaxTravel  = qint16(255)   - qint16(xfStartPos);
    } else {
        xfTraveled   = qint16(xfStartPos) - qint16(xfPos);
        xfMaxTravel  = qint16(xfStartPos);
    }
    if (xfMaxTravel <= 0) xfMaxTravel = 1;
    const uchar xfEffective = uchar(qBound(qint16(0), qint16(xfTraveled * 255 / xfMaxTravel), qint16(255)));

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int activeRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const int stagedRow = (o < m_stagedRow.size()) ? m_stagedRow[o] : -1;

        if (activeRow < 0) continue;

        const PTOutput& out = m_outputs[o];
        if (out.fixtureId == UINT_MAX) continue;

        Fixture* fxi = m_doc->fixture(out.fixtureId);
        if (!fxi) continue;
        if ((int)fxi->channels() != m_columns.size()) continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size()) continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        if (activeRow >= m_rows.size()) continue;

        for (int c = 0; c < m_columns.size(); ++c)
        {
            FadeChannel* fc = fader->getChannelFader(
                m_doc, universes[uni], out.fixtureId, quint32(c));

            if (fc->universe() == Universe::invalid())
            {
                fader->remove(fc);
                continue;
            }

            const QVector<uchar>& aVals = m_rows[activeRow].values;
            uchar aVal = (c < aVals.size()) ? aVals[c] : 0;

            if (!xfEnabled || stagedRow < 0)
            {
                // Direct path: instant output of current row
                fc->setStart(fc->current());
                fc->setTarget(aVal);
                fc->setReady(false);
                fc->setElapsed(0);
            }
            else
            {
                // Crossfade path: blend A → B using effectivePos
                uchar bVal = aVal;
                if (stagedRow < m_rows.size())
                {
                    const QVector<uchar>& bVals = m_rows[stagedRow].values;
                    bVal = (c < bVals.size()) ? bVals[c] : 0;
                }

                uchar finalVal;
                if (m_columns[c].fade)
                    finalVal = uchar(aVal + qint16(bVal - aVal) * qint16(xfEffective) / 255);
                else
                    finalVal = (xfEffective > 127) ? bVal : aVal;

                fc->setStart(finalVal);
                fc->setTarget(finalVal);
                fc->setFadeTime(0);
                fc->setReady(false);
                fc->setElapsed(0);
            }
        }
    }
}

// ==========================================================================
// External input
// ==========================================================================

void PresetTableWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (!acceptsInput()) return;

    quint32 pagedCh = (page() << 16) | channel;

    QMutexLocker lk(&m_stateMutex);
    int numOutputs        = m_outputs.size();
    int numRows           = m_rows.size();
    bool xfEnabled        = m_crossfadeEnabled;
    lk.unlock();

    // Global crossfade position (ID = 255)
    if (checkInputSource(universe, pagedCh, value, sender(), quint8(255)))
    {
        QMutexLocker lk2(&m_stateMutex);
        m_crossfadeGlobalPos = value;

        // Direction-locked: auto-promote when fader reaches the target extreme
        const uchar target = (m_crossfadeStartPos < 128) ? 255 : 0;
        if (value == target)
        {
            for (int o = 0; o < m_activeRow.size(); ++o)
            {
                if (o < m_stagedRow.size() && m_stagedRow[o] >= 0)
                {
                    m_activeRow[o] = m_stagedRow[o];
                    m_stagedRow[o] = -1;
                }
            }
        }
        lk2.unlock();
        refreshRowHighlights();
        return;
    }

    // Per-output row selector (ID = o)
    for (int o = 0; o < numOutputs; ++o)
    {
        if (checkInputSource(universe, pagedCh, value, sender(), quint8(o)))
        {
            int rowIdx = (value == 0) ? -1 : qMin<int>(int(value) - 1, numRows - 1);
            if (xfEnabled)
            {
                // Crossfade mode: selector sets staged row
                // If selecting the currently active row, clear staging instead
                QMutexLocker lk2(&m_stateMutex);
                if (o < m_stagedRow.size())
                {
                    if (rowIdx == m_activeRow[o])
                    {
                        m_stagedRow[o] = -1;  // selector = current: clear staging
                    }
                    else
                    {
                        // Snapshot fader position only when this is the FIRST staging
                        bool wasAnyStaged = false;
                        for (int i = 0; i < m_stagedRow.size(); ++i)
                            if (m_stagedRow[i] >= 0) { wasAnyStaged = true; break; }

                        m_stagedRow[o] = rowIdx;

                        if (!wasAnyStaged)
                            m_crossfadeStartPos = m_crossfadeGlobalPos;
                    }
                }
                lk2.unlock();
                refreshRowHighlights();
            }
            else
            {
                // Normal mode: selector sets current row directly
                setActiveRow(o, rowIdx);
            }
            return;
        }
    }
}

void PresetTableWidget::updateFeedback()
{
    QMutexLocker lk(&m_stateMutex);
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        sendFeedback(ar < 0 ? 0 : ar + 1, quint8(o));
    }
}

// ==========================================================================
// Properties dialog
// ==========================================================================

void PresetTableWidget::editProperties()
{
    if (mode() != Doc::Design) return;

    syncAllDataFromTable();

    QVector<PTColumn> colsCopy;
    QVector<PTOutput>  outsCopy;
    {
        QMutexLocker lk(&m_stateMutex);
        colsCopy = m_columns;
        outsCopy = m_outputs;
    }

    // Collect current input sources per output
    QVector<QSharedPointer<QLCInputSource>> srcsCopy;
    for (int o = 0; o < outsCopy.size(); ++o)
        srcsCopy.append(inputSource(quint8(o)));

    bool xfEnabled;
    QSharedPointer<QLCInputSource> xfSrc;
    {
        QMutexLocker lk(&m_stateMutex);
        xfEnabled = m_crossfadeEnabled;
    }
    xfSrc = inputSource(quint8(255));

    PresetTableConfigDialog dlg(m_doc, colsCopy, outsCopy, srcsCopy, xfEnabled, xfSrc, page(), this);

    if (dlg.exec() != QDialog::Accepted) return;

    QVector<PTColumn> newCols = dlg.columns();
    QVector<PTOutput>  newOuts = dlg.outputs();

    {
        QMutexLocker lk(&m_stateMutex);

        // Adjust row values if column count changed
        int oldNumCols = m_columns.size();
        int newNumCols = newCols.size();
        m_columns = newCols;
        if (newNumCols != oldNumCols)
        {
            for (PTRow& row : m_rows)
                row.values.resize(newNumCols, 0);
        }

        m_outputs = newOuts;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
    }

    // Apply new input sources
    for (int o = 0; o < newOuts.size(); ++o)
        setInputSource(dlg.inputSource(o), quint8(o));

    // Global crossfade input + toggle
    setInputSource(dlg.crossfadeInputSource(), quint8(255));
    {
        QMutexLocker lk(&m_stateMutex);
        m_crossfadeEnabled = dlg.crossfadeEnabled();
        if (!m_crossfadeEnabled)
        {
            m_stagedRow.fill(-1, m_stagedRow.size());
            m_crossfadeGlobalPos = 0;
            m_crossfadeStartPos  = 0;
        }
    }

    // Clear old input sources beyond new output count
    // (sources at indices >= newOuts.size() can be left; VCWidget handles them)

    rebuildTable();
    m_doc->setModified();
}

// ==========================================================================
// createCopy
// ==========================================================================

VCWidget* PresetTableWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    PresetTableWidget* copy = new PresetTableWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }

    QVector<PTColumn> colsCopy;
    QVector<PTRow>    rowsCopy;
    QVector<PTOutput> outsCopy;
    QVector<int>      activeRowCopy;
    QVector<int>      stagedRowCopy;
    bool              xfEnabledCopy;
    uchar             xfPosCopy;
    uchar             xfStartPosCopy;

    {
        QMutexLocker lk(&m_stateMutex);
        colsCopy      = m_columns;
        rowsCopy      = m_rows;
        outsCopy      = m_outputs;
        activeRowCopy = m_activeRow;
        stagedRowCopy = m_stagedRow;
        xfEnabledCopy   = m_crossfadeEnabled;
        xfPosCopy       = m_crossfadeGlobalPos;
        xfStartPosCopy  = m_crossfadeStartPos;
    }

    {
        QMutexLocker lk2(&copy->m_stateMutex);
        copy->m_columns            = colsCopy;
        copy->m_rows               = rowsCopy;
        copy->m_outputs            = outsCopy;
        copy->m_activeRow          = activeRowCopy;
        copy->m_stagedRow          = stagedRowCopy;
        copy->m_crossfadeEnabled   = xfEnabledCopy;
        copy->m_crossfadeGlobalPos = xfPosCopy;
        copy->m_crossfadeStartPos  = xfStartPosCopy;
    }

    // Copy input sources
    for (int o = 0; o < outsCopy.size(); ++o)
        copy->setInputSource(inputSource(quint8(o)), quint8(o));
    copy->setInputSource(inputSource(quint8(255)), quint8(255));

    copy->rebuildTable();
    return copy;
}

// ==========================================================================
// paintEvent
// ==========================================================================

void PresetTableWidget::paintEvent(QPaintEvent* e)
{
    QPainter p(this);
    QColor bg = QColor(0x1c, 0x1c, 0x1c);
    p.fillRect(rect(), bg);
    p.end();
    VCWidget::paintEvent(e);
}

// ==========================================================================
// loadXML / saveXML
// ==========================================================================

bool PresetTableWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot) return false;

    loadXMLCommon(root);

    bool xfEnabled = (root.attributes().value(KXMLCrossfadeEn).toString() == QLatin1String("True"));

    QVector<PTColumn> cols;
    QVector<PTRow>    rows;
    QVector<PTOutput> outs;

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0; bool vis = true;
            loadXMLWindowState(root, &x, &y, &w, &h, &vis);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
        {
            loadXMLAppearance(root);
        }
        else if (root.name() == KXMLColumn)
        {
            auto attrs = root.attributes();
            PTColumn col;
            col.name = attrs.value(KXMLColName).toString();
            col.type = (attrs.value(KXMLColType).toString() == QLatin1String("Dropdown"))
                        ? PTColumn::Dropdown : PTColumn::Numeric;
            col.fade = (attrs.value(KXMLColFade).toString() != QLatin1String("False"));

            // Read child <Option> elements
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLOption)
                {
                    PTOption opt;
                    opt.name  = root.attributes().value(KXMLOptName).toString();
                    opt.value = uchar(root.attributes().value(KXMLOptValue).toUInt());
                    col.options.append(opt);
                    root.skipCurrentElement();
                }
                else
                {
                    root.skipCurrentElement();
                }
            }
            cols.append(col);
        }
        else if (root.name() == KXMLRow)
        {
            auto attrs = root.attributes();
            PTRow row;
            row.name = attrs.value(KXMLRowName).toString();

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLV)
                    row.values.append(uchar(root.readElementText().toUInt()));
                else
                    root.skipCurrentElement();
            }
            rows.append(row);
        }
        else if (root.name() == KXMLOutput)
        {
            auto attrs = root.attributes();
            int idx = attrs.value(KXMLOutIndex).toInt();
            PTOutput out;
            out.name      = attrs.value(KXMLOutName).toString();
            out.fixtureId = attrs.value(KXMLOutFxId).toUInt();

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLOutInput)
                    loadXMLSources(root, quint8(idx));
                else
                    root.skipCurrentElement();
            }
            // Grow outs vector to fit index
            while (outs.size() <= idx) outs.append(PTOutput());
            outs[idx] = out;
        }
        else if (root.name() == KXMLCrossfadeInput)
        {
            loadXMLSources(root, quint8(255));
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns = cols;
        m_rows    = rows;
        // Ensure row values are correct size
        for (PTRow& r : m_rows)
            r.values.resize(m_columns.size(), 0);
        m_outputs = outs;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
        m_crossfadeEnabled   = xfEnabled;
        m_crossfadeGlobalPos = 0;
        m_crossfadeStartPos  = 0;
    }

    rebuildTable();
    return true;
}

bool PresetTableWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    syncAllDataFromTable();

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);
    {
        QMutexLocker lk2(&m_stateMutex);
        doc->writeAttribute(KXMLCrossfadeEn, m_crossfadeEnabled ? QLatin1String("True") : QLatin1String("False"));
    }

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    QMutexLocker lk(&m_stateMutex);

    for (int c = 0; c < m_columns.size(); ++c)
    {
        const PTColumn& col = m_columns[c];
        doc->writeStartElement(KXMLColumn);
        doc->writeAttribute(KXMLColIndex, QString::number(c));
        doc->writeAttribute(KXMLColName,  col.name);
        doc->writeAttribute(KXMLColType,
            col.type == PTColumn::Dropdown ? QLatin1String("Dropdown") : QLatin1String("Numeric"));
        doc->writeAttribute(KXMLColFade,  col.fade ? QLatin1String("True") : QLatin1String("False"));

        for (const PTOption& opt : col.options)
        {
            doc->writeStartElement(KXMLOption);
            doc->writeAttribute(KXMLOptName,  opt.name);
            doc->writeAttribute(KXMLOptValue, QString::number(opt.value));
            doc->writeEndElement();
        }
        doc->writeEndElement();  // Column
    }

    for (int r = 0; r < m_rows.size(); ++r)
    {
        const PTRow& row = m_rows[r];
        doc->writeStartElement(KXMLRow);
        doc->writeAttribute(KXMLRowIndex, QString::number(r));
        doc->writeAttribute(KXMLRowName,  row.name);
        for (uchar v : row.values)
            doc->writeTextElement(KXMLV, QString::number(v));
        doc->writeEndElement();  // Row
    }

    // Collect output data under lock, then write outside
    struct OutData { QString name; quint32 fixtureId; };
    QVector<OutData> outData;
    outData.reserve(m_outputs.size());
    for (const PTOutput& out : m_outputs)
        outData.append({out.name, out.fixtureId});

    lk.unlock();

    for (int o = 0; o < outData.size(); ++o)
    {
        doc->writeStartElement(KXMLOutput);
        doc->writeAttribute(KXMLOutIndex, QString::number(o));
        doc->writeAttribute(KXMLOutName,  outData[o].name);
        doc->writeAttribute(KXMLOutFxId,  QString::number(outData[o].fixtureId));

        auto src = inputSource(quint8(o));
        if (!src.isNull() && src->isValid())
        {
            doc->writeStartElement(KXMLOutInput);
            saveXMLInput(doc, src);
            doc->writeEndElement();
        }

        doc->writeEndElement();  // Output
    }

    // Global crossfade input
    auto xfSrc = inputSource(quint8(255));
    if (!xfSrc.isNull() && xfSrc->isValid())
    {
        doc->writeStartElement(KXMLCrossfadeInput);
        saveXMLInput(doc, xfSrc);
        doc->writeEndElement();
    }

    doc->writeEndElement();  // PluginWidget
    return true;
}

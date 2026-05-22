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
#include "fixturegroup.h"
#include "grouphead.h"
#include "qlcpoint.h"
#include "qlcchannel.h"
#include "qlccapability.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "qlcinputsource.h"
#include "doc.h"

#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutexLocker>
#include <QSpinBox>
#include <QComboBox>
#include <QStyleOptionViewItem>
#include <QHeaderView>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
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
static const QString KXMLOptName     = QStringLiteral("Name");
static const QString KXMLOptValue    = QStringLiteral("Value");
static const QString KXMLOptResource = QStringLiteral("Resource");
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
static const QString KXMLColWidth        = QStringLiteral("Width");
static const QString KXMLNameColWidth    = QStringLiteral("NameColWidth");

// FixtureGroup mode XML constants
static const QString KXMLMode           = QStringLiteral("Mode");
static const QString KXMLFxGroupId      = QStringLiteral("FixtureGroupID");
static const QString KXMLOutRows        = QStringLiteral("Rows");
static const QString KXMLBindMfg        = QStringLiteral("BindMfg");
static const QString KXMLBindModel      = QStringLiteral("BindModel");
static const QString KXMLBindMode       = QStringLiteral("BindMode");
static const QString KXMLBindChan       = QStringLiteral("BindChan");
static const QString KXMLColScalerMin   = QStringLiteral("ScalerMin");
static const QString KXMLColScalerMax   = QStringLiteral("ScalerMax");
static const QString KXMLColScalerSfx   = QStringLiteral("ScalerSuffix");

// ==========================================================================
// Static display helpers (shared by delegate + rebuildTable + pasteValueToItem)
// ==========================================================================

// Returns (displayText, resource) for a Dropdown column given a raw DMX value.
// Exact match in options → that option's name + resource.
// No match → bare number as string + empty resource.
static QPair<QString,QString> optionLabelFor(const PTColumn& col, int dmxVal)
{
    for (const PTOption& opt : col.options)
        if (int(opt.value) == dmxVal)
            return { opt.name, opt.resource };
    return { QString::number(dmxVal), QString() };
}

// Builds a 16×16 QIcon from a resource string ("#rrggbb" colour or image path).
static QIcon makeItemIcon(const QString& resource)
{
    if (resource.isEmpty()) return QIcon();
    QPixmap pm;
    if (resource.startsWith(QLatin1Char('#')))
    {
        pm = QPixmap(16, 16);
        pm.fill(QColor(resource));
    }
    else
    {
        pm.load(resource);
        if (!pm.isNull())
            pm = pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return pm.isNull() ? QIcon() : QIcon(pm);
}

// Scaler ↔ DMX conversion
static inline int dmxToScaler(int dmx, int sMin, int sMax)
{
    return sMin + qRound(dmx * double(sMax - sMin) / 255.0);
}
static inline int scalerToDmx(int v, int sMin, int sMax)
{
    if (sMax == sMin) return 0;
    return qBound(0, qRound((v - sMin) * 255.0 / (sMax - sMin)), 255);
}

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

void PresetTableDelegate::setOwner(const PresetTableWidget* owner)
{
    m_owner = owner;
}

QWidget* PresetTableDelegate::createEditor(QWidget* parent,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
        return QStyledItemDelegate::createEditor(parent, option, index);

    int valCol = col - 1;  // value column index
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return nullptr;

    const PTColumn& ptcol = (*m_columns)[valCol];

    // Scaler: dedicated range spinbox with optional suffix
    if (ptcol.type == PTColumn::Scaler)
    {
        QSpinBox* sb = new QSpinBox(parent);
        sb->setRange(ptcol.scalerMin, ptcol.scalerMax);
        if (!ptcol.scalerSuffix.isEmpty())
            sb->setSuffix(ptcol.scalerSuffix);
        sb->setFrame(false);
        return sb;
    }

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = new QComboBox(parent);
        cb->setEditable(true);
        cb->lineEdit()->setValidator(new QIntValidator(0, 255, cb));
        cb->setInsertPolicy(QComboBox::NoInsert);
        for (const PTOption& opt : ptcol.options)
            cb->addItem(makeItemIcon(opt.resource), opt.name, int(opt.value));
        return cb;
    }

    // Capability-aware editable combo: only in Dropdown mode, with a resolved channel
    const QLCChannel* chan = (ptcol.type == PTColumn::Dropdown && m_owner)
                             ? m_owner->resolveBoundChannel(ptcol) : nullptr;
    if (chan && !chan->capabilities().isEmpty())
    {
        QComboBox* cb = new QComboBox(parent);
        cb->setEditable(true);
        cb->lineEdit()->setValidator(new QIntValidator(0, 255, cb));
        cb->setInsertPolicy(QComboBox::NoInsert);
        for (QLCCapability* cap : chan->capabilities())
        {
            QString label = QString("%1-%2: %3")
                .arg(int(cap->min()), 3, 10, QChar('0'))
                .arg(int(cap->max()), 3, 10, QChar('0'))
                .arg(cap->name());
            cb->addItem(label, int(cap->min()));
        }
        return cb;
    }

    // Fallback: plain numeric spinbox
    QSpinBox* sb = new QSpinBox(parent);
    sb->setRange(0, 255);
    sb->setFrame(false);
    return sb;
}

void PresetTableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
    {
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    }

    int valCol = col - 1;
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return;

    const PTColumn& ptcol = (*m_columns)[valCol];
    int stored = index.data(Qt::UserRole).toInt();

    if (ptcol.type == PTColumn::Scaler)
    {
        QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
        if (sb) sb->setValue(dmxToScaler(stored, ptcol.scalerMin, ptcol.scalerMax));
        return;
    }

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        // Pre-select the matching option (for visual context)
        for (int i = 0; i < cb->count(); ++i)
        {
            if (cb->itemData(i).toInt() == stored)
            {
                cb->setCurrentIndex(i);
                break;
            }
        }
        // Show exact DMX value so user sees / edits the precise number
        cb->lineEdit()->setText(QString::number(stored));
        return;
    }

    // Capability editable combo: show the exact stored DMX value in the line edit
    QComboBox* cb = qobject_cast<QComboBox*>(editor);
    if (cb && cb->isEditable())
    {
        for (int i = 0; i < cb->count(); ++i)
        {
            int capMin = cb->itemData(i).toInt();
            int capMax = (i + 1 < cb->count()) ? cb->itemData(i + 1).toInt() - 1 : 255;
            if (stored >= capMin && stored <= capMax)
            {
                cb->setCurrentIndex(i);
                break;
            }
        }
        cb->lineEdit()->setText(QString::number(stored));
        return;
    }

    QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
    if (sb) sb->setValue(stored);
}

void PresetTableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                        const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
    {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    int valCol = col - 1;
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return;

    const PTColumn& ptcol = (*m_columns)[valCol];

    // ---- Scaler --------------------------------------------------------
    if (ptcol.type == PTColumn::Scaler)
    {
        QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
        if (!sb) return;
        int dmx = scalerToDmx(sb->value(), ptcol.scalerMin, ptcol.scalerMax);
        model->setData(index, dmx, Qt::UserRole);
        model->setData(index,
            QString("%1%2").arg(sb->value()).arg(ptcol.scalerSuffix),
            Qt::DisplayRole);
        model->setData(index, QVariant(), Qt::DecorationRole);
        return;
    }

    // ---- Dropdown with PTOption list -----------------------------------
    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        bool ok = false;
        int val = cb->lineEdit()->text().toInt(&ok);
        if (!ok) val = cb->currentData().toInt();
        val = qBound(0, val, 255);

        auto lbl = optionLabelFor(ptcol, val);
        model->setData(index, val, Qt::UserRole);
        model->setData(index, lbl.first, Qt::DisplayRole);
        QIcon ico = makeItemIcon(lbl.second);
        model->setData(index, ico.isNull() ? QVariant() : QVariant(ico), Qt::DecorationRole);
        return;
    }

    // ---- Capability editable combo (no PTOption, binding has caps) -----
    QComboBox* cb = qobject_cast<QComboBox*>(editor);
    if (cb && cb->isEditable())
    {
        bool ok = false;
        int val = cb->lineEdit()->text().toInt(&ok);
        if (!ok) val = cb->currentData().toInt();
        val = qBound(0, val, 255);

        const QLCChannel* chan = m_owner ? m_owner->resolveBoundChannel(ptcol) : nullptr;
        QString display = QString::number(val);
        if (chan)
        {
            QLCCapability* cap = chan->searchCapability(uchar(val));
            if (cap) display = cap->name();
        }
        model->setData(index, val, Qt::UserRole);
        model->setData(index, display, Qt::DisplayRole);
        model->setData(index, QVariant(), Qt::DecorationRole);
        return;
    }

    // ---- Numeric spinbox -----------------------------------------------
    QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
    if (!sb) return;
    model->setData(index, sb->value(), Qt::UserRole);
    model->setData(index, QString::number(sb->value()), Qt::DisplayRole);
    model->setData(index, QVariant(), Qt::DecorationRole);
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
    QIcon   ico  = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

    const int iconSize = 16;
    const int iconPad  = 2;
    int textLeft = 4;

    if (!ico.isNull())
    {
        QPixmap pm = ico.pixmap(iconSize, iconSize);
        int y = option.rect.top() + (option.rect.height() - pm.height()) / 2;
        painter->drawPixmap(option.rect.left() + 2, y, pm);
        textLeft = 2 + iconSize + iconPad + 2;
    }

    painter->setPen(option.palette.color(QPalette::Text));
    painter->drawText(option.rect.adjusted(textLeft, 0, -2, 0),
                      Qt::AlignVCenter | Qt::AlignLeft, text);
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

    QAction* actAddRow    = m_toolbar->addAction(tr("+ Row"));
    QAction* actRemRow    = m_toolbar->addAction(tr("- Row"));
    m_actColSep  = m_toolbar->addSeparator();
    QAction* actAddCol = m_toolbar->addAction(tr("+ Col"));
    QAction* actRemCol = m_toolbar->addAction(tr("- Col"));
    m_actPropSep = m_toolbar->addSeparator();
    QAction* actProps  = m_toolbar->addAction(tr("Properties..."));

    m_actAddCol = actAddCol;
    m_actRemCol = actRemCol;
    m_actProps  = actProps;

    connect(actAddRow, &QAction::triggered, this, &PresetTableWidget::slotAddRow);
    connect(actRemRow, &QAction::triggered, this, &PresetTableWidget::slotRemoveRow);
    connect(actAddCol, &QAction::triggered, this, &PresetTableWidget::slotAddColumn);
    connect(actRemCol, &QAction::triggered, this, &PresetTableWidget::slotRemoveColumn);
    connect(actProps,  &QAction::triggered, this, &PresetTableWidget::slotProperties);

    m_layout->addWidget(m_toolbar);

    // ---- Table -----------------------------------------------------------
    m_delegate = new PresetTableDelegate(this);
    m_delegate->setColumns(&m_columns);
    m_delegate->setOwner(this);

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

    // Intercept ShortcutOverride so Ctrl+C/V reach us instead of VC's widget-copy actions
    m_table->installEventFilter(this);
    m_table->viewport()->installEventFilter(this);

    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_table, &QTableWidget::cellChanged,
            this, &PresetTableWidget::slotCellChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &PresetTableWidget::slotColumnHeaderDoubleClicked);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized,
            this, &PresetTableWidget::slotHeaderSectionResized);
    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &PresetTableWidget::slotTableContextMenu);

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
        m_toolbar->setVisible(true);
        // Hide structural actions not appropriate during a live show
        if (m_actAddCol)    m_actAddCol->setVisible(false);
        if (m_actRemCol)    m_actRemCol->setVisible(false);
        if (m_actProps)     m_actProps->setVisible(false);
        if (m_actColSep)    m_actColSep->setVisible(false);
        if (m_actPropSep)   m_actPropSep->setVisible(false);
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_statusBar->setVisible(true);
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else
    {
        m_toolbar->setVisible(true);
        if (m_actAddCol)    m_actAddCol->setVisible(true);
        if (m_actRemCol)    m_actRemCol->setVisible(true);
        if (m_actProps)     m_actProps->setVisible(true);
        if (m_actColSep)    m_actColSep->setVisible(true);
        if (m_actPropSep)   m_actPropSep->setVisible(true);
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

    PTMode colMode;
    quint32 groupId;
    {
        QMutexLocker lk(&m_stateMutex);
        colMode = m_mode;
        groupId = m_fixtureGroupId;
    }

    FixtureGroup* grp = (colMode == PTMode::FixtureGroup)
        ? m_doc->fixtureGroup(groupId) : nullptr;

    PresetTableColumnDialog dlg(m_doc, m_columns[valCol], colMode, grp, this);
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
    if (m_table && (obj == m_table || obj == m_table->viewport()))
    {
        // Accept ShortcutOverride to prevent VirtualConsole's Ctrl+C/V QActions
        // from firing the widget-copy menu instead of our cell copy/paste.
        if (ev->type() == QEvent::ShortcutOverride)
        {
            auto* ke = static_cast<QKeyEvent*>(ev);
            if (ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::Paste))
            {
                ke->accept();
                return true;
            }
        }
        if (ev->type() == QEvent::KeyPress)
        {
            auto* ke = static_cast<QKeyEvent*>(ev);
            if (ke->matches(QKeySequence::Copy))  { slotCopySelection();  return true; }
            if (ke->matches(QKeySequence::Paste)) { slotPasteSelection(); return true; }
        }
    }
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

        auto lbl = optionLabelFor(col, dmxVal);
        item->setText(lbl.first);
        item->setIcon(makeItemIcon(lbl.second));
    }
    else if (col.type == PTColumn::Scaler)
    {
        // Accept either a scaler value or raw DMX
        bool ok = false;
        int sv = raw.toInt(&ok);
        if (ok)
            dmxVal = scalerToDmx(sv, col.scalerMin, col.scalerMax);
        else
            dmxVal = qBound(0, raw.toInt(), 255);
        int displayed = dmxToScaler(dmxVal, col.scalerMin, col.scalerMax);
        item->setText(QString("%1%2").arg(displayed).arg(col.scalerSuffix));
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

void PresetTableWidget::slotTableContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* copyAct  = menu.addAction(tr("Copy cells"));
    QAction* pasteAct = menu.addAction(tr("Paste cells"));
    copyAct->setShortcut(QKeySequence::Copy);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(!QApplication::clipboard()->text().isEmpty());

    QAction* chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == copyAct)  slotCopySelection();
    if (chosen == pasteAct) slotPasteSelection();
}

void PresetTableWidget::syncAllDataFromTable()
{
    // Capture column widths from GUI before taking the mutex
    int nameColW = m_table->columnWidth(0);
    QVector<int> colWidths(m_columns.size());
    for (int c = 0; c < m_columns.size(); ++c)
        colWidths[c] = m_table->columnWidth(c + 1);

    QMutexLocker lk(&m_stateMutex);
    bool syncNames = true;
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

    // Persist current column widths
    m_nameColWidth = (nameColW > 0) ? nameColW : -1;
    for (int c = 0; c < m_columns.size() && c < colWidths.size(); ++c)
        m_columns[c].width = (colWidths[c] > 0) ? colWidths[c] : -1;
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
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        m_table->setItem(r, 0, nameItem);

        // Value columns
        for (int c = 0; c < m_columns.size(); ++c)
        {
            uchar dmxVal = (c < row.values.size()) ? row.values[c] : 0;
            const PTColumn& ptcol = m_columns[c];

            QString displayText;
            QIcon   displayIcon;
            if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
            {
                auto lbl = optionLabelFor(ptcol, int(dmxVal));
                displayText = lbl.first;
                displayIcon = makeItemIcon(lbl.second);
            }
            else if (ptcol.type == PTColumn::Scaler)
            {
                int sv = dmxToScaler(int(dmxVal), ptcol.scalerMin, ptcol.scalerMax);
                displayText = QString("%1%2").arg(sv).arg(ptcol.scalerSuffix);
            }
            else
            {
                displayText = QString::number(dmxVal);
            }

            QTableWidgetItem* item = new QTableWidgetItem(displayText);
            item->setData(Qt::UserRole, int(dmxVal));
            if (!displayIcon.isNull())
                item->setIcon(displayIcon);
            m_table->setItem(r, c + 1, item);
        }
    }

    // Apply persisted column widths
    if (m_nameColWidth > 0)
        m_table->setColumnWidth(0, m_nameColWidth);
    for (int c = 0; c < m_columns.size(); ++c)
        if (m_columns[c].width > 0)
            m_table->setColumnWidth(c + 1, m_columns[c].width);

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
        // refreshRowHighlights uses blockSignals so badge text never reaches here
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
// writeDMX helpers
// ==========================================================================

// Apply a DMX value to a fade channel, with optional crossfade blending.
// aVal = active row value, bVal = staged row value (ignored when !xfEnabled or stagedRow < 0)
static void applyFadeValue(GenericFader* fader, Doc* doc, Universe* uni,
                            quint32 fxiId, quint32 chanIdx,
                            uchar aVal, uchar bVal,
                            bool xfEnabled, bool hasStaged,
                            bool colFade, uchar xfEffective)
{
    FadeChannel* fc = fader->getChannelFader(doc, uni, fxiId, chanIdx);
    if (fc->universe() == Universe::invalid())
    {
        fader->remove(fc);
        return;
    }

    if (!xfEnabled || !hasStaged)
    {
        fc->setStart(fc->current());
        fc->setTarget(aVal);
        fc->setReady(false);
        fc->setElapsed(0);
    }
    else
    {
        uchar finalVal;
        if (colFade)
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

void PresetTableWidget::writeDMXLegacy(QList<Universe*>& universes, uchar xfEffective)
{
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

        const QVector<uchar>& aVals = m_rows[activeRow].values;
        const QVector<uchar>* bVals = (stagedRow >= 0 && stagedRow < m_rows.size())
            ? &m_rows[stagedRow].values : nullptr;

        for (int c = 0; c < m_columns.size(); ++c)
        {
            uchar aVal = (c < aVals.size()) ? aVals[c] : 0;
            uchar bVal = (bVals && c < bVals->size()) ? (*bVals)[c] : aVal;
            applyFadeValue(fader.data(), m_doc, universes[uni],
                           out.fixtureId, quint32(c),
                           aVal, bVal,
                           m_crossfadeEnabled, (stagedRow >= 0),
                           m_columns[c].fade, xfEffective);
        }
    }
}

const QLCChannel* PresetTableWidget::resolveBoundChannel(const PTColumn& col) const
{
    if (m_mode != PTMode::FixtureGroup) return nullptr;
    if (!col.binding.isValid()) return nullptr;
    if (!m_doc) return nullptr;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp) return nullptr;

    // Find the first fixture in the group that matches the binding's manufacturer/model/mode
    const QMap<QLCPoint, GroupHead> headsMap = grp->headsMap();
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        Fixture* fxi = m_doc->fixture(it.value().fxi);
        if (!fxi) continue;

        QLCFixtureDef*  fxDef  = fxi->fixtureDef();
        QLCFixtureMode* fxMode = fxi->fixtureMode();
        if (!fxDef || !fxMode) continue;

        if (fxDef->manufacturer() != col.binding.manufacturer) continue;
        if (fxDef->model()        != col.binding.model)        continue;
        if (fxMode->name()        != col.binding.modeName)     continue;

        // Found a matching fixture — return the channel at the absolute index
        return fxMode->channel(quint32(col.binding.channelIndex));
    }
    return nullptr;
}

void PresetTableWidget::writeDMXFixtureGroup(QList<Universe*>& universes, uchar xfEffective)
{
    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp) return;

    const QMap<QLCPoint, GroupHead> headsMap = grp->headsMap();

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int activeRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const int stagedRow = (o < m_stagedRow.size()) ? m_stagedRow[o] : -1;

        if (activeRow < 0 || activeRow >= m_rows.size()) continue;

        const PTOutput& out = m_outputs[o];
        if (out.groupRows.isEmpty()) continue;

        const QVector<uchar>& aVals = m_rows[activeRow].values;
        const QVector<uchar>* bVals = (stagedRow >= 0 && stagedRow < m_rows.size())
            ? &m_rows[stagedRow].values : nullptr;

        // Iterate all group head positions, keep only those in out.groupRows
        for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
        {
            const QLCPoint&  pt   = it.key();
            const GroupHead& head = it.value();

            if (!out.groupRows.contains(pt.y())) continue;

            Fixture* fxi = m_doc->fixture(head.fxi);
            if (!fxi) continue;

            QLCFixtureDef*  fxDef  = fxi->fixtureDef();
            QLCFixtureMode* fxMode = fxi->fixtureMode();
            if (!fxDef || !fxMode) continue;

            quint32 uni = fxi->universe();
            if ((int)uni >= universes.size()) continue;

            auto fader = m_faders.value(uni);
            if (fader.isNull())
            {
                fader = universes[uni]->requestFader(Universe::Auto);
                m_faders.insert(uni, fader);
            }

            // Match each column by type binding
            for (int c = 0; c < m_columns.size(); ++c)
            {
                const PTColumn& col = m_columns[c];
                if (!col.binding.isValid()) continue;

                if (fxDef->manufacturer() != col.binding.manufacturer) continue;
                if (fxDef->model()        != col.binding.model)        continue;
                if (fxMode->name()        != col.binding.modeName)     continue;

                // Absolute channel index stored in binding (0-based within fixture mode)
                quint32 absChannel = quint32(col.binding.channelIndex);
                if (absChannel >= fxi->channels()) continue;

                // Multi-head: only write to the GroupHead that owns this channel.
                // Single-head (no heads defined): write to all matching GroupHeads.
                if (!fxMode->heads().isEmpty())
                {
                    if (head.head < 0 || head.head >= (int)fxMode->heads().size()) continue;
                    if (!fxMode->heads()[head.head].channels().contains(absChannel)) continue;
                }

                uchar aVal = (c < aVals.size()) ? aVals[c] : 0;
                uchar bVal = (bVals && c < bVals->size()) ? (*bVals)[c] : aVal;
                applyFadeValue(fader.data(), m_doc, universes[uni],
                               head.fxi, absChannel,
                               aVal, bVal,
                               m_crossfadeEnabled, (stagedRow >= 0),
                               col.fade, xfEffective);
            }
        }
    }
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

    Q_UNUSED(xfEnabled);

    if (m_mode == PTMode::FixtureGroup)
        writeDMXFixtureGroup(universes, xfEffective);
    else
        writeDMXLegacy(universes, xfEffective);
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
    syncAllDataFromTable();

    QVector<PTColumn> colsCopy;
    QVector<PTOutput>  outsCopy;
    PTMode   modeCopy;
    quint32  groupIdCopy;
    {
        QMutexLocker lk(&m_stateMutex);
        colsCopy    = m_columns;
        outsCopy    = m_outputs;
        modeCopy    = m_mode;
        groupIdCopy = m_fixtureGroupId;
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

    PresetTableConfigDialog dlg(m_doc, colsCopy, outsCopy, srcsCopy,
                                xfEnabled, xfSrc, page(),
                                modeCopy, groupIdCopy, this);

    if (dlg.exec() != QDialog::Accepted) return;

    QVector<PTColumn> newCols = dlg.columns();
    QVector<PTOutput>  newOuts = dlg.outputs();
    PTMode   newMode    = dlg.widgetMode();
    quint32  newGroupId = dlg.selectedFixtureGroupId();

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

        m_mode          = newMode;
        m_fixtureGroupId = newGroupId;
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
    PTMode            modeCopy;
    quint32           groupIdCopy;

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
        modeCopy        = m_mode;
        groupIdCopy     = m_fixtureGroupId;
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
        copy->m_mode               = modeCopy;
        copy->m_fixtureGroupId     = groupIdCopy;
    }

    // Copy input sources
    for (int o = 0; o < outsCopy.size(); ++o)
        copy->setInputSource(inputSource(quint8(o)), quint8(o));
    copy->setInputSource(inputSource(quint8(255)), quint8(255));

    copy->rebuildTable();
    return copy;
}

// ==========================================================================
// Cross-project clipboard
// ==========================================================================

void PresetTableWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    /* Sync any pending UI edits to m_rows/m_columns (same as saveXML does) */
    const_cast<PresetTableWidget*>(this)->syncAllDataFromTable();

    QMutexLocker lk(const_cast<QMutex*>(&m_stateMutex));

    obj["crossfadeEnabled"] = m_crossfadeEnabled;
    obj["mode"] = (m_mode == PTMode::FixtureGroup) ? QStringLiteral("FixtureGroup")
                                                    : QStringLiteral("Legacy");

    if (m_mode == PTMode::FixtureGroup)
    {
        FixtureGroup *grp = doc->fixtureGroup(m_fixtureGroupId);
        obj["fixtureGroupName"] = grp ? grp->name() : QString();
    }

    /* Columns */
    QJsonArray cols;
    for (const PTColumn &col : m_columns)
    {
        QJsonObject c;
        c["name"]  = col.name;
        c["type"]  = (col.type == PTColumn::Dropdown ? QStringLiteral("Dropdown") :
                      col.type == PTColumn::Scaler   ? QStringLiteral("Scaler")   :
                                                       QStringLiteral("Numeric"));
        c["fade"]  = col.fade;
        c["width"] = col.width;
        if (col.type == PTColumn::Scaler)
        {
            c["scalerMin"] = col.scalerMin;
            c["scalerMax"] = col.scalerMax;
            c["scalerSuffix"] = col.scalerSuffix;
        }
        if (col.binding.isValid())
        {
            QJsonObject b;
            b["mfg"]  = col.binding.manufacturer;
            b["model"]= col.binding.model;
            b["mode"] = col.binding.modeName;
            b["chan"] = col.binding.channelIndex;
            c["binding"] = b;
        }
        QJsonArray opts;
        for (const PTOption &opt : col.options)
        {
            QJsonObject o;
            o["name"]  = opt.name;
            o["value"] = (int)opt.value;
            o["resource"] = opt.resource;
            opts.append(o);
        }
        if (!opts.isEmpty())
            c["options"] = opts;
        cols.append(c);
    }
    obj["columns"] = cols;

    /* Rows */
    QJsonArray rows;
    for (const PTRow &row : m_rows)
    {
        QJsonObject r;
        r["name"] = row.name;
        QJsonArray vals;
        for (uchar v : row.values)
            vals.append((int)v);
        r["values"] = vals;
        rows.append(r);
    }
    obj["rows"] = rows;

    /* Outputs — fixture by name (Legacy) or groupRows (FixtureGroup) */
    QJsonArray outs;
    for (const PTOutput &out : m_outputs)
    {
        QJsonObject o;
        o["name"] = out.name;
        if (m_mode == PTMode::Legacy)
        {
            Fixture *fxi = doc->fixture(out.fixtureId);
            o["fixtureName"] = fxi ? fxi->name() : QString();
        }
        else
        {
            QJsonArray gRows;
            for (int r : out.groupRows)
                gRows.append(r);
            o["groupRows"] = gRows;
        }
        outs.append(o);
    }
    obj["outputs"] = outs;
}

void PresetTableWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    QMutexLocker lk(&m_stateMutex);

    m_crossfadeEnabled = obj["crossfadeEnabled"].toBool(false);
    m_mode = (obj["mode"].toString() == QLatin1String("FixtureGroup"))
             ? PTMode::FixtureGroup : PTMode::Legacy;

    m_fixtureGroupId = UINT_MAX;
    if (m_mode == PTMode::FixtureGroup)
    {
        const QString gName = obj["fixtureGroupName"].toString();
        if (!gName.isEmpty())
        {
            for (FixtureGroup *grp : doc->fixtureGroups())
            {
                if (grp && grp->name() == gName)
                {
                    m_fixtureGroupId = grp->id();
                    break;
                }
            }
        }
    }

    /* Columns */
    m_columns.clear();
    for (const QJsonValue &v : obj["columns"].toArray())
    {
        QJsonObject c = v.toObject();
        PTColumn col;
        col.name  = c["name"].toString();
        col.fade  = c["fade"].toBool(true);
        col.width = c["width"].toInt(-1);
        const QString typeStr = c["type"].toString();
        col.type = (typeStr == QLatin1String("Dropdown") ? PTColumn::Dropdown :
                    typeStr == QLatin1String("Scaler")   ? PTColumn::Scaler   :
                                                           PTColumn::Numeric);
        if (col.type == PTColumn::Scaler)
        {
            col.scalerMin    = c["scalerMin"].toInt(0);
            col.scalerMax    = c["scalerMax"].toInt(360);
            col.scalerSuffix = c["scalerSuffix"].toString();
        }
        if (c.contains("binding"))
        {
            QJsonObject b = c["binding"].toObject();
            col.binding.manufacturer = b["mfg"].toString();
            col.binding.model        = b["model"].toString();
            col.binding.modeName     = b["mode"].toString();
            col.binding.channelIndex = b["chan"].toInt(-1);
        }
        for (const QJsonValue &ov : c["options"].toArray())
        {
            QJsonObject o = ov.toObject();
            PTOption opt;
            opt.name     = o["name"].toString();
            opt.value    = (uchar)o["value"].toInt(0);
            opt.resource = o["resource"].toString();
            col.options.append(opt);
        }
        m_columns.append(col);
    }

    /* Rows */
    m_rows.clear();
    for (const QJsonValue &v : obj["rows"].toArray())
    {
        QJsonObject r = v.toObject();
        PTRow row;
        row.name = r["name"].toString();
        for (const QJsonValue &val : r["values"].toArray())
            row.values.append((uchar)val.toInt(0));
        m_rows.append(row);
    }

    /* Outputs */
    m_outputs.clear();
    for (const QJsonValue &v : obj["outputs"].toArray())
    {
        QJsonObject o = v.toObject();
        PTOutput out;
        out.name = o["name"].toString();
        if (m_mode == PTMode::Legacy)
        {
            const QString fxName = o["fixtureName"].toString();
            out.fixtureId = UINT_MAX;
            if (!fxName.isEmpty())
            {
                for (Fixture *fxi : doc->fixtures())
                {
                    if (fxi && fxi->name() == fxName)
                    {
                        out.fixtureId = fxi->id();
                        break;
                    }
                }
            }
        }
        else
        {
            for (const QJsonValue &rv : o["groupRows"].toArray())
                out.groupRows.append(rv.toInt());
        }
        m_outputs.append(out);
    }

    m_activeRow.fill(-1, m_outputs.size());
    m_stagedRow.fill(-1, m_outputs.size());

    lk.unlock();
    rebuildTable();
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
    int  nameColW  = root.attributes().value(KXMLNameColWidth).toInt();

    // Mode (default = Legacy for backward compat)
    PTMode loadedMode = PTMode::Legacy;
    QString modeStr = root.attributes().value(KXMLMode).toString();
    if (modeStr == QLatin1String("FixtureGroup"))
        loadedMode = PTMode::FixtureGroup;

    quint32 loadedGroupId = UINT_MAX;
    QString groupIdStr = root.attributes().value(KXMLFxGroupId).toString();
    if (!groupIdStr.isEmpty())
        loadedGroupId = groupIdStr.toUInt();

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
            col.name  = attrs.value(KXMLColName).toString();
            {
                QString typeStr = attrs.value(KXMLColType).toString();
                if (typeStr == QLatin1String("Dropdown"))
                    col.type = PTColumn::Dropdown;
                else if (typeStr == QLatin1String("Scaler"))
                    col.type = PTColumn::Scaler;
                else
                    col.type = PTColumn::Numeric;
            }
            col.fade  = (attrs.value(KXMLColFade).toString() != QLatin1String("False"));
            col.width = attrs.value(KXMLColWidth).toInt();
            if (col.width <= 0) col.width = -1;

            // FixtureGroup binding (absent in legacy files → isValid() returns false)
            QString bindMfg  = attrs.value(KXMLBindMfg).toString();
            QString bindMod  = attrs.value(KXMLBindModel).toString();
            QString bindMode = attrs.value(KXMLBindMode).toString();
            int     bindChan = attrs.value(KXMLBindChan).toInt() - 1;  // stored as 1-based, 0 if absent
            if (!bindMfg.isEmpty() && bindChan >= 0)
            {
                col.binding.manufacturer = bindMfg;
                col.binding.model        = bindMod;
                col.binding.modeName     = bindMode;
                col.binding.channelIndex = bindChan;
            }

            // Scaler attributes (absent in older files → defaults kept)
            if (col.type == PTColumn::Scaler)
            {
                col.scalerMin = attrs.value(KXMLColScalerMin).toInt();  // 0 if absent
                QString maxStr = attrs.value(KXMLColScalerMax).toString();
                col.scalerMax = maxStr.isEmpty() ? 360 : maxStr.toInt();
                col.scalerSuffix = attrs.value(KXMLColScalerSfx).toString();
            }

            // Read child <Option> elements
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLOption)
                {
                    PTOption opt;
                    opt.name     = root.attributes().value(KXMLOptName).toString();
                    opt.value    = uchar(root.attributes().value(KXMLOptValue).toUInt());
                    opt.resource = root.attributes().value(KXMLOptResource).toString();
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

            // FixtureGroup rows (absent in legacy files)
            QString rowsStr = attrs.value(KXMLOutRows).toString();
            if (!rowsStr.isEmpty())
            {
                for (const QString& rStr : rowsStr.split(QLatin1Char(','), Qt::SkipEmptyParts))
                    out.groupRows.append(rStr.trimmed().toInt());
            }

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
        m_nameColWidth = (nameColW > 0) ? nameColW : -1;
        m_mode           = loadedMode;
        m_fixtureGroupId = loadedGroupId;
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
        if (m_nameColWidth > 0)
            doc->writeAttribute(KXMLNameColWidth, QString::number(m_nameColWidth));

        // Write mode (only write explicit tag for FixtureGroup; Legacy is default for old files)
        if (m_mode == PTMode::FixtureGroup)
        {
            doc->writeAttribute(KXMLMode,    QLatin1String("FixtureGroup"));
            doc->writeAttribute(KXMLFxGroupId, QString::number(m_fixtureGroupId));
        }
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
            col.type == PTColumn::Dropdown ? QLatin1String("Dropdown") :
            col.type == PTColumn::Scaler   ? QLatin1String("Scaler")   :
                                             QLatin1String("Numeric"));
        if (col.type == PTColumn::Scaler)
        {
            doc->writeAttribute(KXMLColScalerMin, QString::number(col.scalerMin));
            doc->writeAttribute(KXMLColScalerMax, QString::number(col.scalerMax));
            if (!col.scalerSuffix.isEmpty())
                doc->writeAttribute(KXMLColScalerSfx, col.scalerSuffix);
        }
        doc->writeAttribute(KXMLColFade,  col.fade ? QLatin1String("True") : QLatin1String("False"));
        if (col.width > 0)
            doc->writeAttribute(KXMLColWidth, QString::number(col.width));

        // FixtureGroup binding — stored as 1-based channelIndex so 0 means "absent"
        if (col.binding.isValid())
        {
            doc->writeAttribute(KXMLBindMfg,   col.binding.manufacturer);
            doc->writeAttribute(KXMLBindModel, col.binding.model);
            doc->writeAttribute(KXMLBindMode,  col.binding.modeName);
            doc->writeAttribute(KXMLBindChan,  QString::number(col.binding.channelIndex + 1));
        }

        for (const PTOption& opt : col.options)
        {
            doc->writeStartElement(KXMLOption);
            doc->writeAttribute(KXMLOptName,  opt.name);
            doc->writeAttribute(KXMLOptValue, QString::number(opt.value));
            if (!opt.resource.isEmpty())
                doc->writeAttribute(KXMLOptResource, opt.resource);
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
    struct OutData {
        QString    name;
        quint32    fixtureId;
        QList<int> groupRows;
    };
    QVector<OutData> outData;
    outData.reserve(m_outputs.size());
    for (const PTOutput& out : m_outputs)
        outData.append({out.name, out.fixtureId, out.groupRows});

    bool isFGMode = (m_mode == PTMode::FixtureGroup);
    lk.unlock();

    for (int o = 0; o < outData.size(); ++o)
    {
        doc->writeStartElement(KXMLOutput);
        doc->writeAttribute(KXMLOutIndex, QString::number(o));
        doc->writeAttribute(KXMLOutName,  outData[o].name);

        if (!isFGMode)
        {
            doc->writeAttribute(KXMLOutFxId, QString::number(outData[o].fixtureId));
        }
        else if (!outData[o].groupRows.isEmpty())
        {
            QStringList rowParts;
            for (int row : outData[o].groupRows)
                rowParts << QString::number(row);
            doc->writeAttribute(KXMLOutRows, rowParts.join(QLatin1Char(',')));
        }

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

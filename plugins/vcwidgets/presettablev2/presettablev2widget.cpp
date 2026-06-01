/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2widget.cpp — Apache 2.0 / public domain
*/

#include "presettablev2widget.h"
#include "presettablev2configdialog.h"
#include "presettablev2columndialog.h"
#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"
#include "ptparammatrixengine.h"
#include "ptspatialfixtureplan.h"
#include "presettablev2transitionprovideriface.h"
#include "presettablev2inputids.h"
#include "ptefxinputids.h"
#include "virtualconsole.h"

#include "genericfader.h"
#include "fadechannel.h"
#include "mastertimer.h"
#include "universe.h"
#include "fixture.h"
#include "fixturegroup.h"
#include "fixturegroupmask.h"
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

const QColor PresetTableV2Widget::s_outputColors[8] = {
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
static const QString KXMLPluginIdVal= QStringLiteral("org.qlcplus.vcwidgets.presettablev2");
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
static const QString KXMLContinuousFxSelectorMode = QStringLiteral("ContinuousFxSelectorMode");
static const QString KXMLColWidth        = QStringLiteral("Width");
static const QString KXMLNameColWidth    = QStringLiteral("NameColWidth");

// FixtureGroup mode XML constants
static const QString KXMLMode           = QStringLiteral("Mode");
static const QString KXMLFxGroupId      = QStringLiteral("FixtureGroupID");
static const QString KXMLOutRows        = QStringLiteral("Rows");
static const QString KXMLOutScope       = QStringLiteral("Scope");
static const QString KXMLOutSweepPreset = QStringLiteral("SweepPreset");
static const QString KXMLOutContinuousPreset = QStringLiteral("ContinuousPreset");
static const QString KXMLOutTransitionPreset = QStringLiteral("TransitionPreset");
static const QString KXMLOutTransitionSecondary = QStringLiteral("TransitionSecondaryPreset");
static const QString KXMLOutSecondaryRow = QStringLiteral("SecondaryRow");
static const QString KXMLOutTransPrimaryInput = QStringLiteral("OutTransPrimaryInput");
static const QString KXMLOutTransSweepInput = QStringLiteral("OutTransSweepInput");
static const QString KXMLOutTransContinuousInput = QStringLiteral("OutTransContinuousInput");
static const QString KXMLOutTransSecondaryInput = QStringLiteral("OutTransSecondaryInput");

static PTOutputScope scopeFromString(const QString& value)
{
    if (value == QLatin1String("Mask"))
        return PTOutputScope::Mask;
    if (value == QLatin1String("Rows"))
        return PTOutputScope::Rows;
    return PTOutputScope::RowsAndMask;
}

static QString scopeToString(PTOutputScope scope)
{
    switch (scope)
    {
        case PTOutputScope::Mask:
            return QStringLiteral("Mask");
        case PTOutputScope::Rows:
            return QStringLiteral("Rows");
        default:
            return QStringLiteral("RowsAndMask");
    }
}

static QString continuousFxSelectorModeToString(PTContinuousFxSelectorMode mode)
{
    switch (mode)
    {
        case PTContinuousFxSelectorMode::Live:
            return QStringLiteral("Live");
        case PTContinuousFxSelectorMode::SmoothMorph:
            return QStringLiteral("SmoothMorph");
        case PTContinuousFxSelectorMode::StagedCommit:
        default:
            return QStringLiteral("StagedCommit");
    }
}

static PTContinuousFxSelectorMode continuousFxSelectorModeFromString(const QString& value)
{
    if (value == QLatin1String("Live"))
        return PTContinuousFxSelectorMode::Live;
    if (value == QLatin1String("SmoothMorph"))
        return PTContinuousFxSelectorMode::SmoothMorph;
    return PTContinuousFxSelectorMode::StagedCommit;
}

static bool outputScopeAllowsPoint(PTOutputScope scope, const QLCPoint& pt, const PTOutput& out)
{
    switch (scope)
    {
        case PTOutputScope::Rows:
            if (out.groupRows.isEmpty())
                return true;
            return out.groupRows.contains(pt.y());
        case PTOutputScope::Mask:
            return true;
        default:
            if (out.groupRows.isEmpty())
                return true;
            return out.groupRows.contains(pt.y());
    }
}
static const QString KXMLBindMfg        = QStringLiteral("BindMfg");
static const QString KXMLBindModel      = QStringLiteral("BindModel");
static const QString KXMLBindMode       = QStringLiteral("BindMode");
static const QString KXMLBindChan       = QStringLiteral("BindChan");
static const QString KXMLColScalerMin   = QStringLiteral("ScalerMin");
static const QString KXMLColScalerMax   = QStringLiteral("ScalerMax");
static const QString KXMLColScalerSfx   = QStringLiteral("ScalerSuffix");
static const QString KXMLSpatialEn      = QStringLiteral("SpatialEnabled");
static const QString KXMLSpatialOrder   = QStringLiteral("SpatialOrder");
static const QString KXMLSpatialStepMs  = QStringLiteral("SpatialStepMs");
static const QString KXMLSpatialFadeMs  = QStringLiteral("SpatialFadeMs");
static const QString KXMLSpatialReverse = QStringLiteral("SpatialReverse");
static const QString KXMLLinkedTransition = QStringLiteral("LinkedTransitionWidgetId");

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
// PresetTableV2Delegate
// ==========================================================================

PresetTableV2Delegate::PresetTableV2Delegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void PresetTableV2Delegate::setColumns(const QVector<PTColumn>* columns)
{
    m_columns = columns;
}

void PresetTableV2Delegate::setOwner(const PresetTableV2Widget* owner)
{
    m_owner = owner;
}

QWidget* PresetTableV2Delegate::createEditor(QWidget* parent,
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

void PresetTableV2Delegate::setEditorData(QWidget* editor, const QModelIndex& index) const
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

void PresetTableV2Delegate::setModelData(QWidget* editor, QAbstractItemModel* model,
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

void PresetTableV2Delegate::updateEditorGeometry(QWidget* editor,
                                                const QStyleOptionViewItem& option,
                                                const QModelIndex& /*index*/) const
{
    if (editor)
        editor->setGeometry(option.rect);
}

void PresetTableV2Delegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
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

QSize PresetTableV2Delegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& /*index*/) const
{
    return QSize(60, option.fontMetrics.height() + 8);
}

// ==========================================================================
// PresetTableV2Widget — construction
// ==========================================================================

PresetTableV2Widget::PresetTableV2Widget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(PresetTableV2Widget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Preset Table v2"));
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

    connect(actAddRow, &QAction::triggered, this, &PresetTableV2Widget::slotAddRow);
    connect(actRemRow, &QAction::triggered, this, &PresetTableV2Widget::slotRemoveRow);
    connect(actAddCol, &QAction::triggered, this, &PresetTableV2Widget::slotAddColumn);
    connect(actRemCol, &QAction::triggered, this, &PresetTableV2Widget::slotRemoveColumn);
    connect(actProps,  &QAction::triggered, this, &PresetTableV2Widget::slotProperties);

    m_layout->addWidget(m_toolbar);

    // ---- Table -----------------------------------------------------------
    m_delegate = new PresetTableV2Delegate(this);
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
            this, &PresetTableV2Widget::slotCellChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &PresetTableV2Widget::slotColumnHeaderDoubleClicked);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized,
            this, &PresetTableV2Widget::slotHeaderSectionResized);
    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &PresetTableV2Widget::slotTableContextMenu);

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

    if (m_doc != nullptr)
    {
        connect(m_doc, SIGNAL(fixtureGroupMaskChanged(quint32)),
                this, SLOT(slotFixtureGroupMaskChanged(quint32)));
    }
}

PresetTableV2Widget::~PresetTableV2Widget()
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

void PresetTableV2Widget::setColumns(const QVector<PTColumn>& cols)
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

void PresetTableV2Widget::setRows(const QVector<PTRow>& rows)
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

void PresetTableV2Widget::setOutputs(const QVector<PTOutput>& outs)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_outputs = outs;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
        m_stagedSecondaryRow.resize(m_outputs.size());
        m_stagedSecondaryRow.fill(-1);
        m_stagedSweepPreset.resize(m_outputs.size());
        m_stagedSweepPreset.fill(-1);
        m_stagedContinuousPreset.resize(m_outputs.size());
        m_stagedContinuousPreset.fill(-1);
        m_stagedSecondaryValid.resize(m_outputs.size());
        m_stagedSecondaryValid.fill(false);
        m_stagedSweepValid.resize(m_outputs.size());
        m_stagedSweepValid.fill(false);
        m_stagedContinuousValid.resize(m_outputs.size());
        m_stagedContinuousValid.fill(false);
        m_spatialAppliedRow.resize(m_outputs.size());
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.resize(m_outputs.size());
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        syncLiveTransitionFromOutputs();
    }
    refreshRowHighlights();
}

// ==========================================================================
// Mode
// ==========================================================================

void PresetTableV2Widget::slotModeChanged(Doc::Mode newMode)
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
            m_stagedSecondaryRow.fill(-1, m_stagedSecondaryRow.size());
            m_stagedSweepPreset.fill(-1, m_stagedSweepPreset.size());
            m_stagedContinuousPreset.fill(-1, m_stagedContinuousPreset.size());
            m_stagedSecondaryValid.fill(false, m_stagedSecondaryValid.size());
            m_stagedSweepValid.fill(false, m_stagedSweepValid.size());
            m_stagedContinuousValid.fill(false, m_stagedContinuousValid.size());
            m_crossfadeGlobalPos = 0;
            m_crossfadeStartPos  = 0;
            m_crossfadePrevPos   = 0;
            m_crossfadeStagedAtLowSide = true;
            m_crossfadeSessionActive = false;
            m_crossfadeEditLaneStaged = true;
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

void PresetTableV2Widget::slotAddRow()
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

void PresetTableV2Widget::slotRemoveRow()
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

void PresetTableV2Widget::slotAddColumn()
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

void PresetTableV2Widget::slotRemoveColumn()
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

void PresetTableV2Widget::slotProperties()
{
    editProperties();
}

// ==========================================================================
// Column header double-click → open column config dialog
// ==========================================================================

void PresetTableV2Widget::slotColumnHeaderDoubleClicked(int logicalIndex)
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

    PresetTableV2ColumnDialog dlg(m_doc, m_columns[valCol], colMode, grp, this);
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

bool PresetTableV2Widget::eventFilter(QObject* obj, QEvent* ev)
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

void PresetTableV2Widget::slotHeaderSectionResized(int logicalIndex, int /*oldSize*/, int newSize)
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

void PresetTableV2Widget::keyPressEvent(QKeyEvent* e)
{
    if (e->matches(QKeySequence::Copy))  { slotCopySelection(); e->accept(); return; }
    if (e->matches(QKeySequence::Paste)) { slotPasteSelection(); e->accept(); return; }
    VCWidget::keyPressEvent(e);
}

// ==========================================================================
// Copy selection → clipboard as TSV
// ==========================================================================

void PresetTableV2Widget::slotCopySelection()
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

void PresetTableV2Widget::pasteValueToItem(QTableWidgetItem* item, const QString& raw)
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

void PresetTableV2Widget::slotTableContextMenu(const QPoint& pos)
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

void PresetTableV2Widget::syncAllDataFromTable()
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

void PresetTableV2Widget::slotPasteSelection()
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

void PresetTableV2Widget::rebuildTable()
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

void PresetTableV2Widget::slotCellChanged(int row, int col)
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

void PresetTableV2Widget::setActiveRow(int outputIdx, int rowIdx)
{
    {
        QMutexLocker lk(&m_stateMutex);
        if (outputIdx < 0 || outputIdx >= m_activeRow.size()) return;
        const int prevRow = m_activeRow[outputIdx];
        m_activeRow[outputIdx] = rowIdx;
        if (outputIdx < m_spatialAppliedRow.size() && rowIdx < 0)
            m_spatialAppliedRow[outputIdx] = -1;
        if (outputIdx >= 0 && outputIdx < m_spatialChase.size())
            m_spatialChase[outputIdx] = PTSpatialChaseOutput();

        if (useMatrixEngineLocked() && rowIdx >= 0 && rowIdx != prevRow)
        {
            ensureMatrixState(outputIdx);
            PTOutputMatrixState& st = m_matrixState[outputIdx];
            if (sweepOnPrimaryChangeLocked(outputIdx, rowIdx) && !st.flashActive)
                beginMatrixSweepLocked(outputIdx, prevRow, rowIdx);
            else if (!st.flashActive)
            {
                st.sweepRunning = false;
                st.appliedRow = rowIdx;
            }
        }
        else if (rowIdx >= 0 && useMatrixEngineLocked())
        {
            ensureMatrixState(outputIdx);
            if (!m_matrixState[outputIdx].sweepRunning && !m_matrixState[outputIdx].flashActive)
                m_matrixState[outputIdx].appliedRow = rowIdx;
        }
    }
    refreshRowHighlights();
    sendFeedback(rowIdx + 1, quint8(outputIdx));   // 0 = off when rowIdx == -1 → gives 0
    update();
}

bool PresetTableV2Widget::spatialEffectsEnabled() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_spatialEffects.enabled;
}

void PresetTableV2Widget::setSpatialEffectsEnabled(bool enabled)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_spatialEffects.enabled = enabled;
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        if (!enabled)
            resetAllMatrixStatesLocked();
    }
    refreshTransitionPresetCache();
}

PTSpatialEffectSettings PresetTableV2Widget::spatialEffectSettings() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_spatialEffects;
}

void PresetTableV2Widget::setSpatialEffectSettings(const PTSpatialEffectSettings& settings)
{
    QMutexLocker lk(&m_stateMutex);
    m_spatialEffects = settings;
    m_spatialAppliedRow.fill(-1);
    m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
}

quint32 PresetTableV2Widget::linkedTransitionWidgetId() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_linkedTransitionWidgetId;
}

void PresetTableV2Widget::setLinkedTransitionWidgetId(quint32 id)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_linkedTransitionWidgetId = id;
    }
    refreshTransitionPresetCache();
}

void PresetTableV2Widget::refreshTransitionPresetCache()
{
    QMutexLocker lk(&m_stateMutex);
    m_cachedTransitionWidgetId = m_linkedTransitionWidgetId;
    m_cachedTransitionSweepCount = 0;
    m_cachedTransitionContinuousCount = 0;
    if (m_linkedTransitionWidgetId == VCWidget::invalidId())
        return;

    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
    {
        m_cachedTransitionSweepCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
        m_cachedTransitionContinuousCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
    }
}

void PresetTableV2Widget::syncLiveTransitionFromOutputs()
{
    m_liveSweepPreset.resize(m_outputs.size());
    m_liveContinuousPreset.resize(m_outputs.size());
    m_liveSecondaryRow.resize(m_outputs.size());
    m_stagedSecondaryRow.resize(m_outputs.size());
    m_stagedSweepPreset.resize(m_outputs.size());
    m_stagedContinuousPreset.resize(m_outputs.size());
    m_stagedSecondaryValid.resize(m_outputs.size());
    m_stagedSweepValid.resize(m_outputs.size());
    m_stagedContinuousValid.resize(m_outputs.size());
    m_stagedSecondaryValid.fill(false);
    m_stagedSweepValid.fill(false);
    m_stagedContinuousValid.fill(false);
    m_continuousElapsedMs.resize(m_outputs.size());
    m_matrixState.resize(m_outputs.size());
    m_flashInputHeldRow.resize(m_outputs.size());
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int prevSweep = (o < m_liveSweepPreset.size()) ? m_liveSweepPreset[o] : -1;
        m_liveSweepPreset[o] = m_outputs[o].sweepPresetIndex;
        m_liveContinuousPreset[o] = m_outputs[o].continuousPresetIndex;
        m_liveSecondaryRow[o] = -1;
        if (m_liveSweepPreset[o] < 0 && m_liveContinuousPreset[o] < 0)
            resetMatrixStateLocked(o);
        else if (m_liveSweepPreset[o] != prevSweep)
        {
            resetMatrixStateLocked(o);
            if (o < m_spatialAppliedRow.size())
                m_spatialAppliedRow[o] = -1;
            if (o < m_spatialChase.size())
                m_spatialChase[o] = PTSpatialChaseOutput();
        }
    }
}

int PresetTableV2Widget::liveSweepPresetIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    return (outputIdx < m_liveSweepPreset.size())
            ? m_liveSweepPreset[outputIdx] : m_outputs[outputIdx].sweepPresetIndex;
}

int PresetTableV2Widget::liveContinuousPresetIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    return (outputIdx < m_liveContinuousPreset.size())
            ? m_liveContinuousPreset[outputIdx] : m_outputs[outputIdx].continuousPresetIndex;
}

PTTransitionPreset PresetTableV2Widget::transitionPresetAtIndexLocked(PTTransitionMode mode,
                                                                      int presetIndex) const
{
    if (presetIndex < 0)
    {
        PTTransitionPreset instant;
        instant.name = QStringLiteral("Instant");
        instant.enabled = false;
        return instant;
    }

    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
    {
        if (presetIndex < provider->transitionPresetCount(mode))
            return provider->effectiveTransitionPreset(mode, presetIndex);
    }

    return PresetTableV2SpatialEngine::presetFromLegacySpatial(m_spatialEffects);
}

PTTransitionPreset PresetTableV2Widget::sweepPresetForOutputLocked(int outputIdx) const
{
    return transitionPresetAtIndexLocked(PTTransitionMode::SweepOnly,
                                         liveSweepPresetIndexLocked(outputIdx));
}

PTTransitionPreset PresetTableV2Widget::continuousPresetForOutputLocked(int outputIdx) const
{
    return transitionPresetAtIndexLocked(PTTransitionMode::Continuous,
                                         liveContinuousPresetIndexLocked(outputIdx));
}

PTTransitionPreset PresetTableV2Widget::continuousPresetForOutputLocked(int outputIdx,
                                                                        uchar xfEffective) const
{
    Q_UNUSED(xfEffective);
    const PTTransitionPreset live = continuousPresetForOutputLocked(outputIdx);
    if (outputIdx < 0 || outputIdx >= m_stagedContinuousPreset.size()
            || outputIdx >= m_stagedContinuousValid.size()
            || !m_stagedContinuousValid[outputIdx])
        return live;

    return transitionPresetAtIndexLocked(
            PTTransitionMode::Continuous, m_stagedContinuousPreset[outputIdx]);
}

static QVector<uchar> blendRowValues(const QVector<uchar>& live,
                                     const QVector<uchar>& staged,
                                     double progress)
{
    const int count = qMax(live.size(), staged.size());
    QVector<uchar> out;
    out.resize(count);
    const qint16 xf = qint16(qBound(0, int(progress * 255.0 + 0.5), 255));
    for (int i = 0; i < count; ++i)
    {
        const uchar a = (i < live.size()) ? live[i] : 0;
        const uchar b = (i < staged.size()) ? staged[i] : a;
        out[i] = uchar(a + qint16(b - a) * xf / 255);
    }
    return out;
}

PresetTableV2Widget::PTContinuousLayerState
PresetTableV2Widget::continuousLayerStateForOutputLocked(int outputIdx,
                                                         int activeRow,
                                                         uchar xfEffective) const
{
    PTContinuousLayerState state;
    if (outputIdx < 0 || outputIdx >= m_outputs.size()
            || activeRow < 0 || activeRow >= m_rows.size())
        return state;

    Q_UNUSED(xfEffective);
    const bool hasStagedPrimary = outputIdx < m_stagedRow.size()
            && m_stagedRow[outputIdx] >= 0 && m_stagedRow[outputIdx] < m_rows.size();
    const bool hasStagedSecondary = outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx]
            && outputIdx < m_stagedSecondaryRow.size()
            && m_stagedSecondaryRow[outputIdx] >= 0
            && m_stagedSecondaryRow[outputIdx] < m_rows.size();
    const bool hasStagedContinuous = outputIdx < m_stagedContinuousValid.size()
            && m_stagedContinuousValid[outputIdx];

    const int liveSecondary = effectiveSecondaryRowLocked(outputIdx, activeRow);
    state.primaryRow = activeRow;
    state.secondaryRow = hasStagedSecondary ? m_stagedSecondaryRow[outputIdx] : liveSecondary;
    state.livePrimaryValues = m_rows[activeRow].values;
    state.primaryValues = state.livePrimaryValues;
    if (hasStagedPrimary)
        state.primaryValues = m_rows[m_stagedRow[outputIdx]].values;

    if (liveSecondary >= 0 && liveSecondary < m_rows.size())
        state.liveSecondaryValues = m_rows[liveSecondary].values;
    else
        state.liveSecondaryValues = state.livePrimaryValues;
    state.secondaryValues = state.liveSecondaryValues;

    if (hasStagedSecondary)
        state.secondaryValues = m_rows[m_stagedSecondaryRow[outputIdx]].values;

    state.livePreset = continuousPresetForOutputLocked(outputIdx);
    state.preset = hasStagedContinuous
            ? continuousPresetForOutputLocked(outputIdx, xfEffective)
            : state.livePreset;
    state.hasStaged = hasStagedPrimary || hasStagedSecondary || hasStagedContinuous;
    state.active = (state.livePreset.enabled || state.preset.enabled)
            && (state.secondaryRow >= 0 || hasStagedSecondary || hasStagedContinuous);
    return state;
}

bool PresetTableV2Widget::sweepEfxActiveForOutputLocked(int outputIdx) const
{
    return liveSweepPresetIndexLocked(outputIdx) >= 0;
}

bool PresetTableV2Widget::continuousEfxActiveForOutputLocked(int outputIdx) const
{
    if (liveContinuousPresetIndexLocked(outputIdx) >= 0)
        return true;
    return m_continuousFxSelectorMode == PTContinuousFxSelectorMode::SmoothMorph
            && outputIdx >= 0 && outputIdx < m_stagedContinuousValid.size()
            && m_stagedContinuousValid[outputIdx];
}

bool PresetTableV2Widget::sweepOnPrimaryChangeLocked(int outputIdx, int newActiveRow) const
{
    if (!sweepEfxActiveForOutputLocked(outputIdx))
        return false;
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    if (continuousEfxActiveForOutputLocked(outputIdx)
            && (hasStagedSecondary || effectiveSecondaryRowLocked(outputIdx, newActiveRow) >= 0))
        return false;
    return true;
}

int PresetTableV2Widget::effectiveSecondaryRowLocked(int outputIdx, int activeRow) const
{
    Q_UNUSED(activeRow);

    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;

    const int live = (outputIdx < m_liveSecondaryRow.size()) ? m_liveSecondaryRow[outputIdx] : -1;
    if (live >= 0 && live < m_rows.size())
        return live;

    const int prop = m_outputs[outputIdx].secondaryRowIndex;
    if (prop >= 0 && prop < m_rows.size())
        return prop;

    return -1;
}

bool PresetTableV2Widget::continuousCrossfadeModeLocked(int outputIdx) const
{
    if (!m_crossfadeEnabled || !continuousEfxActiveForOutputLocked(outputIdx))
        return false;
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    return hasStagedSecondary || effectiveSecondaryRowLocked(outputIdx, m_activeRow[outputIdx]) >= 0;
}

bool PresetTableV2Widget::crossfadeSweepModeLocked(int outputIdx, int activeRow, bool hasStaged) const
{
    if (!m_crossfadeEnabled || !hasStaged || !sweepEfxActiveForOutputLocked(outputIdx))
        return false;
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    if (continuousEfxActiveForOutputLocked(outputIdx)
            && (hasStagedSecondary || effectiveSecondaryRowLocked(outputIdx, activeRow) >= 0))
        return false;
    return true;
}

bool PresetTableV2Widget::continuousCrossfadeActiveAnyLocked() const
{
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        if (continuousCrossfadeModeLocked(o))
            return true;
    }
    return false;
}

bool PresetTableV2Widget::continuousFxSelectionStagedAnyLocked() const
{
    for (int o = 0; o < m_stagedContinuousValid.size(); ++o)
    {
        if (m_stagedContinuousValid[o])
            return true;
    }
    return false;
}

bool PresetTableV2Widget::continuousFxSelectorToStagedLocked() const
{
    return m_crossfadeEnabled
            && m_continuousFxSelectorMode != PTContinuousFxSelectorMode::Live
            && crossfadeRoutesToStagedLocked();
}

bool PresetTableV2Widget::crossfadeManualControlEnabledLocked() const
{
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        return provider->crossfadeManualControlEnabled();
    return true;
}

bool PresetTableV2Widget::crossfadeRoutesToStagedLocked() const
{
    return m_crossfadeEnabled && m_crossfadeEditLaneStaged;
}

bool PresetTableV2Widget::crossfadeHasStagedChangesLocked() const
{
    for (int o = 0; o < m_stagedRow.size(); ++o)
    {
        if (m_stagedRow[o] >= 0)
            return true;
    }
    for (int o = 0; o < m_stagedSecondaryValid.size(); ++o)
    {
        if (m_stagedSecondaryValid[o])
            return true;
    }
    for (int o = 0; o < m_stagedSweepValid.size(); ++o)
    {
        if (m_stagedSweepValid[o])
            return true;
    }
    for (int o = 0; o < m_stagedContinuousValid.size(); ++o)
    {
        if (m_stagedContinuousValid[o])
            return true;
    }
    return false;
}

void PresetTableV2Widget::armCrossfadeStagingLocked()
{
    if (!m_crossfadeEnabled || !crossfadeManualControlEnabledLocked())
        return;
    if (crossfadeHasStagedChangesLocked())
        return;

    m_crossfadeSessionActive = true;
    m_crossfadeEditLaneStaged = true;
    m_crossfadeStagedAtLowSide = (m_crossfadeGlobalPos <= 127);
    m_crossfadeStartPos = m_crossfadeGlobalPos;
}

void PresetTableV2Widget::tickCrossfadeClockLocked(MasterTimer* timer)
{
    if (!m_crossfadeEnabled || !timer)
        return;

    const bool manual = crossfadeManualControlEnabledLocked();
    if (manual != m_crossfadeLastManualControl)
    {
        m_crossfadeLastManualControl = manual;
        resetCrossfadeClockLocked();
        if (manual)
        {
            m_crossfadeStagedAtLowSide = (m_crossfadeGlobalPos <= 127);
            m_crossfadeStartPos = m_crossfadeGlobalPos;
        }
    }

    if (manual)
        return;

    const double prev = m_crossfadeClockProgress01;
    const PTGlobalEffectSettings global = globalEffectSettingsLocked();
    const quint32 cycleMs = crossfadeClockCycleMsLocked(global);
    m_crossfadeClockElapsedMs += timer->tick();
    m_crossfadeClockProgress01 = qMin(1.0, double(m_crossfadeClockElapsedMs)
            / double(qMax(quint32(1), cycleMs)));

    if (prev < 1.0 && m_crossfadeClockProgress01 >= 1.0
            && (continuousCrossfadeActiveAnyLocked() || continuousFxSelectionStagedAnyLocked()))
    {
        promoteStagedToLiveLocked();
        resetCrossfadeClockLocked();
    }
}

double PresetTableV2Widget::crossfadeProgress01Locked(uchar xfEffective) const
{
    if (!m_crossfadeEnabled)
        return 0.0;
    if (crossfadeManualControlEnabledLocked())
        return double(xfEffective) / 255.0;
    return m_crossfadeClockProgress01;
}

void PresetTableV2Widget::resetCrossfadeClockLocked()
{
    m_crossfadeClockElapsedMs = 0;
    m_crossfadeClockProgress01 = 0.0;
}

quint32 PresetTableV2Widget::crossfadeClockCycleMsLocked(
        const PTGlobalEffectSettings& global) const
{
    PTTransitionPreset dur;
    dur.speedMultiplier = global.speedMultiplier;
    return PTParamMatrixEngine::effectiveDurationMs(global, dur, false);
}

uchar PresetTableV2Widget::crossfadeEffectiveLocked(uchar xfPos, uchar xfStartPos) const
{
    if (!m_crossfadeEnabled)
        return 0;

    if (!crossfadeManualControlEnabledLocked())
        return uchar(qBound(0, int(m_crossfadeClockProgress01 * 255.0 + 0.5), 255));

    const uchar xfTarget = m_crossfadeStagedAtLowSide ? 255 : 0;
    qint16 xfTraveled = 0;
    qint16 xfMaxTravel = 0;
    if (xfTarget == 255)
    {
        xfTraveled = qint16(xfPos) - qint16(xfStartPos);
        xfMaxTravel = qint16(255) - qint16(xfStartPos);
    }
    else
    {
        xfTraveled = qint16(xfStartPos) - qint16(xfPos);
        xfMaxTravel = qint16(xfStartPos);
    }
    if (xfMaxTravel <= 0)
        xfMaxTravel = 1;
    return uchar(qBound(qint16(0), qint16(xfTraveled * 255 / xfMaxTravel), qint16(255)));
}

bool PresetTableV2Widget::continuousCrossfadeStagedEditing() const
{
    QMutexLocker lk(&m_stateMutex);
    return crossfadeRoutesToStagedLocked()
            && continuousCrossfadeActiveAnyLocked();
}

int PresetTableV2Widget::fixtureGroupSpanAlongAxis(const PTTransitionPreset& preset,
                                                   const PTGlobalEffectSettings& global) const
{
    if (m_mode != PTMode::FixtureGroup || m_fixtureGroupId == UINT_MAX || !m_doc)
        return 0;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return 0;

    const QSize sz = grp->size();
    return PTDimmerWaveEngine::gridSpanAlongAxis(
            sz.width(), sz.height(), preset.axis, global.fxOrientation);
}

bool PresetTableV2Widget::spatialGridPreview(const PTTransitionPreset& preset,
                                            const PTGlobalEffectSettings& global,
                                            PTSpatialGridPreview& out) const
{
    out = PTSpatialGridPreview();
    if (m_mode != PTMode::FixtureGroup || m_fixtureGroupId == UINT_MAX || !m_doc)
        return false;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return false;

    const QSize sz = grp->size();
    if (sz.width() <= 0 || sz.height() <= 0)
        return false;

    QList<QLCPoint> points;
    const QMap<QLCPoint, GroupHead> heads = grp->headsMap();
    for (auto it = heads.constBegin(); it != heads.constEnd(); ++it)
        points.append(it.key());

    out = PTSpatialFixturePlan::buildGridPreview(
            points, preset, global, sz.width(), sz.height());
    return out.valid;
}

void PresetTableV2Widget::clearStagedLayerLocked(int outputIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;

    if (outputIdx < m_stagedRow.size())
        m_stagedRow[outputIdx] = -1;
    if (outputIdx < m_stagedSecondaryRow.size())
        m_stagedSecondaryRow[outputIdx] = -1;
    if (outputIdx < m_stagedSweepPreset.size())
        m_stagedSweepPreset[outputIdx] = -1;
    if (outputIdx < m_stagedContinuousPreset.size())
        m_stagedContinuousPreset[outputIdx] = -1;
    if (outputIdx < m_stagedSecondaryValid.size())
        m_stagedSecondaryValid[outputIdx] = false;
    if (outputIdx < m_stagedSweepValid.size())
        m_stagedSweepValid[outputIdx] = false;
    if (outputIdx < m_stagedContinuousValid.size())
        m_stagedContinuousValid[outputIdx] = false;
    if (!crossfadeHasStagedChangesLocked())
    {
        m_crossfadeSessionActive = false;
    }
}

void PresetTableV2Widget::stageSecondaryRowLocked(int outputIdx, int rowIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedSecondaryRow.size() <= outputIdx)
        m_stagedSecondaryRow.append(-1);
    while (m_stagedSecondaryValid.size() <= outputIdx)
        m_stagedSecondaryValid.append(false);
    m_stagedSecondaryRow[outputIdx] = rowIdx;
    m_stagedSecondaryValid[outputIdx] = true;
}

void PresetTableV2Widget::stageSweepPresetLocked(int outputIdx, int presetIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedSweepPreset.size() <= outputIdx)
        m_stagedSweepPreset.append(-1);
    while (m_stagedSweepValid.size() <= outputIdx)
        m_stagedSweepValid.append(false);
    m_stagedSweepPreset[outputIdx] = presetIdx;
    m_stagedSweepValid[outputIdx] = true;
}

void PresetTableV2Widget::stageContinuousPresetLocked(int outputIdx, int presetIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedContinuousPreset.size() <= outputIdx)
        m_stagedContinuousPreset.append(-1);
    while (m_stagedContinuousValid.size() <= outputIdx)
        m_stagedContinuousValid.append(false);
    m_stagedContinuousPreset[outputIdx] = presetIdx;
    m_stagedContinuousValid[outputIdx] = true;
}

void PresetTableV2Widget::promoteStagedToLiveLocked()
{
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        bool promoted = false;
        if (o < m_stagedRow.size() && m_stagedRow[o] >= 0)
        {
            m_activeRow[o] = m_stagedRow[o];
            m_stagedRow[o] = -1;
            promoted = true;
        }

        if (o < m_stagedSecondaryValid.size() && m_stagedSecondaryValid[o])
        {
            if (o < m_liveSecondaryRow.size() && o < m_stagedSecondaryRow.size())
                m_liveSecondaryRow[o] = m_stagedSecondaryRow[o];
            m_stagedSecondaryValid[o] = false;
            if (o < m_stagedSecondaryRow.size())
                m_stagedSecondaryRow[o] = -1;
            promoted = true;
        }

        if (o < m_stagedSweepValid.size() && m_stagedSweepValid[o])
        {
            if (o < m_liveSweepPreset.size() && o < m_stagedSweepPreset.size())
                m_liveSweepPreset[o] = m_stagedSweepPreset[o];
            m_stagedSweepValid[o] = false;
            if (o < m_stagedSweepPreset.size())
                m_stagedSweepPreset[o] = -1;
            promoted = true;
        }

        if (o < m_stagedContinuousValid.size() && m_stagedContinuousValid[o])
        {
            if (o < m_liveContinuousPreset.size() && o < m_stagedContinuousPreset.size())
                m_liveContinuousPreset[o] = m_stagedContinuousPreset[o];
            m_stagedContinuousValid[o] = false;
            if (o < m_stagedContinuousPreset.size())
                m_stagedContinuousPreset[o] = -1;
            if (o < m_continuousElapsedMs.size())
                m_continuousElapsedMs[o] = 0;
            promoted = true;
        }

        if (promoted)
        {
            resetMatrixStateLocked(o);
            if (o < m_matrixState.size() && o < m_activeRow.size())
                m_matrixState[o].appliedRow = m_activeRow[o];
        }
    }
    m_crossfadeSessionActive = false;
    m_crossfadeEditLaneStaged = true;

    // EFX parameter overrides are live-only. Staged commit is limited to table
    // selections above, so do not call back into the provider while holding
    // m_stateMutex.
}

PTTransitionPreset PresetTableV2Widget::transitionPresetForOutputLocked(int outputIdx) const
{
    if (continuousEfxActiveForOutputLocked(outputIdx))
        return continuousPresetForOutputLocked(outputIdx);
    if (sweepEfxActiveForOutputLocked(outputIdx))
        return sweepPresetForOutputLocked(outputIdx);
    return transitionPresetAtIndexLocked(PTTransitionMode::SweepOnly, -1);
}

PresetTableV2TransitionProviderIface* PresetTableV2Widget::linkedTransitionProvider() const
{
    QMutexLocker lk(&m_stateMutex);
    return transitionProviderLocked();
}

PresetTableV2TransitionProviderIface* PresetTableV2Widget::transitionProviderLocked() const
{
    if (m_linkedTransitionWidgetId == VCWidget::invalidId())
        return nullptr;
    VirtualConsole* vc = VirtualConsole::instance();
    if (!vc)
        return nullptr;
    return qobject_cast<PresetTableV2TransitionProviderIface*>(
            vc->widget(m_linkedTransitionWidgetId));
}

PTGlobalEffectSettings PresetTableV2Widget::globalEffectSettingsLocked() const
{
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        return provider->globalEffectSettings();
    return PTGlobalEffectSettings();
}

quint32 PresetTableV2Widget::cycleDurationMsLocked(const PTGlobalEffectSettings& global,
                                                   const PTTransitionPreset& preset) const
{
    bool honorPresetDuration = false;
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        honorPresetDuration = provider->hasLiveColumnOverride(PTEfxCol::InputDuration);
    return PTParamMatrixEngine::effectiveDurationMs(global, preset, honorPresetDuration);
}

PTTransitionPreset PresetTableV2Widget::transitionPresetForOutput(int outputIdx) const
{
    QMutexLocker lk(&m_stateMutex);
    return transitionPresetForOutputLocked(outputIdx);
}

void PresetTableV2Widget::refreshRowHighlights()
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
    int boundCols = 0;
    for (const PTColumn& col : m_columns)
    {
        if (col.binding.isValid())
            ++boundCols;
    }

    QMutexLocker lk3(&m_stateMutex);
    const bool matrixCapable = useMatrixEngineLocked();
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        QString outName = (o < m_outputs.size()) ? m_outputs[o].name : tr("Out%1").arg(o + 1);
        if (ar < 0)
        {
            parts.append(QStringLiteral("%1: row off").arg(outName));
            continue;
        }
        QString rowLabel = (ar < m_rows.size()) ? m_rows[ar].name : QString::number(ar + 1);
        QString path = tr("direct");
        if (matrixCapable && efxActiveForOutputLocked(o))
            path = tr("efx/matrix");
        else if (m_spatialEffects.enabled && efxActiveForOutputLocked(o))
            path = tr("efx");
        parts.append(QStringLiteral("%1: %2 | %3 | cols %4/%5")
                             .arg(outName, rowLabel, path)
                             .arg(boundCols)
                             .arg(m_columns.size()));
    }
    if (m_mode == PTMode::FixtureGroup && m_fixtureGroupId == UINT_MAX)
        parts.prepend(tr("No fixture group"));
    else if (boundCols == 0 && !m_columns.isEmpty())
        parts.prepend(tr("No column bindings — DMX will not output"));
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
        fc->setFadeTime(0);
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

static void applyFadeValueTimed(GenericFader* fader, Doc* doc, Universe* uni,
                                quint32 fxiId, quint32 chanIdx, uchar target, uint fadeTimeMs)
{
    FadeChannel* fc = fader->getChannelFader(doc, uni, fxiId, chanIdx);
    if (fc->universe() == Universe::invalid())
    {
        fader->remove(fc);
        return;
    }

    if (fadeTimeMs == 0 || qAbs(int(target) - int(fc->current())) <= 2)
    {
        fc->setCurrent(target);
        fc->setStart(target);
        fc->setTarget(target);
        fc->setFadeTime(0);
        fc->setReady(true);
        fc->setElapsed(0);
        return;
    }

    const bool sameTarget = (fc->target() == target) && !fc->isReady();
    if (!sameTarget)
    {
        fc->setStart(fc->current());
        fc->setTarget(target);
        fc->setFadeTime(fadeTimeMs);
        fc->setReady(false);
        fc->setElapsed(0);
    }
    else
    {
        fc->setFadeTime(fadeTimeMs);
    }
}

static QVector<uchar> continuousColumnValues(const QVector<PTColumn>& columns,
                                             const QVector<uchar>& priVals,
                                             const QVector<uchar>& secVals,
                                             double dimmer,
                                             int waveShape,
                                             int waveFadeIn,
                                             int waveFadeOut,
                                             uchar intensity)
{
    double t = dimmer;
    if (t > 1.0)
        t /= 255.0;
    t = qBound(0.0, t, 1.0);

    QVector<uchar> values;
    values.resize(columns.size());
    const bool globalSnap = (waveShape == 1);
    for (int c = 0; c < columns.size(); ++c)
    {
        const uchar pri = (c < priVals.size()) ? priVals[c] : 0;
        const uchar sec = (c < secVals.size()) ? secVals[c] : 0;
        const bool sharpWave = (waveFadeIn == 0 && waveFadeOut == 0);
        const bool snap = globalSnap || sharpWave || !columns[c].fade;
        uchar val = snap
                ? ((t >= 0.5) ? sec : pri)
                : uchar(qBound(0, int(std::lround(double(pri) + (double(sec) - double(pri)) * t)), 255));
        if (intensity < 255)
            val = uchar(qBound(0, int(std::lround(double(val) * double(intensity) / 255.0)), 255));
        values[c] = val;
    }
    return values;
}

void PresetTableV2Widget::writeDMXLegacy(QList<Universe*>& universes, uchar xfEffective)
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

const QLCChannel* PresetTableV2Widget::resolveBoundChannel(const PTColumn& col) const
{
    if (m_mode != PTMode::FixtureGroup) return nullptr;
    if (!col.binding.isValid()) return nullptr;
    if (!m_doc) return nullptr;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp) return nullptr;

    // Find the first fixture in the group that matches the binding's manufacturer/model/mode
    const QMap<QLCPoint, GroupHead> headsMap = m_doc->effectiveHeadsMap(grp);
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

void PresetTableV2Widget::applyPointChannels(GenericFader* fader, Universe* uni,
                                              const GroupHead& head, Fixture* fxi,
                                              const QLCPoint& /*pt*/,
                                              const QVector<uchar>& aVals,
                                              uint fadeTimeMs)
{
    QLCFixtureDef*  fxDef  = fxi->fixtureDef();
    QLCFixtureMode* fxMode = fxi->fixtureMode();
    if (!fxDef || !fxMode) return;

    for (int c = 0; c < m_columns.size(); ++c)
    {
        const PTColumn& col = m_columns[c];
        if (!col.binding.isValid()) continue;

        if (fxDef->manufacturer() != col.binding.manufacturer) continue;
        if (fxDef->model()        != col.binding.model)        continue;
        if (fxMode->name()        != col.binding.modeName)     continue;

        quint32 absChannel = quint32(col.binding.channelIndex);
        if (absChannel >= fxi->channels()) continue;

        if (!fxMode->heads().isEmpty())
        {
            if (head.head < 0 || head.head >= (int)fxMode->heads().size()) continue;
            if (!fxMode->heads()[head.head].channels().contains(absChannel)) continue;
        }

        uchar aVal = (c < aVals.size()) ? aVals[c] : 0;
        applyFadeValueTimed(fader, m_doc, uni, head.fxi, absChannel, aVal, fadeTimeMs);
    }
}

void PresetTableV2Widget::applyBlendedPointChannels(GenericFader* fader, Universe* uni,
                                                   const GroupHead& head, Fixture* fxi,
                                                   const QLCPoint& /*pt*/,
                                                   const QVector<uchar>& priVals,
                                                   const QVector<uchar>& secVals,
                                                   double dimmer,
                                                   quint32 presetFadeMs,
                                                   int waveShape,
                                                   int waveFadeIn,
                                                   int waveFadeOut,
                                                   uchar intensity)
{
    QLCFixtureDef*  fxDef  = fxi->fixtureDef();
    QLCFixtureMode* fxMode = fxi->fixtureMode();
    if (!fxDef || !fxMode) return;

    double t = dimmer;
    if (t > 1.0)
        t /= 255.0;
    t = qBound(0.0, t, 1.0);
    const bool globalSnap = (waveShape == 1);

    for (int c = 0; c < m_columns.size(); ++c)
    {
        const PTColumn& col = m_columns[c];
        if (!col.binding.isValid()) continue;

        if (fxDef->manufacturer() != col.binding.manufacturer) continue;
        if (fxDef->model()        != col.binding.model)        continue;
        if (fxMode->name()        != col.binding.modeName)     continue;

        quint32 absChannel = quint32(col.binding.channelIndex);
        if (absChannel >= fxi->channels()) continue;

        if (!fxMode->heads().isEmpty())
        {
            if (head.head < 0 || head.head >= (int)fxMode->heads().size()) continue;
            if (!fxMode->heads()[head.head].channels().contains(absChannel)) continue;
        }

        const uchar pri = (c < priVals.size()) ? priVals[c] : 0;
        const uchar sec = (c < secVals.size()) ? secVals[c] : 0;
        const bool sharpWave = (waveFadeIn == 0 && waveFadeOut == 0);
        const bool snap = globalSnap || sharpWave || !col.fade;
        uchar val;
        if (snap)
            val = (t >= 0.5) ? sec : pri;
        else
            val = uchar(qBound(0, int(std::lround(double(pri) + (double(sec) - double(pri)) * t)), 255));

        if (intensity < 255)
            val = uchar(qBound(0, int(std::lround(double(val) * double(intensity) / 255.0)), 255));

        const uint chFade = snap ? 0 : presetFadeMs;
        applyFadeValueTimed(fader, m_doc, uni, head.fxi, absChannel, val, chFade);
    }
}

void PresetTableV2Widget::startSpatialChase(int outputIdx, int rowIdx, const QList<QLCPoint>& points,
                                          const PTTransitionPreset& preset, int gridWidth,
                                          int gridHeight)
{
    if (outputIdx < 0)
        return;
    while (m_spatialChase.size() <= outputIdx)
        m_spatialChase.append(PTSpatialChaseOutput());

    PTSpatialChaseOutput& chase = m_spatialChase[outputIdx];
    chase.active = true;
    chase.targetRow = rowIdx;
    chase.progress = 0.0;
    chase.spatialPreset = preset;
    chase.armed.clear();
    chase.order = PresetTableV2SpatialEngine::buildChaseOrder(points, preset, gridWidth, gridHeight);
}

void PresetTableV2Widget::tickSpatialChase(int outputIdx, MasterTimer* timer,
                                            QList<Universe*>& universes,
                                            const PTOutput& out,
                                            const QVector<uchar>& aVals)
{
    if (outputIdx < 0 || outputIdx >= m_spatialChase.size())
        return;

    PTSpatialChaseOutput& chase = m_spatialChase[outputIdx];
    if (!chase.active)
        return;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp || !timer)
        return;

    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();
    const QMap<QLCPoint, GroupHead>& headsMap =
            (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;

    const quint32 durationMs = qMax(quint32(1), chase.spatialPreset.durationMs);
    const double increment = double(kPTEfxStepMs) / double(durationMs);
    chase.progress = qMin(1.0, chase.progress + increment);

    const quint32 fadeMs = qMax(quint32(1), chase.spatialPreset.fadeMs);

    const int pointCount = chase.order.size();
    for (int i = 0; i < pointCount; ++i)
    {
        const QLCPoint& pt = chase.order.at(i);
        if (chase.armed.contains(pt))
            continue;

        const double threshold = (pointCount <= 1)
                ? 0.0 : double(i) / double(pointCount - 1);
        if (chase.progress < threshold)
            continue;

        auto hit = headsMap.constFind(pt);
        if (hit == headsMap.constEnd())
            continue;
        if (!outputScopeAllowsPoint(out.scope, pt, out))
            continue;

        const GroupHead& head = hit.value();
        Fixture* fxi = m_doc->fixture(head.fxi);
        if (!fxi) continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size()) continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        applyPointChannels(fader.data(), universes[uni], head, fxi, pt, aVals, fadeMs);
        chase.armed.insert(pt);
    }

    if (pointCount == 0 || chase.progress >= 1.0)
    {
        chase.active = false;
        if (outputIdx < m_spatialAppliedRow.size())
            m_spatialAppliedRow[outputIdx] = chase.targetRow;
    }
}

void PresetTableV2Widget::writeContinuousSpatial(int outputIdx, MasterTimer* timer,
                                                 QList<Universe*>& universes,
                                                 const PTOutput& out,
                                                 const QVector<uchar>& priVals,
                                                 const QVector<uchar>& secVals,
                                                 const QSize& gridSize,
                                                 const QMap<QLCPoint, GroupHead>& headsMap,
                                                 const PTTransitionPreset* presetOverride,
                                                 const QVector<uchar>* stagedPriVals,
                                                 const QVector<uchar>* stagedSecVals,
                                                 const PTTransitionPreset* stagedPresetOverride,
                                                 double morphProgress)
{
    Q_UNUSED(timer);

    if (outputIdx < 0)
        return;

    while (m_continuousElapsedMs.size() <= outputIdx)
        m_continuousElapsedMs.append(0);
    m_continuousElapsedMs[outputIdx] += MasterTimer::tick();

    if (!continuousEfxActiveForOutputLocked(outputIdx))
        return;

    const PTTransitionPreset spatialPreset = presetOverride ? *presetOverride
                                                            : continuousPresetForOutputLocked(outputIdx);
    const bool morphOutput = stagedPresetOverride && stagedPriVals && stagedSecVals;
    const PTTransitionPreset stagedPreset = morphOutput ? *stagedPresetOverride : spatialPreset;
    const PTGlobalEffectSettings global = globalEffectSettingsLocked();
    const PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(spatialPreset, &global);
    const quint32 durationMs = qMax(quint32(1), cycleDurationMsLocked(global, spatialPreset));
    const PTDimmerWaveParams stagedWaveParams = PTDimmerWaveEngine::paramsFromPreset(stagedPreset, &global);
    const quint32 stagedDurationMs = qMax(quint32(1), cycleDurationMsLocked(global, stagedPreset));
    const quint32 elapsedMs = quint32(m_continuousElapsedMs[outputIdx]);

    QList<QLCPoint> points;
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        if (outputScopeAllowsPoint(out.scope, it.key(), out))
            points.append(it.key());
    }

    const QList<QLCPoint> order = PresetTableV2SpatialEngine::buildChaseOrder(
            points, spatialPreset, gridSize.width(), gridSize.height());
    const int orderCount = order.size();
    if (orderCount <= 0)
        return;

    QHash<QLCPoint, int> serialIndex;
    for (int i = 0; i < orderCount; ++i)
        serialIndex.insert(order.at(i), i);

    const QList<QLCPoint> stagedOrder = morphOutput
            ? PresetTableV2SpatialEngine::buildChaseOrder(
                points, stagedPreset, gridSize.width(), gridSize.height())
            : QList<QLCPoint>();
    const int stagedOrderCount = stagedOrder.size();
    QHash<QLCPoint, int> stagedSerialIndex;
    for (int i = 0; i < stagedOrderCount; ++i)
        stagedSerialIndex.insert(stagedOrder.at(i), i);

    const quint32 fadeMs = 0;

    for (const QLCPoint& pt : points)
    {
        auto hit = headsMap.constFind(pt);
        if (hit == headsMap.constEnd())
            continue;

        const int headOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                pt.x(), pt.y(), gridSize.width(), gridSize.height(), waveParams);
        const int serialIdx = serialIndex.value(pt, 0);
        const quint32 timeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                serialIdx, orderCount, durationMs, spatialPreset.propagation);
        const float iterator = PTDimmerWaveEngine::iteratorFromElapsed(
                elapsedMs, durationMs, spatialPreset.startOffset, headOffset, timeOffset);
        const float dimmer = PTDimmerWaveEngine::calculateDimmerWave(iterator, waveParams);
        float stagedDimmer = dimmer;
        if (morphOutput)
        {
            const int stagedHeadOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                    pt.x(), pt.y(), gridSize.width(), gridSize.height(), stagedWaveParams);
            const int stagedSerialIdx = stagedSerialIndex.value(pt, 0);
            const quint32 stagedTimeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                    stagedSerialIdx, qMax(1, stagedOrderCount), stagedDurationMs, stagedPreset.propagation);
            const float stagedIterator = PTDimmerWaveEngine::iteratorFromElapsed(
                    elapsedMs, stagedDurationMs, stagedPreset.startOffset,
                    stagedHeadOffset, stagedTimeOffset);
            stagedDimmer = PTDimmerWaveEngine::calculateDimmerWave(stagedIterator, stagedWaveParams);
        }

        const GroupHead& head = hit.value();
        Fixture* fxi = m_doc->fixture(head.fxi);
        if (!fxi)
            continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size())
            continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        if (morphOutput)
        {
            const QVector<uchar> liveValues = continuousColumnValues(
                    m_columns, priVals, secVals, double(dimmer), spatialPreset.waveShape,
                    spatialPreset.waveFadeIn, spatialPreset.waveFadeOut, global.intensity);
            const QVector<uchar> stagedValues = continuousColumnValues(
                    m_columns, *stagedPriVals, *stagedSecVals, double(stagedDimmer),
                    stagedPreset.waveShape, stagedPreset.waveFadeIn,
                    stagedPreset.waveFadeOut, global.intensity);
            const QVector<uchar> finalValues = blendRowValues(liveValues, stagedValues, morphProgress);
            applyPointChannels(fader.data(), universes[uni], head, fxi, pt, finalValues, fadeMs);
        }
        else
        {
            applyBlendedPointChannels(fader.data(), universes[uni], head, fxi, pt,
                                      priVals, secVals, double(dimmer), fadeMs,
                                      spatialPreset.waveShape,
                                      spatialPreset.waveFadeIn, spatialPreset.waveFadeOut,
                                      global.intensity);
        }
    }
}

bool PresetTableV2Widget::matrixProviderReadyLocked() const
{
    return m_linkedTransitionWidgetId != VCWidget::invalidId()
            && (m_cachedTransitionSweepCount > 0 || m_cachedTransitionContinuousCount > 0);
}

bool PresetTableV2Widget::useMatrixEngineLocked() const
{
    return m_spatialEffects.enabled && matrixProviderReadyLocked();
}

bool PresetTableV2Widget::efxActiveForOutputLocked(int outputIdx) const
{
    return sweepEfxActiveForOutputLocked(outputIdx)
            || continuousEfxActiveForOutputLocked(outputIdx);
}

PresetTableV2Widget::PTOutputPlaybackState
PresetTableV2Widget::resolveOutputPlaybackStateLocked(int outputIdx, int activeRow,
                                                       bool hasStaged,
                                                       bool matrixReady,
                                                       bool spatialOn) const
{
    PTOutputPlaybackState state;
    state.transitionOn = sweepEfxActiveForOutputLocked(outputIdx);
    state.continuousFxOn = continuousEfxActiveForOutputLocked(outputIdx);
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx]
            && outputIdx < m_stagedSecondaryRow.size();
    state.secondaryRow = hasStagedSecondary ? m_stagedSecondaryRow[outputIdx]
                                            : effectiveSecondaryRowLocked(outputIdx, activeRow);
    state.crossfadeTransition = crossfadeSweepModeLocked(outputIdx, activeRow, hasStaged);
    state.crossfadeContinuous = continuousCrossfadeModeLocked(outputIdx);
    state.blockMatrixForStaged = hasStaged
            && !state.crossfadeTransition
            && !state.crossfadeContinuous;
    state.matrixForOutput = matrixReady
            && (spatialOn || m_crossfadeEnabled || state.transitionOn || state.continuousFxOn);
    return state;
}

void PresetTableV2Widget::ensureMatrixState(int outputIdx)
{
    while (m_matrixState.size() <= outputIdx)
        m_matrixState.append(PTOutputMatrixState());
}

void PresetTableV2Widget::resetMatrixStateLocked(int outputIdx)
{
    if (outputIdx < 0)
        return;
    ensureMatrixState(outputIdx);
    PTOutputMatrixState& st = m_matrixState[outputIdx];
    st.sweepRunning = false;
    st.sweepManualCrossfade = false;
    st.sweepManualPhase = 0.0;
    st.sweepManualPhasePrev = 0.0;
    st.sweepProgress = 0.0;
    st.sweepFromRow = -1;
    st.sweepToRow = -1;
    st.sweepElapsedMs = 0;
    st.sweepPeakDimmer.clear();
    st.sweepHeldValues.clear();
    st.flashActive = false;
    st.flashPhase = PTFlashPhase::Idle;
    st.flashWaveProgress = 0.0;
    st.appliedRow = -1;
    if (outputIdx < m_continuousElapsedMs.size())
        m_continuousElapsedMs[outputIdx] = 0;
}

void PresetTableV2Widget::resetAllMatrixStatesLocked()
{
    for (int o = 0; o < m_outputs.size(); ++o)
        resetMatrixStateLocked(o);
}

void PresetTableV2Widget::beginMatrixSweepLocked(int outputIdx, int prevRow, int newRowIdx,
                                                bool forceSpatialSweep)
{
    ensureMatrixState(outputIdx);
    PTOutputMatrixState& st = m_matrixState[outputIdx];
    if (newRowIdx < 0 || newRowIdx >= m_rows.size())
        return;

    if (prevRow < 0 || prevRow == newRowIdx)
    {
        st.appliedRow = newRowIdx;
        st.sweepRunning = false;
        st.sweepProgress = 0.0;
        return;
    }

    const PTTransitionPreset fxPreset = sweepEfxActiveForOutputLocked(outputIdx)
            ? sweepPresetForOutputLocked(outputIdx)
            : transitionPresetForOutputLocked(outputIdx);
    if (!forceSpatialSweep
            && PTParamMatrixEngine::sweepInstantForOffset(fxPreset.offsetDirection))
    {
        st.appliedRow = newRowIdx;
        st.sweepRunning = false;
        st.sweepProgress = 0.0;
        return;
    }

    if (forceSpatialSweep)
        resetCrossfadeClockLocked();

    st.sweepFromRow = prevRow;
    st.sweepToRow = newRowIdx;
    st.sweepElapsedMs = 0;
    st.sweepPeakDimmer.clear();
    st.sweepHeldValues.clear();
    st.sweepRunning = true;
    st.sweepProgress = 0.0;
    st.sweepManualPhase = 0.0;
}

void PresetTableV2Widget::releaseMatrixFlashLocked(int outputIdx)
{
    if (outputIdx < 0 || outputIdx >= m_matrixState.size())
        return;

    PTOutputMatrixState& st = m_matrixState[outputIdx];
    if (!st.flashActive)
        return;

    if (st.flashPhase == PTFlashPhase::WaveIn)
    {
        st.flashActive = false;
        st.flashPhase = PTFlashPhase::Idle;
        st.flashWaveProgress = 0.0;
    }
    else if (st.flashPhase == PTFlashPhase::Hold)
    {
        const PTTransitionPreset fxPreset = transitionPresetForOutputLocked(outputIdx);
        if (PTParamMatrixEngine::waveFrontFromOffset(fxPreset.offsetDirection) <= 0)
        {
            st.flashActive = false;
            st.flashPhase = PTFlashPhase::Idle;
        }
        else
        {
            st.flashPhase = PTFlashPhase::WaveOut;
            st.flashWaveProgress = 0.0;
        }
    }
}

void PresetTableV2Widget::requestTableFlash(int tableRowIndex, int transitionPresetIndex)
{
    Q_UNUSED(transitionPresetIndex);

    QMutexLocker lk(&m_stateMutex);
    if (tableRowIndex < 0 || tableRowIndex >= m_rows.size())
        return;

    const QVector<uchar> flashVals = m_rows[tableRowIndex].values;

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        if (o >= m_activeRow.size() || m_activeRow[o] < 0)
            continue;

        ensureMatrixState(o);
        PTOutputMatrixState& st = m_matrixState[o];
        if (!st.flashActive)
        {
            if (continuousEfxActiveForOutputLocked(o))
            {
                const PTTransitionPreset fxPreset = continuousPresetForOutputLocked(o);
                if (o < m_liveSecondaryRow.size() && m_liveSecondaryRow[o] >= 0)
                    st.preFlashState = PTPreFlashState::ColorFx;
                else if (st.sweepRunning)
                    st.preFlashState = PTPreFlashState::Sweep;
                else
                    st.preFlashState = PTPreFlashState::Idle;
            }
            else if (st.sweepRunning)
                st.preFlashState = PTPreFlashState::Sweep;
            else
                st.preFlashState = PTPreFlashState::Idle;
        }

        st.flashValues = flashVals;
        st.flashActive = true;
        const PTTransitionPreset fxPreset = transitionPresetForOutputLocked(o);
        if (PTParamMatrixEngine::waveFrontFromOffset(fxPreset.offsetDirection) <= 0)
            st.flashPhase = PTFlashPhase::Hold;
        else
        {
            st.flashPhase = PTFlashPhase::WaveIn;
            st.flashWaveProgress = 0.0;
        }
    }
}

static float matrixDimmerAtPoint(const QLCPoint& pt,
                                 quint32 elapsedMs,
                                 quint32 cycleMs,
                                 const PTTransitionPreset& preset,
                                 const PTGlobalEffectSettings& global,
                                 const QSize& gridSize,
                                 int serialIndex,
                                 int serialCount)
{
    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(preset, &global);
    if (global.fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    const int headOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
            pt.x(), pt.y(), gridSize.width(), gridSize.height(), waveParams);
    const quint32 timeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
            serialIndex, serialCount, cycleMs, waveParams.propagation);
    const float iterator = PTDimmerWaveEngine::iteratorFromElapsed(
            elapsedMs, cycleMs, waveParams.startOffset, headOffset, timeOffset);
    return PTDimmerWaveEngine::calculateDimmerWave(iterator, waveParams);
}

void PresetTableV2Widget::writeMatrixSpatial(int outputIdx, MasterTimer* timer,
                                              QList<Universe*>& universes,
                                              const PTOutput& out, int activeRow, int secondaryRow,
                                              const PTTransitionPreset& preset,
                                              const PTGlobalEffectSettings& global,
                                              const QSize& gridSize,
                                              const QMap<QLCPoint, GroupHead>& headsMap,
                                              bool forceContinuousBlend,
                                              const QVector<uchar>* primaryOverride,
                                              const QVector<uchar>* secondaryOverride,
                                              const QVector<uchar>* stagedPrimaryOverride,
                                              const QVector<uchar>* stagedSecondaryOverride,
                                              const PTTransitionPreset* stagedPresetOverride,
                                              double morphProgress)
{
    Q_UNUSED(timer);

    ensureMatrixState(outputIdx);
    PTOutputMatrixState& st = m_matrixState[outputIdx];

    const bool useSecondaryBlend = forceContinuousBlend
            || (preset.playbackMode == PTTransitionMode::Continuous
                && secondaryRow >= 0 && secondaryRow < m_rows.size());

    const int blendFromRow = st.sweepRunning ? st.sweepFromRow : activeRow;
    const int blendToRow = st.sweepRunning ? st.sweepToRow
            : (useSecondaryBlend ? secondaryRow : activeRow);

    const QVector<uchar> priVals = primaryOverride ? *primaryOverride
            : ((blendFromRow >= 0 && blendFromRow < m_rows.size())
                ? m_rows[blendFromRow].values : QVector<uchar>());
    const QVector<uchar> secVals = secondaryOverride ? *secondaryOverride
            : ((blendToRow >= 0 && blendToRow < m_rows.size())
                ? m_rows[blendToRow].values : priVals);
    const bool morphOutput = stagedPresetOverride && stagedPrimaryOverride && stagedSecondaryOverride;
    const PTTransitionPreset stagedPreset = morphOutput ? *stagedPresetOverride : preset;

    const double increment = PTParamMatrixEngine::transitionIncrement(global, preset);
    const PTTransitionAxis spatialAxis = (global.fxOrientation == 1)
            ? PTTransitionAxis::Y : preset.axis;

    const int fixtureCount = qMax(1,
            PTParamMatrixEngine::linearPosition(
                    QLCPoint(qMax(0, gridSize.width() - 1), qMax(0, gridSize.height() - 1)),
                    spatialAxis, gridSize.width()) + 1);

    const bool continuousFx = forceContinuousBlend
            || (!st.sweepRunning
                && preset.playbackMode == PTTransitionMode::Continuous);

    const quint32 cycleMs = qMax(quint32(1), cycleDurationMsLocked(global, preset));

    while (m_continuousElapsedMs.size() <= outputIdx)
        m_continuousElapsedMs.append(0);
    if (continuousFx && !st.flashActive)
        m_continuousElapsedMs[outputIdx] += MasterTimer::tick();
    const quint32 elapsedMs = quint32(m_continuousElapsedMs[outputIdx]);

    QList<QLCPoint> points;
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        if (outputScopeAllowsPoint(out.scope, it.key(), out))
            points.append(it.key());
    }

    const PTSpatialFixturePlan spatialPlan = PTSpatialFixturePlan::build(
            points, preset, global, gridSize.width(), gridSize.height());
    const int serialCount = qMax(1, spatialPlan.count());
    const PTSpatialFixturePlan stagedSpatialPlan = morphOutput
            ? PTSpatialFixturePlan::build(points, stagedPreset, global,
                                          gridSize.width(), gridSize.height())
            : PTSpatialFixturePlan();
    const int stagedSerialCount = qMax(1, stagedSpatialPlan.count());

    auto dimmerAtPoint = [&](const QLCPoint& pt, quint32 timeMs) -> float {
        const int serialIdx = spatialPlan.indexByPoint.value(pt, 0);
        return matrixDimmerAtPoint(pt, timeMs, cycleMs, preset, global, gridSize,
                                   serialIdx, serialCount);
    };
    auto stagedDimmerAtPoint = [&](const QLCPoint& pt, quint32 timeMs) -> float {
        const int serialIdx = stagedSpatialPlan.indexByPoint.value(pt, 0);
        const quint32 stagedCycleMs = qMax(quint32(1), cycleDurationMsLocked(global, stagedPreset));
        return matrixDimmerAtPoint(pt, timeMs, stagedCycleMs, stagedPreset, global,
                                   gridSize, serialIdx, stagedSerialCount);
    };

    const bool snapBlend = (preset.waveShape == 1);
    auto applySweepBlend = [&](const QVector<uchar>& fromRow, const QVector<uchar>& toRow,
                               float blend) -> QVector<uchar> {
        if (blend <= 0.0f)
            return PTParamMatrixEngine::blendWithIntensity(fromRow, global.intensity);
        if (blend >= 1.0f)
            return PTParamMatrixEngine::blendWithIntensity(toRow, global.intensity);
        QVector<uchar> blended = PresetTableV2SpatialEngine::blendValues(
                fromRow, toRow, double(blend), snapBlend);
        return PTParamMatrixEngine::blendWithIntensity(blended, global.intensity);
    };

    if (st.sweepRunning && !st.sweepManualCrossfade)
        st.sweepElapsedMs += MasterTimer::tick();

    if (st.flashActive)
    {
        if (st.flashPhase == PTFlashPhase::WaveIn)
        {
            st.flashWaveProgress += increment;
            if (st.flashWaveProgress >= 1.0)
            {
                st.flashPhase = PTFlashPhase::Hold;
                st.flashWaveProgress = 0.0;
            }
        }
        else if (st.flashPhase == PTFlashPhase::WaveOut)
        {
            st.flashWaveProgress += increment;
            if (st.flashWaveProgress >= 1.0)
            {
                st.flashActive = false;
                st.flashPhase = PTFlashPhase::Idle;
                st.flashWaveProgress = 0.0;
            }
        }
    }
    const quint32 fadeMs = st.flashActive ? qMax(quint32(1), preset.fadeMs) : 0;
    const int waveFront = PTParamMatrixEngine::waveFrontFromOffset(preset.offsetDirection);

    const double sweepTimedProgress01 = (st.sweepRunning && !st.sweepManualCrossfade && cycleMs > 0)
            ? qMin(1.0, double(st.sweepElapsedMs) / double(cycleMs * 2))
            : 0.0;

    int sweepScopeCount = 0;
    int sweepDoneCount = 0;

    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        const QLCPoint& pt = it.key();
        if (!outputScopeAllowsPoint(out.scope, pt, out))
            continue;

        const int posIndex = PTParamMatrixEngine::linearPosition(pt, spatialAxis, gridSize.width());

        const GroupHead& head = it.value();
        Fixture* fxi = m_doc->fixture(head.fxi);
        if (!fxi)
            continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size())
            continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        auto applyRow = [&](const QVector<uchar>& rowVals) {
            QVector<uchar> vals = PTParamMatrixEngine::blendWithIntensity(rowVals, global.intensity);
            applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, fadeMs);
        };

        auto applyContinuous = [&]() {
            const float dimmer = dimmerAtPoint(pt, elapsedMs);
            if (morphOutput)
            {
                const float stagedDimmer = stagedDimmerAtPoint(pt, elapsedMs);
                const QVector<uchar> liveValues = continuousColumnValues(
                        m_columns, priVals, secVals, double(dimmer), preset.waveShape,
                        preset.waveFadeIn, preset.waveFadeOut, global.intensity);
                const QVector<uchar> stagedValues = continuousColumnValues(
                        m_columns, *stagedPrimaryOverride, *stagedSecondaryOverride,
                        double(stagedDimmer), stagedPreset.waveShape,
                        stagedPreset.waveFadeIn, stagedPreset.waveFadeOut, global.intensity);
                const QVector<uchar> finalValues = blendRowValues(liveValues, stagedValues,
                                                                  morphProgress);
                applyPointChannels(fader.data(), universes[uni], head, fxi, pt,
                                   finalValues, 0);
            }
            else
            {
                applyBlendedPointChannels(fader.data(), universes[uni], head, fxi, pt,
                                          priVals, secVals, double(dimmer), 0, preset.waveShape,
                                          preset.waveFadeIn, preset.waveFadeOut, global.intensity);
            }
        };

        if (st.flashActive)
        {
            if (st.flashPhase == PTFlashPhase::Hold)
                applyRow(st.flashValues);
            else if (st.flashPhase == PTFlashPhase::WaveIn)
            {
                const bool showFlash = PTParamMatrixEngine::waveShowNew(
                        posIndex, fixtureCount, st.flashWaveProgress, waveFront);
                if (showFlash)
                    applyRow(st.flashValues);
                else if (continuousFx)
                    applyContinuous();
                else
                    applyRow(priVals);
            }
            else
            {
                const bool showUnder = PTParamMatrixEngine::waveShowNew(
                        posIndex, fixtureCount, st.flashWaveProgress, waveFront);
                if (showUnder)
                {
                    if (continuousFx)
                        applyContinuous();
                    else
                        applyRow(priVals);
                }
                else
                    applyRow(st.flashValues);
            }
        }
        else if (st.sweepRunning && st.sweepManualCrossfade)
        {
            if (st.sweepManualPhase + 0.002 < st.sweepManualPhasePrev)
            {
                st.sweepPeakDimmer.clear();
                st.sweepHeldValues.clear();
            }
            st.sweepManualPhasePrev = st.sweepManualPhase;

            const float blend = spatialPlan.sweepBlend01(
                    st.sweepManualPhase, pt, preset, global);
            const QVector<uchar> vals = applySweepBlend(priVals, secVals, blend);
            applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, 0);
        }
        else if (st.sweepRunning)
        {
            ++sweepScopeCount;
            const float blend = spatialPlan.sweepBlend01(
                    sweepTimedProgress01, pt, preset, global);
            const QVector<uchar> vals = applySweepBlend(priVals, secVals, blend);
            applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, 0);
            if (blend >= 0.99f)
                ++sweepDoneCount;
        }
        else if (continuousFx)
        {
            applyContinuous();
        }
        else
        {
            applyRow(priVals);
        }
    }

    if (st.sweepRunning && !st.sweepManualCrossfade)
    {
        st.sweepProgress = sweepTimedProgress01;
        const bool allDone = (sweepScopeCount > 0 && sweepDoneCount >= sweepScopeCount);
        const bool timedOut = st.sweepElapsedMs >= cycleMs * 2;
        if (allDone || timedOut || sweepTimedProgress01 >= 1.0)
        {
            st.sweepRunning = false;
            st.sweepProgress = 0.0;
            st.appliedRow = st.sweepToRow;
            st.sweepPeakDimmer.clear();
            st.sweepHeldValues.clear();
        }
    }
    else if (!st.flashActive && activeRow >= 0 && !st.sweepManualCrossfade)
    {
        st.appliedRow = activeRow;
    }
}

void PresetTableV2Widget::writeDMXFixtureGroup(MasterTimer* timer, QList<Universe*>& universes,
                                                uchar xfEffective)
{
    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp) return;

    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();

    const bool spatialOn = m_spatialEffects.enabled;
    const QSize gridSize = grp->size();

    if (m_spatialAppliedRow.size() != m_outputs.size())
        m_spatialAppliedRow.resize(m_outputs.size());
    if (m_spatialChase.size() != m_outputs.size())
        m_spatialChase.resize(m_outputs.size());
    if (m_matrixState.size() != m_outputs.size())
        m_matrixState.resize(m_outputs.size());
    if (m_flashInputHeldRow.size() != m_outputs.size())
        m_flashInputHeldRow.fill(-1, m_outputs.size());

    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
    {
        m_cachedTransitionSweepCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
        m_cachedTransitionContinuousCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
    }

    const bool matrixReady = matrixProviderReadyLocked();
    const PTGlobalEffectSettings globalFx = globalEffectSettingsLocked();

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int activeRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const int stagedRow = (o < m_stagedRow.size()) ? m_stagedRow[o] : -1;

        if (activeRow < 0 || activeRow >= m_rows.size()) continue;

        const PTOutput& out = m_outputs[o];
        if (out.scope == PTOutputScope::Mask && !docMask.isActive())
            continue;

        const QVector<uchar>& aVals = m_rows[activeRow].values;
        const QVector<uchar>* bVals = (stagedRow >= 0 && stagedRow < m_rows.size())
            ? &m_rows[stagedRow].values : nullptr;

        const QMap<QLCPoint, GroupHead>& headsMap =
                (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;

        const bool hasStaged = (stagedRow >= 0);
        const PTOutputPlaybackState playback = resolveOutputPlaybackStateLocked(
                o, activeRow, hasStaged, matrixReady, spatialOn);
        const bool sweepOn = playback.transitionOn;
        const bool contOn = playback.continuousFxOn;
        const int secRow = playback.secondaryRow;
        const bool crossfadeSweep = playback.crossfadeTransition;
        const bool crossfadeCont = playback.crossfadeContinuous;
        const bool blockMatrixForStaged = playback.blockMatrixForStaged;
        const bool matrixForOutput = playback.matrixForOutput;

        if (matrixForOutput && !blockMatrixForStaged)
        {
            ensureMatrixState(o);
            PTOutputMatrixState& st = m_matrixState[o];

            if (contOn && secRow >= 0)
            {
                const PTContinuousLayerState layer =
                        continuousLayerStateForOutputLocked(o, activeRow, xfEffective);
                if (layer.active)
                {
                    st.sweepRunning = false;
                    st.sweepManualCrossfade = false;
                    writeMatrixSpatial(o, timer, universes, out, activeRow, secRow,
                                       layer.livePreset, globalFx, gridSize, headsMap, true,
                                       &layer.livePrimaryValues, &layer.liveSecondaryValues,
                                       layer.hasStaged ? &layer.primaryValues : nullptr,
                                       layer.hasStaged ? &layer.secondaryValues : nullptr,
                                       layer.hasStaged ? &layer.preset : nullptr,
                                       crossfadeProgress01Locked(xfEffective));
                    continue;
                }
            }

            if (crossfadeSweep)
            {
                PTTransitionPreset sweepPreset = sweepPresetForOutputLocked(o);
                if (sweepPreset.enabled && stagedRow >= 0)
                {
                    if (!st.sweepRunning
                            || st.sweepFromRow != activeRow || st.sweepToRow != stagedRow)
                        beginMatrixSweepLocked(o, activeRow, stagedRow, true);
                    st.sweepManualCrossfade = true;
                    st.sweepManualPhase = crossfadeProgress01Locked(xfEffective);
                    writeMatrixSpatial(o, timer, universes, out, activeRow, activeRow,
                                       sweepPreset, globalFx, gridSize, headsMap);
                    continue;
                }
            }

            st.sweepManualCrossfade = false;

            if (sweepOnPrimaryChangeLocked(o, activeRow) && !st.flashActive && !st.sweepRunning
                    && activeRow >= 0 && activeRow != st.appliedRow)
                beginMatrixSweepLocked(o, st.appliedRow, activeRow);

            if (sweepOn && st.sweepRunning)
            {
                PTTransitionPreset sweepPreset = sweepPresetForOutputLocked(o);
                if (sweepPreset.enabled)
                {
                    writeMatrixSpatial(o, timer, universes, out, activeRow, activeRow,
                                       sweepPreset, globalFx, gridSize, headsMap);
                    continue;
                }
            }

            if (sweepOn)
            {
                PTTransitionPreset sweepPreset = sweepPresetForOutputLocked(o);
                if (sweepPreset.enabled)
                {
                    writeMatrixSpatial(o, timer, universes, out, activeRow, activeRow,
                                       sweepPreset, globalFx, gridSize, headsMap);
                    continue;
                }
            }
        }

        if (crossfadeSweep)
        {
            for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
            {
                const QLCPoint& pt = it.key();
                if (!outputScopeAllowsPoint(out.scope, pt, out))
                    continue;
                const GroupHead& head = it.value();
                Fixture* fxi = m_doc->fixture(head.fxi);
                if (!fxi)
                    continue;
                quint32 uni = fxi->universe();
                if ((int)uni >= universes.size())
                    continue;
                auto fader = m_faders.value(uni);
                if (fader.isNull())
                {
                    fader = universes[uni]->requestFader(Universe::Auto);
                    m_faders.insert(uni, fader);
                }
                QVector<uchar> vals = aVals;
                vals = PTParamMatrixEngine::blendWithIntensity(vals, globalFx.intensity);
                applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, 0);
            }
            continue;
        }

        const bool sweepActive = sweepOn;
        const bool contActive = contOn;

        const bool useContinuous = spatialOn && contActive
                && !blockMatrixForStaged
                && secRow >= 0 && secRow < m_rows.size()
                && !matrixForOutput;

        if (useContinuous)
        {
            const PTContinuousLayerState layer =
                    continuousLayerStateForOutputLocked(o, activeRow, xfEffective);
            if (layer.active)
            {
                writeContinuousSpatial(o, timer, universes, out,
                                       layer.livePrimaryValues, layer.liveSecondaryValues,
                                       gridSize, headsMap, &layer.livePreset,
                                       layer.hasStaged ? &layer.primaryValues : nullptr,
                                       layer.hasStaged ? &layer.secondaryValues : nullptr,
                                       layer.hasStaged ? &layer.preset : nullptr,
                                       crossfadeProgress01Locked(xfEffective));
            }
            continue;
        }

        PTTransitionPreset transPreset = transitionPresetForOutputLocked(o);
        const bool efxActive = sweepActive || contActive;
        const bool useSpatial = spatialOn && efxActive && !blockMatrixForStaged
                && transPreset.enabled && !matrixForOutput;
        const int appliedRow = (o < m_spatialAppliedRow.size()) ? m_spatialAppliedRow[o] : -1;

        if (useSpatial)
        {
            PTSpatialChaseOutput& chase = m_spatialChase[o];
            if (activeRow != appliedRow && (!chase.active || chase.targetRow != activeRow))
            {
                QList<QLCPoint> points;
                for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
                {
                    if (outputScopeAllowsPoint(out.scope, it.key(), out))
                        points.append(it.key());
                }
                startSpatialChase(o, activeRow, points, transPreset,
                                  gridSize.width(), gridSize.height());
            }

            if (m_spatialChase[o].active)
            {
                tickSpatialChase(o, timer, universes, out, aVals);
                continue;
            }
        }

        for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
        {
            const QLCPoint&  pt   = it.key();
            const GroupHead& head = it.value();

            if (!outputScopeAllowsPoint(out.scope, pt, out))
                continue;

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

            for (int c = 0; c < m_columns.size(); ++c)
            {
                const PTColumn& col = m_columns[c];
                if (!col.binding.isValid()) continue;

                if (fxDef->manufacturer() != col.binding.manufacturer) continue;
                if (fxDef->model()        != col.binding.model)        continue;
                if (fxMode->name()        != col.binding.modeName)     continue;

                quint32 absChannel = quint32(col.binding.channelIndex);
                if (absChannel >= fxi->channels()) continue;

                if (!fxMode->heads().isEmpty())
                {
                    if (head.head < 0 || head.head >= (int)fxMode->heads().size()) continue;
                    if (!fxMode->heads()[head.head].channels().contains(absChannel)) continue;
                }

                uchar aVal = (c < aVals.size()) ? aVals[c] : 0;
                uchar bVal = (bVals && c < bVals->size()) ? (*bVals)[c] : aVal;
                const bool linearCrossfade = m_crossfadeEnabled && hasStaged
                        && !crossfadeSweep;
                applyFadeValue(fader.data(), m_doc, universes[uni],
                               head.fxi, absChannel,
                               aVal, bVal,
                               linearCrossfade, linearCrossfade && hasStaged,
                               col.fade, xfEffective);
            }
        }

        if (useSpatial && o < m_spatialAppliedRow.size() && !m_spatialChase[o].active)
            m_spatialAppliedRow[o] = activeRow;
    }
}

// ==========================================================================
// writeDMX (MasterTimer thread)
// ==========================================================================

void PresetTableV2Widget::slotFixtureGroupMaskChanged(quint32 groupId)
{
    if (m_mode != PTMode::FixtureGroup || m_fixtureGroupId != groupId)
        return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
    }
    refreshRowHighlights();
}

void PresetTableV2Widget::writeDMX(MasterTimer* timer, QList<Universe*> universes)
{
    QMutexLocker lk(&m_stateMutex);

    if (m_crossfadeEnabled && timer)
        tickCrossfadeClockLocked(timer);

    const uchar xfPos      = m_crossfadeGlobalPos;
    const uchar xfStartPos = m_crossfadeStartPos;
    const uchar xfEffective = crossfadeEffectiveLocked(xfPos, xfStartPos);

    if (m_mode == PTMode::FixtureGroup)
        writeDMXFixtureGroup(timer, universes, xfEffective);
    else
        writeDMXLegacy(universes, xfEffective);
}

// ==========================================================================
// External input
// ==========================================================================

void PresetTableV2Widget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
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

        const uchar target = m_crossfadeStagedAtLowSide ? 255 : 0;
        if (m_crossfadeEnabled && crossfadeManualControlEnabledLocked()
                && value == target && crossfadeHasStagedChangesLocked())
        {
            promoteStagedToLiveLocked();
            m_crossfadeStagedAtLowSide = !m_crossfadeStagedAtLowSide;
            m_crossfadeStartPos = value;
            resetCrossfadeClockLocked();
        }
        else if (m_crossfadeEnabled && crossfadeManualControlEnabledLocked()
                 && (value == 0 || value == 255)
                 && !crossfadeHasStagedChangesLocked())
        {
            m_crossfadeEditLaneStaged = true;
            m_crossfadeStagedAtLowSide = (value == 0);
            m_crossfadeStartPos = value;
            m_crossfadeSessionActive = false;
            resetCrossfadeClockLocked();
        }

        m_crossfadePrevPos = value;
        lk2.unlock();
        refreshRowHighlights();
        return;
    }

    // Per-output row selector (ID = o)
    for (int o = 0; o < numOutputs; ++o)
    {
        if (checkInputSource(universe, pagedCh, value, sender(), quint8(o)))
        {
            const bool wasFlash = (o < m_flashInputHeldRow.size() && m_flashInputHeldRow[o] >= 0);
            const bool isFlash = (value >= 101);

            if (isFlash)
            {
                const int flashRow = int(value) - 101;
                if (flashRow >= 0 && flashRow < numRows)
                {
                    int effectIdx = 0;
                    {
                        QMutexLocker lk2(&m_stateMutex);
                        if (sweepEfxActiveForOutputLocked(o))
                            effectIdx = liveSweepPresetIndexLocked(o);
                        else if (continuousEfxActiveForOutputLocked(o))
                            effectIdx = liveContinuousPresetIndexLocked(o);
                        if (o < m_flashInputHeldRow.size())
                            m_flashInputHeldRow[o] = flashRow;
                    }
                    requestTableFlash(flashRow, effectIdx >= 0 ? effectIdx : 0);
                }
                refreshRowHighlights();
                return;
            }

            if (wasFlash)
            {
                QMutexLocker lk2(&m_stateMutex);
                if (o < m_flashInputHeldRow.size())
                    m_flashInputHeldRow[o] = -1;
                releaseMatrixFlashLocked(o);
            }

            int rowIdx = (value == 0) ? -1 : qMin<int>(int(value) - 1, numRows - 1);
            if (xfEnabled)
            {
                bool routeToStaged = false;
                {
                    QMutexLocker lk2(&m_stateMutex);
                    routeToStaged = crossfadeRoutesToStagedLocked();
                    if (!routeToStaged && o < m_stagedRow.size())
                        m_stagedRow[o] = -1;
                }
                if (!routeToStaged)
                {
                    setActiveRow(o, rowIdx);
                    refreshRowHighlights();
                    return;
                }

                // Crossfade: staged primary follows the currently armed fader side.
                QMutexLocker lk2(&m_stateMutex);
                if (o < m_stagedRow.size())
                {
                    if (rowIdx == m_activeRow[o])
                        m_stagedRow[o] = -1;
                    else
                    {
                        const int prevStaged = m_stagedRow[o];
                        armCrossfadeStagingLocked();
                        m_stagedRow[o] = rowIdx;
                        if (rowIdx != prevStaged && !crossfadeManualControlEnabledLocked())
                            resetCrossfadeClockLocked();
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

    int sweepPresetCount = 0;
    int continuousPresetCount = 0;
    {
        QMutexLocker lk3(&m_stateMutex);
        sweepPresetCount = m_cachedTransitionSweepCount;
        continuousPresetCount = m_cachedTransitionContinuousCount;
        if (sweepPresetCount <= 0 && continuousPresetCount <= 0)
        {
            if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
            {
                sweepPresetCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
                continuousPresetCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
            }
        }
    }

    for (int o = 0; o < numOutputs; ++o)
    {
        if (o >= 64)
            break;

        if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::transSweep(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const bool toStaged = xfEnabled && crossfadeRoutesToStagedLocked();
            const int prevSweep = (o < m_liveSweepPreset.size()) ? m_liveSweepPreset[o] : -1;
            if (toStaged)
            {
                armCrossfadeStagingLocked();
                stageSweepPresetLocked(o, PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                        value, sweepPresetCount));
            }
            else if (o < m_liveSweepPreset.size())
            {
                m_liveSweepPreset[o] = PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                        value, sweepPresetCount);
                if (o < m_continuousElapsedMs.size())
                    m_continuousElapsedMs[o] = 0;
                if (o < m_spatialAppliedRow.size())
                    m_spatialAppliedRow[o] = -1;
                m_spatialChase[o] = PTSpatialChaseOutput();

                if (m_liveSweepPreset[o] < 0)
                    resetMatrixStateLocked(o);
                else if (m_liveSweepPreset[o] != prevSweep)
                {
                    resetMatrixStateLocked(o);
                    if (o < m_spatialAppliedRow.size())
                        m_spatialAppliedRow[o] = -1;
                    if (o < m_spatialChase.size())
                        m_spatialChase[o] = PTSpatialChaseOutput();
                }
            }
            lk2.unlock();
            refreshTransitionPresetCache();
            sendFeedback(value, PTInputId::transSweep(o));
            return;
        }

        if (o < 64 && checkInputSource(universe, pagedCh, value, sender(), PTInputId::transContinuousBank(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const bool toStaged = continuousFxSelectorToStagedLocked();
            if (toStaged)
            {
                armCrossfadeStagingLocked();
                stageContinuousPresetLocked(o, PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                        value, continuousPresetCount));
                resetCrossfadeClockLocked();
            }
            else if (o < m_liveContinuousPreset.size())
            {
                m_liveContinuousPreset[o] = PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                        value, continuousPresetCount);
                if (o < m_continuousElapsedMs.size())
                    m_continuousElapsedMs[o] = 0;
            }
            lk2.unlock();
            refreshTransitionPresetCache();
            sendFeedback(value, PTInputId::transContinuousBank(o));
            return;
        }

        if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::transSecondaryRow(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const bool toStaged = xfEnabled && crossfadeRoutesToStagedLocked();
            if (toStaged)
            {
                armCrossfadeStagingLocked();
                stageSecondaryRowLocked(o, PresetTableV2SpatialEngine::tableRowIndexFromInput(
                        value, numRows));
            }
            else if (o < m_liveSecondaryRow.size())
            {
                m_liveSecondaryRow[o] = PresetTableV2SpatialEngine::tableRowIndexFromInput(
                        value, numRows);
                if (o < m_continuousElapsedMs.size())
                    m_continuousElapsedMs[o] = 0;
            }
            lk2.unlock();
            refreshTransitionPresetCache();
            sendFeedback(value, PTInputId::transSecondaryRow(o));
            return;
        }
    }
}

void PresetTableV2Widget::updateFeedback()
{
    QMutexLocker lk(&m_stateMutex);
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        sendFeedback(ar < 0 ? 0 : ar + 1, quint8(o));

        const int liveSweep = (o < m_liveSweepPreset.size())
                ? m_liveSweepPreset[o] : m_outputs[o].sweepPresetIndex;
        sendFeedback(liveSweep < 0 ? 0 : liveSweep + 1, PTInputId::transSweep(o));

        const int liveCont = (o < m_liveContinuousPreset.size())
                ? m_liveContinuousPreset[o] : m_outputs[o].continuousPresetIndex;
        if (o < 64)
            sendFeedback(liveCont < 0 ? 0 : liveCont + 1, PTInputId::transContinuousBank(o));

        const int liveSec = (o < m_liveSecondaryRow.size()) ? m_liveSecondaryRow[o] : -1;
        if (liveSec >= 0)
            sendFeedback(liveSec + 1, PTInputId::transSecondaryRow(o));
        else if (o < m_outputs.size() && m_outputs[o].secondaryRowIndex >= 0)
            sendFeedback(m_outputs[o].secondaryRowIndex + 1, PTInputId::transSecondaryRow(o));
        else
            sendFeedback(0, PTInputId::transSecondaryRow(o));
    }
}

// ==========================================================================
// Properties dialog
// ==========================================================================

void PresetTableV2Widget::editProperties()
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
    PTContinuousFxSelectorMode contFxSelectorMode;
    PTSpatialEffectSettings spatialCopy;
    quint32 linkedTransitionId;
    QSharedPointer<QLCInputSource> xfSrc;
    {
        QMutexLocker lk(&m_stateMutex);
        xfEnabled = m_crossfadeEnabled;
        contFxSelectorMode = m_continuousFxSelectorMode;
        spatialCopy = m_spatialEffects;
        linkedTransitionId = m_linkedTransitionWidgetId;
    }
    xfSrc = inputSource(quint8(255));

    PresetTableV2ConfigDialog dlg(m_doc, colsCopy, outsCopy, srcsCopy,
                                xfEnabled, xfSrc, contFxSelectorMode, page(),
                                modeCopy, groupIdCopy, spatialCopy, linkedTransitionId, this);

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
        m_spatialAppliedRow.resize(m_outputs.size());
        m_spatialAppliedRow.fill(-1);

        m_mode          = newMode;
        m_fixtureGroupId = newGroupId;
        m_spatialEffects.enabled = dlg.spatialEffectsEnabled();
        m_linkedTransitionWidgetId = dlg.linkedTransitionWidgetId();
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        syncLiveTransitionFromOutputs();
    }

    refreshTransitionPresetCache();
    {
        QMutexLocker lk(&m_stateMutex);
        syncLiveTransitionFromOutputs();
    }

    // Apply new input sources
    for (int o = 0; o < newOuts.size(); ++o)
    {
        setInputSource(dlg.inputSource(o), PTInputId::rowSelector(o));
        if (newMode == PTMode::FixtureGroup)
        {
            setInputSource(dlg.transSweepInputSource(o), PTInputId::transSweep(o));
            setInputSource(dlg.transContinuousInputSource(o), PTInputId::transContinuousBank(o));
            setInputSource(dlg.transSecondaryInputSource(o), PTInputId::transSecondaryRow(o));
        }
    }

    // Global crossfade input + toggle
    setInputSource(dlg.crossfadeInputSource(), PTInputId::kCrossfade);
    {
        QMutexLocker lk(&m_stateMutex);
        m_crossfadeEnabled = dlg.crossfadeEnabled();
        m_continuousFxSelectorMode = dlg.continuousFxSelectorMode();
        if (!m_crossfadeEnabled)
        {
            m_stagedRow.fill(-1, m_stagedRow.size());
            m_stagedSecondaryRow.fill(-1, m_stagedSecondaryRow.size());
            m_stagedSweepPreset.fill(-1, m_stagedSweepPreset.size());
            m_stagedContinuousPreset.fill(-1, m_stagedContinuousPreset.size());
            m_stagedSecondaryValid.fill(false, m_stagedSecondaryValid.size());
            m_stagedSweepValid.fill(false, m_stagedSweepValid.size());
            m_stagedContinuousValid.fill(false, m_stagedContinuousValid.size());
            m_crossfadeGlobalPos = 0;
            m_crossfadeStartPos  = 0;
            m_crossfadePrevPos   = 0;
            m_crossfadeStagedAtLowSide = true;
            m_crossfadeSessionActive = false;
            m_crossfadeEditLaneStaged = true;
            resetCrossfadeClockLocked();
        }
    }

    rebuildTable();
    m_doc->setModified();
}

// ==========================================================================
// createCopy
// ==========================================================================

VCWidget* PresetTableV2Widget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    PresetTableV2Widget* copy = new PresetTableV2Widget(parent, m_doc);
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
    bool              xfStagedAtLowSideCopy;
    bool              xfEditLaneStagedCopy;
    PTMode            modeCopy;
    quint32           groupIdCopy;
    PTSpatialEffectSettings spatialCopy;
    PTContinuousFxSelectorMode contFxSelectorModeCopy;
    quint32 linkedTransitionCopy;

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
        xfStagedAtLowSideCopy = m_crossfadeStagedAtLowSide;
        xfEditLaneStagedCopy = m_crossfadeEditLaneStaged;
        modeCopy        = m_mode;
        groupIdCopy     = m_fixtureGroupId;
        spatialCopy     = m_spatialEffects;
        contFxSelectorModeCopy = m_continuousFxSelectorMode;
        linkedTransitionCopy = m_linkedTransitionWidgetId;
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
        copy->m_crossfadeStagedAtLowSide = xfStagedAtLowSideCopy;
        copy->m_crossfadeEditLaneStaged = xfEditLaneStagedCopy;
        copy->m_mode               = modeCopy;
        copy->m_fixtureGroupId     = groupIdCopy;
        copy->m_spatialEffects             = spatialCopy;
        copy->m_continuousFxSelectorMode   = contFxSelectorModeCopy;
        copy->m_linkedTransitionWidgetId   = linkedTransitionCopy;
        copy->m_spatialAppliedRow.resize(outsCopy.size());
        copy->m_spatialAppliedRow.fill(-1);
        copy->m_spatialChase.resize(outsCopy.size());
        copy->m_spatialChase.fill(PTSpatialChaseOutput(), outsCopy.size());
        copy->syncLiveTransitionFromOutputs();
    }

    // Copy input sources
    for (int o = 0; o < outsCopy.size(); ++o)
    {
        copy->setInputSource(inputSource(PTInputId::rowSelector(o)), PTInputId::rowSelector(o));
        if (modeCopy == PTMode::FixtureGroup)
        {
            copy->setInputSource(inputSource(PTInputId::transSweep(o)), PTInputId::transSweep(o));
            if (o < 64)
                copy->setInputSource(inputSource(PTInputId::transContinuousBank(o)),
                                    PTInputId::transContinuousBank(o));
            copy->setInputSource(inputSource(PTInputId::transSecondaryRow(o)),
                                 PTInputId::transSecondaryRow(o));
        }
    }
    copy->setInputSource(inputSource(PTInputId::kCrossfade), PTInputId::kCrossfade);

    copy->rebuildTable();
    return copy;
}

// ==========================================================================
// Cross-project clipboard
// ==========================================================================

void PresetTableV2Widget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    /* Sync any pending UI edits to m_rows/m_columns (same as saveXML does) */
    const_cast<PresetTableV2Widget*>(this)->syncAllDataFromTable();

    QMutexLocker lk(const_cast<QMutex*>(&m_stateMutex));

    obj["crossfadeEnabled"] = m_crossfadeEnabled;
    obj["continuousFxSelectorMode"] = continuousFxSelectorModeToString(m_continuousFxSelectorMode);
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
            o["outputScope"] = scopeToString(out.scope);
        }
        outs.append(o);
    }
    obj["outputs"] = outs;
}

void PresetTableV2Widget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    QMutexLocker lk(&m_stateMutex);

    m_crossfadeEnabled = obj["crossfadeEnabled"].toBool(false);
    m_continuousFxSelectorMode = continuousFxSelectorModeFromString(
            obj["continuousFxSelectorMode"].toString());
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
            out.scope = scopeFromString(o["outputScope"].toString());
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

void PresetTableV2Widget::paintEvent(QPaintEvent* e)
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

bool PresetTableV2Widget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot) return false;

    loadXMLCommon(root);

    bool xfEnabled = (root.attributes().value(KXMLCrossfadeEn).toString() == QLatin1String("True"));
    int  nameColW  = root.attributes().value(KXMLNameColWidth).toInt();
    PTContinuousFxSelectorMode loadedContFxSelectorMode = continuousFxSelectorModeFromString(
            root.attributes().value(KXMLContinuousFxSelectorMode).toString());

    // Mode (default = Legacy for backward compat)
    PTMode loadedMode = PTMode::Legacy;
    QString modeStr = root.attributes().value(KXMLMode).toString();
    if (modeStr == QLatin1String("FixtureGroup"))
        loadedMode = PTMode::FixtureGroup;

    quint32 loadedGroupId = UINT_MAX;
    QString groupIdStr = root.attributes().value(KXMLFxGroupId).toString();
    if (!groupIdStr.isEmpty())
        loadedGroupId = groupIdStr.toUInt();

    PTSpatialEffectSettings loadedSpatial;
    if (root.attributes().hasAttribute(KXMLSpatialEn))
        loadedSpatial.enabled = (root.attributes().value(KXMLSpatialEn).toString() == QLatin1String("True"));
    loadedSpatial.order = PresetTableV2SpatialEngine::orderFromString(
            root.attributes().value(KXMLSpatialOrder).toString());
    if (root.attributes().hasAttribute(KXMLSpatialStepMs))
        loadedSpatial.stepDelayMs = root.attributes().value(KXMLSpatialStepMs).toUInt();
    if (root.attributes().hasAttribute(KXMLSpatialFadeMs))
        loadedSpatial.fadeMs = root.attributes().value(KXMLSpatialFadeMs).toUInt();
    loadedSpatial.reverse = (root.attributes().value(KXMLSpatialReverse).toString() == QLatin1String("True"));

    quint32 loadedLinkedTransition = VCWidget::invalidId();
    if (root.attributes().hasAttribute(KXMLLinkedTransition))
        loadedLinkedTransition = root.attributes().value(KXMLLinkedTransition).toUInt();

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
            out.scope = scopeFromString(attrs.value(KXMLOutScope).toString());
            if (attrs.hasAttribute(KXMLOutSweepPreset))
                out.sweepPresetIndex = attrs.value(KXMLOutSweepPreset).toInt();
            else if (attrs.hasAttribute(KXMLOutTransitionPreset))
                out.sweepPresetIndex = attrs.value(KXMLOutTransitionPreset).toInt();
            if (attrs.hasAttribute(KXMLOutContinuousPreset))
                out.continuousPresetIndex = attrs.value(KXMLOutContinuousPreset).toInt();
            if (attrs.hasAttribute(KXMLOutSecondaryRow))
                out.secondaryRowIndex = attrs.value(KXMLOutSecondaryRow).toInt();
            else if (attrs.hasAttribute(KXMLOutTransitionSecondary))
                out.secondaryRowIndex = attrs.value(KXMLOutTransitionSecondary).toInt();

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLOutInput)
                    loadXMLSources(root, PTInputId::rowSelector(idx));
                else if (root.name() == KXMLOutTransSweepInput)
                    loadXMLSources(root, PTInputId::transSweep(idx));
                else if (root.name() == KXMLOutTransPrimaryInput)
                    loadXMLSources(root, PTInputId::transSweep(idx));
                else if (root.name() == KXMLOutTransContinuousInput)
                    loadXMLSources(root, PTInputId::transContinuousBank(idx));
                else if (root.name() == KXMLOutTransSecondaryInput)
                    loadXMLSources(root, PTInputId::transSecondaryRow(idx));
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
        m_continuousFxSelectorMode = loadedContFxSelectorMode;
        m_crossfadeGlobalPos = 0;
        m_crossfadeStartPos  = 0;
        m_crossfadePrevPos   = 0;
        m_crossfadeStagedAtLowSide = true;
        m_crossfadeSessionActive = false;
        m_crossfadeEditLaneStaged = true;
        m_nameColWidth = (nameColW > 0) ? nameColW : -1;
        m_mode           = loadedMode;
        m_fixtureGroupId = loadedGroupId;
        m_spatialEffects = loadedSpatial;
        m_linkedTransitionWidgetId = loadedLinkedTransition;
        m_spatialAppliedRow.resize(m_outputs.size());
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.resize(m_outputs.size());
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
    }

    refreshTransitionPresetCache();
    {
        QMutexLocker lk(&m_stateMutex);
        syncLiveTransitionFromOutputs();
    }
    rebuildTable();
    return true;
}

bool PresetTableV2Widget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    syncAllDataFromTable();

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);
    {
        QMutexLocker lk2(&m_stateMutex);
        doc->writeAttribute(KXMLCrossfadeEn, m_crossfadeEnabled ? QLatin1String("True") : QLatin1String("False"));
        doc->writeAttribute(KXMLContinuousFxSelectorMode,
                            continuousFxSelectorModeToString(m_continuousFxSelectorMode));
        if (m_nameColWidth > 0)
            doc->writeAttribute(KXMLNameColWidth, QString::number(m_nameColWidth));

        // Write mode (only write explicit tag for FixtureGroup; Legacy is default for old files)
        if (m_mode == PTMode::FixtureGroup)
        {
            doc->writeAttribute(KXMLMode,    QLatin1String("FixtureGroup"));
            doc->writeAttribute(KXMLFxGroupId, QString::number(m_fixtureGroupId));
        }

        doc->writeAttribute(KXMLSpatialEn, m_spatialEffects.enabled ? QLatin1String("True") : QLatin1String("False"));
        doc->writeAttribute(KXMLSpatialOrder,
                            PresetTableV2SpatialEngine::orderToString(m_spatialEffects.order));
        doc->writeAttribute(KXMLSpatialStepMs, QString::number(m_spatialEffects.stepDelayMs));
        doc->writeAttribute(KXMLSpatialFadeMs, QString::number(m_spatialEffects.fadeMs));
        doc->writeAttribute(KXMLSpatialReverse, m_spatialEffects.reverse ? QLatin1String("True") : QLatin1String("False"));
        if (m_linkedTransitionWidgetId != VCWidget::invalidId())
            doc->writeAttribute(KXMLLinkedTransition, QString::number(m_linkedTransitionWidgetId));
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
        QString       name;
        quint32       fixtureId;
        QList<int>    groupRows;
        PTOutputScope scope;
        int           sweepPresetIndex;
        int           continuousPresetIndex;
        int           secondaryRowIndex;
    };
    QVector<OutData> outData;
    outData.reserve(m_outputs.size());
    for (const PTOutput& out : m_outputs)
        outData.append({out.name, out.fixtureId, out.groupRows, out.scope,
                        out.sweepPresetIndex, out.continuousPresetIndex, out.secondaryRowIndex});

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
        else
        {
            doc->writeAttribute(KXMLOutScope, scopeToString(outData[o].scope));
            doc->writeAttribute(KXMLOutSweepPreset,
                                QString::number(outData[o].sweepPresetIndex));
            doc->writeAttribute(KXMLOutContinuousPreset,
                                QString::number(outData[o].continuousPresetIndex));
            doc->writeAttribute(KXMLOutSecondaryRow,
                                QString::number(outData[o].secondaryRowIndex));
            if (!outData[o].groupRows.isEmpty())
            {
                QStringList rowParts;
                for (int row : outData[o].groupRows)
                    rowParts << QString::number(row);
                doc->writeAttribute(KXMLOutRows, rowParts.join(QLatin1Char(',')));
            }
        }

        auto src = inputSource(PTInputId::rowSelector(o));
        if (!src.isNull() && src->isValid())
        {
            doc->writeStartElement(KXMLOutInput);
            saveXMLInput(doc, src);
            doc->writeEndElement();
        }

        if (isFGMode)
        {
            auto sweepSrc = inputSource(PTInputId::transSweep(o));
            if (!sweepSrc.isNull() && sweepSrc->isValid())
            {
                doc->writeStartElement(KXMLOutTransSweepInput);
                saveXMLInput(doc, sweepSrc);
                doc->writeEndElement();
            }
            auto contSrc = inputSource(PTInputId::transContinuousBank(o));
            if (!contSrc.isNull() && contSrc->isValid())
            {
                doc->writeStartElement(KXMLOutTransContinuousInput);
                saveXMLInput(doc, contSrc);
                doc->writeEndElement();
            }
            auto secSrc = inputSource(PTInputId::transSecondaryRow(o));
            if (!secSrc.isNull() && secSrc->isValid())
            {
                doc->writeStartElement(KXMLOutTransSecondaryInput);
                saveXMLInput(doc, secSrc);
                doc->writeEndElement();
            }
        }

        doc->writeEndElement();  // Output
    }

    // Global crossfade input
    auto xfSrc = inputSource(PTInputId::kCrossfade);
    if (!xfSrc.isNull() && xfSrc->isValid())
    {
        doc->writeStartElement(KXMLCrossfadeInput);
        saveXMLInput(doc, xfSrc);
        doc->writeEndElement();
    }

    doc->writeEndElement();  // PluginWidget
    return true;
}

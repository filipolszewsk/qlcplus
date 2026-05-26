/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutwidget.cpp — Apache 2.0 / public domain
*/

#include "fixturegrouplayoutwidget.h"
#include "fixturegrouplayoutconfigdialog.h"
#include "fixturegrouplayoutitemdelegate.h"

#include "doc.h"
#include "fixture.h"
#include "fixturegroup.h"
#include "grouphead.h"
#include "qlcpoint.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QBrush>
#include <QColor>
#include <QMouseEvent>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QLineEdit>
#include <QDebug>
#include <QSet>
#include <QTableWidgetItem>
#include <QItemSelectionModel>
#include <QSignalBlocker>

static const QString KXMLRoot           = QStringLiteral("PluginWidget");
static const QString KXMLPluginId       = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal    = QStringLiteral("org.qlcplus.vcwidgets.fixturegrouplayout");
static const QString KXMLFixtureGroupID = QStringLiteral("FixtureGroupID");
static const QString KXMLMaskPresets      = QStringLiteral("MaskPresets");
static const QString KXMLMaskPreset       = QStringLiteral("Preset");
static const QString KXMLMaskPresetName = QStringLiteral("Name");
static const QString KXMLMaskCell         = QStringLiteral("Cell");
static const QString KXMLMaskCellX        = QStringLiteral("X");
static const QString KXMLMaskCellY        = QStringLiteral("Y");
static const QString KXMLPresetSelectInput = QStringLiteral("PresetSelectInput");
static const QString KXMLMaskDisplay         = QStringLiteral("MaskDisplay");
static const QString KXMLMaskVisible         = QStringLiteral("visible");
static const QString KXMLMaskHidden          = QStringLiteral("hidden");
static const QString KXMLMaskHiddenText      = QStringLiteral("hiddenText");
static const QString KXMLMaskHeaderVisible   = QStringLiteral("headerVisible");
static const QString KXMLMaskSelectionBorder = QStringLiteral("selectionBorder");
static const QString KXMLMaskEfxConflict       = QStringLiteral("efxConflict");

static QColor colorFromXmlAttr(const QString& value, const QColor& fallback)
{
    const QColor c(value);
    return c.isValid() ? c : fallback;
}

static MaskChannelConflictPolicy docPolicyFromWidget(MaskEfxConflictPolicy policy)
{
    switch (policy)
    {
        case MaskEfxConflictPolicy::Wait:
            return MaskChannelConflictPolicy::Wait;
        case MaskEfxConflictPolicy::Override:
            return MaskChannelConflictPolicy::Override;
        default:
            return MaskChannelConflictPolicy::None;
    }
}

static MaskEfxConflictPolicy widgetPolicyFromString(const QString& value)
{
    if (value == QLatin1String("wait"))
        return MaskEfxConflictPolicy::Wait;
    if (value == QLatin1String("override"))
        return MaskEfxConflictPolicy::Override;
    return MaskEfxConflictPolicy::None;
}

static QString widgetPolicyToString(MaskEfxConflictPolicy policy)
{
    switch (policy)
    {
        case MaskEfxConflictPolicy::Wait:
            return QStringLiteral("wait");
        case MaskEfxConflictPolicy::Override:
            return QStringLiteral("override");
        default:
            return QStringLiteral("none");
    }
}

/** VC widgets are embedded; parenting modals to `this` can crash on macOS. */
static QWidget* dialogParent(QWidget* widget)
{
    if (widget == nullptr)
        return nullptr;
    QWidget* top = widget->window();
    return (top != widget) ? top : nullptr;
}

FixtureGroupLayoutWidget::FixtureGroupLayoutWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
    , m_fixtureGroupId(FixtureGroup::invalidId())
    , m_lastRow(0)
    , m_lastColumn(0)
    , m_dragging(false)
    , m_erasingSelection(false)
    , m_eraseLastCell(-1, -1)
    , m_dragStartCell(-1, -1)
    , m_dragCurrentCell(-1, -1)
    , m_gridDelegate(nullptr)
{
    setObjectName(FixtureGroupLayoutWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Fixture Group Layout"));
    resize(QSize(320, 240));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(2);

    m_titleLabel = new QLabel(tr("No fixture group"), this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_layout->addWidget(m_titleLabel);

    QHBoxLayout* maskToolbar = new QHBoxLayout();
    m_applyMaskButton = new QPushButton(tr("Apply mask"), this);
    m_applyMaskButton->setToolTip(tr("Add selected cells to the active mask"));
    connect(m_applyMaskButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotApplyMaskClicked);
    maskToolbar->addWidget(m_applyMaskButton);

    m_subtractMaskButton = new QPushButton(tr("Subtract mask"), this);
    m_subtractMaskButton->setToolTip(tr("Remove selected cells from the active mask"));
    connect(m_subtractMaskButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotSubtractMaskClicked);
    maskToolbar->addWidget(m_subtractMaskButton);

    m_clearMaskButton = new QPushButton(tr("Clear mask"), this);
    m_clearMaskButton->setToolTip(tr("Remove active mask (all cells visible to functions)"));
    connect(m_clearMaskButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotClearMaskClicked);
    maskToolbar->addWidget(m_clearMaskButton);

    m_efxConflictCombo = new QComboBox(this);
    m_efxConflictCombo->setMinimumWidth(140);
    m_efxConflictCombo->setToolTip(tr("How a new EFX in the mask interacts with running EFX "
                                      "on the same channel type (Position, Dimmer, RGB)."));
    connect(m_efxConflictCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FixtureGroupLayoutWidget::slotEfxConflictComboChanged);
    maskToolbar->addWidget(m_efxConflictCombo);
    populateEfxConflictCombo();

    maskToolbar->addStretch();
    m_layout->addLayout(maskToolbar);

    QHBoxLayout* presetToolbar = new QHBoxLayout();
    m_presetCombo = new QComboBox(this);
    m_presetCombo->setMinimumWidth(120);
    m_presetCombo->setToolTip(tr("Mask presets (All = clear mask). Selection recalls immediately."));
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FixtureGroupLayoutWidget::slotPresetComboChanged);
    presetToolbar->addWidget(m_presetCombo, 1);

    m_newPresetButton = new QPushButton(tr("New"), this);
    m_newPresetButton->setToolTip(tr("Save the current active mask as a new preset"));
    connect(m_newPresetButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotNewPresetClicked);
    presetToolbar->addWidget(m_newPresetButton);

    m_overwritePresetButton = new QPushButton(tr("Overwrite"), this);
    m_overwritePresetButton->setToolTip(tr("Replace the active preset with the current mask"));
    connect(m_overwritePresetButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotOverwritePresetClicked);
    presetToolbar->addWidget(m_overwritePresetButton);

    m_deletePresetButton = new QPushButton(tr("Delete"), this);
    connect(m_deletePresetButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotDeletePresetClicked);
    presetToolbar->addWidget(m_deletePresetButton);
    m_layout->addLayout(presetToolbar);

    m_table = new QTableWidget(this);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::StrongFocus);
    m_table->installEventFilter(this);
    m_table->viewport()->installEventFilter(this);
    m_layout->addWidget(m_table, 1);

    m_gridDelegate = new MaskGridItemDelegate(this, m_table);
    m_table->setItemDelegate(m_gridDelegate);
    if (m_table->selectionModel() != nullptr)
    {
        QWidget* viewport = m_table->viewport();
        connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
                viewport, [viewport](const QItemSelection&, const QItemSelection&) {
                    viewport->update();
                });
    }
    applyTableSelectionStyle();

    connect(m_table, SIGNAL(cellChanged(int,int)),
            this, SLOT(slotCellChanged(int,int)));
    connect(m_table, SIGNAL(cellPressed(int,int)),
            this, SLOT(slotCellActivated(int,int)));

    if (m_doc != nullptr)
    {
        connect(m_doc, SIGNAL(fixtureGroupChanged(quint32)),
                this, SLOT(slotFixtureGroupChanged(quint32)));
        connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)),
                this, SLOT(slotFixtureGroupRemoved(quint32)));
        connect(m_doc, SIGNAL(fixtureGroupMaskChanged(quint32)),
                this, SLOT(slotFixtureGroupMaskChanged(quint32)));
    }

    populatePresetCombo();
    updatePresetButtonStates();
    rebuildGrid();
}

void FixtureGroupLayoutWidget::slotCellActivated(int row, int column)
{
    m_lastRow = row;
    m_lastColumn = column;
}

FixtureGroupLayoutWidget::~FixtureGroupLayoutWidget()
{
    if (m_doc != nullptr)
    {
        disconnect(m_doc, nullptr, this, nullptr);
    }
}

VCWidget* FixtureGroupLayoutWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    FixtureGroupLayoutWidget* copy = new FixtureGroupLayoutWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }
    copy->setFixtureGroupId(m_fixtureGroupId);
    copy->m_maskPresets = m_maskPresets;
    copy->m_maskColors = m_maskColors;
    copy->m_maskEfxConflictPolicy = m_maskEfxConflictPolicy;
    copy->syncEfxConflictCombo();
    copy->applyTableSelectionStyle();
    copy->populatePresetCombo();
    return copy;
}

void FixtureGroupLayoutWidget::setMaskDisplayColors(const MaskDisplayColors& colors)
{
    m_maskColors = colors;
    applyTableSelectionStyle();
    rebuildGrid();
}

void FixtureGroupLayoutWidget::setMaskEfxConflictPolicy(MaskEfxConflictPolicy policy)
{
    m_maskEfxConflictPolicy = policy;
    syncEfxConflictCombo();
    pushMaskToDoc();
}

void FixtureGroupLayoutWidget::populateEfxConflictCombo()
{
    if (m_efxConflictCombo == nullptr)
        return;

    QSignalBlocker blocker(m_efxConflictCombo);
    m_efxConflictCombo->clear();
    m_efxConflictCombo->addItem(tr("HTP"), int(MaskEfxConflictPolicy::None));
    m_efxConflictCombo->addItem(tr("Wait"), int(MaskEfxConflictPolicy::Wait));
    m_efxConflictCombo->addItem(tr("Override"), int(MaskEfxConflictPolicy::Override));
    syncEfxConflictCombo();
}

void FixtureGroupLayoutWidget::syncEfxConflictCombo()
{
    if (m_efxConflictCombo == nullptr)
        return;

    QSignalBlocker blocker(m_efxConflictCombo);
    const int idx = m_efxConflictCombo->findData(int(m_maskEfxConflictPolicy));
    if (idx >= 0)
        m_efxConflictCombo->setCurrentIndex(idx);
}

void FixtureGroupLayoutWidget::slotEfxConflictComboChanged(int index)
{
    if (m_efxConflictCombo == nullptr || index < 0)
        return;

    const MaskEfxConflictPolicy policy =
            static_cast<MaskEfxConflictPolicy>(m_efxConflictCombo->itemData(index).toInt());
    if (policy == m_maskEfxConflictPolicy)
        return;

    m_maskEfxConflictPolicy = policy;
    pushMaskToDoc();
}

void FixtureGroupLayoutWidget::setFixtureGroupId(quint32 id)
{
    m_fixtureGroupId = id;
    syncMaskFromDoc();
    rebuildGrid();
}

void FixtureGroupLayoutWidget::syncMaskFromDoc()
{
    if (m_doc == nullptr || m_fixtureGroupId == FixtureGroup::invalidId())
    {
        m_localMask.clear();
        return;
    }
    m_localMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
}

void FixtureGroupLayoutWidget::pushMaskToDoc()
{
    if (m_doc == nullptr || m_fixtureGroupId == FixtureGroup::invalidId())
        return;

    m_doc->setMaskChannelConflictPolicy(m_fixtureGroupId,
                                        docPolicyFromWidget(m_maskEfxConflictPolicy));

    if (m_localMask.isActive())
        m_doc->setFixtureGroupMask(m_fixtureGroupId, m_localMask);
    else
        m_doc->clearFixtureGroupMask(m_fixtureGroupId);
}

bool FixtureGroupLayoutWidget::isPointMaskedOut(const QLCPoint& pt) const
{
    if (!m_localMask.isActive())
        return false;
    return !m_localMask.acceptsPoint(pt);
}

bool FixtureGroupLayoutWidget::isColumnMaskedOut(int column) const
{
    if (!m_localMask.isActive())
        return false;
    return !m_localMask.isColumnEnabled(column);
}

QSet<QLCPoint> FixtureGroupLayoutWidget::selectedCells() const
{
    QSet<QLCPoint> cells;
    foreach (QTableWidgetItem* item, m_table->selectedItems())
    {
        if (item == nullptr)
            continue;
        cells.insert(QLCPoint(item->column(), item->row()));
    }
    return cells;
}

QSet<QLCPoint> FixtureGroupLayoutWidget::validatedCells(const QSet<QLCPoint>& cells,
                                                      const FixtureGroup* grp) const
{
    QSet<QLCPoint> valid;
    if (grp == nullptr)
        return valid;

    const int w = grp->size().width();
    const int h = grp->size().height();
    for (const QLCPoint& pt : cells)
    {
        if (pt.x() >= 0 && pt.x() < w && pt.y() >= 0 && pt.y() < h)
            valid.insert(pt);
    }
    return valid;
}

void FixtureGroupLayoutWidget::applyMaskFromCells(const QSet<QLCPoint>& cells)
{
    m_localMask.clear();
    m_localMask.setPoints(cells);
    pushMaskToDoc();
    rebuildGrid();
    clearGridSelection();
}

void FixtureGroupLayoutWidget::clearGridSelection()
{
    if (m_table == nullptr)
        return;
    m_table->clearSelection();
    m_table->setCurrentIndex(QModelIndex());
}

void FixtureGroupLayoutWidget::deselectCellAt(int row, int column)
{
    if (m_table == nullptr || row < 0 || column < 0)
        return;
    if (row >= m_table->rowCount() || column >= m_table->columnCount())
        return;

    const QModelIndex index = m_table->model()->index(row, column);
    if (index.isValid())
        m_table->selectionModel()->select(index, QItemSelectionModel::Deselect);
}

void FixtureGroupLayoutWidget::applyTableSelectionStyle()
{
    if (m_table == nullptr)
        return;

    QPalette pal = m_table->palette();
    pal.setBrush(QPalette::Highlight, Qt::transparent);
    pal.setColor(QPalette::HighlightedText, palette().text().color());
    m_table->setPalette(pal);
    m_table->setStyleSheet(QString());
    m_table->viewport()->update();
}

bool FixtureGroupLayoutWidget::isGridCellSelected(int row, int column) const
{
    if (m_table == nullptr || m_table->selectionModel() == nullptr)
        return false;
    return m_table->selectionModel()->isSelected(m_table->model()->index(row, column));
}

QSet<QLCPoint> FixtureGroupLayoutWidget::currentMaskCellsForPreset() const
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr || !m_localMask.isActive())
        return QSet<QLCPoint>();

    if (!m_localMask.points().isEmpty())
        return validatedCells(m_localMask.points(), grp);

    QSet<QLCPoint> cells;
    const int w = grp->size().width();
    const int h = grp->size().height();
    for (int col = 0; col < w; col++)
    {
        for (int row = 0; row < h; row++)
        {
            const QLCPoint pt(col, row);
            if (m_localMask.acceptsPoint(pt))
                cells.insert(pt);
        }
    }
    return cells;
}

void FixtureGroupLayoutWidget::updatePresetButtonStates()
{
    const int idx = m_presetCombo->currentIndex();
    const bool hasPreset = idx > 0;
    m_deletePresetButton->setEnabled(hasPreset);
    m_overwritePresetButton->setEnabled(hasPreset);
}

void FixtureGroupLayoutWidget::populatePresetCombo()
{
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    m_presetCombo->addItem(tr("All"));
    for (const MaskPreset& preset : m_maskPresets)
        m_presetCombo->addItem(preset.name);
    m_presetCombo->blockSignals(false);
    updatePresetButtonStates();
}

void FixtureGroupLayoutWidget::recallPreset(int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= m_maskPresets.size())
        return;

    FixtureGroup* grp = fixtureGroup();
    const QSet<QLCPoint> cells = validatedCells(m_maskPresets.at(presetIndex).cells, grp);
    if (cells.isEmpty())
        return;

    applyMaskFromCells(cells);
}

void FixtureGroupLayoutWidget::activatePresetComboIndex(int comboIndex)
{
    if (comboIndex < 0 || comboIndex >= m_presetCombo->count())
        return;

    if (comboIndex == 0)
    {
        slotClearMaskClicked();
        return;
    }

    recallPreset(comboIndex - 1);
}

void FixtureGroupLayoutWidget::applyPresetFromChannelValue(uchar value)
{
    if (m_presetCombo->count() == 0)
        return;

    const int comboIndex = qBound(0, int(value), m_presetCombo->count() - 1);
    if (m_presetCombo->currentIndex() != comboIndex)
    {
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(comboIndex);
        m_presetCombo->blockSignals(false);
    }
    activatePresetComboIndex(comboIndex);
    updateFeedback();
}

void FixtureGroupLayoutWidget::slotPresetComboChanged(int index)
{
    updatePresetButtonStates();
    activatePresetComboIndex(index);
    updateFeedback();
}

void FixtureGroupLayoutWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (!acceptsInput())
        return;

    const quint32 pagedCh = (page() << 16) | channel;
    if (!checkInputSource(universe, pagedCh, value, sender(), presetSelectInputSourceId))
        return;

    applyPresetFromChannelValue(value);
}

void FixtureGroupLayoutWidget::updateFeedback()
{
    sendFeedback(m_presetCombo->currentIndex(), presetSelectInputSourceId);
}

FixtureGroup* FixtureGroupLayoutWidget::fixtureGroup() const
{
    if (m_doc == nullptr || m_fixtureGroupId == FixtureGroup::invalidId())
        return nullptr;
    return m_doc->fixtureGroup(m_fixtureGroupId);
}

void FixtureGroupLayoutWidget::updateCaptionLabel()
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
    {
        m_titleLabel->setText(tr("No fixture group"));
        return;
    }

    QString maskHint;
    if (m_localMask.isActive())
    {
        if (!m_localMask.points().isEmpty())
            maskHint = tr(" — mask: %1 cell(s)").arg(m_localMask.points().size());
        else
            maskHint = tr(" — mask: %1 col(s)").arg(m_localMask.columns().size());
    }

    m_titleLabel->setText(QStringLiteral("%1 (%2×%3)%4")
                              .arg(grp->name())
                              .arg(grp->size().width())
                              .arg(grp->size().height())
                              .arg(maskHint));
}

void FixtureGroupLayoutWidget::applyColumnHeaderStyles()
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return;

    const bool maskActive = m_localMask.isActive();
    const QBrush visibleHeaderBrush(m_maskColors.headerVisible);
    const QBrush maskedHeaderBrush(m_maskColors.hidden);
    const QBrush maskedHeaderFg(m_maskColors.hiddenText);

    for (int col = 0; col < grp->size().width(); col++)
    {
        QTableWidgetItem* headerItem = m_table->horizontalHeaderItem(col);
        if (headerItem == nullptr)
        {
            headerItem = new QTableWidgetItem(QString::number(col + 1));
            m_table->setHorizontalHeaderItem(col, headerItem);
        }
        const bool colVisible = !maskActive || !isColumnMaskedOut(col);
        headerItem->setBackground(colVisible ? visibleHeaderBrush : maskedHeaderBrush);
        headerItem->setForeground(colVisible ? QBrush(palette().text().color()) : maskedHeaderFg);
        headerItem->setToolTip(colVisible
            ? tr("Column %1 active (click to toggle mask)").arg(col + 1)
            : tr("Column %1 hidden by mask (click to include)").arg(col + 1));
    }
}

void FixtureGroupLayoutWidget::rebuildGrid()
{
    updateCaptionLabel();

    disconnect(m_table, SIGNAL(cellChanged(int,int)),
               this, SLOT(slotCellChanged(int,int)));

    m_table->clear();
    m_table->setRowCount(0);
    m_table->setColumnCount(0);

    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
    {
        connect(m_table, SIGNAL(cellChanged(int,int)),
                this, SLOT(slotCellChanged(int,int)));
        return;
    }

    m_table->setRowCount(grp->size().height());
    m_table->setColumnCount(grp->size().width());

    const bool maskActive = m_localMask.isActive();

    QMapIterator<QLCPoint, GroupHead> it(grp->headsMap());
    while (it.hasNext())
    {
        it.next();
        QLCPoint pt(it.key());
        GroupHead head(it.value());
        Fixture* fxi = m_doc->fixture(head.fxi);
        if (fxi == nullptr)
            continue;

        QString str = QStringLiteral("%1 H:%2")
                          .arg(fxi->name())
                          .arg(head.head + 1);

        QTableWidgetItem* item = new QTableWidgetItem(fxi->getIconFromType(), str);
        item->setToolTip(QStringLiteral("%1\nU:%2 A:%3")
                             .arg(str)
                             .arg(fxi->universe() + 1)
                             .arg(fxi->address() + 1));
        m_table->setItem(pt.y(), pt.x(), item);
    }

    if (maskActive)
    {
        for (int row = 0; row < grp->size().height(); row++)
        {
            for (int col = 0; col < grp->size().width(); col++)
            {
                QLCPoint pt(col, row);
                if (m_table->item(row, col) != nullptr)
                    continue;
                if (!isPointMaskedOut(pt))
                    continue;
                QTableWidgetItem* emptyItem = new QTableWidgetItem();
                emptyItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                m_table->setItem(row, col, emptyItem);
            }
        }
    }

    connect(m_table, SIGNAL(cellChanged(int,int)),
            this, SLOT(slotCellChanged(int,int)));

    applyColumnHeaderStyles();
    updateCaptionLabel();

    if (m_lastRow < m_table->rowCount() && m_lastColumn < m_table->columnCount())
        m_table->setCurrentCell(m_lastRow, m_lastColumn);
}

void FixtureGroupLayoutWidget::slotFixtureGroupMaskChanged(quint32 id)
{
    if (id != m_fixtureGroupId)
        return;
    syncMaskFromDoc();
    rebuildGrid();
}

void FixtureGroupLayoutWidget::slotApplyMaskClicked()
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return;

    const QSet<QLCPoint> sel = validatedCells(selectedCells(), grp);
    if (sel.isEmpty())
    {
        m_titleLabel->setText(tr("Select grid cells to add to the mask"));
        return;
    }

    QSet<QLCPoint> cells = currentMaskCellsForPreset();
    cells.unite(sel);
    applyMaskFromCells(cells);
}

void FixtureGroupLayoutWidget::slotSubtractMaskClicked()
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return;

    if (!m_localMask.isActive())
    {
        m_titleLabel->setText(tr("No active mask to subtract from"));
        return;
    }

    const QSet<QLCPoint> sel = validatedCells(selectedCells(), grp);
    if (sel.isEmpty())
    {
        m_titleLabel->setText(tr("Select grid cells to remove from the mask"));
        return;
    }

    QSet<QLCPoint> cells = currentMaskCellsForPreset();
    for (const QLCPoint& pt : sel)
        cells.remove(pt);

    if (cells.isEmpty())
        slotClearMaskClicked();
    else
        applyMaskFromCells(cells);
}

void FixtureGroupLayoutWidget::slotNewPresetClicked()
{
    if (fixtureGroup() == nullptr)
        return;

    const QSet<QLCPoint> cells = currentMaskCellsForPreset();
    if (cells.isEmpty())
    {
        m_titleLabel->setText(tr("Build a mask with Apply mask before saving a preset"));
        return;
    }

    bool ok = false;
    const QString name = QInputDialog::getText(
        dialogParent(this),
        tr("New mask preset"),
        tr("Preset name:"),
        QLineEdit::Normal,
        QString(),
        &ok);
    if (!ok)
        return;

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
    {
        m_titleLabel->setText(tr("Preset name cannot be empty"));
        return;
    }

    for (const MaskPreset& preset : m_maskPresets)
    {
        if (preset.name == trimmed)
        {
            m_titleLabel->setText(tr("A preset named \"%1\" already exists").arg(trimmed));
            return;
        }
    }

    m_maskPresets.append({ trimmed, cells });
    populatePresetCombo();
    const int comboIdx = m_presetCombo->findText(trimmed);
    if (comboIdx >= 0)
        m_presetCombo->setCurrentIndex(comboIdx);
    updateCaptionLabel();
    if (m_doc != nullptr)
        m_doc->setModified();
}

void FixtureGroupLayoutWidget::slotOverwritePresetClicked()
{
    const int comboIdx = m_presetCombo->currentIndex();
    if (comboIdx <= 0)
        return;

    const QSet<QLCPoint> cells = currentMaskCellsForPreset();
    if (cells.isEmpty())
    {
        m_titleLabel->setText(tr("No active mask to save to the preset"));
        return;
    }

    m_maskPresets[comboIdx - 1].cells = cells;
    updateCaptionLabel();
    if (m_doc != nullptr)
        m_doc->setModified();
}

void FixtureGroupLayoutWidget::slotDeletePresetClicked()
{
    const int comboIdx = m_presetCombo->currentIndex();
    if (comboIdx <= 0)
        return;

    m_maskPresets.removeAt(comboIdx - 1);
    populatePresetCombo();
    m_presetCombo->setCurrentIndex(0);
    if (m_doc != nullptr)
        m_doc->setModified();
}

void FixtureGroupLayoutWidget::slotClearMaskClicked()
{
    m_localMask.clear();
    pushMaskToDoc();
    rebuildGrid();
    clearGridSelection();
}

void FixtureGroupLayoutWidget::slotModeChanged(Doc::Mode mode)
{
    Q_UNUSED(mode);
    VCWidget::slotModeChanged(mode);
}

void FixtureGroupLayoutWidget::slotFixtureGroupChanged(quint32 id)
{
    if (id == m_fixtureGroupId)
        rebuildGrid();
}

void FixtureGroupLayoutWidget::slotFixtureGroupRemoved(quint32 id)
{
    if (id == m_fixtureGroupId)
    {
        m_fixtureGroupId = FixtureGroup::invalidId();
        rebuildGrid();
    }
}

void FixtureGroupLayoutWidget::slotCellChanged(int row, int column)
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr || row < 0 || column < 0)
    {
        rebuildGrid();
        return;
    }

    QLCPoint from(m_lastColumn, m_lastRow);
    QLCPoint to(column, row);

    if (from != to)
        grp->swap(from, to);

    rebuildGrid();
    m_table->setCurrentCell(row, column);
    m_lastRow = row;
    m_lastColumn = column;
}

QList<QLCPoint> FixtureGroupLayoutWidget::selectedPoints() const
{
    QList<QLCPoint> points;
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return points;

    QMap<QLCPoint, GroupHead> headsMap = grp->headsMap();

    foreach (QTableWidgetItem* item, m_table->selectedItems())
    {
        if (item == nullptr)
            continue;

        QLCPoint pt(item->column(), item->row());
        if (headsMap.contains(pt))
            points.append(pt);
    }

    return points;
}

void FixtureGroupLayoutWidget::reselectPoints(const QList<QLCPoint>& points)
{
    m_table->clearSelection();

    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return;

    int gridWidth = grp->size().width();
    int gridHeight = grp->size().height();

    foreach (const QLCPoint& pt, points)
    {
        if (pt.x() >= 0 && pt.x() < gridWidth && pt.y() >= 0 && pt.y() < gridHeight)
        {
            QTableWidgetItem* item = m_table->item(pt.y(), pt.x());
            if (item != nullptr)
                item->setSelected(true);
            else
                m_table->setCurrentCell(pt.y(), pt.x(), QItemSelectionModel::Select);
        }
    }
}

QPoint FixtureGroupLayoutWidget::cellAtPos(const QPoint& pos) const
{
    int row = m_table->rowAt(pos.y());
    int col = m_table->columnAt(pos.x());
    return QPoint(col, row);
}

void FixtureGroupLayoutWidget::moveSelectedHeads(int deltaX, int deltaY)
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return;

    QList<QLCPoint> selectedPts = selectedPoints();
    if (selectedPts.isEmpty())
        return;

    int gridWidth = grp->size().width();
    int gridHeight = grp->size().height();

    foreach (const QLCPoint& pt, selectedPts)
    {
        int newX = pt.x() + deltaX;
        int newY = pt.y() + deltaY;
        if (newX < 0 || newX >= gridWidth || newY < 0 || newY >= gridHeight)
            return;
    }

    grp->beginUpdate();

    QMap<QLCPoint, GroupHead> currentMap = grp->headsMap();

    QList<QLCPoint> targetPoints;
    foreach (const QLCPoint& pt, selectedPts)
        targetPoints.append(QLCPoint(pt.x() + deltaX, pt.y() + deltaY));

    QMap<QLCPoint, GroupHead> headsToMove;
    foreach (const QLCPoint& pt, selectedPts)
    {
        if (currentMap.contains(pt))
            headsToMove[pt] = currentMap[pt];
    }

    QMap<QLCPoint, GroupHead> headsToSwap;
    foreach (const QLCPoint& targetPt, targetPoints)
    {
        if (currentMap.contains(targetPt) && !selectedPts.contains(targetPt))
            headsToSwap[targetPt] = currentMap[targetPt];
    }

    QSet<QLCPoint> freeSourcePositions;
    foreach (const QLCPoint& srcPt, selectedPts)
    {
        if (!targetPoints.contains(srcPt))
            freeSourcePositions.insert(srcPt);
    }

    foreach (const QLCPoint& pt, selectedPts)
        grp->resignHead(pt);

    QMapIterator<QLCPoint, GroupHead> clearIt(headsToSwap);
    while (clearIt.hasNext())
    {
        clearIt.next();
        grp->resignHead(clearIt.key());
    }

    QMapIterator<QLCPoint, GroupHead> moveIt(headsToMove);
    while (moveIt.hasNext())
    {
        moveIt.next();
        QLCPoint newPt(moveIt.key().x() + deltaX, moveIt.key().y() + deltaY);
        grp->assignHead(newPt, moveIt.value());
    }

    QMapIterator<QLCPoint, GroupHead> swapIt(headsToSwap);
    while (swapIt.hasNext())
    {
        swapIt.next();
        QLCPoint targetPt = swapIt.key();
        GroupHead head = swapIt.value();

        QLCPoint correspondingSourcePt(targetPt.x() - deltaX, targetPt.y() - deltaY);

        if (freeSourcePositions.contains(correspondingSourcePt))
        {
            grp->assignHead(correspondingSourcePt, head);
            freeSourcePositions.remove(correspondingSourcePt);
        }
        else if (!freeSourcePositions.isEmpty())
        {
            QLCPoint freePt = *freeSourcePositions.begin();
            grp->assignHead(freePt, head);
            freeSourcePositions.remove(freePt);
        }
    }

    grp->endUpdate();
}

bool FixtureGroupLayoutWidget::eventFilter(QObject* obj, QEvent* event)
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr)
        return VCWidget::eventFilter(obj, event);

    if (obj == m_table && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        QList<QLCPoint> sel = selectedPoints();
        if (sel.isEmpty())
            return VCWidget::eventFilter(obj, event);

        int deltaX = 0;
        int deltaY = 0;
        switch (keyEvent->key())
        {
            case Qt::Key_Left:  deltaX = -1; break;
            case Qt::Key_Right: deltaX = 1;  break;
            case Qt::Key_Up:    deltaY = -1; break;
            case Qt::Key_Down:  deltaY = 1;  break;
            default:
                return VCWidget::eventFilter(obj, event);
        }

        QList<QLCPoint> newPositions;
        foreach (const QLCPoint& pt, sel)
            newPositions.append(QLCPoint(pt.x() + deltaX, pt.y() + deltaY));

        moveSelectedHeads(deltaX, deltaY);
        rebuildGrid();
        reselectPoints(newPositions);
        return true;
    }

    if (obj == m_table->viewport())
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::RightButton)
            {
                m_erasingSelection = true;
                m_eraseLastCell = QPoint(-1, -1);
                const QPoint cell = cellAtPos(mouseEvent->pos());
                if (cell.x() >= 0 && cell.y() >= 0)
                {
                    deselectCellAt(cell.y(), cell.x());
                    m_eraseLastCell = cell;
                }
                return true;
            }

            if (mouseEvent->button() == Qt::LeftButton)
            {
                m_erasingSelection = false;
                QPoint cell = cellAtPos(mouseEvent->pos());
                if (cell.x() >= 0 && cell.y() >= 0)
                {
                    QList<QLCPoint> sel = selectedPoints();
                    QLCPoint clickedPoint(cell.x(), cell.y());
                    if (sel.contains(clickedPoint) && !sel.isEmpty())
                    {
                        m_dragging = true;
                        m_dragStartCell = cell;
                        m_dragCurrentCell = cell;
                        m_dragOriginalPoints = sel;
                        return true;
                    }
                }
            }
        }

        if (event->type() == QEvent::MouseMove && m_erasingSelection)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::RightButton)
            {
                const QPoint cell = cellAtPos(mouseEvent->pos());
                if (cell.x() >= 0 && cell.y() >= 0 && cell != m_eraseLastCell)
                {
                    deselectCellAt(cell.y(), cell.x());
                    m_eraseLastCell = cell;
                }
                return true;
            }
        }

        if (event->type() == QEvent::MouseMove && m_dragging)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint cell = cellAtPos(mouseEvent->pos());

            if (cell.x() >= 0 && cell.y() >= 0 && cell != m_dragCurrentCell)
            {
                int deltaX = cell.x() - m_dragCurrentCell.x();
                int deltaY = cell.y() - m_dragCurrentCell.y();

                QList<QLCPoint> currentPoints = selectedPoints();
                if (!currentPoints.isEmpty())
                {
                    bool canMove = true;
                    int gridWidth = grp->size().width();
                    int gridHeight = grp->size().height();

                    foreach (const QLCPoint& pt, currentPoints)
                    {
                        int newX = pt.x() + deltaX;
                        int newY = pt.y() + deltaY;
                        if (newX < 0 || newX >= gridWidth || newY < 0 || newY >= gridHeight)
                        {
                            canMove = false;
                            break;
                        }
                    }

                    if (canMove)
                    {
                        QList<QLCPoint> newPositions;
                        foreach (const QLCPoint& pt, currentPoints)
                            newPositions.append(QLCPoint(pt.x() + deltaX, pt.y() + deltaY));

                        moveSelectedHeads(deltaX, deltaY);
                        rebuildGrid();
                        reselectPoints(newPositions);
                        m_dragCurrentCell = cell;
                    }
                }
            }
            return true;
        }

        if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::RightButton && m_erasingSelection)
            {
                m_erasingSelection = false;
                m_eraseLastCell = QPoint(-1, -1);
                return true;
            }

            if (mouseEvent->button() == Qt::LeftButton && m_dragging)
            {
                m_dragging = false;
                int totalDeltaX = m_dragCurrentCell.x() - m_dragStartCell.x();
                int totalDeltaY = m_dragCurrentCell.y() - m_dragStartCell.y();

                QList<QLCPoint> finalPositions;
                foreach (const QLCPoint& pt, m_dragOriginalPoints)
                    finalPositions.append(QLCPoint(pt.x() + totalDeltaX, pt.y() + totalDeltaY));

                reselectPoints(finalPositions);
                m_dragOriginalPoints.clear();
                m_dragStartCell = QPoint(-1, -1);
                m_dragCurrentCell = QPoint(-1, -1);
                return true;
            }
        }
    }

    return VCWidget::eventFilter(obj, event);
}

void FixtureGroupLayoutWidget::editProperties()
{
    if (mode() != Doc::Design)
        return;

    FixtureGroupLayoutConfigDialog dlg(m_doc, m_fixtureGroupId, m_maskColors,
                                       m_maskEfxConflictPolicy,
                                       inputSource(presetSelectInputSourceId),
                                       page(), dialogParent(this));
    if (dlg.exec() != QDialog::Accepted)
        return;

    setFixtureGroupId(dlg.fixtureGroupId());
    setMaskDisplayColors(dlg.maskDisplayColors());
    setMaskEfxConflictPolicy(dlg.maskEfxConflictPolicy());
    setInputSource(dlg.presetSelectInputSource(), presetSelectInputSourceId);
    m_doc->setModified();
}

bool FixtureGroupLayoutWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot)
    {
        qWarning() << Q_FUNC_INFO << "PluginWidget node not found";
        return false;
    }

    loadXMLCommon(root);

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0;
            bool visible = false;
            loadXMLWindowState(root, &x, &y, &w, &h, &visible);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
        {
            loadXMLAppearance(root);
        }
        else if (root.name() == KXMLFixtureGroupID)
        {
            m_fixtureGroupId = root.readElementText().toUInt();
        }
        else if (root.name() == KXMLPresetSelectInput)
        {
            loadXMLSources(root, presetSelectInputSourceId);
        }
        else if (root.name() == KXMLMaskDisplay)
        {
            const auto attrs = root.attributes();
            MaskDisplayColors defaults;
            m_maskColors.visible = colorFromXmlAttr(
                attrs.value(KXMLMaskVisible).toString(), defaults.visible);
            m_maskColors.hidden = colorFromXmlAttr(
                attrs.value(KXMLMaskHidden).toString(), defaults.hidden);
            m_maskColors.hiddenText = colorFromXmlAttr(
                attrs.value(KXMLMaskHiddenText).toString(), defaults.hiddenText);
            m_maskColors.headerVisible = colorFromXmlAttr(
                attrs.value(KXMLMaskHeaderVisible).toString(), defaults.headerVisible);
            m_maskColors.selectionBorder = colorFromXmlAttr(
                attrs.value(KXMLMaskSelectionBorder).toString(), defaults.selectionBorder);
            m_maskEfxConflictPolicy = widgetPolicyFromString(
                attrs.value(KXMLMaskEfxConflict).toString());
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLMaskPresets)
        {
            m_maskPresets.clear();
            while (root.readNextStartElement())
            {
                if (root.name() != KXMLMaskPreset)
                {
                    root.skipCurrentElement();
                    continue;
                }

                MaskPreset preset;
                preset.name = root.attributes().value(KXMLMaskPresetName).toString();
                while (root.readNextStartElement())
                {
                    if (root.name() == KXMLMaskCell)
                    {
                        const int x = root.attributes().value(KXMLMaskCellX).toInt();
                        const int y = root.attributes().value(KXMLMaskCellY).toInt();
                        preset.cells.insert(QLCPoint(x, y));
                        root.skipCurrentElement();
                    }
                    else
                    {
                        root.skipCurrentElement();
                    }
                }
                if (!preset.name.isEmpty() && !preset.cells.isEmpty())
                    m_maskPresets.append(preset);
            }
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    populatePresetCombo();
    syncEfxConflictCombo();
    applyTableSelectionStyle();
    syncMaskFromDoc();
    rebuildGrid();
    pushMaskToDoc();
    return true;
}

bool FixtureGroupLayoutWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);

    saveXMLCommon(doc);

    doc->writeTextElement(KXMLFixtureGroupID, QString::number(m_fixtureGroupId));

    if (!m_maskPresets.isEmpty())
    {
        doc->writeStartElement(KXMLMaskPresets);
        for (const MaskPreset& preset : m_maskPresets)
        {
            doc->writeStartElement(KXMLMaskPreset);
            doc->writeAttribute(KXMLMaskPresetName, preset.name);
            for (const QLCPoint& pt : preset.cells)
            {
                doc->writeStartElement(KXMLMaskCell);
                doc->writeAttribute(KXMLMaskCellX, QString::number(pt.x()));
                doc->writeAttribute(KXMLMaskCellY, QString::number(pt.y()));
                doc->writeEndElement();
            }
            doc->writeEndElement();
        }
        doc->writeEndElement();
    }

    auto presetSrc = inputSource(presetSelectInputSourceId);
    if (!presetSrc.isNull() && presetSrc->isValid())
    {
        doc->writeStartElement(KXMLPresetSelectInput);
        saveXMLInput(doc, presetSrc);
        doc->writeEndElement();
    }

    doc->writeStartElement(KXMLMaskDisplay);
    doc->writeAttribute(KXMLMaskVisible, m_maskColors.visible.name());
    doc->writeAttribute(KXMLMaskHidden, m_maskColors.hidden.name());
    doc->writeAttribute(KXMLMaskHiddenText, m_maskColors.hiddenText.name());
    doc->writeAttribute(KXMLMaskHeaderVisible, m_maskColors.headerVisible.name());
    doc->writeAttribute(KXMLMaskSelectionBorder, m_maskColors.selectionBorder.name());
    doc->writeAttribute(KXMLMaskEfxConflict, widgetPolicyToString(m_maskEfxConflictPolicy));
    doc->writeEndElement();

    saveXMLAppearance(doc);
    saveXMLWindowState(doc);

    doc->writeEndElement();
    return true;
}

void FixtureGroupLayoutWidget::toClipboardJson(QJsonObject& obj, const Doc* doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    FixtureGroup* grp = doc->fixtureGroup(m_fixtureGroupId);
    obj[QStringLiteral("fixtureGroupName")] = grp ? grp->name() : QString();
    obj[QStringLiteral("fixtureGroupId")] = static_cast<int>(m_fixtureGroupId);

    QJsonArray presetsArr;
    for (const MaskPreset& preset : m_maskPresets)
    {
        QJsonObject pObj;
        pObj[QStringLiteral("name")] = preset.name;
        QJsonArray cellsArr;
        for (const QLCPoint& pt : preset.cells)
        {
            QJsonObject cObj;
            cObj[QStringLiteral("x")] = pt.x();
            cObj[QStringLiteral("y")] = pt.y();
            cellsArr.append(cObj);
        }
        pObj[QStringLiteral("cells")] = cellsArr;
        presetsArr.append(pObj);
    }
    obj[QStringLiteral("maskPresets")] = presetsArr;
}

void FixtureGroupLayoutWidget::fromClipboardJson(const QJsonObject& obj, Doc* doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    quint32 id = static_cast<quint32>(obj[QStringLiteral("fixtureGroupId")].toInt(
        static_cast<int>(FixtureGroup::invalidId())));

    if (id == FixtureGroup::invalidId())
    {
        const QString name = obj[QStringLiteral("fixtureGroupName")].toString();
        if (!name.isEmpty())
        {
            for (FixtureGroup* grp : doc->fixtureGroups())
            {
                if (grp && grp->name() == name)
                {
                    id = grp->id();
                    break;
                }
            }
        }
    }

    m_maskPresets.clear();
    const QJsonArray presetsArr = obj[QStringLiteral("maskPresets")].toArray();
    for (const QJsonValue& pv : presetsArr)
    {
        const QJsonObject pObj = pv.toObject();
        MaskPreset preset;
        preset.name = pObj[QStringLiteral("name")].toString();
        const QJsonArray cellsArr = pObj[QStringLiteral("cells")].toArray();
        for (const QJsonValue& cv : cellsArr)
        {
            const QJsonObject cObj = cv.toObject();
            preset.cells.insert(QLCPoint(cObj[QStringLiteral("x")].toInt(),
                                         cObj[QStringLiteral("y")].toInt()));
        }
        if (!preset.name.isEmpty() && !preset.cells.isEmpty())
            m_maskPresets.append(preset);
    }
    populatePresetCombo();

    setFixtureGroupId(id);
}

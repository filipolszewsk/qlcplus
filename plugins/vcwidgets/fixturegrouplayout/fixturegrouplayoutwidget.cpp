/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutwidget.cpp — Apache 2.0 / public domain
*/

#include "fixturegrouplayoutwidget.h"
#include "fixturegrouplayoutconfigdialog.h"

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
#include <QDebug>
#include <QSet>

static const QString KXMLRoot           = QStringLiteral("PluginWidget");
static const QString KXMLPluginId       = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal    = QStringLiteral("org.qlcplus.vcwidgets.fixturegrouplayout");
static const QString KXMLFixtureGroupID = QStringLiteral("FixtureGroupID");

FixtureGroupLayoutWidget::FixtureGroupLayoutWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
    , m_fixtureGroupId(FixtureGroup::invalidId())
    , m_lastRow(0)
    , m_lastColumn(0)
    , m_dragging(false)
    , m_dragStartCell(-1, -1)
    , m_dragCurrentCell(-1, -1)
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

    QHBoxLayout* toolbar = new QHBoxLayout();
    m_clearMaskButton = new QPushButton(tr("Mask: all columns"), this);
    m_clearMaskButton->setToolTip(tr("Clear column mask (all columns active for RGB Matrix, EFX, etc.)"));
    connect(m_clearMaskButton, &QPushButton::clicked,
            this, &FixtureGroupLayoutWidget::slotClearMaskClicked);
    toolbar->addWidget(m_clearMaskButton);
    toolbar->addStretch();
    m_layout->addLayout(toolbar);

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

    connect(m_table, SIGNAL(cellChanged(int,int)),
            this, SLOT(slotCellChanged(int,int)));
    connect(m_table, SIGNAL(cellPressed(int,int)),
            this, SLOT(slotCellActivated(int,int)));
    connect(m_table->horizontalHeader(), SIGNAL(sectionClicked(int)),
            this, SLOT(slotColumnHeaderClicked(int)));

    if (m_doc != nullptr)
    {
        connect(m_doc, SIGNAL(fixtureGroupChanged(quint32)),
                this, SLOT(slotFixtureGroupChanged(quint32)));
        connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)),
                this, SLOT(slotFixtureGroupRemoved(quint32)));
        connect(m_doc, SIGNAL(fixtureGroupMaskChanged(quint32)),
                this, SLOT(slotFixtureGroupMaskChanged(quint32)));
    }

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
    return copy;
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

    if (m_localMask.isActive())
        m_doc->setFixtureGroupMask(m_fixtureGroupId, m_localMask);
    else
        m_doc->clearFixtureGroupMask(m_fixtureGroupId);
}

bool FixtureGroupLayoutWidget::isColumnMaskedOut(int column) const
{
    if (!m_localMask.isActive())
        return false;
    return !m_localMask.isColumnEnabled(column);
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
        maskHint = tr(" — mask: %1 col(s)").arg(m_localMask.columns().size());

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

    const QBrush activeBrush(palette().button());
    const QBrush maskedBrush(QColor(120, 120, 120));

    for (int col = 0; col < grp->size().width(); col++)
    {
        QTableWidgetItem* headerItem = m_table->horizontalHeaderItem(col);
        if (headerItem == nullptr)
        {
            headerItem = new QTableWidgetItem(QString::number(col + 1));
            m_table->setHorizontalHeaderItem(col, headerItem);
        }
        headerItem->setBackground(isColumnMaskedOut(col) ? maskedBrush : activeBrush);
        headerItem->setToolTip(isColumnMaskedOut(col)
            ? tr("Column %1 hidden by mask (click to include)").arg(col + 1)
            : tr("Column %1 active (click to toggle mask)").arg(col + 1));
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
        if (isColumnMaskedOut(pt.x()))
            item->setBackground(QBrush(QColor(70, 70, 70, 120)));
        m_table->setItem(pt.y(), pt.x(), item);
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

void FixtureGroupLayoutWidget::slotColumnHeaderClicked(int column)
{
    FixtureGroup* grp = fixtureGroup();
    if (grp == nullptr || column < 0 || column >= grp->size().width())
        return;

    QSet<int> cols = m_localMask.columns();
    if (!m_localMask.isActive())
    {
        cols.insert(column);
    }
    else if (cols.contains(column))
    {
        cols.remove(column);
        if (cols.isEmpty())
        {
            m_localMask.clear();
            pushMaskToDoc();
            rebuildGrid();
            return;
        }
    }
    else
    {
        cols.insert(column);
    }

    m_localMask.setColumns(cols);
    m_localMask.setRows(QSet<int>());
    pushMaskToDoc();
    rebuildGrid();
}

void FixtureGroupLayoutWidget::slotClearMaskClicked()
{
    m_localMask.clear();
    pushMaskToDoc();
    rebuildGrid();
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
            if (mouseEvent->button() == Qt::LeftButton)
            {
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

        if (event->type() == QEvent::MouseButtonRelease && m_dragging)
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

    return VCWidget::eventFilter(obj, event);
}

void FixtureGroupLayoutWidget::editProperties()
{
    if (mode() != Doc::Design)
        return;

    FixtureGroupLayoutConfigDialog dlg(m_doc, m_fixtureGroupId, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    setFixtureGroupId(dlg.fixtureGroupId());
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
        else
        {
            root.skipCurrentElement();
        }
    }

    rebuildGrid();
    return true;
}

bool FixtureGroupLayoutWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);

    saveXMLCommon(doc);

    doc->writeTextElement(KXMLFixtureGroupID, QString::number(m_fixtureGroupId));

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

    setFixtureGroupId(id);
}

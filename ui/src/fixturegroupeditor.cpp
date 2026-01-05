/*
  Q Light Controller
  fixturegroupeditor.cpp

  Copyright (c) Heikki Junnila

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

#include <QTableWidgetItem>
#include <QTableWidget>
#include <QSettings>
#include <QLineEdit>
#include <QSpinBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>

#include "fixturegroupeditor.h"
#include "fixtureselection.h"
#include "fixturegroup.h"
#include "fixture.h"
#include "doc.h"

#define SETTINGS_GEOMETRY "fixturegroupeditor/geometry"

#define PROP_FIXTURE Qt::UserRole
#define PROP_HEAD Qt::UserRole + 1

FixtureGroupEditor::FixtureGroupEditor(FixtureGroup* grp, Doc* doc, QWidget* parent)
    : QWidget(parent)
    , m_grp(grp)
    , m_doc(doc)
    , m_row(0)
    , m_column(0)
    , m_dragging(false)
    , m_dragStartCell(-1, -1)
    , m_dragCurrentCell(-1, -1)
{
    Q_ASSERT(grp != NULL);
    Q_ASSERT(doc != NULL);

    setupUi(this);

    m_nameEdit->setText(m_grp->name());
    m_xSpin->setValue(m_grp->size().width());
    m_ySpin->setValue(m_grp->size().height());

    connect(m_nameEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(slotNameEdited(const QString&)));
    connect(m_xSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotXSpinValueChanged(int)));
    connect(m_ySpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotYSpinValueChanged(int)));

    connect(m_rightButton, SIGNAL(clicked()),
            this, SLOT(slotRightClicked()));
    connect(m_leftButton, SIGNAL(clicked()),
            this, SLOT(slotLeftClicked()));
    connect(m_downButton, SIGNAL(clicked()),
            this, SLOT(slotDownClicked()));
    connect(m_upButton, SIGNAL(clicked()),
            this, SLOT(slotUpClicked())),
    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(slotRemoveFixtureClicked()));

    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setIconSize(QSize(20, 20));
    
    // Install event filter to catch arrow keys and mouse events for moving selected fixtures
    m_table->installEventFilter(this);
    m_table->viewport()->installEventFilter(this);
    
    updateTable();
}

FixtureGroupEditor::~FixtureGroupEditor()
{
}

bool FixtureGroupEditor::eventFilter(QObject* obj, QEvent* event)
{
    // Handle keyboard arrow keys (on table)
    if (obj == m_table && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        QList<QLCPoint> selectedPoints = getSelectedPoints();
        
        if (!selectedPoints.isEmpty())
        {
            int deltaX = 0;
            int deltaY = 0;
            
            switch (keyEvent->key())
            {
                case Qt::Key_Left:
                    deltaX = -1;
                    break;
                case Qt::Key_Right:
                    deltaX = 1;
                    break;
                case Qt::Key_Up:
                    deltaY = -1;
                    break;
                case Qt::Key_Down:
                    deltaY = 1;
                    break;
                default:
                    return QWidget::eventFilter(obj, event);
            }
            
            // Calculate new positions for reselection
            QList<QLCPoint> newPositions;
            foreach (const QLCPoint& pt, selectedPoints)
                newPositions.append(QLCPoint(pt.x() + deltaX, pt.y() + deltaY));
            
            moveSelectedHeads(deltaX, deltaY);
            updateTable();
            reselectPoints(newPositions);
            return true;
        }
    }
    
    // Handle mouse drag & drop (on viewport)
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
                    // Check if clicking on a selected cell with fixture
                    QList<QLCPoint> selectedPoints = getSelectedPoints();
                    QLCPoint clickedPoint(cell.x(), cell.y());
                    
                    // Start drag if clicking on a selected fixture (even single)
                    if (selectedPoints.contains(clickedPoint) && !selectedPoints.isEmpty())
                    {
                        // Store original state for live preview
                        m_dragging = true;
                        m_dragStartCell = cell;
                        m_dragCurrentCell = cell;
                        m_dragOriginalPoints = selectedPoints;
                        
                        // Save original heads for potential restore
                        m_dragOriginalHeads.clear();
                        QMap<QLCPoint, GroupHead> headsMap = m_grp->headsMap();
                        foreach (const QLCPoint& pt, selectedPoints)
                        {
                            if (headsMap.contains(pt))
                                m_dragOriginalHeads[pt] = headsMap[pt];
                        }
                        
                        return true; // Consume event to prevent default selection change
                    }
                }
            }
        }
        
        if (event->type() == QEvent::MouseMove && m_dragging)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint cell = cellAtPos(mouseEvent->pos());
            
            // Only update if cell changed
            if (cell.x() >= 0 && cell.y() >= 0 && cell != m_dragCurrentCell)
            {
                int deltaX = cell.x() - m_dragCurrentCell.x();
                int deltaY = cell.y() - m_dragCurrentCell.y();
                
                // Try to move - if successful, update current cell
                QList<QLCPoint> currentPoints = getSelectedPoints();
                if (!currentPoints.isEmpty())
                {
                    // Check bounds before moving
                    bool canMove = true;
                    int gridWidth = m_grp->size().width();
                    int gridHeight = m_grp->size().height();
                    
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
                        // Calculate new positions for reselection
                        QList<QLCPoint> newPositions;
                        foreach (const QLCPoint& pt, currentPoints)
                            newPositions.append(QLCPoint(pt.x() + deltaX, pt.y() + deltaY));
                        
                        moveSelectedHeads(deltaX, deltaY);
                        updateTable();
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
            
            // Calculate final positions for selection
            int totalDeltaX = m_dragCurrentCell.x() - m_dragStartCell.x();
            int totalDeltaY = m_dragCurrentCell.y() - m_dragStartCell.y();
            
            QList<QLCPoint> finalPositions;
            foreach (const QLCPoint& pt, m_dragOriginalPoints)
                finalPositions.append(QLCPoint(pt.x() + totalDeltaX, pt.y() + totalDeltaY));
            
            // Ensure selection is correct
            reselectPoints(finalPositions);
            
            // Clear drag state
            m_dragOriginalPoints.clear();
            m_dragOriginalHeads.clear();
            m_dragStartCell = QPoint(-1, -1);
            m_dragCurrentCell = QPoint(-1, -1);
            
            return true;
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

QPoint FixtureGroupEditor::cellAtPos(const QPoint& pos) const
{
    // pos is already in viewport coordinates when coming from viewport events
    int row = m_table->rowAt(pos.y());
    int col = m_table->columnAt(pos.x());
    
    return QPoint(col, row);
}

void FixtureGroupEditor::reselectPoints(const QList<QLCPoint>& points)
{
    m_table->clearSelection();
    
    int gridWidth = m_grp->size().width();
    int gridHeight = m_grp->size().height();
    
    foreach (const QLCPoint& pt, points)
    {
        int col = pt.x();
        int row = pt.y();
        
        // Only select if within bounds
        if (col >= 0 && col < gridWidth && row >= 0 && row < gridHeight)
        {
            QTableWidgetItem* item = m_table->item(row, col);
            if (item != NULL)
                item->setSelected(true);
            else
            {
                // Select the cell even if empty (for consistency)
                m_table->setCurrentCell(row, col, QItemSelectionModel::Select);
            }
        }
    }
}

void FixtureGroupEditor::updateTable()
{
    qDebug() << Q_FUNC_INFO;
    // Store these since they might get reset
    int savedRow = m_row;
    int savedCol = m_column;

    disconnect(m_table, SIGNAL(cellChanged(int,int)),
               this, SLOT(slotCellChanged(int,int)));
    disconnect(m_table, SIGNAL(cellPressed(int,int)),
               this, SLOT(slotCellActivated(int,int)));
    disconnect(m_table->horizontalHeader(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(slotResized()));

    m_table->clear();

    m_table->setRowCount(m_grp->size().height());
    m_table->setColumnCount(m_grp->size().width());

    QMapIterator <QLCPoint,GroupHead> it(m_grp->headsMap());
    while (it.hasNext() == true)
    {
        it.next();

        QLCPoint pt(it.key());

        GroupHead head(it.value());
        Fixture* fxi = m_doc->fixture(head.fxi);
        if (fxi == NULL)
            continue;

        QIcon icon = fxi->getIconFromType();
        QString str = QString("%1 H:%2\nA:%3 U:%4").arg(fxi->name())
                                               .arg(head.head + 1)
                                               .arg(fxi->address() + 1)
                                               .arg(fxi->universe() + 1);

        QTableWidgetItem* item = new QTableWidgetItem(icon, str);
        item->setData(PROP_FIXTURE, head.fxi);
        item->setData(PROP_HEAD, head.head);
        item->setToolTip(str);

        m_table->setItem(pt.y(), pt.x(), item);
    }

    connect(m_table, SIGNAL(cellPressed(int,int)),
            this, SLOT(slotCellActivated(int,int)));
    connect(m_table, SIGNAL(cellChanged(int,int)),
            this, SLOT(slotCellChanged(int,int)));
    connect(m_table->horizontalHeader(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(slotResized()));

    if (savedRow < m_table->rowCount() && savedCol < m_table->columnCount())
    {
        m_row = savedRow;
        m_column = savedCol;
    }
    else
    {
        m_row = 0;
        m_column = 0;
    }

    m_table->setCurrentCell(m_row, m_column);
    slotResized();
}

void FixtureGroupEditor::slotNameEdited(const QString& text)
{
    m_grp->setName(text);
}

void FixtureGroupEditor::slotXSpinValueChanged(int value)
{
    m_grp->setSize(QSize(value, m_grp->size().height()));
    updateTable();
}

void FixtureGroupEditor::slotYSpinValueChanged(int value)
{
    m_grp->setSize(QSize(m_grp->size().width(), value));
    updateTable();
}

void FixtureGroupEditor::slotRightClicked()
{
    addFixtureHeads(Qt::RightArrow);
}

void FixtureGroupEditor::slotLeftClicked()
{
    addFixtureHeads(Qt::LeftArrow);
}

void FixtureGroupEditor::slotDownClicked()
{
    addFixtureHeads(Qt::DownArrow);
}

void FixtureGroupEditor::slotUpClicked()
{
    addFixtureHeads(Qt::UpArrow);
}


void FixtureGroupEditor::slotRemoveFixtureClicked()
{
    QList<QLCPoint> selectedPoints = getSelectedPoints();
    
    if (selectedPoints.isEmpty())
    {
        // Fallback to single cell behavior
        QTableWidgetItem* item = m_table->currentItem();
        if (item == NULL)
            return;

        if (m_grp->resignHead(QLCPoint(m_column, m_row)) == true)
            delete item;
    }
    else
    {
        // Remove all selected fixtures
        foreach (const QLCPoint& pt, selectedPoints)
        {
            m_grp->resignHead(pt);
        }
        updateTable();
    }
}

void FixtureGroupEditor::slotCellActivated(int row, int column)
{
    m_row = row;
    m_column = column;

    // Enable remove button if any selected cell has a fixture
    QList<QLCPoint> selectedPoints = getSelectedPoints();
    m_removeButton->setEnabled(!selectedPoints.isEmpty());
}

void FixtureGroupEditor::slotCellChanged(int row, int column)
{
    // This is now only called for single-cell internal operations
    // Multi-selection movement is handled by arrow keys via eventFilter
    if (row < 0 || column < 0)
    {
        updateTable();
        return;
    }

    // Single cell swap (legacy behavior)
    QLCPoint from(m_column, m_row);
    QLCPoint to(column, row);
    
    if (from != to)
        m_grp->swap(from, to);

    updateTable();
    m_table->setCurrentCell(row, column);
    slotCellActivated(row, column);
}

void FixtureGroupEditor::slotResized()
{
    disconnect(m_table, SIGNAL(cellChanged(int,int)),
               this, SLOT(slotCellChanged(int,int)));

    float cellWidth = (float)(m_table->columnWidth(0) - m_table->iconSize().width());
    QFont font = m_table->font();
    QFontMetrics fm(font);
    float pSizeF = font.pointSizeF();

    for (int y = 0; y < m_table->rowCount(); y++)
    {
        for (int x = 0; x < m_table->columnCount(); x++)
        {
            QTableWidgetItem* item = m_table->item(y, x);
            if (item != NULL)
            {
                QFont scaledFont = font;
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
                float baseWidth  = (float)fm.width(item->text());
#else
                float baseWidth  = (float)fm.horizontalAdvance(item->text());
#endif
                float factor = cellWidth / baseWidth;
                if (factor != 1)
                    scaledFont.setPointSizeF((pSizeF * factor) + 2);
                else
                    scaledFont.setPointSize(font.pointSize() - 2);

                item->setFont(scaledFont);
            }
        }
    }

    connect(m_table, SIGNAL(cellChanged(int,int)),
            this, SLOT(slotCellChanged(int,int)));
}

void FixtureGroupEditor::addFixtureHeads(Qt::ArrowType direction)
{
    FixtureSelection fs(this, m_doc);
    fs.setMultiSelection(true);
    fs.setSelectionMode(FixtureSelection::Heads);
    fs.setDisabledHeads(m_grp->headList());
    if (fs.exec() == QDialog::Accepted)
    {
        int row = m_row;
        int col = m_column;
        foreach (GroupHead gh, fs.selectedHeads())
        {
            m_grp->assignHead(QLCPoint(col, row), gh);
            if (direction == Qt::RightArrow)
                col++;
            else if (direction == Qt::DownArrow)
                row++;
            else if (direction == Qt::LeftArrow)
                col--;
            else if (direction == Qt::UpArrow)
                row--;
        }

        updateTable();
        m_table->setCurrentCell(row, col);
    }
}

QList<QLCPoint> FixtureGroupEditor::getSelectedPoints() const
{
    QList<QLCPoint> points;
    QMap<QLCPoint, GroupHead> headsMap = m_grp->headsMap();
    
    foreach (QTableWidgetItem* item, m_table->selectedItems())
    {
        if (item == NULL)
            continue;
            
        int col = item->column();
        int row = item->row();
        QLCPoint pt(col, row);
        
        // Only include points that have fixtures
        if (headsMap.contains(pt))
            points.append(pt);
    }
    
    return points;
}

void FixtureGroupEditor::moveSelectedHeads(int deltaX, int deltaY)
{
    QList<QLCPoint> selectedPoints = getSelectedPoints();
    if (selectedPoints.isEmpty())
        return;

    int gridWidth = m_grp->size().width();
    int gridHeight = m_grp->size().height();

    // Check if all new positions are within grid bounds
    foreach (const QLCPoint& pt, selectedPoints)
    {
        int newX = pt.x() + deltaX;
        int newY = pt.y() + deltaY;
        
        if (newX < 0 || newX >= gridWidth || newY < 0 || newY >= gridHeight)
        {
            qDebug() << "Move blocked: destination out of bounds";
            return;
        }
    }

    // Collect source heads and clear their positions
    QMap<QLCPoint, GroupHead> headsToMove;
    QMap<QLCPoint, GroupHead> currentMap = m_grp->headsMap();
    
    foreach (const QLCPoint& pt, selectedPoints)
    {
        if (currentMap.contains(pt))
        {
            headsToMove[pt] = currentMap[pt];
        }
    }

    // Calculate target positions and collect heads that need to be swapped
    QMap<QLCPoint, GroupHead> headsToSwap;  // heads at target positions not in selection
    QList<QLCPoint> targetPoints;
    
    foreach (const QLCPoint& pt, selectedPoints)
    {
        QLCPoint targetPt(pt.x() + deltaX, pt.y() + deltaY);
        targetPoints.append(targetPt);
        
        // If target has a head that's not part of our selection, save it for swap
        if (currentMap.contains(targetPt) && !selectedPoints.contains(targetPt))
        {
            headsToSwap[targetPt] = currentMap[targetPt];
        }
    }

    // Clear source positions
    foreach (const QLCPoint& pt, selectedPoints)
    {
        m_grp->resignHead(pt);
    }

    // Clear target positions that had heads not in selection
    foreach (const QLCPoint& pt, headsToSwap.keys())
    {
        m_grp->resignHead(pt);
    }

    // Move heads to new positions
    QMapIterator<QLCPoint, GroupHead> it(headsToMove);
    while (it.hasNext())
    {
        it.next();
        QLCPoint newPt(it.key().x() + deltaX, it.key().y() + deltaY);
        m_grp->assignHead(newPt, it.value());
    }

    // Swap: place displaced heads at original positions of moved heads
    if (!headsToSwap.isEmpty())
    {
        QMapIterator<QLCPoint, GroupHead> swapIt(headsToSwap);
        QList<QLCPoint> sourcePositions = headsToMove.keys();
        int i = 0;
        
        while (swapIt.hasNext() && i < sourcePositions.size())
        {
            swapIt.next();
            // Find an empty source position
            while (i < sourcePositions.size())
            {
                QLCPoint sourcePt = sourcePositions[i];
                // Check if this source position is not a target for another head
                if (!targetPoints.contains(sourcePt))
                {
                    m_grp->assignHead(sourcePt, swapIt.value());
                    break;
                }
                i++;
            }
            i++;
        }
    }
}

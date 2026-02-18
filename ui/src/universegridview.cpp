/*
  Q Light Controller Plus
  universegridview.cpp

  Copyright (c) Heikki Junnila
                Massimo Callegari

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

#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "universegridview.h"
#include "qlcfixturemode.h"
#include "qlcchannel.h"
#include "fixture.h"
#include "doc.h"

#define GRID_COLS 32
#define GRID_ROWS 16
#define MIN_CELL_SIZE 15
#define MARGIN 5

UniverseGridView::UniverseGridView(QWidget *parent, Doc *doc)
    : QWidget(parent)
    , m_doc(doc)
    , m_universeID(Universe::invalid())
    , m_selectedFixtureID(Fixture::invalidId())
    , m_cellSize(20)
    , m_labelWidth(35)
    , m_labelHeight(20)
    , m_draggedFixtureID(Fixture::invalidId())
    , m_dropTargetChannel(-1)
    , m_isDragging(false)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setMinimumSize(GRID_COLS * MIN_CELL_SIZE + m_labelWidth + MARGIN * 2,
                   GRID_ROWS * MIN_CELL_SIZE + m_labelHeight + MARGIN * 2);
    calculateCellSize();
}

UniverseGridView::~UniverseGridView()
{
}

void UniverseGridView::setUniverse(quint32 id)
{
    m_universeID = id;
    update();
}

void UniverseGridView::setSelectedFixture(quint32 fixtureID)
{
    m_selectedFixtureID = fixtureID;
    update();
}

void UniverseGridView::calculateCellSize()
{
    int availableWidth = width() - m_labelWidth - MARGIN * 2;
    int availableHeight = height() - m_labelHeight - MARGIN * 2;

    int cellWidth = availableWidth / GRID_COLS;
    int cellHeight = availableHeight / GRID_ROWS;

    m_cellSize = qMin(cellWidth, cellHeight);
    m_cellSize = qMax(m_cellSize, MIN_CELL_SIZE);
}

int UniverseGridView::getChannelAtPosition(const QPoint &pos) const
{
    int x = pos.x() - m_labelWidth - MARGIN;
    int y = pos.y() - m_labelHeight - MARGIN;

    if (x < 0 || y < 0)
        return -1;

    int col = x / m_cellSize;
    int row = y / m_cellSize;

    if (col >= GRID_COLS || row >= GRID_ROWS)
        return -1;

    return row * GRID_COLS + col;
}

bool UniverseGridView::isAddressAvailable(quint32 startAddress, quint32 channels, quint32 excludeFixtureID) const
{
    for (quint32 i = 0; i < channels; i++)
    {
        quint32 addr = startAddress + i;
        
        // Check if address is within universe bounds
        if ((addr % 512) + channels > 512)
            return false;
            
        quint32 existingFixtureID = m_doc->fixtureForAddress(addr);
        
        if (existingFixtureID != Fixture::invalidId() && 
            existingFixtureID != excludeFixtureID)
        {
            return false;
        }
    }
    return true;
}

void UniverseGridView::startDrag(quint32 fixtureID, const QPoint &pos)
{
    Fixture *fixture = m_doc->fixture(fixtureID);
    if (!fixture)
        return;
    
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    mimeData->setText(QString::number(fixtureID));
    drag->setMimeData(mimeData);
    
    // Create drag pixmap
    int pixmapWidth = m_cellSize * fixture->channels();
    QPixmap pixmap(pixmapWidth, m_cellSize);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int hue = (fixtureID * 137) % 360;
    QColor cellColor = QColor::fromHsv(hue, 180, 230);
    
    for (quint32 i = 0; i < fixture->channels(); i++)
    {
        QRect cellRect(i * m_cellSize, 0, m_cellSize, m_cellSize);
        painter.fillRect(cellRect, cellColor);
        painter.setPen(Qt::black);
        painter.drawRect(cellRect);
    }
    
    painter.end();
    drag->setPixmap(pixmap);
    drag->setHotSpot(pos - m_dragStartPos);
    
    m_isDragging = true;
    drag->exec(Qt::MoveAction);
    m_isDragging = false;
    m_draggedFixtureID = Fixture::invalidId();
}

void UniverseGridView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    calculateCellSize();
    QWidget::resizeEvent(event);
}

void UniverseGridView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), palette().color(QPalette::Window));

    if (m_universeID == Universe::invalid())
        return;

    // Setup font for labels
    QFont labelFont = font();
    labelFont.setPointSize(8);
    painter.setFont(labelFont);
    painter.setPen(palette().color(QPalette::Text));

    // Draw column headers (every 4 columns)
    for (int col = 0; col < GRID_COLS; col += 4)
    {
        int channelNum = col * GRID_ROWS + 1;
        int x = m_labelWidth + MARGIN + col * m_cellSize;
        painter.drawText(x, 0, m_cellSize * 4, m_labelHeight,
                        Qt::AlignCenter | Qt::AlignBottom,
                        QString::number(channelNum));
    }

    // Draw row headers
    for (int row = 0; row < GRID_ROWS; row++)
    {
        int y = m_labelHeight + MARGIN + row * m_cellSize;
        painter.drawText(0, y, m_labelWidth, m_cellSize,
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString("%1:").arg(row + 1));
    }

    // Draw grid cells
    int startX = m_labelWidth + MARGIN;
    int startY = m_labelHeight + MARGIN;

    for (int i = 0; i < 512; i++)
    {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        
        int x = startX + col * m_cellSize;
        int y = startY + row * m_cellSize;
        
        QRect cellRect(x, y, m_cellSize, m_cellSize);
        
        // Default color for empty channel
        QColor cellColor = palette().color(QPalette::Base);
        QColor borderColor = palette().color(QPalette::Mid);

        // Check if channel is occupied
        quint32 fixtureID = m_doc->fixtureForAddress(m_universeID * 512 + i);
        Fixture *fixture = m_doc->fixture(fixtureID);

        if (fixture != NULL)
        {
            // Generate a color based on fixture ID
            int hue = (fixtureID * 137) % 360;
            cellColor = QColor::fromHsv(hue, 180, 230);
            borderColor = QColor::fromHsv(hue, 255, 150);
            
            // Highlight selected fixture with better border
            if (fixtureID == m_selectedFixtureID)
            {
                painter.setBrush(cellColor);
                
                // Outer dark border
                painter.setPen(QPen(QColor(50, 50, 50), 3));
                painter.drawRect(cellRect.adjusted(-1, -1, 1, 1));
                
                // Inner bright border
                painter.setPen(QPen(QColor(255, 220, 0), 2));
                painter.drawRect(cellRect.adjusted(1, 1, -1, -1));
                
                continue;
            }
        }

        painter.setBrush(cellColor);
        painter.setPen(borderColor);
        painter.drawRect(cellRect);
        
        // Draw channel number for larger cells (both empty and occupied)
        if (m_cellSize >= 20)  // Lowered threshold from 25 to 20
        {
            // Calculate contrasting text color based on cell brightness
            int brightness = (cellColor.red() * 299 + cellColor.green() * 587 + cellColor.blue() * 114) / 1000;
            QColor textColor = (brightness > 128) ? Qt::black : Qt::white;
            
            painter.setPen(textColor);
            QFont smallFont = font();
            smallFont.setPointSize(7);
            smallFont.setBold(true);
            painter.setFont(smallFont);
            painter.drawText(cellRect, Qt::AlignCenter, QString::number(i + 1));
            painter.setFont(labelFont);
        }
    }
    
    // Draw drop target indicator
    if (m_dropTargetChannel >= 0 && m_draggedFixtureID != Fixture::invalidId())
    {
        Fixture *draggedFixture = m_doc->fixture(m_draggedFixtureID);
        if (draggedFixture)
        {
            quint32 targetAddress = m_universeID * 512 + m_dropTargetChannel;
            bool available = isAddressAvailable(targetAddress, draggedFixture->channels(), m_draggedFixtureID);
            
            QColor dropColor = available ? QColor(0, 255, 0, 150) : QColor(255, 0, 0, 150);
            
            for (quint32 i = 0; i < draggedFixture->channels(); i++)
            {
                int channelIndex = m_dropTargetChannel + i;
                if (channelIndex >= 512)
                    break;
                    
                int col = channelIndex % GRID_COLS;
                int row = channelIndex / GRID_COLS;
                
                int x = startX + col * m_cellSize;
                int y = startY + row * m_cellSize;
                
                QRect targetRect(x, y, m_cellSize, m_cellSize);
                painter.setPen(QPen(dropColor, 3, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(targetRect);
            }
        }
    }
}

void UniverseGridView::mousePressEvent(QMouseEvent *event)
{
    if (m_universeID == Universe::invalid())
        return;

    if (event->button() != Qt::LeftButton)
        return;

    int channelIndex = getChannelAtPosition(event->pos());
    
    if (channelIndex >= 0 && channelIndex < 512)
    {
        quint32 fixtureID = m_doc->fixtureForAddress(m_universeID * 512 + channelIndex);
        if (fixtureID != Fixture::invalidId())
        {
            m_dragStartPos = event->pos();
            m_draggedFixtureID = fixtureID;
            
            // Also emit selection
            emit fixtureSelected(fixtureID);
        }
    }
}

void UniverseGridView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_universeID == Universe::invalid())
        return;

    // Handle drag start
    if ((event->buttons() & Qt::LeftButton) && m_draggedFixtureID != Fixture::invalidId())
    {
        if ((event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance())
        {
            startDrag(m_draggedFixtureID, event->pos());
            return;
        }
    }

    // Update tooltip
    int channelIndex = getChannelAtPosition(event->pos());
    
    if (channelIndex >= 0 && channelIndex < 512)
    {
        quint32 fixtureID = m_doc->fixtureForAddress(m_universeID * 512 + channelIndex);
        Fixture *fixture = m_doc->fixture(fixtureID);
        
        if (fixture != NULL)
        {
            QString tooltip = QString("<b>Channel %1</b><br>%2<br>Address: %3-%4")
                .arg(channelIndex + 1)
                .arg(fixture->name())
                .arg(fixture->address() + 1)
                .arg(fixture->address() + fixture->channels());
            setToolTip(tooltip);
        }
        else
        {
            setToolTip(QString("Channel %1 (Empty)").arg(channelIndex + 1));
        }
    }
    else
    {
        setToolTip("");
    }
}

void UniverseGridView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText())
    {
        event->acceptProposedAction();
    }
}

void UniverseGridView::dragMoveEvent(QDragMoveEvent *event)
{
    int channelIndex = getChannelAtPosition(event->position().toPoint());
    
    if (channelIndex >= 0 && channelIndex < 512)
    {
        m_dropTargetChannel = channelIndex;
        update();
        event->acceptProposedAction();
    }
    else
    {
        m_dropTargetChannel = -1;
        update();
        event->ignore();
    }
}

void UniverseGridView::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    m_dropTargetChannel = -1;
    update();
}

void UniverseGridView::dropEvent(QDropEvent *event)
{
    int targetChannel = getChannelAtPosition(event->position().toPoint());
    if (targetChannel < 0 || targetChannel >= 512)
    {
        m_dropTargetChannel = -1;
        update();
        return;
    }
    
    quint32 fixtureID = event->mimeData()->text().toUInt();
    Fixture *fixture = m_doc->fixture(fixtureID);
    if (!fixture)
    {
        m_dropTargetChannel = -1;
        update();
        return;
    }
    
    quint32 newAddress = m_universeID * 512 + targetChannel;
    
    // Validate address
    if (!isAddressAvailable(newAddress, fixture->channels(), fixtureID))
    {
        QMessageBox::warning(this, tr("Address Conflict"),
            tr("Cannot move fixture to channel %1 - address range is occupied or out of bounds").arg(targetChannel + 1));
        m_dropTargetChannel = -1;
        update();
        return;
    }
    
    // Emit signal to change address
    emit fixtureAddressChangeRequested(fixtureID, newAddress);
    
    event->acceptProposedAction();
    m_dropTargetChannel = -1;
    update();
}

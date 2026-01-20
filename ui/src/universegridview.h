/*
  Q Light Controller Plus
  universegridview.h

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

#ifndef UNIVERSEGRIDVIEW_H
#define UNIVERSEGRIDVIEW_H

#include <QWidget>

class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

class UniverseGridView : public QWidget
{
    Q_OBJECT
public:
    explicit UniverseGridView(QWidget *parent, Doc *doc);
    ~UniverseGridView();

    /** Set the ID of the universe to display */
    void setUniverse(quint32 id);

public slots:
    /** Set the selected fixture to highlight */
    void setSelectedFixture(quint32 fixtureID);

protected:
    /** @reimp */
    void paintEvent(QPaintEvent *event);

    /** @reimp */
    void mousePressEvent(QMouseEvent *event);

    /** @reimp */
    void mouseMoveEvent(QMouseEvent *event);

    /** @reimp */
    void resizeEvent(QResizeEvent *event);

    /** @reimp */
    void dragEnterEvent(QDragEnterEvent *event);

    /** @reimp */
    void dragMoveEvent(QDragMoveEvent *event);

    /** @reimp */
    void dragLeaveEvent(QDragLeaveEvent *event);

    /** @reimp */
    void dropEvent(QDropEvent *event);

signals:
    /** Emitted when a fixture is clicked in the grid */
    void fixtureSelected(quint32 fixtureID);

    /** Emitted when user wants to change fixture address via drag-drop */
    void fixtureAddressChangeRequested(quint32 fixtureID, quint32 newAddress);

private:
    /** Calculate optimal cell size based on widget dimensions */
    void calculateCellSize();

    /** Get channel index at given position */
    int getChannelAtPosition(const QPoint &pos) const;

    /** Check if address range is available for fixture placement */
    bool isAddressAvailable(quint32 startAddress, quint32 channels, quint32 excludeFixtureID) const;

    /** Start drag operation */
    void startDrag(quint32 fixtureID, const QPoint &pos);

private:
    Doc *m_doc;
    quint32 m_universeID;
    quint32 m_selectedFixtureID;
    int m_cellSize;
    int m_labelWidth;
    int m_labelHeight;
    
    // Drag & drop state
    QPoint m_dragStartPos;
    quint32 m_draggedFixtureID;
    int m_dropTargetChannel;
    bool m_isDragging;
};

/** @} */

#endif // UNIVERSEGRIDVIEW_H

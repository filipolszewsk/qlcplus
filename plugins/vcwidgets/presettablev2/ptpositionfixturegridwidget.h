/*
  ptpositionfixturegridwidget.h — fixture-group grid for Position Mode editing
*/

#pragma once

#include "presettablev2widget.h"

#include <QColor>
#include <QList>
#include <QMap>
#include <QSet>
#include <QSize>
#include <QString>
#include <QVector>
#include <QWidget>

class QContextMenuEvent;
class QKeyEvent;

struct PTPositionGridSelectionLayer
{
    QString name;
    QColor color;
    QSet<QLCPoint> cells;
};

struct PTPositionGridCell
{
    QLCPoint        point;
    QString         label;
    PTPositionValue position;
    bool            inherited = false;
};

class PTPositionFixtureGridWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PTPositionFixtureGridWidget(QWidget* parent = nullptr);

    void setGrid(const QSize& gridSize,
                 const QMap<QLCPoint, PTPositionGridCell>& cells);
    void setPlaceholderText(const QString& text);
    void setSelectedCells(const QSet<QLCPoint>& cells,
                          const QList<QLCPoint>& order = QList<QLCPoint>());
    void setImplicitAllSelection(bool implicitAll);
    void setEditableCells(const QSet<QLCPoint>& cells);
    void setForeignSelectionLayers(const QVector<PTPositionGridSelectionLayer>& layers);

    QSet<QLCPoint> selectedCells() const { return m_selectedCells; }
    QList<QLCPoint> selectionOrder() const { return m_selectionOrder; }

    static QList<QLCPoint> rowMajorOrder(const QSet<QLCPoint>& cells);

signals:
    void selectionChanged(const QSet<QLCPoint>& cells, const QList<QLCPoint>& order);
    void copyRequested();
    void pasteRequested();
    void clearRequested();
    void cellEditRequested(const QLCPoint& point);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    QSize sizeHint() const override;

private:
    QRect cellRect(const QLCPoint& pt) const;
    QLCPoint pointAt(const QPoint& pos) const;
    void selectSingleCell(const QLCPoint& pt);
    void selectRectRange(const QLCPoint& from, const QLCPoint& to);
    void toggleSelectionCell(const QLCPoint& pt);
    bool hasAnchor() const;
    bool isEditableCell(const QLCPoint& pt) const;
    void emitSelectionChanged();

    QSize m_gridSize;
    QMap<QLCPoint, PTPositionGridCell> m_cells;
    QSet<QLCPoint> m_selectedCells;
    QList<QLCPoint> m_selectionOrder;
    QLCPoint m_selectionAnchor = QLCPoint(-1, -1);
    QSet<QLCPoint> m_editableCells;
    QVector<PTPositionGridSelectionLayer> m_foreignSelectionLayers;
    bool m_implicitAllSelected = false;
    QString m_placeholder;
};

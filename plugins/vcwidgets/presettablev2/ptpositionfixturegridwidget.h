/*
  ptpositionfixturegridwidget.h — fixture-group grid for Position Mode editing
*/

#pragma once

#include "presettablev2widget.h"

#include <QList>
#include <QMap>
#include <QSet>
#include <QSize>
#include <QString>
#include <QWidget>

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

    QSet<QLCPoint> selectedCells() const { return m_selectedCells; }
    QList<QLCPoint> selectionOrder() const { return m_selectionOrder; }

    static QList<QLCPoint> rowMajorOrder(const QSet<QLCPoint>& cells);

signals:
    void selectionChanged(const QSet<QLCPoint>& cells, const QList<QLCPoint>& order);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    QRect cellRect(const QLCPoint& pt) const;
    QLCPoint pointAt(const QPoint& pos) const;
    void selectSingleCell(const QLCPoint& pt);
    void selectRectRange(const QLCPoint& from, const QLCPoint& to);
    void toggleSelectionCell(const QLCPoint& pt);
    bool hasAnchor() const;
    void emitSelectionChanged();

    QSize m_gridSize;
    QMap<QLCPoint, PTPositionGridCell> m_cells;
    QSet<QLCPoint> m_selectedCells;
    QList<QLCPoint> m_selectionOrder;
    QLCPoint m_selectionAnchor = QLCPoint(-1, -1);
    bool m_implicitAllSelected = false;
    QString m_placeholder;
};

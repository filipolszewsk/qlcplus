/*
  ptpositionfixturegridwidget.h — fixture-group grid for Position Mode editing
*/

#pragma once

#include "presettablev2widget.h"

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
    void setSelectedCells(const QSet<QLCPoint>& cells);

    QSet<QLCPoint> selectedCells() const { return m_selectedCells; }

signals:
    void selectionChanged(const QSet<QLCPoint>& cells);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    QRect cellRect(const QLCPoint& pt) const;
    QLCPoint pointAt(const QPoint& pos) const;
    void toggleSelection(const QLCPoint& pt, bool extend);

    QSize m_gridSize;
    QMap<QLCPoint, PTPositionGridCell> m_cells;
    QSet<QLCPoint> m_selectedCells;
    QString m_placeholder;
};

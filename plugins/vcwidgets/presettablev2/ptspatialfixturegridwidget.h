/*
  Fixture group spatial preview: chase order, offset°, phase.
*/

#pragma once

#include "ptspatialfixtureplan.h"

#include <QColor>
#include <QSet>
#include <QVector>
#include <QWidget>

struct PTSpatialGridSelectionLayer
{
    int selectionIndex = -1;
    QString name;
    QColor color;
    QSet<QLCPoint> cells;
};

class PTSpatialFixtureGridWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PTSpatialFixtureGridWidget(QWidget* parent = nullptr);

    void setPreview(const PTSpatialGridPreview& preview);
    void setPlaceholderText(const QString& text);
    void setStatusTextVisible(bool visible);
    void setSelectionLayers(bool editable,
                            int activeSelectionIndex,
                            const QSet<QLCPoint>& scopeCells,
                            const QVector<PTSpatialGridSelectionLayer>& layers);

    /** Position edit mode: highlight cells with stored positions and pick active cell. */
    void setPositionEditMode(const PTSpatialGridPreview& preview,
                             const QSet<QLCPoint>& filledCells,
                             const QSet<QLCPoint>& selectedCells,
                             const QSet<QLCPoint>& inheritedCells);

    QSet<QLCPoint> activeSelectionCells() const { return m_activeSelectionCells; }

signals:
    void selectionCellsChanged(const QSet<QLCPoint>& cells);
    /** Emitted when user clicks a cell belonging to another selection layer (0-based index). */
    void selectionLayerActivated(int selectionIndex);
    void positionCellClicked(const QLCPoint& point);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    QRect gridCellRect(const QLCPoint& pt) const;
    QLCPoint pointAtPosition(const QPoint& pos) const;

    PTSpatialGridPreview m_preview;
    QString m_placeholder;
    bool m_statusTextVisible = true;
    bool m_positionEditMode = false;
    QSet<QLCPoint> m_positionFilledCells;
    QSet<QLCPoint> m_positionSelectedCells;
    QSet<QLCPoint> m_positionInheritedCells;
    bool m_selectionEditing = false;
    int m_activeSelectionIndex = -1;
    QSet<QLCPoint> m_selectionScopeCells;
    QSet<QLCPoint> m_activeSelectionCells;
    QVector<PTSpatialGridSelectionLayer> m_selectionLayers;
};

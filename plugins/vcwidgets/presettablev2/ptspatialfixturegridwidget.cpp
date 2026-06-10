/*
  ptspatialfixturegridwidget.cpp
*/

#include "ptspatialfixturegridwidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>

namespace {

QColor offsetFillColor(int offsetDeg)
{
    const int deg = ((offsetDeg % 360) + 360) % 360;
    const int value = 35 + int(double(deg) / 360.0 * 200.0);
    return QColor(value, value, value);
}

QColor outputBorderColor(int outputIndex)
{
    static const QColor colors[] = {
        QColor(230, 80, 80), QColor(60, 150, 240), QColor(80, 180, 110),
        QColor(235, 170, 55), QColor(170, 105, 230), QColor(50, 190, 190),
        QColor(230, 105, 170), QColor(135, 155, 55), QColor(95, 115, 230),
        QColor(210, 115, 55), QColor(75, 170, 150), QColor(185, 85, 115),
        QColor(115, 135, 155), QColor(160, 140, 65), QColor(100, 100, 220),
        QColor(55, 155, 95)
    };
    return colors[qAbs(outputIndex) % (int(sizeof(colors) / sizeof(colors[0])))];
}

QColor textColorForFill(const QColor& fill)
{
    return fill.lightness() < 135 ? QColor(245, 245, 245) : QColor(25, 25, 25);
}

void drawOutputBorder(QPainter& p, const QRect& rect, const QList<int>& outputIndexes)
{
    if (outputIndexes.isEmpty())
        return;

    if (outputIndexes.size() == 1)
    {
        p.setPen(QPen(outputBorderColor(outputIndexes.first()), 3));
        p.drawRect(rect.adjusted(1, 1, -1, -1));
        return;
    }

    const int n = outputIndexes.size();
    const int left = rect.left() + 1;
    const int right = rect.right() - 1;
    const int top = rect.top() + 1;
    const int bottom = rect.bottom() - 1;
    const int w = qMax(1, right - left + 1);
    const int h = qMax(1, bottom - top + 1);

    for (int i = 0; i < n; ++i)
    {
        const QColor color = outputBorderColor(outputIndexes.at(i));
        p.setPen(QPen(color, 3));

        const int x1 = left + (w * i) / n;
        const int x2 = left + (w * (i + 1)) / n - 1;
        p.drawLine(QPoint(x1, top), QPoint(x2, top));
        p.drawLine(QPoint(x1, bottom), QPoint(x2, bottom));

        if (i == 0)
            p.drawLine(QPoint(left, top), QPoint(left, bottom));
        if (i == n - 1)
            p.drawLine(QPoint(right, top), QPoint(right, bottom));
    }
}

} // namespace

PTSpatialFixtureGridWidget::PTSpatialFixtureGridWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(120, 120);
    setAutoFillBackground(true);
}

void PTSpatialFixtureGridWidget::setPreview(const PTSpatialGridPreview& preview)
{
    m_preview = preview;
    m_placeholder.clear();
    updateGeometry();
    update();
}

void PTSpatialFixtureGridWidget::setPlaceholderText(const QString& text)
{
    m_preview = PTSpatialGridPreview();
    m_placeholder = text;
    m_selectionEditing = false;
    m_activeSelectionIndex = -1;
    m_selectionScopeCells.clear();
    m_activeSelectionCells.clear();
    m_selectionLayers.clear();
    update();
}

void PTSpatialFixtureGridWidget::setSelectionLayers(
        bool editable,
        int activeSelectionIndex,
        const QSet<QLCPoint>& scopeCells,
        const QVector<PTSpatialGridSelectionLayer>& layers)
{
    m_selectionEditing = editable;
    m_activeSelectionIndex = activeSelectionIndex;
    m_selectionScopeCells = scopeCells;
    m_selectionLayers = layers;
    m_activeSelectionCells.clear();
    for (const PTSpatialGridSelectionLayer& layer : m_selectionLayers)
    {
        if (layer.selectionIndex == m_activeSelectionIndex)
        {
            m_activeSelectionCells = layer.cells;
            break;
        }
    }
    setCursor(m_selectionEditing ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

QSize PTSpatialFixtureGridWidget::sizeHint() const
{
    if (!m_preview.valid)
        return QSize(160, 120);

    const int cw = qMin(48, qMax(28, 320 / qMax(1, m_preview.gridSize.width())));
    const int ch = qMin(48, qMax(28, 240 / qMax(1, m_preview.gridSize.height())));
    return QSize(qMax(120, m_preview.gridSize.width() * cw + 16),
                 qMax(100, m_preview.gridSize.height() * ch + 36));
}

QRect PTSpatialFixtureGridWidget::gridCellRect(const QLCPoint& pt) const
{
    if (!m_preview.valid)
        return QRect();

    const int cols = qMax(1, m_preview.gridSize.width());
    const int rows = qMax(1, m_preview.gridSize.height());
    if (pt.x() < 0 || pt.y() < 0 || pt.x() >= cols || pt.y() >= rows)
        return QRect();

    const QRect area = rect().adjusted(6, 6, -6, -28);
    const int cellW = qMax(24, area.width() / cols);
    const int cellH = qMax(24, area.height() / rows);
    const int totalW = cellW * cols;
    const int totalH = cellH * rows;
    const int ox = area.left() + (area.width() - totalW) / 2;
    const int oy = area.top() + (area.height() - totalH) / 2;
    return QRect(ox + pt.x() * cellW, oy + pt.y() * cellH, cellW - 2, cellH - 2);
}

QLCPoint PTSpatialFixtureGridWidget::pointAtPosition(const QPoint& pos) const
{
    if (!m_preview.valid)
        return QLCPoint(-1, -1);

    const int cols = qMax(1, m_preview.gridSize.width());
    const int rows = qMax(1, m_preview.gridSize.height());
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const QLCPoint pt(x, y);
            if (gridCellRect(pt).contains(pos))
                return pt;
        }
    }
    return QLCPoint(-1, -1);
}

void PTSpatialFixtureGridWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_selectionEditing || !m_preview.valid || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    const QLCPoint pt = pointAtPosition(event->pos());
    if (pt.x() < 0 || !m_selectionScopeCells.contains(pt))
        return;

    if (m_activeSelectionCells.contains(pt))
        m_activeSelectionCells.remove(pt);
    else
        m_activeSelectionCells.insert(pt);

    for (PTSpatialGridSelectionLayer& layer : m_selectionLayers)
    {
        if (layer.selectionIndex == m_activeSelectionIndex)
            layer.cells = m_activeSelectionCells;
        else
            layer.cells.remove(pt);
    }

    emit selectionCellsChanged(m_activeSelectionCells);
    update();
}

void PTSpatialFixtureGridWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), palette().color(QPalette::Base));

    if (!m_preview.valid)
    {
        p.setPen(palette().color(QPalette::Text));
        p.drawText(rect().adjusted(8, 8, -8, -8),
                   Qt::AlignCenter | Qt::TextWordWrap,
                   m_placeholder.isEmpty()
                           ? tr("Link Preset Table v2 (Fixture Group mode)")
                           : m_placeholder);
        return;
    }

    const int cols = qMax(1, m_preview.gridSize.width());
    const int rows = qMax(1, m_preview.gridSize.height());
    const QRect area = rect().adjusted(6, 6, -6, -28);
    const int cellW = qMax(24, area.width() / cols);
    const int cellH = qMax(24, area.height() / rows);
    const int totalW = cellW * cols;
    const int totalH = cellH * rows;
    const int ox = area.left() + (area.width() - totalW) / 2;
    const int oy = area.top() + (area.height() - totalH) / 2;

    const QFont baseFont = p.font();
    const QColor emptyBg = palette().color(QPalette::Base).darker(105);
    const QColor gridPen = palette().color(QPalette::Mid);
    const QColor textColor = palette().color(QPalette::Text);
    const QColor mutedText = palette().color(QPalette::Mid).darker(135);

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const QLCPoint pt(x, y);
            const PTSpatialGridCellData cell = m_preview.cells.value(pt);
            const QRect cr(ox + x * cellW, oy + y * cellH, cellW - 2, cellH - 2);

            const QColor fill = cell.occupied ? offsetFillColor(cell.headOffsetDeg) : emptyBg;
            p.fillRect(cr, fill);

            QPen border = QPen(gridPen, 1);
            p.setPen(border);
            p.drawRect(cr);

            if (!cell.occupied)
                continue;

            drawOutputBorder(p, cr, cell.outputIndexes);

            if (!m_selectionLayers.isEmpty() && m_selectionScopeCells.contains(pt))
            {
                for (const PTSpatialGridSelectionLayer& layer : m_selectionLayers)
                {
                    if (!layer.cells.contains(pt))
                        continue;
                    const bool activeLayer = layer.selectionIndex == m_activeSelectionIndex;
                    const QColor color = layer.color.isValid() ? layer.color : QColor(70, 210, 255);
                    QColor fillColor = color;
                    fillColor.setAlpha(activeLayer ? 85 : 55);
                    p.fillRect(cr.adjusted(4, 4, -4, -4), fillColor);
                    p.setPen(QPen(color, activeLayer ? 3 : 2));
                    p.drawRect(cr.adjusted(activeLayer ? 6 : 5,
                                           activeLayer ? 6 : 5,
                                           activeLayer ? -6 : -5,
                                           activeLayer ? -6 : -5));
                    break;
                }
            }

            if (cell.offsetCollision || !m_preview.offsetStepOk)
            {
                const QColor warn = cell.offsetCollision ? QColor(220, 60, 40) : QColor(220, 160, 40);
                p.setPen(QPen(warn, 2));
                p.drawRect(cr.adjusted(4, 4, -4, -4));
            }

            QFont smallFont = baseFont;
            smallFont.setPointSize(qMax(7, baseFont.pointSize() - 2));
            QFont numberFont = baseFont;
            numberFont.setBold(true);
            numberFont.setPointSize(qMax(9, baseFont.pointSize() + 1));

            p.setFont(smallFont);
            const QColor cellText = textColorForFill(fill);
            const QColor cellMuted = fill.lightness() < 135 ? QColor(215, 215, 215) : QColor(55, 55, 55);
            p.setPen(cellMuted);
            p.drawText(cr.adjusted(3, 0, -3, -3), Qt::AlignBottom | Qt::AlignHCenter,
                       QStringLiteral("%1°").arg(cell.headOffsetDeg));

            p.setFont(numberFont);
            p.setPen(cellText);
            p.drawText(cr.adjusted(2, 2, -2, -2), Qt::AlignCenter,
                       QString::number(cell.localOrder));

            if (cellW >= 44 && cellH >= 38)
            {
                p.setFont(smallFont);
                p.setPen(cellMuted);
                p.drawText(cr.adjusted(3, 0, -3, -3), Qt::AlignBottom | Qt::AlignRight,
                           QStringLiteral("%1%").arg(int(cell.phaseStart01 * 100.0 + 0.5)));
            }
            p.setFont(baseFont);
        }
    }

    p.setFont(baseFont);
    p.setPen(textColor);
    QString legend = tr("wings %1 · blocks %2 · slots/wing %3 · max step %4°")
            .arg(m_preview.wings)
            .arg(m_preview.blocks)
            .arg(m_preview.slotsPerWing)
            .arg(m_preview.maxOffsetStep);
    legend += tr("  · fill = offset shade · colored border = output");
    if (!m_selectionLayers.isEmpty())
        legend += tr("  · inner color = custom selection");
    if (m_preview.hasOffsetCollisions)
        legend += tr("  · offset collision");
    if (!m_preview.offsetStepOk)
        legend += tr("  · step too large");
    p.drawText(QRect(6, height() - 22, width() - 12, 18), Qt::AlignLeft, legend);
}

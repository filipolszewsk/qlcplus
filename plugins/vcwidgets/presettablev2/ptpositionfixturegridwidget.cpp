/*
  ptpositionfixturegridwidget.cpp
*/

#include "ptpositionfixturegridwidget.h"
#include "ptpositionconverter.h"

#include <QPainter>
#include <QMouseEvent>

PTPositionFixtureGridWidget::PTPositionFixtureGridWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(140, 140);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
}

void PTPositionFixtureGridWidget::setGrid(const QSize& gridSize,
                                          const QMap<QLCPoint, PTPositionGridCell>& cells)
{
    m_gridSize = gridSize;
    m_cells = cells;
    m_placeholder.clear();
    updateGeometry();
    update();
}

void PTPositionFixtureGridWidget::setPlaceholderText(const QString& text)
{
    m_gridSize = QSize();
    m_cells.clear();
    m_placeholder = text;
    update();
}

void PTPositionFixtureGridWidget::setSelectedCells(const QSet<QLCPoint>& cells)
{
    m_selectedCells = cells;
    update();
}

QRect PTPositionFixtureGridWidget::cellRect(const QLCPoint& pt) const
{
    if (!m_gridSize.isValid() || m_gridSize.width() <= 0 || m_gridSize.height() <= 0)
        return QRect();

    const int cols = m_gridSize.width();
    const int rows = m_gridSize.height();
    if (pt.x() < 0 || pt.y() < 0 || pt.x() >= cols || pt.y() >= rows)
        return QRect();

    const QRect area = rect().adjusted(6, 6, -6, -22);
    const int cellW = qMax(36, area.width() / cols);
    const int cellH = qMax(36, area.height() / rows);
    const int totalW = cellW * cols;
    const int totalH = cellH * rows;
    const int ox = area.left() + (area.width() - totalW) / 2;
    const int oy = area.top() + (area.height() - totalH) / 2;
    return QRect(ox + pt.x() * cellW, oy + pt.y() * cellH, cellW - 2, cellH - 2);
}

QLCPoint PTPositionFixtureGridWidget::pointAt(const QPoint& pos) const
{
    for (auto it = m_cells.constBegin(); it != m_cells.constEnd(); ++it)
    {
        if (cellRect(it.key()).contains(pos))
            return it.key();
    }
    return QLCPoint(-1, -1);
}

void PTPositionFixtureGridWidget::toggleSelection(const QLCPoint& pt, bool extend)
{
    if (!m_cells.contains(pt))
        return;

    if (!extend)
        m_selectedCells.clear();

    if (m_selectedCells.contains(pt))
        m_selectedCells.remove(pt);
    else
        m_selectedCells.insert(pt);

    emit selectionChanged(m_selectedCells);
    update();
}

void PTPositionFixtureGridWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_cells.isEmpty() || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    const QLCPoint pt = pointAt(event->pos());
    if (pt.x() < 0)
        return;

    const bool extend = event->modifiers().testFlag(Qt::ShiftModifier)
            || event->modifiers().testFlag(Qt::ControlModifier);
    toggleSelection(pt, extend);
}

void PTPositionFixtureGridWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), palette().color(QPalette::Base));

    if (m_cells.isEmpty())
    {
        p.setPen(palette().color(QPalette::Text));
        p.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignCenter | Qt::TextWordWrap,
                   m_placeholder.isEmpty()
                           ? tr("Select a fixture group in Properties")
                           : m_placeholder);
        return;
    }

    const QColor emptyBg = palette().color(QPalette::AlternateBase);
    const QColor storedBg = QColor(50, 120, 70);
    const QColor inheritedBg = QColor(70, 70, 70);
    const QColor selectedBorder = QColor(60, 140, 240);

    for (auto it = m_cells.constBegin(); it != m_cells.constEnd(); ++it)
    {
        const PTPositionGridCell& cell = it.value();
        const QRect cr = cellRect(it.key());
        if (!cr.isValid())
            continue;

        QColor fill = emptyBg;
        if (cell.position.valid)
            fill = cell.inherited ? inheritedBg : storedBg;
        p.fillRect(cr, fill);
        p.setPen(QPen(palette().color(QPalette::Mid), 1));
        p.drawRect(cr);

        if (m_selectedCells.contains(it.key()))
        {
            p.setPen(QPen(selectedBorder, 3));
            p.drawRect(cr.adjusted(1, 1, -1, -1));
        }

        QFont labelFont = p.font();
        labelFont.setBold(true);
        labelFont.setPointSize(qMax(8, labelFont.pointSize()));
        p.setFont(labelFont);
        p.setPen(palette().color(QPalette::Text));
        p.drawText(cr.adjusted(2, 2, -2, -14), Qt::AlignHCenter | Qt::AlignTop,
                   cell.label);

        QFont valFont = p.font();
        valFont.setBold(false);
        valFont.setPointSize(qMax(7, valFont.pointSize() - 1));
        p.setFont(valFont);
        const QString valText = cell.position.valid
                ? PTPositionConverter::formatPosition(cell.position, 0)
                : QStringLiteral("—");
        p.drawText(cr.adjusted(2, 0, -2, -2), Qt::AlignHCenter | Qt::AlignBottom, valText);
    }

    p.setFont(font());
    p.setPen(palette().color(QPalette::Mid));
    p.drawText(QRect(6, height() - 20, width() - 12, 16), Qt::AlignLeft,
               tr("Click = select · Shift/Ctrl+click = multi-select"));
}

QSize PTPositionFixtureGridWidget::sizeHint() const
{
    if (!m_gridSize.isValid())
        return QSize(200, 160);
    const int cw = qMin(56, qMax(40, 360 / qMax(1, m_gridSize.width())));
    const int ch = qMin(56, qMax(40, 280 / qMax(1, m_gridSize.height())));
    return QSize(qMax(160, m_gridSize.width() * cw + 16),
                 qMax(140, m_gridSize.height() * ch + 32));
}

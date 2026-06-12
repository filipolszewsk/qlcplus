/*
  ptpositionfixturegridwidget.cpp
*/

#include "ptpositionfixturegridwidget.h"
#include "ptpositionconverter.h"

#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>

#include <algorithm>

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

QList<QLCPoint> PTPositionFixtureGridWidget::rowMajorOrder(const QSet<QLCPoint>& cells)
{
    QList<QLCPoint> order;
    for (const QLCPoint& pt : cells)
        order.append(pt);
    std::sort(order.begin(), order.end(), [](const QLCPoint& a, const QLCPoint& b) {
        return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
    });
    return order;
}

void PTPositionFixtureGridWidget::setImplicitAllSelection(bool implicitAll)
{
    if (m_implicitAllSelected == implicitAll)
        return;
    m_implicitAllSelected = implicitAll;
    update();
}

void PTPositionFixtureGridWidget::setEditableCells(const QSet<QLCPoint>& cells)
{
    m_editableCells = cells;
    update();
}

void PTPositionFixtureGridWidget::setForeignSelectionLayers(
        const QVector<PTPositionGridSelectionLayer>& layers)
{
    m_foreignSelectionLayers = layers;
    update();
}

bool PTPositionFixtureGridWidget::isEditableCell(const QLCPoint& pt) const
{
    if (!m_cells.contains(pt))
        return false;
    if (m_editableCells.isEmpty())
        return true;
    return m_editableCells.contains(pt);
}

void PTPositionFixtureGridWidget::setSelectedCells(const QSet<QLCPoint>& cells,
                                                   const QList<QLCPoint>& order)
{
    m_selectedCells = cells;
    m_selectionOrder.clear();
    for (const QLCPoint& pt : order)
    {
        if (cells.contains(pt))
            m_selectionOrder.append(pt);
    }
    for (const QLCPoint& pt : cells)
    {
        if (!m_selectionOrder.contains(pt))
            m_selectionOrder.append(pt);
    }
    if (!cells.isEmpty())
        m_selectionAnchor = m_selectionOrder.isEmpty() ? *cells.constBegin() : m_selectionOrder.first();
    else
        m_selectionAnchor = QLCPoint(-1, -1);
    update();
}

void PTPositionFixtureGridWidget::emitSelectionChanged()
{
    emit selectionChanged(m_selectedCells, m_selectionOrder);
}

bool PTPositionFixtureGridWidget::hasAnchor() const
{
    return m_selectionAnchor.x() >= 0 && isEditableCell(m_selectionAnchor);
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

void PTPositionFixtureGridWidget::selectSingleCell(const QLCPoint& pt)
{
    if (!isEditableCell(pt))
        return;

    m_selectedCells.clear();
    m_selectedCells.insert(pt);
    m_selectionOrder = QList<QLCPoint>() << pt;
    m_selectionAnchor = pt;
    emitSelectionChanged();
    update();
}

void PTPositionFixtureGridWidget::selectRectRange(const QLCPoint& from, const QLCPoint& to)
{
    if (!isEditableCell(from) || !isEditableCell(to))
        return;

    const int x0 = qMin(from.x(), to.x());
    const int x1 = qMax(from.x(), to.x());
    const int y0 = qMin(from.y(), to.y());
    const int y1 = qMax(from.y(), to.y());

    m_selectedCells.clear();
    m_selectionOrder.clear();
    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            const QLCPoint pt(x, y);
            if (!isEditableCell(pt))
                continue;
            m_selectedCells.insert(pt);
            m_selectionOrder.append(pt);
        }
    }

    emitSelectionChanged();
    update();
}

void PTPositionFixtureGridWidget::toggleSelectionCell(const QLCPoint& pt)
{
    if (!isEditableCell(pt))
        return;

    if (m_selectedCells.contains(pt))
    {
        m_selectedCells.remove(pt);
        m_selectionOrder.removeAll(pt);
    }
    else
    {
        m_selectedCells.insert(pt);
        m_selectionOrder.append(pt);
    }

    emitSelectionChanged();
    update();
}

void PTPositionFixtureGridWidget::mousePressEvent(QMouseEvent* event)
{
    setFocus(Qt::MouseFocusReason);

    if (m_cells.isEmpty() || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    const QLCPoint pt = pointAt(event->pos());
    if (pt.x() < 0 || !isEditableCell(pt))
    {
        if (pt.x() >= 0)
            return;
        if (!m_selectedCells.isEmpty())
        {
            m_selectedCells.clear();
            m_selectionOrder.clear();
            m_selectionAnchor = QLCPoint(-1, -1);
            emitSelectionChanged();
            update();
        }
        return;
    }

    if (event->modifiers().testFlag(Qt::ShiftModifier))
    {
        if (!hasAnchor())
            m_selectionAnchor = pt;
        selectRectRange(m_selectionAnchor, pt);
    }
    else if (event->modifiers().testFlag(Qt::ControlModifier)
             || event->modifiers().testFlag(Qt::MetaModifier))
    {
        toggleSelectionCell(pt);
    }
    else
    {
        selectSingleCell(pt);
    }
}

void PTPositionFixtureGridWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    setFocus(Qt::MouseFocusReason);

    if (m_cells.isEmpty() || event->button() != Qt::LeftButton)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const QLCPoint pt = pointAt(event->pos());
    if (pt.x() < 0 || !isEditableCell(pt))
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    if (!m_selectedCells.contains(pt))
        selectSingleCell(pt);

    emit cellEditRequested(pt);
}

void PTPositionFixtureGridWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy))
    {
        emit copyRequested();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Paste))
    {
        emit pasteRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        emit clearRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void PTPositionFixtureGridWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_cells.isEmpty())
    {
        QWidget::contextMenuEvent(event);
        return;
    }

    const QLCPoint pt = pointAt(event->pos());
    if (pt.x() >= 0 && isEditableCell(pt) && !m_selectedCells.contains(pt))
        selectSingleCell(pt);

    QMenu menu(this);
    QAction* copyAct = menu.addAction(tr("Copy cells"));
    QAction* pasteAct = menu.addAction(tr("Paste cells"));
    QAction* clearAct = menu.addAction(tr("Clear cells"));
    copyAct->setShortcut(QKeySequence::Copy);
    pasteAct->setShortcut(QKeySequence::Paste);
    clearAct->setShortcut(QKeySequence::Delete);

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == copyAct)
        emit copyRequested();
    else if (chosen == pasteAct)
        emit pasteRequested();
    else if (chosen == clearAct)
        emit clearRequested();
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
    const QColor disabledBg = palette().color(QPalette::Mid).lighter(130);
    const QColor storedBg = QColor(50, 120, 70);
    const QColor inheritedBg = QColor(70, 70, 70);
    const QColor selectedBorder = QColor(60, 140, 240);

    for (auto it = m_cells.constBegin(); it != m_cells.constEnd(); ++it)
    {
        const PTPositionGridCell& cell = it.value();
        const QRect cr = cellRect(it.key());
        if (!cr.isValid())
            continue;

        const bool editable = isEditableCell(it.key());
        QColor fill = emptyBg;
        if (!editable)
            fill = disabledBg;
        else if (cell.position.valid)
            fill = cell.inherited ? inheritedBg : storedBg;
        p.fillRect(cr, fill);
        p.setPen(QPen(palette().color(QPalette::Mid), 1));
        p.drawRect(cr);

        for (const PTPositionGridSelectionLayer& layer : m_foreignSelectionLayers)
        {
            if (!layer.cells.contains(it.key()))
                continue;
            p.setPen(QPen(layer.color, 2));
            p.drawRect(cr.adjusted(3, 3, -3, -3));
            break;
        }

        const bool selected = editable
                && (m_selectedCells.contains(it.key())
                    || (m_implicitAllSelected && m_selectedCells.isEmpty()));
        if (selected)
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
               tr("No selection = all fixtures · Click = one · Shift = range · "
                  "Ctrl = toggle · click empty = all · green = stored · grey = inherited"));
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

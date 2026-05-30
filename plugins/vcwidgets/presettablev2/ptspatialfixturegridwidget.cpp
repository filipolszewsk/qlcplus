/*
  ptspatialfixturegridwidget.cpp
*/

#include "ptspatialfixturegridwidget.h"

#include <QPainter>
#include <QPaintEvent>

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

    const int maxOrder = qMax(1, m_preview.cells.size());

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const QLCPoint pt(x, y);
            const PTSpatialGridCellData cell = m_preview.cells.value(pt);
            const QRect cr(ox + x * cellW, oy + y * cellH, cellW - 2, cellH - 2);

            QColor bg = palette().color(QPalette::Mid).lighter(140);
            if (cell.occupied)
            {
                const float hue = 240.0f - 240.0f * float(cell.chaseOrder) / float(maxOrder);
                bg = QColor::fromHsv(int(hue) % 360, 160, 200);
            }
            p.fillRect(cr, bg);

            QPen border = QPen(palette().color(QPalette::Dark), 1);
            if (cell.offsetCollision)
                border = QPen(QColor(220, 60, 40), 2);
            else if (!m_preview.offsetStepOk && cell.occupied)
                border = QPen(QColor(220, 160, 40), 2);
            p.setPen(border);
            p.drawRect(cr);

            if (!cell.occupied)
                continue;

            p.setPen(palette().color(QPalette::Text));
            p.drawText(cr.adjusted(2, 2, -2, -2), Qt::AlignTop | Qt::AlignHCenter,
                       QStringLiteral("#%1").arg(cell.chaseOrder));
            p.setFont(QFont(p.font().family(), qMax(7, p.font().pointSize() - 2)));
            p.drawText(cr.adjusted(2, 0, -2, -2), Qt::AlignBottom | Qt::AlignHCenter,
                       QStringLiteral("%1°").arg(cell.headOffsetDeg));
            p.setFont(QFont());
            p.drawText(cr, Qt::AlignCenter,
                       QStringLiteral("%1%").arg(int(cell.phaseStart01 * 100.0 + 0.5)));
        }
    }

    p.setPen(palette().color(QPalette::Text));
    QString legend = tr("Order · offset° · phase%  |  slots %1  max step %2°")
            .arg(m_preview.effectiveOffsetSlots)
            .arg(m_preview.maxOffsetStep);
    if (m_preview.hasOffsetCollisions)
        legend += tr("  · duplicate offsets!");
    if (!m_preview.offsetStepOk)
        legend += tr("  · step too large");
    p.drawText(QRect(6, height() - 22, width() - 12, 18), Qt::AlignLeft, legend);
}

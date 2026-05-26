/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutitemdelegate.cpp — Apache 2.0 / public domain
*/

#include "fixturegrouplayoutitemdelegate.h"
#include "fixturegrouplayoutwidget.h"

#include "qlcpoint.h"

#include <QPainter>
#include <QTableWidget>
#include <QAbstractItemView>

MaskGridItemDelegate::MaskGridItemDelegate(FixtureGroupLayoutWidget* owner, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_owner(owner)
{
}

void MaskGridItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    if (m_owner == nullptr || !index.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int row = index.row();
    const int col = index.column();
    const QLCPoint pt(col, row);
    const MaskDisplayColors colors = m_owner->maskDisplayColors();
    const bool visible = !m_owner->isPointMaskedOut(pt);

    painter->save();
    painter->fillRect(option.rect, visible ? colors.visible : colors.hidden);

    const QString text = index.data(Qt::DisplayRole).toString();
    const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

    const int iconSize = 16;
    const int iconPad = 2;
    int textLeft = 4;

    if (!icon.isNull())
    {
        const QPixmap pm = icon.pixmap(iconSize, iconSize);
        const int y = option.rect.top() + (option.rect.height() - pm.height()) / 2;
        painter->drawPixmap(option.rect.left() + 2, y, pm);
        textLeft = 2 + iconSize + iconPad + 2;
    }

    painter->setPen(visible ? option.palette.color(QPalette::Text) : colors.hiddenText);
    painter->drawText(option.rect.adjusted(textLeft, 0, -2, 0),
                      Qt::AlignVCenter | Qt::AlignLeft, text);

    if (m_owner->isGridCellSelected(row, col))
    {
        QPen borderPen(colors.selectionBorder, 2);
        painter->setPen(borderPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(option.rect.adjusted(1, 1, -2, -2));
    }

    painter->restore();
}

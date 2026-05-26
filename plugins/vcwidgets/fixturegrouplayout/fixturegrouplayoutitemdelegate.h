/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutitemdelegate.h — Apache 2.0 / public domain
*/

#pragma once

#include <QStyledItemDelegate>

class FixtureGroupLayoutWidget;

class MaskGridItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit MaskGridItemDelegate(FixtureGroupLayoutWidget* owner, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    FixtureGroupLayoutWidget* m_owner;
};

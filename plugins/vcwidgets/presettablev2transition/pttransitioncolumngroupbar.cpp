/*
  pttransitioncolumngroupbar.cpp
*/

#include "pttransitioncolumngroupbar.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QToolButton>

PTTransitionColumnGroupBar::PTTransitionColumnGroupBar(QWidget* parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
{
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::buttonClicked,
            this, &PTTransitionColumnGroupBar::onButtonClicked);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 4);
    layout->setSpacing(4);
}

void PTTransitionColumnGroupBar::onButtonClicked(QAbstractButton* btn)
{
    if (!btn)
        return;
    m_activeId = btn->property("groupId").toString();
    emit activeGroupChanged(m_activeId);
}

void PTTransitionColumnGroupBar::setGroups(const QVector<Group>& groups)
{
    QSignalBlocker blocker(m_group);
    while (m_group->buttons().size() > 0)
    {
        if (QAbstractButton* btn = m_group->buttons().first())
        {
            m_group->removeButton(btn);
            btn->deleteLater();
        }
    }

    QHBoxLayout* hLayout = qobject_cast<QHBoxLayout*>(layout());
    if (!hLayout)
        return;

    while (hLayout->count() > 0)
    {
        if (QLayoutItem* item = hLayout->takeAt(0))
        {
            if (QWidget* w = item->widget())
                w->deleteLater();
            delete item;
        }
    }

    auto addButton = [&](const QString& id, const QString& label, bool checked) {
        auto* btn = new QToolButton(this);
        btn->setText(label);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setAutoRaise(true);
        btn->setProperty("groupId", id);
        m_group->addButton(btn);
        hLayout->addWidget(btn);
    };

    addButton(QString(), tr("All"), m_activeId.isEmpty());
    for (const Group& group : groups)
        addButton(group.id, group.label, m_activeId == group.id);
    hLayout->addStretch(1);
}

QString PTTransitionColumnGroupBar::activeGroupId() const
{
    return m_activeId;
}

void PTTransitionColumnGroupBar::setActiveGroupId(const QString& id)
{
    m_activeId = id;
    for (QAbstractButton* btn : m_group->buttons())
        btn->setChecked(btn->property("groupId").toString() == id);
}

/*
  pttransitioncolumngroupbar.h — clickable column section filter for EFX preset tables
*/

#pragma once

#include "presettablev2effectengine.h"

#include <QWidget>

class QAbstractButton;
class QButtonGroup;

class PTTransitionColumnGroupBar : public QWidget
{
    Q_OBJECT

public:
    struct Group
    {
        QString id;
        QString label;
        QVector<int> columns;
    };

    explicit PTTransitionColumnGroupBar(QWidget* parent = nullptr);

    void setAllButtonVisible(bool visible);
    void setAllButtonLabel(const QString& label);
    void setGroups(const QVector<Group>& groups);
    QString activeGroupId() const;
    void setActiveGroupId(const QString& id);

signals:
    void activeGroupChanged(const QString& groupId);

private slots:
    void onButtonClicked(QAbstractButton* btn);

private:
    QButtonGroup* m_group = nullptr;
    QString m_activeId;
    QString m_allButtonLabel;
    bool m_allButtonVisible = true;
};

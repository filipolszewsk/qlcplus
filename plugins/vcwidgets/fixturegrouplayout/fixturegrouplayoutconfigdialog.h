/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>

class Doc;
class QComboBox;
class QDialogButtonBox;

class FixtureGroupLayoutConfigDialog : public QDialog
{
    Q_OBJECT

public:
    FixtureGroupLayoutConfigDialog(Doc* doc, quint32 currentGroupId, QWidget* parent = nullptr);

    quint32 fixtureGroupId() const;

private:
    void populateGroups();

    Doc* m_doc;
    quint32 m_initGroupId;
    QComboBox* m_groupCombo;
    QDialogButtonBox* m_buttons;
};

/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QSharedPointer>

#include "fixturegrouplayoutwidget.h"

class Doc;
class QComboBox;
class QDialogButtonBox;
class InputSelectionWidget;
class QLCInputSource;
class QPushButton;

class FixtureGroupLayoutConfigDialog : public QDialog
{
    Q_OBJECT

public:
    FixtureGroupLayoutConfigDialog(Doc* doc,
                                   quint32 currentGroupId,
                                   const MaskDisplayColors& maskColors,
                                   MaskEfxConflictPolicy efxConflictPolicy,
                                   QSharedPointer<QLCInputSource> presetSelectSrc,
                                   int widgetPage,
                                   QWidget* parent = nullptr);

    quint32 fixtureGroupId() const;
    MaskDisplayColors maskDisplayColors() const;
    MaskEfxConflictPolicy maskEfxConflictPolicy() const;
    QSharedPointer<QLCInputSource> presetSelectInputSource() const;

private slots:
    void slotPickVisibleColor();
    void slotPickHiddenColor();
    void slotPickHiddenTextColor();
    void slotPickHeaderColor();
    void slotPickSelectionBorderColor();

private:
    void populateGroups();
    void updateColorButton(QPushButton* button, const QColor& color);

    Doc* m_doc;
    quint32 m_initGroupId;
    MaskDisplayColors m_maskColors;
    MaskEfxConflictPolicy m_maskEfxConflictPolicy;
    QComboBox* m_groupCombo;
    QComboBox* m_efxConflictCombo = nullptr;
    InputSelectionWidget* m_presetSelectInputSel = nullptr;
    QPushButton* m_visibleColorBtn = nullptr;
    QPushButton* m_hiddenColorBtn = nullptr;
    QPushButton* m_hiddenTextColorBtn = nullptr;
    QPushButton* m_headerColorBtn = nullptr;
    QPushButton* m_selectionBorderBtn = nullptr;
    QDialogButtonBox* m_buttons;
};

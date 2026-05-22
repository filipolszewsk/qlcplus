/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSharedPointer>
#include <QList>
#include <QStringList>

#include "qlcinputsource.h"

class Doc;
class InputSelectionWidget;

class MultiButtonConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MultiButtonConfigDialog(
        Doc*                           doc,
        const QList<quint32>&          funcIds,
        const QStringList&             funcLabels,
        const QStringList&             iconPaths,
        int                            longPressMs,
        bool                           addOffAtEnd,
        bool                           monitorChannelValues,
        QSharedPointer<QLCInputSource> triggerSrc,
        QSharedPointer<QLCInputSource> popupSrc,
        int                            widgetPage,
        QWidget*                       parent = nullptr);

    // ---- Results --------------------------------------------------------
    QList<quint32>                 functionIds()           const;
    QStringList                    functionLabels()        const;
    QStringList                    iconPaths()             const;
    int                            longPressMs()           const;
    bool                           addOffAtEnd()           const;
    bool                           monitorChannelValues()  const;
    QSharedPointer<QLCInputSource> triggerInputSource()    const;
    QSharedPointer<QLCInputSource> popupInputSource()      const;

private slots:
    void slotAdd();
    void slotRemove();
    void slotEditLabel();
    void slotScribbleIcon();
    void slotChooseIcon();
    void slotClearIcon();
    void slotMoveUp();
    void slotMoveDown();
    void slotSelectionChanged();

private:
    void rebuildList();

    Doc* m_doc;

    // ---- Function list data (parallel) ----------------------------------
    QList<quint32> m_ids;
    QStringList    m_labels;
    QStringList    m_icons;   // parallel icon paths

    // ---- UI: function list panel ----------------------------------------
    QListWidget*  m_listWidget    = nullptr;
    QPushButton*  m_addBtn        = nullptr;
    QPushButton*  m_removeBtn     = nullptr;
    QPushButton*  m_editLblBtn    = nullptr;
    QPushButton*  m_scribbleBtn   = nullptr;
    QPushButton*  m_chooseIconBtn = nullptr;
    QPushButton*  m_clearIconBtn  = nullptr;
    QPushButton*  m_upBtn         = nullptr;
    QPushButton*  m_downBtn       = nullptr;

    // ---- UI: behavior ---------------------------------------------------
    QSpinBox*     m_longPressSpin     = nullptr;
    QCheckBox*    m_offAtEndCheck     = nullptr;
    QCheckBox*    m_monitorCheck      = nullptr;

    // ---- UI: input selection --------------------------------------------
    InputSelectionWidget* m_triggerInputSel = nullptr;
    InputSelectionWidget* m_popupInputSel   = nullptr;

    QDialogButtonBox* m_buttons = nullptr;
};

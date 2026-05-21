/*
  QLC+ VC Widget Plugin — Preset Table
  presettablecolumndialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>

#include "presettablewidget.h"

class PresetTableColumnDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetTableColumnDialog(const PTColumn& column, QWidget* parent = nullptr);

    PTColumn column() const;

private slots:
    void slotTypeChanged();
    void slotAddOption();
    void slotRemoveOption();

private:
    void updateOptionsEnabled();

    QLineEdit*        m_nameEdit    = nullptr;
    QRadioButton*     m_rbNumeric   = nullptr;
    QRadioButton*     m_rbDropdown  = nullptr;
    QTableWidget*     m_optTable    = nullptr;
    QPushButton*      m_addOptBtn   = nullptr;
    QPushButton*      m_remOptBtn   = nullptr;
    QRadioButton*     m_rbFade      = nullptr;
    QRadioButton*     m_rbSnap      = nullptr;
    QDialogButtonBox* m_buttons     = nullptr;
};

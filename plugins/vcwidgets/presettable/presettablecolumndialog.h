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

class Doc;

class PresetTableColumnDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetTableColumnDialog(Doc* doc, const PTColumn& column, QWidget* parent = nullptr);

    PTColumn column() const;

private slots:
    void slotTypeChanged();
    void slotAddOption();
    void slotRemoveOption();
    void slotImportFromChannel();

private:
    void updateOptionsEnabled();
    static QIcon makeResourceIcon(const QString& resource);

    Doc*              m_doc         = nullptr;
    QLineEdit*        m_nameEdit    = nullptr;
    QRadioButton*     m_rbNumeric   = nullptr;
    QRadioButton*     m_rbDropdown  = nullptr;
    QTableWidget*     m_optTable    = nullptr;
    QPushButton*      m_addOptBtn   = nullptr;
    QPushButton*      m_remOptBtn   = nullptr;
    QPushButton*      m_importBtn   = nullptr;
    QRadioButton*     m_rbFade      = nullptr;
    QRadioButton*     m_rbSnap      = nullptr;
    QDialogButtonBox* m_buttons     = nullptr;
};

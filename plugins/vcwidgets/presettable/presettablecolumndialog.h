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
#include <QComboBox>
#include <QLabel>

#include "presettablewidget.h"

class Doc;
class FixtureGroup;

class PresetTableColumnDialog : public QDialog
{
    Q_OBJECT

public:
    // mode + group are used to show the Fixture Binding section (FG mode only).
    // Pass PTMode::Legacy and nullptr for legacy columns.
    explicit PresetTableColumnDialog(Doc* doc,
                                     const PTColumn& column,
                                     PTMode mode,
                                     FixtureGroup* group,
                                     QWidget* parent = nullptr);

    PTColumn column() const;

private slots:
    void slotTypeChanged();
    void slotAddOption();
    void slotRemoveOption();
    void slotImportFromChannel();
    void slotFixtureTypeChanged(int index);

private:
    void updateOptionsEnabled();
    void populateFixtureTypeCombo();
    void populateChannelCombo(int fixtureTypeIndex);
    static QIcon makeResourceIcon(const QString& resource);

    Doc*          m_doc   = nullptr;
    PTMode        m_mode;
    FixtureGroup* m_group = nullptr;

    // Binding section (FG mode only)
    QGroupBox*    m_bindGrp       = nullptr;
    QComboBox*    m_fxTypeCombo   = nullptr;
    QLabel*       m_fxTypeLabel   = nullptr;
    QComboBox*    m_channelCombo  = nullptr;
    QLabel*       m_channelLabel  = nullptr;

    // Binding data: one entry per combo index
    struct FxTypeEntry {
        QString manufacturer;
        QString model;
        QString modeName;
        int     count = 0;   // number of fixtures of this type in the group
    };
    QList<FxTypeEntry> m_fxTypeEntries;

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

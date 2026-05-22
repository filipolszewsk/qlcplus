/*
  QLC+ VC Widget Plugin — Preset Table
  presettableconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QSharedPointer>
#include <QVector>
#include <QList>

#include "presettablewidget.h"
#include "qlcinputsource.h"

class Doc;
class FixtureGroup;
class InputSelectionWidget;
class QLineEdit;

// ---- Per-output editor row: Legacy mode (single fixture) -------------------

class OutputEditorRow : public QWidget
{
    Q_OBJECT
public:
    explicit OutputEditorRow(Doc* doc,
                             const PTOutput& output,
                             QSharedPointer<QLCInputSource> src,
                             int requiredChannels,
                             int widgetPage,
                             QWidget* parent = nullptr);

    PTOutput output() const;
    QSharedPointer<QLCInputSource> inputSource() const;

signals:
    void changed();
    void fixtureSelected(quint32 fixtureId);

public:
    void setRequiredChannels(int n);

private slots:
    void slotChooseFixture();
    void slotNameEdited(const QString& text);

private:
    void updateFixtureLabel();

    Doc*    m_doc;
    int     m_requiredChannels;

    QLineEdit*            m_nameEdit        = nullptr;
    QLabel*               m_fixtureLabel    = nullptr;
    QPushButton*          m_chooseFixBtn    = nullptr;
    InputSelectionWidget* m_inputSel        = nullptr;

    quint32 m_fixtureId = UINT_MAX;
};

// ---- Per-output editor row: FixtureGroup mode (row checkboxes) -------------

class FGOutputEditorRow : public QWidget
{
    Q_OBJECT
public:
    explicit FGOutputEditorRow(Doc* doc,
                               const PTOutput& output,
                               QSharedPointer<QLCInputSource> src,
                               FixtureGroup* group,
                               int widgetPage,
                               QWidget* parent = nullptr);

    PTOutput output() const;
    QSharedPointer<QLCInputSource> inputSource() const;

    void setFixtureGroup(FixtureGroup* group);

signals:
    void changed();

private:
    void rebuildRowCheckboxes();

    Doc*          m_doc   = nullptr;
    FixtureGroup* m_group = nullptr;

    QLineEdit*            m_nameEdit   = nullptr;
    QWidget*              m_cbWidget   = nullptr;  // scrollable container for checkboxes
    QVBoxLayout*          m_cbLayout   = nullptr;
    QList<QCheckBox*>     m_rowCBs;                // one per y-row in the group
    InputSelectionWidget* m_inputSel   = nullptr;
};

// ---- Main dialog ------------------------------------------------------------

class PresetTableConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetTableConfigDialog(Doc* doc,
                                     const QVector<PTColumn>& columns,
                                     const QVector<PTOutput>&  outputs,
                                     const QVector<QSharedPointer<QLCInputSource>>& sources,
                                     bool crossfadeEnabled,
                                     QSharedPointer<QLCInputSource> crossfadeSrc,
                                     int widgetPage,
                                     PTMode mode,
                                     quint32 fixtureGroupId,
                                     QWidget* parent = nullptr);

    QVector<PTColumn> columns() const { return m_columns; }
    QVector<PTOutput>  outputs() const;
    QSharedPointer<QLCInputSource> inputSource(int outputIdx) const;
    bool crossfadeEnabled() const;
    QSharedPointer<QLCInputSource> crossfadeInputSource() const;

    PTMode   widgetMode()            const;
    quint32  selectedFixtureGroupId() const;

private slots:
    void slotAddOutput();
    void slotRemoveOutput();
    void slotEditColumn();
    void slotValidate();
    void slotAutoCreateColumns(quint32 fixtureId);
    void slotModeComboChanged(int index);
    void slotGroupComboChanged(int index);

    // Column table slots
    void slotColTableCellChanged(int row, int col);
    void slotColTableItemChanged(QTableWidgetItem* item);
    void slotColumnsAddBlank();
    void slotColumnsRemoveSelected();
    void slotColumnsAddFromChannels();

private:
    void rebuildOutputList();
    void rebuildGroupCombo();
    void updateHintLabel();
    void rebuildColumnTable();
    void updateColumnButtons();
    QString bindingSummary(const PTColumn& col) const;
    FixtureGroup* currentFixtureGroup() const;

    Doc*   m_doc;
    int    m_widgetPage;

    QVector<PTColumn>   m_columns;
    bool                m_updatingColTable = false;

    // Mode selection
    QComboBox*  m_modeCombo      = nullptr;
    QWidget*    m_groupRow       = nullptr;
    QComboBox*  m_groupCombo     = nullptr;

    // Tab: Outputs
    QListWidget*   m_outputList     = nullptr;
    QPushButton*   m_addOutBtn      = nullptr;
    QPushButton*   m_remOutBtn      = nullptr;
    QLabel*        m_outHintLabel   = nullptr;

    // Legacy rows
    QList<OutputEditorRow*>   m_outputRows;
    // FixtureGroup rows
    QList<FGOutputEditorRow*> m_fgOutputRows;

    // Tab: Columns
    QTableWidget*   m_colTable           = nullptr;
    QPushButton*    m_editColBtn         = nullptr;
    QPushButton*    m_addColBtn          = nullptr;
    QPushButton*    m_remColsBtn         = nullptr;
    QPushButton*    m_addChFromGrpBtn    = nullptr;

    QDialogButtonBox* m_buttons      = nullptr;
    QLabel*           m_errorLabel   = nullptr;

    QVector<QSharedPointer<QLCInputSource>> m_initSources;

    // Crossfade section (widget-level)
    QCheckBox*            m_crossfadeChk      = nullptr;
    InputSelectionWidget* m_xfadeInputSel     = nullptr;
    QWidget*              m_xfadeInputWidget  = nullptr;
};

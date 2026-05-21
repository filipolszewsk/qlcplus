/*
  QLC+ VC Widget Plugin — Preset Table
  presettableconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QSharedPointer>
#include <QVector>

#include "presettablewidget.h"
#include "qlcinputsource.h"

class Doc;
class InputSelectionWidget;

// ---- Per-output editor row (embedded in a QWidget inside the list) ----------

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
    void fixtureSelected(quint32 fixtureId);   // emitted after user picks a fixture

public:
    void setRequiredChannels(int n);            // called by dialog when columns are auto-created

private slots:
    void slotChooseFixture();
    void slotNameEdited(const QString& text);

private:
    void updateFixtureLabel();

    Doc*    m_doc;
    int     m_requiredChannels;

    QLabel*               m_nameLabel       = nullptr;
    QLineEdit*            m_nameEdit        = nullptr;
    QLabel*               m_fixtureLabel    = nullptr;
    QPushButton*          m_chooseFixBtn    = nullptr;
    InputSelectionWidget* m_inputSel        = nullptr;

    quint32 m_fixtureId = UINT_MAX;
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
                                     QWidget* parent = nullptr);

    QVector<PTColumn> columns() const { return m_columns; }
    QVector<PTOutput>  outputs() const;
    QSharedPointer<QLCInputSource> inputSource(int outputIdx) const;
    bool crossfadeEnabled() const;
    QSharedPointer<QLCInputSource> crossfadeInputSource() const;

private slots:
    void slotAddOutput();
    void slotRemoveOutput();
    void slotEditColumn();
    void slotValidate();
    void slotAutoCreateColumns(quint32 fixtureId);

private:
    void rebuildOutputList();

    Doc*   m_doc;
    int    m_widgetPage;

    QVector<PTColumn>   m_columns;   // may be modified if user edits columns tab

    // Tab: Outputs
    QListWidget*    m_outputList     = nullptr;
    QList<OutputEditorRow*> m_outputRows;
    QPushButton*    m_addOutBtn      = nullptr;
    QPushButton*    m_remOutBtn      = nullptr;
    QLabel*         m_outHintLabel   = nullptr;

    // Tab: Columns
    QListWidget*    m_colList        = nullptr;
    QPushButton*    m_editColBtn     = nullptr;

    QDialogButtonBox* m_buttons      = nullptr;
    QLabel*           m_errorLabel   = nullptr;

    QVector<QSharedPointer<QLCInputSource>> m_initSources;

    // Crossfade section (widget-level)
    QCheckBox*            m_crossfadeChk      = nullptr;
    InputSelectionWidget* m_xfadeInputSel     = nullptr;
    QWidget*              m_xfadeInputWidget  = nullptr;  // container shown/hidden
};

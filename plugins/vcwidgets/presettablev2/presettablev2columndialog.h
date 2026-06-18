/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2columndialog.h — Apache 2.0 / public domain
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
#include <QSpinBox>
#include <QCheckBox>

#include "presettablev2widget.h"

class Doc;
class FixtureGroup;
class QLCFixtureMode;
class InputSelectionWidget;

class PresetTableV2ColumnDialog : public QDialog
{
    Q_OBJECT

public:
    // mode + group are used to show the Fixture Binding section (FG mode only).
    // Pass PTMode::Legacy and nullptr for legacy columns.
    explicit PresetTableV2ColumnDialog(Doc* doc,
                                     const PTColumn& column,
                                     PTMode mode,
                                     FixtureGroup* group,
                                     const QVector<PTOutput>& outputs = QVector<PTOutput>(),
                                     int widgetPage = 0,
                                     QWidget* parent = nullptr);

    PTColumn column() const;

private slots:
    void slotTypeChanged();
    void slotAddOption();
    void slotRemoveOption();
    void slotImportFromChannel();
    void slotAddBindings();
    void slotRemoveBindings();

private:
    void updateOptionsEnabled();
    void populateFixtureTypeEntries();
    void rebuildBindTable();
    void autoImportFromBinding(bool onlyIfEmpty);
    static QIcon makeResourceIcon(const QString& resource);

    QString bindingTypeLabel(const PTColumnTypeBinding& binding) const;
    QString bindingChannelLabel(const PTColumnTypeBinding& binding) const;
    QLCFixtureMode* representativeMode(const QString& mfg, const QString& model,
                                        const QString& modeName) const;
    bool bindingsContain(const PTColumnTypeBinding& binding) const;

    Doc*          m_doc   = nullptr;
    PTMode        m_mode;
    FixtureGroup* m_group = nullptr;
    QVector<PTOutput> m_outputs;
    int           m_widgetPage = 0;

    // Binding section (FG mode only)
    QGroupBox*    m_bindGrp     = nullptr;
    QTableWidget* m_bindTable   = nullptr;
    QPushButton*  m_addBindBtn  = nullptr;
    QPushButton*  m_remBindBtn  = nullptr;
    QVector<PTColumnTypeBinding> m_bindings;

    struct FxTypeEntry {
        QString manufacturer;
        QString model;
        QString modeName;
        int     count = 0;
    };
    QList<FxTypeEntry> m_fxTypeEntries;

    QLineEdit*        m_nameEdit      = nullptr;
    QCheckBox*        m_useFor1DFxChk = nullptr;
    QGroupBox*        m_intensityGrp  = nullptr;
    QVector<InputSelectionWidget*> m_intensityInputSels;
    QRadioButton*     m_rbNumeric     = nullptr;
    QRadioButton*     m_rbDropdown    = nullptr;
    QRadioButton*     m_rbScaler      = nullptr;
    QTableWidget*     m_optTable      = nullptr;
    QPushButton*      m_addOptBtn     = nullptr;
    QPushButton*      m_remOptBtn     = nullptr;
    QPushButton*      m_importBtn     = nullptr;
    QGroupBox*        m_scalerGrp     = nullptr;
    QSpinBox*         m_scalerMin     = nullptr;
    QSpinBox*         m_scalerMax     = nullptr;
    QLineEdit*        m_scalerSuffix  = nullptr;
    QRadioButton*     m_rbFade        = nullptr;
    QRadioButton*     m_rbSnap        = nullptr;
    QDialogButtonBox* m_buttons       = nullptr;
};

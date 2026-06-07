/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSharedPointer>
#include <QList>
#include <QStringList>
#include <QHash>
#include <functional>

#include "qlcinputsource.h"
#include "multibuttonwidget.h"

class Doc;
class InputSelectionWidget;
class PresetTableV2MultiButtonTargetIface;

class MultiButtonConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MultiButtonConfigDialog(
        Doc*                               doc,
        MultiButtonMode                    widgetMode,
        const QString&                     targetListName,
        const QList<quint32>&              funcIds,
        const QStringList&                 funcLabels,
        const QStringList&                 iconPaths,
        const QList<LevelChannelBinding>&  levelChannelBindings,
        const QList<LevelPreset>&          levelPresets,
        const QList<LevelPreset>&          widgetEntryAppearance,
        int                                longPressMs,
        bool                               addOffAtEnd,
        bool                               monitorChannelValues,
        bool                               receiveInputOnInactiveFramePage,
        MultiButtonLayout                  widgetLayout,
        int                                spreadColumns,
        int                                spreadRows,
        int                                spreadHMargin,
        int                                spreadVMargin,
        int                                spreadTileWidth,
        int                                spreadTileHeight,
        int                                spreadPages,
        bool                               automationEnabled,
        const QList<MultiButtonAutomationProfile>& automationProfiles,
        int                                activeAutomationProfile,
        QSharedPointer<QLCInputSource>     triggerSrc,
        QSharedPointer<QLCInputSource>     popupSrc,
        QSharedPointer<QLCInputSource>     automationSrc,
        QSharedPointer<QLCInputSource>     presetChooseSrc,
        QSharedPointer<QLCInputSource>     entrySelectSrc,
        QSharedPointer<QLCInputSource>     spreadPageSrc,
        QSharedPointer<QLCInputSource>     commitSrc,
        bool                               stageBeforeCommit,
        bool                               entrySelectAutoCommit,
        bool                               logPresetChanges,
        const QList<QSharedPointer<QLCInputSource>>& functionEntryInputs,
        const QList<QKeySequence>&                   functionEntryKeys,
        const QList<int>&                            functionEntryInputValues,
        const QList<QSharedPointer<QLCInputSource>>& spreadSlotInputs,
    const QList<QKeySequence>&                   spreadSlotKeys,
    const QList<bool>&                           functionEntryFlash,
    const QList<bool>&                           functionEntryFlashOverride,
    const QList<bool>&                           functionEntryFlashForceLtp,
    const QList<QColor>&                         functionEntryLabelColors,
        quint32                            ownerWidgetId,
        quint32                            widgetTargetId,
        int                                widgetOutputIndex,
        int                                widgetParameter,
        QSharedPointer<QLCInputSource>     widgetLiveInputSource,
        MultiButtonWidgetBusPolicy         widgetBusPolicy,
        int                                widgetPage,
        QWidget*                           parent = nullptr);

    MultiButtonMode                widgetMode()            const;
    QString                        targetListName()        const;
    QList<quint32>                 functionIds()           const;
    QStringList                    functionLabels()        const;
    QStringList                    iconPaths()             const;
    QList<LevelChannelBinding>     levelChannelBindings()  const;
    QList<LevelPreset>             levelPresets()          const;
    QList<LevelPreset>             widgetEntryAppearance() const;
    int                            longPressMs()           const;
    bool                           addOffAtEnd()           const;
    bool                           monitorChannelValues()  const;
    bool                           receiveInputOnInactiveFramePage() const;
    MultiButtonLayout              widgetLayout()          const;
    int                            spreadColumns()         const;
    int                            spreadRows()            const;
    int                            spreadHMargin()         const;
    int                            spreadVMargin()         const;
    int                            spreadTileWidth()       const;
    int                            spreadTileHeight()      const;
    int                            spreadPages()           const;
    bool                           automationEnabled()     const;
    QList<MultiButtonAutomationProfile> automationProfiles() const;
    int                            activeAutomationProfile() const;
    QSharedPointer<QLCInputSource> triggerInputSource()    const;
    QSharedPointer<QLCInputSource> popupInputSource()      const;
    QSharedPointer<QLCInputSource> automationInputSource() const;
    QSharedPointer<QLCInputSource> presetChooseInputSource() const;
    QSharedPointer<QLCInputSource> entrySelectInputSource() const;
    QSharedPointer<QLCInputSource> spreadPageInputSource()  const;
    QSharedPointer<QLCInputSource> commitInputSource()      const;
    bool                           stageBeforeCommit()     const;
    bool                           entrySelectAutoCommit() const;
    bool                           logPresetChanges()      const;
    QList<QSharedPointer<QLCInputSource>> functionEntryInputs() const;
    QList<QKeySequence>                   functionEntryKeys()  const;
    QList<int>                            functionEntryInputValues() const;
    QList<QSharedPointer<QLCInputSource>> spreadSlotInputs()  const;
    QList<QKeySequence>                   spreadSlotKeys()   const;
    QList<bool>                           functionEntryFlash() const;
    QList<bool>                           functionEntryFlashOverride() const;
    QList<bool>                           functionEntryFlashForceLtp() const;
    QList<QColor>                         functionEntryLabelColors() const;
    quint32                               widgetTargetId() const;
    int                                   widgetOutputIndex() const;
    int                                   widgetParameter() const;
    QSharedPointer<QLCInputSource>        widgetLiveInputSource() const;
    MultiButtonWidgetBusPolicy            widgetBusPolicy() const;

    void accept() override;

private slots:
    void slotModeChanged(int index);
    void slotLayoutChanged(int index);
    void updateSpreadPagesPreview();
    void commitEntryInputEditor();
    void loadEntryInputEditor(int row);
    void updatePresetInputCell(int row);
    void updateAllPresetInputCells();
    void rebuildSpreadSlotTable();
    void updateSpreadColumnInputVisibility();
    bool dialogSpreadPagingActive() const;
    int  dialogSlotsPerPage() const;
    int  spreadLocalSlotForRow(int row) const;
    QSharedPointer<QLCInputSource> spreadSlotInputAt(int localSlot) const;
    void setSpreadSlotInputAt(int localSlot, QSharedPointer<QLCInputSource> src);
    QKeySequence spreadSlotKeyAt(int localSlot) const;
    void setSpreadSlotKeyAt(int localSlot, const QKeySequence& key);
    QSharedPointer<QLCInputSource> entryInputForRow(int row) const;
    void setEntryInputForRow(int row, QSharedPointer<QLCInputSource> src);
    QKeySequence entryKeyForRow(int row) const;
    void setEntryKeyForRow(int row, const QKeySequence& key);
    int entryInputValueForRow(int row) const;
    void setEntryInputValueForRow(int row, int value);
    static QString formatInputPatch(const QSharedPointer<QLCInputSource>& src,
                                    const QKeySequence& key = QKeySequence());
    static QSharedPointer<QLCInputSource> inputFromPatchString(const QString& patch);
    void slotSpreadSlotSelectionChanged();
    void slotAutomationProfileRowChanged(int currentRow, int previousRow);
    void slotAutomationAddProfile();
    void slotAutomationRemoveProfile();
    void slotAutomationProfileModeChanged(int row);
    void slotAutoExcludeItemChanged(QTableWidgetItem* item);
    void slotAdd();
    void slotRemove();
    void slotEditLabel();
    void slotScribbleIcon();
    void slotChooseIcon();
    void slotClearIcon();
    void slotMoveUp();
    void slotMoveDown();
    void slotSelectionChanged();

    void slotChooseChannels();
    void slotLevelAddPreset();
    void slotLevelRemovePreset();
    void slotLevelEditLabel();
    void slotLevelScribbleIcon();
    void slotLevelChooseIcon();
    void slotLevelClearIcon();
    void slotLevelChooseColor();
    void slotLevelClearColor();
    void slotLevelChooseLabelColor();
    void slotLevelClearLabelColor();
    void slotLevelFlashToggled(int state);
    void slotLevelFlashOverrideToggled(int state);
    void slotLevelFlashForceLtpToggled(int state);
    void slotLevelAddFormula();
    void slotLevelClearFormula();
    void slotFunctionFlashToggled(int state);
    void slotFunctionFlashOverrideToggled(int state);
    void slotFunctionFlashForceLtpToggled(int state);
    void slotEntryInputValueCheckToggled(bool checked);
    void slotEntryInputValueChanged(int value);
    void slotFunctionChooseLabelColor();
    void slotFunctionClearLabelColor();
    void slotLevelMoveUp();
    void slotLevelMoveDown();
    void slotLevelSelectionChanged();
    void slotPresetTableItemChanged(QTableWidgetItem* item);
    void slotWidgetTargetChanged();
    void slotWidgetOutputChanged(int index);
    void slotWidgetParameterChanged(int index);
    void slotWidgetTargetSearchChanged(const QString& text);

private:
    void rebuildList();
    void syncPresetTableColumns();
    void syncPresetTableRows();
    quint8 presetTableValue(int row, int col) const;
    QString presetCellFormula(int row, int col) const;
    static quint8 parseDmxCell(const QString& text);
    static QTableWidgetItem* makeValueTableItem(quint8 value);
    static QTableWidgetItem* makePresetValueTableItem(const LevelPreset& preset, int valCol);
    static constexpr int kDmxFormulaUserRole = Qt::UserRole + 2;
    void updatePresetNameCell(int row);
    void syncPresetNameFromCell(int row, const QString& cellText);
    QString presetNameCellText(int row) const;
    QList<int> selectedPresetRows() const;
    bool selectedPresetRowsContiguous() const;
    void commitLevelPresetsFromTable();
    void selectPresetRows(const QList<int>& rows);
    void refreshPresetRows(const QList<int>& rows);
    void applyAppearanceToSelectedRows(const std::function<void(LevelPreset&)>& fn);
    void moveSelectedPresetBlock(int delta);
    void updateChooseChannelsButton();
    void updateMonitorTooltip();
    void syncDataFromProfileTable();
    void rebuildAutomationProfileTable();
    void rebuildAutomationExcludeTable();
    void slotExcludeColumnHeaderClicked(int col);
    void slotExcludeRowHeaderClicked(int row);
    void toggleExcludeColumn(int col);
    void toggleExcludeRow(int row);
    int  entryCountForAutomation() const;
    QString entryLabelForAutomation(int index) const;
    void rebuildWidgetTargetCombo(quint32 preferredId);
    void filterWidgetTargetTree();
    void selectWidgetTargetTreeItem(quint32 widgetId);
    QString widgetTargetTypeLabel(const VCWidget* widget) const;
    void rebuildWidgetOutputCombo();
    void rebuildWidgetParameterCombo();
    void rebuildWidgetPreview();
    void updateWidgetLiveInputUi();
    PresetTableV2MultiButtonTargetIface* selectedWidgetTarget() const;
    static quint64 bindingKey(quint32 fixtureId, quint32 channel);
    static QString bindingHeaderLabel(Doc* doc, const LevelChannelBinding& b);

    Doc* m_doc;
    QString m_targetListName;

    QList<quint32>             m_ids;
    QStringList                m_labels;
    QStringList                m_icons;

    QList<LevelChannelBinding> m_levelChannelBindings;
    QList<LevelPreset>         m_levelPresets;
    QList<LevelPreset>         m_widgetEntryAppearance;
    QList<QSharedPointer<QLCInputSource>> m_functionEntryInputs;
    QList<QKeySequence>                   m_functionEntryKeys;
    QList<int>                            m_functionEntryInputValues;
    QList<QSharedPointer<QLCInputSource>> m_spreadSlotInputs;
    QList<QKeySequence>                   m_spreadSlotKeys;
    QList<bool>                           m_functionEntryFlash;
    QList<bool>                           m_functionEntryFlashOverride;
    QList<bool>                           m_functionEntryFlashForceLtp;
    QList<QColor>                         m_functionEntryLabelColors;
    quint32                               m_ownerWidgetId = VCWidget::invalidId();

    QComboBox*       m_modeCombo    = nullptr;
    QStackedWidget*  m_modeStack    = nullptr;
    QWidget*         m_functionPage = nullptr;
    QWidget*         m_levelPage    = nullptr;
    QWidget*         m_widgetPage   = nullptr;

    QListWidget*  m_listWidget    = nullptr;
    QPushButton*  m_addBtn        = nullptr;
    QPushButton*  m_removeBtn     = nullptr;
    QPushButton*  m_editLblBtn    = nullptr;
    QPushButton*  m_scribbleBtn   = nullptr;
    QPushButton*  m_chooseIconBtn = nullptr;
    QPushButton*  m_clearIconBtn  = nullptr;
    QPushButton*  m_upBtn         = nullptr;
    QPushButton*  m_downBtn       = nullptr;

    QPushButton*  m_chooseChannelsBtn = nullptr;
    QTableWidget* m_presetTable      = nullptr;
    QTableWidget* m_widgetAppearanceTable = nullptr;
    QPushButton*  m_lvlAddBtn        = nullptr;
    QPushButton*  m_lvlRemoveBtn     = nullptr;
    QPushButton*  m_lvlEditLblBtn    = nullptr;
    QPushButton*  m_lvlScribbleBtn   = nullptr;
    QPushButton*  m_lvlChooseIconBtn = nullptr;
    QPushButton*  m_lvlClearIconBtn  = nullptr;
    QPushButton*  m_lvlChooseColorBtn = nullptr;
    QPushButton*  m_lvlClearColorBtn  = nullptr;
    QPushButton*  m_lvlChooseLabelColorBtn = nullptr;
    QPushButton*  m_lvlClearLabelColorBtn  = nullptr;
    QPushButton*  m_lvlAddFormulaBtn       = nullptr;
    QPushButton*  m_lvlClearFormulaBtn     = nullptr;
    QCheckBox*    m_lvlFlashCheck         = nullptr;
    QCheckBox*    m_lvlFlashOverrideCheck = nullptr;
    QCheckBox*    m_lvlFlashForceLtpCheck = nullptr;
    QCheckBox*    m_functionFlashCheck     = nullptr;
    QCheckBox*    m_functionFlashOverrideCheck = nullptr;
    QCheckBox*    m_functionFlashForceLtpCheck = nullptr;
    QPushButton*  m_lvlUpBtn         = nullptr;
    QPushButton*  m_lvlDownBtn       = nullptr;

    QLineEdit*    m_targetListNameEdit = nullptr;
    QLineEdit*    m_widgetTargetSearch = nullptr;
    QTreeWidget*  m_widgetTargetTree = nullptr;
    QComboBox*    m_widgetOutputCombo = nullptr;
    QComboBox*    m_widgetParameterCombo = nullptr;
    QListWidget*  m_widgetPreviewList = nullptr;
    QLabel*       m_widgetLiveInputStatus = nullptr;
    InputSelectionWidget* m_widgetLiveInputSel = nullptr;
    quint32       m_widgetTargetId = VCWidget::invalidId();
    int           m_widgetOutputIndex = 0;
    int           m_widgetParameter = 0;
    QSharedPointer<QLCInputSource> m_widgetLiveInputSource;
    MultiButtonWidgetBusPolicy m_widgetBusPolicy = MultiButtonWidgetBusPolicy::SharedBus;

    QSpinBox*     m_longPressSpin     = nullptr;
    QCheckBox*    m_offAtEndCheck     = nullptr;
    QCheckBox*    m_monitorCheck      = nullptr;

    QComboBox*    m_layoutCombo       = nullptr;
    QSpinBox*     m_colsSpin          = nullptr;
    QSpinBox*     m_rowsSpin          = nullptr;
    QSpinBox*     m_hMarginSpin       = nullptr;
    QSpinBox*     m_vMarginSpin       = nullptr;
    QSpinBox*     m_tileWSpin         = nullptr;
    QSpinBox*     m_tileHSpin         = nullptr;
    QSpinBox*     m_pagesSpin         = nullptr;
    QLabel*       m_spreadPagesPreview = nullptr;
    QGroupBox*    m_singleLayoutGrp    = nullptr;
    QGroupBox*    m_spreadLayoutGrp    = nullptr;
    QGroupBox*    m_spreadPageInputGrp = nullptr;
    QGroupBox*    m_spreadColumnInputGrp = nullptr;
    QTableWidget* m_spreadSlotTable     = nullptr;
    QGroupBox*    m_entryInputGrp       = nullptr;
    InputSelectionWidget* m_presetEntryInputSel = nullptr;
    QCheckBox*    m_entryInputValueCheck = nullptr;
    QSpinBox*     m_entryInputValueSpin = nullptr;

    QCheckBox*    m_autoEnableCheck    = nullptr;
    QTableWidget* m_autoProfileTable   = nullptr;
    QTableWidget* m_autoExcludeTable   = nullptr;

    QList<MultiButtonAutomationProfile> m_automationProfiles;
    int                               m_activeAutomationProfile = 0;
    bool                              m_syncingAutomationUi     = false;
    bool                              m_rebuildingProfileTable  = false;
    bool                              m_rebuildingExcludeTable  = false;

    static constexpr int kProfColName       = 0;
    static constexpr int kProfColMode       = 1;
    static constexpr int kProfColJumpMin    = 2;
    static constexpr int kProfColJumpMax    = 3;
    static constexpr int kProfColMultiplier = 4;
    static constexpr int kProfColBeatOffset = 5;

    InputSelectionWidget* m_triggerInputSel      = nullptr;
    InputSelectionWidget* m_popupInputSel        = nullptr;
    InputSelectionWidget* m_automationInputSel   = nullptr;
    InputSelectionWidget* m_presetChooseInputSel = nullptr;
    InputSelectionWidget* m_entrySelectInputSel  = nullptr;
    InputSelectionWidget* m_spreadPageInputSel   = nullptr;
    InputSelectionWidget* m_commitInputSel       = nullptr;
    QCheckBox*            m_stageBeforeCommitCheck = nullptr;
    QCheckBox*            m_entrySelectAutoCommitCheck = nullptr;
    QCheckBox*            m_receiveInputInactiveFrameCheck = nullptr;
    QDialogButtonBox* m_buttons = nullptr;

    bool m_rebuildingPresetTable = false;
    bool m_syncingEntryInputEditor = false;
    int  m_entryInputEditRow = -1;
    int  m_spreadSlotEditRow = -1;

    static constexpr int kPresetNameColumn     = 0;
    static constexpr int kPresetInputColumn    = 1;
    static constexpr int kPresetFirstDmxColumn = 2;
    static constexpr int kDataRowOffset = 0;
};

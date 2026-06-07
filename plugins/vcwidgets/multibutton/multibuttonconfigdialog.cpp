/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "multibuttonconfigdialog.h"
#include "mbvalueexpr.h"

#include "inputselectionwidget.h"
#include "presettablev2multibuttoniface.h"
#include "virtualconsole.h"
#include "vcframe.h"
#include "functionselection.h"
#include "function.h"
#include "doc.h"
#include "fixture.h"
#include "qlcchannel.h"
#include "scribbledialog.h"
#include "channelsselection.h"

#include <QInputDialog>
#include <QFormLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QBrush>
#include <QColorDialog>
#include <QSignalBlocker>
#include <QHeaderView>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVector>
#include <QSet>
#include <QMessageBox>
#include <algorithm>

static void syncExcludeMaskFromColumn(QTableWidget* table, int col, quint32& mask);
static void syncAllExcludeMasksFromTable(QTableWidget* table,
                                         QList<MultiButtonAutomationProfile>& profiles);

static QTableWidgetItem* widgetColumnPlaceholder()
{
    QTableWidgetItem* item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsEnabled);
    return item;
}

static void tuneActionButton(QPushButton* button)
{
    if (!button)
        return;
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

static void tuneAutomationProfileColumnWidths(QTableWidget* table)
{
    constexpr int colMode = 1;
    constexpr int colJumpMin = 2;
    constexpr int colJumpMax = 3;
    constexpr int colMultiplier = 4;
    constexpr int colBeatOffset = 5;

    if (!table || table->rowCount() == 0)
        return;
    if (table->cellWidget(0, colMode) == nullptr)
        return;

    table->resizeColumnsToContents();
    table->setColumnWidth(colMode, qMax(table->columnWidth(colMode), 90));
    table->setColumnWidth(colJumpMin, qMax(table->columnWidth(colJumpMin), 60));
    table->setColumnWidth(colJumpMax, qMax(table->columnWidth(colJumpMax), 60));
    table->setColumnWidth(colMultiplier, qMax(table->columnWidth(colMultiplier), 70));
    table->setColumnWidth(colBeatOffset, qMax(table->columnWidth(colBeatOffset), 60));
}

MultiButtonConfigDialog::MultiButtonConfigDialog(
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
    bool                               /*logPresetChanges*/,
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
    QWidget*                           parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_targetListName(targetListName)
    , m_ids(funcIds)
    , m_labels(funcLabels)
    , m_icons(iconPaths)
    , m_levelChannelBindings(levelChannelBindings)
    , m_levelPresets(levelPresets)
    , m_widgetEntryAppearance(widgetEntryAppearance)
    , m_functionEntryInputs(functionEntryInputs)
    , m_functionEntryKeys(functionEntryKeys)
    , m_functionEntryInputValues(functionEntryInputValues)
    , m_spreadSlotInputs(spreadSlotInputs)
    , m_spreadSlotKeys(spreadSlotKeys)
    , m_functionEntryFlash(functionEntryFlash)
    , m_functionEntryFlashOverride(functionEntryFlashOverride)
    , m_functionEntryFlashForceLtp(functionEntryFlashForceLtp)
    , m_functionEntryLabelColors(functionEntryLabelColors)
    , m_ownerWidgetId(ownerWidgetId)
    , m_widgetTargetId(widgetTargetId)
    , m_widgetOutputIndex(qMax(0, widgetOutputIndex))
    , m_widgetParameter(qMax(0, widgetParameter))
    , m_widgetLiveInputSource(widgetLiveInputSource)
    , m_widgetBusPolicy(widgetBusPolicy)
    , m_automationProfiles(automationProfiles)
    , m_activeAutomationProfile(activeAutomationProfile)
{
    if (m_automationProfiles.isEmpty())
    {
        MultiButtonAutomationProfile def;
        def.name = tr("Profile %1").arg(1);
        m_automationProfiles.append(def);
    }
    m_activeAutomationProfile = qBound(0, m_activeAutomationProfile,
                                     m_automationProfiles.size() - 1);

    while (m_labels.size() < m_ids.size()) m_labels.append(QString());
    while (m_icons.size() < m_ids.size())  m_icons.append(QString());
    while (m_functionEntryInputValues.size() < m_ids.size())
        m_functionEntryInputValues.append(-1);
    while (m_functionEntryInputValues.size() > m_ids.size())
        m_functionEntryInputValues.removeLast();
    while (m_functionEntryFlash.size() < m_ids.size())
        m_functionEntryFlash.append(false);
    while (m_functionEntryFlash.size() > m_ids.size())
        m_functionEntryFlash.removeLast();
    while (m_functionEntryFlashOverride.size() < m_ids.size())
        m_functionEntryFlashOverride.append(false);
    while (m_functionEntryFlashOverride.size() > m_ids.size())
        m_functionEntryFlashOverride.removeLast();
    while (m_functionEntryFlashForceLtp.size() < m_ids.size())
        m_functionEntryFlashForceLtp.append(false);
    while (m_functionEntryFlashForceLtp.size() > m_ids.size())
        m_functionEntryFlashForceLtp.removeLast();
    while (m_functionEntryLabelColors.size() < m_ids.size())
        m_functionEntryLabelColors.append(QColor());
    while (m_functionEntryLabelColors.size() > m_ids.size())
        m_functionEntryLabelColors.removeLast();

    setWindowTitle(tr("Multi Button — Properties"));
    setMinimumWidth(560);
    setMinimumHeight(420);

    QVBoxLayout* root = new QVBoxLayout(this);

    QTabWidget* tabs = new QTabWidget(this);

    // ---- Tab: Entries ---------------------------------------------------
    QWidget* entriesTab = new QWidget(tabs);
    QVBoxLayout* entriesLay = new QVBoxLayout(entriesTab);

    QHBoxLayout* targetNameRow = new QHBoxLayout;
    targetNameRow->addWidget(new QLabel(tr("List name:"), entriesTab));
    m_targetListNameEdit = new QLineEdit(entriesTab);
    m_targetListNameEdit->setText(m_targetListName);
    m_targetListNameEdit->setPlaceholderText(tr("Name used in linked-widget lists"));
    targetNameRow->addWidget(m_targetListNameEdit, 1);
    root->addLayout(targetNameRow);

    QHBoxLayout* modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel(tr("Mode:"), entriesTab));
    m_modeCombo = new QComboBox(entriesTab);
    m_modeCombo->addItem(tr("Function"), (int) MultiButtonMode::Function);
    m_modeCombo->addItem(tr("Level"),    (int) MultiButtonMode::Level);
    m_modeCombo->addItem(tr("Widget"),   (int) MultiButtonMode::Widget);
    for (int i = 0; i < m_modeCombo->count(); ++i)
    {
        if (m_modeCombo->itemData(i).toInt() == (int) widgetMode)
        {
            m_modeCombo->setCurrentIndex(i);
            break;
        }
    }
    modeRow->addWidget(m_modeCombo, 1);
    root->addLayout(modeRow);

    m_modeStack = new QStackedWidget(this);

    // ---- Function page --------------------------------------------------
    m_functionPage = new QWidget(this);
    QVBoxLayout* funcLay = new QVBoxLayout(m_functionPage);
    funcLay->setContentsMargins(0, 0, 0, 0);

    QGroupBox* listGrp = new QGroupBox(tr("Functions (cycle order)"), m_functionPage);
    QVBoxLayout* listLayout = new QVBoxLayout(listGrp);

    m_listWidget = new QListWidget(listGrp);
    m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setIconSize(QSize(24, 24));
    m_listWidget->setMinimumHeight(160);

    m_addBtn        = new QPushButton(tr("Add..."),          listGrp);
    m_removeBtn     = new QPushButton(tr("Remove"),          listGrp);
    m_editLblBtn    = new QPushButton(tr("Label..."),        listGrp);
    m_scribbleBtn   = new QPushButton(tr("Scribble..."),     listGrp);
    m_chooseIconBtn = new QPushButton(tr("Icon..."),         listGrp);
    m_clearIconBtn  = new QPushButton(tr("Clear icon"),      listGrp);
    m_functionFlashCheck = new QCheckBox(tr("Flash (hold)"), listGrp);
    m_functionFlashCheck->setToolTip(
        tr("While held: flash this function. Release restores the previous latched entry."));
    m_functionFlashOverrideCheck = new QCheckBox(tr("Override priority"), listGrp);
    m_functionFlashOverrideCheck->setToolTip(
        tr("Flash this function with Override priority, like a QLC+ Button flash."));
    m_functionFlashForceLtpCheck = new QCheckBox(tr("Force LTP"), listGrp);
    m_functionFlashForceLtpCheck->setToolTip(
        tr("Force LTP while flashing this function, like a QLC+ Button flash."));
    QPushButton* funcLabelColorBtn = new QPushButton(tr("Text color..."), listGrp);
    QPushButton* funcClearLabelColorBtn = new QPushButton(tr("Clear text"), listGrp);
    m_upBtn         = new QPushButton(tr("Up"),              listGrp);
    m_downBtn       = new QPushButton(tr("Down"),            listGrp);

    QList<QPushButton*> functionActionButtons = {
        m_addBtn, m_removeBtn, m_editLblBtn, m_scribbleBtn, m_chooseIconBtn,
        m_clearIconBtn, funcLabelColorBtn, funcClearLabelColorBtn,
        m_upBtn, m_downBtn
    };
    for (QPushButton* button : functionActionButtons)
        tuneActionButton(button);

    m_addBtn->setToolTip(tr("Add functions to the cycle."));
    m_removeBtn->setToolTip(tr("Remove the selected function entry."));
    m_upBtn->setToolTip(tr("Move the selected function entry up."));
    m_downBtn->setToolTip(tr("Move the selected function entry down."));
    m_editLblBtn->setToolTip(tr("Edit the selected entry label."));
    m_scribbleBtn->setToolTip(tr("Draw a scribble icon for the selected entry."));
    m_chooseIconBtn->setToolTip(tr("Choose an icon for the selected entry."));
    m_clearIconBtn->setToolTip(tr("Clear the selected entry icon."));
    funcLabelColorBtn->setToolTip(tr("Choose text color for the selected entry."));
    funcClearLabelColorBtn->setToolTip(tr("Clear text color from the selected entry."));

    QHBoxLayout* functionTopActions = new QHBoxLayout;
    functionTopActions->setSpacing(6);
    functionTopActions->addStretch();
    functionTopActions->addWidget(m_addBtn);
    functionTopActions->addWidget(m_removeBtn);
    functionTopActions->addSpacing(8);
    functionTopActions->addWidget(m_upBtn);
    functionTopActions->addWidget(m_downBtn);
    listLayout->addLayout(functionTopActions);

    listLayout->addWidget(m_listWidget, 1);

    QHBoxLayout* functionEntryActions = new QHBoxLayout;
    functionEntryActions->setSpacing(6);
    functionEntryActions->addWidget(new QLabel(tr("Entry:"), listGrp));
    functionEntryActions->addWidget(m_editLblBtn);
    functionEntryActions->addWidget(m_scribbleBtn);
    functionEntryActions->addWidget(m_chooseIconBtn);
    functionEntryActions->addWidget(m_clearIconBtn);
    functionEntryActions->addWidget(m_functionFlashCheck);
    functionEntryActions->addWidget(m_functionFlashOverrideCheck);
    functionEntryActions->addWidget(m_functionFlashForceLtpCheck);
    functionEntryActions->addStretch();
    listLayout->addLayout(functionEntryActions);

    QHBoxLayout* functionColorActions = new QHBoxLayout;
    functionColorActions->setSpacing(6);
    functionColorActions->addWidget(new QLabel(tr("Colors:"), listGrp));
    functionColorActions->addWidget(funcLabelColorBtn);
    functionColorActions->addWidget(funcClearLabelColorBtn);
    functionColorActions->addStretch();
    listLayout->addLayout(functionColorActions);
    funcLay->addWidget(listGrp);

    connect(m_functionFlashCheck, &QCheckBox::stateChanged,
            this, &MultiButtonConfigDialog::slotFunctionFlashToggled);
    connect(m_functionFlashOverrideCheck, &QCheckBox::stateChanged,
            this, &MultiButtonConfigDialog::slotFunctionFlashOverrideToggled);
    connect(m_functionFlashForceLtpCheck, &QCheckBox::stateChanged,
            this, &MultiButtonConfigDialog::slotFunctionFlashForceLtpToggled);
    connect(funcLabelColorBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotFunctionChooseLabelColor);
    connect(funcClearLabelColorBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotFunctionClearLabelColor);

    connect(m_addBtn,        &QPushButton::clicked, this, &MultiButtonConfigDialog::slotAdd);
    connect(m_removeBtn,     &QPushButton::clicked, this, &MultiButtonConfigDialog::slotRemove);
    connect(m_editLblBtn,    &QPushButton::clicked, this, &MultiButtonConfigDialog::slotEditLabel);
    connect(m_scribbleBtn,   &QPushButton::clicked, this, &MultiButtonConfigDialog::slotScribbleIcon);
    connect(m_chooseIconBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotChooseIcon);
    connect(m_clearIconBtn,  &QPushButton::clicked, this, &MultiButtonConfigDialog::slotClearIcon);
    connect(m_upBtn,         &QPushButton::clicked, this, &MultiButtonConfigDialog::slotMoveUp);
    connect(m_downBtn,       &QPushButton::clicked, this, &MultiButtonConfigDialog::slotMoveDown);
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotSelectionChanged);

    m_modeStack->addWidget(m_functionPage);

    // ---- Level page -----------------------------------------------------
    m_levelPage = new QWidget(this);
    QVBoxLayout* levelLay = new QVBoxLayout(m_levelPage);
    levelLay->setContentsMargins(0, 0, 0, 0);

    QGroupBox* presetGrp = new QGroupBox(tr("Presets (cycle order)"), m_levelPage);
    QVBoxLayout* presetLayout = new QVBoxLayout(presetGrp);

    m_presetTable = new QTableWidget(presetGrp);
    m_presetTable->setMinimumHeight(160);
    m_presetTable->horizontalHeader()->setVisible(true);
    m_presetTable->verticalHeader()->hide();
    m_presetTable->setIconSize(QSize(24, 24));
    m_presetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_presetTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_presetTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_presetTable->setAlternatingRowColors(true);
    m_presetTable->verticalHeader()->setDefaultSectionSize(22);

    m_chooseChannelsBtn = new QPushButton(tr("Choose channels…"), presetGrp);
    m_lvlAddBtn        = new QPushButton(tr("Add"),             presetGrp);
    m_lvlRemoveBtn     = new QPushButton(tr("Remove"),          presetGrp);
    m_lvlEditLblBtn    = new QPushButton(tr("Label..."),        presetGrp);
    m_lvlScribbleBtn   = new QPushButton(tr("Scribble..."),     presetGrp);
    m_lvlChooseIconBtn = new QPushButton(tr("Icon..."),         presetGrp);
    m_lvlClearIconBtn  = new QPushButton(tr("Clear icon"),      presetGrp);
    m_lvlChooseColorBtn = new QPushButton(tr("Button color..."), presetGrp);
    m_lvlClearColorBtn  = new QPushButton(tr("Clear button"),    presetGrp);
    m_lvlChooseLabelColorBtn = new QPushButton(tr("Text color..."), presetGrp);
    m_lvlClearLabelColorBtn  = new QPushButton(tr("Clear text"),    presetGrp);
    m_lvlAddFormulaBtn = new QPushButton(tr("Formula..."), presetGrp);
    m_lvlClearFormulaBtn = new QPushButton(tr("Clear formula"), presetGrp);
    m_lvlFlashCheck = new QCheckBox(tr("Flash (hold)"), presetGrp);
    m_lvlFlashCheck->setTristate(true);
    m_lvlFlashCheck->setToolTip(
        tr("While held: output this preset's DMX. Release restores the previous latched preset."));
    m_lvlFlashOverrideCheck = new QCheckBox(tr("Override priority"), presetGrp);
    m_lvlFlashOverrideCheck->setTristate(true);
    m_lvlFlashOverrideCheck->setToolTip(
        tr("Flash this preset with Override priority, like a QLC+ Button flash."));
    m_lvlFlashForceLtpCheck = new QCheckBox(tr("Force LTP"), presetGrp);
    m_lvlFlashForceLtpCheck->setTristate(true);
    m_lvlFlashForceLtpCheck->setToolTip(
        tr("Force LTP while flashing this preset, like a QLC+ Button flash."));
    m_lvlUpBtn         = new QPushButton(tr("Up"),             presetGrp);
    m_lvlDownBtn       = new QPushButton(tr("Down"),           presetGrp);

    QList<QPushButton*> levelActionButtons = {
        m_chooseChannelsBtn, m_lvlAddBtn, m_lvlRemoveBtn, m_lvlEditLblBtn,
        m_lvlScribbleBtn, m_lvlChooseIconBtn, m_lvlClearIconBtn,
        m_lvlChooseColorBtn, m_lvlClearColorBtn, m_lvlChooseLabelColorBtn,
        m_lvlClearLabelColorBtn, m_lvlAddFormulaBtn, m_lvlClearFormulaBtn,
        m_lvlUpBtn, m_lvlDownBtn
    };
    for (QPushButton* button : levelActionButtons)
        tuneActionButton(button);

    m_lvlAddBtn->setToolTip(tr("Add a new preset entry."));
    m_lvlRemoveBtn->setToolTip(tr("Remove selected preset entries."));
    m_lvlUpBtn->setToolTip(tr("Move selected preset entries up."));
    m_lvlDownBtn->setToolTip(tr("Move selected preset entries down."));
    m_lvlEditLblBtn->setToolTip(tr("Edit the selected preset label."));
    m_lvlScribbleBtn->setToolTip(tr("Draw a scribble icon for selected presets."));
    m_lvlChooseIconBtn->setToolTip(tr("Choose an icon for selected presets."));
    m_lvlClearIconBtn->setToolTip(tr("Clear icon from selected presets."));
    m_lvlChooseColorBtn->setToolTip(tr("Choose button color for selected presets."));
    m_lvlClearColorBtn->setToolTip(tr("Clear button color from selected presets."));
    m_lvlChooseLabelColorBtn->setToolTip(tr("Choose text color for selected presets."));
    m_lvlClearLabelColorBtn->setToolTip(tr("Clear text color from selected presets."));
    m_lvlAddFormulaBtn->setToolTip(tr("Add an IF(...) formula to the selected DMX value cell."));
    m_lvlClearFormulaBtn->setToolTip(tr("Clear formula from the selected DMX value cell."));

    QHBoxLayout* presetTopActions = new QHBoxLayout;
    presetTopActions->setSpacing(6);
    presetTopActions->addWidget(m_chooseChannelsBtn);
    presetTopActions->addStretch();
    presetTopActions->addWidget(m_lvlAddBtn);
    presetTopActions->addWidget(m_lvlRemoveBtn);
    presetTopActions->addSpacing(8);
    presetTopActions->addWidget(m_lvlUpBtn);
    presetTopActions->addWidget(m_lvlDownBtn);
    presetLayout->addLayout(presetTopActions);

    presetLayout->addWidget(m_presetTable, 1);

    QHBoxLayout* presetEntryActions = new QHBoxLayout;
    presetEntryActions->setSpacing(6);
    presetEntryActions->addWidget(new QLabel(tr("Entry:"), presetGrp));
    presetEntryActions->addWidget(m_lvlEditLblBtn);
    presetEntryActions->addWidget(m_lvlScribbleBtn);
    presetEntryActions->addWidget(m_lvlChooseIconBtn);
    presetEntryActions->addWidget(m_lvlClearIconBtn);
    presetEntryActions->addWidget(m_lvlFlashCheck);
    presetEntryActions->addWidget(m_lvlFlashOverrideCheck);
    presetEntryActions->addWidget(m_lvlFlashForceLtpCheck);
    presetEntryActions->addStretch();
    presetLayout->addLayout(presetEntryActions);

    QHBoxLayout* presetColorActions = new QHBoxLayout;
    presetColorActions->setSpacing(6);
    presetColorActions->addWidget(new QLabel(tr("Colors:"), presetGrp));
    presetColorActions->addWidget(m_lvlChooseColorBtn);
    presetColorActions->addWidget(m_lvlClearColorBtn);
    presetColorActions->addWidget(m_lvlChooseLabelColorBtn);
    presetColorActions->addWidget(m_lvlClearLabelColorBtn);
    presetColorActions->addStretch();
    presetLayout->addLayout(presetColorActions);

    QHBoxLayout* presetFormulaActions = new QHBoxLayout;
    presetFormulaActions->setSpacing(6);
    presetFormulaActions->addWidget(new QLabel(tr("Formula:"), presetGrp));
    presetFormulaActions->addWidget(m_lvlAddFormulaBtn);
    presetFormulaActions->addWidget(m_lvlClearFormulaBtn);
    presetFormulaActions->addStretch();
    presetLayout->addLayout(presetFormulaActions);
    levelLay->addWidget(presetGrp);

    connect(m_chooseChannelsBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotChooseChannels);
    connect(m_lvlAddBtn,        &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelAddPreset);
    connect(m_lvlRemoveBtn,     &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelRemovePreset);
    connect(m_lvlEditLblBtn,    &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelEditLabel);
    connect(m_lvlScribbleBtn,   &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelScribbleIcon);
    connect(m_lvlChooseIconBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelChooseIcon);
    connect(m_lvlClearIconBtn,  &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelClearIcon);
    connect(m_lvlChooseColorBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelChooseColor);
    connect(m_lvlClearColorBtn,  &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelClearColor);
    connect(m_lvlChooseLabelColorBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotLevelChooseLabelColor);
    connect(m_lvlClearLabelColorBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotLevelClearLabelColor);
    connect(m_lvlAddFormulaBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotLevelAddFormula);
    connect(m_lvlClearFormulaBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotLevelClearFormula);
    connect(m_lvlFlashCheck, &QCheckBox::stateChanged,
            this, &MultiButtonConfigDialog::slotLevelFlashToggled);
    connect(m_lvlFlashOverrideCheck, &QCheckBox::stateChanged,
            this, &MultiButtonConfigDialog::slotLevelFlashOverrideToggled);
    connect(m_lvlFlashForceLtpCheck, &QCheckBox::stateChanged,
            this, &MultiButtonConfigDialog::slotLevelFlashForceLtpToggled);
    connect(m_lvlUpBtn,         &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelMoveUp);
    connect(m_lvlDownBtn,       &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelMoveDown);
    connect(m_presetTable, &QTableWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotLevelSelectionChanged);
    connect(m_presetTable, &QTableWidget::currentCellChanged,
            this, [this](int, int, int, int) { slotLevelSelectionChanged(); });
    connect(m_presetTable, &QTableWidget::itemChanged,
            this, &MultiButtonConfigDialog::slotPresetTableItemChanged);

    m_modeStack->addWidget(m_levelPage);

    // ---- Widget link page ----------------------------------------------
    m_widgetPage = new QWidget(this);
    QVBoxLayout* widgetPageLay = new QVBoxLayout(m_widgetPage);
    widgetPageLay->setContentsMargins(0, 0, 0, 0);
    QScrollArea* widgetScroll = new QScrollArea(m_widgetPage);
    widgetScroll->setWidgetResizable(true);
    widgetScroll->setFrameShape(QFrame::NoFrame);
    widgetScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget* widgetScrollContent = new QWidget(widgetScroll);
    QVBoxLayout* widgetLay = new QVBoxLayout(widgetScrollContent);
    widgetLay->setContentsMargins(0, 0, 0, 0);
    widgetLay->setSpacing(8);

    QHBoxLayout* widgetTopRow = new QHBoxLayout;
    widgetTopRow->setSpacing(8);

    QGroupBox* widgetGrp = new QGroupBox(tr("Linked Widget"), widgetScrollContent);
    QVBoxLayout* widgetLinkLay = new QVBoxLayout(widgetGrp);
    m_widgetTargetSearch = new QLineEdit(widgetGrp);
    m_widgetTargetSearch->setPlaceholderText(tr("Search linked widgets..."));
    widgetLinkLay->addWidget(m_widgetTargetSearch);
    m_widgetTargetTree = new QTreeWidget(widgetGrp);
    m_widgetTargetTree->setHeaderHidden(true);
    m_widgetTargetTree->setRootIsDecorated(true);
    m_widgetTargetTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_widgetTargetTree->setMinimumHeight(150);
    widgetLinkLay->addWidget(m_widgetTargetTree, 1);
    QFormLayout* widgetForm = new QFormLayout;
    widgetForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_widgetOutputCombo = new QComboBox(widgetGrp);
    m_widgetParameterCombo = new QComboBox(widgetGrp);
    widgetForm->addRow(tr("Output:"), m_widgetOutputCombo);
    widgetForm->addRow(tr("Parameter:"), m_widgetParameterCombo);
    widgetLinkLay->addLayout(widgetForm);
    widgetTopRow->addWidget(widgetGrp, 1);

    QGroupBox* widgetLiveGrp = new QGroupBox(tr("Selector / recall channel"), widgetScrollContent);
    QVBoxLayout* widgetLiveLay = new QVBoxLayout(widgetLiveGrp);
    m_widgetLiveInputStatus = new QLabel(widgetLiveGrp);
    m_widgetLiveInputStatus->setWordWrap(false);
    widgetLiveLay->addWidget(m_widgetLiveInputStatus);
    m_widgetLiveInputSel = new InputSelectionWidget(doc, widgetLiveGrp);
    m_widgetLiveInputSel->setKeyInputVisibility(false);
    m_widgetLiveInputSel->setWidgetPage(widgetPage);
    m_widgetLiveInputSel->setInputSource(m_widgetLiveInputSource);
    widgetLiveLay->addWidget(m_widgetLiveInputSel);
    widgetTopRow->addWidget(widgetLiveGrp, 1);
    widgetLay->addLayout(widgetTopRow);

    m_widgetPreviewList = new QListWidget(widgetScrollContent);
    m_widgetPreviewList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_widgetPreviewList->hide();

    QGroupBox* widgetAppearanceGrp = new QGroupBox(tr("Linked entries appearance"), widgetScrollContent);
    QVBoxLayout* widgetAppearanceLayout = new QVBoxLayout(widgetAppearanceGrp);
    m_widgetAppearanceTable = new QTableWidget(widgetAppearanceGrp);
    m_widgetAppearanceTable->setMinimumHeight(220);
    m_widgetAppearanceTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_widgetAppearanceTable->horizontalHeader()->setVisible(true);
    m_widgetAppearanceTable->verticalHeader()->hide();
    m_widgetAppearanceTable->setIconSize(QSize(24, 24));
    m_widgetAppearanceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_widgetAppearanceTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_widgetAppearanceTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                             | QAbstractItemView::EditKeyPressed);
    m_widgetAppearanceTable->setAlternatingRowColors(true);
    m_widgetAppearanceTable->verticalHeader()->setDefaultSectionSize(22);
    widgetAppearanceLayout->addWidget(m_widgetAppearanceTable, 1);

    QPushButton* widgetEditLblBtn = new QPushButton(tr("Label..."), widgetAppearanceGrp);
    QPushButton* widgetScribbleBtn = new QPushButton(tr("Scribble..."), widgetAppearanceGrp);
    QPushButton* widgetChooseIconBtn = new QPushButton(tr("Icon..."), widgetAppearanceGrp);
    QPushButton* widgetClearIconBtn = new QPushButton(tr("Clear icon"), widgetAppearanceGrp);
    QPushButton* widgetChooseColorBtn = new QPushButton(tr("Button color..."), widgetAppearanceGrp);
    QPushButton* widgetClearColorBtn = new QPushButton(tr("Clear button"), widgetAppearanceGrp);
    QPushButton* widgetChooseLabelColorBtn = new QPushButton(tr("Text color..."), widgetAppearanceGrp);
    QPushButton* widgetClearLabelColorBtn = new QPushButton(tr("Clear text"), widgetAppearanceGrp);

    QList<QPushButton*> widgetAppearanceButtons = {
        widgetEditLblBtn, widgetScribbleBtn, widgetChooseIconBtn, widgetClearIconBtn,
        widgetChooseColorBtn, widgetClearColorBtn, widgetChooseLabelColorBtn,
        widgetClearLabelColorBtn
    };
    for (QPushButton* button : widgetAppearanceButtons)
        tuneActionButton(button);

    widgetEditLblBtn->setToolTip(tr("Edit the local label override for selected linked entries."));
    widgetScribbleBtn->setToolTip(tr("Draw a scribble icon for selected linked entries."));
    widgetChooseIconBtn->setToolTip(tr("Choose an icon for selected linked entries."));
    widgetClearIconBtn->setToolTip(tr("Clear icon from selected linked entries."));
    widgetChooseColorBtn->setToolTip(tr("Choose button color for selected linked entries."));
    widgetClearColorBtn->setToolTip(tr("Clear button color from selected linked entries."));
    widgetChooseLabelColorBtn->setToolTip(tr("Choose text color for selected linked entries."));
    widgetClearLabelColorBtn->setToolTip(tr("Clear text color from selected linked entries."));

    QHBoxLayout* widgetEntryActions = new QHBoxLayout;
    widgetEntryActions->setSpacing(6);
    widgetEntryActions->addWidget(new QLabel(tr("Entry:"), widgetAppearanceGrp));
    widgetEntryActions->addWidget(widgetEditLblBtn);
    widgetEntryActions->addWidget(widgetScribbleBtn);
    widgetEntryActions->addWidget(widgetChooseIconBtn);
    widgetEntryActions->addWidget(widgetClearIconBtn);
    widgetEntryActions->addStretch();
    widgetAppearanceLayout->addLayout(widgetEntryActions);

    QHBoxLayout* widgetColorActions = new QHBoxLayout;
    widgetColorActions->setSpacing(6);
    widgetColorActions->addWidget(new QLabel(tr("Colors:"), widgetAppearanceGrp));
    widgetColorActions->addWidget(widgetChooseColorBtn);
    widgetColorActions->addWidget(widgetClearColorBtn);
    widgetColorActions->addWidget(widgetChooseLabelColorBtn);
    widgetColorActions->addWidget(widgetClearLabelColorBtn);
    widgetColorActions->addStretch();
    widgetAppearanceLayout->addLayout(widgetColorActions);
    widgetLay->addWidget(widgetAppearanceGrp, 1);
    widgetScroll->setWidget(widgetScrollContent);
    widgetPageLay->addWidget(widgetScroll, 1);

    connect(m_widgetTargetTree, &QTreeWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotWidgetTargetChanged);
    connect(m_widgetTargetSearch, &QLineEdit::textChanged,
            this, &MultiButtonConfigDialog::slotWidgetTargetSearchChanged);
    connect(m_widgetOutputCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MultiButtonConfigDialog::slotWidgetOutputChanged);
    connect(m_widgetParameterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MultiButtonConfigDialog::slotWidgetParameterChanged);
    connect(m_widgetPreviewList, &QListWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotLevelSelectionChanged);
    connect(m_widgetAppearanceTable, &QTableWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotLevelSelectionChanged);
    connect(m_widgetAppearanceTable, &QTableWidget::itemChanged,
            this, &MultiButtonConfigDialog::slotPresetTableItemChanged);
    connect(widgetEditLblBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelEditLabel);
    connect(widgetScribbleBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelScribbleIcon);
    connect(widgetChooseIconBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelChooseIcon);
    connect(widgetClearIconBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelClearIcon);
    connect(widgetChooseColorBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelChooseColor);
    connect(widgetClearColorBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelClearColor);
    connect(widgetChooseLabelColorBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotLevelChooseLabelColor);
    connect(widgetClearLabelColorBtn, &QPushButton::clicked,
            this, &MultiButtonConfigDialog::slotLevelClearLabelColor);

    m_modeStack->addWidget(m_widgetPage);
    entriesLay->addWidget(m_modeStack, 1);

    m_entryInputGrp = new QGroupBox(tr("External input (selected entry)"), entriesTab);
    QVBoxLayout* entryInputLay = new QVBoxLayout(m_entryInputGrp);
    m_presetEntryInputSel = new InputSelectionWidget(doc, m_entryInputGrp);
    m_presetEntryInputSel->setKeyInputVisibility(true);
    m_presetEntryInputSel->setWidgetPage(widgetPage);
    entryInputLay->addWidget(m_presetEntryInputSel);
    QHBoxLayout* entryValueLay = new QHBoxLayout;
    m_entryInputValueCheck = new QCheckBox(tr("Value equals"), m_entryInputGrp);
    m_entryInputValueSpin = new QSpinBox(m_entryInputGrp);
    m_entryInputValueSpin->setRange(0, 255);
    m_entryInputValueSpin->setEnabled(false);
    entryValueLay->addWidget(m_entryInputValueCheck);
    entryValueLay->addWidget(m_entryInputValueSpin);
    entryValueLay->addStretch(1);
    entryInputLay->addLayout(entryValueLay);
    entriesLay->addWidget(m_entryInputGrp);

    connect(m_entryInputValueCheck, &QCheckBox::toggled,
            this, &MultiButtonConfigDialog::slotEntryInputValueCheckToggled);
    connect(m_entryInputValueSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MultiButtonConfigDialog::slotEntryInputValueChanged);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MultiButtonConfigDialog::slotModeChanged);

    tabs->addTab(entriesTab, tr("Entries"));

    // ---- Tab: Layout ----------------------------------------------------
    QWidget* layoutTab = new QWidget(tabs);
    QVBoxLayout* layoutTabLay = new QVBoxLayout(layoutTab);

    m_layoutCombo = new QComboBox(layoutTab);
    m_layoutCombo->addItem(tr("Single button"), (int) MultiButtonLayout::Single);
    m_layoutCombo->addItem(tr("Spread grid"), (int) MultiButtonLayout::Spread);
    m_layoutCombo->setCurrentIndex(widgetLayout == MultiButtonLayout::Spread ? 1 : 0);
    {
        QHBoxLayout* layoutRow = new QHBoxLayout;
        layoutRow->addWidget(new QLabel(tr("Presentation:"), layoutTab));
        layoutRow->addWidget(m_layoutCombo, 1);
        layoutTabLay->addLayout(layoutRow);
    }

    QGroupBox* tileSizeGrp = new QGroupBox(tr("Button tile size"), layoutTab);
    QFormLayout* tileSizeForm = new QFormLayout(tileSizeGrp);

    m_tileWSpin = new QSpinBox(tileSizeGrp);
    m_tileWSpin->setRange(20, 400);
    m_tileWSpin->setSuffix(tr(" px"));
    m_tileWSpin->setValue(spreadTileWidth);
    tileSizeForm->addRow(tr("Tile width:"), m_tileWSpin);

    m_tileHSpin = new QSpinBox(tileSizeGrp);
    m_tileHSpin->setRange(20, 400);
    m_tileHSpin->setSuffix(tr(" px"));
    m_tileHSpin->setValue(spreadTileHeight);
    tileSizeForm->addRow(tr("Tile height:"), m_tileHSpin);

    layoutTabLay->addWidget(tileSizeGrp);

    m_singleLayoutGrp = new QGroupBox(tr("Single button"), layoutTab);
    QFormLayout* singleForm = new QFormLayout(m_singleLayoutGrp);

    m_longPressSpin = new QSpinBox(m_singleLayoutGrp);
    m_longPressSpin->setRange(200, 2000);
    m_longPressSpin->setSingleStep(50);
    m_longPressSpin->setSuffix(tr(" ms"));
    m_longPressSpin->setValue(longPressMs);
    singleForm->addRow(tr("Long press threshold:"), m_longPressSpin);

    layoutTabLay->addWidget(m_singleLayoutGrp);

    m_offAtEndCheck = new QCheckBox(tr("Add \"OFF\" step at end of cycle"), layoutTab);
    m_offAtEndCheck->setChecked(addOffAtEnd);
    layoutTabLay->addWidget(m_offAtEndCheck);

    m_monitorCheck = new QCheckBox(tr("Monitor channel values"), layoutTab);
    m_monitorCheck->setChecked(monitorChannelValues);
    layoutTabLay->addWidget(m_monitorCheck);

    m_spreadLayoutGrp = new QGroupBox(tr("Spread grid"), layoutTab);
    QFormLayout* spreadForm = new QFormLayout(m_spreadLayoutGrp);

    auto makeAutoSpin = [](QWidget* parent, int value) {
        QSpinBox* spin = new QSpinBox(parent);
        spin->setRange(0, 32);
        spin->setSpecialValueText(tr("Auto"));
        spin->setValue(value);
        return spin;
    };

    m_colsSpin = makeAutoSpin(m_spreadLayoutGrp, spreadColumns);
    spreadForm->addRow(tr("Columns:"), m_colsSpin);

    m_rowsSpin = makeAutoSpin(m_spreadLayoutGrp, spreadRows);
    spreadForm->addRow(tr("Rows:"), m_rowsSpin);

    m_hMarginSpin = new QSpinBox(m_spreadLayoutGrp);
    m_hMarginSpin->setRange(0, 64);
    m_hMarginSpin->setSuffix(tr(" px"));
    m_hMarginSpin->setValue(spreadHMargin);
    spreadForm->addRow(tr("Horizontal margin:"), m_hMarginSpin);

    m_vMarginSpin = new QSpinBox(m_spreadLayoutGrp);
    m_vMarginSpin->setRange(0, 64);
    m_vMarginSpin->setSuffix(tr(" px"));
    m_vMarginSpin->setValue(spreadVMargin);
    spreadForm->addRow(tr("Vertical margin:"), m_vMarginSpin);

    m_pagesSpin = makeAutoSpin(m_spreadLayoutGrp, spreadPages);
    spreadForm->addRow(tr("Pages:"), m_pagesSpin);

    m_spreadPagesPreview = new QLabel(m_spreadLayoutGrp);
    m_spreadPagesPreview->setWordWrap(true);
    {
        QFont pf = m_spreadPagesPreview->font();
        pf.setItalic(true);
        m_spreadPagesPreview->setFont(pf);
    }
    spreadForm->addRow(QString(), m_spreadPagesPreview);

    layoutTabLay->addWidget(m_spreadLayoutGrp);

    m_spreadColumnInputGrp = new QGroupBox(tr("Column triggers (page-relative)"), layoutTab);
    QVBoxLayout* spreadColLay = new QVBoxLayout(m_spreadColumnInputGrp);
    QLabel* spreadColHint = new QLabel(
        tr("One input per grid slot (0, 1, 2…). The same control activates that slot on "
           "every page when Spread uses multiple pages."), m_spreadColumnInputGrp);
    spreadColHint->setWordWrap(true);
    spreadColLay->addWidget(spreadColHint);
    m_spreadSlotTable = new QTableWidget(m_spreadColumnInputGrp);
    m_spreadSlotTable->setColumnCount(2);
    m_spreadSlotTable->setHorizontalHeaderLabels({ tr("Slot"), tr("Input") });
    m_spreadSlotTable->horizontalHeader()->setStretchLastSection(true);
    m_spreadSlotTable->verticalHeader()->hide();
    m_spreadSlotTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_spreadSlotTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_spreadSlotTable->setMinimumHeight(80);
    spreadColLay->addWidget(m_spreadSlotTable);
    layoutTabLay->addWidget(m_spreadColumnInputGrp);
    connect(m_spreadSlotTable, &QTableWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotSpreadSlotSelectionChanged);
    connect(m_spreadSlotTable, &QTableWidget::itemChanged,
            this, [this](QTableWidgetItem* item) {
        if (!item || item->column() != 1 || !m_spreadSlotTable)
            return;
        const int row = item->row();
        setSpreadSlotInputAt(row, inputFromPatchString(item->text()));
        item->setText(formatInputPatch(spreadSlotInputAt(row), spreadSlotKeyAt(row)));
        updateAllPresetInputCells();
        if (m_spreadSlotEditRow == row && m_presetEntryInputSel)
        {
            m_syncingEntryInputEditor = true;
            m_presetEntryInputSel->setInputSource(spreadSlotInputAt(row));
            m_syncingEntryInputEditor = false;
        }
    });

    auto spreadPreviewHook = [this]() {
        updateSpreadPagesPreview();
        rebuildSpreadSlotTable();
        updateSpreadColumnInputVisibility();
        updateAllPresetInputCells();
    };
    connect(m_colsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, spreadPreviewHook);
    connect(m_rowsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, spreadPreviewHook);
    connect(m_pagesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, spreadPreviewHook);
    connect(m_offAtEndCheck, &QCheckBox::toggled, this, spreadPreviewHook);
    layoutTabLay->addStretch();

    connect(m_layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MultiButtonConfigDialog::slotLayoutChanged);
    slotLayoutChanged(m_layoutCombo->currentIndex());

    tabs->addTab(layoutTab, tr("Layout"));

    // ---- Tab: Automation ------------------------------------------------
    QWidget* autoTab = new QWidget(tabs);
    QVBoxLayout* autoTabLay = new QVBoxLayout(autoTab);

    m_autoEnableCheck = new QCheckBox(tr("Enable automation"), autoTab);
    m_autoEnableCheck->setChecked(automationEnabled);
    autoTabLay->addWidget(m_autoEnableCheck);

    QLabel* autoHint = new QLabel(
        tr("Automation trigger: rising edge only (0→non-zero), not while held. Multiplier skips "
           "pulses (2 = every 2nd trigger). Exclude matrix: tick a cell to skip that preset "
           "for that profile."),
        autoTab);
    autoHint->setWordWrap(true);
    {
        QFont hf = autoHint->font();
        hf.setItalic(true);
        autoHint->setFont(hf);
    }
    autoTabLay->addWidget(autoHint);

    QHBoxLayout* autoProfileRow = new QHBoxLayout;
    m_autoProfileTable = new QTableWidget(autoTab);
    m_autoProfileTable->setColumnCount(6);
    m_autoProfileTable->setHorizontalHeaderLabels(
        { tr("Profile"), tr("Mode"), tr("Jump min"), tr("Jump max"), tr("Multiplier"),
          tr("Beat offset") });
    m_autoProfileTable->horizontalHeader()->setStretchLastSection(false);
    m_autoProfileTable->horizontalHeader()->setMinimumSectionSize(48);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColName, QHeaderView::Stretch);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColMode, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColJumpMin, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColJumpMax, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColMultiplier, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColBeatOffset, QHeaderView::Fixed);
    m_autoProfileTable->verticalHeader()->setVisible(false);
    m_autoProfileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_autoProfileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_autoProfileTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_autoProfileTable->setMinimumHeight(100);
    autoProfileRow->addWidget(m_autoProfileTable, 1);

    QVBoxLayout* autoProfBtnCol = new QVBoxLayout;
    QPushButton* autoAddBtn = new QPushButton(tr("Add"), autoTab);
    QPushButton* autoRemoveBtn = new QPushButton(tr("Remove"), autoTab);
    autoProfBtnCol->addWidget(autoAddBtn);
    autoProfBtnCol->addWidget(autoRemoveBtn);
    autoProfBtnCol->addStretch();
    autoProfileRow->addLayout(autoProfBtnCol);
    autoTabLay->addLayout(autoProfileRow);

    autoTabLay->addWidget(new QLabel(
        tr("Exclude presets (tick = excluded from automation for that profile):"), autoTab));

    m_autoExcludeTable = new QTableWidget(autoTab);
    m_autoExcludeTable->setColumnCount(0);
    m_autoExcludeTable->horizontalHeader()->setStretchLastSection(false);
    m_autoExcludeTable->verticalHeader()->setVisible(true);
    m_autoExcludeTable->verticalHeader()->setDefaultSectionSize(24);
    m_autoExcludeTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_autoExcludeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    autoTabLay->addWidget(m_autoExcludeTable, 1);

    connect(autoAddBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotAutomationAddProfile);
    connect(autoRemoveBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotAutomationRemoveProfile);
    connect(m_autoProfileTable, &QTableWidget::currentCellChanged,
            this, &MultiButtonConfigDialog::slotAutomationProfileRowChanged);
    connect(m_autoExcludeTable, &QTableWidget::itemChanged,
            this, &MultiButtonConfigDialog::slotAutoExcludeItemChanged);
    connect(m_autoExcludeTable->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &MultiButtonConfigDialog::slotExcludeColumnHeaderClicked);
    connect(m_autoExcludeTable->verticalHeader(), &QHeaderView::sectionClicked,
            this, &MultiButtonConfigDialog::slotExcludeRowHeaderClicked);

    rebuildAutomationProfileTable();
    if (m_autoProfileTable->rowCount() > 0)
        m_autoProfileTable->setCurrentCell(qBound(0, m_activeAutomationProfile,
                                                  m_autoProfileTable->rowCount() - 1), 0);

    tabs->addTab(autoTab, tr("Automation"));

    // ---- Tab: Input -----------------------------------------------------
    QWidget* inputTab = new QWidget(tabs);
    QVBoxLayout* inputTabLayout = new QVBoxLayout(inputTab);
    inputTabLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* inputScroll = new QScrollArea(inputTab);
    inputScroll->setWidgetResizable(true);
    inputScroll->setFrameShape(QFrame::NoFrame);
    inputScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* inputScrollContent = new QWidget(inputScroll);
    inputScrollContent->setMinimumWidth(480);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputScrollContent);

    m_receiveInputInactiveFrameCheck = new QCheckBox(
        tr("Receive global inputs when on a hidden frame sub-page"), inputScrollContent);
    m_receiveInputInactiveFrameCheck->setChecked(receiveInputOnInactiveFramePage);
    m_receiveInputInactiveFrameCheck->setToolTip(
        tr("When this Multi Button is on a frame sub-page that is not currently shown, "
           "QLC+ normally disables and hides it. Enable this only if you need cycle trigger, "
           "automation, entry select, commit, and spread inputs to work while the sub-page "
           "is hidden. Otherwise those inputs are ignored until the widget is visible."));
    inputLayout->addWidget(m_receiveInputInactiveFrameCheck);

    m_stageBeforeCommitCheck = new QCheckBox(
        tr("Stage selection — commit required (Operate)"), inputScrollContent);
    m_stageBeforeCommitCheck->setChecked(stageBeforeCommit);
    m_stageBeforeCommitCheck->setToolTip(
        tr("Clicks and entry-select only highlight the chosen preset (white border). "
           "Level DMX and functions apply only after the commit input reaches 255. "
           "Cycle trigger and automation still activate immediately."));
    inputLayout->addWidget(m_stageBeforeCommitCheck);

    QGroupBox* commitInputGrp = new QGroupBox(tr("Commit staged selection (channel value)"),
                                             inputScrollContent);
    QVBoxLayout* commitInputLayout = new QVBoxLayout(commitInputGrp);
    QLabel* commitHint = new QLabel(
        tr("0 = idle (no commit). Value must equal the source Upper feedback (typically 255) "
           "to apply the staged preset. Only used when stage-before-commit is enabled."),
        commitInputGrp);
    commitHint->setWordWrap(true);
    commitInputLayout->addWidget(commitHint);
    m_commitInputSel = new InputSelectionWidget(doc, commitInputGrp);
    m_commitInputSel->setKeyInputVisibility(false);
    m_commitInputSel->setWidgetPage(widgetPage);
    m_commitInputSel->setInputSource(commitSrc);
    commitInputLayout->addWidget(m_commitInputSel);
    inputLayout->addWidget(commitInputGrp);

    QGroupBox* trigGrp = new QGroupBox(tr("Cycle trigger (short press equivalent)"), inputScrollContent);
    QVBoxLayout* trigLayout = new QVBoxLayout(trigGrp);
    m_triggerInputSel = new InputSelectionWidget(doc, trigGrp);
    m_triggerInputSel->setKeyInputVisibility(false);
    m_triggerInputSel->setWidgetPage(widgetPage);
    m_triggerInputSel->setInputSource(triggerSrc);
    trigLayout->addWidget(m_triggerInputSel);
    inputLayout->addWidget(trigGrp);

    QGroupBox* popGrp = new QGroupBox(tr("Popup trigger (long press equivalent)"), inputScrollContent);
    QVBoxLayout* popLayout = new QVBoxLayout(popGrp);
    m_popupInputSel = new InputSelectionWidget(doc, popGrp);
    m_popupInputSel->setKeyInputVisibility(false);
    m_popupInputSel->setWidgetPage(widgetPage);
    m_popupInputSel->setInputSource(popupSrc);
    popLayout->addWidget(m_popupInputSel);
    inputLayout->addWidget(popGrp);

    QGroupBox* autoTrigGrp = new QGroupBox(tr("Automation trigger"), inputScrollContent);
    QVBoxLayout* autoTrigLayout = new QVBoxLayout(autoTrigGrp);
    m_automationInputSel = new InputSelectionWidget(doc, autoTrigGrp);
    m_automationInputSel->setKeyInputVisibility(false);
    m_automationInputSel->setWidgetPage(widgetPage);
    m_automationInputSel->setInputSource(automationSrc);
    autoTrigLayout->addWidget(m_automationInputSel);
    inputLayout->addWidget(autoTrigGrp);

    QGroupBox* presetChooseGrp = new QGroupBox(tr("Automation profile choose (channel value)"), inputScrollContent);
    QVBoxLayout* presetChooseLayout = new QVBoxLayout(presetChooseGrp);
    QLabel* presetHint = new QLabel(
        tr("0 suspends automation. 1 selects Profile 1, 2 selects Profile 2, and so on "
           "(values above the last profile select the last profile). "
           "Inputs use this widget's VC page (frame sub-pages count). "
           "On a hidden frame sub-page, DMX is received only if the option above is enabled."),
        presetChooseGrp);
    presetHint->setWordWrap(true);
    presetChooseLayout->addWidget(presetHint);
    m_presetChooseInputSel = new InputSelectionWidget(doc, presetChooseGrp);
    m_presetChooseInputSel->setKeyInputVisibility(false);
    m_presetChooseInputSel->setWidgetPage(widgetPage);
    m_presetChooseInputSel->setInputSource(presetChooseSrc);
    presetChooseLayout->addWidget(m_presetChooseInputSel);
    inputLayout->addWidget(presetChooseGrp);

    QGroupBox* entrySelectGrp = new QGroupBox(tr("Entry select (scaled knob/fader)"), inputScrollContent);
    QVBoxLayout* entrySelectLayout = new QVBoxLayout(entrySelectGrp);
    QLabel* entrySelectHint = new QLabel(
        tr("Maps the input range (Lower/Upper feedback on the source) evenly across "
           "all entries%1. In Single layout + Operate mode, moving the control opens a "
           "popup menu and highlights the current entry; the menu closes 500 ms after "
           "the last value change.")
            .arg(addOffAtEnd ? tr(" plus OFF") : QString()),
        entrySelectGrp);
    entrySelectHint->setWordWrap(true);
    entrySelectLayout->addWidget(entrySelectHint);
    m_entrySelectInputSel = new InputSelectionWidget(doc, entrySelectGrp);
    m_entrySelectInputSel->setKeyInputVisibility(false);
    m_entrySelectInputSel->setWidgetPage(widgetPage);
    m_entrySelectInputSel->setInputSource(entrySelectSrc);
    entrySelectLayout->addWidget(m_entrySelectInputSel);
    m_entrySelectAutoCommitCheck = new QCheckBox(
        tr("Auto-commit entry select after 500 ms (Single layout popup)"), entrySelectGrp);
    m_entrySelectAutoCommitCheck->setChecked(entrySelectAutoCommit);
    m_entrySelectAutoCommitCheck->setToolTip(
        tr("When enabled, the entry-select popup closes 500 ms after the last input change "
           "and applies the highlighted preset. Disable to avoid accidental commits from "
           "noisy faders; you can still pick from the popup with the mouse."));
    entrySelectLayout->addWidget(m_entrySelectAutoCommitCheck);
    inputLayout->addWidget(entrySelectGrp);

    m_spreadPageInputGrp = new QGroupBox(tr("Spread page select (channel value)"), inputScrollContent);
    QVBoxLayout* spreadPageLayout = new QVBoxLayout(m_spreadPageInputGrp);
    QLabel* spreadPageHint = new QLabel(
        tr("0 = first page, 1 = second page, and so on (0-based page index). "
           "Values above the last page select the last page. Only used when Spread layout "
           "has more than one page."), m_spreadPageInputGrp);
    spreadPageHint->setWordWrap(true);
    spreadPageLayout->addWidget(spreadPageHint);
    m_spreadPageInputSel = new InputSelectionWidget(doc, m_spreadPageInputGrp);
    m_spreadPageInputSel->setKeyInputVisibility(false);
    m_spreadPageInputSel->setWidgetPage(widgetPage);
    m_spreadPageInputSel->setInputSource(spreadPageSrc);
    spreadPageLayout->addWidget(m_spreadPageInputSel);
    inputLayout->addWidget(m_spreadPageInputGrp);

    inputScroll->setWidget(inputScrollContent);
    inputTabLayout->addWidget(inputScroll);
    tabs->addTab(inputTab, tr("Input"));

    root->addWidget(tabs, 1);

    // ---- Buttons --------------------------------------------------------
    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    rebuildList();
    slotSelectionChanged();
    updateSpreadPagesPreview();
    rebuildSpreadSlotTable();
    updateSpreadColumnInputVisibility();
    rebuildWidgetTargetCombo(m_widgetTargetId);
    updateWidgetLiveInputUi();

    syncPresetTableColumns();
    updateChooseChannelsButton();

    slotModeChanged(m_modeCombo->currentIndex());
    slotLevelSelectionChanged();
    updateMonitorTooltip();
}

// ---- Results getters -------------------------------------------------------

static QList<quint32> listWidgetIds(const QListWidget* lw)
{
    QList<quint32> ids;
    for (int i = 0; i < lw->count(); ++i)
        ids.append(lw->item(i)->data(Qt::UserRole).toUInt());
    return ids;
}

static QStringList listWidgetLabels(const QListWidget* lw)
{
    QStringList labels;
    for (int i = 0; i < lw->count(); ++i)
        labels.append(lw->item(i)->data(Qt::UserRole + 1).toString());
    return labels;
}

static QStringList listWidgetIcons(const QListWidget* lw)
{
    QStringList icons;
    for (int i = 0; i < lw->count(); ++i)
        icons.append(lw->item(i)->data(Qt::UserRole + 2).toString());
    return icons;
}

MultiButtonMode MultiButtonConfigDialog::widgetMode() const
{
    return (MultiButtonMode) m_modeCombo->currentData().toInt();
}

QList<quint32> MultiButtonConfigDialog::functionIds() const
{
    return listWidgetIds(m_listWidget);
}

QStringList MultiButtonConfigDialog::functionLabels() const
{
    return listWidgetLabels(m_listWidget);
}

QStringList MultiButtonConfigDialog::iconPaths() const
{
    return listWidgetIcons(m_listWidget);
}

QList<LevelChannelBinding> MultiButtonConfigDialog::levelChannelBindings() const
{
    return m_levelChannelBindings;
}

QList<LevelPreset> MultiButtonConfigDialog::levelPresets() const
{
    QList<LevelPreset> presets;
    int chanCount = m_levelChannelBindings.size();

    for (int row = 0; row < m_levelPresets.size(); ++row)
    {
        LevelPreset preset;
        preset.iconPath = m_levelPresets.value(row).iconPath;
        preset.color    = m_levelPresets.value(row).color;
        preset.labelColor = m_levelPresets.value(row).labelColor;
        preset.flashOnActivate = m_levelPresets.value(row).flashOnActivate;
        preset.flashOverride = m_levelPresets.value(row).flashOverride;
        preset.flashForceLtp = m_levelPresets.value(row).flashForceLtp;
        QTableWidgetItem* nameItem = m_presetTable
            ? m_presetTable->item(row, kPresetNameColumn) : nullptr;
        if (nameItem)
        {
            const QString t = nameItem->text().trimmed();
            const QString def = tr("Preset %1").arg(row + 1);
            if (t.isEmpty())
            {
                preset.hideName = true;
                preset.label.clear();
            }
            else if (t == def)
            {
                preset.hideName = false;
                preset.label.clear();
            }
            else
            {
                preset.hideName = false;
                preset.label = t;
            }
        }
        else
        {
            preset.label    = m_levelPresets.value(row).label;
            preset.hideName = m_levelPresets.value(row).hideName;
        }
        for (int col = 0; col < chanCount; ++col)
        {
            const QString formula = presetCellFormula(row, kPresetFirstDmxColumn + col);
            preset.valueFormulas.append(formula);
            preset.values.append(presetTableValue(row, kPresetFirstDmxColumn + col));
        }
        if (row < m_levelPresets.size())
        {
            preset.entryInput = m_levelPresets.at(row).entryInput;
            preset.entryKey = m_levelPresets.at(row).entryKey;
            preset.entryInputValue = m_levelPresets.at(row).entryInputValue;
        }
        presets.append(preset);
    }
    return presets;
}

QList<LevelPreset> MultiButtonConfigDialog::widgetEntryAppearance() const
{
    QList<LevelPreset> appearance = m_widgetEntryAppearance;
    const int count = entryCountForAutomation();
    while (appearance.size() < count)
        appearance.append(LevelPreset());
    while (appearance.size() > count)
        appearance.removeLast();
    return appearance;
}

QString MultiButtonConfigDialog::presetCellFormula(int row, int col) const
{
    if (col < kPresetFirstDmxColumn || !m_presetTable)
        return QString();

    if (row < 0 || row >= m_presetTable->rowCount() || col >= m_presetTable->columnCount())
        return QString();

    QTableWidgetItem* item = m_presetTable->item(row, col);
    if (!item)
        return QString();

    if (item->data(kDmxFormulaUserRole).isValid())
        return item->data(kDmxFormulaUserRole).toString().trimmed();

    const QString text = item->text().trimmed();
    return mbValueExprLooksLikeFormula(text) ? text : QString();
}

quint8 MultiButtonConfigDialog::presetTableValue(int row, int col) const
{
    if (col < kPresetFirstDmxColumn)
        return 0;

    if (!m_presetTable) return 0;
    if (row < 0 || row >= m_presetTable->rowCount() || col >= m_presetTable->columnCount())
        return 0;

    QTableWidgetItem* item = m_presetTable->item(row, col);
    if (!item) return 0;

    if (!presetCellFormula(row, col).isEmpty())
    {
        if (item->data(Qt::UserRole).isValid())
            return quint8(item->data(Qt::UserRole).toInt());
        return 0;
    }

    if (item->data(Qt::UserRole).isValid())
        return quint8(item->data(Qt::UserRole).toInt());
    return parseDmxCell(item->text());
}

int MultiButtonConfigDialog::longPressMs() const
{
    return m_longPressSpin->value();
}

bool MultiButtonConfigDialog::addOffAtEnd() const
{
    return m_offAtEndCheck ? m_offAtEndCheck->isChecked() : false;
}

bool MultiButtonConfigDialog::monitorChannelValues() const
{
    return m_monitorCheck ? m_monitorCheck->isChecked() : false;
}

bool MultiButtonConfigDialog::receiveInputOnInactiveFramePage() const
{
    return m_receiveInputInactiveFrameCheck
           ? m_receiveInputInactiveFrameCheck->isChecked() : false;
}

MultiButtonLayout MultiButtonConfigDialog::widgetLayout() const
{
    if (!m_layoutCombo)
        return MultiButtonLayout::Single;
    return static_cast<MultiButtonLayout>(m_layoutCombo->currentData().toInt());
}

int MultiButtonConfigDialog::spreadColumns() const
{
    return m_colsSpin ? m_colsSpin->value() : 0;
}

int MultiButtonConfigDialog::spreadRows() const
{
    return m_rowsSpin ? m_rowsSpin->value() : 1;
}

int MultiButtonConfigDialog::spreadHMargin() const
{
    return m_hMarginSpin ? m_hMarginSpin->value() : 4;
}

int MultiButtonConfigDialog::spreadVMargin() const
{
    return m_vMarginSpin ? m_vMarginSpin->value() : 4;
}

int MultiButtonConfigDialog::spreadTileWidth() const
{
    return m_tileWSpin ? m_tileWSpin->value() : 80;
}

int MultiButtonConfigDialog::spreadTileHeight() const
{
    return m_tileHSpin ? m_tileHSpin->value() : 60;
}

int MultiButtonConfigDialog::spreadPages() const
{
    return m_pagesSpin ? m_pagesSpin->value() : 0;
}

bool MultiButtonConfigDialog::automationEnabled() const
{
    return m_autoEnableCheck && m_autoEnableCheck->isChecked();
}

QList<MultiButtonAutomationProfile> MultiButtonConfigDialog::automationProfiles() const
{
    MultiButtonConfigDialog* self = const_cast<MultiButtonConfigDialog*>(this);
    self->syncDataFromProfileTable();
    const int cur = m_autoProfileTable ? m_autoProfileTable->currentRow() : -1;
    syncAllExcludeMasksFromTable(m_autoExcludeTable, self->m_automationProfiles);
    return self->m_automationProfiles;
}

int MultiButtonConfigDialog::activeAutomationProfile() const
{
    if (m_autoProfileTable && m_autoProfileTable->currentRow() >= 0)
        return m_autoProfileTable->currentRow();
    return m_activeAutomationProfile;
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::triggerInputSource() const
{
    return m_triggerInputSel ? m_triggerInputSel->inputSource()
                             : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::popupInputSource() const
{
    return m_popupInputSel ? m_popupInputSel->inputSource()
                           : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::automationInputSource() const
{
    return m_automationInputSel ? m_automationInputSel->inputSource()
                                : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::presetChooseInputSource() const
{
    return m_presetChooseInputSel ? m_presetChooseInputSel->inputSource()
                                  : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::entrySelectInputSource() const
{
    return m_entrySelectInputSel ? m_entrySelectInputSel->inputSource()
                                 : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::spreadPageInputSource() const
{
    return m_spreadPageInputSel ? m_spreadPageInputSel->inputSource()
                                : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::commitInputSource() const
{
    return m_commitInputSel ? m_commitInputSel->inputSource()
                            : QSharedPointer<QLCInputSource>();
}

bool MultiButtonConfigDialog::stageBeforeCommit() const
{
    return m_stageBeforeCommitCheck ? m_stageBeforeCommitCheck->isChecked() : false;
}

bool MultiButtonConfigDialog::entrySelectAutoCommit() const
{
    return m_entrySelectAutoCommitCheck ? m_entrySelectAutoCommitCheck->isChecked() : true;
}

bool MultiButtonConfigDialog::logPresetChanges() const
{
    return false;
}

void MultiButtonConfigDialog::accept()
{
    commitEntryInputEditor();
    syncDataFromProfileTable();
    syncAllExcludeMasksFromTable(m_autoExcludeTable, m_automationProfiles);
    m_activeAutomationProfile = activeAutomationProfile();
    QDialog::accept();
}

QList<QSharedPointer<QLCInputSource>> MultiButtonConfigDialog::functionEntryInputs() const
{
    return m_functionEntryInputs;
}

QList<QKeySequence> MultiButtonConfigDialog::functionEntryKeys() const
{
    return m_functionEntryKeys;
}

QList<int> MultiButtonConfigDialog::functionEntryInputValues() const
{
    return m_functionEntryInputValues;
}

QList<QSharedPointer<QLCInputSource>> MultiButtonConfigDialog::spreadSlotInputs() const
{
    return m_spreadSlotInputs;
}

QList<QKeySequence> MultiButtonConfigDialog::spreadSlotKeys() const
{
    return m_spreadSlotKeys;
}

QList<bool> MultiButtonConfigDialog::functionEntryFlash() const
{
    return m_functionEntryFlash;
}

QList<bool> MultiButtonConfigDialog::functionEntryFlashOverride() const
{
    return m_functionEntryFlashOverride;
}

QList<bool> MultiButtonConfigDialog::functionEntryFlashForceLtp() const
{
    return m_functionEntryFlashForceLtp;
}

QList<QColor> MultiButtonConfigDialog::functionEntryLabelColors() const
{
    return m_functionEntryLabelColors;
}

QString MultiButtonConfigDialog::targetListName() const
{
    return m_targetListNameEdit
            ? m_targetListNameEdit->text().trimmed()
            : m_targetListName.trimmed();
}

quint32 MultiButtonConfigDialog::widgetTargetId() const
{
    if (m_widgetTargetTree)
    {
        QTreeWidgetItem* item = m_widgetTargetTree->currentItem();
        if (item && item->data(0, Qt::UserRole + 1).toBool())
            return item->data(0, Qt::UserRole).toUInt();
    }
    return m_widgetTargetId;
}

int MultiButtonConfigDialog::widgetOutputIndex() const
{
    return m_widgetOutputCombo ? qMax(0, m_widgetOutputCombo->currentData().toInt())
                               : m_widgetOutputIndex;
}

int MultiButtonConfigDialog::widgetParameter() const
{
    return m_widgetParameterCombo ? qMax(0, m_widgetParameterCombo->currentData().toInt())
                                  : m_widgetParameter;
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::widgetLiveInputSource() const
{
    if (m_widgetLiveInputSel && m_widgetLiveInputSel->isEnabled())
        return m_widgetLiveInputSel->inputSource();
    return m_widgetLiveInputSource;
}

MultiButtonWidgetBusPolicy MultiButtonConfigDialog::widgetBusPolicy() const
{
    return MultiButtonWidgetBusPolicy::SharedBus;
}

QString MultiButtonConfigDialog::formatInputPatch(const QSharedPointer<QLCInputSource>& src,
                                                  const QKeySequence& key)
{
    QString text;
    if (!src.isNull() && src->isValid())
        text = QString("%1.%2").arg(src->universe() + 1).arg(src->channel() + 1);
    if (!key.isEmpty())
    {
        if (!text.isEmpty())
            text += QLatin1Char(' ');
        text += key.toString(QKeySequence::PortableText);
    }
    return text;
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::inputFromPatchString(const QString& patch)
{
    const QStringList list = patch.trimmed().split('.');
    if (list.size() != 2)
        return QSharedPointer<QLCInputSource>();

    bool okU = false, okC = false;
    const quint32 universe = list.at(0).toUInt(&okU);
    const quint32 channel  = list.at(1).toUInt(&okC);
    if (!okU || !okC || universe < 1 || channel < 1)
        return QSharedPointer<QLCInputSource>();

    return QSharedPointer<QLCInputSource>(new QLCInputSource(universe - 1, channel - 1));
}

int MultiButtonConfigDialog::dialogSlotsPerPage() const
{
    const int total = entryCountForAutomation() + (addOffAtEnd() ? 1 : 0);
    if (spreadColumns() > 0 && spreadRows() > 0)
    {
        return spreadColumns() * spreadRows();
    }
    if (spreadColumns() > 0 && spreadRows() <= 0)
    {
        if (spreadPages() > 0)
            return qMax(1, (total + spreadPages() - 1) / spreadPages());
        return qMax(1, total);
    }
    if (spreadPages() > 0)
        return qMax(1, (total + spreadPages() - 1) / spreadPages());
    return qMax(1, total);
}

bool MultiButtonConfigDialog::dialogSpreadPagingActive() const
{
    if (widgetLayout() != MultiButtonLayout::Spread)
        return false;

    int total = entryCountForAutomation();
    if (addOffAtEnd())
        ++total;
    const int spp = dialogSlotsPerPage();
    if (total <= 0 || spp <= 0)
        return false;

    int pages = qMax(1, (total + spp - 1) / spp);
    if (spreadPages() > 0)
        pages = qMax(spreadPages(), pages);
    return pages > 1;
}

int MultiButtonConfigDialog::spreadLocalSlotForRow(int row) const
{
    if (row < 0)
        return -1;
    if (!dialogSpreadPagingActive())
        return row;
    const int spp = dialogSlotsPerPage();
    if (spp <= 0)
        return row;
    return row % spp;
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::spreadSlotInputAt(int localSlot) const
{
    if (localSlot < 0 || localSlot >= m_spreadSlotInputs.size())
        return QSharedPointer<QLCInputSource>();
    return m_spreadSlotInputs.at(localSlot);
}

void MultiButtonConfigDialog::setSpreadSlotInputAt(int localSlot, QSharedPointer<QLCInputSource> src)
{
    if (localSlot < 0)
        return;
    while (m_spreadSlotInputs.size() <= localSlot)
        m_spreadSlotInputs.append(QSharedPointer<QLCInputSource>());
    m_spreadSlotInputs[localSlot] = src;
}

QKeySequence MultiButtonConfigDialog::spreadSlotKeyAt(int localSlot) const
{
    if (localSlot < 0 || localSlot >= m_spreadSlotKeys.size())
        return QKeySequence();
    return m_spreadSlotKeys.at(localSlot);
}

void MultiButtonConfigDialog::setSpreadSlotKeyAt(int localSlot, const QKeySequence& key)
{
    if (localSlot < 0)
        return;
    while (m_spreadSlotKeys.size() <= localSlot)
        m_spreadSlotKeys.append(QKeySequence());
    m_spreadSlotKeys[localSlot] = VCWidget::stripKeySequence(key);
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::entryInputForRow(int row) const
{
    if (row < 0)
        return QSharedPointer<QLCInputSource>();

    if (dialogSpreadPagingActive())
        return spreadSlotInputAt(spreadLocalSlotForRow(row));

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (row >= m_levelPresets.size())
            return QSharedPointer<QLCInputSource>();
        return m_levelPresets.at(row).entryInput;
    }
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (row >= m_widgetEntryAppearance.size())
            return QSharedPointer<QLCInputSource>();
        return m_widgetEntryAppearance.at(row).entryInput;
    }

    if (row >= m_functionEntryInputs.size())
        return QSharedPointer<QLCInputSource>();
    return m_functionEntryInputs.at(row);
}

void MultiButtonConfigDialog::setEntryInputForRow(int row, QSharedPointer<QLCInputSource> src)
{
    if (row < 0)
        return;

    if (dialogSpreadPagingActive())
    {
        setSpreadSlotInputAt(spreadLocalSlotForRow(row), src);
        return;
    }

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (row >= m_levelPresets.size())
            return;
        m_levelPresets[row].entryInput = src;
    }
    else if (widgetMode() == MultiButtonMode::Widget)
    {
        while (m_widgetEntryAppearance.size() <= row)
            m_widgetEntryAppearance.append(LevelPreset());
        m_widgetEntryAppearance[row].entryInput = src;
    }
    else
    {
        while (m_functionEntryInputs.size() <= row)
            m_functionEntryInputs.append(QSharedPointer<QLCInputSource>());
        m_functionEntryInputs[row] = src;
    }
}

QKeySequence MultiButtonConfigDialog::entryKeyForRow(int row) const
{
    if (row < 0)
        return QKeySequence();

    if (dialogSpreadPagingActive())
        return spreadSlotKeyAt(spreadLocalSlotForRow(row));

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (row >= m_levelPresets.size())
            return QKeySequence();
        return m_levelPresets.at(row).entryKey;
    }
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (row >= m_widgetEntryAppearance.size())
            return QKeySequence();
        return m_widgetEntryAppearance.at(row).entryKey;
    }

    if (row >= m_functionEntryKeys.size())
        return QKeySequence();
    return m_functionEntryKeys.at(row);
}

void MultiButtonConfigDialog::setEntryKeyForRow(int row, const QKeySequence& key)
{
    if (row < 0)
        return;

    const QKeySequence stripped = VCWidget::stripKeySequence(key);

    if (dialogSpreadPagingActive())
    {
        setSpreadSlotKeyAt(spreadLocalSlotForRow(row), stripped);
        return;
    }

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (row >= m_levelPresets.size())
            return;
        m_levelPresets[row].entryKey = stripped;
    }
    else if (widgetMode() == MultiButtonMode::Widget)
    {
        while (m_widgetEntryAppearance.size() <= row)
            m_widgetEntryAppearance.append(LevelPreset());
        m_widgetEntryAppearance[row].entryKey = stripped;
    }
    else
    {
        while (m_functionEntryKeys.size() <= row)
            m_functionEntryKeys.append(QKeySequence());
        m_functionEntryKeys[row] = stripped;
    }
}

int MultiButtonConfigDialog::entryInputValueForRow(int row) const
{
    if (row < 0 || dialogSpreadPagingActive())
        return -1;

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (row >= m_levelPresets.size())
            return -1;
        return m_levelPresets.at(row).entryInputValue;
    }
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (row >= m_widgetEntryAppearance.size())
            return -1;
        return m_widgetEntryAppearance.at(row).entryInputValue;
    }

    if (row >= m_functionEntryInputValues.size())
        return -1;
    return m_functionEntryInputValues.at(row);
}

void MultiButtonConfigDialog::setEntryInputValueForRow(int row, int value)
{
    if (row < 0 || dialogSpreadPagingActive())
        return;

    const int normalized = value < 0 ? -1 : qBound(0, value, 255);

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (row >= m_levelPresets.size())
            return;
        m_levelPresets[row].entryInputValue = normalized;
    }
    else if (widgetMode() == MultiButtonMode::Widget)
    {
        while (m_widgetEntryAppearance.size() <= row)
            m_widgetEntryAppearance.append(LevelPreset());
        m_widgetEntryAppearance[row].entryInputValue = normalized;
    }
    else
    {
        while (m_functionEntryInputValues.size() <= row)
            m_functionEntryInputValues.append(-1);
        m_functionEntryInputValues[row] = normalized;
    }
}

void MultiButtonConfigDialog::commitEntryInputEditor()
{
    if (!m_presetEntryInputSel || m_syncingEntryInputEditor)
        return;

    const QSharedPointer<QLCInputSource> src = m_presetEntryInputSel->inputSource();
    const QKeySequence key = VCWidget::stripKeySequence(m_presetEntryInputSel->keySequence());

    if (m_spreadSlotEditRow >= 0)
    {
        setSpreadSlotInputAt(m_spreadSlotEditRow, src);
        setSpreadSlotKeyAt(m_spreadSlotEditRow, key);
        rebuildSpreadSlotTable();
        updateAllPresetInputCells();
        return;
    }

    if (m_entryInputEditRow < 0)
        return;

    setEntryInputForRow(m_entryInputEditRow, src);
    setEntryKeyForRow(m_entryInputEditRow, key);
    setEntryInputValueForRow(m_entryInputEditRow,
                             m_entryInputValueCheck && m_entryInputValueCheck->isChecked()
                                     ? m_entryInputValueSpin->value()
                                     : -1);
    updatePresetInputCell(m_entryInputEditRow);
    if (dialogSpreadPagingActive())
        updateAllPresetInputCells();
}

void MultiButtonConfigDialog::loadEntryInputEditor(int row)
{
    if (!m_presetEntryInputSel)
        return;

    m_syncingEntryInputEditor = true;
    m_entryInputEditRow = row;
    m_spreadSlotEditRow = -1;
    m_presetEntryInputSel->setInputSource(entryInputForRow(row));
    m_presetEntryInputSel->setKeySequence(entryKeyForRow(row));
    const int inputValue = entryInputValueForRow(row);
    if (m_entryInputValueCheck && m_entryInputValueSpin)
    {
        QSignalBlocker checkBlocker(m_entryInputValueCheck);
        QSignalBlocker spinBlocker(m_entryInputValueSpin);
        m_entryInputValueCheck->setChecked(inputValue >= 0);
        m_entryInputValueSpin->setEnabled(inputValue >= 0);
        m_entryInputValueSpin->setValue(inputValue >= 0 ? inputValue : 0);
    }
    m_syncingEntryInputEditor = false;
}

void MultiButtonConfigDialog::updatePresetInputCell(int row)
{
    QTableWidget* table = widgetMode() == MultiButtonMode::Widget
            ? m_widgetAppearanceTable : m_presetTable;
    if (!table || row < 0 || row >= entryCountForAutomation()
            || row >= table->rowCount())
        return;

    m_rebuildingPresetTable = true;
    QTableWidgetItem* item = table->item(row, kPresetInputColumn);
    if (!item)
    {
        item = new QTableWidgetItem;
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        table->setItem(row, kPresetInputColumn, item);
    }
    QString text = formatInputPatch(entryInputForRow(row), entryKeyForRow(row));
    const int inputValue = entryInputValueForRow(row);
    if (inputValue >= 0 && !entryInputForRow(row).isNull()
            && entryInputForRow(row)->isValid())
    {
        text += tr(" = %1").arg(inputValue);
    }
    item->setText(text);
    if (dialogSpreadPagingActive())
    {
        item->setToolTip(tr("Shared grid slot %1 on all pages")
                             .arg(spreadLocalSlotForRow(row)));
    }
    else
    {
        item->setToolTip(QString());
    }
    m_rebuildingPresetTable = false;
}

void MultiButtonConfigDialog::updateAllPresetInputCells()
{
    for (int r = 0; r < entryCountForAutomation(); ++r)
        updatePresetInputCell(r);
}

void MultiButtonConfigDialog::rebuildSpreadSlotTable()
{
    if (!m_spreadSlotTable)
        return;

    const int spp = dialogSpreadPagingActive() ? dialogSlotsPerPage() : 0;
    while (m_spreadSlotInputs.size() < spp)
        m_spreadSlotInputs.append(QSharedPointer<QLCInputSource>());
    while (m_spreadSlotKeys.size() < spp)
        m_spreadSlotKeys.append(QKeySequence());

    m_spreadSlotTable->blockSignals(true);
    m_spreadSlotTable->setRowCount(spp);
    for (int s = 0; s < spp; ++s)
    {
        QTableWidgetItem* slotItem = m_spreadSlotTable->item(s, 0);
        if (!slotItem)
        {
            slotItem = new QTableWidgetItem(QString::number(s));
            slotItem->setFlags(slotItem->flags() & ~Qt::ItemIsEditable);
            m_spreadSlotTable->setItem(s, 0, slotItem);
        }
        else
        {
            slotItem->setText(QString::number(s));
        }

        QTableWidgetItem* inItem = m_spreadSlotTable->item(s, 1);
        if (!inItem)
        {
            inItem = new QTableWidgetItem;
            inItem->setFlags(inItem->flags() | Qt::ItemIsEditable);
            m_spreadSlotTable->setItem(s, 1, inItem);
        }
        inItem->setText(formatInputPatch(spreadSlotInputAt(s), spreadSlotKeyAt(s)));
    }
    m_spreadSlotTable->blockSignals(false);
}

void MultiButtonConfigDialog::updateSpreadColumnInputVisibility()
{
    const bool spread = (widgetLayout() == MultiButtonLayout::Spread);
    const bool paging = dialogSpreadPagingActive();
    if (m_spreadColumnInputGrp)
        m_spreadColumnInputGrp->setVisible(spread && paging);
}

void MultiButtonConfigDialog::slotSpreadSlotSelectionChanged()
{
    if (!m_spreadSlotTable || !m_presetEntryInputSel)
        return;

    commitEntryInputEditor();

    const int row = m_spreadSlotTable->currentRow();
    m_syncingEntryInputEditor = true;
    m_spreadSlotEditRow = row;
    m_entryInputEditRow = -1;
    if (row >= 0)
    {
        m_presetEntryInputSel->setInputSource(spreadSlotInputAt(row));
        m_presetEntryInputSel->setKeySequence(spreadSlotKeyAt(row));
    }
    else
    {
        m_presetEntryInputSel->setInputSource(QSharedPointer<QLCInputSource>());
        m_presetEntryInputSel->setKeySequence(QKeySequence());
    }
    m_syncingEntryInputEditor = false;
}

// ---- Mode switch -----------------------------------------------------------

void MultiButtonConfigDialog::slotModeChanged(int index)
{
    m_modeStack->setCurrentIndex(index);
    if (widgetMode() == MultiButtonMode::Widget)
        rebuildWidgetPreview();
    if (m_entryInputGrp)
        m_entryInputGrp->setVisible(true);
    updateMonitorTooltip();
    if (!m_syncingAutomationUi)
        rebuildAutomationExcludeTable();
    updateSpreadPagesPreview();
    rebuildSpreadSlotTable();
    updateSpreadColumnInputVisibility();

    if (widgetMode() == MultiButtonMode::Function)
        slotSelectionChanged();
    else
        slotLevelSelectionChanged();
}

PresetTableV2MultiButtonTargetIface* MultiButtonConfigDialog::selectedWidgetTarget() const
{
    VirtualConsole* vc = VirtualConsole::instance();
    if (!vc)
        return nullptr;
    return qobject_cast<PresetTableV2MultiButtonTargetIface*>(vc->widget(widgetTargetId()));
}

void MultiButtonConfigDialog::rebuildWidgetTargetCombo(quint32 preferredId)
{
    if (!m_widgetTargetTree)
        return;

    const QSignalBlocker blocker(m_widgetTargetTree);
    m_widgetTargetTree->clear();

    QTreeWidgetItem* noneItem = new QTreeWidgetItem(m_widgetTargetTree);
    noneItem->setText(0, tr("None"));
    noneItem->setData(0, Qt::UserRole, VCWidget::invalidId());
    noneItem->setData(0, Qt::UserRole + 1, true);
    noneItem->setData(0, Qt::UserRole + 2, tr("none"));

    QMap<QString, QTreeWidgetItem*> groups;
    auto groupForType = [this, &groups](const QString& typeLabel) {
        QTreeWidgetItem* group = groups.value(typeLabel, nullptr);
        if (!group)
        {
            group = new QTreeWidgetItem(m_widgetTargetTree);
            group->setText(0, typeLabel);
            group->setData(0, Qt::UserRole + 1, false);
            group->setData(0, Qt::UserRole + 2, typeLabel);
            group->setFlags(Qt::ItemIsEnabled);
            groups.insert(typeLabel, group);
        }
        return group;
    };

    VirtualConsole* vc = VirtualConsole::instance();
    VCFrame* root = vc ? vc->contents() : nullptr;
    if (root)
    {
        const QList<VCWidget*> widgets =
            root->findChildren<VCWidget*>(QString(), Qt::FindChildrenRecursively);
        for (VCWidget* widget : widgets)
        {
            if (!widget || widget->id() == m_ownerWidgetId)
                continue;
            if (!qobject_cast<PresetTableV2MultiButtonTargetIface*>(widget))
                continue;
            const QString typeLabel = widgetTargetTypeLabel(widget);
            QString caption;
            if (const MultiButtonWidget* mb = qobject_cast<const MultiButtonWidget*>(widget))
                caption = mb->targetDisplayName();
            if (caption.isEmpty())
            {
                caption = widget->caption().isEmpty()
                        ? QString::fromLatin1(widget->metaObject()->className())
                        : widget->caption();
            }
            QTreeWidgetItem* item = new QTreeWidgetItem(groupForType(typeLabel));
            item->setText(0, tr("%1 (#%2)").arg(caption).arg(widget->id()));
            item->setData(0, Qt::UserRole, widget->id());
            item->setData(0, Qt::UserRole + 1, true);
            item->setData(0, Qt::UserRole + 2,
                          QStringList{typeLabel, caption, QString::number(widget->id())}
                                  .join(QLatin1Char(' ')));
        }
    }

    m_widgetTargetTree->expandAll();
    filterWidgetTargetTree();
    selectWidgetTargetTreeItem(preferredId);
    if (!m_widgetTargetTree->currentItem())
        selectWidgetTargetTreeItem(VCWidget::invalidId());

    rebuildWidgetOutputCombo();
    rebuildWidgetParameterCombo();
    rebuildWidgetPreview();
}

QString MultiButtonConfigDialog::widgetTargetTypeLabel(const VCWidget* widget) const
{
    if (!widget)
        return tr("Other");
    if (qobject_cast<const MultiButtonWidget*>(widget))
        return tr("Multi Button");

    const QString className = QString::fromLatin1(widget->metaObject()->className());
    if (className.contains(QStringLiteral("PresetTableV2"), Qt::CaseInsensitive))
        return tr("Preset Table v2");

    const QString typeName = VCWidget::typeToString(const_cast<VCWidget*>(widget)->type());
    return typeName.isEmpty() ? className : typeName;
}

void MultiButtonConfigDialog::selectWidgetTargetTreeItem(quint32 widgetId)
{
    if (!m_widgetTargetTree)
        return;

    QList<QTreeWidgetItem*> stack;
    for (int i = 0; i < m_widgetTargetTree->topLevelItemCount(); ++i)
        stack.append(m_widgetTargetTree->topLevelItem(i));

    while (!stack.isEmpty())
    {
        QTreeWidgetItem* item = stack.takeFirst();
        if (!item)
            continue;
        if (item->data(0, Qt::UserRole + 1).toBool()
                && item->data(0, Qt::UserRole).toUInt() == widgetId)
        {
            m_widgetTargetTree->setCurrentItem(item);
            item->setSelected(true);
            return;
        }
        for (int c = 0; c < item->childCount(); ++c)
            stack.append(item->child(c));
    }
}

void MultiButtonConfigDialog::filterWidgetTargetTree()
{
    if (!m_widgetTargetTree)
        return;

    const QString needle = m_widgetTargetSearch
            ? m_widgetTargetSearch->text().trimmed().toLower() : QString();

    for (int i = 0; i < m_widgetTargetTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* top = m_widgetTargetTree->topLevelItem(i);
        if (!top)
            continue;

        if (top->data(0, Qt::UserRole + 1).toBool())
        {
            const QString hay = top->data(0, Qt::UserRole + 2).toString().toLower();
            top->setHidden(!needle.isEmpty() && !hay.contains(needle));
            continue;
        }

        bool anyChildVisible = false;
        const QString groupHay = top->data(0, Qt::UserRole + 2).toString().toLower();
        const bool groupMatches = needle.isEmpty() || groupHay.contains(needle);
        for (int c = 0; c < top->childCount(); ++c)
        {
            QTreeWidgetItem* child = top->child(c);
            const QString hay = child->data(0, Qt::UserRole + 2).toString().toLower();
            const bool visible = needle.isEmpty() || groupMatches || hay.contains(needle);
            child->setHidden(!visible);
            anyChildVisible = anyChildVisible || visible;
        }
        top->setHidden(!anyChildVisible);
        if (anyChildVisible)
            top->setExpanded(true);
    }
}

void MultiButtonConfigDialog::rebuildWidgetOutputCombo()
{
    if (!m_widgetOutputCombo)
        return;

    const int preferred = m_widgetOutputIndex;
    const QSignalBlocker blocker(m_widgetOutputCombo);
    m_widgetOutputCombo->clear();

    PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget();
    const int count = target ? target->multiButtonOutputCount() : 0;
    for (int i = 0; i < count; ++i)
    {
        QString name = target->multiButtonOutputName(i);
        if (name.isEmpty())
            name = tr("Output %1").arg(i + 1);
        m_widgetOutputCombo->addItem(name, i);
    }
    m_widgetOutputCombo->setEnabled(count > 0);

    int index = m_widgetOutputCombo->findData(preferred);
    if (index < 0 && count > 0)
        index = 0;
    if (index >= 0)
        m_widgetOutputCombo->setCurrentIndex(index);
}

void MultiButtonConfigDialog::rebuildWidgetParameterCombo()
{
    if (!m_widgetParameterCombo)
        return;

    const int preferred = m_widgetParameter;
    const QSignalBlocker blocker(m_widgetParameterCombo);
    m_widgetParameterCombo->clear();

    PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget();
    const int count = target ? target->multiButtonParameterCount() : 0;
    for (int i = 0; i < count; ++i)
    {
        QString name = target->multiButtonParameterName(i);
        if (name.isEmpty())
            name = tr("Parameter %1").arg(i + 1);
        m_widgetParameterCombo->addItem(name, i);
    }
    m_widgetParameterCombo->setEnabled(count > 0);

    int index = m_widgetParameterCombo->findData(preferred);
    if (index < 0 && count > 0)
        index = 0;
    if (index >= 0)
        m_widgetParameterCombo->setCurrentIndex(index);
}

void MultiButtonConfigDialog::rebuildWidgetPreview()
{
    if (!m_widgetPreviewList)
        return;

    m_widgetPreviewList->clear();
    PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget();
    if (!target)
    {
        QListWidgetItem* item = new QListWidgetItem(tr("Choose a linked widget."),
                                                    m_widgetPreviewList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        return;
    }

    const int outputIdx = widgetOutputIndex();
    const int parameter = widgetParameter();
    const int count = target->multiButtonEntryCount(outputIdx, parameter);
    while (m_widgetEntryAppearance.size() < count)
        m_widgetEntryAppearance.append(LevelPreset());
    while (m_widgetEntryAppearance.size() > count)
        m_widgetEntryAppearance.removeLast();
    for (int i = 0; i < count; ++i)
    {
        QString name = target->multiButtonEntryName(outputIdx, parameter, i);
        if (name.isEmpty())
            name = tr("Preset %1").arg(i + 1);
        QListWidgetItem* item = new QListWidgetItem(tr("%1. %2").arg(i + 1).arg(name),
                                                    m_widgetPreviewList);
        item->setData(Qt::UserRole, i);
    }
    if (count == 0)
    {
        QListWidgetItem* item = new QListWidgetItem(tr("No linked entries available."),
                                                    m_widgetPreviewList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
    syncPresetTableColumns();
    updateAllPresetInputCells();
}

void MultiButtonConfigDialog::updateWidgetLiveInputUi()
{
    if (!m_widgetLiveInputSel || !m_widgetLiveInputStatus)
        return;

    const QString status = tr("Raw selector channel shared with cue/snapshot.");
    const QString details = tr("One raw DMX channel (not a patched fixture intensity channel) shared with cuelist/snapshots. "
                               "Multi Button and cues take turns via LTP: last action wins. Button click holds staged over a held cue; "
                               "starting a cue adopts into Preset Table. Not wired into Preset Table inputs.");
    m_widgetLiveInputStatus->setText(status);
    m_widgetLiveInputStatus->setToolTip(details);
    m_widgetLiveInputSel->setEnabled(true);
    m_widgetLiveInputSel->setInputSource(m_widgetLiveInputSource);
}

void MultiButtonConfigDialog::slotWidgetTargetChanged()
{
    if (m_widgetTargetTree)
    {
        QTreeWidgetItem* item = m_widgetTargetTree->currentItem();
        if (!item || !item->data(0, Qt::UserRole + 1).toBool())
            return;
    }
    m_widgetTargetId = widgetTargetId();
    rebuildWidgetOutputCombo();
    rebuildWidgetParameterCombo();
    rebuildWidgetPreview();
    updateWidgetLiveInputUi();
    rebuildAutomationExcludeTable();
    updateSpreadPagesPreview();
    rebuildSpreadSlotTable();
    updateSpreadColumnInputVisibility();
}

void MultiButtonConfigDialog::slotWidgetTargetSearchChanged(const QString&)
{
    filterWidgetTargetTree();
}

void MultiButtonConfigDialog::slotWidgetOutputChanged(int)
{
    m_widgetOutputIndex = widgetOutputIndex();
    rebuildWidgetPreview();
    updateWidgetLiveInputUi();
    rebuildAutomationExcludeTable();
    updateSpreadPagesPreview();
    rebuildSpreadSlotTable();
    updateSpreadColumnInputVisibility();
}

void MultiButtonConfigDialog::slotWidgetParameterChanged(int)
{
    m_widgetParameter = widgetParameter();
    rebuildWidgetPreview();
    updateWidgetLiveInputUi();
    rebuildAutomationExcludeTable();
    updateSpreadPagesPreview();
    rebuildSpreadSlotTable();
    updateSpreadColumnInputVisibility();
}

void MultiButtonConfigDialog::slotLayoutChanged(int index)
{
    const bool spread = (index == 1);
    if (m_spreadLayoutGrp)
        m_spreadLayoutGrp->setEnabled(spread);
    if (m_singleLayoutGrp)
        m_singleLayoutGrp->setEnabled(!spread);
    if (m_longPressSpin)
        m_longPressSpin->setEnabled(!spread);
    if (m_spreadPageInputGrp)
        m_spreadPageInputGrp->setVisible(spread);
    updateSpreadPagesPreview();
    rebuildSpreadSlotTable();
    updateSpreadColumnInputVisibility();
    updateAllPresetInputCells();
}

void MultiButtonConfigDialog::updateSpreadPagesPreview()
{
    if (!m_spreadPagesPreview)
        return;

    const int entries = entryCountForAutomation();
    const int totalSlots = entries + (addOffAtEnd() ? 1 : 0);

    int slotsPerPage = 1;
    if (spreadColumns() > 0 && spreadRows() > 0)
    {
        slotsPerPage = spreadColumns() * spreadRows();
    }
    else if (spreadColumns() > 0 && spreadRows() <= 0)
    {
        if (spreadPages() > 0)
            slotsPerPage = qMax(1, (totalSlots + spreadPages() - 1) / spreadPages());
        else
            slotsPerPage = qMax(1, totalSlots);
    }
    else if (spreadPages() > 0)
        slotsPerPage = qMax(1, (totalSlots + spreadPages() - 1) / spreadPages());
    else
        slotsPerPage = qMax(1, totalSlots);

    int pageCount = 1;
    if (totalSlots > 0 && slotsPerPage > 0)
        pageCount = qMax(1, (totalSlots + slotsPerPage - 1) / slotsPerPage);
    if (spreadPages() > 0)
        pageCount = qMax(pageCount, spreadPages());

    if (widgetLayout() != MultiButtonLayout::Spread || totalSlots == 0)
    {
        m_spreadPagesPreview->setText(QString());
        return;
    }

    m_spreadPagesPreview->setText(
        tr("%1 page(s) · %2 preset(s)/page · %3 total slot(s)")
            .arg(pageCount)
            .arg(slotsPerPage)
            .arg(totalSlots));
}

void MultiButtonConfigDialog::updateMonitorTooltip()
{
    if (!m_monitorCheck) return;

    if (widgetMode() == MultiButtonMode::Function)
    {
        m_monitorCheck->setToolTip(
            tr("When enabled, the button automatically highlights the entry whose Scene matches\n"
               "the current DMX output. Only applies to Scene entries."));
    }
    else
    {
        m_monitorCheck->setToolTip(
            tr("When enabled, the button automatically highlights the preset whose channel values\n"
               "match the current DMX output on the selected channels."));
    }
}

// ---- Level helpers ---------------------------------------------------------

quint64 MultiButtonConfigDialog::bindingKey(quint32 fixtureId, quint32 channel)
{
    return (quint64(fixtureId) << 32) | quint64(channel);
}

QString MultiButtonConfigDialog::bindingHeaderLabel(Doc* doc, const LevelChannelBinding& b)
{
    Fixture* fxi = doc->fixture(b.fixtureId);
    const QLCChannel* ch = fxi ? fxi->channel(b.channel) : nullptr;
    QString fxName = fxi ? fxi->name() : tr("Fixture %1").arg(b.fixtureId);
    QString chName = ch ? ch->name() : tr("Ch %1").arg(b.channel + 1);
    return QString("%1 / %2").arg(fxName, chName);
}

quint8 MultiButtonConfigDialog::parseDmxCell(const QString& text)
{
    return quint8(qBound(0, text.trimmed().toInt(), 255));
}

QTableWidgetItem* MultiButtonConfigDialog::makeValueTableItem(quint8 value)
{
    QTableWidgetItem* item = new QTableWidgetItem(QString::number(value));
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setData(Qt::UserRole, int(value));
    return item;
}

QTableWidgetItem* MultiButtonConfigDialog::makePresetValueTableItem(const LevelPreset& preset,
                                                                    int valCol)
{
    if (valCol >= 0 && valCol < preset.valueFormulas.size()
        && !preset.valueFormulas.at(valCol).trimmed().isEmpty())
    {
        const QString formula = preset.valueFormulas.at(valCol).trimmed();
        QTableWidgetItem* item = new QTableWidgetItem(formula);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(kDmxFormulaUserRole, formula);
        QFont f = item->font();
        f.setItalic(true);
        item->setFont(f);
        const quint8 fallback = valCol < preset.values.size() ? preset.values.at(valCol) : 0;
        item->setData(Qt::UserRole, int(fallback));
        item->setToolTip(tr("Formula. Syntax: IF(u1.ch2 >= 128, 255, 0) — ch is 1-based (patch 1.2)"));
        return item;
    }

    const quint8 value = valCol < preset.values.size() ? preset.values.at(valCol) : 0;
    return makeValueTableItem(value);
}

void MultiButtonConfigDialog::updateChooseChannelsButton()
{
    if (!m_chooseChannelsBtn) return;

    if (m_levelChannelBindings.isEmpty())
    {
        m_chooseChannelsBtn->setText(tr("Choose channels…"));
        m_chooseChannelsBtn->setToolTip(tr("Open the channel selection dialog."));
        return;
    }

    m_chooseChannelsBtn->setText(tr("Channels (%1)…").arg(m_levelChannelBindings.size()));

    QStringList parts;
    for (const LevelChannelBinding& b : m_levelChannelBindings)
    {
        Fixture* fxi = m_doc->fixture(b.fixtureId);
        const QLCChannel* ch = fxi ? fxi->channel(b.channel) : nullptr;
        parts << QString("%1 / %2")
                 .arg(fxi ? fxi->name() : tr("?"))
                 .arg(ch ? ch->name() : tr("Ch %1").arg(b.channel + 1));
    }

    m_chooseChannelsBtn->setToolTip(parts.join("\n"));
}

void MultiButtonConfigDialog::slotChooseChannels()
{
    m_levelPresets = levelPresets();

    ChannelsSelection cs(m_doc, this);
    QList<SceneValue> current;
    for (const LevelChannelBinding& b : m_levelChannelBindings)
        current.append(SceneValue(b.fixtureId, b.channel, 0));
    cs.setChannelsList(current);

    if (cs.exec() != QDialog::Accepted)
        return;

    QList<QHash<quint64, quint8>> perRowOld;
    QList<QHash<quint64, QString>> perRowFormulas;
    perRowOld.resize(m_levelPresets.size());
    perRowFormulas.resize(m_levelPresets.size());
    for (int row = 0; row < m_levelPresets.size(); ++row)
    {
        const LevelPreset& preset = m_levelPresets.at(row);
        for (int col = 0; col < m_levelChannelBindings.size() && col < preset.values.size(); ++col)
        {
            const LevelChannelBinding& b = m_levelChannelBindings.at(col);
            const quint64 key = bindingKey(b.fixtureId, b.channel);
            perRowOld[row].insert(key, preset.values.at(col));
            if (col < preset.valueFormulas.size())
                perRowFormulas[row].insert(key, preset.valueFormulas.at(col));
        }
    }

    m_levelChannelBindings.clear();
    for (const SceneValue& sv : cs.channelsList())
    {
        LevelChannelBinding b;
        b.fixtureId = sv.fxi;
        b.channel   = sv.channel;
        m_levelChannelBindings.append(b);
    }

    for (int row = 0; row < m_levelPresets.size(); ++row)
    {
        QList<quint8> newValues;
        QStringList newFormulas;
        for (const LevelChannelBinding& b : m_levelChannelBindings)
        {
            const quint64 key = bindingKey(b.fixtureId, b.channel);
            newValues.append(perRowOld[row].value(key, 0));
            newFormulas.append(perRowFormulas[row].value(key, QString()));
        }
        m_levelPresets[row].values = newValues;
        m_levelPresets[row].valueFormulas = newFormulas;
    }

    syncPresetTableColumns();
    updateChooseChannelsButton();
}

static QIcon thumbIcon(const QString& path)
{
    if (path.isEmpty()) return QIcon();
    QPixmap px(path);
    if (px.isNull()) return QIcon();
    return QIcon(px.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MultiButtonConfigDialog::syncPresetNameFromCell(int row, const QString& cellText)
{
    QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (row < 0 || row >= presets.size())
        return;

    LevelPreset& preset = presets[row];
    const QString t = cellText.trimmed();
    QString def = tr("Preset %1").arg(row + 1);
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget())
        {
            const QString linked = target->multiButtonEntryName(
                    widgetOutputIndex(), widgetParameter(), row);
            if (!linked.isEmpty())
                def = linked;
        }
    }

    if (t.isEmpty())
    {
        preset.hideName = true;
        preset.label.clear();
    }
    else if (t == def)
    {
        preset.hideName = false;
        preset.label.clear();
    }
    else
    {
        preset.hideName = false;
        preset.label = t;
    }
}

QString MultiButtonConfigDialog::presetNameCellText(int row) const
{
    const QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (row < 0 || row >= presets.size())
        return QString();

    const LevelPreset& preset = presets.at(row);
    if (preset.hideName)
        return QString();
    if (!preset.label.isEmpty())
        return preset.label;
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget())
        {
            const QString linked = target->multiButtonEntryName(
                    widgetOutputIndex(), widgetParameter(), row);
            if (!linked.isEmpty())
                return linked;
        }
    }
    return tr("Preset %1").arg(row + 1);
}

QList<int> MultiButtonConfigDialog::selectedPresetRows() const
{
    QSet<int> rowSet;
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (m_widgetAppearanceTable)
        {
            for (const QTableWidgetSelectionRange& range : m_widgetAppearanceTable->selectedRanges())
            {
                for (int r = range.topRow(); r <= range.bottomRow(); ++r)
                {
                    if (r >= 0 && r < m_widgetEntryAppearance.size())
                        rowSet.insert(r);
                }
            }
            if (rowSet.isEmpty() && m_widgetAppearanceTable->currentRow() >= 0
                    && m_widgetAppearanceTable->currentRow() < m_widgetEntryAppearance.size())
            {
                rowSet.insert(m_widgetAppearanceTable->currentRow());
            }
        }

        QList<int> rows = rowSet.values();
        std::sort(rows.begin(), rows.end());
        return rows;
    }

    if (m_presetTable)
    {
        for (const QTableWidgetSelectionRange& range : m_presetTable->selectedRanges())
        {
            for (int r = range.topRow(); r <= range.bottomRow(); ++r)
            {
                if (r >= 0 && r < m_levelPresets.size())
                    rowSet.insert(r);
            }
        }
    }
    if (rowSet.isEmpty() && m_presetTable && m_presetTable->currentRow() >= 0
        && m_presetTable->currentRow() < m_levelPresets.size())
    {
        rowSet.insert(m_presetTable->currentRow());
    }

    QList<int> rows = rowSet.values();
    std::sort(rows.begin(), rows.end());
    return rows;
}

bool MultiButtonConfigDialog::selectedPresetRowsContiguous() const
{
    const QList<int> rows = selectedPresetRows();
    if (rows.size() <= 1)
        return true;
    for (int i = 1; i < rows.size(); ++i)
    {
        if (rows.at(i) != rows.at(i - 1) + 1)
            return false;
    }
    return true;
}

void MultiButtonConfigDialog::commitLevelPresetsFromTable()
{
    if (widgetMode() == MultiButtonMode::Widget)
    {
        if (!m_widgetAppearanceTable)
            return;
        while (m_widgetEntryAppearance.size() < m_widgetAppearanceTable->rowCount())
            m_widgetEntryAppearance.append(LevelPreset());
        while (m_widgetEntryAppearance.size() > m_widgetAppearanceTable->rowCount())
            m_widgetEntryAppearance.removeLast();
        for (int row = 0; row < m_widgetEntryAppearance.size(); ++row)
        {
            QTableWidgetItem* item = m_widgetAppearanceTable->item(row, kPresetNameColumn);
            if (item)
                syncPresetNameFromCell(row, item->text());
        }
        return;
    }
    m_levelPresets = levelPresets();
}

void MultiButtonConfigDialog::selectPresetRows(const QList<int>& rows)
{
    if (!m_presetTable || rows.isEmpty())
        return;

    m_presetTable->clearSelection();
    for (int r : rows)
        m_presetTable->selectRow(r);
    m_presetTable->setCurrentCell(rows.last(), kPresetNameColumn);
}

void MultiButtonConfigDialog::refreshPresetRows(const QList<int>& rows)
{
    for (int row : rows)
        updatePresetNameCell(row);
}

void MultiButtonConfigDialog::applyAppearanceToSelectedRows(
    const std::function<void(LevelPreset&)>& fn)
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    commitLevelPresetsFromTable();
    QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    for (int row : rows)
    {
        if (row >= 0 && row < presets.size())
            fn(presets[row]);
    }
    refreshPresetRows(rows);
}

void MultiButtonConfigDialog::moveSelectedPresetBlock(int delta)
{
    QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    commitLevelPresetsFromTable();
    const int cnt = m_levelPresets.size();

    if (rows.size() == 1 || !selectedPresetRowsContiguous())
    {
        const int row = rows.first();
        if (delta < 0 && row > 0)
        {
            m_levelPresets.swapItemsAt(row - 1, row);
            syncPresetTableColumns();
            selectPresetRows({row - 1});
        }
        else if (delta > 0 && row < cnt - 1)
        {
            m_levelPresets.swapItemsAt(row, row + 1);
            syncPresetTableColumns();
            selectPresetRows({row + 1});
        }
        slotLevelSelectionChanged();
        return;
    }

    const int first = rows.first();
    const int last  = rows.last();
    if (delta < 0 && first == 0)
        return;
    if (delta > 0 && last >= cnt - 1)
        return;

    QList<LevelPreset> block;
    for (int r : rows)
        block.append(m_levelPresets.at(r));

    for (int i = rows.size() - 1; i >= 0; --i)
        m_levelPresets.removeAt(rows.at(i));

    const int insertAt = first + delta;
    for (int i = 0; i < block.size(); ++i)
        m_levelPresets.insert(insertAt + i, block.at(i));

    syncPresetTableColumns();

    QList<int> newSel;
    for (int i = 0; i < block.size(); ++i)
        newSel.append(insertAt + i);
    selectPresetRows(newSel);
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::updatePresetNameCell(int row)
{
    QTableWidget* table = widgetMode() == MultiButtonMode::Widget
            ? m_widgetAppearanceTable : m_presetTable;
    const QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (!table || row < 0 || row >= presets.size())
        return;

    const LevelPreset& preset = presets.at(row);
    const QString display = presetNameCellText(row);

    m_rebuildingPresetTable = true;
    QTableWidgetItem* item = table->item(row, kPresetNameColumn);
    if (!item)
    {
        item = new QTableWidgetItem(display);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        table->setItem(row, kPresetNameColumn, item);
    }
    else
    {
        item->setText(display);
    }
    QString tip = preset.hideName
        ? tr("Preset %1 — name hidden on button (color/icon only)").arg(row + 1)
        : QString();
    if (preset.flashOnActivate)
    {
        if (!tip.isEmpty())
            tip += QLatin1Char('\n');
        tip += tr("Flash (hold)");
    }
    item->setToolTip(tip);

    if (preset.flashOnActivate && preset.iconPath.isEmpty())
        item->setIcon(QIcon(QStringLiteral(":/flash.png")));
    else
        item->setIcon(thumbIcon(preset.iconPath));
    if (preset.color.isValid())
        item->setBackground(preset.color);
    else
        item->setBackground(QBrush());
    m_rebuildingPresetTable = false;
}

void MultiButtonConfigDialog::syncPresetTableColumns()
{
    auto syncTable = [this](QTableWidget* table, QList<LevelPreset>& presets, bool widgetTable) {
        if (!table)
            return;

        const int chanCount = widgetTable ? 0 : m_levelChannelBindings.size();
        const int presetCount = presets.size();
        const int tableCols = kPresetFirstDmxColumn + chanCount;

        QVector<QHash<quint64, quint8>> rowByKey(presetCount);
        QVector<QHash<quint64, QString>> rowFormulaByKey(presetCount);

        for (int r = 0; r < presetCount; ++r)
        {
            const LevelPreset& preset = presets.at(r);
            for (int c = 0; c < chanCount && c < preset.values.size(); ++c)
            {
                const LevelChannelBinding& b = m_levelChannelBindings.at(c);
                rowByKey[r].insert(bindingKey(b.fixtureId, b.channel), preset.values.at(c));
                if (c < preset.valueFormulas.size())
                {
                    rowFormulaByKey[r].insert(bindingKey(b.fixtureId, b.channel),
                                              preset.valueFormulas.at(c));
                }
            }
        }

        m_rebuildingPresetTable = true;
        table->blockSignals(true);
        table->setRowCount(presetCount);
        table->setColumnCount(tableCols);

        QStringList hLabels;
        hLabels << tr("Name") << tr("Input");
        for (const LevelChannelBinding& b : m_levelChannelBindings)
            hLabels << bindingHeaderLabel(m_doc, b);
        table->setHorizontalHeaderLabels(hLabels);

        for (int r = 0; r < presetCount; ++r)
        {
            QList<quint8> newValues;
            QStringList newFormulas;
            LevelPreset rowPreset = presets.at(r);
            for (int col = 0; col < chanCount; ++col)
            {
                const LevelChannelBinding& b = m_levelChannelBindings.at(col);
                const quint64 key = bindingKey(b.fixtureId, b.channel);
                const quint8 val = rowByKey[r].value(key, 0);
                const QString formula = rowFormulaByKey[r].value(key, QString());
                newValues.append(val);
                newFormulas.append(formula);
                rowPreset.values = newValues;
                rowPreset.valueFormulas = newFormulas;
                table->setItem(r, kPresetFirstDmxColumn + col,
                               makePresetValueTableItem(rowPreset, col));
            }
            if (!widgetTable)
            {
                presets[r].values = newValues;
                presets[r].valueFormulas = newFormulas;
            }
            const LevelPreset& preset = presets.at(r);
            QString display;
            if (!preset.hideName)
            {
                if (!preset.label.isEmpty())
                    display = preset.label;
                else if (widgetTable)
                {
                    if (PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget())
                        display = target->multiButtonEntryName(widgetOutputIndex(), widgetParameter(), r);
                }
                if (display.isEmpty())
                    display = tr("Preset %1").arg(r + 1);
            }

            QTableWidgetItem* nameItem = table->item(r, kPresetNameColumn);
            if (!nameItem)
            {
                nameItem = new QTableWidgetItem(display);
                nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
                table->setItem(r, kPresetNameColumn, nameItem);
            }
            else
            {
                nameItem->setText(display);
            }
            QString tip = preset.hideName
                    ? tr("Preset %1 — name hidden on button (color/icon only)").arg(r + 1)
                    : QString();
            if (preset.flashOnActivate)
            {
                if (!tip.isEmpty())
                    tip += QLatin1Char('\n');
                tip += tr("Flash (hold)");
            }
            nameItem->setToolTip(tip);
            if (preset.flashOnActivate && preset.iconPath.isEmpty())
                nameItem->setIcon(QIcon(QStringLiteral(":/flash.png")));
            else
                nameItem->setIcon(thumbIcon(preset.iconPath));
            if (preset.color.isValid())
                nameItem->setBackground(preset.color);
            else
                nameItem->setBackground(QBrush());

            QTableWidgetItem* inputItem = table->item(r, kPresetInputColumn);
            if (!inputItem)
            {
                inputItem = new QTableWidgetItem;
                inputItem->setFlags(inputItem->flags() | Qt::ItemIsEditable);
                table->setItem(r, kPresetInputColumn, inputItem);
            }
            const QSharedPointer<QLCInputSource> entryInput =
                    widgetTable ? m_widgetEntryAppearance.value(r).entryInput
                                : entryInputForRow(r);
            QString inputText = formatInputPatch(
                    entryInput,
                    widgetTable ? m_widgetEntryAppearance.value(r).entryKey
                                : entryKeyForRow(r));
            const int inputValue = widgetTable
                    ? m_widgetEntryAppearance.value(r).entryInputValue
                    : entryInputValueForRow(r);
            if (inputValue >= 0 && !entryInput.isNull() && entryInput->isValid())
                inputText += tr(" = %1").arg(inputValue);
            inputItem->setText(inputText);
            if (dialogSpreadPagingActive())
                inputItem->setToolTip(tr("Shared grid slot %1 on all pages")
                                      .arg(spreadLocalSlotForRow(r)));
            else
                inputItem->setToolTip(QString());
        }

        table->resizeColumnsToContents();
        table->horizontalHeader()->setStretchLastSection(true);
        table->blockSignals(false);
        m_rebuildingPresetTable = false;
    };

    syncTable(m_presetTable, m_levelPresets, false);
    syncTable(m_widgetAppearanceTable, m_widgetEntryAppearance, true);
}

void MultiButtonConfigDialog::slotPresetTableItemChanged(QTableWidgetItem* item)
{
    if (m_rebuildingPresetTable || !item)
        return;

    QTableWidget* table = widgetMode() == MultiButtonMode::Widget
            ? m_widgetAppearanceTable : m_presetTable;
    QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (!table || item->tableWidget() != table)
        return;

    const int row = item->row();
    const int col = item->column();
    if (row < 0 || row >= presets.size())
        return;

    if (col == kPresetInputColumn)
    {
        QString patchText = item->text().trimmed();
        int inputValue = entryInputValueForRow(row);
        const int equalsPos = patchText.indexOf(QLatin1Char('='));
        if (equalsPos >= 0)
        {
            bool ok = false;
            const int parsed = patchText.mid(equalsPos + 1).trimmed().toInt(&ok);
            inputValue = ok ? qBound(0, parsed, 255) : -1;
            patchText = patchText.left(equalsPos).trimmed();
        }
        setEntryInputForRow(row, inputFromPatchString(patchText));
        setEntryInputValueForRow(row, inputValue);
        updatePresetInputCell(row);
        if (m_entryInputEditRow == row && m_presetEntryInputSel)
        {
            m_syncingEntryInputEditor = true;
            m_presetEntryInputSel->setInputSource(entryInputForRow(row));
            m_presetEntryInputSel->setKeySequence(entryKeyForRow(row));
            const int editorInputValue = entryInputValueForRow(row);
            if (m_entryInputValueCheck && m_entryInputValueSpin)
            {
                m_entryInputValueCheck->setChecked(editorInputValue >= 0);
                m_entryInputValueSpin->setEnabled(editorInputValue >= 0);
                m_entryInputValueSpin->setValue(editorInputValue >= 0 ? editorInputValue : 0);
            }
            m_syncingEntryInputEditor = false;
        }
        if (dialogSpreadPagingActive())
            updateAllPresetInputCells();
        return;
    }

    if (col == kPresetNameColumn)
    {
        syncPresetNameFromCell(row, item->text());
        updatePresetNameCell(row);
        return;
    }

    const int valCol = col - kPresetFirstDmxColumn;
    if (valCol < 0 || valCol >= m_levelChannelBindings.size())
        return;

    if (widgetMode() == MultiButtonMode::Widget)
        return;

    LevelPreset& preset = m_levelPresets[row];
    MultiButtonWidget::alignLevelPresetArrays(preset, m_levelChannelBindings.size());

    const QString text = item->text().trimmed();
    if (mbValueExprLooksLikeFormula(text))
    {
        MbValueExpr expr;
        QString err;
        if (mbParseValueExpr(text, expr, &err))
            item->setToolTip(tr("Formula OK"));
        else
            item->setToolTip(err);

        m_rebuildingPresetTable = true;
        preset.valueFormulas[valCol] = text;
        item->setData(kDmxFormulaUserRole, text);
        QFont f = item->font();
        f.setItalic(true);
        item->setFont(f);
        item->setData(Qt::UserRole, int(preset.values.at(valCol)));
        m_rebuildingPresetTable = false;
        return;
    }

    const quint8 val = parseDmxCell(text);
    const QString normalized = QString::number(val);

    m_rebuildingPresetTable = true;
    preset.valueFormulas[valCol].clear();
    item->setData(kDmxFormulaUserRole, QVariant());
    QFont f = item->font();
    f.setItalic(false);
    item->setFont(f);
    if (item->text() != normalized)
        item->setText(normalized);
    item->setData(Qt::UserRole, int(val));
    m_rebuildingPresetTable = false;

    preset.values[valCol] = val;
}

void MultiButtonConfigDialog::syncPresetTableRows()
{
    if (!m_presetTable) return;

    for (int r = 0; r < m_levelPresets.size(); ++r)
        updatePresetNameCell(r);
}

void MultiButtonConfigDialog::slotLevelSelectionChanged()
{
    commitEntryInputEditor();
    m_spreadSlotEditRow = -1;
    if (m_spreadSlotTable)
        m_spreadSlotTable->clearSelection();

    const QList<int> rows = selectedPresetRows();
    const bool has = !rows.isEmpty();
    const int  cnt = m_levelPresets.size();

    if (rows.size() == 1)
        loadEntryInputEditor(rows.first());
    else if (m_presetEntryInputSel)
    {
        m_entryInputEditRow = -1;
        m_syncingEntryInputEditor = true;
        m_presetEntryInputSel->setInputSource(QSharedPointer<QLCInputSource>());
        m_presetEntryInputSel->setKeySequence(QKeySequence());
        if (m_entryInputValueCheck && m_entryInputValueSpin)
        {
            m_entryInputValueCheck->setChecked(false);
            m_entryInputValueSpin->setEnabled(false);
            m_entryInputValueSpin->setValue(0);
        }
        m_syncingEntryInputEditor = false;
    }

    if (m_entryInputGrp)
        m_entryInputGrp->setEnabled(rows.size() == 1);

    const bool levelMode = widgetMode() == MultiButtonMode::Level;
    m_lvlRemoveBtn->setEnabled(levelMode && has);
    m_lvlEditLblBtn->setEnabled(levelMode && rows.size() == 1);
    m_lvlScribbleBtn->setEnabled(levelMode && has);
    m_lvlChooseIconBtn->setEnabled(levelMode && has);
    m_lvlClearIconBtn->setEnabled(levelMode && has);
    m_lvlChooseColorBtn->setEnabled(levelMode && has);
    m_lvlClearColorBtn->setEnabled(levelMode && has);
    m_lvlChooseLabelColorBtn->setEnabled(levelMode && has);
    m_lvlClearLabelColorBtn->setEnabled(levelMode && has);
    const QTableWidgetItem* currentItem = m_presetTable ? m_presetTable->currentItem() : nullptr;
    const bool hasCurrentDmxCell = levelMode && currentItem
        && currentItem->row() >= 0
        && currentItem->row() < m_levelPresets.size()
        && currentItem->column() >= kPresetFirstDmxColumn;
    const bool currentDmxCellHasFormula = hasCurrentDmxCell
        && !currentItem->data(kDmxFormulaUserRole).toString().trimmed().isEmpty();
    if (m_lvlAddFormulaBtn)
        m_lvlAddFormulaBtn->setEnabled(hasCurrentDmxCell);
    if (m_lvlClearFormulaBtn)
        m_lvlClearFormulaBtn->setEnabled(currentDmxCellHasFormula);

    if (m_lvlFlashCheck)
    {
        QSignalBlocker blocker(m_lvlFlashCheck);
        m_lvlFlashCheck->setEnabled(levelMode && has);
        if (!levelMode || !has)
        {
            m_lvlFlashCheck->setCheckState(Qt::Unchecked);
        }
        else
        {
            commitLevelPresetsFromTable();
            int flashOn = 0;
            for (int r : rows)
            {
                if (r >= 0 && r < m_levelPresets.size()
                    && m_levelPresets.at(r).flashOnActivate)
                {
                    flashOn++;
                }
            }
            if (flashOn == 0)
                m_lvlFlashCheck->setCheckState(Qt::Unchecked);
            else if (flashOn == rows.size())
                m_lvlFlashCheck->setCheckState(Qt::Checked);
            else
                m_lvlFlashCheck->setCheckState(Qt::PartiallyChecked);
        }
    }

    auto updateLevelFlashOption = [&](QCheckBox* check, bool LevelPreset::*member) {
        if (!check)
            return;
        QSignalBlocker blocker(check);
        if (!levelMode || !has)
        {
            check->setEnabled(false);
            check->setCheckState(Qt::Unchecked);
            return;
        }
        int flashRows = 0;
        int enabledRows = 0;
        for (int r : rows)
        {
            if (r < 0 || r >= m_levelPresets.size()
                || !m_levelPresets.at(r).flashOnActivate)
                continue;
            ++flashRows;
            if (m_levelPresets.at(r).*member)
                ++enabledRows;
        }
        check->setEnabled(flashRows > 0);
        if (flashRows == 0 || enabledRows == 0)
            check->setCheckState(Qt::Unchecked);
        else if (enabledRows == flashRows)
            check->setCheckState(Qt::Checked);
        else
            check->setCheckState(Qt::PartiallyChecked);
    };
    updateLevelFlashOption(m_lvlFlashOverrideCheck, &LevelPreset::flashOverride);
    updateLevelFlashOption(m_lvlFlashForceLtpCheck, &LevelPreset::flashForceLtp);

    bool canMoveUp = false;
    bool canMoveDown = false;
    if (has)
    {
        if (rows.size() > 1 && selectedPresetRowsContiguous())
        {
            canMoveUp   = rows.first() > 0;
            canMoveDown = rows.last() < cnt - 1;
        }
        else
        {
            const int row = rows.first();
            canMoveUp   = row > 0;
            canMoveDown = row < cnt - 1;
        }
    }
    m_lvlUpBtn->setEnabled(levelMode && canMoveUp);
    m_lvlDownBtn->setEnabled(levelMode && canMoveDown);
}

void MultiButtonConfigDialog::slotLevelAddPreset()
{
    LevelPreset preset;
    preset.hideName = false;
    preset.values = QList<quint8>(m_levelChannelBindings.size(), 0);
    preset.valueFormulas = QStringList(m_levelChannelBindings.size(), QString());
    m_levelPresets.append(preset);
    syncPresetTableColumns();
    if (!m_levelPresets.isEmpty())
        m_presetTable->setCurrentCell(m_levelPresets.size() - 1, 0);
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelRemovePreset()
{
    QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    commitLevelPresetsFromTable();
    for (int i = rows.size() - 1; i >= 0; --i)
    {
        const int row = rows.at(i);
        if (row >= 0 && row < m_levelPresets.size())
            m_levelPresets.removeAt(row);
    }
    syncPresetTableColumns();
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelEditLabel()
{
    const QList<int> rows = selectedPresetRows();
    if (rows.size() != 1)
        return;

    const int row = rows.first();
    QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (row < 0 || row >= presets.size()) return;

    commitLevelPresetsFromTable();

    bool ok = false;
    QString newLbl = QInputDialog::getText(
        this, tr("Custom label"),
        tr("Label for preset %1 (blank = hide name on button, default is Preset %1):").arg(row + 1),
        QLineEdit::Normal, presetNameCellText(row), &ok);
    if (!ok) return;

    syncPresetNameFromCell(row, newLbl);
    updatePresetNameCell(row);
}

void MultiButtonConfigDialog::slotLevelScribbleIcon()
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    ScribbleDialog dlg(m_doc, this);
    if (dlg.exec() != QDialog::Accepted) return;

    const QString path = dlg.savedIconPath();
    applyAppearanceToSelectedRows([&path](LevelPreset& preset) {
        preset.iconPath = path;
    });
}

void MultiButtonConfigDialog::slotLevelChooseIcon()
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    QString formats;
    for (const QByteArray& ba : QImageReader::supportedImageFormats())
        formats += QString("*.%1 ").arg(QString(ba).toLower());

    commitLevelPresetsFromTable();
    QString startPath;
    const QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (rows.first() >= 0 && rows.first() < presets.size())
        startPath = presets.at(rows.first()).iconPath;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select icon image"), startPath,
        tr("Images (%1)").arg(formats));
    if (path.isEmpty()) return;

    applyAppearanceToSelectedRows([&path](LevelPreset& preset) {
        preset.iconPath = path;
    });
}

void MultiButtonConfigDialog::slotLevelClearIcon()
{
    if (selectedPresetRows().isEmpty())
        return;

    applyAppearanceToSelectedRows([](LevelPreset& preset) {
        preset.iconPath.clear();
    });
}

void MultiButtonConfigDialog::slotLevelChooseColor()
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    commitLevelPresetsFromTable();
    QColor initial = Qt::white;
    const QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (rows.first() >= 0 && rows.first() < presets.size()
        && presets.at(rows.first()).color.isValid())
    {
        initial = presets.at(rows.first()).color;
    }

    const QColor chosen = QColorDialog::getColor(initial, this, tr("Preset button color"));
    if (!chosen.isValid())
        return;

    applyAppearanceToSelectedRows([&chosen](LevelPreset& preset) {
        preset.color = chosen;
    });
}

void MultiButtonConfigDialog::slotLevelClearColor()
{
    if (selectedPresetRows().isEmpty())
        return;

    applyAppearanceToSelectedRows([](LevelPreset& preset) {
        preset.color = QColor();
    });
}

void MultiButtonConfigDialog::slotLevelChooseLabelColor()
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    commitLevelPresetsFromTable();
    QColor initial = Qt::black;
    const QList<LevelPreset>& presets = widgetMode() == MultiButtonMode::Widget
            ? m_widgetEntryAppearance : m_levelPresets;
    if (rows.first() >= 0 && rows.first() < presets.size()
        && presets.at(rows.first()).labelColor.isValid())
    {
        initial = presets.at(rows.first()).labelColor;
    }

    const QColor chosen = QColorDialog::getColor(initial, this, tr("Preset label color"));
    if (!chosen.isValid())
        return;

    applyAppearanceToSelectedRows([&chosen](LevelPreset& preset) {
        preset.labelColor = chosen;
    });
}

void MultiButtonConfigDialog::slotLevelClearLabelColor()
{
    if (selectedPresetRows().isEmpty())
        return;

    applyAppearanceToSelectedRows([](LevelPreset& preset) {
        preset.labelColor = QColor();
    });
}

void MultiButtonConfigDialog::slotLevelAddFormula()
{
    if (!m_presetTable || widgetMode() == MultiButtonMode::Widget)
        return;

    const QModelIndex idx = m_presetTable->currentIndex();
    if (!idx.isValid() || idx.column() < kPresetFirstDmxColumn)
    {
        QMessageBox::information(this, tr("Formula"),
                                 tr("Select a DMX value cell first."));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add formula"));
    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    QFormLayout* form = new QFormLayout;
    QSpinBox* universeSpin = new QSpinBox(&dlg);
    universeSpin->setRange(1, 512);
    universeSpin->setValue(1);
    QSpinBox* channelSpin = new QSpinBox(&dlg);
    channelSpin->setRange(1, 512);
    channelSpin->setValue(1);
    QComboBox* opCombo = new QComboBox(&dlg);
    opCombo->addItems({ QStringLiteral(">="), QStringLiteral(">"),
                        QStringLiteral("<="), QStringLiteral("<"),
                        QStringLiteral("=="), QStringLiteral("!=") });
    QSpinBox* compareSpin = new QSpinBox(&dlg);
    compareSpin->setRange(0, 255);
    compareSpin->setValue(128);
    QSpinBox* trueSpin = new QSpinBox(&dlg);
    trueSpin->setRange(0, 255);
    trueSpin->setValue(255);
    QSpinBox* falseSpin = new QSpinBox(&dlg);
    falseSpin->setRange(0, 255);
    falseSpin->setValue(0);

    form->addRow(tr("Universe"), universeSpin);
    form->addRow(tr("Channel"), channelSpin);
    form->addRow(tr("Operator"), opCombo);
    form->addRow(tr("Compare value"), compareSpin);
    form->addRow(tr("True value"), trueSpin);
    form->addRow(tr("False value"), falseSpin);
    lay->addLayout(form);

    QLabel* preview = new QLabel(&dlg);
    preview->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addWidget(preview);

    auto updatePreview = [&]() {
        preview->setText(QStringLiteral("IF(u%1.ch%2 %3 %4, %5, %6)")
                         .arg(universeSpin->value())
                         .arg(channelSpin->value())
                         .arg(opCombo->currentText())
                         .arg(compareSpin->value())
                         .arg(trueSpin->value())
                         .arg(falseSpin->value()));
    };
    updatePreview();
    connect(universeSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updatePreview);
    connect(channelSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updatePreview);
    connect(opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, updatePreview);
    connect(compareSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updatePreview);
    connect(trueSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updatePreview);
    connect(falseSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dlg, updatePreview);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel, &dlg);
    lay->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString formula = preview->text();
    MbValueExpr expr;
    QString err;
    if (!mbParseValueExpr(formula, expr, &err))
    {
        QMessageBox::warning(this, tr("Formula"), err);
        return;
    }

    QTableWidgetItem* item = m_presetTable->item(idx.row(), idx.column());
    if (!item)
    {
        item = makeValueTableItem(0);
        m_presetTable->setItem(idx.row(), idx.column(), item);
    }
    item->setText(formula);
    slotPresetTableItemChanged(item);
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelClearFormula()
{
    if (!m_presetTable || widgetMode() == MultiButtonMode::Widget)
        return;
    const QModelIndex idx = m_presetTable->currentIndex();
    if (!idx.isValid() || idx.column() < kPresetFirstDmxColumn)
    {
        QMessageBox::information(this, tr("Formula"),
                                 tr("Select a DMX value cell first."));
        return;
    }
    QTableWidgetItem* item = m_presetTable->item(idx.row(), idx.column());
    if (!item)
        return;
    const int value = item->data(Qt::UserRole).isValid()
            ? item->data(Qt::UserRole).toInt() : 0;
    item->setText(QString::number(qBound(0, value, 255)));
    slotPresetTableItemChanged(item);
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelFlashToggled(int state)
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    const bool enable = (state != Qt::Unchecked);
    applyAppearanceToSelectedRows([enable](LevelPreset& preset) {
        preset.flashOnActivate = enable;
        if (!enable)
        {
            preset.flashOverride = false;
            preset.flashForceLtp = false;
        }
    });

    if (m_lvlFlashCheck)
    {
        QSignalBlocker blocker(m_lvlFlashCheck);
        m_lvlFlashCheck->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
    }
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelFlashOverrideToggled(int state)
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    const bool enable = (state != Qt::Unchecked);
    applyAppearanceToSelectedRows([enable](LevelPreset& preset) {
        if (preset.flashOnActivate)
            preset.flashOverride = enable;
    });

    if (m_lvlFlashOverrideCheck)
    {
        QSignalBlocker blocker(m_lvlFlashOverrideCheck);
        m_lvlFlashOverrideCheck->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
    }
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelFlashForceLtpToggled(int state)
{
    const QList<int> rows = selectedPresetRows();
    if (rows.isEmpty())
        return;

    const bool enable = (state != Qt::Unchecked);
    applyAppearanceToSelectedRows([enable](LevelPreset& preset) {
        if (preset.flashOnActivate)
            preset.flashForceLtp = enable;
    });

    if (m_lvlFlashForceLtpCheck)
    {
        QSignalBlocker blocker(m_lvlFlashForceLtpCheck);
        m_lvlFlashForceLtpCheck->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
    }
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotFunctionFlashToggled(int state)
{
    int row = m_listWidget ? m_listWidget->currentRow() : -1;
    if (row < 0 || row >= m_functionEntryFlash.size())
        return;

    m_functionEntryFlash[row] = (state == Qt::Checked);
    if (!m_functionEntryFlash[row])
    {
        if (row < m_functionEntryFlashOverride.size())
            m_functionEntryFlashOverride[row] = false;
        if (row < m_functionEntryFlashForceLtp.size())
            m_functionEntryFlashForceLtp[row] = false;
    }
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotFunctionFlashOverrideToggled(int state)
{
    int row = m_listWidget ? m_listWidget->currentRow() : -1;
    if (row < 0 || row >= m_functionEntryFlash.size()
        || !m_functionEntryFlash.at(row))
        return;
    while (m_functionEntryFlashOverride.size() <= row)
        m_functionEntryFlashOverride.append(false);
    m_functionEntryFlashOverride[row] = (state == Qt::Checked);
}

void MultiButtonConfigDialog::slotFunctionFlashForceLtpToggled(int state)
{
    int row = m_listWidget ? m_listWidget->currentRow() : -1;
    if (row < 0 || row >= m_functionEntryFlash.size()
        || !m_functionEntryFlash.at(row))
        return;
    while (m_functionEntryFlashForceLtp.size() <= row)
        m_functionEntryFlashForceLtp.append(false);
    m_functionEntryFlashForceLtp[row] = (state == Qt::Checked);
}

void MultiButtonConfigDialog::slotEntryInputValueCheckToggled(bool checked)
{
    if (m_syncingEntryInputEditor)
        return;
    if (m_entryInputValueSpin)
        m_entryInputValueSpin->setEnabled(checked);
    commitEntryInputEditor();
}

void MultiButtonConfigDialog::slotEntryInputValueChanged(int)
{
    if (m_syncingEntryInputEditor)
        return;
    if (m_entryInputValueCheck && m_entryInputValueCheck->isChecked())
        commitEntryInputEditor();
}

void MultiButtonConfigDialog::slotFunctionChooseLabelColor()
{
    int row = m_listWidget ? m_listWidget->currentRow() : -1;
    if (row < 0)
        return;

    const QColor initial = (row < m_functionEntryLabelColors.size()
                            && m_functionEntryLabelColors.at(row).isValid())
        ? m_functionEntryLabelColors.at(row) : QColor(Qt::black);

    const QColor chosen = QColorDialog::getColor(initial, this, tr("Entry label color"));
    if (!chosen.isValid())
        return;

    while (m_functionEntryLabelColors.size() <= row)
        m_functionEntryLabelColors.append(QColor());
    m_functionEntryLabelColors[row] = chosen;
}

void MultiButtonConfigDialog::slotFunctionClearLabelColor()
{
    int row = m_listWidget ? m_listWidget->currentRow() : -1;
    if (row < 0)
        return;

    while (m_functionEntryLabelColors.size() <= row)
        m_functionEntryLabelColors.append(QColor());
    m_functionEntryLabelColors[row] = QColor();
}

void MultiButtonConfigDialog::slotLevelMoveUp()
{
    moveSelectedPresetBlock(-1);
}

void MultiButtonConfigDialog::slotLevelMoveDown()
{
    moveSelectedPresetBlock(1);
}

// ---- Function list helpers -------------------------------------------------

void MultiButtonConfigDialog::rebuildList()
{
    m_listWidget->clear();
    for (int i = 0; i < m_ids.size(); ++i)
    {
        quint32 id    = m_ids.at(i);
        QString label = m_labels.value(i);
        QString icon  = m_icons.value(i);

        Function* f   = m_doc->function(id);
        QString display = label.isEmpty()
            ? (f ? f->name() : tr("ID %1 (missing)").arg(id))
            : QString("%1 (%2)").arg(label, f ? f->name() : tr("?"));

        QString patch = formatInputPatch(entryInputForRow(i), entryKeyForRow(i));
        const int inputValue = entryInputValueForRow(i);
        const QSharedPointer<QLCInputSource> src = entryInputForRow(i);
        if (inputValue >= 0 && !src.isNull() && src->isValid())
            patch += tr(" = %1").arg(inputValue);
        if (!patch.isEmpty())
            display += QStringLiteral(" — ") + patch;

        QListWidgetItem* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole,     QVariant::fromValue<quint32>(id));
        item->setData(Qt::UserRole + 1, label);
        item->setData(Qt::UserRole + 2, icon);
        item->setIcon(thumbIcon(icon));
        if (!f)
            item->setForeground(Qt::red);
        m_listWidget->addItem(item);
    }
}

void MultiButtonConfigDialog::slotSelectionChanged()
{
    commitEntryInputEditor();

    bool has = (m_listWidget->currentRow() >= 0);
    int  row = m_listWidget->currentRow();
    int  cnt = m_listWidget->count();

    if (has)
    {
        loadEntryInputEditor(row);
    }
    else if (m_presetEntryInputSel)
    {
        m_entryInputEditRow = -1;
        m_syncingEntryInputEditor = true;
        m_presetEntryInputSel->setInputSource(QSharedPointer<QLCInputSource>());
        m_presetEntryInputSel->setKeySequence(QKeySequence());
        if (m_entryInputValueCheck && m_entryInputValueSpin)
        {
            m_entryInputValueCheck->setChecked(false);
            m_entryInputValueSpin->setEnabled(false);
            m_entryInputValueSpin->setValue(0);
        }
        m_syncingEntryInputEditor = false;
    }

    if (m_entryInputGrp)
        m_entryInputGrp->setEnabled(has);

    m_removeBtn->setEnabled(has);
    m_editLblBtn->setEnabled(has);
    m_scribbleBtn->setEnabled(has);
    m_chooseIconBtn->setEnabled(has);
    m_clearIconBtn->setEnabled(has);
    m_upBtn->setEnabled(has && row > 0);
    m_downBtn->setEnabled(has && row < cnt - 1);

    if (m_functionFlashCheck)
    {
        QSignalBlocker blocker(m_functionFlashCheck);
        m_functionFlashCheck->setEnabled(has);
        m_functionFlashCheck->setChecked(has && row < m_functionEntryFlash.size()
                                        && m_functionEntryFlash.at(row));
    }
    const bool functionFlash = has && row < m_functionEntryFlash.size()
            && m_functionEntryFlash.at(row);
    if (m_functionFlashOverrideCheck)
    {
        QSignalBlocker blocker(m_functionFlashOverrideCheck);
        m_functionFlashOverrideCheck->setEnabled(functionFlash);
        m_functionFlashOverrideCheck->setChecked(functionFlash
                && row < m_functionEntryFlashOverride.size()
                && m_functionEntryFlashOverride.at(row));
    }
    if (m_functionFlashForceLtpCheck)
    {
        QSignalBlocker blocker(m_functionFlashForceLtpCheck);
        m_functionFlashForceLtpCheck->setEnabled(functionFlash);
        m_functionFlashForceLtpCheck->setChecked(functionFlash
                && row < m_functionEntryFlashForceLtp.size()
                && m_functionEntryFlashForceLtp.at(row));
    }
}

void MultiButtonConfigDialog::slotAdd()
{
    FunctionSelection fs(this, m_doc);
    fs.setMultiSelection(true);
    if (fs.exec() != QDialog::Accepted) return;

    for (quint32 fid : fs.selection())
    {
        if (m_ids.contains(fid)) continue;
        Function* f = m_doc->function(fid);

        m_ids.append(fid);
        m_labels.append(QString());
        m_icons.append(QString());
        m_functionEntryInputs.append(QSharedPointer<QLCInputSource>());
        m_functionEntryKeys.append(QKeySequence());
        m_functionEntryInputValues.append(-1);
        m_functionEntryFlash.append(false);
        m_functionEntryFlashOverride.append(false);
        m_functionEntryFlashForceLtp.append(false);
        m_functionEntryLabelColors.append(QColor());

        QListWidgetItem* item = new QListWidgetItem(f ? f->name() : tr("ID %1").arg(fid));
        item->setData(Qt::UserRole,     QVariant::fromValue<quint32>(fid));
        item->setData(Qt::UserRole + 1, QString());
        item->setData(Qt::UserRole + 2, QString());
        m_listWidget->addItem(item);
    }
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotRemove()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    delete m_listWidget->takeItem(row);
    if (row < m_ids.size())
    {
        m_ids.removeAt(row);
        m_labels.removeAt(row);
        m_icons.removeAt(row);
    }
    if (row < m_functionEntryInputs.size())
        m_functionEntryInputs.removeAt(row);
    if (row < m_functionEntryKeys.size())
        m_functionEntryKeys.removeAt(row);
    if (row < m_functionEntryInputValues.size())
        m_functionEntryInputValues.removeAt(row);
    if (row < m_functionEntryFlash.size())
        m_functionEntryFlash.removeAt(row);
    if (row < m_functionEntryFlashOverride.size())
        m_functionEntryFlashOverride.removeAt(row);
    if (row < m_functionEntryFlashForceLtp.size())
        m_functionEntryFlashForceLtp.removeAt(row);
    if (row < m_functionEntryLabelColors.size())
        m_functionEntryLabelColors.removeAt(row);
    rebuildList();
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotEditLabel()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    QListWidgetItem* item = m_listWidget->item(row);
    quint32 fid    = item->data(Qt::UserRole).toUInt();
    QString curLbl = item->data(Qt::UserRole + 1).toString();
    Function* f    = m_doc->function(fid);

    bool ok = false;
    QString newLbl = QInputDialog::getText(
        this, tr("Custom label"),
        tr("Label for \"%1\" (leave blank to use function name):").arg(f ? f->name() : tr("?")),
        QLineEdit::Normal, curLbl, &ok);

    if (!ok) return;

    item->setData(Qt::UserRole + 1, newLbl);
    QString display = newLbl.isEmpty()
        ? (f ? f->name() : tr("ID %1 (missing)").arg(fid))
        : QString("%1 (%2)").arg(newLbl, f ? f->name() : tr("?"));
    item->setText(display);

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotScribbleIcon()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    ScribbleDialog dlg(m_doc, this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString path = dlg.savedIconPath();
    if (path.isEmpty()) return;

    QListWidgetItem* item = m_listWidget->item(row);
    item->setData(Qt::UserRole + 2, path);
    item->setIcon(thumbIcon(path));

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotChooseIcon()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    QString formats;
    for (const QByteArray& ba : QImageReader::supportedImageFormats())
        formats += QString("*.%1 ").arg(QString(ba).toLower());

    QString cur = m_listWidget->item(row)->data(Qt::UserRole + 2).toString();
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select icon image"), cur,
        tr("Images (%1)").arg(formats));

    if (path.isEmpty()) return;

    QListWidgetItem* item = m_listWidget->item(row);
    item->setData(Qt::UserRole + 2, path);
    item->setIcon(thumbIcon(path));

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotClearIcon()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    QListWidgetItem* item = m_listWidget->item(row);
    item->setData(Qt::UserRole + 2, QString());
    item->setIcon(QIcon());

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotMoveUp()
{
    int row = m_listWidget->currentRow();
    if (row <= 0) return;

    QListWidgetItem* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row - 1, item);
    m_listWidget->setCurrentRow(row - 1);

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
    if (row < m_functionEntryInputs.size() && row - 1 >= 0)
        m_functionEntryInputs.swapItemsAt(row, row - 1);
    if (row < m_functionEntryKeys.size() && row - 1 >= 0)
        m_functionEntryKeys.swapItemsAt(row, row - 1);
    if (row < m_functionEntryInputValues.size() && row - 1 >= 0)
        m_functionEntryInputValues.swapItemsAt(row, row - 1);
    if (row < m_functionEntryFlash.size() && row - 1 >= 0)
        m_functionEntryFlash.swapItemsAt(row, row - 1);
    if (row < m_functionEntryFlashOverride.size() && row - 1 >= 0)
        m_functionEntryFlashOverride.swapItemsAt(row, row - 1);
    if (row < m_functionEntryFlashForceLtp.size() && row - 1 >= 0)
        m_functionEntryFlashForceLtp.swapItemsAt(row, row - 1);
    if (row < m_functionEntryLabelColors.size() && row - 1 >= 0)
        m_functionEntryLabelColors.swapItemsAt(row, row - 1);
    rebuildList();
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotMoveDown()
{
    int row = m_listWidget->currentRow();
    if (row < 0 || row >= m_listWidget->count() - 1) return;

    QListWidgetItem* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row + 1, item);
    m_listWidget->setCurrentRow(row + 1);

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
    if (row + 1 < m_functionEntryInputs.size())
        m_functionEntryInputs.swapItemsAt(row, row + 1);
    if (row + 1 < m_functionEntryKeys.size())
        m_functionEntryKeys.swapItemsAt(row, row + 1);
    if (row + 1 < m_functionEntryInputValues.size())
        m_functionEntryInputValues.swapItemsAt(row, row + 1);
    if (row + 1 < m_functionEntryFlash.size())
        m_functionEntryFlash.swapItemsAt(row, row + 1);
    if (row + 1 < m_functionEntryFlashOverride.size())
        m_functionEntryFlashOverride.swapItemsAt(row, row + 1);
    if (row + 1 < m_functionEntryFlashForceLtp.size())
        m_functionEntryFlashForceLtp.swapItemsAt(row, row + 1);
    if (row + 1 < m_functionEntryLabelColors.size())
        m_functionEntryLabelColors.swapItemsAt(row, row + 1);
    rebuildList();
    slotSelectionChanged();
}

// ---- Automation tab -------------------------------------------------------

static void syncExcludeMaskFromColumn(QTableWidget* table, int col, quint32& mask)
{
    mask = 0;
    if (!table || col < 0 || col >= table->columnCount())
        return;
    for (int row = 0; row < table->rowCount(); ++row)
    {
        QTableWidgetItem* item = table->item(row, col);
        if (item && item->checkState() == Qt::Checked && row < 32)
            mask |= (1u << row);
    }
}

static void syncAllExcludeMasksFromTable(QTableWidget* table,
                                         QList<MultiButtonAutomationProfile>& profiles)
{
    if (!table || table->columnCount() != profiles.size())
        return;
    for (int col = 0; col < profiles.size(); ++col)
        syncExcludeMaskFromColumn(table, col, profiles[col].excludeMask);
}

int MultiButtonConfigDialog::entryCountForAutomation() const
{
    if (widgetMode() == MultiButtonMode::Level)
        return m_levelPresets.size();
    if (widgetMode() == MultiButtonMode::Widget)
    {
        PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget();
        return target ? target->multiButtonEntryCount(widgetOutputIndex(), widgetParameter()) : 0;
    }
    return m_listWidget ? m_listWidget->count() : 0;
}

QString MultiButtonConfigDialog::entryLabelForAutomation(int index) const
{
    if (index < 0)
        return QString();

    if (widgetMode() == MultiButtonMode::Level)
    {
        if (index >= m_levelPresets.size())
            return QString();
        const LevelPreset& preset = m_levelPresets.at(index);
        if (!preset.hideName && !preset.label.isEmpty())
            return preset.label;
        return tr("Preset %1").arg(index + 1);
    }

    if (widgetMode() == MultiButtonMode::Widget)
    {
        PresetTableV2MultiButtonTargetIface* target = selectedWidgetTarget();
        if (!target)
            return QString();
        QString name = target->multiButtonEntryName(widgetOutputIndex(), widgetParameter(), index);
        if (name.isEmpty())
            name = tr("Preset %1").arg(index + 1);
        return name;
    }

    if (!m_listWidget || index >= m_listWidget->count())
        return QString();
    return m_listWidget->item(index)->text();
}

void MultiButtonConfigDialog::rebuildAutomationExcludeTable()
{
    if (!m_autoExcludeTable)
        return;

    if (m_autoExcludeTable->columnCount() == m_automationProfiles.size()
        && m_autoExcludeTable->columnCount() > 0)
    {
        syncAllExcludeMasksFromTable(m_autoExcludeTable, m_automationProfiles);
    }

    m_rebuildingExcludeTable = true;
    m_autoExcludeTable->setRowCount(0);
    m_autoExcludeTable->setColumnCount(0);
    m_autoExcludeTable->clearSpans();

    const int profileCount = m_automationProfiles.size();
    const int presetCount  = entryCountForAutomation();

    if (presetCount <= 0 || profileCount <= 0)
    {
        m_autoExcludeTable->setColumnCount(1);
        m_autoExcludeTable->setRowCount(1);
        QTableWidgetItem* hint = new QTableWidgetItem(
            presetCount <= 0
                ? tr("Add entries on the Entries tab first.")
                : tr("Add at least one automation profile."));
        hint->setFlags(Qt::ItemIsEnabled);
        m_autoExcludeTable->setItem(0, 0, hint);
        m_autoExcludeTable->setSpan(0, 0, 1, 1);
        m_autoExcludeTable->horizontalHeader()->setVisible(profileCount > 0);
        m_rebuildingExcludeTable = false;
        return;
    }

    QStringList colHeaders;
    for (const MultiButtonAutomationProfile& profile : m_automationProfiles)
    {
        QString name = profile.name.trimmed();
        if (name.isEmpty())
            name = tr("Profile");
        colHeaders.append(name);
    }

    m_autoExcludeTable->setColumnCount(profileCount);
    m_autoExcludeTable->setHorizontalHeaderLabels(colHeaders);
    m_autoExcludeTable->horizontalHeader()->setVisible(true);
    for (int col = 0; col < profileCount; ++col)
    {
        m_autoExcludeTable->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Fixed);
        m_autoExcludeTable->setColumnWidth(col, 44);
        if (QTableWidgetItem* hdr = m_autoExcludeTable->horizontalHeaderItem(col))
            hdr->setToolTip(m_automationProfiles.at(col).name);
    }

    m_autoExcludeTable->setRowCount(presetCount);
    for (int row = 0; row < presetCount; ++row)
    {
        QTableWidgetItem* headerItem = new QTableWidgetItem(entryLabelForAutomation(row));
        headerItem->setFlags(Qt::ItemIsEnabled);
        m_autoExcludeTable->setVerticalHeaderItem(row, headerItem);

        const bool rowExcludedFromMask = (row >= 32);
        for (int col = 0; col < profileCount; ++col)
        {
            const quint32 mask = m_automationProfiles.at(col).excludeMask;
            QTableWidgetItem* cell = new QTableWidgetItem;
            Qt::ItemFlags flags = Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
            if (rowExcludedFromMask)
                flags &= ~Qt::ItemIsUserCheckable;
            cell->setFlags(flags);
            cell->setTextAlignment(Qt::AlignCenter);
            if (!rowExcludedFromMask)
            {
                cell->setCheckState((mask & (1u << row)) ? Qt::Checked : Qt::Unchecked);
            }
            else
            {
                cell->setToolTip(tr("Automation exclude supports up to 32 presets."));
            }
            m_autoExcludeTable->setItem(row, col, cell);
        }
    }

    m_rebuildingExcludeTable = false;
}

void MultiButtonConfigDialog::syncDataFromProfileTable()
{
    if (!m_autoProfileTable || m_rebuildingProfileTable)
        return;

    const int rows = m_autoProfileTable->rowCount();
    if (rows == 0 || m_autoProfileTable->cellWidget(0, kProfColMode) == nullptr)
        return;

    for (int row = 0; row < m_automationProfiles.size(); ++row)
    {
        if (row >= rows
            || m_autoProfileTable->cellWidget(row, kProfColMode) == nullptr)
        {
            continue;
        }

        MultiButtonAutomationProfile& profile = m_automationProfiles[row];

        if (QTableWidgetItem* nameItem = m_autoProfileTable->item(row, kProfColName))
            profile.name = nameItem->text().trimmed();

        if (auto* modeCombo = qobject_cast<QComboBox*>(
                m_autoProfileTable->cellWidget(row, kProfColMode)))
        {
            profile.mode = static_cast<MultiButtonAutomationMode>(
                modeCombo->currentData().toInt());
        }

        if (auto* minSpin = qobject_cast<QSpinBox*>(
                m_autoProfileTable->cellWidget(row, kProfColJumpMin)))
            profile.stepMin = minSpin->value();

        if (auto* maxSpin = qobject_cast<QSpinBox*>(
                m_autoProfileTable->cellWidget(row, kProfColJumpMax)))
            profile.stepMax = qMax(profile.stepMin, maxSpin->value());

        if (auto* multSpin = qobject_cast<QSpinBox*>(
                m_autoProfileTable->cellWidget(row, kProfColMultiplier)))
            profile.multiplier = qMax(1, multSpin->value());

        if (auto* offsetSpin = qobject_cast<QSpinBox*>(
                m_autoProfileTable->cellWidget(row, kProfColBeatOffset)))
        {
            profile.beatOffset = offsetSpin->value();
            const int maxOff = qMax(0, profile.multiplier - 1);
            profile.beatOffset = qBound(0, profile.beatOffset, maxOff);
            if (offsetSpin->value() != profile.beatOffset)
                offsetSpin->setValue(profile.beatOffset);
        }
    }
}

void MultiButtonConfigDialog::rebuildAutomationProfileTable()
{
    if (!m_autoProfileTable)
        return;

    if (!m_rebuildingProfileTable)
        syncDataFromProfileTable();

    m_rebuildingProfileTable = true;
    m_autoProfileTable->blockSignals(true);

    const int sel = m_autoProfileTable->currentRow();
    m_autoProfileTable->clearSpans();
    m_autoProfileTable->setRowCount(0);

    if (m_automationProfiles.isEmpty())
    {
        m_autoProfileTable->insertRow(0);
        QTableWidgetItem* hint = new QTableWidgetItem(tr("No profiles — use Add"));
        hint->setFlags(Qt::ItemIsEnabled);
        m_autoProfileTable->setItem(0, kProfColName, hint);
        m_autoProfileTable->setEnabled(false);
    }
    else
    {
        m_autoProfileTable->setEnabled(true);

        for (int row = 0; row < m_automationProfiles.size(); ++row)
        {
            const MultiButtonAutomationProfile& profile = m_automationProfiles.at(row);
            m_autoProfileTable->insertRow(row);

            QTableWidgetItem* nameItem = new QTableWidgetItem(profile.name);
            nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
            m_autoProfileTable->setItem(row, kProfColName, nameItem);

            m_autoProfileTable->setItem(row, kProfColMode, widgetColumnPlaceholder());
            QComboBox* modeCombo = new QComboBox(m_autoProfileTable);
            modeCombo->addItem(tr("Next"),   (int) MultiButtonAutomationMode::Next);
            modeCombo->addItem(tr("Random"), (int) MultiButtonAutomationMode::Random);
            modeCombo->addItem(tr("Jump"),   (int) MultiButtonAutomationMode::Jump);
            for (int i = 0; i < modeCombo->count(); ++i)
            {
                if (modeCombo->itemData(i).toInt() == (int) profile.mode)
                {
                    modeCombo->setCurrentIndex(i);
                    break;
                }
            }
            const int capturedRow = row;
            connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this, capturedRow](int) { slotAutomationProfileModeChanged(capturedRow); });
            m_autoProfileTable->setCellWidget(row, kProfColMode, modeCombo);

            m_autoProfileTable->setItem(row, kProfColJumpMin, widgetColumnPlaceholder());
            QSpinBox* minSpin = new QSpinBox(m_autoProfileTable);
            minSpin->setRange(1, 32);
            minSpin->setValue(qMax(1, profile.stepMin));
            minSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            minSpin->setMaximumWidth(72);
            m_autoProfileTable->setCellWidget(row, kProfColJumpMin, minSpin);

            m_autoProfileTable->setItem(row, kProfColJumpMax, widgetColumnPlaceholder());
            QSpinBox* maxSpin = new QSpinBox(m_autoProfileTable);
            maxSpin->setRange(1, 32);
            maxSpin->setValue(qMax(profile.stepMin, profile.stepMax));
            maxSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            maxSpin->setMaximumWidth(72);
            m_autoProfileTable->setCellWidget(row, kProfColJumpMax, maxSpin);

            m_autoProfileTable->setItem(row, kProfColMultiplier, widgetColumnPlaceholder());
            QSpinBox* multSpin = new QSpinBox(m_autoProfileTable);
            multSpin->setRange(1, 255);
            multSpin->setValue(qMax(1, profile.multiplier));
            multSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            multSpin->setMaximumWidth(72);
            m_autoProfileTable->setCellWidget(row, kProfColMultiplier, multSpin);

            m_autoProfileTable->setItem(row, kProfColBeatOffset, widgetColumnPlaceholder());
            QSpinBox* offsetSpin = new QSpinBox(m_autoProfileTable);
            const int maxOff = qMax(0, profile.multiplier - 1);
            offsetSpin->setRange(0, maxOff);
            offsetSpin->setValue(qBound(0, profile.beatOffset, maxOff));
            offsetSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            offsetSpin->setMaximumWidth(72);
            m_autoProfileTable->setCellWidget(row, kProfColBeatOffset, offsetSpin);

            connect(multSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this, capturedRow](int mult) {
                if (m_rebuildingProfileTable || !m_autoProfileTable)
                    return;
                if (auto* offSpin = qobject_cast<QSpinBox*>(
                        m_autoProfileTable->cellWidget(capturedRow, kProfColBeatOffset)))
                {
                    const int maxO = qMax(0, mult - 1);
                    offSpin->setMaximum(maxO);
                    if (offSpin->value() > maxO)
                        offSpin->setValue(maxO);
                }
            });

            const bool isJump = (profile.mode == MultiButtonAutomationMode::Jump);
            minSpin->setEnabled(isJump);
            maxSpin->setEnabled(isJump);
        }

        tuneAutomationProfileColumnWidths(m_autoProfileTable);

        const int pick = qBound(0, sel >= 0 ? sel : m_activeAutomationProfile,
                                m_automationProfiles.size() - 1);
        m_autoProfileTable->setCurrentCell(pick, kProfColName);
    }

    m_autoProfileTable->blockSignals(false);
    m_rebuildingProfileTable = false;

    rebuildAutomationExcludeTable();
}

void MultiButtonConfigDialog::slotAutomationProfileRowChanged(int currentRow, int previousRow)
{
    Q_UNUSED(previousRow);
    Q_UNUSED(currentRow);

    if (m_syncingAutomationUi || m_rebuildingProfileTable || !m_autoProfileTable)
        return;

    syncDataFromProfileTable();
}

void MultiButtonConfigDialog::slotAutomationProfileModeChanged(int row)
{
    if (!m_autoProfileTable || m_rebuildingProfileTable)
        return;

    auto* modeCombo = qobject_cast<QComboBox*>(
        m_autoProfileTable->cellWidget(row, kProfColMode));
    if (!modeCombo)
        return;

    const bool isJump = (modeCombo->currentData().toInt()
                         == (int) MultiButtonAutomationMode::Jump);
    if (auto* minSpin = qobject_cast<QSpinBox*>(m_autoProfileTable->cellWidget(row, kProfColJumpMin)))
        minSpin->setEnabled(isJump);
    if (auto* maxSpin = qobject_cast<QSpinBox*>(m_autoProfileTable->cellWidget(row, kProfColJumpMax)))
        maxSpin->setEnabled(isJump);
}

void MultiButtonConfigDialog::slotAutoExcludeItemChanged(QTableWidgetItem* item)
{
    if (m_syncingAutomationUi || m_rebuildingExcludeTable || !item)
        return;

    const int col = item->column();
    if (col < 0 || col >= m_automationProfiles.size())
        return;

    syncExcludeMaskFromColumn(m_autoExcludeTable,
                              col,
                              m_automationProfiles[col].excludeMask);
}

void MultiButtonConfigDialog::slotAutomationAddProfile()
{
    syncDataFromProfileTable();

    MultiButtonAutomationProfile profile;
    profile.name = tr("Profile %1").arg(m_automationProfiles.size() + 1);
    m_automationProfiles.append(profile);

    rebuildAutomationProfileTable();
    if (m_autoProfileTable)
        m_autoProfileTable->setCurrentCell(m_automationProfiles.size() - 1, kProfColName);
}

void MultiButtonConfigDialog::slotAutomationRemoveProfile()
{
    if (m_automationProfiles.size() <= 1)
        return;

    syncDataFromProfileTable();

    const int row = m_autoProfileTable ? m_autoProfileTable->currentRow() : -1;
    if (row < 0)
        return;

    m_automationProfiles.removeAt(row);
    rebuildAutomationProfileTable();
}

void MultiButtonConfigDialog::toggleExcludeColumn(int col)
{
    if (m_rebuildingExcludeTable || !m_autoExcludeTable
        || col < 0 || col >= m_automationProfiles.size())
    {
        return;
    }

    syncDataFromProfileTable();
    syncAllExcludeMasksFromTable(m_autoExcludeTable, m_automationProfiles);

    bool allChecked = true;
    for (int row = 0; row < m_autoExcludeTable->rowCount(); ++row)
    {
        QTableWidgetItem* item = m_autoExcludeTable->item(row, col);
        if (!item || item->checkState() != Qt::Checked)
        {
            allChecked = false;
            break;
        }
    }

    const Qt::CheckState newState = allChecked ? Qt::Unchecked : Qt::Checked;
    m_syncingAutomationUi = true;
    for (int row = 0; row < m_autoExcludeTable->rowCount(); ++row)
    {
        if (QTableWidgetItem* item = m_autoExcludeTable->item(row, col))
            item->setCheckState(newState);
    }
    m_syncingAutomationUi = false;

    syncExcludeMaskFromColumn(m_autoExcludeTable, col, m_automationProfiles[col].excludeMask);
}

void MultiButtonConfigDialog::toggleExcludeRow(int row)
{
    if (m_rebuildingExcludeTable || !m_autoExcludeTable
        || row < 0 || row >= m_autoExcludeTable->rowCount())
    {
        return;
    }

    syncDataFromProfileTable();
    syncAllExcludeMasksFromTable(m_autoExcludeTable, m_automationProfiles);

    bool allChecked = true;
    for (int col = 0; col < m_autoExcludeTable->columnCount(); ++col)
    {
        QTableWidgetItem* item = m_autoExcludeTable->item(row, col);
        if (!item || item->checkState() != Qt::Checked)
        {
            allChecked = false;
            break;
        }
    }

    const Qt::CheckState newState = allChecked ? Qt::Unchecked : Qt::Checked;
    m_syncingAutomationUi = true;
    for (int col = 0; col < m_autoExcludeTable->columnCount(); ++col)
    {
        if (QTableWidgetItem* item = m_autoExcludeTable->item(row, col))
            item->setCheckState(newState);
    }
    m_syncingAutomationUi = false;

    for (int col = 0; col < m_automationProfiles.size(); ++col)
        syncExcludeMaskFromColumn(m_autoExcludeTable, col, m_automationProfiles[col].excludeMask);
}

void MultiButtonConfigDialog::slotExcludeColumnHeaderClicked(int col)
{
    toggleExcludeColumn(col);
}

void MultiButtonConfigDialog::slotExcludeRowHeaderClicked(int row)
{
    toggleExcludeRow(row);
}

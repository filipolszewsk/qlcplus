/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "multibuttonconfigdialog.h"

#include "inputselectionwidget.h"
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
#include <QHeaderView>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVector>
#include <algorithm>

static void syncExcludeMaskFromTable(QTableWidget* table, quint32& mask);

static QTableWidgetItem* widgetColumnPlaceholder()
{
    QTableWidgetItem* item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsEnabled);
    return item;
}

static void tuneAutomationProfileColumnWidths(QTableWidget* table)
{
    constexpr int colMode = 1;
    constexpr int colJumpMin = 2;
    constexpr int colJumpMax = 3;
    constexpr int colMultiplier = 4;

    if (!table || table->rowCount() == 0)
        return;
    if (table->cellWidget(0, colMode) == nullptr)
        return;

    table->resizeColumnsToContents();
    table->setColumnWidth(colMode, qMax(table->columnWidth(colMode), 90));
    table->setColumnWidth(colJumpMin, qMax(table->columnWidth(colJumpMin), 60));
    table->setColumnWidth(colJumpMax, qMax(table->columnWidth(colJumpMax), 60));
    table->setColumnWidth(colMultiplier, qMax(table->columnWidth(colMultiplier), 70));
}

MultiButtonConfigDialog::MultiButtonConfigDialog(
    Doc*                               doc,
    MultiButtonMode                    widgetMode,
    const QList<quint32>&              funcIds,
    const QStringList&                 funcLabels,
    const QStringList&                 iconPaths,
    const QList<LevelChannelBinding>&  levelChannelBindings,
    const QList<LevelPreset>&          levelPresets,
    int                                longPressMs,
    bool                               addOffAtEnd,
    bool                               monitorChannelValues,
    MultiButtonLayout                  widgetLayout,
    int                                spreadColumns,
    int                                spreadRows,
    int                                spreadHMargin,
    int                                spreadVMargin,
    int                                spreadTileWidth,
    int                                spreadTileHeight,
    bool                               automationEnabled,
    const QList<MultiButtonAutomationProfile>& automationProfiles,
    int                                activeAutomationProfile,
    QSharedPointer<QLCInputSource>     triggerSrc,
    QSharedPointer<QLCInputSource>     popupSrc,
    QSharedPointer<QLCInputSource>     automationSrc,
    QSharedPointer<QLCInputSource>     presetChooseSrc,
    QSharedPointer<QLCInputSource>     entrySelectSrc,
    int                                widgetPage,
    QWidget*                           parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_ids(funcIds)
    , m_labels(funcLabels)
    , m_icons(iconPaths)
    , m_levelChannelBindings(levelChannelBindings)
    , m_levelPresets(levelPresets)
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

    setWindowTitle(tr("Multi Button — Properties"));
    setMinimumWidth(560);
    setMinimumHeight(420);

    QVBoxLayout* root = new QVBoxLayout(this);

    QTabWidget* tabs = new QTabWidget(this);

    // ---- Tab: Entries ---------------------------------------------------
    QWidget* entriesTab = new QWidget(tabs);
    QVBoxLayout* entriesLay = new QVBoxLayout(entriesTab);

    QHBoxLayout* modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel(tr("Mode:"), entriesTab));
    m_modeCombo = new QComboBox(entriesTab);
    m_modeCombo->addItem(tr("Function"), (int) MultiButtonMode::Function);
    m_modeCombo->addItem(tr("Level"),    (int) MultiButtonMode::Level);
    m_modeCombo->setCurrentIndex(widgetMode == MultiButtonMode::Level ? 1 : 0);
    modeRow->addWidget(m_modeCombo, 1);
    root->addLayout(modeRow);

    m_modeStack = new QStackedWidget(this);

    // ---- Function page --------------------------------------------------
    m_functionPage = new QWidget(this);
    QVBoxLayout* funcLay = new QVBoxLayout(m_functionPage);
    funcLay->setContentsMargins(0, 0, 0, 0);

    QGroupBox* listGrp = new QGroupBox(tr("Functions (cycle order)"), m_functionPage);
    QHBoxLayout* listLayout = new QHBoxLayout(listGrp);

    m_listWidget = new QListWidget(listGrp);
    m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setIconSize(QSize(24, 24));
    m_listWidget->setMinimumHeight(160);
    listLayout->addWidget(m_listWidget, 1);

    QVBoxLayout* btnCol = new QVBoxLayout;
    m_addBtn        = new QPushButton(tr("Add…"),           listGrp);
    m_removeBtn     = new QPushButton(tr("Remove"),         listGrp);
    m_editLblBtn    = new QPushButton(tr("Custom label"),   listGrp);
    m_scribbleBtn   = new QPushButton(tr("Scribble icon…"), listGrp);
    m_chooseIconBtn = new QPushButton(tr("Choose icon…"),   listGrp);
    m_clearIconBtn  = new QPushButton(tr("Clear icon"),     listGrp);
    m_upBtn         = new QPushButton(tr("Move up"),        listGrp);
    m_downBtn       = new QPushButton(tr("Move down"),      listGrp);

    btnCol->addWidget(m_addBtn);
    btnCol->addWidget(m_removeBtn);
    btnCol->addSpacing(6);
    btnCol->addWidget(m_editLblBtn);
    btnCol->addSpacing(6);
    btnCol->addWidget(m_scribbleBtn);
    btnCol->addWidget(m_chooseIconBtn);
    btnCol->addWidget(m_clearIconBtn);
    btnCol->addStretch();
    btnCol->addWidget(m_upBtn);
    btnCol->addWidget(m_downBtn);
    listLayout->addLayout(btnCol);
    funcLay->addWidget(listGrp);

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
    QHBoxLayout* presetLayout = new QHBoxLayout(presetGrp);

    m_presetTable = new QTableWidget(presetGrp);
    m_presetTable->setMinimumHeight(160);
    m_presetTable->horizontalHeader()->setVisible(true);
    m_presetTable->verticalHeader()->hide();
    m_presetTable->setIconSize(QSize(24, 24));
    m_presetTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_presetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_presetTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_presetTable->setAlternatingRowColors(true);
    m_presetTable->verticalHeader()->setDefaultSectionSize(22);
    presetLayout->addWidget(m_presetTable, 1);

    QVBoxLayout* lvlBtnCol = new QVBoxLayout;
    m_chooseChannelsBtn = new QPushButton(tr("Choose channels…"), presetGrp);
    m_lvlAddBtn        = new QPushButton(tr("Add"),           presetGrp);
    m_lvlRemoveBtn     = new QPushButton(tr("Remove"),        presetGrp);
    m_lvlEditLblBtn    = new QPushButton(tr("Custom label"),  presetGrp);
    m_lvlScribbleBtn   = new QPushButton(tr("Scribble icon…"), presetGrp);
    m_lvlChooseIconBtn = new QPushButton(tr("Choose icon…"),  presetGrp);
    m_lvlClearIconBtn  = new QPushButton(tr("Clear icon"),    presetGrp);
    m_lvlChooseColorBtn = new QPushButton(tr("Choose color…"), presetGrp);
    m_lvlClearColorBtn  = new QPushButton(tr("Clear color"),   presetGrp);
    m_lvlUpBtn         = new QPushButton(tr("Move up"),       presetGrp);
    m_lvlDownBtn       = new QPushButton(tr("Move down"),     presetGrp);

    lvlBtnCol->addWidget(m_chooseChannelsBtn);
    lvlBtnCol->addSpacing(6);
    lvlBtnCol->addWidget(m_lvlAddBtn);
    lvlBtnCol->addWidget(m_lvlRemoveBtn);
    lvlBtnCol->addSpacing(6);
    lvlBtnCol->addWidget(m_lvlEditLblBtn);
    lvlBtnCol->addSpacing(6);
    lvlBtnCol->addWidget(m_lvlScribbleBtn);
    lvlBtnCol->addWidget(m_lvlChooseIconBtn);
    lvlBtnCol->addWidget(m_lvlClearIconBtn);
    lvlBtnCol->addSpacing(6);
    lvlBtnCol->addWidget(m_lvlChooseColorBtn);
    lvlBtnCol->addWidget(m_lvlClearColorBtn);
    lvlBtnCol->addStretch();
    lvlBtnCol->addWidget(m_lvlUpBtn);
    lvlBtnCol->addWidget(m_lvlDownBtn);
    presetLayout->addLayout(lvlBtnCol);
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
    connect(m_lvlUpBtn,         &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelMoveUp);
    connect(m_lvlDownBtn,       &QPushButton::clicked, this, &MultiButtonConfigDialog::slotLevelMoveDown);
    connect(m_presetTable, &QTableWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotLevelSelectionChanged);
    connect(m_presetTable, &QTableWidget::itemChanged,
            this, &MultiButtonConfigDialog::slotPresetTableItemChanged);

    m_modeStack->addWidget(m_levelPage);
    entriesLay->addWidget(m_modeStack, 1);

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

    layoutTabLay->addWidget(m_spreadLayoutGrp);
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
        tr("Automation trigger: Multiplier on the selected profile skips pulses (2 = every "
           "2nd trigger). Exclude presets below apply to the selected profile row."), autoTab);
    autoHint->setWordWrap(true);
    {
        QFont hf = autoHint->font();
        hf.setItalic(true);
        autoHint->setFont(hf);
    }
    autoTabLay->addWidget(autoHint);

    QHBoxLayout* autoProfileRow = new QHBoxLayout;
    m_autoProfileTable = new QTableWidget(autoTab);
    m_autoProfileTable->setColumnCount(5);
    m_autoProfileTable->setHorizontalHeaderLabels(
        { tr("Profile"), tr("Mode"), tr("Jump min"), tr("Jump max"), tr("Multiplier") });
    m_autoProfileTable->horizontalHeader()->setStretchLastSection(false);
    m_autoProfileTable->horizontalHeader()->setMinimumSectionSize(48);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColName, QHeaderView::Stretch);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColMode, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColJumpMin, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColJumpMax, QHeaderView::Fixed);
    m_autoProfileTable->horizontalHeader()->setSectionResizeMode(kProfColMultiplier, QHeaderView::Fixed);
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

    autoTabLay->addWidget(new QLabel(tr("Exclude presets (selected profile):"), autoTab));

    m_autoExcludeTable = new QTableWidget(autoTab);
    m_autoExcludeTable->setColumnCount(2);
    m_autoExcludeTable->setHorizontalHeaderLabels(
        { tr("Preset"), tr("Exclude") });
    m_autoExcludeTable->horizontalHeader()->setStretchLastSection(false);
    m_autoExcludeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_autoExcludeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_autoExcludeTable->verticalHeader()->setVisible(false);
    m_autoExcludeTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_autoExcludeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    autoTabLay->addWidget(m_autoExcludeTable, 1);

    connect(autoAddBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotAutomationAddProfile);
    connect(autoRemoveBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotAutomationRemoveProfile);
    connect(m_autoProfileTable, &QTableWidget::currentCellChanged,
            this, &MultiButtonConfigDialog::slotAutomationProfileRowChanged);
    connect(m_autoExcludeTable, &QTableWidget::itemChanged,
            this, &MultiButtonConfigDialog::slotAutoExcludeItemChanged);

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
           "(values above the last profile select the last profile)."), presetChooseGrp);
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
    inputLayout->addWidget(entrySelectGrp);

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
            preset.values.append(presetTableValue(row, kPresetFirstDmxColumn + col));
        presets.append(preset);
    }
    return presets;
}

quint8 MultiButtonConfigDialog::presetTableValue(int row, int col) const
{
    if (col < kPresetFirstDmxColumn)
        return 0;

    int tableRow = row + kDataRowOffset;
    if (!m_presetTable) return 0;
    if (tableRow >= m_presetTable->rowCount() || col >= m_presetTable->columnCount()) return 0;
    QTableWidgetItem* item = m_presetTable->item(tableRow, col);
    if (!item) return 0;
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

bool MultiButtonConfigDialog::automationEnabled() const
{
    return m_autoEnableCheck && m_autoEnableCheck->isChecked();
}

QList<MultiButtonAutomationProfile> MultiButtonConfigDialog::automationProfiles() const
{
    MultiButtonConfigDialog* self = const_cast<MultiButtonConfigDialog*>(this);
    self->syncDataFromProfileTable();
    const int cur = m_autoProfileTable ? m_autoProfileTable->currentRow() : -1;
    if (cur >= 0 && cur < self->m_automationProfiles.size())
        syncExcludeMaskFromTable(m_autoExcludeTable, self->m_automationProfiles[cur].excludeMask);
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

void MultiButtonConfigDialog::accept()
{
    syncDataFromProfileTable();
    const int cur = m_autoProfileTable ? m_autoProfileTable->currentRow() : -1;
    if (cur >= 0 && cur < m_automationProfiles.size())
        syncExcludeMaskFromTable(m_autoExcludeTable, m_automationProfiles[cur].excludeMask);
    m_activeAutomationProfile = activeAutomationProfile();
    QDialog::accept();
}

// ---- Mode switch -----------------------------------------------------------

void MultiButtonConfigDialog::slotModeChanged(int index)
{
    m_modeStack->setCurrentIndex(index);
    updateMonitorTooltip();
    if (!m_syncingAutomationUi)
        rebuildAutomationExcludeTable();
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
    perRowOld.resize(m_levelPresets.size());
    for (int row = 0; row < m_levelPresets.size(); ++row)
    {
        const LevelPreset& preset = m_levelPresets.at(row);
        for (int col = 0; col < m_levelChannelBindings.size() && col < preset.values.size(); ++col)
        {
            const LevelChannelBinding& b = m_levelChannelBindings.at(col);
            perRowOld[row].insert(bindingKey(b.fixtureId, b.channel), preset.values.at(col));
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
        for (const LevelChannelBinding& b : m_levelChannelBindings)
            newValues.append(perRowOld[row].value(bindingKey(b.fixtureId, b.channel), 0));
        m_levelPresets[row].values = newValues;
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
    if (row < 0 || row >= m_levelPresets.size())
        return;

    LevelPreset& preset = m_levelPresets[row];
    const QString t = cellText.trimmed();
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

QString MultiButtonConfigDialog::presetNameCellText(int row) const
{
    if (row < 0 || row >= m_levelPresets.size())
        return QString();

    const LevelPreset& preset = m_levelPresets.at(row);
    if (preset.hideName)
        return QString();
    if (!preset.label.isEmpty())
        return preset.label;
    return tr("Preset %1").arg(row + 1);
}

void MultiButtonConfigDialog::updatePresetNameCell(int row)
{
    if (!m_presetTable || row < 0 || row >= m_levelPresets.size())
        return;

    const LevelPreset& preset = m_levelPresets.at(row);
    const QString display = presetNameCellText(row);

    m_rebuildingPresetTable = true;
    QTableWidgetItem* item = m_presetTable->item(row, kPresetNameColumn);
    if (!item)
    {
        item = new QTableWidgetItem(display);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_presetTable->setItem(row, kPresetNameColumn, item);
    }
    else
    {
        item->setText(display);
    }
    item->setToolTip(preset.hideName
        ? tr("Preset %1 — name hidden on button (color/icon only)").arg(row + 1)
        : QString());
    item->setIcon(thumbIcon(preset.iconPath));
    if (preset.color.isValid())
        item->setBackground(preset.color);
    else
        item->setBackground(QBrush());
    m_rebuildingPresetTable = false;
}

void MultiButtonConfigDialog::syncPresetTableColumns()
{
    if (!m_presetTable) return;

    const int chanCount    = m_levelChannelBindings.size();
    const int presetCount  = m_levelPresets.size();
    const int tableCols    = kPresetFirstDmxColumn + chanCount;
    const int oldRows      = m_presetTable->rowCount();
    const int oldCols      = m_presetTable->columnCount();
    const bool sameGrid    = (oldRows == presetCount && oldCols == tableCols);

    QVector<QHash<quint64, quint8>> rowByKey(presetCount);

    for (int r = 0; r < presetCount; ++r)
    {
        const LevelPreset& preset = m_levelPresets.at(r);
        for (int c = 0; c < chanCount && c < preset.values.size(); ++c)
        {
            const LevelChannelBinding& b = m_levelChannelBindings.at(c);
            rowByKey[r].insert(bindingKey(b.fixtureId, b.channel), preset.values.at(c));
        }
    }

    if (sameGrid)
    {
        for (int r = 0; r < oldRows && r < presetCount; ++r)
        {
            for (int c = kPresetFirstDmxColumn; c < oldCols; ++c)
            {
                const int bindIdx = c - kPresetFirstDmxColumn;
                if (bindIdx < 0 || bindIdx >= chanCount)
                    continue;
                const LevelChannelBinding& b = m_levelChannelBindings.at(bindIdx);
                quint8 val = presetTableValue(r, c);
                rowByKey[r].insert(bindingKey(b.fixtureId, b.channel), val);
            }
        }
    }

    m_rebuildingPresetTable = true;
    m_presetTable->blockSignals(true);
    m_presetTable->setRowCount(presetCount);
    m_presetTable->setColumnCount(tableCols);

    QStringList hLabels;
    hLabels << tr("Name");
    for (const LevelChannelBinding& b : m_levelChannelBindings)
        hLabels << bindingHeaderLabel(m_doc, b);
    m_presetTable->setHorizontalHeaderLabels(hLabels);

    for (int r = 0; r < presetCount; ++r)
    {
        QList<quint8> newValues;
        for (int col = 0; col < chanCount; ++col)
        {
            const LevelChannelBinding& b = m_levelChannelBindings.at(col);
            const quint8 val = rowByKey[r].value(bindingKey(b.fixtureId, b.channel), 0);
            newValues.append(val);
            m_presetTable->setItem(r, kPresetFirstDmxColumn + col, makeValueTableItem(val));
        }
        m_levelPresets[r].values = newValues;
        updatePresetNameCell(r);
    }

    m_presetTable->resizeColumnsToContents();
    m_presetTable->horizontalHeader()->setStretchLastSection(true);
    m_presetTable->blockSignals(false);
    m_rebuildingPresetTable = false;
}

void MultiButtonConfigDialog::slotPresetTableItemChanged(QTableWidgetItem* item)
{
    if (m_rebuildingPresetTable || !item || !m_presetTable)
        return;

    const int row = item->row();
    const int col = item->column();
    if (row < 0 || row >= m_levelPresets.size())
        return;

    if (col == kPresetNameColumn)
    {
        syncPresetNameFromCell(row, item->text());
        updatePresetNameCell(row);
        return;
    }

    const int valCol = col - kPresetFirstDmxColumn;
    if (valCol < 0 || valCol >= m_levelChannelBindings.size())
        return;

    const quint8 val = parseDmxCell(item->text());
    const QString normalized = QString::number(val);

    m_rebuildingPresetTable = true;
    if (item->text() != normalized)
        item->setText(normalized);
    item->setData(Qt::UserRole, int(val));
    m_rebuildingPresetTable = false;

    LevelPreset& preset = m_levelPresets[row];
    while (preset.values.size() < m_levelChannelBindings.size())
        preset.values.append(0);
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
    int row = m_presetTable->currentRow();
    bool has = (row >= 0);
    int cnt  = m_levelPresets.size();

    m_lvlRemoveBtn->setEnabled(has);
    m_lvlEditLblBtn->setEnabled(has);
    m_lvlScribbleBtn->setEnabled(has);
    m_lvlChooseIconBtn->setEnabled(has);
    m_lvlClearIconBtn->setEnabled(has);
    m_lvlChooseColorBtn->setEnabled(has);
    m_lvlClearColorBtn->setEnabled(has);
    m_lvlUpBtn->setEnabled(has && row > 0);
    m_lvlDownBtn->setEnabled(has && row < cnt - 1);
}

void MultiButtonConfigDialog::slotLevelAddPreset()
{
    LevelPreset preset;
    preset.hideName = false;
    preset.values = QList<quint8>(m_levelChannelBindings.size(), 0);
    m_levelPresets.append(preset);
    syncPresetTableColumns();
    if (!m_levelPresets.isEmpty())
        m_presetTable->setCurrentCell(m_levelPresets.size() - 1, 0);
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelRemovePreset()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    m_levelPresets = levelPresets();
    m_levelPresets.removeAt(row);
    syncPresetTableColumns();
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelEditLabel()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    m_levelPresets = levelPresets();

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
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    ScribbleDialog dlg(m_doc, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_levelPresets = levelPresets();
    m_levelPresets[row].iconPath = dlg.savedIconPath();
    updatePresetNameCell(row);
}

void MultiButtonConfigDialog::slotLevelChooseIcon()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    QString formats;
    for (const QByteArray& ba : QImageReader::supportedImageFormats())
        formats += QString("*.%1 ").arg(QString(ba).toLower());

    m_levelPresets = levelPresets();
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select icon image"), m_levelPresets.at(row).iconPath,
        tr("Images (%1)").arg(formats));
    if (path.isEmpty()) return;

    m_levelPresets[row].iconPath = path;
    updatePresetNameCell(row);
}

void MultiButtonConfigDialog::slotLevelClearIcon()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    m_levelPresets = levelPresets();
    m_levelPresets[row].iconPath.clear();
    updatePresetNameCell(row);
}

void MultiButtonConfigDialog::slotLevelChooseColor()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    m_levelPresets = levelPresets();
    const QColor initial = m_levelPresets.at(row).color.isValid()
        ? m_levelPresets.at(row).color : QColor(Qt::white);

    const QColor chosen = QColorDialog::getColor(initial, this, tr("Preset button color"));
    if (!chosen.isValid())
        return;

    m_levelPresets[row].color = chosen;
    updatePresetNameCell(row);
}

void MultiButtonConfigDialog::slotLevelClearColor()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size()) return;

    m_levelPresets = levelPresets();
    m_levelPresets[row].color = QColor();
    updatePresetNameCell(row);
}

void MultiButtonConfigDialog::slotLevelMoveUp()
{
    int row = m_presetTable->currentRow();
    if (row <= 0 || row >= m_levelPresets.size()) return;

    m_levelPresets = levelPresets();
    m_levelPresets.swapItemsAt(row - 1, row);
    syncPresetTableColumns();
    m_presetTable->setCurrentCell(row - 1, 0);
    slotLevelSelectionChanged();
}

void MultiButtonConfigDialog::slotLevelMoveDown()
{
    int row = m_presetTable->currentRow();
    if (row < 0 || row >= m_levelPresets.size() - 1) return;

    m_levelPresets = levelPresets();
    m_levelPresets.swapItemsAt(row, row + 1);
    syncPresetTableColumns();
    m_presetTable->setCurrentCell(row + 1, 0);
    slotLevelSelectionChanged();
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
    bool has = (m_listWidget->currentRow() >= 0);
    int  row = m_listWidget->currentRow();
    int  cnt = m_listWidget->count();

    m_removeBtn->setEnabled(has);
    m_editLblBtn->setEnabled(has);
    m_scribbleBtn->setEnabled(has);
    m_chooseIconBtn->setEnabled(has);
    m_clearIconBtn->setEnabled(has);
    m_upBtn->setEnabled(has && row > 0);
    m_downBtn->setEnabled(has && row < cnt - 1);
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
    slotSelectionChanged();
}

// ---- Automation tab -------------------------------------------------------

static void syncExcludeMaskFromTable(QTableWidget* table, quint32& mask)
{
    mask = 0;
    if (!table)
        return;
    for (int r = 0; r < table->rowCount(); ++r)
    {
        QTableWidgetItem* ex = table->item(r, 1);
        if (ex && ex->checkState() == Qt::Checked && r < 32)
            mask |= (1u << r);
    }
}

int MultiButtonConfigDialog::entryCountForAutomation() const
{
    if (widgetMode() == MultiButtonMode::Level)
        return m_levelPresets.size();
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

    if (!m_listWidget || index >= m_listWidget->count())
        return QString();
    return m_listWidget->item(index)->text();
}

void MultiButtonConfigDialog::rebuildAutomationExcludeTable()
{
    if (!m_autoExcludeTable)
        return;

    m_rebuildingExcludeTable = true;
    m_autoExcludeTable->setRowCount(0);
    m_autoExcludeTable->clearSpans();

    const int n = entryCountForAutomation();
    if (n <= 0)
    {
        m_autoExcludeTable->setRowCount(1);
        QTableWidgetItem* hint = new QTableWidgetItem(
            tr("Add entries on the Entries tab first."));
        hint->setFlags(Qt::ItemIsEnabled);
        m_autoExcludeTable->setItem(0, 0, hint);
        m_autoExcludeTable->setSpan(0, 0, 1, 2);
        m_rebuildingExcludeTable = false;
        return;
    }

    quint32 mask = 0;
    if (m_activeAutomationProfile >= 0
        && m_activeAutomationProfile < m_automationProfiles.size())
    {
        mask = m_automationProfiles.at(m_activeAutomationProfile).excludeMask;
    }

    m_autoExcludeTable->setRowCount(n);
    for (int i = 0; i < n; ++i)
    {
        QTableWidgetItem* nameItem = new QTableWidgetItem(entryLabelForAutomation(i));
        nameItem->setFlags(Qt::ItemIsEnabled);
        m_autoExcludeTable->setItem(i, 0, nameItem);

        QTableWidgetItem* exItem = new QTableWidgetItem;
        exItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        exItem->setCheckState((mask & (1u << i)) ? Qt::Checked : Qt::Unchecked);
        m_autoExcludeTable->setItem(i, 1, exItem);
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
}

void MultiButtonConfigDialog::loadExcludeTableForProfile(int profileIndex)
{
    if (profileIndex < 0 || profileIndex >= m_automationProfiles.size())
        return;

    m_activeAutomationProfile = profileIndex;
    rebuildAutomationExcludeTable();
}

void MultiButtonConfigDialog::slotAutomationProfileRowChanged(int currentRow, int previousRow)
{
    if (m_syncingAutomationUi || m_rebuildingProfileTable || !m_autoProfileTable)
        return;

    // Exclude table still shows the previous profile — save its mask before reloading.
    if (previousRow >= 0 && previousRow < m_automationProfiles.size()
        && !m_rebuildingExcludeTable)
    {
        syncExcludeMaskFromTable(m_autoExcludeTable,
                                 m_automationProfiles[previousRow].excludeMask);
    }

    syncDataFromProfileTable();

    if (currentRow >= 0)
        loadExcludeTableForProfile(currentRow);
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
    if (item->column() != 1)
        return;

    const int cur = m_autoProfileTable ? m_autoProfileTable->currentRow() : -1;
    if (cur >= 0 && cur < m_automationProfiles.size())
        syncExcludeMaskFromTable(m_autoExcludeTable, m_automationProfiles[cur].excludeMask);
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

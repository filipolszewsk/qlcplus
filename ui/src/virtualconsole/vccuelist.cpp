/*
  Q Light Controller Plus
  vccuelist.cpp

  Copyright (c) Heikki Junnila
                Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QStyledItemDelegate>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTreeWidgetItem>
#include <QFontMetrics>
#include <QProgressBar>
#include <QTreeWidget>
#include <QHeaderView>
#include <QGridLayout>
#include <QSettings>
#include <QCheckBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QLabel>
#include <QDebug>
#include <QTimer>

#include "vccuelistproperties.h"
#include "vcpropertieseditor.h"
#include "clickandgoslider.h"
#include "chaserrunner.h"
#include "mastertimer.h"
#include "chaserstep.h"
#include "vccuelist.h"
#include "qlcmacros.h"
#include "function.h"
#include "vcwidget.h"
#include "apputil.h"
#include "chaser.h"
#include "qmath.h"
#include "doc.h"
#include "scene.h"
#include "universe.h"
#include "fixture.h"
#include "scenevalue.h"
#include "genericfader.h"
#include "fadechannel.h"
#include "qlcchannel.h"
#include "channelcolumneditor.h"
#include "columnvisibilitydialog.h"

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QMenu>
#include <QAction>

#include "qlcclipboard.h"

/*****************************************************************************
 * ChannelValueDelegate - delegate for editing channel values
 * Supports Raw (SpinBox), Scaled (SpinBox with conversion), and Dropdown modes
 *****************************************************************************/

class ChannelValueDelegate : public QStyledItemDelegate
{
public:
    ChannelValueDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_columnInfo(nullptr)
    {}

    void setColumnInfo(const ChannelColumnInfo *info) { m_columnInfo = info; }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);

        if (m_columnInfo && m_columnInfo->displayMode == DisplayDropdown &&
            !m_columnInfo->dropdownMappings.isEmpty())
        {
            QComboBox *combo = new QComboBox(parent);
            combo->setFrame(false);

            // Add mappings sorted by DMX value
            QMapIterator<int, QString> it(m_columnInfo->dropdownMappings);
            while (it.hasNext())
            {
                it.next();
                combo->addItem(it.value(), it.key());
            }
            return combo;
        }
        else if (m_columnInfo && m_columnInfo->displayMode == DisplayScaled)
        {
            QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
            editor->setFrame(false);
            editor->setDecimals(1);
            editor->setMinimum(m_columnInfo->scaleMin);
            editor->setMaximum(m_columnInfo->scaleMax);
            return editor;
        }
        else
        {
            QSpinBox *editor = new QSpinBox(parent);
            editor->setFrame(false);
            editor->setMinimum(0);
            editor->setMaximum(255);
            return editor;
        }
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        int dmxValue = index.model()->data(index, Qt::UserRole).toInt();

        if (QComboBox *combo = qobject_cast<QComboBox*>(editor))
        {
            // Find nearest lower mapping
            int bestIdx = 0;
            int bestValue = -1;
            for (int i = 0; i < combo->count(); ++i)
            {
                int mapValue = combo->itemData(i).toInt();
                if (mapValue <= dmxValue && mapValue > bestValue)
                {
                    bestValue = mapValue;
                    bestIdx = i;
                }
            }
            combo->setCurrentIndex(bestIdx);
        }
        else if (QDoubleSpinBox *dblSpinBox = qobject_cast<QDoubleSpinBox*>(editor))
        {
            // Convert DMX (0-255) to scaled value
            double scaled = m_columnInfo->scaleMin +
                (dmxValue / 255.0) * (m_columnInfo->scaleMax - m_columnInfo->scaleMin);
            dblSpinBox->setValue(scaled);
        }
        else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor))
        {
            spinBox->setValue(dmxValue);
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        int dmxValue = 0;

        if (QComboBox *combo = qobject_cast<QComboBox*>(editor))
        {
            dmxValue = combo->currentData().toInt();
        }
        else if (QDoubleSpinBox *dblSpinBox = qobject_cast<QDoubleSpinBox*>(editor))
        {
            dblSpinBox->interpretText();
            double scaled = dblSpinBox->value();
            // Convert scaled value back to DMX (0-255)
            if (m_columnInfo && m_columnInfo->scaleMax != m_columnInfo->scaleMin)
            {
                dmxValue = qRound(((scaled - m_columnInfo->scaleMin) /
                    (m_columnInfo->scaleMax - m_columnInfo->scaleMin)) * 255.0);
                dmxValue = qBound(0, dmxValue, 255);
            }
        }
        else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor))
        {
            spinBox->interpretText();
            dmxValue = spinBox->value();
        }

        model->setData(index, dmxValue, Qt::EditRole);
        model->setData(index, dmxValue, Qt::UserRole);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        editor->setGeometry(option.rect);
    }

    QString displayText(const QVariant &value, const QLocale &locale) const override
    {
        Q_UNUSED(locale);

        int dmxValue = value.toInt();

        if (!m_columnInfo)
            return QString::number(dmxValue);

        switch (m_columnInfo->displayMode)
        {
            case DisplayDropdown:
            {
                // Find nearest lower mapping
                QString label;
                int bestValue = -1;
                QMapIterator<int, QString> it(m_columnInfo->dropdownMappings);
                while (it.hasNext())
                {
                    it.next();
                    if (it.key() <= dmxValue && it.key() > bestValue)
                    {
                        bestValue = it.key();
                        label = it.value();
                    }
                }
                return label.isEmpty() ? QString::number(dmxValue) : label;
            }

            case DisplayScaled:
            {
                double scaled = m_columnInfo->scaleMin +
                    (dmxValue / 255.0) * (m_columnInfo->scaleMax - m_columnInfo->scaleMin);
                return QString::number(scaled, 'f', 1) + m_columnInfo->scaleSuffix;
            }

            case DisplayRaw:
            default:
                return QString::number(dmxValue);
        }
    }

private:
    const ChannelColumnInfo *m_columnInfo;
};

#define COL_NUM      0
#define COL_NAME     1
#define COL_FADEIN   2
#define COL_FADEOUT  3
#define COL_DURATION 4
#define COL_NOTES    5

#define PROP_ID  Qt::UserRole
#define HYSTERESIS 3 // Hysteresis for next/previous external input

#define PROGRESS_INTERVAL 200
#define UPDATE_TIMEOUT 100

const quint8 VCCueList::nextInputSourceId = 0;
const quint8 VCCueList::previousInputSourceId = 1;
const quint8 VCCueList::playbackInputSourceId = 2;
const quint8 VCCueList::sideFaderInputSourceId = 3;
const quint8 VCCueList::stopInputSourceId = 4;
const quint8 VCCueList::recordInputSourceId = 5;
const quint8 VCCueList::overwriteInputSourceId = 6;
const quint8 VCCueList::deleteInputSourceId = 7;
const quint8 VCCueList::secondarySelectInputSourceId = 8;
const quint8 VCCueList::renameInputSourceId = 9;

const QString progressDisabledStyle =
        "QProgressBar { border: 2px solid #C3C3C3; border-radius: 4px; background-color: #DCDCDC; }";
const QString progressFadeStyle =
        "QProgressBar { border: 2px solid grey; border-radius: 4px; background-color: #C3C3C3; text-align: center; }"
        "QProgressBar::chunk { background-color: #63C10B; width: 1px; }";
const QString progressHoldStyle =
        "QProgressBar { border: 2px solid grey; border-radius: 4px; background-color: #C3C3C3; text-align: center; }"
        "QProgressBar::chunk { background-color: #0F9BEC; width: 1px; }";

const QString cfLabelBlueStyle =
        "QLabel { background-color: #4E8DDE; color: white; border: 1px solid; border-radius: 3px; font: bold; }";
const QString cfLabelOrangeStyle =
        "QLabel { background-color: orange; color: black; border: 1px solid; border-radius: 3px; font: bold; }";
const QString cfLabelNoStyle =
        "QLabel { border: 1px solid; border-radius: 3px; font: bold; }";

VCCueList::VCCueList(QWidget *parent, Doc *doc) : VCWidget(parent, doc)
    , m_chaserID(Function::invalidId())
    , m_nextPrevBehavior(DefaultRunFirst)
    , m_playbackLayout(PlayPauseStop)
    , m_autoStartInOperate(false)
    , m_autoStartOffset(0)
    , m_timer(NULL)
    , m_primaryIndex(0)
    , m_secondaryIndex(0)
    , m_primaryTop(true)
    , m_slidersMode(None)
    , m_nextPrevControlsSecondary(false)
    , m_recordAllChannels(true)
    , m_recordNonZeroOnly(false)
    , m_recordCuePrefix("cue")
    , m_isRecordDialogOpen(false)
    , m_showChannelColumns(false)
    , m_hideButtons(false)
    , m_bottomControlsWidget(nullptr)
    , m_stepIndexOutputEnabled(false)
    , m_stepIndexOutputFixture(Function::invalidId())
    , m_stepIndexOutputChannel(0)
    , m_currentStepIndexValue(-1)
{
    /* Set the class name "VCCueList" as the object name as well */
    setObjectName(VCCueList::staticMetaObject.className());

    /* Create a layout for this widget */
    QGridLayout *grid = new QGridLayout(this);
    grid->setSpacing(2);

    QFontMetrics m_fm = QFontMetrics(this->font());

    m_topPercentageLabel = new QLabel("100%");
    m_topPercentageLabel->setAlignment(Qt::AlignHCenter);
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
    m_topPercentageLabel->setFixedWidth(m_fm.width("100%"));
#else
    m_topPercentageLabel->setFixedWidth(m_fm.horizontalAdvance("100%"));
#endif
    grid->addWidget(m_topPercentageLabel, 1, 0, 1, 1);

    m_topStepLabel = new QLabel("");
    m_topStepLabel->setStyleSheet(cfLabelNoStyle);
    m_topStepLabel->setAlignment(Qt::AlignCenter);
    m_topStepLabel->setFixedSize(32, 24);
    grid->addWidget(m_topStepLabel, 2, 0, 1, 1);

    m_sideFader = new ClickAndGoSlider();
    m_sideFader->setSliderStyleSheet(CNG_DEFAULT_STYLE);
    m_sideFader->setFixedWidth(32);
    m_sideFader->setRange(0, 100);
    m_sideFader->setValue(100);
    grid->addWidget(m_sideFader, 3, 0, 1, 1);

    m_bottomStepLabel = new QLabel("");
    m_bottomStepLabel->setStyleSheet(cfLabelNoStyle);
    m_bottomStepLabel->setAlignment(Qt::AlignCenter);
    m_bottomStepLabel->setFixedSize(32, 24);
    grid->addWidget(m_bottomStepLabel, 4, 0, 1, 1);

    m_bottomPercentageLabel = new QLabel("0%");
    m_bottomPercentageLabel->setAlignment(Qt::AlignHCenter);
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
    m_bottomPercentageLabel->setFixedWidth(m_fm.width("100%"));
#else
    m_bottomPercentageLabel->setFixedWidth(m_fm.horizontalAdvance("100%"));
#endif
    grid->addWidget(m_bottomPercentageLabel, 5, 0, 1, 1);

    connect(m_sideFader, SIGNAL(valueChanged(int)),
            this, SLOT(slotSideFaderValueChanged(int)));

    slotShowCrossfadePanel(false);

    QVBoxLayout *vbox = new QVBoxLayout();

    /* Create a list for scenes (cues) */
    m_tree = new QTreeWidget(this);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    //m_tree->setAlternatingRowColors(true);
    m_tree->setAllColumnsShowFocus(true);
    m_tree->setRootIsDecorated(false);
    m_tree->setItemsExpandable(false);
    m_tree->header()->setSortIndicatorShown(false);
    m_tree->header()->setMinimumSectionSize(0); // allow columns to be hidden
    m_tree->header()->setSectionsClickable(true);  // Enable for double-click rename of channel columns
    m_tree->header()->setSectionsMovable(false);

    // Connect header double-click for renaming channel columns
    connect(m_tree->header(), SIGNAL(sectionDoubleClicked(int)),
            this, SLOT(slotHeaderDoubleClicked(int)));

    // Make only the notes column editable
    m_tree->setItemDelegateForColumn(COL_NUM, new NoEditDelegate(this));
    m_tree->setItemDelegateForColumn(COL_NAME, new NoEditDelegate(this));
    m_tree->setItemDelegateForColumn(COL_FADEIN, new NoEditDelegate(this));
    m_tree->setItemDelegateForColumn(COL_FADEOUT, new NoEditDelegate(this));
    m_tree->setItemDelegateForColumn(COL_DURATION, new NoEditDelegate(this));

    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::customContextMenuRequested,
            this, &VCCueList::slotTreeContextMenu);

    connect(m_tree, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(slotItemActivated(QTreeWidgetItem*)));
    connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotItemClicked(QTreeWidgetItem*)));
    connect(m_tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(slotItemChanged(QTreeWidgetItem*,int)));
    vbox->addWidget(m_tree);

    m_progress = new QProgressBar(this);
    m_progress->setOrientation(Qt::Horizontal);
    m_progress->setStyleSheet(progressDisabledStyle);
    m_progress->setProperty("status", 0);
    m_progress->setFixedHeight(20);
    vbox->addWidget(m_progress);

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(slotProgressTimeout()));

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, SIGNAL(timeout()),
            this, SLOT(slotUpdateStepList()));
    m_updateTimer->setSingleShot(true);

    // Deferred function deletion: MasterTimer::functionStopped fires from the engine thread
    // after the function pointer is queued for removal from m_functionList.
    // QueuedConnection ensures the slot runs in the UI thread after that tick completes.
    connect(m_doc->masterTimer(), SIGNAL(functionStopped(quint32)),
            this, SLOT(slotPendingFunctionStopped(quint32)),
            Qt::QueuedConnection);

    /* Create control buttons */
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setSpacing(2);

    m_crossfadeButton = new QToolButton(this);
    m_crossfadeButton->setIcon(QIcon(":/slider.png"));
    m_crossfadeButton->setIconSize(QSize(24, 24));
    m_crossfadeButton->setCheckable(true);
    m_crossfadeButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_crossfadeButton->setFixedHeight(32);
    m_crossfadeButton->setToolTip(tr("Show/Hide crossfade sliders"));
    m_crossfadeButton->setVisible(false);
    connect(m_crossfadeButton, SIGNAL(toggled(bool)),
            this, SLOT(slotShowCrossfadePanel(bool)));
    hbox->addWidget(m_crossfadeButton);

    m_playbackButton = new QToolButton(this);
    m_playbackButton->setIcon(QIcon(":/player_play.png"));
    m_playbackButton->setIconSize(QSize(24, 24));
    m_playbackButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_playbackButton->setFixedHeight(32);
    m_playbackButton->setToolTip(tr("Play/Pause Cue list"));
    connect(m_playbackButton, SIGNAL(clicked()), this, SLOT(slotPlayback()));
    hbox->addWidget(m_playbackButton);

    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(QIcon(":/player_stop.png"));
    m_stopButton->setIconSize(QSize(24, 24));
    m_stopButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_stopButton->setFixedHeight(32);
    m_stopButton->setToolTip(tr("Stop Cue list"));
    connect(m_stopButton, SIGNAL(clicked()), this, SLOT(slotStop()));
    hbox->addWidget(m_stopButton);

    m_previousButton = new QToolButton(this);
    m_previousButton->setIcon(QIcon(":/back.png"));
    m_previousButton->setIconSize(QSize(24, 24));
    m_previousButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_previousButton->setFixedHeight(32);
    m_previousButton->setToolTip(tr("Go to previous step in the list"));
    connect(m_previousButton, SIGNAL(clicked()), this, SLOT(slotPreviousCue()));
    hbox->addWidget(m_previousButton);

    m_nextButton = new QToolButton(this);
    m_nextButton->setIcon(QIcon(":/forward.png"));
    m_nextButton->setIconSize(QSize(24, 24));
    m_nextButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_nextButton->setFixedHeight(32);
    m_nextButton->setToolTip(tr("Go to next step in the list"));
    connect(m_nextButton, SIGNAL(clicked()), this, SLOT(slotNextCue()));
    hbox->addWidget(m_nextButton);

    m_recordButton = new QToolButton(this);
    m_recordButton->setIcon(QIcon(":/record.png"));
    m_recordButton->setIconSize(QSize(24, 24));
    m_recordButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_recordButton->setFixedHeight(32);
    m_recordButton->setToolTip(tr("Record current DMX values as new cue"));
    m_recordButton->setEnabled(false);
    connect(m_recordButton, SIGNAL(clicked()), this, SLOT(slotRecordButtonClicked()));
    hbox->addWidget(m_recordButton);

    m_overwriteButton = new QToolButton(this);
    m_overwriteButton->setIcon(QIcon(":/edit.png"));
    m_overwriteButton->setIconSize(QSize(24, 24));
    m_overwriteButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_overwriteButton->setFixedHeight(32);
    m_overwriteButton->setToolTip(tr("Overwrite selected cue with current DMX values"));
    m_overwriteButton->setEnabled(false);
    connect(m_overwriteButton, SIGNAL(clicked()), this, SLOT(slotOverwriteButtonClicked()));
    hbox->addWidget(m_overwriteButton);

    m_deleteButton = new QToolButton(this);
    m_deleteButton->setIcon(QIcon(":/editdelete.png"));
    m_deleteButton->setIconSize(QSize(24, 24));
    m_deleteButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_deleteButton->setFixedHeight(32);
    m_deleteButton->setToolTip(tr("Delete selected cue"));
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteButtonClicked()));
    hbox->addWidget(m_deleteButton);

    m_renameButton = new QToolButton(this);
    m_renameButton->setIcon(QIcon(":/editclear.png"));
    m_renameButton->setIconSize(QSize(24, 24));
    m_renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_renameButton->setFixedHeight(32);
    m_renameButton->setToolTip(tr("Rename selected cue"));
    m_renameButton->setEnabled(false);
    connect(m_renameButton, SIGNAL(clicked()), this, SLOT(slotRenameButtonClicked()));
    hbox->addWidget(m_renameButton);

    m_bottomControlsWidget = new QWidget(this);
    m_bottomControlsWidget->setLayout(hbox);
    vbox->addWidget(m_bottomControlsWidget);
    grid->addItem(vbox, 0, 1, 6);

    setFrameStyle(KVCFrameStyleSunken);
    setType(VCWidget::CueListWidget);
    setCaption(tr("Cue list"));

    QSettings settings;
    QVariant var = settings.value(SETTINGS_CUELIST_SIZE);
    if (var.isValid() == true)
        resize(var.toSize());
    else
        resize(QSize(300, 220));

    slotModeChanged(m_doc->mode());
    setLiveEdit(m_liveEdit);

    connect(m_doc, SIGNAL(functionRemoved(quint32)),
            this, SLOT(slotFunctionRemoved(quint32)));
    connect(m_doc, SIGNAL(functionChanged(quint32)),
            this, SLOT(slotFunctionChanged(quint32)));
    connect(m_doc, SIGNAL(functionNameChanged(quint32)),
            this, SLOT(slotFunctionNameChanged(quint32)));

    m_nextLatestValue = 0;
    m_previousLatestValue = 0;
    m_playbackLatestValue = 0;
    m_stopLatestValue = 0;
    m_recordLatestValue = 0;
    m_overwriteLatestValue = 0;
    m_deleteLatestValue = 0;
    m_renameLatestValue = 0;
}

VCCueList::~VCCueList()
{
    // Unregister DMXSource if registered
    if (m_stepIndexOutputEnabled && m_stepIndexOutputFixture != Function::invalidId())
    {
        if (m_doc->masterTimer() != NULL)
            m_doc->masterTimer()->unregisterDMXSource(this);
    }
}

void VCCueList::enableWidgetUI(bool enable)
{
    // Tree stays interactive in Design mode so the user can select rows
    // for context-menu copy/paste of step channel values.
    // Playback-triggering actions are already guarded by mode() checks.
    m_tree->setEnabled(true);
    m_playbackButton->setEnabled(enable);
    m_stopButton->setEnabled(enable);
    m_previousButton->setEnabled(enable);
    m_nextButton->setEnabled(enable);
    // Record button is enabled only in Operate mode, when chaser is attached, and not for Sequence
    Function *func = m_doc->function(m_chaserID);
    bool isSequence = (func != NULL && func->type() == Function::SequenceType);
    m_recordButton->setEnabled(enable && m_chaserID != Function::invalidId() && !isSequence);

    // Overwrite button is enabled only in Operate mode, when chaser is attached, stopped, and item is selected
    Chaser *ch = chaser();
    bool chaserStopped = (ch != NULL && ch->stopped());
    QTreeWidgetItem *selectedItem = m_tree->currentItem();
    bool hasSelectedItem = (selectedItem != NULL);
    bool overwriteEnabled = false;
    bool deleteEnabled = false;
    if (enable && m_chaserID != Function::invalidId() && !isSequence && chaserStopped && hasSelectedItem)
    {
        // Check if selected item is a Scene
        int stepIndex = m_tree->indexOfTopLevelItem(selectedItem);
        if (stepIndex >= 0 && stepIndex < ch->steps().count())
        {
            ChaserStep step = ch->steps().at(stepIndex);
            Function *stepFunc = m_doc->function(step.fid);
            if (stepFunc != NULL && stepFunc->type() == Function::SceneType)
                overwriteEnabled = true;
            // Delete is enabled for any step type
            deleteEnabled = true;
        }
    }
    m_overwriteButton->setEnabled(overwriteEnabled);
    m_deleteButton->setEnabled(deleteEnabled);
    m_renameButton->setEnabled(deleteEnabled);

    m_topPercentageLabel->setEnabled(enable);
    m_sideFader->setEnabled(enable);
    m_topStepLabel->setEnabled(enable);
    m_bottomPercentageLabel->setEnabled(enable);
    m_bottomStepLabel->setEnabled(enable);
}

/*****************************************************************************
 * Clipboard
 *****************************************************************************/

VCWidget *VCCueList::createCopy(VCWidget *parent)
{
    Q_ASSERT(parent != NULL);

    VCCueList *cuelist = new VCCueList(parent, m_doc);
    if (cuelist->copyFrom(this) == false)
    {
        delete cuelist;
        cuelist = NULL;
    }

    return cuelist;
}

bool VCCueList::copyFrom(const VCWidget *widget)
{
    const VCCueList *cuelist = qobject_cast<const VCCueList*> (widget);
    if (cuelist == NULL)
        return false;

    /* Function list contents */
    setChaser(cuelist->chaserID());

    /* Key sequence */
    setNextKeySequence(cuelist->nextKeySequence());
    setPreviousKeySequence(cuelist->previousKeySequence());
    setPlaybackKeySequence(cuelist->playbackKeySequence());
    setStopKeySequence(cuelist->stopKeySequence());
    setRecordKeySequence(cuelist->recordKeySequence());
    setOverwriteKeySequence(cuelist->overwriteKeySequence());
    setDeleteKeySequence(cuelist->deleteKeySequence());

    /* Sliders mode */
    setSideFaderMode(cuelist->sideFaderMode());

    /* Next/Prev controls secondary */
    setNextPrevControlsSecondary(cuelist->nextPrevControlsSecondary());

    /* Recording settings */
    setRecordAllChannels(cuelist->recordAllChannels());
    setRecordNonZeroOnly(cuelist->recordNonZeroOnly());
    setRecordChannelsMask(cuelist->recordChannelsMask());
    setRecordCuePrefix(cuelist->recordCuePrefix());

    /* Step Index Output settings */
    setStepIndexOutputEnabled(cuelist->stepIndexOutputEnabled());
    setStepIndexOutputFixture(cuelist->stepIndexOutputFixture());
    setStepIndexOutputChannel(cuelist->stepIndexOutputChannel());

    /* Auto start */
    setAutoStartInOperate(cuelist->autoStartInOperate());
    setAutoStartOffset(cuelist->autoStartOffset());

    /* Key sequence - rename */
    setRenameKeySequence(cuelist->renameKeySequence());

    /* Hide buttons */
    setHideButtons(cuelist->hideButtons());

    /* Channel columns - copy directly to preserve custom names/modes/mappings */
    m_channelColumns = cuelist->m_channelColumns;
    m_showChannelColumns = cuelist->m_showChannelColumns;
    if (m_showChannelColumns)
    {
        int colOffset = COL_NOTES + 1;
        for (int i = 0; i < m_channelColumns.size(); i++)
        {
            ChannelValueDelegate *delegate = new ChannelValueDelegate(this);
            delegate->setColumnInfo(&m_channelColumns[i]);
            m_tree->setItemDelegateForColumn(colOffset + i, delegate);
        }
        updateTreeHeader();
        updateStepList();
        applyColumnHiddenState();
    }

    /* Hidden fixed columns */
    m_hiddenFixedColumns = cuelist->m_hiddenFixedColumns;
    applyFixedColumnHiddenState();

    /* Common stuff */
    return VCWidget::copyFrom(widget);
}

QList<QPair<VCWidget::PastePropertyGroup, QString>> VCCueList::pasteablePropertyGroups() const
{
    QList<QPair<PastePropertyGroup, QString>> groups = VCWidget::pasteablePropertyGroups();
    groups << qMakePair(PasteSpecific0, tr("Chaser Function"));
    groups << qMakePair(PasteSpecific1, tr("Key Sequences"));
    groups << qMakePair(PasteSpecific2, tr("Side Fader Mode"));
    groups << qMakePair(PasteSpecific3, tr("Recording Settings"));
    groups << qMakePair(PasteSpecific4, tr("Step Index Output"));
    groups << qMakePair(PasteSpecific5, tr("Channel Columns"));
    groups << qMakePair(PasteSpecific6, tr("Step Channel Values"));
    return groups;
}

void VCCueList::applyPropertiesFrom(const VCWidget* source, PastePropertyGroups flags)
{
    const VCCueList* cuelist = qobject_cast<const VCCueList*>(source);
    if (cuelist == nullptr)
        return;

    if (flags & PasteSpecific0)
        setChaser(cuelist->chaserID());

    if (flags & PasteSpecific1)
    {
        setNextKeySequence(cuelist->nextKeySequence());
        setPreviousKeySequence(cuelist->previousKeySequence());
        setPlaybackKeySequence(cuelist->playbackKeySequence());
        setStopKeySequence(cuelist->stopKeySequence());
        setRecordKeySequence(cuelist->recordKeySequence());
        setOverwriteKeySequence(cuelist->overwriteKeySequence());
        setDeleteKeySequence(cuelist->deleteKeySequence());
        setRenameKeySequence(cuelist->renameKeySequence());
        setNextPrevControlsSecondary(cuelist->nextPrevControlsSecondary());
    }

    if (flags & PasteSpecific2)
        setSideFaderMode(cuelist->sideFaderMode());

    if (flags & PasteSpecific3)
    {
        setRecordAllChannels(cuelist->recordAllChannels());
        setRecordNonZeroOnly(cuelist->recordNonZeroOnly());
        setRecordChannelsMask(cuelist->recordChannelsMask());
        setRecordCuePrefix(cuelist->recordCuePrefix());
        setAutoStartInOperate(cuelist->autoStartInOperate());
        setAutoStartOffset(cuelist->autoStartOffset());
        setHideButtons(cuelist->hideButtons());
    }

    if (flags & PasteSpecific4)
    {
        setStepIndexOutputEnabled(cuelist->stepIndexOutputEnabled());
        setStepIndexOutputFixture(cuelist->stepIndexOutputFixture());
        setStepIndexOutputChannel(cuelist->stepIndexOutputChannel());
    }

    if (flags & PasteSpecific5)
    {
        // Copy only display parameters by column index.
        // Channel references (address/fixture) stay from this cuelist's recording mask.
        int copyCount = qMin(cuelist->m_channelColumns.size(),
                             m_channelColumns.size());
        for (int i = 0; i < copyCount; i++)
        {
            const ChannelColumnInfo &src = cuelist->m_channelColumns.at(i);
            ChannelColumnInfo &dst = m_channelColumns[i];
            dst.customName       = src.customName;
            dst.displayMode      = src.displayMode;
            dst.dropdownMappings = src.dropdownMappings;
            dst.scaleMin         = src.scaleMin;
            dst.scaleMax         = src.scaleMax;
            dst.scaleSuffix      = src.scaleSuffix;
            dst.hidden           = src.hidden;
            // absoluteAddress / fixtureId / fixtureChannel / activeInMask
            // remain from this cuelist's own recording mask
        }
        m_showChannelColumns = cuelist->m_showChannelColumns;
        if (m_showChannelColumns)
        {
            int colOffset = COL_NOTES + 1;
            for (int i = 0; i < m_channelColumns.size(); i++)
            {
                if (m_channelColumns[i].activeInMask)
                {
                    ChannelValueDelegate *delegate = new ChannelValueDelegate(this);
                    delegate->setColumnInfo(&m_channelColumns[i]);
                    m_tree->setItemDelegateForColumn(colOffset + i, delegate);
                }
            }
            updateTreeHeader();
            updateStepList();
            applyColumnHiddenState();
        }
        m_hiddenFixedColumns = cuelist->m_hiddenFixedColumns;
        applyFixedColumnHiddenState();
    }

    if (flags & PasteSpecific6)
    {
        // Copy step channel values by column index.
        // Values from source column i go into target column i's fixture/channel —
        // as if the user had manually typed them into those cells.
        Chaser *srcChaser = qobject_cast<Chaser*>(m_doc->function(cuelist->chaserID()));
        Chaser *dstChaser = chaser();

        if (srcChaser != nullptr && dstChaser != nullptr)
        {
            int stepCount = qMin(srcChaser->steps().count(),
                                 dstChaser->steps().count());
            int colCount  = qMin(cuelist->m_channelColumns.size(),
                                 m_channelColumns.size());

            for (int stepIdx = 0; stepIdx < stepCount; stepIdx++)
            {
                Scene *srcScene = qobject_cast<Scene*>(
                    m_doc->function(srcChaser->steps().at(stepIdx).fid));
                Scene *dstScene = qobject_cast<Scene*>(
                    m_doc->function(dstChaser->steps().at(stepIdx).fid));

                if (srcScene == nullptr || dstScene == nullptr)
                    continue;

                for (int colIdx = 0; colIdx < colCount; colIdx++)
                {
                    const ChannelColumnInfo &srcCol =
                        cuelist->m_channelColumns.at(colIdx);
                    const ChannelColumnInfo &dstCol =
                        m_channelColumns.at(colIdx);

                    if (!srcCol.activeInMask || srcCol.fixtureId == UINT_MAX)
                        continue;
                    if (!dstCol.activeInMask || dstCol.fixtureId == UINT_MAX)
                        continue;

                    uchar val = srcScene->value(srcCol.fixtureId,
                                                srcCol.fixtureChannel);
                    dstScene->setValue(SceneValue(dstCol.fixtureId,
                                                  dstCol.fixtureChannel, val));
                }
            }

            updateStepList();
        }
    }

    VCWidget::applyPropertiesFrom(source, flags);
    m_doc->setModified();
}

/*****************************************************************************
 * Cue list
 *****************************************************************************/

void VCCueList::setChaser(quint32 id)
{
    Function *old = m_doc->function(m_chaserID);
    if (old != NULL)
    {
        /* Get rid of old function connections */
        disconnect(old, SIGNAL(running(quint32)),
                this, SLOT(slotFunctionRunning(quint32)));
        disconnect(old, SIGNAL(stopped(quint32)),
                this, SLOT(slotFunctionStopped(quint32)));
        disconnect(old, SIGNAL(currentStepChanged(int)),
                this, SLOT(slotCurrentStepChanged(int)));
    }

    Chaser *chaser = qobject_cast<Chaser*> (m_doc->function(id));
    if (chaser != NULL)
    {
        /* Connect to the new function */
        connect(chaser, SIGNAL(running(quint32)),
                this, SLOT(slotFunctionRunning(quint32)));
        connect(chaser, SIGNAL(stopped(quint32)),
                this, SLOT(slotFunctionStopped(quint32)));
        connect(chaser, SIGNAL(currentStepChanged(int)),
                this, SLOT(slotCurrentStepChanged(int)));

        m_chaserID = id;
    }
    else
    {
        m_chaserID = Function::invalidId();
    }

    updateStepList();

    /* Update record button state */
    if (m_recordButton != NULL)
    {
        Function *func = m_doc->function(m_chaserID);
        bool isSequence = (func != NULL && func->type() == Function::SequenceType);
        bool enable = (m_doc->mode() == Doc::Operate && 
                      m_chaserID != Function::invalidId() && 
                      !isSequence);
        m_recordButton->setEnabled(enable);
    }

    /* Update overwrite button state */
    if (m_overwriteButton != NULL)
    {
        enableWidgetUI(m_doc->mode() == Doc::Operate);
    }

    /* Current status */
    if (chaser != NULL && chaser->isRunning())
    {
        slotFunctionRunning(m_chaserID);
        slotCurrentStepChanged(chaser->currentStepIndex());
    }
    else
        slotFunctionStopped(m_chaserID);
}

quint32 VCCueList::chaserID() const
{
    return m_chaserID;
}

QList<quint32> VCCueList::referencedFunctions() const
{
    if (m_chaserID != Function::invalidId())
        return QList<quint32>() << m_chaserID;
    return QList<quint32>();
}

Chaser *VCCueList::chaser()
{
    if (m_chaserID == Function::invalidId())
        return NULL;
    Chaser *chaser = qobject_cast<Chaser*>(m_doc->function(m_chaserID));
    return chaser;
}

void VCCueList::updateStepList()
{
    m_listIsUpdating = true;

    m_tree->clear();

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    QListIterator <ChaserStep> it(ch->steps());
    while (it.hasNext() == true)
    {
        ChaserStep step(it.next());

        Function *function = m_doc->function(step.fid);
        Q_ASSERT(function != NULL);

        QTreeWidgetItem *item = new QTreeWidgetItem(m_tree);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        int index = m_tree->indexOfTopLevelItem(item) + 1;
        item->setText(COL_NUM, QString("%1").arg(index));
        item->setData(COL_NUM, PROP_ID, function->id());
        item->setText(COL_NAME, function->name());
        if (step.note.isEmpty() == false)
            item->setText(COL_NOTES, step.note);

        switch (ch->fadeInMode())
        {
            case Chaser::Common:
                item->setText(COL_FADEIN, Function::speedToString(ch->fadeInSpeed()));
                break;
            case Chaser::PerStep:
                item->setText(COL_FADEIN, Function::speedToString(step.fadeIn));
                break;
            default:
            case Chaser::Default:
                item->setText(COL_FADEIN, QString());
        }

        switch (ch->fadeOutMode())
        {
            case Chaser::Common:
                item->setText(COL_FADEOUT, Function::speedToString(ch->fadeOutSpeed()));
                break;
            case Chaser::PerStep:
                item->setText(COL_FADEOUT, Function::speedToString(step.fadeOut));
                break;
            default:
            case Chaser::Default:
                item->setText(COL_FADEOUT, QString());
        }

        switch (ch->durationMode())
        {
            case Chaser::Common:
                item->setText(COL_DURATION, Function::speedToString(ch->duration()));
                break;
            case Chaser::PerStep:
                item->setText(COL_DURATION, Function::speedToString(step.duration));
                break;
            default:
            case Chaser::Default:
                item->setText(COL_DURATION, QString());
        }

        // Add channel values if channel columns are enabled
        if (m_showChannelColumns && !m_channelColumns.isEmpty())
        {
            Scene *scene = qobject_cast<Scene*>(function);
            int colOffset = COL_NOTES + 1;
            for (int i = 0; i < m_channelColumns.size(); i++)
            {
                const ChannelColumnInfo &col = m_channelColumns.at(i);
                if (!col.activeInMask)
                {
                    item->setText(colOffset + i, "-");
                    item->setData(colOffset + i, Qt::ForegroundRole,
                                  QColor(Qt::gray));
                }
                else if (scene != nullptr && col.fixtureId != UINT_MAX)
                {
                    uchar value = scene->value(col.fixtureId, col.fixtureChannel);
                    item->setData(colOffset + i, Qt::UserRole, value);
                    item->setText(colOffset + i, QString::number(value));
                    // Ensure foreground is reset to default in case it was grayed before
                    item->setData(colOffset + i, Qt::ForegroundRole, QVariant());
                }
                else
                {
                    item->setText(colOffset + i, "-");
                }
            }
        }
    }

    QTreeWidgetItem *item = m_tree->topLevelItem(0);
    if (item != NULL)
        m_defCol = item->background(COL_NUM);

    m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
    m_tree->header()->setSectionHidden(COL_NAME, ch->type() == Function::SequenceType ? true : false);
    m_listIsUpdating = false;
}

int VCCueList::getCurrentIndex()
{
    int index = m_tree->indexOfTopLevelItem(m_tree->currentItem());
    if (index == -1)
        index = 0;
    return index;
}

int VCCueList::getNextIndex()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return -1;

    if (ch->direction() == Function::Forward)
        return getNextTreeIndex();
    else
        return getPrevTreeIndex();
}

int VCCueList::getPrevIndex()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return -1;

    if (ch->direction() == Function::Forward)
        return getPrevTreeIndex();
    else
        return getNextTreeIndex();
}

int VCCueList::getFirstIndex()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return -1;

    if (ch->direction() == Function::Forward)
        return getFirstTreeIndex();
    else
        return getLastTreeIndex();
}

int VCCueList::getLastIndex()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return -1;

    if (ch->direction() == Function::Forward)
        return getLastTreeIndex();
    else
        return getFirstTreeIndex();
}

int VCCueList::getNextTreeIndex()
{
    int count = m_tree->topLevelItemCount();
    if (count > 0)
        return (getCurrentIndex() + 1) % count;
    return 0;
}

int VCCueList::getPrevTreeIndex()
{
    int currentIndex = getCurrentIndex();
    if (currentIndex <= 0)
        return getLastTreeIndex();
    return currentIndex - 1;
}

int VCCueList::getFirstTreeIndex()
{
    return 0;
}

int VCCueList::getLastTreeIndex()
{
    return m_tree->topLevelItemCount() - 1;
}

qreal VCCueList::getPrimaryIntensity() const
{
    if (sideFaderMode() == Steps)
        return  1.0;

    return m_primaryTop ? qreal(m_sideFader->value() / 100.0) : qreal((100 - m_sideFader->value()) / 100.0);
}

void VCCueList::notifyFunctionStarting(quint32 fid, qreal intensity)
{
    Q_UNUSED(intensity);

    if (mode() == Doc::Design)
        return;

    if (fid == m_chaserID)
        return;

    stopChaser();
}

void VCCueList::slotFunctionRemoved(quint32 fid)
{
    if (fid == m_chaserID)
    {
        setChaser(Function::invalidId());
        resetIntensityOverrideAttribute();
    }
}

void VCCueList::slotFunctionChanged(quint32 fid)
{
    if (fid == m_chaserID)
    {
        if (!m_updateTimer->isActive())
            m_updateTimer->start(UPDATE_TIMEOUT);
        return;
    }

    // Check if fid is a Scene that is a step in the chaser
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    foreach (ChaserStep step, ch->steps())
    {
        if (step.fid == fid)
        {
            if (!m_updateTimer->isActive())
                m_updateTimer->start(UPDATE_TIMEOUT);
            return;
        }
    }
}

void VCCueList::slotFunctionNameChanged(quint32 fid)
{
    if (fid == m_chaserID)
        m_updateTimer->start(UPDATE_TIMEOUT);
    else
    {
        // fid might be an ID of a ChaserStep of m_chaser
        Chaser *ch = chaser();
        if (ch == NULL)
            return;
        foreach (ChaserStep step, ch->steps())
        {
            if (step.fid == fid)
            {
                m_updateTimer->start(UPDATE_TIMEOUT);
                return;
            }
        }
    }
}

void VCCueList::slotUpdateStepList()
{
    updateStepList();
}

void VCCueList::slotPlayback()
{
    if (mode() != Doc::Operate)
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    if (ch->isRunning())
    {
        if (playbackLayout() == PlayPauseStop)
        {
            if (ch->isPaused())
            {
                m_playbackButton->setStyleSheet(QString("QToolButton{ background: %1; }")
                                                .arg(m_stopButton->palette().window().color().name()));
                m_playbackButton->setIcon(QIcon(":/player_pause.png"));
            }
            else
            {
                m_playbackButton->setStyleSheet("QToolButton{ background: #5B81FF; }");
                m_playbackButton->setIcon(QIcon(":/player_play.png"));
            }

            // check if the item selection has been changed during pause
            int currentTreeIndex = m_tree->indexOfTopLevelItem(m_tree->currentItem());
            if (currentTreeIndex != ch->currentStepIndex())
                playCueAtIndex(currentTreeIndex);

            ch->setPause(!ch->isPaused());
        }
        else if (playbackLayout() == PlayStopPause)
        {
            stopChaser();
            m_stopButton->setStyleSheet(QString("QToolButton{ background: %1; }")
                                            .arg(m_playbackButton->palette().window().color().name()));
        }
    }
    else
    {
        if (m_tree->currentItem() != NULL)
            startChaser(getCurrentIndex());
        else
            startChaser();
    }

    emit playbackButtonClicked();
}

void VCCueList::slotStop()
{
    if (mode() != Doc::Operate)
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    if (ch->isRunning())
    {
        if (playbackLayout() == PlayPauseStop)
        {
            stopChaser();
            m_playbackButton->setStyleSheet(QString("QToolButton{ background: %1; }")
                                            .arg(m_stopButton->palette().window().color().name()));
            m_progress->setFormat("");
            m_progress->setValue(0);

            emit progressStateChanged();
        }
        else if (playbackLayout() == PlayStopPause)
        {
            if (ch->isPaused())
            {
                m_stopButton->setStyleSheet(QString("QToolButton{ background: %1; }")
                                                .arg(m_playbackButton->palette().window().color().name()));
                m_stopButton->setIcon(QIcon(":/player_pause.png"));
            }
            else
            {
                m_stopButton->setStyleSheet("QToolButton{ background: #5B81FF; }");
            }
            ch->setPause(!ch->isPaused());
        }
    }
    else
    {
        m_primaryIndex = 0;
        m_tree->setCurrentItem(m_tree->topLevelItem(getFirstIndex()));
    }

    emit stopButtonClicked();
}

void VCCueList::slotNextCue()
{
    if (mode() != Doc::Operate)
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // In Crossfade mode with secondary control enabled, Next/Prev change the crossfade target
    if (sideFaderMode() == Crossfade && m_nextPrevControlsSecondary)
    {
        int stepsCount = m_tree->topLevelItemCount();
        if (stepsCount <= 1)
            return;

        int newSecondaryIndex = m_secondaryIndex + 1;
        if (newSecondaryIndex >= stepsCount)
            newSecondaryIndex = 0;
        // Skip primary index
        if (newSecondaryIndex == m_primaryIndex)
        {
            newSecondaryIndex++;
            if (newSecondaryIndex >= stepsCount)
                newSecondaryIndex = 0;
        }
        setSecondaryIndex(newSecondaryIndex);
        return;
    }

    if (ch->isRunning())
    {
        if (ch->isPaused())
        {
            m_tree->setCurrentItem(m_tree->topLevelItem(getNextIndex()));
        }
        else
        {
            ChaserAction action;
            action.m_action = ChaserNextStep;
            action.m_masterIntensity = intensity();
            action.m_stepIntensity = getPrimaryIntensity();
            action.m_fadeMode = getFadeMode();
            ch->setAction(action);
        }
    }
    else
    {
        switch (m_nextPrevBehavior)
        {
            case DefaultRunFirst:
                startChaser(getFirstIndex());
            break;
            case RunNext:
                startChaser(getNextIndex());
            break;
            case Select:
                m_tree->setCurrentItem(m_tree->topLevelItem(getNextIndex()));
            break;
            case Nothing:
            break;
            default:
                Q_ASSERT(false);
        }
    }
}

void VCCueList::slotPreviousCue()
{
    if (mode() != Doc::Operate)
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // In Crossfade mode with secondary control enabled, Next/Prev change the crossfade target
    if (sideFaderMode() == Crossfade && m_nextPrevControlsSecondary)
    {
        int stepsCount = m_tree->topLevelItemCount();
        if (stepsCount <= 1)
            return;

        int newSecondaryIndex = m_secondaryIndex - 1;
        if (newSecondaryIndex < 0)
            newSecondaryIndex = stepsCount - 1;
        // Skip primary index
        if (newSecondaryIndex == m_primaryIndex)
        {
            newSecondaryIndex--;
            if (newSecondaryIndex < 0)
                newSecondaryIndex = stepsCount - 1;
        }
        setSecondaryIndex(newSecondaryIndex);
        return;
    }

    if (ch->isRunning())
    {
        if (ch->isPaused())
        {
            m_tree->setCurrentItem(m_tree->topLevelItem(getPrevIndex()));
        }
        else
        {
            ChaserAction action;
            action.m_action = ChaserPreviousStep;
            action.m_masterIntensity = intensity();
            action.m_stepIntensity = getPrimaryIntensity();
            action.m_fadeMode = getFadeMode();
            ch->setAction(action);
        }
    }
    else
    {
        switch (m_nextPrevBehavior)
        {
            case DefaultRunFirst:
                startChaser(getLastIndex());
            break;
            case RunNext:
                startChaser(getPrevIndex());
            break;
            case Select:
                m_tree->setCurrentItem(m_tree->topLevelItem(getPrevIndex()));
            break;
            case Nothing:
            break;
            default:
                Q_ASSERT(false);
        }
    }
}

void VCCueList::slotCurrentStepChanged(int stepNumber)
{
    // Chaser is being edited, channels count may change.
    // Wait for the CueList to update its steps.
    if (m_updateTimer->isActive())
        return;

    if (stepNumber < 0 || stepNumber >= m_tree->topLevelItemCount())
        return;
    QTreeWidgetItem *item = m_tree->topLevelItem(stepNumber);
    if (item == NULL)
        return;
    m_tree->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    m_tree->setCurrentItem(item);
    m_primaryIndex = stepNumber;
    if (sideFaderMode() == Steps)
    {
        m_bottomStepLabel->setStyleSheet(cfLabelBlueStyle);
        m_bottomStepLabel->setText(QString("#%1").arg(m_primaryIndex + 1));

        float stepVal;
        int stepsCount = m_tree->topLevelItemCount();
        if (stepsCount < 256) 
        {
            stepVal = 256.0 / (float)stepsCount; //divide up the full 0..255 range
            stepVal = qFloor((stepVal * 100000.0) + 0.5) / 100000.0; //round to 5 decimals to fix corner cases
        }
        else 
        {
            stepVal = 1.0;
        }
        
        // value->step# truncates down in slotSideFaderValueChanged; so use ceiling for step#->value
        float slValue = stepVal * (float)stepNumber;
        if (slValue > 255)
            slValue = 255.0;

        int upperBound = 255 - qCeil(slValue);
        int lowerBound = qFloor(256.0 - slValue - stepVal);
        // if the Step slider is already in range, then do not set its value
        // this means a user interaction is going on, either with the mouse or external controller
        if (m_sideFader->value() < lowerBound || m_sideFader->value() >= upperBound)
        {
            m_sideFader->blockSignals(true);
            m_sideFader->setValue(upperBound);
            m_topPercentageLabel->setText(QString("%1").arg(qCeil(slValue)));
            m_sideFader->blockSignals(false);

            //qDebug() << "Slider value:" << m_sideFader->value() << "->" << 255-qCeil(slValue) 
            //    << "(disp:" << slValue << ")" << "Step range:" << upperBound << lowerBound 
            //    << "(stepSize:" << stepVal << ")" 
            //    << "(raw lower:" << (256.0 - slValue - stepVal) << ")";
        }
    }
    else
    {
        setFaderInfo(m_primaryIndex);
    }
    
    // Update step index output value (will be written by writeDMX in next cycle)
    if (m_stepIndexOutputEnabled && m_stepIndexOutputFixture != Function::invalidId())
    {
        m_currentStepIndexValue = stepNumber;
    }
    
    emit stepChanged(m_primaryIndex);
    emit sideFaderValueChanged();

    // Update overwrite and delete button states
    updateOverwriteButtonState();
    updateDeleteButtonState();
}

void VCCueList::slotItemActivated(QTreeWidgetItem *item)
{
    if (mode() != Doc::Operate)
        return;

    int clickedIndex = m_tree->indexOfTopLevelItem(item);

    // In Crossfade mode with secondary control enabled, activation changes the crossfade target
    if (sideFaderMode() == Crossfade && m_nextPrevControlsSecondary)
    {
        // Allow setting secondary to primary (secondary = primary is valid)
        setSecondaryIndex(clickedIndex);
        return;
    }

    playCueAtIndex(clickedIndex);
    
    // Update overwrite and delete button states
    updateOverwriteButtonState();
    updateDeleteButtonState();
}

void VCCueList::slotItemClicked(QTreeWidgetItem *item)
{
    if (mode() != Doc::Operate)
        return;

    // In Crossfade mode with secondary control enabled, clicks change the crossfade target
    if (sideFaderMode() == Crossfade && m_nextPrevControlsSecondary)
    {
        int clickedIndex = m_tree->indexOfTopLevelItem(item);
        // Allow setting secondary to primary (secondary = primary is valid)
        setSecondaryIndex(clickedIndex);
    }
    else if (sideFaderMode() == Steps)
    {
        int clickedIndex = m_tree->indexOfTopLevelItem(item);
        playCueAtIndex(clickedIndex);
    }

    // Update overwrite and delete button states
    updateOverwriteButtonState();
    updateDeleteButtonState();
}

void VCCueList::slotItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_listIsUpdating)
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    int idx = m_tree->indexOfTopLevelItem(item);
    if (idx < 0 || idx >= ch->steps().count())
        return;

    if (column == COL_NOTES)
    {
        // Handle notes column edit
        QString itemText = item->text(column);
        ChaserStep step = ch->steps().at(idx);
        step.note = itemText;
        ch->replaceStep(step, idx);
        emit stepNoteChanged(idx, itemText);
    }
    else if (m_showChannelColumns && column > COL_NOTES && 
             column <= COL_NOTES + m_channelColumns.size())
    {
        // Handle channel value column edit
        int channelIdx = column - COL_NOTES - 1;
        bool ok;
        int value = item->text(column).toInt(&ok);
        if (!ok || value < 0 || value > 255)
        {
            // Revert to old value by refreshing
            updateStepList();
            return;
        }
        updateSceneChannelValue(idx, channelIdx, static_cast<uchar>(value));
    }
}

void VCCueList::slotStepNoteChanged(int idx, QString note)
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;
    ChaserStep step = ch->steps().at(idx);
    step.note = note;
    ch->replaceStep(step, idx);
}

void VCCueList::slotHeaderDoubleClicked(int logicalIndex)
{
    int firstChannelCol = COL_NOTES + 1;

    if (logicalIndex < firstChannelCol)
    {
        // Fixed columns (0-5): open column visibility dialog
        QStringList names;
        names << "#" << caption() << tr("Fade In") << tr("Fade Out") << tr("Duration") << tr("Notes");

        QList<QPair<QString, bool>> cols;
        for (int i = 0; i < firstChannelCol; i++)
            cols.append(qMakePair(names[i], m_tree->header()->isSectionHidden(i)));

        ColumnVisibilityDialog dlg(cols, this);
        if (dlg.exec() == QDialog::Accepted)
        {
            QList<bool> states = dlg.hiddenStates();
            m_hiddenFixedColumns.clear();
            for (int i = 0; i < states.size(); i++)
            {
                m_tree->header()->setSectionHidden(i, states[i]);
                if (states[i])
                    m_hiddenFixedColumns.append(i);
            }
            m_doc->setModified();
        }
        return;
    }

    // Dynamic channel columns (after COL_NOTES): open display-only editor
    // (channel address is controlled automatically by the recording mask)
    int channelIdx = logicalIndex - firstChannelCol;
    if (channelIdx < 0 || channelIdx >= m_channelColumns.size())
        return;

    // Only allow editing display name/mode — not the channel address
    ChannelColumnEditor editor(m_channelColumns[channelIdx], m_doc, this);
    editor.setAddressReadOnly(true);
    if (editor.exec() == QDialog::Accepted)
    {
        ChannelColumnInfo updated = editor.columnInfo();
        // Preserve address and activeInMask from the current column
        updated.absoluteAddress = m_channelColumns[channelIdx].absoluteAddress;
        updated.fixtureId = m_channelColumns[channelIdx].fixtureId;
        updated.fixtureChannel = m_channelColumns[channelIdx].fixtureChannel;
        updated.activeInMask = m_channelColumns[channelIdx].activeInMask;
        m_channelColumns[channelIdx] = updated;

        if (m_channelColumns[channelIdx].activeInMask)
        {
            ChannelValueDelegate *delegate = new ChannelValueDelegate(this);
            delegate->setColumnInfo(&m_channelColumns[channelIdx]);
            m_tree->setItemDelegateForColumn(logicalIndex, delegate);
        }

        m_tree->header()->setSectionHidden(logicalIndex, m_channelColumns[channelIdx].hidden);

        updateTreeHeader();
        updateStepList();
        m_doc->setModified();
    }
}

void VCCueList::slotFunctionRunning(quint32 fid)
{
    if (fid != m_chaserID)
        return;

    if (playbackLayout() == PlayPauseStop)
        m_playbackButton->setIcon(QIcon(":/player_pause.png"));
    else if (playbackLayout() == PlayStopPause)
        m_playbackButton->setIcon(QIcon(":/player_stop.png"));
    m_timer->start(PROGRESS_INTERVAL);
    emit playbackStatusChanged();
    updateFeedback();
}

void VCCueList::slotFunctionStopped(quint32 fid)
{
    if (fid != m_chaserID)
        return;

    m_playbackButton->setIcon(QIcon(":/player_play.png"));
    m_topStepLabel->setText("");
    m_topStepLabel->setStyleSheet(cfLabelNoStyle);
    m_bottomStepLabel->setText("");
    m_bottomStepLabel->setStyleSheet(cfLabelNoStyle);
    // reset any previously set background
    QTreeWidgetItem *item = m_tree->topLevelItem(m_secondaryIndex);
    if (item != NULL)
        item->setBackground(COL_NUM, m_defCol);

    emit stepChanged(-1);

    // Clear step index output value when stopped
    if (m_stepIndexOutputEnabled)
    {
        m_currentStepIndexValue = -1;
    }

    m_progress->setFormat("");
    m_progress->setValue(0);    

    emit progressStateChanged();
    emit sideFaderValueChanged();
    emit playbackStatusChanged();

    qDebug() << Q_FUNC_INFO << "Cue stopped";
    updateFeedback();

    // Update overwrite and delete button states
    updateOverwriteButtonState();
    updateDeleteButtonState();
}

void VCCueList::slotProgressTimeout()
{
    Chaser *ch = chaser();
    if (ch == NULL || !ch->isRunning())
        return;

    ChaserRunnerStep step(ch->currentRunningStep());
    if (step.m_function != NULL)
    {
        int status = m_progress->property("status").toInt();
        int newstatus;
        if (step.m_fadeIn == Function::defaultSpeed())
            newstatus = 1;
        else if (step.m_elapsed > (quint32)step.m_fadeIn)
            newstatus = 1;
        else
            newstatus = 0;

        if (newstatus != status)
        {
            if (newstatus == 0)
                m_progress->setStyleSheet(progressFadeStyle);
            else
                m_progress->setStyleSheet(progressHoldStyle);
            m_progress->setProperty("status", newstatus);
        }
        if (step.m_duration == Function::infiniteSpeed())
        {
            if (newstatus == 0 && step.m_fadeIn != Function::defaultSpeed())
            {
                double progress = ((double)step.m_elapsed / (double)step.m_fadeIn) * (double)m_progress->width();
                m_progress->setFormat(QString("-%1").arg(Function::speedToString(step.m_fadeIn - step.m_elapsed)));
                m_progress->setValue(progress);

                emit progressStateChanged();
            }
            else
            {
                m_progress->setValue(m_progress->maximum());
                m_progress->setFormat("");

                emit progressStateChanged();
            }
            return;
        }
        else
        {
            double progress = ((double)step.m_elapsed / (double)step.m_duration) * (double)m_progress->width();
            m_progress->setFormat(QString("-%1").arg(Function::speedToString(step.m_duration - step.m_elapsed)));
            m_progress->setValue(progress);

            emit progressStateChanged();
        }
    }
    else
    {
        m_progress->setValue(0);
    }
}

QString VCCueList::progressText()
{
    return m_progress->text();
}

double VCCueList::progressPercent()
{
    return ((double)m_progress->value() * 100) / (double)m_progress->width();
}

void VCCueList::startChaser(int startIndex)
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    adjustFunctionIntensity(ch, intensity());

    // Reset crossfade state so the selected step always starts at full intensity.
    // Without this, m_primaryTop and slider value can be left in a stale state
    // from a previous crossfade cycle, causing getPrimaryIntensity() to return 0.
    if (sideFaderMode() == Crossfade)
    {
        m_primaryTop = true;
        m_sideFader->blockSignals(true);
        m_sideFader->setValue(100);
        m_sideFader->blockSignals(false);
        m_topPercentageLabel->setText("100%");
        m_bottomPercentageLabel->setText("0%");
    }

    ChaserAction action;
    action.m_action = ChaserSetStepIndex;
    action.m_stepIndex = startIndex;
    action.m_masterIntensity = intensity();
    action.m_stepIntensity = getPrimaryIntensity();
    action.m_fadeMode = getFadeMode();
    ch->setAction(action);

    ch->start(m_doc->masterTimer(), functionParent());
    emit functionStarting(m_chaserID);
}

void VCCueList::stopChaser()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    ch->stop(functionParent());
    resetIntensityOverrideAttribute();
}

int VCCueList::getFadeMode()
{
    if (sideFaderMode() != Crossfade)
        return Chaser::FromFunction;

    if (m_sideFader->value() != 0 && m_sideFader->value() != 100)
        return Chaser::BlendedCrossfade;

    return Chaser::Blended;
}

void VCCueList::setNextPrevBehavior(NextPrevBehavior nextPrev)
{
    Q_ASSERT(nextPrev == DefaultRunFirst
            || nextPrev == RunNext
            || nextPrev == Select
            || nextPrev == Nothing);
    m_nextPrevBehavior = nextPrev;
}

VCCueList::NextPrevBehavior VCCueList::nextPrevBehavior() const
{
    return m_nextPrevBehavior;
}

void VCCueList::setPlaybackLayout(VCCueList::PlaybackLayout layout)
{
    if (layout == m_playbackLayout)
        return;

    if (layout == PlayStopPause)
    {
        m_stopButton->setIcon(QIcon(":/player_pause.png"));
        m_playbackButton->setToolTip(tr("Play/Stop Cue list"));
        m_stopButton->setToolTip(tr("Pause Cue list"));
    }
    else if (layout == PlayPauseStop)
    {
        m_stopButton->setIcon(QIcon(":/player_stop.png"));
        m_playbackButton->setToolTip(tr("Play/Pause Cue list"));
        m_stopButton->setToolTip(tr("Stop Cue list"));
    }
    else
    {
        qWarning() << "Playback layout" << layout << "doesn't exist!";
        layout = PlayPauseStop;
    }

    m_playbackLayout = layout;
}

VCCueList::PlaybackLayout VCCueList::playbackLayout() const
{
    return m_playbackLayout;
}

void VCCueList::setAutoStartInOperate(bool enable)
{
    m_autoStartInOperate = enable;
}

bool VCCueList::autoStartInOperate() const
{
    return m_autoStartInOperate;
}

void VCCueList::setAutoStartOffset(int ms)
{
    m_autoStartOffset = qBound(0, ms, 2000);
}

int VCCueList::autoStartOffset() const
{
    return m_autoStartOffset;
}

VCCueList::FaderMode VCCueList::sideFaderMode() const
{
    return m_slidersMode;
}

void VCCueList::setSideFaderMode(VCCueList::FaderMode mode)
{
    m_slidersMode = mode;

    bool show = (mode == None) ? false : true;
    if (m_hideButtons) show = false;
    m_crossfadeButton->setVisible(show);
    m_topPercentageLabel->setVisible(show);
    m_topStepLabel->setVisible(mode == Steps ? false : show);
    m_sideFader->setVisible(show);
    m_bottomPercentageLabel->setVisible(mode == Steps ? false : show);
    m_bottomStepLabel->setVisible(show);
    m_sideFader->setMaximum(mode == Steps ? 255 : 100);
    m_sideFader->setValue(mode == Steps ? 255 : 100);
}

VCCueList::FaderMode VCCueList::stringToFaderMode(QString modeStr)
{
    if (modeStr == "Crossfade")
        return Crossfade;
    else if (modeStr == "Steps")
        return Steps;

    return None;
}

QString VCCueList::faderModeToString(VCCueList::FaderMode mode)
{
    if (mode == Crossfade)
        return "Crossfade";
    else if (mode == Steps)
        return "Steps";

    return "None";
}

void VCCueList::setNextPrevControlsSecondary(bool enable)
{
    m_nextPrevControlsSecondary = enable;
}

bool VCCueList::nextPrevControlsSecondary() const
{
    return m_nextPrevControlsSecondary;
}

/*****************************************************************************
 * Crossfade
 *****************************************************************************/
void VCCueList::setFaderInfo(int index)
{
    Chaser *ch = chaser();
    if (ch == NULL || !ch->isRunning())
        return;

    // Reset any previously set background
    QTreeWidgetItem *item = m_tree->topLevelItem(m_secondaryIndex);
    if (item != NULL)
        item->setBackground(COL_NUM, m_defCol);

    int tmpIndex;
    if (m_nextPrevControlsSecondary)
    {
        // In manual mode, secondary starts equal to primary (no auto-advance)
        tmpIndex = index;
    }
    else
    {
        // In automatic mode, secondary is the next step
        tmpIndex = ch->computeNextStep(index);
    }

    m_topStepLabel->setText(QString("#%1").arg(m_primaryTop ? index + 1 : tmpIndex + 1));
    m_topStepLabel->setStyleSheet(m_primaryTop ? cfLabelBlueStyle : cfLabelOrangeStyle);

    m_bottomStepLabel->setText(QString("#%1").arg(m_primaryTop ? tmpIndex + 1 : index + 1));
    m_bottomStepLabel->setStyleSheet(m_primaryTop ? cfLabelOrangeStyle : cfLabelBlueStyle);

    // Only highlight secondary if different from primary
    if (tmpIndex != index)
    {
        item = m_tree->topLevelItem(tmpIndex);
        if (item != NULL)
            item->setBackground(COL_NUM, QColor("#FF8000"));
    }
    m_secondaryIndex = tmpIndex;

    emit sideFaderValueChanged();
}

void VCCueList::setSecondaryIndex(int index)
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    int stepsCount = m_tree->topLevelItemCount();
    if (stepsCount == 0)
        return;

    // Wrap index around if needed
    if (index < 0)
        index = stepsCount - 1;
    else if (index >= stepsCount)
        index = 0;

    // Reset any previously set background
    QTreeWidgetItem *item = m_tree->topLevelItem(m_secondaryIndex);
    if (item != NULL)
        item->setBackground(COL_NUM, m_defCol);

    // Set new secondary index
    m_secondaryIndex = index;

    // Update labels based on which is on top
    m_topStepLabel->setText(QString("#%1").arg(m_primaryTop ? m_primaryIndex + 1 : m_secondaryIndex + 1));
    m_topStepLabel->setStyleSheet(m_primaryTop ? cfLabelBlueStyle : cfLabelOrangeStyle);

    m_bottomStepLabel->setText(QString("#%1").arg(m_primaryTop ? m_secondaryIndex + 1 : m_primaryIndex + 1));
    m_bottomStepLabel->setStyleSheet(m_primaryTop ? cfLabelOrangeStyle : cfLabelBlueStyle);

    // Only highlight secondary if different from primary
    if (m_secondaryIndex != m_primaryIndex)
    {
        item = m_tree->topLevelItem(m_secondaryIndex);
        if (item != NULL)
            item->setBackground(COL_NUM, QColor("#FF8000"));
    }

    emit sideFaderValueChanged();
}

void VCCueList::slotShowCrossfadePanel(bool enable)
{
    bool show = enable && !m_hideButtons;
    m_topPercentageLabel->setVisible(show);
    m_topStepLabel->setVisible(show);
    m_sideFader->setVisible(show);
    m_bottomStepLabel->setVisible(show);
    m_bottomPercentageLabel->setVisible(show);

    emit sideFaderButtonToggled();
}

QString VCCueList::topPercentageValue()
{
    return m_topPercentageLabel->text();
}

QString VCCueList::bottomPercentageValue()
{
    return m_bottomPercentageLabel->text();
}

QString VCCueList::topStepValue()
{
    return m_topStepLabel->text();
}

QString VCCueList::bottomStepValue()
{
    return m_bottomStepLabel->text();
}

int VCCueList::sideFaderValue()
{
    return m_sideFader->value();
}

bool VCCueList::primaryTop()
{
    return m_primaryTop;
}

void VCCueList::slotSideFaderButtonChecked(bool enable)
{
    m_crossfadeButton->setChecked(enable);
    emit sideFaderButtonChecked();
}

bool VCCueList::isSideFaderVisible()
{
    return m_sideFader->isVisible();
}

bool VCCueList::sideFaderButtonIsChecked()
{
    return m_crossfadeButton->isChecked();
}

void VCCueList::slotSetSideFaderValue(int value)
{
    m_sideFader->setValue(value);
}

void VCCueList::slotSideFaderValueChanged(int value)
{
    if (sideFaderMode() == Steps)
    {
        value = 255 - value;
        m_topPercentageLabel->setText(QString("%1").arg(value));

        emit sideFaderValueChanged();

        Chaser *ch = chaser();
        if (ch == NULL || ch->stopped())
            return;

        int newStep = value; // by default we assume the Chaser has more than 256 steps
        if (ch->stepsCount() < 256)
        {
            float stepSize = 256.0 / (float)ch->stepsCount();  //divide up the full 0..255 range
            stepSize = qFloor((stepSize * 100000.0) + 0.5) / 100000.0; //round to 5 decimals to fix corner cases
            if (value >= 256.0 - stepSize)
                newStep = ch->stepsCount() - 1;
            else
                newStep = qFloor((float)value / stepSize);
            //qDebug() << "value:" << value << " new step:" << newStep << " stepSize:" << stepSize;
        }

        if (newStep == ch->currentStepIndex())
            return;

        ChaserAction action;
        action.m_action = ChaserSetStepIndex;
        action.m_stepIndex = newStep;
        action.m_masterIntensity = intensity();
        action.m_stepIntensity = getPrimaryIntensity();
        action.m_fadeMode = getFadeMode();
        ch->setAction(action);
    }
    else
    {
        m_topPercentageLabel->setText(QString("%1%").arg(value));
        m_bottomPercentageLabel->setText(QString("%1%").arg(100 - value));

        emit sideFaderValueChanged();

        Chaser *ch = chaser();
        if (!(ch == NULL || ch->stopped()))
        {
            if (m_secondaryIndex == m_primaryIndex)
            {
                // "Virtual" crossfade - secondary equals primary
                // But we still need to flip m_primaryTop when slider reaches the end for bidirectional operation
                int primaryValue = m_primaryTop ? value : 100 - value;
                if (primaryValue == 0)
                {
                    // Flip direction for next crossfade
                    m_primaryTop = !m_primaryTop;
                    
                    // Update labels to reflect the flip
                    m_topStepLabel->setText(QString("#%1").arg(m_primaryTop ? m_primaryIndex + 1 : m_secondaryIndex + 1));
                    m_topStepLabel->setStyleSheet(m_primaryTop ? cfLabelBlueStyle : cfLabelOrangeStyle);
                    m_bottomStepLabel->setText(QString("#%1").arg(m_primaryTop ? m_secondaryIndex + 1 : m_primaryIndex + 1));
                    m_bottomStepLabel->setStyleSheet(m_primaryTop ? cfLabelOrangeStyle : cfLabelBlueStyle);
                }
                // Force the step's scene to re-create its faders at the end of the
                // Universe fader list, so LTP channels win over other functions (e.g. buttons).
                // This mimics the effect of stop+play but without interrupting playback.
                ch->reapplyStepValues(m_primaryIndex);
                updateFeedback();
                return;
            }

            ch->adjustStepIntensity(qreal(value) / 100.0, m_primaryTop ? m_primaryIndex : m_secondaryIndex,
                                    Chaser::FadeControlMode(getFadeMode()));
            ch->adjustStepIntensity(qreal(100 - value) / 100.0, m_primaryTop ? m_secondaryIndex : m_primaryIndex,
                                    Chaser::FadeControlMode(getFadeMode()));
            stopStepIfNeeded(ch);
        }
    }

    updateFeedback();
}

void VCCueList::stopStepIfNeeded(Chaser *ch)
{
    if (ch->runningStepsNumber() != 2)
        return;

    int primaryValue = m_primaryTop ? m_sideFader->value() : 100 - m_sideFader->value();
    int secondaryValue = m_primaryTop ? 100 - m_sideFader->value() : m_sideFader->value();

    ChaserAction action;
    action.m_action = ChaserStopStep;

    if (primaryValue == 0)
    {
        // Crossfade completed: primary reached 0%
        // Stop the old primary step
        action.m_stepIndex = m_primaryIndex;
        ch->setAction(action);
        
        // Flip the direction for bidirectional operation
        m_primaryTop = !m_primaryTop;
        
        // CRITICAL: Swap indices so the now-active step becomes the new primary
        // and the stopped step becomes the new secondary (ready for next crossfade)
        int tmp = m_primaryIndex;
        m_primaryIndex = m_secondaryIndex;
        m_secondaryIndex = tmp;
        
        // Update labels to reflect the swap
        m_topStepLabel->setText(QString("#%1").arg(m_primaryTop ? m_primaryIndex + 1 : m_secondaryIndex + 1));
        m_topStepLabel->setStyleSheet(m_primaryTop ? cfLabelBlueStyle : cfLabelOrangeStyle);
        m_bottomStepLabel->setText(QString("#%1").arg(m_primaryTop ? m_secondaryIndex + 1 : m_primaryIndex + 1));
        m_bottomStepLabel->setStyleSheet(m_primaryTop ? cfLabelOrangeStyle : cfLabelBlueStyle);
        
        // Update tree highlight: clear old, set new secondary
        QTreeWidgetItem *item = m_tree->topLevelItem(m_primaryIndex);
        if (item != NULL)
            item->setBackground(COL_NUM, m_defCol);  // Clear old secondary (now primary)
        if (m_secondaryIndex != m_primaryIndex)
        {
            item = m_tree->topLevelItem(m_secondaryIndex);
            if (item != NULL)
                item->setBackground(COL_NUM, QColor("#FF8000"));  // Highlight new secondary
        }
    }
    else if (secondaryValue == 0)
    {
        // User dragged slider back to start without completing crossfade
        // Just stop the secondary step
        action.m_stepIndex = m_secondaryIndex;
        ch->setAction(action);
    }
}

/*****************************************************************************
 * Key Sequences
 *****************************************************************************/

void VCCueList::setNextKeySequence(const QKeySequence& keySequence)
{
    m_nextKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::nextKeySequence() const
{
    return m_nextKeySequence;
}

void VCCueList::setPreviousKeySequence(const QKeySequence& keySequence)
{
    m_previousKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::previousKeySequence() const
{
    return m_previousKeySequence;
}

void VCCueList::setPlaybackKeySequence(const QKeySequence& keySequence)
{
    m_playbackKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::playbackKeySequence() const
{
    return m_playbackKeySequence;
}

void VCCueList::setStopKeySequence(const QKeySequence &keySequence)
{
    m_stopKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::stopKeySequence() const
{
    return m_stopKeySequence;
}

void VCCueList::setRecordKeySequence(const QKeySequence& keySequence)
{
    m_recordKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::recordKeySequence() const
{
    return m_recordKeySequence;
}

void VCCueList::setOverwriteKeySequence(const QKeySequence& keySequence)
{
    m_overwriteKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::overwriteKeySequence() const
{
    return m_overwriteKeySequence;
}

void VCCueList::setDeleteKeySequence(const QKeySequence& keySequence)
{
    m_deleteKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::deleteKeySequence() const
{
    return m_deleteKeySequence;
}

void VCCueList::setRenameKeySequence(const QKeySequence& keySequence)
{
    m_renameKeySequence = QKeySequence(keySequence);
}

QKeySequence VCCueList::renameKeySequence() const
{
    return m_renameKeySequence;
}

void VCCueList::slotKeyPressed(const QKeySequence& keySequence)
{
    if (acceptsInput() == false)
        return;

    if (m_nextKeySequence == keySequence)
        slotNextCue();
    else if (m_previousKeySequence == keySequence)
        slotPreviousCue();
    else if (m_playbackKeySequence == keySequence)
        slotPlayback();
    else if (m_stopKeySequence == keySequence)
        slotStop();
    else if (m_recordKeySequence == keySequence)
        slotRecordButtonClicked();
    else if (m_overwriteKeySequence == keySequence)
        slotOverwriteButtonClicked();
    else if (m_deleteKeySequence == keySequence)
        slotDeleteButtonClicked();
    else if (m_renameKeySequence == keySequence)
        slotRenameButtonClicked();
}

void VCCueList::updateFeedback()
{
    int fbv = int(SCALE(float(m_sideFader->value()), 
                        float(m_sideFader->minimum()),
                        float(m_sideFader->maximum()), 
                        float(0), float(UCHAR_MAX)));
    sendFeedback(fbv, sideFaderInputSourceId);

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    sendFeedback(ch->isRunning() ? UCHAR_MAX : 0, playbackInputSourceId);
}

/*****************************************************************************
 * External Input
 *****************************************************************************/

void VCCueList::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    /* Don't let input data through in design mode or if disabled */
    if (acceptsInput() == false)
        return;

    quint32 pagedCh = (page() << 16) | channel;

    if (checkInputSource(universe, pagedCh, value, sender(), nextInputSourceId))
    {
        // Use hysteresis for values, in case the cue list is being controlled
        // by a slider. The value has to go to zero before the next non-zero
        // value is accepted as input. And the non-zero values have to visit
        // above $HYSTERESIS before a zero is accepted again.
        if (m_nextLatestValue == 0 && value > 0)
        {
            slotNextCue();
            m_nextLatestValue = value;
        }
        else if (m_nextLatestValue > HYSTERESIS && value == 0)
        {
            m_nextLatestValue = 0;
        }

        if (value > HYSTERESIS)
            m_nextLatestValue = value;
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), previousInputSourceId))
    {
        // Use hysteresis for values, in case the cue list is being controlled
        // by a slider. The value has to go to zero before the next non-zero
        // value is accepted as input. And the non-zero values have to visit
        // above $HYSTERESIS before a zero is accepted again.
        if (m_previousLatestValue == 0 && value > 0)
        {
            slotPreviousCue();
            m_previousLatestValue = value;
        }
        else if (m_previousLatestValue > HYSTERESIS && value == 0)
        {
            m_previousLatestValue = 0;
        }

        if (value > HYSTERESIS)
            m_previousLatestValue = value;
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), playbackInputSourceId))
    {
        // Use hysteresis for values, in case the cue list is being controlled
        // by a slider. The value has to go to zero before the next non-zero
        // value is accepted as input. And the non-zero values have to visit
        // above $HYSTERESIS before a zero is accepted again.
        if (m_playbackLatestValue == 0 && value > 0)
        {
            slotPlayback();
            m_playbackLatestValue = value;
        }
        else if (m_playbackLatestValue > HYSTERESIS && value == 0)
        {
            m_playbackLatestValue = 0;
        }

        if (value > HYSTERESIS)
            m_playbackLatestValue = value;
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), stopInputSourceId))
    {
        // Use hysteresis for values, in case the cue list is being controlled
        // by a slider. The value has to go to zero before the next non-zero
        // value is accepted as input. And the non-zero values have to visit
        // above $HYSTERESIS before a zero is accepted again.
        if (m_stopLatestValue == 0 && value > 0)
        {
            slotStop();
            m_stopLatestValue = value;
        }
        else if (m_stopLatestValue > HYSTERESIS && value == 0)
        {
            m_stopLatestValue = 0;
        }

        if (value > HYSTERESIS)
            m_stopLatestValue = value;
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), recordInputSourceId))
    {
        // Button-style hysteresis: trigger on rising edge (0 -> non-zero),
        // reset on falling edge (non-zero -> 0). Any non-zero value is accepted
        // so that controllers sending low values (1-3) also work reliably.
        if (m_recordLatestValue == 0 && value > 0)
        {
            slotRecordButtonClicked();
            // slotRecordButtonClicked() opens a modal dialog whose event loop
            // consumes the button release (value=0). Return immediately so
            // m_recordLatestValue stays at 0 (reset inside the slot) and the
            // next press is accepted without requiring an extra cycle.
            return;
        }
        else if (m_recordLatestValue > 0 && value == 0)
        {
            m_recordLatestValue = 0;
        }
        else if (value > 0)
        {
            m_recordLatestValue = value;
        }
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), overwriteInputSourceId))
    {
        if (m_overwriteLatestValue == 0 && value > 0)
        {
            slotOverwriteButtonClicked();
            // Modal dialog consumes release event; reset handled inside slot
            return;
        }
        else if (m_overwriteLatestValue > 0 && value == 0)
        {
            m_overwriteLatestValue = 0;
        }
        else if (value > 0)
        {
            m_overwriteLatestValue = value;
        }
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), deleteInputSourceId))
    {
        if (m_deleteLatestValue == 0 && value > 0)
        {
            slotDeleteButtonClicked();
            // Modal dialog consumes release event; reset handled inside slot
            return;
        }
        else if (m_deleteLatestValue > 0 && value == 0)
        {
            m_deleteLatestValue = 0;
        }
        else if (value > 0)
        {
            m_deleteLatestValue = value;
        }
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), renameInputSourceId))
    {
        if (m_renameLatestValue == 0 && value > 0)
        {
            slotRenameButtonClicked();
            // Modal dialog consumes release event; reset handled inside slot
            return;
        }
        else if (m_renameLatestValue > 0 && value == 0)
        {
            m_renameLatestValue = 0;
        }
        else if (value > 0)
        {
            m_renameLatestValue = value;
        }
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), sideFaderInputSourceId))
    {
        if (sideFaderMode() == None)
            return;

        float val = SCALE((float) value, (float) 0, (float) UCHAR_MAX,
                          (float) m_sideFader->minimum(),
                          (float) m_sideFader->maximum());
        m_sideFader->setValue(val);
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), secondarySelectInputSourceId))
    {
        // Secondary select slider: 0 = nothing, 1 = first position, 2 = second, etc.
        // Works in both Crossfade (sets secondary/target cue) and Steps (jumps directly to cue).
        if (sideFaderMode() == None)
            return;

        if (value == 0)
            return; // 0 does nothing

        int stepsCount = m_tree->topLevelItemCount();
        if (stepsCount == 0)
            return;

        // value 1 = index 0, value 2 = index 1, etc.
        int targetIndex = value - 1;

        // Clamp to last available position
        if (targetIndex >= stepsCount)
            targetIndex = stepsCount - 1;

        if (sideFaderMode() == Steps)
        {
            // In Steps mode: jump directly to the selected cue index
            Chaser *ch = chaser();
            if (ch == NULL || ch->stopped())
                return;
            ChaserAction action;
            action.m_action = ChaserSetStepIndex;
            action.m_stepIndex = targetIndex;
            action.m_masterIntensity = intensity();
            action.m_stepIntensity = getPrimaryIntensity();
            action.m_fadeMode = getFadeMode();
            ch->setAction(action);
        }
        else if (sideFaderMode() == Crossfade && m_nextPrevControlsSecondary)
        {
            // In Crossfade mode: set the secondary (target) cue
            setSecondaryIndex(targetIndex);
        }
    }
}

/*****************************************************************************
 * VCWidget-inherited methods
 *****************************************************************************/

void VCCueList::adjustIntensity(qreal val)
{
    Chaser *ch = chaser();
    if (ch != NULL)
    {
        adjustFunctionIntensity(ch, val);
/*
        // Refresh intensity of current steps
        if (!ch->stopped() && sideFaderMode() == Crossfade && m_sideFader->value() != 100)
        {
                ch->adjustStepIntensity((qreal)m_sideFader->value() / 100, m_primaryTop ? m_primaryIndex : m_secondaryIndex);
                ch->adjustStepIntensity((qreal)(100 - m_sideFader->value()) / 100, m_primaryTop ? m_secondaryIndex : m_primaryIndex);
        }
*/
    }

    VCWidget::adjustIntensity(val);
}

void VCCueList::setCaption(const QString& text)
{
    VCWidget::setCaption(text);
    updateTreeHeader();
}

void VCCueList::setFont(const QFont& font)
{
    VCWidget::setFont(font);

    QFontMetrics m_fm = QFontMetrics(font);
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
    int w = m_fm.width("100%");
#else
    int w = m_fm.horizontalAdvance("100%");
#endif
    m_topPercentageLabel->setFixedWidth(w);
    m_bottomPercentageLabel->setFixedWidth(w);
}

void VCCueList::slotModeChanged(Doc::Mode mode)
{
    bool enable = false;
    if (mode == Doc::Operate)
    {
        m_progress->setStyleSheet(progressFadeStyle);
        m_progress->setRange(0, m_progress->width());
        enable = true;
        // send the initial feedback for the current step slider
        updateFeedback();
        
        // Register DMXSource for step index output if enabled
        if (m_stepIndexOutputEnabled && m_stepIndexOutputFixture != Function::invalidId())
        {
            m_doc->masterTimer()->registerDMXSource(this);
        }
        
        // Auto start cue list if enabled and chaser is attached
        if (m_autoStartInOperate && m_chaserID != Function::invalidId())
        {
            Chaser *ch = chaser();
            if (ch != NULL && !ch->isRunning())
            {
                if (m_autoStartOffset > 0)
                    QTimer::singleShot(m_autoStartOffset, this, SLOT(slotPlayback()));
                else
                    slotPlayback();
            }
        }
    }
    else
    {
        m_topStepLabel->setStyleSheet(cfLabelNoStyle);
        m_topStepLabel->setText("");
        m_bottomStepLabel->setStyleSheet(cfLabelNoStyle);
        m_bottomStepLabel->setText("");
        m_progress->setStyleSheet(progressDisabledStyle);
        // reset any previously set background
        QTreeWidgetItem *item = m_tree->topLevelItem(m_secondaryIndex);
        if (item != NULL)
            item->setBackground(COL_NUM, m_defCol);
        
        // Unregister DMXSource for step index output
        if (m_stepIndexOutputEnabled)
        {
            m_doc->masterTimer()->unregisterDMXSource(this);
            m_fadersMap.clear();
            m_currentStepIndexValue = -1;
        }
    }

    enableWidgetUI(enable);

    /* Always start from the beginning */
    m_tree->setCurrentItem(NULL);

    VCWidget::slotModeChanged(mode);

    emit sideFaderValueChanged();
}

void VCCueList::editProperties()
{
    VCCueListProperties prop(this, m_doc);
    if (prop.exec() == QDialog::Accepted)
        m_doc->setModified();
}

void VCCueList::playCueAtIndex(int idx)
{
    if (mode() != Doc::Operate)
        return;

    m_primaryIndex = idx;
    
    // Update step index output value
    if (m_stepIndexOutputEnabled && m_stepIndexOutputFixture != Function::invalidId())
    {
        m_currentStepIndexValue = idx;
    }

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    if (ch->isRunning())
    {
        ChaserAction action;
        action.m_action = ChaserSetStepIndex;
        action.m_stepIndex = m_primaryIndex;
        action.m_masterIntensity = intensity();
        action.m_stepIntensity = getPrimaryIntensity();
        action.m_fadeMode = getFadeMode();
        ch->setAction(action);
    }
    else
    {
        startChaser(m_primaryIndex);
    }

    if (sideFaderMode() == Crossfade)
        setFaderInfo(m_primaryIndex);
}

FunctionParent VCCueList::functionParent() const
{
    return FunctionParent(FunctionParent::ManualVCWidget, id());
}

/*****************************************************************************
 * Recording
 *****************************************************************************/

void VCCueList::updateOverwriteButtonState()
{
    if (m_overwriteButton == NULL)
        return;

    if (m_doc->mode() != Doc::Operate)
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    if (m_chaserID == Function::invalidId())
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    Function *func = m_doc->function(m_chaserID);
    if (func == NULL || func->type() == Function::SequenceType)
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    Chaser *ch = chaser();
    if (ch == NULL)
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    QTreeWidgetItem *selectedItem = m_tree->currentItem();
    if (selectedItem == NULL)
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    int stepIndex = m_tree->indexOfTopLevelItem(selectedItem);
    if (stepIndex < 0 || stepIndex >= ch->steps().count())
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    ChaserStep step = ch->steps().at(stepIndex);
    Function *stepFunc = m_doc->function(step.fid);
    if (stepFunc == NULL || stepFunc->type() != Function::SceneType)
    {
        m_overwriteButton->setEnabled(false);
        return;
    }

    m_overwriteButton->setEnabled(true);
}

void VCCueList::updateDeleteButtonState()
{
    if (m_deleteButton == NULL)
        return;

    if (m_doc->mode() != Doc::Operate)
    {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        return;
    }

    if (m_chaserID == Function::invalidId())
    {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        return;
    }

    Function *func = m_doc->function(m_chaserID);
    if (func == NULL || func->type() == Function::SequenceType)
    {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        return;
    }

    Chaser *ch = chaser();
    if (ch == NULL)
    {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        return;
    }

    QTreeWidgetItem *selectedItem = m_tree->currentItem();
    if (selectedItem == NULL)
    {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        return;
    }

    int stepIndex = m_tree->indexOfTopLevelItem(selectedItem);
    if (stepIndex < 0 || stepIndex >= ch->steps().count())
    {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        return;
    }

    m_deleteButton->setEnabled(true);
    m_renameButton->setEnabled(true);
}

/*****************************************************************************
 * Recording
 *****************************************************************************/

void VCCueList::slotRecordButtonClicked()
{
    if (m_doc->mode() != Doc::Operate)
        return;

    if (m_chaserID == Function::invalidId())
        return;

    // Don't allow recording for Sequence
    Function *func = m_doc->function(m_chaserID);
    if (func != NULL && func->type() == Function::SequenceType)
        return;

    // Prevent re-entry: recordLiveCue() opens a modal dialog which
    // runs its own event loop. External input events arriving during
    // the dialog could trigger this slot again, opening stacked dialogs.
    if (m_isRecordDialogOpen)
        return;

    m_isRecordDialogOpen = true;
    recordLiveCue();
    m_isRecordDialogOpen = false;

    // Force reset the hysteresis state so the next external input press
    // is immediately accepted. Without this, the modal dialog may have
    // consumed the release event (value=0), leaving m_recordLatestValue
    // stuck at a non-zero value and requiring an extra press to reset.
    m_recordLatestValue = 0;
}

void VCCueList::slotOverwriteButtonClicked()
{
    if (m_doc->mode() != Doc::Operate)
        return;

    if (m_chaserID == Function::invalidId())
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // Check if item is selected
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return; // No scene selected

    // Get step index and scene info for confirmation dialog
    int stepIndex = m_tree->indexOfTopLevelItem(item);
    if (stepIndex < 0 || stepIndex >= ch->steps().count())
        return;

    ChaserStep step = ch->steps().at(stepIndex);
    Function *func = m_doc->function(step.fid);
    if (func == NULL || func->type() != Function::SceneType)
        return;

    // Show confirmation dialog
    QString sceneName = func->name();
    int ret = QMessageBox::question(this,
                                    tr("Overwrite Cue"),
                                    tr("Are you sure you want to overwrite scene \"%1\"?\n\nThis will replace all values in the scene with current DMX values.").arg(sceneName),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::Yes);

    if (ret == QMessageBox::Yes)
    {
        overwriteSelectedCue();
    }

    // Modal dialog consumed button release; reset so next press works
    m_overwriteLatestValue = 0;
}

void VCCueList::slotDeleteButtonClicked()
{
    if (m_doc->mode() != Doc::Operate)
        return;

    if (m_chaserID == Function::invalidId())
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // Check if item is selected
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return; // No cue selected

    // Get step index and function info for confirmation dialog
    int stepIndex = m_tree->indexOfTopLevelItem(item);
    if (stepIndex < 0 || stepIndex >= ch->steps().count())
        return;

    ChaserStep step = ch->steps().at(stepIndex);
    Function *func = m_doc->function(step.fid);
    QString funcName = func ? func->name() : tr("Unknown");

    // Show confirmation dialog
    int ret = QMessageBox::question(this,
                                    tr("Delete Cue"),
                                    tr("Are you sure you want to delete cue \"%1\"?\n\nIf the associated scene is not used elsewhere, it will also be deleted from the document.").arg(funcName),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        deleteSelectedCue();
    }

    // Modal dialog consumed button release; reset so next press works
    m_deleteLatestValue = 0;
}

void VCCueList::slotRenameButtonClicked()
{
    if (m_doc->mode() != Doc::Operate)
        return;

    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return;

    int idx = m_tree->indexOfTopLevelItem(item);
    if (idx < 0 || idx >= ch->steps().count())
        return;

    Function *func = m_doc->function(ch->steps().at(idx).fid);
    if (func == NULL)
        return;

    bool ok;
    QString newName = QInputDialog::getText(this,
                                            tr("Rename Cue"),
                                            tr("Enter new cue name:"),
                                            QLineEdit::Normal,
                                            func->name(),
                                            &ok);
    if (ok && !newName.isEmpty())
        func->setName(newName);

    // Modal dialog consumed button release; reset so next press works
    m_renameLatestValue = 0;
}

void VCCueList::slotPendingFunctionStopped(quint32 id)
{
    if (!m_pendingDeleteFunctionIds.contains(id))
        return;

    m_pendingDeleteFunctionIds.remove(id);
    // By the time this slot runs (Qt::QueuedConnection from the engine thread),
    // MasterTimer has already processed postRun() and removed the pointer from
    // m_functionList, so deleting the Function object is now safe.
    m_doc->deleteFunction(id);
}

void VCCueList::slotTreeContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (item == NULL)
        return;

    QMenu menu(m_tree);

    QAction *copyAction = menu.addAction(tr("Copy step channel values"));
    copyAction->setEnabled(m_channelColumns.size() > 0);
    connect(copyAction, &QAction::triggered, this, &VCCueList::slotCopyStepChannelValues);

    QAction *pasteAction = menu.addAction(tr("Paste step channel values"));
    pasteAction->setEnabled(m_doc->clipboard()->hasStepChannelValues() &&
                            m_channelColumns.size() > 0);
    connect(pasteAction, &QAction::triggered, this, &VCCueList::slotPasteStepChannelValues);

    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

void VCCueList::slotCopyStepChannelValues()
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return;

    int stepIdx = m_tree->indexOfTopLevelItem(item);
    Chaser *ch = chaser();
    if (ch == NULL || stepIdx < 0 || stepIdx >= ch->steps().count())
        return;

    Scene *scene = qobject_cast<Scene*>(m_doc->function(ch->steps().at(stepIdx).fid));
    if (scene == NULL)
        return;

    StepChannelData data;
    for (int colIdx = 0; colIdx < m_channelColumns.size(); colIdx++)
    {
        const ChannelColumnInfo &col = m_channelColumns.at(colIdx);
        uchar val = 0;
        if (col.fixtureId != UINT_MAX)
            val = scene->value(col.fixtureId, col.fixtureChannel);
        data.columnValues.append(val);
    }
    data.isValid = true;

    m_doc->clipboard()->copyStepChannelValues(data);
}

void VCCueList::slotPasteStepChannelValues()
{
    if (!m_doc->clipboard()->hasStepChannelValues())
        return;

    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return;

    int stepIdx = m_tree->indexOfTopLevelItem(item);
    Chaser *ch = chaser();
    if (ch == NULL || stepIdx < 0 || stepIdx >= ch->steps().count())
        return;

    Scene *scene = qobject_cast<Scene*>(m_doc->function(ch->steps().at(stepIdx).fid));
    if (scene == NULL)
        return;

    StepChannelData data = m_doc->clipboard()->stepChannelValues();
    int colCount = qMin(data.columnValues.size(), m_channelColumns.size());

    for (int colIdx = 0; colIdx < colCount; colIdx++)
    {
        const ChannelColumnInfo &dstCol = m_channelColumns.at(colIdx);
        if (!dstCol.activeInMask || dstCol.fixtureId == UINT_MAX)
            continue;
        scene->setValue(SceneValue(dstCol.fixtureId, dstCol.fixtureChannel,
                                   data.columnValues.at(colIdx)));
    }

    updateStepList();
    m_doc->setModified();
}

void VCCueList::deleteSelectedCue()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // Get currently selected item
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return;

    // Get index of selected item
    int stepIndex = m_tree->indexOfTopLevelItem(item);
    if (stepIndex < 0 || stepIndex >= ch->steps().count())
        return;

    // Detect whether we are deleting the currently active/running step
    bool chaserWasRunning = ch->isRunning();
    bool deletingActiveStep = chaserWasRunning &&
                              (ch->currentStepIndex() == stepIndex ||
                               m_primaryIndex == stepIndex);
    int safeIndex = -1;
    if (deletingActiveStep)
    {
        int stepsAfterDelete = ch->steps().count() - 1;
        if (stepsAfterDelete > 0)
            safeIndex = qMax(0, stepIndex - 1);
        // safeIndex == -1: list will be empty after delete → stop chaser
    }

    // Get ChaserStep to retrieve function ID before removing
    ChaserStep step = ch->steps().at(stepIndex);
    quint32 functionId = step.fid;

    // Remove step from chaser
    if (!ch->removeStep(stepIndex))
        return;

    // Check if the function is used elsewhere in the document
    Function *func = m_doc->function(functionId);
    if (func != NULL)
    {
        bool usedElsewhere = false;

        // Check all chasers (including this one after removal)
        foreach (Function *f, m_doc->functionsByType(Function::ChaserType))
        {
            Chaser *otherChaser = qobject_cast<Chaser*>(f);
            if (otherChaser == NULL)
                continue;

            foreach (ChaserStep s, otherChaser->steps())
            {
                if (s.fid == functionId)
                {
                    usedElsewhere = true;
                    break;
                }
            }
            if (usedElsewhere)
                break;
        }

        // Check all sequences
        if (!usedElsewhere)
        {
            foreach (Function *f, m_doc->functionsByType(Function::SequenceType))
            {
                Chaser *seq = qobject_cast<Chaser*>(f);
                if (seq == NULL)
                    continue;

                foreach (ChaserStep s, seq->steps())
                {
                    if (s.fid == functionId)
                    {
                        usedElsewhere = true;
                        break;
                    }
                }
                if (usedElsewhere)
                    break;
            }
        }

        // If not used elsewhere, delete the function — defer if it is still running
        // to avoid a use-after-free crash in MasterTimer::timerTickFunctions (Thread 11).
        // Doc::deleteFunction() destroys the object immediately; if MasterTimer still
        // holds a pointer to it in m_functionList, calling write() on the freed object
        // causes an EXC_BAD_ACCESS / SIGSEGV. Calling stop() marks m_stop=true so the
        // next timer tick calls postRun() and removes the pointer, then emits
        // functionStopped — at that point it is safe to delete.
        if (!usedElsewhere)
        {
            Function *delFunc = m_doc->function(functionId);
            if (delFunc != NULL && delFunc->isRunning())
            {
                delFunc->stop(FunctionParent::master());
                m_pendingDeleteFunctionIds.insert(functionId);
                // Actual deletion happens in slotPendingFunctionStopped
            }
            else
            {
                m_doc->deleteFunction(functionId);
            }
        }
    }

    // Rebuild tree first so playCueAtIndex works with correct indices
    updateStepList();

    // If we deleted the active step, navigate to a safe position
    if (deletingActiveStep)
    {
        if (safeIndex >= 0)
        {
            m_primaryIndex = safeIndex;
            playCueAtIndex(safeIndex);
        }
        else
        {
            // No steps left — stop the chaser
            stopChaser();
            m_primaryIndex = 0;
        }
    }
    else if (ch->steps().count() > 0)
    {
        // Clamp m_primaryIndex in case a step before current was removed
        m_primaryIndex = qBound(0, m_primaryIndex, ch->steps().count() - 1);
    }

    // Mark document as modified
    m_doc->setModified();

    // Update button states
    updateOverwriteButtonState();
    updateDeleteButtonState();
}

void VCCueList::recordLiveCue()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // Don't allow recording for Sequence
    if (ch->type() == Function::SequenceType)
        return;

    // Get current DMX values from universes
    QList<Universe*> ua = m_doc->inputOutputMap()->claimUniverses();
    QByteArray preGMValues(ua.size() * UNIVERSE_SIZE, 0);

    for (int i = 0; i < ua.count(); ++i)
    {
        if (ua.at(i) == NULL)
            continue;
        const int offset = i * UNIVERSE_SIZE;
        preGMValues.replace(offset, UNIVERSE_SIZE, ua.at(i)->preGMValues());
        if (ua.at(i)->passthrough())
        {
            for (int j = 0; j < UNIVERSE_SIZE; ++j)
            {
                const int ofs = offset + j;
                preGMValues[ofs] =
                    static_cast<char>(ua.at(i)->applyPassthrough(j, static_cast<uchar>(preGMValues[ofs])));
            }
        }
    }

    m_doc->inputOutputMap()->releaseUniverses(false);

    // Create new scene
    Scene *newScene = new Scene(m_doc);
    
    // Set path to /bank/[chaser name]
    QString chaserName = ch->name();
    if (chaserName.isEmpty())
        chaserName = tr("Unnamed");
    QString scenePath = QString("bank/%1").arg(chaserName);
    newScene->setPath(scenePath);

    // Get channel mask
    QByteArray recordMask = m_recordChannelsMask;
    if (m_recordAllChannels || recordMask.isEmpty())
    {
        // If all channels or mask not set, create mask for all channels
        int totalChannels = ua.size() * UNIVERSE_SIZE;
        if (totalChannels == 0)
            totalChannels = 4 * UNIVERSE_SIZE; // Default to 4 universes
        recordMask = QByteArray(totalChannels, 1);
    }

    // Iterate through all fixtures and channels
    foreach (Fixture *fxi, m_doc->fixtures())
    {
        if (fxi == NULL)
            continue;

        quint32 baseAddress = fxi->universeAddress();
        quint32 fxID = fxi->id();

        for (quint32 channel = 0; channel < fxi->channels(); channel++)
        {
            quint32 absAddress = baseAddress + channel;

            if (absAddress >= (quint32)recordMask.length())
                continue;

            // Check if channel should be recorded
            bool shouldRecord = false;
            if (m_recordAllChannels)
            {
                shouldRecord = true;
            }
            else
            {
                if (absAddress < (quint32)recordMask.length())
                    shouldRecord = (recordMask[absAddress] == 1);
            }

            if (shouldRecord)
            {
                uchar value = 0;
                if (absAddress < (quint32)preGMValues.length())
                    value = preGMValues.at(absAddress);

                // Skip zero values if option is enabled
                if (m_recordNonZeroOnly && value == 0)
                    continue;

                SceneValue sv = SceneValue(fxID, channel, value);
                newScene->setValue(sv);
            }
        }
    }

    // Show dialog to enter scene name
    QString prefix = m_recordCuePrefix.isEmpty() ? "cue" : m_recordCuePrefix;
    // Sanitize prefix - remove invalid characters for file paths
    prefix = prefix.replace("/", "_").replace("\\", "_").replace(":", "_");
    QString defaultName = QString("%1_%2").arg(prefix).arg(ch->steps().count() + 1);
    bool ok;
    QString sceneName = QInputDialog::getText(this, 
                                             tr("Record Cue"), 
                                             tr("Enter cue name:"), 
                                             QLineEdit::Normal, 
                                             defaultName, 
                                             &ok);
    
    // If user cancelled, delete scene and return
    if (!ok)
    {
        delete newScene;
        return;
    }
    
    // Sanitize and validate scene name
    sceneName = sceneName.trimmed();
    if (sceneName.isEmpty())
    {
        delete newScene;
        return;
    }
    // Remove invalid characters from scene name
    sceneName = sceneName.replace("/", "_").replace("\\", "_").replace(":", "_");
    
    // Set the scene name
    newScene->setName(sceneName);

    // Add scene to document
    if (m_doc->addFunction(newScene))
    {
        // Add scene as step to chaser
        ChaserStep step(newScene->id());
        ch->addStep(step);

        // Update the step list
        updateStepList();

        // Mark document as modified
        m_doc->setModified();
    }
    else
    {
        delete newScene;
    }
}

void VCCueList::overwriteSelectedCue()
{
    Chaser *ch = chaser();
    if (ch == NULL)
        return;

    // Get currently selected item
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == NULL)
        return;

    // Get index of selected item
    int stepIndex = m_tree->indexOfTopLevelItem(item);
    if (stepIndex < 0 || stepIndex >= ch->steps().count())
        return;

    // Get ChaserStep
    ChaserStep step = ch->steps().at(stepIndex);
    
    // Get Scene
    Function *func = m_doc->function(step.fid);
    if (func == NULL || func->type() != Function::SceneType)
        return;

    Scene *scene = qobject_cast<Scene*>(func);
    if (scene == NULL)
        return;

    // Get current DMX values (same logic as recordLiveCue)
    QList<Universe*> ua = m_doc->inputOutputMap()->claimUniverses();
    QByteArray preGMValues(ua.size() * UNIVERSE_SIZE, 0);

    for (int i = 0; i < ua.count(); ++i)
    {
        if (ua.at(i) == NULL)
            continue;
        const int offset = i * UNIVERSE_SIZE;
        preGMValues.replace(offset, UNIVERSE_SIZE, ua.at(i)->preGMValues());
        if (ua.at(i)->passthrough())
        {
            for (int j = 0; j < UNIVERSE_SIZE; ++j)
            {
                const int ofs = offset + j;
                preGMValues[ofs] =
                    static_cast<char>(ua.at(i)->applyPassthrough(j, static_cast<uchar>(preGMValues[ofs])));
            }
        }
    }

    m_doc->inputOutputMap()->releaseUniverses(false);

    // Get channel mask (same logic as recordLiveCue)
    QByteArray recordMask = m_recordChannelsMask;
    if (m_recordAllChannels || recordMask.isEmpty())
    {
        int totalChannels = ua.size() * UNIVERSE_SIZE;
        if (totalChannels == 0)
            totalChannels = 4 * UNIVERSE_SIZE; // Default to 4 universes
        recordMask = QByteArray(totalChannels, 1);
    }

    // Overwrite values in scene
    foreach (Fixture *fxi, m_doc->fixtures())
    {
        if (fxi == NULL)
            continue;

        quint32 baseAddress = fxi->universeAddress();
        quint32 fxID = fxi->id();

        for (quint32 channel = 0; channel < fxi->channels(); channel++)
        {
            quint32 absAddress = baseAddress + channel;

            if (absAddress >= (quint32)recordMask.length())
                continue;

            // Check if channel should be overwritten
            bool shouldOverwrite = false;
            if (m_recordAllChannels)
            {
                shouldOverwrite = true;
            }
            else
            {
                if (absAddress < (quint32)recordMask.length())
                    shouldOverwrite = (recordMask[absAddress] == 1);
            }

            if (shouldOverwrite)
            {
                uchar value = 0;
                if (absAddress < (quint32)preGMValues.length())
                    value = preGMValues.at(absAddress);

                // Skip zero values if option is enabled
                if (m_recordNonZeroOnly && value == 0)
                {
                    // Remove value from scene if it was set
                    scene->unsetValue(fxID, channel);
                }
                else
                {
                    // Overwrite value in scene
                    SceneValue sv = SceneValue(fxID, channel, value);
                    scene->setValue(sv);
                }
            }
        }
    }

    // Mark document as modified
    m_doc->setModified();
}

void VCCueList::setRecordChannelsMask(const QByteArray &mask)
{
    m_recordChannelsMask = mask;
    if (m_showChannelColumns)
        syncChannelColumnsWithMask();
}

QByteArray VCCueList::recordChannelsMask() const
{
    return m_recordChannelsMask;
}

void VCCueList::setRecordAllChannels(bool allChannels)
{
    m_recordAllChannels = allChannels;
}

bool VCCueList::recordAllChannels() const
{
    return m_recordAllChannels;
}

void VCCueList::setRecordNonZeroOnly(bool nonZeroOnly)
{
    m_recordNonZeroOnly = nonZeroOnly;
}

bool VCCueList::recordNonZeroOnly() const
{
    return m_recordNonZeroOnly;
}

void VCCueList::setRecordCuePrefix(const QString &prefix)
{
    m_recordCuePrefix = prefix;
}

QString VCCueList::recordCuePrefix() const
{
    return m_recordCuePrefix;
}

/*****************************************************************************
 * Channel Columns
 *****************************************************************************/

void VCCueList::setShowChannelColumns(bool show)
{
    if (m_showChannelColumns == show)
        return;

    m_showChannelColumns = show;

    if (show)
    {
        buildChannelColumns();
        // Set up delegates for channel value columns with column info
        int colOffset = COL_NOTES + 1;
        for (int i = 0; i < m_channelColumns.size(); i++)
        {
            ChannelValueDelegate *delegate = new ChannelValueDelegate(this);
            delegate->setColumnInfo(&m_channelColumns[i]);
            m_tree->setItemDelegateForColumn(colOffset + i, delegate);
        }
    }
    else
    {
        // Remove delegates for channel columns (set to default)
        int colOffset = COL_NOTES + 1;
        for (int i = 0; i < m_channelColumns.size(); i++)
        {
            m_tree->setItemDelegateForColumn(colOffset + i, nullptr);
        }
        m_channelColumns.clear();
    }

    updateTreeHeader();
    updateStepList();
    applyColumnHiddenState();
}

void VCCueList::applyColumnHiddenState()
{
    if (!m_showChannelColumns)
        return;

    int colOffset = COL_NOTES + 1;
    for (int i = 0; i < m_channelColumns.size(); i++)
    {
        m_tree->header()->setSectionHidden(colOffset + i, m_channelColumns[i].hidden);
    }
}

void VCCueList::setFixedColumnHidden(int col, bool hidden)
{
    if (col < 0 || col > COL_NOTES)
        return;
    m_tree->header()->setSectionHidden(col, hidden);
    if (hidden)
    {
        if (!m_hiddenFixedColumns.contains(col))
            m_hiddenFixedColumns.append(col);
    }
    else
    {
        m_hiddenFixedColumns.removeAll(col);
    }
}

bool VCCueList::isFixedColumnHidden(int col) const
{
    return m_hiddenFixedColumns.contains(col);
}

void VCCueList::applyFixedColumnHiddenState()
{
    for (int col : m_hiddenFixedColumns)
        m_tree->header()->setSectionHidden(col, true);
}

bool VCCueList::showChannelColumns() const
{
    return m_showChannelColumns;
}

QList<ChannelColumnInfo> VCCueList::channelColumns() const
{
    return m_channelColumns;
}

void VCCueList::setChannelColumnName(int index, const QString &name)
{
    if (index < 0 || index >= m_channelColumns.size())
        return;

    m_channelColumns[index].customName = name;
    updateTreeHeader();
}

int VCCueList::channelColumnCount() const
{
    return m_channelColumns.size();
}

void VCCueList::buildChannelColumns()
{
    m_channelColumns.clear();

    if (m_recordChannelsMask.isEmpty())
        return;

    // Iterate through the recording mask and find fixtures for each enabled address
    for (int addr = 0; addr < m_recordChannelsMask.size(); addr++)
    {
        if (m_recordChannelsMask[addr] == 0)
            continue;

        // Find fixture and channel for this absolute address
        ChannelColumnInfo info = findFixtureForAddress(addr);
        if (info.fixtureId != UINT_MAX)
            m_channelColumns.append(info);
    }
}

void VCCueList::syncChannelColumnsWithMask()
{
    if (!m_showChannelColumns)
        return;

    // Step 1: mark all existing columns as inactive
    for (int i = 0; i < m_channelColumns.size(); i++)
        m_channelColumns[i].activeInMask = false;

    // Step 2: iterate over new mask — activate existing or add new columns
    for (int addr = 0; addr < m_recordChannelsMask.size(); addr++)
    {
        if (m_recordChannelsMask[addr] == 0)
            continue;

        quint32 absAddr = static_cast<quint32>(addr);

        // Find existing column with this address
        bool found = false;
        for (int i = 0; i < m_channelColumns.size(); i++)
        {
            if (m_channelColumns[i].absoluteAddress == absAddr)
            {
                m_channelColumns[i].activeInMask = true;
                found = true;
                break;
            }
        }

        if (!found)
        {
            // New channel — add it
            ChannelColumnInfo info = findFixtureForAddress(absAddr);
            if (info.fixtureId != UINT_MAX)
            {
                info.activeInMask = true;
                m_channelColumns.append(info);
            }
        }
    }

    // Step 3: rebuild delegates — active columns get ChannelValueDelegate,
    //         inactive columns get null delegate (non-editable)
    int colOffset = COL_NOTES + 1;
    for (int i = 0; i < m_channelColumns.size(); i++)
    {
        if (m_channelColumns[i].activeInMask)
        {
            ChannelValueDelegate *delegate = new ChannelValueDelegate(this);
            delegate->setColumnInfo(&m_channelColumns[i]);
            m_tree->setItemDelegateForColumn(colOffset + i, delegate);
        }
        else
        {
            m_tree->setItemDelegateForColumn(colOffset + i, nullptr);
        }
    }

    updateTreeHeader();
    updateStepList();
    applyColumnHiddenState();
}

bool VCCueList::hasCueListColumnsForFixture(quint32 fixtureId) const
{
    foreach (const ChannelColumnInfo &col, m_channelColumns)
    {
        if (col.fixtureId == fixtureId)
            return true;
    }
    return false;
}

void VCCueList::remapCueListFixtureChannels(quint32 fixtureId,
                                             quint32 oldAbsBase,
                                             quint32 newAbsBase,
                                             quint32 channels)
{
    if (m_channelColumns.isEmpty())
        return;

    // Snapshot the old mask bits for the moved fixture's channels before clearing them,
    // so that overlapping old/new ranges are handled correctly.
    QVector<quint8> savedBits(channels, 0);
    for (quint32 offset = 0; offset < channels; ++offset)
    {
        quint32 oldIdx = oldAbsBase + offset;
        if (!m_recordChannelsMask.isEmpty() && oldIdx < (quint32)m_recordChannelsMask.size())
            savedBits[offset] = (quint8)m_recordChannelsMask[(int)oldIdx];
    }

    // Extend mask if the new range goes beyond current size
    quint32 requiredSize = newAbsBase + channels;
    if (requiredSize > (quint32)m_recordChannelsMask.size())
        m_recordChannelsMask.resize((int)requiredSize, 0);

    // Clear old mask positions for this fixture
    for (quint32 offset = 0; offset < channels; ++offset)
    {
        quint32 oldIdx = oldAbsBase + offset;
        if (oldIdx < (quint32)m_recordChannelsMask.size())
            m_recordChannelsMask[(int)oldIdx] = 0;
    }

    // Write saved bits to new positions
    for (quint32 offset = 0; offset < channels; ++offset)
    {
        if (savedBits[offset] != 0)
            m_recordChannelsMask[(int)(newAbsBase + offset)] = savedBits[offset];
    }

    // Update absoluteAddress in columns that belong to the moved fixture
    for (int i = 0; i < m_channelColumns.size(); ++i)
    {
        if (m_channelColumns[i].fixtureId == fixtureId)
            m_channelColumns[i].absoluteAddress = newAbsBase + m_channelColumns[i].fixtureChannel;
    }

    syncChannelColumnsWithMask();
}

ChannelColumnInfo VCCueList::findFixtureForAddress(quint32 address) const
{
    ChannelColumnInfo info;
    info.absoluteAddress = address;

    // Calculate universe and channel within universe
    quint32 universe = address / UNIVERSE_SIZE;
    quint32 channelInUniverse = address % UNIVERSE_SIZE;

    // Iterate through all fixtures to find one that contains this address
    foreach (Fixture *fxi, m_doc->fixtures())
    {
        if (fxi == NULL)
            continue;

        if (fxi->universe() != universe)
            continue;

        quint32 fxiAddress = fxi->address();
        quint32 fxiChannels = fxi->channels();

        // Check if the channel falls within this fixture's range
        if (channelInUniverse >= fxiAddress && channelInUniverse < fxiAddress + fxiChannels)
        {
            info.fixtureId = fxi->id();
            info.fixtureChannel = channelInUniverse - fxiAddress;
            return info;
        }
    }

    return info;
}

QString VCCueList::getDefaultChannelName(const ChannelColumnInfo &col) const
{
    if (col.fixtureId == UINT_MAX)
        return QString("Ch %1").arg(col.absoluteAddress + 1);

    Fixture *fxi = m_doc->fixture(col.fixtureId);
    if (fxi == NULL)
        return QString("Ch %1").arg(col.absoluteAddress + 1);

    const QLCChannel *ch = fxi->channel(col.fixtureChannel);
    if (ch != NULL)
    {
        // Use fixture name abbreviation + channel name
        QString fxiName = fxi->name();
        if (fxiName.length() > 8)
            fxiName = fxiName.left(8);
        return QString("%1: %2").arg(fxiName).arg(ch->name());
    }

    return QString("%1 Ch%2").arg(fxi->name().left(8)).arg(col.fixtureChannel + 1);
}

void VCCueList::updateSceneChannelValue(int stepIdx, int channelIdx, uchar value)
{
    Chaser *ch = chaser();
    if (ch == NULL || stepIdx < 0 || stepIdx >= ch->steps().count())
        return;

    if (channelIdx < 0 || channelIdx >= m_channelColumns.size())
        return;

    ChaserStep step = ch->steps().at(stepIdx);
    Scene *scene = qobject_cast<Scene*>(m_doc->function(step.fid));
    if (scene == NULL)
        return;

    const ChannelColumnInfo &col = m_channelColumns.at(channelIdx);
    if (col.fixtureId == UINT_MAX)
        return;

    // Create SceneValue with the new value
    // When called with blind=false (default), Scene::setValue will update
    // the running scene's DMX output immediately in Operate mode
    SceneValue sv(col.fixtureId, col.fixtureChannel, value);
    scene->setValue(sv);

    m_doc->setModified();
}

void VCCueList::updateTreeHeader()
{
    QStringList list;
    list << "#" << caption() << tr("Fade In") << tr("Fade Out") << tr("Duration") << tr("Notes");

    // Add channel column headers if enabled
    if (m_showChannelColumns)
    {
        for (int i = 0; i < m_channelColumns.size(); i++)
        {
            const ChannelColumnInfo &col = m_channelColumns.at(i);
            QString name = col.customName.isEmpty() ? getDefaultChannelName(col) : col.customName;
            if (!col.activeInMask)
                name = QString("[%1]").arg(name);  // bracket indicates inactive
            list << name;
        }
    }

    m_tree->setHeaderLabels(list);
    m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

/*****************************************************************************
 * Hide Buttons
 *****************************************************************************/

void VCCueList::setHideButtons(bool hide)
{
    m_hideButtons = hide;
    m_bottomControlsWidget->setVisible(!hide);
    m_progress->setVisible(!hide);
    if (hide)
    {
        m_topPercentageLabel->setVisible(false);
        m_topStepLabel->setVisible(false);
        m_sideFader->setVisible(false);
        m_bottomPercentageLabel->setVisible(false);
        m_bottomStepLabel->setVisible(false);
    }
    else
    {
        setSideFaderMode(m_slidersMode);
    }
}

bool VCCueList::hideButtons() const
{
    return m_hideButtons;
}

/*****************************************************************************
 * Step Index Output
 *****************************************************************************/

void VCCueList::setStepIndexOutputEnabled(bool enable)
{
    bool wasEnabled = m_stepIndexOutputEnabled;
    m_stepIndexOutputEnabled = enable;
    
    // Register/unregister DMXSource based on enable state
    if (enable && !wasEnabled && m_stepIndexOutputFixture != Function::invalidId())
    {
        if (m_doc->mode() == Doc::Operate)
            m_doc->masterTimer()->registerDMXSource(this);
    }
    else if (!enable && wasEnabled)
    {
        if (m_doc->mode() == Doc::Operate)
            m_doc->masterTimer()->unregisterDMXSource(this);
        // Clear faders
        m_fadersMap.clear();
        m_currentStepIndexValue = -1;
    }
}

bool VCCueList::stepIndexOutputEnabled() const
{
    return m_stepIndexOutputEnabled;
}

void VCCueList::setStepIndexOutputFixture(quint32 fixture)
{
    m_stepIndexOutputFixture = fixture;
    // Clear faders when fixture changes - they will be recreated in writeDMX
    m_fadersMap.clear();
}

quint32 VCCueList::stepIndexOutputFixture() const
{
    return m_stepIndexOutputFixture;
}

void VCCueList::setStepIndexOutputChannel(quint32 channel)
{
    m_stepIndexOutputChannel = channel;
}

quint32 VCCueList::stepIndexOutputChannel() const
{
    return m_stepIndexOutputChannel;
}

void VCCueList::writeDMX(MasterTimer *timer, QList<Universe*> universes)
{
    Q_UNUSED(timer);

    if (!m_stepIndexOutputEnabled || 
        m_stepIndexOutputFixture == Function::invalidId() ||
        m_currentStepIndexValue < 0)
    {
        return;
    }

    Fixture *fxi = m_doc->fixture(m_stepIndexOutputFixture);
    if (fxi == NULL)
        return;

    quint32 universe = fxi->universe();
    if (universe >= (quint32)universes.count())
        return;

    // Value: m_currentStepIndexValue (0-based) -> DMX value (1-based)
    // Step 0 = DMX 1, Step 1 = DMX 2, etc.
    uchar value = static_cast<uchar>((m_currentStepIndexValue + 1) % 256);

    QSharedPointer<GenericFader> fader = m_fadersMap.value(universe, QSharedPointer<GenericFader>());
    if (fader.isNull())
    {
        // Use Override priority so our value takes precedence
        fader = universes[universe]->requestFader(Universe::Override);
        fader->adjustIntensity(intensity());
        m_fadersMap[universe] = fader;
    }

    FadeChannel *fc = fader->getChannelFader(m_doc, universes[universe], m_stepIndexOutputFixture, m_stepIndexOutputChannel);
    if (fc->universe() == Universe::invalid())
    {
        fader->remove(fc);
        return;
    }

    // Add Override flag so our value takes precedence over running functions
    fc->addFlag(FadeChannel::Override);
    
    // For LTP channels, mark for autoremove when done
    const QLCChannel *qlcch = fxi->channel(m_stepIndexOutputChannel);
    if (qlcch != NULL && qlcch->group() != QLCChannel::Intensity)
        fc->addFlag(FadeChannel::AutoRemove);

    fc->setCurrent(value);
    fc->setStart(value);
    fc->setTarget(value);
    fc->setReady(false);
    fc->setElapsed(0);
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

bool VCCueList::loadXML(QXmlStreamReader &root)
{
    QList <quint32> legacyStepList;

    if (root.name() != KXMLQLCVCCueList)
    {
        qWarning() << Q_FUNC_INFO << "CueList node not found";
        return false;
    }

    /* Widget commons */
    loadXMLCommon(root);

    /* Children */
    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            bool visible = false;
            int x = 0, y = 0, w = 0, h = 0;
            loadXMLWindowState(root, &x, &y, &w, &h, &visible);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
        {
            loadXMLAppearance(root);
        }
        else if (root.name() == KXMLQLCVCCueListNext)
        {
            QString str = loadXMLSources(root, nextInputSourceId);
            if (str.isEmpty() == false)
                m_nextKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListPrevious)
        {
            QString str = loadXMLSources(root, previousInputSourceId);
            if (str.isEmpty() == false)
                m_previousKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListPlayback)
        {
            QString str = loadXMLSources(root, playbackInputSourceId);
            if (str.isEmpty() == false)
                m_playbackKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListStop)
        {
            QString str = loadXMLSources(root, stopInputSourceId);
            if (str.isEmpty() == false)
                m_stopKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListRecord)
        {
            QString str = loadXMLSources(root, recordInputSourceId);
            if (str.isEmpty() == false)
                m_recordKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListOverwrite)
        {
            QString str = loadXMLSources(root, overwriteInputSourceId);
            if (str.isEmpty() == false)
                m_overwriteKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListDelete)
        {
            QString str = loadXMLSources(root, deleteInputSourceId);
            if (str.isEmpty() == false)
                m_deleteKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListRename)
        {
            QString str = loadXMLSources(root, renameInputSourceId);
            if (str.isEmpty() == false)
                m_renameKeySequence = stripKeySequence(QKeySequence(str));
        }
        else if (root.name() == KXMLQLCVCCueListSlidersMode)
        {
            setSideFaderMode(stringToFaderMode(root.readElementText()));
        }
        else if (root.name() == KXMLQLCVCCueListNextPrevSecondary)
        {
            QString val = root.readElementText();
            setNextPrevControlsSecondary(val == "1" || val.toLower() == "true");
        }
        else if (root.name() == KXMLQLCVCCueListSecondarySelect)
        {
            loadXMLSources(root, secondarySelectInputSourceId);
        }
        else if (root.name() == KXMLQLCVCCueListCrossfadeLeft)
        {
            loadXMLSources(root, sideFaderInputSourceId);
        }
        else if (root.name() == KXMLQLCVCCueListCrossfadeRight) /* Legacy */
        {
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCVCWidgetKey) /* Legacy */
        {
            setNextKeySequence(QKeySequence(root.readElementText()));
        }
        else if (root.name() == KXMLQLCVCCueListChaser)
        {
            setChaser(root.readElementText().toUInt());
        }
        else if (root.name() == KXMLQLCVCCueListPlaybackLayout)
        {
            PlaybackLayout layout = PlaybackLayout(root.readElementText().toInt());
            if (layout != PlayPauseStop && layout != PlayStopPause)
            {
                qWarning() << Q_FUNC_INFO << "Playback layout" << layout << "does not exist.";
                layout = PlayPauseStop;
            }
            setPlaybackLayout(layout);
        }
        else if (root.name() == KXMLQLCVCCueListNextPrevBehavior)
        {
            NextPrevBehavior nextPrev = NextPrevBehavior(root.readElementText().toInt());
            if (nextPrev != DefaultRunFirst
                    && nextPrev != RunNext
                    && nextPrev != Select
                    && nextPrev != Nothing)
            {
                qWarning() << Q_FUNC_INFO << "Next/Prev behavior" << nextPrev << "does not exist.";
                nextPrev = DefaultRunFirst;
            }
            setNextPrevBehavior(nextPrev);
        }
        else if (root.name() == KXMLQLCVCCueListCrossfade)
        {
            m_crossfadeButton->setChecked(true);
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCVCCueListFunction)
        {
            // Collect legacy file format steps into a list
            legacyStepList << root.readElementText().toUInt();
        }
        else if (root.name() == KXMLQLCVCCueListRecordAllChannels)
        {
            setRecordAllChannels(root.readElementText() == "1" || root.readElementText().toLower() == "true");
        }
        else if (root.name() == KXMLQLCVCCueListRecordNonZeroOnly)
        {
            setRecordNonZeroOnly(root.readElementText() == "1" || root.readElementText().toLower() == "true");
        }
        else if (root.name() == KXMLQLCVCCueListRecordMask)
        {
            QString maskStr = root.readElementText();
            QByteArray mask = QByteArray::fromBase64(maskStr.toLatin1());
            setRecordChannelsMask(mask);
        }
        else if (root.name() == KXMLQLCVCCueListRecordPrefix)
        {
            setRecordCuePrefix(root.readElementText());
        }
        else if (root.name() == KXMLQLCVCCueListStepIndexOutput)
        {
            QXmlStreamAttributes attrs = root.attributes();
            bool enabled = attrs.value(KXMLQLCVCCueListStepIndexOutputEnabled).toString() == "true";
            quint32 fixture = attrs.value(KXMLQLCVCCueListStepIndexOutputFixture).toString().toUInt();
            quint32 channel = attrs.value(KXMLQLCVCCueListStepIndexOutputChannel).toString().toUInt();
            
            setStepIndexOutputEnabled(enabled);
            setStepIndexOutputFixture(fixture);
            setStepIndexOutputChannel(channel);
            
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCVCCueListAutoStart)
        {
            setAutoStartInOperate(root.readElementText() == "1");
        }
        else if (root.name() == KXMLQLCVCCueListAutoStartOffset)
        {
            setAutoStartOffset(root.readElementText().toInt());
        }
        else if (root.name() == KXMLQLCVCCueListHideButtons)
        {
            setHideButtons(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLQLCVCCueListFixedColumnsHidden)
        {
            QString s = root.readElementText();
            m_hiddenFixedColumns.clear();
            for (const QString &v : s.split(",", Qt::SkipEmptyParts))
            {
                int col = v.trimmed().toInt();
                if (col >= 0 && col <= COL_NOTES)
                    m_hiddenFixedColumns.append(col);
            }
            applyFixedColumnHiddenState();
        }
        else if (root.name() == KXMLQLCVCCueListChannelColumns)
        {
            QXmlStreamAttributes attrs = root.attributes();
            bool showColumns = attrs.value(KXMLQLCVCCueListShowChannelColumns).toString() == "1";
            
            // Read channel column definitions
            m_channelColumns.clear();
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLQLCVCCueListChannelColumn)
                {
                    QXmlStreamAttributes colAttrs = root.attributes();
                    quint32 address = colAttrs.value(KXMLQLCVCCueListChannelColumnAddress).toString().toUInt();
                    QString customName = colAttrs.value(KXMLQLCVCCueListChannelColumnName).toString();
                    
                    ChannelColumnInfo info = findFixtureForAddress(address);
                    info.customName = customName;
                    
                    // Load display mode
                    if (colAttrs.hasAttribute(KXMLQLCVCCueListChannelColumnDisplayMode))
                    {
                        info.displayMode = static_cast<ChannelDisplayMode>(
                            colAttrs.value(KXMLQLCVCCueListChannelColumnDisplayMode).toString().toInt());
                    }
                    
                    // Load scale settings
                    if (colAttrs.hasAttribute(KXMLQLCVCCueListChannelColumnScaleMin))
                        info.scaleMin = colAttrs.value(KXMLQLCVCCueListChannelColumnScaleMin).toString().toDouble();
                    if (colAttrs.hasAttribute(KXMLQLCVCCueListChannelColumnScaleMax))
                        info.scaleMax = colAttrs.value(KXMLQLCVCCueListChannelColumnScaleMax).toString().toDouble();
                    if (colAttrs.hasAttribute(KXMLQLCVCCueListChannelColumnScaleSuffix))
                        info.scaleSuffix = colAttrs.value(KXMLQLCVCCueListChannelColumnScaleSuffix).toString();
                    
                    // Load hidden state
                    if (colAttrs.hasAttribute(KXMLQLCVCCueListChannelColumnHidden))
                        info.hidden = colAttrs.value(KXMLQLCVCCueListChannelColumnHidden).toString() == "1";
                    
                    // Load dropdown mappings
                    while (root.readNextStartElement())
                    {
                        if (root.name() == KXMLQLCVCCueListChannelColumnMapping)
                        {
                            QXmlStreamAttributes mapAttrs = root.attributes();
                            int mapValue = mapAttrs.value(KXMLQLCVCCueListChannelColumnMappingValue).toString().toInt();
                            QString mapLabel = root.readElementText();
                            info.dropdownMappings[mapValue] = mapLabel;
                        }
                        else
                        {
                            root.skipCurrentElement();
                        }
                    }
                    
                    m_channelColumns.append(info);
                }
                else
                {
                    root.skipCurrentElement();
                }
            }
            
            // Reconcile loaded columns with the recording mask
            if (!m_recordChannelsMask.isEmpty())
            {
                for (int i = 0; i < m_channelColumns.size(); i++)
                {
                    quint32 addr = m_channelColumns[i].absoluteAddress;
                    m_channelColumns[i].activeInMask =
                        (addr < (quint32)m_recordChannelsMask.size()
                         && m_recordChannelsMask[(int)addr] != 0);
                }
            }

            // Set up the show flag and delegates after loading all columns
            m_showChannelColumns = showColumns;
            if (showColumns && !m_channelColumns.isEmpty())
            {
                int colOffset = COL_NOTES + 1;
                for (int i = 0; i < m_channelColumns.size(); i++)
                {
                    if (m_channelColumns[i].activeInMask)
                    {
                        ChannelValueDelegate *delegate = new ChannelValueDelegate(this);
                        delegate->setColumnInfo(&m_channelColumns[i]);
                        m_tree->setItemDelegateForColumn(colOffset + i, delegate);
                    }
                }
            }
            
            updateTreeHeader();
            updateStepList();
            applyColumnHiddenState();
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unknown cuelist tag:" << root.name().toString();
            root.skipCurrentElement();
        }
    }

    if (legacyStepList.isEmpty() == false)
    {
        /* Construct a new chaser from legacy step functions and use that chaser */
        Chaser *chaser = new Chaser(m_doc);
        chaser->setName(caption());

        // Legacy cue lists relied on individual functions' timings and a common hold time
        chaser->setFadeInMode(Chaser::Default);
        chaser->setFadeOutMode(Chaser::Default);
        chaser->setDurationMode(Chaser::Common);

        foreach (quint32 id, legacyStepList)
        {
            Function *function = m_doc->function(id);
            if (function == NULL)
                continue;

            // Legacy cuelists relied on individual functions' fadein/out speed and
            // infinite duration. So don't touch them at all.
            ChaserStep step(id);
            chaser->addStep(step);
        }

        // Add the chaser to Doc and attach it to the cue list
        m_doc->addFunction(chaser);
        setChaser(chaser->id());
    }

    return true;
}

bool VCCueList::saveXML(QXmlStreamWriter *doc)
{
    Q_ASSERT(doc != NULL);

    /* VC CueList entry */
    doc->writeStartElement(KXMLQLCVCCueList);

    saveXMLCommon(doc);

    /* Window state */
    saveXMLWindowState(doc);

    /* Appearance */
    saveXMLAppearance(doc);

    /* Chaser */
    doc->writeTextElement(KXMLQLCVCCueListChaser, QString::number(chaserID()));

    /* Playback layout */
    if (playbackLayout() != PlayPauseStop)
        doc->writeTextElement(KXMLQLCVCCueListPlaybackLayout, QString::number(playbackLayout()));

    /* Next/Prev behavior */
    doc->writeTextElement(KXMLQLCVCCueListNextPrevBehavior, QString::number(nextPrevBehavior()));

    /* Next cue */
    doc->writeStartElement(KXMLQLCVCCueListNext);
    if (m_nextKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_nextKeySequence.toString());
    saveXMLInput(doc, inputSource(nextInputSourceId));
    doc->writeEndElement();

    /* Previous cue */
    doc->writeStartElement(KXMLQLCVCCueListPrevious);
    if (m_previousKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_previousKeySequence.toString());
    saveXMLInput(doc, inputSource(previousInputSourceId));
    doc->writeEndElement();

    /* Cue list playback */
    doc->writeStartElement(KXMLQLCVCCueListPlayback);
    if (m_playbackKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_playbackKeySequence.toString());
    saveXMLInput(doc, inputSource(playbackInputSourceId));
    doc->writeEndElement();

    /* Cue list stop */
    doc->writeStartElement(KXMLQLCVCCueListStop);
    if (m_stopKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_stopKeySequence.toString());
    saveXMLInput(doc, inputSource(stopInputSourceId));
    doc->writeEndElement();

    /* Cue list record */
    doc->writeStartElement(KXMLQLCVCCueListRecord);
    if (m_recordKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_recordKeySequence.toString());
    saveXMLInput(doc, inputSource(recordInputSourceId));
    doc->writeEndElement();

    /* Cue list overwrite */
    doc->writeStartElement(KXMLQLCVCCueListOverwrite);
    if (m_overwriteKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_overwriteKeySequence.toString());
    saveXMLInput(doc, inputSource(overwriteInputSourceId));
    doc->writeEndElement();

    /* Cue list delete */
    doc->writeStartElement(KXMLQLCVCCueListDelete);
    if (m_deleteKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_deleteKeySequence.toString());
    saveXMLInput(doc, inputSource(deleteInputSourceId));
    doc->writeEndElement();

    /* Cue list rename */
    doc->writeStartElement(KXMLQLCVCCueListRename);
    if (m_renameKeySequence.toString().isEmpty() == false)
        doc->writeTextElement(KXMLQLCVCWidgetKey, m_renameKeySequence.toString());
    saveXMLInput(doc, inputSource(renameInputSourceId));
    doc->writeEndElement();

    /* Crossfade cue list */
    if (sideFaderMode() != None)
        doc->writeTextElement(KXMLQLCVCCueListSlidersMode, faderModeToString(sideFaderMode()));

    /* Next/Prev controls secondary */
    if (m_nextPrevControlsSecondary)
        doc->writeTextElement(KXMLQLCVCCueListNextPrevSecondary, QString::number(1));

    QSharedPointer<QLCInputSource> cf1Src = inputSource(sideFaderInputSourceId);
    if (!cf1Src.isNull() && cf1Src->isValid())
    {
        doc->writeStartElement(KXMLQLCVCCueListCrossfadeLeft);
        saveXMLInput(doc, cf1Src);
        doc->writeEndElement();
    }

    /* Secondary select slider */
    QSharedPointer<QLCInputSource> secSelSrc = inputSource(secondarySelectInputSourceId);
    if (!secSelSrc.isNull() && secSelSrc->isValid())
    {
        doc->writeStartElement(KXMLQLCVCCueListSecondarySelect);
        saveXMLInput(doc, secSelSrc);
        doc->writeEndElement();
    }

    /* Recording settings */
    doc->writeTextElement(KXMLQLCVCCueListRecordAllChannels, QString::number(m_recordAllChannels ? 1 : 0));
    if (m_recordNonZeroOnly)
        doc->writeTextElement(KXMLQLCVCCueListRecordNonZeroOnly, QString::number(m_recordNonZeroOnly ? 1 : 0));
    if (!m_recordChannelsMask.isEmpty())
    {
        QString maskBase64 = QString::fromLatin1(m_recordChannelsMask.toBase64());
        doc->writeTextElement(KXMLQLCVCCueListRecordMask, maskBase64);
    }
    if (!m_recordCuePrefix.isEmpty() && m_recordCuePrefix != "cue")
        doc->writeTextElement(KXMLQLCVCCueListRecordPrefix, m_recordCuePrefix);

    /* Step Index Output */
    if (m_stepIndexOutputEnabled && m_stepIndexOutputFixture != Function::invalidId())
    {
        doc->writeStartElement(KXMLQLCVCCueListStepIndexOutput);
        doc->writeAttribute(KXMLQLCVCCueListStepIndexOutputEnabled, 
                           m_stepIndexOutputEnabled ? "true" : "false");
        doc->writeAttribute(KXMLQLCVCCueListStepIndexOutputFixture, 
                           QString::number(m_stepIndexOutputFixture));
        doc->writeAttribute(KXMLQLCVCCueListStepIndexOutputChannel, 
                           QString::number(m_stepIndexOutputChannel));
        doc->writeEndElement();
    }

    /* Auto start in operate mode */
    if (m_autoStartInOperate)
    {
        doc->writeTextElement(KXMLQLCVCCueListAutoStart, QString::number(1));
        if (m_autoStartOffset > 0)
            doc->writeTextElement(KXMLQLCVCCueListAutoStartOffset, QString::number(m_autoStartOffset));
    }

    /* Hide buttons */
    if (m_hideButtons)
        doc->writeTextElement(KXMLQLCVCCueListHideButtons, QString::number(1));

    /* Hidden fixed columns */
    if (!m_hiddenFixedColumns.isEmpty())
    {
        QStringList vals;
        for (int col : m_hiddenFixedColumns)
            vals << QString::number(col);
        doc->writeTextElement(KXMLQLCVCCueListFixedColumnsHidden, vals.join(","));
    }

    /* Channel columns */
    if (m_showChannelColumns || !m_channelColumns.isEmpty())
    {
        doc->writeStartElement(KXMLQLCVCCueListChannelColumns);
        doc->writeAttribute(KXMLQLCVCCueListShowChannelColumns, 
                           m_showChannelColumns ? "1" : "0");
        
        for (int i = 0; i < m_channelColumns.size(); i++)
        {
            const ChannelColumnInfo &col = m_channelColumns.at(i);
            doc->writeStartElement(KXMLQLCVCCueListChannelColumn);
            doc->writeAttribute(KXMLQLCVCCueListChannelColumnAddress, 
                               QString::number(col.absoluteAddress));
            if (!col.customName.isEmpty())
                doc->writeAttribute(KXMLQLCVCCueListChannelColumnName, col.customName);
            
            // Save display mode settings
            doc->writeAttribute(KXMLQLCVCCueListChannelColumnDisplayMode,
                               QString::number(col.displayMode));
            
            // Save scale settings (for Scaled mode)
            if (col.displayMode == DisplayScaled)
            {
                doc->writeAttribute(KXMLQLCVCCueListChannelColumnScaleMin,
                                   QString::number(col.scaleMin, 'f', 2));
                doc->writeAttribute(KXMLQLCVCCueListChannelColumnScaleMax,
                                   QString::number(col.scaleMax, 'f', 2));
                if (!col.scaleSuffix.isEmpty())
                    doc->writeAttribute(KXMLQLCVCCueListChannelColumnScaleSuffix, col.scaleSuffix);
            }
            
            // Save hidden state
            if (col.hidden)
                doc->writeAttribute(KXMLQLCVCCueListChannelColumnHidden, "1");
            
            // Save dropdown mappings (for Dropdown mode)
            if (col.displayMode == DisplayDropdown && !col.dropdownMappings.isEmpty())
            {
                QMapIterator<int, QString> it(col.dropdownMappings);
                while (it.hasNext())
                {
                    it.next();
                    doc->writeStartElement(KXMLQLCVCCueListChannelColumnMapping);
                    doc->writeAttribute(KXMLQLCVCCueListChannelColumnMappingValue,
                                       QString::number(it.key()));
                    doc->writeCharacters(it.value());
                    doc->writeEndElement();
                }
            }
            
            doc->writeEndElement();
        }
        
        doc->writeEndElement();
    }

    /* End the <CueList> tag */
    doc->writeEndElement();

    return true;
}



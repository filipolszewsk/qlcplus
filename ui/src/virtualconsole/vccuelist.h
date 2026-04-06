/*
  Q Light Controller Plus
  vccuelist.h

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

#ifndef VCCUELIST_H
#define VCCUELIST_H

#include <QKeySequence>
#include <QMap>
#include <QWidget>

#include "dmxsource.h"
#include "vcwidget.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QTreeWidgetItem;
class QProgressBar;
class QTreeWidget;
class QToolButton;
class QCheckBox;
class QLabel;

class VCCueListProperties;
class ClickAndGoSlider;
class ChaserRunner;
class MasterTimer;
class GenericFader;
class Chaser;
class Doc;

/** @addtogroup ui_vc_widgets
 * @{
 */

#define KXMLQLCVCCueList                QStringLiteral("CueList")
#define KXMLQLCVCCueListFunction        QStringLiteral("Function") // Legacy
#define KXMLQLCVCCueListChaser          QStringLiteral("Chaser")
#define KXMLQLCVCCueListPlaybackLayout  QStringLiteral("PlaybackLayout")
#define KXMLQLCVCCueListNextPrevBehavior QStringLiteral("NextPrevBehavior")
#define KXMLQLCVCCueListCrossfade       QStringLiteral("Crossfade")
#define KXMLQLCVCCueListBlend           QStringLiteral("Blend")
#define KXMLQLCVCCueListLinked          QStringLiteral("Linked")
#define KXMLQLCVCCueListNext            QStringLiteral("Next")
#define KXMLQLCVCCueListPrevious        QStringLiteral("Previous")
#define KXMLQLCVCCueListPlayback        QStringLiteral("Playback")
#define KXMLQLCVCCueListStop            QStringLiteral("Stop")
#define KXMLQLCVCCueListRecord          QStringLiteral("Record")
#define KXMLQLCVCCueListOverwrite       QStringLiteral("Overwrite")
#define KXMLQLCVCCueListDelete          QStringLiteral("Delete")
#define KXMLQLCVCCueListRename          QStringLiteral("Rename")
#define KXMLQLCVCCueListCrossfadeLeft   QStringLiteral("CrossLeft")
#define KXMLQLCVCCueListCrossfadeRight  QStringLiteral("CrossRight")
#define KXMLQLCVCCueListSlidersMode     QStringLiteral("SlidersMode")
#define KXMLQLCVCCueListRecordAllChannels QStringLiteral("RecordAllChannels")
#define KXMLQLCVCCueListRecordNonZeroOnly QStringLiteral("RecordNonZeroOnly")
#define KXMLQLCVCCueListRecordMask      QStringLiteral("RecordMask")
#define KXMLQLCVCCueListRecordPrefix    QStringLiteral("RecordPrefix")
#define KXMLQLCVCCueListNextPrevSecondary QStringLiteral("NextPrevSecondary")
#define KXMLQLCVCCueListSecondarySelect QStringLiteral("SecondarySelect")
#define KXMLQLCVCCueListStepIndexOutput QStringLiteral("StepIndexOutput")
#define KXMLQLCVCCueListStepIndexOutputEnabled QStringLiteral("Enabled")
#define KXMLQLCVCCueListStepIndexOutputFixture QStringLiteral("Fixture")
#define KXMLQLCVCCueListStepIndexOutputChannel QStringLiteral("Channel")
#define KXMLQLCVCCueListAutoStart       QStringLiteral("AutoStart")
#define KXMLQLCVCCueListChannelColumns  QStringLiteral("ChannelColumns")
#define KXMLQLCVCCueListChannelColumn   QStringLiteral("Column")
#define KXMLQLCVCCueListChannelColumnAddress QStringLiteral("Address")
#define KXMLQLCVCCueListChannelColumnName QStringLiteral("CustomName")
#define KXMLQLCVCCueListShowChannelColumns QStringLiteral("ShowColumns")
#define KXMLQLCVCCueListHideButtons     QStringLiteral("HideButtons")
#define KXMLQLCVCCueListChannelColumnDisplayMode QStringLiteral("DisplayMode")
#define KXMLQLCVCCueListChannelColumnScaleMin QStringLiteral("ScaleMin")
#define KXMLQLCVCCueListChannelColumnScaleMax QStringLiteral("ScaleMax")
#define KXMLQLCVCCueListChannelColumnScaleSuffix QStringLiteral("ScaleSuffix")
#define KXMLQLCVCCueListChannelColumnMapping QStringLiteral("Mapping")
#define KXMLQLCVCCueListChannelColumnMappingValue QStringLiteral("Value")
#define KXMLQLCVCCueListChannelColumnHidden QStringLiteral("Hidden")
#define KXMLQLCVCCueListFixedColumnsHidden QStringLiteral("FixedColumnsHidden")

/**
 * Display mode for channel columns
 */
enum ChannelDisplayMode
{
    DisplayRaw = 0,      ///< Raw DMX value 0-255
    DisplayDropdown,     ///< Dropdown with custom value->label mappings
    DisplayScaled        ///< Scaled display (e.g., 0-360°, 0-100%)
};

/**
 * Structure to hold information about a channel column in the cue list
 */
struct ChannelColumnInfo
{
    quint32 absoluteAddress;  ///< DMX address (universe * 512 + channel)
    quint32 fixtureId;        ///< Fixture ID
    quint32 fixtureChannel;   ///< Channel number relative to fixture
    QString customName;       ///< Optional custom name for the column header

    ChannelDisplayMode displayMode;     ///< How to display values
    QMap<int, QString> dropdownMappings; ///< DMX value -> label for dropdown mode
    double scaleMin;                     ///< Minimum scaled value
    double scaleMax;                     ///< Maximum scaled value
    QString scaleSuffix;                 ///< Suffix for scaled display (e.g., "°", "%")
    bool hidden;                         ///< Whether this column is hidden

    bool activeInMask;  ///< true = channel is currently in the recording mask

    ChannelColumnInfo()
        : absoluteAddress(0)
        , fixtureId(UINT_MAX)
        , fixtureChannel(0)
        , displayMode(DisplayRaw)
        , scaleMin(0.0)
        , scaleMax(255.0)
        , hidden(false)
        , activeInMask(true)
    {}

    ChannelColumnInfo(quint32 addr, quint32 fxiId, quint32 fxiCh, const QString &name = QString())
        : absoluteAddress(addr)
        , fixtureId(fxiId)
        , fixtureChannel(fxiCh)
        , customName(name)
        , displayMode(DisplayRaw)
        , scaleMin(0.0)
        , scaleMax(255.0)
        , hidden(false)
        , activeInMask(true)
    {}
};

/**
 * VCCueList provides a \ref VirtualConsole widget to control cue lists.
 *
 * @see VCWidget
 * @see VirtualConsole
 */
class VCCueList : public VCWidget, public DMXSource
{
    Q_OBJECT
    Q_DISABLE_COPY(VCCueList)

    friend class VCCueListProperties;

public:
    static const quint8 nextInputSourceId;
    static const quint8 previousInputSourceId;
    static const quint8 playbackInputSourceId;
    static const quint8 stopInputSourceId;
    static const quint8 sideFaderInputSourceId;
    static const quint8 recordInputSourceId;
    static const quint8 overwriteInputSourceId;
    static const quint8 deleteInputSourceId;
    static const quint8 renameInputSourceId;
    static const quint8 secondarySelectInputSourceId;

    /*************************************************************************
     * Initialization
     *************************************************************************/
public:
    /** Constructor */
    VCCueList(QWidget *parent, Doc *doc);

    /** Destructor */
    ~VCCueList();

    /** @reimp */
    void enableWidgetUI(bool enable);

    /*************************************************************************
     * Clipboard
     *************************************************************************/
public:
    /** Create a copy of this widget into the given parent */
    VCWidget *createCopy(VCWidget *parent);

protected:
    /** Copy the contents for this widget from another widget */
    bool copyFrom(const VCWidget *widget);

public:
    QList<QPair<PastePropertyGroup, QString>> pasteablePropertyGroups() const override;
    void applyPropertiesFrom(const VCWidget* source, PastePropertyGroups flags) override;

    /*************************************************************************
     * Cue list
     *************************************************************************/
public:
    /** Set the chaser function that is used as cue list steps */
    void setChaser(quint32 fid);

    /** Get the chaser ID that is used as cue list steps */
    quint32 chaserID() const;

    /** Get the chaser function that is used as cue list steps */
    Chaser *chaser();

    /** @reimp */
    QList<quint32> referencedFunctions() const override;

public:
    /** Get the currently selected item index, otherwise 0 */
    int getCurrentIndex();
    /** Get the progress text of the selected item */
    QString progressText();
    double progressPercent();

private:
    /** Get the index of the next item, based on the chaser direction */
    int getNextIndex();

    /** Get the index of the previous item, based on the chaser direction */
    int getPrevIndex();

    /** Get the index of the first item, based on the chaser direction */
    int getFirstIndex();

    /** Get the index of the last item, based on the chaser direction */
    int getLastIndex();

    /** Get the index of the item above the selected item */
    int getNextTreeIndex();

    /** Get the index of the item below the selected item */
    int getPrevTreeIndex();

    /** Get the index of the item on top of the tree */
    int getFirstTreeIndex();

    /** Get the index of the item at the bottom of the tree */
    int getLastTreeIndex();

private:
    /** Get the intensity of the current primary slider */
    qreal getPrimaryIntensity() const;

public:
    /** @reimp */
    virtual void notifyFunctionStarting(quint32 fid, qreal intensity);

private:
    /** Update the list of steps */
    void updateStepList();

    /** timer for updating the step list */
    QTimer *m_updateTimer;

public slots:
    /** Play/stop/resume the cue list from the current selection */
    void slotPlayback();

    /** Stop the cue list */
    void slotStop();

    /** Skip to the next cue */
    void slotNextCue();

    /** Skip to the previous cue */
    void slotPreviousCue();

    /** Called when m_runner skips to another step */
    void slotCurrentStepChanged(int stepNumber);

    /** Update cue step note */
    void slotStepNoteChanged(int idx, QString note);

signals:
    /** progress percent value and text */
    void progressStateChanged();

private slots:
    /** Removes destroyed functions from the list */
    void slotFunctionRemoved(quint32 fid);

    /** Updates the list if chaser got changed */
    void slotFunctionChanged(quint32 fid);

    /** Updates name in the list if function got changed */
    void slotFunctionNameChanged(quint32 fid);

    /** Update the step list at m_updateTimer timeout */
    void slotUpdateStepList();

    /** Slot that is called whenever the current item changes (either by
        pressing the key binding or clicking an item with mouse) */
    void slotItemActivated(QTreeWidgetItem *item);

    /** Slot that is called when an item is clicked */
    void slotItemClicked(QTreeWidgetItem *item);

    /** Slot that is called whenever an item field has been changed.
        Note that only 'Notes" column is considered */
    void slotItemChanged(QTreeWidgetItem*item, int column);

    /** Slot called when a header column is double-clicked (for renaming channel columns) */
    void slotHeaderDoubleClicked(int logicalIndex);

    /** Slot called whenever a function is started */
    void slotFunctionRunning(quint32 fid);

    /** Slot called whenever a function is stopped */
    void slotFunctionStopped(quint32 fid);

    /** Slot called every 200ms to update the step progress bar */
    void slotProgressTimeout();

private:
    /** Start associated chaser */
    void startChaser(int startIndex = -1);

    /** Stop associated */
    void stopChaser();

    int getFadeMode();

public:
    enum NextPrevBehavior
    {
        DefaultRunFirst = 0,
        RunNext,
        Select,
        Nothing
    };

    enum PlaybackLayout
    {
        PlayPauseStop = 0,
        PlayStopPause
    };

    void setNextPrevBehavior(NextPrevBehavior nextPrev);
    NextPrevBehavior nextPrevBehavior() const;

    void setPlaybackLayout(PlaybackLayout layout);
    PlaybackLayout playbackLayout() const;

    /** Set whether the cue list should auto-start when entering Operate mode */
    void setAutoStartInOperate(bool enable);

    /** Get whether the cue list should auto-start when entering Operate mode */
    bool autoStartInOperate() const;
signals:
    void playbackButtonClicked();
    void stopButtonClicked();
    void playbackStatusChanged();

private:
    /** ID of the Chaser this Cue List will be controlling */
    quint32 m_chaserID;

    NextPrevBehavior m_nextPrevBehavior;
    PlaybackLayout m_playbackLayout;
    bool m_autoStartInOperate;
    QTreeWidget *m_tree;
    QToolButton *m_crossfadeButton;
    QToolButton *m_playbackButton;
    QToolButton *m_stopButton;
    QToolButton *m_previousButton;
    QToolButton *m_nextButton;
    QToolButton *m_recordButton;
    QToolButton *m_overwriteButton;
    QToolButton *m_deleteButton;
    QToolButton *m_renameButton;
    QProgressBar *m_progress;
    bool m_listIsUpdating;

    QTimer *m_timer;

    /*************************************************************************
     * Crossfade
     *************************************************************************/
public:
    enum FaderMode
    {
        None = 0,
        Crossfade,
        Steps
    };

    FaderMode sideFaderMode() const;
    void setSideFaderMode(FaderMode mode);

    FaderMode stringToFaderMode(QString modeStr);
    QString faderModeToString(FaderMode mode);

    /** Set whether Next/Prev buttons control secondary selection in Crossfade mode */
    void setNextPrevControlsSecondary(bool enable);

    /** Get whether Next/Prev buttons control secondary selection in Crossfade mode */
    bool nextPrevControlsSecondary() const;

    bool isSideFaderVisible();
    bool sideFaderButtonIsChecked();
    QString topPercentageValue();
    QString bottomPercentageValue();
    QString topStepValue();
    QString bottomStepValue();
    int sideFaderValue();
    bool primaryTop();

signals:
    void sideFaderButtonToggled();
    void sideFaderValueChanged();
    void sideFaderButtonChecked();

public slots:
    void slotSideFaderButtonChecked(bool enable);
    void slotSetSideFaderValue(int value);

protected:
    void setFaderInfo(int index);

    /** Set the secondary (crossfade target) index and update UI */
    void setSecondaryIndex(int index);

protected slots:
    void slotShowCrossfadePanel(bool enable);
    void slotSideFaderValueChanged(int value);

protected:
    void stopStepIfNeeded(Chaser *ch);

private:
    QLabel *m_topPercentageLabel;
    QLabel *m_topStepLabel;
    ClickAndGoSlider *m_sideFader;
    QLabel *m_bottomPercentageLabel;
    QLabel *m_bottomStepLabel;

    QBrush m_defCol;
    int m_primaryIndex, m_secondaryIndex;
    bool m_primaryTop;
    FaderMode m_slidersMode;
    bool m_nextPrevControlsSecondary;

    /*************************************************************************
     * Key sequences
     *************************************************************************/
public:
    /** Set the keyboard key combination for skipping to the next cue */
    void setNextKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for skipping to the next cue */
    QKeySequence nextKeySequence() const;

    /** Set the keyboard key combination for skipping to the previous cue */
    void setPreviousKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for skipping to the previous cue */
    QKeySequence previousKeySequence() const;

    /** Set the keyboard key combination for playing/resuming the cue list */
    void setPlaybackKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for playing/resuming the cue list */
    QKeySequence playbackKeySequence() const;

    /** Set the keyboard key combination for stopping the cue list */
    void setStopKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for stopping the cue list */
    QKeySequence stopKeySequence() const;

    /** Set the keyboard key combination for recording a new cue */
    void setRecordKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for recording a new cue */
    QKeySequence recordKeySequence() const;

    /** Set the keyboard key combination for overwriting selected cue */
    void setOverwriteKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for overwriting selected cue */
    QKeySequence overwriteKeySequence() const;

    /** Set the keyboard key combination for deleting selected cue */
    void setDeleteKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for deleting selected cue */
    QKeySequence deleteKeySequence() const;

    /** Set the keyboard key combination for renaming selected cue */
    void setRenameKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for renaming selected cue */
    QKeySequence renameKeySequence() const;

protected slots:
    void slotKeyPressed(const QKeySequence& keySequence);

private:
    QKeySequence m_nextKeySequence;
    QKeySequence m_previousKeySequence;
    QKeySequence m_playbackKeySequence;
    QKeySequence m_stopKeySequence;
    QKeySequence m_recordKeySequence;
    QKeySequence m_overwriteKeySequence;
    QKeySequence m_deleteKeySequence;
    QKeySequence m_renameKeySequence;

    /*************************************************************************
     * External Input
     *************************************************************************/
public:
    void updateFeedback();

protected slots:
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value);

private:
    quint32 m_nextLatestValue;
    quint32 m_previousLatestValue;
    quint32 m_playbackLatestValue;
    quint32 m_stopLatestValue;
    quint32 m_recordLatestValue;
    quint32 m_overwriteLatestValue;
    quint32 m_deleteLatestValue;
    quint32 m_renameLatestValue;

    /*************************************************************************
     * VCWidget-inherited
     *************************************************************************/
public:
    /** @reimp */
    void adjustIntensity(qreal val);

    /** @reimp */
    void setCaption(const QString& text);

    /** @reimp */
    void setFont(const QFont& font);

    /** @reimp */
    void slotModeChanged(Doc::Mode mode);

    /** @reimp */
    void editProperties();

    /*********************************************************************
     * Web access
     *********************************************************************/
public:
    /** Play a specific cue at the given index */
    void playCueAtIndex(int idx);

signals:
    /** Signal to webaccess */
    void stepChanged(int idx);

    void stepNoteChanged(int idx, QString note);

private:
    FunctionParent functionParent() const;

    /*************************************************************************
     * Recording
     *************************************************************************/
public:
    /** Record current DMX values as a new scene and add to chaser */
    void recordLiveCue();

    /** Overwrite selected cue with current DMX values */
    void overwriteSelectedCue();

    /** Delete selected cue from chaser and optionally delete the scene */
    void deleteSelectedCue();

private:
    /** Update overwrite button enabled state based on current conditions */
    void updateOverwriteButtonState();

    /** Update delete button enabled state based on current conditions */
    void updateDeleteButtonState();

    /** Set the channel mask for recording */
    void setRecordChannelsMask(const QByteArray &mask);

    /** Get the channel mask for recording */
    QByteArray recordChannelsMask() const;

    /** Set recording mode: true = all channels, false = only selected */
    void setRecordAllChannels(bool allChannels);

    /** Get recording mode */
    bool recordAllChannels() const;

    /** Set non-zero values only mode */
    void setRecordNonZeroOnly(bool nonZeroOnly);

    /** Get non-zero values only mode */
    bool recordNonZeroOnly() const;

    /** Set the prefix for recorded cue names */
    void setRecordCuePrefix(const QString &prefix);

    /** Get the prefix for recorded cue names */
    QString recordCuePrefix() const;

private slots:
    /** Slot called when record button is clicked */
    void slotRecordButtonClicked();

    /** Slot called when overwrite button is clicked */
    void slotOverwriteButtonClicked();

    /** Slot called when delete button is clicked */
    void slotDeleteButtonClicked();

    /** Slot called when rename button is clicked */
    void slotRenameButtonClicked();

private:
    /** Channel mask for recording (0 = excluded, 1 = included) */
    QByteArray m_recordChannelsMask;

    /** True if recording all channels, false if only selected */
    bool m_recordAllChannels;

    /** True if recording only non-zero values */
    bool m_recordNonZeroOnly;

    /** Prefix for recorded cue names */
    QString m_recordCuePrefix;

    /** Guard to prevent re-entry while record dialog is open */
    bool m_isRecordDialogOpen;

    /*************************************************************************
     * Channel Columns
     *************************************************************************/
public:
    /** Set whether channel columns should be displayed */
    void setShowChannelColumns(bool show);

    /** Get whether channel columns are displayed */
    bool showChannelColumns() const;

    /** Get the list of channel columns */
    QList<ChannelColumnInfo> channelColumns() const;

    /** Set custom name for a channel column */
    void setChannelColumnName(int index, const QString &name);

    /** Get the number of channel columns */
    int channelColumnCount() const;

    /** Rebuild channel columns based on recording mask */
    void buildChannelColumns();

    /**
     * Synchronize channel columns with the current recording mask:
     * - Columns whose address is no longer in the mask are marked activeInMask=false (grayed out).
     * - Addresses newly present in the mask that have no column yet are added.
     * Updates tree header, step list, and delegates.
     */
    void syncChannelColumnsWithMask();

    /** @reimp — returns true if any column references the given fixture */
    bool hasCueListColumnsForFixture(quint32 fixtureId) const override;

    /** @reimp — shifts absoluteAddress in columns and mask bits to the new address range */
    void remapCueListFixtureChannels(quint32 fixtureId,
                                     quint32 oldAbsBase,
                                     quint32 newAbsBase,
                                     quint32 channels) override;

private:
    /** Find fixture and channel for a given absolute DMX address */
    ChannelColumnInfo findFixtureForAddress(quint32 address) const;

    /** Get default name for a channel column */
    QString getDefaultChannelName(const ChannelColumnInfo &col) const;

    /** Update a scene's channel value */
    void updateSceneChannelValue(int stepIdx, int channelIdx, uchar value);

    /** Update the tree widget header labels */
    void updateTreeHeader();

    /** Apply hidden state to channel columns */
    void applyColumnHiddenState();

    /** Set or clear hidden state for a fixed column (0=NUM … 5=NOTES) */
    void setFixedColumnHidden(int col, bool hidden);

    /** Return true if the fixed column is currently hidden */
    bool isFixedColumnHidden(int col) const;

    /** Apply saved hidden state to all fixed columns */
    void applyFixedColumnHiddenState();

private:
    /** Whether to show channel columns */
    bool m_showChannelColumns;

    /** List of channel columns to display */
    QList<ChannelColumnInfo> m_channelColumns;

    /** Indices of fixed columns that are hidden */
    QList<int> m_hiddenFixedColumns;

    /*************************************************************************
     * Hide Buttons
     *************************************************************************/
public:
    /** Set whether control buttons and faders should be hidden */
    void setHideButtons(bool hide);

    /** Get whether control buttons and faders are hidden */
    bool hideButtons() const;

private:
    bool m_hideButtons;
    QWidget *m_bottomControlsWidget;

    /*************************************************************************
     * Step Index Output
     *************************************************************************/
public:
    /** Set whether step index output is enabled */
    void setStepIndexOutputEnabled(bool enable);

    /** Get whether step index output is enabled */
    bool stepIndexOutputEnabled() const;

    /** Set the fixture for step index output */
    void setStepIndexOutputFixture(quint32 fixture);

    /** Get the fixture for step index output */
    quint32 stepIndexOutputFixture() const;

    /** Set the channel for step index output */
    void setStepIndexOutputChannel(quint32 channel);

    /** Get the channel for step index output */
    quint32 stepIndexOutputChannel() const;

    /*************************************************************************
     * DMXSource
     *************************************************************************/
public:
    /** @reimp */
    void writeDMX(MasterTimer *timer, QList<Universe*> universes);

private:
    /** True if step index output is enabled */
    bool m_stepIndexOutputEnabled;

    /** Fixture ID for step index output (Function::invalidId() = not set) */
    quint32 m_stepIndexOutputFixture;

    /** Channel within fixture for step index output (0-based) */
    quint32 m_stepIndexOutputChannel;

    /** Current step index value to output */
    int m_currentStepIndexValue;

    /** Map used to lookup a GenericFader instance for a Universe ID */
    QMap<quint32, QSharedPointer<GenericFader> > m_fadersMap;

    /*************************************************************************
     * Load & Save
     *************************************************************************/
public:
    /** @reimp */
    bool loadXML(QXmlStreamReader &root);

    /** @reimp */
    bool saveXML(QXmlStreamWriter *doc);
};

/** @} */

#endif

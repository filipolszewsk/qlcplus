/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonwidget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QTimer>
#include <QPoint>
#include <QList>
#include <QStringList>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPaintEvent>
#include <QElapsedTimer>
#include <QPixmap>
#include <QColor>
#include <QHash>
#include <QMap>
#include <QMenu>
#include <QPointer>
#include <QMutex>
#include <QKeySequence>

#include "vcwidget.h"
#include "genericfader.h"
#include "functionparent.h"
#include "scenevalue.h"
#include "dmxsource.h"
#include "presettablev2multibuttoniface.h"

class Doc;
class Function;
class EntrySelectOverlay;

enum class MultiButtonMode
{
    Function,
    Level,
    Widget
};

enum class MultiButtonLayout
{
    Single,
    Spread
};

/** How Widget mode publishes the selector/recall DMX channel. */
enum class MultiButtonWidgetBusPolicy
{
  /** Publish on UI change via sendFeedback; cuelist can own the bus between publishes. */
    SharedBus = 0,
  /** Legacy: hold Universe::Override on the channel every MasterTimer tick. */
    HoldOverride
};

struct SpreadTileInfo
{
    int   index = -1;   // entry index, or -1 for OFF tile
    QRect rect;
};

struct LevelChannelBinding
{
    quint32 fixtureId = 0;
    quint32 channel   = 0;

    bool operator==(const LevelChannelBinding& o) const
    { return fixtureId == o.fixtureId && channel == o.channel; }
};

namespace MBInputId
{
    static constexpr quint8 kSpreadSlotBase = 16;
    static constexpr quint8 kEntryBase      = 64;
    static constexpr int    kMaxSpreadSlots = 32;
    static constexpr int    kMaxEntryInputs = 160;

    inline quint8 spreadSlot(int localSlot) { return quint8(kSpreadSlotBase + localSlot); }
    inline quint8 entryTrigger(int idx)     { return quint8(kEntryBase + idx); }
}

struct LevelPreset
{
    QString       label;
    QString       iconPath;
    QColor        color;           // invalid = use default widget background
    QColor        labelColor;      // invalid = auto contrast on background
    bool          hideName = false;  // true = no text on button (user cleared name)
    bool          flashOnActivate = false;
    bool          flashOverride = false;
    bool          flashForceLtp = false;
    QList<quint8> values;   // parallel to m_levelChannelBindings
    QStringList   valueFormulas;
    QSharedPointer<QLCInputSource> entryInput;
    QKeySequence                 entryKey;
    int                          entryInputValue = -1; // -1 = any active value
};

enum class MultiButtonAutomationMode
{
    Next,
    Random,
    Jump
};

struct MultiButtonAutomationProfile
{
    QString                   name;
    MultiButtonAutomationMode mode       = MultiButtonAutomationMode::Next;
    int                       stepMin    = 1;
    int                       stepMax    = 1;
    int                       multiplier = 1;   // automation trigger fires advance every N-th pulse
    int                       beatOffset = 0;   // phase within multiplier window (0..multiplier-1)
    quint32                   excludeMask = 0;  // bit i excludes entry index i
};

class MultiButtonWidget : public VCWidget, public DMXSource, public PresetTableV2MultiButtonTargetIface
{
    Q_OBJECT
    Q_INTERFACES(PresetTableV2MultiButtonTargetIface)

public:
    static const quint8 triggerInputSourceId      = 0;   // cycle-next
    static const quint8 popupInputSourceId        = 1;   // open popup
    static const quint8 automationInputSourceId   = 2;   // advance automation
    static const quint8 presetChooseInputSourceId = 3;   // DMX value selects automation profile
    static const quint8 entrySelectInputSourceId  = 4;   // scaled entry/preset (knob/fader)
    static const quint8 spreadPageInputSourceId   = 5;   // spread page index (0-based channel value)
    static const quint8 commitInputSourceId       = 6;   // 0 = idle, upper (255) = apply staged
    static const quint8 widgetRecallInputSourceId = 7;   // widget selector/recall channel
    static constexpr int kAllWidgetOutputsIndex = -1;

    explicit MultiButtonWidget(QWidget* parent, Doc* doc);
    ~MultiButtonWidget() override;

    // ---- Mode ------------------------------------------------------------
    MultiButtonMode widgetMode() const { return m_mode; }
    void setWidgetMode(MultiButtonMode mode);

    // ---- Function list management ----------------------------------------
    void           setEntries(const QList<quint32>& ids, const QStringList& labels,
                              const QStringList& iconPaths = QStringList());
    QList<quint32> functionIds()    const { return m_functionIds; }
    QStringList    functionLabels() const { return m_functionLabels; }
    QStringList    iconPaths()      const { return m_iconPaths; }

    // ---- Level mode ------------------------------------------------------
    void setLevelConfig(const QList<LevelChannelBinding>& bindings,
                        const QList<LevelPreset>& presets);
    QList<LevelChannelBinding> levelChannelBindings() const { return m_levelChannelBindings; }
    QList<LevelPreset>         levelPresets()         const { return m_levelPresets; }
    void setWidgetEntryAppearance(const QList<LevelPreset>& appearance);
    QList<LevelPreset> widgetEntryAppearance() const { return m_widgetEntryAppearance; }

    void setCurrentIndex(int idx);    // -1 = none active (calls activate internally)
    int  currentIndex()  const { return m_currentIndex; }

    void setIconForEntry(int idx, const QString& path);

    // ---- Settings --------------------------------------------------------
    int  longPressMs()   const { return m_longPressMs; }
    void setLongPressMs(int ms);

    bool addOffAtEnd()   const { return m_addOffAtEnd; }
    void setAddOffAtEnd(bool v);

    bool monitorChannelValues() const { return m_monitorChannelValues; }
    void setMonitorChannelValues(bool enable);

    bool receiveInputOnInactiveFramePage() const { return m_receiveInputOnInactiveFramePage; }
    void setReceiveInputOnInactiveFramePage(bool enable);

    bool entrySelectAutoCommit() const { return m_entrySelectAutoCommit; }
    void setEntrySelectAutoCommit(bool enable);

    bool logPresetChanges() const { return m_logPresetChanges; }
    void setLogPresetChanges(bool enable);

    bool stageBeforeCommit() const { return m_stageBeforeCommit; }
    void setStageBeforeCommit(bool enable);

    static void alignLevelPresetArrays(LevelPreset& preset, int bindingCount);

    MultiButtonLayout widgetLayout() const { return m_layout; }
    void setWidgetLayout(MultiButtonLayout layout);

    int  spreadColumns() const { return m_spreadColumns; }
    void setSpreadColumns(int columns);
    int  spreadRows() const { return m_spreadRows; }
    void setSpreadRows(int rows);
    int  spreadHMargin() const { return m_spreadHMargin; }
    void setSpreadHMargin(int margin);
    int  spreadVMargin() const { return m_spreadVMargin; }
    void setSpreadVMargin(int margin);
    int  spreadTileWidth() const { return m_spreadTileWidth; }
    void setSpreadTileWidth(int width);
    int  spreadTileHeight() const { return m_spreadTileHeight; }
    void setSpreadTileHeight(int height);
    int  spreadPages() const { return m_spreadPages; }
    void setSpreadPages(int pages);
    int  spreadPageIndex() const { return m_spreadPageIndex; }

    bool automationEnabled() const { return m_automationEnabled; }
    void setAutomationEnabled(bool enable);

    QList<MultiButtonAutomationProfile> automationProfiles() const { return m_automationProfiles; }
    void setAutomationProfiles(const QList<MultiButtonAutomationProfile>& profiles,
                               int activeIndex);
    int  activeAutomationProfile() const { return m_activeAutomationProfile; }
    QString targetListName() const { return m_targetListName; }
    QString targetDisplayName() const;

    QSharedPointer<QLCInputSource> entryInputSource(int idx) const;
    void setEntryInputSource(int idx, QSharedPointer<QLCInputSource> src);
    QKeySequence entryKeySource(int idx) const;
    void setEntryKeySource(int idx, const QKeySequence& key);
    int entryInputValueSource(int idx) const;
    void setEntryInputValueSource(int idx, int value);

    QList<QSharedPointer<QLCInputSource>> spreadSlotInputs() const { return m_spreadSlotInputs; }
    QList<QKeySequence>                   spreadSlotKeys()  const { return m_spreadSlotKeys; }
    void setSpreadSlotInputs(const QList<QSharedPointer<QLCInputSource>>& inputs);
    void setSpreadSlotKeys(const QList<QKeySequence>& keys);
    QSharedPointer<QLCInputSource> spreadSlotInput(int localSlot) const;
    void setSpreadSlotInput(int localSlot, QSharedPointer<QLCInputSource> src);
    QKeySequence spreadSlotKey(int localSlot) const;
    void setSpreadSlotKey(int localSlot, const QKeySequence& key);

    QList<QKeySequence> functionEntryKeys() const { return m_functionEntryKeys; }
    QList<int> functionEntryInputValues() const { return m_functionEntryInputValues; }
    void setFunctionEntryKeys(const QList<QKeySequence>& keys);

    // ---- FunctionParent (required for Function::start/stop) --------------
    FunctionParent functionParent() const;

    // ---- DMXSource -------------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

    // ---- Widget-mode target: expose automation profiles to another MB ----
    int multiButtonOutputCount() const override;
    QString multiButtonOutputName(int outputIdx) const override;
    int multiButtonParameterCount() const override;
    QString multiButtonParameterName(int parameter) const override;
    int multiButtonEntryCount(int outputIdx, int parameter) const override;
    QString multiButtonEntryName(int outputIdx, int parameter, int index) const override;
    int multiButtonCurrentIndex(int outputIdx, int parameter) const override;
    int multiButtonLiveIndex(int outputIdx, int parameter) const override;
    bool multiButtonStagingAvailable(int outputIdx, int parameter) const override;
    quint64 multiButtonStateRevision(int outputIdx, int parameter) const override;
    bool multiButtonHasStagedIndex(int outputIdx, int parameter) const override;
    int multiButtonStagedIndex(int outputIdx, int parameter) const override;
    bool multiButtonActivate(int outputIdx, int parameter, int index) override;
    bool multiButtonActivateStaged(int outputIdx, int parameter, int index) override;
    QSharedPointer<QLCInputSource> multiButtonLiveInputSource(int outputIdx, int parameter) const override;
    bool multiButtonSetLiveInputSource(int outputIdx, int parameter,
                                       QSharedPointer<QLCInputSource> src) override;

    // ---- VCWidget overrides ----------------------------------------------
    QList<QPair<PastePropertyGroup, QString>> pasteablePropertyGroups() const override;
    void applyPropertiesFrom(const VCWidget* source, PastePropertyGroups flags) override;

    void      setPage(int pNum);
    VCWidget* createCopy(VCWidget* parent) override;
    void      toClipboardJson(QJsonObject &obj, const Doc *doc) const override;
    void      fromClipboardJson(const QJsonObject &obj, Doc *doc) override;
    void      updateFeedback() override;
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      postLoad() override;
    void      editProperties() override;

    QMenu*    customMenu(QMenu* parentMenu) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;
    void slotKeyPressed(const QKeySequence& keySequence) override;
    void slotKeyReleased(const QKeySequence& keySequence) override;

private slots:
    void slotLongPressFired();
    void slotCheckChannelValues();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void showEvent(QShowEvent* e) override;

    friend class EntrySelectOverlay;

private:
    void cycleNext();
    void onAutomationTrigger();
    void advanceAutomation();
    void handlePresetChooseInput(uchar value);
    void handleEntrySelectInput(uchar value);
    void handleSpreadPageInput(uchar value);
    void handleCommitInput(uchar value);
    void assignInputSource(const QSharedPointer<QLCInputSource>& src, quint8 id);
    void syncAllInputSourcePages();
    void syncAutomationSuspendDefault();
    void sendPresetChooseFeedback(uchar value);
    void sendInputFeedback(uchar value, const QSharedPointer<QLCInputSource>& src);
    bool isOnInactiveFrameSubPage() const;
    bool acceptsBackgroundInput() const;
    bool acceptsOperationalInput() const;
    bool presetUsesLiveFormulas(const LevelPreset& preset) const;
    static quint8 staticPresetChannelValue(const LevelPreset& preset, int channelIndex);
    quint8 monitorExpectedChannelValue(const LevelPreset& preset, int channelIndex,
                                       const QList<Universe*>& universes) const;
    void activateFromGlobalSlot(int globalSlot);
    void syncEntryInputSources();
    void resizeSpreadSlotInputs();
    int  entryInputLocalSlot(int globalRow) const;
    void activateAutomationLive(int idx);
    void activate(int idx, bool allowFlashEntry = false);
    void stopCurrent();
    bool stagingActive() const;
    bool hasLocalStagedSelection() const;
    void clearLocalStagedSelection();
    void stageEntry(int idx, bool allowFlashEntry = false);
    void commitStaged();
    void clearStagedOnExternalMonitorChange(int matchIdx);
    bool widgetLinkStateSyncNeeded() const;
    void ensureChannelMonitorTimer();
    void updateChannelMonitorTimerInterval();
    void updateChannelMonitorTimerState();
    bool entryIsFlash(int idx) const;
    void beginFlashHold(int idx);
    void endFlashHold();
    int  levelDmxPresetIndex() const;
    void paintTileBackground(QPainter& p, const QRect& rect, int tileIndex,
                             bool isLive, bool isStaged, bool isPressed,
                             QColor& outBg) const;
    void showPopupMenu(const QPoint& globalPos);
    int  pickEntryIndexModal(const QPoint& globalPos);
    void applyEntryPick(int idx);
    void destroyEntrySelectOverlay();
    void clearPressTracking();
    void openEntrySelectPopup();
    void updateEntrySelectPopupHighlight();
    void closeEntrySelectPopup(bool commitSelection = true);
    void cancelEntrySelectPreview();
    void commitEntrySelectPreview();
    void armEntrySelectPopupDismissTimer();
    bool syncWidgetLinkLiveStagedState();
    QSharedPointer<QLCInputSource> widgetLiveInputSourceResolved() const;
    void syncWidgetLiveInputSourceToTarget();
    void markWidgetSelectorPublishPending();
    void setWidgetSelectorLatchedIndex(int idx);
    void publishSelectorToBus(int selectorIdx);
    void commitWidgetBusFromUi(int selectorIdx);
    void applyWidgetRecallInput(uchar value);
    bool widgetBusInPublishGrace() const;
    void writeWidgetBusChannel(QList<Universe*>& universes, quint32 universe,
                               quint32 channel, uchar value,
                               Universe::FaderPriority priority, bool forceLtp);
    int  widgetBusTargetIndex(PresetTableV2MultiButtonTargetIface* target) const;
    static uchar selectorIndexToBusValue(int selectorIdx);
    int  displayedEntryIndex() const;
    void syncEntrySelectInputOutput(uchar rawValue);
    uchar entrySelectOutputValueForSlot(int slot) const;

    bool offSlotAvailable() const;
    bool widgetLinkHasImplicitOff() const;
    int selectableSlotCount() const;
    int slotFromInputValue(uchar value, const QLCInputSource* src) const;
    int slotToEntryIndex(int slot) const;
    void syncWidgetEntryAppearanceCount();
    const LevelPreset* entryAppearancePreset(int idx) const;
    LevelPreset* mutableEntryAppearancePreset(int idx);
    QString linkedWidgetEntryName(int idx) const;

    QString popupMenuTextForEntry(int idx) const;

    void recalcLayoutSize();
    bool widgetLinkEntryCountDynamic() const;
    void syncDynamicEntryCountLayout();
    void clampSpreadPageIndex();
    int  totalSpreadSlots() const;
    int  spreadSlotsPerPage(bool forPaging) const;
    int  spreadPageCount() const;
    bool spreadPagingActive() const;
    int  spreadTileCount() const;
    void resolveSpreadGrid(int& cols, int& rows) const;
    QVector<SpreadTileInfo> computeSpreadTiles() const;
    QSize singleButtonSize() const;
    QSize spreadTotalSize() const;
    int   spreadHitTest(const QPoint& pos) const;
    QString tileCaption(int idx) const;
    void drawTile(QPainter& p, const QRect& tileRect, int tileIndex,
                  bool isLive, bool isStaged, bool isPressed) const;
    int monitorHighlightIndex() const;
    int stagedHighlightIndex() const;
    bool stagedHighlightValid() const;
    bool widgetLinkUsesInternalStaging() const;
    void paintSpread(QPainter& p);
    void paintSingle(QPainter& p);
    QColor buttonTextColor(const QColor& tileBg) const;
    QColor defaultTileBackground() const;
    bool automationVisualActive() const;

    void rebuildSceneCache();
    void updateDmxRegistration();
    void releaseLevelFaders();
    void resetLevelWriteCache();
    void reactivateLevelPreset();

    int     entryCount() const;
    QString entryLabel(int idx) const;
    QString levelPresetDisplayName(int idx) const;
    QString entryIconPath(int idx) const;

    QString   activeFunctionCaption() const;
    Function* functionAt(int idx) const;
    QPixmap   iconForEntry(int idx) const;

    const MultiButtonAutomationProfile* activeAutomationProfilePtr() const;
    QVector<int> buildAllowedAutomationSlots(const MultiButtonAutomationProfile& profile) const;
    bool automationProfileTargetValid(int outputIdx, int parameter) const;
    void bumpAutomationProfileTargetRevision();

    void applyEntryNamesFrom(const MultiButtonWidget* src);
    void applyEntryButtonColorsFrom(const MultiButtonWidget* src);
    void applyChannelBindingsFrom(const MultiButtonWidget* src);
    void applyFunctionAssignmentsFrom(const MultiButtonWidget* src);
    void applyLevelValuesFrom(const MultiButtonWidget* src);

    QList<uchar> resolvedPresetValues(const LevelPreset& preset,
                                      const QList<Universe*>& universes) const;
    quint8 resolvedPresetChannelValue(const LevelPreset& preset, int channelIndex,
                                      const QList<Universe*>& universes) const;
    class PresetTableV2MultiButtonTargetIface* widgetLinkTarget() const;
    class PresetTableV2MultiButtonTargetExtrasIface* widgetLinkTargetExtras() const;
    bool isAllOutputsMode() const;
    int  leaderOutputIndex() const;
    int  widgetLinkReadOutputIndex() const;
    bool activateLinkedOutput(PresetTableV2MultiButtonTargetIface* target, int idx,
                              bool staged) const;
    void normalizeWidgetOutputIndexAfterLoad();
    struct WidgetLinkOutputState
    {
        int liveIdx = -1;
        int stagedIdx = -1;
        bool stagedValid = false;
        bool matches(const WidgetLinkOutputState& other) const;
    };
    struct WidgetLinkConsensusState
    {
        int liveIdx = -1;
        int stagedIdx = -1;
        bool stagedValid = false;
        bool valid = false;
    };
    WidgetLinkOutputState widgetLinkOutputState(PresetTableV2MultiButtonTargetIface* target,
                                                int outputIdx) const;
    bool refreshAllOutputsConsensus(PresetTableV2MultiButtonTargetIface* target);
    void releaseWidgetLiveFaders();

    // ---- Mode ------------------------------------------------------------
    MultiButtonMode m_mode = MultiButtonMode::Function;

    // ---- Function list state --------------------------------------------
    QList<quint32> m_functionIds;
    QStringList    m_functionLabels;
    QStringList    m_iconPaths;
    QList<QSharedPointer<QLCInputSource>> m_functionEntryInputs;
    QList<QKeySequence>                   m_functionEntryKeys;
    QList<int>                            m_functionEntryInputValues;
    QList<bool>                           m_functionEntryFlash;
    QList<bool>                           m_functionEntryFlashOverride;
    QList<bool>                           m_functionEntryFlashForceLtp;
    QList<QColor>                         m_functionEntryLabelColors;

    // ---- Level mode state -----------------------------------------------
    QList<LevelChannelBinding> m_levelChannelBindings;
    QList<LevelPreset>         m_levelPresets;
    QList<LevelPreset>         m_widgetEntryAppearance;
    int                        m_contextMenuEntryIndex = -2;
    quint32                    m_widgetTargetId = VCWidget::invalidId();
    int                        m_widgetOutputIndex = 0;
    int                        m_widgetParameter = 0;
    int                        m_lastResolvedEntryCount = -1;
    QSharedPointer<QLCInputSource> m_widgetLiveInputSource;
    MultiButtonWidgetBusPolicy m_widgetBusPolicy = MultiButtonWidgetBusPolicy::SharedBus;
    quint64        m_widgetTargetStateRevision = 0;
    WidgetLinkConsensusState m_allOutputsConsensusState;
    mutable QMutex     m_dmxMutex;
    QMap<quint32, QSharedPointer<GenericFader>> m_fadersMap;
    QMap<quint32, QSharedPointer<GenericFader>> m_widgetLiveFaders;
    bool           m_widgetLiveWriteDirty = true;
    quint32        m_widgetLiveLastUniverse = UINT_MAX;
    quint32        m_widgetLiveLastChannel = UINT_MAX;
    uchar          m_widgetLiveLastValue = 0;
    uchar          m_lastPublishedValue = 0;
    uchar          m_widgetBusCommittedValue = 0;
    uchar          m_widgetBusLastSeen = 0;
    bool           m_widgetBusForceReassert = false;
    QElapsedTimer  m_publishSuppressTimer;
    bool           m_widgetSelectorLatchedValid = false;
    int            m_widgetSelectorLatchedIndex = -1;
    int            m_lastWrittenPresetIndex = -1;
    QList<uchar>   m_lastWrittenPresetValues;

    int            m_currentIndex = -1;
    bool           m_visualOnly   = false;
    bool           m_widgetLiveActivationOverride = false;
    bool           m_stageBeforeCommit    = false;
    int            m_stagedIndex          = -1;
    bool           m_stagedValid          = false;
    uchar          m_commitInputLastValue = 0;
    QList<bool>    m_entryInputValueMatched;

    // ---- Icon cache (keyed by entry index) ------------------------------
    mutable QHash<int, QPixmap> m_iconCache;

    // ---- Monitor --------------------------------------------------------
    bool                         m_monitorChannelValues = false;
    bool                         m_receiveInputOnInactiveFramePage = false;
    bool                         m_entrySelectAutoCommit         = true;
    bool                         m_logPresetChanges              = false;
    QElapsedTimer                m_entrySelectDebounceTime;
    int                          m_entrySelectDebounceSlot       = -1;
    QTimer*                      m_channelMonitorTimer  = nullptr;
    QList<QList<SceneValue>>     m_cachedSceneValues;
    QElapsedTimer                m_lastActivationTime;
    int                          m_monitorMatchIndex   = -1;  // bus match (paint only)
    int                          m_monitorDisplayIndex = -1;  // observed entry for single-button display
    int                          m_lastMonitorMatchIdx = -2;  // previous tick (staged clear)
    int                          m_pendingMonitorMatchIdx = -2;
    int                          m_pendingMonitorMatchCount = 0;
    int                          m_monitorNoMatchCount = 0;

    // ---- Settings --------------------------------------------------------
    int  m_longPressMs  = 500;
    bool m_addOffAtEnd  = false;

    MultiButtonLayout m_layout           = MultiButtonLayout::Single;
    int               m_spreadColumns    = 0;
    int               m_spreadRows       = 1;
    int               m_spreadHMargin    = 4;
    int               m_spreadVMargin    = 4;
    int               m_spreadTileWidth  = 80;
    int               m_spreadTileHeight = 60;
    int               m_spreadPages      = 0;   // 0 = Auto
    int               m_spreadPageIndex  = 0;
    QList<QSharedPointer<QLCInputSource>> m_spreadSlotInputs;
    QList<QKeySequence>                   m_spreadSlotKeys;

    bool                              m_automationEnabled      = false;
    bool                              m_automationSuspended    = false;
    QList<MultiButtonAutomationProfile> m_automationProfiles;
    int                               m_activeAutomationProfile = 0;
    int                               m_automationPulseCounter  = 0;
    uchar                             m_automationProfileSelectorValue = 0;
    quint64                           m_automationProfileTargetRevision = 1;
    QString                           m_targetListName;
    uchar                             m_triggerLastValue        = 0;
    uchar                             m_automationLastValue     = 0;

    // ---- Press-tracking state (GUI thread only) --------------------------
    QTimer* m_longPressTimer = nullptr;
    bool    m_pressActive    = false;
    bool    m_longFired      = false;
    QPoint  m_pressPos;
    int     m_pressTileIndex = -2;   // spread: entry index, -1=OFF, -2=none

    int     m_flashHoldIndex = -1;   // entry held for flash (-1 = none)
    int     m_restoreIndex   = -1;   // latched index before flash hold

    QPointer<EntrySelectOverlay> m_entrySelectOverlay;
    QTimer*         m_entrySelectDismissTimer  = nullptr;
    bool            m_entrySelectPreviewActive = false;
    int             m_entrySelectPreviewIndex  = -1;
};

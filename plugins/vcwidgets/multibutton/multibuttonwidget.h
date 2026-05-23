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

#include "vcwidget.h"
#include "genericfader.h"
#include "functionparent.h"
#include "scenevalue.h"
#include "dmxsource.h"

class Doc;
class Function;
class EntrySelectOverlay;

enum class MultiButtonMode
{
    Function,
    Level
};

enum class MultiButtonLayout
{
    Single,
    Spread
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

struct LevelPreset
{
    QString       label;
    QString       iconPath;
    QColor        color;      // invalid = use default widget background
    bool          hideName = false;  // true = no text on button (user cleared name)
    QList<quint8> values;   // parallel to m_levelChannelBindings
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
    quint32                   excludeMask = 0;  // bit i excludes entry index i
};

class MultiButtonWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    static const quint8 triggerInputSourceId      = 0;   // cycle-next
    static const quint8 popupInputSourceId        = 1;   // open popup
    static const quint8 automationInputSourceId   = 2;   // advance automation
    static const quint8 presetChooseInputSourceId = 3;   // DMX value selects automation profile
    static const quint8 entrySelectInputSourceId  = 4;   // scaled entry/preset (knob/fader)

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

    bool automationEnabled() const { return m_automationEnabled; }
    void setAutomationEnabled(bool enable);

    QList<MultiButtonAutomationProfile> automationProfiles() const { return m_automationProfiles; }
    void setAutomationProfiles(const QList<MultiButtonAutomationProfile>& profiles,
                               int activeIndex);
    int  activeAutomationProfile() const { return m_activeAutomationProfile; }

    // ---- FunctionParent (required for Function::start/stop) --------------
    FunctionParent functionParent() const;

    // ---- DMXSource -------------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

    // ---- VCWidget overrides ----------------------------------------------
    QList<QPair<PastePropertyGroup, QString>> pasteablePropertyGroups() const override;
    void applyPropertiesFrom(const VCWidget* source, PastePropertyGroups flags) override;

    VCWidget* createCopy(VCWidget* parent) override;
    void      toClipboardJson(QJsonObject &obj, const Doc *doc) const override;
    void      fromClipboardJson(const QJsonObject &obj, Doc *doc) override;
    void      updateFeedback() override;
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      editProperties() override;

    QMenu*    customMenu(QMenu* parentMenu) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

private slots:
    void slotLongPressFired();
    void slotCheckChannelValues();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    void paintEvent(QPaintEvent* e) override;

    friend class EntrySelectOverlay;

private:
    void cycleNext();
    void onAutomationTrigger();
    void advanceAutomation();
    void handlePresetChooseInput(uchar value);
    void handleEntrySelectInput(uchar value);
    void activate(int idx);
    void stopCurrent();
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
    int  displayedEntryIndex() const;
    void syncEntrySelectInputOutput(uchar rawValue);
    uchar entrySelectOutputValueForSlot(int slot) const;

    int selectableSlotCount() const;
    int slotFromInputValue(uchar value, const QLCInputSource* src) const;
    int slotToEntryIndex(int slot) const;

    QString popupMenuTextForEntry(int idx) const;

    void recalcLayoutSize();
    int  spreadTileCount() const;
    void resolveSpreadGrid(int& cols, int& rows) const;
    QVector<SpreadTileInfo> computeSpreadTiles() const;
    QSize singleButtonSize() const;
    QSize spreadTotalSize() const;
    int   spreadHitTest(const QPoint& pos) const;
    QString tileCaption(int idx) const;
    void drawTile(QPainter& p, const QRect& tileRect, int tileIndex,
                  bool isActive, bool isPressed) const;
    void paintSpread(QPainter& p);
    void paintSingle(QPainter& p);

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

    void applyEntryNamesFrom(const MultiButtonWidget* src);
    void applyChannelBindingsFrom(const MultiButtonWidget* src);
    void applyFunctionAssignmentsFrom(const MultiButtonWidget* src);
    void applyLevelValuesFrom(const MultiButtonWidget* src);

    // ---- Mode ------------------------------------------------------------
    MultiButtonMode m_mode = MultiButtonMode::Function;

    // ---- Function list state --------------------------------------------
    QList<quint32> m_functionIds;
    QStringList    m_functionLabels;
    QStringList    m_iconPaths;

    // ---- Level mode state -----------------------------------------------
    QList<LevelChannelBinding> m_levelChannelBindings;
    QList<LevelPreset>         m_levelPresets;
    mutable QMutex     m_dmxMutex;
    QMap<quint32, QSharedPointer<GenericFader>> m_fadersMap;
    int            m_lastWrittenPresetIndex = -1;
    QList<uchar>   m_lastWrittenPresetValues;

    int            m_currentIndex = -1;
    bool           m_visualOnly   = false;

    // ---- Icon cache (keyed by entry index) ------------------------------
    mutable QHash<int, QPixmap> m_iconCache;

    // ---- Monitor --------------------------------------------------------
    bool                         m_monitorChannelValues = false;
    QTimer*                      m_channelMonitorTimer  = nullptr;
    QList<QList<SceneValue>>     m_cachedSceneValues;
    QElapsedTimer                m_lastActivationTime;

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

    bool                              m_automationEnabled      = false;
    bool                              m_automationSuspended    = false;
    QList<MultiButtonAutomationProfile> m_automationProfiles;
    int                               m_activeAutomationProfile = 0;
    int                               m_automationPulseCounter  = 0;

    // ---- Press-tracking state (GUI thread only) --------------------------
    QTimer* m_longPressTimer = nullptr;
    bool    m_pressActive    = false;
    bool    m_longFired      = false;
    QPoint  m_pressPos;
    int     m_pressTileIndex = -2;   // spread: entry index, -1=OFF, -2=none

    QPointer<EntrySelectOverlay> m_entrySelectOverlay;
    QTimer*         m_entrySelectDismissTimer  = nullptr;
    bool            m_entrySelectPreviewActive = false;
    int             m_entrySelectPreviewIndex  = -1;
};

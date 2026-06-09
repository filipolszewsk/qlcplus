/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2widget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QMutex>
#include <QHash>
#include <QVector>
#include <QList>
#include <QTableWidget>
#include <QTableView>
#include <QHeaderView>
#include <QToolBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QClipboard>

#include <QJsonObject>
#include <QJsonArray>
#include "vcwidget.h"
#include "dmxsource.h"
#include "genericfader.h"
#include "qlcinputsource.h"
#include "qlcchannel.h"
#include "presettablev2effectengine.h"
#include "presettablev2controliface.h"
#include "presettablev2multibuttoniface.h"
#include "presettablev2transitionprovideriface.h"
#include "ptparammatrixengine.h"

class Doc;
class FixtureGroup;
class MasterTimer;
class Universe;
class GenericFader;
class GroupHead;
class Fixture;

// ---------------------------------------------------------------------------
// Widget mode
// ---------------------------------------------------------------------------

enum class PTMode { Legacy = 0, FixtureGroup = 1 };

/** Which fixture-group grid cells an output drives (see Doc mask for Mask / RowsAndMask). */
enum class PTOutputScope
{
    Rows = 0,
    Mask,
    RowsAndMask
};

enum class PTContinuousFxSelectorMode
{
    Live = 0,
    StagedCommit,
    SmoothMorph
};

enum class PTWidgetFlashBehavior
{
    PrimaryRowModifier = 0,
    StagedRowTrigger
};

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

struct PTOption {
    QString name;
    uchar   value    = 0;
    QString resource; // image path (Picture cap) or "#RRGGBB" (SingleColor); empty = no icon
};

struct PTColumnTypeBinding {
    QString  manufacturer;
    QString  model;
    QString  modeName;
    qint32   channelIndex = -1;  // 0-based index within the fixture mode channels
    bool isValid() const { return channelIndex >= 0 && !manufacturer.isEmpty(); }
    bool operator==(const PTColumnTypeBinding& o) const
    {
        return manufacturer == o.manufacturer && model == o.model
                && modeName == o.modeName && channelIndex == o.channelIndex;
    }
};

struct PTColumn {
    enum Type { Numeric, Dropdown, Scaler };

    QString           name;
    Type              type         = Numeric;
    bool              fade         = true;    // true=interpolate, false=snap at 127
    QVector<PTOption> options;                // used when type == Dropdown
    int               width        = -1;      // persisted pixel width; -1 = Qt default
    QVector<PTColumnTypeBinding> bindings;    // used only in PTMode::FixtureGroup
    bool hasBindings() const
    {
        for (const PTColumnTypeBinding& b : bindings)
        {
            if (b.isValid())
                return true;
        }
        return false;
    }
    // Scaler type fields
    int               scalerMin    = 0;
    int               scalerMax    = 360;
    QString           scalerSuffix;           // e.g. "°"
};

struct PTRow {
    QString        name;
    QVector<uchar> values;  // size == number of value columns
};

struct PTOutput {
    QString    name;
    quint32    fixtureId = UINT_MAX;    // used in PTMode::Legacy
    QList<int> groupRows;               // used in PTMode::FixtureGroup: y-coords in group grid
    PTOutputScope scope = PTOutputScope::RowsAndMask;
    /** Default Transition preset when no DMX; -1 = instant (legacy selector_sweep 0). */
    int        sweepPresetIndex = -1;
    /** Default Continuous FX preset; -1 = off (legacy selector_continuous 0). */
    int        continuousPresetIndex = -1;
    /** Default MultiFX background preset; -1 = off. */
    int        multiFxPresetIndex = -1;
    /** Secondary table row for continuous FX (-1 = off). Overridden by external input when mapped. */
    int        secondaryRowIndex = -1;
};

// ---------------------------------------------------------------------------
// Delegate — SpinBox for Numeric, ComboBox for Dropdown
// ---------------------------------------------------------------------------

class PresetTableV2Widget;  // forward decl

class PresetTableV2Delegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PresetTableV2Delegate(QObject* parent = nullptr);

    // Column definitions provided by the widget (pointer, not owned)
    void setColumns(const QVector<PTColumn>* columns);
    // Back-pointer to owner widget (for capability lookup)
    void setOwner(const PresetTableV2Widget* owner);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    const QVector<PTColumn>*    m_columns = nullptr;
    const PresetTableV2Widget*    m_owner   = nullptr;
};

// ---------------------------------------------------------------------------
// Main widget
// ---------------------------------------------------------------------------

class PresetTableV2Widget : public VCWidget, public DMXSource, public PresetTableV2ControlIface,
                            public PresetTableV2MultiButtonTargetIface,
                            public PresetTableV2MultiButtonTargetExtrasIface,
                            public PresetTableV2MultiButtonFlashIface
{
    Q_OBJECT
    Q_INTERFACES(PresetTableV2ControlIface)
    Q_INTERFACES(PresetTableV2MultiButtonTargetIface)
    Q_INTERFACES(PresetTableV2MultiButtonTargetExtrasIface)
    Q_INTERFACES(PresetTableV2MultiButtonFlashIface)

public:
    explicit PresetTableV2Widget(QWidget* parent, Doc* doc);
    ~PresetTableV2Widget() override;

    // ---- Data accessors (GUI thread) -------------------------------------
    int  numValueColumns() const { return m_columns.size(); }

    const QVector<PTColumn>& columns() const { return m_columns; }
    const QVector<PTRow>&    rows()    const { return m_rows;    }
    const QVector<PTOutput>& outputs() const { return m_outputs; }

    PTMode   widgetMode()      const { return m_mode; }
    quint32  fixtureGroupId()  const { return m_fixtureGroupId; }

    void setColumns(const QVector<PTColumn>& cols);
    void setRows(const QVector<PTRow>& rows);
    void setOutputs(const QVector<PTOutput>& outs);

    bool spatialEffectsEnabled() const override;
    void setSpatialEffectsEnabled(bool enabled) override;

    PTSpatialEffectSettings spatialEffectSettings() const override;
    void setSpatialEffectSettings(const PTSpatialEffectSettings& settings) override;

    quint32 linkedTransitionWidgetId() const override;
    void setLinkedTransitionWidgetId(quint32 id) override;
    void refreshTransitionPresetCache() override;
    void requestTableFlash(int tableRowIndex, int transitionPresetIndex) override;
    bool continuousCrossfadeStagedEditing() const override;
    int outputCountForPresetOverrides() const override;
    QString outputNameForPresetOverride(int outputIdx) const override;
    int fixtureGroupSpanAlongAxis(const PTTransitionPreset& preset,
                                  const PTGlobalEffectSettings& global) const override;
    bool spatialGridPreview(const PTTransitionPreset& preset,
                            const PTGlobalEffectSettings& global,
                            PTSpatialGridPreview& out) const override;

    int multiButtonOutputCount() const override;
    QString multiButtonOutputName(int outputIdx) const override;
    bool multiButtonSupportsAllOutputs() const override;
    int multiButtonParameterCount() const override;
    QString multiButtonParameterName(int parameter) const override;
    int multiButtonEntryCount(int outputIdx, int parameter) const override;
    QString multiButtonEntryName(int outputIdx, int parameter, int index) const override;
    int multiButtonCurrentIndex(int outputIdx, int parameter) const override;
    int multiButtonLiveIndex(int outputIdx, int parameter) const override;
    bool multiButtonStagingAvailable(int outputIdx, int parameter) const override;
    quint64 multiButtonStateRevision(int outputIdx, int parameter) const override;
    bool multiButtonOutputControlsParameter(int outputIdx, int parameter) const override;
    bool multiButtonHasStagedIndex(int outputIdx, int parameter) const override;
    int multiButtonStagedIndex(int outputIdx, int parameter) const override;
    bool multiButtonActivate(int outputIdx, int parameter, int index) override;
    bool multiButtonActivateStaged(int outputIdx, int parameter, int index) override;
    QSharedPointer<QLCInputSource> multiButtonLiveInputSource(int outputIdx, int parameter) const override;
    bool multiButtonSetLiveInputSource(int outputIdx, int parameter,
                                       QSharedPointer<QLCInputSource> src) override;
    bool multiButtonBeginFlash(int outputIdx, int parameter, int index,
                               quint32 sourceWidgetId, quint64 token,
                               double timeMultiplier = 1.0) override;
    bool multiButtonEndFlash(int outputIdx, int parameter, int index,
                             quint32 sourceWidgetId, quint64 token) override;
    bool multiButtonFlashGateActive() const override;

    // ---- VCWidget overrides -----------------------------------------------
    VCWidget* createCopy(VCWidget* parent) override;
    void      toClipboardJson(QJsonObject &obj, const Doc *doc) const override;
    void      fromClipboardJson(const QJsonObject &obj, Doc *doc) override;
    void      updateFeedback() override;
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      editProperties() override;

    // ---- DMXSource --------------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;
    void slotKeyPressed(const QKeySequence& keySequence) override;
    void slotKeyReleased(const QKeySequence& keySequence) override;

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private slots:
    void slotAddRow();
    void slotRemoveRow();
    void slotAddColumn();
    void slotRemoveColumn();
    void slotProperties();
    void slotCellChanged(int row, int col);
    void slotColumnHeaderDoubleClicked(int logicalIndex);

    // ---- Multi-column resize ---------------------------------------------
    void slotHeaderSectionResized(int logicalIndex, int oldSize, int newSize);

    // ---- Copy / paste ----------------------------------------------------
    void slotCopySelection();
    void slotPasteSelection();
    void slotTableContextMenu(const QPoint& pos);
    void slotFixtureGroupMaskChanged(quint32 groupId);

private:
    void rebuildTable();
    void syncFrozenNameColumnLayout();
    void refreshTableFromData();
    void setActiveRow(int outputIdx, int rowIdx);   // -1 = off
    void refreshRowHighlights();
    void syncDataFromTable();
    void syncAllDataFromTable();

    // Helper: paste a raw string value into a table item and update m_rows
    void pasteValueToItem(QTableWidgetItem* item, const QString& raw);

    // writeDMX helpers
    void writeDMXLegacy(QList<Universe*>& universes, uchar xfEffective);
    void writeDMXFixtureGroup(MasterTimer* timer, QList<Universe*>& universes, uchar xfEffective);

    void applyPointChannels(GenericFader* fader, Universe* uni,
                            const GroupHead& head, Fixture* fxi,
                            const QLCPoint& pt, const QVector<uchar>& aVals,
                            uint fadeTimeMs);

    void applyBlendedPointChannels(GenericFader* fader, Universe* uni,
                                   const GroupHead& head, Fixture* fxi,
                                   const QLCPoint& pt,
                                   const QVector<uchar>& priVals,
                                   const QVector<uchar>& secVals,
                                   double dimmer,
                                   quint32 presetFadeMs,
                                   int waveShape,
                                   int waveFadeIn = 25,
                                   int waveFadeOut = 25,
                                   uchar intensity = 255);

    PresetTableV2TransitionProviderIface* linkedTransitionProvider() const;
    /** Caller must hold m_stateMutex (writeDMX path). */
    PresetTableV2TransitionProviderIface* transitionProviderLocked() const;
    PTGlobalEffectSettings globalEffectSettingsLocked() const;
    quint32 cycleDurationMsLocked(const PTGlobalEffectSettings& global,
                                    const PTTransitionPreset& preset) const;
    static void rescaleElapsedForDurationChange(quint32& elapsedMs,
                                                quint32 oldDurationMs,
                                                quint32 newDurationMs);
    void ensurePhaseStableCycleLocked(QVector<quint32>& elapsed,
                                      QVector<quint32>& lastCycle,
                                      int outputIdx,
                                      quint32 currentCycleMs);

    PTTransitionPreset transitionPresetForOutput(int outputIdx) const;
    /** Caller must hold m_stateMutex (writeDMX path). */
    PTTransitionPreset transitionPresetForOutputLocked(int outputIdx) const;
    PTTransitionPreset transitionPresetAtIndexLocked(PTTransitionMode mode, int presetIndex,
                                                     int outputIdx = -1) const;
    PTTransitionPreset sweepPresetForOutputLocked(int outputIdx) const;
    PTTransitionPreset continuousPresetForOutputLocked(int outputIdx) const;
    PTTransitionPreset continuousPresetForOutputLocked(int outputIdx, uchar xfEffective) const;
    PTTransitionPreset multiFxPresetForOutputLocked(int outputIdx) const;
    struct PTContinuousLayerState
    {
        bool active = false;
        int primaryRow = -1;
        int secondaryRow = -1;
        QVector<uchar> livePrimaryValues;
        QVector<uchar> liveSecondaryValues;
        QVector<uchar> primaryValues;
        QVector<uchar> secondaryValues;
        PTTransitionPreset livePreset;
        PTTransitionPreset preset;
        bool hasStaged = false;
    };
    PTContinuousLayerState continuousLayerStateForOutputLocked(int outputIdx,
                                                               int activeRow,
                                                               uchar xfEffective) const;
    int liveSweepPresetIndexLocked(int outputIdx) const;
    int liveContinuousPresetIndexLocked(int outputIdx) const;
    int liveMultiFxPresetIndexLocked(int outputIdx) const;
    int rawLiveSecondaryRowIndexLocked(int outputIdx) const;
    int liveSecondaryRowIndexLocked(int outputIdx) const;
    void sendLiveSelectorFeedbackLocked(int outputIdx);
    bool sweepEfxActiveForOutputLocked(int outputIdx) const;
    bool continuousEfxActiveForOutputLocked(int outputIdx) const;
    bool multiFxActiveForOutputLocked(int outputIdx) const;
    bool hasStagedMultiFxPresetLocked(int outputIdx) const;
    int stagedMultiFxPresetIndexLocked(int outputIdx) const;
    bool hasStagedMultiFxAnyLocked() const;
    /** Sweep on primary row change only when Continuous is not driving the layer. */
    bool sweepOnPrimaryChangeLocked(int outputIdx, int newActiveRow) const;
    /** Secondary table row for Continuous only (DMX 128+ / Properties / activeRow). */
    int effectiveSecondaryRowLocked(int outputIdx, int activeRow) const;

    bool continuousCrossfadeModeLocked(int outputIdx) const;
    bool multiFxCrossfadeModeLocked(int outputIdx) const;
    bool crossfadeSweepModeLocked(int outputIdx, int activeRow, bool hasStaged) const;
    bool continuousCrossfadeActiveAnyLocked() const;
    bool continuousFxSelectionStagedAnyLocked() const;
    bool continuousFxSelectorToStagedLocked() const;
    bool crossfadeManualControlEnabledLocked() const;
    bool crossfadeRoutesToStagedLocked() const;
    double crossfadeProgress01Locked(uchar xfEffective) const;
    void armCrossfadeStagingLocked();
    bool crossfadeHasStagedChangesLocked() const;
    void stagePrimaryRowLocked(int outputIdx, int rowIdx);
    void materializeContinuousRowsLocked(int outputIdx, bool toStaged);
    void syncCommittedPlaybackStateLocked(int outputIdx, bool resetFxPlayback);
    void clearStagedLayerLocked(int outputIdx);
    void stageSecondaryRowLocked(int outputIdx, int rowIdx);
    void stageSweepPresetLocked(int outputIdx, int presetIdx);
    void stageContinuousPresetLocked(int outputIdx, int presetIdx);
    void stageMultiFxPresetLocked(int outputIdx, int presetIdx);
    void ensureMultiButtonRevisionSizeLocked();
    int multiButtonRevisionSlotLocked(int parameter) const;
    void bumpMultiButtonStateRevisionLocked(int outputIdx, int parameter);
    void tickCrossfadeClockLocked(MasterTimer* timer);
    void resetCrossfadeClockLocked();
    /** Reset MultiFX phase when crossfade fader leaves the start edge (mirror cue-list EFX lazy-start). */
    void syncMultiFxPhaseOnCrossfadeMotionLocked();
    void resetMultiFxCrossfadePhaseAnchorLocked();
    quint32 crossfadeClockCycleMsLocked(const PTGlobalEffectSettings& global) const;
    uchar crossfadeEffectiveLocked(uchar xfPos, uchar xfStartPos) const;
    void promoteStagedToLiveLocked();

    void syncLiveTransitionFromOutputs();
    void writeContinuousSpatial(int outputIdx, MasterTimer* timer,
                                QList<Universe*>& universes, const PTOutput& out,
                                const QVector<uchar>& priVals, const QVector<uchar>& secVals,
                                const QSize& gridSize,
                                const QMap<QLCPoint, GroupHead>& headsMap,
                                const PTTransitionPreset* presetOverride = nullptr,
                                const QVector<uchar>* stagedPriVals = nullptr,
                                const QVector<uchar>* stagedSecVals = nullptr,
                                const PTTransitionPreset* stagedPresetOverride = nullptr,
                                double morphProgress = 0.0,
                                bool useMultiFx = false);

    void startSpatialChase(int outputIdx, int rowIdx, const QList<QLCPoint>& points,
                          const PTTransitionPreset& preset, int gridWidth, int gridHeight);
    void tickSpatialChase(int outputIdx, MasterTimer* timer,
                          QList<Universe*>& universes, const PTOutput& out,
                          const QVector<uchar>& aVals);

    bool useMatrixEngineLocked() const;
    /** Linked EFX Engine with at least one sweep preset (ignores spatial checkbox). */
    bool matrixProviderReadyLocked() const;
    /** Per-output: bank preset active (efx_selector / Properties default >= 0). */
    bool efxActiveForOutputLocked(int outputIdx) const;
    struct PTOutputPlaybackState
    {
        bool transitionOn = false;
        bool continuousFxOn = false;
        int secondaryRow = -1;
        bool crossfadeTransition = false;
        bool crossfadeContinuous = false;
        bool blockMatrixForStaged = false;
        bool matrixForOutput = false;
    };
    PTOutputPlaybackState resolveOutputPlaybackStateLocked(int outputIdx, int activeRow,
                                                           bool hasStaged,
                                                           bool matrixReady,
                                                           bool spatialOn) const;
    void ensureMatrixState(int outputIdx);
    void resetMatrixStateLocked(int outputIdx);
    void resetAllMatrixStatesLocked();
    void beginMatrixSweepLocked(int outputIdx, int prevRow, int newRowIdx,
                                bool forceSpatialSweep = false);
    bool beginMatrixFlashLocked(int outputIdx, int rowIdx, int transitionPresetIndex,
                                quint32 sourceWidgetId, quint64 token,
                                double timeMultiplier = 1.0);
    bool endMatrixFlashLocked(int outputIdx, int rowIdx, quint32 sourceWidgetId,
                              quint64 token);
    void releaseMatrixFlashLocked(int outputIdx);
    void beginMatrixFlashWaveOutLocked(PTOutputMatrixState& st);
    void setWidgetFlashGateActiveLocked(bool active, uchar value);
    void beginWidgetStagedFlashLocked();
    void endWidgetStagedFlashLocked();
    void writeMatrixSpatial(int outputIdx, MasterTimer* timer, QList<Universe*>& universes,
                            const PTOutput& out, int activeRow, int secondaryRow,
                            const PTTransitionPreset& preset,
                            const PTGlobalEffectSettings& global,
                            const QSize& gridSize,
                            const QMap<QLCPoint, GroupHead>& headsMap,
                            bool forceContinuousBlend = false,
                            const QVector<uchar>* primaryOverride = nullptr,
                            const QVector<uchar>* secondaryOverride = nullptr,
                            const QVector<uchar>* stagedPrimaryOverride = nullptr,
                            const QVector<uchar>* stagedSecondaryOverride = nullptr,
                            const PTTransitionPreset* stagedPresetOverride = nullptr,
                            double morphProgress = 0.0,
                            bool useMultiFx = false);

public:
    // Resolve the QLCChannel* bound to a column (FixtureGroup mode only); nullptr otherwise.
    const QLCChannel* resolveBoundChannel(const PTColumn& col) const;

    // ---- Shared state (mutex-protected, read in writeDMX) ----------------
    mutable QMutex   m_stateMutex;
    QVector<PTColumn> m_columns;
    QVector<PTRow>    m_rows;
    QVector<PTOutput> m_outputs;
    QVector<int>      m_activeRow;           // per output, -1 = off
    QVector<int>      m_stagedRow;           // per output, -1 = off/no row; m_stagedRowValid disambiguates
    QVector<bool>     m_stagedRowValid;
    bool              m_crossfadeEnabled   = false;  // widget-level toggle
    uchar             m_crossfadeGlobalPos = 0;      // physical fader position 0-255
    uchar             m_crossfadeStartPos  = 0;      // fader position when first staging was set
    uchar             m_crossfadePrevPos   = 0;      // previous physical fader position
    bool              m_crossfadeStagedAtLowSide = true;
    quint32           m_crossfadeClockElapsedMs = 0;
    quint32           m_crossfadeClockLastCycleMs = 0;
    double            m_crossfadeClockProgress01 = 0.0;
    bool              m_crossfadeLastManualControl = true;
    bool              m_crossfadeSessionActive = false;
    bool              m_crossfadeEditLaneStaged = true;
    bool              m_initialInputSyncPending = false;
    /** When true, reset MultiFX phase as manual crossfade leaves the start edge. */
    bool              m_syncMultiFxPhaseToCrossfade = false;
    /** Hold staged MultiFX clock at 0 for this many ms after crossfade anchor (tune vs cue-list EFX). */
    int               m_multiFxCrossfadeSyncOffsetMs = 40;
    bool              m_multiFxXfPhaseAnchored = false;
    int               m_multiFxStagedHoldTicksRemaining = 0;

    QVector<int>      m_stagedSecondaryRow;
    QVector<int>      m_stagedSweepPreset;
    QVector<int>      m_stagedContinuousPreset;
    QVector<int>      m_stagedMultiFxPreset;
    QVector<bool>     m_stagedSecondaryValid;
    QVector<bool>     m_stagedSweepValid;
    QVector<bool>     m_stagedContinuousValid;
    QVector<bool>     m_stagedMultiFxValid;
    QVector<QVector<quint64>> m_multiButtonStateRevision;

    PTMode            m_mode            = PTMode::Legacy;
    quint32           m_fixtureGroupId  = UINT_MAX;  // valid only when m_mode == FixtureGroup

    PTSpatialEffectSettings m_spatialEffects;
    PTContinuousFxSelectorMode m_continuousFxSelectorMode = PTContinuousFxSelectorMode::StagedCommit;
    quint32                 m_linkedTransitionWidgetId = VCWidget::invalidId();
    quint32                     m_cachedTransitionWidgetId = VCWidget::invalidId();
    int                         m_cachedTransitionSweepCount = 0;
    int                         m_cachedTransitionContinuousCount = 0;
    int                         m_cachedTransitionMultiFxCount = 0;
    QVector<int>                m_liveSweepPreset;
    QVector<int>                m_liveContinuousPreset;
    QVector<int>                m_liveMultiFxPreset;
    QVector<int>                m_liveSecondaryRow;
    QVector<quint32>            m_continuousElapsedMs;
    QVector<quint32>            m_multiFxElapsedMs;
    QVector<quint32>            m_multiFxStagedElapsedMs;
    QVector<quint32>            m_continuousLastCycleMs;
    QVector<quint32>            m_multiFxLastCycleMs;
    QVector<quint32>            m_multiFxStagedLastCycleMs;
    QKeySequence                m_multiFxRestartKey;
    uchar                       m_multiFxBlend = 0;
    QVector<int>            m_spatialAppliedRow;
    QVector<PTSpatialChaseOutput> m_spatialChase;
    QVector<PTOutputMatrixState>    m_matrixState;
    QVector<int>                    m_flashInputHeldRow;
    bool                            m_widgetFlashGateActive = false;
    uchar                           m_widgetFlashGateLastValue = 0;
    QKeySequence                    m_widgetFlashGateKey;
    int                             m_widgetFlashTimeMultiplierIndex = 2;
    PTWidgetFlashBehavior           m_widgetFlashBehavior =
            PTWidgetFlashBehavior::PrimaryRowModifier;
    quint64                         m_nextWidgetStagedFlashToken = 1;
    quint64                         m_widgetStagedFlashToken = 0;

    // ---- DMX faders (per universe, lazy) ---------------------------------
    QHash<quint32, QSharedPointer<GenericFader>> m_faders;

    // ---- UI (GUI thread only) --------------------------------------------
    QVBoxLayout*          m_layout     = nullptr;
    QToolBar*             m_toolbar    = nullptr;
    QTableWidget*         m_table      = nullptr;
    QTableView*           m_nameFrozenTable = nullptr;
    QLabel*               m_statusBar  = nullptr;
    PresetTableV2Delegate*  m_delegate   = nullptr;

    // Actions hidden in Operate mode (structural/column/props)
    QAction* m_actAddCol  = nullptr;
    QAction* m_actRemCol  = nullptr;
    QAction* m_actProps   = nullptr;
    QAction* m_actColSep  = nullptr;
    QAction* m_actPropSep = nullptr;

    bool m_rebuildingTable  = false;   // guard against recursive slotCellChanged
    bool m_resizingColumns  = false;
    int  m_nameColWidth     = -1;      // persisted width of Name column (col 0)

    // Badge colors for up to 8 outputs
    static const QColor s_outputColors[8];
};

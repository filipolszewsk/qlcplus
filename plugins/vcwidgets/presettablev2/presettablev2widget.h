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
};

struct PTColumn {
    enum Type { Numeric, Dropdown, Scaler };

    QString           name;
    Type              type         = Numeric;
    bool              fade         = true;    // true=interpolate, false=snap at 127
    QVector<PTOption> options;                // used when type == Dropdown
    int               width        = -1;      // persisted pixel width; -1 = Qt default
    PTColumnTypeBinding binding;              // used only in PTMode::FixtureGroup
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
    /** Default sweep bank preset when no DMX; -1 = off (selector_sweep 0). */
    int        sweepPresetIndex = -1;
    /** Default continuous bank preset; -1 = off (selector_continuous 0). */
    int        continuousPresetIndex = -1;
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

class PresetTableV2Widget : public VCWidget, public DMXSource, public PresetTableV2ControlIface
{
    Q_OBJECT
    Q_INTERFACES(PresetTableV2ControlIface)

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

protected:
    void paintEvent(QPaintEvent* e) override;
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

    PTTransitionPreset transitionPresetForOutput(int outputIdx) const;
    /** Caller must hold m_stateMutex (writeDMX path). */
    PTTransitionPreset transitionPresetForOutputLocked(int outputIdx) const;
    PTTransitionPreset transitionPresetAtIndexLocked(PTTransitionMode mode, int presetIndex) const;
    PTTransitionPreset sweepPresetForOutputLocked(int outputIdx) const;
    PTTransitionPreset continuousPresetForOutputLocked(int outputIdx) const;
    int liveSweepPresetIndexLocked(int outputIdx) const;
    int liveContinuousPresetIndexLocked(int outputIdx) const;
    bool sweepEfxActiveForOutputLocked(int outputIdx) const;
    bool continuousEfxActiveForOutputLocked(int outputIdx) const;
    /** Sweep on primary row change only when Continuous is not driving the layer. */
    bool sweepOnPrimaryChangeLocked(int outputIdx, int newActiveRow) const;
    /** Secondary table row for Continuous only (DMX 128+ / Properties / activeRow). */
    int effectiveSecondaryRowLocked(int outputIdx, int activeRow) const;

    bool continuousCrossfadeModeLocked(int outputIdx) const;
    bool crossfadeSweepModeLocked(int outputIdx, int activeRow, bool hasStaged) const;
    bool continuousCrossfadeActiveAnyLocked() const;
    void ensureStagedSnapshotLocked(int outputIdx);
    void promoteStagedToLiveLocked();

    void syncLiveTransitionFromOutputs();
    void writeContinuousSpatial(int outputIdx, MasterTimer* timer,
                                QList<Universe*>& universes, const PTOutput& out,
                                const QVector<uchar>& priVals, const QVector<uchar>& secVals,
                                const QSize& gridSize,
                                const QMap<QLCPoint, GroupHead>& headsMap);

    void startSpatialChase(int outputIdx, int rowIdx, const QList<QLCPoint>& points,
                          const PTTransitionPreset& preset, int gridWidth, int gridHeight);
    void tickSpatialChase(int outputIdx, MasterTimer* timer,
                          QList<Universe*>& universes, const PTOutput& out,
                          const QVector<uchar>& aVals);

    bool useMatrixEngineLocked() const;
    /** Per-output: bank preset active (efx_selector / Properties default >= 0). */
    bool efxActiveForOutputLocked(int outputIdx) const;
    void ensureMatrixState(int outputIdx);
    void resetMatrixStateLocked(int outputIdx);
    void resetAllMatrixStatesLocked();
    void beginMatrixSweepLocked(int outputIdx, int prevRow, int newRowIdx);
    void releaseMatrixFlashLocked(int outputIdx);
    void writeMatrixSpatial(int outputIdx, MasterTimer* timer, QList<Universe*>& universes,
                            const PTOutput& out, int activeRow, int secondaryRow,
                            const PTTransitionPreset& preset,
                            const PTGlobalEffectSettings& global,
                            const QSize& gridSize,
                            const QMap<QLCPoint, GroupHead>& headsMap,
                            bool forceContinuousBlend = false);

public:
    // Resolve the QLCChannel* bound to a column (FixtureGroup mode only); nullptr otherwise.
    const QLCChannel* resolveBoundChannel(const PTColumn& col) const;

    // ---- Shared state (mutex-protected, read in writeDMX) ----------------
    mutable QMutex   m_stateMutex;
    QVector<PTColumn> m_columns;
    QVector<PTRow>    m_rows;
    QVector<PTOutput> m_outputs;
    QVector<int>      m_activeRow;           // per output, -1 = off
    QVector<int>      m_stagedRow;           // per output, -1 = no staging (only used when crossfade enabled)
    bool              m_crossfadeEnabled   = false;  // widget-level toggle
    uchar             m_crossfadeGlobalPos = 0;      // physical fader position 0-255
    uchar             m_crossfadeStartPos  = 0;      // fader position when first staging was set
    uchar             m_crossfadePrevPos   = 0;      // previous fader pos (127/128 edge detect)

    QVector<int>      m_stagedSecondaryRow;
    QVector<int>      m_stagedSweepPreset;
    QVector<int>      m_stagedContinuousPreset;
    QVector<bool>     m_stagedSnapshotValid;

    PTMode            m_mode            = PTMode::Legacy;
    quint32           m_fixtureGroupId  = UINT_MAX;  // valid only when m_mode == FixtureGroup

    PTSpatialEffectSettings m_spatialEffects;
    quint32                 m_linkedTransitionWidgetId = VCWidget::invalidId();
    quint32                     m_cachedTransitionWidgetId = VCWidget::invalidId();
    int                         m_cachedTransitionSweepCount = 0;
    int                         m_cachedTransitionContinuousCount = 0;
    QVector<int>                m_liveSweepPreset;
    QVector<int>                m_liveContinuousPreset;
    QVector<int>                m_liveSecondaryRow;
    QVector<quint32>            m_continuousElapsedMs;
    QVector<int>            m_spatialAppliedRow;
    QVector<PTSpatialChaseOutput> m_spatialChase;
    QVector<PTOutputMatrixState>    m_matrixState;
    QVector<int>                    m_flashInputHeldRow;

    // ---- DMX faders (per universe, lazy) ---------------------------------
    QHash<quint32, QSharedPointer<GenericFader>> m_faders;

    // ---- UI (GUI thread only) --------------------------------------------
    QVBoxLayout*          m_layout     = nullptr;
    QToolBar*             m_toolbar    = nullptr;
    QTableWidget*         m_table      = nullptr;
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

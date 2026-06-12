/*
  QLC+ VC Widget Plugin — Preset Table v2 EFX Engine (UI only, no DMX)
*/

#pragma once

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeView>
#include <QAction>
#include <QToolBar>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QHash>
#include <QMutex>
#include <QSet>
#include "vcwidget.h"
#include "presettablev2transitionprovideriface.h"
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"
#include "pttransitioncolumngroupbar.h"

class Doc;
class PresetTableV2ControlIface;
class PTDimmerWaveCurveWidget;
class PTSpatialFixtureGridWidget;

struct PTTransitionCellKey
{
    QTreeWidgetItem* item = nullptr;
    int col = -1;
    bool operator==(const PTTransitionCellKey& other) const
    {
        return item == other.item && col == other.col;
    }
};

inline size_t qHash(const PTTransitionCellKey& key, size_t seed = 0) noexcept
{
    seed = ::qHash(reinterpret_cast<quintptr>(key.item), seed);
    return ::qHash(key.col, seed);
}

class PresetTableV2TransitionWidget : public VCWidget,
                                       public PresetTableV2TransitionProviderIface
{
    Q_OBJECT
    Q_INTERFACES(PresetTableV2TransitionProviderIface)

public:
    explicit PresetTableV2TransitionWidget(QWidget* parent, Doc* doc);
    ~PresetTableV2TransitionWidget() override;

    quint32 targetTableId() const;
    void setTargetTableId(quint32 id);

    PresetTableV2ControlIface* linkedTable() const;
    Doc* doc() const { return m_doc; }

    const QVector<PTTransitionPreset>& sweepPresets() const { return m_sweepPresets; }
    const QVector<PTTransitionPreset>& continuousPresets() const { return m_continuousPresets; }
    const QVector<PTTransitionPreset>& multiFxPresets() const { return m_multiFxPresets; }

    int transitionPresetCount(PTTransitionMode mode) const override;
    PTTransitionPreset transitionPreset(PTTransitionMode mode, int index) const override;
    PTTransitionPreset effectiveTransitionPreset(PTTransitionMode mode, int index) const override;
    PTTransitionPreset effectiveTransitionPresetForOutput(PTTransitionMode mode, int index,
                                                          int outputIdx) const override;
    PTTransitionPreset effectiveTransitionPresetForPoint(PTTransitionMode mode, int index,
                                                         int outputIdx,
                                                         const QLCPoint& point) const override;
    int transitionSelectionKeyForPoint(PTTransitionMode mode, int index,
                                       int outputIdx,
                                       const QLCPoint& point) const override;
    PTTransitionPreset effectiveTransitionPresetForSelection(PTTransitionMode mode, int index,
                                                             int outputIdx,
                                                             int selectionKey) const override;
    QString transitionPresetName(PTTransitionMode mode, int index) const override;
    PTTransitionMode transitionMode() const override;
    PTGlobalEffectSettings globalEffectSettings() const override { return m_globalSettings; }
    bool hasLiveColumnOverride(quint8 inputId) const override;
    void requestFlash(int tableRowIndex, int transitionPresetIndex) override;
    void promoteStagedColumnOverrides() override;
    bool crossfadeManualControlEnabled() const override;

    VCWidget* createCopy(VCWidget* parent) override;
    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;
    void editProperties() override;
    void updateFeedback() override {}

    static QString columnTitle(int col);
    QString columnTitleForCol(int col) const;
    bool linkedTableUsesPositionMode() const;

protected slots:
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;
    void slotModeChanged(Doc::Mode mode) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void slotAddPreset();
    void slotAddSelection();
    void slotRemovePreset();
    void slotDuplicatePreset();
    void slotPresetItemChanged(QTreeWidgetItem* item, int col);
    void slotPresetChanged(PTTransitionMode mode, int row, int col, int outputIdx = -1,
                           int selectionIdx = -1);
    void slotRefreshTableLink();
    void slotColumnHeaderDoubleClicked(int logicalIndex);
    void slotBankTabChanged(int index);
    void slotOpenCustomCurveEditor();
    void slotPresetContextMenuRequested(const QPoint& pos);
    void slotSelectionLayerActivatedFromGrid(int selectionIndex);

private:
    enum PresetColumn {
        ColName = 0,
        ColAxis,
        ColOffsetDir,
        ColWings,
        ColBlocks,
        ColWingsSymmetry,
        ColOffsetStep,
        ColDuration,
        ColWaveWidth,
        ColWaveShape,
        ColFadeIn,
        ColFadeOut,
        ColWaveLevel,
        ColStartOffset,
        ColPropagation,
        ColSpeedMult,
        ColPositionMotion,
        ColPositionMotionDir,
        ColPositionPanSize,
        ColPositionTiltSize,
        ColCount
    };

    struct PTTransitionPresetOverride
    {
        PTTransitionPreset values;
        QSet<int> columns;
    };

    struct PTTransitionSelection
    {
        QString name;
        QVector<QLCPoint> cells;
        PTTransitionPresetOverride overrides;
    };

    struct PTTransitionOutputLayer
    {
        PTTransitionPresetOverride all;
        QVector<PTTransitionSelection> selections;
    };

    PTTransitionMode activeBankMode() const;
    QVector<PTTransitionPreset>& presetsForMode(PTTransitionMode mode);
    const QVector<PTTransitionPreset>& presetsForMode(PTTransitionMode mode) const;
    QVector<QHash<int, PTTransitionOutputLayer>>& overridesForMode(PTTransitionMode mode);
    const QVector<QHash<int, PTTransitionOutputLayer>>& overridesForMode(PTTransitionMode mode) const;
    QTreeWidget* tableForMode(PTTransitionMode mode) const;
    QTreeView* frozenNameViewForMode(PTTransitionMode mode) const;
    PTTransitionColumnGroupBar* columnGroupBarForMode(PTTransitionMode mode) const;
    QTreeWidget* activeTable() const;

    void updateGlobalSummaryLabel();
    void updatePresetRowUiForItem(QTreeWidgetItem* item, PTTransitionMode bankMode);

    void buildUi();
    void rebuildPresetTable(PTTransitionMode mode);
    void rebuildAllPresetTables();
    void updateColumnHeaders(QTreeWidget* table);
    void syncPresetFromTable(PTTransitionMode mode, int row);
    void syncActiveBankFromTable();
    void pushSpatialEnabledToTable();
    void notifyTablePresetCacheRefresh();
    void scheduleDeferredTableLinkRefresh(int attemptsLeft = 6);
    bool editCustomCurveForPreset(PTTransitionMode mode, int row,
                                  int outputIdx = -1, int selectionIdx = -1);
    bool editPositionShapeForPreset(PTTransitionMode mode, int row,
                                    int outputIdx = -1, int selectionIdx = -1,
                                    int motionFromUi = -1);
    bool removeSelectionAt(PTTransitionMode mode, int row, int outputIdx, int selectionIndex);
    QVector<PTTransitionColumnGroupBar::Group> columnGroupsForMode(PTTransitionMode mode) const;
    void applyColumnGroupFilter(QTreeWidget* table, PTTransitionMode mode);
    void refreshColumnGroupBarForActiveTab();
    void migrateLegacyInputSources();
    void updateEffectPreview();
    int gridSpanForPreset(const PTTransitionPreset& preset) const;
    void updateOffsetStepLimitForItem(QTreeWidgetItem* item, PTTransitionMode mode);
    bool applyGlobalInput(quint8 inputId, uchar value);
    void mapColumnInput(quint8 inputId, const QString& title);
    PTTransitionPreset presetFromItem(PTTransitionMode mode, QTreeWidgetItem* item) const;
    PTTransitionPreset effectivePresetForOutputNoLive(PTTransitionMode mode, int row,
                                                      int outputIdx) const;
    PTTransitionPreset effectivePresetForSelectionNoLive(PTTransitionMode mode, int row,
                                                         int outputIdx,
                                                         int selectionIdx) const;
    int selectionIndexForPoint(PTTransitionMode mode, int row, int outputIdx,
                               const QLCPoint& point) const;
    void writePresetXml(QXmlStreamWriter* doc, const PTTransitionPreset& p,
                        const QHash<int, PTTransitionOutputLayer>& overrides) const;
    bool readPresetAttrs(PTTransitionPreset& p, const QXmlStreamAttributes& pattrs,
                         int legacySpeedMult);
    void readOutputOverride(PTTransitionPresetOverride& ov,
                            const QXmlStreamAttributes& attrs,
                            const PTTransitionPreset& base);
    void applySelectionCellsFromGrid(const QSet<QLCPoint>& cells);
    void writeOutputOverrideXml(QXmlStreamWriter* doc, int outputIdx,
                                const PTTransitionOutputLayer& layer) const;
    int linkedOutputCount() const;
    QString linkedOutputName(int outputIdx) const;
    QTreeWidgetItem* parentItemForPreset(PTTransitionMode mode, int row) const;
    QTreeWidgetItem* selectedPresetItem(QTreeWidget* table) const;
    void setColumnOverrideValue(PTTransitionPresetOverride& ov, int col,
                                const PTTransitionPreset& value);
    bool overrideColumnDiffersFromParent(const PTTransitionPreset& parent,
                                         const PTTransitionPresetOverride& ov,
                                         int col) const;
    void normalizeNoopOverridesForPreset(PTTransitionMode mode, int row);
    void normalizeNoopOverrides();
    void clearColumnOverride(PTTransitionMode mode, int row, int outputIdx, int selectionIdx,
                             int col);
    void clearAllOverridesForOutput(PTTransitionMode mode, int row, int outputIdx,
                                    int selectionIdx);
    void copyOverridesToAllOutputs(PTTransitionMode mode, int row, int outputIdx);
    void normalizeOverrideStorage();
    void refreshOverrideVisualsForPreset(PTTransitionMode mode, int row);
    void refreshOverrideVisualsForItem(PTTransitionMode mode, QTreeWidgetItem* item);
    void applySelectionRowVisuals(QTreeWidgetItem* item);
    void updateRemoveActionLabel();
    void configureFrozenNameView(PTTransitionMode mode);
    void applyPositionModeColumnVisibility(QTreeWidget* table, PTTransitionMode mode);
    void applyDefaultColumnWidths(QTreeWidget* table);
    QString columnTooltipForCol(int col) const;
    QSet<int>& expandedSetForMode(PTTransitionMode mode);
    const QSet<int>& expandedSetForMode(PTTransitionMode mode) const;
    void captureExpandedState(PTTransitionMode mode);
    PTTransitionMode modeForTable(QTreeWidget* table) const;
    QTreeWidget* tableFromFocusObject(QObject* watched) const;
    int focusColumn(QTreeWidget* table) const;
    void setFocusColumn(QTreeWidget* table, QTreeWidgetItem* item, int col);
    void activateCellForClipboard(QTreeWidget* table, QTreeWidgetItem* item, int col,
                                  Qt::KeyboardModifiers mods);
    void showParameterContextMenu(PTTransitionMode mode, QTreeWidget* table,
                                  QTreeWidgetItem* item, int col, const QPoint& globalPos);
    bool isClipboardCell(QTreeWidget* table, QTreeWidgetItem* item, int col) const;
    bool sameClipboardContext(QTreeWidgetItem* a, QTreeWidgetItem* b) const;
    QList<QTreeWidgetItem*> clipboardRowsInContext(QTreeWidget* table,
                                                   QTreeWidgetItem* contextItem) const;
    QList<PTTransitionCellKey> selectedCellsInVisualOrder(QTreeWidget* table) const;
    void clearCellSelection(QTreeWidget* table);
    void updateCellSelectionVisuals(QTreeWidget* table);
    void updateColumnFocusVisuals(QTreeWidget* table);
    QList<QTreeWidgetItem*> selectedRowsInVisualOrder(QTreeWidget* table) const;
    bool findItemForEditor(QTreeWidget* table, QWidget* editor,
                           QTreeWidgetItem** item, int* col) const;
    int clipboardColumnForPaste() const;
    void copyCells(QTreeWidget* table);
    void pasteCells(QTreeWidget* table);
    void pasteValueToPresetCell(PTTransitionMode mode, QTreeWidget* table,
                                QTreeWidgetItem* item, int col, const QString& raw);
    QVariant editorValue(QTreeWidget* table, QTreeWidgetItem* item, int col) const;
    void setEditorValue(QTreeWidget* table, QTreeWidgetItem* item, int col, const QString& raw);

    static QComboBox* makeAxisCombo(QWidget* parent);
    static QComboBox* makeOffsetDirCombo(QWidget* parent);
    static QComboBox* makeWaveShapeCombo(QWidget* parent);
    static QComboBox* makePositionMotionCombo(QWidget* parent);
    static QComboBox* makePositionMotionDirCombo(QWidget* parent);
    static QComboBox* makePropagationCombo(QWidget* parent);
    static QComboBox* makeWingsSymmetryCombo(QWidget* parent);
    static QComboBox* makeSpeedMultCombo(QWidget* parent);
    static void configureTransitionCombo(QComboBox* combo, int popupMinWidth = 72);
    static QVariant presetColumnValue(const PTTransitionPreset& preset, int col);
    static void setPresetColumnValue(PTTransitionPreset& preset, int col,
                                     const QVariant& value);
    static QString presetColumnXmlName(int col);
    static int presetColumnFromXmlName(const QString& name);

    quint32 m_targetTableId = VCWidget::invalidId();
    QVector<PTTransitionPreset> m_sweepPresets;
    QVector<PTTransitionPreset> m_continuousPresets;
    QVector<PTTransitionPreset> m_multiFxPresets;
    QVector<QHash<int, PTTransitionOutputLayer>> m_sweepOutputOverrides;
    QVector<QHash<int, PTTransitionOutputLayer>> m_continuousOutputOverrides;
    QVector<QHash<int, PTTransitionOutputLayer>> m_multiFxOutputOverrides;
    QVector<PTCustomCurveGalleryItem> m_customCurveGallery;
    QVector<PTShapeGalleryItem> m_shapeGallery;
    QHash<int, QString> m_columnGroupFilterByMode;
    QHash<QTreeWidget*, int> m_focusColumnByTable;
    QHash<QTreeWidget*, PTTransitionCellKey> m_cellSelectionAnchorByTable;
    QHash<QTreeWidget*, QSet<PTTransitionCellKey>> m_selectedCellsByTable;
    PTGlobalEffectSettings m_globalSettings;
    bool m_crossfadeManualControl = true;
    bool m_crossfadeManualInputMapped = false;
    bool m_rebuildingTable = false;
    bool m_pastingCells = false;

    mutable QMutex m_liveMutex;
    QHash<quint8, uchar> m_liveColumnOverrides;

    QVBoxLayout*  m_layout = nullptr;
    QLabel*       m_linkLabel = nullptr;
    QLabel*       m_globalSummaryLabel = nullptr;
    QCheckBox*    m_enableChk = nullptr;
    QToolBar*     m_toolbar = nullptr;
    QAction*      m_removeAction = nullptr;
    QWidget*                 m_previewRow = nullptr;
    QLabel*                  m_curveLabel = nullptr;
    PTDimmerWaveCurveWidget* m_curveWidget = nullptr;
    QLabel*                  m_positionPreviewLabel = nullptr;
    class PTPositionPathPreviewWidget* m_positionPathWidget = nullptr;
    PTSpatialFixtureGridWidget* m_spatialGridWidget = nullptr;
    QTabWidget*   m_bankTabs = nullptr;
    QTreeWidget* m_sweepTable = nullptr;
    QTreeWidget* m_continuousTable = nullptr;
    QTreeWidget* m_multiFxTable = nullptr;
    QTreeView* m_sweepNameView = nullptr;
    QTreeView* m_continuousNameView = nullptr;
    QTreeView* m_multiFxNameView = nullptr;
    PTTransitionColumnGroupBar* m_sweepColumnGroupBar = nullptr;
    PTTransitionColumnGroupBar* m_continuousColumnGroupBar = nullptr;
    PTTransitionColumnGroupBar* m_multiFxColumnGroupBar = nullptr;
    QSet<int> m_sweepExpandedPresets;
    QSet<int> m_continuousExpandedPresets;
    QSet<int> m_multiFxExpandedPresets;
};

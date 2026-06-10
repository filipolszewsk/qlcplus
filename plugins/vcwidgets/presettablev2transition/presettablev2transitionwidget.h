/*
  QLC+ VC Widget Plugin — Preset Table v2 EFX Engine (UI only, no DMX)
*/

#pragma once

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeView>
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

class Doc;
class PresetTableV2ControlIface;
class PTDimmerWaveCurveWidget;
class PTSpatialFixtureGridWidget;

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
    void configureFrozenNameView(PTTransitionMode mode);
    QSet<int>& expandedSetForMode(PTTransitionMode mode);
    const QSet<int>& expandedSetForMode(PTTransitionMode mode) const;
    void captureExpandedState(PTTransitionMode mode);
    void copySelectionToClipboard(QTreeWidget* table) const;
    void pasteClipboardToSelection(QTreeWidget* table);
    QVariant editorValue(QTreeWidget* table, QTreeWidgetItem* item, int col) const;
    void setEditorValue(QTreeWidget* table, QTreeWidgetItem* item, int col, const QString& raw);

    static QComboBox* makeAxisCombo(QWidget* parent);
    static QComboBox* makeOffsetDirCombo(QWidget* parent);
    static QComboBox* makeWaveShapeCombo(QWidget* parent);
    static QComboBox* makePropagationCombo(QWidget* parent);
    static QComboBox* makeWingsSymmetryCombo(QWidget* parent);
    static QComboBox* makeSpeedMultCombo(QWidget* parent);
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
    PTGlobalEffectSettings m_globalSettings;
    bool m_crossfadeManualControl = true;
    bool m_crossfadeManualInputMapped = false;
    bool m_rebuildingTable = false;

    mutable QMutex m_liveMutex;
    QHash<quint8, uchar> m_liveColumnOverrides;

    QVBoxLayout*  m_layout = nullptr;
    QLabel*       m_linkLabel = nullptr;
    QLabel*       m_globalSummaryLabel = nullptr;
    QCheckBox*    m_enableChk = nullptr;
    QToolBar*     m_toolbar = nullptr;
    QWidget*                 m_previewRow = nullptr;
    PTDimmerWaveCurveWidget* m_curveWidget = nullptr;
    PTSpatialFixtureGridWidget* m_spatialGridWidget = nullptr;
    QTabWidget*   m_bankTabs = nullptr;
    QTreeWidget* m_sweepTable = nullptr;
    QTreeWidget* m_continuousTable = nullptr;
    QTreeWidget* m_multiFxTable = nullptr;
    QTreeView* m_sweepNameView = nullptr;
    QTreeView* m_continuousNameView = nullptr;
    QTreeView* m_multiFxNameView = nullptr;
    QSet<int> m_sweepExpandedPresets;
    QSet<int> m_continuousExpandedPresets;
    QSet<int> m_multiFxExpandedPresets;
};

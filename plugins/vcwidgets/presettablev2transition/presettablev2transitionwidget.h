/*
  QLC+ VC Widget Plugin — Preset Table v2 EFX Engine (UI only, no DMX)
*/

#pragma once

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QToolBar>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QHash>
#include <QMutex>
#include "vcwidget.h"
#include "presettablev2transitionprovideriface.h"
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"

class Doc;
class PresetTableV2ControlIface;
class PTDimmerWaveCurveWidget;

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

    int transitionPresetCount(PTTransitionMode mode) const override;
    PTTransitionPreset transitionPreset(PTTransitionMode mode, int index) const override;
    PTTransitionPreset effectiveTransitionPreset(PTTransitionMode mode, int index) const override;
    QString transitionPresetName(PTTransitionMode mode, int index) const override;
    PTTransitionMode transitionMode() const override;
    PTGlobalEffectSettings globalEffectSettings() const override { return m_globalSettings; }
    bool hasLiveColumnOverride(quint8 inputId) const override;
    void requestFlash(int tableRowIndex, int transitionPresetIndex) override;
    void promoteStagedColumnOverrides() override;

    VCWidget* createCopy(VCWidget* parent) override;
    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;
    void editProperties() override;
    void updateFeedback() override {}

    static QString columnTitle(int col);

protected slots:
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;
    void slotModeChanged(Doc::Mode mode) override;

private slots:
    void slotAddPreset();
    void slotRemovePreset();
    void slotDuplicatePreset();
    void slotPresetCellChanged(int row, int col);
    void slotPresetChanged(PTTransitionMode mode, int row, int col);
    void slotRefreshTableLink();
    void slotColumnHeaderDoubleClicked(int logicalIndex);
    void slotBankTabChanged(int index);

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

    PTTransitionMode activeBankMode() const;
    QVector<PTTransitionPreset>& presetsForMode(PTTransitionMode mode);
    const QVector<PTTransitionPreset>& presetsForMode(PTTransitionMode mode) const;
    QTableWidget* tableForMode(PTTransitionMode mode) const;
    QTableWidget* activeTable() const;

    void updateGlobalSummaryLabel();
    void updatePresetRowUiForMode(int row, PTTransitionMode bankMode);

    void buildUi();
    void rebuildPresetTable(PTTransitionMode mode);
    void rebuildAllPresetTables();
    void updateColumnHeaders(QTableWidget* table);
    void syncPresetFromTable(PTTransitionMode mode, int row);
    void syncActiveBankFromTable();
    void pushSpatialEnabledToTable();
    void notifyTablePresetCacheRefresh();
    void migrateLegacyInputSources();
    void updateCurvePreview();
    bool applyGlobalInput(quint8 inputId, uchar value);
    void mapColumnInput(quint8 inputId, const QString& title);
    PTTransitionPreset presetFromRow(PTTransitionMode mode, int row) const;
    void writePresetXml(QXmlStreamWriter* doc, const PTTransitionPreset& p) const;
    bool readPresetAttrs(PTTransitionPreset& p, const QXmlStreamAttributes& pattrs,
                         int legacySpeedMult);

    static QComboBox* makeAxisCombo(QWidget* parent);
    static QComboBox* makeOffsetDirCombo(QWidget* parent);
    static QComboBox* makeWaveShapeCombo(QWidget* parent);
    static QComboBox* makePropagationCombo(QWidget* parent);
    static QComboBox* makeWingsSymmetryCombo(QWidget* parent);
    static QComboBox* makeSpeedMultCombo(QWidget* parent);

    quint32 m_targetTableId = VCWidget::invalidId();
    QVector<PTTransitionPreset> m_sweepPresets;
    QVector<PTTransitionPreset> m_continuousPresets;
    PTGlobalEffectSettings m_globalSettings;
    bool m_rebuildingTable = false;

    mutable QMutex m_liveMutex;
    QHash<quint8, uchar> m_liveColumnOverrides;
    QHash<quint8, uchar> m_stagedColumnOverrides;

    QVBoxLayout*  m_layout = nullptr;
    QLabel*       m_linkLabel = nullptr;
    QLabel*       m_globalSummaryLabel = nullptr;
    QCheckBox*    m_enableChk = nullptr;
    QToolBar*     m_toolbar = nullptr;
    PTDimmerWaveCurveWidget* m_curveWidget = nullptr;
    QTabWidget*   m_bankTabs = nullptr;
    QTableWidget* m_sweepTable = nullptr;
    QTableWidget* m_continuousTable = nullptr;
};

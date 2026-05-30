/*
  QLC+ VC Widget Plugin — Preset Table v2 EFX Engine
*/

#include "presettablev2transitionwidget.h"
#include "presettablev2transitionconfigdialog.h"
#include "presettablev2transitioncolumndialog.h"
#include "presettablev2controliface.h"
#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"
#include "ptdimmerwavecurvewidget.h"
#include "ptparammatrixengine.h"
#include "presettablev2vclookup.h"
#include "ptefxinputids.h"

#include "virtualconsole.h"
#include "doc.h"

#include <QDialog>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QSpinBox>

static const QString KXMLRoot = QStringLiteral("PluginWidget");
static const QString KXMLPluginId = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal = QStringLiteral("org.qlcplus.vcwidgets.presettablev2transition");
static const QString KXMLTargetTable = QStringLiteral("TargetTableId");
static const QString KXMLTransitionMode = QStringLiteral("TransitionMode");
static const QString KXMLPreset = QStringLiteral("TransitionPreset");
static const QString KXMLSweepPresets = QStringLiteral("SweepPresets");
static const QString KXMLContinuousPresets = QStringLiteral("ContinuousPresets");
static const QString KXMLPresetName = QStringLiteral("Name");
static const QString KXMLPresetAxis = QStringLiteral("Axis");
static const QString KXMLPresetOffsetDir = QStringLiteral("OffsetDir");
static const QString KXMLPresetDir = QStringLiteral("Direction");
static const QString KXMLPresetOffsetStep = QStringLiteral("OffsetStep");
static const QString KXMLPresetBlocks = QStringLiteral("Blocks");
static const QString KXMLPresetWingsSymmetry = QStringLiteral("WingsSymmetry");
static const QString KXMLPresetWings = QStringLiteral("Wings");
static const QString KXMLPresetDuration = QStringLiteral("DurationMs");
static const QString KXMLPresetMinMs = QStringLiteral("MinMs");
static const QString KXMLPresetMaxMs = QStringLiteral("MaxMs");
static const QString KXMLPresetWaveWidth = QStringLiteral("WaveWidth");
static const QString KXMLPresetLength = QStringLiteral("Length");
static const QString KXMLPresetWaveShape = QStringLiteral("WaveShape");
static const QString KXMLPresetFadeIn = QStringLiteral("FadeIn");
static const QString KXMLPresetFadeOut = QStringLiteral("FadeOut");
static const QString KXMLPresetWaveLevel = QStringLiteral("WaveLevel");
static const QString KXMLPresetStartOffset = QStringLiteral("StartOffset");
static const QString KXMLPresetPhase = QStringLiteral("Phase");
static const QString KXMLPresetPropagation = QStringLiteral("Propagation");
static const QString KXMLPresetPlaybackMode = QStringLiteral("PlaybackMode");
static const QString KXMLPresetSpeedMult = QStringLiteral("SpeedMult");
static const QString KXMLGlobalSpeedInput = QStringLiteral("GlobalSpeedInput");
static const QString KXMLGlobalIntensityInput = QStringLiteral("GlobalIntensityInput");
static const QString KXMLEfxColumnInput = QStringLiteral("EfxColumnInput");
static const QString KXMLInputIdAttr = QStringLiteral("InputId");
static const QString KXMLGlobalMinMsRoot = QStringLiteral("GlobalMinMs");
static const QString KXMLGlobalMaxMsRoot = QStringLiteral("GlobalMaxMs");
static const QString KXMLGlobal = QStringLiteral("GlobalEffect");
static const QString KXMLGlobalSpeed = QStringLiteral("Speed");
static const QString KXMLGlobalMinMs = QStringLiteral("MinMs");
static const QString KXMLGlobalMaxMs = QStringLiteral("MaxMs");
static const QString KXMLGlobalMultiplier = QStringLiteral("Multiplier");
static const QString KXMLGlobalDirection = QStringLiteral("Direction");
static const QString KXMLGlobalIntensity = QStringLiteral("Intensity");
static const QString KXMLGlobalBlocks = QStringLiteral("Blocks");
static const QString KXMLGlobalPhase = QStringLiteral("Phase");
static const QString KXMLGlobalSymmetry = QStringLiteral("Symmetry");
static const QString KXMLGlobalOrientation = QStringLiteral("Orientation");
static const QString KXMLGlobalFxMultiplier = QStringLiteral("FxMultiplier");

static PTTransitionPreset defaultPreset(int index, PTTransitionMode bankMode)
{
    PTTransitionPreset p;
    p.name = QObject::tr("Preset %1").arg(index + 1);
    p.enabled = true;
    p.playbackMode = (bankMode == PTTransitionMode::Continuous)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    return p;
}

QString PresetTableV2TransitionWidget::columnTitle(int col)
{
    switch (col)
    {
        case ColAxis:          return QStringLiteral("Axis");
        case ColOffsetDir:     return QObject::tr("Offset dir");
        case ColWings:         return QObject::tr("Wings");
        case ColBlocks:        return QObject::tr("Blocks");
        case ColWingsSymmetry: return QObject::tr("Wings sym.");
        case ColOffsetStep:    return QObject::tr("Offset step");
        case ColDuration:      return QObject::tr("Duration ms");
        case ColWaveWidth:     return QObject::tr("Wave width");
        case ColWaveShape:     return QObject::tr("Wave shape");
        case ColFadeIn:        return QObject::tr("Fade in %");
        case ColFadeOut:       return QObject::tr("Fade out %");
        case ColWaveLevel:     return QObject::tr("Wave level");
        case ColStartOffset:   return QObject::tr("Start offset");
        case ColPropagation:   return QObject::tr("Propagation");
        case ColSpeedMult:     return QObject::tr("Mult.");
        default:               return QObject::tr("Name");
    }
}

PresetTableV2TransitionWidget::PresetTableV2TransitionWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setCaption(tr("EFX Engine"));
    setMinimumSize(480, 280);
    setFrameStyle(KVCFrameStyleSunken);

    if (m_sweepPresets.isEmpty())
        m_sweepPresets.append(defaultPreset(0, PTTransitionMode::SweepOnly));
    if (m_continuousPresets.isEmpty())
        m_continuousPresets.append(defaultPreset(0, PTTransitionMode::Continuous));

    buildUi();
    rebuildAllPresetTables();
    updateGlobalSummaryLabel();
    slotRefreshTableLink();
}

PresetTableV2TransitionWidget::~PresetTableV2TransitionWidget() = default;

PTTransitionMode PresetTableV2TransitionWidget::activeBankMode() const
{
    if (!m_bankTabs || m_bankTabs->currentIndex() <= 0)
        return PTTransitionMode::SweepOnly;
    return PTTransitionMode::Continuous;
}

QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::presetsForMode(PTTransitionMode mode)
{
    return (mode == PTTransitionMode::Continuous) ? m_continuousPresets : m_sweepPresets;
}

const QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::presetsForMode(PTTransitionMode mode) const
{
    return (mode == PTTransitionMode::Continuous) ? m_continuousPresets : m_sweepPresets;
}

QTableWidget* PresetTableV2TransitionWidget::tableForMode(PTTransitionMode mode) const
{
    return (mode == PTTransitionMode::Continuous) ? m_continuousTable : m_sweepTable;
}

QTableWidget* PresetTableV2TransitionWidget::activeTable() const
{
    return tableForMode(activeBankMode());
}

QComboBox* PresetTableV2TransitionWidget::makeAxisCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QStringLiteral("X"), int(PTTransitionAxis::X));
    c->addItem(QStringLiteral("Y"), int(PTTransitionAxis::Y));
    c->addItem(QStringLiteral("XY"), int(PTTransitionAxis::XY));
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeOffsetDirCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QStringLiteral("LR"), int(PTOffsetDirection::LeftToRight));
    c->addItem(QStringLiteral("RL"), int(PTOffsetDirection::RightToLeft));
    c->addItem(QStringLiteral("IN"), int(PTOffsetDirection::CenterToSides));
    c->addItem(QStringLiteral("OUT"), int(PTOffsetDirection::SidesToCenter));
    c->addItem(QStringLiteral("ALT"), int(PTOffsetDirection::Alternate));
    c->addItem(QStringLiteral("SYM"), int(PTOffsetDirection::Symmetric));
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeWaveShapeCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QStringLiteral("Sine"), 0);
    c->addItem(QStringLiteral("Square"), 1);
    c->addItem(QStringLiteral("Triangle"), 2);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makePropagationCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Parallel"), int(PTPropagationMode::Parallel));
    c->addItem(QObject::tr("Serial"), int(PTPropagationMode::Serial));
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeWingsSymmetryCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Normal"), 0);
    c->addItem(QObject::tr("Alternate"), 1);
    c->addItem(QObject::tr("Mirror"), 2);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeSpeedMultCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QStringLiteral("0.5x"), 0);
    c->addItem(QStringLiteral("1.0x"), 1);
    c->addItem(QStringLiteral("2.0x"), 2);
    c->addItem(QStringLiteral("3.0x"), 3);
    c->addItem(QStringLiteral("4.0x"), 4);
    c->addItem(QStringLiteral("5.0x"), 5);
    return c;
}

PTTransitionMode PresetTableV2TransitionWidget::transitionMode() const
{
    if (!m_enableChk || !m_enableChk->isChecked())
        return PTTransitionMode::Off;
    return activeBankMode();
}

void PresetTableV2TransitionWidget::updateGlobalSummaryLabel()
{
    if (!m_globalSummaryLabel)
        return;

    auto inputMark = [this](quint8 inputId) -> QString {
        const auto src = inputSource(inputId);
        return (src && src->isValid()) ? QStringLiteral(" *") : QString();
    };

    const QString text = tr("Speed: %1%2 | Intensity: %3%4 | Cycle: %5–%6 ms")
            .arg(m_globalSettings.speed)
            .arg(inputMark(PTEfxCol::InputGlobalSpeed))
            .arg(m_globalSettings.intensity)
            .arg(inputMark(PTEfxCol::InputGlobalIntensity))
            .arg(m_globalSettings.minDurationMs)
            .arg(m_globalSettings.maxDurationMs);
    m_globalSummaryLabel->setText(text);
    m_globalSummaryLabel->setToolTip(
            tr("Global speed, intensity and min/max cycle times — open widget properties to edit."));
}

void PresetTableV2TransitionWidget::buildUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    m_linkLabel = new QLabel(tr("No table linked"), this);
    m_linkLabel->setWordWrap(true);
    m_layout->addWidget(m_linkLabel);

    m_globalSummaryLabel = new QLabel(this);
    m_globalSummaryLabel->setWordWrap(true);
    m_layout->addWidget(m_globalSummaryLabel);

    m_enableChk = new QCheckBox(tr("Enable EFX"), this);
    m_layout->addWidget(m_enableChk);

    m_toolbar = new QToolBar(this);
    m_toolbar->addAction(tr("Add preset"), this, &PresetTableV2TransitionWidget::slotAddPreset);
    m_toolbar->addAction(tr("Remove"), this, &PresetTableV2TransitionWidget::slotRemovePreset);
    m_toolbar->addAction(tr("Duplicate"), this, &PresetTableV2TransitionWidget::slotDuplicatePreset);
    m_layout->addWidget(m_toolbar);

    m_curveWidget = new PTDimmerWaveCurveWidget(this);
    m_layout->addWidget(m_curveWidget);

    m_bankTabs = new QTabWidget(this);
    m_sweepTable = new QTableWidget(m_bankTabs);
    m_continuousTable = new QTableWidget(m_bankTabs);
    for (QTableWidget* table : { m_sweepTable, m_continuousTable })
    {
        table->setColumnCount(ColCount);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(table, &QTableWidget::cellChanged,
                this, &PresetTableV2TransitionWidget::slotPresetCellChanged);
        connect(table, &QTableWidget::itemSelectionChanged,
                this, &PresetTableV2TransitionWidget::updateCurvePreview);
        connect(table->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
                this, &PresetTableV2TransitionWidget::slotColumnHeaderDoubleClicked);
    }
    m_bankTabs->addTab(m_sweepTable, tr("Sweep"));
    m_bankTabs->addTab(m_continuousTable, tr("Continuous"));
    m_layout->addWidget(m_bankTabs, 1);

    connect(m_enableChk, &QCheckBox::toggled, this, [this]() {
        pushSpatialEnabledToTable();
        notifyTablePresetCacheRefresh();
    });
    connect(m_bankTabs, &QTabWidget::currentChanged,
            this, &PresetTableV2TransitionWidget::slotBankTabChanged);
}

void PresetTableV2TransitionWidget::rebuildAllPresetTables()
{
    rebuildPresetTable(PTTransitionMode::SweepOnly);
    rebuildPresetTable(PTTransitionMode::Continuous);
}

void PresetTableV2TransitionWidget::updateColumnHeaders(QTableWidget* table)
{
    if (!table)
        return;

    for (int c = 0; c < ColCount; ++c)
    {
        QString title = (c == ColName) ? tr("Name") : columnTitle(c);
        if (PTEfxCol::hasExternalInput(c))
        {
            const auto src = inputSource(PTEfxCol::inputIdForColumn(c));
            if (src && src->isValid())
                title += QStringLiteral(" *");
        }
        QTableWidgetItem* hi = table->horizontalHeaderItem(c);
        if (!hi)
        {
            hi = new QTableWidgetItem(title);
            table->setHorizontalHeaderItem(c, hi);
        }
        else
            hi->setText(title);
    }
}

void PresetTableV2TransitionWidget::rebuildPresetTable(PTTransitionMode mode)
{
    QTableWidget* table = tableForMode(mode);
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (!table)
        return;

    m_rebuildingTable = true;
    table->setRowCount(presets.size());

    QStringList headers;
    for (int c = 0; c < ColCount; ++c)
        headers << ((c == ColName) ? tr("Name") : columnTitle(c));
    table->setHorizontalHeaderLabels(headers);

    auto setComboIndex = [](QComboBox* c, int value) {
        for (int i = 0; i < c->count(); ++i)
        {
            if (c->itemData(i).toInt() == value)
            {
                c->setCurrentIndex(i);
                return;
            }
        }
    };

    for (int r = 0; r < presets.size(); ++r)
    {
        const PTTransitionPreset& p = presets[r];

        if (!table->item(r, ColName))
            table->setItem(r, ColName, new QTableWidgetItem);
        table->item(r, ColName)->setText(p.name);

        auto axisCell = [=](int col, PTTransitionAxis axis) {
            QWidget* w = table->cellWidget(r, col);
            QComboBox* c = qobject_cast<QComboBox*>(w);
            if (!c)
            {
                c = makeAxisCombo(table);
                table->setCellWidget(r, col, c);
            }
            else
                c->disconnect(this);
            connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, r](int) { slotPresetChanged(mode, r, ColAxis); });
            setComboIndex(c, int(axis));
        };

        auto offsetDirCell = [=](int col, PTOffsetDirection dir) {
            QWidget* w = table->cellWidget(r, col);
            QComboBox* c = qobject_cast<QComboBox*>(w);
            if (!c)
            {
                c = makeOffsetDirCombo(table);
                table->setCellWidget(r, col, c);
            }
            else
                c->disconnect(this);
            connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, r](int) { slotPresetChanged(mode, r, ColOffsetDir); });
            setComboIndex(c, int(dir));
        };

        auto waveShapeCell = [=](int col, int shape) {
            QWidget* w = table->cellWidget(r, col);
            QComboBox* c = qobject_cast<QComboBox*>(w);
            if (!c)
            {
                c = makeWaveShapeCombo(table);
                table->setCellWidget(r, col, c);
            }
            else
                c->disconnect(this);
            connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, r](int) { slotPresetChanged(mode, r, ColWaveShape); });
            setComboIndex(c, shape);
        };

        auto propagationCell = [=](int col, PTPropagationMode propMode) {
            QWidget* w = table->cellWidget(r, col);
            QComboBox* c = qobject_cast<QComboBox*>(w);
            if (!c)
            {
                c = makePropagationCombo(table);
                table->setCellWidget(r, col, c);
            }
            else
                c->disconnect(this);
            connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, r](int) { slotPresetChanged(mode, r, ColPropagation); });
            setComboIndex(c, int(propMode));
        };

        auto spinCell = [=](int col, int value, int minV, int maxV) {
            QWidget* w = table->cellWidget(r, col);
            QSpinBox* s = qobject_cast<QSpinBox*>(w);
            if (!s)
            {
                s = new QSpinBox(table);
                table->setCellWidget(r, col, s);
            }
            else
                s->disconnect(this);
            s->setRange(minV, maxV);
            connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this,
                    [this, mode, r, col](int) { slotPresetChanged(mode, r, col); });
            s->setValue(value);
        };

        axisCell(ColAxis, p.axis);
        offsetDirCell(ColOffsetDir, p.offsetDirection);
        spinCell(ColWings, p.wings, 1, 64);
        spinCell(ColBlocks, p.blocks, 1, 64);

        auto wingsSymmetryCell = [=](int col, int symmetry) {
            QWidget* w = table->cellWidget(r, col);
            QComboBox* c = qobject_cast<QComboBox*>(w);
            if (!c)
            {
                c = makeWingsSymmetryCombo(table);
                table->setCellWidget(r, col, c);
            }
            else
                c->disconnect(this);
            connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, r](int) { slotPresetChanged(mode, r, ColWingsSymmetry); });
            setComboIndex(c, qBound(0, symmetry, 2));
        };
        wingsSymmetryCell(ColWingsSymmetry, p.wingsSymmetry);

        spinCell(ColOffsetStep, p.offsetStep, 1, 360);
        spinCell(ColDuration, int(p.durationMs), 20, 60000);
        spinCell(ColWaveWidth, p.waveWidth, 1, 360);
        waveShapeCell(ColWaveShape, p.waveShape);
        spinCell(ColFadeIn, p.waveFadeIn, 0, 100);
        spinCell(ColFadeOut, p.waveFadeOut, 0, 100);
        spinCell(ColWaveLevel, p.waveLevel, 0, 255);
        spinCell(ColStartOffset, p.startOffset, 0, 360);
        propagationCell(ColPropagation, p.propagation);

        auto speedMultCell = [=](int col, int mult) {
            QWidget* w = table->cellWidget(r, col);
            QComboBox* c = qobject_cast<QComboBox*>(w);
            if (!c)
            {
                c = makeSpeedMultCombo(table);
                table->setCellWidget(r, col, c);
            }
            else
                c->disconnect(this);
            connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, r](int) { slotPresetChanged(mode, r, ColSpeedMult); });
            setComboIndex(c, qBound(0, mult, 5));
        };
        speedMultCell(ColSpeedMult, p.speedMultiplier);
        updatePresetRowUiForMode(r, mode);
    }

    updateColumnHeaders(table);
    table->setColumnHidden(ColDuration, true);
    m_rebuildingTable = false;
    updateCurvePreview();
}

PTTransitionPreset PresetTableV2TransitionWidget::presetFromRow(PTTransitionMode mode, int row) const
{
    PTTransitionPreset p;
    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QTableWidget* table = tableForMode(mode);
    if (row < 0 || row >= presets.size() || !table)
        return p;

    p = presets[row];
    p.playbackMode = (mode == PTTransitionMode::Continuous)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;

    if (QTableWidgetItem* nameItem = table->item(row, ColName))
        p.name = nameItem->text();

    if (auto* axis = qobject_cast<QComboBox*>(table->cellWidget(row, ColAxis)))
        p.axis = PTTransitionAxis(axis->currentData().toInt());
    if (auto* dir = qobject_cast<QComboBox*>(table->cellWidget(row, ColOffsetDir)))
        p.offsetDirection = PTOffsetDirection(dir->currentData().toInt());
    if (auto* wings = qobject_cast<QSpinBox*>(table->cellWidget(row, ColWings)))
        p.wings = wings->value();
    if (auto* blocks = qobject_cast<QSpinBox*>(table->cellWidget(row, ColBlocks)))
        p.blocks = blocks->value();
    if (auto* sym = qobject_cast<QComboBox*>(table->cellWidget(row, ColWingsSymmetry)))
        p.wingsSymmetry = sym->currentData().toInt();
    if (auto* step = qobject_cast<QSpinBox*>(table->cellWidget(row, ColOffsetStep)))
        p.offsetStep = step->value();
    if (auto* dur = qobject_cast<QSpinBox*>(table->cellWidget(row, ColDuration)))
        p.durationMs = quint32(dur->value());
    if (auto* ww = qobject_cast<QSpinBox*>(table->cellWidget(row, ColWaveWidth)))
        p.waveWidth = ww->value();
    if (auto* ws = qobject_cast<QComboBox*>(table->cellWidget(row, ColWaveShape)))
        p.waveShape = ws->currentData().toInt();
    if (auto* fi = qobject_cast<QSpinBox*>(table->cellWidget(row, ColFadeIn)))
        p.waveFadeIn = fi->value();
    if (auto* fo = qobject_cast<QSpinBox*>(table->cellWidget(row, ColFadeOut)))
        p.waveFadeOut = fo->value();
    if (auto* wl = qobject_cast<QSpinBox*>(table->cellWidget(row, ColWaveLevel)))
        p.waveLevel = wl->value();
    if (auto* so = qobject_cast<QSpinBox*>(table->cellWidget(row, ColStartOffset)))
        p.startOffset = so->value();
    if (auto* prop = qobject_cast<QComboBox*>(table->cellWidget(row, ColPropagation)))
        p.propagation = PTPropagationMode(prop->currentData().toInt());
    if (auto* mult = qobject_cast<QComboBox*>(table->cellWidget(row, ColSpeedMult)))
        p.speedMultiplier = mult->currentData().toInt();

    p.enabled = true;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    return p;
}

void PresetTableV2TransitionWidget::updatePresetRowUiForMode(int row, PTTransitionMode bankMode)
{
    QTableWidget* table = tableForMode(bankMode);
    if (!table || row < 0 || row >= table->rowCount())
        return;

    const bool sweep = (bankMode != PTTransitionMode::Continuous);

    auto tuneSpin = [&](int col, bool enabled, int minV, int maxV, int value) {
        QSpinBox* s = qobject_cast<QSpinBox*>(table->cellWidget(row, col));
        if (!s)
            return;
        const QSignalBlocker block(s);
        s->setEnabled(enabled);
        s->setRange(minV, maxV);
        s->setValue(qBound(minV, value, maxV));
    };

    int waveWidth = 180;
    int waveLevel = 255;
    int fadeIn = 25;
    int fadeOut = 25;
    if (auto* ww = qobject_cast<QSpinBox*>(table->cellWidget(row, ColWaveWidth)))
        waveWidth = ww->value();
    if (auto* wl = qobject_cast<QSpinBox*>(table->cellWidget(row, ColWaveLevel)))
        waveLevel = wl->value();
    if (auto* fi = qobject_cast<QSpinBox*>(table->cellWidget(row, ColFadeIn)))
        fadeIn = fi->value();
    if (auto* fo = qobject_cast<QSpinBox*>(table->cellWidget(row, ColFadeOut)))
        fadeOut = fo->value();

    if (sweep)
    {
        tuneSpin(ColWaveWidth, false, 360, 360, 360);
        tuneSpin(ColWaveLevel, false, 255, 255, 255);
        tuneSpin(ColFadeIn, true, 0, 50, qMin(fadeIn, 50));
        tuneSpin(ColFadeOut, true, 0, 50, qMin(fadeOut, 50));
    }
    else
    {
        tuneSpin(ColWaveWidth, true, 1, 360, waveWidth);
        tuneSpin(ColWaveLevel, true, 0, 255, waveLevel);
        tuneSpin(ColFadeIn, true, 0, 100, fadeIn);
        tuneSpin(ColFadeOut, true, 0, 100, fadeOut);
    }
}

void PresetTableV2TransitionWidget::syncPresetFromTable(PTTransitionMode mode, int row)
{
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return;
    presets[row] = presetFromRow(mode, row);
}

void PresetTableV2TransitionWidget::syncActiveBankFromTable()
{
    const PTTransitionMode mode = activeBankMode();
    QTableWidget* table = tableForMode(mode);
    if (!table)
        return;
    for (int r = 0; r < table->rowCount(); ++r)
        syncPresetFromTable(mode, r);
}

void PresetTableV2TransitionWidget::slotPresetChanged(PTTransitionMode mode, int row, int col)
{
    Q_UNUSED(col);

    if (m_rebuildingTable)
        return;

    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return;

    updatePresetRowUiForMode(row, mode);
    syncPresetFromTable(mode, row);
    updatePresetRowUiForMode(row, mode);
    notifyTablePresetCacheRefresh();
    updateCurvePreview();
}

void PresetTableV2TransitionWidget::slotPresetCellChanged(int row, int col)
{
    if (m_rebuildingTable)
        return;

    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    PTTransitionMode mode = PTTransitionMode::SweepOnly;
    if (table == m_continuousTable)
        mode = PTTransitionMode::Continuous;
    else if (table != m_sweepTable)
        return;

    slotPresetChanged(mode, row, col);
}

void PresetTableV2TransitionWidget::slotBankTabChanged(int)
{
    if (m_sweepTable)
    {
        for (int r = 0; r < m_sweepTable->rowCount(); ++r)
            syncPresetFromTable(PTTransitionMode::SweepOnly, r);
    }
    if (m_continuousTable)
    {
        for (int r = 0; r < m_continuousTable->rowCount(); ++r)
            syncPresetFromTable(PTTransitionMode::Continuous, r);
    }
    notifyTablePresetCacheRefresh();
    updateCurvePreview();
}

void PresetTableV2TransitionWidget::mapColumnInput(quint8 inputId, const QString& title)
{
    PresetTableV2TransitionColumnDialog dlg(m_doc, title, inputSource(inputId), page(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    setInputSource(dlg.inputSource(), inputId);
    rebuildAllPresetTables();
    m_doc->setModified();
}

void PresetTableV2TransitionWidget::slotColumnHeaderDoubleClicked(int logicalIndex)
{
    if (mode() != Doc::Design)
        return;
    if (!PTEfxCol::hasExternalInput(logicalIndex))
        return;

    const quint8 inputId = PTEfxCol::inputIdForColumn(logicalIndex);
    mapColumnInput(inputId, columnTitle(logicalIndex));
}

bool PresetTableV2TransitionWidget::hasLiveColumnOverride(quint8 inputId) const
{
    QMutexLocker lk(&m_liveMutex);
    return m_liveColumnOverrides.contains(inputId);
}

void PresetTableV2TransitionWidget::promoteStagedColumnOverrides()
{
    QMutexLocker lk(&m_liveMutex);
    for (auto it = m_stagedColumnOverrides.constBegin();
         it != m_stagedColumnOverrides.constEnd(); ++it)
        m_liveColumnOverrides.insert(it.key(), it.value());
    m_stagedColumnOverrides.clear();
    lk.unlock();
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::migrateLegacyInputSources()
{
    const bool preBlocksLayout = m_inputs.contains(4)
            && !m_inputs.contains(PTEfxCol::InputBlocks)
            && !m_inputs.contains(PTEfxCol::InputOffsetStep);

    static const quint8 preBlocksMap[13] = {
        0,
        PTEfxCol::InputAxis,
        PTEfxCol::InputOffsetDir,
        PTEfxCol::InputWings,
        PTEfxCol::InputOffsetStep,
        PTEfxCol::InputDuration,
        PTEfxCol::InputWaveWidth,
        PTEfxCol::InputWaveShape,
        PTEfxCol::InputFadeIn,
        PTEfxCol::InputFadeOut,
        PTEfxCol::InputWaveLevel,
        PTEfxCol::InputStartOffset,
        PTEfxCol::InputPropagation
    };

    const QList<quint8> keys = m_inputs.keys();
    for (quint8 key : keys)
    {
        if (PTEfxCol::isStableInputId(key))
            continue;

        quint8 stable = 0;
        if (preBlocksLayout && key >= 1 && key <= 12)
            stable = preBlocksMap[key];
        else if (key >= 1 && key < ColCount)
            stable = PTEfxCol::inputIdForColumn(int(key));

        if (stable == 0 || stable == key)
            continue;

        const QSharedPointer<QLCInputSource> src = m_inputs.take(key);
        if (!src.isNull() && !m_inputs.contains(stable))
            m_inputs.insert(stable, src);
    }

    if (m_inputs.contains(PTEfxCol::InputGlobalDirection)
            && !m_inputs.contains(PTEfxCol::InputOffsetDir))
    {
        m_inputs.insert(PTEfxCol::InputOffsetDir, m_inputs.take(PTEfxCol::InputGlobalDirection));
    }
}

bool PresetTableV2TransitionWidget::applyGlobalInput(quint8 inputId, uchar value)
{
    switch (inputId)
    {
        case PTEfxCol::InputGlobalSpeed:
            m_globalSettings.speed = value;
            updateGlobalSummaryLabel();
            return true;
        case PTEfxCol::InputGlobalIntensity:
            m_globalSettings.intensity = value;
            updateGlobalSummaryLabel();
            return true;
        default:
            return false;
    }
}

void PresetTableV2TransitionWidget::updateCurvePreview()
{
    if (!m_curveWidget)
        return;

    const PTTransitionMode mode = activeBankMode();
    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (presets.isEmpty())
        return;

    QTableWidget* table = activeTable();
    int row = table ? table->currentRow() : 0;
    if (row < 0)
        row = 0;
    if (row >= presets.size())
        row = presets.size() - 1;

    const PTTransitionPreset preset = presetFromRow(mode, row);
    const PTDimmerWaveParams params = PTDimmerWaveEngine::paramsFromPreset(preset, &m_globalSettings);
    const quint32 cycleMs = PTParamMatrixEngine::effectiveDurationMs(m_globalSettings, preset, false);
    m_curveWidget->setParams(params);
    m_curveWidget->setCycleDurationMs(cycleMs);
}

void PresetTableV2TransitionWidget::slotAddPreset()
{
    PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    presets.append(defaultPreset(presets.size(), mode));
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::slotRemovePreset()
{
    const PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QTableWidget* table = tableForMode(mode);
    const int row = table ? table->currentRow() : -1;
    if (row < 0 || presets.size() <= 1)
        return;
    presets.removeAt(row);
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::slotDuplicatePreset()
{
    const PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QTableWidget* table = tableForMode(mode);
    const int row = table ? table->currentRow() : -1;
    if (row < 0)
        return;
    syncPresetFromTable(mode, row);
    PTTransitionPreset copy = presets[row];
    copy.name += tr(" copy");
    presets.append(copy);
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::pushSpatialEnabledToTable()
{
    if (PresetTableV2ControlIface* table = linkedTable())
        table->setSpatialEffectsEnabled(m_enableChk->isChecked());
}

void PresetTableV2TransitionWidget::notifyTablePresetCacheRefresh()
{
    if (PresetTableV2ControlIface* table = linkedTable())
        table->refreshTransitionPresetCache();
}

quint32 PresetTableV2TransitionWidget::targetTableId() const
{
    return m_targetTableId;
}

void PresetTableV2TransitionWidget::setTargetTableId(quint32 id)
{
    m_targetTableId = id;
    slotRefreshTableLink();
}

PresetTableV2ControlIface* PresetTableV2TransitionWidget::linkedTable() const
{
    if (m_targetTableId == VCWidget::invalidId())
        return nullptr;
    VirtualConsole* vc = VirtualConsole::instance();
    if (!vc)
        return nullptr;
    return qobject_cast<PresetTableV2ControlIface*>(vc->widget(m_targetTableId));
}

void PresetTableV2TransitionWidget::slotRefreshTableLink()
{
    PresetTableV2ControlIface* table = linkedTable();
    if (table)
    {
        VCWidget* w = qobject_cast<VCWidget*>(VirtualConsole::instance()->widget(m_targetTableId));
        m_linkLabel->setText(tr("Linked: %1").arg(w ? PresetTableV2VCLookup::vcWidgetLabel(w)
                                                      : QString::number(m_targetTableId)));

        m_enableChk->blockSignals(true);
        m_enableChk->setChecked(table->spatialEffectsEnabled());
        m_enableChk->blockSignals(false);
    }
    else if (m_targetTableId != VCWidget::invalidId())
        m_linkLabel->setText(tr("Table #%1 not found").arg(m_targetTableId));
    else
        m_linkLabel->setText(tr("No table — double-click column headers to map inputs"));
}

int PresetTableV2TransitionWidget::transitionPresetCount(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::Continuous)
        return m_continuousPresets.size();
    if (mode == PTTransitionMode::SweepOnly)
        return m_sweepPresets.size();
    return 0;
}

PTTransitionPreset PresetTableV2TransitionWidget::transitionPreset(PTTransitionMode mode, int index) const
{
    const QVector<PTTransitionPreset>& bank = presetsForMode(mode);
    if (index < 0 || index >= bank.size())
        return PTTransitionPreset();
    return bank.at(index);
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveTransitionPreset(PTTransitionMode mode,
                                                                            int index) const
{
    QHash<quint8, uchar> live;
    {
        QMutexLocker lk(&m_liveMutex);
        live = m_liveColumnOverrides;
    }
    PTTransitionPreset p = PresetTableV2SpatialEngine::mergePreset(transitionPreset(mode, index), live);
    p.playbackMode = (mode == PTTransitionMode::Continuous)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    return p;
}

QString PresetTableV2TransitionWidget::transitionPresetName(PTTransitionMode mode, int index) const
{
    const QVector<PTTransitionPreset>& bank = presetsForMode(mode);
    if (index < 0 || index >= bank.size())
        return QString();
    const QString n = bank.at(index).name;
    return n.isEmpty() ? tr("Preset %1").arg(index + 1) : n;
}

void PresetTableV2TransitionWidget::requestFlash(int tableRowIndex, int transitionPresetIndex)
{
    if (PresetTableV2ControlIface* table = linkedTable())
        table->requestTableFlash(tableRowIndex, transitionPresetIndex);
}

void PresetTableV2TransitionWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (!acceptsInput())
        return;

    const quint32 pagedCh = (page() << 16) | channel;

    static const quint8 kAllInputs[] = {
        PTEfxCol::InputAxis,
        PTEfxCol::InputOffsetDir,
        PTEfxCol::InputWings,
        PTEfxCol::InputBlocks,
        PTEfxCol::InputWingsSymmetry,
        PTEfxCol::InputOffsetStep,
        PTEfxCol::InputDuration,
        PTEfxCol::InputWaveWidth,
        PTEfxCol::InputWaveShape,
        PTEfxCol::InputFadeIn,
        PTEfxCol::InputFadeOut,
        PTEfxCol::InputWaveLevel,
        PTEfxCol::InputStartOffset,
        PTEfxCol::InputPropagation,
        PTEfxCol::InputSpeedMult,
        PTEfxCol::InputGlobalSpeed,
        PTEfxCol::InputGlobalIntensity
    };

    for (quint8 inputId : kAllInputs)
    {
        if (!checkInputSource(universe, pagedCh, value, sender(), inputId))
            continue;

        if (applyGlobalInput(inputId, value))
        {
            notifyTablePresetCacheRefresh();
            sendFeedback(value, inputId);
            return;
        }

        const bool stagedEditing = [this]() {
            if (PresetTableV2ControlIface* table = linkedTable())
                return table->continuousCrossfadeStagedEditing();
            return false;
        }();

        QMutexLocker lk(&m_liveMutex);
        if (stagedEditing)
            m_stagedColumnOverrides.insert(inputId, value);
        else
            m_liveColumnOverrides.insert(inputId, value);
        lk.unlock();
        notifyTablePresetCacheRefresh();
        sendFeedback(value, inputId);
        return;
    }
}

void PresetTableV2TransitionWidget::slotModeChanged(Doc::Mode mode)
{
    VCWidget::slotModeChanged(mode);
    const bool enabled = (mode == Doc::Design || mode == Doc::Operate);
    m_enableChk->setEnabled(enabled);
    if (m_sweepTable)
        m_sweepTable->setEnabled(enabled);
    if (m_continuousTable)
        m_continuousTable->setEnabled(enabled);
    if (m_toolbar)
        m_toolbar->setEnabled(enabled);
    if (m_bankTabs)
        m_bankTabs->setEnabled(enabled);
    slotRefreshTableLink();
}

void PresetTableV2TransitionWidget::editProperties()
{
    PresetTableV2TransitionConfigDialog dlg(this, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    setCaption(dlg.widgetCaption());
    setTargetTableId(dlg.targetTableId());

    const PTGlobalEffectSettings gs = dlg.globalSettings();
    m_globalSettings.speed = gs.speed;
    m_globalSettings.intensity = gs.intensity;
    m_globalSettings.minDurationMs = gs.minDurationMs;
    m_globalSettings.maxDurationMs = gs.maxDurationMs;

    setInputSource(dlg.globalSpeedInputSource(), PTEfxCol::InputGlobalSpeed);
    setInputSource(dlg.globalIntensityInputSource(), PTEfxCol::InputGlobalIntensity);
    updateGlobalSummaryLabel();
    updateCurvePreview();

    if (PresetTableV2ControlIface* table = linkedTable())
    {
        table->setLinkedTransitionWidgetId(id());
        table->refreshTransitionPresetCache();
    }

    pushSpatialEnabledToTable();
    m_doc->setModified();
}

VCWidget* PresetTableV2TransitionWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    auto* copy = new PresetTableV2TransitionWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }
    copy->m_targetTableId = m_targetTableId;
    copy->m_sweepPresets = m_sweepPresets;
    copy->m_continuousPresets = m_continuousPresets;
    copy->m_globalSettings = m_globalSettings;
    copy->rebuildAllPresetTables();
    copy->updateGlobalSummaryLabel();
    for (int col = 1; col < ColCount; ++col)
    {
        const quint8 stableId = PTEfxCol::inputIdForColumn(col);
        copy->setInputSource(inputSource(stableId), stableId);
    }
    copy->setInputSource(inputSource(PTEfxCol::InputGlobalSpeed), PTEfxCol::InputGlobalSpeed);
    copy->setInputSource(inputSource(PTEfxCol::InputGlobalIntensity), PTEfxCol::InputGlobalIntensity);
    copy->slotRefreshTableLink();
    return copy;
}

bool PresetTableV2TransitionWidget::readPresetAttrs(PTTransitionPreset& p,
                                                    const QXmlStreamAttributes& pattrs,
                                                    int legacySpeedMult)
{
    p.name = pattrs.value(KXMLPresetName).toString();
    p.axis = PresetTableV2SpatialEngine::axisFromString(pattrs.value(KXMLPresetAxis).toString());
    if (pattrs.hasAttribute(KXMLPresetOffsetDir))
        p.offsetDirection = PresetTableV2SpatialEngine::offsetDirectionFromString(
                pattrs.value(KXMLPresetOffsetDir).toString());
    else if (pattrs.hasAttribute(KXMLPresetDir))
        p.offsetDirection = PresetTableV2SpatialEngine::offsetDirectionFromString(
                pattrs.value(KXMLPresetDir).toString());
    p.wings = pattrs.hasAttribute(KXMLPresetWings) ? pattrs.value(KXMLPresetWings).toInt() : 1;
    if (pattrs.hasAttribute(KXMLPresetBlocks))
        p.blocks = qMax(1, pattrs.value(KXMLPresetBlocks).toInt());
    if (pattrs.hasAttribute(KXMLPresetWingsSymmetry))
        p.wingsSymmetry = qBound(0, pattrs.value(KXMLPresetWingsSymmetry).toInt(), 2);
    if (pattrs.hasAttribute(KXMLPresetOffsetStep))
        p.offsetStep = pattrs.value(KXMLPresetOffsetStep).toInt();
    if (pattrs.hasAttribute(KXMLPresetDuration))
        p.durationMs = pattrs.value(KXMLPresetDuration).toUInt();
    else if (pattrs.hasAttribute(KXMLPresetMinMs) || pattrs.hasAttribute(KXMLPresetMaxMs))
    {
        const quint32 minMs = pattrs.hasAttribute(KXMLPresetMinMs)
                ? pattrs.value(KXMLPresetMinMs).toUInt() : 5000;
        const quint32 maxMs = pattrs.hasAttribute(KXMLPresetMaxMs)
                ? pattrs.value(KXMLPresetMaxMs).toUInt() : minMs;
        p.durationMs = (minMs + maxMs) / 2;
    }
    if (pattrs.hasAttribute(KXMLPresetWaveWidth))
        p.waveWidth = pattrs.value(KXMLPresetWaveWidth).toInt();
    else if (pattrs.hasAttribute(KXMLPresetLength))
        p.waveWidth = qBound(1, int(pattrs.value(KXMLPresetLength).toUInt()) * 360 / 255, 360);
    if (pattrs.hasAttribute(KXMLPresetWaveShape))
        p.waveShape = pattrs.value(KXMLPresetWaveShape).toInt();
    if (pattrs.hasAttribute(KXMLPresetFadeIn))
    {
        const int fi = pattrs.value(KXMLPresetFadeIn).toInt();
        p.waveFadeIn = (fi <= 100) ? fi : qBound(0, fi * 100 / 255, 100);
    }
    if (pattrs.hasAttribute(KXMLPresetFadeOut))
    {
        const int fo = pattrs.value(KXMLPresetFadeOut).toInt();
        p.waveFadeOut = (fo <= 100) ? fo : qBound(0, fo * 100 / 255, 100);
    }
    if (pattrs.hasAttribute(KXMLPresetWaveLevel))
        p.waveLevel = pattrs.value(KXMLPresetWaveLevel).toInt();
    if (pattrs.hasAttribute(KXMLPresetStartOffset))
        p.startOffset = pattrs.value(KXMLPresetStartOffset).toInt();
    else if (pattrs.hasAttribute(KXMLPresetPhase))
        p.startOffset = qBound(0, int(pattrs.value(KXMLPresetPhase).toUInt()) * 360 / 255, 360);
    if (pattrs.hasAttribute(KXMLPresetPropagation))
        p.propagation = (pattrs.value(KXMLPresetPropagation).toInt() == 0)
                ? PTPropagationMode::Parallel : PTPropagationMode::Serial;
    if (pattrs.hasAttribute(KXMLPresetPlaybackMode))
    {
        const int m = pattrs.value(KXMLPresetPlaybackMode).toInt();
        p.playbackMode = (m == int(PTTransitionMode::Continuous))
                ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    }
    if (pattrs.hasAttribute(KXMLPresetSpeedMult))
        p.speedMultiplier = qBound(0, pattrs.value(KXMLPresetSpeedMult).toInt(), 5);
    else
        p.speedMultiplier = qBound(0, legacySpeedMult, 5);

    return true;
}

void PresetTableV2TransitionWidget::writePresetXml(QXmlStreamWriter* doc, const PTTransitionPreset& p) const
{
    doc->writeStartElement(KXMLPreset);
    doc->writeAttribute(KXMLPresetName, p.name);
    doc->writeAttribute(KXMLPresetPlaybackMode, QString::number(int(p.playbackMode)));
    doc->writeAttribute(KXMLPresetAxis, PresetTableV2SpatialEngine::axisToString(p.axis));
    doc->writeAttribute(KXMLPresetOffsetDir,
                        PresetTableV2SpatialEngine::offsetDirectionToString(p.offsetDirection));
    doc->writeAttribute(KXMLPresetWings, QString::number(p.wings));
    doc->writeAttribute(KXMLPresetBlocks, QString::number(p.blocks));
    doc->writeAttribute(KXMLPresetWingsSymmetry, QString::number(p.wingsSymmetry));
    doc->writeAttribute(KXMLPresetOffsetStep, QString::number(p.offsetStep));
    doc->writeAttribute(KXMLPresetDuration, QString::number(p.durationMs));
    doc->writeAttribute(KXMLPresetWaveWidth, QString::number(p.waveWidth));
    doc->writeAttribute(KXMLPresetWaveShape, QString::number(p.waveShape));
    doc->writeAttribute(KXMLPresetFadeIn, QString::number(p.waveFadeIn));
    doc->writeAttribute(KXMLPresetFadeOut, QString::number(p.waveFadeOut));
    doc->writeAttribute(KXMLPresetWaveLevel, QString::number(p.waveLevel));
    doc->writeAttribute(KXMLPresetStartOffset, QString::number(p.startOffset));
    doc->writeAttribute(KXMLPresetPropagation, QString::number(int(p.propagation)));
    doc->writeAttribute(KXMLPresetSpeedMult, QString::number(p.speedMultiplier));
    doc->writeEndElement();
}

bool PresetTableV2TransitionWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot)
        return false;

    loadXMLCommon(root);
    const auto rootAttrs = root.attributes();
    m_targetTableId = rootAttrs.value(KXMLTargetTable).toUInt();
    if (rootAttrs.hasAttribute(KXMLGlobalMinMsRoot))
        m_globalSettings.minDurationMs = rootAttrs.value(KXMLGlobalMinMsRoot).toUInt();
    if (rootAttrs.hasAttribute(KXMLGlobalMaxMsRoot))
        m_globalSettings.maxDurationMs = rootAttrs.value(KXMLGlobalMaxMsRoot).toUInt();

    PTTransitionMode legacyGlobalMode = PTTransitionMode::SweepOnly;
    if (rootAttrs.hasAttribute(KXMLTransitionMode))
        legacyGlobalMode = PTTransitionMode(rootAttrs.value(KXMLTransitionMode).toInt());

    m_sweepPresets.clear();
    m_continuousPresets.clear();
    int legacySweepDir = 0;
    int legacySpeedMult = 1;

    auto finalizePreset = [&](PTTransitionPreset& p, PTTransitionMode bankHint,
                              const QXmlStreamAttributes& pattrs) {
        if (!pattrs.hasAttribute(KXMLPresetPlaybackMode))
        {
            if (bankHint == PTTransitionMode::Continuous)
                p.playbackMode = PTTransitionMode::Continuous;
            else if (bankHint == PTTransitionMode::SweepOnly)
                p.playbackMode = PTTransitionMode::SweepOnly;
            else if (legacyGlobalMode == PTTransitionMode::Continuous)
                p.playbackMode = PTTransitionMode::Continuous;
        }
        if (!pattrs.hasAttribute(KXMLPresetOffsetDir) && legacySweepDir > 0)
        {
            p.offsetDirection = PTParamMatrixEngine::offsetFromGlobalDirection(
                    legacySweepDir);
        }
        PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
        p.enabled = true;
    };

    auto storePreset = [&](const PTTransitionPreset& p, PTTransitionMode bankHint) {
        PTTransitionPreset stored = p;
        if (bankHint == PTTransitionMode::Continuous)
            stored.playbackMode = PTTransitionMode::Continuous;
        else if (bankHint == PTTransitionMode::SweepOnly)
            stored.playbackMode = PTTransitionMode::SweepOnly;

        if (stored.playbackMode == PTTransitionMode::Continuous)
            m_continuousPresets.append(stored);
        else
            m_sweepPresets.append(stored);
    };

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0;
            bool vis = true;
            loadXMLWindowState(root, &x, &y, &w, &h, &vis);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
            loadXMLAppearance(root);
        else if (root.name() == KXMLGlobalSpeedInput)
            loadXMLSources(root, PTEfxCol::InputGlobalSpeed);
        else if (root.name() == KXMLGlobalIntensityInput)
            loadXMLSources(root, PTEfxCol::InputGlobalIntensity);
        else if (root.name() == KXMLEfxColumnInput)
        {
            const quint8 inputId = PTEfxCol::migrateLegacyInputKey(
                    quint8(root.attributes().value(KXMLInputIdAttr).toUInt()));
            if (inputId != 0)
                loadXMLSources(root, inputId);
            else
                root.skipCurrentElement();
        }
        else if (root.name() == KXMLGlobal)
        {
            const auto gattrs = root.attributes();
            if (gattrs.hasAttribute(KXMLGlobalSpeed))
                m_globalSettings.speed = uchar(gattrs.value(KXMLGlobalSpeed).toInt());
            if (gattrs.hasAttribute(KXMLGlobalMinMs))
                m_globalSettings.minDurationMs = gattrs.value(KXMLGlobalMinMs).toUInt();
            if (gattrs.hasAttribute(KXMLGlobalMaxMs))
                m_globalSettings.maxDurationMs = gattrs.value(KXMLGlobalMaxMs).toUInt();
            if (gattrs.hasAttribute(KXMLGlobalMultiplier))
                legacySpeedMult = gattrs.value(KXMLGlobalMultiplier).toInt();
            if (gattrs.hasAttribute(KXMLGlobalDirection))
                legacySweepDir = gattrs.value(KXMLGlobalDirection).toInt();
            if (gattrs.hasAttribute(KXMLGlobalIntensity))
                m_globalSettings.intensity = uchar(gattrs.value(KXMLGlobalIntensity).toInt());
            if (gattrs.hasAttribute(KXMLGlobalBlocks))
                m_globalSettings.fxBlocks = gattrs.value(KXMLGlobalBlocks).toInt();
            if (gattrs.hasAttribute(KXMLGlobalPhase))
                m_globalSettings.fxPhaseOffset = gattrs.value(KXMLGlobalPhase).toInt();
            if (gattrs.hasAttribute(KXMLGlobalSymmetry))
                m_globalSettings.fxWingsSymmetry = gattrs.value(KXMLGlobalSymmetry).toInt();
            if (gattrs.hasAttribute(KXMLGlobalOrientation))
                m_globalSettings.fxOrientation = gattrs.value(KXMLGlobalOrientation).toInt();
            if (gattrs.hasAttribute(KXMLGlobalFxMultiplier))
                m_globalSettings.fxMultiplier = gattrs.value(KXMLGlobalFxMultiplier).toInt();
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLSweepPresets)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLPreset)
                {
                    const auto pattrs = root.attributes();
                    PTTransitionPreset p;
                    readPresetAttrs(p, pattrs, legacySpeedMult);
                    finalizePreset(p, PTTransitionMode::SweepOnly, pattrs);
                    m_sweepPresets.append(p);
                    root.skipCurrentElement();
                }
                else
                    root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLContinuousPresets)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLPreset)
                {
                    const auto pattrs = root.attributes();
                    PTTransitionPreset p;
                    readPresetAttrs(p, pattrs, legacySpeedMult);
                    finalizePreset(p, PTTransitionMode::Continuous, pattrs);
                    m_continuousPresets.append(p);
                    root.skipCurrentElement();
                }
                else
                    root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLPreset)
        {
            const auto pattrs = root.attributes();
            PTTransitionPreset p;
            readPresetAttrs(p, pattrs, legacySpeedMult);
            finalizePreset(p, PTTransitionMode::Off, pattrs);
            storePreset(p, PTTransitionMode::Off);
            root.skipCurrentElement();
        }
        else
            root.skipCurrentElement();
    }

    if (m_sweepPresets.isEmpty())
        m_sweepPresets.append(defaultPreset(0, PTTransitionMode::SweepOnly));
    if (m_continuousPresets.isEmpty())
        m_continuousPresets.append(defaultPreset(0, PTTransitionMode::Continuous));

    migrateLegacyInputSources();

    rebuildAllPresetTables();
    updateGlobalSummaryLabel();
    slotRefreshTableLink();
    notifyTablePresetCacheRefresh();
    return true;
}

bool PresetTableV2TransitionWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    if (m_sweepTable)
    {
        for (int r = 0; r < m_sweepTable->rowCount(); ++r)
            syncPresetFromTable(PTTransitionMode::SweepOnly, r);
    }
    if (m_continuousTable)
    {
        for (int r = 0; r < m_continuousTable->rowCount(); ++r)
            syncPresetFromTable(PTTransitionMode::Continuous, r);
    }

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);
    if (m_targetTableId != VCWidget::invalidId())
        doc->writeAttribute(KXMLTargetTable, QString::number(m_targetTableId));
    doc->writeAttribute(KXMLGlobalMinMsRoot, QString::number(m_globalSettings.minDurationMs));
    doc->writeAttribute(KXMLGlobalMaxMsRoot, QString::number(m_globalSettings.maxDurationMs));
    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    doc->writeStartElement(KXMLGlobal);
    doc->writeAttribute(KXMLGlobalSpeed, QString::number(m_globalSettings.speed));
    doc->writeAttribute(KXMLGlobalMinMs, QString::number(m_globalSettings.minDurationMs));
    doc->writeAttribute(KXMLGlobalMaxMs, QString::number(m_globalSettings.maxDurationMs));
    doc->writeAttribute(KXMLGlobalIntensity, QString::number(m_globalSettings.intensity));
    doc->writeAttribute(KXMLGlobalPhase, QString::number(m_globalSettings.fxPhaseOffset));
    doc->writeAttribute(KXMLGlobalSymmetry, QString::number(m_globalSettings.fxWingsSymmetry));
    doc->writeAttribute(KXMLGlobalOrientation, QString::number(m_globalSettings.fxOrientation));
    doc->writeAttribute(KXMLGlobalFxMultiplier, QString::number(m_globalSettings.fxMultiplier));
    doc->writeEndElement();

    auto saveInputBinding = [&](quint8 inputId, const QString& wrapperTag) {
        const auto src = inputSource(inputId);
        if (src.isNull() || !src->isValid())
            return;
        doc->writeStartElement(wrapperTag);
        if (wrapperTag == KXMLEfxColumnInput)
            doc->writeAttribute(KXMLInputIdAttr, QString::number(inputId));
        saveXMLInput(doc, src);
        doc->writeEndElement();
    };

    saveInputBinding(PTEfxCol::InputGlobalSpeed, KXMLGlobalSpeedInput);
    saveInputBinding(PTEfxCol::InputGlobalIntensity, KXMLGlobalIntensityInput);

    for (int col = 1; col < ColCount; ++col)
        saveInputBinding(PTEfxCol::inputIdForColumn(col), KXMLEfxColumnInput);

    doc->writeStartElement(KXMLSweepPresets);
    for (const PTTransitionPreset& p : m_sweepPresets)
        writePresetXml(doc, p);
    doc->writeEndElement();

    doc->writeStartElement(KXMLContinuousPresets);
    for (const PTTransitionPreset& p : m_continuousPresets)
        writePresetXml(doc, p);
    doc->writeEndElement();

    doc->writeEndElement();
    return true;
}

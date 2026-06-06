/*
  QLC+ VC Widget Plugin — Preset Table v2 EFX Engine
*/

#include "presettablev2transitionwidget.h"
#include "presettablev2transitionconfigdialog.h"
#include "presettablev2transitioncolumndialog.h"
#include "ptcustomcurvedialog.h"
#include "presettablev2controliface.h"
#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"
#include "ptdimmerwavecurvewidget.h"
#include "ptspatialfixturegridwidget.h"
#include "ptparammatrixengine.h"
#include "presettablev2vclookup.h"
#include "ptefxinputids.h"

#include "virtualconsole.h"
#include "doc.h"

#include <QDialog>
#include <QAbstractScrollArea>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QStringList>
#include <QMenu>
#include <QFont>
#include <QScrollBar>
#include <QTreeView>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QRegularExpression>

static const QString KXMLRoot = QStringLiteral("PluginWidget");
static const QString KXMLPluginId = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal = QStringLiteral("org.qlcplus.vcwidgets.presettablev2transition");
static const QString KXMLTargetTable = QStringLiteral("TargetTableId");
static const QString KXMLTransitionMode = QStringLiteral("TransitionMode");
static const QString KXMLPreset = QStringLiteral("TransitionPreset");
static const QString KXMLSweepPresets = QStringLiteral("SweepPresets");
static const QString KXMLContinuousPresets = QStringLiteral("ContinuousPresets");
static const QString KXMLMultiFxPresets = QStringLiteral("MultiFxPresets");
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
static const QString KXMLPresetCustomCurveEnabled = QStringLiteral("CustomCurveEnabled");
static const QString KXMLPresetCustomCurve = QStringLiteral("CustomCurve");
static const QString KXMLOutputOverride = QStringLiteral("OutputOverride");
static const QString KXMLOutputOverrideIndex = QStringLiteral("Output");
static const QString KXMLCustomCurveGallery = QStringLiteral("CustomCurveGallery");
static const QString KXMLCustomCurveItem = QStringLiteral("CustomCurveItem");
static const QString KXMLCustomCurveItemName = QStringLiteral("Name");
static const QString KXMLCustomCurveItemCurve = QStringLiteral("Curve");
static const QString KXMLGlobalSpeedInput = QStringLiteral("GlobalSpeedInput");
static const QString KXMLGlobalIntensityInput = QStringLiteral("GlobalIntensityInput");
static const QString KXMLGlobalCrossfadeManualInput = QStringLiteral("GlobalCrossfadeManualInput");
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

static const int kItemPresetIndexRole = Qt::UserRole + 1;
static const int kItemOutputIndexRole = Qt::UserRole + 2;
static const int kEfxTreeRowHeight = 24;
static const QColor kOverrideOrange(230, 126, 34);

static PTTransitionPreset defaultPreset(int index, PTTransitionMode bankMode)
{
    PTTransitionPreset p;
    p.name = QObject::tr("Preset %1").arg(index + 1);
    p.enabled = true;
    p.playbackMode = (bankMode == PTTransitionMode::Continuous || bankMode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    return p;
}

static QVector<PTCustomCurvePoint> defaultTransitionCustomCurve()
{
    QVector<PTCustomCurvePoint> pts;
    PTCustomCurvePoint a;
    a.xDeg = 0.0; a.yValue = 0.0;
    a.leftHandleXDeg = 0.0; a.leftHandleYValue = 0.0;
    a.rightHandleXDeg = 60.0; a.rightHandleYValue = 0.0;
    PTCustomCurvePoint b;
    b.xDeg = 180.0; b.yValue = 255.0;
    b.leftHandleXDeg = 120.0; b.leftHandleYValue = 255.0;
    b.rightHandleXDeg = 240.0; b.rightHandleYValue = 255.0;
    PTCustomCurvePoint c;
    c.xDeg = 360.0; c.yValue = 0.0;
    c.leftHandleXDeg = 300.0; c.leftHandleYValue = 0.0;
    c.rightHandleXDeg = 360.0; c.rightHandleYValue = 0.0;
    pts << a << b << c;
    return pts;
}

static QString serializeCustomCurve(const QVector<PTCustomCurvePoint>& points)
{
    QStringList encoded;
    for (const PTCustomCurvePoint& p : points)
    {
        encoded << QStringLiteral("%1,%2,%3,%4,%5,%6,%7")
                .arg(p.xDeg, 0, 'f', 2)
                .arg(p.yValue, 0, 'f', 2)
                .arg(p.leftHandleXDeg, 0, 'f', 2)
                .arg(p.leftHandleYValue, 0, 'f', 2)
                .arg(p.rightHandleXDeg, 0, 'f', 2)
                .arg(p.rightHandleYValue, 0, 'f', 2)
                .arg(qBound(0, p.segmentMode, 1));
    }
    return encoded.join(QLatin1Char(';'));
}

static QVector<PTCustomCurvePoint> parseCustomCurve(const QString& text)
{
    QVector<PTCustomCurvePoint> points;
    const QStringList encodedPoints = text.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString& encoded : encodedPoints)
    {
        const QStringList values = encoded.split(QLatin1Char(','));
        if (values.size() != 6 && values.size() != 7)
            continue;
        PTCustomCurvePoint p;
        p.xDeg = values.at(0).toDouble();
        p.yValue = values.at(1).toDouble();
        p.leftHandleXDeg = values.at(2).toDouble();
        p.leftHandleYValue = values.at(3).toDouble();
        p.rightHandleXDeg = values.at(4).toDouble();
        p.rightHandleYValue = values.at(5).toDouble();
        p.segmentMode = values.size() >= 7 ? qBound(0, values.at(6).toInt(), 1)
                                           : PTCustomCurvePoint::Bezier;
        points.append(p);
    }
    return points;
}

static void setComboDataIndex(QComboBox* combo, int value)
{
    if (!combo)
        return;
    for (int i = 0; i < combo->count(); ++i)
    {
        if (combo->itemData(i).toInt() == value)
        {
            combo->setCurrentIndex(i);
            return;
        }
    }
}

static void styleTransitionEditorWidget(QWidget* widget, bool inherited,
                                        bool parentHasOutputOverride = false)
{
    if (!widget)
        return;

    QFont f = widget->font();
    f.setItalic(inherited);
    f.setBold(false);
    widget->setFont(f);

    if (parentHasOutputOverride)
    {
        widget->setStyleSheet(QStringLiteral("color: rgb(%1, %2, %3);")
                                      .arg(kOverrideOrange.red())
                                      .arg(kOverrideOrange.green())
                                      .arg(kOverrideOrange.blue()));
        widget->setToolTip(QObject::tr("One or more outputs override this parameter"));
    }
    else
    {
        widget->setStyleSheet(QStringLiteral("color: palette(text);"));
        widget->setToolTip(inherited ? QObject::tr("Inherited from parent preset")
                                     : QObject::tr("Override"));
    }
}

static void applyPersistentScrollBars(QAbstractScrollArea* area)
{
    if (!area)
        return;

    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setStyleSheet(QStringLiteral(
        "QTreeView::item { min-height: %1px; }"
        "QScrollBar:vertical {"
        "  width: 14px;"
        "  margin: 0px;"
        "  background: palette(base);"
        "}"
        "QScrollBar:horizontal {"
        "  height: 14px;"
        "  margin: 0px;"
        "  background: palette(base);"
        "}"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background: rgb(115, 115, 115);"
        "  border-radius: 6px;"
        "  min-height: 28px;"
        "  min-width: 28px;"
        "}"
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
        "  background: rgb(145, 145, 145);"
        "}"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        "  width: 0px;"
        "  height: 0px;"
        "}"
        "QScrollBar::add-page, QScrollBar::sub-page {"
        "  background: transparent;"
        "}").arg(kEfxTreeRowHeight));
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
    if (m_multiFxPresets.isEmpty())
        m_multiFxPresets.append(defaultPreset(0, PTTransitionMode::MultiFx));

    buildUi();
    rebuildAllPresetTables();
    updateGlobalSummaryLabel();
    slotRefreshTableLink();
}

PresetTableV2TransitionWidget::~PresetTableV2TransitionWidget() = default;

PTTransitionMode PresetTableV2TransitionWidget::activeBankMode() const
{
    if (!m_bankTabs)
        return PTTransitionMode::SweepOnly;
    if (m_bankTabs->currentIndex() == 1)
        return PTTransitionMode::Continuous;
    if (m_bankTabs->currentIndex() == 2)
        return PTTransitionMode::MultiFx;
    return PTTransitionMode::SweepOnly;
}

QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::presetsForMode(PTTransitionMode mode)
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxPresets;
    return (mode == PTTransitionMode::Continuous) ? m_continuousPresets : m_sweepPresets;
}

const QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::presetsForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxPresets;
    return (mode == PTTransitionMode::Continuous) ? m_continuousPresets : m_sweepPresets;
}

QVector<QHash<int, PresetTableV2TransitionWidget::PTTransitionPresetOverride>>&
PresetTableV2TransitionWidget::overridesForMode(PTTransitionMode mode)
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxOutputOverrides;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousOutputOverrides : m_sweepOutputOverrides;
}

const QVector<QHash<int, PresetTableV2TransitionWidget::PTTransitionPresetOverride>>&
PresetTableV2TransitionWidget::overridesForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxOutputOverrides;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousOutputOverrides : m_sweepOutputOverrides;
}

QTreeWidget* PresetTableV2TransitionWidget::tableForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxTable;
    return (mode == PTTransitionMode::Continuous) ? m_continuousTable : m_sweepTable;
}

QTreeView* PresetTableV2TransitionWidget::frozenNameViewForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxNameView;
    return (mode == PTTransitionMode::Continuous) ? m_continuousNameView : m_sweepNameView;
}

QTreeWidget* PresetTableV2TransitionWidget::activeTable() const
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
    c->addItem(QStringLiteral("Custom"), 3);
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

QVariant PresetTableV2TransitionWidget::presetColumnValue(const PTTransitionPreset& preset, int col)
{
    switch (col)
    {
        case ColAxis:          return int(preset.axis);
        case ColOffsetDir:     return int(preset.offsetDirection);
        case ColWings:         return preset.wings;
        case ColBlocks:        return preset.blocks;
        case ColWingsSymmetry: return preset.wingsSymmetry;
        case ColOffsetStep:    return preset.offsetStep;
        case ColDuration:      return int(preset.durationMs);
        case ColWaveWidth:     return preset.waveWidth;
        case ColWaveShape:     return preset.customCurveEnabled ? 3 : preset.waveShape;
        case ColFadeIn:        return preset.waveFadeIn;
        case ColFadeOut:       return preset.waveFadeOut;
        case ColWaveLevel:     return preset.waveLevel;
        case ColStartOffset:   return preset.startOffset;
        case ColPropagation:   return int(preset.propagation);
        case ColSpeedMult:     return preset.speedMultiplier;
        default:               return QVariant();
    }
}

void PresetTableV2TransitionWidget::setPresetColumnValue(PTTransitionPreset& preset, int col,
                                                         const QVariant& value)
{
    switch (col)
    {
        case ColAxis:
            preset.axis = PTTransitionAxis(value.toInt());
            break;
        case ColOffsetDir:
            preset.offsetDirection = PTOffsetDirection(value.toInt());
            break;
        case ColWings:
            preset.wings = value.toInt();
            break;
        case ColBlocks:
            preset.blocks = value.toInt();
            break;
        case ColWingsSymmetry:
            preset.wingsSymmetry = value.toInt();
            break;
        case ColOffsetStep:
            preset.offsetStep = value.toInt();
            break;
        case ColDuration:
            preset.durationMs = quint32(value.toInt());
            break;
        case ColWaveWidth:
            preset.waveWidth = value.toInt();
            break;
        case ColWaveShape:
            preset.customCurveEnabled = value.toInt() == 3;
            preset.waveShape = preset.customCurveEnabled ? 0 : value.toInt();
            if (preset.customCurveEnabled && preset.customCurve.size() < 2)
                preset.customCurve = defaultTransitionCustomCurve();
            break;
        case ColFadeIn:
            preset.waveFadeIn = value.toInt();
            break;
        case ColFadeOut:
            preset.waveFadeOut = value.toInt();
            break;
        case ColWaveLevel:
            preset.waveLevel = value.toInt();
            break;
        case ColStartOffset:
            preset.startOffset = value.toInt();
            break;
        case ColPropagation:
            preset.propagation = PTPropagationMode(value.toInt());
            break;
        case ColSpeedMult:
            preset.speedMultiplier = value.toInt();
            break;
        default:
            break;
    }
}

QString PresetTableV2TransitionWidget::presetColumnXmlName(int col)
{
    switch (col)
    {
        case ColAxis:          return KXMLPresetAxis;
        case ColOffsetDir:     return KXMLPresetOffsetDir;
        case ColWings:         return KXMLPresetWings;
        case ColBlocks:        return KXMLPresetBlocks;
        case ColWingsSymmetry: return KXMLPresetWingsSymmetry;
        case ColOffsetStep:    return KXMLPresetOffsetStep;
        case ColDuration:      return KXMLPresetDuration;
        case ColWaveWidth:     return KXMLPresetWaveWidth;
        case ColWaveShape:     return KXMLPresetWaveShape;
        case ColFadeIn:        return KXMLPresetFadeIn;
        case ColFadeOut:       return KXMLPresetFadeOut;
        case ColWaveLevel:     return KXMLPresetWaveLevel;
        case ColStartOffset:   return KXMLPresetStartOffset;
        case ColPropagation:   return KXMLPresetPropagation;
        case ColSpeedMult:     return KXMLPresetSpeedMult;
        default:               return QString();
    }
}

int PresetTableV2TransitionWidget::presetColumnFromXmlName(const QString& name)
{
    for (int col = ColAxis; col < ColCount; ++col)
    {
        if (presetColumnXmlName(col) == name)
            return col;
    }
    return -1;
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

    const QString xfMode = crossfadeManualControlEnabled()
            ? tr("XF manual ON") : tr("XF clock");
    const QString text = tr("Speed: %1%2 | Intensity: %3%4 | Cycle: %5–%6 ms | %7%8")
            .arg(m_globalSettings.speed)
            .arg(inputMark(PTEfxCol::InputGlobalSpeed))
            .arg(m_globalSettings.intensity)
            .arg(inputMark(PTEfxCol::InputGlobalIntensity))
            .arg(m_globalSettings.minDurationMs)
            .arg(m_globalSettings.maxDurationMs)
            .arg(xfMode)
            .arg(inputMark(PTEfxCol::InputCrossfadeManual));
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

    m_previewRow = new QWidget(this);
    QHBoxLayout* previewLayout = new QHBoxLayout(m_previewRow);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);
    m_curveWidget = new PTDimmerWaveCurveWidget(m_previewRow);
    connect(m_curveWidget, &PTDimmerWaveCurveWidget::customCurveEditRequested,
            this, &PresetTableV2TransitionWidget::slotOpenCustomCurveEditor);
    m_spatialGridWidget = new PTSpatialFixtureGridWidget(m_previewRow);
    m_spatialGridWidget->setToolTip(
            tr("Fixture group: sweep order, head offset (°), phase start. "
               "Orange border = offset step too large; red = duplicate offsets."));
    previewLayout->addWidget(m_curveWidget, 3);
    previewLayout->addWidget(m_spatialGridWidget, 2);
    m_layout->addWidget(m_previewRow);

    m_bankTabs = new QTabWidget(this);
    m_sweepTable = new QTreeWidget(m_bankTabs);
    m_continuousTable = new QTreeWidget(m_bankTabs);
    m_multiFxTable = new QTreeWidget(m_bankTabs);
    m_sweepNameView = new QTreeView(m_bankTabs);
    m_continuousNameView = new QTreeView(m_bankTabs);
    m_multiFxNameView = new QTreeView(m_bankTabs);
    for (QTreeWidget* table : { m_sweepTable, m_continuousTable, m_multiFxTable })
    {
        table->setColumnCount(ColCount);
        table->header()->setStretchLastSection(true);
        table->setColumnHidden(ColName, true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setRootIsDecorated(true);
        table->setUniformRowHeights(true);
        table->setAlternatingRowColors(true);
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        applyPersistentScrollBars(table);
        table->installEventFilter(this);
        table->viewport()->installEventFilter(this);
        connect(table, &QTreeWidget::itemChanged,
                this, &PresetTableV2TransitionWidget::slotPresetItemChanged);
        connect(table, &QTreeWidget::itemSelectionChanged,
                this, &PresetTableV2TransitionWidget::updateEffectPreview);
        connect(table, &QTreeWidget::customContextMenuRequested,
                this, &PresetTableV2TransitionWidget::slotPresetContextMenuRequested);
        connect(table->header(), &QHeaderView::sectionDoubleClicked,
                this, &PresetTableV2TransitionWidget::slotColumnHeaderDoubleClicked);
        connect(table, &QTreeWidget::itemExpanded, this, [this, table](QTreeWidgetItem* item) {
            if (!item || item->parent())
                return;
            PTTransitionMode mode = PTTransitionMode::SweepOnly;
            if (table == m_continuousTable)
                mode = PTTransitionMode::Continuous;
            else if (table == m_multiFxTable)
                mode = PTTransitionMode::MultiFx;
            const int row = item->data(0, kItemPresetIndexRole).toInt();
            expandedSetForMode(mode).insert(row);
            if (QTreeView* frozen = frozenNameViewForMode(mode))
                frozen->expand(table->indexFromItem(item));
        });
        connect(table, &QTreeWidget::itemCollapsed, this, [this, table](QTreeWidgetItem* item) {
            if (!item || item->parent())
                return;
            PTTransitionMode mode = PTTransitionMode::SweepOnly;
            if (table == m_continuousTable)
                mode = PTTransitionMode::Continuous;
            else if (table == m_multiFxTable)
                mode = PTTransitionMode::MultiFx;
            const int row = item->data(0, kItemPresetIndexRole).toInt();
            expandedSetForMode(mode).remove(row);
            if (QTreeView* frozen = frozenNameViewForMode(mode))
                frozen->collapse(table->indexFromItem(item));
        });
    }
    auto makeBankPage = [](QTreeView* names, QTreeWidget* table) {
        QWidget* page = new QWidget;
        QHBoxLayout* lay = new QHBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
        lay->addWidget(names);
        lay->addWidget(table, 1);
        return page;
    };
    m_bankTabs->addTab(makeBankPage(m_sweepNameView, m_sweepTable), tr("Transitions"));
    m_bankTabs->addTab(makeBankPage(m_continuousNameView, m_continuousTable), tr("Continuous FX"));
    m_bankTabs->addTab(makeBankPage(m_multiFxNameView, m_multiFxTable), tr("MultiFX"));
    auto linkFrozenExpansion = [this](QTreeView* frozen, QTreeWidget* table,
                                      PTTransitionMode mode) {
        if (!frozen || !table)
            return;
        frozen->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(frozen, &QTreeView::expanded, this,
                [this, mode](const QModelIndex& idx) {
            if (idx.data(kItemOutputIndexRole).toInt() < 0)
            {
                const int row = idx.data(kItemPresetIndexRole).toInt();
                expandedSetForMode(mode).insert(row);
                if (QTreeWidgetItem* item = parentItemForPreset(mode, row))
                    item->setExpanded(true);
            }
        });
        connect(frozen, &QTreeView::collapsed, this,
                [this, mode](const QModelIndex& idx) {
            if (idx.data(kItemOutputIndexRole).toInt() < 0)
            {
                const int row = idx.data(kItemPresetIndexRole).toInt();
                expandedSetForMode(mode).remove(row);
                if (QTreeWidgetItem* item = parentItemForPreset(mode, row))
                    item->setExpanded(false);
            }
        });
        connect(frozen, &QTreeView::doubleClicked, this,
                [this, mode](const QModelIndex& idx) {
            if (idx.data(kItemOutputIndexRole).toInt() >= 0)
                return;
            const int row = idx.data(kItemPresetIndexRole).toInt();
            if (QTreeWidgetItem* item = parentItemForPreset(mode, row))
                item->setExpanded(!item->isExpanded());
        });
        connect(frozen, &QTreeView::customContextMenuRequested, this,
                [this, frozen, mode](const QPoint& pos) {
            const QModelIndex idx = frozen->indexAt(pos);
            if (!idx.isValid() || idx.data(kItemOutputIndexRole).toInt() >= 0)
                return;
            const int row = idx.data(kItemPresetIndexRole).toInt();
            QTreeWidgetItem* item = parentItemForPreset(mode, row);
            if (!item || item->childCount() <= 0)
                return;
            QMenu menu(this);
            QAction* toggleAct = menu.addAction(item->isExpanded()
                    ? tr("Collapse outputs") : tr("Expand outputs"));
            QAction* chosen = menu.exec(frozen->viewport()->mapToGlobal(pos));
            if (chosen == toggleAct)
                item->setExpanded(!item->isExpanded());
        });
    };
    linkFrozenExpansion(m_sweepNameView, m_sweepTable, PTTransitionMode::SweepOnly);
    linkFrozenExpansion(m_continuousNameView, m_continuousTable, PTTransitionMode::Continuous);
    linkFrozenExpansion(m_multiFxNameView, m_multiFxTable, PTTransitionMode::MultiFx);
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
    rebuildPresetTable(PTTransitionMode::MultiFx);
}

void PresetTableV2TransitionWidget::updateColumnHeaders(QTreeWidget* table)
{
    if (!table)
        return;

    QStringList headers;
    for (int c = 0; c < ColCount; ++c)
    {
        QString title = (c == ColName) ? tr("Name") : columnTitle(c);
        if (PTEfxCol::hasExternalInput(c))
        {
            const auto src = inputSource(PTEfxCol::inputIdForColumn(c));
            if (src && src->isValid())
                title += QStringLiteral(" *");
        }
        headers << title;
    }
    table->setHeaderLabels(headers);
}

void PresetTableV2TransitionWidget::rebuildPresetTable(PTTransitionMode mode)
{
    QTreeWidget* table = tableForMode(mode);
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (!table)
        return;

    normalizeOverrideStorage();
    for (int r = 0; r < presets.size(); ++r)
        normalizeNoopOverridesForPreset(mode, r);
    captureExpandedState(mode);
    const int scrollValue = table->verticalScrollBar() ? table->verticalScrollBar()->value() : 0;
    QTreeWidgetItem* currentBefore = table->currentItem();
    const int currentRow = currentBefore ? currentBefore->data(0, kItemPresetIndexRole).toInt() : -1;
    const int currentOutput = currentBefore ? currentBefore->data(0, kItemOutputIndexRole).toInt() : -1;
    m_rebuildingTable = true;
    table->clear();
    updateColumnHeaders(table);
    table->setColumnHidden(ColName, true);

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

    auto makeCell = [&](QTreeWidgetItem* item, int row, int outputIdx, int col,
                        const PTTransitionPreset& preset, bool inherited) {
        const QVariant value = presetColumnValue(preset, col);
        QWidget* editor = nullptr;
        if (col == ColAxis || col == ColOffsetDir || col == ColWaveShape
                || col == ColPropagation || col == ColWingsSymmetry || col == ColSpeedMult)
        {
            QComboBox* combo = nullptr;
            if (col == ColAxis)
                combo = makeAxisCombo(table);
            else if (col == ColOffsetDir)
                combo = makeOffsetDirCombo(table);
            else if (col == ColWaveShape)
                combo = makeWaveShapeCombo(table);
            else if (col == ColPropagation)
                combo = makePropagationCombo(table);
            else if (col == ColWingsSymmetry)
                combo = makeWingsSymmetryCombo(table);
            else
                combo = makeSpeedMultCombo(table);
            setComboIndex(combo, value.toInt());
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, mode, row, outputIdx, col](int) {
                        slotPresetChanged(mode, row, col, outputIdx);
                    });
            editor = combo;
        }
        else
        {
            QSpinBox* spin = new QSpinBox(table);
            int minV = 0;
            int maxV = 360;
            switch (col)
            {
                case ColWings:
                case ColBlocks:
                    minV = 1; maxV = 64; break;
                case ColOffsetStep:
                    minV = 1; maxV = 360; break;
                case ColDuration:
                    minV = 20; maxV = 60000; break;
                case ColWaveWidth:
                    minV = 1; maxV = 360; break;
                case ColFadeIn:
                case ColFadeOut:
                    minV = 0; maxV = 100; break;
                case ColWaveLevel:
                    minV = 0; maxV = 255; break;
                case ColStartOffset:
                    minV = 0; maxV = 360; break;
                default:
                    break;
            }
            spin->setRange(minV, maxV);
            spin->setKeyboardTracking(false);
            spin->setFixedHeight(kEfxTreeRowHeight);
            spin->setValue(value.toInt());
            connect(spin, &QSpinBox::editingFinished, this,
                    [this, mode, row, outputIdx, col]() {
                        slotPresetChanged(mode, row, col, outputIdx);
                    });
            editor = spin;
        }
        if (editor)
        {
            editor->setFixedHeight(kEfxTreeRowHeight);
            editor->installEventFilter(this);
        }
        styleTransitionEditorWidget(editor, inherited);
        table->setItemWidget(item, col, editor);
    };

    const int outputCount = linkedOutputCount();
    const auto& bankOverrides = overridesForMode(mode);
    for (int r = 0; r < presets.size(); ++r)
    {
        QTreeWidgetItem* parent = new QTreeWidgetItem(table);
        parent->setData(0, kItemPresetIndexRole, r);
        parent->setData(0, kItemOutputIndexRole, -1);
        parent->setFlags(parent->flags() | Qt::ItemIsEditable);
        parent->setText(ColName, presets[r].name);

        for (int col = ColAxis; col < ColCount; ++col)
            makeCell(parent, r, -1, col, presets[r], false);
        updatePresetRowUiForItem(parent, mode);
        updateOffsetStepLimitForItem(parent, mode);

        for (int o = 0; o < outputCount; ++o)
        {
            QTreeWidgetItem* child = new QTreeWidgetItem(parent);
            child->setData(0, kItemPresetIndexRole, r);
            child->setData(0, kItemOutputIndexRole, o);
            child->setFlags((child->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled)
                            & ~Qt::ItemIsEditable);
            child->setText(ColName, linkedOutputName(o));
            child->setToolTip(ColName, tr("Per-output overrides for this preset"));
            const bool outputHasOverrides = r < bankOverrides.size()
                    && bankOverrides.at(r).contains(o)
                    && !bankOverrides.at(r).value(o).columns.isEmpty();
            Q_UNUSED(outputHasOverrides);
            const PTTransitionPreset effective = effectivePresetForOutputNoLive(mode, r, o);
            for (int col = ColAxis; col < ColCount; ++col)
            {
                const bool inherited = !(r < bankOverrides.size()
                        && bankOverrides.at(r).contains(o)
                        && bankOverrides.at(r).value(o).columns.contains(col));
                makeCell(child, r, o, col, effective, inherited);
            }
            updatePresetRowUiForItem(child, mode);
            updateOffsetStepLimitForItem(child, mode);
        }
        refreshOverrideVisualsForPreset(mode, r);
        parent->setExpanded(expandedSetForMode(mode).contains(r));
    }

    table->setColumnHidden(ColDuration, true);
    configureFrozenNameView(mode);
    if (currentRow >= 0)
    {
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, currentRow))
        {
            QTreeWidgetItem* restore = currentOutput >= 0 ? parent->child(currentOutput) : parent;
            if (restore)
                table->setCurrentItem(restore);
        }
    }
    if (table->verticalScrollBar())
        table->verticalScrollBar()->setValue(scrollValue);
    m_rebuildingTable = false;
    updateEffectPreview();
}

PTTransitionPreset PresetTableV2TransitionWidget::presetFromItem(PTTransitionMode mode,
                                                                 QTreeWidgetItem* item) const
{
    PTTransitionPreset p;
    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QTreeWidget* table = tableForMode(mode);
    if (!item || !table)
        return p;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    if (row < 0 || row >= presets.size())
        return p;

    p = presets[row];
    p.playbackMode = (mode == PTTransitionMode::Continuous || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;

    if (outputIdx >= 0)
        p = effectivePresetForOutputNoLive(mode, row, outputIdx);

    if (outputIdx < 0)
        p.name = item->text(ColName);

    for (int col = ColAxis; col < ColCount; ++col)
    {
        QVariant value;
        if (auto* combo = qobject_cast<QComboBox*>(table->itemWidget(item, col)))
            value = combo->currentData();
        else if (auto* spin = qobject_cast<QSpinBox*>(table->itemWidget(item, col)))
            value = spin->value();
        if (value.isValid())
            setPresetColumnValue(p, col, value);
    }

    p.enabled = true;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    PTDimmerWaveEngine::clampOffsetStep(p, gridSpanForPreset(p));
    return p;
}

void PresetTableV2TransitionWidget::updatePresetRowUiForItem(QTreeWidgetItem* item,
                                                             PTTransitionMode bankMode)
{
    QTreeWidget* table = tableForMode(bankMode);
    if (!table || !item)
        return;

    const bool sweep = (bankMode == PTTransitionMode::SweepOnly);

    auto tuneSpin = [&](int col, bool enabled, int minV, int maxV, int value) {
        QSpinBox* s = qobject_cast<QSpinBox*>(table->itemWidget(item, col));
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
    if (auto* ww = qobject_cast<QSpinBox*>(table->itemWidget(item, ColWaveWidth)))
        waveWidth = ww->value();
    if (auto* wl = qobject_cast<QSpinBox*>(table->itemWidget(item, ColWaveLevel)))
        waveLevel = wl->value();
    if (auto* fi = qobject_cast<QSpinBox*>(table->itemWidget(item, ColFadeIn)))
        fadeIn = fi->value();
    if (auto* fo = qobject_cast<QSpinBox*>(table->itemWidget(item, ColFadeOut)))
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
    presets[row] = presetFromItem(mode, parentItemForPreset(mode, row));
}

void PresetTableV2TransitionWidget::syncActiveBankFromTable()
{
    const PTTransitionMode mode = activeBankMode();
    QTreeWidget* table = tableForMode(mode);
    if (!table)
        return;
    for (int r = 0; r < table->topLevelItemCount(); ++r)
        syncPresetFromTable(mode, r);
}

void PresetTableV2TransitionWidget::slotPresetChanged(PTTransitionMode mode, int row,
                                                      int col, int outputIdx)
{
    if (m_rebuildingTable)
        return;

    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return;

    if (col == ColWaveShape)
    {
        QTreeWidget* table = tableForMode(mode);
        QTreeWidgetItem* item = outputIdx >= 0
                ? (parentItemForPreset(mode, row)
                   ? parentItemForPreset(mode, row)->child(outputIdx) : nullptr)
                : parentItemForPreset(mode, row);
        QComboBox* combo = table && item
                ? qobject_cast<QComboBox*>(table->itemWidget(item, ColWaveShape)) : nullptr;
        if (combo && combo->currentData().toInt() == 3 && outputIdx < 0)
        {
            const PTTransitionPreset before = outputIdx >= 0
                    ? effectivePresetForOutputNoLive(mode, row, outputIdx) : presets.at(row);
            if (!editCustomCurveForPreset(mode, row))
            {
                QSignalBlocker blocker(combo);
                setComboDataIndex(combo, before.customCurveEnabled ? 3 : before.waveShape);
            }
            updatePresetRowUiForItem(item, mode);
            updateOffsetStepLimitForItem(item, mode);
            notifyTablePresetCacheRefresh();
            updateEffectPreview();
            return;
        }
    }

    QTreeWidgetItem* item = outputIdx >= 0
            ? (parentItemForPreset(mode, row) ? parentItemForPreset(mode, row)->child(outputIdx) : nullptr)
            : parentItemForPreset(mode, row);
    if (!item)
        return;

    if (outputIdx >= 0)
    {
        updatePresetRowUiForItem(item, mode);
        QVector<QHash<int, PTTransitionPresetOverride>>& allOverrides = overridesForMode(mode);
        while (allOverrides.size() <= row)
            allOverrides.append(QHash<int, PTTransitionPresetOverride>());
        PTTransitionPresetOverride ov = allOverrides[row].value(outputIdx);
        ov.values = effectivePresetForOutputNoLive(mode, row, outputIdx);
        setColumnOverrideValue(ov, col, presetFromItem(mode, item));
        allOverrides[row].insert(outputIdx, ov);
        expandedSetForMode(mode).insert(row);
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
            parent->setExpanded(true);
    }
    else
    {
        syncPresetFromTable(mode, row);
        updatePresetRowUiForItem(item, mode);
    }
    normalizeNoopOverridesForPreset(mode, row);
    for (int i = 0; item && outputIdx < 0 && i < item->childCount(); ++i)
    {
        updatePresetRowUiForItem(item->child(i), mode);
        updateOffsetStepLimitForItem(item->child(i), mode);
    }
    updateOffsetStepLimitForItem(item, mode);
    refreshOverrideVisualsForPreset(mode, row);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
}

void PresetTableV2TransitionWidget::slotPresetItemChanged(QTreeWidgetItem* item, int col)
{
    if (m_rebuildingTable)
        return;
    if (!item || col != ColName || item->data(0, kItemOutputIndexRole).toInt() >= 0)
        return;

    QTreeWidget* table = qobject_cast<QTreeWidget*>(sender());
    PTTransitionMode mode = PTTransitionMode::SweepOnly;
    if (table == m_continuousTable)
        mode = PTTransitionMode::Continuous;
    else if (table == m_multiFxTable)
        mode = PTTransitionMode::MultiFx;
    else if (table != m_sweepTable)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    slotPresetChanged(mode, row, col);
}

void PresetTableV2TransitionWidget::slotOpenCustomCurveEditor()
{
    QTreeWidget* table = activeTable();
    if (!table)
        return;
    QTreeWidgetItem* item = selectedPresetItem(table);
    const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
    editCustomCurveForPreset(activeBankMode(), row);
}

bool PresetTableV2TransitionWidget::editCustomCurveForPreset(PTTransitionMode mode, int row)
{
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return false;

    PTTransitionPreset candidate = presets.at(row);
    candidate.customCurveEnabled = true;
    candidate.waveShape = 0;
    if (candidate.customCurve.size() < 2)
        candidate.customCurve = defaultTransitionCustomCurve();

    PTCustomCurveDialog dlg(candidate, m_customCurveGallery, this);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    candidate.customCurve = dlg.customCurve();
    candidate.customCurveEnabled = true;
    candidate.waveShape = 0;
    presets[row] = candidate;
    m_customCurveGallery = dlg.gallery();

    QTreeWidget* table = tableForMode(mode);
    if (table)
    {
        QTreeWidgetItem* item = parentItemForPreset(mode, row);
        if (auto* combo = item ? qobject_cast<QComboBox*>(
                table->itemWidget(item, ColWaveShape)) : nullptr)
        {
            QSignalBlocker blocker(combo);
            setComboDataIndex(combo, 3);
        }
    }
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
    return true;
}

void PresetTableV2TransitionWidget::slotBankTabChanged(int)
{
    if (m_sweepTable)
    {
        for (int r = 0; r < m_sweepTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::SweepOnly, r);
    }
    if (m_continuousTable)
    {
        for (int r = 0; r < m_continuousTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::Continuous, r);
    }
    if (m_multiFxTable)
    {
        for (int r = 0; r < m_multiFxTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::MultiFx, r);
    }
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
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
    // EFX parameters are always live. Staged/commit is owned by the table and
    // applies only to primary/secondary row and bank preset selections.
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
        case PTEfxCol::InputCrossfadeManual:
            m_crossfadeManualInputMapped = true;
            m_crossfadeManualControl = (value > 127);
            updateGlobalSummaryLabel();
            notifyTablePresetCacheRefresh();
            return true;
        default:
            return false;
    }
}

bool PresetTableV2TransitionWidget::crossfadeManualControlEnabled() const
{
    const auto src = inputSource(PTEfxCol::InputCrossfadeManual);
    if (src && src->isValid())
        return m_crossfadeManualControl;
    return true;
}

int PresetTableV2TransitionWidget::gridSpanForPreset(const PTTransitionPreset& preset) const
{
    if (PresetTableV2ControlIface* table = linkedTable())
        return table->fixtureGroupSpanAlongAxis(preset, m_globalSettings);
    return 0;
}

void PresetTableV2TransitionWidget::updateOffsetStepLimitForItem(QTreeWidgetItem* item,
                                                                 PTTransitionMode mode)
{
    QTreeWidget* table = tableForMode(mode);
    if (!table || !item)
        return;

    QSpinBox* step = qobject_cast<QSpinBox*>(table->itemWidget(item, ColOffsetStep));
    if (!step)
        return;

    const PTTransitionPreset preset = presetFromItem(mode, item);
    int maxStep = 360;
    int slotCount = 0;
    const int span = gridSpanForPreset(preset);
    if (span > 0)
    {
        slotCount = PTDimmerWaveEngine::effectiveOffsetSlotCount(span, preset);
        maxStep = PTDimmerWaveEngine::maxOffsetStepForGrid(span, preset);
    }

    const QSignalBlocker block(step);
    step->setMaximum(maxStep);
    if (step->value() > maxStep)
        step->setValue(maxStep);
    if (slotCount > 0)
    {
        step->setToolTip(tr("Offset step (max %1° for %2 slots: wings×blocks per wing)")
                                 .arg(maxStep)
                                 .arg(slotCount));
    }
    else
    {
        step->setToolTip(tr("Offset step (link Fixture Group table for max limit)"));
    }
}

void PresetTableV2TransitionWidget::updateEffectPreview()
{
    if (!m_curveWidget)
        return;

    const PTTransitionMode mode = activeBankMode();
    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (presets.isEmpty())
    {
        if (m_spatialGridWidget)
            m_spatialGridWidget->setPlaceholderText(tr("Add a preset"));
        return;
    }

    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    int row = item ? item->data(0, kItemPresetIndexRole).toInt() : 0;
    if (row < 0)
        row = 0;
    if (row >= presets.size())
        row = presets.size() - 1;

    if (!item)
        item = parentItemForPreset(mode, row);
    const PTTransitionPreset preset = presetFromItem(mode, item);
    const PTDimmerWaveParams params = PTDimmerWaveEngine::paramsFromPreset(preset, &m_globalSettings);
    const quint32 cycleMs = PTParamMatrixEngine::effectiveDurationMs(m_globalSettings, preset, false);
    m_curveWidget->setParams(params);
    m_curveWidget->setEditable(false);
    if (preset.customCurveEnabled)
        m_curveWidget->setCustomCurve(preset.customCurve);
    m_curveWidget->setCycleDurationMs(cycleMs);

    if (!m_spatialGridWidget)
        return;

    PTSpatialGridPreview preview;
    if (PresetTableV2ControlIface* tableIface = linkedTable())
    {
        if (tableIface->spatialGridPreview(preset, m_globalSettings, preview))
        {
            m_spatialGridWidget->setPreview(preview);
            return;
        }
    }
    m_spatialGridWidget->setPlaceholderText(
            tr("Link Preset Table v2 in Fixture Group mode to preview spatial order"));
}

void PresetTableV2TransitionWidget::slotAddPreset()
{
    PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    presets.append(defaultPreset(presets.size(), mode));
    overrides.append(QHash<int, PTTransitionPresetOverride>());
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::slotRemovePreset()
{
    const PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    QTreeWidget* table = tableForMode(mode);
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
    if (row < 0 || presets.size() <= 1)
        return;
    presets.removeAt(row);
    if (row < overrides.size())
        overrides.removeAt(row);
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::slotDuplicatePreset()
{
    const PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    QTreeWidget* table = tableForMode(mode);
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
    if (row < 0)
        return;
    syncPresetFromTable(mode, row);
    PTTransitionPreset copy = presets[row];
    copy.name += tr(" copy");
    presets.append(copy);
    overrides.append(row < overrides.size()
            ? overrides.at(row) : QHash<int, PTTransitionPresetOverride>());
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

int PresetTableV2TransitionWidget::linkedOutputCount() const
{
    if (PresetTableV2ControlIface* table = linkedTable())
        return table->outputCountForPresetOverrides();
    return 0;
}

QString PresetTableV2TransitionWidget::linkedOutputName(int outputIdx) const
{
    if (PresetTableV2ControlIface* table = linkedTable())
    {
        const QString name = table->outputNameForPresetOverride(outputIdx);
        if (!name.isEmpty())
            return name;
    }
    return tr("Output %1").arg(outputIdx + 1);
}

QTreeWidgetItem* PresetTableV2TransitionWidget::parentItemForPreset(PTTransitionMode mode,
                                                                    int row) const
{
    QTreeWidget* table = tableForMode(mode);
    if (!table || row < 0 || row >= table->topLevelItemCount())
        return nullptr;
    return table->topLevelItem(row);
}

QTreeWidgetItem* PresetTableV2TransitionWidget::selectedPresetItem(QTreeWidget* table) const
{
    if (!table)
        return nullptr;
    QTreeWidgetItem* item = table->currentItem();
    if (!item && table->topLevelItemCount() > 0)
        item = table->topLevelItem(0);
    return item;
}

void PresetTableV2TransitionWidget::normalizeOverrideStorage()
{
    auto normalize = [](QVector<PTTransitionPreset>& presets,
                       QVector<QHash<int, PTTransitionPresetOverride>>& overrides) {
        while (overrides.size() < presets.size())
            overrides.append(QHash<int, PTTransitionPresetOverride>());
        while (overrides.size() > presets.size())
            overrides.removeLast();
    };
    normalize(m_sweepPresets, m_sweepOutputOverrides);
    normalize(m_continuousPresets, m_continuousOutputOverrides);
    normalize(m_multiFxPresets, m_multiFxOutputOverrides);
}

void PresetTableV2TransitionWidget::refreshOverrideVisualsForPreset(PTTransitionMode mode,
                                                                    int row)
{
    QTreeWidgetItem* parent = parentItemForPreset(mode, row);
    if (!parent)
        return;

    refreshOverrideVisualsForItem(mode, parent);
    for (int i = 0; i < parent->childCount(); ++i)
        refreshOverrideVisualsForItem(mode, parent->child(i));
}

void PresetTableV2TransitionWidget::refreshOverrideVisualsForItem(PTTransitionMode mode,
                                                                  QTreeWidgetItem* item)
{
    QTreeWidget* table = tableForMode(mode);
    if (!table || !item)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const auto& overrides = overridesForMode(mode);
    if (row < 0)
        return;

    for (int col = ColAxis; col < ColCount; ++col)
    {
        QWidget* widget = table->itemWidget(item, col);
        if (!widget)
            continue;

        if (outputIdx < 0)
        {
            bool hasOutputOverride = false;
            PTTransitionPreset parent = row < presetsForMode(mode).size()
                    ? presetsForMode(mode).at(row) : PTTransitionPreset();
            parent.playbackMode = (mode == PTTransitionMode::Continuous
                                   || mode == PTTransitionMode::MultiFx)
                    ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
            if (mode == PTTransitionMode::SweepOnly)
                PresetTableV2SpatialEngine::applySweepPresetConstraints(parent);
            PTDimmerWaveEngine::clampOffsetStep(parent, gridSpanForPreset(parent));
            if (row < overrides.size())
            {
                for (auto it = overrides.at(row).constBegin();
                     it != overrides.at(row).constEnd(); ++it)
                {
                    if (overrideColumnDiffersFromParent(parent, it.value(), col))
                    {
                        hasOutputOverride = true;
                        break;
                    }
                }
            }
            styleTransitionEditorWidget(widget, false, hasOutputOverride);
            continue;
        }

        const bool inherited = !(row < overrides.size()
                && overrides.at(row).contains(outputIdx)
                && overrides.at(row).value(outputIdx).columns.contains(col));
        if (inherited)
        {
            const QVariant value = presetColumnValue(
                    effectivePresetForOutputNoLive(mode, row, outputIdx), col);
            if (auto* combo = qobject_cast<QComboBox*>(widget))
            {
                QSignalBlocker block(combo);
                setComboDataIndex(combo, value.toInt());
            }
            else if (auto* spin = qobject_cast<QSpinBox*>(widget))
            {
                QSignalBlocker block(spin);
                spin->setValue(qBound(spin->minimum(), value.toInt(), spin->maximum()));
            }
        }
        styleTransitionEditorWidget(widget, inherited);
    }
}

QSet<int>& PresetTableV2TransitionWidget::expandedSetForMode(PTTransitionMode mode)
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxExpandedPresets;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousExpandedPresets : m_sweepExpandedPresets;
}

const QSet<int>& PresetTableV2TransitionWidget::expandedSetForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxExpandedPresets;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousExpandedPresets : m_sweepExpandedPresets;
}

void PresetTableV2TransitionWidget::captureExpandedState(PTTransitionMode mode)
{
    QTreeWidget* table = tableForMode(mode);
    if (!table)
        return;
    QSet<int>& expanded = expandedSetForMode(mode);
    expanded.clear();
    for (int r = 0; r < table->topLevelItemCount(); ++r)
    {
        if (QTreeWidgetItem* item = table->topLevelItem(r); item && item->isExpanded())
            expanded.insert(r);
    }
}

void PresetTableV2TransitionWidget::configureFrozenNameView(PTTransitionMode mode)
{
    QTreeWidget* table = tableForMode(mode);
    QTreeView* frozen = frozenNameViewForMode(mode);
    if (!table || !frozen)
        return;

    frozen->setModel(table->model());
    frozen->setSelectionModel(table->selectionModel());
    frozen->setRootIsDecorated(true);
    frozen->setItemsExpandable(true);
    frozen->setExpandsOnDoubleClick(true);
    frozen->setUniformRowHeights(true);
    frozen->setAlternatingRowColors(true);
    frozen->setSelectionBehavior(QAbstractItemView::SelectRows);
    frozen->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    frozen->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozen->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozen->setStyleSheet(QStringLiteral("QTreeView::item { min-height: %1px; }")
                          .arg(kEfxTreeRowHeight));
    frozen->header()->setStretchLastSection(true);
    frozen->setColumnHidden(ColName, false);
    for (int c = ColAxis; c < ColCount; ++c)
        frozen->setColumnHidden(c, true);
    frozen->setFixedWidth(190);
    frozen->installEventFilter(this);
    frozen->viewport()->installEventFilter(this);

    connect(table->verticalScrollBar(), &QScrollBar::valueChanged,
            frozen->verticalScrollBar(), &QScrollBar::setValue, Qt::UniqueConnection);
    connect(frozen->verticalScrollBar(), &QScrollBar::valueChanged,
            table->verticalScrollBar(), &QScrollBar::setValue, Qt::UniqueConnection);

    for (int r = 0; r < table->topLevelItemCount(); ++r)
    {
        if (QTreeWidgetItem* parent = table->topLevelItem(r))
        {
            const QModelIndex idx = table->indexFromItem(parent);
            parent->setToolTip(ColName, parent->childCount() > 0
                    ? tr("Expand to edit per-output overrides")
                    : tr("No linked outputs available"));
            if (parent->isExpanded())
                frozen->expand(idx);
            else
                frozen->collapse(idx);
        }
    }
}

PTTransitionPreset PresetTableV2TransitionWidget::effectivePresetForOutputNoLive(
        PTTransitionMode mode, int row, int outputIdx) const
{
    PTTransitionPreset p = transitionPreset(mode, row);
    const auto& overrides = overridesForMode(mode);
    if (row >= 0 && row < overrides.size() && overrides.at(row).contains(outputIdx))
    {
        const PTTransitionPresetOverride ov = overrides.at(row).value(outputIdx);
        for (int col : ov.columns)
        {
            setPresetColumnValue(p, col, presetColumnValue(ov.values, col));
            if (col == ColWaveShape)
            {
                p.customCurveEnabled = ov.values.customCurveEnabled;
                p.customCurve = ov.values.customCurve;
                p.waveShape = ov.values.waveShape;
            }
        }
    }
    p.playbackMode = (mode == PTTransitionMode::Continuous || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    PTDimmerWaveEngine::clampOffsetStep(p, gridSpanForPreset(p));
    return p;
}

void PresetTableV2TransitionWidget::setColumnOverrideValue(PTTransitionPresetOverride& ov,
                                                          int col,
                                                          const PTTransitionPreset& value)
{
    if (col <= ColName || col >= ColCount)
        return;
    setPresetColumnValue(ov.values, col, presetColumnValue(value, col));
    if (col == ColWaveShape)
    {
        ov.values.customCurveEnabled = value.customCurveEnabled;
        ov.values.customCurve = value.customCurve;
        ov.values.waveShape = value.waveShape;
    }
    ov.columns.insert(col);
}

bool PresetTableV2TransitionWidget::overrideColumnDiffersFromParent(
        const PTTransitionPreset& parent, const PTTransitionPresetOverride& ov, int col) const
{
    if (!ov.columns.contains(col))
        return false;
    if (col == ColWaveShape)
    {
        return ov.values.customCurveEnabled != parent.customCurveEnabled
                || ov.values.waveShape != parent.waveShape
                || serializeCustomCurve(ov.values.customCurve)
                   != serializeCustomCurve(parent.customCurve);
    }
    return presetColumnValue(parent, col) != presetColumnValue(ov.values, col);
}

void PresetTableV2TransitionWidget::normalizeNoopOverridesForPreset(PTTransitionMode mode,
                                                                    int row)
{
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= overrides.size() || row >= presets.size())
        return;

    PTTransitionPreset parent = presets.at(row);
    parent.playbackMode = (mode == PTTransitionMode::Continuous || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(parent);
    PTDimmerWaveEngine::clampOffsetStep(parent, gridSpanForPreset(parent));

    QList<int> emptyOutputs;
    for (auto it = overrides[row].begin(); it != overrides[row].end(); ++it)
    {
        PTTransitionPresetOverride ov = it.value();
        const QList<int> cols = ov.columns.values();
        for (int col : cols)
        {
            if (!overrideColumnDiffersFromParent(parent, ov, col))
                ov.columns.remove(col);
        }
        if (ov.columns.isEmpty())
            emptyOutputs.append(it.key());
        else
            it.value() = ov;
    }
    for (int outputIdx : emptyOutputs)
        overrides[row].remove(outputIdx);
}

void PresetTableV2TransitionWidget::normalizeNoopOverrides()
{
    normalizeOverrideStorage();
    for (int row = 0; row < m_sweepPresets.size(); ++row)
        normalizeNoopOverridesForPreset(PTTransitionMode::SweepOnly, row);
    for (int row = 0; row < m_continuousPresets.size(); ++row)
        normalizeNoopOverridesForPreset(PTTransitionMode::Continuous, row);
    for (int row = 0; row < m_multiFxPresets.size(); ++row)
        normalizeNoopOverridesForPreset(PTTransitionMode::MultiFx, row);
}

void PresetTableV2TransitionWidget::clearColumnOverride(PTTransitionMode mode, int row,
                                                       int outputIdx, int col)
{
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides[row].contains(outputIdx))
        return;
    PTTransitionPresetOverride ov = overrides[row].value(outputIdx);
    ov.columns.remove(col);
    if (ov.columns.isEmpty())
        overrides[row].remove(outputIdx);
    else
        overrides[row].insert(outputIdx, ov);
}

void PresetTableV2TransitionWidget::clearAllOverridesForOutput(PTTransitionMode mode,
                                                               int row, int outputIdx)
{
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    if (row >= 0 && row < overrides.size())
        overrides[row].remove(outputIdx);
}

void PresetTableV2TransitionWidget::copyOverridesToAllOutputs(PTTransitionMode mode,
                                                              int row, int outputIdx)
{
    QVector<QHash<int, PTTransitionPresetOverride>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides[row].contains(outputIdx))
        return;
    const PTTransitionPresetOverride ov = overrides[row].value(outputIdx);
    const int count = linkedOutputCount();
    for (int o = 0; o < count; ++o)
    {
        if (o == outputIdx)
            continue;
        if (ov.columns.isEmpty())
            overrides[row].remove(o);
        else
            overrides[row].insert(o, ov);
    }
}

void PresetTableV2TransitionWidget::slotPresetContextMenuRequested(const QPoint& pos)
{
    QTreeWidget* table = qobject_cast<QTreeWidget*>(sender());
    if (!table)
        return;

    PTTransitionMode mode = PTTransitionMode::SweepOnly;
    if (table == m_continuousTable)
        mode = PTTransitionMode::Continuous;
    else if (table == m_multiFxTable)
        mode = PTTransitionMode::MultiFx;
    else if (table != m_sweepTable)
        return;

    QTreeWidgetItem* item = table->itemAt(pos);
    const int col = table->columnAt(pos.x());
    if (!item || col < ColName || col >= ColCount)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    if (outputIdx < 0)
    {
        if (item->childCount() <= 0)
            return;
        QMenu menu(this);
        QAction* toggleAct = menu.addAction(item->isExpanded()
                ? tr("Collapse outputs") : tr("Expand outputs"));
        QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
        if (chosen == toggleAct)
            item->setExpanded(!item->isExpanded());
        return;
    }
    if (col <= ColName)
        return;

    QMenu menu(this);
    QAction* overrideAct = menu.addAction(tr("Override parameter"));
    QAction* resetParamAct = menu.addAction(tr("Reset parameter override"));
    QAction* resetAllAct = menu.addAction(tr("Reset all overrides for this output"));
    QAction* copyAllAct = menu.addAction(tr("Copy overrides to all outputs"));
    QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == overrideAct)
        slotPresetChanged(mode, row, col, outputIdx);
    else if (chosen == resetParamAct)
        clearColumnOverride(mode, row, outputIdx, col);
    else if (chosen == resetAllAct)
        clearAllOverridesForOutput(mode, row, outputIdx);
    else if (chosen == copyAllAct)
        copyOverridesToAllOutputs(mode, row, outputIdx);

    normalizeNoopOverridesForPreset(mode, row);
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
}

QVariant PresetTableV2TransitionWidget::editorValue(QTreeWidget* table,
                                                    QTreeWidgetItem* item, int col) const
{
    if (!table || !item)
        return QVariant();
    if (col == ColName)
        return item->text(ColName);
    if (auto* combo = qobject_cast<QComboBox*>(table->itemWidget(item, col)))
        return combo->currentText();
    if (auto* spin = qobject_cast<QSpinBox*>(table->itemWidget(item, col)))
        return spin->value();
    return QVariant();
}

void PresetTableV2TransitionWidget::setEditorValue(QTreeWidget* table,
                                                   QTreeWidgetItem* item, int col,
                                                   const QString& raw)
{
    if (!table || !item || col <= ColName || col >= ColCount)
        return;
    if (auto* combo = qobject_cast<QComboBox*>(table->itemWidget(item, col)))
    {
        const QString trimmed = raw.trimmed();
        int match = combo->findText(trimmed, Qt::MatchFixedString);
        if (match < 0)
        {
            bool ok = false;
            const int numeric = trimmed.toInt(&ok);
            if (ok)
            {
                for (int i = 0; i < combo->count(); ++i)
                {
                    if (combo->itemData(i).toInt() == numeric)
                    {
                        match = i;
                        break;
                    }
                }
            }
        }
        if (match >= 0)
        {
            QSignalBlocker block(combo);
            combo->setCurrentIndex(match);
        }
    }
    else if (auto* spin = qobject_cast<QSpinBox*>(table->itemWidget(item, col)))
    {
        bool ok = false;
        const int value = raw.trimmed().remove(QChar(0x00B0)).toInt(&ok);
        if (ok)
            spin->setValue(qBound(spin->minimum(), value, spin->maximum()));
    }
}

void PresetTableV2TransitionWidget::copySelectionToClipboard(QTreeWidget* table) const
{
    if (!table)
        return;
    QList<QTreeWidgetItem*> items = table->selectedItems();
    if (items.isEmpty() && table->currentItem())
        items.append(table->currentItem());
    if (items.isEmpty())
        return;
    std::sort(items.begin(), items.end(), [table](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        return table->indexOfTopLevelItem(a) < table->indexOfTopLevelItem(b);
    });
    const int col = qBound(int(ColAxis), table->currentColumn(), int(ColCount - 1));
    QStringList lines;
    for (QTreeWidgetItem* item : items)
        lines << editorValue(table, item, col).toString();
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

void PresetTableV2TransitionWidget::pasteClipboardToSelection(QTreeWidget* table)
{
    if (!table)
        return;
    QTreeWidgetItem* item = table->currentItem();
    if (!item)
        return;
    const int startCol = qBound(int(ColAxis), table->currentColumn(), int(ColCount - 1));
    const QString text = QApplication::clipboard()->text();
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")),
                                         Qt::SkipEmptyParts);
    PTTransitionMode mode = PTTransitionMode::SweepOnly;
    if (table == m_continuousTable)
        mode = PTTransitionMode::Continuous;
    else if (table == m_multiFxTable)
        mode = PTTransitionMode::MultiFx;
    for (const QString& line : lines)
    {
        QStringList cells = line.split(QLatin1Char('\t'));
        for (int i = 0; i < cells.size(); ++i)
        {
            const int col = startCol + i;
            if (col >= ColCount)
                break;
            setEditorValue(table, item, col, cells.at(i));
            slotPresetChanged(mode,
                              item->data(0, kItemPresetIndexRole).toInt(),
                              col,
                              item->data(0, kItemOutputIndexRole).toInt());
        }
        item = table->itemBelow(item);
        if (!item)
            break;
    }
}

bool PresetTableV2TransitionWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        QTreeWidget* table = nullptr;
        QWidget* watchedWidget = qobject_cast<QWidget*>(watched);
        for (QTreeWidget* candidate : { m_sweepTable, m_continuousTable, m_multiFxTable })
        {
            if (watched == candidate || watched == candidate->viewport()
                    || (watchedWidget && candidate && candidate->isAncestorOf(watchedWidget)))
            {
                table = candidate;
                break;
            }
        }
        if (!table)
        {
            const QVector<QPair<QTreeView*, QTreeWidget*>> frozenViews = {
                { m_sweepNameView, m_sweepTable },
                { m_continuousNameView, m_continuousTable },
                { m_multiFxNameView, m_multiFxTable }
            };
            for (const auto& pair : frozenViews)
            {
                if (watched == pair.first || (pair.first && watched == pair.first->viewport()))
                {
                    table = pair.second;
                    break;
                }
            }
        }
        if (table)
        {
            if (key->matches(QKeySequence::Copy))
            {
                copySelectionToClipboard(table);
                return true;
            }
            if (key->matches(QKeySequence::Paste))
            {
                pasteClipboardToSelection(table);
                return true;
            }
        }
    }
    return VCWidget::eventFilter(watched, event);
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

    rebuildAllPresetTables();
    updateEffectPreview();
}

int PresetTableV2TransitionWidget::transitionPresetCount(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxPresets.size();
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
    return effectiveTransitionPresetForOutput(mode, index, -1);
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveTransitionPresetForOutput(
        PTTransitionMode mode, int index, int outputIdx) const
{
    QHash<quint8, uchar> live;
    {
        QMutexLocker lk(&m_liveMutex);
        live = m_liveColumnOverrides;
    }
    const PTTransitionPreset base = outputIdx >= 0
            ? effectivePresetForOutputNoLive(mode, index, outputIdx)
            : transitionPreset(mode, index);
    PTTransitionPreset p = PresetTableV2SpatialEngine::mergePreset(base, live);
    p.playbackMode = (mode == PTTransitionMode::Continuous || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    PTDimmerWaveEngine::clampOffsetStep(p, gridSpanForPreset(p));
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
        PTEfxCol::InputGlobalIntensity,
        PTEfxCol::InputCrossfadeManual
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

        QMutexLocker lk(&m_liveMutex);
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
    if (m_multiFxTable)
        m_multiFxTable->setEnabled(enabled);
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
    setInputSource(dlg.globalCrossfadeManualInputSource(), PTEfxCol::InputCrossfadeManual);
    m_crossfadeManualInputMapped = dlg.globalCrossfadeManualInputSource()
            && dlg.globalCrossfadeManualInputSource()->isValid();
    updateGlobalSummaryLabel();
    updateEffectPreview();

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
    copy->m_multiFxPresets = m_multiFxPresets;
    copy->m_sweepOutputOverrides = m_sweepOutputOverrides;
    copy->m_continuousOutputOverrides = m_continuousOutputOverrides;
    copy->m_multiFxOutputOverrides = m_multiFxOutputOverrides;
    copy->m_customCurveGallery = m_customCurveGallery;
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
    copy->setInputSource(inputSource(PTEfxCol::InputCrossfadeManual), PTEfxCol::InputCrossfadeManual);
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
    if (pattrs.hasAttribute(KXMLPresetCustomCurveEnabled))
        p.customCurveEnabled = pattrs.value(KXMLPresetCustomCurveEnabled).toInt() != 0;
    if (pattrs.hasAttribute(KXMLPresetCustomCurve))
        p.customCurve = parseCustomCurve(pattrs.value(KXMLPresetCustomCurve).toString());
    if (p.customCurveEnabled && p.customCurve.size() < 2)
        p.customCurve = defaultTransitionCustomCurve();
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
        p.playbackMode = (m == int(PTTransitionMode::Continuous) || m == int(PTTransitionMode::MultiFx))
                ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    }
    if (pattrs.hasAttribute(KXMLPresetSpeedMult))
        p.speedMultiplier = qBound(0, pattrs.value(KXMLPresetSpeedMult).toInt(), 5);
    else
        p.speedMultiplier = qBound(0, legacySpeedMult, 5);

    return true;
}

void PresetTableV2TransitionWidget::readOutputOverride(PTTransitionPresetOverride& ov,
                                                       const QXmlStreamAttributes& attrs,
                                                       const PTTransitionPreset& base)
{
    ov.values = base;
    ov.columns.clear();
    for (int col = ColAxis; col < ColCount; ++col)
    {
        const QString attr = presetColumnXmlName(col);
        if (attr.isEmpty() || !attrs.hasAttribute(attr))
            continue;
        if (col == ColAxis)
            ov.values.axis = PresetTableV2SpatialEngine::axisFromString(attrs.value(attr).toString());
        else if (col == ColOffsetDir)
            ov.values.offsetDirection = PresetTableV2SpatialEngine::offsetDirectionFromString(
                    attrs.value(attr).toString());
        else if (col == ColPropagation)
            ov.values.propagation = (attrs.value(attr).toInt() == 0)
                    ? PTPropagationMode::Parallel : PTPropagationMode::Serial;
        else
            setPresetColumnValue(ov.values, col, attrs.value(attr).toInt());
        ov.columns.insert(col);
    }
    if (attrs.hasAttribute(KXMLPresetCustomCurveEnabled))
        ov.values.customCurveEnabled = attrs.value(KXMLPresetCustomCurveEnabled).toInt() != 0;
    if (attrs.hasAttribute(KXMLPresetCustomCurve))
        ov.values.customCurve = parseCustomCurve(attrs.value(KXMLPresetCustomCurve).toString());
}

void PresetTableV2TransitionWidget::writeOutputOverrideXml(
        QXmlStreamWriter* doc, int outputIdx, const PTTransitionPresetOverride& ov) const
{
    if (ov.columns.isEmpty())
        return;
    doc->writeStartElement(KXMLOutputOverride);
    doc->writeAttribute(KXMLOutputOverrideIndex, QString::number(outputIdx));
    for (int col : ov.columns)
    {
        const QString attr = presetColumnXmlName(col);
        if (attr.isEmpty())
            continue;
        if (col == ColAxis)
            doc->writeAttribute(attr, PresetTableV2SpatialEngine::axisToString(ov.values.axis));
        else if (col == ColOffsetDir)
            doc->writeAttribute(attr,
                    PresetTableV2SpatialEngine::offsetDirectionToString(ov.values.offsetDirection));
        else if (col == ColPropagation)
            doc->writeAttribute(attr, QString::number(int(ov.values.propagation)));
        else
            doc->writeAttribute(attr, presetColumnValue(ov.values, col).toString());
    }
    if (ov.columns.contains(ColWaveShape) && ov.values.customCurveEnabled)
    {
        doc->writeAttribute(KXMLPresetCustomCurveEnabled, QStringLiteral("1"));
        doc->writeAttribute(KXMLPresetCustomCurve, serializeCustomCurve(ov.values.customCurve));
    }
    doc->writeEndElement();
}

void PresetTableV2TransitionWidget::writePresetXml(
        QXmlStreamWriter* doc, const PTTransitionPreset& p,
        const QHash<int, PTTransitionPresetOverride>& overrides) const
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
    if (p.customCurveEnabled)
    {
        doc->writeAttribute(KXMLPresetCustomCurveEnabled, QStringLiteral("1"));
        doc->writeAttribute(KXMLPresetCustomCurve, serializeCustomCurve(p.customCurve));
    }
    doc->writeAttribute(KXMLPresetFadeIn, QString::number(p.waveFadeIn));
    doc->writeAttribute(KXMLPresetFadeOut, QString::number(p.waveFadeOut));
    doc->writeAttribute(KXMLPresetWaveLevel, QString::number(p.waveLevel));
    doc->writeAttribute(KXMLPresetStartOffset, QString::number(p.startOffset));
    doc->writeAttribute(KXMLPresetPropagation, QString::number(int(p.propagation)));
    doc->writeAttribute(KXMLPresetSpeedMult, QString::number(p.speedMultiplier));
    const QList<int> outputIndexes = overrides.keys();
    for (int outputIdx : outputIndexes)
        writeOutputOverrideXml(doc, outputIdx, overrides.value(outputIdx));
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
    m_multiFxPresets.clear();
    m_sweepOutputOverrides.clear();
    m_continuousOutputOverrides.clear();
    m_multiFxOutputOverrides.clear();
    m_customCurveGallery.clear();
    int legacySweepDir = 0;
    int legacySpeedMult = 1;

    auto finalizePreset = [&](PTTransitionPreset& p, PTTransitionMode bankHint,
                              const QXmlStreamAttributes& pattrs) {
        if (!pattrs.hasAttribute(KXMLPresetPlaybackMode))
        {
            if (bankHint == PTTransitionMode::Continuous)
                p.playbackMode = PTTransitionMode::Continuous;
            else if (bankHint == PTTransitionMode::MultiFx)
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
        else if (bankHint == PTTransitionMode::MultiFx)
            stored.playbackMode = PTTransitionMode::Continuous;
        else if (bankHint == PTTransitionMode::SweepOnly)
            stored.playbackMode = PTTransitionMode::SweepOnly;

        if (bankHint == PTTransitionMode::MultiFx)
            m_multiFxPresets.append(stored);
        else if (stored.playbackMode == PTTransitionMode::Continuous)
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
        else if (root.name() == KXMLGlobalCrossfadeManualInput)
            loadXMLSources(root, PTEfxCol::InputCrossfadeManual);
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
                    QHash<int, PTTransitionPresetOverride> presetOverrides;
                    while (root.readNextStartElement())
                    {
                        if (root.name() == KXMLOutputOverride)
                        {
                            const auto attrs = root.attributes();
                            bool ok = false;
                            const int outputIdx = attrs.value(KXMLOutputOverrideIndex).toInt(&ok);
                            if (!ok)
                                root.skipCurrentElement();
                            if (!ok)
                                continue;
                            if (outputIdx >= 0)
                            {
                                PTTransitionPresetOverride ov;
                                readOutputOverride(ov, attrs, p);
                                if (!ov.columns.isEmpty())
                                    presetOverrides.insert(outputIdx, ov);
                            }
                            root.skipCurrentElement();
                        }
                        else
                            root.skipCurrentElement();
                    }
                    m_sweepPresets.append(p);
                    m_sweepOutputOverrides.append(presetOverrides);
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
                    QHash<int, PTTransitionPresetOverride> presetOverrides;
                    while (root.readNextStartElement())
                    {
                        if (root.name() == KXMLOutputOverride)
                        {
                            const auto attrs = root.attributes();
                            bool ok = false;
                            const int outputIdx = attrs.value(KXMLOutputOverrideIndex).toInt(&ok);
                            if (!ok)
                                root.skipCurrentElement();
                            if (!ok)
                                continue;
                            if (outputIdx >= 0)
                            {
                                PTTransitionPresetOverride ov;
                                readOutputOverride(ov, attrs, p);
                                if (!ov.columns.isEmpty())
                                    presetOverrides.insert(outputIdx, ov);
                            }
                            root.skipCurrentElement();
                        }
                        else
                            root.skipCurrentElement();
                    }
                    m_continuousPresets.append(p);
                    m_continuousOutputOverrides.append(presetOverrides);
                }
                else
                    root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLMultiFxPresets)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLPreset)
                {
                    const auto pattrs = root.attributes();
                    PTTransitionPreset p;
                    readPresetAttrs(p, pattrs, legacySpeedMult);
                    finalizePreset(p, PTTransitionMode::MultiFx, pattrs);
                    QHash<int, PTTransitionPresetOverride> presetOverrides;
                    while (root.readNextStartElement())
                    {
                        if (root.name() == KXMLOutputOverride)
                        {
                            const auto attrs = root.attributes();
                            bool ok = false;
                            const int outputIdx = attrs.value(KXMLOutputOverrideIndex).toInt(&ok);
                            if (!ok)
                                root.skipCurrentElement();
                            if (!ok)
                                continue;
                            if (outputIdx >= 0)
                            {
                                PTTransitionPresetOverride ov;
                                readOutputOverride(ov, attrs, p);
                                if (!ov.columns.isEmpty())
                                    presetOverrides.insert(outputIdx, ov);
                            }
                            root.skipCurrentElement();
                        }
                        else
                            root.skipCurrentElement();
                    }
                    m_multiFxPresets.append(p);
                    m_multiFxOutputOverrides.append(presetOverrides);
                }
                else
                    root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLCustomCurveGallery)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLCustomCurveItem)
                {
                    const auto attrs = root.attributes();
                    PTCustomCurveGalleryItem item;
                    item.name = attrs.value(KXMLCustomCurveItemName).toString();
                    item.points = parseCustomCurve(attrs.value(KXMLCustomCurveItemCurve).toString());
                    if (!item.name.isEmpty() && item.points.size() >= 2)
                        m_customCurveGallery.append(item);
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
    if (m_multiFxPresets.isEmpty())
        m_multiFxPresets.append(defaultPreset(0, PTTransitionMode::MultiFx));

    normalizeNoopOverrides();
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
        for (int r = 0; r < m_sweepTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::SweepOnly, r);
    }
    if (m_continuousTable)
    {
        for (int r = 0; r < m_continuousTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::Continuous, r);
    }
    if (m_multiFxTable)
    {
        for (int r = 0; r < m_multiFxTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::MultiFx, r);
    }
    normalizeNoopOverrides();

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
    saveInputBinding(PTEfxCol::InputCrossfadeManual, KXMLGlobalCrossfadeManualInput);

    for (int col = 1; col < ColCount; ++col)
        saveInputBinding(PTEfxCol::inputIdForColumn(col), KXMLEfxColumnInput);

    doc->writeStartElement(KXMLSweepPresets);
    for (int i = 0; i < m_sweepPresets.size(); ++i)
        writePresetXml(doc, m_sweepPresets.at(i),
                       i < m_sweepOutputOverrides.size()
                       ? m_sweepOutputOverrides.at(i)
                       : QHash<int, PTTransitionPresetOverride>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLContinuousPresets);
    for (int i = 0; i < m_continuousPresets.size(); ++i)
        writePresetXml(doc, m_continuousPresets.at(i),
                       i < m_continuousOutputOverrides.size()
                       ? m_continuousOutputOverrides.at(i)
                       : QHash<int, PTTransitionPresetOverride>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLMultiFxPresets);
    for (int i = 0; i < m_multiFxPresets.size(); ++i)
        writePresetXml(doc, m_multiFxPresets.at(i),
                       i < m_multiFxOutputOverrides.size()
                       ? m_multiFxOutputOverrides.at(i)
                       : QHash<int, PTTransitionPresetOverride>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLCustomCurveGallery);
    for (const PTCustomCurveGalleryItem& item : m_customCurveGallery)
    {
        if (item.name.isEmpty() || item.points.size() < 2)
            continue;
        doc->writeStartElement(KXMLCustomCurveItem);
        doc->writeAttribute(KXMLCustomCurveItemName, item.name);
        doc->writeAttribute(KXMLCustomCurveItemCurve, serializeCustomCurve(item.points));
        doc->writeEndElement();
    }
    doc->writeEndElement();

    doc->writeEndElement();
    return true;
}

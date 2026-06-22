/*
  QLC+ VC Widget Plugin — Preset Table v2 EFX Engine
*/

#include "presettablev2transitionwidget.h"
#include "presettablev2transitionconfigdialog.h"
#include "presettablev2transitioncolumndialog.h"
#include "ptcustomcurvedialog.h"
#include "ptpositionshapedialog.h"
#include "pttransitioncolumngroupbar.h"
#include "ptshapesgallery.h"
#include "ptpositionfxengine.h"
#include "presettablev2controliface.h"
#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"
#include "ptdimmerwavecurvewidget.h"
#include "ptpositionpathpreviewwidget.h"
#include "ptpositionmotion1dpreviewwidget.h"
#include "ptspatialfixturegridwidget.h"
#include "ptparammatrixengine.h"
#include "presettablev2vclookup.h"
#include "vcplugindiagnostics.h"
#include "ptefxinputids.h"

#include "virtualconsole.h"
#include "doc.h"

#include <QDialog>
#include <QPainter>
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
#include <QTreeWidgetItemIterator>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QMimeData>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QScopedPointer>
#include <QPointer>
#include <QMutexLocker>

#include <algorithm>
#include <functional>

static const int kPresetCellValueRole = Qt::UserRole + 20;
static const int kPresetCellInheritedRole = Qt::UserRole + 21;

struct ScopedBoolFlag
{
    bool& flag;
    bool previous = false;
    explicit ScopedBoolFlag(bool& value) : flag(value), previous(value) { flag = true; }
    ~ScopedBoolFlag() { flag = previous; }
};

static const QString KXMLRoot = QStringLiteral("PluginWidget");
static const QString KXMLPluginId = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal = QStringLiteral("org.qlcplus.vcwidgets.presettablev2transition");
static const QString KXMLTargetTable = QStringLiteral("TargetTableId");
static const QString KDefaultEngineCaption = QStringLiteral("Preset Table Engine");

static bool isDefaultPresetTableEngineCaption(const QString& caption)
{
    const QString c = caption.trimmed();
    if (c.isEmpty()
            || c == QStringLiteral("EFX Engine")
            || c == KDefaultEngineCaption
            || c == QStringLiteral("Preset Table v2 Transition"))
        return true;
    return c.endsWith(QStringLiteral(" - EFX Engine"))
            || c.endsWith(QStringLiteral(" - Preset Table Engine"));
}

static QString generatedPresetTableEngineCaption(const QString& tableCaption)
{
    const QString tableName = tableCaption.trimmed().isEmpty()
            ? QStringLiteral("Preset Table v2")
            : tableCaption.trimmed();
    return tableName + QStringLiteral(" - Preset Table Engine");
}

static QString transitionBankTitle(PTTransitionMode mode)
{
    switch (mode)
    {
        case PTTransitionMode::Off:            break;
        case PTTransitionMode::SweepOnly:      return QObject::tr("Transition");
        case PTTransitionMode::Continuous:     return QObject::tr("Interpolation");
        case PTTransitionMode::Channel1D:      return QObject::tr("1D FX");
        case PTTransitionMode::PositionMotion: return QObject::tr("2D FX");
        case PTTransitionMode::MultiFx:        return QObject::tr("MultiFX");
    }
    return QObject::tr("Bank");
}

static const QString KXMLTransitionMode = QStringLiteral("TransitionMode");
static const QString KXMLBankSource = QStringLiteral("BankSource");
static const QString KXMLBankSourceMode = QStringLiteral("Mode");
static const QString KXMLBankSourceEngine = QStringLiteral("EngineId");
static const QString KXMLPreset = QStringLiteral("TransitionPreset");
static const QString KXMLSweepPresets = QStringLiteral("SweepPresets");
static const QString KXMLContinuousPresets = QStringLiteral("ContinuousPresets");
static const QString KXMLMultiFxPresets = QStringLiteral("MultiFxPresets");
static const QString KXMLMultiFxTargets = QStringLiteral("MultiFxTargets");
static const QString KXMLMultiFxTargetPreset = QStringLiteral("Preset");
static const QString KXMLMultiFxTargetTable = QStringLiteral("TargetTable");
static const QString KXMLMultiFxTargetOutput = QStringLiteral("Output");
static const QString KXMLMultiFxTargetPresetIndex = QStringLiteral("Index");
static const QString KXMLMultiFxTargetTableId = QStringLiteral("TableId");
static const QString KXMLMultiFxTargetEnabled = QStringLiteral("Enabled");
static const QString KXMLMultiFxTargetLayerKind = QStringLiteral("LayerKind");
static const QString KXMLMultiFxTargetOutputIndex = QStringLiteral("Index");
static const QString KXMLMultiFxTargetSelections = QStringLiteral("Selections");
static const QString KXMLPositionMotionPresets = QStringLiteral("PositionMotionPresets");
static const QString KXMLChannel1DPresets = QStringLiteral("Channel1DFxPresets");
static const QString KXMLPresetName = QStringLiteral("Name");
static const QString KXMLPresetAxis = QStringLiteral("Axis");
static const QString KXMLPresetOffsetDir = QStringLiteral("OffsetDir");
static const QString KXMLPresetDir = QStringLiteral("Direction");
static const QString KXMLPresetOffsetStep = QStringLiteral("OffsetStep");
static const QString KXMLPresetOffsetStepMode = QStringLiteral("OffsetStepMode");
static const QString KXMLPresetOffsetCoverage = QStringLiteral("OffsetCoverage");
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
static const QString KXMLPresetPositionMotion = QStringLiteral("PositionMotion");
static const QString KXMLPresetPositionMotionDir = QStringLiteral("PositionMotionDir");
static const QString KXMLPresetPositionPanSize = QStringLiteral("PositionPanSize");
static const QString KXMLPresetPositionTiltSize = QStringLiteral("PositionTiltSize");
static const QString KXMLPresetChannel1DTarget = QStringLiteral("Channel1DTarget");
static const QString KXMLPresetChannel1DTargetMode = QStringLiteral("Channel1DTargetMode");
static const QString KXMLPresetChannel1DApplyMode = QStringLiteral("Channel1DApplyMode");
static const QString KXMLPresetChannel1DLow = QStringLiteral("Channel1DLow");
static const QString KXMLPresetChannel1DHigh = QStringLiteral("Channel1DHigh");
static const QString KXMLPresetChannel1DAmount = QStringLiteral("Channel1DAmount");
static const QString KXMLPresetChannel1DCustomColumn = QStringLiteral("Channel1DCustomColumn");
static const QString KXMLPresetMultiFxInterpolationSource = QStringLiteral("MultiFxInterpolationSource");
static const QString KXMLPresetMultiFxInterpolationPrimary = QStringLiteral("MultiFxInterpolationPrimary");
static const QString KXMLPresetMultiFxInterpolationSecondary = QStringLiteral("MultiFxInterpolationSecondary");
static const QString KXMLPresetCustomCurveEnabled = QStringLiteral("CustomCurveEnabled");
static const QString KXMLPresetCustomCurve = QStringLiteral("CustomCurve");
static const QString KXMLOutputOverride = QStringLiteral("OutputOverride");
static const QString KXMLOutputOverrideIndex = QStringLiteral("Output");
static const QString KXMLSelection = QStringLiteral("Selection");
static const QString KXMLSelectionName = QStringLiteral("Name");
static const QString KXMLSelectionCells = QStringLiteral("Cells");
static const QString KXMLCustomCurveGallery = QStringLiteral("CustomCurveGallery");
static const QString KXMLShapeGallery = QStringLiteral("ShapeGallery");
static const QString KXMLShapeGalleryItem = QStringLiteral("ShapeGalleryItem");
static const QString KXMLShapeGalleryKind = QStringLiteral("Kind");
static const QString KXMLShapeGalleryPath2DClosed = QStringLiteral("Path2DClosed");
static const QString KXMLPresetPositionMotionCurve = QStringLiteral("PositionMotionCurve");
static const QString KXMLPresetPositionMotionCurveEnabled = QStringLiteral("PositionMotionCurveEnabled");
static const QString KXMLPresetPositionMotionWaveShape = QStringLiteral("PositionMotionWaveShape");
static const QString KXMLPresetPosition1DBuiltinMode = QStringLiteral("Position1DBuiltinMode");

static void migrateLegacyPositionMotionFields(PTTransitionPreset& p,
                                              int legacyMotionWaveShape = -1,
                                              bool legacyMotionCurveEnabled = false,
                                              const QVector<PTCustomCurvePoint>& legacyMotionCurve = {})
{
    if (legacyMotionCurve.size() >= 2
            && (legacyMotionCurveEnabled || legacyMotionWaveShape == 3))
    {
        if (!p.customCurveEnabled || p.customCurve.size() < 2)
        {
            p.customCurve = legacyMotionCurve;
            p.customCurveEnabled = true;
        }
    }
    else if (legacyMotionWaveShape >= 0 && legacyMotionWaveShape < 3 && !p.customCurveEnabled)
    {
        p.waveShape = legacyMotionWaveShape;
    }

    if (p.positionMotion == int(PTPositionMotion::CustomPan1D))
    {
        p.positionMotion = int(PTPositionMotion::Pan1D);
        p.customCurveEnabled = true;
    }
    else if (p.positionMotion == int(PTPositionMotion::CustomTilt1D))
    {
        p.positionMotion = int(PTPositionMotion::Tilt1D);
        p.customCurveEnabled = true;
    }
}
static const QString KXMLPresetPositionPath2D = QStringLiteral("PositionPath2D");
static const QString KXMLPresetPositionPath2DClosed = QStringLiteral("PositionPath2DClosed");
static const QString KXMLCustomCurveItem = QStringLiteral("CustomCurveItem");
static const QString KXMLCustomCurveItemName = QStringLiteral("Name");
static const QString KXMLCustomCurveItemCurve = QStringLiteral("Curve");
static const QString KXMLGlobalSpeedInput = QStringLiteral("GlobalSpeedInput");
static const QString KXMLGlobalIntensityInput = QStringLiteral("GlobalIntensityInput");
static const QString KXMLGlobalPositionSizeInput = QStringLiteral("GlobalPositionSizeInput");
static const QString KXMLGlobalCrossfadeManualInput = QStringLiteral("GlobalCrossfadeManualInput");
static const QString KXMLEfxColumnInput = QStringLiteral("EfxColumnInput");
static const QString KXMLInputIdAttr = QStringLiteral("InputId");
static const QString KXMLGlobalMinMsRoot = QStringLiteral("GlobalMinMs");
static const QString KXMLGlobalMaxMsRoot = QStringLiteral("GlobalMaxMs");
static const QString KXMLGlobal = QStringLiteral("GlobalEffect");
static const QString KXMLGlobalSpeed = QStringLiteral("Speed");
static const QString KXMLGlobalMinMs = QStringLiteral("MinMs");
static const QString KXMLGlobalMaxMs = QStringLiteral("MaxMs");
static const QString KXMLGlobalSizeSpeedCeiling = QStringLiteral("SizeSpeedCeiling");
static const QString KXMLGlobalSmallSizeMinMs = QStringLiteral("SmallSizeMinMs");
static const QString KXMLGlobalSpeedOverdriveKnee = QStringLiteral("SpeedOverdriveKnee");
static const QString KXMLGlobalMultiplier = QStringLiteral("Multiplier");
static const QString KXMLGlobalDirection = QStringLiteral("Direction");
static const QString KXMLGlobalIntensity = QStringLiteral("Intensity");
static const QString KXMLGlobalPositionSize = QStringLiteral("PositionSize");
static const QString KXMLGlobalBlocks = QStringLiteral("Blocks");
static const QString KXMLGlobalPhase = QStringLiteral("Phase");
static const QString KXMLGlobalSymmetry = QStringLiteral("Symmetry");
static const QString KXMLGlobalOrientation = QStringLiteral("Orientation");
static const QString KXMLGlobalFxMultiplier = QStringLiteral("FxMultiplier");

static const int kItemPresetIndexRole = Qt::UserRole + 1;
static const int kItemOutputIndexRole = Qt::UserRole + 2;
static const int kItemSelectionIndexRole = Qt::UserRole + 3; // -1 preset, 0 All/output, 1.. custom
static const int kItemMultiFxRouteIndexRole = Qt::UserRole + 4; // -1 for normal rows
static const int kItemMultiFxRouteTableIdRole = Qt::UserRole + 5;
static const int kItemMultiFxRouteOutputIndexRole = Qt::UserRole + 6;
static const int kEfxTreeRowHeight = 22;
static const int kFrozenNameWidth = 210;
static const char kMimeEfxColumn[] = "application/x-qlc-efx-col";
static const QColor kOverrideOrange(230, 126, 34);

static QColor selectionLayerColor(int selectionIndex)
{
    static const QColor colors[] = {
        QColor(70, 210, 255), QColor(255, 180, 65), QColor(120, 225, 110),
        QColor(220, 120, 255), QColor(255, 105, 145), QColor(95, 160, 255),
        QColor(240, 220, 80), QColor(95, 220, 190), QColor(255, 135, 85),
        QColor(165, 145, 255)
    };
    return colors[qAbs(selectionIndex) % (int(sizeof(colors) / sizeof(colors[0])))];
}

static PTTransitionPreset defaultPreset(int index, PTTransitionMode bankMode)
{
    PTTransitionPreset p;
    p.name = QObject::tr("Preset %1").arg(index + 1);
    p.enabled = true;
    p.playbackMode = (bankMode == PTTransitionMode::Continuous
                      || bankMode == PTTransitionMode::MultiFx
                      || bankMode == PTTransitionMode::PositionMotion
                      || bankMode == PTTransitionMode::Channel1D)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    p.offsetDirection = PTOffsetDirection::CenterToSides;
    if (bankMode == PTTransitionMode::Channel1D)
    {
        p.name = QObject::tr("1D FX %1").arg(index + 1);
        p.channel1DTarget = int(PTChannel1DTarget::Dimmer);
        p.channel1DTargetMode = int(PTChannel1DTargetMode::First);
        p.channel1DApplyMode = int(PTChannel1DApplyMode::MultiplyBase);
    }
    PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    return p;
}

static void migrateLegacyPositionOrbitFields(PTTransitionPreset& p)
{
    if (p.positionMotion != int(PTPositionMotion::Off))
        return;
    if (p.customCurveEnabled)
        return;
    if (p.waveShape < 3 || p.waveShape > 4)
        return;

    static const int motionFromLegacy[] = {
        int(PTPositionMotion::Circle2D),
        int(PTPositionMotion::Line2D),
        int(PTPositionMotion::Pan1D),
        int(PTPositionMotion::Tilt1D),
        int(PTPositionMotion::Figure8_2D)
    };
    p.positionMotion = motionFromLegacy[qBound(0, p.waveShape, 4)];
    p.positionPanSize = qMax(0, p.offsetStep);
    p.positionTiltSize = qMax(1, p.waveWidth);
    p.offsetStep = 20;
    p.waveWidth = 180;
    p.waveShape = 0;
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

static QString offsetStepModeToString(PTOffsetStepMode mode)
{
    switch (mode)
    {
        case PTOffsetStepMode::Off:             return QStringLiteral("Off");
        case PTOffsetStepMode::AutoFit:         return QStringLiteral("AutoFit");
        case PTOffsetStepMode::CoveragePercent: return QStringLiteral("Coverage");
        case PTOffsetStepMode::FixedDegrees:    return QStringLiteral("Fixed");
    }
    return QStringLiteral("Fixed");
}

static PTOffsetStepMode offsetStepModeFromString(const QString& value)
{
    const QString s = value.trimmed();
    bool ok = false;
    const int numeric = s.toInt(&ok);
    if (ok)
        return PTOffsetStepMode(qBound(0, numeric, int(PTOffsetStepMode::FixedDegrees)));
    if (s.compare(QStringLiteral("Off"), Qt::CaseInsensitive) == 0)
        return PTOffsetStepMode::Off;
    if (s.compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0
            || s.compare(QStringLiteral("AutoFit"), Qt::CaseInsensitive) == 0)
        return PTOffsetStepMode::AutoFit;
    if (s.compare(QStringLiteral("Coverage"), Qt::CaseInsensitive) == 0
            || s.compare(QStringLiteral("CoveragePercent"), Qt::CaseInsensitive) == 0)
        return PTOffsetStepMode::CoveragePercent;
    return PTOffsetStepMode::FixedDegrees;
}

static QString serializeSelectionCells(const QVector<QLCPoint>& cells)
{
    QStringList parts;
    for (const QLCPoint& pt : cells)
        parts << QStringLiteral("%1,%2").arg(pt.x()).arg(pt.y());
    return parts.join(QLatin1Char(';'));
}

static QVector<QLCPoint> parseSelectionCells(const QString& encoded)
{
    QVector<QLCPoint> cells;
    QSet<QLCPoint> seen;
    for (const QString& part : encoded.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        const QStringList xy = part.split(QLatin1Char(','));
        if (xy.size() != 2)
            continue;
        bool okX = false;
        bool okY = false;
        const int x = xy.at(0).trimmed().toInt(&okX);
        const int y = xy.at(1).trimmed().toInt(&okY);
        if (!okX || !okY)
            continue;
        const QLCPoint pt(x, y);
        if (seen.contains(pt))
            continue;
        seen.insert(pt);
        cells.append(pt);
    }
    return cells;
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

static bool isLegacyChannel1DApplyMode(int value)
{
    return value == int(PTChannel1DApplyMode::AbsoluteRange)
            || value == int(PTChannel1DApplyMode::RelativeAroundBase);
}

static bool isLegacyWaveShape(int value)
{
    return value == 1;
}

PresetTableV2TransitionDelegate::PresetTableV2TransitionDelegate(
        PresetTableV2TransitionWidget* owner, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_owner(owner)
{
}

QWidget* PresetTableV2TransitionDelegate::createEditor(
        QWidget* parent, const QStyleOptionViewItem& option,
        const QModelIndex& index) const
{
    if (!m_owner || !index.isValid() || index.column() <= 0)
        return QStyledItemDelegate::createEditor(parent, option, index);
    auto* tree = qobject_cast<QTreeWidget*>(index.model() ? index.model()->parent() : nullptr);
    QTreeWidgetItem* item = tree ? tree->itemFromIndex(index) : nullptr;
    const PTTransitionMode mode = tree ? m_owner->modeForTable(tree) : PTTransitionMode::Off;
    if (item && mode == PTTransitionMode::MultiFx)
    {
        const bool rootContext = !item->data(0, kItemMultiFxRouteIndexRole).isValid();
        const PTTransitionMode contextMode = m_owner->multiFxContextModeForItem(item);
        if (!m_owner->allowedMultiFxColumnsForContext(contextMode, rootContext)
                .contains(index.column()))
            return nullptr;
        QWidget* editor = m_owner->createEditorForItemColumn(parent, item, index.column());
        if (index.column() == PresetTableV2TransitionWidget::ColMultiFxTargetMode)
        {
            if (auto* combo = qobject_cast<QComboBox*>(editor))
            {
                auto* self = const_cast<PresetTableV2TransitionDelegate*>(this);
                connect(combo, QOverload<int>::of(&QComboBox::activated),
                        self, [self, combo]() {
                    emit self->commitData(combo);
                    emit self->closeEditor(combo, QAbstractItemDelegate::NoHint);
                });
            }
        }
        return editor;
    }
    else if (!m_owner->isPresetColumnAllowedForMode(mode, index.column()))
    {
        return nullptr;
    }
    return m_owner->createEditorForColumn(parent, index.column());
}

void PresetTableV2TransitionDelegate::setEditorData(QWidget* editor,
                                                    const QModelIndex& index) const
{
    const QVariant value = index.data(kPresetCellValueRole);
    if (auto* combo = qobject_cast<QComboBox*>(editor))
    {
        if (m_owner && index.column() == PresetTableV2TransitionWidget::ColWaveShape
                && combo->findData(value.toInt()) < 0
                && isLegacyWaveShape(value.toInt()))
        {
            combo->addItem(m_owner->comboDisplayTextForColumn(index.column(), value), value.toInt());
        }
        if (m_owner && index.column() == PresetTableV2TransitionWidget::ColChannel1DApplyMode
                && combo->findData(value.toInt()) < 0
                && isLegacyChannel1DApplyMode(value.toInt()))
        {
            combo->addItem(m_owner->comboDisplayTextForColumn(index.column(), value), value.toInt());
        }
        setComboDataIndex(combo, value.toInt());
        return;
    }
    if (auto* spin = qobject_cast<QSpinBox*>(editor))
    {
        if (index.column() == PresetTableV2TransitionWidget::ColOffsetStep)
        {
            const int mode = index.siblingAtColumn(PresetTableV2TransitionWidget::ColOffsetStepMode)
                    .data(kPresetCellValueRole).toInt();
            spin->setSuffix(mode == int(PTOffsetStepMode::CoveragePercent)
                            ? QObject::tr("%") : QObject::tr("°"));
            spin->setMaximum(mode == int(PTOffsetStepMode::CoveragePercent) ? 100 : 360);
        }
        spin->setValue(qBound(spin->minimum(), value.toInt(), spin->maximum()));
        return;
    }
    QStyledItemDelegate::setEditorData(editor, index);
}

void PresetTableV2TransitionDelegate::setModelData(QWidget* editor,
                                                   QAbstractItemModel* model,
                                                   const QModelIndex& index) const
{
    if (!m_owner || !index.isValid()
            || m_owner->m_rebuildingTable
            || m_owner->m_closingTableEditors
            || m_owner->m_editingCustomCurve
            || m_owner->m_committingCustomDialog)
        return;

    QVariant value;
    if (auto* combo = qobject_cast<QComboBox*>(editor))
        value = combo->currentData();
    else if (auto* spin = qobject_cast<QSpinBox*>(editor))
        value = spin->value();
    else
        value = editor->property("text");

    auto* tree = qobject_cast<QTreeWidget*>(model->parent());
    if (!tree)
        return;

    QTreeWidgetItem* item = tree->itemFromIndex(index);
    const int col = index.column();
    if (!item || col <= PresetTableV2TransitionWidget::ColName
            || col >= PresetTableV2TransitionWidget::ColCount)
        return;

    const PTTransitionMode mode = m_owner->modeForTable(tree);
    PTTransitionCellAddress address;
    address.row = item->data(0, kItemPresetIndexRole).toInt();
    address.outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    address.selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    address.multiFxRouteIdx = item->data(0, kItemMultiFxRouteIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteIndexRole).toInt() : -1;
    address.multiFxRouteOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
    address.col = col;
    QTreeWidgetItem* expectedItem = nullptr;
    if (mode == PTTransitionMode::MultiFx && address.multiFxRouteIdx >= 0)
    {
        expectedItem = m_owner->itemForMultiFxRouteAddress(
                address.row, address.multiFxRouteIdx,
                address.multiFxRouteOutputIdx, address.selectionIdx);
    }
    else
    {
        expectedItem = m_owner->itemForPresetAddress(mode, address.row,
                                                     address.outputIdx,
                                                     address.selectionIdx);
    }
    if (expectedItem != item)
    {
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), m_owner->id(), m_owner->caption(),
                QStringLiteral("editor delegate stale index ignored mode=%1 row=%2 output=%3 selection=%4 col=%5")
                        .arg(int(mode)).arg(address.row).arg(address.outputIdx)
                        .arg(address.selectionIdx).arg(PresetTableV2TransitionWidget::presetColumnXmlName(col)));
        return;
    }

    ScopedBoolFlag delegateGuard(m_owner->m_committingDelegateEditor);
    QSignalBlocker tableBlocker(tree);
    const bool inherited = item->data(col, kPresetCellInheritedRole).toBool();
    m_owner->setPresetCellValue(item, col, value, inherited);
    m_owner->commitPresetCellEdit(tree, address, value);
}

void PresetTableV2TransitionDelegate::updateEditorGeometry(
        QWidget* editor, const QStyleOptionViewItem& option,
        const QModelIndex& /*index*/) const
{
    if (editor)
        editor->setGeometry(option.rect);
}

void PresetTableV2TransitionDelegate::paint(QPainter* painter,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
    if (!painter || !index.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    bool nameRowContext = false;
    if (m_owner && option.widget)
    {
        QTreeWidget* table = qobject_cast<QTreeWidget*>(const_cast<QWidget*>(option.widget));
        if (!table)
            table = qobject_cast<QTreeWidget*>(const_cast<QWidget*>(option.widget)->parentWidget());
        if (table)
        {
            if (QTreeWidgetItem* item = table->itemFromIndex(index))
                nameRowContext = m_owner->isNameRowContextItem(table, item);
        }
    }

    const bool selected = (opt.state & QStyle::State_Selected) || nameRowContext;
    const bool current = opt.state & QStyle::State_HasFocus;
    const bool inherited = index.data(kPresetCellInheritedRole).toBool();

    painter->save();
    painter->setClipRect(opt.rect);

    if (selected)
        painter->fillRect(opt.rect, opt.palette.highlight());
    else
        painter->fillRect(opt.rect, opt.backgroundBrush);

    QFont font = opt.font;
    font.setItalic(inherited);
    painter->setFont(font);

    QColor textColor;
    if (selected)
        textColor = opt.palette.color(QPalette::HighlightedText);
    else
        textColor = qvariant_cast<QBrush>(index.data(Qt::ForegroundRole)).color();
    if (!textColor.isValid())
        textColor = opt.palette.color(QPalette::Text);

    painter->setPen(textColor);
    painter->drawText(opt.rect.adjusted(6, 0, -4, 0),
                      Qt::AlignVCenter | Qt::AlignLeft,
                      opt.text);

    if (current)
    {
        QPen pen(opt.palette.color(QPalette::Highlight), 2);
        painter->setPen(pen);
        painter->drawRect(opt.rect.adjusted(1, 1, -2, -2));
    }

    painter->restore();
}

static QString transitionCellStyleSheet(const QString& colorRule)
{
    return QStringLiteral(
        "QSpinBox, QComboBox { background: transparent; border: none; padding: 0 2px; %1 }"
        "QSpinBox::up-button, QSpinBox::down-button { width: 0; height: 0; border: none; }"
        "QComboBox::drop-down { border: none; width: 14px; }").arg(colorRule);
}

static void styleTransitionCellEditor(QWidget* widget)
{
    if (auto* spin = qobject_cast<QSpinBox*>(widget))
    {
        spin->setFrame(false);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    }
    else if (auto* combo = qobject_cast<QComboBox*>(widget))
    {
        combo->setFrame(false);
    }
}

static void styleTransitionEditorWidget(QWidget* widget, bool inherited,
                                        bool parentHasOutputOverride = false,
                                        bool selected = false)
{
    if (!widget)
        return;

    styleTransitionCellEditor(widget);

    QFont f = widget->font();
    f.setItalic(inherited);
    f.setBold(false);
    widget->setFont(f);

    QString colorRule;
    if (parentHasOutputOverride)
    {
        colorRule = QStringLiteral("color: rgb(%1, %2, %3);")
                            .arg(kOverrideOrange.red())
                            .arg(kOverrideOrange.green())
                            .arg(kOverrideOrange.blue());
        widget->setToolTip(QObject::tr("One or more outputs override this parameter"));
    }
    else
    {
        colorRule = inherited
                ? QStringLiteral("color: rgba(220, 220, 220, 155);")
                : QStringLiteral("color: palette(text);");
        widget->setToolTip(inherited ? QObject::tr("Inherited from parent preset")
                                     : QObject::tr("Override"));
    }
    Q_UNUSED(selected)
    colorRule += QStringLiteral(" border: none;");
    widget->setStyleSheet(transitionCellStyleSheet(colorRule));
}

void PresetTableV2TransitionWidget::setPresetCellValue(QTreeWidgetItem* item, int col,
                                                       const QVariant& value, bool inherited,
                                                       bool parentHasOutputOverride,
                                                       bool selected)
{
    if (!item || col <= ColName || col >= ColCount)
        return;

    ScopedBoolFlag commitGuard(m_committingPresetCell);
    QScopedPointer<QSignalBlocker> blocker;
    if (item->treeWidget())
        blocker.reset(new QSignalBlocker(item->treeWidget()));

    item->setData(col, kPresetCellValueRole, value);
    item->setData(col, kPresetCellInheritedRole, inherited);
    QString display = displayTextForColumn(col, value);
    if (col == ColOffsetStep)
    {
        const PTOffsetStepMode mode = PTOffsetStepMode(
                item->data(ColOffsetStepMode, kPresetCellValueRole).toInt());
        switch (mode)
        {
            case PTOffsetStepMode::Off:
                display = tr("Off");
                break;
            case PTOffsetStepMode::AutoFit:
                display = tr("Auto");
                break;
            case PTOffsetStepMode::CoveragePercent:
                display = tr("%1%").arg(value.toInt());
                break;
            case PTOffsetStepMode::FixedDegrees:
                display = tr("%1°").arg(value.toInt());
                break;
        }
    }
    item->setText(col, display);

    QFont f = item->font(col);
    f.setItalic(inherited);
    f.setBold(false);
    item->setFont(col, f);

    if (parentHasOutputOverride)
    {
        item->setForeground(col, QBrush(kOverrideOrange));
        item->setToolTip(col, tr("One or more outputs override this parameter"));
    }
    else
    {
        QColor color = palette().color(QPalette::Text);
        if (inherited)
            color.setAlpha(155);
        item->setForeground(col, QBrush(color));
        item->setToolTip(col, inherited ? tr("Inherited from parent preset") : tr("Override"));
    }

    Q_UNUSED(selected)
    item->setBackground(col, QBrush());
}

void PresetTableV2TransitionWidget::closeActiveTableEditors(QTreeWidget* table)
{
    if (!table)
        return;

    if (m_closingTableEditors)
        return;

    ScopedBoolFlag closingGuard(m_closingTableEditors);
    const QAbstractItemView::EditTriggers triggers = table->editTriggers();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    const QModelIndex current = table->currentIndex();
    int row = -1;
    int outputIdx = -1;
    int selectionIdx = -1;
    int col = -1;
    if (current.isValid())
    {
        col = current.column();
        if (QTreeWidgetItem* item = table->itemFromIndex(current))
        {
            row = item->data(0, kItemPresetIndexRole).toInt();
            outputIdx = item->data(0, kItemOutputIndexRole).toInt();
            selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
            if (col > ColName && col < ColCount)
            {
                VCPluginDiagnostics::breadcrumb(
                        QStringLiteral("presettablev2transition"), id(), caption(),
                        QStringLiteral("editor close begin mode=%1 row=%2 output=%3 selection=%4 col=%5")
                                .arg(int(modeForTable(table))).arg(row).arg(outputIdx)
                                .arg(selectionIdx).arg(presetColumnXmlName(col)));
                table->closePersistentEditor(item, col);
            }
        }
    }

    QWidget* fw = QApplication::focusWidget();
    while (fw)
    {
        if (fw == table || fw == table->viewport())
            break;
        if (table->isAncestorOf(fw)
                && (qobject_cast<QComboBox*>(fw) || qobject_cast<QSpinBox*>(fw)))
        {
            fw->clearFocus();
            break;
        }
        fw = fw->parentWidget();
    }

    table->clearFocus();
    table->setCurrentIndex(QModelIndex());
    if (QWidget* window = table->window())
        window->setFocus(Qt::OtherFocusReason);

    table->setEditTriggers(triggers);

    if (col > ColName && col < ColCount)
    {
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("editor close end mode=%1 row=%2 output=%3 selection=%4 col=%5")
                        .arg(int(modeForTable(table))).arg(row).arg(outputIdx)
                        .arg(selectionIdx).arg(presetColumnXmlName(col)));
    }
}

void PresetTableV2TransitionWidget::commitPresetCellEdit(QTreeWidget* table,
                                                         const PTTransitionCellAddress& address,
                                                         const QVariant& value)
{
    if (m_rebuildingTable || m_committingPresetCell || m_editingCustomCurve
            || m_committingCustomDialog
            || m_closingTableEditors
            || !table)
        return;
    if (address.col <= ColName || address.col >= ColCount)
        return;

    const PTTransitionMode mode = modeForTable(table);
    QTreeWidgetItem* item = (mode == PTTransitionMode::MultiFx
                             && address.multiFxRouteIdx >= 0)
            ? itemForMultiFxRouteAddress(address.row, address.multiFxRouteIdx,
                                         address.multiFxRouteOutputIdx,
                                         address.selectionIdx)
            : itemForPresetAddress(mode, address.row,
                                   address.outputIdx,
                                   address.selectionIdx);
    if (mode == PTTransitionMode::MultiFx && address.col == ColMultiFxTargetMode)
    {
        if (address.row < 0 || address.multiFxRouteIdx < 0)
            return;
        normalizeMultiFxTargetRoutes();
        if (address.row >= m_multiFxTargetRoutes.size()
                || address.multiFxRouteIdx >= m_multiFxTargetRoutes.at(address.row).size())
            return;

        PTMultiFxTargetTableRoute& route =
                m_multiFxTargetRoutes[address.row][address.multiFxRouteIdx];
        const int oldKind = route.layerKind;
        if (value.toInt() == 1)
        {
            route.layerKind = int(PTMultiFxTargetLayerKind::Interpolation);
        }
        else
        {
            route.layerKind = targetTableUsesPositionMode(route.tableId)
                    ? int(PTMultiFxTargetLayerKind::PositionMotion)
                    : int(PTMultiFxTargetLayerKind::Channel1D);
        }
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("multifx route mode commit row=%1 route=%2 output=%3 selection=%4 table=%5 old=%6 new=%7")
                        .arg(address.row).arg(address.multiFxRouteIdx)
                        .arg(address.multiFxRouteOutputIdx).arg(address.selectionIdx)
                        .arg(route.tableId).arg(oldKind).arg(route.layerKind));
        publishProviderSnapshot(QStringLiteral("multifx route mode"));
        rebuildPresetTable(mode);
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, address.row))
        {
            parent->setExpanded(true);
            if (QTreeWidgetItem* routeItem = itemForMultiFxRouteAddress(
                        address.row, address.multiFxRouteIdx, -1, -1))
            {
                routeItem->setExpanded(true);
                if (address.multiFxRouteOutputIdx >= 0)
                {
                    if (QTreeWidgetItem* outputItem = itemForMultiFxRouteAddress(
                                address.row, address.multiFxRouteIdx,
                                address.multiFxRouteOutputIdx, 0))
                        outputItem->setExpanded(true);
                }
            }
        }
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }
    if (mode == PTTransitionMode::MultiFx)
    {
        const bool rootContext = !item
                || !item->data(0, kItemMultiFxRouteIndexRole).isValid();
        const PTTransitionMode contextMode = multiFxContextModeForItem(item);
        if (!allowedMultiFxColumnsForContext(contextMode, rootContext)
                .contains(address.col))
            return;
    }
    else if (!isPresetColumnAllowedForMode(mode, address.col))
    {
        return;
    }
    if (mode == PTTransitionMode::SweepOnly
            && (address.col == ColOffsetStepMode || address.col == ColOffsetStep))
    {
        QTreeWidgetItem* item = itemForPresetAddress(mode, address.row,
                                                     address.outputIdx,
                                                     address.selectionIdx);
        if (item)
        {
            const PTTransitionPreset effective = address.selectionIdx > 0
                    ? effectivePresetForSelectionNoLive(mode, address.row, address.outputIdx,
                                                        address.selectionIdx - 1)
                    : (address.outputIdx >= 0
                       ? effectivePresetForOutputNoLive(mode, address.row, address.outputIdx)
                       : presetsForMode(mode).value(address.row));
            QSignalBlocker blocker(table);
            setPresetCellValue(item, ColOffsetStepMode,
                               presetColumnValue(effective, ColOffsetStepMode), true);
            setPresetCellValue(item, ColOffsetStep,
                               presetColumnValue(effective, ColOffsetStep), true);
            item->setToolTip(ColOffsetStepMode,
                             tr("Transition spread is locked to Auto Fit 100%."));
            item->setToolTip(ColOffsetStep,
                             tr("Transition spread is locked to Auto Fit 100%."));
        }
        return;
    }

    ScopedBoolFlag guard(m_committingPresetCell);
    if (item)
    {
        QSignalBlocker blocker(table);
        setPresetCellValue(item, address.col, value,
                           item->data(address.col, kPresetCellInheritedRole).toBool());
    }

    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("transition parameter commit begin mode=%1 row=%2 output=%3 selection=%4 col=%5 value=%6 item=%7 delegate=%8")
                    .arg(int(mode)).arg(address.row).arg(address.outputIdx)
                    .arg(address.selectionIdx)
                    .arg(presetColumnXmlName(address.col), value.toString())
                    .arg(item ? 1 : 0).arg(m_committingDelegateEditor ? 1 : 0));
    slotPresetChanged(mode, address.row, address.col,
                      address.outputIdx, address.selectionIdx,
                      address.multiFxRouteIdx,
                      address.multiFxRouteOutputIdx);
    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("transition parameter commit end mode=%1 row=%2 output=%3 selection=%4 col=%5 value=%6")
                    .arg(int(mode)).arg(address.row).arg(address.outputIdx)
                    .arg(address.selectionIdx)
                    .arg(presetColumnXmlName(address.col), value.toString()));
}

QSet<int> PresetTableV2TransitionWidget::applySmartWingsDefaults(PTTransitionMode mode,
                                                                 QTreeWidgetItem* item)
{
    QSet<int> changed;
    if (!item)
        return changed;

    const int wings = item->data(ColWings, kPresetCellValueRole).toInt();
    if (wings <= 1)
        return changed;

    const int wingDirection = item->data(ColWingsSymmetry, kPresetCellValueRole).toInt();
    if (wingDirection == 0)
    {
        setPresetCellValue(item, ColWingsSymmetry, 1, false);
        changed.insert(ColWingsSymmetry);
    }

    if (linkedTableUsesPositionMode()
            && (mode == PTTransitionMode::PositionMotion || mode == PTTransitionMode::MultiFx))
    {
        const int motionDirection =
                item->data(ColPositionMotionDir, kPresetCellValueRole).toInt();
        if (motionDirection == int(PTPositionMotionDirection::Forward))
        {
            setPresetCellValue(item, ColPositionMotionDir,
                               int(PTPositionMotionDirection::AlternateWings), false);
            changed.insert(ColPositionMotionDir);
        }
    }

    if (!changed.isEmpty())
    {
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("smart wings defaults mode=%1 row=%2 output=%3 selection=%4 wings=%5 cols=%6")
                        .arg(int(mode))
                        .arg(item->data(0, kItemPresetIndexRole).toInt())
                        .arg(item->data(0, kItemOutputIndexRole).toInt())
                        .arg(item->data(0, kItemSelectionIndexRole).toInt())
                        .arg(wings)
                        .arg(changed.size()));
    }
    return changed;
}

void PresetTableV2TransitionWidget::configureTransitionCombo(QComboBox* combo, int popupMinWidth)
{
    if (!combo)
        return;

    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setMinimumContentsLength(4);
    combo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    if (combo->view())
        combo->view()->setMinimumWidth(popupMinWidth);
}

static void configureTransitionTableView(QAbstractScrollArea* area)
{
    if (!area)
        return;

    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setStyleSheet(QStringLiteral(
        "QTreeView::item {"
        "  min-height: %1px;"
        "  border-right: 1px solid palette(mid);"
        "  border-bottom: 1px solid palette(mid);"
        "}").arg(kEfxTreeRowHeight));
}

QString PresetTableV2TransitionWidget::columnTitle(int col)
{
    switch (col)
    {
        case ColAxis:          return QStringLiteral("Axis");
        case ColOffsetDir:     return QObject::tr("Spread direction");
        case ColWings:         return QObject::tr("Wings");
        case ColBlocks:        return QObject::tr("Block size");
        case ColWingsSymmetry: return QObject::tr("Wing direction");
        case ColOffsetStepMode: return QObject::tr("Spread mode");
        case ColOffsetStep:    return QObject::tr("Spread amount");
        case ColDuration:      return QObject::tr("Duration ms");
        case ColWaveWidth:     return QObject::tr("Wave width");
        case ColWaveShape:     return QObject::tr("Wave shape");
        case ColFadeIn:        return QObject::tr("Fade in %");
        case ColFadeOut:       return QObject::tr("Fade out %");
        case ColWaveLevel:     return QObject::tr("Wave level");
        case ColStartOffset:   return QObject::tr("Start offset");
        case ColPropagation:   return QObject::tr("Propagation");
        case ColSpeedMult:     return QObject::tr("Mult.");
        case ColPositionMotion:  return QObject::tr("Motion");
        case ColPositionMotionDir: return QObject::tr("Motion direction");
        case ColPosition1DBuiltinMode: return QObject::tr("1D builtin");
        case ColPositionPanSize: return QObject::tr("Pan size °");
        case ColPositionTiltSize: return QObject::tr("Tilt size °");
        case ColChannel1DTarget: return QObject::tr("Target");
        case ColChannel1DTargetMode: return QObject::tr("Target mode");
        case ColChannel1DApplyMode: return QObject::tr("Mode");
        case ColChannel1DLow: return QObject::tr("Range Min");
        case ColChannel1DHigh: return QObject::tr("Range Max");
        case ColChannel1DAmount: return QObject::tr("Wave level");
        case ColChannel1DCustomColumn: return QObject::tr("Column");
        case ColMultiFxTargetMode: return QObject::tr("Mode");
        case ColMultiFxInterpolationSource: return QObject::tr("Source");
        case ColMultiFxInterpolationPrimary: return QObject::tr("Primary row");
        case ColMultiFxInterpolationSecondary: return QObject::tr("Secondary row");
        default:               return QObject::tr("Name");
    }
}

bool PresetTableV2TransitionWidget::linkedTableUsesPositionMode() const
{
    if (PresetTableV2ControlIface* table = linkedTable())
        return table->tableUsesPositionMode();
    return false;
}

void PresetTableV2TransitionWidget::applyPositionModeColumnVisibility(QTreeWidget* table,
                                                                    PTTransitionMode mode)
{
    if (!table)
        return;

    const QSet<int> visible = allowedPresetColumnsForMode(mode);
    for (int col = ColAxis; col < ColCount; ++col)
        table->setColumnHidden(col, !visible.contains(col));
}

QString PresetTableV2TransitionWidget::columnTitleForCol(int col) const
{
    if (linkedTableUsesPositionMode())
    {
        switch (col)
        {
            case ColWaveShape:
                return (activeBankMode() == PTTransitionMode::PositionMotion
                        || activeBankMode() == PTTransitionMode::MultiFx)
                        ? tr("Motion curve") : tr("Row morph");
            case ColFadeIn:
                return (activeBankMode() == PTTransitionMode::PositionMotion
                        || activeBankMode() == PTTransitionMode::MultiFx)
                        ? tr("Orbit fade in") : tr("Row fade in");
            case ColFadeOut:
                return (activeBankMode() == PTTransitionMode::PositionMotion
                        || activeBankMode() == PTTransitionMode::MultiFx)
                        ? tr("Orbit fade out") : tr("Row fade out");
            case ColWaveWidth:
                return (activeBankMode() == PTTransitionMode::PositionMotion
                        || activeBankMode() == PTTransitionMode::MultiFx)
                        ? tr("Orbit width °") : tr("Morph width °");
            case ColPosition1DBuiltinMode: return tr("1D builtin");
            case ColDuration:   return tr("Cycle ms");
            default:            break;
        }
    }
    return columnTitle(col);
}

QString PresetTableV2TransitionWidget::columnTooltipForCol(int col) const
{
    if (activeBankMode() == PTTransitionMode::Channel1D)
    {
        switch (col)
        {
            case ColChannel1DApplyMode:
                return tr("Dimmer FX stays under the base value; Bump adds above the base value");
            case ColChannel1DAmount:
                return tr("Main wave level of the 1D effect");
            case ColChannel1DLow:
            case ColChannel1DHigh:
                return tr("Legacy range value used only by old Absolute Range presets");
            case ColWaveShape:
                return tr("Shape of the full-scale 1D wave before Wave level is applied");
            case ColWaveWidth:
                return tr("Active part of the 1D wave within each cycle (degrees)");
            default:
                break;
        }
    }

    if (!linkedTableUsesPositionMode())
        return QString();

    switch (col)
    {
        case ColWaveShape:
            return (activeBankMode() == PTTransitionMode::PositionMotion
                    || activeBankMode() == PTTransitionMode::MultiFx)
                    ? tr("Motion curve packet (Sine/Triangle/Custom — same fields as row morph)")
                    : tr("Wave shape for primary↔secondary row blend (not preset crossfade)");
        case ColFadeIn:
            return (activeBankMode() == PTTransitionMode::PositionMotion
                    || activeBankMode() == PTTransitionMode::MultiFx)
                    ? tr("Amplitude fade-in inside the orbit window")
                    : tr("Fade-in for primary↔secondary row morph");
        case ColFadeOut:
            return (activeBankMode() == PTTransitionMode::PositionMotion
                    || activeBankMode() == PTTransitionMode::MultiFx)
                    ? tr("Amplitude fade-out inside the orbit window")
                    : tr("Fade-out for primary↔secondary row morph");
        case ColWaveWidth:
            return (activeBankMode() == PTTransitionMode::PositionMotion
                    || activeBankMode() == PTTransitionMode::MultiFx)
                    ? tr("Active arc of orbit motion within each cycle (degrees)")
                    : tr("Active part of row interpolation within each cycle (degrees)");
        case ColPosition1DBuiltinMode:
            return tr("Builtin 1D: Morph packet (like row morph) or Oscillate within the orbit window");
        case ColOffsetStep:
            return tr("Spread amount: Off syncs all fixtures, Auto Fit scales to the group, Coverage % is scalable, Fixed ° is legacy/expert");
        case ColOffsetStepMode:
            return tr("How spatial phase spread is scaled across fixtures");
        default:
            return QString();
    }
}

void PresetTableV2TransitionWidget::applyDefaultColumnWidths(QTreeWidget* table)
{
    if (!table)
        return;

    auto setFixed = [&](int col, int width) {
        table->setColumnWidth(col, width);
        if (QHeaderView* header = table->header())
            header->setSectionResizeMode(col, QHeaderView::Fixed);
    };

    setFixed(ColAxis, 42);
    setFixed(ColOffsetDir, 132);
    setFixed(ColWings, 32);
    setFixed(ColBlocks, 56);
    setFixed(ColWingsSymmetry, 116);
    setFixed(ColOffsetStepMode, 86);
    setFixed(ColOffsetStep, 76);
    setFixed(ColSpeedMult, 36);
    setFixed(ColPositionMotion, 88);
    setFixed(ColPositionMotionDir, 52);
    setFixed(ColPosition1DBuiltinMode, 72);
    setFixed(ColWaveShape, 72);
    setFixed(ColPropagation, 52);
    setFixed(ColChannel1DTarget, 126);
    setFixed(ColChannel1DTargetMode, 72);
    setFixed(ColChannel1DApplyMode, 120);
    setFixed(ColChannel1DLow, 76);
    setFixed(ColChannel1DHigh, 76);
    setFixed(ColChannel1DAmount, 60);
    setFixed(ColChannel1DCustomColumn, 56);
}

QVector<PTTransitionColumnGroupBar::Group>
PresetTableV2TransitionWidget::columnGroupsForMode(PTTransitionMode mode) const
{
    QVector<PTTransitionColumnGroupBar::Group> groups;
    const QSet<int> allowed = allowedPresetColumnsForMode(mode);
    auto add = [&](const QString& id, const QString& label, std::initializer_list<int> cols) {
        PTTransitionColumnGroupBar::Group group;
        group.id = id;
        group.label = label;
        for (int col : cols)
        {
            if (allowed.contains(col))
                group.columns.append(col);
        }
        if (!group.columns.isEmpty())
            groups.append(group);
    };

    if (linkedTableUsesPositionMode())
    {
        if (mode == PTTransitionMode::SweepOnly)
        {
            add(QStringLiteral("spatial"), tr("Spatial"),
                { ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry });
            add(QStringLiteral("wave"), tr("Wave"),
                { ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut });
            add(QStringLiteral("timing"), tr("Timing"),
                { ColStartOffset, ColSpeedMult });
        }
        else if (mode == PTTransitionMode::Continuous)
        {
            add(QStringLiteral("spread"), tr("Spread"),
                { ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                  ColOffsetStepMode, ColOffsetStep });
            add(QStringLiteral("morph"), tr("Interpolation"),
                { ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut });
            add(QStringLiteral("timing"), tr("Timing"),
                { ColStartOffset, ColSpeedMult });
        }
        else
        {
            add(QStringLiteral("orbit"), tr("Orbit"),
                { ColPositionMotion, ColPositionMotionDir, ColWaveWidth, ColWaveShape,
                  ColFadeIn, ColFadeOut, ColPosition1DBuiltinMode, ColAxis });
            add(QStringLiteral("spread"), tr("Spread"),
                { ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                  ColOffsetStepMode, ColOffsetStep });
            add(QStringLiteral("timing"), tr("Timing"),
                { ColStartOffset, ColSpeedMult });
        }
        return groups;
    }

    add(QStringLiteral("spatial"), tr("Spatial"),
        mode == PTTransitionMode::SweepOnly
                ? std::initializer_list<int>{ ColAxis, ColOffsetDir, ColWings,
                                              ColBlocks, ColWingsSymmetry }
                : std::initializer_list<int>{ ColAxis, ColOffsetDir, ColWings,
                                              ColBlocks, ColWingsSymmetry,
                                              ColOffsetStepMode, ColOffsetStep });
    if (mode == PTTransitionMode::Channel1D || mode == PTTransitionMode::MultiFx)
    {
        add(QStringLiteral("wave"), tr("Wave"),
            { ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut, ColStartOffset });
    }
    else
    {
        add(QStringLiteral("wave"), tr("Wave"),
            { ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut,
              ColWaveLevel, ColStartOffset });
    }
    if (mode == PTTransitionMode::Channel1D || mode == PTTransitionMode::MultiFx)
    {
        add(QStringLiteral("value"), tr("Value"),
            { ColChannel1DApplyMode, ColChannel1DAmount });
    }
    add(QStringLiteral("timing"), tr("Timing"),
        { ColSpeedMult });
    return groups;
}

PTTransitionMode PresetTableV2TransitionWidget::multiFxContextModeForItem(
        QTreeWidgetItem* item) const
{
    if (!item || !item->data(0, kItemMultiFxRouteIndexRole).isValid())
        return PTTransitionMode::MultiFx;
    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int routeIdx = item->data(0, kItemMultiFxRouteIndexRole).toInt();
    if (row < 0 || row >= m_multiFxTargetRoutes.size()
            || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size())
        return PTTransitionMode::MultiFx;
    return multiFxRouteMode(m_multiFxTargetRoutes.at(row).at(routeIdx));
}

QSet<int> PresetTableV2TransitionWidget::allowedMultiFxColumnsForContext(
        PTTransitionMode contextMode, bool rootContext) const
{
    QSet<int> cols;
    auto add = [&cols](std::initializer_list<int> list) {
        for (int col : list)
            cols.insert(col);
    };

    if (rootContext)
    {
        add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
              ColOffsetStepMode, ColOffsetStep, ColWaveWidth, ColWaveShape,
              ColFadeIn, ColFadeOut,
              ColStartOffset, ColSpeedMult });
        return cols;
    }

    add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
          ColOffsetStepMode, ColOffsetStep, ColWaveWidth, ColWaveShape,
          ColFadeIn, ColFadeOut, ColStartOffset, ColSpeedMult,
          ColMultiFxTargetMode });
    if (contextMode == PTTransitionMode::PositionMotion)
        add({ ColPositionMotion, ColPositionMotionDir, ColPosition1DBuiltinMode });
    else if (contextMode == PTTransitionMode::Channel1D)
        add({ ColChannel1DApplyMode, ColChannel1DAmount });
    else if (contextMode == PTTransitionMode::Continuous)
        add({ ColMultiFxInterpolationSource, ColMultiFxInterpolationPrimary,
              ColMultiFxInterpolationSecondary });
    return cols;
}

QVector<PTTransitionColumnGroupBar::Group>
PresetTableV2TransitionWidget::columnGroupsForMultiFxContext(
        PTTransitionMode contextMode, bool rootContext) const
{
    QVector<PTTransitionColumnGroupBar::Group> groups;
    QSet<int> visible;
    auto addVisible = [&visible](std::initializer_list<int> list) {
        for (int col : list)
            visible.insert(col);
    };
    addVisible({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                 ColOffsetStepMode, ColOffsetStep, ColWaveWidth, ColWaveShape,
                 ColFadeIn, ColFadeOut, ColStartOffset, ColSpeedMult,
                 ColPositionMotion, ColPositionMotionDir, ColPosition1DBuiltinMode,
                 ColChannel1DApplyMode, ColChannel1DAmount,
                 ColMultiFxTargetMode,
                 ColMultiFxInterpolationSource,
                 ColMultiFxInterpolationPrimary,
                 ColMultiFxInterpolationSecondary });
    Q_UNUSED(rootContext);

    auto add = [&](const QString& id, const QString& label, std::initializer_list<int> cols) {
        PTTransitionColumnGroupBar::Group group;
        group.id = id;
        group.label = label;
        for (int col : cols)
        {
            if (visible.contains(col))
                group.columns.append(col);
        }
        if (!group.columns.isEmpty())
            groups.append(group);
    };

    add(QStringLiteral("common"), tr("Common"),
        { ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut });
    add(QStringLiteral("fx"), tr("FX"),
        { ColMultiFxTargetMode,
          ColPositionMotion, ColPositionMotionDir, ColPosition1DBuiltinMode,
          ColChannel1DApplyMode, ColChannel1DAmount,
          ColMultiFxInterpolationSource,
          ColMultiFxInterpolationPrimary,
          ColMultiFxInterpolationSecondary });
    add(QStringLiteral("spread"), tr("Spread"),
        { ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
          ColOffsetStepMode, ColOffsetStep });
    add(QStringLiteral("timing"), tr("Timing"),
        { ColStartOffset, ColSpeedMult });
    return groups;
}

QSet<int> PresetTableV2TransitionWidget::allowedPresetColumnsForMode(PTTransitionMode mode) const
{
    QSet<int> visible;
    auto add = [&visible](std::initializer_list<int> cols) {
        for (int col : cols)
            visible.insert(col);
    };

    const bool positionMode = linkedTableUsesPositionMode();
    if (!positionMode)
    {
        if (mode == PTTransitionMode::SweepOnly)
        {
            add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                  ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut,
                  ColStartOffset, ColSpeedMult });
        }
        else
        {
            add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                  ColOffsetStepMode, ColOffsetStep, ColWaveWidth, ColWaveShape,
                  ColFadeIn, ColFadeOut, ColStartOffset, ColSpeedMult });
        }

        if (mode == PTTransitionMode::SweepOnly)
        {
            // Transition locks Wave level and Spread amount in runtime.
        }
        else if (mode == PTTransitionMode::Channel1D || mode == PTTransitionMode::MultiFx)
            add({ ColChannel1DApplyMode, ColChannel1DAmount });

        return visible;
    }

    if (mode == PTTransitionMode::SweepOnly)
    {
        add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
              ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut,
              ColStartOffset, ColSpeedMult });
        return visible;
    }

    if (mode == PTTransitionMode::Continuous)
    {
        add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
              ColOffsetStepMode, ColOffsetStep, ColWaveWidth, ColWaveShape,
              ColFadeIn, ColFadeOut, ColStartOffset, ColSpeedMult });
        return visible;
    }

    if (mode == PTTransitionMode::PositionMotion || mode == PTTransitionMode::MultiFx)
    {
        add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
              ColOffsetStepMode, ColOffsetStep, ColPositionMotion,
              ColPositionMotionDir, ColPosition1DBuiltinMode, ColWaveWidth,
              ColWaveShape, ColFadeIn, ColFadeOut, ColStartOffset, ColSpeedMult });
        return visible;
    }

    add({ ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
          ColOffsetStepMode, ColOffsetStep, ColWaveWidth, ColWaveShape,
          ColFadeIn, ColFadeOut, ColStartOffset, ColSpeedMult });
    return visible;
}

bool PresetTableV2TransitionWidget::isPresetColumnAllowedForMode(PTTransitionMode mode,
                                                                 int col) const
{
    return allowedPresetColumnsForMode(mode).contains(col);
}

void PresetTableV2TransitionWidget::applyColumnGroupFilter(QTreeWidget* table,
                                                         PTTransitionMode mode)
{
    if (!table)
        return;

    if (mode == PTTransitionMode::MultiFx)
    {
        QTreeWidgetItem* current = selectedPresetItem(table);
        const bool rootContext = !current
                || !current->data(0, kItemMultiFxRouteIndexRole).isValid();
        const PTTransitionMode contextMode = multiFxContextModeForItem(current);
        const QVector<PTTransitionColumnGroupBar::Group> groups =
                columnGroupsForMultiFxContext(contextMode, rootContext);
        auto groupExists = [&groups](const QString& id) {
            for (const PTTransitionColumnGroupBar::Group& group : groups)
            {
                if (group.id == id)
                    return true;
            }
            return false;
        };
        const bool hasStoredActive = m_columnGroupFilterByMode.contains(int(mode));
        QString activeId = m_columnGroupFilterByMode.value(int(mode));
        const QString preferredId = rootContext ? QStringLiteral("common")
                                                : QStringLiteral("fx");
        if (!hasStoredActive || (!activeId.isEmpty() && !groupExists(activeId)))
        {
            activeId = groupExists(preferredId) ? preferredId
                                                : (groups.isEmpty() ? QString() : groups.first().id);
            if (!activeId.isEmpty())
                m_columnGroupFilterByMode.insert(int(mode), activeId);
        }

        updateColumnHeaders(table);
        if (PTTransitionColumnGroupBar* bar = columnGroupBarForMode(mode))
        {
            bar->blockSignals(true);
            bar->setGroups(groups);
            bar->setActiveGroupId(activeId);
            bar->blockSignals(false);
        }

        QString groupId = activeId;
        QSet<int> filtered;
        for (const PTTransitionColumnGroupBar::Group& group : groups)
        {
            if (group.id == groupId)
            {
                for (int col : group.columns)
                    filtered.insert(col);
                break;
            }
        }
        if (filtered.isEmpty() && groupId.isEmpty())
        {
            for (const PTTransitionColumnGroupBar::Group& group : groups)
            {
                for (int col : group.columns)
                    filtered.insert(col);
            }
        }
        if (filtered.isEmpty())
        {
            for (const PTTransitionColumnGroupBar::Group& group : groups)
            {
                if (group.id == QStringLiteral("common"))
                {
                    for (int col : group.columns)
                        filtered.insert(col);
                    break;
                }
            }
        }
        for (int col = ColAxis; col < ColCount; ++col)
            table->setColumnHidden(col, !filtered.contains(col));
        return;
    }

    applyPositionModeColumnVisibility(table, mode);

    QSet<int> positionHidden;
    for (int col = ColAxis; col < ColCount; ++col)
    {
        if (table->isColumnHidden(col))
            positionHidden.insert(col);
    }

    const QString groupId = m_columnGroupFilterByMode.value(int(mode));
    if (groupId.isEmpty())
        return;

    QSet<int> allowed;
    for (const PTTransitionColumnGroupBar::Group& group : columnGroupsForMode(mode))
    {
        if (group.id == groupId)
        {
            for (int col : group.columns)
                allowed.insert(col);
            break;
        }
    }
    if (allowed.isEmpty())
        return;

    for (int col = ColAxis; col < ColCount; ++col)
    {
        if (col == ColName)
            continue;
        if (positionHidden.contains(col))
        {
            table->setColumnHidden(col, true);
            continue;
        }
        table->setColumnHidden(col, !allowed.contains(col));
    }
}

void PresetTableV2TransitionWidget::refreshColumnGroupBarForActiveTab()
{
    for (PTTransitionMode mode : { PTTransitionMode::SweepOnly,
                                   PTTransitionMode::Continuous,
                                   PTTransitionMode::Channel1D,
                                   PTTransitionMode::PositionMotion,
                                   PTTransitionMode::MultiFx })
    {
        PTTransitionColumnGroupBar* bar = columnGroupBarForMode(mode);
        if (!bar)
            continue;
        const QString activeId = m_columnGroupFilterByMode.value(int(mode));
        bar->blockSignals(true);
        bar->setGroups(columnGroupsForMode(mode));
        bar->setActiveGroupId(activeId);
        bar->blockSignals(false);
    }

    const PTTransitionMode mode = activeBankMode();
    if (QTreeWidget* table = tableForMode(mode))
        applyColumnGroupFilter(table, mode);
}

PresetTableV2TransitionWidget::PresetTableV2TransitionWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setCaption(tr("Preset Table Engine"));
    VCPluginDiagnostics::install(QStringLiteral("presettablev2transition"), id(), caption());
    setMinimumSize(480, 280);
    setFrameStyle(KVCFrameStyleSunken);

    if (m_sweepPresets.isEmpty())
        m_sweepPresets.append(defaultPreset(0, PTTransitionMode::SweepOnly));
    if (m_continuousPresets.isEmpty())
        m_continuousPresets.append(defaultPreset(0, PTTransitionMode::Continuous));
    if (m_channel1DPresets.isEmpty())
        m_channel1DPresets.append(defaultPreset(0, PTTransitionMode::Channel1D));
    if (m_positionMotionPresets.isEmpty())
        m_positionMotionPresets.append(defaultPreset(0, PTTransitionMode::PositionMotion));
    if (m_multiFxPresets.isEmpty())
        m_multiFxPresets.append(defaultPreset(0, PTTransitionMode::MultiFx));

    buildUi();
    rebuildAllPresetTables();
    updateGlobalSummaryLabel();
    slotRefreshTableLink();
    publishProviderSnapshot(QStringLiteral("construct"));
}

PresetTableV2TransitionWidget::~PresetTableV2TransitionWidget() = default;

PTTransitionMode PresetTableV2TransitionWidget::activeBankMode() const
{
    if (!m_bankTabs)
        return PTTransitionMode::SweepOnly;
    if (m_bankTabs->currentIndex() == 1)
        return PTTransitionMode::Continuous;
    if (m_bankTabs->currentIndex() == 2)
        return PTTransitionMode::Channel1D;
    if (m_bankTabs->currentIndex() == 3)
        return PTTransitionMode::PositionMotion;
    if (m_bankTabs->currentIndex() == 4)
        return PTTransitionMode::MultiFx;
    return PTTransitionMode::SweepOnly;
}

QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::presetsForMode(PTTransitionMode mode)
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxPresets;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionPresets;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DPresets;
    return (mode == PTTransitionMode::Continuous) ? m_continuousPresets : m_sweepPresets;
}

const QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::presetsForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxPresets;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionPresets;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DPresets;
    return (mode == PTTransitionMode::Continuous) ? m_continuousPresets : m_sweepPresets;
}

QVector<QHash<int, PresetTableV2TransitionWidget::PTTransitionOutputLayer>>&
PresetTableV2TransitionWidget::overridesForMode(PTTransitionMode mode)
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxOutputOverrides;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionOutputOverrides;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DOutputOverrides;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousOutputOverrides : m_sweepOutputOverrides;
}

const QVector<QHash<int, PresetTableV2TransitionWidget::PTTransitionOutputLayer>>&
PresetTableV2TransitionWidget::overridesForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxOutputOverrides;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionOutputOverrides;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DOutputOverrides;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousOutputOverrides : m_sweepOutputOverrides;
}

QTreeWidget* PresetTableV2TransitionWidget::tableForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxTable;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionTable;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DTable;
    return (mode == PTTransitionMode::Continuous) ? m_continuousTable : m_sweepTable;
}

QTreeView* PresetTableV2TransitionWidget::frozenNameViewForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxNameView;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionNameView;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DNameView;
    return (mode == PTTransitionMode::Continuous) ? m_continuousNameView : m_sweepNameView;
}

PTTransitionColumnGroupBar* PresetTableV2TransitionWidget::columnGroupBarForMode(
        PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxColumnGroupBar;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionColumnGroupBar;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DColumnGroupBar;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousColumnGroupBar : m_sweepColumnGroupBar;
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
    configureTransitionCombo(c, 56);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeOffsetDirCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Left → Right"), int(PTOffsetDirection::LeftToRight));
    c->addItem(QObject::tr("Right → Left"), int(PTOffsetDirection::RightToLeft));
    c->addItem(QObject::tr("Center → Edges"), int(PTOffsetDirection::CenterToSides));
    c->addItem(QObject::tr("Edges → Center"), int(PTOffsetDirection::SidesToCenter));
    c->addItem(QObject::tr("Odd/Even Split"), int(PTOffsetDirection::Alternate));
    c->addItem(QObject::tr("Mirror Pairs"), int(PTOffsetDirection::Symmetric));
    configureTransitionCombo(c, 160);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeOffsetStepModeCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Off"), int(PTOffsetStepMode::Off));
    c->addItem(QObject::tr("Auto Fit"), int(PTOffsetStepMode::AutoFit));
    c->addItem(QObject::tr("Coverage %"), int(PTOffsetStepMode::CoveragePercent));
    c->addItem(QObject::tr("Fixed °"), int(PTOffsetStepMode::FixedDegrees));
    configureTransitionCombo(c, 116);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeWaveShapeCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QStringLiteral("Sine"), 0);
    c->addItem(QStringLiteral("Triangle"), 2);
    c->addItem(QStringLiteral("Custom"), 3);
    configureTransitionCombo(c, 72);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makePositionMotionCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Off"), int(PTPositionMotion::Off));
    c->addItem(QObject::tr("Pan 1D"), int(PTPositionMotion::Pan1D));
    c->setItemData(c->count() - 1,
                   QObject::tr("Pan oscillates; tilt stays at base. Engine preview: offset vs time."),
                   Qt::ToolTipRole);
    c->addItem(QObject::tr("Tilt 1D"), int(PTPositionMotion::Tilt1D));
    c->setItemData(c->count() - 1,
                   QObject::tr("Tilt oscillates; pan stays at base. Engine preview: offset vs time."),
                   Qt::ToolTipRole);
    c->addItem(QObject::tr("Circle"), int(PTPositionMotion::Circle2D));
    c->addItem(QObject::tr("Line"), int(PTPositionMotion::Line2D));
    c->setItemData(c->count() - 1,
                   QObject::tr("Diagonal pan+tilt path: both axes use sin(φ) with the same phase."),
                   Qt::ToolTipRole);
    c->addItem(QObject::tr("Figure-8"), int(PTPositionMotion::Figure8_2D));
    c->addItem(QObject::tr("Custom 2D"), int(PTPositionMotion::Custom2D));
    configureTransitionCombo(c, 120);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makePositionMotionDirCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Forward"), int(PTPositionMotionDirection::Forward));
    c->addItem(QObject::tr("Reverse"), int(PTPositionMotionDirection::Reverse));
    c->addItem(QObject::tr("Alternate Wings"), int(PTPositionMotionDirection::AlternateWings));
    c->addItem(QObject::tr("Reverse Alternate Wings"),
               int(PTPositionMotionDirection::ReverseAlternateWings));
    c->addItem(QObject::tr("Mirror Pairs"), int(PTPositionMotionDirection::SymmetricPairs));
    configureTransitionCombo(c, 180);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makePosition1DBuiltinModeCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Morph packet"), 0);
    c->addItem(QObject::tr("Oscillate"), 1);
    configureTransitionCombo(c, 96);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeChannel1DTargetCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Dimmer"), int(PTChannel1DTarget::Dimmer));
    c->addItem(QObject::tr("All Intensity"), int(PTChannel1DTarget::AllIntensity));
    c->addItem(QObject::tr("Zoom"), int(PTChannel1DTarget::Zoom));
    c->addItem(QObject::tr("Focus"), int(PTChannel1DTarget::Focus));
    c->addItem(QObject::tr("Iris"), int(PTChannel1DTarget::Iris));
    c->addItem(QObject::tr("Prism"), int(PTChannel1DTarget::Prism));
    c->addItem(QObject::tr("Gobo Index"), int(PTChannel1DTarget::GoboIndex));
    c->addItem(QObject::tr("Shutter/Strobe"), int(PTChannel1DTarget::ShutterStrobe));
    c->addItem(QObject::tr("Speed"), int(PTChannel1DTarget::Speed));
    c->addItem(QObject::tr("Color Wheel/Macro"), int(PTChannel1DTarget::Color));
    c->addItem(QObject::tr("Custom Column"), int(PTChannel1DTarget::CustomColumn));
    configureTransitionCombo(c, 150);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeChannel1DTargetModeCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("First"), int(PTChannel1DTargetMode::First));
    c->addItem(QObject::tr("All"), int(PTChannel1DTargetMode::All));
    configureTransitionCombo(c, 90);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeChannel1DApplyModeCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Dimmer FX"), int(PTChannel1DApplyMode::MultiplyBase));
    c->addItem(QObject::tr("Bump"), int(PTChannel1DApplyMode::BumpAdd));
    configureTransitionCombo(c, 140);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makePropagationCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Parallel"), int(PTPropagationMode::Parallel));
    c->addItem(QObject::tr("Serial"), int(PTPropagationMode::Serial));
    configureTransitionCombo(c, 80);
    return c;
}

QComboBox* PresetTableV2TransitionWidget::makeWingsSymmetryCombo(QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->addItem(QObject::tr("Same"), 0);
    c->addItem(QObject::tr("Alternate Wings"), 1);
    c->addItem(QObject::tr("Mirrored Halves"), 2);
    configureTransitionCombo(c, 150);
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
    configureTransitionCombo(c, 56);
    return c;
}

QWidget* PresetTableV2TransitionWidget::createEditorForColumn(QWidget* parent, int col) const
{
    QComboBox* combo = nullptr;
    if (col == ColAxis)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeAxisCombo(parent);
    else if (col == ColOffsetDir)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeOffsetDirCombo(parent);
    else if (col == ColOffsetStepMode)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeOffsetStepModeCombo(parent);
    else if (col == ColWaveShape)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeWaveShapeCombo(parent);
    else if (col == ColPositionMotion)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makePositionMotionCombo(parent);
    else if (col == ColPositionMotionDir)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makePositionMotionDirCombo(parent);
    else if (col == ColPosition1DBuiltinMode)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makePosition1DBuiltinModeCombo(parent);
    else if (col == ColChannel1DTarget)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeChannel1DTargetCombo(parent);
    else if (col == ColChannel1DTargetMode)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeChannel1DTargetModeCombo(parent);
    else if (col == ColChannel1DApplyMode)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeChannel1DApplyModeCombo(parent);
    else if (col == ColPropagation)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makePropagationCombo(parent);
    else if (col == ColWingsSymmetry)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeWingsSymmetryCombo(parent);
    else if (col == ColSpeedMult)
        combo = const_cast<PresetTableV2TransitionWidget*>(this)->makeSpeedMultCombo(parent);

    if (combo)
    {
        combo->setFrame(false);
        combo->installEventFilter(const_cast<PresetTableV2TransitionWidget*>(this));
        return combo;
    }

    QSpinBox* spin = new QSpinBox(parent);
    int minV = 0;
    int maxV = 360;
    switch (col)
    {
        case ColWings:
        case ColBlocks:
            minV = 1; maxV = 64; break;
        case ColOffsetStep:
            minV = 0; maxV = 360; break;
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
        case ColPositionPanSize:
            minV = 0; maxV = 540; break;
        case ColPositionTiltSize:
            minV = 0; maxV = 270; break;
        case ColChannel1DLow:
        case ColChannel1DHigh:
        case ColChannel1DAmount:
        case ColChannel1DCustomColumn:
            minV = 0; maxV = 255; break;
        default:
            break;
    }
    spin->setRange(minV, maxV);
    spin->setKeyboardTracking(false);
    spin->setFrame(false);
    spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spin->installEventFilter(const_cast<PresetTableV2TransitionWidget*>(this));
    return spin;
}

QWidget* PresetTableV2TransitionWidget::createEditorForItemColumn(
        QWidget* parent, QTreeWidgetItem* item, int col) const
{
    if (col == ColMultiFxInterpolationSource)
    {
        QComboBox* c = new QComboBox(parent);
        c->addItem(tr("Dynamic"), int(PTMultiFxInterpolationSourceMode::Dynamic));
        c->addItem(tr("Static"), int(PTMultiFxInterpolationSourceMode::Static));
        configureTransitionCombo(c, 92);
        c->setFrame(false);
        c->installEventFilter(const_cast<PresetTableV2TransitionWidget*>(this));
        return c;
    }

    if (col == ColMultiFxTargetMode)
    {
        QComboBox* c = new QComboBox(parent);
        c->addItem(tr("FX"), 0);
        c->addItem(tr("Interpolation"), 1);
        configureTransitionCombo(c, 118);
        c->setFrame(false);
        c->installEventFilter(const_cast<PresetTableV2TransitionWidget*>(this));
        return c;
    }

    if (col == ColMultiFxInterpolationPrimary || col == ColMultiFxInterpolationSecondary)
    {
        const int routeIdx = item && item->data(0, kItemMultiFxRouteIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteIndexRole).toInt() : -1;
        const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
        quint32 tableId = VCWidget::invalidId();
        if (row >= 0 && row < m_multiFxTargetRoutes.size()
                && routeIdx >= 0 && routeIdx < m_multiFxTargetRoutes.at(row).size())
            tableId = m_multiFxTargetRoutes.at(row).at(routeIdx).tableId;

        if (PresetTableV2ControlIface* table =
                PresetTableV2VCLookup::controlIfaceByVcId(tableId))
        {
            QComboBox* c = new QComboBox(parent);
            c->addItem(col == ColMultiFxInterpolationPrimary ? tr("Dynamic")
                                                             : tr("Off"),
                       -1);
            const int count = table->presetTableRowCountForPresetOverride();
            for (int i = 0; i < count; ++i)
            {
                QString name = table->presetTableRowNameForPresetOverride(i);
                if (name.trimmed().isEmpty())
                    name = tr("Preset %1").arg(i + 1);
                c->addItem(tr("%1: %2").arg(i + 1).arg(name), i);
            }
            configureTransitionCombo(c, 132);
            c->setFrame(false);
            c->installEventFilter(const_cast<PresetTableV2TransitionWidget*>(this));
            return c;
        }
    }

    return createEditorForColumn(parent, col);
}

QVariant PresetTableV2TransitionWidget::normalizedColumnValue(int col, const QString& raw) const
{
    const QString trimmed = raw.trimmed();
    if (trimmed.isEmpty())
        return QVariant();

    if (col == ColChannel1DApplyMode)
    {
        const QString normalized = QString(trimmed).remove(QLatin1Char('['))
                .remove(QLatin1Char(']')).trimmed().toLower();
        if (normalized == QStringLiteral("dimmer fx")
                || normalized == QStringLiteral("multiply base"))
            return int(PTChannel1DApplyMode::MultiplyBase);
        if (normalized == QStringLiteral("bump")
                || normalized == QStringLiteral("bump/add"))
            return int(PTChannel1DApplyMode::BumpAdd);
        if (normalized == QStringLiteral("legacy absolute range")
                || normalized == QStringLiteral("absolute range"))
            return int(PTChannel1DApplyMode::AbsoluteRange);
        if (normalized == QStringLiteral("legacy relative base")
                || normalized == QStringLiteral("relative base"))
            return int(PTChannel1DApplyMode::RelativeAroundBase);
    }
    if (col == ColMultiFxInterpolationSource)
    {
        const QString normalized = trimmed.toLower();
        if (normalized == QStringLiteral("dynamic"))
            return int(PTMultiFxInterpolationSourceMode::Dynamic);
        if (normalized == QStringLiteral("static"))
            return int(PTMultiFxInterpolationSourceMode::Static);
    }
    if (col == ColMultiFxTargetMode)
    {
        const QString normalized = trimmed.toLower();
        if (normalized == QStringLiteral("fx")
                || normalized == QStringLiteral("1d fx")
                || normalized == QStringLiteral("2d fx"))
            return 0;
        if (normalized == QStringLiteral("interpolation"))
            return 1;
    }
    if (col == ColWaveShape)
    {
        const QString normalized = QString(trimmed).remove(QLatin1Char('['))
                .remove(QLatin1Char(']')).trimmed().toLower();
        if (normalized == QStringLiteral("sine"))
            return 0;
        if (normalized == QStringLiteral("legacy square")
                || normalized == QStringLiteral("square"))
            return 1;
        if (normalized == QStringLiteral("triangle"))
            return 2;
        if (normalized == QStringLiteral("custom"))
            return 3;
    }

    auto comboValue = [&](QComboBox* combo) -> QVariant {
        if (!combo)
            return QVariant();
        int match = combo->findText(trimmed, Qt::MatchFixedString);
        if (match >= 0)
            return combo->itemData(match);
        bool ok = false;
        const int numeric = trimmed.toInt(&ok);
        if (ok)
        {
            for (int i = 0; i < combo->count(); ++i)
                if (combo->itemData(i).toInt() == numeric)
                    return combo->itemData(i);
        }
        return QVariant();
    };

    QScopedPointer<QWidget> editor(createEditorForColumn(nullptr, col));
    if (auto* combo = qobject_cast<QComboBox*>(editor.data()))
        return comboValue(combo);
    if (auto* spin = qobject_cast<QSpinBox*>(editor.data()))
    {
        bool ok = false;
        const int value = QString(trimmed).remove(QChar(0x00B0)).remove(QLatin1Char('%')).toInt(&ok);
        if (ok)
            return qBound(spin->minimum(), value, spin->maximum());
    }
    return QVariant();
}

QString PresetTableV2TransitionWidget::comboDisplayTextForColumn(int col,
                                                               const QVariant& value) const
{
    if (!value.isValid())
        return QString();

    const int v = value.toInt();
    switch (col)
    {
        case ColAxis:
            switch (PTTransitionAxis(v))
            {
                case PTTransitionAxis::X:  return QStringLiteral("X");
                case PTTransitionAxis::Y:  return QStringLiteral("Y");
                case PTTransitionAxis::XY: return QStringLiteral("XY");
            }
            break;
        case ColOffsetDir:
            switch (PTOffsetDirection(v))
            {
                case PTOffsetDirection::LeftToRight:    return tr("Left → Right");
                case PTOffsetDirection::RightToLeft:    return tr("Right → Left");
                case PTOffsetDirection::CenterToSides:  return tr("Center → Edges");
                case PTOffsetDirection::SidesToCenter:  return tr("Edges → Center");
                case PTOffsetDirection::Alternate:      return tr("Odd/Even Split");
                case PTOffsetDirection::Symmetric:      return tr("Mirror Pairs");
            }
            break;
        case ColOffsetStepMode:
            switch (PTOffsetStepMode(v))
            {
                case PTOffsetStepMode::Off:             return tr("Off");
                case PTOffsetStepMode::AutoFit:         return tr("Auto Fit");
                case PTOffsetStepMode::CoveragePercent: return tr("Coverage %");
                case PTOffsetStepMode::FixedDegrees:    return tr("Fixed °");
            }
            break;
        case ColWaveShape:
            switch (v)
            {
                case 0: return QStringLiteral("Sine");
                case 1: return tr("[Legacy] Square");
                case 2: return QStringLiteral("Triangle");
                case 3: return QStringLiteral("Custom");
            }
            break;
        case ColPositionMotion:
            switch (PTPositionMotion(v))
            {
                case PTPositionMotion::Off:        return tr("Off");
                case PTPositionMotion::Pan1D:    return tr("Pan 1D");
                case PTPositionMotion::Tilt1D:   return tr("Tilt 1D");
                case PTPositionMotion::Circle2D: return tr("Circle");
                case PTPositionMotion::Line2D:   return tr("Line");
                case PTPositionMotion::Figure8_2D: return tr("Figure-8");
                case PTPositionMotion::Custom2D:   return tr("Custom 2D");
                default: break;
            }
            break;
        case ColPositionMotionDir:
            switch (PTPositionMotionDirection(v))
            {
                case PTPositionMotionDirection::Forward:        return tr("Forward");
                case PTPositionMotionDirection::Reverse:        return tr("Reverse");
                case PTPositionMotionDirection::AlternateWings: return tr("Alternate Wings");
                case PTPositionMotionDirection::ReverseAlternateWings:
                    return tr("Reverse Alternate Wings");
                case PTPositionMotionDirection::SymmetricPairs: return tr("Mirror Pairs");
            }
            break;
        case ColPosition1DBuiltinMode:
            switch (v)
            {
                case 0: return tr("Morph packet");
                case 1: return tr("Oscillate");
            }
            break;
        case ColChannel1DTarget:
            switch (PTChannel1DTarget(v))
            {
                case PTChannel1DTarget::Dimmer: return tr("Dimmer");
                case PTChannel1DTarget::AllIntensity: return tr("All Intensity");
                case PTChannel1DTarget::Zoom: return tr("Zoom");
                case PTChannel1DTarget::Focus: return tr("Focus");
                case PTChannel1DTarget::Iris: return tr("Iris");
                case PTChannel1DTarget::Prism: return tr("Prism");
                case PTChannel1DTarget::GoboIndex: return tr("Gobo Index");
                case PTChannel1DTarget::ShutterStrobe: return tr("Shutter/Strobe");
                case PTChannel1DTarget::Speed: return tr("Speed");
                case PTChannel1DTarget::Color: return tr("Color Wheel/Macro");
                case PTChannel1DTarget::CustomColumn: return tr("Custom Column");
            }
            break;
        case ColChannel1DTargetMode:
            switch (PTChannel1DTargetMode(v))
            {
                case PTChannel1DTargetMode::First: return tr("First");
                case PTChannel1DTargetMode::All: return tr("All");
            }
            break;
        case ColChannel1DApplyMode:
            switch (PTChannel1DApplyMode(v))
            {
                case PTChannel1DApplyMode::AbsoluteRange: return tr("[Legacy] Absolute Range");
                case PTChannel1DApplyMode::RelativeAroundBase: return tr("[Legacy] Relative Base");
                case PTChannel1DApplyMode::MultiplyBase: return tr("Dimmer FX");
                case PTChannel1DApplyMode::BumpAdd: return tr("Bump");
            }
            break;
        case ColPropagation:
            switch (PTPropagationMode(v))
            {
                case PTPropagationMode::Parallel: return tr("Parallel");
                case PTPropagationMode::Serial:   return tr("Serial");
            }
            break;
        case ColWingsSymmetry:
            switch (v)
            {
                case 0: return tr("Same");
                case 1: return tr("Alternate Wings");
                case 2: return tr("Mirrored Halves");
            }
            break;
        case ColSpeedMult:
            switch (v)
            {
                case 0: return QStringLiteral("0.5x");
                case 1: return QStringLiteral("1.0x");
                case 2: return QStringLiteral("2.0x");
                case 3: return QStringLiteral("3.0x");
                case 4: return QStringLiteral("4.0x");
                case 5: return QStringLiteral("5.0x");
            }
            break;
        case ColMultiFxInterpolationSource:
            return v == int(PTMultiFxInterpolationSourceMode::Static)
                    ? tr("Static") : tr("Dynamic");
        case ColMultiFxTargetMode:
            return v == 1 ? tr("Interpolation") : tr("FX");
        default:
            break;
    }
    return QString();
}

QString PresetTableV2TransitionWidget::displayTextForColumn(int col, const QVariant& value) const
{
    const QString comboText = comboDisplayTextForColumn(col, value);
    if (!comboText.isEmpty())
        return comboText;
    if (col == ColOffsetStep)
        return tr("%1").arg(value.toInt());
    if (col == ColMultiFxInterpolationPrimary
            || col == ColMultiFxInterpolationSecondary)
    {
        const int row = value.toInt();
        if (row < 0)
            return col == ColMultiFxInterpolationPrimary ? tr("Dynamic") : tr("Off");
        return tr("Row %1").arg(row + 1);
    }
    return value.toString();
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
        case ColOffsetStepMode:
            return preset.playbackMode == PTTransitionMode::SweepOnly
                    ? int(PTOffsetStepMode::AutoFit) : int(preset.offsetStepMode);
        case ColOffsetStep:
            if (preset.playbackMode == PTTransitionMode::SweepOnly)
                return 100;
            return preset.offsetStepMode == PTOffsetStepMode::CoveragePercent
                    ? preset.offsetCoverage : preset.offsetStep;
        case ColDuration:      return int(preset.durationMs);
        case ColWaveWidth:     return preset.waveWidth;
        case ColWaveShape:     return preset.customCurveEnabled ? 3 : preset.waveShape;
        case ColFadeIn:        return preset.waveFadeIn;
        case ColFadeOut:       return preset.waveFadeOut;
        case ColWaveLevel:     return preset.waveLevel;
        case ColStartOffset:   return preset.startOffset;
        case ColPropagation:   return int(preset.propagation);
        case ColSpeedMult:     return preset.speedMultiplier;
        case ColPositionMotion:  return preset.positionMotion;
        case ColPositionMotionDir: return preset.positionMotionDirection;
        case ColPosition1DBuiltinMode:
            return preset.position1DBuiltinMode;
        case ColPositionPanSize: return preset.positionPanSize;
        case ColPositionTiltSize: return preset.positionTiltSize;
        case ColChannel1DTarget: return preset.channel1DTarget;
        case ColChannel1DTargetMode: return preset.channel1DTargetMode;
        case ColChannel1DApplyMode: return preset.channel1DApplyMode;
        case ColChannel1DLow: return preset.channel1DLow;
        case ColChannel1DHigh: return preset.channel1DHigh;
        case ColChannel1DAmount: return preset.channel1DAmount;
        case ColChannel1DCustomColumn: return preset.channel1DCustomColumn;
        case ColMultiFxInterpolationSource:
            return preset.multiFxInterpolationSourceMode;
        case ColMultiFxInterpolationPrimary:
            return preset.multiFxInterpolationPrimaryRow;
        case ColMultiFxInterpolationSecondary:
            return preset.multiFxInterpolationSecondaryRow;
        default:               return QVariant();
    }
}

void PresetTableV2TransitionWidget::setPresetColumnValue(PTTransitionPreset& preset, int col,
                                                         const QVariant& value)
{
    if (preset.playbackMode == PTTransitionMode::SweepOnly
            && (col == ColOffsetStepMode || col == ColOffsetStep))
    {
        preset.offsetStepMode = PTOffsetStepMode::AutoFit;
        preset.offsetCoverage = 100;
        preset.offsetStep = 0;
        return;
    }

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
        case ColOffsetStepMode:
            preset.offsetStepMode = PTOffsetStepMode(
                    qBound(0, value.toInt(), int(PTOffsetStepMode::FixedDegrees)));
            break;
        case ColOffsetStep:
            if (preset.offsetStepMode == PTOffsetStepMode::CoveragePercent)
                preset.offsetCoverage = qBound(0, value.toInt(), 100);
            else
                preset.offsetStep = qBound(0, value.toInt(), 360);
            break;
        case ColDuration:
            preset.durationMs = quint32(value.toInt());
            break;
        case ColWaveWidth:
            preset.waveWidth = qBound(1, value.toInt(), 360);
            break;
        case ColWaveShape:
            preset.customCurveEnabled = value.toInt() == 3;
            preset.waveShape = preset.customCurveEnabled ? 0 : value.toInt();
            if (preset.customCurveEnabled && preset.customCurve.size() < 2)
                preset.customCurve = defaultTransitionCustomCurve();
            break;
        case ColFadeIn:
            preset.waveFadeIn = qBound(0, value.toInt(), 100);
            break;
        case ColFadeOut:
            preset.waveFadeOut = qBound(0, value.toInt(), 100);
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
        case ColPositionMotion:
            preset.positionMotion = value.toInt();
            if (preset.positionMotion == int(PTPositionMotion::Custom2D)
                    && preset.positionPath2D.size() < 2)
            {
                preset.positionPath2D = PTShapesGallery::defaultMotionPath2D();
            }
            break;
        case ColPositionMotionDir:
            preset.positionMotionDirection = value.toInt();
            break;
        case ColPosition1DBuiltinMode:
            preset.position1DBuiltinMode = qBound(0, value.toInt(), 1);
            break;
        case ColPositionPanSize:
            preset.positionPanSize = value.toInt();
            break;
        case ColPositionTiltSize:
            preset.positionTiltSize = value.toInt();
            break;
        case ColChannel1DTarget:
            preset.channel1DTarget = qBound(0, value.toInt(),
                                            int(PTChannel1DTarget::CustomColumn));
            break;
        case ColChannel1DTargetMode:
            preset.channel1DTargetMode = qBound(0, value.toInt(), int(PTChannel1DTargetMode::All));
            break;
        case ColChannel1DApplyMode:
            preset.channel1DApplyMode = qBound(0, value.toInt(), int(PTChannel1DApplyMode::BumpAdd));
            break;
        case ColChannel1DLow:
            preset.channel1DLow = qBound(0, value.toInt(), 255);
            break;
        case ColChannel1DHigh:
            preset.channel1DHigh = qBound(0, value.toInt(), 255);
            break;
        case ColChannel1DAmount:
            preset.channel1DAmount = qBound(0, value.toInt(), 255);
            break;
        case ColChannel1DCustomColumn:
            preset.channel1DCustomColumn = qBound(0, value.toInt(), 255);
            break;
        case ColMultiFxInterpolationSource:
            preset.multiFxInterpolationSourceMode =
                    qBound(0, value.toInt(), int(PTMultiFxInterpolationSourceMode::Static));
            break;
        case ColMultiFxInterpolationPrimary:
            preset.multiFxInterpolationPrimaryRow = qMax(-1, value.toInt());
            break;
        case ColMultiFxInterpolationSecondary:
            preset.multiFxInterpolationSecondaryRow = qMax(-1, value.toInt());
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
        case ColOffsetStepMode: return KXMLPresetOffsetStepMode;
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
        case ColPositionMotion:  return KXMLPresetPositionMotion;
        case ColPositionMotionDir: return KXMLPresetPositionMotionDir;
        case ColPosition1DBuiltinMode: return KXMLPresetPosition1DBuiltinMode;
        case ColPositionPanSize: return KXMLPresetPositionPanSize;
        case ColPositionTiltSize: return KXMLPresetPositionTiltSize;
        case ColChannel1DTarget: return KXMLPresetChannel1DTarget;
        case ColChannel1DTargetMode: return KXMLPresetChannel1DTargetMode;
        case ColChannel1DApplyMode: return KXMLPresetChannel1DApplyMode;
        case ColChannel1DLow: return KXMLPresetChannel1DLow;
        case ColChannel1DHigh: return KXMLPresetChannel1DHigh;
        case ColChannel1DAmount: return KXMLPresetChannel1DAmount;
        case ColChannel1DCustomColumn: return KXMLPresetChannel1DCustomColumn;
        case ColMultiFxInterpolationSource: return KXMLPresetMultiFxInterpolationSource;
        case ColMultiFxInterpolationPrimary: return KXMLPresetMultiFxInterpolationPrimary;
        case ColMultiFxInterpolationSecondary: return KXMLPresetMultiFxInterpolationSecondary;
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
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    if (!snapshot.enabled)
        return PTTransitionMode::Off;
    return snapshot.activeMode;
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
    PTTransitionPreset summaryPreset;
    const quint32 effectiveCycle =
            PTParamMatrixEngine::effectiveDurationMs(m_globalSettings, summaryPreset);
    const QString ceilingText = m_globalSettings.sizeSpeedCeilingEnabled
            ? tr(" | Ceiling: %1 ms@small/knee %2")
                    .arg(m_globalSettings.smallSizeMinDurationMs)
                    .arg(m_globalSettings.speedOverdriveKnee)
            : QString();
    const QString text = tr("Speed: %1%2 | Intensity: %3%4 | Pos size: %5%6 | Cycle: %7–%8 ms (%9 now)%10 | %11%12")
            .arg(m_globalSettings.speed)
            .arg(inputMark(PTEfxCol::InputGlobalSpeed))
            .arg(m_globalSettings.intensity)
            .arg(inputMark(PTEfxCol::InputGlobalIntensity))
            .arg(m_globalSettings.positionSize)
            .arg(inputMark(PTEfxCol::InputGlobalPositionSize))
            .arg(m_globalSettings.minDurationMs)
            .arg(m_globalSettings.maxDurationMs)
            .arg(effectiveCycle)
            .arg(ceilingText)
            .arg(xfMode)
            .arg(inputMark(PTEfxCol::InputCrossfadeManual));
    m_globalSummaryLabel->setText(text);
    m_globalSummaryLabel->setToolTip(
            tr("Global speed, intensity, position size and min/max cycle times — open widget properties to edit."));
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
    m_toolbar->addAction(tr("+ Selection"), this, &PresetTableV2TransitionWidget::slotAddSelection);
    m_removeAction = m_toolbar->addAction(tr("Remove"), this,
                                          &PresetTableV2TransitionWidget::slotRemovePreset);
    m_toolbar->addAction(tr("Duplicate"), this, &PresetTableV2TransitionWidget::slotDuplicatePreset);
    m_toolbar->addSeparator();
    m_toolbar->addAction(tr("Edit motion curve"), this,
                         &PresetTableV2TransitionWidget::slotOpenMotionCurveEditor);
    m_layout->addWidget(m_toolbar);

    m_previewRow = new QWidget(this);
    QHBoxLayout* previewLayout = new QHBoxLayout(m_previewRow);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    m_leftPreview = new QWidget(m_previewRow);
    QVBoxLayout* leftLayout = new QVBoxLayout(m_leftPreview);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(2);

    m_curveLabel = new QLabel(tr("Dimmer wave"), m_leftPreview);
    m_curveWidget = new PTDimmerWaveCurveWidget(m_leftPreview);
    m_curveWidget->setMinimumHeight(110);
    connect(m_curveWidget, &PTDimmerWaveCurveWidget::customCurveEditRequested,
            this, &PresetTableV2TransitionWidget::slotOpenCustomCurveEditor);
    m_previewRefreshTimer = new QTimer(this);
    m_previewRefreshTimer->setInterval(50);
    connect(m_previewRefreshTimer, &QTimer::timeout, this, [this]() {
        if (!isVisible() || activeBankMode() != PTTransitionMode::SweepOnly)
            return;
        PresetTableV2ControlIface* linkedTableIface = linkedTable();
        QObject* linkedObject = dynamic_cast<QObject*>(linkedTableIface);
        auto* previewState = linkedObject
                ? qobject_cast<PresetTableV2PreviewStateIface*>(linkedObject) : nullptr;
        bool active = false;
        if (previewState)
            previewState->crossfadePreviewProgress01(&active);
        if (active)
            updateEffectPreview();
    });
    m_previewRefreshTimer->start();
    QWidget* positionPreviewHeader = new QWidget(m_leftPreview);
    QHBoxLayout* positionPreviewHeaderLayout = new QHBoxLayout(positionPreviewHeader);
    positionPreviewHeaderLayout->setContentsMargins(0, 0, 0, 0);
    positionPreviewHeaderLayout->setSpacing(4);
    m_positionPreviewLabel = new QLabel(tr("Position motion"), positionPreviewHeader);
    m_positionPreviewModeButton = new QToolButton(positionPreviewHeader);
    m_positionPreviewModeButton->setText(tr("Pan/Tilt"));
    m_positionPreviewModeButton->setCheckable(true);
    m_positionPreviewModeButton->setAutoRaise(true);
    m_positionPreviewModeButton->setToolTip(tr("Show 2D motion as separate Pan and Tilt curves"));
    connect(m_positionPreviewModeButton, &QToolButton::toggled,
            this, [this]() { updateEffectPreview(); });
    positionPreviewHeaderLayout->addWidget(m_positionPreviewLabel, 1);
    positionPreviewHeaderLayout->addWidget(m_positionPreviewModeButton);
    m_positionMotionStack = new QStackedWidget(m_leftPreview);
    m_positionMotionStack->setMinimumHeight(110);
    m_positionMotion1DWidget = new PTPositionMotion1DPreviewWidget(m_positionMotionStack);
    m_positionMotion1DWidget->setMinimumHeight(110);
    connect(m_positionMotion1DWidget, &PTPositionMotion1DPreviewWidget::motionCurveEditRequested,
            this, &PresetTableV2TransitionWidget::slotOpenMotionCurveEditor);
    m_positionPathWidget = new PTPositionPathPreviewWidget(m_positionMotionStack);
    m_positionPathWidget->setMinimumHeight(110);
    m_positionMotionStack->addWidget(m_positionMotion1DWidget);
    m_positionMotionStack->addWidget(m_positionPathWidget);
    leftLayout->addWidget(m_curveLabel);
    leftLayout->addWidget(m_curveWidget, 2);
    leftLayout->addWidget(positionPreviewHeader);
    leftLayout->addWidget(m_positionMotionStack, 2);

    m_spatialPreviewColumn = new QWidget(m_previewRow);
    QVBoxLayout* spatialLayout = new QVBoxLayout(m_spatialPreviewColumn);
    spatialLayout->setContentsMargins(0, 0, 0, 0);
    spatialLayout->setSpacing(2);

    m_spatialGridWidget = new PTSpatialFixtureGridWidget(m_spatialPreviewColumn);
    m_spatialGridWidget->setToolTip(
            tr("Fixture group: sweep order, head offset (°), phase start. "
               "Orange border = offset step too large; red = duplicate offsets."));
    connect(m_spatialGridWidget, &PTSpatialFixtureGridWidget::selectionCellsChanged,
            this, &PresetTableV2TransitionWidget::applySelectionCellsFromGrid);
    connect(m_spatialGridWidget, &PTSpatialFixtureGridWidget::selectionLayerActivated,
            this, &PresetTableV2TransitionWidget::slotSelectionLayerActivatedFromGrid);
    m_spatialGridCaption = new QLabel(m_spatialPreviewColumn);
    m_spatialGridCaption->setWordWrap(true);
    QFont captionFont = m_spatialGridCaption->font();
    captionFont.setPointSize(qMax(7, captionFont.pointSize() - 1));
    m_spatialGridCaption->setFont(captionFont);
    spatialLayout->addWidget(m_spatialGridWidget, 1);
    spatialLayout->addWidget(m_spatialGridCaption);

    previewLayout->addWidget(m_leftPreview, 2);
    previewLayout->addWidget(m_spatialPreviewColumn, 3);
    m_layout->addWidget(m_previewRow);

    m_bankTabs = new QTabWidget(this);
    m_sweepTable = new QTreeWidget(m_bankTabs);
    m_continuousTable = new QTreeWidget(m_bankTabs);
    m_positionMotionTable = new QTreeWidget(m_bankTabs);
    m_channel1DTable = new QTreeWidget(m_bankTabs);
    m_multiFxTable = new QTreeWidget(m_bankTabs);
    m_transitionDelegate = new PresetTableV2TransitionDelegate(this, this);
    m_sweepNameView = new QTreeView(m_bankTabs);
    m_continuousNameView = new QTreeView(m_bankTabs);
    m_positionMotionNameView = new QTreeView(m_bankTabs);
    m_channel1DNameView = new QTreeView(m_bankTabs);
    m_multiFxNameView = new QTreeView(m_bankTabs);
    for (QTreeWidget* table : { m_sweepTable, m_continuousTable,
                                m_positionMotionTable, m_channel1DTable, m_multiFxTable })
    {
        table->setColumnCount(ColCount);
        table->header()->setStretchLastSection(true);
        table->setColumnHidden(ColName, true);
        table->setSelectionBehavior(QAbstractItemView::SelectItems);
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        table->setItemDelegate(m_transitionDelegate);
        table->setRootIsDecorated(true);
        table->setUniformRowHeights(true);
        table->setAlternatingRowColors(true);
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        configureTransitionTableView(table);
        table->installEventFilter(this);
        table->viewport()->installEventFilter(this);
        connect(table, &QTreeWidget::itemChanged,
                this, &PresetTableV2TransitionWidget::slotPresetItemChanged);
        connect(table, &QTreeWidget::itemSelectionChanged, this, [this]() {
            if (QTreeWidget* table = qobject_cast<QTreeWidget*>(sender()))
                applyColumnGroupFilter(table, modeForTable(table));
            updateRemoveActionLabel();
            updateEffectPreview();
        });
        connect(table, &QTreeWidget::customContextMenuRequested,
                this, &PresetTableV2TransitionWidget::slotPresetContextMenuRequested);
        connect(table->header(), &QHeaderView::sectionDoubleClicked,
                this, &PresetTableV2TransitionWidget::slotColumnHeaderDoubleClicked);
        connect(table->header(), &QHeaderView::sectionClicked, this,
                [this, table](int logicalIndex) {
            if (logicalIndex > ColName && logicalIndex < ColCount)
            {
                m_focusColumnByTable.insert(table, logicalIndex);
                updateColumnFocusVisuals(table);
            }
        });
        connect(table, &QTreeWidget::itemExpanded, this, [this, table](QTreeWidgetItem* item) {
            if (!item)
                return;
            if (m_syncingFrozenExpansion)
                return;
            PTTransitionMode mode = PTTransitionMode::SweepOnly;
            if (table == m_continuousTable)
                mode = PTTransitionMode::Continuous;
            else if (table == m_positionMotionTable)
                mode = PTTransitionMode::PositionMotion;
            else if (table == m_channel1DTable)
                mode = PTTransitionMode::Channel1D;
            else if (table == m_multiFxTable)
                mode = PTTransitionMode::MultiFx;
            if (!item->parent())
            {
                const int row = item->data(0, kItemPresetIndexRole).toInt();
                expandedSetForMode(mode).insert(row);
            }
            if (QTreeView* frozen = frozenNameViewForMode(mode))
            {
                ScopedBoolFlag syncGuard(m_syncingFrozenExpansion);
                frozen->expand(table->indexFromItem(item));
            }
        });
        connect(table, &QTreeWidget::itemCollapsed, this, [this, table](QTreeWidgetItem* item) {
            if (!item)
                return;
            if (m_syncingFrozenExpansion)
                return;
            PTTransitionMode mode = PTTransitionMode::SweepOnly;
            if (table == m_continuousTable)
                mode = PTTransitionMode::Continuous;
            else if (table == m_positionMotionTable)
                mode = PTTransitionMode::PositionMotion;
            else if (table == m_channel1DTable)
                mode = PTTransitionMode::Channel1D;
            else if (table == m_multiFxTable)
                mode = PTTransitionMode::MultiFx;
            if (!item->parent())
            {
                const int row = item->data(0, kItemPresetIndexRole).toInt();
                expandedSetForMode(mode).remove(row);
            }
            if (QTreeView* frozen = frozenNameViewForMode(mode))
            {
                ScopedBoolFlag syncGuard(m_syncingFrozenExpansion);
                frozen->collapse(table->indexFromItem(item));
            }
        });
    }
    auto makeBankPage = [this](QTreeView* names, QTreeWidget* table, PTTransitionMode mode) {
        QWidget* page = new QWidget;
        QVBoxLayout* vlay = new QVBoxLayout(page);
        vlay->setContentsMargins(0, 0, 0, 0);
        vlay->setSpacing(0);

        QHBoxLayout* filterRow = new QHBoxLayout;
        filterRow->setContentsMargins(0, 0, 0, 0);
        filterRow->setSpacing(0);
        QWidget* spacer = new QWidget(page);
        spacer->setFixedWidth(kFrozenNameWidth);
        filterRow->addWidget(spacer);

        PTTransitionColumnGroupBar* bar = new PTTransitionColumnGroupBar(page);
        connect(bar, &PTTransitionColumnGroupBar::activeGroupChanged,
                this, [this, mode](const QString& groupId) {
            m_columnGroupFilterByMode.insert(int(mode), groupId);
            if (QTreeWidget* t = tableForMode(mode))
                applyColumnGroupFilter(t, mode);
        });
        filterRow->addWidget(bar, 1);
        vlay->addLayout(filterRow);

        QLabel* sourceLabel = new QLabel(page);
        sourceLabel->setWordWrap(true);
        sourceLabel->setVisible(false);
        QFont sourceFont = sourceLabel->font();
        sourceFont.setPointSize(qMax(7, sourceFont.pointSize() - 1));
        sourceLabel->setFont(sourceFont);
        vlay->addWidget(sourceLabel);
        m_bankSourceLabels.insert(int(mode), sourceLabel);

        QHBoxLayout* tableRow = new QHBoxLayout;
        tableRow->setContentsMargins(0, 0, 0, 0);
        tableRow->setSpacing(0);
        QWidget* nameWrap = new QWidget(page);
        QHBoxLayout* nameLay = new QHBoxLayout(nameWrap);
        nameLay->setContentsMargins(0, 0, 0, 0);
        nameLay->setSpacing(0);
        nameLay->addWidget(names);
        tableRow->addWidget(nameWrap);
        tableRow->addWidget(table, 1);
        vlay->addLayout(tableRow, 1);

        if (mode == PTTransitionMode::SweepOnly)
            m_sweepColumnGroupBar = bar;
        else if (mode == PTTransitionMode::Continuous)
            m_continuousColumnGroupBar = bar;
        else if (mode == PTTransitionMode::PositionMotion)
            m_positionMotionColumnGroupBar = bar;
        else if (mode == PTTransitionMode::Channel1D)
            m_channel1DColumnGroupBar = bar;
        else
            m_multiFxColumnGroupBar = bar;
        return page;
    };
    m_bankTabs->addTab(makeBankPage(m_sweepNameView, m_sweepTable, PTTransitionMode::SweepOnly),
                       tr("Transition"));
    m_bankTabs->addTab(makeBankPage(m_continuousNameView, m_continuousTable,
                                     PTTransitionMode::Continuous),
                       tr("Interpolation"));
    m_bankTabs->addTab(makeBankPage(m_channel1DNameView, m_channel1DTable,
                                     PTTransitionMode::Channel1D),
                       tr("1D FX"));
    m_bankTabs->addTab(makeBankPage(m_positionMotionNameView, m_positionMotionTable,
                                     PTTransitionMode::PositionMotion),
                       tr("2D FX"));
    m_bankTabs->addTab(makeBankPage(m_multiFxNameView, m_multiFxTable, PTTransitionMode::MultiFx),
                       tr("MultiFX"));
    auto linkFrozenExpansion = [this](QTreeView* frozen, QTreeWidget* table,
                                      PTTransitionMode mode) {
        if (!frozen || !table)
            return;
        frozen->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(frozen, &QTreeView::expanded, this,
                [this, table, mode](const QModelIndex& idx) {
            if (!idx.isValid())
                return;
            if (m_syncingFrozenExpansion)
                return;
            QTreeWidgetItem* item = table->itemFromIndex(idx);
            if (item)
            {
                ScopedBoolFlag syncGuard(m_syncingFrozenExpansion);
                item->setExpanded(true);
            }
            if (item && !item->parent())
            {
                const int row = item->data(0, kItemPresetIndexRole).toInt();
                expandedSetForMode(mode).insert(row);
            }
        });
        connect(frozen, &QTreeView::collapsed, this,
                [this, table, mode](const QModelIndex& idx) {
            if (!idx.isValid())
                return;
            if (m_syncingFrozenExpansion)
                return;
            QTreeWidgetItem* item = table->itemFromIndex(idx);
            if (item)
            {
                ScopedBoolFlag syncGuard(m_syncingFrozenExpansion);
                item->setExpanded(false);
            }
            if (item && !item->parent())
            {
                const int row = item->data(0, kItemPresetIndexRole).toInt();
                expandedSetForMode(mode).remove(row);
            }
        });
        connect(frozen, &QTreeView::doubleClicked, this,
                [this, table](const QModelIndex& idx) {
            if (!idx.isValid())
                return;
            QTreeWidgetItem* item = table->itemFromIndex(idx);
            if (item)
                item->setExpanded(!item->isExpanded());
        });
        connect(frozen, &QTreeView::customContextMenuRequested, this,
                [this, frozen, table, mode](const QPoint& pos) {
            const QModelIndex idx = frozen->indexAt(pos);
            if (!idx.isValid())
                return;
            QTreeWidgetItem* item = table->itemFromIndex(idx);
            if (!item)
                return;
            activateNameRowContext(table, item);
            showNameContextMenu(mode, table, item, frozen->viewport()->mapToGlobal(pos));
        });
    };
    linkFrozenExpansion(m_sweepNameView, m_sweepTable, PTTransitionMode::SweepOnly);
    linkFrozenExpansion(m_continuousNameView, m_continuousTable, PTTransitionMode::Continuous);
    linkFrozenExpansion(m_channel1DNameView, m_channel1DTable, PTTransitionMode::Channel1D);
    linkFrozenExpansion(m_positionMotionNameView, m_positionMotionTable,
                        PTTransitionMode::PositionMotion);
    linkFrozenExpansion(m_multiFxNameView, m_multiFxTable, PTTransitionMode::MultiFx);
    m_layout->addWidget(m_bankTabs, 1);

    connect(m_enableChk, &QCheckBox::toggled, this, [this]() {
        pushSpatialEnabledToTable();
        notifyTablePresetCacheRefresh();
    });
    connect(m_bankTabs, &QTabWidget::currentChanged,
            this, &PresetTableV2TransitionWidget::slotBankTabChanged);

    if (m_shapeGallery.isEmpty())
        m_shapeGallery = PTShapesGallery::defaultBuiltinItems();
    refreshColumnGroupBarForActiveTab();
    updateRemoveActionLabel();
}

void PresetTableV2TransitionWidget::rebuildAllPresetTables()
{
    rebuildPresetTable(PTTransitionMode::SweepOnly);
    rebuildPresetTable(PTTransitionMode::Continuous);
    rebuildPresetTable(PTTransitionMode::Channel1D);
    rebuildPresetTable(PTTransitionMode::PositionMotion);
    rebuildPresetTable(PTTransitionMode::MultiFx);
}

void PresetTableV2TransitionWidget::updateColumnHeaders(QTreeWidget* table)
{
    if (!table)
        return;

    const PTTransitionMode tableMode = modeForTable(table);
    const bool multiFxTable = tableMode == PTTransitionMode::MultiFx;
    QTreeWidgetItem* multiFxCurrent = multiFxTable ? selectedPresetItem(table) : nullptr;
    const bool multiFxRootContext = !multiFxCurrent
            || !multiFxCurrent->data(0, kItemMultiFxRouteIndexRole).isValid();
    const PTTransitionMode multiFxContextMode = multiFxTable
            ? multiFxContextModeForItem(multiFxCurrent) : PTTransitionMode::MultiFx;
    auto multiFxColumnTitle = [&](int col) -> QString {
        switch (col)
        {
            case ColWaveShape:
                if (multiFxRootContext)
                    return tr("Curve");
                return multiFxContextMode == PTTransitionMode::PositionMotion
                        ? tr("Motion curve") : tr("Wave curve");
            case ColFadeIn:
                if (multiFxRootContext)
                    return tr("Fade in");
                return multiFxContextMode == PTTransitionMode::PositionMotion
                        ? tr("Orbit fade in") : tr("Wave fade in");
            case ColFadeOut:
                if (multiFxRootContext)
                    return tr("Fade out");
                return multiFxContextMode == PTTransitionMode::PositionMotion
                        ? tr("Orbit fade out") : tr("Wave fade out");
            case ColWaveWidth:
                if (multiFxRootContext)
                    return tr("Width °");
                return multiFxContextMode == PTTransitionMode::PositionMotion
                        ? tr("Orbit width °") : tr("Wave width °");
            default:
                return columnTitleForCol(col);
        }
    };

    QStringList headers;
    for (int c = 0; c < ColCount; ++c)
    {
        QString title = (c == ColName) ? tr("Name")
                : (multiFxTable ? multiFxColumnTitle(c) : columnTitleForCol(c));
        if (PTEfxCol::hasExternalInput(c))
        {
            const auto src = inputSource(PTEfxCol::inputIdForColumn(c));
            if (src && src->isValid())
                title += QStringLiteral(" *");
        }
        headers << title;
    }
    table->setHeaderLabels(headers);
    for (int c = 0; c < ColCount; ++c)
    {
        const QString tip = columnTooltipForCol(c);
        if (!tip.isEmpty() && table->headerItem())
            table->headerItem()->setToolTip(c, tip);
    }
}

void PresetTableV2TransitionWidget::rebuildPresetTable(PTTransitionMode mode)
{
    QTreeWidget* table = tableForMode(mode);
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (!table)
        return;

    closeActiveTableEditors(table);
    const bool slaved = bankIsSlaved(mode);
    if (!slaved)
    {
        normalizeOverrideStorage();
        for (int r = 0; r < presets.size(); ++r)
            normalizeNoopOverridesForPreset(mode, r);
    }
    captureExpandedState(mode);
    const int scrollValue = table->verticalScrollBar() ? table->verticalScrollBar()->value() : 0;
    QTreeWidgetItem* currentBefore = table->currentItem();
    const int currentRow = currentBefore ? currentBefore->data(0, kItemPresetIndexRole).toInt() : -1;
    const int currentOutput = currentBefore ? currentBefore->data(0, kItemOutputIndexRole).toInt() : -1;
    const int currentSelection = currentBefore ? currentBefore->data(0, kItemSelectionIndexRole).toInt() : -1;
    QSignalBlocker tableBlocker(table);
    m_rebuildingTable = true;
    m_nameRowContextItemByTable.remove(table);
    m_frozenNameContextItemByTable.remove(table);
    clearCellSelection(table);
    table->clear();
    updateColumnHeaders(table);
    table->setColumnHidden(ColName, true);

    normalizeMultiFxTargetRoutes();
    QVector<PTTransitionPreset> displayPresets;
    QVector<QVector<PTMultiFxTargetTableRoute>> displayMultiFxRoutes;
    displayDataForMode(mode, displayPresets, displayMultiFxRoutes);

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

    auto makeCell = [&](QTreeWidgetItem* item, int row, int outputIdx, int selectionIdx, int col,
                        const PTTransitionPreset& preset, bool inherited) {
        const QVariant value = presetColumnValue(preset, col);
        Q_UNUSED(row)
        Q_UNUSED(outputIdx)
        Q_UNUSED(selectionIdx)
        if (!slaved)
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        setPresetCellValue(item, col, value, inherited);
        const QString tip = columnTooltipForCol(col);
        if (!tip.isEmpty())
            item->setToolTip(col, tip);
    };

    const int outputCount = slaved ? 0 : linkedOutputCount();
    const auto& bankOverrides = overridesForMode(mode);
    for (int r = 0; r < displayPresets.size(); ++r)
    {
        QTreeWidgetItem* parent = new QTreeWidgetItem(table);
        parent->setData(0, kItemPresetIndexRole, r);
        parent->setData(0, kItemOutputIndexRole, -1);
        parent->setData(0, kItemSelectionIndexRole, -1);
        parent->setFlags(slaved ? ((parent->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled)
                                   & ~Qt::ItemIsEditable)
                                : (parent->flags() | Qt::ItemIsEditable));
        parent->setText(ColName, displayPresets[r].name);
        if (slaved)
            parent->setToolTip(ColName, bankSourceDescription(mode));

        for (int col = ColAxis; col < ColCount; ++col)
            makeCell(parent, r, -1, -1, col, displayPresets[r], slaved);
        updatePresetRowUiForItem(parent, mode);
        updateOffsetStepLimitForItem(parent, mode);

        if (mode == PTTransitionMode::MultiFx)
        {
            const QVector<PTMultiFxTargetTableRoute> routes =
                    r < displayMultiFxRoutes.size()
                    ? displayMultiFxRoutes.at(r)
                    : QVector<PTMultiFxTargetTableRoute>();
            for (int routeIdx = 0; routeIdx < routes.size(); ++routeIdx)
            {
                const PTMultiFxTargetTableRoute& route = routes.at(routeIdx);
                QTreeWidgetItem* routeItem = new QTreeWidgetItem(parent);
                routeItem->setData(0, kItemPresetIndexRole, r);
                routeItem->setData(0, kItemOutputIndexRole, -1);
                routeItem->setData(0, kItemSelectionIndexRole, -1);
                routeItem->setData(0, kItemMultiFxRouteIndexRole, routeIdx);
                routeItem->setData(0, kItemMultiFxRouteTableIdRole, route.tableId);
                routeItem->setFlags(slaved ? ((routeItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled)
                                              & ~Qt::ItemIsEditable)
                                           : (routeItem->flags() | Qt::ItemIsEditable
                                              | Qt::ItemIsSelectable | Qt::ItemIsEnabled));
                const MultiFxUiRow routeRow = multiFxUiRowForAddress(
                        r, routeIdx, -1, -1, displayPresets, displayMultiFxRoutes);
                routeItem->setText(ColName, routeRow.label);
                routeItem->setToolTip(ColName, routeRow.tooltip);
                renderPresetRowCells(routeItem, routeRow);

                const QVector<int> outputIndices = multiFxOutputIndicesForRoute(route);
                for (int routeOutputIdx : outputIndices)
                {
                    QTreeWidgetItem* outputItem = new QTreeWidgetItem(routeItem);
                    outputItem->setData(0, kItemPresetIndexRole, r);
                    outputItem->setData(0, kItemOutputIndexRole, routeOutputIdx);
                    outputItem->setData(0, kItemSelectionIndexRole, 0);
                    outputItem->setData(0, kItemMultiFxRouteIndexRole, routeIdx);
                    outputItem->setData(0, kItemMultiFxRouteTableIdRole, route.tableId);
                    outputItem->setData(0, kItemMultiFxRouteOutputIndexRole, routeOutputIdx);
                    outputItem->setFlags(slaved ? ((outputItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled)
                                                  & ~Qt::ItemIsEditable)
                                               : (outputItem->flags() | Qt::ItemIsEditable
                                                  | Qt::ItemIsSelectable | Qt::ItemIsEnabled));
                    const MultiFxUiRow outputRow = multiFxUiRowForAddress(
                            r, routeIdx, routeOutputIdx, -1,
                            displayPresets, displayMultiFxRoutes);
                    outputItem->setText(ColName, outputRow.label);
                    outputItem->setToolTip(ColName, outputRow.tooltip);
                    renderPresetRowCells(outputItem, outputRow);

                    const PTTransitionProviderOutputLayer outputLayer =
                            route.outputOverrides.value(routeOutputIdx);
                    const QVector<PTTransitionProviderSelection> selections =
                            outputLayer.selections;
                    for (int s = 0; s < selections.size(); ++s)
                    {
                        QTreeWidgetItem* selItem = new QTreeWidgetItem(outputItem);
                        selItem->setData(0, kItemPresetIndexRole, r);
                        selItem->setData(0, kItemOutputIndexRole, routeOutputIdx);
                        selItem->setData(0, kItemSelectionIndexRole, s + 1);
                        selItem->setData(0, kItemMultiFxRouteIndexRole, routeIdx);
                        selItem->setData(0, kItemMultiFxRouteTableIdRole, route.tableId);
                        selItem->setData(0, kItemMultiFxRouteOutputIndexRole,
                                         routeOutputIdx);
                        selItem->setFlags(slaved
                                ? ((selItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled)
                                   & ~Qt::ItemIsEditable)
                                : (selItem->flags() | Qt::ItemIsEditable
                                   | Qt::ItemIsSelectable | Qt::ItemIsEnabled));
                        const MultiFxUiRow selectionRow = multiFxUiRowForAddress(
                                r, routeIdx, routeOutputIdx, s,
                                displayPresets, displayMultiFxRoutes);
                        selItem->setText(ColName, selectionRow.label);
                        selItem->setToolTip(ColName, selectionRow.tooltip);
                        renderPresetRowCells(selItem, selectionRow);
                        applySelectionRowVisuals(selItem);
                    }
                }
            }
        }

        if (mode == PTTransitionMode::MultiFx)
        {
            refreshMultiFxRouteVisualsForPreset(r, displayPresets, displayMultiFxRoutes);
            parent->setExpanded(expandedSetForMode(mode).contains(r));
            continue;
        }

        for (int o = 0; o < outputCount; ++o)
        {
            QTreeWidgetItem* outputItem = new QTreeWidgetItem(parent);
            outputItem->setData(0, kItemPresetIndexRole, r);
            outputItem->setData(0, kItemOutputIndexRole, o);
            outputItem->setData(0, kItemSelectionIndexRole, 0);
            outputItem->setFlags((outputItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled)
                            & ~Qt::ItemIsEditable);
            outputItem->setText(ColName, linkedOutputName(o) + tr(" / All"));
            outputItem->setToolTip(ColName, tr("All cells in this output that are not assigned to a custom selection"));
            const PTTransitionPreset effective = effectivePresetForOutputNoLive(mode, r, o);
            for (int col = ColAxis; col < ColCount; ++col)
            {
                const bool inherited = !(r < bankOverrides.size()
                        && bankOverrides.at(r).contains(o)
                        && bankOverrides.at(r).value(o).all.columns.contains(col));
                makeCell(outputItem, r, o, 0, col, effective, inherited);
            }
            updatePresetRowUiForItem(outputItem, mode);
            updateOffsetStepLimitForItem(outputItem, mode);

            if (r < bankOverrides.size() && bankOverrides.at(r).contains(o))
            {
                const QVector<PTTransitionSelection> selections =
                        bankOverrides.at(r).value(o).selections;
                for (int s = 0; s < selections.size(); ++s)
                {
                    QTreeWidgetItem* selItem = new QTreeWidgetItem(outputItem);
                    selItem->setData(0, kItemPresetIndexRole, r);
                    selItem->setData(0, kItemOutputIndexRole, o);
                    selItem->setData(0, kItemSelectionIndexRole, s + 1);
                    selItem->setFlags((selItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled
                                       | Qt::ItemIsEditable));
                    const QString label = selections.at(s).name.isEmpty()
                            ? tr("Selection %1").arg(s + 1) : selections.at(s).name;
                    selItem->setText(ColName, label);
                    selItem->setToolTip(ColName,
                            tr("Custom selection (%1 cell(s)); select this row and click Fixture Grid cells")
                            .arg(selections.at(s).cells.size()));
                    const PTTransitionPreset selPreset =
                            effectivePresetForSelectionNoLive(mode, r, o, s);
                    for (int col = ColAxis; col < ColCount; ++col)
                    {
                        const bool inherited = !selections.at(s).overrides.columns.contains(col);
                        makeCell(selItem, r, o, s + 1, col, selPreset, inherited);
                    }
                    updatePresetRowUiForItem(selItem, mode);
                    updateOffsetStepLimitForItem(selItem, mode);
                    applySelectionRowVisuals(selItem);
                }
            }
        }
        refreshOverrideVisualsForPreset(mode, r);
        parent->setExpanded(expandedSetForMode(mode).contains(r));
    }

    applyColumnGroupFilter(table, mode);
    applyDefaultColumnWidths(table);
    configureFrozenNameView(mode);
    updateBankSourceUi(mode);
    if (currentRow >= 0)
    {
        if (QTreeWidgetItem* restore =
                itemForPresetAddress(mode, currentRow, currentOutput, currentSelection))
            table->setCurrentItem(restore);
    }
    if (table->verticalScrollBar())
        table->verticalScrollBar()->setValue(scrollValue);
    m_rebuildingTable = false;
    updateCellSelectionVisuals(table);
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
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    if (row < 0 || row >= presets.size())
        return p;

    p = presets[row];
    p.playbackMode = (mode == PTTransitionMode::Continuous
                      || mode == PTTransitionMode::PositionMotion
                      || mode == PTTransitionMode::Channel1D
                      || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;

    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIndex = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
        p = effectiveMultiFxRoutePreset(row, routeIndex, routeOutputIdx,
                                        selectionIdx > 0 ? selectionIdx - 1 : -1);
    }
    else if (outputIdx >= 0 && selectionIdx > 0)
        p = effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1);
    else if (outputIdx >= 0)
        p = effectivePresetForOutputNoLive(mode, row, outputIdx);

    if (outputIdx < 0)
        p.name = item->text(ColName);

    const QVector<PTCustomCurvePoint> storedCustomCurve = p.customCurve;
    const bool storedCustomCurveEnabled = p.customCurveEnabled;
    const QVariant motionValue = item->data(ColPositionMotion, kPresetCellValueRole);
    const PTPositionMotion positionMotion = PTPositionMotion(
            motionValue.isValid() ? motionValue.toInt() : p.positionMotion);
    const bool waveShapeIsMotionCurve1D = mode != PTTransitionMode::SweepOnly
            && linkedTableUsesPositionMode()
            && (positionMotion == PTPositionMotion::Pan1D
                || positionMotion == PTPositionMotion::Tilt1D);

    for (int col = ColAxis; col < ColCount; ++col)
    {
        const QVariant value = item->data(col, kPresetCellValueRole);
        if (!value.isValid())
            continue;
        setPresetColumnValue(p, col, value);
        if (col == ColWaveShape && value.toInt() == 3)
        {
            p.customCurveEnabled = true;
            p.waveShape = 0;
            if (storedCustomCurveEnabled && storedCustomCurve.size() >= 2)
                p.customCurve = storedCustomCurve;
            else if (waveShapeIsMotionCurve1D)
                p.customCurve = PTShapesGallery::defaultMotionCurve1D();
        }
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
    QTreeWidget* table = item ? item->treeWidget() : nullptr;
    if (!table || !item)
        return;

    ScopedBoolFlag commitGuard(m_committingPresetCell);
    QSignalBlocker blocker(table);
    const bool sweep = (bankMode == PTTransitionMode::SweepOnly);

    auto tuneValue = [&](int col, bool enabled, int minV, int maxV, int value) {
        const int bounded = qBound(minV, value, maxV);
        setPresetCellValue(item, col, bounded,
                           item->data(col, kPresetCellInheritedRole).toBool());
        item->setToolTip(col, enabled ? columnTooltipForCol(col) : tr("Fixed by this mode"));
    };

    const int waveWidth = item->data(ColWaveWidth, kPresetCellValueRole).isValid()
            ? item->data(ColWaveWidth, kPresetCellValueRole).toInt() : 180;
    const int waveLevel = item->data(ColWaveLevel, kPresetCellValueRole).isValid()
            ? item->data(ColWaveLevel, kPresetCellValueRole).toInt() : 255;
    const int fadeIn = item->data(ColFadeIn, kPresetCellValueRole).isValid()
            ? item->data(ColFadeIn, kPresetCellValueRole).toInt() : 25;
    const int fadeOut = item->data(ColFadeOut, kPresetCellValueRole).isValid()
            ? item->data(ColFadeOut, kPresetCellValueRole).toInt() : 25;

    if (sweep)
    {
        tuneValue(ColWaveWidth, false, 360, 360, 360);
        tuneValue(ColWaveLevel, false, 255, 255, 255);
        tuneValue(ColFadeIn, true, 0, 50, qMin(fadeIn, 50));
        tuneValue(ColFadeOut, true, 0, 50, qMin(fadeOut, 50));
    }
    else
    {
        tuneValue(ColWaveWidth, true, 1, 360, waveWidth);
        tuneValue(ColWaveLevel, true, 0, 255, waveLevel);
        tuneValue(ColFadeIn, true, 0, 100, fadeIn);
        tuneValue(ColFadeOut, true, 0, 100, fadeOut);
    }

    if (linkedTableUsesPositionMode()
            && !item->data(0, kItemMultiFxRouteIndexRole).isValid())
    {
        const int motion = item->data(ColPositionMotion, kPresetCellValueRole).isValid()
                ? item->data(ColPositionMotion, kPresetCellValueRole).toInt()
                : int(PTPositionMotion::Off);
        const bool needsPan = motion == int(PTPositionMotion::Pan1D)
                || motion == int(PTPositionMotion::Circle2D)
                || motion == int(PTPositionMotion::Line2D)
                || motion == int(PTPositionMotion::Figure8_2D)
                || motion == int(PTPositionMotion::CustomPan1D)
                || motion == int(PTPositionMotion::Custom2D);
        const bool needsTilt = motion == int(PTPositionMotion::Tilt1D)
                || motion == int(PTPositionMotion::Circle2D)
                || motion == int(PTPositionMotion::Figure8_2D)
                || motion == int(PTPositionMotion::CustomTilt1D)
                || motion == int(PTPositionMotion::Custom2D);
        const int panSize = item->data(ColPositionPanSize, kPresetCellValueRole).isValid()
                ? item->data(ColPositionPanSize, kPresetCellValueRole).toInt() : 45;
        const int tiltSize = item->data(ColPositionTiltSize, kPresetCellValueRole).isValid()
                ? item->data(ColPositionTiltSize, kPresetCellValueRole).toInt() : 30;
        tuneValue(ColPositionPanSize, needsPan, 0, 540, panSize);
        tuneValue(ColPositionTiltSize, needsTilt, 0, 270, tiltSize);
    }
}

void PresetTableV2TransitionWidget::syncPresetFromTable(PTTransitionMode mode, int row)
{
    if (bankIsSlaved(mode))
        return;
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
                                                      int col, int outputIdx,
                                                      int selectionIdx,
                                                      int routeIdx,
                                                      int routeOutputIdx)
{
    if (m_rebuildingTable)
        return;
    if (bankIsSlaved(mode))
        return;

    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return;
    if (col > ColName && col < ColCount
            && !(mode == PTTransitionMode::MultiFx && routeIdx >= 0)
            && !isPresetColumnAllowedForMode(mode, col))
        return;

    if (mode == PTTransitionMode::MultiFx && routeIdx >= 0)
    {
        normalizeMultiFxTargetRoutes();
        if (row < 0 || row >= m_multiFxTargetRoutes.size()
                || routeIdx >= m_multiFxTargetRoutes.at(row).size())
            return;
        QTreeWidgetItem* item = itemForMultiFxRouteAddress(row, routeIdx,
                                                          routeOutputIdx,
                                                          selectionIdx);
        if (!item)
            return;

        PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
        const PTTransitionMode routeMode = multiFxRouteMode(route);
        if (!allowedMultiFxColumnsForContext(routeMode, false).contains(col))
            return;
        const QSet<int> smartDefaultCols =
                (col == ColWings) ? applySmartWingsDefaults(routeMode, item) : QSet<int>();
        QSet<int> changedCols = smartDefaultCols;
        changedCols.insert(col);
        updatePresetRowUiForItem(item, routeMode);
        const PTTransitionPreset itemPreset = presetFromItem(mode, item);

        if (routeOutputIdx >= 0)
        {
            bool knownOutput = false;
            for (const PTMultiFxTargetOutputRoute& out : route.outputs)
            {
                if (out.outputIndex == routeOutputIdx)
                {
                    knownOutput = true;
                    break;
                }
            }
            if (!knownOutput)
            {
                PTMultiFxTargetOutputRoute out;
                out.outputIndex = routeOutputIdx;
                route.outputs.append(out);
            }

            PTTransitionProviderOutputLayer layer =
                    route.outputOverrides.value(routeOutputIdx);
            if (selectionIdx > 0)
            {
                const int sel = selectionIdx - 1;
                while (layer.selections.size() <= sel)
                {
                    PTTransitionProviderSelection selection;
                    selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                    layer.selections.append(selection);
                }
                PTTransitionProviderPresetOverride ov = layer.selections[sel].overrides;
                ov.values = effectiveMultiFxRoutePreset(row, routeIdx, routeOutputIdx, sel);
                setColumnOverrideValue(ov, col, itemPreset);
                for (int smartCol : smartDefaultCols)
                    setColumnOverrideValue(ov, smartCol, itemPreset);
                layer.selections[sel].overrides = ov;
            }
            else
            {
                PTTransitionProviderPresetOverride ov = layer.all;
                ov.values = effectiveMultiFxRoutePreset(row, routeIdx, routeOutputIdx, -1);
                setColumnOverrideValue(ov, col, itemPreset);
                for (int smartCol : smartDefaultCols)
                    setColumnOverrideValue(ov, smartCol, itemPreset);
                layer.all = ov;
                for (PTTransitionProviderSelection& selection : layer.selections)
                {
                    for (int changedCol : changedCols)
                        clearMultiFxProviderOverrideColumn(selection.overrides, changedCol);
                }
            }
            route.outputOverrides.insert(routeOutputIdx, layer);
        }
        else
        {
            PTTransitionProviderPresetOverride ov = route.tableOverride;
            ov.values = effectiveMultiFxRoutePreset(row, routeIdx, -1, -1);
            setColumnOverrideValue(ov, col, itemPreset);
            for (int smartCol : smartDefaultCols)
                setColumnOverrideValue(ov, smartCol, itemPreset);
            route.tableOverride = ov;
            clearMultiFxDescendantColumnOverrides(row, routeIdx, -1, -1, changedCols);
        }

        expandedSetForMode(mode).insert(row);
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
            parent->setExpanded(true);
        updateOffsetStepLimitForItem(item, routeMode);
        refreshOverrideVisualsForPreset(mode, row);
        publishProviderSnapshot(QStringLiteral("multifx route edit"));
        scheduleDeferredPresetCacheRefreshAndPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }

    if (col == ColWaveShape && !m_pastingCells
            && !m_editingCustomCurve && !m_committingCustomDialog)
    {
        QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
        const int waveShape = item ? item->data(ColWaveShape, kPresetCellValueRole).toInt() : -1;
        if (item && waveShape == 3)
        {
            const PTTransitionPreset before = selectionIdx > 0
                    ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
                    : (outputIdx >= 0
                       ? effectivePresetForOutputNoLive(mode, row, outputIdx)
                       : presets.at(row));
            QTimer::singleShot(0, this, [this, mode, row, outputIdx, selectionIdx, before]() {
                if (m_rebuildingTable)
                    return;
                QTreeWidgetItem* deferredItem =
                        itemForPresetAddress(mode, row, outputIdx, selectionIdx);
                if (!deferredItem)
                    return;
                const bool edited = colWaveShapeEditsMotionCurve(mode, row, outputIdx, selectionIdx)
                        ? editPositionMotionCurve1DForPreset(mode, row, outputIdx, selectionIdx)
                        : editCustomCurveForPreset(mode, row, outputIdx, selectionIdx);
                if (!edited)
                {
                    setPresetCellValue(deferredItem, ColWaveShape,
                                       before.customCurveEnabled ? 3 : before.waveShape,
                                       deferredItem->data(ColWaveShape, kPresetCellInheritedRole).toBool());
                    updatePresetRowUiForItem(deferredItem, mode);
                    updateOffsetStepLimitForItem(deferredItem, mode);
                }
            });
            return;
        }
    }

    if (col == ColPositionMotion && !m_pastingCells && !m_committingCustomDialog)
    {
        QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
        const int motion = item ? item->data(ColPositionMotion, kPresetCellValueRole).toInt() : -1;
        if (item && motion == int(PTPositionMotion::Custom2D))
        {
            const PTTransitionPreset before = selectionIdx > 0
                    ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
                    : (outputIdx >= 0
                       ? effectivePresetForOutputNoLive(mode, row, outputIdx)
                       : presets.at(row));
            QTimer::singleShot(0, this, [this, mode, row, outputIdx, selectionIdx, before, motion]() {
                if (m_rebuildingTable)
                    return;
                QTreeWidgetItem* deferredItem =
                        itemForPresetAddress(mode, row, outputIdx, selectionIdx);
                if (!deferredItem)
                    return;
                if (!editPositionShapeForPreset(mode, row, outputIdx, selectionIdx, motion))
                {
                    setPresetCellValue(deferredItem, ColPositionMotion, before.positionMotion,
                                       deferredItem->data(ColPositionMotion, kPresetCellInheritedRole).toBool());
                    updatePresetRowUiForItem(deferredItem, mode);
                    scheduleDeferredPresetCacheRefreshAndPreview();
                }
            });
            return;
        }
    }

    QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
    if (!item)
        return;

    const QSet<int> smartDefaultCols =
            (col == ColWings) ? applySmartWingsDefaults(mode, item) : QSet<int>();
    QSet<int> changedCols = smartDefaultCols;
    changedCols.insert(col);

    if (outputIdx >= 0)
    {
        updatePresetRowUiForItem(item, mode);
        const PTTransitionPreset itemPreset = presetFromItem(mode, item);
        QVector<QHash<int, PTTransitionOutputLayer>>& allOverrides = overridesForMode(mode);
        while (allOverrides.size() <= row)
            allOverrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = allOverrides[row].value(outputIdx);
        if (selectionIdx > 0)
        {
            const int sel = selectionIdx - 1;
            while (layer.selections.size() <= sel)
            {
                PTTransitionSelection selection;
                selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                layer.selections.append(selection);
            }
            PTTransitionPresetOverride ov = layer.selections[sel].overrides;
            ov.values = effectivePresetForSelectionNoLive(mode, row, outputIdx, sel);
            setColumnOverrideValue(ov, col, itemPreset);
            for (int smartCol : smartDefaultCols)
                setColumnOverrideValue(ov, smartCol, itemPreset);
            layer.selections[sel].overrides = ov;
        }
        else
        {
            PTTransitionPresetOverride ov = layer.all;
            ov.values = effectivePresetForOutputNoLive(mode, row, outputIdx);
            setColumnOverrideValue(ov, col, itemPreset);
            for (int smartCol : smartDefaultCols)
                setColumnOverrideValue(ov, smartCol, itemPreset);
            layer.all = ov;
            for (PTTransitionSelection& selection : layer.selections)
            {
                for (int changedCol : changedCols)
                    clearPresetOverrideColumn(selection.overrides, changedCol);
            }
        }
        allOverrides[row].insert(outputIdx, layer);
        expandedSetForMode(mode).insert(row);
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
            parent->setExpanded(true);
    }
    else
    {
        syncPresetFromTable(mode, row);
        updatePresetRowUiForItem(item, mode);
        if (mode == PTTransitionMode::MultiFx)
            clearMultiFxDescendantColumnOverrides(row, -1, -1, -1, changedCols);
        else
            clearDescendantColumnOverrides(mode, row, -1, -1, changedCols);
    }
    normalizeNoopOverridesForPreset(mode, row);
    for (int i = 0; item && outputIdx < 0 && i < item->childCount(); ++i)
    {
        updatePresetRowUiForItem(item->child(i), mode);
        updateOffsetStepLimitForItem(item->child(i), mode);
        for (int j = 0; j < item->child(i)->childCount(); ++j)
        {
            updatePresetRowUiForItem(item->child(i)->child(j), mode);
            updateOffsetStepLimitForItem(item->child(i)->child(j), mode);
        }
    }
    updateOffsetStepLimitForItem(item, mode);
    refreshOverrideVisualsForPreset(mode, row);
    scheduleDeferredPresetCacheRefreshAndPreview();
    if (m_doc)
        m_doc->setModified();
}

void PresetTableV2TransitionWidget::slotPresetItemChanged(QTreeWidgetItem* item, int col)
{
    if (m_rebuildingTable || m_committingPresetCell || m_committingDelegateEditor
            || m_closingTableEditors || m_committingCustomDialog)
        return;
    if (!item)
        return;

    QTreeWidget* table = qobject_cast<QTreeWidget*>(sender());
    PTTransitionMode mode = PTTransitionMode::SweepOnly;
    if (table == m_continuousTable)
        mode = PTTransitionMode::Continuous;
    else if (table == m_positionMotionTable)
        mode = PTTransitionMode::PositionMotion;
    else if (table == m_channel1DTable)
        mode = PTTransitionMode::Channel1D;
    else if (table == m_multiFxTable)
        mode = PTTransitionMode::MultiFx;
    else if (table != m_sweepTable)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    const int routeIdx = item->data(0, kItemMultiFxRouteIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteIndexRole).toInt() : -1;
    const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
    if (col > ColName && col < ColCount)
    {
        slotPresetChanged(mode, row, col, outputIdx, selectionIdx,
                          routeIdx, routeOutputIdx);
        return;
    }
    if (col != ColName)
        return;
    if (mode == PTTransitionMode::MultiFx && routeIdx >= 0)
    {
        if (selectionIdx > 0 && row >= 0 && row < m_multiFxTargetRoutes.size()
                && routeIdx < m_multiFxTargetRoutes.at(row).size()
                && routeOutputIdx >= 0)
        {
            PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
            PTTransitionProviderOutputLayer layer =
                    route.outputOverrides.value(routeOutputIdx);
            const int sel = selectionIdx - 1;
            if (sel >= 0 && sel < layer.selections.size())
            {
                layer.selections[sel].name = item->text(ColName).trimmed();
                route.outputOverrides.insert(routeOutputIdx, layer);
                publishProviderSnapshot(QStringLiteral("multifx route selection rename"));
                if (m_doc)
                    m_doc->setModified();
            }
        }
        return;
    }
    if (outputIdx >= 0 && selectionIdx > 0)
    {
        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        while (overrides.size() <= row)
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
        const int sel = selectionIdx - 1;
        if (sel >= 0 && sel < layer.selections.size())
        {
            layer.selections[sel].name = item->text(ColName).trimmed();
            overrides[row].insert(outputIdx, layer);
            VCPluginDiagnostics::breadcrumb(
                    QStringLiteral("presettablev2transition"), id(), caption(),
                    QStringLiteral("rename selection mode=%1 row=%2 output=%3 selection=%4 name=\"%5\"")
                            .arg(int(mode)).arg(row).arg(outputIdx).arg(sel)
                            .arg(layer.selections[sel].name));
            scheduleDeferredPresetCacheRefreshAndPreview();
            if (m_doc)
                m_doc->setModified();
        }
        return;
    }
    if (outputIdx >= 0)
        return;
    const QString name = item->text(ColName).trimmed();
    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("rename preset mode=%1 row=%2 name=\"%3\"")
                    .arg(int(mode)).arg(row).arg(name));
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row >= 0 && row < presets.size())
    {
        presets[row].name = name;
        scheduleDeferredPresetCacheRefreshAndPreview();
        if (m_doc)
            m_doc->setModified();
    }
}

void PresetTableV2TransitionWidget::slotOpenCustomCurveEditor()
{
    QTreeWidget* table = activeTable();
    if (!table)
        return;
    QTreeWidgetItem* item = selectedPresetItem(table);
    const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
    const int outputIdx = item ? item->data(0, kItemOutputIndexRole).toInt() : -1;
    const int selectionIdx = item ? item->data(0, kItemSelectionIndexRole).toInt() : -1;
    const PTTransitionMode mode = activeBankMode();
    if (bankIsSlaved(mode))
        return;
    QTimer::singleShot(0, this, [this, mode, row, outputIdx, selectionIdx]() {
        editCustomCurveForPreset(mode, row, outputIdx, selectionIdx);
    });
}

void PresetTableV2TransitionWidget::slotOpenMotionCurveEditor()
{
    QTreeWidget* table = activeTable();
    if (!table)
        return;
    QTreeWidgetItem* item = selectedPresetItem(table);
    const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
    const int outputIdx = item ? item->data(0, kItemOutputIndexRole).toInt() : -1;
    const int selectionIdx = item ? item->data(0, kItemSelectionIndexRole).toInt() : -1;
    if (row < 0)
        return;

    const PTTransitionMode mode = activeBankMode();
    if (bankIsSlaved(mode))
        return;
    if (mode != PTTransitionMode::PositionMotion && mode != PTTransitionMode::MultiFx)
        return;
    const PTTransitionPreset preset = selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
            : (outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, row, outputIdx)
               : presetsForMode(mode).at(row));
    if (!PTPositionFxEngine::motionIs1D(PTPositionMotion(preset.positionMotion)))
        return;

    QTimer::singleShot(0, this, [this, mode, row, outputIdx, selectionIdx]() {
        editPositionMotionCurve1DForPreset(mode, row, outputIdx, selectionIdx);
    });
}

void PresetTableV2TransitionWidget::refreshCustomCurveVisualsAfterEdit(PTTransitionMode mode,
                                                                     int row,
                                                                     int outputIdx,
                                                                     int selectionIdx)
{
    QTreeWidgetItem* parent = parentItemForPreset(mode, row);
    if (!parent)
        return;

    const auto& overrides = overridesForMode(mode);

    auto waveShapeInherited = [&](int oIdx, int selIdx) -> bool {
        if (oIdx < 0)
            return false;
        if (row >= overrides.size() || !overrides.at(row).contains(oIdx))
            return true;
        const PTTransitionOutputLayer layer = overrides.at(row).value(oIdx);
        if (selIdx > 0)
        {
            const int sel = selIdx - 1;
            return !(sel >= 0 && sel < layer.selections.size()
                    && layer.selections.at(sel).overrides.columns.contains(ColWaveShape));
        }
        return !layer.all.columns.contains(ColWaveShape);
    };

    auto refreshWaveShapeCell = [&](QTreeWidgetItem* item, bool inherited) {
        if (!item)
            return;
        const int itemOutputIdx = item->data(0, kItemOutputIndexRole).toInt();
        const int itemSelIdx = item->data(0, kItemSelectionIndexRole).toInt();
        const PTTransitionPreset effective = itemSelIdx > 0
                ? effectivePresetForSelectionNoLive(mode, row, itemOutputIdx, itemSelIdx - 1)
                : (itemOutputIdx >= 0
                   ? effectivePresetForOutputNoLive(mode, row, itemOutputIdx)
                   : presetsForMode(mode).at(row));
        setPresetCellValue(item, ColWaveShape, presetColumnValue(effective, ColWaveShape),
                           inherited);
    };

    if (outputIdx < 0 && selectionIdx <= 0)
    {
        refreshWaveShapeCell(parent, false);
        for (int i = 0; i < parent->childCount(); ++i)
        {
            QTreeWidgetItem* outItem = parent->child(i);
            const int oIdx = outItem->data(0, kItemOutputIndexRole).toInt();
            refreshWaveShapeCell(outItem, waveShapeInherited(oIdx, 0));
            for (int j = 0; j < outItem->childCount(); ++j)
            {
                QTreeWidgetItem* selItem = outItem->child(j);
                const int selIdx = selItem->data(0, kItemSelectionIndexRole).toInt();
                refreshWaveShapeCell(selItem, waveShapeInherited(oIdx, selIdx));
            }
        }
        return;
    }

    QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
    refreshWaveShapeCell(item, waveShapeInherited(outputIdx, selectionIdx));
}

void PresetTableV2TransitionWidget::finishCustomCurveEdit(PTTransitionMode mode, int row,
                                                          int outputIdx, int selectionIdx)
{
    QTreeWidget* table = tableForMode(mode);
    closeActiveTableEditors(table);
    QScopedPointer<QSignalBlocker> signalBlocker;
    if (table)
        signalBlocker.reset(new QSignalBlocker(table));
    refreshCustomCurveVisualsAfterEdit(mode, row, outputIdx, selectionIdx);
    normalizeNoopOverridesForPreset(mode, row);
    scheduleDeferredPresetCacheRefreshAndPreview();
    if (m_doc)
        m_doc->setModified();
}

bool PresetTableV2TransitionWidget::colWaveShapeEditsMotionCurve(PTTransitionMode mode, int row,
                                                                 int outputIdx,
                                                                 int selectionIdx) const
{
    if ((mode != PTTransitionMode::PositionMotion && mode != PTTransitionMode::MultiFx)
            || !linkedTableUsesPositionMode())
        return false;

    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return false;

    const PTTransitionPreset preset = selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
            : (outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, row, outputIdx)
               : presets.at(row));
    const PTPositionMotion motion = PTPositionMotion(preset.positionMotion);
    return motion == PTPositionMotion::Pan1D || motion == PTPositionMotion::Tilt1D;
}

bool PresetTableV2TransitionWidget::editCustomCurveForPreset(PTTransitionMode mode, int row,
                                                             int outputIdx, int selectionIdx)
{
    if (m_editingCustomCurve || m_committingCustomDialog || m_rebuildingTable)
        return false;

    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return false;

    ScopedBoolFlag editingGuard(m_editingCustomCurve);
    ScopedBoolFlag customDialogGuard(m_committingCustomDialog);

    PTTransitionPreset candidate = selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
            : (outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, row, outputIdx)
               : presets.at(row));
    candidate.customCurveEnabled = true;
    candidate.waveShape = 0;
    if (candidate.customCurve.size() < 2)
        candidate.customCurve = defaultTransitionCustomCurve();

    closeActiveTableEditors(tableForMode(mode));

    PTCustomCurveDialog dlg(candidate, m_customCurveGallery, this);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    candidate.customCurve = dlg.customCurve();
    if (candidate.customCurve.size() < 2)
        candidate.customCurve = defaultTransitionCustomCurve();
    candidate.customCurveEnabled = true;
    candidate.waveShape = 0;
    m_customCurveGallery = dlg.gallery();
    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("custom curve accepted mode=%1 row=%2 output=%3 selection=%4 points=%5")
                    .arg(int(mode)).arg(row).arg(outputIdx).arg(selectionIdx)
                    .arg(candidate.customCurve.size()));

    if (outputIdx >= 0)
    {
        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        while (overrides.size() <= row)
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
        if (selectionIdx > 0)
        {
            const int sel = selectionIdx - 1;
            while (layer.selections.size() <= sel)
            {
                PTTransitionSelection selection;
                selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                layer.selections.append(selection);
            }
            PTTransitionPresetOverride ov = layer.selections[sel].overrides;
            ov.values = effectivePresetForSelectionNoLive(mode, row, outputIdx, sel);
            setColumnOverrideValue(ov, ColWaveShape, candidate);
            layer.selections[sel].overrides = ov;
        }
        else
        {
            PTTransitionPresetOverride ov = layer.all;
            ov.values = effectivePresetForOutputNoLive(mode, row, outputIdx);
            setColumnOverrideValue(ov, ColWaveShape, candidate);
            layer.all = ov;
        }
        overrides[row].insert(outputIdx, layer);
    }
    else
    {
        presets[row] = candidate;
    }

    QTreeWidget* table = tableForMode(mode);
    if (table)
    {
        QSignalBlocker blocker(table);
        QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
        if (item)
            setPresetCellValue(item, ColWaveShape, 3,
                               item->data(ColWaveShape, kPresetCellInheritedRole).toBool());
    }

    finishCustomCurveEdit(mode, row, outputIdx, selectionIdx);
    return true;
}

bool PresetTableV2TransitionWidget::editPositionMotionCurve1DForPreset(PTTransitionMode mode,
                                                                       int row,
                                                                       int outputIdx,
                                                                       int selectionIdx)
{
    if (m_editingCustomCurve || m_committingCustomDialog || m_rebuildingTable)
        return false;

    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return false;

    PTTransitionPreset candidate = selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
            : (outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, row, outputIdx)
               : presets.at(row));
    const PTPositionMotion originalMotion = PTPositionMotion(candidate.positionMotion);
    if (originalMotion != PTPositionMotion::Pan1D && originalMotion != PTPositionMotion::Tilt1D)
        return false;

    ScopedBoolFlag editingGuard(m_editingCustomCurve);
    ScopedBoolFlag customDialogGuard(m_committingCustomDialog);

    candidate.customCurveEnabled = true;
    candidate.waveShape = 0;
    if (candidate.customCurve.size() < 2)
        candidate.customCurve = PTShapesGallery::defaultMotionCurve1D();

    PTTransitionPreset dialogPreset = candidate;
    const PTPositionMotion dialogMotion = originalMotion == PTPositionMotion::Tilt1D
            ? PTPositionMotion::CustomTilt1D : PTPositionMotion::CustomPan1D;
    dialogPreset.positionMotion = int(dialogMotion);

    closeActiveTableEditors(tableForMode(mode));

    PTPositionShapeDialog dlg(dialogMotion, dialogPreset, m_shapeGallery, this);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    candidate.customCurve = dlg.motionCurve1D();
    if (candidate.customCurve.size() < 2)
        candidate.customCurve = PTShapesGallery::defaultMotionCurve1D();
    candidate.customCurveEnabled = true;
    candidate.waveShape = 0;
    candidate.positionMotion = int(originalMotion);
    m_shapeGallery = dlg.gallery();
    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("motion curve 1d accepted mode=%1 row=%2 output=%3 selection=%4 motion=%5 points=%6")
                    .arg(int(mode)).arg(row).arg(outputIdx).arg(selectionIdx)
                    .arg(int(originalMotion)).arg(candidate.customCurve.size()));

    if (outputIdx >= 0)
    {
        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        while (overrides.size() <= row)
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
        if (selectionIdx > 0)
        {
            const int sel = selectionIdx - 1;
            while (layer.selections.size() <= sel)
            {
                PTTransitionSelection selection;
                selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                layer.selections.append(selection);
            }
            PTTransitionPresetOverride ov = layer.selections[sel].overrides;
            ov.values = effectivePresetForSelectionNoLive(mode, row, outputIdx, sel);
            setColumnOverrideValue(ov, ColWaveShape, candidate);
            layer.selections[sel].overrides = ov;
        }
        else
        {
            PTTransitionPresetOverride ov = layer.all;
            ov.values = effectivePresetForOutputNoLive(mode, row, outputIdx);
            setColumnOverrideValue(ov, ColWaveShape, candidate);
            layer.all = ov;
        }
        overrides[row].insert(outputIdx, layer);
    }
    else
    {
        presets[row] = candidate;
    }

    QTreeWidget* table = tableForMode(mode);
    if (table)
    {
        QSignalBlocker blocker(table);
        QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
        if (item)
            setPresetCellValue(item, ColWaveShape, 3,
                               item->data(ColWaveShape, kPresetCellInheritedRole).toBool());
    }

    finishCustomCurveEdit(mode, row, outputIdx, selectionIdx);
    return true;
}

void PresetTableV2TransitionWidget::finishPositionShapeEdit(PTTransitionMode mode, int row,
                                                            int outputIdx, int selectionIdx)
{
    QTreeWidget* table = tableForMode(mode);
    closeActiveTableEditors(table);
    QScopedPointer<QSignalBlocker> signalBlocker;
    if (table)
        signalBlocker.reset(new QSignalBlocker(table));

    if (QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx))
    {
        const int itemOutputIdx = item->data(0, kItemOutputIndexRole).toInt();
        const int itemSelIdx = item->data(0, kItemSelectionIndexRole).toInt();
        const PTTransitionPreset effective = itemSelIdx > 0
                ? effectivePresetForSelectionNoLive(mode, row, itemOutputIdx, itemSelIdx - 1)
                : (itemOutputIdx >= 0
                   ? effectivePresetForOutputNoLive(mode, row, itemOutputIdx)
                   : presetsForMode(mode).at(row));
        setPresetCellValue(item, ColPositionMotion,
                           presetColumnValue(effective, ColPositionMotion),
                           item->data(ColPositionMotion, kPresetCellInheritedRole).toBool());
    }

    refreshOverrideVisualsForPreset(mode, row);
    normalizeNoopOverridesForPreset(mode, row);
    scheduleDeferredPresetCacheRefreshAndPreview();
    if (m_doc)
        m_doc->setModified();
}

bool PresetTableV2TransitionWidget::editPositionShapeForPreset(PTTransitionMode mode, int row,
                                                               int outputIdx, int selectionIdx,
                                                               int motionFromUi)
{
    if (m_committingCustomDialog || m_rebuildingTable)
        return false;

    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return false;

    ScopedBoolFlag customDialogGuard(m_committingCustomDialog);

    PTTransitionPreset candidate = selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
            : (outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, row, outputIdx)
               : presets.at(row));
    const PTPositionMotion motion = motionFromUi >= 0
            ? PTPositionMotion(motionFromUi)
            : PTPositionMotion(candidate.positionMotion);
    if (motion != PTPositionMotion::Custom2D)
        return false;

    if (candidate.positionPath2D.size() < 2)
        candidate.positionPath2D = PTShapesGallery::defaultMotionPath2D();
    if (motionFromUi >= 0)
        candidate.positionMotion = int(motion);

    PTPositionShapeDialog dlg(motion, candidate, m_shapeGallery, this);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    candidate.positionPath2D = dlg.motionPath2D();
    candidate.positionPath2DClosed = dlg.path2DClosed();
    m_shapeGallery = dlg.gallery();
    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("position shape accepted mode=%1 row=%2 output=%3 selection=%4 points=%5 closed=%6")
                    .arg(int(mode)).arg(row).arg(outputIdx).arg(selectionIdx)
                    .arg(candidate.positionPath2D.size())
                    .arg(candidate.positionPath2DClosed ? 1 : 0));

    if (outputIdx >= 0)
    {
        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        while (overrides.size() <= row)
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
        if (selectionIdx > 0)
        {
            const int sel = selectionIdx - 1;
            while (layer.selections.size() <= sel)
            {
                PTTransitionSelection selection;
                selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                layer.selections.append(selection);
            }
            PTTransitionPresetOverride ov = layer.selections[sel].overrides;
            ov.values = candidate;
            ov.columns.insert(ColPositionMotion);
            layer.selections[sel].overrides = ov;
        }
        else
        {
            PTTransitionPresetOverride ov = layer.all;
            ov.values = candidate;
            ov.columns.insert(ColPositionMotion);
            layer.all = ov;
        }
        overrides[row].insert(outputIdx, layer);
    }
    else
    {
        presets[row] = candidate;
    }

    QTreeWidget* table = tableForMode(mode);
    if (table)
    {
        QSignalBlocker blocker(table);
        QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
        if (item)
        {
            setPresetCellValue(item, ColPositionMotion, candidate.positionMotion,
                               item->data(ColPositionMotion, kPresetCellInheritedRole).toBool());
        }
    }
    finishPositionShapeEdit(mode, row, outputIdx, selectionIdx);
    return true;
}

void PresetTableV2TransitionWidget::slotBankTabChanged(int)
{
    if (m_rebuildingTable)
        return;

    auto syncBank = [this](PTTransitionMode mode, QTreeWidget* table) {
        if (!table)
            return;
        for (int r = 0; r < table->topLevelItemCount(); ++r)
            syncPresetFromTable(mode, r);
    };

    syncBank(PTTransitionMode::SweepOnly, m_sweepTable);
    syncBank(PTTransitionMode::Continuous, m_continuousTable);
    syncBank(PTTransitionMode::PositionMotion, m_positionMotionTable);
    syncBank(PTTransitionMode::Channel1D, m_channel1DTable);
    syncBank(PTTransitionMode::MultiFx, m_multiFxTable);

    notifyTablePresetCacheRefresh();
    refreshColumnGroupBarForActiveTab();
    // Defer preview refresh so tab switch finishes before linked-table queries.
    QTimer::singleShot(0, this, [this]() {
        if (m_rebuildingTable)
            return;
        updateEffectPreview();
    });
}

void PresetTableV2TransitionWidget::mapColumnInput(quint8 inputId, const QString& title)
{
    PresetTableV2TransitionColumnDialog dlg(m_doc, title, inputSource(inputId), page(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    setInputSource(dlg.inputSource(), inputId);
    rebuildAllPresetTables();
    notifyTablePresetCacheRefresh();
    m_doc->setModified();
}

void PresetTableV2TransitionWidget::slotColumnHeaderDoubleClicked(int logicalIndex)
{
    if (mode() != Doc::Design)
        return;
    if (!PTEfxCol::hasExternalInput(logicalIndex))
        return;
    QTreeWidget* table = nullptr;
    QObject* src = sender();
    for (QTreeWidget* candidate : { m_sweepTable, m_continuousTable,
                                    m_positionMotionTable, m_channel1DTable,
                                    m_multiFxTable })
    {
        if (candidate && candidate->header() == src)
        {
            table = candidate;
            break;
        }
    }
    const PTTransitionMode bankMode = table ? modeForTable(table) : activeBankMode();
    if (bankIsSlaved(bankMode))
        return;
    if (!isPresetColumnAllowedForMode(bankMode, logicalIndex))
        return;

    const quint8 inputId = PTEfxCol::inputIdForColumn(logicalIndex);
    mapColumnInput(inputId, columnTitleForCol(logicalIndex));
}

bool PresetTableV2TransitionWidget::hasLiveColumnOverride(quint8 inputId) const
{
    return providerSnapshotCopy().liveColumnOverrides.contains(inputId);
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
    const bool preSpreadModeLayout = !preBlocksLayout
            && !m_inputs.contains(PTEfxCol::InputOffsetStepMode);

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
    static const quint8 preSpreadModeMap[28] = {
        0,
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
        PTEfxCol::InputSpeedMult,
        PTEfxCol::InputPositionMotion,
        PTEfxCol::InputPositionMotionDir,
        PTEfxCol::InputPosition1DBuiltinMode,
        PTEfxCol::InputPositionPanSize,
        PTEfxCol::InputPositionTiltSize,
        PTEfxCol::InputChannel1DTarget,
        PTEfxCol::InputChannel1DTargetMode,
        PTEfxCol::InputChannel1DApplyMode,
        PTEfxCol::InputChannel1DLow,
        PTEfxCol::InputChannel1DHigh,
        PTEfxCol::InputChannel1DAmount,
        PTEfxCol::InputChannel1DCustomColumn
    };

    const QList<quint8> keys = m_inputs.keys();
    for (quint8 key : keys)
    {
        if (PTEfxCol::isStableInputId(key))
            continue;

        quint8 stable = 0;
        if (preBlocksLayout && key >= 1 && key <= 12)
            stable = preBlocksMap[key];
        else if (preSpreadModeLayout && key >= 1 && key <= 20)
            stable = preSpreadModeMap[key];
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
        case PTEfxCol::InputGlobalPositionSize:
            m_globalSettings.positionSize = value;
            updateGlobalSummaryLabel();
            return true;
        case PTEfxCol::InputCrossfadeManual:
            m_crossfadeManualInputMapped = true;
            m_crossfadeManualControl = (value > 127);
            publishProviderSnapshot(QStringLiteral("crossfade input"));
            updateGlobalSummaryLabel();
            notifyTablePresetCacheRefresh();
            return true;
        default:
            return false;
    }
}

bool PresetTableV2TransitionWidget::crossfadeManualControlEnabled() const
{
    return providerSnapshotCopy().crossfadeManualControl;
}

PTGlobalEffectSettings PresetTableV2TransitionWidget::globalEffectSettings() const
{
    return providerSnapshotCopy().globalSettings;
}

::PTTransitionProviderSnapshot PresetTableV2TransitionWidget::transitionProviderSnapshot() const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    auto sharedOverride = [](const PTTransitionPresetOverride& ov) {
        PTTransitionProviderPresetOverride out;
        out.values = ov.values;
        out.columns = ov.columns;
        return out;
    };
    auto sharedSelection = [&](const PTTransitionSelection& sel) {
        PTTransitionProviderSelection out;
        out.name = sel.name;
        out.cells = sel.cells;
        out.overrides = sharedOverride(sel.overrides);
        return out;
    };
    auto sharedLayer = [&](const PTTransitionOutputLayer& layer) {
        PTTransitionProviderOutputLayer out;
        out.all = sharedOverride(layer.all);
        out.selections.reserve(layer.selections.size());
        for (const auto& sel : layer.selections)
            out.selections.append(sharedSelection(sel));
        return out;
    };
    auto sharedOverrides = [&](const QVector<QHash<int, PTTransitionOutputLayer>>& src) {
        QVector<QHash<int, PTTransitionProviderOutputLayer>> out;
        out.reserve(src.size());
        for (const auto& row : src)
        {
            QHash<int, PTTransitionProviderOutputLayer> dstRow;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it)
                dstRow.insert(it.key(), sharedLayer(it.value()));
            out.append(dstRow);
        }
        return out;
    };

    ::PTTransitionProviderSnapshot out;
    out.sweepPresets = snapshot.sweepPresets;
    out.continuousPresets = snapshot.continuousPresets;
    out.multiFxPresets = snapshot.multiFxPresets;
    out.positionMotionPresets = snapshot.positionMotionPresets;
    out.channel1DPresets = snapshot.channel1DPresets;
    out.sweepOutputOverrides = sharedOverrides(snapshot.sweepOutputOverrides);
    out.continuousOutputOverrides = sharedOverrides(snapshot.continuousOutputOverrides);
    out.multiFxOutputOverrides = sharedOverrides(snapshot.multiFxOutputOverrides);
    out.positionMotionOutputOverrides = sharedOverrides(snapshot.positionMotionOutputOverrides);
    out.channel1DOutputOverrides = sharedOverrides(snapshot.channel1DOutputOverrides);
    out.multiFxTargetRoutes = snapshot.multiFxTargetRoutes;
    out.liveColumnOverrides = snapshot.liveColumnOverrides;
    out.globalSettings = snapshot.globalSettings;
    out.activeMode = snapshot.activeMode;
    out.enabled = snapshot.enabled;
    out.crossfadeManualControl = snapshot.crossfadeManualControl;
    out.spanX = snapshot.spanX;
    out.spanY = snapshot.spanY;
    out.spanXY = snapshot.spanXY;
    out.revision = snapshot.revision;
    return out;
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
    QTreeWidget* table = item ? item->treeWidget() : nullptr;
    if (!table || !item)
        return;

    ScopedBoolFlag commitGuard(m_committingPresetCell);
    QSignalBlocker blocker(table);
    const PTTransitionPreset preset = presetFromItem(mode, item);
    if (mode == PTTransitionMode::SweepOnly)
    {
        setPresetCellValue(item, ColOffsetStepMode, int(PTOffsetStepMode::AutoFit), true);
        setPresetCellValue(item, ColOffsetStep, 100, true);
        item->setToolTip(ColOffsetStepMode,
                         tr("Transition spread is locked to Auto Fit 100%."));
        item->setToolTip(ColOffsetStep,
                         tr("Transition spread is locked to Auto Fit 100%."));
        return;
    }

    int maxStep = 360;
    int slotCount = 0;
    const int span = gridSpanForPreset(preset);
    if (span > 0)
    {
        slotCount = PTDimmerWaveEngine::effectiveOffsetSlotCount(span, preset);
        maxStep = PTDimmerWaveEngine::maxOffsetStepForGrid(span, preset);
    }

    const int current = item->data(ColOffsetStep, kPresetCellValueRole).toInt();
    const PTOffsetStepMode stepMode = PTOffsetStepMode(
            item->data(ColOffsetStepMode, kPresetCellValueRole).toInt());
    const int maxAmount = stepMode == PTOffsetStepMode::CoveragePercent ? 100 : maxStep;
    if (current > maxAmount)
        setPresetCellValue(item, ColOffsetStep, maxAmount,
                           item->data(ColOffsetStep, kPresetCellInheritedRole).toBool());

    setPresetCellValue(item, ColOffsetStep,
                       qMin(item->data(ColOffsetStep, kPresetCellValueRole).toInt(), maxAmount),
                       item->data(ColOffsetStep, kPresetCellInheritedRole).toBool());

    if (stepMode == PTOffsetStepMode::Off)
    {
        item->setToolTip(ColOffsetStep, tr("Off: all fixtures share the same phase"));
    }
    else if (stepMode == PTOffsetStepMode::AutoFit)
    {
        item->setToolTip(ColOffsetStep,
                         tr("Auto Fit: full even spread for the current fixture span (%1° effective)")
                                 .arg(PTDimmerWaveEngine::effectiveOffsetStepForSpan(span, preset)));
    }
    else if (stepMode == PTOffsetStepMode::CoveragePercent)
    {
        item->setToolTip(ColOffsetStep,
                         tr("Coverage percent of Auto Fit (max %1° for %2 slots)")
                                 .arg(maxStep)
                                 .arg(slotCount));
    }
    else if (slotCount > 0)
    {
        item->setToolTip(ColOffsetStep,
                         tr("Fixed degrees (max %1° for %2 slots: wings×block size per wing). 0 = sync.")
                                 .arg(maxStep)
                                 .arg(slotCount));
    }
    else
    {
        item->setToolTip(ColOffsetStep,
                         tr("Offset step (link Fixture Group table for max limit). 0 = sync."));
    }
}

void PresetTableV2TransitionWidget::updateEffectPreview()
{
    if (!m_curveWidget)
        return;

    const PTTransitionMode mode = activeBankMode();
    const bool slaved = bankIsSlaved(mode);
    ::PTTransitionProviderSnapshot slaveSnapshot;
    const bool slaveReady = slaved && effectiveProviderSnapshotForBank(mode, &slaveSnapshot);
    const QVector<PTTransitionPreset>& localPresets = presetsForMode(mode);
    auto slavePresetsForMode = [&]() -> const QVector<PTTransitionPreset>& {
        if (mode == PTTransitionMode::MultiFx)
            return slaveSnapshot.multiFxPresets;
        if (mode == PTTransitionMode::PositionMotion)
            return slaveSnapshot.positionMotionPresets;
        if (mode == PTTransitionMode::Channel1D)
            return slaveSnapshot.channel1DPresets;
        if (mode == PTTransitionMode::Continuous)
            return slaveSnapshot.continuousPresets;
        if (mode == PTTransitionMode::SweepOnly)
            return slaveSnapshot.sweepPresets;
        static const QVector<PTTransitionPreset> empty;
        return empty;
    };
    const QVector<PTTransitionPreset>& displayPresets =
            (slaved && slaveReady) ? slavePresetsForMode() : localPresets;
    if (slaved && !slaveReady)
    {
        if (m_spatialGridWidget)
            m_spatialGridWidget->setPlaceholderText(bankSourceDescription(mode));
        if (m_spatialGridCaption)
            m_spatialGridCaption->setText(bankSourceDescription(mode));
        if (m_curveWidget)
            m_curveWidget->setPhaseMarkers({});
        return;
    }
    if (displayPresets.isEmpty())
    {
        if (m_spatialGridWidget)
            m_spatialGridWidget->setPlaceholderText(tr("Add a preset"));
        if (m_curveWidget)
            m_curveWidget->setPhaseMarkers({});
        return;
    }

    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    int row = item ? item->data(0, kItemPresetIndexRole).toInt() : 0;
    if (row < 0)
        row = 0;
    if (row >= displayPresets.size())
        row = displayPresets.size() - 1;

    if (!item)
        item = parentItemForPreset(mode, row);

    const int outputIdx = item ? item->data(0, kItemOutputIndexRole).toInt() : -1;
    const int selectionIdx = item ? item->data(0, kItemSelectionIndexRole).toInt() : -1;
    const QVariant multiFxRouteVar = item ? item->data(0, kItemMultiFxRouteIndexRole) : QVariant();
    const int multiFxRouteIdx = multiFxRouteVar.isValid() ? multiFxRouteVar.toInt() : -1;
    const int multiFxRouteOutputIdx = item && item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
    PresetTableV2ControlIface* const linkedTableIface = linkedTable();
    PresetTableV2ControlIface* previewTableIface = linkedTableIface;
    if (mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0
            && row >= 0 && row < m_multiFxTargetRoutes.size()
            && multiFxRouteIdx < m_multiFxTargetRoutes.at(row).size())
    {
        previewTableIface = PresetTableV2VCLookup::controlIfaceByVcId(
                m_multiFxTargetRoutes.at(row).at(multiFxRouteIdx).tableId);
    }

    const auto effectivePresetForPreview = [&]() -> PTTransitionPreset {
        if (slaved)
        {
            PTTransitionPreset p = displayPresets.at(row);
            p.playbackMode = (mode == PTTransitionMode::Continuous
                              || mode == PTTransitionMode::PositionMotion
                              || mode == PTTransitionMode::Channel1D
                              || mode == PTTransitionMode::MultiFx)
                    ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
            if (mode == PTTransitionMode::SweepOnly)
                PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
            const int span = p.axis == PTTransitionAxis::Y
                    ? slaveSnapshot.spanY
                    : (p.axis == PTTransitionAxis::XY ? slaveSnapshot.spanXY
                                                      : slaveSnapshot.spanX);
            PTDimmerWaveEngine::clampOffsetStep(p, span);
            return p;
        }
        if (mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0)
            return effectiveMultiFxRoutePreset(
                    row, multiFxRouteIdx, multiFxRouteOutputIdx,
                    selectionIdx > 0 ? selectionIdx - 1 : -1);
        if (selectionIdx > 0)
            return effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1);
        if (outputIdx >= 0)
            return effectivePresetForOutputNoLive(mode, row, outputIdx);
        return localPresets.at(row);
    };

    const PTTransitionPreset storedPreset = effectivePresetForPreview();
    PTTransitionPreset preset = slaved ? storedPreset : presetFromItem(mode, item);
    if (storedPreset.customCurveEnabled && storedPreset.customCurve.size() >= 2)
    {
        preset.customCurveEnabled = true;
        preset.customCurve = storedPreset.customCurve;
        preset.waveShape = storedPreset.waveShape;
    }

    const PTGlobalEffectSettings previewGlobal =
            (slaved && slaveReady) ? slaveSnapshot.globalSettings : m_globalSettings;
    const PTDimmerWaveParams params = PTDimmerWaveEngine::paramsFromPreset(preset, &previewGlobal);
    const quint32 cycleMs = PTParamMatrixEngine::effectiveDurationMs(previewGlobal, preset, false);
    const bool positionMode = previewTableIface && previewTableIface->tableUsesPositionMode();
    const PTTransitionMode previewLayerMode =
            (mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0)
            ? multiFxContextModeForItem(item)
            : mode;
    const bool transitionTab = (mode == PTTransitionMode::SweepOnly);
    const bool showMotion = positionMode && previewLayerMode == PTTransitionMode::PositionMotion;
    const bool showCurvePreview = !showMotion;
    double crossfadePreviewProgress = 0.0;
    if (transitionTab)
    {
        QObject* linkedObject = dynamic_cast<QObject*>(linkedTableIface);
        if (auto* previewState = linkedObject
                ? qobject_cast<PresetTableV2PreviewStateIface*>(linkedObject) : nullptr)
        {
            crossfadePreviewProgress = previewState->crossfadePreviewProgress01(nullptr);
        }
    }

    if (m_leftPreview)
        m_leftPreview->setVisible(showCurvePreview || showMotion);
    if (QHBoxLayout* previewLayout = qobject_cast<QHBoxLayout*>(m_previewRow->layout()))
    {
        previewLayout->setStretch(0, 2);
        previewLayout->setStretch(1, 3);
    }

    if (m_curveLabel)
    {
        m_curveLabel->setVisible(showCurvePreview);
        if (positionMode && mode == PTTransitionMode::Continuous)
            m_curveLabel->setText(tr("Position interpolation envelope"));
        else if (positionMode && transitionTab)
            m_curveLabel->setText(tr("Position transition envelope"));
        else
            m_curveLabel->setText(tr("Dimmer wave"));
    }
    if (m_curveWidget)
    {
        m_curveWidget->setVisible(showCurvePreview);
        m_curveWidget->setPhaseMarkers({});
        m_curveWidget->setOneShotProgress(crossfadePreviewProgress);
        if (showCurvePreview)
        {
            m_curveWidget->setParams(params);
            m_curveWidget->setEditable(false);
            m_curveWidget->setCycleDurationMs(cycleMs);
        }
    }

    const PTPositionMotion motionForPreview = PTPositionMotion(preset.positionMotion);
    const bool motion1D = PTPositionMotion1DPreviewWidget::is1DMotion(motionForPreview);

    if (m_positionPreviewLabel)
    {
        m_positionPreviewLabel->setVisible(showMotion);
        if (showMotion)
        {
            if (motion1D)
            {
                const bool tiltAxis = motionForPreview == PTPositionMotion::Tilt1D
                        || motionForPreview == PTPositionMotion::CustomTilt1D;
                m_positionPreviewLabel->setText(tiltAxis
                        ? tr("Tilt offset vs time") : tr("Pan offset vs time"));
            }
            else
            {
                m_positionPreviewLabel->setText(tr("Position motion"));
            }
        }
    }
    if (m_positionPreviewModeButton)
    {
        m_positionPreviewModeButton->setVisible(showMotion && !motion1D);
        m_positionPreviewModeButton->setEnabled(showMotion && !motion1D);
    }
    if (m_positionMotionStack)
    {
        m_positionMotionStack->setVisible(showMotion);
        if (!showMotion)
        {
            if (m_positionMotion1DWidget)
                m_positionMotion1DWidget->clear();
            if (m_positionPathWidget)
                m_positionPathWidget->clear();
        }
    }

    if (showMotion && m_positionMotionStack && item)
    {
        QVector<PTPositionMotion1DPreviewWidget::PhaseMarker> markers;
        QList<PTPositionPathPreviewWidget::OrbitBall> balls;

        if (previewTableIface)
        {
            PTSpatialGridPreview preview;
            const bool hasPreview = outputIdx >= 0
                    ? previewTableIface->spatialGridPreviewForOutput(multiFxRouteOutputIdx, preset, previewGlobal, preview)
                    : previewTableIface->spatialGridPreview(preset, previewGlobal, preview);
            if (hasPreview)
            {
                QList<QLCPoint> scopePoints;
                if (outputIdx < 0)
                {
                    scopePoints = previewTableIface->fixtureGroupPoints();
                }
                else if (mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0
                         && selectionIdx > 0)
                {
                    if (row >= 0 && row < m_multiFxTargetRoutes.size()
                            && multiFxRouteIdx < m_multiFxTargetRoutes.at(row).size()
                            && multiFxRouteOutputIdx >= 0)
                    {
                        const PTTransitionProviderOutputLayer layer =
                                m_multiFxTargetRoutes.at(row).at(multiFxRouteIdx)
                                .outputOverrides.value(multiFxRouteOutputIdx);
                        const int sel = selectionIdx - 1;
                        if (sel >= 0 && sel < layer.selections.size())
                        {
                            for (const QLCPoint& pt : layer.selections.at(sel).cells)
                                scopePoints.append(pt);
                        }
                    }
                }
                else if (selectionIdx > 0)
                {
                    const auto& overrides = overridesForMode(mode);
                    if (row >= 0 && row < overrides.size()
                            && overrides.at(row).contains(outputIdx))
                    {
                        const PTTransitionOutputLayer layer = overrides.at(row).value(outputIdx);
                        const int sel = selectionIdx - 1;
                        if (sel >= 0 && sel < layer.selections.size())
                        {
                            for (const QLCPoint& pt : layer.selections.at(sel).cells)
                                scopePoints.append(pt);
                        }
                    }
                }
                else
                {
                    scopePoints = previewTableIface->outputPointsForPresetOverride(multiFxRouteOutputIdx);
                }

                int colorIdx = 0;
                for (const QLCPoint& pt : scopePoints)
                {
                    if (!preview.cells.contains(pt))
                        continue;
                    PTPositionPathPreviewWidget::OrbitBall ball;
                    ball.phaseOffset01 = qreal(preview.cells.value(pt).headOffsetDeg) / 360.0;
                    ball.color = selectionLayerColor(colorIdx++);
                    balls.append(ball);
                }

                markers.reserve(balls.size());
                for (const PTPositionPathPreviewWidget::OrbitBall& ball : balls)
                {
                    PTPositionMotion1DPreviewWidget::PhaseMarker marker;
                    marker.phaseOffset01 = ball.phaseOffset01;
                    marker.color = ball.color;
                    markers.append(marker);
                }
            }
        }

        const PTPositionMotion motion = PTPositionMotion(preset.positionMotion);
        if (motion != PTPositionMotion::Off)
        {
            const bool showPanTiltGraph = !motion1D && m_positionPreviewModeButton
                    && m_positionPreviewModeButton->isChecked();
            if (motion1D && m_positionMotion1DWidget)
            {
                m_positionMotionStack->setCurrentWidget(m_positionMotion1DWidget);
                PTDimmerWaveParams waveParams =
                        PTDimmerWaveEngine::paramsFromPreset(preset, &previewGlobal);
                m_positionMotion1DWidget->setMotionPreviewFromPreset(
                        preset, waveParams, markers, cycleMs);
            }
            else if (showPanTiltGraph && m_positionMotion1DWidget)
            {
                m_positionMotionStack->setCurrentWidget(m_positionMotion1DWidget);
                m_positionMotion1DWidget->setMotionPreview2DFromPreset(
                        preset, markers, cycleMs);
            }
            else if (m_positionPathWidget)
            {
                m_positionMotionStack->setCurrentWidget(m_positionPathWidget);
                m_positionPathWidget->setOrbitPreviewFromPreset(preset, balls, cycleMs);
            }
        }
        else
        {
            if (m_positionMotion1DWidget)
                m_positionMotion1DWidget->clear();
            if (m_positionPathWidget)
                m_positionPathWidget->clear();
        }
    }

    if (!m_spatialGridWidget)
        return;

    if (previewTableIface)
    {
        PTSpatialGridPreview preview;
        const bool hasPreview = outputIdx >= 0
                ? previewTableIface->spatialGridPreviewForOutput(multiFxRouteOutputIdx, preset, previewGlobal, preview)
                : previewTableIface->spatialGridPreview(preset, previewGlobal, preview);
        if (hasPreview)
        {
            m_spatialGridWidget->setPreview(preview);
            if (m_curveWidget && showCurvePreview)
            {
                QList<QLCPoint> markerPoints;
                for (auto it = preview.cells.constBegin(); it != preview.cells.constEnd(); ++it)
                {
                    if (it.value().occupied)
                        markerPoints.append(it.key());
                }
                std::sort(markerPoints.begin(), markerPoints.end(),
                          [&preview](const QLCPoint& a, const QLCPoint& b) {
                              const int ao = preview.cells.value(a).chaseOrder;
                              const int bo = preview.cells.value(b).chaseOrder;
                              if (ao != bo)
                                  return ao < bo;
                              if (a.y() != b.y())
                                  return a.y() < b.y();
                              return a.x() < b.x();
                          });

                QVector<PTDimmerWaveCurveWidget::PhaseMarker> phaseMarkers;
                phaseMarkers.reserve(markerPoints.size());
                int colorIdx = 0;
                for (const QLCPoint& pt : markerPoints)
                {
                    const PTSpatialGridCellData cell = preview.cells.value(pt);
                    PTDimmerWaveCurveWidget::PhaseMarker marker;
                    marker.phaseOffset01 = qBound<qreal>(0.0, cell.phaseStart01, 1.0);
                    marker.color = selectionLayerColor(colorIdx++);
                    phaseMarkers.append(marker);
                }
                m_curveWidget->setPhaseMarkers(
                        phaseMarkers,
                        transitionTab ? PTDimmerWaveCurveWidget::MarkerMode::OneShot
                                      : PTDimmerWaveCurveWidget::MarkerMode::Cyclic);
                if (transitionTab)
                    m_curveWidget->setOneShotProgress(crossfadePreviewProgress);
            }
            QSet<QLCPoint> scopeCells;
            QVector<PTSpatialGridSelectionLayer> layers;

            auto addSelectionLayersForOutput = [&](int layerOutputIdx) {
                const QList<QLCPoint> scope =
                        previewTableIface->outputPointsForPresetOverride(layerOutputIdx);
                QSet<QLCPoint> outputScope;
                for (const QLCPoint& pt : scope)
                {
                    outputScope.insert(pt);
                    scopeCells.insert(pt);
                }

                if (mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0)
                {
                    if (row < 0 || row >= m_multiFxTargetRoutes.size()
                            || multiFxRouteIdx >= m_multiFxTargetRoutes.at(row).size())
                        return;
                    const PTMultiFxTargetTableRoute& route =
                            m_multiFxTargetRoutes.at(row).at(multiFxRouteIdx);
                    const PTTransitionProviderOutputLayer layer =
                            route.outputOverrides.value(layerOutputIdx);
                    QSet<QLCPoint> occupied;
                    for (int s = 0; s < layer.selections.size(); ++s)
                    {
                        PTSpatialGridSelectionLayer gridLayer;
                        gridLayer.selectionIndex = s;
                        gridLayer.name = layer.selections.at(s).name.isEmpty()
                                ? tr("Selection %1").arg(s + 1)
                                : layer.selections.at(s).name;
                        if (outputIdx < 0)
                            gridLayer.name = targetTableOutputName(route.tableId, layerOutputIdx)
                                    + QStringLiteral(" / ") + gridLayer.name;
                        gridLayer.color = selectionLayerColor(s);

                        for (const QLCPoint& pt : layer.selections.at(s).cells)
                        {
                            if (!outputScope.contains(pt) || occupied.contains(pt))
                                continue;
                            gridLayer.cells.insert(pt);
                            occupied.insert(pt);
                        }

                        if (!gridLayer.cells.isEmpty() || layerOutputIdx == multiFxRouteOutputIdx)
                            layers.append(gridLayer);
                    }
                    return;
                }

                const auto& overrides = overridesForMode(mode);
                if (row < 0 || row >= overrides.size() || !overrides.at(row).contains(layerOutputIdx))
                    return;

                const PTTransitionOutputLayer layer = overrides.at(row).value(layerOutputIdx);
                QSet<QLCPoint> occupied;
                for (int s = 0; s < layer.selections.size(); ++s)
                {
                    PTSpatialGridSelectionLayer gridLayer;
                    gridLayer.selectionIndex = s;
                    gridLayer.name = layer.selections.at(s).name.isEmpty()
                            ? tr("Selection %1").arg(s + 1) : layer.selections.at(s).name;
                    if (outputIdx < 0)
                        gridLayer.name = linkedOutputName(layerOutputIdx) + QStringLiteral(" / ")
                                + gridLayer.name;
                    gridLayer.color = selectionLayerColor(s);

                    for (const QLCPoint& pt : layer.selections.at(s).cells)
                    {
                        if (!outputScope.contains(pt) || occupied.contains(pt))
                            continue;
                        gridLayer.cells.insert(pt);
                        occupied.insert(pt);
                    }

                    if (!gridLayer.cells.isEmpty() || layerOutputIdx == outputIdx)
                        layers.append(gridLayer);
                }
            };

            if (outputIdx >= 0)
            {
                addSelectionLayersForOutput(multiFxRouteOutputIdx);
            }
            else
            {
                const int count = mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0
                        && row >= 0 && row < m_multiFxTargetRoutes.size()
                        && multiFxRouteIdx < m_multiFxTargetRoutes.at(row).size()
                        ? targetTableOutputCount(m_multiFxTargetRoutes.at(row).at(multiFxRouteIdx).tableId)
                        : linkedOutputCount();
                for (int o = 0; o < count; ++o)
                    addSelectionLayersForOutput(o);
            }
            m_spatialGridWidget->setSelectionLayers(
                    outputIdx >= 0 && selectionIdx > 0,
                    selectionIdx > 0 ? selectionIdx - 1 : -1,
                    scopeCells, layers);
            if (outputIdx >= 0 && selectionIdx > 0)
                m_spatialGridWidget->setToolTip(
                        tr("Click Fixture Grid cells to edit this selection. "
                           "Colored inner borders show other custom selections."));
            else
                m_spatialGridWidget->setToolTip(
                        tr("Colored inner borders show custom selections for this preset."));

            if (m_spatialGridCaption)
            {
                if (positionMode)
                {
                    if (selectionIdx > 0)
                    {
                        m_spatialGridCaption->setText(
                                tr("Spread order (offset°) · click cells to edit this Selection"));
                    }
                    else
                    {
                        m_spatialGridCaption->setText(
                                tr("Spread order (offset°) · colors = custom selections"));
                    }
                }
                else
                {
                    m_spatialGridCaption->setText(
                            tr("Chase order and head offset° · orange/red = offset warnings"));
                }
            }

            return;
        }
    }
    m_spatialGridWidget->setPlaceholderText(
            tr("Link Preset Table v2 in Fixture Group mode to preview spatial order"));
    if (m_spatialGridCaption)
        m_spatialGridCaption->clear();
}

void PresetTableV2TransitionWidget::slotAddPreset()
{
    PTTransitionMode mode = activeBankMode();
    if (bankIsSlaved(mode))
        return;
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    presets.append(defaultPreset(presets.size(), mode));
    overrides.append(QHash<int, PTTransitionOutputLayer>());
    if (mode == PTTransitionMode::MultiFx)
        m_multiFxTargetRoutes.append(QVector<PTMultiFxTargetTableRoute>());
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::slotAddSelection()
{
    const PTTransitionMode mode = activeBankMode();
    if (bankIsSlaved(mode))
        return;
    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    if (!item)
        return;
    const int row = item->data(0, kItemPresetIndexRole).toInt();
    int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    if (outputIdx < 0)
        return;

    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        normalizeMultiFxTargetRoutes();
        if (row < 0 || row >= m_multiFxTargetRoutes.size()
                || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size()
                || routeOutputIdx < 0)
            return;

        PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
        PTTransitionProviderOutputLayer layer =
                route.outputOverrides.value(routeOutputIdx);
        PTTransitionProviderSelection selection;
        selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
        layer.selections.append(selection);
        route.outputOverrides.insert(routeOutputIdx, layer);
        const int newSelectionIdx = layer.selections.size() - 1;
        rebuildPresetTable(mode);
        if (QTreeWidgetItem* outputItem =
                itemForMultiFxRouteAddress(row, routeIdx, routeOutputIdx, 0))
        {
            outputItem->setExpanded(true);
            if (QTreeWidgetItem* selItem =
                    itemForMultiFxRouteAddress(row, routeIdx, routeOutputIdx,
                                               newSelectionIdx + 1))
                table->setCurrentItem(selItem, ColName);
        }
        publishProviderSnapshot(QStringLiteral("multifx route selection add"));
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }

    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    while (overrides.size() <= row)
        overrides.append(QHash<int, PTTransitionOutputLayer>());
    PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    PTTransitionSelection selection;
    selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
    layer.selections.append(selection);
    overrides[row].insert(outputIdx, layer);
    const int newSelectionIdx = layer.selections.size() - 1;
    rebuildPresetTable(mode);
    if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
    {
        parent->setExpanded(true);
        if (QTreeWidgetItem* outputItem = itemForPresetAddress(mode, row, outputIdx, 0))
        {
            outputItem->setExpanded(true);
            if (QTreeWidgetItem* selItem =
                    itemForPresetAddress(mode, row, outputIdx, newSelectionIdx + 1))
            {
                QTreeWidget* table = tableForMode(mode);
                if (table)
                    table->setCurrentItem(selItem, ColName);
            }
        }
    }
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
}

void PresetTableV2TransitionWidget::slotRemovePreset()
{
    const PTTransitionMode mode = activeBankMode();
    if (bankIsSlaved(mode))
        return;
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    QTreeWidget* table = tableForMode(mode);
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    if (!item)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
        if (selectionIdx > 0 && row >= 0 && row < m_multiFxTargetRoutes.size()
                && routeIdx >= 0 && routeIdx < m_multiFxTargetRoutes.at(row).size()
                && routeOutputIdx >= 0)
        {
            PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
            if (!route.outputOverrides.contains(routeOutputIdx))
                return;
            PTTransitionProviderOutputLayer layer =
                    route.outputOverrides.value(routeOutputIdx);
            const int sel = selectionIdx - 1;
            if (sel < 0 || sel >= layer.selections.size())
                return;
            layer.selections.removeAt(sel);
            if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
                route.outputOverrides.remove(routeOutputIdx);
            else
                route.outputOverrides.insert(routeOutputIdx, layer);
            publishProviderSnapshot(QStringLiteral("multifx route selection remove"));
        }
        else if (routeOutputIdx < 0 && selectionIdx <= 0 && routeIdx > 0)
        {
            removeMultiFxTargetTableRoute(row, routeIdx);
        }
        else
        {
            return;
        }
        rebuildPresetTable(mode);
        notifyTablePresetCacheRefresh();
        updateRemoveActionLabel();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }

    if (selectionIdx > 0)
    {
        removeSelectionAt(mode, row, outputIdx, selectionIdx - 1);
        return;
    }
    if (outputIdx >= 0)
        return;

    if (row < 0 || presets.size() <= 1)
        return;
    presets.removeAt(row);
    if (row < overrides.size())
        overrides.removeAt(row);
    if (mode == PTTransitionMode::MultiFx && row < m_multiFxTargetRoutes.size())
        m_multiFxTargetRoutes.removeAt(row);
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateRemoveActionLabel();
}

void PresetTableV2TransitionWidget::slotDuplicatePreset()
{
    const PTTransitionMode mode = activeBankMode();
    if (bankIsSlaved(mode))
        return;
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    QTreeWidget* table = tableForMode(mode);
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    while (item && item->parent())
        item = item->parent();
    const int row = item ? item->data(0, kItemPresetIndexRole).toInt() : -1;
    if (row < 0)
        return;
    syncPresetFromTable(mode, row);
    QVector<PTMultiFxTargetTableRoute> multiFxRouteCopy;
    if (mode == PTTransitionMode::MultiFx)
    {
        normalizeMultiFxTargetRoutes();
        QVector<PTTransitionPreset> displayPresets;
        QVector<QVector<PTMultiFxTargetTableRoute>> displayRoutes;
        displayDataForMode(mode, displayPresets, displayRoutes);
        if (row < displayRoutes.size())
            multiFxRouteCopy = displayRoutes.at(row);
    }
    PTTransitionPreset copy = presets[row];
    copy.name += tr(" copy");
    presets.append(copy);
    overrides.append(row < overrides.size()
            ? overrides.at(row) : QHash<int, PTTransitionOutputLayer>());
    if (mode == PTTransitionMode::MultiFx)
    {
        normalizeMultiFxTargetRoutes();
        if (m_multiFxTargetRoutes.size() < presets.size())
            m_multiFxTargetRoutes.append(QVector<PTMultiFxTargetTableRoute>());
        m_multiFxTargetRoutes[presets.size() - 1] = multiFxRouteCopy;
    }
    publishProviderSnapshot(QStringLiteral("duplicate preset"));
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    if (m_doc)
        m_doc->setModified();
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

QString PresetTableV2TransitionWidget::targetTableName(quint32 tableId) const
{
    if (tableId == VCWidget::invalidId())
        return tr("Missing table");
    for (VCWidget* widget : PresetTableV2VCLookup::allVcWidgets())
    {
        if (!widget || widget->id() != tableId)
            continue;
        if (qobject_cast<PresetTableV2ControlIface*>(widget))
            return PresetTableV2VCLookup::vcWidgetLabel(widget);
    }
    return tr("Table #%1 not found").arg(tableId);
}

int PresetTableV2TransitionWidget::targetTableOutputCount(quint32 tableId) const
{
    if (PresetTableV2ControlIface* table = PresetTableV2VCLookup::controlIfaceByVcId(tableId))
        return table->outputCountForPresetOverrides();
    return 0;
}

QString PresetTableV2TransitionWidget::targetTableOutputName(quint32 tableId,
                                                             int outputIdx) const
{
    if (PresetTableV2ControlIface* table = PresetTableV2VCLookup::controlIfaceByVcId(tableId))
    {
        const QString name = table->outputNameForPresetOverride(outputIdx);
        if (!name.isEmpty())
            return name;
    }
    return tr("Output %1").arg(outputIdx + 1);
}

bool PresetTableV2TransitionWidget::targetTableUsesPositionMode(quint32 tableId) const
{
    if (PresetTableV2ControlIface* table = PresetTableV2VCLookup::controlIfaceByVcId(tableId))
        return table->tableUsesPositionMode();
    return false;
}

PTTransitionMode PresetTableV2TransitionWidget::multiFxRouteMode(
        const PTMultiFxTargetTableRoute& route) const
{
    const PTMultiFxTargetLayerKind kind = PTMultiFxTargetLayerKind(route.layerKind);
    if (kind == PTMultiFxTargetLayerKind::Interpolation)
        return PTTransitionMode::Continuous;
    if (kind == PTMultiFxTargetLayerKind::PositionMotion)
        return PTTransitionMode::PositionMotion;
    if (kind == PTMultiFxTargetLayerKind::Channel1D)
        return PTTransitionMode::Channel1D;
    return targetTableUsesPositionMode(route.tableId)
            ? PTTransitionMode::PositionMotion : PTTransitionMode::Channel1D;
}

QString PresetTableV2TransitionWidget::multiFxRouteModeLabel(
        const PTMultiFxTargetTableRoute& route) const
{
    const PTTransitionMode routeMode = multiFxRouteMode(route);
    if (routeMode == PTTransitionMode::Continuous)
        return tr("Interpolation");
    return routeMode == PTTransitionMode::PositionMotion ? tr("2D FX") : tr("1D FX");
}

PresetTableV2TransitionWidget::MultiFxRouteCellContext
PresetTableV2TransitionWidget::multiFxRouteCellContext(
        int row, int routeIndex, int outputIdx, int selectionIdx,
        const QVector<PTTransitionPreset>& displayPresets,
        const QVector<QVector<PTMultiFxTargetTableRoute>>& displayRoutes) const
{
    MultiFxRouteCellContext context;
    if (row < 0 || row >= displayPresets.size()
            || row >= displayRoutes.size()
            || routeIndex < 0 || routeIndex >= displayRoutes.at(row).size())
        return context;

    const PTMultiFxTargetTableRoute& route = displayRoutes.at(row).at(routeIndex);
    context.routeMode = multiFxRouteMode(route);
    context.allowedColumns = allowedMultiFxColumnsForContext(context.routeMode, false);

    PTTransitionPreset p = displayPresets.at(row);
    p.playbackMode = PTTransitionMode::Continuous;
    applyOverrideColumnsToPreset(p, route.tableOverride);
    context.overrideColumns = route.tableOverride.columns;

    if (outputIdx >= 0)
    {
        context.overrideColumns.clear();
        if (route.outputOverrides.contains(outputIdx))
        {
            const PTTransitionProviderOutputLayer layer =
                    route.outputOverrides.value(outputIdx);
            applyOverrideColumnsToPreset(p, layer.all);
            context.overrideColumns = layer.all.columns;

            if (selectionIdx >= 0)
            {
                context.overrideColumns.clear();
                if (selectionIdx < layer.selections.size())
                {
                    applyOverrideColumnsToPreset(p, layer.selections.at(selectionIdx).overrides);
                    context.overrideColumns = layer.selections.at(selectionIdx).overrides.columns;
                }
            }
        }
    }

    context.effective = p;
    context.valid = true;
    return context;
}

PresetTableV2TransitionWidget::MultiFxRouteCellContext
PresetTableV2TransitionWidget::multiFxRouteCellContextForItem(QTreeWidgetItem* item) const
{
    if (!item || !item->data(0, kItemMultiFxRouteIndexRole).isValid())
        return MultiFxRouteCellContext();

    QVector<PTTransitionPreset> displayPresets;
    QVector<QVector<PTMultiFxTargetTableRoute>> displayRoutes;
    displayDataForMode(PTTransitionMode::MultiFx, displayPresets, displayRoutes);

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int routeIndex = item->data(0, kItemMultiFxRouteIndexRole).toInt();
    const int outputIdx =
            item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    return multiFxRouteCellContext(row, routeIndex, outputIdx,
                                   selectionIdx > 0 ? selectionIdx - 1 : -1,
                                   displayPresets, displayRoutes);
}

QVector<int> PresetTableV2TransitionWidget::multiFxOutputIndicesForRoute(
        const PTMultiFxTargetTableRoute& route) const
{
    QSet<int> seen;
    QVector<int> outputs;
    auto addOutput = [&](int outputIdx) {
        if (outputIdx < 0 || seen.contains(outputIdx))
            return;
        seen.insert(outputIdx);
        outputs.append(outputIdx);
    };

    const int liveOutputCount = targetTableOutputCount(route.tableId);
    if (liveOutputCount > 0)
    {
        for (int outputIdx = 0; outputIdx < liveOutputCount; ++outputIdx)
            addOutput(outputIdx);
    }
    else
    {
        for (const PTMultiFxTargetOutputRoute& output : route.outputs)
            addOutput(output.outputIndex);
    }

    for (auto it = route.outputOverrides.constBegin(); it != route.outputOverrides.constEnd(); ++it)
        addOutput(it.key());

    return outputs;
}

PresetTableV2TransitionWidget::MultiFxUiRow
PresetTableV2TransitionWidget::multiFxUiRowForAddress(
        int row, int routeIndex, int outputIdx, int selectionIdx,
        const QVector<PTTransitionPreset>& displayPresets,
        const QVector<QVector<PTMultiFxTargetTableRoute>>& displayRoutes) const
{
    MultiFxUiRow uiRow;
    uiRow.presetRow = row;
    uiRow.routeIndex = routeIndex;
    uiRow.outputIndex = outputIdx;
    uiRow.selectionIndex = selectionIdx;

    const MultiFxRouteCellContext context =
            multiFxRouteCellContext(row, routeIndex, outputIdx, selectionIdx,
                                    displayPresets, displayRoutes);
    if (!context.valid)
        return uiRow;

    const PTMultiFxTargetTableRoute& route = displayRoutes.at(row).at(routeIndex);
    uiRow.tableId = route.tableId;
    uiRow.routeMode = context.routeMode;
    uiRow.effective = context.effective;
    uiRow.allowedColumns = context.allowedColumns;
    uiRow.overrideColumns = context.overrideColumns;
    uiRow.valid = true;

    if (selectionIdx >= 0)
    {
        uiRow.kind = MultiFxUiRowKind::Selection;
        const PTTransitionProviderOutputLayer layer =
                route.outputOverrides.value(outputIdx);
        const QString selectionName =
                selectionIdx >= 0 && selectionIdx < layer.selections.size()
                ? layer.selections.at(selectionIdx).name : QString();
        uiRow.label = selectionName.isEmpty()
                ? tr("Selection %1").arg(selectionIdx + 1) : selectionName;
        const int cellCount = selectionIdx >= 0 && selectionIdx < layer.selections.size()
                ? layer.selections.at(selectionIdx).cells.size() : 0;
        uiRow.tooltip = tr("MultiFX target selection override (%1 cell(s)).")
                .arg(cellCount);
    }
    else if (outputIdx >= 0)
    {
        uiRow.kind = MultiFxUiRowKind::OutputAll;
        uiRow.label = targetTableOutputName(route.tableId, outputIdx) + tr(" / All");
        uiRow.tooltip = tr("MultiFX target output override.");
    }
    else
    {
        uiRow.kind = MultiFxUiRowKind::TargetTable;
        uiRow.label = targetTableName(route.tableId)
                + QStringLiteral(" - ")
                + multiFxRouteModeLabel(route);
        uiRow.tooltip = tr("MultiFX target table. This level can override the shared preset for this table only.");
    }

    return uiRow;
}

void PresetTableV2TransitionWidget::renderPresetRowCells(
        QTreeWidgetItem* item, const MultiFxUiRow& row,
        bool cellSelection)
{
    QTreeWidget* table = item ? item->treeWidget() : nullptr;
    if (!item || !row.valid)
        return;

    ScopedBoolFlag renderGuard(m_committingPresetCell);
    QScopedPointer<QSignalBlocker> tableBlocker;
    if (table)
        tableBlocker.reset(new QSignalBlocker(table));

    auto setUnsupportedCell = [&](int col) {
        Q_UNUSED(cellSelection)
        item->setData(col, kPresetCellValueRole, QVariant());
        item->setData(col, kPresetCellInheritedRole, true);
        item->setText(col, QString());
        QFont f = item->font(col);
        f.setItalic(true);
        f.setBold(false);
        item->setFont(col, f);
        QColor color = palette().color(QPalette::Text);
        color.setAlpha(85);
        item->setForeground(col, QBrush(color));
        item->setBackground(col, QBrush());
        item->setToolTip(col, tr("Not used by this MultiFX target"));
    };

    bool renderedCommonValue = false;
    for (int col = ColAxis; col < ColCount; ++col)
    {
        if (!row.allowedColumns.contains(col))
        {
            setUnsupportedCell(col);
            continue;
        }

        if (col == ColMultiFxTargetMode)
        {
            const int modeValue = row.routeMode == PTTransitionMode::Continuous ? 1 : 0;
            setPresetCellValue(item, col, modeValue,
                               row.kind != MultiFxUiRowKind::TargetTable,
                               false, false);
            item->setToolTip(col, row.routeMode == PTTransitionMode::Continuous
                             ? tr("This target runs MultiFX as Interpolation.")
                             : tr("FX is automatic: Position tables use 2D FX, fixture tables use 1D FX."));
            continue;
        }

        const bool inherited = !row.overrideColumns.contains(col);
        const bool selected = cellSelection && table
                && m_selectedCellsByTable.value(table).contains(
                        PTTransitionCellKey { item, col });
        setPresetCellValue(item, col, presetColumnValue(row.effective, col),
                           inherited, false, selected);
        if (col == ColWaveWidth || col == ColWaveShape || col == ColFadeIn
                || col == ColFadeOut || col == ColStartOffset || col == ColSpeedMult)
            renderedCommonValue = true;

        const QString tip = columnTooltipForCol(col);
        if (!tip.isEmpty())
            item->setToolTip(col, tip);
    }

    if (!renderedCommonValue)
    {
        const int presetRow = item->data(0, kItemPresetIndexRole).toInt();
        const int routeIdx = item->data(0, kItemMultiFxRouteIndexRole).toInt();
        const int outputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
        VCPluginDiagnostics::breadcrumbRateLimited(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("presettablev2transition/multifx-empty-common/%1/%2/%3/%4")
                        .arg(id()).arg(presetRow).arg(routeIdx).arg(outputIdx),
                1000,
                QStringLiteral("multifx common cells not rendered row=%1 route=%2 output=%3 routeMode=%4 waveWidth=%5 waveShape=%6")
                        .arg(presetRow).arg(routeIdx).arg(outputIdx)
                        .arg(int(row.routeMode))
                        .arg(row.effective.waveWidth)
                        .arg(row.effective.waveShape));
    }

    updatePresetRowUiForItem(item, row.routeMode);
    updateOffsetStepLimitForItem(item, row.routeMode);
    for (int col = ColAxis; col < ColCount; ++col)
    {
        if (!row.allowedColumns.contains(col))
            setUnsupportedCell(col);
    }
}

void PresetTableV2TransitionWidget::refreshMultiFxRouteVisualsForPreset(
        int row,
        const QVector<PTTransitionPreset>& displayPresets,
        const QVector<QVector<PTMultiFxTargetTableRoute>>& displayRoutes)
{
    QTreeWidgetItem* parent = parentItemForPreset(PTTransitionMode::MultiFx, row);
    if (!parent)
        return;

    if (row < 0 || row >= displayPresets.size()
            || row >= displayRoutes.size())
        return;

    const QVector<PTMultiFxTargetTableRoute>& routes = displayRoutes.at(row);
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* routeItem = parent->child(i);
        if (!routeItem || !routeItem->data(0, kItemMultiFxRouteIndexRole).isValid())
            continue;

        const int routeIdx = routeItem->data(0, kItemMultiFxRouteIndexRole).toInt();
        if (routeIdx < 0 || routeIdx >= routes.size())
            continue;

        auto paintItem = [&](QTreeWidgetItem* item) {
            const int outputIdx =
                    item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                    ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
            const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
            const MultiFxUiRow uiRow = multiFxUiRowForAddress(
                    row, routeIdx, outputIdx, selectionIdx > 0 ? selectionIdx - 1 : -1,
                    displayPresets, displayRoutes);
            if (!uiRow.valid)
            {
                const PTMultiFxTargetTableRoute& route = routes.at(routeIdx);
                VCPluginDiagnostics::breadcrumbRateLimited(
                        QStringLiteral("presettablev2transition"), id(), caption(),
                        QStringLiteral("presettablev2transition/multifx-invalid-ui-context/%1/%2/%3/%4")
                                .arg(id()).arg(row).arg(routeIdx).arg(outputIdx),
                        1000,
                        QStringLiteral("multifx ui context invalid row=%1 route=%2 output=%3 selection=%4 table=%5 layerKind=%6 presets=%7 routeRows=%8")
                                .arg(row).arg(routeIdx).arg(outputIdx).arg(selectionIdx)
                                .arg(route.tableId).arg(route.layerKind)
                                .arg(displayPresets.size()).arg(displayRoutes.size()));
                return;
            }
            item->setText(ColName, uiRow.label);
            item->setToolTip(ColName, uiRow.tooltip);
            renderPresetRowCells(item, uiRow, true);
            if (selectionIdx > 0)
                applySelectionRowVisuals(item);
        };

        paintItem(routeItem);
        for (int o = 0; o < routeItem->childCount(); ++o)
        {
            QTreeWidgetItem* outputItem = routeItem->child(o);
            paintItem(outputItem);
            for (int s = 0; s < outputItem->childCount(); ++s)
                paintItem(outputItem->child(s));
        }
    }
}

void PresetTableV2TransitionWidget::applyOverrideColumnsToPreset(
        PTTransitionPreset& preset, const PTTransitionProviderPresetOverride& ov) const
{
    for (int col : ov.columns)
        setPresetColumnValue(preset, col, presetColumnValue(ov.values, col));
    if (ov.columns.contains(ColWaveShape))
    {
        preset.customCurveEnabled = ov.values.customCurveEnabled;
        preset.customCurve = ov.values.customCurve;
        preset.waveShape = ov.values.waveShape;
    }
}

void PresetTableV2TransitionWidget::normalizeMultiFxTargetRoutes()
{
    while (m_multiFxTargetRoutes.size() < m_multiFxPresets.size())
        m_multiFxTargetRoutes.append(QVector<PTMultiFxTargetTableRoute>());
    while (m_multiFxTargetRoutes.size() > m_multiFxPresets.size())
        m_multiFxTargetRoutes.removeLast();

    for (QVector<PTMultiFxTargetTableRoute>& routes : m_multiFxTargetRoutes)
    {
        if (m_targetTableId != VCWidget::invalidId())
        {
            int existingCurrent = -1;
            for (int i = 0; i < routes.size(); ++i)
            {
                if (routes.at(i).tableId == m_targetTableId)
                {
                    existingCurrent = i;
                    break;
                }
            }
            if (existingCurrent < 0)
            {
                PTMultiFxTargetTableRoute currentRoute;
                currentRoute.enabled = true;
                currentRoute.tableId = m_targetTableId;
                currentRoute.layerKind = targetTableUsesPositionMode(m_targetTableId)
                        ? int(PTMultiFxTargetLayerKind::PositionMotion)
                        : int(PTMultiFxTargetLayerKind::Channel1D);
                routes.prepend(currentRoute);
            }
            else if (existingCurrent > 0)
            {
                const PTMultiFxTargetTableRoute route = routes.takeAt(existingCurrent);
                routes.prepend(route);
            }
        }

        for (PTMultiFxTargetTableRoute& route : routes)
        {
            if (PTMultiFxTargetLayerKind(route.layerKind) == PTMultiFxTargetLayerKind::Auto
                    && route.tableId != VCWidget::invalidId()
                    && PresetTableV2VCLookup::controlIfaceByVcId(route.tableId))
            {
                route.layerKind = targetTableUsesPositionMode(route.tableId)
                        ? int(PTMultiFxTargetLayerKind::PositionMotion)
                        : int(PTMultiFxTargetLayerKind::Channel1D);
            }

            const int outputCount = targetTableOutputCount(route.tableId);
            if (route.outputs.isEmpty() && outputCount > 0)
            {
                for (int outputIdx = 0; outputIdx < outputCount; ++outputIdx)
                {
                    PTMultiFxTargetOutputRoute output;
                    output.outputIndex = outputIdx;
                    route.outputs.append(output);
                }
            }

            QSet<int> seenOutputs;
            QVector<PTMultiFxTargetOutputRoute> outputs;
            for (const PTMultiFxTargetOutputRoute& output : route.outputs)
            {
                if (output.outputIndex < 0 || seenOutputs.contains(output.outputIndex))
                    continue;
                seenOutputs.insert(output.outputIndex);
                outputs.append(output);
            }
            for (auto it = route.outputOverrides.constBegin();
                    it != route.outputOverrides.constEnd(); ++it)
            {
                if (it.key() < 0 || seenOutputs.contains(it.key()))
                    continue;
                PTMultiFxTargetOutputRoute output;
                output.outputIndex = it.key();
                seenOutputs.insert(output.outputIndex);
                outputs.append(output);
            }
            for (int outputIdx = 0; outputIdx < outputCount; ++outputIdx)
            {
                if (seenOutputs.contains(outputIdx))
                    continue;
                PTMultiFxTargetOutputRoute output;
                output.outputIndex = outputIdx;
                seenOutputs.insert(output.outputIndex);
                outputs.append(output);
            }
            route.outputs = outputs;
        }
    }
}

void PresetTableV2TransitionWidget::displayDataForMode(
        PTTransitionMode mode,
        QVector<PTTransitionPreset>& displayPresets,
        QVector<QVector<PTMultiFxTargetTableRoute>>& displayRoutes) const
{
    displayPresets = presetsForMode(mode);
    displayRoutes = mode == PTTransitionMode::MultiFx
            ? m_multiFxTargetRoutes
            : QVector<QVector<PTMultiFxTargetTableRoute>>();

    if (!bankIsSlaved(mode))
        return;

    ::PTTransitionProviderSnapshot source;
    if (!effectiveProviderSnapshotForBank(mode, &source))
    {
        displayPresets.clear();
        displayRoutes.clear();
        return;
    }

    switch (mode)
    {
        case PTTransitionMode::Off:
            displayPresets.clear();
            displayRoutes.clear();
            break;
        case PTTransitionMode::SweepOnly:
            displayPresets = source.sweepPresets;
            displayRoutes.clear();
            break;
        case PTTransitionMode::Continuous:
            displayPresets = source.continuousPresets;
            displayRoutes.clear();
            break;
        case PTTransitionMode::Channel1D:
            displayPresets = source.channel1DPresets;
            displayRoutes.clear();
            break;
        case PTTransitionMode::PositionMotion:
            displayPresets = source.positionMotionPresets;
            displayRoutes.clear();
            break;
        case PTTransitionMode::MultiFx:
            displayPresets = source.multiFxPresets;
            displayRoutes = source.multiFxTargetRoutes;
            break;
    }
}

void PresetTableV2TransitionWidget::addMultiFxTargetTableRoute(int presetIndex,
                                                               quint32 tableId)
{
    if (presetIndex < 0 || presetIndex >= m_multiFxPresets.size()
            || tableId == VCWidget::invalidId())
        return;

    normalizeMultiFxTargetRoutes();
    QVector<PTMultiFxTargetTableRoute>& routes = m_multiFxTargetRoutes[presetIndex];
    for (const PTMultiFxTargetTableRoute& route : routes)
    {
        if (route.tableId == tableId)
            return;
    }

    PTMultiFxTargetTableRoute route;
    route.enabled = true;
    route.tableId = tableId;
    route.layerKind = targetTableUsesPositionMode(tableId)
            ? int(PTMultiFxTargetLayerKind::PositionMotion)
            : int(PTMultiFxTargetLayerKind::Channel1D);
    const int outputCount = targetTableOutputCount(tableId);
    for (int outputIdx = 0; outputIdx < outputCount; ++outputIdx)
    {
        PTMultiFxTargetOutputRoute outputRoute;
        outputRoute.outputIndex = outputIdx;
        route.outputs.append(outputRoute);
    }
    if (route.outputs.isEmpty())
    {
        PTMultiFxTargetOutputRoute outputRoute;
        outputRoute.outputIndex = 0;
        route.outputs.append(outputRoute);
    }
    routes.append(route);
    publishProviderSnapshot(QStringLiteral("multifx target add"));
    if (m_doc)
        m_doc->setModified();
}

void PresetTableV2TransitionWidget::removeMultiFxTargetTableRoute(int presetIndex,
                                                                  int routeIndex)
{
    if (presetIndex < 0 || presetIndex >= m_multiFxTargetRoutes.size()
            || routeIndex < 0 || routeIndex >= m_multiFxTargetRoutes.at(presetIndex).size())
        return;
    m_multiFxTargetRoutes[presetIndex].removeAt(routeIndex);
    publishProviderSnapshot(QStringLiteral("multifx target remove"));
    if (m_doc)
        m_doc->setModified();
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveMultiFxRoutePreset(
        int row, int routeIndex, int outputIdx, int selectionIdx) const
{
    PTTransitionPreset preset;
    if (row < 0 || row >= m_multiFxPresets.size())
        return preset;
    preset = m_multiFxPresets.at(row);
    preset.playbackMode = PTTransitionMode::Continuous;

    if (row < 0 || row >= m_multiFxTargetRoutes.size()
            || routeIndex < 0 || routeIndex >= m_multiFxTargetRoutes.at(row).size())
        return preset;

    const PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes.at(row).at(routeIndex);
    applyOverrideColumnsToPreset(preset, route.tableOverride);

    if (outputIdx >= 0 && route.outputOverrides.contains(outputIdx))
    {
        const PTTransitionProviderOutputLayer layer = route.outputOverrides.value(outputIdx);
        applyOverrideColumnsToPreset(preset, layer.all);
        if (selectionIdx >= 0 && selectionIdx < layer.selections.size())
            applyOverrideColumnsToPreset(preset, layer.selections.at(selectionIdx).overrides);
    }

    return preset;
}

QTreeWidgetItem* PresetTableV2TransitionWidget::itemForMultiFxRouteAddress(
        int row, int routeIndex, int outputIdx, int selectionIdx) const
{
    QTreeWidgetItem* parent = parentItemForPreset(PTTransitionMode::MultiFx, row);
    if (!parent || routeIndex < 0)
        return nullptr;

    QTreeWidgetItem* routeItem = nullptr;
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* child = parent->child(i);
        if (child && child->data(0, kItemMultiFxRouteIndexRole).isValid()
                && child->data(0, kItemMultiFxRouteIndexRole).toInt() == routeIndex)
        {
            routeItem = child;
            break;
        }
    }
    if (!routeItem || outputIdx < 0)
        return routeItem;

    QTreeWidgetItem* outputItem = nullptr;
    for (int i = 0; i < routeItem->childCount(); ++i)
    {
        QTreeWidgetItem* child = routeItem->child(i);
        if (child && child->data(0, kItemMultiFxRouteOutputIndexRole).toInt() == outputIdx
                && child->data(0, kItemSelectionIndexRole).toInt() == 0)
        {
            outputItem = child;
            break;
        }
    }
    if (!outputItem || selectionIdx <= 0)
        return outputItem;

    for (int i = 0; i < outputItem->childCount(); ++i)
    {
        QTreeWidgetItem* child = outputItem->child(i);
        if (child && child->data(0, kItemSelectionIndexRole).toInt() == selectionIdx)
            return child;
    }
    return nullptr;
}

QTreeWidgetItem* PresetTableV2TransitionWidget::parentItemForPreset(PTTransitionMode mode,
                                                                    int row) const
{
    QTreeWidget* table = tableForMode(mode);
    if (!table || row < 0 || row >= table->topLevelItemCount())
        return nullptr;
    return table->topLevelItem(row);
}

QTreeWidgetItem* PresetTableV2TransitionWidget::itemForPresetAddress(PTTransitionMode mode,
                                                                     int row,
                                                                     int outputIdx,
                                                                     int selectionIdx) const
{
    QTreeWidgetItem* parent = parentItemForPreset(mode, row);
    if (!parent || outputIdx < 0)
        return parent;

    QTreeWidgetItem* outputItem = nullptr;
    for (int i = 0; i < parent->childCount(); ++i)
    {
        QTreeWidgetItem* child = parent->child(i);
        if (child
                && child->data(0, kItemOutputIndexRole).toInt() == outputIdx
                && child->data(0, kItemSelectionIndexRole).toInt() == 0)
        {
            outputItem = child;
            break;
        }
    }

    if (!outputItem || selectionIdx <= 0)
        return outputItem;

    for (int i = 0; i < outputItem->childCount(); ++i)
    {
        QTreeWidgetItem* child = outputItem->child(i);
        if (child
                && child->data(0, kItemOutputIndexRole).toInt() == outputIdx
                && child->data(0, kItemSelectionIndexRole).toInt() == selectionIdx)
        {
            return child;
        }
    }

    return nullptr;
}

QTreeWidgetItem* PresetTableV2TransitionWidget::selectedPresetItem(QTreeWidget* table) const
{
    if (!table)
        return nullptr;
    QWidget* focus = QApplication::focusWidget();
    if (focus)
    {
        const QVector<QTreeView*> frozenViews = {
            m_sweepNameView, m_continuousNameView, m_channel1DNameView,
            m_positionMotionNameView, m_multiFxNameView
        };
        for (QTreeView* frozen : frozenViews)
        {
            if (!frozen || frozenNameViewForMode(modeForTable(table)) != frozen)
                continue;
            if (focus == frozen || focus == frozen->viewport()
                    || frozen->isAncestorOf(focus))
            {
                if (QTreeWidgetItem* item =
                        m_frozenNameContextItemByTable.value(table, nullptr))
                {
                    if (item->treeWidget() == table)
                        return item;
                }
            }
        }
    }
    return table->currentItem();
}

void PresetTableV2TransitionWidget::updateNameRowContextVisual(QTreeWidget* table,
                                                               QTreeWidgetItem* item)
{
    if (!table || !item || item->treeWidget() != table)
        return;

    const QRect rect = table->visualItemRect(item);
    if (rect.isValid())
        table->viewport()->update(rect);
}

bool PresetTableV2TransitionWidget::isNameRowContextItem(QTreeWidget* table,
                                                         QTreeWidgetItem* item) const
{
    if (!table || !item || item->treeWidget() != table)
        return false;
    return m_nameRowContextItemByTable.value(table, nullptr) == item;
}

void PresetTableV2TransitionWidget::activateNameRowContext(QTreeWidget* table,
                                                           QTreeWidgetItem* item)
{
    if (!table || !item || item->treeWidget() != table)
        return;

    QTreeWidgetItem* previous = m_nameRowContextItemByTable.value(table, nullptr);
    m_nameRowContextItemByTable.insert(table, item);
    m_frozenNameContextItemByTable.insert(table, item);

    if (previous != item)
    {
        updateNameRowContextVisual(table, previous);
        updateNameRowContextVisual(table, item);
    }

    updateRemoveActionLabel();
    applyColumnGroupFilter(table, modeForTable(table));
    updateEffectPreview();
}

bool PresetTableV2TransitionWidget::removeSelectionAt(PTTransitionMode mode, int row,
                                                      int outputIdx, int selectionIndex)
{
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides[row].contains(outputIdx))
        return false;

    PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    if (selectionIndex < 0 || selectionIndex >= layer.selections.size())
        return false;

    layer.selections.removeAt(selectionIndex);
    if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
        overrides[row].remove(outputIdx);
    else
        overrides[row].insert(outputIdx, layer);

    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    updateRemoveActionLabel();
    if (m_doc)
        m_doc->setModified();
    return true;
}

void PresetTableV2TransitionWidget::applySelectionRowVisuals(QTreeWidgetItem* item)
{
    if (!item)
        return;

    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    if (selectionIdx <= 0)
        return;

    const QColor color = selectionLayerColor(selectionIdx - 1);
    item->setForeground(ColName, color);
    QFont font = item->font(ColName);
    font.setBold(true);
    item->setFont(ColName, font);
}

void PresetTableV2TransitionWidget::updateRemoveActionLabel()
{
    if (!m_removeAction)
        return;

    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    const bool slaved = bankIsSlaved(activeBankMode());
    const int selectionIdx = item ? item->data(0, kItemSelectionIndexRole).toInt() : -1;
    const int outputIdx = item ? item->data(0, kItemOutputIndexRole).toInt() : -1;

    if (slaved)
    {
        m_removeAction->setText(tr("Remove"));
        m_removeAction->setToolTip(tr("Slave bank is read-only"));
        m_removeAction->setEnabled(false);
    }
    else
    if (selectionIdx > 0)
    {
        m_removeAction->setText(tr("Remove selection"));
        m_removeAction->setToolTip(tr("Remove the selected custom selection"));
        m_removeAction->setEnabled(true);
    }
    else if (outputIdx < 0)
    {
        m_removeAction->setText(tr("Remove preset"));
        m_removeAction->setToolTip(tr("Remove the selected preset"));
        m_removeAction->setEnabled(true);
    }
    else
    {
        m_removeAction->setText(tr("Remove"));
        m_removeAction->setToolTip(tr("Select a selection row or preset root to remove"));
        m_removeAction->setEnabled(false);
    }
}

void PresetTableV2TransitionWidget::slotSelectionLayerActivatedFromGrid(int selectionIndex)
{
    const PTTransitionMode mode = activeBankMode();
    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    if (!item)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    if (row < 0 || outputIdx < 0)
        return;

    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
        {
            parent->setExpanded(true);
            if (QTreeWidgetItem* routeItem =
                    itemForMultiFxRouteAddress(row, routeIdx, -1, -1))
            {
                routeItem->setExpanded(true);
                if (QTreeWidgetItem* outputItem =
                        itemForMultiFxRouteAddress(row, routeIdx, routeOutputIdx, 0))
                {
                    outputItem->setExpanded(true);
                    if (selectionIndex >= 0)
                    {
                        QTreeWidgetItem* selItem = itemForMultiFxRouteAddress(
                                row, routeIdx, routeOutputIdx, selectionIndex + 1);
                        if (selItem)
                        {
                            table->setCurrentItem(selItem, ColName);
                            updateRemoveActionLabel();
                            updateEffectPreview();
                        }
                    }
                }
            }
        }
        return;
    }

    if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
    {
        parent->setExpanded(true);
        if (QTreeWidgetItem* outputItem = itemForPresetAddress(mode, row, outputIdx, 0))
        {
            outputItem->setExpanded(true);
            if (selectionIndex >= 0)
            {
                QTreeWidgetItem* selItem =
                        itemForPresetAddress(mode, row, outputIdx, selectionIndex + 1);
                if (selItem)
                {
                    table->setCurrentItem(selItem, ColName);
                    updateRemoveActionLabel();
                    updateEffectPreview();
                }
            }
        }
    }
}

void PresetTableV2TransitionWidget::normalizeOverrideStorage()
{
    auto normalize = [](QVector<PTTransitionPreset>& presets,
                       QVector<QHash<int, PTTransitionOutputLayer>>& overrides) {
        while (overrides.size() < presets.size())
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        while (overrides.size() > presets.size())
            overrides.removeLast();
    };
    normalize(m_sweepPresets, m_sweepOutputOverrides);
    normalize(m_continuousPresets, m_continuousOutputOverrides);
    normalize(m_channel1DPresets, m_channel1DOutputOverrides);
    normalize(m_positionMotionPresets, m_positionMotionOutputOverrides);
    normalize(m_multiFxPresets, m_multiFxOutputOverrides);
    normalizeMultiFxTargetRoutes();
}

void PresetTableV2TransitionWidget::refreshOverrideVisualsForPreset(PTTransitionMode mode,
                                                                    int row)
{
    if (mode == PTTransitionMode::MultiFx)
    {
        QVector<PTTransitionPreset> displayPresets;
        QVector<QVector<PTMultiFxTargetTableRoute>> displayRoutes;
        displayDataForMode(mode, displayPresets, displayRoutes);
        refreshMultiFxRouteVisualsForPreset(row, displayPresets, displayRoutes);
        return;
    }

    QTreeWidgetItem* parent = parentItemForPreset(mode, row);
    if (!parent)
        return;

    refreshOverrideVisualsForItem(mode, parent);
    std::function<void(QTreeWidgetItem*)> refreshChildren = [&](QTreeWidgetItem* item) {
        if (!item)
            return;
        for (int i = 0; i < item->childCount(); ++i)
        {
            QTreeWidgetItem* child = item->child(i);
            refreshOverrideVisualsForItem(mode, child);
            refreshChildren(child);
        }
    };
    refreshChildren(parent);
}

void PresetTableV2TransitionWidget::refreshOverrideVisualsForItem(PTTransitionMode mode,
                                                                  QTreeWidgetItem* item)
{
    QTreeWidget* table = tableForMode(mode);
    if (!table || !item)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    if (mode == PTTransitionMode::MultiFx
            && item->data(0, kItemMultiFxRouteIndexRole).isValid())
    {
        QVector<PTTransitionPreset> displayPresets;
        QVector<QVector<PTMultiFxTargetTableRoute>> displayRoutes;
        displayDataForMode(mode, displayPresets, displayRoutes);
        const int routeIdx = item->data(0, kItemMultiFxRouteIndexRole).toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
        const MultiFxUiRow uiRow = multiFxUiRowForAddress(
                row, routeIdx, routeOutputIdx,
                selectionIdx > 0 ? selectionIdx - 1 : -1,
                displayPresets, displayRoutes);
        if (uiRow.valid)
        {
            item->setText(ColName, uiRow.label);
            item->setToolTip(ColName, uiRow.tooltip);
            renderPresetRowCells(item, uiRow, true);
        }
        if (selectionIdx > 0)
            applySelectionRowVisuals(item);
        return;
    }
    const auto& overrides = overridesForMode(mode);
    if (row < 0)
        return;

    for (int col = ColAxis; col < ColCount; ++col)
    {
        if (outputIdx < 0)
        {
            bool hasOutputOverride = false;
            PTTransitionPreset parent = row < presetsForMode(mode).size()
                    ? presetsForMode(mode).at(row) : PTTransitionPreset();
            parent.playbackMode = (mode == PTTransitionMode::Continuous
                                   || mode == PTTransitionMode::PositionMotion
                                   || mode == PTTransitionMode::Channel1D
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
                    const PTTransitionOutputLayer layer = it.value();
                    if (overrideColumnDiffersFromParent(parent, layer.all, col))
                    {
                        hasOutputOverride = true;
                        break;
                    }
                    for (const PTTransitionSelection& selection : layer.selections)
                    {
                        if (overrideColumnDiffersFromParent(parent, selection.overrides, col))
                        {
                            hasOutputOverride = true;
                            break;
                        }
                    }
                    if (hasOutputOverride)
                        break;
                }
            }
            const bool cellSelected = m_selectedCellsByTable.value(table)
                    .contains(PTTransitionCellKey { item, col });
            setPresetCellValue(item, col, item->data(col, kPresetCellValueRole),
                               false, hasOutputOverride, cellSelected);
            continue;
        }

        bool inherited = true;
        if (row < overrides.size() && overrides.at(row).contains(outputIdx))
        {
            const PTTransitionOutputLayer layer = overrides.at(row).value(outputIdx);
            if (selectionIdx > 0)
            {
                const int sel = selectionIdx - 1;
                inherited = !(sel >= 0 && sel < layer.selections.size()
                        && layer.selections.at(sel).overrides.columns.contains(col));
            }
            else
            {
                inherited = !layer.all.columns.contains(col);
            }
        }
        const bool cellSelected = m_selectedCellsByTable.value(table)
                .contains(PTTransitionCellKey { item, col });
        const PTTransitionPreset effective = selectionIdx > 0
                ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
                : effectivePresetForOutputNoLive(mode, row, outputIdx);
        if (inherited)
        {
            setPresetCellValue(item, col, presetColumnValue(effective, col),
                               true, false, cellSelected);
        }
        else
        {
            setPresetCellValue(item, col, presetColumnValue(effective, col),
                               false, false, cellSelected);
        }
    }

    if (selectionIdx > 0)
        applySelectionRowVisuals(item);
}

QSet<int>& PresetTableV2TransitionWidget::expandedSetForMode(PTTransitionMode mode)
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxExpandedPresets;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionExpandedPresets;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DExpandedPresets;
    return (mode == PTTransitionMode::Continuous)
            ? m_continuousExpandedPresets : m_sweepExpandedPresets;
}

const QSet<int>& PresetTableV2TransitionWidget::expandedSetForMode(PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return m_multiFxExpandedPresets;
    if (mode == PTTransitionMode::PositionMotion)
        return m_positionMotionExpandedPresets;
    if (mode == PTTransitionMode::Channel1D)
        return m_channel1DExpandedPresets;
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
    frozen->setRootIsDecorated(true);
    frozen->setItemsExpandable(true);
    frozen->setExpandsOnDoubleClick(false);
    frozen->setIndentation(22);
    frozen->setUniformRowHeights(true);
    frozen->setAlternatingRowColors(true);
    frozen->setSelectionBehavior(QAbstractItemView::SelectRows);
    frozen->setSelectionMode(QAbstractItemView::SingleSelection);
    frozen->setEditTriggers(bankIsSlaved(mode) ? QAbstractItemView::NoEditTriggers
                                               : (QAbstractItemView::DoubleClicked
                                                  | QAbstractItemView::EditKeyPressed));
    frozen->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozen->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    frozen->setStyleSheet(QStringLiteral(
        "QTreeView::item {"
        "  min-height: %1px;"
        "  border-right: 1px solid palette(mid);"
        "  border-bottom: 1px solid palette(mid);"
        "}"
        "QTreeView::item:selected,"
        "QTreeView::item:selected:active,"
        "QTreeView::item:selected:!active {"
        "  background: palette(highlight);"
        "  color: palette(highlighted-text);"
        "}").arg(kEfxTreeRowHeight));
    frozen->header()->setStretchLastSection(true);
    frozen->setColumnHidden(ColName, false);
    for (int c = ColAxis; c < ColCount; ++c)
        frozen->setColumnHidden(c, true);
    frozen->setFixedWidth(kFrozenNameWidth);
    frozen->removeEventFilter(this);
    frozen->viewport()->removeEventFilter(this);
    frozen->installEventFilter(this);
    frozen->viewport()->installEventFilter(this);

    connect(table->verticalScrollBar(), &QScrollBar::valueChanged,
            frozen->verticalScrollBar(), &QScrollBar::setValue, Qt::UniqueConnection);
    connect(frozen->verticalScrollBar(), &QScrollBar::valueChanged,
            table->verticalScrollBar(), &QScrollBar::setValue, Qt::UniqueConnection);

    if (m_tableToFrozenSelectionConnections.contains(table))
        QObject::disconnect(m_tableToFrozenSelectionConnections.take(table));
    if (m_frozenToTableSelectionConnections.contains(table))
        QObject::disconnect(m_frozenToTableSelectionConnections.take(table));

    auto syncSelectionToFrozen = [this, frozen, table](const QItemSelection& selected,
                                                const QItemSelection& deselected) {
        Q_UNUSED(deselected);
        if (m_syncingFrozenSelection)
            return;
        ScopedBoolFlag syncGuard(m_syncingFrozenSelection);
        QItemSelectionModel* frozenSel = frozen->selectionModel();
        if (!frozenSel)
            return;
        QSignalBlocker blocker(frozenSel);
        frozenSel->clearSelection();
        QItemSelection frozenSelection;
        for (const QModelIndex& idx : selected.indexes())
        {
            const QModelIndex nameIdx = idx.siblingAtColumn(ColName);
            if (nameIdx.isValid())
                frozenSelection.select(nameIdx, nameIdx);
        }
        frozenSel->select(frozenSelection, QItemSelectionModel::Select);
        const QModelIndex cur = selected.indexes().isEmpty()
                ? QModelIndex() : selected.indexes().constFirst();
        if (cur.isValid())
        {
            const QModelIndex frozenCur = cur.siblingAtColumn(ColName);
            if (frozenCur.isValid())
            {
                frozenSel->setCurrentIndex(frozenCur, QItemSelectionModel::NoUpdate);
                if (QTreeWidgetItem* item = table->itemFromIndex(frozenCur))
                    m_frozenNameContextItemByTable.insert(table, item);
            }
        }
    };

    auto syncSelectionToTable = [this, table](const QItemSelection& selected,
                                              const QItemSelection& deselected) {
        Q_UNUSED(deselected);
        if (m_syncingFrozenSelection)
            return;
        const QModelIndex cur = selected.indexes().isEmpty()
                ? QModelIndex() : selected.indexes().constFirst();
        if (cur.isValid())
        {
            if (QTreeWidgetItem* item = table->itemFromIndex(cur))
                activateNameRowContext(table, item);
        }
    };

    if (QItemSelectionModel* tableSel = table->selectionModel())
    {
        m_tableToFrozenSelectionConnections.insert(
                table,
                connect(tableSel, &QItemSelectionModel::selectionChanged,
                        frozen, syncSelectionToFrozen));
    }
    if (QItemSelectionModel* frozenSel = frozen->selectionModel())
    {
        m_frozenToTableSelectionConnections.insert(
                table,
                connect(frozenSel, &QItemSelectionModel::selectionChanged,
                        table, syncSelectionToTable));
    }

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
        const PTTransitionPresetOverride ov = overrides.at(row).value(outputIdx).all;
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
    p.playbackMode = (mode == PTTransitionMode::Continuous
                      || mode == PTTransitionMode::PositionMotion
                      || mode == PTTransitionMode::Channel1D
                      || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    PTDimmerWaveEngine::clampOffsetStep(p, gridSpanForPreset(p));
    return p;
}

PTTransitionPreset PresetTableV2TransitionWidget::effectivePresetForSelectionNoLive(
        PTTransitionMode mode, int row, int outputIdx, int selectionIdx) const
{
    PTTransitionPreset p = effectivePresetForOutputNoLive(mode, row, outputIdx);
    const auto& overrides = overridesForMode(mode);
    if (row >= 0 && row < overrides.size() && overrides.at(row).contains(outputIdx))
    {
        const QVector<PTTransitionSelection> selections =
                overrides.at(row).value(outputIdx).selections;
        if (selectionIdx >= 0 && selectionIdx < selections.size())
        {
            const PTTransitionPresetOverride ov = selections.at(selectionIdx).overrides;
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
    }
    p.playbackMode = (mode == PTTransitionMode::Continuous
                      || mode == PTTransitionMode::PositionMotion
                      || mode == PTTransitionMode::Channel1D
                      || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(p);
    PTDimmerWaveEngine::clampOffsetStep(p, gridSpanForPreset(p));
    return p;
}

int PresetTableV2TransitionWidget::selectionIndexForPoint(
        PTTransitionMode mode, int row, int outputIdx, const QLCPoint& point) const
{
    const auto& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides.at(row).contains(outputIdx))
        return -1;
    const QVector<PTTransitionSelection> selections =
            overrides.at(row).value(outputIdx).selections;
    for (int i = 0; i < selections.size(); ++i)
    {
        if (selections.at(i).cells.contains(point))
            return i;
    }
    return -1;
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

void PresetTableV2TransitionWidget::setColumnOverrideValue(
        PTTransitionProviderPresetOverride& ov, int col, const PTTransitionPreset& value)
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

void PresetTableV2TransitionWidget::clearPresetOverrideColumn(
        PTTransitionPresetOverride& ov, int col)
{
    if (col <= ColName || col >= ColCount)
        return;
    ov.columns.remove(col);
    if (col == ColWaveShape)
    {
        ov.values.customCurveEnabled = false;
        ov.values.customCurve.clear();
    }
}

void PresetTableV2TransitionWidget::clearDescendantColumnOverrides(
        PTTransitionMode mode, int row, int outputIdx, int selectionIdx,
        const QSet<int>& cols)
{
    if (cols.isEmpty())
        return;
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size())
        return;

    auto clearCols = [&](PTTransitionPresetOverride& ov) {
        for (int c : cols)
            clearPresetOverrideColumn(ov, c);
    };

    auto clearSelections = [&](PTTransitionOutputLayer& layer) {
        for (PTTransitionSelection& selection : layer.selections)
            clearCols(selection.overrides);
    };

    if (outputIdx < 0)
    {
        for (auto it = overrides[row].begin(); it != overrides[row].end(); ++it)
        {
            PTTransitionOutputLayer layer = it.value();
            clearCols(layer.all);
            clearSelections(layer);
            it.value() = layer;
        }
        return;
    }

    if (!overrides[row].contains(outputIdx))
        return;

    PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    if (selectionIdx <= 0)
        clearSelections(layer);
    overrides[row].insert(outputIdx, layer);
}

void PresetTableV2TransitionWidget::clearMultiFxProviderOverrideColumn(
        PTTransitionProviderPresetOverride& ov, int col)
{
    if (col <= ColName || col >= ColCount)
        return;
    ov.columns.remove(col);
    if (col == ColWaveShape)
    {
        ov.values.customCurveEnabled = false;
        ov.values.customCurve.clear();
    }
}

void PresetTableV2TransitionWidget::clearMultiFxDescendantColumnOverrides(
        int row, int routeIdx, int outputIdx, int selectionIdx, const QSet<int>& cols)
{
    if (row < 0 || row >= m_multiFxTargetRoutes.size() || cols.isEmpty())
        return;

    auto clearCols = [&](PTTransitionProviderPresetOverride& ov) {
        for (int c : cols)
            clearMultiFxProviderOverrideColumn(ov, c);
    };

    auto clearSelections = [&](PTTransitionProviderOutputLayer& layer) {
        for (PTTransitionProviderSelection& selection : layer.selections)
            clearCols(selection.overrides);
    };

    if (routeIdx < 0)
    {
        for (PTMultiFxTargetTableRoute& route : m_multiFxTargetRoutes[row])
        {
            clearCols(route.tableOverride);
            for (auto it = route.outputOverrides.begin(); it != route.outputOverrides.end(); ++it)
            {
                clearCols(it.value().all);
                clearSelections(it.value());
            }
        }
        return;
    }

    if (routeIdx >= m_multiFxTargetRoutes.at(row).size())
        return;

    PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
    if (outputIdx < 0)
    {
        for (auto it = route.outputOverrides.begin(); it != route.outputOverrides.end(); ++it)
        {
            clearCols(it.value().all);
            clearSelections(it.value());
        }
        return;
    }

    if (!route.outputOverrides.contains(outputIdx))
        return;

    PTTransitionProviderOutputLayer layer = route.outputOverrides.value(outputIdx);
    if (selectionIdx <= 0)
        clearSelections(layer);
    route.outputOverrides.insert(outputIdx, layer);
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
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= overrides.size() || row >= presets.size())
        return;

    PTTransitionPreset parent = presets.at(row);
    parent.playbackMode = (mode == PTTransitionMode::Continuous
                           || mode == PTTransitionMode::PositionMotion
                           || mode == PTTransitionMode::Channel1D
                           || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    if (mode == PTTransitionMode::SweepOnly)
        PresetTableV2SpatialEngine::applySweepPresetConstraints(parent);
    PTDimmerWaveEngine::clampOffsetStep(parent, gridSpanForPreset(parent));

    QList<int> emptyOutputs;
    for (auto it = overrides[row].begin(); it != overrides[row].end(); ++it)
    {
        PTTransitionOutputLayer layer = it.value();
        PTTransitionPresetOverride ov = layer.all;
        const QList<int> cols = ov.columns.values();
        for (int col : cols)
        {
            if (!overrideColumnDiffersFromParent(parent, ov, col))
                ov.columns.remove(col);
        }
        layer.all = ov;

        PTTransitionPreset allParent = parent;
        for (int col : layer.all.columns)
            setPresetColumnValue(allParent, col, presetColumnValue(layer.all.values, col));
        for (PTTransitionSelection& selection : layer.selections)
        {
            PTTransitionPresetOverride selOv = selection.overrides;
            const QList<int> selCols = selOv.columns.values();
            for (int col : selCols)
            {
                if (!overrideColumnDiffersFromParent(allParent, selOv, col))
                    selOv.columns.remove(col);
            }
            selection.overrides = selOv;
        }

        if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
            emptyOutputs.append(it.key());
        else
            it.value() = layer;
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
    for (int row = 0; row < m_channel1DPresets.size(); ++row)
        normalizeNoopOverridesForPreset(PTTransitionMode::Channel1D, row);
    for (int row = 0; row < m_positionMotionPresets.size(); ++row)
        normalizeNoopOverridesForPreset(PTTransitionMode::PositionMotion, row);
    for (int row = 0; row < m_multiFxPresets.size(); ++row)
        normalizeNoopOverridesForPreset(PTTransitionMode::MultiFx, row);
}

void PresetTableV2TransitionWidget::clearColumnOverride(PTTransitionMode mode, int row,
                                                       int outputIdx, int selectionIdx,
                                                       int col)
{
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides[row].contains(outputIdx))
        return;
    PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    if (selectionIdx > 0)
    {
        const int sel = selectionIdx - 1;
        if (sel >= 0 && sel < layer.selections.size())
            layer.selections[sel].overrides.columns.remove(col);
    }
    else
    {
        layer.all.columns.remove(col);
    }
    if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
        overrides[row].remove(outputIdx);
    else
        overrides[row].insert(outputIdx, layer);
}

void PresetTableV2TransitionWidget::clearAllOverridesForOutput(PTTransitionMode mode,
                                                               int row, int outputIdx,
                                                               int selectionIdx)
{
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides[row].contains(outputIdx))
        return;
    PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    if (selectionIdx > 0)
    {
        const int sel = selectionIdx - 1;
        if (sel >= 0 && sel < layer.selections.size())
            layer.selections[sel].overrides.columns.clear();
    }
    else
    {
        layer.all.columns.clear();
    }
    if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
        overrides[row].remove(outputIdx);
    else
        overrides[row].insert(outputIdx, layer);
}

void PresetTableV2TransitionWidget::copyOverridesToAllOutputs(PTTransitionMode mode,
                                                              int row, int outputIdx)
{
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (row < 0 || row >= overrides.size() || !overrides[row].contains(outputIdx))
        return;
    const PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    const int count = linkedOutputCount();
    for (int o = 0; o < count; ++o)
    {
        if (o == outputIdx)
            continue;
        if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
            overrides[row].remove(o);
        else
            overrides[row].insert(o, layer);
    }
}

void PresetTableV2TransitionWidget::applySelectionCellsFromGrid(const QSet<QLCPoint>& cells)
{
    const PTTransitionMode mode = activeBankMode();
    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    if (!item)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt() - 1;
    if (row < 0 || outputIdx < 0 || selectionIdx < 0)
        return;

    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        normalizeMultiFxTargetRoutes();
        if (row < 0 || row >= m_multiFxTargetRoutes.size()
                || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size()
                || routeOutputIdx < 0)
            return;

        PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
        PTTransitionProviderOutputLayer layer =
                route.outputOverrides.value(routeOutputIdx);
        if (selectionIdx >= layer.selections.size())
            return;

        QSet<QLCPoint> scopeSet;
        if (PresetTableV2ControlIface* tableIface =
                PresetTableV2VCLookup::controlIfaceByVcId(route.tableId))
        {
            const QList<QLCPoint> scopePoints =
                    tableIface->outputPointsForPresetOverride(routeOutputIdx);
            for (const QLCPoint& pt : scopePoints)
                scopeSet.insert(pt);
        }

        QVector<QLCPoint> newCells;
        QSet<QLCPoint> newSet;
        QList<QLCPoint> sortedCells = cells.values();
        std::sort(sortedCells.begin(), sortedCells.end(), [](const QLCPoint& a, const QLCPoint& b) {
            return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
        });
        for (const QLCPoint& pt : sortedCells)
        {
            if (!scopeSet.isEmpty() && !scopeSet.contains(pt))
                continue;
            newCells.append(pt);
            newSet.insert(pt);
        }
        QSet<QLCPoint> occupied = newSet;
        for (int s = 0; s < layer.selections.size(); ++s)
        {
            if (s == selectionIdx)
                continue;
            QVector<QLCPoint> kept;
            for (const QLCPoint& pt : layer.selections[s].cells)
            {
                if (!occupied.contains(pt))
                {
                    kept.append(pt);
                    occupied.insert(pt);
                }
            }
            layer.selections[s].cells = kept;
        }
        layer.selections[selectionIdx].cells = newCells;
        route.outputOverrides.insert(routeOutputIdx, layer);
        publishProviderSnapshot(QStringLiteral("multifx route selection cells"));
        notifyTablePresetCacheRefresh();
        refreshOverrideVisualsForPreset(mode, row);
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }

    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    while (overrides.size() <= row)
        overrides.append(QHash<int, PTTransitionOutputLayer>());
    PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
    if (selectionIdx >= layer.selections.size())
        return;

    QSet<QLCPoint> scopeSet;
    if (PresetTableV2ControlIface* tableIface = linkedTable())
    {
        const QList<QLCPoint> scopePoints = tableIface->outputPointsForPresetOverride(outputIdx);
        for (const QLCPoint& pt : scopePoints)
            scopeSet.insert(pt);
    }

    QVector<QLCPoint> newCells;
    QSet<QLCPoint> newSet;
    QList<QLCPoint> sortedCells = cells.values();
    std::sort(sortedCells.begin(), sortedCells.end(), [](const QLCPoint& a, const QLCPoint& b) {
        return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
    });
    for (const QLCPoint& pt : sortedCells)
    {
        if (!scopeSet.isEmpty() && !scopeSet.contains(pt))
            continue;
        newCells.append(pt);
        newSet.insert(pt);
    }
    QSet<QLCPoint> occupied = newSet;
    for (int s = 0; s < layer.selections.size(); ++s)
    {
        if (s == selectionIdx)
            continue;
        QVector<QLCPoint> kept;
        for (const QLCPoint& pt : layer.selections[s].cells)
        {
            if (!occupied.contains(pt))
            {
                kept.append(pt);
                occupied.insert(pt);
            }
        }
        layer.selections[s].cells = kept;
    }
    layer.selections[selectionIdx].cells = newCells;
    overrides[row].insert(outputIdx, layer);
    notifyTablePresetCacheRefresh();
    refreshOverrideVisualsForPreset(mode, row);
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
}

void PresetTableV2TransitionWidget::slotPresetContextMenuRequested(const QPoint& pos)
{
    QTreeWidget* table = qobject_cast<QTreeWidget*>(sender());
    if (!table)
        return;

    PTTransitionMode mode = PTTransitionMode::SweepOnly;
    if (table == m_continuousTable)
        mode = PTTransitionMode::Continuous;
    else if (table == m_positionMotionTable)
        mode = PTTransitionMode::PositionMotion;
    else if (table == m_channel1DTable)
        mode = PTTransitionMode::Channel1D;
    else if (table == m_multiFxTable)
        mode = PTTransitionMode::MultiFx;
    else if (table != m_sweepTable)
        return;

    const bool slaved = bankIsSlaved(mode);
    QTreeWidgetItem* item = table->itemAt(pos);
    const int col = table->columnAt(pos.x());
    if (!item || col < ColName || col >= ColCount)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    const QVariant multiFxRouteVar = item->data(0, kItemMultiFxRouteIndexRole);
    const int multiFxRouteIdx = multiFxRouteVar.isValid() ? multiFxRouteVar.toInt() : -1;
    const quint32 multiFxRouteTableId =
            item->data(0, kItemMultiFxRouteTableIdRole).toUInt();
    const int multiFxRouteOutputIdx =
            item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : -1;
    if (col <= ColName)
    {
        showNameContextMenu(mode, table, item, table->viewport()->mapToGlobal(pos));
        return;
    }
    if (mode == PTTransitionMode::MultiFx && multiFxRouteIdx >= 0)
    {
        QMenu menu(this);
        QAction* copySelectionAct = nullptr;
        QAction* pasteSelectionAct = nullptr;
        QAction* removeRouteAct = nullptr;
        QAction* routeAsInterpolationAct = nullptr;
        QAction* routeAsFxAct = nullptr;
        QAction* addSelectionAct = nullptr;
        QAction* removeSelectionAct = nullptr;
        if (canCopySelectionLayer(item))
        {
            copySelectionAct = menu.addAction(tr("Copy selection"));
            copySelectionAct->setShortcut(QKeySequence::Copy);
        }
        if (canPasteSelectionLayer(mode, item))
        {
            pasteSelectionAct = menu.addAction(tr("Paste selection"));
            pasteSelectionAct->setShortcut(QKeySequence::Paste);
        }
        if (!menu.isEmpty())
            menu.addSeparator();
        if (!slaved && multiFxRouteOutputIdx < 0)
        {
            QMenu* kindMenu = menu.addMenu(tr("Mode"));
            routeAsFxAct = kindMenu->addAction(tr("FX"));
            routeAsInterpolationAct = kindMenu->addAction(tr("Interpolation"));
            if (multiFxRouteIdx > 0)
                removeRouteAct = menu.addAction(tr("Remove target table"));
        }
        if (!slaved && multiFxRouteOutputIdx >= 0)
        {
            if (selectionIdx <= 0)
                addSelectionAct = menu.addAction(tr("Add selection"));
            else
                removeSelectionAct = menu.addAction(tr("Remove selection"));
        }
        if (menu.isEmpty())
            return;
        QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
        if (chosen && chosen == copySelectionAct)
        {
            copySelectionLayer(item);
        }
        else if (chosen && chosen == pasteSelectionAct)
        {
            pasteSelectionLayer(mode, item);
        }
        else if (chosen && chosen == removeRouteAct)
        {
            removeMultiFxTargetTableRoute(row, multiFxRouteIdx);
            rebuildPresetTable(mode);
            notifyTablePresetCacheRefresh();
            updateEffectPreview();
        }
        else if (chosen && (chosen == routeAsInterpolationAct
                            || chosen == routeAsFxAct))
        {
            normalizeMultiFxTargetRoutes();
            if (row >= 0 && row < m_multiFxTargetRoutes.size()
                    && multiFxRouteIdx >= 0
                    && multiFxRouteIdx < m_multiFxTargetRoutes.at(row).size())
            {
                PTMultiFxTargetTableRoute& route =
                        m_multiFxTargetRoutes[row][multiFxRouteIdx];
                if (chosen == routeAsInterpolationAct)
                    route.layerKind = int(PTMultiFxTargetLayerKind::Interpolation);
                else
                    route.layerKind = targetTableUsesPositionMode(route.tableId)
                            ? int(PTMultiFxTargetLayerKind::PositionMotion)
                            : int(PTMultiFxTargetLayerKind::Channel1D);
                publishProviderSnapshot(QStringLiteral("multifx route layer"));
                rebuildPresetTable(mode);
                if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
                {
                    parent->setExpanded(true);
                    if (QTreeWidgetItem* routeItem =
                            itemForMultiFxRouteAddress(row, multiFxRouteIdx, -1, -1))
                        routeItem->setExpanded(true);
                }
                notifyTablePresetCacheRefresh();
                updateEffectPreview();
                if (m_doc)
                    m_doc->setModified();
            }
        }
        else if (chosen && (chosen == addSelectionAct || chosen == removeSelectionAct))
        {
            normalizeMultiFxTargetRoutes();
            if (row >= 0 && row < m_multiFxTargetRoutes.size()
                    && multiFxRouteIdx < m_multiFxTargetRoutes.at(row).size())
            {
                PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][multiFxRouteIdx];
                PTTransitionProviderOutputLayer layer =
                        route.outputOverrides.value(multiFxRouteOutputIdx);
                if (chosen == addSelectionAct)
                {
                    PTTransitionProviderSelection selection;
                    selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                    layer.selections.append(selection);
                }
                else if (selectionIdx > 0)
                {
                    const int sel = selectionIdx - 1;
                    if (sel >= 0 && sel < layer.selections.size())
                        layer.selections.removeAt(sel);
                }
                route.outputOverrides.insert(multiFxRouteOutputIdx, layer);
                publishProviderSnapshot(QStringLiteral("multifx route selection"));
                rebuildPresetTable(mode);
                if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
                {
                    parent->setExpanded(true);
                    if (QTreeWidgetItem* routeItem =
                            itemForMultiFxRouteAddress(row, multiFxRouteIdx, -1, -1))
                    {
                        routeItem->setExpanded(true);
                        if (QTreeWidgetItem* outputItem =
                                itemForMultiFxRouteAddress(row, multiFxRouteIdx,
                                                           multiFxRouteOutputIdx, 0))
                            outputItem->setExpanded(true);
                    }
                }
                notifyTablePresetCacheRefresh();
                updateEffectPreview();
                if (m_doc)
                    m_doc->setModified();
            }
        }
        Q_UNUSED(multiFxRouteTableId);
        return;
    }
    if (outputIdx < 0)
    {
        QMenu menu(this);
        QAction* pasteSelectionAct = nullptr;
        if (canPasteSelectionLayer(mode, item))
        {
            pasteSelectionAct = menu.addAction(tr("Paste selection"));
            pasteSelectionAct->setShortcut(QKeySequence::Paste);
            menu.addSeparator();
        }
        QAction* toggleAct = item->childCount() > 0
                ? menu.addAction(item->isExpanded()
                        ? tr("Collapse outputs") : tr("Expand outputs"))
                : nullptr;
        QMenu* addTargetMenu = nullptr;
        if (mode == PTTransitionMode::MultiFx && !slaved)
        {
            addTargetMenu = menu.addMenu(tr("Add target table"));
            for (VCWidget* widget : PresetTableV2VCLookup::allVcWidgets())
            {
                if (!widget || !qobject_cast<PresetTableV2ControlIface*>(widget))
                    continue;
                QAction* act = addTargetMenu->addAction(PresetTableV2VCLookup::vcWidgetLabel(widget));
                act->setData(widget->id());
            }
            if (addTargetMenu->isEmpty())
                addTargetMenu->setEnabled(false);
        }
        if (menu.isEmpty())
            return;
        QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
        if (chosen && chosen == pasteSelectionAct)
            pasteSelectionLayer(mode, item);
        else if (chosen && chosen == toggleAct)
            item->setExpanded(!item->isExpanded());
        else if (chosen && addTargetMenu && addTargetMenu->actions().contains(chosen))
        {
            addMultiFxTargetTableRoute(row, chosen->data().toUInt());
            rebuildPresetTable(mode);
            if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
                parent->setExpanded(true);
            notifyTablePresetCacheRefresh();
            updateEffectPreview();
        }
        return;
    }

    if (col <= ColName)
    {
        if (slaved)
            return;
        QMenu menu(this);
        QAction* copySelectionAct = nullptr;
        QAction* pasteSelectionAct = nullptr;
        QAction* addSelectionAct = nullptr;
        QAction* removeSelectionAct = nullptr;
        if (canCopySelectionLayer(item))
        {
            copySelectionAct = menu.addAction(tr("Copy selection"));
            copySelectionAct->setShortcut(QKeySequence::Copy);
        }
        if (canPasteSelectionLayer(mode, item))
        {
            pasteSelectionAct = menu.addAction(tr("Paste selection"));
            pasteSelectionAct->setShortcut(QKeySequence::Paste);
        }
        if (!menu.isEmpty())
            menu.addSeparator();
        if (selectionIdx <= 0)
            addSelectionAct = menu.addAction(tr("Add selection"));
        else
        {
            removeSelectionAct = menu.addAction(tr("Remove selection"));
        }
        QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
        if (!chosen)
            return;
        if (chosen == copySelectionAct)
        {
            copySelectionLayer(item);
            return;
        }
        if (chosen == pasteSelectionAct)
        {
            pasteSelectionLayer(mode, item);
            return;
        }

        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        while (overrides.size() <= row)
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = overrides[row].value(outputIdx);

        if (chosen == addSelectionAct)
        {
            PTTransitionSelection selection;
            selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
            layer.selections.append(selection);
            overrides[row].insert(outputIdx, layer);
            const int newSelectionIdx = layer.selections.size() - 1;
            rebuildPresetTable(mode);
            if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
            {
                parent->setExpanded(true);
                if (QTreeWidgetItem* outputItem = itemForPresetAddress(mode, row, outputIdx, 0))
                {
                    outputItem->setExpanded(true);
                    if (QTreeWidgetItem* selItem =
                            itemForPresetAddress(mode, row, outputIdx, newSelectionIdx + 1))
                        table->setCurrentItem(selItem, ColName);
                }
            }
            notifyTablePresetCacheRefresh();
            updateEffectPreview();
            updateRemoveActionLabel();
            if (m_doc)
                m_doc->setModified();
        }
        else if (chosen == removeSelectionAct)
        {
            removeSelectionAt(mode, row, outputIdx, selectionIdx - 1);
        }
        return;
    }

    showParameterContextMenu(mode, table, item, col, table->viewport()->mapToGlobal(pos));
}

QVariant PresetTableV2TransitionWidget::editorValue(QTreeWidget* table,
                                                    QTreeWidgetItem* item, int col) const
{
    if (!table || !item)
        return QVariant();
    if (col == ColName)
        return item->text(ColName);
    return item->text(col);
}

void PresetTableV2TransitionWidget::setEditorValue(QTreeWidget* table,
                                                   QTreeWidgetItem* item, int col,
                                                   const QString& raw)
{
    if (!table || !item || col <= ColName || col >= ColCount)
        return;
    const QVariant value = normalizedColumnValue(col, raw);
    if (!value.isValid())
        return;
    setPresetCellValue(item, col, value,
                       item->data(col, kPresetCellInheritedRole).toBool());
}

PTTransitionMode PresetTableV2TransitionWidget::modeForTable(QTreeWidget* table) const
{
    if (table == m_continuousTable)
        return PTTransitionMode::Continuous;
    if (table == m_positionMotionTable)
        return PTTransitionMode::PositionMotion;
    if (table == m_channel1DTable)
        return PTTransitionMode::Channel1D;
    if (table == m_multiFxTable)
        return PTTransitionMode::MultiFx;
    return PTTransitionMode::SweepOnly;
}

QTreeWidget* PresetTableV2TransitionWidget::tableFromFocusObject(QObject* watched) const
{
    QWidget* watchedWidget = qobject_cast<QWidget*>(watched);
    for (QTreeWidget* candidate : { m_sweepTable, m_continuousTable,
                                    m_positionMotionTable, m_channel1DTable,
                                    m_multiFxTable })
    {
        if (!candidate)
            continue;
        if (watched == candidate || watched == candidate->viewport()
                || (watchedWidget && candidate->isAncestorOf(watchedWidget)))
        {
            return candidate;
        }
    }

    const QVector<QPair<QTreeView*, QTreeWidget*>> frozenViews = {
        { m_sweepNameView, m_sweepTable },
        { m_continuousNameView, m_continuousTable },
        { m_channel1DNameView, m_channel1DTable },
        { m_positionMotionNameView, m_positionMotionTable },
        { m_multiFxNameView, m_multiFxTable }
    };
    for (const auto& pair : frozenViews)
    {
        if (watched == pair.first || (pair.first && watched == pair.first->viewport()))
            return pair.second;
    }
    return nullptr;
}

int PresetTableV2TransitionWidget::focusColumn(QTreeWidget* table) const
{
    if (!table)
        return ColAxis;

    if (m_focusColumnByTable.contains(table))
    {
        const int stored = m_focusColumnByTable.value(table);
        if (stored > ColName && stored < ColCount && !table->isColumnHidden(stored))
            return stored;
    }

    const int current = table->currentColumn();
    if (current > ColName && current < ColCount && !table->isColumnHidden(current))
        return current;

    return ColAxis;
}

void PresetTableV2TransitionWidget::setFocusColumn(QTreeWidget* table,
                                                 QTreeWidgetItem* item, int col)
{
    if (!table || !item || col <= ColName || col >= ColCount || table->isColumnHidden(col))
        return;

    m_focusColumnByTable.insert(table, col);
    table->setCurrentItem(item, col);
    updateColumnFocusVisuals(table);
}

bool PresetTableV2TransitionWidget::isClipboardCell(QTreeWidget* table,
                                                    QTreeWidgetItem* item, int col) const
{
    if (!table || !item || col <= ColName || col >= ColCount || table->isColumnHidden(col))
        return false;
    return true;
}

bool PresetTableV2TransitionWidget::sameClipboardContext(QTreeWidgetItem* a,
                                                         QTreeWidgetItem* b) const
{
    if (!a || !b)
        return false;
    return a->data(0, kItemOutputIndexRole).toInt() == b->data(0, kItemOutputIndexRole).toInt()
            && a->data(0, kItemSelectionIndexRole).toInt()
                    == b->data(0, kItemSelectionIndexRole).toInt();
}

QList<QTreeWidgetItem*> PresetTableV2TransitionWidget::clipboardRowsInContext(
        QTreeWidget* table, QTreeWidgetItem* contextItem) const
{
    QList<QTreeWidgetItem*> rows;
    if (!table || !contextItem)
        return rows;

    QTreeWidgetItemIterator it(table);
    while (*it)
    {
        if (sameClipboardContext(*it, contextItem))
            rows.append(*it);
        ++it;
    }
    return rows;
}

QList<PTTransitionCellKey> PresetTableV2TransitionWidget::selectedCellsInVisualOrder(
        QTreeWidget* table) const
{
    QList<PTTransitionCellKey> ordered;
    if (!table)
        return ordered;

    const QModelIndexList indexes = table->selectionModel()
            ? table->selectionModel()->selectedIndexes() : QModelIndexList();
    for (const QModelIndex& idx : indexes)
    {
        const int col = idx.column();
        if (col <= ColName || col >= ColCount || table->isColumnHidden(col))
            continue;
        if (QTreeWidgetItem* item = table->itemFromIndex(idx))
            ordered.append(PTTransitionCellKey { item, col });
    }
    std::sort(ordered.begin(), ordered.end(), [table](const PTTransitionCellKey& a,
                                                      const PTTransitionCellKey& b) {
        const QModelIndex ia = table->indexFromItem(a.item, a.col);
        const QModelIndex ib = table->indexFromItem(b.item, b.col);
        if (ia.row() == ib.row())
            return ia.column() < ib.column();
        return ia.row() < ib.row();
    });
    if (!ordered.isEmpty())
        return ordered;

    const QSet<PTTransitionCellKey> selected = m_selectedCellsByTable.value(table);
    if (selected.isEmpty())
        return ordered;

    QTreeWidgetItemIterator it(table);
    while (*it)
    {
        for (int col = ColAxis; col < ColCount; ++col)
        {
            const PTTransitionCellKey key { *it, col };
            if (selected.contains(key))
                ordered.append(key);
        }
        ++it;
    }
    return ordered;
}

void PresetTableV2TransitionWidget::clearCellSelection(QTreeWidget* table)
{
    if (!table)
        return;
    m_selectedCellsByTable.remove(table);
    m_cellSelectionAnchorByTable.remove(table);
}

void PresetTableV2TransitionWidget::updateCellSelectionVisuals(QTreeWidget* table)
{
    if (!table)
        return;

    const PTTransitionMode mode = modeForTable(table);
    QTreeWidgetItemIterator it(table);
    while (*it)
    {
        refreshOverrideVisualsForItem(mode, *it);
        ++it;
    }
}

void PresetTableV2TransitionWidget::activateCellForClipboard(QTreeWidget* table,
                                                           QTreeWidgetItem* item, int col,
                                                           Qt::KeyboardModifiers mods)
{
    if (!isClipboardCell(table, item, col))
        return;

    Q_UNUSED(item)
    Q_UNUSED(mods)
    if (QTreeWidgetItem* previous = m_nameRowContextItemByTable.take(table))
        updateNameRowContextVisual(table, previous);
    m_frozenNameContextItemByTable.remove(table);
    m_focusColumnByTable.insert(table, col);
    clearCellSelection(table);
    updateColumnFocusVisuals(table);
}

void PresetTableV2TransitionWidget::updateColumnFocusVisuals(QTreeWidget* table)
{
    if (!table)
        return;

    QTreeWidgetItem* header = table->headerItem();
    if (!header)
        return;

    const int focus = focusColumn(table);
    const QColor normal = palette().color(QPalette::Text);
    const QColor active = palette().color(QPalette::Highlight);
    for (int c = ColAxis; c < ColCount; ++c)
        header->setForeground(c, c == focus ? active : normal);
}

void PresetTableV2TransitionWidget::showParameterContextMenu(PTTransitionMode mode,
                                                             QTreeWidget* table,
                                                             QTreeWidgetItem* item, int col,
                                                             const QPoint& globalPos)
{
    if (!table || !item || col <= ColName || col >= ColCount)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();

    activateCellForClipboard(table, item, col, Qt::NoModifier);

    QMenu menu(this);
    QAction* copyAct = menu.addAction(tr("Copy cells"));
    QAction* pasteAct = menu.addAction(tr("Paste cells"));
    copyAct->setShortcut(QKeySequence::Copy);
    pasteAct->setShortcut(QKeySequence::Paste);
    const int clipCol = clipboardColumnForPaste();
    const bool slaved = bankIsSlaved(mode);
    pasteAct->setEnabled(!QApplication::clipboard()->text().trimmed().isEmpty()
                         && (clipCol < 0 || clipCol == col)
                         && !slaved);
    menu.addSeparator();
    QAction* overrideAct = menu.addAction(tr("Override parameter"));
    QAction* resetParamAct = menu.addAction(tr("Reset parameter override"));
    QAction* resetAllAct = menu.addAction(selectionIdx > 0
            ? tr("Reset all overrides for this selection")
            : tr("Reset all overrides for All"));
    QAction* copyAllAct = selectionIdx > 0 ? nullptr
            : menu.addAction(tr("Copy output layer to all outputs"));
    overrideAct->setEnabled(!slaved);
    resetParamAct->setEnabled(!slaved);
    resetAllAct->setEnabled(!slaved);
    if (copyAllAct)
        copyAllAct->setEnabled(!slaved);
    QAction* chosen = menu.exec(globalPos);
    if (!chosen)
        return;

    if (chosen == copyAct)
    {
        copyCells(table);
        return;
    }
    if (chosen == pasteAct)
    {
        pasteCells(table);
        return;
    }

    if (chosen == overrideAct)
        slotPresetChanged(mode, row, col, outputIdx, selectionIdx);
    else if (chosen == resetParamAct)
        clearColumnOverride(mode, row, outputIdx, selectionIdx, col);
    else if (chosen == resetAllAct)
        clearAllOverridesForOutput(mode, row, outputIdx, selectionIdx);
    else if (copyAllAct && chosen == copyAllAct)
        copyOverridesToAllOutputs(mode, row, outputIdx);

    normalizeNoopOverridesForPreset(mode, row);
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
}

void PresetTableV2TransitionWidget::showNameContextMenu(
        PTTransitionMode mode, QTreeWidget* table, QTreeWidgetItem* item,
        const QPoint& globalPos)
{
    if (!table || !item)
        return;

    const bool slaved = bankIsSlaved(mode);
    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    const int routeIdx = routeVar.isValid() ? routeVar.toInt() : -1;
    const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;

    QMenu menu(this);
    QAction* copyLayerAct = menu.addAction(tr("Copy layer"));
    copyLayerAct->setShortcut(QKeySequence::Copy);
    QAction* pasteLayerAct = menu.addAction(tr("Paste layer"));
    pasteLayerAct->setShortcut(QKeySequence::Paste);
    pasteLayerAct->setEnabled(canPasteTreeLayer(mode, item));

    QAction* duplicateAct = nullptr;
    if (!slaved && (!item->parent() || selectionIdx > 0))
    {
        duplicateAct = menu.addAction(!item->parent()
                ? tr("Duplicate preset with children")
                : tr("Duplicate selection"));
    }

    QAction* addSelectionAct = nullptr;
    QAction* removeSelectionAct = nullptr;
    QAction* addTargetMenuAction = nullptr;
    QAction* removeTargetAct = nullptr;
    QMenu* addTargetMenu = nullptr;
    if (!slaved)
    {
        if (selectionIdx > 0)
            removeSelectionAct = menu.addAction(tr("Remove selection"));
        else if (outputIdx >= 0 || (mode == PTTransitionMode::MultiFx && routeOutputIdx >= 0))
            addSelectionAct = menu.addAction(tr("Add selection"));

        if (mode == PTTransitionMode::MultiFx)
        {
            if (!item->parent())
            {
                addTargetMenu = menu.addMenu(tr("Add target table"));
                for (VCWidget* widget : PresetTableV2VCLookup::allVcWidgets())
                {
                    if (!widget || !qobject_cast<PresetTableV2ControlIface*>(widget))
                        continue;
                    QAction* act = addTargetMenu->addAction(
                            PresetTableV2VCLookup::vcWidgetLabel(widget));
                    act->setData(widget->id());
                }
                if (addTargetMenu->isEmpty())
                    addTargetMenu->setEnabled(false);
            }
            else if (routeIdx > 0 && routeOutputIdx < 0 && selectionIdx <= 0)
                removeTargetAct = menu.addAction(tr("Remove target table"));
        }
    }

    QAction* toggleAct = nullptr;
    if (item->childCount() > 0)
    {
        menu.addSeparator();
        toggleAct = menu.addAction(item->isExpanded()
                ? tr("Collapse children") : tr("Expand children"));
    }

    QAction* chosen = menu.exec(globalPos);
    if (!chosen)
        return;

    if (chosen == copyLayerAct)
    {
        copyTreeLayer(item);
        return;
    }
    if (chosen == pasteLayerAct)
    {
        pasteTreeLayer(mode, item);
        return;
    }
    if (chosen == duplicateAct)
    {
        duplicateTreeLayer(mode, item);
        return;
    }
    if (chosen == toggleAct)
    {
        item->setExpanded(!item->isExpanded());
        return;
    }
    if (chosen == removeTargetAct)
    {
        removeMultiFxTargetTableRoute(row, routeIdx);
        rebuildPresetTable(mode);
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }
    if (addTargetMenu && addTargetMenu->actions().contains(chosen))
    {
        addMultiFxTargetTableRoute(row, chosen->data().toUInt());
        rebuildPresetTable(mode);
        if (QTreeWidgetItem* parent = parentItemForPreset(mode, row))
            parent->setExpanded(true);
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }
    Q_UNUSED(addTargetMenuAction);

    if (chosen == addSelectionAct || chosen == removeSelectionAct)
    {
        if (mode == PTTransitionMode::MultiFx && routeIdx >= 0)
        {
            normalizeMultiFxTargetRoutes();
            if (row >= 0 && row < m_multiFxTargetRoutes.size()
                    && routeIdx < m_multiFxTargetRoutes.at(row).size()
                    && routeOutputIdx >= 0)
            {
                PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
                PTTransitionProviderOutputLayer layer =
                        route.outputOverrides.value(routeOutputIdx);
                if (chosen == addSelectionAct)
                {
                    PTTransitionProviderSelection selection;
                    selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                    layer.selections.append(selection);
                }
                else if (selectionIdx > 0)
                {
                    const int sel = selectionIdx - 1;
                    if (sel >= 0 && sel < layer.selections.size())
                        layer.selections.removeAt(sel);
                }
                route.outputOverrides.insert(routeOutputIdx, layer);
                publishProviderSnapshot(QStringLiteral("multifx route selection"));
            }
        }
        else
        {
            QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
            while (overrides.size() <= row)
                overrides.append(QHash<int, PTTransitionOutputLayer>());
            PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
            if (chosen == addSelectionAct)
            {
                PTTransitionSelection selection;
                selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                layer.selections.append(selection);
            }
            else if (selectionIdx > 0)
            {
                const int sel = selectionIdx - 1;
                if (sel >= 0 && sel < layer.selections.size())
                    layer.selections.removeAt(sel);
            }
            overrides[row].insert(outputIdx, layer);
        }

        rebuildPresetTable(mode);
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
    }
}

QList<QTreeWidgetItem*> PresetTableV2TransitionWidget::selectedRowsInVisualOrder(
        QTreeWidget* table) const
{
    if (!table)
        return {};

    const QList<PTTransitionCellKey> cells = selectedCellsInVisualOrder(table);
    if (!cells.isEmpty())
    {
        QList<QTreeWidgetItem*> items;
        for (const PTTransitionCellKey& key : cells)
        {
            if (key.item && !items.contains(key.item))
                items.append(key.item);
        }
        return items;
    }

    QList<QTreeWidgetItem*> items = table->selectedItems();
    if (items.isEmpty() && table->currentItem())
        items.append(table->currentItem());
    std::sort(items.begin(), items.end(), [table](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        return table->indexFromItem(a) < table->indexFromItem(b);
    });
    return items;
}

bool PresetTableV2TransitionWidget::findItemForEditor(QTreeWidget* table, QWidget* editor,
                                                      QTreeWidgetItem** item, int* col) const
{
    if (!table || !editor || !item || !col)
        return false;

    QTreeWidgetItemIterator it(table);
    while (*it)
    {
        for (int c = ColAxis; c < ColCount; ++c)
        {
            if (table->itemWidget(*it, c) == editor)
            {
                *item = *it;
                *col = c;
                return true;
            }
        }
        ++it;
    }
    return false;
}

int PresetTableV2TransitionWidget::clipboardColumnForPaste() const
{
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasFormat(QLatin1String(kMimeEfxColumn)))
        return -1;

    bool ok = false;
    const int col = mime->data(QLatin1String(kMimeEfxColumn)).toInt(&ok);
    return ok ? col : -1;
}

QTreeWidgetItem* PresetTableV2TransitionWidget::itemForAddress(
        PTTransitionMode mode, const PTTransitionCellAddress& address) const
{
    QTreeWidgetItem* item = parentItemForPreset(mode, address.row);
    if (item && address.outputIdx >= 0)
        item = item->child(address.outputIdx);
    if (item && address.selectionIdx > 0)
        item = item->child(address.selectionIdx - 1);
    return item;
}

void PresetTableV2TransitionWidget::applyPastedCellValue(
        PTTransitionMode mode, const PTTransitionCellAddress& address,
        const QString& raw, QSet<int>& touchedRows)
{
    if (address.row < 0 || address.col <= ColName || address.col >= ColCount)
        return;
    if (mode == PTTransitionMode::SweepOnly
            && (address.col == ColOffsetStepMode || address.col == ColOffsetStep))
        return;

    const QVariant value = normalizedColumnValue(address.col, raw);
    if (!value.isValid())
        return;

    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (address.row >= presets.size())
        return;

    QTreeWidgetItem* item = itemForAddress(mode, address);
    if (!item)
        return;

    setPresetCellValue(item, address.col, value,
                       item->data(address.col, kPresetCellInheritedRole).toBool());

    PTTransitionPreset candidate = address.selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, address.row, address.outputIdx,
                                                address.selectionIdx - 1)
            : (address.outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, address.row, address.outputIdx)
               : presets.at(address.row));
    setPresetColumnValue(candidate, address.col, value);

    if (address.outputIdx >= 0)
    {
        QVector<QHash<int, PTTransitionOutputLayer>>& allOverrides = overridesForMode(mode);
        while (allOverrides.size() <= address.row)
            allOverrides.append(QHash<int, PTTransitionOutputLayer>());
        PTTransitionOutputLayer layer = allOverrides[address.row].value(address.outputIdx);
        if (address.selectionIdx > 0)
        {
            const int sel = address.selectionIdx - 1;
            while (layer.selections.size() <= sel)
            {
                PTTransitionSelection selection;
                selection.name = tr("Selection %1").arg(layer.selections.size() + 1);
                layer.selections.append(selection);
            }
            PTTransitionPresetOverride ov = layer.selections[sel].overrides;
            ov.values = candidate;
            ov.columns.insert(address.col);
            layer.selections[sel].overrides = ov;
        }
        else
        {
            PTTransitionPresetOverride ov = layer.all;
            ov.values = candidate;
            ov.columns.insert(address.col);
            layer.all = ov;
        }
        allOverrides[address.row].insert(address.outputIdx, layer);
        expandedSetForMode(mode).insert(address.row);
    }
    else
    {
        presets[address.row] = candidate;
    }

    touchedRows.insert(address.row);
}

void PresetTableV2TransitionWidget::pasteValueToPresetCell(PTTransitionMode mode,
                                                           QTreeWidget* table,
                                                           QTreeWidgetItem* item, int col,
                                                           const QString& raw)
{
    if (!table || !item || col <= ColName || col >= ColCount)
        return;
    if (bankIsSlaved(mode))
        return;

    setEditorValue(table, item, col, raw);
    slotPresetChanged(mode,
                      item->data(0, kItemPresetIndexRole).toInt(),
                      col,
                      item->data(0, kItemOutputIndexRole).toInt(),
                      item->data(0, kItemSelectionIndexRole).toInt());
}

void PresetTableV2TransitionWidget::copyCells(QTreeWidget* table)
{
    if (!table)
        return;

    QList<PTTransitionCellKey> cells = selectedCellsInVisualOrder(table);
    int col = -1;
    if (!cells.isEmpty())
    {
        for (const PTTransitionCellKey& key : cells)
        {
            if (col < 0)
                col = key.col;
            else if (key.col != col)
                return;
        }
    }
    else
    {
        const QList<QTreeWidgetItem*> items = selectedRowsInVisualOrder(table);
        if (items.isEmpty())
            return;
        col = focusColumn(table);
        for (QTreeWidgetItem* item : items)
            cells.append(PTTransitionCellKey { item, col });
    }

    if (col <= ColName || col >= ColCount || table->isColumnHidden(col))
        return;

    QStringList lines;
    for (const PTTransitionCellKey& key : cells)
        lines << editorValue(table, key.item, key.col).toString();

    auto* mime = new QMimeData();
    mime->setText(lines.join(QLatin1Char('\n')));
    mime->setData(QLatin1String(kMimeEfxColumn), QByteArray::number(col));
    QApplication::clipboard()->setMimeData(mime);
}

bool PresetTableV2TransitionWidget::canCopySelectionLayer(QTreeWidgetItem* item) const
{
    if (!item || item->data(0, kItemSelectionIndexRole).toInt() <= 0)
        return false;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt() - 1;
    if (row < 0 || outputIdx < 0 || selectionIdx < 0)
        return false;

    QTreeWidget* table = item->treeWidget();
    const PTTransitionMode mode = modeForTable(table);
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        if (row >= m_multiFxTargetRoutes.size()
                || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size()
                || routeOutputIdx < 0)
            return false;
        const PTTransitionProviderOutputLayer layer =
                m_multiFxTargetRoutes.at(row).at(routeIdx).outputOverrides.value(routeOutputIdx);
        return selectionIdx < layer.selections.size();
    }

    const QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (row >= overrides.size() || !overrides.at(row).contains(outputIdx))
        return false;
    return selectionIdx < overrides.at(row).value(outputIdx).selections.size();
}

bool PresetTableV2TransitionWidget::copyTreeLayer(QTreeWidgetItem* item)
{
    if (!item)
        return false;

    const PTTransitionMode mode = modeForTable(item->treeWidget());
    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    if (row < 0)
        return false;

    auto visibleColumnsForItem = [](QTreeWidgetItem* src) {
        QSet<int> cols;
        if (!src)
            return cols;
        for (int col = ColAxis; col < ColCount; ++col)
        {
            if (src->data(col, kPresetCellValueRole).isValid())
                cols.insert(col);
        }
        return cols;
    };
    auto providerOverrideFromEffective = [&](QTreeWidgetItem* src,
                                             const PTTransitionPreset& effective) {
        PTTransitionProviderPresetOverride ov;
        ov.values = effective;
        ov.columns = visibleColumnsForItem(src);
        return ov;
    };
    auto classicOverrideFromEffective = [&](QTreeWidgetItem* src,
                                            const PTTransitionPreset& effective) {
        PTTransitionPresetOverride ov;
        ov.values = effective;
        ov.columns = visibleColumnsForItem(src);
        return ov;
    };

    PTEfxTreeClipboard clip;
    clip.valid = true;
    clip.mode = mode;

    if (!item->parent())
    {
        if (!bankIsSlaved(mode))
            syncPresetFromTable(mode, row);

        QVector<PTTransitionPreset> displayPresets;
        QVector<QVector<PTMultiFxTargetTableRoute>> displayRoutes;
        if (mode == PTTransitionMode::MultiFx)
            normalizeMultiFxTargetRoutes();
        displayDataForMode(mode, displayPresets, displayRoutes);
        if (row >= displayPresets.size())
            return false;
        clip.kind = PTEfxTreeClipboardKind::Preset;
        clip.preset = displayPresets.at(row);
        const QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        if (!bankIsSlaved(mode) && row < overrides.size())
            clip.classicOutputLayers = overrides.at(row);
        if (mode == PTTransitionMode::MultiFx)
        {
            if (row < displayRoutes.size())
                clip.multiFxRoutes = displayRoutes.at(row);
        }
        m_treeClipboard = clip;
        return true;
    }

    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        normalizeMultiFxTargetRoutes();
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        if (row >= m_multiFxTargetRoutes.size()
                || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size())
            return false;
        const PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes.at(row).at(routeIdx);
        if (selectionIdx > 0)
        {
            const PTTransitionProviderOutputLayer layer =
                    route.outputOverrides.value(routeOutputIdx);
            const int sel = selectionIdx - 1;
            if (sel < 0 || sel >= layer.selections.size())
                return false;
            const PTTransitionPreset effective =
                    effectiveMultiFxRoutePreset(row, routeIdx, routeOutputIdx, sel);
            clip.kind = PTEfxTreeClipboardKind::SelectionLayer;
            clip.providerLayer = true;
            clip.providerSelection = layer.selections.at(sel);
            clip.providerSelection.overrides =
                    providerOverrideFromEffective(item, effective);
            m_selectionClipboard.valid = true;
            m_selectionClipboard.name = clip.providerSelection.name;
            m_selectionClipboard.cells = clip.providerSelection.cells;
            m_selectionClipboard.overrides = clip.providerSelection.overrides;
        }
        else if (routeOutputIdx >= 0)
        {
            clip.kind = PTEfxTreeClipboardKind::OutputLayer;
            clip.providerLayer = true;
            clip.outputIndex = routeOutputIdx;
            clip.providerOutputLayer = route.outputOverrides.value(routeOutputIdx);
            clip.providerOutputLayer.all = providerOverrideFromEffective(
                    item, effectiveMultiFxRoutePreset(row, routeIdx, routeOutputIdx, -1));
        }
        else
        {
            clip.kind = PTEfxTreeClipboardKind::MultiFxRoute;
            clip.multiFxRoute = route;
            clip.multiFxRoute.tableOverride = providerOverrideFromEffective(
                    item, effectiveMultiFxRoutePreset(row, routeIdx, -1, -1));
        }
        m_treeClipboard = clip;
        return true;
    }

    const QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    if (outputIdx < 0 || row >= overrides.size())
        return false;
    const PTTransitionOutputLayer layer = overrides.at(row).value(outputIdx);
    if (selectionIdx > 0)
    {
        const int sel = selectionIdx - 1;
        if (sel < 0 || sel >= layer.selections.size())
            return false;
        const PTTransitionPreset effective =
                effectivePresetForSelectionNoLive(mode, row, outputIdx, sel);
        clip.kind = PTEfxTreeClipboardKind::SelectionLayer;
        clip.providerLayer = false;
        clip.classicSelection = layer.selections.at(sel);
        clip.classicSelection.overrides =
                classicOverrideFromEffective(item, effective);
        m_selectionClipboard.valid = true;
        m_selectionClipboard.name = clip.classicSelection.name;
        m_selectionClipboard.cells = clip.classicSelection.cells;
        m_selectionClipboard.overrides.values = clip.classicSelection.overrides.values;
        m_selectionClipboard.overrides.columns = clip.classicSelection.overrides.columns;
    }
    else
    {
        clip.kind = PTEfxTreeClipboardKind::OutputLayer;
        clip.providerLayer = false;
        clip.outputIndex = outputIdx;
        clip.classicOutputLayer = layer;
        clip.classicOutputLayer.all =
                classicOverrideFromEffective(item,
                                             effectivePresetForOutputNoLive(mode, row, outputIdx));
    }
    m_treeClipboard = clip;
    return true;
}

bool PresetTableV2TransitionWidget::canPasteTreeLayer(
        PTTransitionMode mode, QTreeWidgetItem* item) const
{
    if (!item || !m_treeClipboard.valid || bankIsSlaved(mode))
        return false;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    if (row < 0)
        return false;

    const bool isTopPreset = !item->parent();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;

    switch (m_treeClipboard.kind)
    {
    case PTEfxTreeClipboardKind::Preset:
        return isTopPreset && mode == m_treeClipboard.mode;
    case PTEfxTreeClipboardKind::MultiFxRoute:
        return mode == PTTransitionMode::MultiFx
                && (isTopPreset || (routeVar.isValid() && routeOutputIdx < 0));
    case PTEfxTreeClipboardKind::OutputLayer:
        if (mode == PTTransitionMode::MultiFx)
            return isTopPreset || routeVar.isValid();
        return isTopPreset || outputIdx >= 0;
    case PTEfxTreeClipboardKind::SelectionLayer:
        return canPasteSelectionLayer(mode, item);
    case PTEfxTreeClipboardKind::None:
        break;
    }
    return false;
}

bool PresetTableV2TransitionWidget::pasteTreeLayer(
        PTTransitionMode mode, QTreeWidgetItem* item)
{
    if (!canPasteTreeLayer(mode, item))
        return false;

    auto providerOverrideFromClassic = [](const PTTransitionPresetOverride& ov) {
        PTTransitionProviderPresetOverride out;
        out.values = ov.values;
        out.columns = ov.columns;
        return out;
    };
    auto classicOverrideFromProvider = [](const PTTransitionProviderPresetOverride& ov) {
        PTTransitionPresetOverride out;
        out.values = ov.values;
        out.columns = ov.columns;
        return out;
    };
    auto providerSelectionFromClassic = [&](const PTTransitionSelection& sel) {
        PTTransitionProviderSelection out;
        out.name = sel.name;
        out.cells = sel.cells;
        out.overrides = providerOverrideFromClassic(sel.overrides);
        return out;
    };
    auto classicSelectionFromProvider = [&](const PTTransitionProviderSelection& sel) {
        PTTransitionSelection out;
        out.name = sel.name;
        out.cells = sel.cells;
        out.overrides = classicOverrideFromProvider(sel.overrides);
        return out;
    };
    auto providerLayerFromClassic = [&](const PTTransitionOutputLayer& layer) {
        PTTransitionProviderOutputLayer out;
        out.all = providerOverrideFromClassic(layer.all);
        for (const PTTransitionSelection& sel : layer.selections)
            out.selections.append(providerSelectionFromClassic(sel));
        return out;
    };
    auto classicLayerFromProvider = [&](const PTTransitionProviderOutputLayer& layer) {
        PTTransitionOutputLayer out;
        out.all = classicOverrideFromProvider(layer.all);
        for (const PTTransitionProviderSelection& sel : layer.selections)
            out.selections.append(classicSelectionFromProvider(sel));
        return out;
    };
    auto filterProviderLayerCells = [](PTTransitionProviderOutputLayer layer,
                                       PresetTableV2ControlIface* tableIface,
                                       int outputIdx) {
        QSet<QLCPoint> scope;
        if (tableIface && outputIdx >= 0)
        {
            for (const QLCPoint& pt : tableIface->outputPointsForPresetOverride(outputIdx))
                scope.insert(pt);
        }
        for (PTTransitionProviderSelection& selection : layer.selections)
        {
            if (scope.isEmpty())
                continue;
            QVector<QLCPoint> cells;
            for (const QLCPoint& pt : selection.cells)
            {
                if (scope.contains(pt))
                    cells.append(pt);
            }
            selection.cells = cells;
        }
        return layer;
    };
    auto filterClassicLayerCells = [&](PTTransitionOutputLayer layer,
                                       PresetTableV2ControlIface* tableIface,
                                       int outputIdx) {
        return classicLayerFromProvider(filterProviderLayerCells(
                providerLayerFromClassic(layer), tableIface, outputIdx));
    };

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    const int routeIdx = routeVar.isValid() ? routeVar.toInt() : -1;
    const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
            ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;

    if (m_treeClipboard.kind == PTEfxTreeClipboardKind::SelectionLayer)
    {
        if (m_treeClipboard.providerLayer)
        {
            m_selectionClipboard.valid = true;
            m_selectionClipboard.name = m_treeClipboard.providerSelection.name;
            m_selectionClipboard.cells = m_treeClipboard.providerSelection.cells;
            m_selectionClipboard.overrides = m_treeClipboard.providerSelection.overrides;
        }
        else
        {
            m_selectionClipboard.valid = true;
            m_selectionClipboard.name = m_treeClipboard.classicSelection.name;
            m_selectionClipboard.cells = m_treeClipboard.classicSelection.cells;
            m_selectionClipboard.overrides.values = m_treeClipboard.classicSelection.overrides.values;
            m_selectionClipboard.overrides.columns = m_treeClipboard.classicSelection.overrides.columns;
        }
        pasteSelectionLayer(mode, item);
        return true;
    }

    if (m_treeClipboard.kind == PTEfxTreeClipboardKind::Preset)
    {
        QVector<PTTransitionPreset>& presets = presetsForMode(mode);
        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        if (row < 0 || row >= presets.size())
            return false;
        presets[row] = m_treeClipboard.preset;
        while (overrides.size() <= row)
            overrides.append(QHash<int, PTTransitionOutputLayer>());
        overrides[row] = m_treeClipboard.classicOutputLayers;
        if (mode == PTTransitionMode::MultiFx)
        {
            normalizeMultiFxTargetRoutes();
            while (m_multiFxTargetRoutes.size() <= row)
                m_multiFxTargetRoutes.append(QVector<PTMultiFxTargetTableRoute>());
            m_multiFxTargetRoutes[row] = m_treeClipboard.multiFxRoutes;
        }
    }
    else if (m_treeClipboard.kind == PTEfxTreeClipboardKind::MultiFxRoute)
    {
        if (mode != PTTransitionMode::MultiFx)
            return false;
        normalizeMultiFxTargetRoutes();
        while (m_multiFxTargetRoutes.size() <= row)
            m_multiFxTargetRoutes.append(QVector<PTMultiFxTargetTableRoute>());
        QVector<PTMultiFxTargetTableRoute>& routes = m_multiFxTargetRoutes[row];
        int dstRouteIdx = routeIdx;
        if (dstRouteIdx < 0)
        {
            for (int i = 0; i < routes.size(); ++i)
            {
                if (routes.at(i).tableId == m_treeClipboard.multiFxRoute.tableId)
                {
                    dstRouteIdx = i;
                    break;
                }
            }
        }
        if (dstRouteIdx >= 0 && dstRouteIdx < routes.size())
            routes[dstRouteIdx] = m_treeClipboard.multiFxRoute;
        else
            routes.append(m_treeClipboard.multiFxRoute);
    }
    else if (m_treeClipboard.kind == PTEfxTreeClipboardKind::OutputLayer)
    {
        if (mode == PTTransitionMode::MultiFx)
        {
            normalizeMultiFxTargetRoutes();
            if (row < 0 || row >= m_multiFxTargetRoutes.size())
                return false;
            QList<int> routeIndices;
            if (routeIdx >= 0)
                routeIndices.append(routeIdx);
            else
            {
                for (int i = 0; i < m_multiFxTargetRoutes.at(row).size(); ++i)
                    routeIndices.append(i);
            }
            for (int rIdx : routeIndices)
            {
                if (rIdx < 0 || rIdx >= m_multiFxTargetRoutes[row].size())
                    continue;
                PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][rIdx];
                PresetTableV2ControlIface* targetIface =
                        PresetTableV2VCLookup::controlIfaceByVcId(route.tableId);
                QList<int> outputs;
                if (routeOutputIdx >= 0)
                    outputs.append(routeOutputIdx);
                else
                    outputs = multiFxOutputIndicesForRoute(route);
                for (int outIdx : outputs)
                {
                    PTTransitionProviderOutputLayer layer = m_treeClipboard.providerLayer
                            ? m_treeClipboard.providerOutputLayer
                            : providerLayerFromClassic(m_treeClipboard.classicOutputLayer);
                    route.outputOverrides.insert(
                            outIdx, filterProviderLayerCells(layer, targetIface, outIdx));
                }
            }
        }
        else
        {
            QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
            while (overrides.size() <= row)
                overrides.append(QHash<int, PTTransitionOutputLayer>());
            PresetTableV2ControlIface* tableIface = linkedTable();
            QList<int> outputs;
            if (outputIdx >= 0)
                outputs.append(outputIdx);
            else
            {
                const int count = linkedOutputCount();
                for (int o = 0; o < count; ++o)
                    outputs.append(o);
            }
            for (int outIdx : outputs)
            {
                PTTransitionOutputLayer layer = m_treeClipboard.providerLayer
                        ? classicLayerFromProvider(m_treeClipboard.providerOutputLayer)
                        : m_treeClipboard.classicOutputLayer;
                overrides[row].insert(outIdx,
                                      filterClassicLayerCells(layer, tableIface, outIdx));
            }
        }
    }

    if (mode == PTTransitionMode::MultiFx)
        publishProviderSnapshot(QStringLiteral("multifx tree paste"));
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
    return true;
}

bool PresetTableV2TransitionWidget::duplicateTreeLayer(
        PTTransitionMode mode, QTreeWidgetItem* item)
{
    if (!item || bankIsSlaved(mode))
        return false;
    if (!item->parent())
    {
        QTreeWidget* table = tableForMode(mode);
        if (table)
            table->setCurrentItem(item, ColName);
        slotDuplicatePreset();
        return true;
    }

    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    if (selectionIdx <= 0)
        return false;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);
    auto uniqueName = [this](const QString& sourceName, auto existingNames) {
        const QString base = sourceName.trimmed().isEmpty()
                ? tr("Selection copy") : sourceName + tr(" copy");
        QString candidate = base;
        int suffix = 2;
        while (existingNames.contains(candidate))
            candidate = base + QStringLiteral(" %1").arg(suffix++);
        return candidate;
    };

    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        normalizeMultiFxTargetRoutes();
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        if (row < 0 || row >= m_multiFxTargetRoutes.size()
                || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size()
                || routeOutputIdx < 0)
            return false;
        PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
        PTTransitionProviderOutputLayer layer = route.outputOverrides.value(routeOutputIdx);
        const int sel = selectionIdx - 1;
        if (sel < 0 || sel >= layer.selections.size())
            return false;
        PTTransitionProviderSelection copy = layer.selections.at(sel);
        QSet<QString> names;
        for (const PTTransitionProviderSelection& selection : layer.selections)
            names.insert(selection.name);
        copy.name = uniqueName(copy.name, names);
        layer.selections.append(copy);
        route.outputOverrides.insert(routeOutputIdx, layer);
        publishProviderSnapshot(QStringLiteral("multifx duplicate selection"));
    }
    else
    {
        QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
        if (row < 0 || row >= overrides.size() || outputIdx < 0)
            return false;
        PTTransitionOutputLayer layer = overrides[row].value(outputIdx);
        const int sel = selectionIdx - 1;
        if (sel < 0 || sel >= layer.selections.size())
            return false;
        PTTransitionSelection copy = layer.selections.at(sel);
        QSet<QString> names;
        for (const PTTransitionSelection& selection : layer.selections)
            names.insert(selection.name);
        copy.name = uniqueName(copy.name, names);
        layer.selections.append(copy);
        overrides[row].insert(outputIdx, layer);
    }

    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
    return true;
}

bool PresetTableV2TransitionWidget::canPasteSelectionLayer(
        PTTransitionMode mode, QTreeWidgetItem* item) const
{
    if (!item || !m_selectionClipboard.valid || bankIsSlaved(mode))
        return false;
    const int row = item->data(0, kItemPresetIndexRole).toInt();
    if (row < 0)
        return false;
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    if (selectionIdx > 0)
        return true;
    if (outputIdx >= 0)
        return true;
    if (mode == PTTransitionMode::MultiFx
            && item->data(0, kItemMultiFxRouteIndexRole).isValid())
        return true;
    return item->childCount() > 0;
}

void PresetTableV2TransitionWidget::copySelectionLayer(QTreeWidgetItem* item)
{
    if (!canCopySelectionLayer(item))
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt() - 1;
    const PTTransitionMode mode = modeForTable(item->treeWidget());
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);

    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        const int routeIdx = routeVar.toInt();
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        const PTTransitionProviderOutputLayer layer =
                m_multiFxTargetRoutes.at(row).at(routeIdx).outputOverrides.value(routeOutputIdx);
        const PTTransitionProviderSelection selection = layer.selections.at(selectionIdx);
        m_selectionClipboard.valid = true;
        m_selectionClipboard.name = selection.name;
        m_selectionClipboard.cells = selection.cells;
        m_selectionClipboard.overrides = selection.overrides;
        return;
    }

    const PTTransitionOutputLayer layer = overridesForMode(mode).at(row).value(outputIdx);
    const PTTransitionSelection selection = layer.selections.at(selectionIdx);
    m_selectionClipboard.valid = true;
    m_selectionClipboard.name = selection.name;
    m_selectionClipboard.cells = selection.cells;
    m_selectionClipboard.overrides.values = selection.overrides.values;
    m_selectionClipboard.overrides.columns = selection.overrides.columns;
}

void PresetTableV2TransitionWidget::pasteSelectionLayer(
        PTTransitionMode mode, QTreeWidgetItem* item)
{
    if (!canPasteSelectionLayer(mode, item))
        return;

    auto filteredCells = [this](PresetTableV2ControlIface* tableIface, int outputIdx) {
        QSet<QLCPoint> scopeSet;
        if (tableIface && outputIdx >= 0)
        {
            const QList<QLCPoint> scopePoints =
                    tableIface->outputPointsForPresetOverride(outputIdx);
            for (const QLCPoint& pt : scopePoints)
                scopeSet.insert(pt);
        }

        QVector<QLCPoint> cells;
        QList<QLCPoint> sortedCells = m_selectionClipboard.cells;
        std::sort(sortedCells.begin(), sortedCells.end(), [](const QLCPoint& a, const QLCPoint& b) {
            return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
        });
        for (const QLCPoint& pt : sortedCells)
        {
            if (!scopeSet.isEmpty() && !scopeSet.contains(pt))
                continue;
            cells.append(pt);
        }
        return cells;
    };

    auto providerPasteIndex = [this](PTTransitionProviderOutputLayer& layer, int requested) {
        if (requested >= 0 && requested < layer.selections.size())
            return requested;
        for (int i = 0; i < layer.selections.size(); ++i)
        {
            if (!m_selectionClipboard.name.isEmpty()
                    && layer.selections.at(i).name == m_selectionClipboard.name)
                return i;
        }
        PTTransitionProviderSelection selection;
        selection.name = m_selectionClipboard.name.isEmpty()
                ? tr("Selection %1").arg(layer.selections.size() + 1)
                : m_selectionClipboard.name;
        layer.selections.append(selection);
        return int(layer.selections.size()) - 1;
    };

    auto classicPasteIndex = [this](PTTransitionOutputLayer& layer, int requested) {
        if (requested >= 0 && requested < layer.selections.size())
            return requested;
        for (int i = 0; i < layer.selections.size(); ++i)
        {
            if (!m_selectionClipboard.name.isEmpty()
                    && layer.selections.at(i).name == m_selectionClipboard.name)
                return i;
        }
        PTTransitionSelection selection;
        selection.name = m_selectionClipboard.name.isEmpty()
                ? tr("Selection %1").arg(layer.selections.size() + 1)
                : m_selectionClipboard.name;
        layer.selections.append(selection);
        return int(layer.selections.size()) - 1;
    };

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt() - 1;
    const QVariant routeVar = item->data(0, kItemMultiFxRouteIndexRole);

    if (mode == PTTransitionMode::MultiFx && routeVar.isValid())
    {
        normalizeMultiFxTargetRoutes();
        const int routeIdx = routeVar.toInt();
        if (row < 0 || row >= m_multiFxTargetRoutes.size()
                || routeIdx < 0 || routeIdx >= m_multiFxTargetRoutes.at(row).size())
            return;

        PTMultiFxTargetTableRoute& route = m_multiFxTargetRoutes[row][routeIdx];
        PresetTableV2ControlIface* tableIface =
                PresetTableV2VCLookup::controlIfaceByVcId(route.tableId);
        QList<int> outputs;
        const int routeOutputIdx = item->data(0, kItemMultiFxRouteOutputIndexRole).isValid()
                ? item->data(0, kItemMultiFxRouteOutputIndexRole).toInt() : outputIdx;
        if (routeOutputIdx >= 0)
            outputs.append(routeOutputIdx);
        else
        {
            const int count = targetTableOutputCount(route.tableId);
            for (int o = 0; o < count; ++o)
                outputs.append(o);
            if (outputs.isEmpty())
            {
                for (const PTMultiFxTargetOutputRoute& out : route.outputs)
                    outputs.append(out.outputIndex);
            }
        }

        for (int outIdx : outputs)
        {
            if (outIdx < 0)
                continue;
            bool knownOutput = false;
            for (const PTMultiFxTargetOutputRoute& out : route.outputs)
            {
                if (out.outputIndex == outIdx)
                {
                    knownOutput = true;
                    break;
                }
            }
            if (!knownOutput)
            {
                PTMultiFxTargetOutputRoute out;
                out.outputIndex = outIdx;
                route.outputs.append(out);
            }

            PTTransitionProviderOutputLayer layer = route.outputOverrides.value(outIdx);
            const int pasteIdx = providerPasteIndex(layer, routeOutputIdx >= 0 ? selectionIdx : -1);
            layer.selections[pasteIdx].name = m_selectionClipboard.name.isEmpty()
                    ? tr("Selection %1").arg(pasteIdx + 1) : m_selectionClipboard.name;
            layer.selections[pasteIdx].cells = filteredCells(tableIface, outIdx);
            layer.selections[pasteIdx].overrides = m_selectionClipboard.overrides;
            route.outputOverrides.insert(outIdx, layer);
        }

        publishProviderSnapshot(QStringLiteral("multifx selection paste"));
        rebuildPresetTable(mode);
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
        if (m_doc)
            m_doc->setModified();
        return;
    }

    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    while (overrides.size() <= row)
        overrides.append(QHash<int, PTTransitionOutputLayer>());

    QList<int> outputs;
    if (outputIdx >= 0)
        outputs.append(outputIdx);
    else
    {
        const int count = linkedOutputCount();
        for (int o = 0; o < count; ++o)
            outputs.append(o);
    }
    PresetTableV2ControlIface* tableIface = linkedTable();
    for (int outIdx : outputs)
    {
        if (outIdx < 0)
            continue;
        PTTransitionOutputLayer layer = overrides[row].value(outIdx);
        const int pasteIdx = classicPasteIndex(layer, outputIdx >= 0 ? selectionIdx : -1);
        layer.selections[pasteIdx].name = m_selectionClipboard.name.isEmpty()
                ? tr("Selection %1").arg(pasteIdx + 1) : m_selectionClipboard.name;
        layer.selections[pasteIdx].cells = filteredCells(tableIface, outIdx);
        layer.selections[pasteIdx].overrides.values = m_selectionClipboard.overrides.values;
        layer.selections[pasteIdx].overrides.columns = m_selectionClipboard.overrides.columns;
        overrides[row].insert(outIdx, layer);
    }

    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateEffectPreview();
    if (m_doc)
        m_doc->setModified();
}

void PresetTableV2TransitionWidget::pasteCells(QTreeWidget* table)
{
    if (!table)
        return;
    if (bankIsSlaved(modeForTable(table)))
        return;

    const QString text = QApplication::clipboard()->text().trimmed();
    if (text.isEmpty())
        return;

    QList<PTTransitionCellKey> cells = selectedCellsInVisualOrder(table);
    int focusCol = -1;
    if (!cells.isEmpty())
        focusCol = cells.first().col;
    else
        focusCol = focusColumn(table);

    if (focusCol <= ColName || focusCol >= ColCount || table->isColumnHidden(focusCol))
        return;

    for (const PTTransitionCellKey& key : cells)
    {
        if (key.col != focusCol)
            return;
    }

    const int clipCol = clipboardColumnForPaste();
    if (clipCol >= 0 && clipCol != focusCol)
        return;

    if (cells.isEmpty())
    {
        const QList<QTreeWidgetItem*> items = selectedRowsInVisualOrder(table);
        if (items.isEmpty())
            return;
        for (QTreeWidgetItem* item : items)
            cells.append(PTTransitionCellKey { item, focusCol });
    }

    const PTTransitionMode mode = modeForTable(table);
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")),
                                         Qt::SkipEmptyParts);
    const bool singleValue = (lines.size() == 1 && !lines.at(0).contains(QLatin1Char('\t')));

    struct PasteOp
    {
        PTTransitionCellAddress address;
        QString raw;
    };
    QVector<PasteOp> ops;

    ScopedBoolFlag pasteGuard(m_pastingCells);
    if (singleValue)
    {
        const QString raw = lines.first().trimmed();
        for (const PTTransitionCellKey& key : cells)
        {
            if (!key.item)
                continue;
            PTTransitionCellAddress address;
            address.row = key.item->data(0, kItemPresetIndexRole).toInt();
            address.outputIdx = key.item->data(0, kItemOutputIndexRole).toInt();
            address.selectionIdx = key.item->data(0, kItemSelectionIndexRole).toInt();
            address.col = key.col;
            ops.append({ address, raw });
        }
    }
    else if (cells.size() > 1)
    {
        const int count = qMin(lines.size(), cells.size());
        for (int i = 0; i < count; ++i)
        {
            const QString raw = lines.at(i).section(QLatin1Char('\t'), 0, 0).trimmed();
            QTreeWidgetItem* item = cells.at(i).item;
            if (!item)
                continue;
            PTTransitionCellAddress address;
            address.row = item->data(0, kItemPresetIndexRole).toInt();
            address.outputIdx = item->data(0, kItemOutputIndexRole).toInt();
            address.selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
            address.col = cells.at(i).col;
            ops.append({ address, raw });
        }
    }
    else
    {
        QTreeWidgetItem* item = cells.first().item;
        for (const QString& line : lines)
        {
            if (!item)
                break;
            const QString raw = line.section(QLatin1Char('\t'), 0, 0).trimmed();
            PTTransitionCellAddress address;
            address.row = item->data(0, kItemPresetIndexRole).toInt();
            address.outputIdx = item->data(0, kItemOutputIndexRole).toInt();
            address.selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
            address.col = focusCol;
            ops.append({ address, raw });
            item = table->itemBelow(item);
        }
    }

    QSet<int> touchedRows;
    for (const PasteOp& op : ops)
        applyPastedCellValue(mode, op.address, op.raw, touchedRows);

    for (int row : touchedRows)
        normalizeNoopOverridesForPreset(mode, row);

    for (int row : touchedRows)
        refreshOverrideVisualsForPreset(mode, row);

    if (!ops.isEmpty())
    {
        QTreeWidgetItem* first = itemForAddress(mode, ops.first().address);
        if (first)
            table->setCurrentItem(first, ops.first().address.col);
    }

    updateCellSelectionVisuals(table);

    if (!ops.isEmpty())
    {
        notifyTablePresetCacheRefresh();
        updateEffectPreview();
    }
    if (m_doc && !ops.isEmpty())
        m_doc->setModified();
}

static QWidget* editorWidgetForEvent(QObject* watched)
{
    QWidget* widget = qobject_cast<QWidget*>(watched);
    while (widget)
    {
        if (qobject_cast<QComboBox*>(widget) || qobject_cast<QSpinBox*>(widget))
            return widget;
        widget = widget->parentWidget();
    }
    return nullptr;
}

bool PresetTableV2TransitionWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        if (QWidget* editor = editorWidgetForEvent(watched))
        {
            for (QTreeWidget* table : { m_sweepTable, m_continuousTable,
                                        m_positionMotionTable, m_channel1DTable,
                                        m_multiFxTable })
            {
                if (!table || !table->isAncestorOf(editor))
                    continue;
                QTreeWidgetItem* item = nullptr;
                int col = -1;
                if (findItemForEditor(table, editor, &item, &col))
                    activateCellForClipboard(table, item, col, mouse->modifiers());
                break;
            }
        }
        else if (mouse->button() == Qt::LeftButton)
        {
            for (QTreeWidget* table : { m_sweepTable, m_continuousTable,
                                        m_positionMotionTable, m_channel1DTable,
                                        m_multiFxTable })
            {
                if (!table || watched != table->viewport())
                    continue;
                QTreeWidgetItem* item = table->itemAt(mouse->pos());
                const int col = table->columnAt(mouse->pos().x());
                if (item && col > ColName && col < ColCount)
                    activateCellForClipboard(table, item, col, mouse->modifiers());
                break;
            }
        }
    }

    if (event->type() == QEvent::ContextMenu)
    {
        if (QWidget* editor = editorWidgetForEvent(watched))
        {
            for (QTreeWidget* table : { m_sweepTable, m_continuousTable,
                                        m_positionMotionTable, m_channel1DTable,
                                        m_multiFxTable })
            {
                if (!table || !table->isAncestorOf(editor))
                    continue;
                QTreeWidgetItem* item = nullptr;
                int col = -1;
                if (!findItemForEditor(table, editor, &item, &col))
                    break;

                PTTransitionMode mode = modeForTable(table);
                QContextMenuEvent* ctx = static_cast<QContextMenuEvent*>(event);
                showParameterContextMenu(mode, table, item, col, ctx->globalPos());
                return true;
            }
        }
    }

    if (event->type() == QEvent::FocusIn)
    {
        if (QWidget* editor = editorWidgetForEvent(watched))
        {
            for (QTreeWidget* table : { m_sweepTable, m_continuousTable,
                                        m_positionMotionTable, m_channel1DTable,
                                        m_multiFxTable })
            {
                if (!table || !table->isAncestorOf(editor))
                    continue;
                QTreeWidgetItem* item = nullptr;
                int col = -1;
                if (findItemForEditor(table, editor, &item, &col))
                {
                    setFocusColumn(table, item, col);
                    const PTTransitionCellKey key { item, col };
                    QSet<PTTransitionCellKey>& selected = m_selectedCellsByTable[table];
                    if (!selected.contains(key))
                    {
                        selected.insert(key);
                        m_cellSelectionAnchorByTable.insert(table, key);
                        updateCellSelectionVisuals(table);
                    }
                }
                break;
            }
        }
    }

    if (event->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if (tableFromFocusObject(watched)
                && (key->matches(QKeySequence::Copy) || key->matches(QKeySequence::Paste)))
        {
            key->accept();
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if (QTreeWidget* table = tableFromFocusObject(watched))
        {
            const QVector<QTreeView*> frozenViews = {
                m_sweepNameView, m_continuousNameView, m_channel1DNameView,
                m_positionMotionNameView, m_multiFxNameView
            };
            bool frozenNameFocus = false;
            for (QTreeView* frozen : frozenViews)
            {
                if (frozen && (watched == frozen || watched == frozen->viewport()))
                {
                    frozenNameFocus = true;
                    break;
                }
            }
            if (frozenNameFocus)
            {
                QTreeWidgetItem* item = selectedPresetItem(table);
                const PTTransitionMode mode = modeForTable(table);
                if (key->matches(QKeySequence::Copy) && item)
                {
                    copyTreeLayer(item);
                    return true;
                }
                if (key->matches(QKeySequence::Paste) && canPasteTreeLayer(mode, item))
                {
                    pasteTreeLayer(mode, item);
                    return true;
                }
            }
            if (key->matches(QKeySequence::Copy))
            {
                copyCells(table);
                return true;
            }
            if (key->matches(QKeySequence::Paste))
            {
                pasteCells(table);
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

void PresetTableV2TransitionWidget::publishProviderSnapshot(const QString& reason)
{
    PTTransitionProviderSnapshot snapshot;
    snapshot.sweepPresets = m_sweepPresets;
    snapshot.continuousPresets = m_continuousPresets;
    snapshot.positionMotionPresets = m_positionMotionPresets;
    snapshot.channel1DPresets = m_channel1DPresets;
    snapshot.multiFxPresets = m_multiFxPresets;
    normalizeMultiFxTargetRoutes();
    snapshot.multiFxTargetRoutes = m_multiFxTargetRoutes;
    snapshot.sweepOutputOverrides = m_sweepOutputOverrides;
    snapshot.continuousOutputOverrides = m_continuousOutputOverrides;
    snapshot.positionMotionOutputOverrides = m_positionMotionOutputOverrides;
    snapshot.channel1DOutputOverrides = m_channel1DOutputOverrides;
    snapshot.multiFxOutputOverrides = m_multiFxOutputOverrides;
    snapshot.globalSettings = m_globalSettings;
    snapshot.activeMode = activeBankMode();
    snapshot.enabled = !m_enableChk || m_enableChk->isChecked();

    auto applySourceBank = [&](PTTransitionMode mode) {
        if (!bankIsSlaved(mode))
            return;
        ::PTTransitionProviderSnapshot source;
        const bool ok = effectiveProviderSnapshotForBank(mode, &source);
        QVector<PTTransitionPreset>* presets = nullptr;
        QVector<QHash<int, PTTransitionOutputLayer>>* overrides = nullptr;
        const QVector<PTTransitionPreset>* sourcePresets = nullptr;
        switch (mode)
        {
            case PTTransitionMode::Off:
                break;
            case PTTransitionMode::SweepOnly:
                presets = &snapshot.sweepPresets;
                overrides = &snapshot.sweepOutputOverrides;
                sourcePresets = &source.sweepPresets;
                break;
            case PTTransitionMode::Continuous:
                presets = &snapshot.continuousPresets;
                overrides = &snapshot.continuousOutputOverrides;
                sourcePresets = &source.continuousPresets;
                break;
            case PTTransitionMode::Channel1D:
                presets = &snapshot.channel1DPresets;
                overrides = &snapshot.channel1DOutputOverrides;
                sourcePresets = &source.channel1DPresets;
                break;
            case PTTransitionMode::PositionMotion:
                presets = &snapshot.positionMotionPresets;
                overrides = &snapshot.positionMotionOutputOverrides;
                sourcePresets = &source.positionMotionPresets;
                break;
            case PTTransitionMode::MultiFx:
                presets = &snapshot.multiFxPresets;
                overrides = &snapshot.multiFxOutputOverrides;
                sourcePresets = &source.multiFxPresets;
                if (ok)
                    snapshot.multiFxTargetRoutes = source.multiFxTargetRoutes;
                break;
        }
        if (!presets || !overrides)
            return;
        *presets = ok && sourcePresets ? *sourcePresets : QVector<PTTransitionPreset>();
        overrides->clear();
    };
    for (PTTransitionMode mode : { PTTransitionMode::SweepOnly,
                                   PTTransitionMode::Continuous,
                                   PTTransitionMode::Channel1D,
                                   PTTransitionMode::PositionMotion,
                                   PTTransitionMode::MultiFx })
        applySourceBank(mode);

    for (PTTransitionPreset& preset : snapshot.sweepPresets)
        preset = PTDimmerWaveEngine::normalizedTransitionSweepPreset(preset);

    const auto crossfadeSrc = inputSource(PTEfxCol::InputCrossfadeManual);
    snapshot.crossfadeManualControl = !(crossfadeSrc && crossfadeSrc->isValid())
            || m_crossfadeManualControl;

    {
        QMutexLocker lk(&m_liveMutex);
        snapshot.liveColumnOverrides = m_liveColumnOverrides;
    }

    if (PresetTableV2ControlIface* table = linkedTable())
    {
        PTTransitionPreset p;
        p.axis = PTTransitionAxis::X;
        snapshot.spanX = table->fixtureGroupSpanAlongAxis(p, snapshot.globalSettings);
        p.axis = PTTransitionAxis::Y;
        snapshot.spanY = table->fixtureGroupSpanAlongAxis(p, snapshot.globalSettings);
        p.axis = PTTransitionAxis::XY;
        snapshot.spanXY = table->fixtureGroupSpanAlongAxis(p, snapshot.globalSettings);
    }

    {
        QMutexLocker lk(&m_providerSnapshotMutex);
        snapshot.revision = ++m_providerSnapshotRevision;
        m_providerSnapshot = snapshot;
    }

    const QString message =
            QStringLiteral("provider snapshot publish revision=%1 reason=%2 sweep=%3 interpolation=%4 channel1d=%5 motion=%6 multifx=%7 live=%8")
                    .arg(snapshot.revision)
                    .arg(reason)
                    .arg(snapshot.sweepPresets.size())
                    .arg(snapshot.continuousPresets.size())
                    .arg(snapshot.channel1DPresets.size())
                    .arg(snapshot.positionMotionPresets.size())
                    .arg(snapshot.multiFxPresets.size())
                    .arg(snapshot.liveColumnOverrides.size());
    if (reason == QStringLiteral("cache refresh"))
    {
        VCPluginDiagnostics::breadcrumbRateLimited(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("presettablev2transition/provider-cache-refresh/%1").arg(id()),
                250, message);
    }
    else
    {
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), id(), caption(), message);
    }

    if (!reason.startsWith(QStringLiteral("source refresh")))
        notifySlaveEnginesOfSnapshotChange(reason);
}

PresetTableV2TransitionWidget::PTTransitionProviderSnapshot
PresetTableV2TransitionWidget::providerSnapshotCopy() const
{
    QMutexLocker lk(&m_providerSnapshotMutex);
    return m_providerSnapshot;
}

const QVector<PTTransitionPreset>& PresetTableV2TransitionWidget::snapshotPresetsForMode(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return snapshot.multiFxPresets;
    if (mode == PTTransitionMode::PositionMotion)
        return snapshot.positionMotionPresets;
    if (mode == PTTransitionMode::Channel1D)
        return snapshot.channel1DPresets;
    if (mode == PTTransitionMode::Continuous)
        return snapshot.continuousPresets;
    if (mode == PTTransitionMode::SweepOnly)
        return snapshot.sweepPresets;
    static const QVector<PTTransitionPreset> empty;
    return empty;
}

const QVector<QHash<int, PresetTableV2TransitionWidget::PTTransitionOutputLayer>>&
PresetTableV2TransitionWidget::snapshotOverridesForMode(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode) const
{
    if (mode == PTTransitionMode::MultiFx)
        return snapshot.multiFxOutputOverrides;
    if (mode == PTTransitionMode::PositionMotion)
        return snapshot.positionMotionOutputOverrides;
    if (mode == PTTransitionMode::Channel1D)
        return snapshot.channel1DOutputOverrides;
    if (mode == PTTransitionMode::Continuous)
        return snapshot.continuousOutputOverrides;
    if (mode == PTTransitionMode::SweepOnly)
        return snapshot.sweepOutputOverrides;
    static const QVector<QHash<int, PTTransitionOutputLayer>> empty;
    return empty;
}

PTTransitionPreset PresetTableV2TransitionWidget::snapshotTransitionPreset(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode, int index) const
{
    const QVector<PTTransitionPreset>& bank = snapshotPresetsForMode(snapshot, mode);
    if (index < 0 || index >= bank.size())
    {
        if (index >= 0)
        {
            VCPluginDiagnostics::breadcrumb(
                    QStringLiteral("presettablev2transition"), id(), caption(),
                    QStringLiteral("provider snapshot invalid preset revision=%1 mode=%2 index=%3 count=%4")
                            .arg(snapshot.revision)
                            .arg(int(mode))
                            .arg(index)
                            .arg(bank.size()));
        }
        return PTTransitionPreset();
    }
    return bank.at(index);
}

void PresetTableV2TransitionWidget::applyOverrideColumnsToPreset(
        PTTransitionPreset& preset, const PTTransitionPresetOverride& ov) const
{
    for (int col : ov.columns)
    {
        setPresetColumnValue(preset, col, presetColumnValue(ov.values, col));
        if (col == ColWaveShape)
        {
            preset.customCurveEnabled = ov.values.customCurveEnabled;
            preset.customCurve = ov.values.customCurve;
            preset.waveShape = ov.values.waveShape;
        }
    }
}

int PresetTableV2TransitionWidget::snapshotGridSpanForPreset(
        const PTTransitionProviderSnapshot& snapshot, const PTTransitionPreset& preset) const
{
    if (preset.axis == PTTransitionAxis::Y)
        return snapshot.spanY;
    if (preset.axis == PTTransitionAxis::XY)
        return snapshot.spanXY;
    return snapshot.spanX;
}

PTTransitionPreset PresetTableV2TransitionWidget::finalizeSnapshotPreset(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode,
        const PTTransitionPreset& preset) const
{
    PTTransitionPreset p = preset;
    p.playbackMode = (mode == PTTransitionMode::Continuous
                      || mode == PTTransitionMode::PositionMotion
                      || mode == PTTransitionMode::Channel1D
                      || mode == PTTransitionMode::MultiFx)
            ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    PTDimmerWaveEngine::clampOffsetStep(p, snapshotGridSpanForPreset(snapshot, p));
    return p;
}

PTTransitionPreset PresetTableV2TransitionWidget::snapshotEffectivePresetForOutput(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode, int row,
        int outputIdx, bool applyLive) const
{
    PTTransitionPreset p = snapshotTransitionPreset(snapshot, mode, row);
    const QVector<QHash<int, PTTransitionOutputLayer>>& overrides =
            snapshotOverridesForMode(snapshot, mode);
    if (outputIdx >= 0 && row >= 0 && row < overrides.size())
    {
        const QHash<int, PTTransitionOutputLayer>& rowOverrides = overrides.at(row);
        if (rowOverrides.contains(outputIdx))
            applyOverrideColumnsToPreset(p, rowOverrides.value(outputIdx).all);
    }

    if (applyLive)
        p = PresetTableV2SpatialEngine::mergePreset(p, snapshot.liveColumnOverrides);
    return finalizeSnapshotPreset(snapshot, mode, p);
}

PTTransitionPreset PresetTableV2TransitionWidget::snapshotEffectivePresetForSelection(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode, int row,
        int outputIdx, int selectionIdx, bool applyLive) const
{
    PTTransitionPreset p = snapshotEffectivePresetForOutput(snapshot, mode, row, outputIdx, false);
    const QVector<QHash<int, PTTransitionOutputLayer>>& overrides =
            snapshotOverridesForMode(snapshot, mode);
    if (outputIdx >= 0 && row >= 0 && row < overrides.size())
    {
        const QHash<int, PTTransitionOutputLayer>& rowOverrides = overrides.at(row);
        if (rowOverrides.contains(outputIdx))
        {
            const PTTransitionOutputLayer layer = rowOverrides.value(outputIdx);
            if (selectionIdx >= 0 && selectionIdx < layer.selections.size())
                applyOverrideColumnsToPreset(p, layer.selections.at(selectionIdx).overrides);
        }
    }

    if (applyLive)
        p = PresetTableV2SpatialEngine::mergePreset(p, snapshot.liveColumnOverrides);
    return finalizeSnapshotPreset(snapshot, mode, p);
}

int PresetTableV2TransitionWidget::snapshotSelectionIndexForPoint(
        const PTTransitionProviderSnapshot& snapshot, PTTransitionMode mode, int row,
        int outputIdx, const QLCPoint& point) const
{
    if (outputIdx < 0)
        return -1;
    const QVector<QHash<int, PTTransitionOutputLayer>>& overrides =
            snapshotOverridesForMode(snapshot, mode);
    if (row < 0 || row >= overrides.size())
        return -1;
    const QHash<int, PTTransitionOutputLayer>& rowOverrides = overrides.at(row);
    if (!rowOverrides.contains(outputIdx))
        return -1;
    const PTTransitionOutputLayer layer = rowOverrides.value(outputIdx);
    for (int i = 0; i < layer.selections.size(); ++i)
    {
        if (layer.selections.at(i).cells.contains(point))
            return i;
    }
    return -1;
}

void PresetTableV2TransitionWidget::notifyTablePresetCacheRefresh()
{
    publishProviderSnapshot(QStringLiteral("cache refresh"));

    if (m_committingCustomDialog || m_committingDelegateEditor
            || m_closingTableEditors || m_rebuildingTable)
    {
        scheduleDeferredPresetCacheRefreshAndPreview();
        return;
    }

    QSet<quint32> refreshedTables;
    if (PresetTableV2ControlIface* table = linkedTable())
    {
        table->refreshTransitionPresetCache();
        if (m_targetTableId != VCWidget::invalidId())
            refreshedTables.insert(m_targetTableId);
    }

    for (const QVector<PTMultiFxTargetTableRoute>& routes : std::as_const(m_multiFxTargetRoutes))
    {
        for (const PTMultiFxTargetTableRoute& route : routes)
        {
            if (!route.enabled || route.tableId == VCWidget::invalidId()
                    || refreshedTables.contains(route.tableId))
                continue;
            if (PresetTableV2ControlIface* table =
                    PresetTableV2VCLookup::controlIfaceByVcId(route.tableId))
            {
                table->refreshTransitionPresetCache();
                refreshedTables.insert(route.tableId);
            }
        }
    }
}

void PresetTableV2TransitionWidget::scheduleDeferredPresetCacheRefreshAndPreview()
{
    publishProviderSnapshot(QStringLiteral("deferred refresh"));

    if (m_deferredTransitionUiRefreshPending)
        return;
    m_deferredTransitionUiRefreshPending = true;

    QPointer<PresetTableV2TransitionWidget> self(this);
    QTimer::singleShot(0, this, [self]() {
        if (!self)
            return;
        self->m_deferredTransitionUiRefreshPending = false;
        if (self->m_rebuildingTable || self->m_committingDelegateEditor
                || self->m_closingTableEditors)
        {
            self->scheduleDeferredPresetCacheRefreshAndPreview();
            return;
        }
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), self->id(), self->caption(),
                QStringLiteral("deferred transition ui refresh run"));
        self->notifyTablePresetCacheRefresh();
        self->updateEffectPreview();
    });
}

void PresetTableV2TransitionWidget::notifySlaveEnginesOfSnapshotChange(const QString& reason)
{
    for (VCWidget* widget : PresetTableV2VCLookup::allTransitionWidgets())
    {
        auto* slave = qobject_cast<PresetTableV2TransitionWidget*>(widget);
        if (!slave || slave == this)
            continue;

        bool dependsOnThis = false;
        for (PTTransitionMode mode : { PTTransitionMode::SweepOnly,
                                       PTTransitionMode::Continuous,
                                       PTTransitionMode::Channel1D,
                                       PTTransitionMode::PositionMotion,
                                       PTTransitionMode::MultiFx })
        {
            if (slave->bankSourceEngineId(mode) == id())
            {
                dependsOnThis = true;
                break;
            }
        }
        if (!dependsOnThis)
            continue;

        QPointer<PresetTableV2TransitionWidget> target(slave);
        QTimer::singleShot(0, slave, [target, reason]() {
            if (!target)
                return;
            target->publishProviderSnapshot(QStringLiteral("source refresh: %1").arg(reason));
            if (PresetTableV2ControlIface* table = target->linkedTable())
                table->refreshTransitionPresetCache();
            target->rebuildAllPresetTables();
            target->updateEffectPreview();
        });
    }
}

void PresetTableV2TransitionWidget::scheduleDeferredTableLinkRefresh(int attemptsLeft)
{
    if (attemptsLeft <= 0)
        return;

    QTimer::singleShot(attemptsLeft == 6 ? 0 : 50, this, [this, attemptsLeft]() {
        slotRefreshTableLink();
        if (!linkedTable() && m_targetTableId != VCWidget::invalidId())
            scheduleDeferredTableLinkRefresh(attemptsLeft - 1);
    });
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

quint32 PresetTableV2TransitionWidget::bankSourceEngineId(PTTransitionMode mode) const
{
    return m_bankSourceEngineIds.value(int(mode), VCWidget::invalidId());
}

void PresetTableV2TransitionWidget::setBankSourceEngineId(PTTransitionMode mode,
                                                          quint32 engineId)
{
    if (engineId == id() || bankSourceWouldCreateCycle(mode, engineId))
        engineId = VCWidget::invalidId();
    if (engineId == VCWidget::invalidId())
        m_bankSourceEngineIds.remove(int(mode));
    else
        m_bankSourceEngineIds.insert(int(mode), engineId);
    updateBankSourceUi(mode);
}

bool PresetTableV2TransitionWidget::bankIsSlaved(PTTransitionMode mode) const
{
    return bankSourceEngineId(mode) != VCWidget::invalidId();
}

bool PresetTableV2TransitionWidget::bankSourceWouldCreateCycle(
        PTTransitionMode mode, quint32 sourceEngineId) const
{
    if (sourceEngineId == VCWidget::invalidId())
        return false;
    if (sourceEngineId == id())
        return true;

    QSet<quint32> visited;
    quint32 current = sourceEngineId;
    while (current != VCWidget::invalidId())
    {
        if (current == id())
            return true;
        if (visited.contains(current))
            return true;
        visited.insert(current);

        PresetTableV2TransitionWidget* source = nullptr;
        for (VCWidget* widget : PresetTableV2VCLookup::allTransitionWidgets())
        {
            if (widget && widget->id() == current)
            {
                source = qobject_cast<PresetTableV2TransitionWidget*>(widget);
                break;
            }
        }
        if (!source)
            return false;
        current = source->bankSourceEngineId(mode);
    }
    return false;
}

bool PresetTableV2TransitionWidget::effectiveProviderSnapshotForBank(
        PTTransitionMode mode, ::PTTransitionProviderSnapshot* snapshot) const
{
    if (!snapshot)
        return false;
    const quint32 sourceId = bankSourceEngineId(mode);
    if (sourceId == VCWidget::invalidId() || bankSourceWouldCreateCycle(mode, sourceId))
        return false;
    PresetTableV2TransitionProviderIface* provider =
            PresetTableV2VCLookup::transitionProviderByVcId(sourceId);
    if (!provider)
        return false;
    *snapshot = provider->transitionProviderSnapshot();
    return true;
}

QString PresetTableV2TransitionWidget::bankSourceDescription(PTTransitionMode mode) const
{
    const quint32 sourceId = bankSourceEngineId(mode);
    if (sourceId == VCWidget::invalidId())
        return QString();
    if (bankSourceWouldCreateCycle(mode, sourceId))
        return tr("Slave source cycle blocked for engine #%1").arg(sourceId);
    for (VCWidget* widget : PresetTableV2VCLookup::allTransitionWidgets())
    {
        if (widget && widget->id() == sourceId)
        {
            return tr("Slave: %1 / %2")
                    .arg(PresetTableV2VCLookup::vcWidgetLabel(widget))
                    .arg(transitionBankTitle(mode));
        }
    }
    return tr("Source engine #%1 not found").arg(sourceId);
}

void PresetTableV2TransitionWidget::updateBankSourceUi(PTTransitionMode bankMode)
{
    QLabel* label = m_bankSourceLabels.value(int(bankMode), nullptr);
    const QString text = bankSourceDescription(bankMode);
    if (label)
    {
        label->setText(text);
        label->setVisible(!text.isEmpty());
    }
    if (QTreeWidget* table = tableForMode(bankMode))
    {
        const bool slaved = bankIsSlaved(bankMode);
        table->setEditTriggers(slaved ? QAbstractItemView::NoEditTriggers
                                      : (QAbstractItemView::DoubleClicked
                                         | QAbstractItemView::EditKeyPressed));
        table->setProperty("ptBankSlaved", slaved);
        table->viewport()->setProperty("ptBankSlaved", slaved);
    }
    if (QTreeView* frozen = frozenNameViewForMode(bankMode))
    {
        const bool slaved = bankIsSlaved(bankMode);
        frozen->setEditTriggers(slaved ? QAbstractItemView::NoEditTriggers
                                       : (QAbstractItemView::DoubleClicked
                                          | QAbstractItemView::EditKeyPressed));
    }
}

PresetTableV2ControlIface* PresetTableV2TransitionWidget::linkedTable() const
{
    if (m_disableLiveLinkedTableLookup)
        return nullptr;
    return PresetTableV2VCLookup::controlIfaceByVcId(m_targetTableId);
}

void PresetTableV2TransitionWidget::slotRefreshTableLink()
{
    const bool hasSavedTarget = m_targetTableId != VCWidget::invalidId();
    PresetTableV2ControlIface* table = hasSavedTarget ? linkedTable() : nullptr;
    if (!table && !hasSavedTarget)
    {
        for (VCWidget* candidate : PresetTableV2VCLookup::allVcWidgets())
        {
            auto* candidateTable = qobject_cast<PresetTableV2ControlIface*>(candidate);
            if (candidateTable && candidateTable->linkedTransitionWidgetId() == id())
            {
                m_targetTableId = candidate->id();
                table = candidateTable;
                VCPluginDiagnostics::breadcrumb(
                        QStringLiteral("presettablev2transition"), id(), caption(),
                        QStringLiteral("engine auto-linked fresh table=%1")
                                .arg(m_targetTableId));
                break;
            }
        }
    }
    else if (!table && hasSavedTarget)
    {
        for (VCWidget* candidate : PresetTableV2VCLookup::allVcWidgets())
        {
            auto* candidateTable = qobject_cast<PresetTableV2ControlIface*>(candidate);
            if (candidateTable && candidateTable->linkedTransitionWidgetId() == id())
            {
                VCPluginDiagnostics::breadcrumb(
                        QStringLiteral("presettablev2transition"), id(), caption(),
                        QStringLiteral("engine conflicting table ignored saved=%1 candidate=%2")
                                .arg(m_targetTableId).arg(candidate->id()));
                break;
            }
        }
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("engine target missing preserved table=%1")
                        .arg(m_targetTableId));
    }

    if (table)
    {
        VCWidget* w = nullptr;
        for (VCWidget* candidate : PresetTableV2VCLookup::allVcWidgets())
        {
            if (candidate && candidate->id() == m_targetTableId)
            {
                w = candidate;
                break;
            }
        }
        QString linkText = tr("Linked: %1").arg(w ? PresetTableV2VCLookup::vcWidgetLabel(w)
                                                    : QString::number(m_targetTableId));
        VCPluginDiagnostics::breadcrumb(
                QStringLiteral("presettablev2transition"), id(), caption(),
                QStringLiteral("engine link resolved table=%1").arg(m_targetTableId));
        if (w && isDefaultPresetTableEngineCaption(caption()))
            setCaption(generatedPresetTableEngineCaption(w->caption()));
        if (table->tableUsesPositionMode())
        {
            linkText += tr(" — Position Mode: morph uses dimmer wave; orbit uses Motion column");
            for (PTTransitionPreset& p : m_sweepPresets)
                migrateLegacyPositionOrbitFields(p);
            for (PTTransitionPreset& p : m_continuousPresets)
                migrateLegacyPositionOrbitFields(p);
            for (PTTransitionPreset& p : m_positionMotionPresets)
                migrateLegacyPositionOrbitFields(p);
            for (PTTransitionPreset& p : m_multiFxPresets)
                migrateLegacyPositionOrbitFields(p);
        }
        if (m_bankTabs)
        {
            const bool positionMode = table->tableUsesPositionMode();
            m_bankTabs->setTabText(1, tr("Interpolation"));
            if (m_bankTabs->count() > 2)
                m_bankTabs->setTabVisible(2, !positionMode);
            if (m_bankTabs->count() > 3)
                m_bankTabs->setTabVisible(3, positionMode);
            if (!positionMode && activeBankMode() == PTTransitionMode::PositionMotion)
                m_bankTabs->setCurrentIndex(1);
            if (positionMode && activeBankMode() == PTTransitionMode::Channel1D)
                m_bankTabs->setCurrentIndex(1);
        }
        m_linkLabel->setText(linkText);

        if (table->linkedTransitionWidgetId() != id())
            table->setLinkedTransitionWidgetId(id());
        else
        {
            publishProviderSnapshot(QStringLiteral("table link"));
            table->refreshTransitionPresetCache();
        }

        m_enableChk->blockSignals(true);
        m_enableChk->setChecked(table->spatialEffectsEnabled());
        m_enableChk->blockSignals(false);
    }
    else if (m_targetTableId != VCWidget::invalidId())
        m_linkLabel->setText(tr("Table #%1 not found").arg(m_targetTableId));
    else
        m_linkLabel->setText(tr("No table — double-click column headers to map inputs"));

    rebuildAllPresetTables();
    for (PTTransitionMode mode : { PTTransitionMode::SweepOnly,
                                   PTTransitionMode::Continuous,
                                   PTTransitionMode::Channel1D,
                                   PTTransitionMode::PositionMotion,
                                   PTTransitionMode::MultiFx })
        updateBankSourceUi(mode);
    updateEffectPreview();
}

int PresetTableV2TransitionWidget::transitionPresetCount(PTTransitionMode mode) const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    return snapshotPresetsForMode(snapshot, mode).size();
}

PTTransitionPreset PresetTableV2TransitionWidget::transitionPreset(PTTransitionMode mode, int index) const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    return snapshotTransitionPreset(snapshot, mode, index);
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveTransitionPreset(PTTransitionMode mode,
                                                                            int index) const
{
    return effectiveTransitionPresetForOutput(mode, index, -1);
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveTransitionPresetForOutput(
        PTTransitionMode mode, int index, int outputIdx) const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    return outputIdx >= 0
            ? snapshotEffectivePresetForOutput(snapshot, mode, index, outputIdx, true)
            : finalizeSnapshotPreset(snapshot, mode, PresetTableV2SpatialEngine::mergePreset(
                                             snapshotTransitionPreset(snapshot, mode, index),
                                             snapshot.liveColumnOverrides));
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveTransitionPresetForPoint(
        PTTransitionMode mode, int index, int outputIdx, const QLCPoint& point) const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    const int selectionIdx = snapshotSelectionIndexForPoint(snapshot, mode, index, outputIdx, point);
    if (outputIdx >= 0 && selectionIdx >= 0)
        return snapshotEffectivePresetForSelection(snapshot, mode, index, outputIdx, selectionIdx, true);
    if (outputIdx >= 0)
        return snapshotEffectivePresetForOutput(snapshot, mode, index, outputIdx, true);
    return finalizeSnapshotPreset(snapshot, mode, PresetTableV2SpatialEngine::mergePreset(
                                         snapshotTransitionPreset(snapshot, mode, index),
                                         snapshot.liveColumnOverrides));
}

int PresetTableV2TransitionWidget::transitionSelectionKeyForPoint(
        PTTransitionMode mode, int index, int outputIdx, const QLCPoint& point) const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    return snapshotSelectionIndexForPoint(snapshot, mode, index, outputIdx, point);
}

PTTransitionPreset PresetTableV2TransitionWidget::effectiveTransitionPresetForSelection(
        PTTransitionMode mode, int index, int outputIdx, int selectionKey) const
{
    if (selectionKey < 0)
        return effectiveTransitionPresetForOutput(mode, index, outputIdx);

    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    return snapshotEffectivePresetForSelection(snapshot, mode, index, outputIdx, selectionKey, true);
}

QString PresetTableV2TransitionWidget::transitionPresetName(PTTransitionMode mode, int index) const
{
    const PTTransitionProviderSnapshot snapshot = providerSnapshotCopy();
    const QVector<PTTransitionPreset>& bank = snapshotPresetsForMode(snapshot, mode);
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
        PTEfxCol::InputOffsetStepMode,
        PTEfxCol::InputOffsetStep,
        PTEfxCol::InputDuration,
        PTEfxCol::InputWaveWidth,
        PTEfxCol::InputWaveShape,
        PTEfxCol::InputFadeIn,
        PTEfxCol::InputFadeOut,
        PTEfxCol::InputWaveLevel,
        PTEfxCol::InputStartOffset,
        PTEfxCol::InputSpeedMult,
        PTEfxCol::InputPositionMotion,
        PTEfxCol::InputPositionMotionDir,
        PTEfxCol::InputPosition1DBuiltinMode,
        PTEfxCol::InputPositionPanSize,
        PTEfxCol::InputPositionTiltSize,
        PTEfxCol::InputChannel1DTarget,
        PTEfxCol::InputChannel1DTargetMode,
        PTEfxCol::InputChannel1DApplyMode,
        PTEfxCol::InputChannel1DLow,
        PTEfxCol::InputChannel1DHigh,
        PTEfxCol::InputChannel1DAmount,
        PTEfxCol::InputChannel1DCustomColumn,
        PTEfxCol::InputGlobalSpeed,
        PTEfxCol::InputGlobalIntensity,
        PTEfxCol::InputGlobalPositionSize,
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
    if (m_positionMotionTable)
        m_positionMotionTable->setEnabled(enabled);
    if (m_channel1DTable)
        m_channel1DTable->setEnabled(enabled);
    if (m_multiFxTable)
        m_multiFxTable->setEnabled(enabled);
    if (m_toolbar)
        m_toolbar->setEnabled(enabled);
    if (m_bankTabs)
        m_bankTabs->setEnabled(enabled);
    if (m_sweepTable)
        configureFrozenNameView(PTTransitionMode::SweepOnly);
    if (m_continuousTable)
        configureFrozenNameView(PTTransitionMode::Continuous);
    if (m_positionMotionTable)
        configureFrozenNameView(PTTransitionMode::PositionMotion);
    if (m_channel1DTable)
        configureFrozenNameView(PTTransitionMode::Channel1D);
    if (m_multiFxTable)
        configureFrozenNameView(PTTransitionMode::MultiFx);
    for (PTTransitionMode bankMode : { PTTransitionMode::SweepOnly,
                                       PTTransitionMode::Continuous,
                                       PTTransitionMode::Channel1D,
                                       PTTransitionMode::PositionMotion,
                                       PTTransitionMode::MultiFx })
        updateBankSourceUi(bankMode);
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
    m_globalSettings.positionSize = gs.positionSize;
    m_globalSettings.minDurationMs = gs.minDurationMs;
    m_globalSettings.maxDurationMs = gs.maxDurationMs;
    m_globalSettings.sizeSpeedCeilingEnabled = gs.sizeSpeedCeilingEnabled;
    m_globalSettings.smallSizeMinDurationMs = gs.smallSizeMinDurationMs;
    m_globalSettings.speedOverdriveKnee = gs.speedOverdriveKnee;

    for (PTTransitionMode bankMode : { PTTransitionMode::SweepOnly,
                                       PTTransitionMode::Continuous,
                                       PTTransitionMode::Channel1D,
                                       PTTransitionMode::PositionMotion,
                                       PTTransitionMode::MultiFx })
    {
        setBankSourceEngineId(bankMode, dlg.bankSourceEngineId(bankMode));
    }

    setInputSource(dlg.globalSpeedInputSource(), PTEfxCol::InputGlobalSpeed);
    setInputSource(dlg.globalIntensityInputSource(), PTEfxCol::InputGlobalIntensity);
    setInputSource(dlg.globalPositionSizeInputSource(), PTEfxCol::InputGlobalPositionSize);
    setInputSource(dlg.globalCrossfadeManualInputSource(), PTEfxCol::InputCrossfadeManual);
    m_crossfadeManualInputMapped = dlg.globalCrossfadeManualInputSource()
            && dlg.globalCrossfadeManualInputSource()->isValid();
    publishProviderSnapshot(QStringLiteral("properties"));
    updateGlobalSummaryLabel();
    rebuildAllPresetTables();
    updateEffectPreview();

    if (PresetTableV2ControlIface* table = linkedTable())
        table->setLinkedTransitionWidgetId(id());
    notifyTablePresetCacheRefresh();

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
    copy->m_positionMotionPresets = m_positionMotionPresets;
    copy->m_channel1DPresets = m_channel1DPresets;
    copy->m_multiFxPresets = m_multiFxPresets;
    copy->m_sweepOutputOverrides = m_sweepOutputOverrides;
    copy->m_continuousOutputOverrides = m_continuousOutputOverrides;
    copy->m_positionMotionOutputOverrides = m_positionMotionOutputOverrides;
    copy->m_channel1DOutputOverrides = m_channel1DOutputOverrides;
    copy->m_multiFxOutputOverrides = m_multiFxOutputOverrides;
    copy->m_multiFxTargetRoutes = m_multiFxTargetRoutes;
    copy->m_customCurveGallery = m_customCurveGallery;
    copy->m_shapeGallery = m_shapeGallery;
    copy->m_globalSettings = m_globalSettings;
    copy->m_bankSourceEngineIds = m_bankSourceEngineIds;
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
    copy->publishProviderSnapshot(QStringLiteral("copy"));
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
    p.offsetStepMode = pattrs.hasAttribute(KXMLPresetOffsetStepMode)
            ? offsetStepModeFromString(pattrs.value(KXMLPresetOffsetStepMode).toString())
            : PTOffsetStepMode::FixedDegrees;
    if (pattrs.hasAttribute(KXMLPresetOffsetCoverage))
        p.offsetCoverage = qBound(0, pattrs.value(KXMLPresetOffsetCoverage).toInt(), 100);
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
        p.playbackMode = (m == int(PTTransitionMode::Continuous)
                          || m == int(PTTransitionMode::PositionMotion)
                          || m == int(PTTransitionMode::Channel1D)
                          || m == int(PTTransitionMode::MultiFx))
                ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
    }
    if (pattrs.hasAttribute(KXMLPresetSpeedMult))
        p.speedMultiplier = qBound(0, pattrs.value(KXMLPresetSpeedMult).toInt(), 5);
    else
        p.speedMultiplier = qBound(0, legacySpeedMult, 5);
    if (pattrs.hasAttribute(KXMLPresetPositionMotion))
        p.positionMotion = pattrs.value(KXMLPresetPositionMotion).toInt();
    if (pattrs.hasAttribute(KXMLPresetPositionMotionDir))
        p.positionMotionDirection = pattrs.value(KXMLPresetPositionMotionDir).toInt();
    if (pattrs.hasAttribute(KXMLPresetPositionPanSize))
        p.positionPanSize = pattrs.value(KXMLPresetPositionPanSize).toInt();
    if (pattrs.hasAttribute(KXMLPresetPositionTiltSize))
        p.positionTiltSize = pattrs.value(KXMLPresetPositionTiltSize).toInt();
    if (pattrs.hasAttribute(KXMLPresetChannel1DTarget))
        p.channel1DTarget = qBound(0, pattrs.value(KXMLPresetChannel1DTarget).toInt(),
                                   int(PTChannel1DTarget::CustomColumn));
    if (pattrs.hasAttribute(KXMLPresetChannel1DTargetMode))
        p.channel1DTargetMode = qBound(0, pattrs.value(KXMLPresetChannel1DTargetMode).toInt(),
                                       int(PTChannel1DTargetMode::All));
    if (pattrs.hasAttribute(KXMLPresetChannel1DApplyMode))
        p.channel1DApplyMode = qBound(0, pattrs.value(KXMLPresetChannel1DApplyMode).toInt(),
                                      int(PTChannel1DApplyMode::BumpAdd));
    if (pattrs.hasAttribute(KXMLPresetChannel1DLow))
        p.channel1DLow = qBound(0, pattrs.value(KXMLPresetChannel1DLow).toInt(), 255);
    if (pattrs.hasAttribute(KXMLPresetChannel1DHigh))
        p.channel1DHigh = qBound(0, pattrs.value(KXMLPresetChannel1DHigh).toInt(), 255);
    if (pattrs.hasAttribute(KXMLPresetChannel1DAmount))
        p.channel1DAmount = qBound(0, pattrs.value(KXMLPresetChannel1DAmount).toInt(), 255);
    if (pattrs.hasAttribute(KXMLPresetChannel1DCustomColumn))
        p.channel1DCustomColumn =
                qBound(0, pattrs.value(KXMLPresetChannel1DCustomColumn).toInt(), 255);
    if (pattrs.hasAttribute(KXMLPresetPosition1DBuiltinMode))
        p.position1DBuiltinMode = qBound(0, pattrs.value(KXMLPresetPosition1DBuiltinMode).toInt(), 1);
  {
        int legacyMotionWaveShape = -1;
        const bool legacyMotionCurveEnabled =
                pattrs.hasAttribute(KXMLPresetPositionMotionCurveEnabled)
                && pattrs.value(KXMLPresetPositionMotionCurveEnabled).toInt() != 0;
        QVector<PTCustomCurvePoint> legacyMotionCurve;
        if (pattrs.hasAttribute(KXMLPresetPositionMotionWaveShape))
            legacyMotionWaveShape = qBound(0, pattrs.value(KXMLPresetPositionMotionWaveShape).toInt(), 3);
        if (pattrs.hasAttribute(KXMLPresetPositionMotionCurve))
            legacyMotionCurve = parseCustomCurve(pattrs.value(KXMLPresetPositionMotionCurve).toString());
        migrateLegacyPositionMotionFields(p, legacyMotionWaveShape,
                                          legacyMotionCurveEnabled, legacyMotionCurve);
    }
    if (pattrs.hasAttribute(KXMLPresetPositionPath2D))
        p.positionPath2D = PTShapesGallery::parsePath2D(pattrs.value(KXMLPresetPositionPath2D).toString());
    if (pattrs.hasAttribute(KXMLPresetPositionPath2DClosed))
        p.positionPath2DClosed = pattrs.value(KXMLPresetPositionPath2DClosed).toInt() != 0;

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
        else if (col == ColOffsetStepMode)
            ov.values.offsetStepMode = offsetStepModeFromString(attrs.value(attr).toString());
        else
            setPresetColumnValue(ov.values, col, attrs.value(attr).toInt());
        ov.columns.insert(col);
    }
    if (attrs.hasAttribute(KXMLPresetOffsetCoverage))
    {
        ov.values.offsetCoverage = qBound(0, attrs.value(KXMLPresetOffsetCoverage).toInt(), 100);
        ov.columns.insert(ColOffsetStep);
    }
    if (attrs.hasAttribute(KXMLPresetCustomCurveEnabled))
        ov.values.customCurveEnabled = attrs.value(KXMLPresetCustomCurveEnabled).toInt() != 0;
    if (attrs.hasAttribute(KXMLPresetCustomCurve))
        ov.values.customCurve = parseCustomCurve(attrs.value(KXMLPresetCustomCurve).toString());
    if (attrs.hasAttribute(KXMLPresetPositionMotionCurveEnabled)
            || attrs.hasAttribute(KXMLPresetPositionMotionCurve)
            || attrs.hasAttribute(KXMLPresetPositionMotionWaveShape))
    {
        int legacyMotionWaveShape = -1;
        if (attrs.hasAttribute(KXMLPresetPositionMotionWaveShape))
            legacyMotionWaveShape = qBound(0, attrs.value(KXMLPresetPositionMotionWaveShape).toInt(), 3);
        const bool legacyMotionCurveEnabled =
                attrs.hasAttribute(KXMLPresetPositionMotionCurveEnabled)
                && attrs.value(KXMLPresetPositionMotionCurveEnabled).toInt() != 0;
        QVector<PTCustomCurvePoint> legacyMotionCurve;
        if (attrs.hasAttribute(KXMLPresetPositionMotionCurve))
            legacyMotionCurve = parseCustomCurve(attrs.value(KXMLPresetPositionMotionCurve).toString());
        migrateLegacyPositionMotionFields(ov.values, legacyMotionWaveShape,
                                          legacyMotionCurveEnabled, legacyMotionCurve);
    }
    if (attrs.hasAttribute(KXMLPresetPositionPath2D))
        ov.values.positionPath2D =
                PTShapesGallery::parsePath2D(attrs.value(KXMLPresetPositionPath2D).toString());
    if (attrs.hasAttribute(KXMLPresetPositionPath2DClosed))
        ov.values.positionPath2DClosed =
                attrs.value(KXMLPresetPositionPath2DClosed).toInt() != 0;
}

void PresetTableV2TransitionWidget::writeOutputOverrideXml(
        QXmlStreamWriter* doc, int outputIdx, const PTTransitionOutputLayer& layer) const
{
    if (layer.all.columns.isEmpty() && layer.selections.isEmpty())
        return;

    auto writeOverrideAttrs = [&](const PTTransitionPresetOverride& ov) {
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
            else if (col == ColOffsetStepMode)
                doc->writeAttribute(attr, offsetStepModeToString(ov.values.offsetStepMode));
            else if (col == ColOffsetStep
                     && ov.values.offsetStepMode == PTOffsetStepMode::CoveragePercent)
                doc->writeAttribute(KXMLPresetOffsetCoverage,
                                    QString::number(qBound(0, ov.values.offsetCoverage, 100)));
            else
                doc->writeAttribute(attr, presetColumnValue(ov.values, col).toString());
        }
        if (ov.columns.contains(ColWaveShape) && ov.values.customCurveEnabled)
        {
            doc->writeAttribute(KXMLPresetCustomCurveEnabled, QStringLiteral("1"));
            doc->writeAttribute(KXMLPresetCustomCurve, serializeCustomCurve(ov.values.customCurve));
        }
    };

    doc->writeStartElement(KXMLOutputOverride);
    doc->writeAttribute(KXMLOutputOverrideIndex, QString::number(outputIdx));
    writeOverrideAttrs(layer.all);
    for (const PTTransitionSelection& selection : layer.selections)
    {
        doc->writeStartElement(KXMLSelection);
        doc->writeAttribute(KXMLSelectionName, selection.name);
        doc->writeAttribute(KXMLSelectionCells, serializeSelectionCells(selection.cells));
        writeOverrideAttrs(selection.overrides);
        doc->writeEndElement();
    }
    doc->writeEndElement();
}

void PresetTableV2TransitionWidget::writePresetXml(
        QXmlStreamWriter* doc, const PTTransitionPreset& p,
        const QHash<int, PTTransitionOutputLayer>& overrides) const
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
    doc->writeAttribute(KXMLPresetOffsetStepMode, offsetStepModeToString(p.offsetStepMode));
    doc->writeAttribute(KXMLPresetOffsetCoverage,
                        QString::number(qBound(0, p.offsetCoverage, 100)));
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
    doc->writeAttribute(KXMLPresetPositionMotion, QString::number(p.positionMotion));
    doc->writeAttribute(KXMLPresetPositionMotionDir, QString::number(p.positionMotionDirection));
    if (p.position1DBuiltinMode != 0)
        doc->writeAttribute(KXMLPresetPosition1DBuiltinMode, QString::number(p.position1DBuiltinMode));
    doc->writeAttribute(KXMLPresetPositionPanSize, QString::number(p.positionPanSize));
    doc->writeAttribute(KXMLPresetPositionTiltSize, QString::number(p.positionTiltSize));
    doc->writeAttribute(KXMLPresetChannel1DTarget, QString::number(p.channel1DTarget));
    doc->writeAttribute(KXMLPresetChannel1DTargetMode, QString::number(p.channel1DTargetMode));
    doc->writeAttribute(KXMLPresetChannel1DApplyMode, QString::number(p.channel1DApplyMode));
    doc->writeAttribute(KXMLPresetChannel1DLow, QString::number(p.channel1DLow));
    doc->writeAttribute(KXMLPresetChannel1DHigh, QString::number(p.channel1DHigh));
    doc->writeAttribute(KXMLPresetChannel1DAmount, QString::number(p.channel1DAmount));
    doc->writeAttribute(KXMLPresetChannel1DCustomColumn,
                        QString::number(p.channel1DCustomColumn));
    if (!p.positionPath2D.isEmpty())
    {
        doc->writeAttribute(KXMLPresetPositionPath2D,
                            PTShapesGallery::serializePath2D(p.positionPath2D));
        doc->writeAttribute(KXMLPresetPositionPath2DClosed,
                            p.positionPath2DClosed ? QStringLiteral("1") : QStringLiteral("0"));
    }
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
    m_channel1DPresets.clear();
    m_positionMotionPresets.clear();
    m_multiFxPresets.clear();
    m_sweepOutputOverrides.clear();
    m_continuousOutputOverrides.clear();
    m_channel1DOutputOverrides.clear();
    m_positionMotionOutputOverrides.clear();
    m_multiFxOutputOverrides.clear();
    m_multiFxTargetRoutes.clear();
    m_customCurveGallery.clear();
    m_shapeGallery.clear();
    int legacySweepDir = 0;
    int legacySpeedMult = 1;

    auto finalizePreset = [&](PTTransitionPreset& p, PTTransitionMode bankHint,
                              const QXmlStreamAttributes& pattrs) {
        if (!pattrs.hasAttribute(KXMLPresetPlaybackMode))
        {
            if (bankHint == PTTransitionMode::Continuous)
                p.playbackMode = PTTransitionMode::Continuous;
            else if (bankHint == PTTransitionMode::PositionMotion)
                p.playbackMode = PTTransitionMode::Continuous;
            else if (bankHint == PTTransitionMode::Channel1D)
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
        else if (bankHint == PTTransitionMode::PositionMotion)
            stored.playbackMode = PTTransitionMode::Continuous;
        else if (bankHint == PTTransitionMode::Channel1D)
            stored.playbackMode = PTTransitionMode::Continuous;
        else if (bankHint == PTTransitionMode::MultiFx)
            stored.playbackMode = PTTransitionMode::Continuous;
        else if (bankHint == PTTransitionMode::SweepOnly)
            stored.playbackMode = PTTransitionMode::SweepOnly;

        if (bankHint == PTTransitionMode::PositionMotion)
            m_positionMotionPresets.append(stored);
        else if (bankHint == PTTransitionMode::Channel1D)
            m_channel1DPresets.append(stored);
        else if (bankHint == PTTransitionMode::MultiFx)
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
        else if (root.name() == KXMLGlobalPositionSizeInput)
            loadXMLSources(root, PTEfxCol::InputGlobalPositionSize);
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
            if (gattrs.hasAttribute(KXMLGlobalSizeSpeedCeiling))
                m_globalSettings.sizeSpeedCeilingEnabled =
                        gattrs.value(KXMLGlobalSizeSpeedCeiling).toInt() != 0;
            if (gattrs.hasAttribute(KXMLGlobalSmallSizeMinMs))
                m_globalSettings.smallSizeMinDurationMs =
                        qMax(quint32(20), gattrs.value(KXMLGlobalSmallSizeMinMs).toUInt());
            else
                m_globalSettings.smallSizeMinDurationMs = m_globalSettings.minDurationMs;
            if (gattrs.hasAttribute(KXMLGlobalSpeedOverdriveKnee))
                m_globalSettings.speedOverdriveKnee =
                        qBound(1, gattrs.value(KXMLGlobalSpeedOverdriveKnee).toInt(), 254);
            if (gattrs.hasAttribute(KXMLGlobalMultiplier))
                legacySpeedMult = gattrs.value(KXMLGlobalMultiplier).toInt();
            if (gattrs.hasAttribute(KXMLGlobalDirection))
                legacySweepDir = gattrs.value(KXMLGlobalDirection).toInt();
            if (gattrs.hasAttribute(KXMLGlobalIntensity))
                m_globalSettings.intensity = uchar(gattrs.value(KXMLGlobalIntensity).toInt());
            if (gattrs.hasAttribute(KXMLGlobalPositionSize))
                m_globalSettings.positionSize = uchar(gattrs.value(KXMLGlobalPositionSize).toInt());
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
        else if (root.name() == KXMLBankSource)
        {
            const QXmlStreamAttributes attrs = root.attributes();
            const int modeValue = attrs.value(KXMLBankSourceMode).toInt();
            const quint32 engineId = attrs.value(KXMLBankSourceEngine).toUInt();
            const PTTransitionMode bankMode = PTTransitionMode(modeValue);
            if (engineId != VCWidget::invalidId() && !bankSourceWouldCreateCycle(bankMode, engineId))
                m_bankSourceEngineIds.insert(modeValue, engineId);
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
                    QHash<int, PTTransitionOutputLayer> presetOverrides;
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
                                PTTransitionOutputLayer layer;
                                readOutputOverride(layer.all, attrs, p);
                                while (root.readNextStartElement())
                                {
                                    if (root.name() == KXMLSelection)
                                    {
                                        PTTransitionSelection selection;
                                        selection.name = root.attributes().value(KXMLSelectionName).toString();
                                        selection.cells = parseSelectionCells(
                                                root.attributes().value(KXMLSelectionCells).toString());
                                        readOutputOverride(selection.overrides, root.attributes(), p);
                                        layer.selections.append(selection);
                                    }
                                    root.skipCurrentElement();
                                }
                                if (!layer.all.columns.isEmpty() || !layer.selections.isEmpty())
                                    presetOverrides.insert(outputIdx, layer);
                            }
                            else
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
                    QHash<int, PTTransitionOutputLayer> presetOverrides;
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
                                PTTransitionOutputLayer layer;
                                readOutputOverride(layer.all, attrs, p);
                                while (root.readNextStartElement())
                                {
                                    if (root.name() == KXMLSelection)
                                    {
                                        PTTransitionSelection selection;
                                        selection.name = root.attributes().value(KXMLSelectionName).toString();
                                        selection.cells = parseSelectionCells(
                                                root.attributes().value(KXMLSelectionCells).toString());
                                        readOutputOverride(selection.overrides, root.attributes(), p);
                                        layer.selections.append(selection);
                                    }
                                    root.skipCurrentElement();
                                }
                                if (!layer.all.columns.isEmpty() || !layer.selections.isEmpty())
                                    presetOverrides.insert(outputIdx, layer);
                            }
                            else
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
        else if (root.name() == KXMLChannel1DPresets)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLPreset)
                {
                    const auto pattrs = root.attributes();
                    PTTransitionPreset p;
                    readPresetAttrs(p, pattrs, legacySpeedMult);
                    finalizePreset(p, PTTransitionMode::Channel1D, pattrs);
                    QHash<int, PTTransitionOutputLayer> presetOverrides;
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
                                PTTransitionOutputLayer layer;
                                readOutputOverride(layer.all, attrs, p);
                                while (root.readNextStartElement())
                                {
                                    if (root.name() == KXMLSelection)
                                    {
                                        PTTransitionSelection selection;
                                        selection.name =
                                                root.attributes().value(KXMLSelectionName).toString();
                                        selection.cells = parseSelectionCells(
                                                root.attributes().value(KXMLSelectionCells).toString());
                                        readOutputOverride(selection.overrides, root.attributes(), p);
                                        layer.selections.append(selection);
                                    }
                                    root.skipCurrentElement();
                                }
                                if (!layer.all.columns.isEmpty() || !layer.selections.isEmpty())
                                    presetOverrides.insert(outputIdx, layer);
                            }
                            else
                                root.skipCurrentElement();
                        }
                        else
                            root.skipCurrentElement();
                    }
                    m_channel1DPresets.append(p);
                    m_channel1DOutputOverrides.append(presetOverrides);
                }
                else
                    root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLPositionMotionPresets)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLPreset)
                {
                    const auto pattrs = root.attributes();
                    PTTransitionPreset p;
                    readPresetAttrs(p, pattrs, legacySpeedMult);
                    finalizePreset(p, PTTransitionMode::PositionMotion, pattrs);
                    QHash<int, PTTransitionOutputLayer> presetOverrides;
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
                                PTTransitionOutputLayer layer;
                                readOutputOverride(layer.all, attrs, p);
                                while (root.readNextStartElement())
                                {
                                    if (root.name() == KXMLSelection)
                                    {
                                        PTTransitionSelection selection;
                                        selection.name = root.attributes().value(KXMLSelectionName).toString();
                                        selection.cells = parseSelectionCells(
                                                root.attributes().value(KXMLSelectionCells).toString());
                                        readOutputOverride(selection.overrides, root.attributes(), p);
                                        layer.selections.append(selection);
                                    }
                                    root.skipCurrentElement();
                                }
                                if (!layer.all.columns.isEmpty() || !layer.selections.isEmpty())
                                    presetOverrides.insert(outputIdx, layer);
                            }
                            else
                                root.skipCurrentElement();
                        }
                        else
                            root.skipCurrentElement();
                    }
                    m_positionMotionPresets.append(p);
                    m_positionMotionOutputOverrides.append(presetOverrides);
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
                    QHash<int, PTTransitionOutputLayer> presetOverrides;
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
                                PTTransitionOutputLayer layer;
                                readOutputOverride(layer.all, attrs, p);
                                while (root.readNextStartElement())
                                {
                                    if (root.name() == KXMLSelection)
                                    {
                                        PTTransitionSelection selection;
                                        selection.name = root.attributes().value(KXMLSelectionName).toString();
                                        selection.cells = parseSelectionCells(
                                                root.attributes().value(KXMLSelectionCells).toString());
                                        readOutputOverride(selection.overrides, root.attributes(), p);
                                        layer.selections.append(selection);
                                    }
                                    root.skipCurrentElement();
                                }
                                if (!layer.all.columns.isEmpty() || !layer.selections.isEmpty())
                                    presetOverrides.insert(outputIdx, layer);
                            }
                            else
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
        else if (root.name() == KXMLMultiFxTargets)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLMultiFxTargetPreset)
                {
                    bool presetOk = false;
                    const int presetIndex =
                            root.attributes().value(KXMLMultiFxTargetPresetIndex).toInt(&presetOk);
                    QVector<PTMultiFxTargetTableRoute> routes;
                    while (root.readNextStartElement())
                    {
                        if (root.name() == KXMLMultiFxTargetTable)
                        {
                            bool tableOk = false;
                            PTMultiFxTargetTableRoute tableRoute;
                            tableRoute.tableId =
                                    root.attributes().value(KXMLMultiFxTargetTableId).toUInt(&tableOk);
                            tableRoute.enabled =
                                    !root.attributes().hasAttribute(KXMLMultiFxTargetEnabled)
                                    || root.attributes().value(KXMLMultiFxTargetEnabled).toInt() != 0;
                            tableRoute.layerKind =
                                    root.attributes().hasAttribute(KXMLMultiFxTargetLayerKind)
                                    ? root.attributes().value(KXMLMultiFxTargetLayerKind).toInt()
                                    : int(PTMultiFxTargetLayerKind::Auto);
                            PTTransitionPreset routeBase =
                                    (presetIndex >= 0 && presetIndex < m_multiFxPresets.size())
                                    ? m_multiFxPresets.at(presetIndex) : PTTransitionPreset();
                            PTTransitionPresetOverride tableOv;
                            readOutputOverride(tableOv, root.attributes(), routeBase);
                            tableRoute.tableOverride.values = tableOv.values;
                            tableRoute.tableOverride.columns = tableOv.columns;
                            while (root.readNextStartElement())
                            {
                                if (root.name() == KXMLMultiFxTargetOutput)
                                {
                                    bool outputOk = false;
                                    PTMultiFxTargetOutputRoute outputRoute;
                                    outputRoute.outputIndex =
                                            root.attributes().value(KXMLMultiFxTargetOutputIndex)
                                            .toInt(&outputOk);
                                    const QStringList selectionParts =
                                            root.attributes().value(KXMLMultiFxTargetSelections)
                                            .toString().split(QLatin1Char(','),
                                                              Qt::SkipEmptyParts);
                                    for (const QString& part : selectionParts)
                                    {
                                        bool selectionOk = false;
                                        const int selectionKey = part.toInt(&selectionOk);
                                        if (selectionOk)
                                            outputRoute.selectionKeys.append(selectionKey);
                                    }
                                    PTTransitionProviderOutputLayer layer;
                                    PTTransitionPreset outputBase = routeBase;
                                    applyOverrideColumnsToPreset(outputBase, tableRoute.tableOverride);
                                    PTTransitionPresetOverride outputOv;
                                    readOutputOverride(outputOv, root.attributes(), outputBase);
                                    layer.all.values = outputOv.values;
                                    layer.all.columns = outputOv.columns;
                                    while (root.readNextStartElement())
                                    {
                                        if (root.name() == KXMLSelection)
                                        {
                                            PTTransitionProviderSelection selection;
                                            selection.name =
                                                    root.attributes().value(KXMLSelectionName).toString();
                                            selection.cells = parseSelectionCells(
                                                    root.attributes().value(KXMLSelectionCells).toString());
                                            PTTransitionPreset selectionBase = outputBase;
                                            applyOverrideColumnsToPreset(selectionBase, layer.all);
                                            PTTransitionPresetOverride selectionOv;
                                            readOutputOverride(selectionOv, root.attributes(),
                                                               selectionBase);
                                            selection.overrides.values = selectionOv.values;
                                            selection.overrides.columns = selectionOv.columns;
                                            layer.selections.append(selection);
                                            root.skipCurrentElement();
                                        }
                                        else
                                            root.skipCurrentElement();
                                    }
                                    if (outputOk && outputRoute.outputIndex >= 0)
                                    {
                                        tableRoute.outputs.append(outputRoute);
                                        if (!layer.all.columns.isEmpty()
                                                || !layer.selections.isEmpty())
                                            tableRoute.outputOverrides.insert(
                                                    outputRoute.outputIndex, layer);
                                    }
                                }
                                else
                                    root.skipCurrentElement();
                            }
                            if (tableOk && tableRoute.tableId != VCWidget::invalidId())
                                routes.append(tableRoute);
                        }
                        else
                            root.skipCurrentElement();
                    }
                    if (presetOk && presetIndex >= 0)
                    {
                        while (m_multiFxTargetRoutes.size() <= presetIndex)
                            m_multiFxTargetRoutes.append(QVector<PTMultiFxTargetTableRoute>());
                        m_multiFxTargetRoutes[presetIndex] = routes;
                    }
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
        else if (root.name() == KXMLShapeGallery)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLShapeGalleryItem)
                {
                    const auto attrs = root.attributes();
                    PTShapeGalleryItem item;
                    item.name = attrs.value(KXMLCustomCurveItemName).toString();
                    item.kind = attrs.value(KXMLShapeGalleryKind).toInt() == 1
                            ? PTShapeGalleryKind::Path2D : PTShapeGalleryKind::Curve1D;
                    item.curve1D = parseCustomCurve(attrs.value(KXMLCustomCurveItemCurve).toString());
                    item.path2D = PTShapesGallery::parsePath2D(
                            attrs.value(KXMLPresetPositionPath2D).toString());
                    item.path2DClosed = attrs.value(KXMLShapeGalleryPath2DClosed).toInt() != 0;
                    if (!item.name.isEmpty())
                        m_shapeGallery.append(item);
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
    if (m_positionMotionPresets.isEmpty())
        m_positionMotionPresets.append(defaultPreset(0, PTTransitionMode::PositionMotion));
    if (m_multiFxPresets.isEmpty())
        m_multiFxPresets.append(defaultPreset(0, PTTransitionMode::MultiFx));
    if (m_shapeGallery.isEmpty())
        m_shapeGallery = PTShapesGallery::defaultBuiltinItems();

    normalizeNoopOverrides();
    migrateLegacyInputSources();

    rebuildAllPresetTables();
    updateGlobalSummaryLabel();
    slotRefreshTableLink();
    scheduleDeferredTableLinkRefresh();
    notifyTablePresetCacheRefresh();
    return true;
}

bool PresetTableV2TransitionWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    ScopedBoolFlag noLiveLinkedTableLookup(m_disableLiveLinkedTableLookup);

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
    if (m_channel1DTable)
    {
        for (int r = 0; r < m_channel1DTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::Channel1D, r);
    }
    if (m_positionMotionTable)
    {
        for (int r = 0; r < m_positionMotionTable->topLevelItemCount(); ++r)
            syncPresetFromTable(PTTransitionMode::PositionMotion, r);
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
    doc->writeAttribute(KXMLGlobalSizeSpeedCeiling,
                        QString::number(m_globalSettings.sizeSpeedCeilingEnabled ? 1 : 0));
    doc->writeAttribute(KXMLGlobalSmallSizeMinMs,
                        QString::number(m_globalSettings.smallSizeMinDurationMs));
    doc->writeAttribute(KXMLGlobalSpeedOverdriveKnee,
                        QString::number(qBound(1, m_globalSettings.speedOverdriveKnee, 254)));
    doc->writeAttribute(KXMLGlobalIntensity, QString::number(m_globalSettings.intensity));
    doc->writeAttribute(KXMLGlobalPositionSize, QString::number(m_globalSettings.positionSize));
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
    saveInputBinding(PTEfxCol::InputGlobalPositionSize, KXMLGlobalPositionSizeInput);
    saveInputBinding(PTEfxCol::InputCrossfadeManual, KXMLGlobalCrossfadeManualInput);

    for (int col = 1; col < ColCount; ++col)
        saveInputBinding(PTEfxCol::inputIdForColumn(col), KXMLEfxColumnInput);

    for (PTTransitionMode bankMode : { PTTransitionMode::SweepOnly,
                                       PTTransitionMode::Continuous,
                                       PTTransitionMode::Channel1D,
                                       PTTransitionMode::PositionMotion,
                                       PTTransitionMode::MultiFx })
    {
        const quint32 sourceId = bankSourceEngineId(bankMode);
        if (sourceId == VCWidget::invalidId())
            continue;
        doc->writeStartElement(KXMLBankSource);
        doc->writeAttribute(KXMLBankSourceMode, QString::number(int(bankMode)));
        doc->writeAttribute(KXMLBankSourceEngine, QString::number(sourceId));
        doc->writeEndElement();
    }

    doc->writeStartElement(KXMLSweepPresets);
    for (int i = 0; i < m_sweepPresets.size(); ++i)
        writePresetXml(doc, m_sweepPresets.at(i),
                       i < m_sweepOutputOverrides.size()
                       ? m_sweepOutputOverrides.at(i)
                       : QHash<int, PTTransitionOutputLayer>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLContinuousPresets);
    for (int i = 0; i < m_continuousPresets.size(); ++i)
        writePresetXml(doc, m_continuousPresets.at(i),
                       i < m_continuousOutputOverrides.size()
                       ? m_continuousOutputOverrides.at(i)
                       : QHash<int, PTTransitionOutputLayer>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLChannel1DPresets);
    for (int i = 0; i < m_channel1DPresets.size(); ++i)
        writePresetXml(doc, m_channel1DPresets.at(i),
                       i < m_channel1DOutputOverrides.size()
                       ? m_channel1DOutputOverrides.at(i)
                       : QHash<int, PTTransitionOutputLayer>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLPositionMotionPresets);
    for (int i = 0; i < m_positionMotionPresets.size(); ++i)
        writePresetXml(doc, m_positionMotionPresets.at(i),
                       i < m_positionMotionOutputOverrides.size()
                       ? m_positionMotionOutputOverrides.at(i)
                       : QHash<int, PTTransitionOutputLayer>());
    doc->writeEndElement();

    doc->writeStartElement(KXMLMultiFxPresets);
    for (int i = 0; i < m_multiFxPresets.size(); ++i)
        writePresetXml(doc, m_multiFxPresets.at(i),
                       i < m_multiFxOutputOverrides.size()
                       ? m_multiFxOutputOverrides.at(i)
                       : QHash<int, PTTransitionOutputLayer>());
    doc->writeEndElement();

    auto writeProviderOverrideAttrs = [&](const PTTransitionProviderPresetOverride& ov) {
        for (int col : ov.columns)
        {
            const QString attr = presetColumnXmlName(col);
            if (attr.isEmpty())
                continue;
            if (col == ColAxis)
                doc->writeAttribute(attr,
                        PresetTableV2SpatialEngine::axisToString(ov.values.axis));
            else if (col == ColOffsetDir)
                doc->writeAttribute(attr,
                        PresetTableV2SpatialEngine::offsetDirectionToString(
                                ov.values.offsetDirection));
            else if (col == ColPropagation)
                doc->writeAttribute(attr, QString::number(int(ov.values.propagation)));
            else if (col == ColOffsetStepMode)
                doc->writeAttribute(attr, offsetStepModeToString(ov.values.offsetStepMode));
            else if (col == ColOffsetStep
                     && ov.values.offsetStepMode == PTOffsetStepMode::CoveragePercent)
                doc->writeAttribute(KXMLPresetOffsetCoverage,
                                    QString::number(qBound(0, ov.values.offsetCoverage, 100)));
            else
                doc->writeAttribute(attr, presetColumnValue(ov.values, col).toString());
        }
        if (ov.columns.contains(ColWaveShape) && ov.values.customCurveEnabled)
        {
            doc->writeAttribute(KXMLPresetCustomCurveEnabled, QStringLiteral("1"));
            doc->writeAttribute(KXMLPresetCustomCurve,
                                serializeCustomCurve(ov.values.customCurve));
        }
    };

    doc->writeStartElement(KXMLMultiFxTargets);
    for (int presetIndex = 0; presetIndex < m_multiFxTargetRoutes.size(); ++presetIndex)
    {
        const QVector<PTMultiFxTargetTableRoute>& routes = m_multiFxTargetRoutes.at(presetIndex);
        if (routes.isEmpty())
            continue;
        doc->writeStartElement(KXMLMultiFxTargetPreset);
        doc->writeAttribute(KXMLMultiFxTargetPresetIndex, QString::number(presetIndex));
        for (const PTMultiFxTargetTableRoute& tableRoute : routes)
        {
            if (tableRoute.tableId == VCWidget::invalidId())
                continue;
            doc->writeStartElement(KXMLMultiFxTargetTable);
            doc->writeAttribute(KXMLMultiFxTargetTableId, QString::number(tableRoute.tableId));
            doc->writeAttribute(KXMLMultiFxTargetEnabled, tableRoute.enabled ? QStringLiteral("1")
                                                                             : QStringLiteral("0"));
            doc->writeAttribute(KXMLMultiFxTargetLayerKind,
                                QString::number(tableRoute.layerKind));
            writeProviderOverrideAttrs(tableRoute.tableOverride);
            for (const PTMultiFxTargetOutputRoute& outputRoute : tableRoute.outputs)
            {
                if (outputRoute.outputIndex < 0)
                    continue;
                doc->writeStartElement(KXMLMultiFxTargetOutput);
                doc->writeAttribute(KXMLMultiFxTargetOutputIndex,
                                    QString::number(outputRoute.outputIndex));
                const PTTransitionProviderOutputLayer layer =
                        tableRoute.outputOverrides.value(outputRoute.outputIndex);
                writeProviderOverrideAttrs(layer.all);
                if (!outputRoute.selectionKeys.isEmpty())
                {
                    QStringList selectionParts;
                    for (int key : outputRoute.selectionKeys)
                        selectionParts.append(QString::number(key));
                    doc->writeAttribute(KXMLMultiFxTargetSelections,
                                        selectionParts.join(QLatin1Char(',')));
                }
                for (const PTTransitionProviderSelection& selection : layer.selections)
                {
                    doc->writeStartElement(KXMLSelection);
                    doc->writeAttribute(KXMLSelectionName, selection.name);
                    doc->writeAttribute(KXMLSelectionCells,
                                        serializeSelectionCells(selection.cells));
                    writeProviderOverrideAttrs(selection.overrides);
                    doc->writeEndElement();
                }
                doc->writeEndElement();
            }
            doc->writeEndElement();
        }
        doc->writeEndElement();
    }
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

    doc->writeStartElement(KXMLShapeGallery);
    for (const PTShapeGalleryItem& item : m_shapeGallery)
    {
        if (item.name.isEmpty())
            continue;
        doc->writeStartElement(KXMLShapeGalleryItem);
        doc->writeAttribute(KXMLCustomCurveItemName, item.name);
        doc->writeAttribute(KXMLShapeGalleryKind,
                            QString::number(int(item.kind)));
        if (item.kind == PTShapeGalleryKind::Curve1D && item.curve1D.size() >= 2)
            doc->writeAttribute(KXMLCustomCurveItemCurve, serializeCustomCurve(item.curve1D));
        if (item.kind == PTShapeGalleryKind::Path2D && item.path2D.size() >= 2)
        {
            doc->writeAttribute(KXMLPresetPositionPath2D,
                                PTShapesGallery::serializePath2D(item.path2D));
            doc->writeAttribute(KXMLShapeGalleryPath2DClosed,
                                item.path2DClosed ? QStringLiteral("1") : QStringLiteral("0"));
        }
        doc->writeEndElement();
    }
    doc->writeEndElement();

    doc->writeEndElement();
    return true;
}

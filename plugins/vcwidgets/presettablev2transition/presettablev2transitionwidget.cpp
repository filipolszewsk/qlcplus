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
#include <QVBoxLayout>
#include <QTimer>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QScopedPointer>
#include <QPointer>
#include <QMutexLocker>

#include <algorithm>

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

static const QString KXMLTransitionMode = QStringLiteral("TransitionMode");
static const QString KXMLPreset = QStringLiteral("TransitionPreset");
static const QString KXMLSweepPresets = QStringLiteral("SweepPresets");
static const QString KXMLContinuousPresets = QStringLiteral("ContinuousPresets");
static const QString KXMLMultiFxPresets = QStringLiteral("MultiFxPresets");
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
    address.col = col;
    if (m_owner->itemForPresetAddress(mode, address.row,
                                      address.outputIdx,
                                      address.selectionIdx) != item)
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

    const bool selected = opt.state & QStyle::State_Selected;
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
    if (selected)
        colorRule += QStringLiteral(" border: 2px solid palette(highlight); border-radius: 2px;");
    else
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

    ScopedBoolFlag guard(m_committingPresetCell);
    const PTTransitionMode mode = modeForTable(table);
    QTreeWidgetItem* item = itemForPresetAddress(mode, address.row,
                                                 address.outputIdx,
                                                 address.selectionIdx);
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
                      address.outputIdx, address.selectionIdx);
    VCPluginDiagnostics::breadcrumb(
            QStringLiteral("presettablev2transition"), id(), caption(),
            QStringLiteral("transition parameter commit end mode=%1 row=%2 output=%3 selection=%4 col=%5 value=%6")
                    .arg(int(mode)).arg(address.row).arg(address.outputIdx)
                    .arg(address.selectionIdx)
                    .arg(presetColumnXmlName(address.col), value.toString()));
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
        case ColChannel1DAmount: return QObject::tr("Depth");
        case ColChannel1DCustomColumn: return QObject::tr("Column");
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

    const bool positionMode = linkedTableUsesPositionMode();
    table->setColumnHidden(ColDuration, positionMode);
    if (!positionMode)
    {
        table->setColumnHidden(ColPositionMotion, true);
        table->setColumnHidden(ColPositionMotionDir, true);
        table->setColumnHidden(ColPosition1DBuiltinMode, true);
        table->setColumnHidden(ColPositionPanSize, true);
        table->setColumnHidden(ColPositionTiltSize, true);
        table->setColumnHidden(ColPropagation, true);
        if (mode == PTTransitionMode::Channel1D)
        {
            const int visibleCols[] = {
                ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                ColOffsetStepMode, ColOffsetStep, ColWaveWidth,
                ColWaveShape, ColFadeIn, ColFadeOut, ColStartOffset,
                ColSpeedMult,
                ColChannel1DApplyMode, ColChannel1DAmount, -1
            };
            QSet<int> visible;
            for (int i = 0; visibleCols[i] >= 0; ++i)
                visible.insert(visibleCols[i]);
            for (int col = ColAxis; col < ColCount; ++col)
                table->setColumnHidden(col, !visible.contains(col));
            return;
        }
        for (int col = ColAxis; col < ColCount; ++col)
        {
            if (col == ColDuration || col == ColPositionMotion || col == ColPositionMotionDir
                    || col == ColPosition1DBuiltinMode
                    || col == ColPositionPanSize || col == ColPositionTiltSize
                    || col == ColChannel1DTarget || col == ColChannel1DTargetMode
                    || col == ColChannel1DApplyMode || col == ColChannel1DLow
                    || col == ColChannel1DHigh || col == ColChannel1DAmount
                    || col == ColChannel1DCustomColumn
                    || col == ColPropagation)
            {
                continue;
            }
            table->setColumnHidden(col, false);
        }
        return;
    }

    table->setColumnHidden(ColPositionPanSize, true);
    table->setColumnHidden(ColPositionTiltSize, true);
    for (int col : { ColChannel1DTarget, ColChannel1DTargetMode, ColChannel1DApplyMode,
                     ColChannel1DLow, ColChannel1DHigh, ColChannel1DAmount,
                     ColChannel1DCustomColumn })
        table->setColumnHidden(col, true);

    if (mode == PTTransitionMode::SweepOnly)
    {
        const int visibleCols[] = {
            ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
            ColOffsetStepMode, ColOffsetStep,
            ColWaveWidth, ColWaveShape, ColFadeIn, ColStartOffset,
            ColSpeedMult, -1
        };
        QSet<int> visible;
        for (int i = 0; visibleCols[i] >= 0; ++i)
            visible.insert(visibleCols[i]);
        for (int col = ColAxis; col < ColCount; ++col)
            table->setColumnHidden(col, !visible.contains(col));
        table->setColumnHidden(ColPositionMotion, true);
        table->setColumnHidden(ColPositionMotionDir, true);
        table->setColumnHidden(ColPosition1DBuiltinMode, true);
        table->setColumnHidden(ColFadeOut, true);
        table->setColumnHidden(ColWaveLevel, true);
        return;
    }

    if (mode == PTTransitionMode::Continuous)
    {
        const int visibleCols[] = {
            ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
            ColOffsetStepMode, ColOffsetStep,
            ColWaveWidth, ColWaveShape, ColFadeIn, ColFadeOut, ColStartOffset,
            ColSpeedMult, -1
        };
        QSet<int> visible;
        for (int i = 0; visibleCols[i] >= 0; ++i)
            visible.insert(visibleCols[i]);
        for (int col = ColAxis; col < ColCount; ++col)
            table->setColumnHidden(col, !visible.contains(col));
        table->setColumnHidden(ColPositionMotion, true);
        table->setColumnHidden(ColPositionMotionDir, true);
        table->setColumnHidden(ColPosition1DBuiltinMode, true);
        table->setColumnHidden(ColWaveLevel, true);
        table->setColumnHidden(ColPropagation, true);
        return;
    }

    if (mode == PTTransitionMode::PositionMotion || mode == PTTransitionMode::MultiFx)
    {
        for (int col = ColAxis; col < ColCount; ++col)
        {
            if (col == ColPositionPanSize || col == ColPositionTiltSize || col == ColDuration)
                continue;
            table->setColumnHidden(col, false);
        }
        table->setColumnHidden(ColWaveLevel, true);
        table->setColumnHidden(ColPropagation, true);
        return;
    }

    table->setColumnHidden(ColPositionMotion, true);
    table->setColumnHidden(ColPositionMotionDir, true);
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
                return tr("Main strength of the 1D effect");
            case ColChannel1DLow:
            case ColChannel1DHigh:
                return tr("Legacy range value used only by old Absolute Range presets");
            case ColWaveShape:
                return tr("Shape of the full-scale 1D wave before Depth is applied");
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
    auto add = [&](const QString& id, const QString& label, std::initializer_list<int> cols) {
        PTTransitionColumnGroupBar::Group group;
        group.id = id;
        group.label = label;
        group.columns = QVector<int>(cols);
        groups.append(group);
    };

    if (linkedTableUsesPositionMode())
    {
        if (mode == PTTransitionMode::SweepOnly)
        {
            add(QStringLiteral("spread"), tr("Spread"),
                { ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
                  ColOffsetStepMode, ColOffsetStep });
            add(QStringLiteral("motion"), tr("Motion"),
                { ColAxis, ColWaveWidth, ColStartOffset });
            add(QStringLiteral("morph"), tr("Row morph"),
                { ColWaveShape, ColFadeIn, ColFadeOut });
            add(QStringLiteral("timing"), tr("Timing"),
                { ColSpeedMult });
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
        { ColAxis, ColOffsetDir, ColWings, ColBlocks, ColWingsSymmetry,
          ColOffsetStepMode, ColOffsetStep });
    if (mode == PTTransitionMode::Channel1D)
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
    if (mode == PTTransitionMode::Channel1D)
    {
        add(QStringLiteral("value"), tr("Value"),
            { ColChannel1DApplyMode, ColChannel1DAmount });
    }
    add(QStringLiteral("timing"), tr("Timing"),
        { ColSpeedMult });
    return groups;
}

void PresetTableV2TransitionWidget::applyColumnGroupFilter(QTreeWidget* table,
                                                         PTTransitionMode mode)
{
    if (!table)
        return;

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
    c->addItem(QObject::tr("Mirror Pairs"), int(PTPositionMotionDirection::SymmetricPairs));
    configureTransitionCombo(c, 140);
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
        case ColOffsetStepMode: return int(preset.offsetStepMode);
        case ColOffsetStep:
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
    connect(m_curveWidget, &PTDimmerWaveCurveWidget::customCurveEditRequested,
            this, &PresetTableV2TransitionWidget::slotOpenCustomCurveEditor);
    m_positionPreviewLabel = new QLabel(tr("Position motion"), m_leftPreview);
    m_positionMotionStack = new QStackedWidget(m_leftPreview);
    m_positionMotion1DWidget = new PTPositionMotion1DPreviewWidget(m_positionMotionStack);
    connect(m_positionMotion1DWidget, &PTPositionMotion1DPreviewWidget::motionCurveEditRequested,
            this, &PresetTableV2TransitionWidget::slotOpenMotionCurveEditor);
    m_positionPathWidget = new PTPositionPathPreviewWidget(m_positionMotionStack);
    m_positionMotionStack->addWidget(m_positionMotion1DWidget);
    m_positionMotionStack->addWidget(m_positionPathWidget);
    leftLayout->addWidget(m_curveLabel);
    leftLayout->addWidget(m_curveWidget, 2);
    leftLayout->addWidget(m_positionPreviewLabel);
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
                frozen->expand(table->indexFromItem(item));
        });
        connect(table, &QTreeWidget::itemCollapsed, this, [this, table](QTreeWidgetItem* item) {
            if (!item)
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
                frozen->collapse(table->indexFromItem(item));
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
                       tr("Transitions"));
    m_bankTabs->addTab(makeBankPage(m_continuousNameView, m_continuousTable,
                                     PTTransitionMode::Continuous),
                       tr("Interpolation"));
    m_bankTabs->addTab(makeBankPage(m_channel1DNameView, m_channel1DTable,
                                     PTTransitionMode::Channel1D),
                       tr("1D Channel FX"));
    m_bankTabs->addTab(makeBankPage(m_positionMotionNameView, m_positionMotionTable,
                                     PTTransitionMode::PositionMotion),
                       tr("Continuous Motion"));
    m_bankTabs->addTab(makeBankPage(m_multiFxNameView, m_multiFxTable, PTTransitionMode::MultiFx),
                       tr("MultiFX"));
    auto linkFrozenExpansion = [this](QTreeView* frozen, QTreeWidget* table,
                                      PTTransitionMode mode) {
        if (!frozen || !table)
            return;
        frozen->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(frozen, &QTreeView::expanded, this,
                [this, mode](const QModelIndex& idx) {
            if (!idx.isValid())
                return;
            const int row = idx.data(kItemPresetIndexRole).toInt();
            const int outputIdx = idx.data(kItemOutputIndexRole).toInt();
            const int selectionIdx = idx.data(kItemSelectionIndexRole).toInt();
            QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
            if (item)
                item->setExpanded(true);
            if (idx.data(kItemOutputIndexRole).toInt() < 0)
            {
                expandedSetForMode(mode).insert(row);
            }
        });
        connect(frozen, &QTreeView::collapsed, this,
                [this, mode](const QModelIndex& idx) {
            if (!idx.isValid())
                return;
            const int row = idx.data(kItemPresetIndexRole).toInt();
            const int outputIdx = idx.data(kItemOutputIndexRole).toInt();
            const int selectionIdx = idx.data(kItemSelectionIndexRole).toInt();
            QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
            if (item)
                item->setExpanded(false);
            if (idx.data(kItemOutputIndexRole).toInt() < 0)
            {
                expandedSetForMode(mode).remove(row);
            }
        });
        connect(frozen, &QTreeView::doubleClicked, this,
                [this, mode](const QModelIndex& idx) {
            if (!idx.isValid())
                return;
            const int row = idx.data(kItemPresetIndexRole).toInt();
            const int outputIdx = idx.data(kItemOutputIndexRole).toInt();
            const int selectionIdx = idx.data(kItemSelectionIndexRole).toInt();
            QTreeWidgetItem* item = itemForPresetAddress(mode, row, outputIdx, selectionIdx);
            if (item)
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

    QStringList headers;
    for (int c = 0; c < ColCount; ++c)
    {
        QString title = (c == ColName) ? tr("Name") : columnTitleForCol(c);
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
    normalizeOverrideStorage();
    for (int r = 0; r < presets.size(); ++r)
        normalizeNoopOverridesForPreset(mode, r);
    captureExpandedState(mode);
    const int scrollValue = table->verticalScrollBar() ? table->verticalScrollBar()->value() : 0;
    QTreeWidgetItem* currentBefore = table->currentItem();
    const int currentRow = currentBefore ? currentBefore->data(0, kItemPresetIndexRole).toInt() : -1;
    const int currentOutput = currentBefore ? currentBefore->data(0, kItemOutputIndexRole).toInt() : -1;
    const int currentSelection = currentBefore ? currentBefore->data(0, kItemSelectionIndexRole).toInt() : -1;
    QSignalBlocker tableBlocker(table);
    m_rebuildingTable = true;
    clearCellSelection(table);
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

    auto makeCell = [&](QTreeWidgetItem* item, int row, int outputIdx, int selectionIdx, int col,
                        const PTTransitionPreset& preset, bool inherited) {
        const QVariant value = presetColumnValue(preset, col);
        Q_UNUSED(row)
        Q_UNUSED(outputIdx)
        Q_UNUSED(selectionIdx)
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        setPresetCellValue(item, col, value, inherited);
        const QString tip = columnTooltipForCol(col);
        if (!tip.isEmpty())
            item->setToolTip(col, tip);
    };

    const int outputCount = linkedOutputCount();
    const auto& bankOverrides = overridesForMode(mode);
    for (int r = 0; r < presets.size(); ++r)
    {
        QTreeWidgetItem* parent = new QTreeWidgetItem(table);
        parent->setData(0, kItemPresetIndexRole, r);
        parent->setData(0, kItemOutputIndexRole, -1);
        parent->setData(0, kItemSelectionIndexRole, -1);
        parent->setFlags(parent->flags() | Qt::ItemIsEditable);
        parent->setText(ColName, presets[r].name);

        for (int col = ColAxis; col < ColCount; ++col)
            makeCell(parent, r, -1, -1, col, presets[r], false);
        updatePresetRowUiForItem(parent, mode);
        updateOffsetStepLimitForItem(parent, mode);

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

    if (outputIdx >= 0 && selectionIdx > 0)
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
    QTreeWidget* table = tableForMode(bankMode);
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

    if (linkedTableUsesPositionMode())
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
                                                      int selectionIdx)
{
    if (m_rebuildingTable)
        return;

    const QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    if (row < 0 || row >= presets.size())
        return;

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

    if (outputIdx >= 0)
    {
        updatePresetRowUiForItem(item, mode);
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
            setColumnOverrideValue(ov, col, presetFromItem(mode, item));
            layer.selections[sel].overrides = ov;
        }
        else
        {
            PTTransitionPresetOverride ov = layer.all;
            ov.values = effectivePresetForOutputNoLive(mode, row, outputIdx);
            setColumnOverrideValue(ov, col, presetFromItem(mode, item));
            layer.all = ov;
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
    if (col > ColName && col < ColCount)
    {
        slotPresetChanged(mode, row, col, outputIdx, selectionIdx);
        return;
    }
    if (col != ColName)
        return;
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
    QTreeWidget* table = tableForMode(mode);
    if (!table || !item)
        return;

    ScopedBoolFlag commitGuard(m_committingPresetCell);
    QSignalBlocker blocker(table);
    const PTTransitionPreset preset = presetFromItem(mode, item);
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

    const int outputIdx = item ? item->data(0, kItemOutputIndexRole).toInt() : -1;
    const int selectionIdx = item ? item->data(0, kItemSelectionIndexRole).toInt() : -1;
    const PTTransitionPreset storedPreset = selectionIdx > 0
            ? effectivePresetForSelectionNoLive(mode, row, outputIdx, selectionIdx - 1)
            : (outputIdx >= 0
               ? effectivePresetForOutputNoLive(mode, row, outputIdx)
               : presets.at(row));

    PTTransitionPreset preset = presetFromItem(mode, item);
    if (storedPreset.customCurveEnabled && storedPreset.customCurve.size() >= 2)
    {
        preset.customCurveEnabled = true;
        preset.customCurve = storedPreset.customCurve;
        preset.waveShape = storedPreset.waveShape;
    }

    const PTDimmerWaveParams params = PTDimmerWaveEngine::paramsFromPreset(preset, &m_globalSettings);
    const quint32 cycleMs = PTParamMatrixEngine::effectiveDurationMs(m_globalSettings, preset, false);
    PresetTableV2ControlIface* const linkedTableIface = linkedTable();
    const bool positionMode = linkedTableIface && linkedTableIface->tableUsesPositionMode();
    const bool morphTab = (mode == PTTransitionMode::SweepOnly);
    const bool showMorph = !positionMode || morphTab;
    const bool showMotion = positionMode && !morphTab;

    if (m_leftPreview)
        m_leftPreview->setVisible(showMorph || showMotion);
    if (QHBoxLayout* previewLayout = qobject_cast<QHBoxLayout*>(m_previewRow->layout()))
    {
        if (positionMode)
        {
            previewLayout->setStretch(0, 2);
            previewLayout->setStretch(1, 3);
        }
        else
        {
            previewLayout->setStretch(0, 3);
            previewLayout->setStretch(1, 2);
        }
    }

    if (m_curveLabel)
    {
        m_curveLabel->setVisible(showMorph);
        m_curveLabel->setText(positionMode
                ? tr("Position morph envelope")
                : tr("Dimmer wave"));
    }
    if (m_curveWidget)
    {
        m_curveWidget->setVisible(showMorph);
        if (showMorph)
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

        if (linkedTableIface)
        {
            PTSpatialGridPreview preview;
            const bool hasPreview = outputIdx >= 0
                    ? linkedTableIface->spatialGridPreviewForOutput(outputIdx, preset, m_globalSettings, preview)
                    : linkedTableIface->spatialGridPreview(preset, m_globalSettings, preview);
            if (hasPreview)
            {
                QList<QLCPoint> scopePoints;
                if (outputIdx < 0)
                {
                    scopePoints = linkedTableIface->fixtureGroupPoints();
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
                    scopePoints = linkedTableIface->outputPointsForPresetOverride(outputIdx);
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
            if (motion1D && m_positionMotion1DWidget)
            {
                m_positionMotionStack->setCurrentWidget(m_positionMotion1DWidget);
                PTDimmerWaveParams waveParams =
                        PTDimmerWaveEngine::paramsFromPreset(preset, &m_globalSettings);
                m_positionMotion1DWidget->setMotionPreviewFromPreset(
                        preset, waveParams, markers, cycleMs);
            }
            else if (m_positionPathWidget)
            {
                m_positionMotionStack->setCurrentWidget(m_positionPathWidget);
                if (PTPositionFxEngine::motionUsesCustomData(motion))
                    m_positionPathWidget->setOrbitPreviewFromPreset(preset, balls, cycleMs);
                else
                    m_positionPathWidget->setOrbitPreview(motion, 1.0, 1.0, balls, cycleMs);
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

    if (linkedTableIface)
    {
        PTSpatialGridPreview preview;
        const bool hasPreview = outputIdx >= 0
                ? linkedTableIface->spatialGridPreviewForOutput(outputIdx, preset, m_globalSettings, preview)
                : linkedTableIface->spatialGridPreview(preset, m_globalSettings, preview);
        if (hasPreview)
        {
            m_spatialGridWidget->setPreview(preview);
            QSet<QLCPoint> scopeCells;
            QVector<PTSpatialGridSelectionLayer> layers;

            auto addSelectionLayersForOutput = [&](int layerOutputIdx) {
                const QList<QLCPoint> scope =
                        linkedTableIface->outputPointsForPresetOverride(layerOutputIdx);
                QSet<QLCPoint> outputScope;
                for (const QLCPoint& pt : scope)
                {
                    outputScope.insert(pt);
                    scopeCells.insert(pt);
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
                addSelectionLayersForOutput(outputIdx);
            }
            else
            {
                const int count = linkedOutputCount();
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
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    presets.append(defaultPreset(presets.size(), mode));
    overrides.append(QHash<int, PTTransitionOutputLayer>());
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
}

void PresetTableV2TransitionWidget::slotAddSelection()
{
    const PTTransitionMode mode = activeBankMode();
    QTreeWidget* table = activeTable();
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    if (!item)
        return;
    const int row = item->data(0, kItemPresetIndexRole).toInt();
    int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    if (outputIdx < 0)
        return;

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
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
    QTreeWidget* table = tableForMode(mode);
    QTreeWidgetItem* item = table ? selectedPresetItem(table) : nullptr;
    if (!item)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();

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
    rebuildPresetTable(mode);
    notifyTablePresetCacheRefresh();
    updateRemoveActionLabel();
}

void PresetTableV2TransitionWidget::slotDuplicatePreset()
{
    const PTTransitionMode mode = activeBankMode();
    QVector<PTTransitionPreset>& presets = presetsForMode(mode);
    QVector<QHash<int, PTTransitionOutputLayer>>& overrides = overridesForMode(mode);
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
            ? overrides.at(row) : QHash<int, PTTransitionOutputLayer>());
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
    return table->currentItem();
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
    const int selectionIdx = item ? item->data(0, kItemSelectionIndexRole).toInt() : -1;
    const int outputIdx = item ? item->data(0, kItemOutputIndexRole).toInt() : -1;

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
}

void PresetTableV2TransitionWidget::refreshOverrideVisualsForPreset(PTTransitionMode mode,
                                                                    int row)
{
    QTreeWidgetItem* parent = parentItemForPreset(mode, row);
    if (!parent)
        return;

    refreshOverrideVisualsForItem(mode, parent);
    for (int i = 0; i < parent->childCount(); ++i)
    {
        refreshOverrideVisualsForItem(mode, parent->child(i));
        for (int j = 0; j < parent->child(i)->childCount(); ++j)
            refreshOverrideVisualsForItem(mode, parent->child(i)->child(j));
    }
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
    frozen->setSelectionMode(QAbstractItemView::ExtendedSelection);
    frozen->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    frozen->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozen->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    frozen->setStyleSheet(QStringLiteral(
        "QTreeView::item {"
        "  min-height: %1px;"
        "  border-right: 1px solid palette(mid);"
        "  border-bottom: 1px solid palette(mid);"
        "}").arg(kEfxTreeRowHeight));
    frozen->header()->setStretchLastSection(true);
    frozen->setColumnHidden(ColName, false);
    for (int c = ColAxis; c < ColCount; ++c)
        frozen->setColumnHidden(c, true);
    frozen->setFixedWidth(kFrozenNameWidth);
    frozen->installEventFilter(this);
    frozen->viewport()->installEventFilter(this);

    connect(table->verticalScrollBar(), &QScrollBar::valueChanged,
            frozen->verticalScrollBar(), &QScrollBar::setValue, Qt::UniqueConnection);
    connect(frozen->verticalScrollBar(), &QScrollBar::valueChanged,
            table->verticalScrollBar(), &QScrollBar::setValue, Qt::UniqueConnection);

    auto syncSelectionToFrozen = [frozen](const QItemSelection& selected,
                                          const QItemSelection& deselected) {
        Q_UNUSED(deselected);
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
                frozenSel->setCurrentIndex(frozenCur, QItemSelectionModel::NoUpdate);
        }
    };

    auto syncSelectionToTable = [table](const QItemSelection& selected,
                                        const QItemSelection& deselected) {
        Q_UNUSED(deselected);
        QItemSelectionModel* tableSel = table->selectionModel();
        if (!tableSel)
            return;
        QSignalBlocker blocker(tableSel);
        tableSel->clearSelection();
        QItemSelection tableSelection;
        for (const QModelIndex& idx : selected.indexes())
        {
            const QModelIndex nameIdx = idx.siblingAtColumn(ColName);
            if (nameIdx.isValid())
                tableSelection.select(nameIdx, nameIdx);
        }
        tableSel->select(tableSelection, QItemSelectionModel::Select);
        const QModelIndex cur = selected.indexes().isEmpty()
                ? QModelIndex() : selected.indexes().constFirst();
        if (cur.isValid())
        {
            const QModelIndex tableCur = cur.siblingAtColumn(ColName);
            if (tableCur.isValid())
                tableSel->setCurrentIndex(tableCur, QItemSelectionModel::NoUpdate);
        }
    };

    if (QItemSelectionModel* tableSel = table->selectionModel())
    {
        connect(tableSel, &QItemSelectionModel::selectionChanged,
                frozen, syncSelectionToFrozen);
    }
    if (QItemSelectionModel* frozenSel = frozen->selectionModel())
    {
        connect(frozenSel, &QItemSelectionModel::selectionChanged,
                table, syncSelectionToTable);
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

    QTreeWidgetItem* item = table->itemAt(pos);
    const int col = table->columnAt(pos.x());
    if (!item || col < ColName || col >= ColCount)
        return;

    const int row = item->data(0, kItemPresetIndexRole).toInt();
    const int outputIdx = item->data(0, kItemOutputIndexRole).toInt();
    const int selectionIdx = item->data(0, kItemSelectionIndexRole).toInt();
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
    {
        QMenu menu(this);
        QAction* addSelectionAct = nullptr;
        QAction* removeSelectionAct = nullptr;
        if (selectionIdx <= 0)
            addSelectionAct = menu.addAction(tr("Add selection"));
        else
        {
            removeSelectionAct = menu.addAction(tr("Remove selection"));
        }
        QAction* chosen = menu.exec(table->viewport()->mapToGlobal(pos));
        if (!chosen)
            return;

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

void PresetTableV2TransitionWidget::clearCellSelection(QTreeWidget* table)
{
    if (!table)
        return;
    m_selectedCellsByTable.remove(table);
    m_cellSelectionAnchorByTable.remove(table);
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
    pasteAct->setEnabled(!QApplication::clipboard()->text().trimmed().isEmpty()
                         && (clipCol < 0 || clipCol == col));
    menu.addSeparator();
    QAction* overrideAct = menu.addAction(tr("Override parameter"));
    QAction* resetParamAct = menu.addAction(tr("Reset parameter override"));
    QAction* resetAllAct = menu.addAction(selectionIdx > 0
            ? tr("Reset all overrides for this selection")
            : tr("Reset all overrides for All"));
    QAction* copyAllAct = selectionIdx > 0 ? nullptr
            : menu.addAction(tr("Copy output layer to all outputs"));
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

void PresetTableV2TransitionWidget::pasteCells(QTreeWidget* table)
{
    if (!table)
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
    snapshot.sweepOutputOverrides = m_sweepOutputOverrides;
    snapshot.continuousOutputOverrides = m_continuousOutputOverrides;
    snapshot.positionMotionOutputOverrides = m_positionMotionOutputOverrides;
    snapshot.channel1DOutputOverrides = m_channel1DOutputOverrides;
    snapshot.multiFxOutputOverrides = m_multiFxOutputOverrides;
    snapshot.globalSettings = m_globalSettings;
    snapshot.activeMode = activeBankMode();
    snapshot.enabled = !m_enableChk || m_enableChk->isChecked();
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

    if (PresetTableV2ControlIface* table = linkedTable())
        table->refreshTransitionPresetCache();
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

PresetTableV2ControlIface* PresetTableV2TransitionWidget::linkedTable() const
{
    if (m_disableLiveLinkedTableLookup)
        return nullptr;
    return PresetTableV2VCLookup::controlIfaceByVcId(m_targetTableId);
}

void PresetTableV2TransitionWidget::slotRefreshTableLink()
{
    PresetTableV2ControlIface* table = linkedTable();
    if (!table)
    {
        for (VCWidget* candidate : PresetTableV2VCLookup::allVcWidgets())
        {
            auto* candidateTable = qobject_cast<PresetTableV2ControlIface*>(candidate);
            if (candidateTable && candidateTable->linkedTransitionWidgetId() == id())
            {
                m_targetTableId = candidate->id();
                table = candidateTable;
                break;
            }
        }
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
            m_bankTabs->setTabText(1, positionMode ? tr("Interpolation") : tr("Continuous FX"));
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

    setInputSource(dlg.globalSpeedInputSource(), PTEfxCol::InputGlobalSpeed);
    setInputSource(dlg.globalIntensityInputSource(), PTEfxCol::InputGlobalIntensity);
    setInputSource(dlg.globalPositionSizeInputSource(), PTEfxCol::InputGlobalPositionSize);
    setInputSource(dlg.globalCrossfadeManualInputSource(), PTEfxCol::InputCrossfadeManual);
    m_crossfadeManualInputMapped = dlg.globalCrossfadeManualInputSource()
            && dlg.globalCrossfadeManualInputSource()->isValid();
    publishProviderSnapshot(QStringLiteral("properties"));
    updateGlobalSummaryLabel();
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
    copy->m_customCurveGallery = m_customCurveGallery;
    copy->m_shapeGallery = m_shapeGallery;
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

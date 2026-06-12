/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2widget.cpp — Apache 2.0 / public domain
*/

#include "presettablev2widget.h"
#include "presettablev2configdialog.h"
#include "presettablev2columndialog.h"
#include "presettablev2effectengine.h"
#include "ptdimmerwaveengine.h"
#include "ptparammatrixengine.h"
#include "ptspatialfixtureplan.h"
#include "ptpositionfixturegridwidget.h"
#include "ptpositionxypadwidget.h"
#include "ptpositionconverter.h"
#include "ptpositionfxengine.h"
#include "presettablev2transitionprovideriface.h"
#include "presettablev2inputids.h"
#include "ptefxinputids.h"
#include "virtualconsole.h"

#include "genericfader.h"
#include "fadechannel.h"
#include "mastertimer.h"
#include "universe.h"
#include "fixture.h"
#include "fixturegroup.h"
#include "fixturegroupmask.h"
#include "grouphead.h"
#include "qlcpoint.h"
#include "qlcchannel.h"
#include "qlccapability.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "qlcinputsource.h"
#include "inputoutputmap.h"
#include "doc.h"

#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutexLocker>
#include <QSet>
#include <QSpinBox>
#include <QComboBox>
#include <QStyleOptionViewItem>
#include <QHeaderView>
#include <QAction>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMenu>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMetaType>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QSignalBlocker>
#include <QTableView>
#include <QTimer>
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <climits>

// ---- Static colors for output badges -------------------------------------

namespace {

qreal positionSpreadSigned(int raw)
{
    raw = qBound(0, raw, 255);
    if (raw >= 128)
        return qreal(raw - 128) / 127.0;
    return qreal(raw - 128) / 128.0;
}

int positionSpreadLabelPct(int raw)
{
    raw = qBound(0, raw, 255);
    if (raw == 128)
        return 0;
    if (raw > 128)
        return qRound(qreal(raw - 128) * 100.0 / 127.0);
    return qRound(qreal(raw - 128) * 100.0 / 128.0);
}

} // namespace

const QColor PresetTableV2Widget::s_outputColors[8] = {
    QColor("#3a86ff"),   // 0 — blue
    QColor("#ff006e"),   // 1 — pink
    QColor("#ffbe0b"),   // 2 — amber
    QColor("#06d6a0"),   // 3 — teal
    QColor("#fb5607"),   // 4 — orange
    QColor("#8338ec"),   // 5 — purple
    QColor("#ef233c"),   // 6 — red
    QColor("#80b918"),   // 7 — lime
};

// ---- XML tag constants ---------------------------------------------------

static const QString KXMLRoot       = QStringLiteral("PluginWidget");
static const QString KXMLPluginId   = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal= QStringLiteral("org.qlcplus.vcwidgets.presettablev2");
static const QString KXMLColumn     = QStringLiteral("Column");
static const QString KXMLColIndex   = QStringLiteral("Index");
static const QString KXMLColName    = QStringLiteral("Name");
static const QString KXMLColType    = QStringLiteral("Type");
static const QString KXMLColFade    = QStringLiteral("Fade");
static const QString KXMLOption     = QStringLiteral("Option");
static const QString KXMLOptName     = QStringLiteral("Name");
static const QString KXMLOptValue    = QStringLiteral("Value");
static const QString KXMLOptResource = QStringLiteral("Resource");
static const QString KXMLRow        = QStringLiteral("Row");
static const QString KXMLRowIndex   = QStringLiteral("Index");
static const QString KXMLRowName    = QStringLiteral("Name");
static const QString KXMLV          = QStringLiteral("V");
static const QString KXMLOutput     = QStringLiteral("Output");
static const QString KXMLOutIndex   = QStringLiteral("Index");
static const QString KXMLOutName    = QStringLiteral("Name");
static const QString KXMLOutFxId    = QStringLiteral("FixtureID");
static const QString KXMLOutInput        = QStringLiteral("OutInput");
static const QString KXMLCrossfadeEn     = QStringLiteral("CrossfadeEnabled");
static const QString KXMLSyncMultiFxPhaseToCrossfade = QStringLiteral("SyncMultiFxPhaseToCrossfade");
static const QString KXMLMultiFxCrossfadeSyncOffsetMs = QStringLiteral("MultiFxCrossfadeSyncOffsetMs");
static const QString KXMLCrossfadeInput  = QStringLiteral("CrossfadeInput");
static const QString KXMLMultiFxBlendInput = QStringLiteral("MultiFxBlendInput");
static const QString KXMLMultiFxRestartInput = QStringLiteral("MultiFxRestartInput");
static const QString KXMLWidgetFlashGateInput = QStringLiteral("WidgetFlashGateInput");
static const QString KXMLWidgetFlashTimeMultiplier = QStringLiteral("WidgetFlashTimeMultiplier");
static const QString KXMLWidgetFlashBehavior = QStringLiteral("WidgetFlashBehavior");
static const QString KXMLSelectorStateOutput = QStringLiteral("SelectorStateOutput");
static const QString KXMLContinuousFxSelectorMode = QStringLiteral("ContinuousFxSelectorMode");
static const QString KXMLPositionConfirmDiscardDraft = QStringLiteral("PositionConfirmDiscardDraft");
static const QString KXMLPositionShowStatusStrip = QStringLiteral("PositionShowStatusStrip");
static const QString KXMLPositionShowEditorHints = QStringLiteral("PositionShowEditorHints");
static const QString KXMLColWidth        = QStringLiteral("Width");
static const QString KXMLNameColWidth    = QStringLiteral("NameColWidth");

// FixtureGroup mode XML constants
static const QString KXMLMode           = QStringLiteral("Mode");
static const QString KXMLFxGroupId      = QStringLiteral("FixtureGroupID");
static const QString KXMLOutRows        = QStringLiteral("Rows");
static const QString KXMLOutScope       = QStringLiteral("Scope");
static const QString KXMLOutSweepPreset = QStringLiteral("SweepPreset");
static const QString KXMLOutContinuousPreset = QStringLiteral("ContinuousPreset");
static const QString KXMLOutMultiFxPreset = QStringLiteral("MultiFxPreset");
static const QString KXMLOutTransitionPreset = QStringLiteral("TransitionPreset");
static const QString KXMLOutTransitionSecondary = QStringLiteral("TransitionSecondaryPreset");
static const QString KXMLOutSecondaryRow = QStringLiteral("SecondaryRow");
static const QString KXMLOutTransPrimaryInput = QStringLiteral("OutTransPrimaryInput");
static const QString KXMLOutTransSweepInput = QStringLiteral("OutTransSweepInput");
static const QString KXMLOutTransContinuousInput = QStringLiteral("OutTransContinuousInput");
static const QString KXMLOutTransSecondaryInput = QStringLiteral("OutTransSecondaryInput");
static const QString KXMLOutMultiFxInput = QStringLiteral("OutMultiFxInput");

struct PTInputBinding
{
    QSharedPointer<QLCInputSource> source;
    QKeySequence key;
};

static PTInputBinding readPTInputBlock(QXmlStreamReader& root, VCWidget* widget)
{
    PTInputBinding binding;
    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCVCWidgetInput)
        {
            binding.source = widget->getXMLInput(root);
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCVCWidgetKey)
        {
            binding.key = VCWidget::stripKeySequence(QKeySequence(root.readElementText()));
        }
        else
        {
            root.skipCurrentElement();
        }
    }
    return binding;
}

static void savePTInputBlock(QXmlStreamWriter* doc,
                             const QSharedPointer<QLCInputSource>& src,
                             const QKeySequence& key = QKeySequence())
{
    if (!src.isNull() && src->isValid())
        VCWidget::saveXMLInput(doc, src);
    if (!key.isEmpty())
        doc->writeTextElement(KXMLQLCVCWidgetKey, key.toString());
}

static const int kWidgetFlashTimeMultiplierMax = 6;

static double widgetFlashTimeMultiplierValue(int index)
{
    switch (index)
    {
        case 0: return 0.25;
        case 1: return 0.5;
        case 3: return 2.0;
        case 4: return 4.0;
        case 5: return 0.125;
        case 6: return 0.0625;
        case 2:
        default: return 1.0;
    }
}

static int cueListCrossfadeSliderValue(uchar value)
{
    // Match VCCueList: SCALE(raw 0..255 -> slider 0..100), then QSlider stores int.
    return qBound(0, int(value) * 100 / 255, 100);
}

static bool crossfadeAtLowEdge(uchar value)
{
    return cueListCrossfadeSliderValue(value) == 0;
}

static bool crossfadeAtHighEdge(uchar value)
{
    return cueListCrossfadeSliderValue(value) == 100;
}

static bool crossfadeAtTargetEdge(uchar value, bool stagedAtLowSide)
{
    return stagedAtLowSide ? crossfadeAtHighEdge(value) : crossfadeAtLowEdge(value);
}

static uchar crossfadeNormalizedEdge(uchar value)
{
    return crossfadeAtLowEdge(value) ? 0 : 255;
}

static bool crossfadeLowSideFromPosition(uchar value)
{
    if (crossfadeAtLowEdge(value))
        return true;
    if (crossfadeAtHighEdge(value))
        return false;
    return cueListCrossfadeSliderValue(value) <= 50;
}

static double cueListCrossfadeProgress01(uchar xfPos, uchar xfStartPos, bool stagedAtLowSide)
{
    const int pos = cueListCrossfadeSliderValue(xfPos);
    const int start = cueListCrossfadeSliderValue(xfStartPos);
    const int target = stagedAtLowSide ? 100 : 0;
    const int maxTravel = qAbs(target - start);
    if (maxTravel <= 0)
        return 0.0;

    const int traveled = stagedAtLowSide ? (pos - start) : (start - pos);
    return qBound(0.0, double(traveled) / double(maxTravel), 1.0);
}

static PTOutputScope scopeFromString(const QString& value)
{
    if (value == QLatin1String("Mask"))
        return PTOutputScope::Mask;
    if (value == QLatin1String("Rows"))
        return PTOutputScope::Rows;
    return PTOutputScope::RowsAndMask;
}

static QString scopeToString(PTOutputScope scope)
{
    switch (scope)
    {
        case PTOutputScope::Mask:
            return QStringLiteral("Mask");
        case PTOutputScope::Rows:
            return QStringLiteral("Rows");
        default:
            return QStringLiteral("RowsAndMask");
    }
}

static QString continuousFxSelectorModeToString(PTContinuousFxSelectorMode mode)
{
    switch (mode)
    {
        case PTContinuousFxSelectorMode::Live:
            return QStringLiteral("Live");
        case PTContinuousFxSelectorMode::SmoothMorph:
            return QStringLiteral("SmoothMorph");
        case PTContinuousFxSelectorMode::StagedCommit:
        default:
            return QStringLiteral("StagedCommit");
    }
}

static PTContinuousFxSelectorMode continuousFxSelectorModeFromString(const QString& value)
{
    if (value == QLatin1String("Live"))
        return PTContinuousFxSelectorMode::Live;
    if (value == QLatin1String("SmoothMorph"))
        return PTContinuousFxSelectorMode::SmoothMorph;
    return PTContinuousFxSelectorMode::StagedCommit;
}

static QString widgetFlashBehaviorToString(PTWidgetFlashBehavior behavior)
{
    switch (behavior)
    {
        case PTWidgetFlashBehavior::StagedRowTrigger:
            return QStringLiteral("StagedRowTrigger");
        case PTWidgetFlashBehavior::PrimaryRowModifier:
        default:
            return QStringLiteral("PrimaryRowModifier");
    }
}

static PTWidgetFlashBehavior widgetFlashBehaviorFromString(const QString& value)
{
    if (value == QLatin1String("StagedRowTrigger"))
        return PTWidgetFlashBehavior::StagedRowTrigger;
    return PTWidgetFlashBehavior::PrimaryRowModifier;
}

static bool outputScopeAllowsPoint(PTOutputScope scope, const QLCPoint& pt, const PTOutput& out)
{
    switch (scope)
    {
        case PTOutputScope::Rows:
            if (out.groupRows.isEmpty())
                return true;
            return out.groupRows.contains(pt.y());
        case PTOutputScope::Mask:
            return true;
        default:
            if (out.groupRows.isEmpty())
                return true;
            return out.groupRows.contains(pt.y());
    }
}

struct PTOutputScopeFixture
{
    quint32   fxiId = UINT_MAX;
    GroupHead head;
    QLCPoint  point;
};

struct PTResolvedPositionChannels
{
    int pan = -1;
    int panFine = -1;
    int tilt = -1;
    int tiltFine = -1;
    bool valid() const { return pan >= 0 && tilt >= 0; }
};

static PTResolvedPositionChannels resolvePositionChannelsForFixture(Fixture* fxi);

static QList<PTOutputScopeFixture> collectOutputScopeFixtures(
        const QMap<QLCPoint, GroupHead>& headsMap, const PTOutput& out)
{
    QList<PTOutputScopeFixture> result;
    QSet<quint32> seen;
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        const QLCPoint& pt = it.key();
        if (!outputScopeAllowsPoint(out.scope, pt, out))
            continue;
        const GroupHead& head = it.value();
        if (seen.contains(head.fxi))
            continue;
        seen.insert(head.fxi);
        PTOutputScopeFixture entry;
        entry.fxiId = head.fxi;
        entry.head = head;
        entry.point = pt;
        result.append(entry);
    }
    return result;
}

static bool bindingMatchesFixture(const PTColumnTypeBinding& binding, Fixture* fxi)
{
    if (!binding.isValid() || !fxi)
        return false;
    QLCFixtureDef* fxDef = fxi->fixtureDef();
    QLCFixtureMode* fxMode = fxi->fixtureMode();
    if (!fxDef || !fxMode)
        return false;
    if (fxDef->manufacturer() != binding.manufacturer)
        return false;
    if (fxDef->model() != binding.model)
        return false;
    if (fxMode->name() != binding.modeName)
        return false;
    const quint32 absChannel = quint32(binding.channelIndex);
    return absChannel < fxi->channels();
}

static bool columnBindingMatchesFixture(const PTColumn& col, Fixture* fxi)
{
    for (const PTColumnTypeBinding& binding : col.bindings)
    {
        if (bindingMatchesFixture(binding, fxi))
            return true;
    }
    return false;
}

static const QString KXMLBindMfg        = QStringLiteral("BindMfg");
static const QString KXMLBinding        = QStringLiteral("Binding");
static const QString KXMLBindModel      = QStringLiteral("BindModel");
static const QString KXMLBindMode       = QStringLiteral("BindMode");
static const QString KXMLBindChan       = QStringLiteral("BindChan");
static const QString KXMLColScalerMin   = QStringLiteral("ScalerMin");
static const QString KXMLColScalerMax   = QStringLiteral("ScalerMax");
static const QString KXMLColScalerSfx   = QStringLiteral("ScalerSuffix");
static const QString KXMLSpatialEn      = QStringLiteral("SpatialEnabled");
static const QString KXMLSpatialOrder   = QStringLiteral("SpatialOrder");
static const QString KXMLSpatialStepMs  = QStringLiteral("SpatialStepMs");
static const QString KXMLSpatialFadeMs  = QStringLiteral("SpatialFadeMs");
static const QString KXMLSpatialReverse = QStringLiteral("SpatialReverse");
static const QString KXMLLinkedTransition = QStringLiteral("LinkedTransitionWidgetId");
static const QString KXMLPosition       = QStringLiteral("Position");
static const QString KXMLPositionX      = QStringLiteral("X");
static const QString KXMLPositionY      = QStringLiteral("Y");
static const QString KXMLPositionPan    = QStringLiteral("Pan");
static const QString KXMLPositionTilt   = QStringLiteral("Tilt");
static const QString KXMLPositionPanDeg = QStringLiteral("PanDeg");
static const QString KXMLPositionTiltDeg= QStringLiteral("TiltDeg");
static const QString KXMLPositionOverride = QStringLiteral("PositionOverride");
static const QString KXMLPosOvRow       = QStringLiteral("Row");
static const QString KXMLPosOvOutput    = QStringLiteral("Output");
static const QString KXMLPosOvSelection = QStringLiteral("Selection");
static const QString KXMLPosOvSelName   = QStringLiteral("SelName");
static const QString KXMLPosOvSelColor  = QStringLiteral("SelColor");

// ==========================================================================
// Static display helpers (shared by delegate + rebuildTable + pasteValueToItem)
// ==========================================================================

// Returns (displayText, resource) for a Dropdown column given a raw DMX value.
// Exact match in options → that option's name + resource.
// No match → bare number as string + empty resource.
static QPair<QString,QString> optionLabelFor(const PTColumn& col, int dmxVal)
{
    for (const PTOption& opt : col.options)
        if (int(opt.value) == dmxVal)
            return { opt.name, opt.resource };
    return { QString::number(dmxVal), QString() };
}

// Builds a 16×16 QIcon from a resource string ("#rrggbb" colour or image path).
static QIcon makeItemIcon(const QString& resource)
{
    if (resource.isEmpty()) return QIcon();
    QPixmap pm;
    if (resource.startsWith(QLatin1Char('#')))
    {
        pm = QPixmap(16, 16);
        pm.fill(QColor(resource));
    }
    else
    {
        pm.load(resource);
        if (!pm.isNull())
            pm = pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return pm.isNull() ? QIcon() : QIcon(pm);
}

// Scaler ↔ DMX conversion
static inline int dmxToScaler(int dmx, int sMin, int sMax)
{
    return sMin + qRound(dmx * double(sMax - sMin) / 255.0);
}
static inline int scalerToDmx(int v, int sMin, int sMax)
{
    if (sMax == sMin) return 0;
    return qBound(0, qRound((v - sMin) * 255.0 / (sMax - sMin)), 255);
}

// ==========================================================================
// PresetTableV2Delegate
// ==========================================================================

PresetTableV2Delegate::PresetTableV2Delegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void PresetTableV2Delegate::setColumns(const QVector<PTColumn>* columns)
{
    m_columns = columns;
}

void PresetTableV2Delegate::setOwner(const PresetTableV2Widget* owner)
{
    m_owner = owner;
}

QWidget* PresetTableV2Delegate::createEditor(QWidget* parent,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
        return QStyledItemDelegate::createEditor(parent, option, index);

    int valCol = col - 1;  // value column index
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return nullptr;

    const PTColumn& ptcol = (*m_columns)[valCol];

    // Scaler: dedicated range spinbox with optional suffix
    if (ptcol.type == PTColumn::Scaler)
    {
        QSpinBox* sb = new QSpinBox(parent);
        sb->setRange(ptcol.scalerMin, ptcol.scalerMax);
        if (!ptcol.scalerSuffix.isEmpty())
            sb->setSuffix(ptcol.scalerSuffix);
        sb->setFrame(false);
        return sb;
    }

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = new QComboBox(parent);
        cb->setEditable(true);
        cb->lineEdit()->setValidator(new QIntValidator(0, 255, cb));
        cb->setInsertPolicy(QComboBox::NoInsert);
        for (const PTOption& opt : ptcol.options)
            cb->addItem(makeItemIcon(opt.resource), opt.name, int(opt.value));
        return cb;
    }

    // Capability-aware editable combo: only in Dropdown mode, with a resolved channel
    const QLCChannel* chan = (ptcol.type == PTColumn::Dropdown && m_owner)
                             ? m_owner->resolveBoundChannel(ptcol) : nullptr;
    if (chan && !chan->capabilities().isEmpty())
    {
        QComboBox* cb = new QComboBox(parent);
        cb->setEditable(true);
        cb->lineEdit()->setValidator(new QIntValidator(0, 255, cb));
        cb->setInsertPolicy(QComboBox::NoInsert);
        for (QLCCapability* cap : chan->capabilities())
        {
            QString label = QString("%1-%2: %3")
                .arg(int(cap->min()), 3, 10, QChar('0'))
                .arg(int(cap->max()), 3, 10, QChar('0'))
                .arg(cap->name());
            cb->addItem(label, int(cap->min()));
        }
        return cb;
    }

    // Fallback: plain numeric spinbox
    QSpinBox* sb = new QSpinBox(parent);
    sb->setRange(0, 255);
    sb->setFrame(false);
    return sb;
}

void PresetTableV2Delegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
    {
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    }

    int valCol = col - 1;
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return;

    const PTColumn& ptcol = (*m_columns)[valCol];
    int stored = index.data(Qt::UserRole).toInt();

    if (ptcol.type == PTColumn::Scaler)
    {
        QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
        if (sb) sb->setValue(dmxToScaler(stored, ptcol.scalerMin, ptcol.scalerMax));
        return;
    }

    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        // Pre-select the matching option (for visual context)
        for (int i = 0; i < cb->count(); ++i)
        {
            if (cb->itemData(i).toInt() == stored)
            {
                cb->setCurrentIndex(i);
                break;
            }
        }
        // Show exact DMX value so user sees / edits the precise number
        cb->lineEdit()->setText(QString::number(stored));
        return;
    }

    // Capability editable combo: show the exact stored DMX value in the line edit
    QComboBox* cb = qobject_cast<QComboBox*>(editor);
    if (cb && cb->isEditable())
    {
        for (int i = 0; i < cb->count(); ++i)
        {
            int capMin = cb->itemData(i).toInt();
            int capMax = (i + 1 < cb->count()) ? cb->itemData(i + 1).toInt() - 1 : 255;
            if (stored >= capMin && stored <= capMax)
            {
                cb->setCurrentIndex(i);
                break;
            }
        }
        cb->lineEdit()->setText(QString::number(stored));
        return;
    }

    QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
    if (sb) sb->setValue(stored);
}

void PresetTableV2Delegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                        const QModelIndex& index) const
{
    int col = index.column();
    if (col == 0)
    {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    int valCol = col - 1;
    if (!m_columns || valCol < 0 || valCol >= m_columns->size())
        return;

    const PTColumn& ptcol = (*m_columns)[valCol];

    // ---- Scaler --------------------------------------------------------
    if (ptcol.type == PTColumn::Scaler)
    {
        QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
        if (!sb) return;
        int dmx = scalerToDmx(sb->value(), ptcol.scalerMin, ptcol.scalerMax);
        model->setData(index, dmx, Qt::UserRole);
        model->setData(index,
            QString("%1%2").arg(sb->value()).arg(ptcol.scalerSuffix),
            Qt::DisplayRole);
        model->setData(index, QVariant(), Qt::DecorationRole);
        return;
    }

    // ---- Dropdown with PTOption list -----------------------------------
    if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
    {
        QComboBox* cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        bool ok = false;
        int val = cb->lineEdit()->text().toInt(&ok);
        if (!ok) val = cb->currentData().toInt();
        val = qBound(0, val, 255);

        auto lbl = optionLabelFor(ptcol, val);
        model->setData(index, val, Qt::UserRole);
        model->setData(index, lbl.first, Qt::DisplayRole);
        QIcon ico = makeItemIcon(lbl.second);
        model->setData(index, ico.isNull() ? QVariant() : QVariant(ico), Qt::DecorationRole);
        return;
    }

    // ---- Capability editable combo (no PTOption, binding has caps) -----
    QComboBox* cb = qobject_cast<QComboBox*>(editor);
    if (cb && cb->isEditable())
    {
        bool ok = false;
        int val = cb->lineEdit()->text().toInt(&ok);
        if (!ok) val = cb->currentData().toInt();
        val = qBound(0, val, 255);

        const QLCChannel* chan = m_owner ? m_owner->resolveBoundChannel(ptcol) : nullptr;
        QString display = QString::number(val);
        if (chan)
        {
            QLCCapability* cap = chan->searchCapability(uchar(val));
            if (cap) display = cap->name();
        }
        model->setData(index, val, Qt::UserRole);
        model->setData(index, display, Qt::DisplayRole);
        model->setData(index, QVariant(), Qt::DecorationRole);
        return;
    }

    // ---- Numeric spinbox -----------------------------------------------
    QSpinBox* sb = qobject_cast<QSpinBox*>(editor);
    if (!sb) return;
    model->setData(index, sb->value(), Qt::UserRole);
    model->setData(index, QString::number(sb->value()), Qt::DisplayRole);
    model->setData(index, QVariant(), Qt::DecorationRole);
}

void PresetTableV2Delegate::updateEditorGeometry(QWidget* editor,
                                                const QStyleOptionViewItem& option,
                                                const QModelIndex& /*index*/) const
{
    if (editor)
        editor->setGeometry(option.rect);
}

void PresetTableV2Delegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    // Draw background selection
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    else
        painter->fillRect(option.rect, option.backgroundBrush);

    QString text = index.data(Qt::DisplayRole).toString();
    QIcon   ico  = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

    const int iconSize = 16;
    const int iconPad  = 2;
    int textLeft = 4;

    if (!ico.isNull())
    {
        QPixmap pm = ico.pixmap(iconSize, iconSize);
        int y = option.rect.top() + (option.rect.height() - pm.height()) / 2;
        painter->drawPixmap(option.rect.left() + 2, y, pm);
        textLeft = 2 + iconSize + iconPad + 2;
    }

    painter->setPen(option.palette.color(QPalette::Text));
    painter->drawText(option.rect.adjusted(textLeft, 0, -2, 0),
                      Qt::AlignVCenter | Qt::AlignLeft, text);
}

QSize PresetTableV2Delegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& /*index*/) const
{
    return QSize(60, option.fontMetrics.height() + 8);
}

// ==========================================================================
// PresetTableV2Widget — construction
// ==========================================================================

PresetTableV2Widget::PresetTableV2Widget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    qRegisterMetaType<PTPositionTreeRef>();
    setObjectName(PresetTableV2Widget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Preset Table v2"));
    resize(QSize(400, 260));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);

    // ---- Toolbar (Design mode only) --------------------------------------
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setMovable(false);

    QAction* actAddRow    = m_toolbar->addAction(tr("+ Row"));
    QAction* actRemRow    = m_toolbar->addAction(tr("- Row"));
    m_actColSep  = m_toolbar->addSeparator();
    QAction* actAddCol = m_toolbar->addAction(tr("+ Col"));
    QAction* actRemCol = m_toolbar->addAction(tr("- Col"));
    m_actPropSep = m_toolbar->addSeparator();
    QAction* actProps  = m_toolbar->addAction(tr("Properties..."));

    m_actAddCol = actAddCol;
    m_actRemCol = actRemCol;
    m_actProps  = actProps;

    connect(actAddRow, &QAction::triggered, this, &PresetTableV2Widget::slotAddRow);
    connect(actRemRow, &QAction::triggered, this, &PresetTableV2Widget::slotRemoveRow);
    connect(actAddCol, &QAction::triggered, this, &PresetTableV2Widget::slotAddColumn);
    connect(actRemCol, &QAction::triggered, this, &PresetTableV2Widget::slotRemoveColumn);
    connect(actProps,  &QAction::triggered, this, &PresetTableV2Widget::slotProperties);

    m_layout->addWidget(m_toolbar);

    // ---- Table -----------------------------------------------------------
    m_delegate = new PresetTableV2Delegate(this);
    m_delegate->setColumns(&m_columns);
    m_delegate->setOwner(this);

    m_table = new QTableWidget(0, 1, this);  // start: 0 rows, 1 col (Name)
    m_table->setHorizontalHeaderLabels(QStringList() << tr("Name"));
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->verticalHeader()->setDefaultSectionSize(22);
    m_table->verticalHeader()->hide();
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setAlternatingRowColors(true);
    m_table->setItemDelegate(m_delegate);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    m_nameFrozenTable = new QTableView(this);
    m_nameFrozenTable->setModel(m_table->model());
    m_nameFrozenTable->setSelectionModel(m_table->selectionModel());
    m_nameFrozenTable->setItemDelegate(m_delegate);
    m_nameFrozenTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_nameFrozenTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_nameFrozenTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_nameFrozenTable->setAlternatingRowColors(true);
    m_nameFrozenTable->verticalHeader()->hide();
    m_nameFrozenTable->verticalHeader()->setDefaultSectionSize(22);
    m_nameFrozenTable->horizontalHeader()->setStretchLastSection(true);
    m_nameFrozenTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nameFrozenTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nameFrozenTable->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_nameFrozenTable->setFixedWidth((m_nameColWidth > 0 ? m_nameColWidth : 140) + 2);
    m_nameFrozenTable->installEventFilter(this);
    m_nameFrozenTable->viewport()->installEventFilter(this);
    for (int col = 1; col < m_table->columnCount(); ++col)
        m_nameFrozenTable->setColumnHidden(col, true);

    // Intercept ShortcutOverride so Ctrl+C/V reach us instead of VC's widget-copy actions
    m_table->installEventFilter(this);
    m_table->viewport()->installEventFilter(this);

    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_table, &QTableWidget::cellChanged,
            this, &PresetTableV2Widget::slotCellChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &PresetTableV2Widget::slotColumnHeaderDoubleClicked);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized,
            this, &PresetTableV2Widget::slotHeaderSectionResized);
    connect(m_nameFrozenTable->horizontalHeader(), &QHeaderView::sectionResized,
            this, [this](int logicalIndex, int, int newSize) {
        if (logicalIndex != 0 || m_resizingColumns)
            return;
        m_nameColWidth = newSize;
        m_nameFrozenTable->setFixedWidth(newSize + 2);
    });
    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &PresetTableV2Widget::slotTableContextMenu);
    connect(m_table->verticalScrollBar(), &QScrollBar::valueChanged,
            m_nameFrozenTable->verticalScrollBar(), &QScrollBar::setValue);
    connect(m_nameFrozenTable->verticalScrollBar(), &QScrollBar::valueChanged,
            m_table->verticalScrollBar(), &QScrollBar::setValue);

    m_tableWrap = new QWidget(this);
    QHBoxLayout* tableLayout = new QHBoxLayout(m_tableWrap);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    tableLayout->addWidget(m_nameFrozenTable);
    tableLayout->addWidget(m_table, 1);

    m_positionRowListPanel = new QWidget(this);
    QVBoxLayout* rowListLayout = new QVBoxLayout(m_positionRowListPanel);
    rowListLayout->setContentsMargins(4, 4, 4, 4);
    rowListLayout->setSpacing(4);
    QLabel* rowListTitle = new QLabel(tr("Position presets"), m_positionRowListPanel);
    rowListLayout->addWidget(rowListTitle);
    m_positionPresetTree = new QTreeWidget(m_positionRowListPanel);
    m_positionPresetTree->setHeaderHidden(true);
    m_positionPresetTree->setRootIsDecorated(true);
    m_positionPresetTree->setIndentation(14);
    rowListLayout->addWidget(m_positionPresetTree, 1);
    m_positionRowListPanel->setMinimumWidth(200);
    m_positionRowListPanel->setMaximumWidth(280);

    m_positionEditorPanel = new QWidget(this);
    QVBoxLayout* posEditorLayout = new QVBoxLayout(m_positionEditorPanel);
    posEditorLayout->setContentsMargins(4, 4, 4, 4);
    posEditorLayout->setSpacing(6);

    m_positionGrid = new PTPositionFixtureGridWidget(m_positionEditorPanel);
    m_positionXYPad = new PTPositionXYPadWidget(m_positionEditorPanel);

    m_positionValueStrip = new QLabel(m_positionEditorPanel);
    m_positionValueStrip->setWordWrap(true);
    m_positionValueStrip->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont stripFont = m_positionValueStrip->font();
    stripFont.setPointSize(qMax(8, stripFont.pointSize() - 1));
    m_positionValueStrip->setFont(stripFont);
    posEditorLayout->addWidget(m_positionValueStrip);

    m_positionHintLabel = new QLabel(m_positionEditorPanel);
    m_positionHintLabel->setWordWrap(true);
    m_positionHintLabel->setFont(stripFont);
    m_positionHintLabel->setText(tr("Symmetric spread: encoder center = no spread · "
                                    "double-click spread = reset center · "
                                    "Pan: ends opposite · Tilt: ends equal. "
                                    "Crossfade: Transition sweep preset."));
    posEditorLayout->addWidget(m_positionHintLabel);

    m_positionGrid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_positionXYPad->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_positionPanSpin = new QDoubleSpinBox(m_positionEditorPanel);
    m_positionPanSpin->setSuffix(QStringLiteral("°"));
    m_positionPanSpin->setDecimals(1);
    m_positionPanSpin->setRange(-10000, 10000);
    m_positionTiltSpin = new QDoubleSpinBox(m_positionEditorPanel);
    m_positionTiltSpin->setSuffix(QStringLiteral("°"));
    m_positionTiltSpin->setDecimals(1);
    m_positionTiltSpin->setRange(-10000, 10000);

    QWidget* padColumn = new QWidget(m_positionEditorPanel);
    QVBoxLayout* padColumnLayout = new QVBoxLayout(padColumn);
    padColumnLayout->setContentsMargins(0, 0, 0, 0);
    padColumnLayout->setSpacing(4);
    padColumnLayout->addWidget(m_positionXYPad, 1);
    QHBoxLayout* panRow = new QHBoxLayout();
    panRow->addWidget(new QLabel(tr("Pan"), padColumn));
    panRow->addWidget(m_positionPanSpin, 1);
    padColumnLayout->addLayout(panRow);
    QHBoxLayout* tiltRow = new QHBoxLayout();
    tiltRow->addWidget(new QLabel(tr("Tilt"), padColumn));
    tiltRow->addWidget(m_positionTiltSpin, 1);
    padColumnLayout->addLayout(tiltRow);

    QHBoxLayout* gridPadRow = new QHBoxLayout();
    gridPadRow->setSpacing(6);
    gridPadRow->addWidget(m_positionGrid, 3);
    gridPadRow->addWidget(padColumn, 2);
    posEditorLayout->addLayout(gridPadRow);

    m_positionSymmetricSpreadCheck = new QCheckBox(
            tr("Symmetric spread (multi-select)"), m_positionEditorPanel);
    m_positionSymmetricSpreadCheck->setEnabled(false);
    m_positionSymmetricSpreadCheck->setToolTip(
            tr("Symmetric spread: encoder center = no spread · "
               "double-click spread slider = reset center · "
               "Pan: ends opposite · Tilt: ends equal."));
    posEditorLayout->addWidget(m_positionSymmetricSpreadCheck);

    QHBoxLayout* spreadRow = new QHBoxLayout();
    spreadRow->addWidget(new QLabel(tr("Spread"), m_positionEditorPanel));
    m_positionSpreadAxisCombo = new QComboBox(m_positionEditorPanel);
    m_positionSpreadAxisCombo->addItem(tr("Pan"));
    m_positionSpreadAxisCombo->addItem(tr("Tilt"));
    m_positionSpreadAxisCombo->setEnabled(false);
    m_positionSpreadSlider = new QSlider(Qt::Horizontal, m_positionEditorPanel);
    m_positionSpreadSlider->setRange(0, 255);
    m_positionSpreadSlider->setValue(128);
    m_positionSpreadSlider->setEnabled(false);
    m_positionSpreadValueLabel = new QLabel(QStringLiteral("0%"), m_positionEditorPanel);
    m_positionSpreadValueLabel->setMinimumWidth(40);
    spreadRow->addWidget(m_positionSpreadAxisCombo);
    spreadRow->addWidget(m_positionSpreadSlider, 1);
    spreadRow->addWidget(m_positionSpreadValueLabel);
    posEditorLayout->addLayout(spreadRow);
    m_positionSpreadSlider->installEventFilter(this);

    QHBoxLayout* toolRow = new QHBoxLayout();
    QPushButton* copyBtn = new QPushButton(tr("Copy"), m_positionEditorPanel);
    QPushButton* pasteBtn = new QPushButton(tr("Paste"), m_positionEditorPanel);
    QPushButton* clearBtn = new QPushButton(tr("Clear"), m_positionEditorPanel);
    m_positionCopyLayerBtn = new QPushButton(tr("Copy layer"), m_positionEditorPanel);
    m_positionPasteLayerBtn = new QPushButton(tr("Paste layer"), m_positionEditorPanel);
    m_positionPasteLayerBtn->setEnabled(false);
    m_positionOverwriteBtn = new QPushButton(tr("Overwrite"), m_positionEditorPanel);
    m_positionSaveAsBtn = new QPushButton(tr("Save As"), m_positionEditorPanel);
    m_positionRevertBtn = new QPushButton(tr("Revert"), m_positionEditorPanel);
    m_positionOverwriteBtn->setEnabled(false);
    m_positionSaveAsBtn->setEnabled(false);
    m_positionRevertBtn->setEnabled(false);
    toolRow->addWidget(copyBtn);
    toolRow->addWidget(pasteBtn);
    toolRow->addWidget(clearBtn);
    toolRow->addWidget(m_positionCopyLayerBtn);
    toolRow->addWidget(m_positionPasteLayerBtn);
    toolRow->addStretch(1);
    toolRow->addWidget(m_positionOverwriteBtn);
    toolRow->addWidget(m_positionSaveAsBtn);
    toolRow->addWidget(m_positionRevertBtn);
    posEditorLayout->addLayout(toolRow);
    posEditorLayout->addStretch(1);

    m_positionValueStrip->setMaximumHeight(48);

    m_positionSplitter = new QSplitter(Qt::Horizontal, this);
    m_positionSplitter->addWidget(m_positionRowListPanel);
    m_positionSplitter->addWidget(m_tableWrap);
    m_positionSplitter->addWidget(m_positionEditorPanel);
    m_positionSplitter->setStretchFactor(0, 0);
    m_positionSplitter->setStretchFactor(1, 3);
    m_positionSplitter->setStretchFactor(2, 4);
    m_positionRowListPanel->setVisible(false);
    m_positionEditorPanel->setVisible(false);
    m_layout->addWidget(m_positionSplitter, 1);

    connect(m_positionGrid, &PTPositionFixtureGridWidget::selectionChanged,
            this, &PresetTableV2Widget::slotPositionGridSelectionChanged);
    connect(m_positionGrid, &PTPositionFixtureGridWidget::copyRequested,
            this, &PresetTableV2Widget::copyPositionSelectionToClipboard);
    connect(m_positionGrid, &PTPositionFixtureGridWidget::pasteRequested,
            this, &PresetTableV2Widget::pastePositionClipboardToSelection);
    connect(m_positionGrid, &PTPositionFixtureGridWidget::clearRequested,
            this, &PresetTableV2Widget::clearPositionSelectionToDraft);
    connect(m_positionGrid, &PTPositionFixtureGridWidget::cellEditRequested,
            this, &PresetTableV2Widget::slotPositionGridCellEditRequested);
    connect(m_positionPresetTree, &QTreeWidget::currentItemChanged,
            this, &PresetTableV2Widget::slotPositionPresetTreeChanged);
    connect(m_positionPresetTree, &QTreeWidget::itemDoubleClicked,
            this, &PresetTableV2Widget::slotPositionPresetTreeDoubleClicked);
    connect(m_positionXYPad, &PTPositionXYPadWidget::positionChanged,
            this, &PresetTableV2Widget::slotPositionXYPadChanged);
    connect(m_positionPanSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PresetTableV2Widget::slotPositionPanSpinChanged);
    connect(m_positionTiltSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PresetTableV2Widget::slotPositionTiltSpinChanged);
    connect(m_positionSymmetricSpreadCheck, &QCheckBox::toggled,
            this, &PresetTableV2Widget::slotPositionSymmetricSpreadToggled);
    connect(m_positionSpreadSlider, &QSlider::valueChanged,
            this, &PresetTableV2Widget::slotPositionSpreadChanged);
    connect(m_positionSpreadAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresetTableV2Widget::slotPositionSpreadAxisChanged);
    connect(copyBtn, &QPushButton::clicked, this, &PresetTableV2Widget::slotPositionCopyCell);
    connect(pasteBtn, &QPushButton::clicked, this, &PresetTableV2Widget::slotPositionPasteCell);
    connect(clearBtn, &QPushButton::clicked, this, &PresetTableV2Widget::slotPositionClearCell);
    connect(m_positionCopyLayerBtn, &QPushButton::clicked,
            this, &PresetTableV2Widget::slotPositionCopyLayer);
    connect(m_positionPasteLayerBtn, &QPushButton::clicked,
            this, &PresetTableV2Widget::slotPositionPasteLayer);
    connect(m_positionOverwriteBtn, &QPushButton::clicked,
            this, &PresetTableV2Widget::slotPositionOverwrite);
    connect(m_positionSaveAsBtn, &QPushButton::clicked,
            this, &PresetTableV2Widget::slotPositionSaveAs);
    connect(m_positionRevertBtn, &QPushButton::clicked,
            this, &PresetTableV2Widget::slotPositionRevert);
    connect(m_table, &QTableWidget::currentCellChanged,
            this, &PresetTableV2Widget::slotTableCurrentCellChanged);

    // ---- Status bar (Operate mode only) ----------------------------------
    m_statusBar = new QLabel(this);
    m_statusBar->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont sf = m_statusBar->font();
    sf.setPointSize(7);
    m_statusBar->setFont(sf);
    m_statusBar->setVisible(false);
    m_layout->addWidget(m_statusBar);

    setLayout(m_layout);

    if (m_doc != nullptr)
    {
        connect(m_doc, SIGNAL(fixtureGroupMaskChanged(quint32)),
                this, SLOT(slotFixtureGroupMaskChanged(quint32)));
    }
}

PresetTableV2Widget::~PresetTableV2Widget()
{
    if (m_doc && m_doc->masterTimer())
        m_doc->masterTimer()->unregisterDMXSource(this);

    QMutexLocker lk(&m_stateMutex);
    for (auto& fader : m_faders)
        if (!fader.isNull()) fader->requestDelete();
    m_faders.clear();
}

// ==========================================================================
// Data setters
// ==========================================================================

void PresetTableV2Widget::setColumns(const QVector<PTColumn>& cols)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_columns = cols;
        // Resize row value vectors
        for (PTRow& row : m_rows)
            row.values.resize(m_columns.size(), 0);
    }
    rebuildTable();
}

void PresetTableV2Widget::setRows(const QVector<PTRow>& rows)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_rows = rows;
        for (PTRow& row : m_rows)
            row.values.resize(m_columns.size(), 0);
        // Reset active rows
        m_activeRow.fill(-1, m_outputs.size());
        m_stagedRow.fill(-1, m_outputs.size());
        m_stagedRowValid.fill(false, m_outputs.size());
    }
    rebuildTable();
}

void PresetTableV2Widget::setOutputs(const QVector<PTOutput>& outs)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_outputs = outs;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
        m_stagedRowValid.resize(m_outputs.size());
        m_stagedRowValid.fill(false);
        m_stagedSecondaryRow.resize(m_outputs.size());
        m_stagedSecondaryRow.fill(-1);
        m_stagedSweepPreset.resize(m_outputs.size());
        m_stagedSweepPreset.fill(-1);
        m_stagedContinuousPreset.resize(m_outputs.size());
        m_stagedContinuousPreset.fill(-1);
        m_stagedMultiFxPreset.resize(m_outputs.size());
        m_stagedMultiFxPreset.fill(-1);
        m_stagedSecondaryValid.resize(m_outputs.size());
        m_stagedSecondaryValid.fill(false);
        m_stagedSweepValid.resize(m_outputs.size());
        m_stagedSweepValid.fill(false);
        m_stagedContinuousValid.resize(m_outputs.size());
        m_stagedContinuousValid.fill(false);
        m_stagedMultiFxValid.resize(m_outputs.size());
        m_stagedMultiFxValid.fill(false);
        m_liveMultiFxPreset.resize(m_outputs.size());
        m_multiFxElapsedMs.resize(m_outputs.size());
        m_multiFxStagedElapsedMs.resize(m_outputs.size());
        m_multiFxLastCycleMs.resize(m_outputs.size());
        m_multiFxStagedLastCycleMs.resize(m_outputs.size());
        m_spatialAppliedRow.resize(m_outputs.size());
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.resize(m_outputs.size());
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        syncLiveTransitionFromOutputs();
        ensureMultiButtonRevisionSizeLocked();
    }
    refreshRowHighlights();
}

// ==========================================================================
// Mode
// ==========================================================================

void PresetTableV2Widget::slotModeChanged(Doc::Mode newMode)
{
    if (newMode == Doc::Operate)
    {
        {
            QMutexLocker lk(&m_stateMutex);
            m_initialInputSyncPending = true;
        }
        m_toolbar->setVisible(true);
        // Hide structural actions not appropriate during a live show
        if (m_actProps)     m_actProps->setVisible(false);
        if (m_actPropSep)   m_actPropSep->setVisible(false);
        updatePositionModeChrome();
        if (m_mode == PTMode::Position)
        {
            m_positionEditOutput = -1;
            m_positionEditSelection = -1;
        }
        if (m_mode == PTMode::Position)
            initOperatePositionSelection();
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_statusBar->setVisible(true);
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else
    {
        m_toolbar->setVisible(true);
        updatePositionModeChrome();
        if (m_actProps)     m_actProps->setVisible(true);
        if (m_actPropSep)   m_actPropSep->setVisible(true);
        m_statusBar->setVisible(false);
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_doc->masterTimer()->unregisterDMXSource(this);

        {
            QMutexLocker lk(&m_stateMutex);
            for (auto& fader : m_faders)
                if (!fader.isNull()) fader->requestDelete();
            m_faders.clear();
            m_activeRow.fill(-1, m_activeRow.size());
            m_stagedRow.fill(-1, m_stagedRow.size());
            m_stagedRowValid.fill(false, m_stagedRowValid.size());
            m_stagedSecondaryRow.fill(-1, m_stagedSecondaryRow.size());
            m_stagedSweepPreset.fill(-1, m_stagedSweepPreset.size());
            m_stagedContinuousPreset.fill(-1, m_stagedContinuousPreset.size());
        if (m_stagedMultiFxPreset.size() > 0)
            m_stagedMultiFxPreset.fill(-1, m_stagedMultiFxPreset.size());
            m_stagedSecondaryValid.fill(false, m_stagedSecondaryValid.size());
            m_stagedSweepValid.fill(false, m_stagedSweepValid.size());
            m_stagedContinuousValid.fill(false, m_stagedContinuousValid.size());
        if (m_stagedMultiFxValid.size() > 0)
            m_stagedMultiFxValid.fill(false, m_stagedMultiFxValid.size());
            m_crossfadeGlobalPos = 0;
            m_crossfadeStartPos  = 0;
            m_crossfadePrevPos   = 0;
            m_crossfadeStagedAtLowSide = true;
            m_crossfadeSessionActive = false;
            m_crossfadeEditLaneStaged = true;
            m_initialInputSyncPending = false;
            m_widgetFlashGateActive = false;
            m_widgetFlashGateLastValue = 0;
            for (int o = 0; o < m_matrixState.size(); ++o)
                releaseMatrixFlashLocked(o);
        }
    }

    VCWidget::slotModeChanged(newMode);

    if (newMode == Doc::Operate && m_doc && m_doc->inputOutputMap())
    {
        m_doc->inputOutputMap()->flushInputs();
        QMutexLocker lk(&m_stateMutex);
        m_initialInputSyncPending = false;
    }

    if (newMode == Doc::Design)
    {
        // Rebuild AFTER mode is Design so rebuildTable/refreshRowHighlights
        // won't add badge-prefixed text back into cells
        rebuildTable();
    }
    else
    {
        syncFrozenNameColumnLayout();
    }

    refreshRowHighlights();
    update();

    QTimer::singleShot(0, this, [this]() {
        syncFrozenNameColumnLayout();
        refreshRowHighlights();
        update();
    });
}

// ==========================================================================
// Toolbar actions
// ==========================================================================

void PresetTableV2Widget::slotAddRow()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Row"),
                                         tr("Preset name:"), QLineEdit::Normal,
                                         tr("Preset %1").arg(m_rows.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    PTRow row;
    row.name = name.trimmed();
    row.values.resize(m_columns.size(), 0);
    if (m_mode == PTMode::Position)
    {
        for (const QLCPoint& pt : positionTablePoints())
        {
            Fixture* fxi = fixtureAtPoint(pt);
            const GroupHead gh = groupHeadAtPoint(pt);
            PTPositionValue pos = fxi
                    ? PTPositionConverter::centerPosition(fxi, gh.head)
                    : PTPositionValue();
            if (pos.valid)
                row.positions.insert(pt, pos);
        }
    }

    {
        QMutexLocker lk(&m_stateMutex);
        m_rows.append(row);
        m_positionOverrides.append(QHash<int, PTPositionOutputLayer>());
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableV2Widget::slotRemoveRow()
{
    int selRow = m_table->currentRow();
    if (selRow < 0 || selRow >= m_rows.size()) return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_rows.remove(selRow);
        if (selRow >= 0 && selRow < m_positionOverrides.size())
            m_positionOverrides.remove(selRow);
        // Clamp active rows
        for (int& ar : m_activeRow)
            if (ar >= m_rows.size()) ar = -1;
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableV2Widget::slotAddColumn()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Column"),
                                         tr("Channel name:"), QLineEdit::Normal,
                                         tr("Ch %1").arg(m_columns.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    PTColumn col;
    col.name = name.trimmed();
    col.type = PTColumn::Numeric;

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns.append(col);
        for (PTRow& row : m_rows)
            row.values.append(0);
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableV2Widget::slotRemoveColumn()
{
    int selCol = m_table->currentColumn() - 1;  // -1 because col 0 is Name
    if (selCol < 0 || selCol >= m_columns.size()) return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns.remove(selCol);
        for (PTRow& row : m_rows)
            if (selCol < row.values.size()) row.values.remove(selCol);
    }
    rebuildTable();
    m_doc->setModified();
}

void PresetTableV2Widget::slotProperties()
{
    editProperties();
}

// ==========================================================================
// Column header double-click → open column config dialog
// ==========================================================================

void PresetTableV2Widget::slotColumnHeaderDoubleClicked(int logicalIndex)
{
    if (mode() != Doc::Design) return;
    if (logicalIndex == 0) return;  // Name column — no config

    int valCol = logicalIndex - 1;
    if (valCol < 0 || valCol >= m_columns.size()) return;

    PTMode colMode;
    quint32 groupId;
    {
        QMutexLocker lk(&m_stateMutex);
        colMode = m_mode;
        groupId = m_fixtureGroupId;
    }

    FixtureGroup* grp = (colMode == PTMode::FixtureGroup || colMode == PTMode::Position)
        ? m_doc->fixtureGroup(groupId) : nullptr;

    PresetTableV2ColumnDialog dlg(m_doc, m_columns[valCol], colMode, grp, this);
    if (dlg.exec() != QDialog::Accepted) return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns[valCol] = dlg.column();
    }
    rebuildTable();
    m_doc->setModified();
}

// ==========================================================================
// Event filter — multi-column header selection (Ctrl+click)
// ==========================================================================

bool PresetTableV2Widget::eventFilter(QObject* obj, QEvent* ev)
{
    if (m_positionSpreadSlider && obj == m_positionSpreadSlider
            && ev->type() == QEvent::MouseButtonDblClick)
    {
        QSignalBlocker blocker(m_positionSpreadSlider);
        m_positionSpreadSlider->setValue(128);
        updatePositionSpreadValueLabel(128);
        if (m_positionSymmetricSpreadCheck && m_positionSymmetricSpreadCheck->isChecked()
                && positionTargetCells().size() >= 2)
        {
            stageFromEditorControls();
        }
        return true;
    }

    if (m_table && (obj == m_table || obj == m_table->viewport()
            || obj == m_nameFrozenTable
            || (m_nameFrozenTable && obj == m_nameFrozenTable->viewport())))
    {
        // Accept ShortcutOverride to prevent VirtualConsole's Ctrl+C/V QActions
        // from firing the widget-copy menu instead of our cell copy/paste.
        if (ev->type() == QEvent::ShortcutOverride)
        {
            auto* ke = static_cast<QKeyEvent*>(ev);
            if (ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::Paste))
            {
                ke->accept();
                return true;
            }
        }
        if (ev->type() == QEvent::KeyPress)
        {
            auto* ke = static_cast<QKeyEvent*>(ev);
            if (ke->matches(QKeySequence::Copy))  { slotCopySelection();  return true; }
            if (ke->matches(QKeySequence::Paste)) { slotPasteSelection(); return true; }
        }
    }
    return VCWidget::eventFilter(obj, ev);
}

// ==========================================================================
// Multi-column resize — propagate width to all selected columns
// ==========================================================================

void PresetTableV2Widget::slotHeaderSectionResized(int logicalIndex, int /*oldSize*/, int newSize)
{
    if (m_resizingColumns) return;
    if (logicalIndex == 0)
    {
        m_nameColWidth = newSize;
        if (m_nameFrozenTable)
        {
            m_nameFrozenTable->setColumnWidth(0, newSize);
            m_nameFrozenTable->setFixedWidth(newSize + 2);
        }
        return;
    }
    QList<int> selected;
    for (const QModelIndex& idx : m_table->selectionModel()->selectedColumns())
        selected.append(idx.column());
    if (!selected.contains(logicalIndex) || selected.size() < 2) return;
    m_resizingColumns = true;
    for (int col : selected)
        if (col != logicalIndex)
            m_table->setColumnWidth(col, newSize);
    m_resizingColumns = false;
}

// ==========================================================================
// keyPressEvent — Ctrl+C / Ctrl+V
// ==========================================================================

void PresetTableV2Widget::keyPressEvent(QKeyEvent* e)
{
    if (m_mode == PTMode::Position)
    {
        if (e->matches(QKeySequence::Copy))  { copyPositionSelectionToClipboard(); e->accept(); return; }
        if (e->matches(QKeySequence::Paste)) { pastePositionClipboardToSelection(); e->accept(); return; }
    }
    else
    {
        if (e->matches(QKeySequence::Copy))  { slotCopySelection(); e->accept(); return; }
        if (e->matches(QKeySequence::Paste)) { slotPasteSelection(); e->accept(); return; }
    }
    VCWidget::keyPressEvent(e);
}

// ==========================================================================
// Copy selection → clipboard as TSV
// ==========================================================================

void PresetTableV2Widget::slotCopySelection()
{
    QList<QTableWidgetItem*> items = m_table->selectedItems();
    if (items.isEmpty()) return;

    int minRow = INT_MAX, maxRow = INT_MIN, minCol = INT_MAX, maxCol = INT_MIN;
    for (auto* it : items)
    {
        minRow = qMin(minRow, it->row());    maxRow = qMax(maxRow, it->row());
        minCol = qMin(minCol, it->column()); maxCol = qMax(maxCol, it->column());
    }

    QStringList lines;
    for (int r = minRow; r <= maxRow; ++r)
    {
        QStringList parts;
        for (int c = minCol; c <= maxCol; ++c)
        {
            auto* it = m_table->item(r, c);
            parts << (it ? it->text() : QString());
        }
        lines << parts.join(QLatin1Char('\t'));
    }
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

// ==========================================================================
// Paste from clipboard → selected cells / rows
// ==========================================================================

void PresetTableV2Widget::pasteValueToItem(QTableWidgetItem* item, const QString& raw)
{
    if (m_mode == PTMode::Position)
    {
        const int pointCol = item->column() - 1;
        const QVector<QLCPoint> points = positionTablePoints();
        if (pointCol < 0 || pointCol >= points.size())
            return;
        const QLCPoint pt = points.at(pointCol);
        Fixture* fxi = fixtureAtPoint(pt);
        const GroupHead gh = groupHeadAtPoint(pt);
        const PTPositionValue pos = PTPositionConverter::parsePositionText(raw, fxi, gh.head);
        const QString text = PTPositionConverter::formatPosition(pos);
        item->setText(text);
        item->setData(Qt::UserRole, text);
        const int row = item->row();
        QMutexLocker lk(&m_stateMutex);
        if (row >= 0 && row < m_rows.size())
        {
            if (pos.valid)
                m_rows[row].positions.insert(pt, pos);
            else
                m_rows[row].positions.remove(pt);
        }
        return;
    }

    int valCol = item->column() - 1;
    if (valCol < 0 || valCol >= m_columns.size()) return;

    const PTColumn& col = m_columns[valCol];
    int dmxVal = 0;

    if (col.type == PTColumn::Dropdown && !col.options.isEmpty())
    {
        // Try name match first, then numeric fallback
        bool found = false;
        for (const PTOption& opt : col.options)
        {
            if (opt.name.compare(raw, Qt::CaseInsensitive) == 0)
                { dmxVal = opt.value; found = true; break; }
        }
        if (!found) dmxVal = qBound(0, raw.toInt(), 255);

        auto lbl = optionLabelFor(col, dmxVal);
        item->setText(lbl.first);
        item->setIcon(makeItemIcon(lbl.second));
    }
    else if (col.type == PTColumn::Scaler)
    {
        // Accept either a scaler value or raw DMX
        bool ok = false;
        int sv = raw.toInt(&ok);
        if (ok)
            dmxVal = scalerToDmx(sv, col.scalerMin, col.scalerMax);
        else
            dmxVal = qBound(0, raw.toInt(), 255);
        int displayed = dmxToScaler(dmxVal, col.scalerMin, col.scalerMax);
        item->setText(QString("%1%2").arg(displayed).arg(col.scalerSuffix));
    }
    else
    {
        dmxVal = qBound(0, raw.toInt(), 255);
        item->setText(QString::number(dmxVal));
    }

    item->setData(Qt::UserRole, dmxVal);

    // Sync directly into m_rows (already under rebuildingTable guard)
    int row = item->row();
    QMutexLocker lk(&m_stateMutex);
    if (row < m_rows.size() && valCol < m_rows[row].values.size())
        m_rows[row].values[valCol] = uchar(dmxVal);
}

void PresetTableV2Widget::slotTableContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* copyAct  = menu.addAction(tr("Copy cells"));
    QAction* pasteAct = menu.addAction(tr("Paste cells"));
    copyAct->setShortcut(QKeySequence::Copy);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(!QApplication::clipboard()->text().isEmpty());

    QAction* chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
    if (chosen == copyAct)  slotCopySelection();
    if (chosen == pasteAct) slotPasteSelection();
}

void PresetTableV2Widget::syncAllDataFromTable()
{
    // Capture column widths from GUI before taking the mutex
    int nameColW = m_table->columnWidth(0);
    QVector<int> colWidths(m_columns.size());
    for (int c = 0; c < m_columns.size(); ++c)
        colWidths[c] = m_table->columnWidth(c + 1);

    QMutexLocker lk(&m_stateMutex);
    const bool positionMode = (m_mode == PTMode::Position);
    const QVector<QLCPoint> points = positionMode ? positionTablePoints() : QVector<QLCPoint>();
    bool syncNames = true;
    for (int r = 0; r < m_table->rowCount() && r < m_rows.size(); ++r)
    {
        if (syncNames)
        {
            auto* nameItem = m_table->item(r, 0);
            if (nameItem) m_rows[r].name = nameItem->text();
        }

        if (positionMode)
        {
            m_rows[r].positions.clear();
            for (int c = 0; c < points.size(); ++c)
            {
                auto* item = m_table->item(r, c + 1);
                if (!item)
                    continue;
                Fixture* fxi = fixtureAtPoint(points.at(c));
                const GroupHead gh = groupHeadAtPoint(points.at(c));
                const PTPositionValue pos =
                        PTPositionConverter::parsePositionText(item->text(), fxi, gh.head);
                if (pos.valid)
                    m_rows[r].positions.insert(points.at(c), pos);
            }
            continue;
        }

        for (int c = 0; c < m_columns.size(); ++c)
        {
            auto* item = m_table->item(r, c + 1);
            if (item && c < m_rows[r].values.size())
                m_rows[r].values[c] = uchar(item->data(Qt::UserRole).toInt());
        }
    }

    // Persist current column widths
    m_nameColWidth = (nameColW > 0) ? nameColW : -1;
    for (int c = 0; c < m_columns.size() && c < colWidths.size(); ++c)
        m_columns[c].width = (colWidths[c] > 0) ? colWidths[c] : -1;
}

void PresetTableV2Widget::slotPasteSelection()
{
    QString text = QApplication::clipboard()->text().trimmed();
    if (text.isEmpty()) return;

    QList<QTableWidgetItem*> sel = m_table->selectedItems();
    if (sel.isEmpty()) return;

    QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    m_table->blockSignals(true);
    m_rebuildingTable = true;

    // Single value → paste to ALL selected non-name cells
    bool singleValue = (lines.size() == 1 && !lines[0].contains(QLatin1Char('\t')));
    if (singleValue)
    {
        QString raw = lines[0].trimmed();
        for (auto* item : sel)
        {
            if (item->column() == 0) continue;  // never overwrite row name
            pasteValueToItem(item, raw);
        }
    }
    else
    {
        // Multi-row paste: anchor at top-left of selection
        int anchorRow = sel.first()->row();
        int anchorCol = sel.first()->column();
        for (auto* it : sel)
        {
            anchorRow = qMin(anchorRow, it->row());
            anchorCol = qMin(anchorCol, it->column());
        }

        for (int li = 0; li < lines.size(); ++li)
        {
            int r = anchorRow + li;
            if (r >= m_table->rowCount()) break;
            QStringList cols = lines[li].split(QLatin1Char('\t'));
            for (int ci = 0; ci < cols.size(); ++ci)
            {
                int c = anchorCol + ci;
                if (c >= m_table->columnCount()) break;
                if (c == 0) continue;  // skip Name column
                auto* item = m_table->item(r, c);
                if (item) pasteValueToItem(item, cols[ci].trimmed());
            }
        }
    }

    m_rebuildingTable = false;
    m_table->blockSignals(false);
    m_doc->setModified();
}

QVector<QLCPoint> PresetTableV2Widget::positionTablePoints() const
{
    QVector<QLCPoint> points;
    if ((m_mode != PTMode::Position && m_mode != PTMode::FixtureGroup)
            || m_fixtureGroupId == UINT_MAX || !m_doc)
        return points;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return points;

    const QMap<QLCPoint, GroupHead> heads = grp->headsMap();
    for (auto it = heads.constBegin(); it != heads.constEnd(); ++it)
        points.append(it.key());
    std::sort(points.begin(), points.end(), [](const QLCPoint& a, const QLCPoint& b) {
        return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
    });
    return points;
}

GroupHead PresetTableV2Widget::groupHeadAtPoint(const QLCPoint& pt) const
{
    if (!m_doc || m_fixtureGroupId == UINT_MAX)
        return GroupHead();
    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return GroupHead();
    return grp->headsMap().value(pt);
}

Fixture* PresetTableV2Widget::fixtureAtPoint(const QLCPoint& pt) const
{
    const GroupHead gh = groupHeadAtPoint(pt);
    if (!gh.isValid() || !m_doc)
        return nullptr;
    return m_doc->fixture(gh.fxi);
}

QString PresetTableV2Widget::positionColumnHeader(const QLCPoint& pt) const
{
    Fixture* fxi = fixtureAtPoint(pt);
    const QString name = fxi ? fxi->name() : tr("Empty");
    return QStringLiteral("%1 (%2,%3)").arg(name).arg(pt.x()).arg(pt.y());
}

QString PresetTableV2Widget::positionCellTooltip(const QLCPoint& pt) const
{
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    if (!fxi)
        return tr("No fixture at this grid cell");
    const QRectF r = PTPositionConverter::degreesRange(fxi, gh.head);
    return tr("Pan %1°–%2°, Tilt %3°–%4°\nEdit as degrees, e.g. 270°,135°")
            .arg(r.left(), 0, 'f', 1)
            .arg(r.left() + r.width(), 0, 'f', 1)
            .arg(r.top(), 0, 'f', 1)
            .arg(r.top() + r.height(), 0, 'f', 1);
}

PTPositionValue PresetTableV2Widget::effectivePositionValue(int rowIdx, int outputIdx,
                                                            const QLCPoint& point) const
{
    PTPositionValue base;
    if (rowIdx >= 0 && rowIdx < m_rows.size())
        base = m_rows[rowIdx].positions.value(point);

    if (outputIdx < 0 || rowIdx < 0 || rowIdx >= m_positionOverrides.size())
        return base;

    const PTPositionOutputLayer layer = m_positionOverrides[rowIdx].value(outputIdx);
    for (const PTPositionSelectionLayer& sel : layer.selections)
    {
        if (sel.cells.contains(point) && sel.overrides.contains(point))
            return sel.overrides.value(point);
    }
    if (layer.allOverrides.contains(point))
        return layer.allOverrides.value(point);
    return base;
}

PTPositionValue PresetTableV2Widget::positionValueForEditLayer(int rowIdx, int outputIdx,
                                                               int selectionIdx,
                                                               const QLCPoint& point) const
{
    if (outputIdx < 0)
        return (rowIdx >= 0 && rowIdx < m_rows.size())
                ? m_rows[rowIdx].positions.value(point) : PTPositionValue();

    if (rowIdx < 0 || rowIdx >= m_positionOverrides.size())
        return PTPositionValue();

    const PTPositionOutputLayer layer = m_positionOverrides[rowIdx].value(outputIdx);
    if (selectionIdx >= 0 && selectionIdx < layer.selections.size())
        return layer.selections.at(selectionIdx).overrides.value(point);
    return layer.allOverrides.value(point);
}

PTPositionValue PresetTableV2Widget::positionValueForDisplay(int rowIdx, int outputIdx,
                                                             int selectionIdx,
                                                             const QLCPoint& point) const
{
    if (m_positionDraftDirty
            && m_positionDraftCtx.row == rowIdx
            && m_positionDraftCtx.output == outputIdx
            && m_positionDraftCtx.selection == selectionIdx
            && m_positionDraftCells.contains(point))
    {
        return m_positionDraftCells.value(point);
    }
    return positionValueForEditLayer(rowIdx, outputIdx, selectionIdx, point);
}

bool PresetTableV2Widget::positionDraftAppliesToDmx(int outputIdx, int activeRow) const
{
    if (!m_positionDraftDirty || m_positionDraftCtx.row < 0)
        return false;
    if (activeRow != m_positionDraftCtx.row)
        return false;
    if (m_positionDraftCtx.output < 0)
        return true;
    return m_positionDraftCtx.output == outputIdx;
}

PTPositionValue PresetTableV2Widget::positionDraftValueForOutput(int outputIdx, int rowIdx,
                                                                 const QLCPoint& point) const
{
    if (!positionDraftAppliesToDmx(outputIdx, rowIdx) || !m_positionDraftCells.contains(point))
        return effectivePositionValue(rowIdx, outputIdx, point);

    const PTPositionValue draft = m_positionDraftCells.value(point);

    if (m_positionDraftCtx.output < 0)
    {
        PTPositionValue base = draft;
        if (outputIdx < 0 || rowIdx < 0 || rowIdx >= m_positionOverrides.size())
            return base;

        const PTPositionOutputLayer layer = m_positionOverrides[rowIdx].value(outputIdx);
        for (const PTPositionSelectionLayer& sel : layer.selections)
        {
            if (sel.cells.contains(point) && sel.overrides.contains(point))
                return sel.overrides.value(point);
        }
        if (layer.allOverrides.contains(point))
            return layer.allOverrides.value(point);
        return base;
    }

    if (m_positionDraftCtx.output != outputIdx)
        return effectivePositionValue(rowIdx, outputIdx, point);

    PTPositionValue base = (rowIdx >= 0 && rowIdx < m_rows.size())
            ? m_rows[rowIdx].positions.value(point) : PTPositionValue();

    if (rowIdx < 0 || rowIdx >= m_positionOverrides.size())
        return draft.valid ? draft : base;

    const PTPositionOutputLayer layer = m_positionOverrides[rowIdx].value(outputIdx);

    if (m_positionDraftCtx.selection >= 0)
    {
        for (int s = 0; s < layer.selections.size(); ++s)
        {
            const PTPositionSelectionLayer& sel = layer.selections.at(s);
            if (s == m_positionDraftCtx.selection)
                return draft;
            if (sel.cells.contains(point) && sel.overrides.contains(point))
                return sel.overrides.value(point);
        }
        if (layer.allOverrides.contains(point))
            return layer.allOverrides.value(point);
        return base;
    }

    for (const PTPositionSelectionLayer& sel : layer.selections)
    {
        if (sel.cells.contains(point) && sel.overrides.contains(point))
            return sel.overrides.value(point);
    }
    return draft;
}

bool PresetTableV2Widget::tableUsesPositionMode() const
{
    return m_mode == PTMode::Position;
}

QList<QLCPoint> PresetTableV2Widget::fixtureGroupPoints() const
{
    return positionTablePoints().toList();
}

PTPositionValue PresetTableV2Widget::positionForPreview(int tableRow, int outputIdx,
                                                        int selectionIdx,
                                                        const QLCPoint& pt) const
{
    if (tableRow < 0 || tableRow >= m_rows.size())
        return PTPositionValue();
    if (outputIdx < 0)
        return m_rows[tableRow].positions.value(pt);
    if (selectionIdx > 0)
    {
        const PTPositionValue layered = positionValueForEditLayer(
                tableRow, outputIdx, selectionIdx - 1, pt);
        if (layered.valid)
            return layered;
    }
    return effectivePositionValue(tableRow, outputIdx, pt);
}

void PresetTableV2Widget::updatePositionModeChrome()
{
    const bool positionMode = (m_mode == PTMode::Position);
    if (m_positionEditorPanel)
        m_positionEditorPanel->setVisible(positionMode);
    if (m_positionRowListPanel)
        m_positionRowListPanel->setVisible(positionMode);
    if (m_tableWrap)
        m_tableWrap->setVisible(!positionMode);
    if (m_actAddCol)
        m_actAddCol->setVisible(!positionMode && mode() == Doc::Design);
    if (m_actRemCol)
        m_actRemCol->setVisible(!positionMode && mode() == Doc::Design);
    if (m_actColSep)
        m_actColSep->setVisible(!positionMode && mode() == Doc::Design);
}

int PresetTableV2Widget::currentPositionEditRow() const
{
    if (m_positionEditRow >= 0)
        return m_positionEditRow;
    if (m_table)
        return m_table->currentRow();
    return -1;
}

void PresetTableV2Widget::initOperatePositionSelection()
{
    if (m_mode != PTMode::Position || mode() != Doc::Operate)
        return;

    const int ctxOut = positionContextOutput();
    {
        QMutexLocker lk(&m_stateMutex);
        if (ctxOut < m_activeRow.size() && m_activeRow[ctxOut] >= 0)
            m_positionEditRow = m_activeRow[ctxOut];
    }

    m_positionSelectedCells.clear();
    m_positionSelectionOrder.clear();
    m_positionFollowLiveRow = -1;
    m_positionFollowLiveContextOut = -1;
    m_positionLastFollowDrivingOutput = -1;
    rebuildPositionEditor();
    followLivePresetSelection();
}

int PresetTableV2Widget::positionContextOutput() const
{
    return m_positionEditOutput >= 0 ? m_positionEditOutput : 0;
}

int PresetTableV2Widget::positionMarkerContextOutput() const
{
    if (m_positionEditOutput >= 0)
        return m_positionEditOutput;
    if (m_positionLastFollowDrivingOutput >= 0)
        return m_positionLastFollowDrivingOutput;
    return 0;
}

int PresetTableV2Widget::effectivePrimaryRowForOutput(int outputIdx) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    if (m_crossfadeEnabled
            && outputIdx < m_stagedRowValid.size()
            && m_stagedRowValid[outputIdx]
            && outputIdx < m_stagedRow.size())
    {
        return m_stagedRow[outputIdx];
    }
    if (outputIdx < m_activeRow.size())
        return m_activeRow[outputIdx];
    return -1;
}

void PresetTableV2Widget::updateOperateRowListLiveMarkers()
{
    if (!m_positionPresetTree || m_mode != PTMode::Position || mode() != Doc::Operate)
        return;

    const int ctxOut = positionMarkerContextOutput();
    int liveRow = -1;
    int stagedRow = -1;
    bool hasStaged = false;
    {
        QMutexLocker lk(&m_stateMutex);
        if (ctxOut < m_activeRow.size())
            liveRow = m_activeRow[ctxOut];
        if (m_crossfadeEnabled
                && ctxOut < m_stagedRowValid.size()
                && m_stagedRowValid[ctxOut]
                && ctxOut < m_stagedRow.size())
        {
            hasStaged = true;
            stagedRow = m_stagedRow[ctxOut];
        }
    }

    const QFont defaultFont = m_positionPresetTree->font();
    QFont liveFont = defaultFont;
    liveFont.setBold(true);
    const QColor liveColor = s_outputColors[ctxOut % 8];

    for (int i = 0; i < m_positionPresetTree->topLevelItemCount() && i < m_rows.size(); ++i)
    {
        QTreeWidgetItem* item = m_positionPresetTree->topLevelItem(i);
        if (!item)
            continue;
        const PTPositionTreeRef ref = item->data(0, Qt::UserRole).value<PTPositionTreeRef>();
        if (ref.row < 0 || ref.row >= m_rows.size())
            continue;
        QString name = m_rows[ref.row].name;
        if (positionLayerHasOverrides(ref.row, -1, -1))
            name += QStringLiteral(" *");
        const bool isLive = (ref.row == liveRow && liveRow >= 0);
        const bool isStaged = hasStaged && ref.row == stagedRow && stagedRow >= 0 && !isLive;
        if (isLive)
            name += QStringLiteral("  (LIVE)");
        else if (isStaged)
            name += QStringLiteral("  (STAGED)");
        item->setText(0, name);
        item->setFont(0, isLive ? liveFont : defaultFont);
        item->setForeground(0, isLive ? QBrush(liveColor) : QBrush());
    }
}

void PresetTableV2Widget::followLivePresetSelection(int drivingOutput)
{
    if (m_mode != PTMode::Position || mode() != Doc::Operate || !m_positionPresetTree)
        return;

    const int ctxOut = drivingOutput >= 0 ? drivingOutput : positionContextOutput();
    const int targetRow = effectivePrimaryRowForOutput(ctxOut);
    if (targetRow < 0)
        return;

    const int targetOutput = drivingOutput >= 0 ? drivingOutput : m_positionEditOutput;
    const int targetSelection = drivingOutput >= 0 ? -1 : m_positionEditSelection;

    const bool atTarget = (m_positionEditRow == targetRow
            && m_positionEditOutput == targetOutput
            && m_positionEditSelection == targetSelection);

    if (atTarget
            && targetRow == m_positionFollowLiveRow
            && ctxOut == m_positionFollowLiveContextOut)
    {
        m_positionTreeExpandedRows.insert(targetRow);
        if (QTreeWidgetItem* presetItem = positionTreeItemForLayer(targetRow, -1, -1))
        {
            QSignalBlocker blocker(m_positionPresetTree);
            presetItem->setExpanded(true);
        }
        if (drivingOutput >= 0)
            m_positionLastFollowDrivingOutput = drivingOutput;
        return;
    }

    if (!atTarget && !confirmDiscardPositionDraft())
        return;

    m_positionTreeExpandedRows.insert(targetRow);
    if (QTreeWidgetItem* presetItem = positionTreeItemForLayer(targetRow, -1, -1))
    {
        QSignalBlocker blocker(m_positionPresetTree);
        presetItem->setExpanded(true);
    }

    selectPositionTreeLayer(targetRow, targetOutput, targetSelection);
    m_positionSelectedCells.clear();
    m_positionSelectionOrder.clear();

    m_positionFollowLiveRow = targetRow;
    m_positionFollowLiveContextOut = ctxOut;
    if (drivingOutput >= 0)
        m_positionLastFollowDrivingOutput = drivingOutput;

    refreshPositionGridCells();
    updatePositionValueStrip();
    refreshPositionEditorFromSelection();
}

void PresetTableV2Widget::refreshOperatePositionChrome(int drivingOutput)
{
    if (m_mode != PTMode::Position || mode() != Doc::Operate)
        return;
    updateOperateRowListLiveMarkers();
    followLivePresetSelection(drivingOutput);
    updatePositionValueStrip();
}

bool PresetTableV2Widget::positionLayerHasOverrides(int row, int output, int selection) const
{
    if (row < 0 || row >= m_rows.size())
        return false;

    if (output < 0)
        return !m_rows[row].positions.isEmpty();

    if (row >= m_positionOverrides.size())
        return false;

    const PTPositionOutputLayer layer = m_positionOverrides[row].value(output);
    if (selection < 0)
        return !layer.allOverrides.isEmpty();

    if (selection >= layer.selections.size())
        return false;
    return !layer.selections.at(selection).overrides.isEmpty();
}

static QColor positionSelectionLayerColor(int selectionIndex)
{
    static const QColor colors[] = {
        QColor(70, 210, 255), QColor(255, 180, 65), QColor(120, 225, 110),
        QColor(220, 120, 255), QColor(255, 105, 145), QColor(95, 160, 255),
        QColor(240, 220, 80), QColor(95, 220, 190), QColor(255, 135, 85),
        QColor(165, 145, 255)
    };
    return colors[qAbs(selectionIndex) % (int(sizeof(colors) / sizeof(colors[0])))];
}

QSet<QLCPoint> PresetTableV2Widget::positionEditableCells() const
{
    return positionEditableCellsForLayer(currentPositionEditRow(),
                                         m_positionEditOutput, m_positionEditSelection);
}

QSet<QLCPoint> PresetTableV2Widget::positionEditableCellsForLayer(int row, int output,
                                                                  int selection) const
{
    QSet<QLCPoint> result;

    if (output < 0)
    {
        for (const QLCPoint& pt : positionTablePoints())
            result.insert(pt);
        return result;
    }

    for (const QLCPoint& pt : outputPointsForPresetOverride(output))
        result.insert(pt);

    if (selection < 0)
        return result;

    if (row < 0 || row >= m_positionOverrides.size())
        return result;

    QMutexLocker lk(&m_stateMutex);
    const PTPositionOutputLayer layer = m_positionOverrides[row].value(output);
    if (selection >= layer.selections.size())
        return result;

    const PTPositionSelectionLayer& selLayer = layer.selections.at(selection);
    if (!selLayer.cells.isEmpty())
    {
        QSet<QLCPoint> filtered;
        for (const QLCPoint& pt : selLayer.cells)
        {
            if (result.contains(pt))
                filtered.insert(pt);
        }
        return filtered;
    }

    QSet<QLCPoint> occupied;
    for (int s = 0; s < layer.selections.size(); ++s)
    {
        if (s == selection)
            continue;
        for (const QLCPoint& pt : layer.selections.at(s).cells)
            occupied.insert(pt);
    }

    QSet<QLCPoint> available;
    for (const QLCPoint& pt : result)
    {
        if (!occupied.contains(pt))
            available.insert(pt);
    }
    return available;
}

void PresetTableV2Widget::prunePositionGridSelection()
{
    const QSet<QLCPoint> editable = positionEditableCells();
    if (m_positionSelectedCells.isEmpty())
        return;

    QSet<QLCPoint> pruned;
    QList<QLCPoint> prunedOrder;
    for (const QLCPoint& pt : m_positionSelectionOrder)
    {
        if (editable.contains(pt))
        {
            pruned.insert(pt);
            prunedOrder.append(pt);
        }
    }
    for (const QLCPoint& pt : m_positionSelectedCells)
    {
        if (editable.contains(pt) && !prunedOrder.contains(pt))
            prunedOrder.append(pt);
        if (editable.contains(pt))
            pruned.insert(pt);
    }
    m_positionSelectedCells = pruned;
    m_positionSelectionOrder = prunedOrder;
}

QSet<QLCPoint> PresetTableV2Widget::positionTargetCells() const
{
    const QSet<QLCPoint> editable = positionEditableCells();
    if (!m_positionSelectedCells.isEmpty())
    {
        QSet<QLCPoint> filtered;
        for (const QLCPoint& pt : m_positionSelectedCells)
        {
            if (editable.contains(pt))
                filtered.insert(pt);
        }
        return filtered.isEmpty() ? editable : filtered;
    }
    return editable;
}

QList<QLCPoint> PresetTableV2Widget::positionTargetCellOrder() const
{
    if (!m_positionSelectedCells.isEmpty() && !m_positionSelectionOrder.isEmpty())
    {
        QList<QLCPoint> order;
        for (const QLCPoint& pt : m_positionSelectionOrder)
        {
            if (m_positionSelectedCells.contains(pt))
                order.append(pt);
        }
        for (const QLCPoint& pt : m_positionSelectedCells)
        {
            if (!order.contains(pt))
                order.append(pt);
        }
        return order;
    }
    if (!m_positionSelectedCells.isEmpty())
        return PTPositionFixtureGridWidget::rowMajorOrder(m_positionSelectedCells);
    return PTPositionFixtureGridWidget::rowMajorOrder(positionTargetCells());
}

QLCPoint PresetTableV2Widget::positionReferencePoint() const
{
    const QList<QLCPoint> order = positionTargetCellOrder();
    if (order.isEmpty())
        return QLCPoint(-1, -1);
    if (order.size() >= 2 && !m_positionSelectionOrder.isEmpty())
        return order.at(order.size() / 2);
    return order.first();
}

QTreeWidgetItem* PresetTableV2Widget::positionTreeItemForLayer(int row, int output,
                                                               int selection) const
{
    if (!m_positionPresetTree || row < 0)
        return nullptr;

    for (int i = 0; i < m_positionPresetTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* presetItem = m_positionPresetTree->topLevelItem(i);
        if (!presetItem)
            continue;
        const PTPositionTreeRef presetRef =
                presetItem->data(0, Qt::UserRole).value<PTPositionTreeRef>();
        if (presetRef.row != row)
            continue;

        if (output < 0 && selection < 0)
            return presetItem;

        QList<QTreeWidgetItem*> stack;
        for (int c = 0; c < presetItem->childCount(); ++c)
            stack.append(presetItem->child(c));

        while (!stack.isEmpty())
        {
            QTreeWidgetItem* item = stack.takeFirst();
            const PTPositionTreeRef layerRef =
                    item->data(0, Qt::UserRole).value<PTPositionTreeRef>();
            if (layerRef.isLayerLeaf && layerRef.output == output
                    && layerRef.selection == selection)
            {
                return item;
            }
            for (int c = 0; c < item->childCount(); ++c)
                stack.append(item->child(c));
        }
    }
    return nullptr;
}

bool PresetTableV2Widget::selectPositionTreeLayer(int row, int output, int selection)
{
    if (!m_positionPresetTree)
        return false;

    QTreeWidgetItem* item = positionTreeItemForLayer(row, output, selection);
    if (!item && row >= 0)
        item = positionTreeItemForLayer(row, -1, -1);

    if (!item)
        return false;

    QSignalBlocker blocker(m_positionPresetTree);
    m_positionPresetTree->setCurrentItem(item);
    m_positionPresetTree->scrollToItem(item);

    const PTPositionTreeRef ref = item->data(0, Qt::UserRole).value<PTPositionTreeRef>();
    if (ref.isLayerLeaf)
    {
        m_positionEditRow = ref.row;
        m_positionEditOutput = ref.output;
        m_positionEditSelection = ref.selection;
    }
    return true;
}

QString PresetTableV2Widget::positionEditLayerLabel() const
{
    if (m_positionEditOutput < 0)
    {
        if (m_positionEditRow >= 0 && m_positionEditRow < m_rows.size())
            return m_rows[m_positionEditRow].name;
        return tr("Preset");
    }

    QString outLabel;
    if (m_positionEditOutput < m_outputs.size() && !m_outputs[m_positionEditOutput].name.isEmpty())
        outLabel = m_outputs[m_positionEditOutput].name;
    else
        outLabel = tr("Output %1").arg(m_positionEditOutput + 1);

    if (m_positionEditSelection < 0)
        return outLabel;

    const int row = m_positionEditRow;
    if (row >= 0 && row < m_positionOverrides.size())
    {
        const PTPositionOutputLayer layer = m_positionOverrides[row].value(m_positionEditOutput);
        if (m_positionEditSelection < layer.selections.size())
        {
            const PTPositionSelectionLayer& sel = layer.selections.at(m_positionEditSelection);
            const QString selLabel = sel.name.isEmpty()
                    ? tr("Selection %1").arg(m_positionEditSelection + 1) : sel.name;
            return QStringLiteral("%1 · %2").arg(outLabel, selLabel);
        }
    }
    return QStringLiteral("%1 · %2").arg(outLabel,
            tr("Selection %1").arg(m_positionEditSelection + 1));
}

void PresetTableV2Widget::rebuildPositionPresetTree()
{
    if (!m_positionPresetTree || m_mode != PTMode::Position)
        return;

    QSet<int> expandedRows;
    for (int i = 0; i < m_positionPresetTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* top = m_positionPresetTree->topLevelItem(i);
        if (!top)
            continue;
        const PTPositionTreeRef ref = top->data(0, Qt::UserRole).value<PTPositionTreeRef>();
        if (ref.row >= 0 && top->isExpanded())
            expandedRows.insert(ref.row);
    }
    if (!expandedRows.isEmpty())
        m_positionTreeExpandedRows = expandedRows;

    const int prevRow = m_positionEditRow;
    const int prevOutput = m_positionEditOutput;
    const int prevSelection = m_positionEditSelection;

    QSignalBlocker blocker(m_positionPresetTree);
    m_positionPresetTree->clear();

    QMutexLocker lk(&m_stateMutex);

    auto addLayerItem = [&](QTreeWidgetItem* parent, const QString& label,
                            int row, int output, int selection) -> QTreeWidgetItem* {
        PTPositionTreeRef layerRef;
        layerRef.row = row;
        layerRef.output = output;
        layerRef.selection = selection;
        layerRef.isLayerLeaf = true;
        QString text = label;
        if (positionLayerHasOverrides(row, output, selection))
            text += QStringLiteral(" *");
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        item->setText(0, text);
        item->setData(0, Qt::UserRole, QVariant::fromValue(layerRef));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };

    for (int r = 0; r < m_rows.size(); ++r)
    {
        PTPositionTreeRef presetRef;
        presetRef.row = r;
        presetRef.output = -1;
        presetRef.selection = -1;
        presetRef.isLayerLeaf = true;
        QTreeWidgetItem* presetItem = new QTreeWidgetItem(m_positionPresetTree);
        QString presetLabel = m_rows[r].name;
        if (positionLayerHasOverrides(r, -1, -1))
            presetLabel += QStringLiteral(" *");
        presetItem->setText(0, presetLabel);
        presetItem->setData(0, Qt::UserRole, QVariant::fromValue(presetRef));
        presetItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        for (int o = 0; o < m_outputs.size(); ++o)
        {
            const QString outName = m_outputs[o].name.isEmpty()
                    ? tr("Output %1").arg(o + 1) : m_outputs[o].name;
            const QVector<PTPositionSelectionLayer> sels =
                    (r < m_positionOverrides.size())
                    ? m_positionOverrides[r].value(o).selections : QVector<PTPositionSelectionLayer>();

            if (sels.isEmpty())
            {
                addLayerItem(presetItem, outName, r, o, -1);
            }
            else
            {
                PTPositionTreeRef outRef;
                outRef.row = r;
                outRef.output = o;
                outRef.isLayerLeaf = false;
                QTreeWidgetItem* outItem = new QTreeWidgetItem(presetItem);
                outItem->setText(0, outName);
                outItem->setData(0, Qt::UserRole, QVariant::fromValue(outRef));
                outItem->setFlags(Qt::ItemIsEnabled);

                addLayerItem(outItem, tr("All"), r, o, -1);
                for (int s = 0; s < sels.size(); ++s)
                {
                    const QString selLabel = sels[s].name.isEmpty()
                            ? tr("Selection %1").arg(s + 1) : sels[s].name;
                    addLayerItem(outItem, selLabel, r, o, s);
                }
            }
        }

        presetItem->setExpanded(m_positionTreeExpandedRows.contains(r));
    }

    lk.unlock();
    updateOperateRowListLiveMarkers();

    int targetRow = prevRow;
    if (targetRow < 0 && !m_rows.isEmpty())
        targetRow = 0;
    if (targetRow >= 0)
        selectPositionTreeLayer(targetRow, prevOutput, prevSelection);
}

void PresetTableV2Widget::refreshPositionGridCells()
{
    if (!m_positionGrid || m_mode != PTMode::Position)
        return;

    FixtureGroup* grp = m_doc ? m_doc->fixtureGroup(m_fixtureGroupId) : nullptr;
    if (!grp)
    {
        m_positionGrid->setPlaceholderText(tr("Select a fixture group in Properties"));
        return;
    }

    const QSize gridSize = grp->size();
    const QMap<QLCPoint, GroupHead> heads = grp->headsMap();
    const int row = currentPositionEditRow();
    QMap<QLCPoint, PTPositionGridCell> cells;

    for (auto it = heads.constBegin(); it != heads.constEnd(); ++it)
    {
        const QLCPoint pt = it.key();
        PTPositionGridCell cell;
        cell.point = pt;
        Fixture* fxi = m_doc->fixture(it.value().fxi);
        QString name = fxi ? fxi->name() : tr("Empty");
        if (name.size() > 6)
            name = name.left(5) + QLatin1Char('.');
        cell.label = name;

        cells.insert(pt, cell);
    }

    if (row >= 0)
    {
        QMutexLocker lk(&m_stateMutex);
        for (auto it = cells.begin(); it != cells.end(); ++it)
        {
            const QLCPoint pt = it.key();
            const PTPositionValue base = (row < m_rows.size())
                    ? m_rows[row].positions.value(pt) : PTPositionValue();
            PTPositionGridCell& cell = it.value();
            cell.position = positionValueForDisplay(row, m_positionEditOutput,
                                                      m_positionEditSelection, pt);
            cell.inherited = m_positionEditOutput >= 0 && !cell.position.valid && base.valid;
            if (!cell.position.valid && base.valid)
                cell.position = base;
        }
    }

    prunePositionGridSelection();

    const QSet<QLCPoint> editable = positionEditableCells();
    QVector<PTPositionGridSelectionLayer> foreignLayers;
    if (m_positionEditOutput >= 0 && row >= 0 && row < m_positionOverrides.size())
    {
        QMutexLocker lk(&m_stateMutex);
        const PTPositionOutputLayer layer = m_positionOverrides[row].value(m_positionEditOutput);
        for (int s = 0; s < layer.selections.size(); ++s)
        {
            if (m_positionEditSelection >= 0 && s == m_positionEditSelection)
                continue;
            const PTPositionSelectionLayer& sel = layer.selections.at(s);
            if (sel.cells.isEmpty())
                continue;
            PTPositionGridSelectionLayer gridLayer;
            gridLayer.name = sel.name;
            gridLayer.color = positionSelectionLayerColor(s);
            for (const QLCPoint& pt : sel.cells)
            {
                if (editable.contains(pt) || cells.contains(pt))
                    gridLayer.cells.insert(pt);
            }
            if (!gridLayer.cells.isEmpty())
                foreignLayers.append(gridLayer);
        }
    }

    m_positionGrid->setGrid(gridSize, cells);
    m_positionGrid->setEditableCells(editable);
    m_positionGrid->setForeignSelectionLayers(foreignLayers);
    m_positionGrid->setImplicitAllSelection(m_positionSelectedCells.isEmpty());
    m_positionGrid->setSelectedCells(m_positionSelectedCells, m_positionSelectionOrder);
}

void PresetTableV2Widget::applyPositionEditorChromeVisibility()
{
    if (m_positionValueStrip)
        m_positionValueStrip->setVisible(m_positionShowStatusStrip);
    if (m_positionHintLabel)
        m_positionHintLabel->setVisible(m_positionShowEditorHints);
}

void PresetTableV2Widget::updatePositionValueStrip()
{
    if (!m_positionValueStrip)
        return;

    const QSet<QLCPoint> targetCells = positionTargetCells();
    if (targetCells.isEmpty())
    {
        m_positionValueStrip->setText(tr("No fixtures in fixture group"));
        return;
    }

    const int row = currentPositionEditRow();
    const bool implicitAll = m_positionSelectedCells.isEmpty();
    QStringList parts;
    for (const QLCPoint& pt : targetCells)
    {
        Fixture* fxi = fixtureAtPoint(pt);
        const QString fname = fxi ? fxi->name() : tr("Empty");
        PTPositionValue pos;
        {
            QMutexLocker lk(&m_stateMutex);
            pos = positionValueForDisplay(row, m_positionEditOutput,
                                          m_positionEditSelection, pt);
        }
        parts << QStringLiteral("%1: %2")
                     .arg(fname)
                     .arg(pos.valid ? PTPositionConverter::formatPosition(pos)
                                    : QStringLiteral("—"));
    }
    QString text = parts.join(QStringLiteral("  |  "));
    if (implicitAll)
        text = tr("All fixtures — %1").arg(text);
    if (mode() == Doc::Operate)
    {
        const int o = positionMarkerContextOutput();
        int liveRow = -1;
        int stagedRow = -1;
        bool hasStaged = false;
        {
            QMutexLocker lk(&m_stateMutex);
            if (o < m_activeRow.size())
                liveRow = m_activeRow[o];
            if (m_crossfadeEnabled
                    && o < m_stagedRowValid.size()
                    && m_stagedRowValid[o]
                    && o < m_stagedRow.size())
            {
                hasStaged = true;
                stagedRow = m_stagedRow[o];
            }
        }
        const bool isLive = (row == liveRow && liveRow >= 0);
        const bool isStagedEdit = hasStaged && row == stagedRow && stagedRow >= 0;
        const QString liveLabel = liveRow >= 0
                ? QString::number(liveRow + 1) : QStringLiteral("—");
        text = tr("Editing: %1 · Live: %2 — %3")
                .arg(positionEditLayerLabel()).arg(liveLabel).arg(text);
        if (m_positionDraftDirty)
        {
            text += isLive
                    ? tr("  [LIVE preview — not saved]")
                    : tr("  [unsaved draft — not on output]");
        }
        else
        {
            if (isLive)
                text += tr("  [LIVE — output updates]");
            else if (isStagedEdit)
                text += tr("  [STAGED — commits on crossfade]");
            else
                text += tr("  [stored only — not on output]");
        }
    }
    else
    {
        text = tr("%1 — %2").arg(positionEditLayerLabel()).arg(text);
        if (m_positionDraftDirty)
            text += tr("  [unsaved draft]");
    }
    m_positionValueStrip->setText(text);
}

void PresetTableV2Widget::rebuildPositionEditor()
{
    if (!m_positionGrid || m_mode != PTMode::Position)
        return;

    rebuildPositionPresetTree();
    refreshPositionGridCells();
    updatePositionValueStrip();
    applyPositionEditorChromeVisibility();
    refreshPositionEditorFromSelection();
}

void PresetTableV2Widget::slotPositionPresetTreeChanged(QTreeWidgetItem* current,
                                                       QTreeWidgetItem* /*previous*/)
{
    if (!current)
        return;

    const PTPositionTreeRef ref = current->data(0, Qt::UserRole).value<PTPositionTreeRef>();
    if (!ref.isLayerLeaf)
        return;

    const int oldRow = m_positionEditRow;
    const int oldOutput = m_positionEditOutput;
    const int oldSelection = m_positionEditSelection;

    if (ref.row == oldRow && ref.output == oldOutput && ref.selection == oldSelection)
        return;

    if (!confirmDiscardPositionDraft())
    {
        selectPositionTreeLayer(oldRow, oldOutput, oldSelection);
        return;
    }

    m_positionEditRow = ref.row;
    m_positionEditOutput = ref.output;
    m_positionEditSelection = ref.selection;
    m_positionSelectedCells.clear();
    m_positionSelectionOrder.clear();
    prunePositionGridSelection();
    refreshPositionGridCells();
    updatePositionValueStrip();
    refreshPositionEditorFromSelection();
}

void PresetTableV2Widget::slotPositionPresetTreeDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item || item->parent() != nullptr)
        return;

    const PTPositionTreeRef ref = item->data(0, Qt::UserRole).value<PTPositionTreeRef>();
    if (ref.row < 0 || ref.row >= m_rows.size())
        return;

    const QString oldName = m_rows[ref.row].name;
    bool ok = false;
    const QString newName = QInputDialog::getText(
            this, tr("Rename preset"), tr("Preset name:"),
            QLineEdit::Normal, oldName, &ok).trimmed();
    if (!ok || newName.isEmpty() || newName == oldName)
        return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_rows[ref.row].name = newName;
    }
    if (m_doc)
        m_doc->setModified();

    if (m_table && ref.row < m_table->rowCount())
    {
        if (QTableWidgetItem* nameItem = m_table->item(ref.row, 0))
            nameItem->setText(newName);
    }

    updateOperateRowListLiveMarkers();
    updatePositionValueStrip();
}

QLCPoint PresetTableV2Widget::selectionGridCenter(const QSet<QLCPoint>& cells,
                                                 qreal& cx, qreal& cy) const
{
    cx = 0;
    cy = 0;
    if (cells.isEmpty())
        return QLCPoint(-1, -1);

    for (const QLCPoint& pt : cells)
    {
        cx += qreal(pt.x());
        cy += qreal(pt.y());
    }
    cx /= qreal(cells.size());
    cy /= qreal(cells.size());

    QLCPoint closest = *cells.constBegin();
    qreal bestDist = 1e30;
    for (const QLCPoint& pt : cells)
    {
        const qreal dx = qreal(pt.x()) - cx;
        const qreal dy = qreal(pt.y()) - cy;
        const qreal dist = dx * dx + dy * dy;
        if (dist < bestDist)
        {
            bestDist = dist;
            closest = pt;
        }
    }
    return closest;
}

void PresetTableV2Widget::updatePositionSpreadValueLabel(int raw)
{
    if (!m_positionSpreadValueLabel)
        return;
    const int pct = positionSpreadLabelPct(raw);
    if (pct == 0)
        m_positionSpreadValueLabel->setText(QStringLiteral("0%"));
    else if (pct > 0)
        m_positionSpreadValueLabel->setText(QStringLiteral("+%1%").arg(pct));
    else
        m_positionSpreadValueLabel->setText(QStringLiteral("%1%").arg(pct));
}

QLCPoint PresetTableV2Widget::selectionMiddlePoint() const
{
    const QList<QLCPoint> order = positionTargetCellOrder();
    if (order.isEmpty())
        return QLCPoint(-1, -1);
    return order.at(order.size() / 2);
}

void PresetTableV2Widget::capturePositionSpreadPivotFromSelection()
{
    if (positionTargetCells().size() < 2 || !m_positionPanSpin || !m_positionTiltSpin)
        return;

    const int row = currentPositionEditRow();
    qreal sumPan = 0;
    qreal sumTilt = 0;
    int count = 0;
    {
        QMutexLocker lk(&m_stateMutex);
        for (const QLCPoint& cellPt : positionTargetCells())
        {
            const PTPositionValue cellPos = positionValueForDisplay(
                    row, m_positionEditOutput, m_positionEditSelection, cellPt);
            if (!cellPos.valid)
                continue;
            sumPan += cellPos.panDeg;
            sumTilt += cellPos.tiltDeg;
            ++count;
        }
    }
    if (count == 0)
        return;

    const QLCPoint pt = selectionMiddlePoint();
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    if (!fxi)
        return;

    const qreal pivotPan = sumPan / count;
    const qreal pivotTilt = sumTilt / count;

    m_positionEditorSyncing = true;
    m_positionPanSpin->setValue(pivotPan);
    m_positionTiltSpin->setValue(pivotTilt);
    qreal xNorm = 0.5;
    qreal yNorm = 0.5;
    PTPositionConverter::degreesToNormalized(fxi, gh.head, pivotPan, pivotTilt, xNorm, yNorm);
    if (m_positionXYPad)
        m_positionXYPad->setNormalizedPosition(xNorm, yNorm);
    m_positionEditorSyncing = false;
}

void PresetTableV2Widget::updatePositionSpreadChrome()
{
    const bool multi = positionTargetCells().size() >= 2;
    if (m_positionSymmetricSpreadCheck)
        m_positionSymmetricSpreadCheck->setEnabled(multi);
    const bool spreadOn = multi && m_positionSymmetricSpreadCheck
            && m_positionSymmetricSpreadCheck->isChecked();
    if (m_positionSpreadAxisCombo)
        m_positionSpreadAxisCombo->setEnabled(spreadOn);
    if (m_positionSpreadSlider)
        m_positionSpreadSlider->setEnabled(spreadOn);
}

void PresetTableV2Widget::refreshPositionEditorFromSelection()
{
    if (!m_positionXYPad || m_mode != PTMode::Position || m_positionEditorSyncing)
        return;

    updatePositionSpreadChrome();

    const QSet<QLCPoint> targets = positionTargetCells();
    if (targets.isEmpty())
    {
        m_positionXYPad->setFixture(nullptr, 0);
        m_positionPanSpin->setEnabled(false);
        m_positionTiltSpin->setEnabled(false);
        return;
    }

    const bool spreadActive = targets.size() >= 2
            && m_positionSymmetricSpreadCheck && m_positionSymmetricSpreadCheck->isChecked();
    const QLCPoint pt = spreadActive ? selectionMiddlePoint() : positionReferencePoint();
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    m_positionXYPad->setFixture(fxi, gh.head);
    m_positionPanSpin->setEnabled(fxi != nullptr);
    m_positionTiltSpin->setEnabled(fxi != nullptr);
    if (!fxi)
        return;

    updatePositionValueStrip();

    const QRectF r = PTPositionConverter::degreesRange(fxi, gh.head);
    m_positionPanSpin->setRange(r.left(), r.left() + r.width());
    m_positionTiltSpin->setRange(r.top(), r.top() + r.height());

    if (spreadActive)
        return;

    const int row = currentPositionEditRow();
    PTPositionValue pos;
    {
        QMutexLocker lk(&m_stateMutex);
        pos = positionValueForDisplay(row, m_positionEditOutput, m_positionEditSelection, pt);
    }

    m_positionEditorSyncing = true;
    if (pos.valid)
    {
        qreal xNorm = 0.5;
        qreal yNorm = 0.5;
        PTPositionConverter::degreesToNormalized(fxi, gh.head, pos.panDeg, pos.tiltDeg, xNorm, yNorm);
        m_positionXYPad->setNormalizedPosition(xNorm, yNorm);
        m_positionPanSpin->setValue(pos.panDeg);
        m_positionTiltSpin->setValue(pos.tiltDeg);
    }
    else
    {
        pos = PTPositionConverter::centerPosition(fxi, gh.head);
        qreal xNorm = 0.5;
        qreal yNorm = 0.5;
        PTPositionConverter::degreesToNormalized(fxi, gh.head, pos.panDeg, pos.tiltDeg, xNorm, yNorm);
        m_positionXYPad->setNormalizedPosition(xNorm, yNorm);
        m_positionPanSpin->setValue(pos.panDeg);
        m_positionTiltSpin->setValue(pos.tiltDeg);
    }
    m_positionEditorSyncing = false;
}

PTPositionValue PresetTableV2Widget::readPositionEditorValue() const
{
    PTPositionValue pos;
    const QLCPoint pt = positionReferencePoint();
    if (pt.x() < 0)
        return pos;
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    if (!fxi)
        return pos;
    pos.valid = true;
    pos.panDeg = m_positionPanSpin->value();
    pos.tiltDeg = m_positionTiltSpin->value();
    return PTPositionConverter::clampPosition(fxi, gh.head, pos);
}

void PresetTableV2Widget::writePositionToCells(const QSet<QLCPoint>& cells,
                                               const PTPositionValue& pos,
                                               int row, int output, int selection)
{
    if (row < 0 || row >= m_rows.size() || cells.isEmpty())
        return;

    const QSet<QLCPoint> allowed = positionEditableCellsForLayer(row, output, selection);

    QMutexLocker lk(&m_stateMutex);
    for (const QLCPoint& pt : cells)
    {
        if (!allowed.contains(pt))
            continue;
        if (output < 0)
        {
            if (pos.valid)
                m_rows[row].positions.insert(pt, pos);
            else
                m_rows[row].positions.remove(pt);
        }
        else
        {
            while (m_positionOverrides.size() <= row)
                m_positionOverrides.append(QHash<int, PTPositionOutputLayer>());
            PTPositionOutputLayer& layer = m_positionOverrides[row][output];
            if (selection < 0)
            {
                if (pos.valid)
                    layer.allOverrides.insert(pt, pos);
                else
                    layer.allOverrides.remove(pt);
            }
            else
            {
                while (layer.selections.size() <= selection)
                    layer.selections.append(PTPositionSelectionLayer());
                PTPositionSelectionLayer& sel = layer.selections[selection];
                sel.cells.insert(pt);
                if (pos.valid)
                    sel.overrides.insert(pt, pos);
                else
                    sel.overrides.remove(pt);
            }
        }
    }
}

bool PresetTableV2Widget::ensurePositionDraftContextForCurrent()
{
    const int row = currentPositionEditRow();
    if (row < 0)
        return false;

    if (m_positionDraftDirty
            && (m_positionDraftCtx.row != row
                || m_positionDraftCtx.output != m_positionEditOutput
                || m_positionDraftCtx.selection != m_positionEditSelection))
    {
        if (!confirmDiscardPositionDraft())
            return false;
    }

    if (!m_positionDraftDirty)
    {
        m_positionDraftCtx.row = row;
        m_positionDraftCtx.output = m_positionEditOutput;
        m_positionDraftCtx.selection = m_positionEditSelection;
    }
    return true;
}

void PresetTableV2Widget::stagePositionValues(const QMap<QLCPoint, PTPositionValue>& values)
{
    if (values.isEmpty() || !ensurePositionDraftContextForCurrent())
        return;

    const int row = currentPositionEditRow();
    if (row < 0)
        return;

    const QSet<QLCPoint> editable = positionEditableCellsForLayer(
            row, m_positionEditOutput, m_positionEditSelection);
    bool applied = false;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
    {
        if (!editable.contains(it.key()))
            continue;

        PTPositionValue pos = it.value();
        if (pos.valid)
        {
            Fixture* fxi = fixtureAtPoint(it.key());
            const GroupHead gh = groupHeadAtPoint(it.key());
            pos = PTPositionConverter::clampPosition(fxi, gh.head, pos);
        }
        m_positionDraftCells.insert(it.key(), pos);
        applied = true;
    }

    if (!applied)
        return;

    m_positionDraftDirty = true;
    refreshPositionGridCells();
    updatePositionValueStrip();
    refreshPositionEditorFromSelection();
    updatePositionDraftButtons();
}

void PresetTableV2Widget::applyDraftToRowData(int targetRow, const PTPositionDraftContext& ctx,
                                              const QMap<QLCPoint, PTPositionValue>& cells)
{
    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it)
        writePositionToCells({it.key()}, it.value(), targetRow, ctx.output, ctx.selection);
}

void PresetTableV2Widget::stageFromEditorControls()
{
    const QSet<QLCPoint> targets = positionTargetCells();
    if (targets.isEmpty())
        return;

    const int row = currentPositionEditRow();
    if (row < 0)
        return;

    if (!ensurePositionDraftContextForCurrent())
        return;

    const bool symmetric = m_positionSymmetricSpreadCheck
            && m_positionSymmetricSpreadCheck->isChecked()
            && targets.size() >= 2;

    if (symmetric)
    {
        QList<QLCPoint> order = positionTargetCellOrder();

        const qreal centerPan = m_positionPanSpin->value();
        const qreal centerTilt = m_positionTiltSpin->value();
        const int spreadRaw = m_positionSpreadSlider ? m_positionSpreadSlider->value() : 128;
        const qreal spreadSigned = positionSpreadSigned(spreadRaw);
        const bool spreadPan = !m_positionSpreadAxisCombo
                || m_positionSpreadAxisCombo->currentIndex() == 0;

        const int n = order.size();
        const qreal centerIdx = (n > 0) ? (n - 1) / 2.0 : 0;
        const qreal maxOff = (n > 1)
                ? qMax(centerIdx, qreal(n - 1) - centerIdx) : 0;

        for (int i = 0; i < order.size(); ++i)
        {
            const QLCPoint& cellPt = order.at(i);
            if (!targets.contains(cellPt))
                continue;

            Fixture* fxi = fixtureAtPoint(cellPt);
            const GroupHead gh = groupHeadAtPoint(cellPt);
            if (!fxi)
                continue;

            PTPositionValue base;
            if (m_positionDraftDirty && m_positionDraftCells.contains(cellPt))
                base = m_positionDraftCells.value(cellPt);
            else
                base = positionValueForEditLayer(row, m_positionEditOutput,
                                                 m_positionEditSelection, cellPt);
            if (!base.valid)
                base = PTPositionConverter::centerPosition(fxi, gh.head);

            const QRectF range = PTPositionConverter::degreesRange(fxi, gh.head);
            const qreal panMin = range.left();
            const qreal panMax = range.left() + range.width();
            const qreal tiltMin = range.top();
            const qreal tiltMax = range.top() + range.height();
            const qreal panAmp = PTPositionConverter::symmetricHeadroom(centerPan, panMin, panMax);
            const qreal tiltAmp = PTPositionConverter::symmetricHeadroom(centerTilt, tiltMin, tiltMax);

            qreal panNorm = 0;
            qreal tiltNorm = 0;
            if (maxOff > 0)
            {
                const qreal off = (qreal(i) - centerIdx) / maxOff;
                panNorm = off;
                tiltNorm = qAbs(off);
            }

            PTPositionValue pos;
            pos.valid = true;
            if (spreadPan)
            {
                pos.panDeg = centerPan + spreadSigned * panNorm * panAmp;
                pos.tiltDeg = base.tiltDeg;
            }
            else
            {
                pos.panDeg = base.panDeg;
                pos.tiltDeg = centerTilt - spreadSigned * tiltNorm * tiltAmp;
            }
            m_positionDraftCells.insert(cellPt,
                    PTPositionConverter::clampPosition(fxi, gh.head, pos));
        }
    }
    else
    {
        const PTPositionValue pos = readPositionEditorValue();
        for (const QLCPoint& cellPt : targets)
            m_positionDraftCells.insert(cellPt, pos);
    }

    m_positionDraftDirty = true;
    refreshPositionGridCells();
    updatePositionValueStrip();
    updatePositionDraftButtons();
}

void PresetTableV2Widget::stagePositionEditorValue(const PTPositionValue& pos)
{
    const QSet<QLCPoint> targets = positionTargetCells();
    if (targets.isEmpty())
        return;

    const int row = currentPositionEditRow();
    if (row < 0)
        return;

    if (!ensurePositionDraftContextForCurrent())
        return;

    for (const QLCPoint& pt : targets)
        m_positionDraftCells.insert(pt, pos);

    m_positionDraftDirty = true;
    refreshPositionGridCells();
    updatePositionValueStrip();
    updatePositionDraftButtons();
}

void PresetTableV2Widget::clearPositionDraft()
{
    m_positionDraftDirty = false;
    m_positionDraftCtx = PTPositionDraftContext();
    m_positionDraftCells.clear();
    updatePositionDraftButtons();
}

void PresetTableV2Widget::flushPositionPromoteUiIfNeeded()
{
    bool refresh = false;
    {
        QMutexLocker lk(&m_stateMutex);
        if (m_positionPromoteUiRefresh)
        {
            m_positionPromoteUiRefresh = false;
            refresh = true;
        }
    }
    if (refresh && m_mode == PTMode::Position)
        refreshOperatePositionChrome();
}

void PresetTableV2Widget::updatePositionDraftButtons()
{
    const bool dirty = m_positionDraftDirty;
    if (m_positionOverwriteBtn)
        m_positionOverwriteBtn->setEnabled(dirty);
    if (m_positionSaveAsBtn)
        m_positionSaveAsBtn->setEnabled(dirty);
    if (m_positionRevertBtn)
        m_positionRevertBtn->setEnabled(dirty);
}

bool PresetTableV2Widget::confirmDiscardPositionDraft()
{
    if (!m_positionDraftDirty)
        return true;

    if (!m_positionConfirmDiscardDraft)
    {
        clearPositionDraft();
        refreshPositionGridCells();
        updatePositionValueStrip();
        refreshPositionEditorFromSelection();
        return true;
    }

    const QMessageBox::StandardButton btn = QMessageBox::question(
            this, tr("Unsaved position changes"),
            tr("You have unsaved position edits. Discard them?"),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Cancel);
    if (btn != QMessageBox::Discard)
    {
        selectPositionTreeLayer(m_positionDraftCtx.row, m_positionDraftCtx.output,
                                m_positionDraftCtx.selection);
        return false;
    }

    clearPositionDraft();
    refreshPositionGridCells();
    updatePositionValueStrip();
    refreshPositionEditorFromSelection();
    return true;
}

void PresetTableV2Widget::commitPositionDraftOverwrite()
{
    if (!m_positionDraftDirty)
        return;

    const PTPositionDraftContext ctx = m_positionDraftCtx;
    const QMap<QLCPoint, PTPositionValue> cells = m_positionDraftCells;

    applyDraftToRowData(ctx.row, ctx, cells);
    clearPositionDraft();
    rebuildTable();
    refreshPositionGridCells();
    updatePositionValueStrip();
    m_doc->setModified();
}

void PresetTableV2Widget::commitPositionDraftSaveAs()
{
    if (!m_positionDraftDirty)
        return;

    const int srcRow = m_positionDraftCtx.row;
    if (srcRow < 0 || srcRow >= m_rows.size())
        return;

    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save Position Preset As"),
                                               tr("Preset name:"), QLineEdit::Normal,
                                               tr("Preset %1").arg(m_rows.size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    PTRow newRow = m_rows.at(srcRow);
    newRow.name = name.trimmed();
    QHash<int, PTPositionOutputLayer> newOverrides;
    if (srcRow >= 0 && srcRow < m_positionOverrides.size())
        newOverrides = m_positionOverrides.at(srcRow);

    const PTPositionDraftContext ctx = m_positionDraftCtx;
    const QMap<QLCPoint, PTPositionValue> cells = m_positionDraftCells;

    int targetRow = -1;
    {
        QMutexLocker lk(&m_stateMutex);
        m_rows.append(newRow);
        m_positionOverrides.append(newOverrides);
        targetRow = m_rows.size() - 1;
    }
    applyDraftToRowData(targetRow, ctx, cells);

    clearPositionDraft();
    rebuildTable();
    m_positionEditRow = targetRow;
    m_positionEditOutput = ctx.output;
    m_positionEditSelection = ctx.selection;
    rebuildPositionEditor();
    m_doc->setModified();
}

void PresetTableV2Widget::writePositionEditorValue(const PTPositionValue& pos,
                                                   bool liveUpdateOnly)
{
    const QSet<QLCPoint> targets = positionTargetCells();
    if (targets.isEmpty())
        return;

    const int row = currentPositionEditRow();
    writePositionToCells(targets, pos, row,
                         m_positionEditOutput, m_positionEditSelection);

    if (liveUpdateOnly)
    {
        refreshPositionGridCells();
        updatePositionValueStrip();
    }
    else
    {
        rebuildTable();
    }
    m_doc->setModified();
}

void PresetTableV2Widget::slotPositionGridSelectionChanged(const QSet<QLCPoint>& cells,
                                                         const QList<QLCPoint>& order)
{
    m_positionSelectedCells = cells;
    m_positionSelectionOrder = order;
    if (m_positionGrid)
        m_positionGrid->setImplicitAllSelection(m_positionSelectedCells.isEmpty());
    refreshPositionEditorFromSelection();
    updatePositionValueStrip();
}

void PresetTableV2Widget::copyPositionSelectionToClipboard()
{
    if (m_mode != PTMode::Position)
        return;

    QSet<QLCPoint> cells = m_positionSelectedCells;
    if (cells.isEmpty())
        cells = positionTargetCells();
    if (cells.isEmpty())
        return;

    int minX = INT_MAX;
    int maxX = INT_MIN;
    int minY = INT_MAX;
    int maxY = INT_MIN;
    for (const QLCPoint& pt : cells)
    {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }

    const int row = currentPositionEditRow();
    QStringList lines;
    {
        QMutexLocker lk(&m_stateMutex);
        for (int y = minY; y <= maxY; ++y)
        {
            QStringList parts;
            for (int x = minX; x <= maxX; ++x)
            {
                const QLCPoint pt(x, y);
                if (!cells.contains(pt))
                {
                    parts << QString();
                    continue;
                }

                const PTPositionValue pos = positionValueForDisplay(
                        row, m_positionEditOutput, m_positionEditSelection, pt);
                parts << (pos.valid ? PTPositionConverter::formatPosition(pos) : QString());
            }
            lines << parts.join(QLatin1Char('\t'));
        }
    }
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

void PresetTableV2Widget::pastePositionClipboardToSelection()
{
    if (m_mode != PTMode::Position)
        return;

    QString text = QApplication::clipboard()->text();
    if (text.isEmpty())
        return;

    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    while (text.endsWith(QLatin1Char('\n')))
        text.chop(1);

    QStringList lines = text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    if (lines.isEmpty())
        return;

    QMap<QLCPoint, PTPositionValue> staged;
    const bool singleValue = (lines.size() == 1 && !lines.first().contains(QLatin1Char('\t')));

    if (singleValue)
    {
        const QString raw = lines.first().trimmed();
        for (const QLCPoint& pt : positionTargetCells())
        {
            PTPositionValue pos;
            if (!raw.isEmpty())
            {
                Fixture* fxi = fixtureAtPoint(pt);
                const GroupHead gh = groupHeadAtPoint(pt);
                pos = PTPositionConverter::parsePositionText(raw, fxi, gh.head);
                if (!pos.valid)
                    continue;
            }
            staged.insert(pt, pos);
        }
    }
    else
    {
        QLCPoint anchor = positionReferencePoint();
        if (anchor.x() < 0)
            return;

        const QSet<QLCPoint> editable = positionEditableCells();
        for (int li = 0; li < lines.size(); ++li)
        {
            const QStringList cols = lines.at(li).split(QLatin1Char('\t'), Qt::KeepEmptyParts);
            for (int ci = 0; ci < cols.size(); ++ci)
            {
                const QLCPoint pt(anchor.x() + ci, anchor.y() + li);
                if (!editable.contains(pt))
                    continue;

                const QString raw = cols.at(ci).trimmed();
                PTPositionValue pos;
                if (!raw.isEmpty())
                {
                    Fixture* fxi = fixtureAtPoint(pt);
                    const GroupHead gh = groupHeadAtPoint(pt);
                    pos = PTPositionConverter::parsePositionText(raw, fxi, gh.head);
                    if (!pos.valid)
                        continue;
                }
                staged.insert(pt, pos);
            }
        }
    }

    stagePositionValues(staged);
}

void PresetTableV2Widget::clearPositionSelectionToDraft()
{
    if (m_mode != PTMode::Position)
        return;

    QMap<QLCPoint, PTPositionValue> staged;
    const PTPositionValue invalid;
    for (const QLCPoint& pt : positionTargetCells())
        staged.insert(pt, invalid);
    stagePositionValues(staged);
}

void PresetTableV2Widget::editPositionCell(const QLCPoint& point)
{
    if (m_mode != PTMode::Position || !positionEditableCells().contains(point))
        return;

    const int row = currentPositionEditRow();
    if (row < 0)
        return;

    Fixture* fxi = fixtureAtPoint(point);
    const GroupHead gh = groupHeadAtPoint(point);
    if (!fxi)
        return;

    PTPositionValue pos;
    {
        QMutexLocker lk(&m_stateMutex);
        pos = positionValueForDisplay(row, m_positionEditOutput,
                                      m_positionEditSelection, point);
    }
    if (!pos.valid)
        pos = PTPositionConverter::centerPosition(fxi, gh.head);

    const QRectF range = PTPositionConverter::degreesRange(fxi, gh.head);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Edit position"));
    QFormLayout* form = new QFormLayout(&dlg);
    QDoubleSpinBox* pan = new QDoubleSpinBox(&dlg);
    pan->setSuffix(QStringLiteral("°"));
    pan->setDecimals(1);
    pan->setRange(range.left(), range.left() + range.width());
    pan->setValue(pos.panDeg);
    QDoubleSpinBox* tilt = new QDoubleSpinBox(&dlg);
    tilt->setSuffix(QStringLiteral("°"));
    tilt->setDecimals(1);
    tilt->setRange(range.top(), range.top() + range.height());
    tilt->setValue(pos.tiltDeg);
    form->addRow(tr("Pan"), pan);
    form->addRow(tr("Tilt"), tilt);
    QDialogButtonBox* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    PTPositionValue edited;
    edited.valid = true;
    edited.panDeg = pan->value();
    edited.tiltDeg = tilt->value();

    QSet<QLCPoint> targets;
    if (m_positionSelectedCells.contains(point) && m_positionSelectedCells.size() > 1)
        targets = positionTargetCells();
    else
        targets.insert(point);

    QMap<QLCPoint, PTPositionValue> staged;
    for (const QLCPoint& pt : targets)
        staged.insert(pt, edited);
    stagePositionValues(staged);
}

void PresetTableV2Widget::slotPositionGridCellEditRequested(const QLCPoint& point)
{
    editPositionCell(point);
}

void PresetTableV2Widget::slotPositionXYPadChanged(qreal xNorm, qreal yNorm)
{
    if (m_positionEditorSyncing || positionTargetCells().isEmpty())
        return;
    const QLCPoint pt = positionReferencePoint();
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    if (!fxi)
        return;

    qreal panDeg = 0;
    qreal tiltDeg = 0;
    PTPositionConverter::normalizedToDegrees(fxi, gh.head, xNorm, yNorm, panDeg, tiltDeg);
    m_positionEditorSyncing = true;
    m_positionPanSpin->setValue(panDeg);
    m_positionTiltSpin->setValue(tiltDeg);
    m_positionEditorSyncing = false;

    stageFromEditorControls();
}

void PresetTableV2Widget::slotPositionPanSpinChanged(double value)
{
    if (m_positionEditorSyncing)
        return;
    PTPositionValue pos = readPositionEditorValue();
    if (!pos.valid)
        return;
    pos.panDeg = value;
    m_positionEditorSyncing = true;
    const QLCPoint pt = positionReferencePoint();
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    if (fxi)
    {
        qreal xNorm = 0.5;
        qreal yNorm = 0.5;
        PTPositionConverter::degreesToNormalized(fxi, gh.head, pos.panDeg, pos.tiltDeg, xNorm, yNorm);
        m_positionXYPad->setNormalizedPosition(xNorm, yNorm);
    }
    m_positionEditorSyncing = false;
    stageFromEditorControls();
}

void PresetTableV2Widget::slotPositionTiltSpinChanged(double value)
{
    if (m_positionEditorSyncing)
        return;
    PTPositionValue pos = readPositionEditorValue();
    if (!pos.valid)
        return;
    pos.tiltDeg = value;
    m_positionEditorSyncing = true;
    const QLCPoint pt = positionReferencePoint();
    Fixture* fxi = fixtureAtPoint(pt);
    const GroupHead gh = groupHeadAtPoint(pt);
    if (fxi)
    {
        qreal xNorm = 0.5;
        qreal yNorm = 0.5;
        PTPositionConverter::degreesToNormalized(fxi, gh.head, pos.panDeg, pos.tiltDeg, xNorm, yNorm);
        m_positionXYPad->setNormalizedPosition(xNorm, yNorm);
    }
    m_positionEditorSyncing = false;
    stageFromEditorControls();
}

void PresetTableV2Widget::slotPositionSymmetricSpreadToggled(bool enabled)
{
    updatePositionSpreadChrome();
    if (enabled)
    {
        capturePositionSpreadPivotFromSelection();
        if (m_positionSpreadSlider)
        {
            QSignalBlocker blocker(m_positionSpreadSlider);
            m_positionSpreadSlider->setValue(128);
            updatePositionSpreadValueLabel(128);
        }
    }
    if (m_positionSymmetricSpreadCheck && m_positionSymmetricSpreadCheck->isChecked()
            && !positionTargetCells().isEmpty())
        stageFromEditorControls();
}

void PresetTableV2Widget::slotPositionSpreadChanged(int value)
{
    updatePositionSpreadValueLabel(value);
    if (m_positionEditorSyncing)
        return;
    if (m_positionSymmetricSpreadCheck && m_positionSymmetricSpreadCheck->isChecked()
            && positionTargetCells().size() >= 2)
        stageFromEditorControls();
}

void PresetTableV2Widget::slotPositionSpreadAxisChanged(int /*index*/)
{
    if (m_positionSpreadSlider)
    {
        QSignalBlocker blocker(m_positionSpreadSlider);
        m_positionSpreadSlider->setValue(128);
        updatePositionSpreadValueLabel(128);
    }
    if (m_positionEditorSyncing)
        return;
    if (m_positionSymmetricSpreadCheck && m_positionSymmetricSpreadCheck->isChecked()
            && positionTargetCells().size() >= 2)
        stageFromEditorControls();
}

void PresetTableV2Widget::slotPositionCopyLayer()
{
    const int row = currentPositionEditRow();
    if (row < 0)
        return;

    m_positionLayerClipboard.clear();
    const QVector<QLCPoint> points = positionTablePoints();
    {
        QMutexLocker lk(&m_stateMutex);
        for (const QLCPoint& pt : points)
        {
            const PTPositionValue pos = positionValueForEditLayer(
                    row, m_positionEditOutput, m_positionEditSelection, pt);
            if (pos.valid)
                m_positionLayerClipboard.insert(pt, pos);
        }
    }

    if (m_positionPasteLayerBtn)
        m_positionPasteLayerBtn->setEnabled(!m_positionLayerClipboard.isEmpty());
}

void PresetTableV2Widget::slotPositionPasteLayer()
{
    if (m_positionLayerClipboard.isEmpty())
        return;

    const int row = currentPositionEditRow();
    if (row < 0)
        return;

    stagePositionValues(m_positionLayerClipboard);
}

void PresetTableV2Widget::slotPositionCopyCell()
{
    copyPositionSelectionToClipboard();
}

void PresetTableV2Widget::slotPositionPasteCell()
{
    pastePositionClipboardToSelection();
}

void PresetTableV2Widget::slotPositionClearCell()
{
    clearPositionSelectionToDraft();
}

void PresetTableV2Widget::slotPositionOverwrite()
{
    commitPositionDraftOverwrite();
}

void PresetTableV2Widget::slotPositionSaveAs()
{
    commitPositionDraftSaveAs();
}

void PresetTableV2Widget::slotPositionRevert()
{
    if (!m_positionDraftDirty)
        return;
    clearPositionDraft();
    refreshPositionGridCells();
    updatePositionValueStrip();
    refreshPositionEditorFromSelection();
}

void PresetTableV2Widget::slotTableCurrentCellChanged(int /*row*/, int /*col*/)
{
}

// ==========================================================================
// Table rebuild from data
// ==========================================================================

void PresetTableV2Widget::rebuildTable()
{
    m_rebuildingTable = true;

    m_table->blockSignals(true);

    const bool positionMode = (m_mode == PTMode::Position);
    const QVector<QLCPoint> positionPoints = positionMode ? positionTablePoints() : QVector<QLCPoint>();
    int numCols = positionMode ? 1 + positionPoints.size() : 1 + m_columns.size();
    int numRows = m_rows.size();

    m_table->setRowCount(0);
    m_table->setColumnCount(numCols);

    // Header
    QStringList headers;
    headers << tr("Name");
    if (positionMode)
    {
        for (const QLCPoint& pt : positionPoints)
            headers << positionColumnHeader(pt);
    }
    else
    {
        for (const PTColumn& c : m_columns)
            headers << c.name;
    }
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setRowCount(numRows);

    for (int r = 0; r < numRows; ++r)
    {
        const PTRow& row = m_rows[r];

        // Name column — always plain text item
        QTableWidgetItem* nameItem = new QTableWidgetItem(row.name);
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        m_table->setItem(r, 0, nameItem);

        if (positionMode)
        {
            for (int c = 0; c < positionPoints.size(); ++c)
            {
                const QLCPoint pt = positionPoints.at(c);
                const PTPositionValue pos = row.positions.value(pt);
                const QString text = PTPositionConverter::formatPosition(pos);
                QTableWidgetItem* item = new QTableWidgetItem(text);
                item->setData(Qt::UserRole, text);
                item->setToolTip(positionCellTooltip(pt));
                m_table->setItem(r, c + 1, item);
            }
            continue;
        }

        // Value columns
        for (int c = 0; c < m_columns.size(); ++c)
        {
            uchar dmxVal = (c < row.values.size()) ? row.values[c] : 0;
            const PTColumn& ptcol = m_columns[c];

            QString displayText;
            QIcon   displayIcon;
            if (ptcol.type == PTColumn::Dropdown && !ptcol.options.isEmpty())
            {
                auto lbl = optionLabelFor(ptcol, int(dmxVal));
                displayText = lbl.first;
                displayIcon = makeItemIcon(lbl.second);
            }
            else if (ptcol.type == PTColumn::Scaler)
            {
                int sv = dmxToScaler(int(dmxVal), ptcol.scalerMin, ptcol.scalerMax);
                displayText = QString("%1%2").arg(sv).arg(ptcol.scalerSuffix);
            }
            else
            {
                displayText = QString::number(dmxVal);
            }

            QTableWidgetItem* item = new QTableWidgetItem(displayText);
            item->setData(Qt::UserRole, int(dmxVal));
            if (!displayIcon.isNull())
                item->setIcon(displayIcon);
            m_table->setItem(r, c + 1, item);
        }
    }

    // Apply persisted column widths
    if (m_nameColWidth > 0)
        m_table->setColumnWidth(0, m_nameColWidth);
    syncFrozenNameColumnLayout();
    for (int c = 0; c < m_columns.size(); ++c)
        if (m_columns[c].width > 0)
            m_table->setColumnWidth(c + 1, m_columns[c].width);

    m_table->blockSignals(false);
    m_rebuildingTable = false;

    refreshRowHighlights();
    updatePositionModeChrome();
    rebuildPositionEditor();
}

void PresetTableV2Widget::syncFrozenNameColumnLayout()
{
    if (!m_table || !m_nameFrozenTable)
        return;

    const int numCols = m_table->columnCount();
    const int numRows = m_table->rowCount();
    const int frozenWidth = m_nameColWidth > 0 ? m_nameColWidth : 140;

    m_nameFrozenTable->setModel(m_table->model());
    m_nameFrozenTable->setSelectionModel(m_table->selectionModel());
    m_nameFrozenTable->setColumnHidden(0, false);
    for (int c = 1; c < numCols; ++c)
        m_nameFrozenTable->setColumnHidden(c, true);

    m_nameFrozenTable->setColumnWidth(0, frozenWidth);
    m_nameFrozenTable->setMinimumWidth(frozenWidth + 2);
    m_nameFrozenTable->setFixedWidth(frozenWidth + 2);

    for (int r = 0; r < numRows; ++r)
        m_nameFrozenTable->setRowHeight(r, m_table->rowHeight(r));

    m_table->setColumnHidden(0, true);

    m_nameFrozenTable->horizontalHeader()->setVisible(true);
    m_table->horizontalHeader()->setVisible(true);
    m_nameFrozenTable->horizontalHeader()->updateGeometry();
    m_table->horizontalHeader()->updateGeometry();
    m_nameFrozenTable->viewport()->update();
    m_table->viewport()->update();
}

// ==========================================================================
// Cell changed — sync back to m_rows
// ==========================================================================

void PresetTableV2Widget::slotCellChanged(int row, int col)
{
    if (m_rebuildingTable) return;
    if (row < 0 || row >= m_rows.size()) return;

    QTableWidgetItem* item = m_table->item(row, col);
    if (!item) return;

    QMutexLocker lk(&m_stateMutex);

    if (col == 0)
    {
        // refreshRowHighlights uses blockSignals so badge text never reaches here
        m_rows[row].name = item->text();
    }
    else
    {
        if (m_mode == PTMode::Position)
        {
            const QVector<QLCPoint> points = positionTablePoints();
            const int pointCol = col - 1;
            if (pointCol >= 0 && pointCol < points.size())
            {
                const QLCPoint pt = points.at(pointCol);
                Fixture* fxi = fixtureAtPoint(pt);
                const GroupHead gh = groupHeadAtPoint(pt);
                const PTPositionValue pos =
                        PTPositionConverter::parsePositionText(item->text(), fxi, gh.head);
                const QString text = PTPositionConverter::formatPosition(pos);
                item->setText(text);
                item->setData(Qt::UserRole, text);
                if (pos.valid)
                    m_rows[row].positions.insert(pt, pos);
                else
                    m_rows[row].positions.remove(pt);
            }
            return;
        }
        int valCol = col - 1;
        if (valCol < m_rows[row].values.size())
            m_rows[row].values[valCol] = uchar(item->data(Qt::UserRole).toInt());
    }
}

// ==========================================================================
// Row highlights (Operate: badge with output number)
// ==========================================================================

void PresetTableV2Widget::setActiveRow(int outputIdx, int rowIdx)
{
    {
        QMutexLocker lk(&m_stateMutex);
        if (outputIdx < 0 || outputIdx >= m_activeRow.size()) return;
        const int prevRow = m_activeRow[outputIdx];
        m_activeRow[outputIdx] = rowIdx;
        if (rowIdx != prevRow)
            bumpMultiButtonStateRevisionLocked(
                    outputIdx, PresetTableV2MultiButtonTargetIface::PrimaryRow);
        if (outputIdx < m_spatialAppliedRow.size() && rowIdx < 0)
            m_spatialAppliedRow[outputIdx] = -1;
        if (outputIdx >= 0 && outputIdx < m_spatialChase.size())
            m_spatialChase[outputIdx] = PTSpatialChaseOutput();

        if (useMatrixEngineLocked() && rowIdx >= 0 && rowIdx != prevRow)
        {
            ensureMatrixState(outputIdx);
            PTOutputMatrixState& st = m_matrixState[outputIdx];
            if (sweepOnPrimaryChangeLocked(outputIdx, rowIdx) && !st.flashActive)
                beginMatrixSweepLocked(outputIdx, prevRow, rowIdx);
            else if (!st.flashActive)
            {
                st.sweepRunning = false;
                st.appliedRow = rowIdx;
            }
        }
        else if (rowIdx >= 0 && useMatrixEngineLocked())
        {
            ensureMatrixState(outputIdx);
            if (!m_matrixState[outputIdx].sweepRunning && !m_matrixState[outputIdx].flashActive)
                m_matrixState[outputIdx].appliedRow = rowIdx;
        }
    }
    refreshRowHighlights();
    refreshOperatePositionChrome(outputIdx);
    sendFeedback(rowIdx + 1, quint8(outputIdx));   // 0 = off when rowIdx == -1 → gives 0
    update();
}

bool PresetTableV2Widget::spatialEffectsEnabled() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_spatialEffects.enabled;
}

void PresetTableV2Widget::setSpatialEffectsEnabled(bool enabled)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_spatialEffects.enabled = enabled;
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        if (!enabled)
            resetAllMatrixStatesLocked();
    }
    refreshTransitionPresetCache();
}

PTSpatialEffectSettings PresetTableV2Widget::spatialEffectSettings() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_spatialEffects;
}

void PresetTableV2Widget::setSpatialEffectSettings(const PTSpatialEffectSettings& settings)
{
    QMutexLocker lk(&m_stateMutex);
    m_spatialEffects = settings;
    m_spatialAppliedRow.fill(-1);
    m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
}

quint32 PresetTableV2Widget::linkedTransitionWidgetId() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_linkedTransitionWidgetId;
}

void PresetTableV2Widget::setLinkedTransitionWidgetId(quint32 id)
{
    {
        QMutexLocker lk(&m_stateMutex);
        m_linkedTransitionWidgetId = id;
    }
    refreshTransitionPresetCache();
}

void PresetTableV2Widget::refreshTransitionPresetCache()
{
    QMutexLocker lk(&m_stateMutex);
    m_cachedTransitionWidgetId = m_linkedTransitionWidgetId;
    m_cachedTransitionSweepCount = 0;
    m_cachedTransitionContinuousCount = 0;
    m_cachedTransitionMultiFxCount = 0;
    if (m_linkedTransitionWidgetId == VCWidget::invalidId())
        return;

    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
    {
        m_cachedTransitionSweepCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
        m_cachedTransitionContinuousCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
        m_cachedTransitionMultiFxCount = provider->transitionPresetCount(PTTransitionMode::MultiFx);
    }
}

void PresetTableV2Widget::syncLiveTransitionFromOutputs()
{
    m_liveSweepPreset.resize(m_outputs.size());
    m_liveContinuousPreset.resize(m_outputs.size());
    m_liveMultiFxPreset.resize(m_outputs.size());
    m_liveSecondaryRow.resize(m_outputs.size());
    m_stagedRow.resize(m_outputs.size());
    m_stagedRow.fill(-1);
    m_stagedRowValid.resize(m_outputs.size());
    m_stagedSecondaryRow.resize(m_outputs.size());
    m_stagedSweepPreset.resize(m_outputs.size());
    m_stagedContinuousPreset.resize(m_outputs.size());
    m_stagedMultiFxPreset.resize(m_outputs.size());
    m_stagedRowValid.fill(false);
    m_stagedSecondaryValid.resize(m_outputs.size());
    m_stagedSweepValid.resize(m_outputs.size());
    m_stagedContinuousValid.resize(m_outputs.size());
    m_stagedMultiFxValid.resize(m_outputs.size());
    m_stagedSecondaryValid.fill(false);
    m_stagedSweepValid.fill(false);
    m_stagedContinuousValid.fill(false);
    m_stagedMultiFxValid.fill(false);
    ensureMultiButtonRevisionSizeLocked();
    m_continuousElapsedMs.resize(m_outputs.size());
    m_multiFxElapsedMs.resize(m_outputs.size());
    m_multiFxStagedElapsedMs.resize(m_outputs.size());
    m_continuousLastCycleMs.resize(m_outputs.size());
    m_multiFxLastCycleMs.resize(m_outputs.size());
    m_multiFxStagedLastCycleMs.resize(m_outputs.size());
    m_selectionMatrixStateSlots.clear();
    m_matrixState.resize(m_outputs.size());
    m_flashInputHeldRow.resize(m_outputs.size());
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int prevSweep = (o < m_liveSweepPreset.size()) ? m_liveSweepPreset[o] : -1;
        m_liveSweepPreset[o] = m_outputs[o].sweepPresetIndex;
        m_liveContinuousPreset[o] = m_outputs[o].continuousPresetIndex;
        m_liveMultiFxPreset[o] = m_outputs[o].multiFxPresetIndex;
        m_liveSecondaryRow[o] = -1;
        if (m_liveSweepPreset[o] < 0 && m_liveContinuousPreset[o] < 0)
            resetMatrixStateLocked(o);
        else if (m_liveSweepPreset[o] != prevSweep)
        {
            resetMatrixStateLocked(o);
            if (o < m_spatialAppliedRow.size())
                m_spatialAppliedRow[o] = -1;
            if (o < m_spatialChase.size())
                m_spatialChase[o] = PTSpatialChaseOutput();
        }
    }
}

int PresetTableV2Widget::liveSweepPresetIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    return (outputIdx < m_liveSweepPreset.size())
            ? m_liveSweepPreset[outputIdx] : m_outputs[outputIdx].sweepPresetIndex;
}

int PresetTableV2Widget::liveContinuousPresetIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    return (outputIdx < m_liveContinuousPreset.size())
            ? m_liveContinuousPreset[outputIdx] : m_outputs[outputIdx].continuousPresetIndex;
}

int PresetTableV2Widget::liveMultiFxPresetIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    return (outputIdx < m_liveMultiFxPreset.size())
            ? m_liveMultiFxPreset[outputIdx] : m_outputs[outputIdx].multiFxPresetIndex;
}

PTTransitionPreset PresetTableV2Widget::transitionPresetAtIndexLocked(PTTransitionMode mode,
                                                                      int presetIndex,
                                                                      int outputIdx,
                                                                      const QLCPoint* point) const
{
    if (presetIndex < 0)
    {
        PTTransitionPreset instant;
        instant.name = QStringLiteral("Instant");
        instant.enabled = false;
        return instant;
    }

    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
    {
        if (presetIndex < provider->transitionPresetCount(mode))
        {
            if (point != nullptr)
                return provider->effectiveTransitionPresetForPoint(mode, presetIndex,
                                                                   outputIdx, *point);
            return provider->effectiveTransitionPresetForOutput(mode, presetIndex, outputIdx);
        }
    }

    return PresetTableV2SpatialEngine::presetFromLegacySpatial(m_spatialEffects);
}

PTTransitionPreset PresetTableV2Widget::sweepPresetForOutputLocked(int outputIdx) const
{
    return transitionPresetAtIndexLocked(PTTransitionMode::SweepOnly,
                                         liveSweepPresetIndexLocked(outputIdx), outputIdx);
}

PTTransitionPreset PresetTableV2Widget::continuousPresetForOutputLocked(int outputIdx) const
{
    return transitionPresetAtIndexLocked(PTTransitionMode::Continuous,
                                         liveContinuousPresetIndexLocked(outputIdx), outputIdx);
}

PTTransitionPreset PresetTableV2Widget::multiFxPresetForOutputLocked(int outputIdx) const
{
    return transitionPresetAtIndexLocked(PTTransitionMode::MultiFx,
                                         liveMultiFxPresetIndexLocked(outputIdx), outputIdx);
}

PTTransitionPreset PresetTableV2Widget::continuousPresetForOutputLocked(int outputIdx,
                                                                        uchar xfEffective) const
{
    Q_UNUSED(xfEffective);
    const PTTransitionPreset live = continuousPresetForOutputLocked(outputIdx);
    if (outputIdx < 0 || outputIdx >= m_stagedContinuousPreset.size()
            || outputIdx >= m_stagedContinuousValid.size()
            || !m_stagedContinuousValid[outputIdx])
        return live;

    return transitionPresetAtIndexLocked(
            PTTransitionMode::Continuous, m_stagedContinuousPreset[outputIdx], outputIdx);
}

static QVector<uchar> blendRowValues(const QVector<uchar>& live,
                                     const QVector<uchar>& staged,
                                     double progress)
{
    const int count = qMax(live.size(), staged.size());
    QVector<uchar> out;
    out.resize(count);
    const qint16 xf = qint16(qBound(0, int(progress * 255.0 + 0.5), 255));
    for (int i = 0; i < count; ++i)
    {
        const uchar a = (i < live.size()) ? live[i] : 0;
        const uchar b = (i < staged.size()) ? staged[i] : a;
        out[i] = uchar(a + qint16(b - a) * xf / 255);
    }
    return out;
}

PresetTableV2Widget::PTContinuousLayerState
PresetTableV2Widget::continuousLayerStateForOutputLocked(int outputIdx,
                                                         int activeRow,
                                                         uchar xfEffective) const
{
    PTContinuousLayerState state;
    if (outputIdx < 0 || outputIdx >= m_outputs.size()
            || activeRow < 0 || activeRow >= m_rows.size())
        return state;

    Q_UNUSED(xfEffective);
    const bool hasStagedPrimary = outputIdx < m_stagedRowValid.size()
            && m_stagedRowValid[outputIdx]
            && outputIdx < m_stagedRow.size()
            && m_stagedRow[outputIdx] != activeRow
            && m_stagedRow[outputIdx] < m_rows.size();
    const bool hasStagedSecondary = outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx]
            && outputIdx < m_stagedSecondaryRow.size();
    const bool hasStagedContinuous = outputIdx < m_stagedContinuousValid.size()
            && m_stagedContinuousValid[outputIdx];
    const bool hasStagedMultiFx = hasStagedMultiFxPresetLocked(outputIdx);

    const int liveSecondary = effectiveSecondaryRowLocked(outputIdx, activeRow);
    state.primaryRow = activeRow;
    const int stagedSecondary = hasStagedSecondary
            ? ((m_stagedSecondaryRow[outputIdx] >= 0
                && m_stagedSecondaryRow[outputIdx] < m_rows.size())
               ? m_stagedSecondaryRow[outputIdx]
               : liveSecondary)
            : -1;
    state.secondaryRow = hasStagedSecondary ? stagedSecondary : liveSecondary;
    state.livePrimaryValues = m_rows[activeRow].values;
    state.primaryValues = state.livePrimaryValues;
    if (hasStagedPrimary && m_stagedRow[outputIdx] >= 0)
        state.primaryValues = m_rows[m_stagedRow[outputIdx]].values;
    else if (hasStagedPrimary)
        state.primaryValues = QVector<uchar>(m_columns.size(), uchar(0));

    if (liveSecondary >= 0 && liveSecondary < m_rows.size())
        state.liveSecondaryValues = m_rows[liveSecondary].values;
    else
        state.liveSecondaryValues = state.livePrimaryValues;
    state.secondaryValues = state.liveSecondaryValues;

    if (hasStagedSecondary && stagedSecondary >= 0 && stagedSecondary < m_rows.size())
        state.secondaryValues = m_rows[stagedSecondary].values;

    state.livePreset = continuousPresetForOutputLocked(outputIdx);
    state.preset = hasStagedContinuous
            ? continuousPresetForOutputLocked(outputIdx, xfEffective)
            : state.livePreset;
    state.hasStaged = hasStagedPrimary || hasStagedSecondary
            || hasStagedContinuous || hasStagedMultiFx;
    state.active = (state.livePreset.enabled || state.preset.enabled
                    || multiFxActiveForOutputLocked(outputIdx))
            && (state.secondaryRow >= 0 || hasStagedSecondary || hasStagedContinuous
                || hasStagedPrimary || hasStagedMultiFx);
    return state;
}

bool PresetTableV2Widget::sweepEfxActiveForOutputLocked(int outputIdx) const
{
    return liveSweepPresetIndexLocked(outputIdx) >= 0;
}

bool PresetTableV2Widget::continuousEfxActiveForOutputLocked(int outputIdx) const
{
    if (liveContinuousPresetIndexLocked(outputIdx) >= 0)
        return true;
    return outputIdx >= 0 && outputIdx < m_stagedContinuousValid.size()
            && m_stagedContinuousValid[outputIdx];
}

bool PresetTableV2Widget::multiFxActiveForOutputLocked(int outputIdx) const
{
    return liveMultiFxPresetIndexLocked(outputIdx) >= 0
            || hasStagedMultiFxPresetLocked(outputIdx);
}

bool PresetTableV2Widget::hasStagedMultiFxPresetLocked(int outputIdx) const
{
    return outputIdx >= 0
            && outputIdx < m_stagedMultiFxValid.size()
            && outputIdx < m_stagedMultiFxPreset.size()
            && m_stagedMultiFxValid[outputIdx];
}

int PresetTableV2Widget::stagedMultiFxPresetIndexLocked(int outputIdx) const
{
    if (!hasStagedMultiFxPresetLocked(outputIdx))
        return -1;
    return m_stagedMultiFxPreset[outputIdx];
}

bool PresetTableV2Widget::hasStagedMultiFxAnyLocked() const
{
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        if (hasStagedMultiFxPresetLocked(o))
            return true;
    }
    return false;
}

bool PresetTableV2Widget::sweepOnPrimaryChangeLocked(int outputIdx, int newActiveRow) const
{
    if (!sweepEfxActiveForOutputLocked(outputIdx))
        return false;
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    if (continuousEfxActiveForOutputLocked(outputIdx)
            && (hasStagedSecondary || effectiveSecondaryRowLocked(outputIdx, newActiveRow) >= 0))
        return false;
    return true;
}

int PresetTableV2Widget::rawLiveSecondaryRowIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;

    return (outputIdx < m_liveSecondaryRow.size()) ? m_liveSecondaryRow[outputIdx] : -1;
}

int PresetTableV2Widget::liveSecondaryRowIndexLocked(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;

    const int live = rawLiveSecondaryRowIndexLocked(outputIdx);
    if (live >= 0 && live < m_rows.size())
        return live;

    const int prop = m_outputs[outputIdx].secondaryRowIndex;
    if (prop >= 0 && prop < m_rows.size())
        return prop;

    return -1;
}

int PresetTableV2Widget::effectiveSecondaryRowLocked(int outputIdx, int activeRow) const
{
    Q_UNUSED(activeRow);
    return liveSecondaryRowIndexLocked(outputIdx);
}

void PresetTableV2Widget::sendLiveSelectorFeedbackLocked(int outputIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size()
            || outputIdx >= PTInputId::kMaxRoutableOutputs)
        return;

    const int livePrimary = (outputIdx < m_activeRow.size()) ? m_activeRow[outputIdx] : -1;
    sendFeedback(livePrimary < 0 ? 0 : livePrimary + 1, PTInputId::rowSelector(outputIdx));

    const int liveSweep = liveSweepPresetIndexLocked(outputIdx);
    sendFeedback(liveSweep < 0 ? 0 : liveSweep + 1, PTInputId::transSweep(outputIdx));

    const int liveContinuous = liveContinuousPresetIndexLocked(outputIdx);
    sendFeedback(liveContinuous < 0 ? 0 : liveContinuous + 1,
                 PTInputId::transContinuousBank(outputIdx));

    const int liveMultiFx = liveMultiFxPresetIndexLocked(outputIdx);
    sendFeedback(liveMultiFx < 0 ? 0 : liveMultiFx + 1, PTInputId::multiFxBank(outputIdx));

    const int liveSecondary = rawLiveSecondaryRowIndexLocked(outputIdx);
    sendFeedback(liveSecondary < 0 ? 0 : liveSecondary + 1,
                 PTInputId::transSecondaryRow(outputIdx));
}

bool PresetTableV2Widget::continuousCrossfadeModeLocked(int outputIdx) const
{
    if (!m_crossfadeEnabled || !continuousEfxActiveForOutputLocked(outputIdx))
        return false;
    const bool hasStagedContinuous = outputIdx >= 0
            && outputIdx < m_stagedContinuousValid.size()
            && m_stagedContinuousValid[outputIdx];
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    return hasStagedContinuous || hasStagedSecondary
            || effectiveSecondaryRowLocked(outputIdx, m_activeRow[outputIdx]) >= 0;
}

bool PresetTableV2Widget::multiFxCrossfadeModeLocked(int outputIdx) const
{
    if (!m_crossfadeEnabled || !multiFxActiveForOutputLocked(outputIdx))
        return false;
    const int activeRow = (outputIdx >= 0 && outputIdx < m_activeRow.size())
            ? m_activeRow[outputIdx] : -1;
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    const bool hasStagedPrimary = outputIdx >= 0
            && outputIdx < m_stagedRowValid.size()
            && m_stagedRowValid[outputIdx]
            && outputIdx < m_stagedRow.size()
            && m_stagedRow[outputIdx] != activeRow
            && m_stagedRow[outputIdx] < m_rows.size();
    return hasStagedSecondary || hasStagedPrimary
            || hasStagedMultiFxPresetLocked(outputIdx)
            || effectiveSecondaryRowLocked(outputIdx, activeRow) >= 0;
}

bool PresetTableV2Widget::crossfadeSweepModeLocked(int outputIdx, int activeRow, bool hasStaged) const
{
    if (!m_crossfadeEnabled || !hasStaged || !sweepEfxActiveForOutputLocked(outputIdx))
        return false;
    if (m_mode == PTMode::Position)
        return true;
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx];
    if (continuousEfxActiveForOutputLocked(outputIdx)
            && (hasStagedSecondary || effectiveSecondaryRowLocked(outputIdx, activeRow) >= 0))
        return false;
    return true;
}

bool PresetTableV2Widget::continuousCrossfadeActiveAnyLocked() const
{
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        if (continuousCrossfadeModeLocked(o))
            return true;
    }
    return false;
}

bool PresetTableV2Widget::continuousFxSelectionStagedAnyLocked() const
{
    for (int o = 0; o < m_stagedContinuousValid.size(); ++o)
    {
        if (m_stagedContinuousValid[o])
            return true;
    }
    for (int o = 0; o < m_stagedMultiFxValid.size(); ++o)
    {
        if (m_stagedMultiFxValid[o])
            return true;
    }
    return false;
}

bool PresetTableV2Widget::continuousFxSelectorToStagedLocked() const
{
    return m_crossfadeEnabled
            && m_continuousFxSelectorMode != PTContinuousFxSelectorMode::Live
            && crossfadeRoutesToStagedLocked();
}

bool PresetTableV2Widget::crossfadeManualControlEnabledLocked() const
{
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        return provider->crossfadeManualControlEnabled();
    return true;
}

bool PresetTableV2Widget::crossfadeRoutesToStagedLocked() const
{
    return m_crossfadeEnabled && m_crossfadeEditLaneStaged;
}

bool PresetTableV2Widget::crossfadeHasStagedChangesLocked() const
{
    for (int o = 0; o < m_stagedRow.size(); ++o)
    {
        const int liveRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const bool stagedValid = o < m_stagedRowValid.size() && m_stagedRowValid[o];
        if (stagedValid && m_stagedRow[o] != liveRow)
            return true;
    }
    for (int o = 0; o < m_stagedSecondaryValid.size(); ++o)
    {
        if (m_stagedSecondaryValid[o])
            return true;
    }
    for (int o = 0; o < m_stagedContinuousValid.size(); ++o)
    {
        if (m_stagedContinuousValid[o])
            return true;
    }
    for (int o = 0; o < m_stagedMultiFxValid.size(); ++o)
    {
        if (m_stagedMultiFxValid[o])
            return true;
    }
    return false;
}

void PresetTableV2Widget::armCrossfadeStagingLocked()
{
    if (!m_crossfadeEnabled || !crossfadeManualControlEnabledLocked())
        return;
    if (crossfadeHasStagedChangesLocked())
        return;

    m_crossfadeSessionActive = true;
    m_crossfadeEditLaneStaged = true;
    resetMultiFxCrossfadePhaseAnchorLocked();
    m_crossfadeStagedAtLowSide = crossfadeLowSideFromPosition(m_crossfadeGlobalPos);
    m_crossfadeStartPos = (crossfadeAtLowEdge(m_crossfadeGlobalPos)
            || crossfadeAtHighEdge(m_crossfadeGlobalPos))
            ? crossfadeNormalizedEdge(m_crossfadeGlobalPos)
            : m_crossfadeGlobalPos;
}

void PresetTableV2Widget::tickCrossfadeClockLocked(MasterTimer* timer)
{
    if (!m_crossfadeEnabled || !timer)
        return;

    const bool manual = crossfadeManualControlEnabledLocked();
    if (manual != m_crossfadeLastManualControl)
    {
        m_crossfadeLastManualControl = manual;
        resetCrossfadeClockLocked();
        if (manual)
        {
            m_crossfadeStagedAtLowSide = crossfadeLowSideFromPosition(m_crossfadeGlobalPos);
            m_crossfadeStartPos = (crossfadeAtLowEdge(m_crossfadeGlobalPos)
                    || crossfadeAtHighEdge(m_crossfadeGlobalPos))
                    ? crossfadeNormalizedEdge(m_crossfadeGlobalPos)
                    : m_crossfadeGlobalPos;
        }
    }

    if (manual)
        return;

    const double prev = m_crossfadeClockProgress01;
    const PTGlobalEffectSettings global = globalEffectSettingsLocked();
    const quint32 cycleMs = qMax(quint32(1), crossfadeClockCycleMsLocked(global));
    if (m_crossfadeClockLastCycleMs > 0 && m_crossfadeClockLastCycleMs != cycleMs)
        rescaleElapsedForDurationChange(m_crossfadeClockElapsedMs,
                                        m_crossfadeClockLastCycleMs,
                                        cycleMs);
    m_crossfadeClockLastCycleMs = cycleMs;
    m_crossfadeClockElapsedMs += timer->tick();
    m_crossfadeClockProgress01 = qMin(1.0, double(m_crossfadeClockElapsedMs)
            / double(cycleMs));

    if (prev < 1.0 && m_crossfadeClockProgress01 >= 1.0
            && crossfadeHasStagedChangesLocked())
    {
        promoteStagedToLiveLocked();
        resetCrossfadeClockLocked();
    }

    syncMultiFxPhaseOnCrossfadeMotionLocked();
}

void PresetTableV2Widget::resetMultiFxCrossfadePhaseAnchorLocked()
{
    m_multiFxXfPhaseAnchored = false;
    m_multiFxStagedHoldTicksRemaining = 0;
}

void PresetTableV2Widget::syncMultiFxPhaseOnCrossfadeMotionLocked()
{
    if (!m_syncMultiFxPhaseToCrossfade || !m_crossfadeEnabled
            || !hasStagedMultiFxAnyLocked())
        return;

    double prerunFraction01 = 0.0;
    if (!crossfadeManualControlEnabledLocked())
        prerunFraction01 = m_crossfadeClockProgress01;
    else
        prerunFraction01 = cueListCrossfadeProgress01(
                m_crossfadeGlobalPos, m_crossfadeStartPos, m_crossfadeStagedAtLowSide);

    if (prerunFraction01 <= 0.0)
    {
        resetMultiFxCrossfadePhaseAnchorLocked();
        return;
    }

    if (m_multiFxXfPhaseAnchored)
        return;

    while (m_multiFxStagedElapsedMs.size() < m_outputs.size())
        m_multiFxStagedElapsedMs.append(0);
    while (m_multiFxStagedLastCycleMs.size() < m_outputs.size())
        m_multiFxStagedLastCycleMs.append(0);
    m_multiFxStagedElapsedMs.fill(0, m_outputs.size());
    m_multiFxStagedLastCycleMs.fill(0, m_outputs.size());
    m_multiFxStagedHoldTicksRemaining = 0;
    m_multiFxXfPhaseAnchored = true;
}

double PresetTableV2Widget::crossfadeProgress01Locked(uchar xfEffective) const
{
    Q_UNUSED(xfEffective);

    if (!m_crossfadeEnabled)
        return 0.0;
    if (crossfadeManualControlEnabledLocked())
        return cueListCrossfadeProgress01(
                m_crossfadeGlobalPos, m_crossfadeStartPos, m_crossfadeStagedAtLowSide);
    return m_crossfadeClockProgress01;
}

void PresetTableV2Widget::resetCrossfadeClockLocked()
{
    m_crossfadeClockElapsedMs = 0;
    m_crossfadeClockLastCycleMs = 0;
    m_crossfadeClockProgress01 = 0.0;
}

quint32 PresetTableV2Widget::crossfadeClockCycleMsLocked(
        const PTGlobalEffectSettings& global) const
{
    PTTransitionPreset dur;
    dur.speedMultiplier = global.speedMultiplier;
    return PTParamMatrixEngine::effectiveDurationMs(global, dur, false);
}

uchar PresetTableV2Widget::crossfadeEffectiveLocked(uchar xfPos, uchar xfStartPos) const
{
    if (!m_crossfadeEnabled)
        return 0;

    if (!crossfadeManualControlEnabledLocked())
        return uchar(qBound(0, int(m_crossfadeClockProgress01 * 255.0 + 0.5), 255));

    const double progress = cueListCrossfadeProgress01(
            xfPos, xfStartPos, m_crossfadeStagedAtLowSide);
    return uchar(qBound(0, int(progress * 255.0 + 0.5), 255));
}

bool PresetTableV2Widget::continuousCrossfadeStagedEditing() const
{
    QMutexLocker lk(&m_stateMutex);
    return crossfadeRoutesToStagedLocked()
            && continuousCrossfadeActiveAnyLocked();
}

int PresetTableV2Widget::outputCountForPresetOverrides() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_outputs.size();
}

QString PresetTableV2Widget::outputNameForPresetOverride(int outputIdx) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return QString();
    const QString name = m_outputs.at(outputIdx).name;
    return name.isEmpty() ? tr("Output %1").arg(outputIdx + 1) : name;
}

QList<QLCPoint> PresetTableV2Widget::outputPointsForPresetOverride(int outputIdx) const
{
    QMutexLocker lk(&m_stateMutex);
    QList<QLCPoint> result;
    if ((m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position)
            || outputIdx < 0 || outputIdx >= m_outputs.size()
            || m_fixtureGroupId == UINT_MAX || !m_doc)
        return result;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return result;

    const PTOutput out = m_outputs.at(outputIdx);
    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    if (out.scope == PTOutputScope::Mask && !docMask.isActive())
        return result;

    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead>& headsMap =
            (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        if (outputScopeAllowsPoint(out.scope, it.key(), out))
            result.append(it.key());
    }
    std::sort(result.begin(), result.end(), [](const QLCPoint& a, const QLCPoint& b) {
        return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
    });
    return result;
}

int PresetTableV2Widget::fixtureGroupSpanAlongAxis(const PTTransitionPreset& preset,
                                                   const PTGlobalEffectSettings& global) const
{
    if ((m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position)
            || m_fixtureGroupId == UINT_MAX || !m_doc)
        return 0;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return 0;

    const QSize sz = grp->size();
    return PTDimmerWaveEngine::gridSpanAlongAxis(
            sz.width(), sz.height(), preset.axis, global.fxOrientation);
}

bool PresetTableV2Widget::spatialGridPreview(const PTTransitionPreset& preset,
                                            const PTGlobalEffectSettings& global,
                                            PTSpatialGridPreview& out) const
{
    out = PTSpatialGridPreview();
    if ((m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position)
            || m_fixtureGroupId == UINT_MAX || !m_doc)
        return false;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return false;

    const QSize sz = grp->size();
    if (sz.width() <= 0 || sz.height() <= 0)
        return false;

    QList<QLCPoint> points;
    const QMap<QLCPoint, GroupHead> heads = grp->headsMap();
    for (auto it = heads.constBegin(); it != heads.constEnd(); ++it)
        points.append(it.key());

    out = PTSpatialFixturePlan::buildGridPreview(
            points, preset, global, sz.width(), sz.height());
    if (!out.valid)
        return false;

    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const PTOutput& ptOut = m_outputs[o];
        if (ptOut.scope == PTOutputScope::Mask && !docMask.isActive())
            continue;

        const QMap<QLCPoint, GroupHead>& scopeHeads =
                (ptOut.scope == PTOutputScope::Rows) ? heads : maskedHeads;
        for (auto it = scopeHeads.constBegin(); it != scopeHeads.constEnd(); ++it)
        {
            const QLCPoint& pt = it.key();
            if (!outputScopeAllowsPoint(ptOut.scope, pt, ptOut))
                continue;
            auto cellIt = out.cells.find(pt);
            if (cellIt != out.cells.end() && !cellIt->outputIndexes.contains(o))
                cellIt->outputIndexes.append(o);
        }
    }
    return out.valid;
}

bool PresetTableV2Widget::spatialGridPreviewForOutput(int outputIdx,
                                                       const PTTransitionPreset& preset,
                                                       const PTGlobalEffectSettings& global,
                                                       PTSpatialGridPreview& out) const
{
    out = PTSpatialGridPreview();
    if ((m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position)
            || m_fixtureGroupId == UINT_MAX || !m_doc)
        return false;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp || outputIdx < 0 || outputIdx >= m_outputs.size())
        return false;

    const QSize sz = grp->size();
    if (sz.width() <= 0 || sz.height() <= 0)
        return false;

    const PTOutput& ptOut = m_outputs.at(outputIdx);
    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    if (ptOut.scope == PTOutputScope::Mask && !docMask.isActive())
        return false;

    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead>& scopeHeads =
            (ptOut.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;

    QList<QLCPoint> points;
    for (auto it = scopeHeads.constBegin(); it != scopeHeads.constEnd(); ++it)
    {
        if (outputScopeAllowsPoint(ptOut.scope, it.key(), ptOut))
            points.append(it.key());
    }
    if (points.isEmpty())
        return false;

    out = PTSpatialFixturePlan::buildGridPreview(
            points, preset, global, sz.width(), sz.height());
    if (!out.valid)
        return false;

    for (const QLCPoint& pt : points)
    {
        auto cellIt = out.cells.find(pt);
        if (cellIt != out.cells.end() && !cellIt->outputIndexes.contains(outputIdx))
            cellIt->outputIndexes.append(outputIdx);
    }
    return out.valid;
}

int PresetTableV2Widget::multiButtonOutputCount() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_outputs.size();
}

QString PresetTableV2Widget::multiButtonOutputName(int outputIdx) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return QString();
    const QString name = m_outputs.at(outputIdx).name;
    return name.isEmpty() ? tr("Output %1").arg(outputIdx + 1) : name;
}

bool PresetTableV2Widget::multiButtonSupportsAllOutputs() const
{
    return true;
}

int PresetTableV2Widget::multiButtonParameterCount() const
{
    return 5;
}

QString PresetTableV2Widget::multiButtonParameterName(int parameter) const
{
    const bool positionMode = (m_mode == PTMode::Position);
    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::TransitionPreset:
            return positionMode ? tr("Position transition preset")
                                : tr("Transition preset");
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            return positionMode ? tr("Position FX preset")
                                : tr("Continuous FX preset");
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return positionMode ? tr("Position MultiFX preset")
                                : tr("MultiFX preset");
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
            return positionMode ? tr("Position row") : tr("Primary row");
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
            return positionMode ? tr("Secondary position row")
                                : tr("Secondary row");
        default:
            return QString();
    }
}

static PTTransitionMode multiButtonParamToTransitionMode(int parameter)
{
    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::TransitionPreset:
            return PTTransitionMode::SweepOnly;
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            return PTTransitionMode::Continuous;
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return PTTransitionMode::MultiFx;
        default:
            return PTTransitionMode::Off;
    }
}

static quint8 multiButtonParamToPresetTableInputId(int outputIdx, int parameter)
{
    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
            return PTInputId::rowSelector(outputIdx);
        case PresetTableV2MultiButtonTargetIface::TransitionPreset:
            return PTInputId::transSweep(outputIdx);
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
            return PTInputId::transSecondaryRow(outputIdx);
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            return PTInputId::transContinuousBank(outputIdx);
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return PTInputId::multiFxBank(outputIdx);
        default:
            return 0;
    }
}

int PresetTableV2Widget::multiButtonEntryCount(int outputIdx, int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return 0;
    if (parameter == PresetTableV2MultiButtonTargetIface::PrimaryRow
            || parameter == PresetTableV2MultiButtonTargetIface::SecondaryRow)
        return m_rows.size();

    const PTTransitionMode mode = multiButtonParamToTransitionMode(parameter);
    if (mode == PTTransitionMode::Off)
        return 0;
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        return provider->transitionPresetCount(mode);
    return 0;
}

QString PresetTableV2Widget::multiButtonEntryName(int outputIdx, int parameter, int index) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return QString();
    if (parameter == PresetTableV2MultiButtonTargetIface::PrimaryRow
            || parameter == PresetTableV2MultiButtonTargetIface::SecondaryRow)
    {
        if (index < 0 || index >= m_rows.size())
            return QString();
        const QString name = m_rows.at(index).name;
        return name.isEmpty() ? tr("Row %1").arg(index + 1) : name;
    }

    const PTTransitionMode mode = multiButtonParamToTransitionMode(parameter);
    if (mode == PTTransitionMode::Off)
        return QString();
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        return provider->transitionPresetName(mode, index);
    return QString();
}

int PresetTableV2Widget::multiButtonCurrentIndex(int outputIdx, int parameter) const
{
    return multiButtonLiveIndex(outputIdx, parameter);
}

int PresetTableV2Widget::multiButtonLiveIndex(int outputIdx, int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
            return outputIdx < m_activeRow.size() ? m_activeRow.at(outputIdx) : -1;
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
            return rawLiveSecondaryRowIndexLocked(outputIdx);
        case PresetTableV2MultiButtonTargetIface::TransitionPreset:
            return liveSweepPresetIndexLocked(outputIdx);
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            return liveContinuousPresetIndexLocked(outputIdx);
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return liveMultiFxPresetIndexLocked(outputIdx);
        default:
            return -1;
    }
}

bool PresetTableV2Widget::multiButtonStagingAvailable(int outputIdx, int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size() || !m_crossfadeEnabled)
        return false;

    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return true;
        default:
            return false;
    }
}

quint64 PresetTableV2Widget::multiButtonStateRevision(int outputIdx, int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    const int slot = multiButtonRevisionSlotLocked(parameter);
    if (outputIdx < 0 || slot < 0
            || outputIdx >= m_multiButtonStateRevision.size()
            || slot >= m_multiButtonStateRevision.at(outputIdx).size())
        return 0;
    return m_multiButtonStateRevision.at(outputIdx).at(slot);
}

bool PresetTableV2Widget::multiButtonOutputControlsParameter(int outputIdx,
                                                             int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return false;
    if (parameter < 0 || parameter >= multiButtonParameterCount())
        return false;
    if (m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position)
        return true;
    if (!m_doc || m_fixtureGroupId == UINT_MAX)
        return false;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return false;

    const PTOutput& out = m_outputs.at(outputIdx);
    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    if (out.scope == PTOutputScope::Mask && !docMask.isActive())
        return false;

    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();
    const QMap<QLCPoint, GroupHead>& headsMap =
            (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;

    const QList<PTOutputScopeFixture> scopeFixtures =
            collectOutputScopeFixtures(headsMap, out);
    for (const PTOutputScopeFixture& sf : scopeFixtures)
    {
        Fixture* fxi = m_doc->fixture(sf.fxiId);
        if (!fxi)
            continue;

        if (m_mode == PTMode::Position)
        {
            if (resolvePositionChannelsForFixture(fxi).valid())
                return true;
            continue;
        }

        for (const PTColumn& col : m_columns)
        {
            if (columnBindingMatchesFixture(col, fxi))
                return true;
        }
    }

    return false;
}

bool PresetTableV2Widget::multiButtonHasStagedIndex(int outputIdx, int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return false;

    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
            return outputIdx < m_stagedRowValid.size()
                    && m_stagedRowValid.at(outputIdx);
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
            return outputIdx < m_stagedSecondaryValid.size()
                    && m_stagedSecondaryValid.at(outputIdx);
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            return outputIdx < m_stagedContinuousValid.size()
                    && m_stagedContinuousValid.at(outputIdx);
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return outputIdx < m_stagedMultiFxValid.size()
                    && m_stagedMultiFxValid.at(outputIdx);
        default:
            return false;
    }
}

int PresetTableV2Widget::multiButtonStagedIndex(int outputIdx, int parameter) const
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return -1;
    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
            if (outputIdx < m_stagedRowValid.size()
                    && outputIdx < m_stagedRow.size()
                    && m_stagedRowValid.at(outputIdx))
                return m_stagedRow.at(outputIdx);
            return -1;
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
            if (outputIdx < m_stagedSecondaryValid.size()
                    && outputIdx < m_stagedSecondaryRow.size()
                    && m_stagedSecondaryValid.at(outputIdx))
                return m_stagedSecondaryRow.at(outputIdx);
            return -1;
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            if (outputIdx < m_stagedContinuousValid.size()
                    && outputIdx < m_stagedContinuousPreset.size()
                    && m_stagedContinuousValid[outputIdx])
                return m_stagedContinuousPreset[outputIdx];
            return -1;
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            if (outputIdx < m_stagedMultiFxValid.size()
                    && outputIdx < m_stagedMultiFxPreset.size()
                    && m_stagedMultiFxValid[outputIdx])
                return m_stagedMultiFxPreset[outputIdx];
            return -1;
        default:
            return -1;
    }
}

QSharedPointer<QLCInputSource> PresetTableV2Widget::multiButtonLiveInputSource(int outputIdx,
                                                                               int parameter) const
{
    if (outputIdx < 0 || outputIdx >= PTInputId::kMaxRoutableOutputs)
        return QSharedPointer<QLCInputSource>();
    if (outputIdx >= m_outputs.size())
        return QSharedPointer<QLCInputSource>();
    const quint8 id = multiButtonParamToPresetTableInputId(outputIdx, parameter);
    if (id == 0 && parameter != PresetTableV2MultiButtonTargetIface::PrimaryRow)
        return QSharedPointer<QLCInputSource>();
    return inputSource(id);
}

bool PresetTableV2Widget::multiButtonSetLiveInputSource(int outputIdx, int parameter,
                                                        QSharedPointer<QLCInputSource> src)
{
    if (outputIdx < 0 || outputIdx >= PTInputId::kMaxRoutableOutputs)
        return false;
    if (outputIdx >= m_outputs.size())
        return false;
    const quint8 id = multiButtonParamToPresetTableInputId(outputIdx, parameter);
    if (id == 0 && parameter != PresetTableV2MultiButtonTargetIface::PrimaryRow)
        return false;
    setInputSource(src, id);
    if (m_doc)
        m_doc->setModified();
    return true;
}

bool PresetTableV2Widget::multiButtonActivateStaged(int outputIdx, int parameter, int index)
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return false;

    if (parameter == PresetTableV2MultiButtonTargetIface::PrimaryRow
            || parameter == PresetTableV2MultiButtonTargetIface::SecondaryRow)
    {
        if (index < -1 || index >= m_rows.size() || !m_crossfadeEnabled)
            return false;
        armCrossfadeStagingLocked();
        if (parameter == PresetTableV2MultiButtonTargetIface::PrimaryRow)
            stagePrimaryRowLocked(outputIdx, index);
        else
            stageSecondaryRowLocked(outputIdx, index);
        resetCrossfadeClockLocked();
        update();
        lk.unlock();
        if (parameter == PresetTableV2MultiButtonTargetIface::PrimaryRow)
            refreshOperatePositionChrome(outputIdx);
        if (m_doc)
            m_doc->setModified();
        return true;
    }

    const PTTransitionMode mode = multiButtonParamToTransitionMode(parameter);
    PresetTableV2TransitionProviderIface* provider = transitionProviderLocked();
    if (!provider || mode == PTTransitionMode::Off
            || index < -1 || index >= provider->transitionPresetCount(mode))
        return false;

    if (parameter == PresetTableV2MultiButtonTargetIface::TransitionPreset)
    {
        while (m_liveSweepPreset.size() <= outputIdx)
            m_liveSweepPreset.append(-1);
        const int prevSweep = m_liveSweepPreset[outputIdx];
        m_liveSweepPreset[outputIdx] = index;
        if (outputIdx < m_stagedSweepValid.size())
            m_stagedSweepValid[outputIdx] = false;
        if (outputIdx < m_stagedSweepPreset.size())
            m_stagedSweepPreset[outputIdx] = -1;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::TransitionPreset);
        if (prevSweep != index)
            syncCommittedPlaybackStateLocked(outputIdx, false);
        sendFeedback(index + 1, PTInputId::transSweep(outputIdx));
    }
    else if (parameter == PresetTableV2MultiButtonTargetIface::ContinuousPreset)
    {
        if (!m_crossfadeEnabled)
            return false;
        armCrossfadeStagingLocked();
        materializeContinuousRowsLocked(outputIdx, true);
        stageContinuousPresetLocked(outputIdx, index);
        resetCrossfadeClockLocked();
    }
    else if (parameter == PresetTableV2MultiButtonTargetIface::MultiFxPreset)
    {
        if (!m_crossfadeEnabled)
            return false;
        armCrossfadeStagingLocked();
        materializeContinuousRowsLocked(outputIdx, true);
        stageMultiFxPresetLocked(outputIdx, index);
        resetCrossfadeClockLocked();
    }
    else
        return false;

    m_cachedTransitionSweepCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
    m_cachedTransitionContinuousCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
    m_cachedTransitionMultiFxCount = provider->transitionPresetCount(PTTransitionMode::MultiFx);
    update();
    if (m_doc)
        m_doc->setModified();
    return true;
}

bool PresetTableV2Widget::multiButtonBeginFlash(int outputIdx, int parameter, int index,
                                                quint32 sourceWidgetId, quint64 token,
                                                double timeMultiplier)
{
    if (parameter != PresetTableV2MultiButtonTargetIface::PrimaryRow)
        return false;

    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size()
            || index < 0 || index >= m_rows.size())
        return false;

    const int presetIdx = liveSweepPresetIndexLocked(outputIdx);
    timeMultiplier = widgetFlashTimeMultiplierValue(m_widgetFlashTimeMultiplierIndex);
    return beginMatrixFlashLocked(outputIdx, index, presetIdx, sourceWidgetId, token,
                                  timeMultiplier);
}

bool PresetTableV2Widget::multiButtonEndFlash(int outputIdx, int parameter, int index,
                                              quint32 sourceWidgetId, quint64 token)
{
    if (parameter != PresetTableV2MultiButtonTargetIface::PrimaryRow)
        return false;

    QMutexLocker lk(&m_stateMutex);
    return endMatrixFlashLocked(outputIdx, index, sourceWidgetId, token);
}

bool PresetTableV2Widget::multiButtonFlashGateActive() const
{
    QMutexLocker lk(&m_stateMutex);
    return m_widgetFlashBehavior == PTWidgetFlashBehavior::PrimaryRowModifier
            && m_widgetFlashGateActive;
}

bool PresetTableV2Widget::multiButtonActivate(int outputIdx, int parameter, int index)
{
    QMutexLocker lk(&m_stateMutex);
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return false;

    const auto clearCrossfadeSessionIfNoStaged = [this]()
    {
        if (!crossfadeHasStagedChangesLocked())
            m_crossfadeSessionActive = false;
    };

    if (parameter == PresetTableV2MultiButtonTargetIface::PrimaryRow)
    {
        if (index < -1 || index >= m_rows.size())
            return false;
        if (outputIdx < m_stagedRow.size())
            m_stagedRow[outputIdx] = -1;
        if (outputIdx < m_stagedRowValid.size())
            m_stagedRowValid[outputIdx] = false;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::PrimaryRow);
        clearCrossfadeSessionIfNoStaged();
        lk.unlock();
        setActiveRow(outputIdx, index);
        sendFeedback(index < 0 ? 0 : index + 1, PTInputId::rowSelector(outputIdx));
        refreshRowHighlights();
        if (m_doc)
            m_doc->setModified();
        return true;
    }

    if (parameter == PresetTableV2MultiButtonTargetIface::SecondaryRow)
    {
        if (index < -1 || index >= m_rows.size())
            return false;
        while (m_liveSecondaryRow.size() <= outputIdx)
            m_liveSecondaryRow.append(-1);
        m_liveSecondaryRow[outputIdx] = index;
        if (outputIdx < m_stagedSecondaryValid.size())
            m_stagedSecondaryValid[outputIdx] = false;
        if (outputIdx < m_stagedSecondaryRow.size())
            m_stagedSecondaryRow[outputIdx] = -1;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::SecondaryRow);
        materializeContinuousRowsLocked(outputIdx, false);
        clearCrossfadeSessionIfNoStaged();
        sendFeedback(index < 0 ? 0 : index + 1, PTInputId::transSecondaryRow(outputIdx));
        lk.unlock();
        refreshTransitionPresetCache();
        update();
        if (m_doc)
            m_doc->setModified();
        return true;
    }

    const PTTransitionMode mode = multiButtonParamToTransitionMode(parameter);
    PresetTableV2TransitionProviderIface* provider = transitionProviderLocked();
    if (!provider || mode == PTTransitionMode::Off
            || index < -1 || index >= provider->transitionPresetCount(mode))
        return false;

    if (parameter == PresetTableV2MultiButtonTargetIface::TransitionPreset)
    {
        while (m_liveSweepPreset.size() <= outputIdx)
            m_liveSweepPreset.append(-1);
        const int prevSweep = m_liveSweepPreset[outputIdx];
        m_liveSweepPreset[outputIdx] = index;
        if (outputIdx < m_stagedSweepValid.size())
            m_stagedSweepValid[outputIdx] = false;
        if (outputIdx < m_stagedSweepPreset.size())
            m_stagedSweepPreset[outputIdx] = -1;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::TransitionPreset);
        if (prevSweep != index)
            syncCommittedPlaybackStateLocked(outputIdx, false);
        sendFeedback(index < 0 ? 0 : index + 1, PTInputId::transSweep(outputIdx));
    }
    else if (parameter == PresetTableV2MultiButtonTargetIface::ContinuousPreset)
    {
        while (m_liveContinuousPreset.size() <= outputIdx)
            m_liveContinuousPreset.append(-1);
        m_liveContinuousPreset[outputIdx] = index;
        if (outputIdx < m_stagedContinuousValid.size())
            m_stagedContinuousValid[outputIdx] = false;
        if (outputIdx < m_stagedContinuousPreset.size())
            m_stagedContinuousPreset[outputIdx] = -1;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::ContinuousPreset);
        materializeContinuousRowsLocked(outputIdx, false);
        clearCrossfadeSessionIfNoStaged();
        sendFeedback(index < 0 ? 0 : index + 1, PTInputId::transContinuousBank(outputIdx));
    }
    else if (parameter == PresetTableV2MultiButtonTargetIface::MultiFxPreset)
    {
        while (m_liveMultiFxPreset.size() <= outputIdx)
            m_liveMultiFxPreset.append(-1);
        m_liveMultiFxPreset[outputIdx] = index;
        if (outputIdx < m_stagedMultiFxValid.size())
            m_stagedMultiFxValid[outputIdx] = false;
        if (outputIdx < m_stagedMultiFxPreset.size())
            m_stagedMultiFxPreset[outputIdx] = -1;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::MultiFxPreset);
        clearCrossfadeSessionIfNoStaged();
        sendFeedback(index < 0 ? 0 : index + 1, PTInputId::multiFxBank(outputIdx));
    }
    else
        return false;

    m_cachedTransitionSweepCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
    m_cachedTransitionContinuousCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
    m_cachedTransitionMultiFxCount = provider->transitionPresetCount(PTTransitionMode::MultiFx);
    update();
    if (m_doc)
        m_doc->setModified();
    return true;
}

void PresetTableV2Widget::clearStagedLayerLocked(int outputIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;

    if (outputIdx < m_stagedRow.size())
        m_stagedRow[outputIdx] = -1;
    if (outputIdx < m_stagedRowValid.size())
        m_stagedRowValid[outputIdx] = false;
    if (outputIdx < m_stagedSecondaryRow.size())
        m_stagedSecondaryRow[outputIdx] = -1;
    if (outputIdx < m_stagedSweepPreset.size())
        m_stagedSweepPreset[outputIdx] = -1;
    if (outputIdx < m_stagedContinuousPreset.size())
        m_stagedContinuousPreset[outputIdx] = -1;
    if (outputIdx < m_stagedMultiFxPreset.size())
        m_stagedMultiFxPreset[outputIdx] = -1;
    if (outputIdx < m_stagedSecondaryValid.size())
        m_stagedSecondaryValid[outputIdx] = false;
    if (outputIdx < m_stagedSweepValid.size())
        m_stagedSweepValid[outputIdx] = false;
    if (outputIdx < m_stagedContinuousValid.size())
        m_stagedContinuousValid[outputIdx] = false;
    if (outputIdx < m_stagedMultiFxValid.size())
        m_stagedMultiFxValid[outputIdx] = false;
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::PrimaryRow);
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::SecondaryRow);
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::TransitionPreset);
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::ContinuousPreset);
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::MultiFxPreset);
    if (!crossfadeHasStagedChangesLocked())
    {
        m_crossfadeSessionActive = false;
    }
}

void PresetTableV2Widget::stageSecondaryRowLocked(int outputIdx, int rowIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedSecondaryRow.size() <= outputIdx)
        m_stagedSecondaryRow.append(-1);
    while (m_stagedSecondaryValid.size() <= outputIdx)
        m_stagedSecondaryValid.append(false);

    const int liveRow = liveSecondaryRowIndexLocked(outputIdx);
    if (rowIdx == liveRow)
    {
        m_stagedSecondaryRow[outputIdx] = -1;
        m_stagedSecondaryValid[outputIdx] = false;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::SecondaryRow);
        if (!crossfadeHasStagedChangesLocked())
            m_crossfadeSessionActive = false;
        return;
    }

    m_stagedSecondaryRow[outputIdx] = rowIdx;
    m_stagedSecondaryValid[outputIdx] = true;
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::SecondaryRow);
}

void PresetTableV2Widget::stageSweepPresetLocked(int outputIdx, int presetIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedSweepPreset.size() <= outputIdx)
        m_stagedSweepPreset.append(-1);
    while (m_stagedSweepValid.size() <= outputIdx)
        m_stagedSweepValid.append(false);
    m_stagedSweepPreset[outputIdx] = presetIdx;
    m_stagedSweepValid[outputIdx] = true;
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::TransitionPreset);
}

void PresetTableV2Widget::stageContinuousPresetLocked(int outputIdx, int presetIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedContinuousPreset.size() <= outputIdx)
        m_stagedContinuousPreset.append(-1);
    while (m_stagedContinuousValid.size() <= outputIdx)
        m_stagedContinuousValid.append(false);
    if (presetIdx == liveContinuousPresetIndexLocked(outputIdx))
    {
        m_stagedContinuousPreset[outputIdx] = -1;
        m_stagedContinuousValid[outputIdx] = false;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::ContinuousPreset);
        if (!crossfadeHasStagedChangesLocked())
            m_crossfadeSessionActive = false;
        return;
    }
    m_stagedContinuousPreset[outputIdx] = presetIdx;
    m_stagedContinuousValid[outputIdx] = true;
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::ContinuousPreset);
}

void PresetTableV2Widget::stageMultiFxPresetLocked(int outputIdx, int presetIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedMultiFxPreset.size() <= outputIdx)
        m_stagedMultiFxPreset.append(-1);
    while (m_stagedMultiFxValid.size() <= outputIdx)
        m_stagedMultiFxValid.append(false);
    if (presetIdx == liveMultiFxPresetIndexLocked(outputIdx))
    {
        m_stagedMultiFxPreset[outputIdx] = -1;
        m_stagedMultiFxValid[outputIdx] = false;
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::MultiFxPreset);
        if (!crossfadeHasStagedChangesLocked())
            m_crossfadeSessionActive = false;
        return;
    }
    m_stagedMultiFxPreset[outputIdx] = presetIdx;
    m_stagedMultiFxValid[outputIdx] = true;
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::MultiFxPreset);
}

void PresetTableV2Widget::ensureMultiButtonRevisionSizeLocked()
{
    while (m_multiButtonStateRevision.size() < m_outputs.size())
        m_multiButtonStateRevision.append(QVector<quint64>(5, 0));
    while (m_multiButtonStateRevision.size() > m_outputs.size())
        m_multiButtonStateRevision.removeLast();
    for (QVector<quint64>& revisions : m_multiButtonStateRevision)
        revisions.resize(5);
}

int PresetTableV2Widget::multiButtonRevisionSlotLocked(int parameter) const
{
    switch (parameter)
    {
        case PresetTableV2MultiButtonTargetIface::TransitionPreset:
            return 0;
        case PresetTableV2MultiButtonTargetIface::ContinuousPreset:
            return 1;
        case PresetTableV2MultiButtonTargetIface::MultiFxPreset:
            return 2;
        case PresetTableV2MultiButtonTargetIface::PrimaryRow:
            return 3;
        case PresetTableV2MultiButtonTargetIface::SecondaryRow:
            return 4;
        default:
            return -1;
    }
}

void PresetTableV2Widget::bumpMultiButtonStateRevisionLocked(int outputIdx, int parameter)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    const int slot = multiButtonRevisionSlotLocked(parameter);
    if (slot < 0)
        return;
    ensureMultiButtonRevisionSizeLocked();
    ++m_multiButtonStateRevision[outputIdx][slot];
}

void PresetTableV2Widget::stagePrimaryRowLocked(int outputIdx, int rowIdx)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;
    while (m_stagedRow.size() <= outputIdx)
        m_stagedRow.append(-1);
    while (m_stagedRowValid.size() <= outputIdx)
        m_stagedRowValid.append(false);

    const int liveRow = (outputIdx < m_activeRow.size()) ? m_activeRow[outputIdx] : -1;
    if (rowIdx == liveRow)
    {
        m_stagedRow[outputIdx] = -1;
        m_stagedRowValid[outputIdx] = false;
        syncCommittedPlaybackStateLocked(outputIdx, false);
        bumpMultiButtonStateRevisionLocked(
                outputIdx, PresetTableV2MultiButtonTargetIface::PrimaryRow);
        if (!crossfadeHasStagedChangesLocked())
            m_crossfadeSessionActive = false;
        return;
    }

    m_stagedRow[outputIdx] = rowIdx;
    m_stagedRowValid[outputIdx] = true;
    bumpMultiButtonStateRevisionLocked(
            outputIdx, PresetTableV2MultiButtonTargetIface::PrimaryRow);
}

void PresetTableV2Widget::materializeContinuousRowsLocked(int outputIdx, bool toStaged)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;

    const int propSecondary = m_outputs[outputIdx].secondaryRowIndex;
    if (propSecondary < 0 || propSecondary >= m_rows.size())
        return;

    const int liveSecondary = (outputIdx < m_liveSecondaryRow.size())
            ? m_liveSecondaryRow[outputIdx] : -1;
    const bool hasLiveSecondary = liveSecondary >= 0 && liveSecondary < m_rows.size();
    const bool hasStagedSecondary = outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx]
            && outputIdx < m_stagedSecondaryRow.size();

    if (toStaged)
    {
        if (!hasLiveSecondary && !hasStagedSecondary)
            stageSecondaryRowLocked(outputIdx, propSecondary);
        return;
    }

    Q_UNUSED(hasLiveSecondary);
}

void PresetTableV2Widget::syncCommittedPlaybackStateLocked(int outputIdx, bool resetFxPlayback)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size())
        return;

    if (resetFxPlayback)
    {
        resetMatrixStateLocked(outputIdx);
    }
    else
    {
        ensureMatrixState(outputIdx);
        PTOutputMatrixState& st = m_matrixState[outputIdx];
        st.sweepRunning = false;
        st.sweepManualCrossfade = false;
        st.sweepManualPhase = 0.0;
        st.sweepManualPhasePrev = 0.0;
        st.sweepProgress = 0.0;
        st.sweepFromRow = -1;
        st.sweepToRow = -1;
        st.sweepElapsedMs = 0;
        st.sweepLastCycleMs = 0;
        st.sweepPeakDimmer.clear();
        st.sweepHeldValues.clear();
    }

    if (outputIdx < m_matrixState.size() && outputIdx < m_activeRow.size())
        m_matrixState[outputIdx].appliedRow = m_activeRow[outputIdx];
    if (outputIdx < m_spatialAppliedRow.size() && outputIdx < m_activeRow.size())
        m_spatialAppliedRow[outputIdx] = m_activeRow[outputIdx];
    if (outputIdx < m_spatialChase.size())
        m_spatialChase[outputIdx] = PTSpatialChaseOutput();
}

void PresetTableV2Widget::promoteStagedToLiveLocked()
{
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        bool promoted = false;
        bool promotedFxPreset = false;
        if (o < m_stagedRowValid.size() && m_stagedRowValid[o]
                && o < m_stagedRow.size())
        {
            m_activeRow[o] = m_stagedRow[o];
            m_stagedRow[o] = -1;
            m_stagedRowValid[o] = false;
            bumpMultiButtonStateRevisionLocked(
                    o, PresetTableV2MultiButtonTargetIface::PrimaryRow);
            promoted = true;
        }

        if (o < m_stagedSecondaryValid.size() && m_stagedSecondaryValid[o])
        {
            while (m_liveSecondaryRow.size() <= o)
                m_liveSecondaryRow.append(-1);
            if (o < m_stagedSecondaryRow.size())
                m_liveSecondaryRow[o] = m_stagedSecondaryRow[o];
            m_stagedSecondaryValid[o] = false;
            if (o < m_stagedSecondaryRow.size())
                m_stagedSecondaryRow[o] = -1;
            if (o < m_outputs.size() && o < m_liveSecondaryRow.size()
                    && m_liveSecondaryRow[o] >= 0)
                m_outputs[o].secondaryRowIndex = m_liveSecondaryRow[o];
            bumpMultiButtonStateRevisionLocked(
                    o, PresetTableV2MultiButtonTargetIface::SecondaryRow);
            promoted = true;
        }

        if (o < m_stagedSweepValid.size() && m_stagedSweepValid[o])
            m_stagedSweepValid[o] = false;
        if (o < m_stagedSweepPreset.size())
            m_stagedSweepPreset[o] = -1;

        if (o < m_stagedContinuousValid.size() && m_stagedContinuousValid[o])
        {
            if (o < m_liveContinuousPreset.size() && o < m_stagedContinuousPreset.size())
                m_liveContinuousPreset[o] = m_stagedContinuousPreset[o];
            m_stagedContinuousValid[o] = false;
            if (o < m_stagedContinuousPreset.size())
                m_stagedContinuousPreset[o] = -1;
            if (o < m_outputs.size() && o < m_liveContinuousPreset.size())
                m_outputs[o].continuousPresetIndex = m_liveContinuousPreset[o];
            bumpMultiButtonStateRevisionLocked(
                    o, PresetTableV2MultiButtonTargetIface::ContinuousPreset);
            promoted = true;
            promotedFxPreset = true;
        }

        if (o < m_stagedMultiFxValid.size() && m_stagedMultiFxValid[o])
        {
            if (o < m_liveMultiFxPreset.size() && o < m_stagedMultiFxPreset.size())
                m_liveMultiFxPreset[o] = m_stagedMultiFxPreset[o];
            if (o < m_multiFxElapsedMs.size() && o < m_multiFxStagedElapsedMs.size())
                m_multiFxElapsedMs[o] = m_multiFxStagedElapsedMs[o];
            m_stagedMultiFxValid[o] = false;
            if (o < m_stagedMultiFxPreset.size())
                m_stagedMultiFxPreset[o] = -1;
            if (o < m_outputs.size() && o < m_liveMultiFxPreset.size())
                m_outputs[o].multiFxPresetIndex = m_liveMultiFxPreset[o];
            bumpMultiButtonStateRevisionLocked(
                    o, PresetTableV2MultiButtonTargetIface::MultiFxPreset);
            promoted = true;
            promotedFxPreset = true;
        }

        if (promoted)
        {
            syncCommittedPlaybackStateLocked(o, promotedFxPreset);
            sendLiveSelectorFeedbackLocked(o);
        }
    }
    m_crossfadeSessionActive = false;
    m_crossfadeEditLaneStaged = true;
    resetMultiFxCrossfadePhaseAnchorLocked();

    if (m_mode == PTMode::Position && m_positionDraftDirty)
    {
        clearPositionDraft();
        m_positionPromoteUiRefresh = true;
    }

    // EFX parameter overrides are live-only. Staged commit is limited to table
    // selections above, so do not call back into the provider while holding
    // m_stateMutex.
}

PTTransitionPreset PresetTableV2Widget::transitionPresetForOutputLocked(int outputIdx) const
{
    if (continuousEfxActiveForOutputLocked(outputIdx))
        return continuousPresetForOutputLocked(outputIdx);
    if (sweepEfxActiveForOutputLocked(outputIdx))
        return sweepPresetForOutputLocked(outputIdx);
    return transitionPresetAtIndexLocked(PTTransitionMode::SweepOnly, -1, outputIdx);
}

PresetTableV2TransitionProviderIface* PresetTableV2Widget::linkedTransitionProvider() const
{
    QMutexLocker lk(&m_stateMutex);
    return transitionProviderLocked();
}

PresetTableV2TransitionProviderIface* PresetTableV2Widget::transitionProviderLocked() const
{
    if (m_linkedTransitionWidgetId == VCWidget::invalidId())
        return nullptr;
    VirtualConsole* vc = VirtualConsole::instance();
    if (!vc)
        return nullptr;
    return qobject_cast<PresetTableV2TransitionProviderIface*>(
            vc->widget(m_linkedTransitionWidgetId));
}

PTGlobalEffectSettings PresetTableV2Widget::globalEffectSettingsLocked() const
{
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        return provider->globalEffectSettings();
    return PTGlobalEffectSettings();
}

quint32 PresetTableV2Widget::cycleDurationMsLocked(const PTGlobalEffectSettings& global,
                                                   const PTTransitionPreset& preset) const
{
    bool honorPresetDuration = false;
    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
        honorPresetDuration = provider->hasLiveColumnOverride(PTEfxCol::InputDuration);
    return PTParamMatrixEngine::effectiveDurationMs(global, preset, honorPresetDuration);
}

void PresetTableV2Widget::rescaleElapsedForDurationChange(quint32& elapsedMs,
                                                          quint32 oldDurationMs,
                                                          quint32 newDurationMs)
{
    if (elapsedMs == 0 || oldDurationMs == 0 || newDurationMs == 0
            || oldDurationMs == newDurationMs)
        return;

    // Match QLC EFXFixture::durationChanged(): preserve current phase when duration changes.
    const double phase = double(elapsedMs % oldDurationMs) / double(oldDurationMs);
    elapsedMs = quint32(phase * double(newDurationMs));
}

void PresetTableV2Widget::ensurePhaseStableCycleLocked(QVector<quint32>& elapsed,
                                                       QVector<quint32>& lastCycle,
                                                       int outputIdx,
                                                       quint32 currentCycleMs)
{
    if (outputIdx < 0)
        return;
    while (elapsed.size() <= outputIdx)
        elapsed.append(0);
    while (lastCycle.size() <= outputIdx)
        lastCycle.append(0);

    currentCycleMs = qMax(quint32(1), currentCycleMs);
    if (lastCycle[outputIdx] > 0 && lastCycle[outputIdx] != currentCycleMs)
        rescaleElapsedForDurationChange(elapsed[outputIdx], lastCycle[outputIdx], currentCycleMs);
    lastCycle[outputIdx] = currentCycleMs;
}

PTTransitionPreset PresetTableV2Widget::transitionPresetForOutput(int outputIdx) const
{
    QMutexLocker lk(&m_stateMutex);
    return transitionPresetForOutputLocked(outputIdx);
}

void PresetTableV2Widget::refreshRowHighlights()
{
    if (mode() != Doc::Operate) return;

    m_table->blockSignals(true);

    int numRows = m_table->rowCount();

    // Clear all Name-column items' background first
    for (int r = 0; r < numRows; ++r)
    {
        QTableWidgetItem* it = m_table->item(r, 0);
        if (it)
        {
            it->setBackground(QBrush());
            it->setForeground(QBrush());
            it->setText(m_rows[r].name);
        }
    }

    // Build per-row display: collect outputs active on each row
    QMap<int, QStringList> rowOutputLabels;

    QMutexLocker lk(&m_stateMutex);
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        if (ar >= 0 && ar < numRows)
            rowOutputLabels[ar].append(QString::number(o + 1));
    }
    lk.unlock();

    for (auto it = rowOutputLabels.constBegin(); it != rowOutputLabels.constEnd(); ++it)
    {
        int r = it.key();
        QTableWidgetItem* item = m_table->item(r, 0);
        if (!item) continue;

        // Use first output's color as row tint
        QColor tint = s_outputColors[0];
        // Find first output index mapped to this row
        QMutexLocker lk2(&m_stateMutex);
        for (int o = 0; o < m_activeRow.size(); ++o)
            if (m_activeRow[o] == r) { tint = s_outputColors[o % 8]; break; }
        lk2.unlock();

        QColor bg = tint;
        bg.setAlpha(60);
        item->setBackground(bg);
    }

    // Update status bar
    QStringList parts;
    int boundCols = 0;
    for (const PTColumn& col : m_columns)
    {
        if (col.hasBindings())
            ++boundCols;
    }

    QMutexLocker lk3(&m_stateMutex);
    const bool matrixCapable = useMatrixEngineLocked();
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        int ar = m_activeRow[o];
        QString outName = (o < m_outputs.size()) ? m_outputs[o].name : tr("Out%1").arg(o + 1);
        if (ar < 0)
        {
            parts.append(QStringLiteral("%1: row off").arg(outName));
            continue;
        }
        QString rowLabel = (ar < m_rows.size()) ? m_rows[ar].name : QString::number(ar + 1);
        QString path = tr("direct");
        if (matrixCapable && efxActiveForOutputLocked(o))
            path = tr("efx/matrix");
        else if (m_spatialEffects.enabled && efxActiveForOutputLocked(o))
            path = tr("efx");
        parts.append(QStringLiteral("%1: %2 | %3 | cols %4/%5")
                             .arg(outName, rowLabel, path)
                             .arg(boundCols)
                             .arg(m_columns.size()));
    }
    if ((m_mode == PTMode::FixtureGroup || m_mode == PTMode::Position)
            && m_fixtureGroupId == UINT_MAX)
        parts.prepend(tr("No fixture group"));
    else if (boundCols == 0 && !m_columns.isEmpty())
        parts.prepend(tr("No column bindings — DMX will not output"));
    lk3.unlock();
    if (m_statusBar)
        m_statusBar->setText(parts.join(QLatin1String("    ")));

    m_table->blockSignals(false);
}

// ==========================================================================
// writeDMX helpers
// ==========================================================================

// Apply a DMX value to a fade channel, with optional crossfade blending.
// aVal = active row value, bVal = staged row value (ignored when !xfEnabled or stagedRow < 0)
static void applyFadeValue(GenericFader* fader, Doc* doc, Universe* uni,
                            quint32 fxiId, quint32 chanIdx,
                            uchar aVal, uchar bVal,
                            bool xfEnabled, bool hasStaged,
                            bool colFade, uchar xfEffective)
{
    FadeChannel* fc = fader->getChannelFader(doc, uni, fxiId, chanIdx);
    if (fc->universe() == Universe::invalid())
    {
        fader->remove(fc);
        return;
    }

    if (!xfEnabled || !hasStaged)
    {
        fc->setStart(fc->current());
        fc->setTarget(aVal);
        fc->setFadeTime(0);
        fc->setReady(false);
        fc->setElapsed(0);
    }
    else
    {
        uchar finalVal;
        if (colFade)
            finalVal = uchar(aVal + qint16(bVal - aVal) * qint16(xfEffective) / 255);
        else
            finalVal = (xfEffective > 127) ? bVal : aVal;

        fc->setStart(finalVal);
        fc->setTarget(finalVal);
        fc->setFadeTime(0);
        fc->setReady(false);
        fc->setElapsed(0);
    }
}

static void applyFadeValueTimed(GenericFader* fader, Doc* doc, Universe* uni,
                                quint32 fxiId, quint32 chanIdx, uchar target, uint fadeTimeMs)
{
    FadeChannel* fc = fader->getChannelFader(doc, uni, fxiId, chanIdx);
    if (fc->universe() == Universe::invalid())
    {
        fader->remove(fc);
        return;
    }

    if (fadeTimeMs == 0 || qAbs(int(target) - int(fc->current())) <= 2)
    {
        fc->setCurrent(target);
        fc->setStart(target);
        fc->setTarget(target);
        fc->setFadeTime(0);
        fc->setReady(true);
        fc->setElapsed(0);
        return;
    }

    const bool sameTarget = (fc->target() == target) && !fc->isReady();
    if (!sameTarget)
    {
        fc->setStart(fc->current());
        fc->setTarget(target);
        fc->setFadeTime(fadeTimeMs);
        fc->setReady(false);
        fc->setElapsed(0);
    }
    else
    {
        fc->setFadeTime(fadeTimeMs);
    }
}

static PTResolvedPositionChannels resolvePositionChannelsForFixture(Fixture* fxi)
{
    PTResolvedPositionChannels result;
    if (!fxi || !fxi->fixtureMode())
        return result;

    QLCFixtureMode* mode = fxi->fixtureMode();
    for (quint32 i = 0; i < mode->channels().size(); ++i)
    {
        QLCChannel* ch = mode->channel(i);
        if (!ch)
            continue;

        const QLCChannel::Preset preset = ch->preset();
        const QLCChannel::Group group = ch->group();
        const bool fine = ch->controlByte() == QLCChannel::LSB
                || ch->name().contains(QStringLiteral("fine"), Qt::CaseInsensitive);

        if (preset == QLCChannel::PositionPanFine
                || (group == QLCChannel::Pan && fine))
            result.panFine = int(i);
        else if (preset == QLCChannel::PositionPan
                 || (group == QLCChannel::Pan && !fine && result.pan < 0))
            result.pan = int(i);
        else if (preset == QLCChannel::PositionTiltFine
                 || (group == QLCChannel::Tilt && fine))
            result.tiltFine = int(i);
        else if (preset == QLCChannel::PositionTilt
                 || (group == QLCChannel::Tilt && !fine && result.tilt < 0))
            result.tilt = int(i);
    }
    return result;
}

static QVector<uchar> continuousColumnValues(const QVector<PTColumn>& columns,
                                             const QVector<uchar>& priVals,
                                             const QVector<uchar>& secVals,
                                             double dimmer,
                                             int waveShape,
                                             int waveFadeIn,
                                             int waveFadeOut,
                                             uchar intensity)
{
    double t = dimmer;
    if (t > 1.0)
        t /= 255.0;
    t = qBound(0.0, t, 1.0);

    QVector<uchar> values;
    values.resize(columns.size());
    const bool globalSnap = (waveShape == 1);
    for (int c = 0; c < columns.size(); ++c)
    {
        const uchar pri = (c < priVals.size()) ? priVals[c] : 0;
        const uchar sec = (c < secVals.size()) ? secVals[c] : 0;
        const bool sharpWave = (waveFadeIn == 0 && waveFadeOut == 0);
        const bool snap = globalSnap || sharpWave || !columns[c].fade;
        uchar val = snap
                ? ((t >= 0.5) ? sec : pri)
                : uchar(qBound(0, int(std::lround(double(pri) + (double(sec) - double(pri)) * t)), 255));
        if (intensity < 255)
            val = uchar(qBound(0, int(std::lround(double(val) * double(intensity) / 255.0)), 255));
        values[c] = val;
    }
    return values;
}

static QVector<uchar> staticContinuousValues(const QVector<uchar>& values, uchar intensity)
{
    return PTParamMatrixEngine::blendWithIntensity(values, intensity);
}

static QVector<uchar> continuousOutputValues(const QVector<PTColumn>& columns,
                                             const QVector<uchar>& priVals,
                                             const QVector<uchar>& secVals,
                                             const PTTransitionPreset& preset,
                                             double dimmer,
                                             uchar intensity)
{
    if (!preset.enabled)
        return staticContinuousValues(priVals, intensity);
    return continuousColumnValues(columns, priVals, secVals, dimmer,
                                  preset.waveShape, preset.waveFadeIn,
                                  preset.waveFadeOut, intensity);
}

void PresetTableV2Widget::writeDMXLegacy(QList<Universe*>& universes, uchar xfEffective)
{
    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int activeRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const bool stagedPrimaryValid = o < m_stagedRowValid.size() && m_stagedRowValid[o];
        const int stagedRow = (stagedPrimaryValid && o < m_stagedRow.size()) ? m_stagedRow[o] : -1;

        if (activeRow < 0 && !stagedPrimaryValid) continue;

        const PTOutput& out = m_outputs[o];
        if (out.fixtureId == UINT_MAX) continue;

        Fixture* fxi = m_doc->fixture(out.fixtureId);
        if (!fxi) continue;
        if ((int)fxi->channels() != m_columns.size()) continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size()) continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        if (activeRow >= m_rows.size()) continue;

        const QVector<uchar> offVals(m_columns.size(), uchar(0));
        const QVector<uchar>& aVals = activeRow >= 0 ? m_rows[activeRow].values : offVals;
        const QVector<uchar>* bVals = (stagedRow >= 0 && stagedRow < m_rows.size())
            ? &m_rows[stagedRow].values : (stagedPrimaryValid ? &offVals : nullptr);

        for (int c = 0; c < m_columns.size(); ++c)
        {
            uchar aVal = (c < aVals.size()) ? aVals[c] : 0;
            uchar bVal = (bVals && c < bVals->size()) ? (*bVals)[c] : aVal;
            applyFadeValue(fader.data(), m_doc, universes[uni],
                           out.fixtureId, quint32(c),
                           aVal, bVal,
                           m_crossfadeEnabled, stagedPrimaryValid,
                           m_columns[c].fade, xfEffective);
        }
    }
}

const QLCChannel* PresetTableV2Widget::resolveBoundChannel(const PTColumn& col) const
{
    if (m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position) return nullptr;
    if (!col.hasBindings()) return nullptr;
    if (!m_doc) return nullptr;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp) return nullptr;

    const PTColumnTypeBinding* bindingPtr = nullptr;
    for (const PTColumnTypeBinding& candidate : col.bindings)
    {
        if (candidate.isValid())
        {
            bindingPtr = &candidate;
            break;
        }
    }
    if (bindingPtr == nullptr)
        return nullptr;
    const PTColumnTypeBinding& binding = *bindingPtr;

    // Find the first fixture in the group that matches the binding's manufacturer/model/mode
    const QMap<QLCPoint, GroupHead> headsMap = m_doc->effectiveHeadsMap(grp);
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        Fixture* fxi = m_doc->fixture(it.value().fxi);
        if (!fxi) continue;

        QLCFixtureDef*  fxDef  = fxi->fixtureDef();
        QLCFixtureMode* fxMode = fxi->fixtureMode();
        if (!fxDef || !fxMode) continue;

        if (fxDef->manufacturer() != binding.manufacturer) continue;
        if (fxDef->model()        != binding.model)        continue;
        if (fxMode->name()        != binding.modeName)     continue;

        return fxMode->channel(quint32(binding.channelIndex));
    }
    return nullptr;
}

void PresetTableV2Widget::applyPointChannels(GenericFader* fader, Universe* uni,
                                              const GroupHead& head, Fixture* fxi,
                                              const QLCPoint& /*pt*/,
                                              const QVector<uchar>& aVals,
                                              uint fadeTimeMs)
{
    QLCFixtureDef*  fxDef  = fxi->fixtureDef();
    QLCFixtureMode* fxMode = fxi->fixtureMode();
    if (!fxDef || !fxMode) return;

    for (int c = 0; c < m_columns.size(); ++c)
    {
        const PTColumn& col = m_columns[c];
        const uchar aVal = (c < aVals.size()) ? aVals[c] : 0;

        for (const PTColumnTypeBinding& binding : col.bindings)
        {
            if (!bindingMatchesFixture(binding, fxi))
                continue;

            const quint32 absChannel = quint32(binding.channelIndex);
            applyFadeValueTimed(fader, m_doc, uni, head.fxi, absChannel, aVal, fadeTimeMs);
        }
    }
}

void PresetTableV2Widget::applyPointPosition(GenericFader* fader, Universe* uni,
                                             const GroupHead& head, Fixture* fxi,
                                             const PTPositionValue& pos,
                                             uint fadeTimeMs)
{
    if (!pos.valid)
        return;

    const PTResolvedPositionChannels chans = resolvePositionChannelsForFixture(fxi);
    if (!chans.valid())
        return;

    const quint16 pan16 = PTPositionConverter::panDegToPan16(fxi, head.head, pos.panDeg);
    const quint16 tilt16 = PTPositionConverter::tiltDegToTilt16(fxi, head.head, pos.tiltDeg);
    const uchar panCoarse = uchar((pan16 >> 8) & 0xff);
    const uchar panFine = uchar(pan16 & 0xff);
    const uchar tiltCoarse = uchar((tilt16 >> 8) & 0xff);
    const uchar tiltFine = uchar(tilt16 & 0xff);

    applyFadeValueTimed(fader, m_doc, uni, head.fxi, quint32(chans.pan), panCoarse, fadeTimeMs);
    if (chans.panFine >= 0)
        applyFadeValueTimed(fader, m_doc, uni, head.fxi, quint32(chans.panFine), panFine, fadeTimeMs);
    applyFadeValueTimed(fader, m_doc, uni, head.fxi, quint32(chans.tilt), tiltCoarse, fadeTimeMs);
    if (chans.tiltFine >= 0)
        applyFadeValueTimed(fader, m_doc, uni, head.fxi, quint32(chans.tiltFine), tiltFine, fadeTimeMs);
}

void PresetTableV2Widget::applyBlendedPointChannels(GenericFader* fader, Universe* uni,
                                                   const GroupHead& head, Fixture* fxi,
                                                   const QLCPoint& /*pt*/,
                                                   const QVector<uchar>& priVals,
                                                   const QVector<uchar>& secVals,
                                                   double dimmer,
                                                   quint32 presetFadeMs,
                                                   int waveShape,
                                                   int waveFadeIn,
                                                   int waveFadeOut,
                                                   uchar intensity)
{
    QLCFixtureDef*  fxDef  = fxi->fixtureDef();
    QLCFixtureMode* fxMode = fxi->fixtureMode();
    if (!fxDef || !fxMode) return;

    double t = dimmer;
    if (t > 1.0)
        t /= 255.0;
    t = qBound(0.0, t, 1.0);
    const bool globalSnap = (waveShape == 1);

    for (int c = 0; c < m_columns.size(); ++c)
    {
        const PTColumn& col = m_columns[c];
        const uchar pri = (c < priVals.size()) ? priVals[c] : 0;
        const uchar sec = (c < secVals.size()) ? secVals[c] : 0;
        const bool sharpWave = (waveFadeIn == 0 && waveFadeOut == 0);
        const bool snap = globalSnap || sharpWave || !col.fade;
        uchar val;
        if (snap)
            val = (t >= 0.5) ? sec : pri;
        else
            val = uchar(qBound(0, int(std::lround(double(pri) + (double(sec) - double(pri)) * t)), 255));

        if (intensity < 255)
            val = uchar(qBound(0, int(std::lround(double(val) * double(intensity) / 255.0)), 255));

        const uint chFade = snap ? 0 : presetFadeMs;

        for (const PTColumnTypeBinding& binding : col.bindings)
        {
            if (!bindingMatchesFixture(binding, fxi))
                continue;

            const quint32 absChannel = quint32(binding.channelIndex);
            applyFadeValueTimed(fader, m_doc, uni, head.fxi, absChannel, val, chFade);
        }
    }
}

void PresetTableV2Widget::startSpatialChase(int outputIdx, int rowIdx, const QList<QLCPoint>& points,
                                          const PTTransitionPreset& preset, int gridWidth,
                                          int gridHeight)
{
    if (outputIdx < 0)
        return;
    while (m_spatialChase.size() <= outputIdx)
        m_spatialChase.append(PTSpatialChaseOutput());

    PTSpatialChaseOutput& chase = m_spatialChase[outputIdx];
    chase.active = true;
    chase.targetRow = rowIdx;
    chase.progress = 0.0;
    chase.spatialPreset = preset;
    chase.armed.clear();
    chase.armedFixtures.clear();
    chase.order = PresetTableV2SpatialEngine::buildChaseOrder(points, preset, gridWidth, gridHeight);
}

void PresetTableV2Widget::tickSpatialChase(int outputIdx, MasterTimer* timer,
                                            QList<Universe*>& universes,
                                            const PTOutput& out,
                                            const QVector<uchar>& aVals)
{
    if (outputIdx < 0 || outputIdx >= m_spatialChase.size())
        return;

    PTSpatialChaseOutput& chase = m_spatialChase[outputIdx];
    if (!chase.active)
        return;

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp || !timer)
        return;

    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();
    const QMap<QLCPoint, GroupHead>& headsMap =
            (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;

    const quint32 durationMs = qMax(quint32(1), chase.spatialPreset.durationMs);
    const double increment = double(kPTEfxStepMs) / double(durationMs);
    chase.progress = qMin(1.0, chase.progress + increment);

    const quint32 fadeMs = qMax(quint32(1), chase.spatialPreset.fadeMs);

    const int pointCount = chase.order.size();
    for (int i = 0; i < pointCount; ++i)
    {
        const QLCPoint& pt = chase.order.at(i);
        if (chase.armed.contains(pt))
            continue;

        const double threshold = (pointCount <= 1)
                ? 0.0 : double(i) / double(pointCount - 1);
        if (chase.progress < threshold)
            continue;

        auto hit = headsMap.constFind(pt);
        if (hit == headsMap.constEnd())
            continue;
        if (!outputScopeAllowsPoint(out.scope, pt, out))
            continue;

        const GroupHead& head = hit.value();
        if (chase.armedFixtures.contains(head.fxi))
        {
            chase.armed.insert(pt);
            continue;
        }

        Fixture* fxi = m_doc->fixture(head.fxi);
        if (!fxi) continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size()) continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        applyPointChannels(fader.data(), universes[uni], head, fxi, pt, aVals, fadeMs);
        chase.armed.insert(pt);
        chase.armedFixtures.insert(head.fxi);
    }

    if (pointCount == 0 || chase.progress >= 1.0)
    {
        chase.active = false;
        if (outputIdx < m_spatialAppliedRow.size())
            m_spatialAppliedRow[outputIdx] = chase.targetRow;
    }
}

void PresetTableV2Widget::writeContinuousSpatial(int outputIdx, MasterTimer* timer,
                                                 QList<Universe*>& universes,
                                                 const PTOutput& out,
                                                 const QVector<uchar>& priVals,
                                                 const QVector<uchar>& secVals,
                                                 const QSize& gridSize,
                                                 const QMap<QLCPoint, GroupHead>& headsMap,
                                                 const PTTransitionPreset* presetOverride,
                                                 const QVector<uchar>* stagedPriVals,
                                                 const QVector<uchar>* stagedSecVals,
                                                 const PTTransitionPreset* stagedPresetOverride,
                                                 double morphProgress,
                                                 bool useMultiFx)
{
    Q_UNUSED(timer);

    if (outputIdx < 0)
        return;

    const PTTransitionPreset spatialPreset = presetOverride ? *presetOverride
                                                            : continuousPresetForOutputLocked(outputIdx);
    const PTTransitionPreset multiFxPreset = multiFxPresetForOutputLocked(outputIdx);
    const int stagedMultiFxIdx = stagedMultiFxPresetIndexLocked(outputIdx);
    const bool hasStagedMultiFx = hasStagedMultiFxPresetLocked(outputIdx);
    const PTTransitionPreset stagedMultiFxPreset = hasStagedMultiFx
            ? transitionPresetAtIndexLocked(PTTransitionMode::MultiFx, stagedMultiFxIdx, outputIdx)
            : multiFxPreset;
    const bool mixMultiFx = useMultiFx && m_multiFxBlend > 0
            && (multiFxPreset.enabled || stagedMultiFxPreset.enabled);
    if (!continuousEfxActiveForOutputLocked(outputIdx) && !mixMultiFx)
        return;
    const bool morphOutput = stagedPresetOverride && stagedPriVals && stagedSecVals;
    const PTTransitionPreset stagedPreset = morphOutput ? *stagedPresetOverride : spatialPreset;
    const PTGlobalEffectSettings global = globalEffectSettingsLocked();
    const PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(spatialPreset, &global);
    const quint32 durationMs = qMax(quint32(1), cycleDurationMsLocked(global, spatialPreset));
    const PTDimmerWaveParams stagedWaveParams = PTDimmerWaveEngine::paramsFromPreset(stagedPreset, &global);
    const quint32 stagedDurationMs = qMax(quint32(1), cycleDurationMsLocked(global, stagedPreset));
    const PTDimmerWaveParams multiFxWaveParams = PTDimmerWaveEngine::paramsFromPreset(multiFxPreset, &global);
    const quint32 multiFxDurationMs = qMax(quint32(1), cycleDurationMsLocked(global, multiFxPreset));
    const PTDimmerWaveParams stagedMultiFxWaveParams =
            PTDimmerWaveEngine::paramsFromPreset(stagedMultiFxPreset, &global);
    const quint32 stagedMultiFxDurationMs =
            qMax(quint32(1), cycleDurationMsLocked(global, stagedMultiFxPreset));
    ensurePhaseStableCycleLocked(m_continuousElapsedMs, m_continuousLastCycleMs,
                                 outputIdx, durationMs);
    ensurePhaseStableCycleLocked(m_multiFxElapsedMs, m_multiFxLastCycleMs,
                                 outputIdx, multiFxDurationMs);
    if (hasStagedMultiFx)
    {
        ensurePhaseStableCycleLocked(m_multiFxStagedElapsedMs, m_multiFxStagedLastCycleMs,
                                     outputIdx, stagedMultiFxDurationMs);
    }
    m_continuousElapsedMs[outputIdx] += MasterTimer::tick();
    if (m_continuousElapsedMs[outputIdx] > durationMs)
        m_continuousElapsedMs[outputIdx] = 0;
    const quint32 elapsedMs = quint32(m_continuousElapsedMs[outputIdx]);
    const quint32 multiFxElapsedMs = quint32(m_multiFxElapsedMs[outputIdx]);
    const quint32 stagedMultiFxElapsedMs = hasStagedMultiFx
            ? quint32(m_multiFxStagedElapsedMs[outputIdx]) : multiFxElapsedMs;

    QList<QLCPoint> points;
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        if (outputScopeAllowsPoint(out.scope, it.key(), out))
            points.append(it.key());
    }

    const QList<QLCPoint> order = PresetTableV2SpatialEngine::buildChaseOrder(
                points, spatialPreset, gridSize.width(), gridSize.height(), &global);
    const int orderCount = order.size();
    if (orderCount <= 0)
        return;

    QHash<QLCPoint, int> serialIndex;
    for (int i = 0; i < orderCount; ++i)
        serialIndex.insert(order.at(i), i);

    const QList<QLCPoint> stagedOrder = morphOutput
            ? PresetTableV2SpatialEngine::buildChaseOrder(
                points, stagedPreset, gridSize.width(), gridSize.height(), &global)
            : QList<QLCPoint>();
    const int stagedOrderCount = stagedOrder.size();
    QHash<QLCPoint, int> stagedSerialIndex;
    for (int i = 0; i < stagedOrderCount; ++i)
        stagedSerialIndex.insert(stagedOrder.at(i), i);
    const QList<QLCPoint> multiFxOrder = mixMultiFx
            ? PresetTableV2SpatialEngine::buildChaseOrder(
                points, multiFxPreset, gridSize.width(), gridSize.height(), &global)
            : QList<QLCPoint>();
    const int multiFxOrderCount = multiFxOrder.size();
    QHash<QLCPoint, int> multiFxSerialIndex;
    for (int i = 0; i < multiFxOrderCount; ++i)
        multiFxSerialIndex.insert(multiFxOrder.at(i), i);
    const QList<QLCPoint> stagedMultiFxOrder = mixMultiFx && hasStagedMultiFx
            ? PresetTableV2SpatialEngine::buildChaseOrder(
                points, stagedMultiFxPreset, gridSize.width(), gridSize.height(), &global)
            : QList<QLCPoint>();
    const int stagedMultiFxOrderCount = stagedMultiFxOrder.size();
    QHash<QLCPoint, int> stagedMultiFxSerialIndex;
    for (int i = 0; i < stagedMultiFxOrderCount; ++i)
        stagedMultiFxSerialIndex.insert(stagedMultiFxOrder.at(i), i);

    const quint32 fadeMs = 0;
    QSet<quint32> writtenFixtures;

    for (const QLCPoint& pt : points)
    {
        auto hit = headsMap.constFind(pt);
        if (hit == headsMap.constEnd())
            continue;

        const int headOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                pt.x(), pt.y(), gridSize.width(), gridSize.height(), waveParams);
        const int serialIdx = serialIndex.value(pt, 0);
        const quint32 timeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                serialIdx, orderCount, durationMs, spatialPreset.propagation);
        const float iterator = PTDimmerWaveEngine::iteratorFromElapsed(
                elapsedMs, durationMs, spatialPreset.startOffset, headOffset, timeOffset);
        const float dimmer = PTDimmerWaveEngine::calculateDimmerWave(iterator, waveParams);
        float stagedDimmer = dimmer;
        if (morphOutput)
        {
            const int stagedHeadOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                    pt.x(), pt.y(), gridSize.width(), gridSize.height(), stagedWaveParams);
            const int stagedSerialIdx = stagedSerialIndex.value(pt, 0);
            const quint32 stagedTimeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                    stagedSerialIdx, qMax(1, stagedOrderCount), stagedDurationMs, stagedPreset.propagation);
            const float stagedIterator = PTDimmerWaveEngine::iteratorFromElapsed(
                    elapsedMs, stagedDurationMs, stagedPreset.startOffset,
                    stagedHeadOffset, stagedTimeOffset);
            stagedDimmer = PTDimmerWaveEngine::calculateDimmerWave(stagedIterator, stagedWaveParams);
        }
        float multiFxDimmer = dimmer;
        float stagedMultiFxDimmer = multiFxDimmer;
        if (mixMultiFx)
        {
            const int multiFxHeadOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                    pt.x(), pt.y(), gridSize.width(), gridSize.height(), multiFxWaveParams);
            const int multiFxSerialIdx = multiFxSerialIndex.value(pt, 0);
            const quint32 multiFxTimeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                    multiFxSerialIdx, qMax(1, multiFxOrderCount), multiFxDurationMs,
                    multiFxPreset.propagation);
            const float multiFxIterator = PTDimmerWaveEngine::iteratorFromElapsed(
                    multiFxElapsedMs, multiFxDurationMs, multiFxPreset.startOffset,
                    multiFxHeadOffset, multiFxTimeOffset);
            multiFxDimmer = PTDimmerWaveEngine::calculateDimmerWave(
                    multiFxIterator, multiFxWaveParams);
            if (hasStagedMultiFx)
            {
                const int stagedMultiFxHeadOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                        pt.x(), pt.y(), gridSize.width(), gridSize.height(), stagedMultiFxWaveParams);
                const int stagedMultiFxSerialIdx = stagedMultiFxSerialIndex.value(pt, 0);
                const quint32 stagedMultiFxTimeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                        stagedMultiFxSerialIdx, qMax(1, stagedMultiFxOrderCount),
                        stagedMultiFxDurationMs, stagedMultiFxPreset.propagation);
                const float stagedMultiFxIterator = PTDimmerWaveEngine::iteratorFromElapsed(
                        stagedMultiFxElapsedMs, stagedMultiFxDurationMs,
                        stagedMultiFxPreset.startOffset, stagedMultiFxHeadOffset,
                        stagedMultiFxTimeOffset);
                stagedMultiFxDimmer = PTDimmerWaveEngine::calculateDimmerWave(
                        stagedMultiFxIterator, stagedMultiFxWaveParams);
            }
        }

        const GroupHead& head = hit.value();
        if (writtenFixtures.contains(head.fxi))
            continue;

        Fixture* fxi = m_doc->fixture(head.fxi);
        if (!fxi)
            continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size())
            continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        QVector<uchar> normalValues;
        if (morphOutput)
        {
            const QVector<uchar> liveValues = continuousOutputValues(
                    m_columns, priVals, secVals, spatialPreset, double(dimmer),
                    global.intensity);
            const QVector<uchar> stagedValues = continuousOutputValues(
                    m_columns, *stagedPriVals, *stagedSecVals, stagedPreset,
                    double(stagedDimmer), global.intensity);
            normalValues = blendRowValues(liveValues, stagedValues, morphProgress);
        }
        else
        {
            normalValues = continuousOutputValues(
                    m_columns, priVals, secVals, spatialPreset, double(dimmer),
                    global.intensity);
        }
        if (mixMultiFx)
        {
            const QVector<uchar> multiValues = multiFxPreset.enabled
                    ? continuousColumnValues(
                        m_columns, priVals, secVals,
                        double(multiFxDimmer), multiFxPreset.waveShape,
                        multiFxPreset.waveFadeIn, multiFxPreset.waveFadeOut,
                        global.intensity)
                    : QVector<uchar>(m_columns.size(), 0);
            QVector<uchar> effectiveMultiValues = multiValues;
            if (hasStagedMultiFx)
            {
                const QVector<uchar>& stagedPri = (morphOutput && stagedPriVals)
                        ? *stagedPriVals : priVals;
                const QVector<uchar>& stagedSec = (morphOutput && stagedSecVals)
                        ? *stagedSecVals : secVals;
                const QVector<uchar> stagedMultiValues = stagedMultiFxPreset.enabled
                        ? continuousColumnValues(
                            m_columns, stagedPri, stagedSec,
                            double(stagedMultiFxDimmer), stagedMultiFxPreset.waveShape,
                            stagedMultiFxPreset.waveFadeIn, stagedMultiFxPreset.waveFadeOut,
                            global.intensity)
                        : QVector<uchar>(m_columns.size(), 0);
                effectiveMultiValues = blendRowValues(multiValues, stagedMultiValues,
                                                       morphProgress);
            }
            normalValues = blendRowValues(normalValues, effectiveMultiValues,
                                          double(m_multiFxBlend) / 255.0);
        }
        applyPointChannels(fader.data(), universes[uni], head, fxi, pt, normalValues, fadeMs);
        writtenFixtures.insert(head.fxi);
    }
}

bool PresetTableV2Widget::matrixProviderReadyLocked() const
{
    return m_linkedTransitionWidgetId != VCWidget::invalidId()
            && (m_cachedTransitionSweepCount > 0 || m_cachedTransitionContinuousCount > 0
                || m_cachedTransitionMultiFxCount > 0);
}

bool PresetTableV2Widget::useMatrixEngineLocked() const
{
    return m_spatialEffects.enabled && matrixProviderReadyLocked();
}

bool PresetTableV2Widget::efxActiveForOutputLocked(int outputIdx) const
{
    return sweepEfxActiveForOutputLocked(outputIdx)
            || continuousEfxActiveForOutputLocked(outputIdx);
}

PresetTableV2Widget::PTOutputPlaybackState
PresetTableV2Widget::resolveOutputPlaybackStateLocked(int outputIdx, int activeRow,
                                                       bool hasStaged,
                                                       bool matrixReady,
                                                       bool spatialOn) const
{
    PTOutputPlaybackState state;
    state.transitionOn = sweepEfxActiveForOutputLocked(outputIdx);
    state.continuousFxOn = continuousEfxActiveForOutputLocked(outputIdx);
    const bool hasStagedSecondary = outputIdx >= 0
            && outputIdx < m_stagedSecondaryValid.size()
            && m_stagedSecondaryValid[outputIdx]
            && outputIdx < m_stagedSecondaryRow.size();
    const int effectiveSecondary = effectiveSecondaryRowLocked(outputIdx, activeRow);
    state.secondaryRow = hasStagedSecondary
            ? ((m_stagedSecondaryRow[outputIdx] >= 0
                && m_stagedSecondaryRow[outputIdx] < m_rows.size())
               ? m_stagedSecondaryRow[outputIdx]
               : effectiveSecondary)
            : effectiveSecondary;
    state.crossfadeTransition = crossfadeSweepModeLocked(outputIdx, activeRow, hasStaged);
    state.crossfadeContinuous = continuousCrossfadeModeLocked(outputIdx)
            || multiFxCrossfadeModeLocked(outputIdx);
    state.blockMatrixForStaged = hasStaged
            && !state.crossfadeTransition
            && !state.crossfadeContinuous;
    state.matrixForOutput = matrixReady
            && (spatialOn || m_crossfadeEnabled || state.transitionOn || state.continuousFxOn);
    return state;
}

void PresetTableV2Widget::ensureMatrixState(int outputIdx)
{
    while (m_matrixState.size() <= outputIdx)
        m_matrixState.append(PTOutputMatrixState());
}

int PresetTableV2Widget::matrixStateSlotForSelectionLocked(int outputIdx, int selectionKey)
{
    if (selectionKey < 0)
        return outputIdx;

    const quint64 key = (quint64(quint32(outputIdx)) << 32) | quint32(selectionKey);
    auto it = m_selectionMatrixStateSlots.constFind(key);
    if (it != m_selectionMatrixStateSlots.constEnd())
        return it.value();

    const int slot = m_matrixState.size();
    m_selectionMatrixStateSlots.insert(key, slot);
    m_matrixState.append(PTOutputMatrixState());
    while (m_continuousElapsedMs.size() <= slot)
        m_continuousElapsedMs.append(0);
    while (m_multiFxElapsedMs.size() <= slot)
        m_multiFxElapsedMs.append(0);
    while (m_multiFxStagedElapsedMs.size() <= slot)
        m_multiFxStagedElapsedMs.append(0);
    while (m_continuousLastCycleMs.size() <= slot)
        m_continuousLastCycleMs.append(0);
    while (m_multiFxLastCycleMs.size() <= slot)
        m_multiFxLastCycleMs.append(0);
    while (m_multiFxStagedLastCycleMs.size() <= slot)
        m_multiFxStagedLastCycleMs.append(0);
    return slot;
}

void PresetTableV2Widget::resetMatrixStateLocked(int outputIdx)
{
    if (outputIdx < 0)
        return;
    ensureMatrixState(outputIdx);
    PTOutputMatrixState& st = m_matrixState[outputIdx];
    st.sweepRunning = false;
    st.sweepManualCrossfade = false;
    st.sweepManualPhase = 0.0;
    st.sweepManualPhasePrev = 0.0;
    st.sweepProgress = 0.0;
    st.sweepFromRow = -1;
    st.sweepToRow = -1;
    st.sweepElapsedMs = 0;
    st.sweepLastCycleMs = 0;
    st.sweepPeakDimmer.clear();
    st.sweepHeldValues.clear();
    st.flashActive = false;
    st.flashPhase = PTFlashPhase::Idle;
    st.flashWaveProgress = 0.0;
    st.flashReleaseProgress = 1.0;
    st.flashElapsedMs = 0;
    st.flashLastCycleMs = 0;
    st.flashSourceWidgetId = 0;
    st.flashToken = 0;
    st.flashRow = -1;
    st.flashReturnRow = -1;
    st.flashValues.clear();
    st.flashReturnValues.clear();
    st.flashPreset = PTTransitionPreset();
    st.flashTimeMultiplier = 1.0;
    st.appliedRow = -1;
    if (outputIdx < m_continuousElapsedMs.size())
        m_continuousElapsedMs[outputIdx] = 0;
    if (outputIdx < m_continuousLastCycleMs.size())
        m_continuousLastCycleMs[outputIdx] = 0;
    if (outputIdx < m_multiFxElapsedMs.size())
        m_multiFxElapsedMs[outputIdx] = 0;
    if (outputIdx < m_multiFxLastCycleMs.size())
        m_multiFxLastCycleMs[outputIdx] = 0;
    if (outputIdx < m_multiFxStagedElapsedMs.size())
        m_multiFxStagedElapsedMs[outputIdx] = 0;
    if (outputIdx < m_multiFxStagedLastCycleMs.size())
        m_multiFxStagedLastCycleMs[outputIdx] = 0;
}

void PresetTableV2Widget::resetAllMatrixStatesLocked()
{
    for (int o = 0; o < m_matrixState.size(); ++o)
        resetMatrixStateLocked(o);
}

void PresetTableV2Widget::beginMatrixSweepLocked(int outputIdx, int prevRow, int newRowIdx,
                                                bool forceSpatialSweep)
{
    ensureMatrixState(outputIdx);
    PTOutputMatrixState& st = m_matrixState[outputIdx];
    if (newRowIdx < 0 || newRowIdx >= m_rows.size())
        return;

    if (prevRow < 0 || prevRow == newRowIdx)
    {
        st.appliedRow = newRowIdx;
        st.sweepRunning = false;
        st.sweepProgress = 0.0;
        return;
    }

    const PTTransitionPreset fxPreset = sweepEfxActiveForOutputLocked(outputIdx)
            ? sweepPresetForOutputLocked(outputIdx)
            : transitionPresetForOutputLocked(outputIdx);
    if (!forceSpatialSweep
            && PTParamMatrixEngine::sweepInstantForOffset(fxPreset.offsetDirection))
    {
        st.appliedRow = newRowIdx;
        st.sweepRunning = false;
        st.sweepProgress = 0.0;
        return;
    }

    st.sweepFromRow = prevRow;
    st.sweepToRow = newRowIdx;
    st.sweepElapsedMs = 0;
    st.sweepLastCycleMs = 0;
    st.sweepPeakDimmer.clear();
    st.sweepHeldValues.clear();
    st.sweepRunning = true;
    st.sweepProgress = 0.0;
    st.sweepManualPhase = 0.0;
}

bool PresetTableV2Widget::beginMatrixFlashLocked(int outputIdx, int rowIdx,
                                                 int transitionPresetIndex,
                                                 quint32 sourceWidgetId,
                                                 quint64 token,
                                                 double timeMultiplier)
{
    if (outputIdx < 0 || outputIdx >= m_outputs.size()
            || rowIdx < 0 || rowIdx >= m_rows.size())
        return false;

    ensureMatrixState(outputIdx);
    PTOutputMatrixState& st = m_matrixState[outputIdx];

    if (!st.flashActive)
    {
        st.flashReturnRow = (outputIdx < m_activeRow.size()) ? m_activeRow[outputIdx] : -1;
        st.flashReturnValues = (st.flashReturnRow >= 0 && st.flashReturnRow < m_rows.size())
                ? m_rows[st.flashReturnRow].values : QVector<uchar>(m_columns.size(), uchar(0));
    }

    st.flashSourceWidgetId = sourceWidgetId;
    st.flashToken = token;
    st.flashRow = rowIdx;
    st.flashValues = m_rows[rowIdx].values;
    st.flashPreset = transitionPresetAtIndexLocked(PTTransitionMode::SweepOnly,
                                                   transitionPresetIndex, outputIdx);
    if (transitionPresetIndex < 0 || !st.flashPreset.enabled)
        st.flashPreset = transitionPresetForOutputLocked(outputIdx);
    st.flashTimeMultiplier = qBound(0.05, timeMultiplier, 16.0);

    st.flashActive = true;
    st.flashElapsedMs = 0;
    st.flashLastCycleMs = 0;
    st.flashWaveProgress = 0.0;
    st.flashReleaseProgress = 1.0;

    if (PTParamMatrixEngine::waveFrontFromOffset(st.flashPreset.offsetDirection) <= 0)
        st.flashPhase = PTFlashPhase::Hold;
    else
        st.flashPhase = PTFlashPhase::WaveIn;

    return true;
}

void PresetTableV2Widget::beginMatrixFlashWaveOutLocked(PTOutputMatrixState& st)
{
    if (st.flashPhase == PTFlashPhase::WaveIn)
        st.flashReleaseProgress = st.flashWaveProgress;
    else
        st.flashReleaseProgress = 1.0;

    st.flashPhase = PTFlashPhase::WaveOut;
    st.flashWaveProgress = 0.0;
    st.flashElapsedMs = 0;
    st.flashLastCycleMs = 0;
}

bool PresetTableV2Widget::endMatrixFlashLocked(int outputIdx, int rowIdx,
                                               quint32 sourceWidgetId, quint64 token)
{
    if (outputIdx < 0 || outputIdx >= m_matrixState.size())
        return false;

    PTOutputMatrixState& st = m_matrixState[outputIdx];
    if (!st.flashActive)
        return false;
    if (st.flashSourceWidgetId != sourceWidgetId || st.flashToken != token)
        return false;
    if (rowIdx >= 0 && st.flashRow != rowIdx)
        return false;

    if (PTParamMatrixEngine::waveFrontFromOffset(st.flashPreset.offsetDirection) <= 0)
    {
        st.flashActive = false;
        st.flashPhase = PTFlashPhase::Idle;
        st.flashRow = -1;
    }
    else
    {
        beginMatrixFlashWaveOutLocked(st);
    }
    return true;
}

void PresetTableV2Widget::releaseMatrixFlashLocked(int outputIdx)
{
    if (outputIdx < 0 || outputIdx >= m_matrixState.size())
        return;

    PTOutputMatrixState& st = m_matrixState[outputIdx];
    if (!st.flashActive)
        return;

    if (st.flashPhase == PTFlashPhase::WaveIn)
    {
        beginMatrixFlashWaveOutLocked(st);
    }
    else if (st.flashPhase == PTFlashPhase::Hold)
    {
        const PTTransitionPreset fxPreset = st.flashPreset;
        if (PTParamMatrixEngine::waveFrontFromOffset(fxPreset.offsetDirection) <= 0)
        {
            st.flashActive = false;
            st.flashPhase = PTFlashPhase::Idle;
            st.flashRow = -1;
        }
        else
        {
            beginMatrixFlashWaveOutLocked(st);
        }
    }
}

void PresetTableV2Widget::beginWidgetStagedFlashLocked()
{
    if (m_mode == PTMode::Position)
        return;

    const quint64 token = m_nextWidgetStagedFlashToken++;
    bool anyStarted = false;
    const double timeMultiplier =
            widgetFlashTimeMultiplierValue(m_widgetFlashTimeMultiplierIndex);

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int liveRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const bool stagedValid = o < m_stagedRowValid.size() && m_stagedRowValid[o]
                && o < m_stagedRow.size();
        const int stagedRow = stagedValid ? m_stagedRow[o] : -1;
        if (stagedRow < 0 || stagedRow >= m_rows.size() || stagedRow == liveRow)
            continue;

        const int presetIdx = liveSweepPresetIndexLocked(o);
        if (beginMatrixFlashLocked(o, stagedRow, presetIdx, id(), token, timeMultiplier))
            anyStarted = true;
    }

    m_widgetStagedFlashToken = anyStarted ? token : 0;
}

void PresetTableV2Widget::endWidgetStagedFlashLocked()
{
    if (m_widgetStagedFlashToken == 0)
        return;

    const quint64 token = m_widgetStagedFlashToken;
    m_widgetStagedFlashToken = 0;
    for (int o = 0; o < m_outputs.size(); ++o)
        endMatrixFlashLocked(o, -1, id(), token);
}

void PresetTableV2Widget::setWidgetFlashGateActiveLocked(bool active, uchar value)
{
    if (m_widgetFlashGateActive == active)
    {
        m_widgetFlashGateLastValue = value;
        return;
    }

    m_widgetFlashGateActive = active;
    m_widgetFlashGateLastValue = value;

    if (m_widgetFlashBehavior == PTWidgetFlashBehavior::StagedRowTrigger)
    {
        if (active)
            beginWidgetStagedFlashLocked();
        else
            endWidgetStagedFlashLocked();
        return;
    }

    if (!active)
    {
        for (int o = 0; o < m_matrixState.size(); ++o)
            releaseMatrixFlashLocked(o);
    }
}

void PresetTableV2Widget::requestTableFlash(int tableRowIndex, int transitionPresetIndex)
{
    QMutexLocker lk(&m_stateMutex);
    if (tableRowIndex < 0 || tableRowIndex >= m_rows.size())
        return;

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        if (o >= m_activeRow.size() || m_activeRow[o] < 0)
            continue;
        beginMatrixFlashLocked(o, tableRowIndex, transitionPresetIndex, id(), 0, 1.0);
    }
}

static float matrixDimmerAtPoint(const QLCPoint& pt,
                                 quint32 elapsedMs,
                                 quint32 cycleMs,
                                 const PTTransitionPreset& preset,
                                 const PTGlobalEffectSettings& global,
                                 const QSize& gridSize,
                                 int serialIndex,
                                 int serialCount)
{
    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(preset, &global);
    if (global.fxOrientation == 1)
        waveParams.axis = PTTransitionAxis::Y;

    const int headOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
            pt.x(), pt.y(), gridSize.width(), gridSize.height(), waveParams);
    const quint32 timeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
            serialIndex, serialCount, cycleMs, waveParams.propagation);
    const float iterator = PTDimmerWaveEngine::iteratorFromElapsed(
            elapsedMs, cycleMs, waveParams.startOffset, headOffset, timeOffset);
    return PTDimmerWaveEngine::calculateDimmerWave(iterator, waveParams);
}

void PresetTableV2Widget::writeDMXPositionFixtureGroup(MasterTimer* /*timer*/,
                                                       QList<Universe*>& universes,
                                                       uchar xfEffective)
{
    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp)
        return;

    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();
    const QSize gridSize = grp->size();
    const bool spatialOn = m_spatialEffects.enabled;
    const PTGlobalEffectSettings globalFx = globalEffectSettingsLocked();

    while (m_continuousElapsedMs.size() < m_outputs.size())
        m_continuousElapsedMs.append(0);
    while (m_continuousLastCycleMs.size() < m_outputs.size())
        m_continuousLastCycleMs.append(0);

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int activeRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const bool stagedPrimaryValid = o < m_stagedRowValid.size() && m_stagedRowValid[o];
        const int stagedRow = (stagedPrimaryValid && o < m_stagedRow.size()) ? m_stagedRow[o] : -1;
        const bool activeRowValid = activeRow >= 0 && activeRow < m_rows.size();
        const bool stagedRowValid = stagedRow >= 0 && stagedRow < m_rows.size();
        if (!activeRowValid && !stagedRowValid)
            continue;

        const PTOutput& out = m_outputs[o];
        if (out.scope == PTOutputScope::Mask && !docMask.isActive())
            continue;

        const QMap<QLCPoint, GroupHead>& headsMap =
                (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;
        const QList<PTOutputScopeFixture> scopeFixtures =
                collectOutputScopeFixtures(headsMap, out);
        if (scopeFixtures.isEmpty())
            continue;

        const bool hasStaged = stagedPrimaryValid && stagedRow != activeRow;
        const PTOutputPlaybackState playback = activeRowValid
                ? resolveOutputPlaybackStateLocked(o, activeRow, hasStaged,
                                                   true, spatialOn)
                : PTOutputPlaybackState();
        const bool contOn = playback.continuousFxOn;
        const bool sweepOn = playback.transitionOn;
        const int secondaryRow = playback.secondaryRow;
        const bool secondaryValid = secondaryRow >= 0 && secondaryRow < m_rows.size();
        const bool multiFxOn = multiFxActiveForOutputLocked(o);
        const bool blockFxDuringXf = m_crossfadeEnabled && hasStaged;
        const bool relativeFxOn = spatialOn && !blockFxDuringXf
                && (contOn || sweepOn || multiFxOn);

        quint32 cycleMs = qMax(quint32(1), cycleDurationMsLocked(globalFx, PTTransitionPreset()));
        if (relativeFxOn)
        {
            PTTransitionPreset cyclePreset = contOn
                    ? continuousPresetForOutputLocked(o) : transitionPresetForOutputLocked(o);
            cycleMs = qMax(quint32(1), cycleDurationMsLocked(globalFx, cyclePreset));
            ensurePhaseStableCycleLocked(m_continuousElapsedMs, m_continuousLastCycleMs, o, cycleMs);
            m_continuousElapsedMs[o] += MasterTimer::tick();
            if (m_continuousElapsedMs[o] > cycleMs)
                m_continuousElapsedMs[o] = 0;
        }
        const quint32 elapsedMs = quint32(o < m_continuousElapsedMs.size()
                ? m_continuousElapsedMs[o] : 0);

        QList<QLCPoint> points;
        for (const PTOutputScopeFixture& sf : scopeFixtures)
            points.append(sf.point);

        const bool crossfadeSweep = playback.crossfadeTransition;
        const PTTransitionPreset sweepPreset = transitionPresetForOutputLocked(o);
        const PTSpatialFixturePlan sweepPlan = PTSpatialFixturePlan::build(
                points, sweepPreset, globalFx, gridSize.width(), gridSize.height());
        const double xfProgress = crossfadeProgress01Locked(xfEffective);

        for (const PTOutputScopeFixture& sf : scopeFixtures)
        {
            Fixture* fxi = m_doc->fixture(sf.fxiId);
            if (!fxi)
                continue;

            quint32 uni = fxi->universe();
            if ((int)uni >= universes.size())
                continue;

            PTTransitionPreset fxPreset = contOn
                    ? transitionPresetAtIndexLocked(PTTransitionMode::Continuous,
                                                    liveContinuousPresetIndexLocked(o), o, &sf.point)
                    : transitionPresetAtIndexLocked(PTTransitionMode::SweepOnly,
                                                    liveSweepPresetIndexLocked(o), o, &sf.point);
            if (!fxPreset.enabled)
                fxPreset = PTTransitionPreset();

            const PTSpatialFixturePlan spatialPlan = PTSpatialFixturePlan::build(
                    points, fxPreset, globalFx, gridSize.width(), gridSize.height());
            const int serialCount = qMax(1, spatialPlan.count());

            const bool positionXfActive = stagedRowValid && m_crossfadeEnabled && hasStaged;

            PTPositionValue base;
            if (activeRowValid)
            {
                base = positionXfActive
                        ? effectivePositionValue(activeRow, o, sf.point)
                        : positionDraftValueForOutput(o, activeRow, sf.point);
            }
            if (!base.valid && stagedRowValid)
                base = effectivePositionValue(stagedRow, o, sf.point);
            if (!base.valid)
                continue;

            if (positionXfActive)
            {
                const PTPositionValue staged = effectivePositionValue(stagedRow, o, sf.point);
                double blend = double(xfEffective) / 255.0;
                if (crossfadeSweep && sweepPreset.enabled)
                {
                    blend = sweepPlan.sweepBlend01(xfProgress, sf.point, sweepPreset,
                                                   globalFx);
                }
                base = PTPositionConverter::blendPositions(base, staged, blend);
            }

            if (contOn && secondaryValid && !blockFxDuringXf)
            {
                const PTPositionValue secondary = effectivePositionValue(secondaryRow, o, sf.point);
                if (secondary.valid)
                {
                    const int serialIdx = spatialPlan.indexByPoint.value(sf.point, 0);
                    const float dimmer = matrixDimmerAtPoint(sf.point, elapsedMs, cycleMs,
                                                             fxPreset, globalFx, gridSize,
                                                             serialIdx, serialCount);
                    base = PTPositionConverter::blendPositions(base, secondary, double(dimmer));
                }
            }
            else if (relativeFxOn && fxPreset.enabled
                       && fxPreset.positionMotion != int(PTPositionMotion::Off))
            {
                PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(
                        fxPreset, &globalFx);
                if (globalFx.fxOrientation == 1)
                    waveParams.axis = PTTransitionAxis::Y;
                waveParams.propagation = PTPropagationMode::Parallel;
                const int headOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                        sf.point.x(), sf.point.y(), gridSize.width(), gridSize.height(),
                        waveParams);
                const int serialIdx = spatialPlan.indexByPoint.value(sf.point, 0);
                const quint32 timeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                        serialIdx, serialCount, cycleMs, waveParams.propagation);
                const float iterator = PTDimmerWaveEngine::iteratorFromElapsed(
                        elapsedMs, cycleMs, waveParams.startOffset, headOffset, timeOffset);
                const PTDimmerWaveOffsetInfo spatialInfo =
                        PTDimmerWaveEngine::offsetInfoForPoint(
                                sf.point.x(), sf.point.y(), gridSize.width(),
                                gridSize.height(), waveParams);
                const PTPositionMotion fxMotion = PTPositionMotion(fxPreset.positionMotion);
                const bool fxMotion1D = PTPositionFxEngine::motionIs1D(fxMotion);
                double orbitPhase = 0.0;
                const bool inOrbitWindow = PTPositionFxEngine::orbitPhaseFromIterator(
                        iterator, waveParams.waveWidth, orbitPhase);
                if (fxMotion1D || inOrbitWindow)
                {
                    double phaseForApply = orbitPhase;
                    if (!fxMotion1D)
                    {
                        phaseForApply = PTPositionFxEngine::applyMotionDirection(
                                orbitPhase,
                                PTPositionMotionDirection(fxPreset.positionMotionDirection),
                                spatialInfo);
                    }
                    const qreal size01 = qreal(globalFx.positionSize) / 255.0;
                    base = PTPositionFxEngine::applySmartMotionFromPreset(
                            base, fxi, sf.head.head, fxPreset, phaseForApply, size01,
                            fxMotion1D ? iterator : -1.0f,
                            fxMotion1D ? &waveParams : nullptr,
                            fxMotion1D ? &spatialInfo : nullptr,
                            fxMotion1D ? headOffset : 0);
                }
            }

            if (multiFxOn && !blockFxDuringXf)
            {
                const PTTransitionPreset mfPreset = transitionPresetAtIndexLocked(
                        PTTransitionMode::MultiFx, liveMultiFxPresetIndexLocked(o), o, &sf.point);
                if (mfPreset.enabled
                        && mfPreset.positionMotion != int(PTPositionMotion::Off))
                {
                    while (m_multiFxElapsedMs.size() <= o)
                        m_multiFxElapsedMs.append(0);
                    const quint32 mfCycle = qMax(quint32(1), cycleDurationMsLocked(globalFx, mfPreset));
                    ensurePhaseStableCycleLocked(m_multiFxElapsedMs, m_multiFxLastCycleMs, o, mfCycle);
                    m_multiFxElapsedMs[o] += MasterTimer::tick();
                    if (m_multiFxElapsedMs[o] > mfCycle)
                        m_multiFxElapsedMs[o] = 0;
                    const PTSpatialFixturePlan mfPlan = PTSpatialFixturePlan::build(
                            points, mfPreset, globalFx, gridSize.width(), gridSize.height());
                    const int mfSerialCount = qMax(1, mfPlan.count());
                    PTDimmerWaveParams waveParams = PTDimmerWaveEngine::paramsFromPreset(
                            mfPreset, &globalFx);
                    if (globalFx.fxOrientation == 1)
                        waveParams.axis = PTTransitionAxis::Y;
                    waveParams.propagation = PTPropagationMode::Parallel;
                    const int serialIdx = mfPlan.indexByPoint.value(sf.point, 0);
                    const quint32 timeOffset = PTDimmerWaveEngine::serialTimeOffsetMs(
                            serialIdx, mfSerialCount, mfCycle, waveParams.propagation);
                    const int headOffset = PTDimmerWaveEngine::calculateHeadStartOffsetExtended(
                            sf.point.x(), sf.point.y(), gridSize.width(), gridSize.height(),
                            waveParams);
                    const float iterator = PTDimmerWaveEngine::iteratorFromElapsed(
                            m_multiFxElapsedMs[o], mfCycle, waveParams.startOffset,
                            headOffset, timeOffset);
                    const PTDimmerWaveOffsetInfo spatialInfo =
                            PTDimmerWaveEngine::offsetInfoForPoint(
                                    sf.point.x(), sf.point.y(), gridSize.width(),
                                    gridSize.height(), waveParams);
                    const PTPositionMotion mfMotion = PTPositionMotion(mfPreset.positionMotion);
                    const bool mfMotion1D = PTPositionFxEngine::motionIs1D(mfMotion);
                    double orbitPhase = 0.0;
                    const bool inOrbitWindow = PTPositionFxEngine::orbitPhaseFromIterator(
                            iterator, waveParams.waveWidth, orbitPhase);
                    if (mfMotion1D || inOrbitWindow)
                    {
                        double phaseForApply = orbitPhase;
                        if (!mfMotion1D)
                        {
                            phaseForApply = PTPositionFxEngine::applyMotionDirection(
                                    orbitPhase,
                                    PTPositionMotionDirection(mfPreset.positionMotionDirection),
                                    spatialInfo);
                        }
                        const qreal blend = qreal(m_multiFxBlend) / 255.0;
                        const qreal size01 = (qreal(globalFx.positionSize) / 255.0) * blend;
                        base = PTPositionFxEngine::applySmartMotionFromPreset(
                                base, fxi, sf.head.head, mfPreset, phaseForApply, size01,
                                mfMotion1D ? iterator : -1.0f,
                                mfMotion1D ? &waveParams : nullptr,
                                mfMotion1D ? &spatialInfo : nullptr,
                                mfMotion1D ? headOffset : 0);
                    }
                }
            }

            auto fader = m_faders.value(uni);
            if (fader.isNull())
            {
                fader = universes[uni]->requestFader(Universe::Auto);
                m_faders.insert(uni, fader);
            }
            applyPointPosition(fader.data(), universes[uni], sf.head, fxi, base, 0);
        }
    }
}

void PresetTableV2Widget::writeMatrixSpatial(int outputIdx, MasterTimer* timer,
                                              QList<Universe*>& universes,
                                              const PTOutput& out, int activeRow, int secondaryRow,
                                              const PTTransitionPreset& preset,
                                              const PTGlobalEffectSettings& global,
                                              const QSize& gridSize,
                                              const QMap<QLCPoint, GroupHead>& headsMap,
                                              bool forceContinuousBlend,
                                              const QVector<uchar>* primaryOverride,
                                              const QVector<uchar>* secondaryOverride,
                                              const QVector<uchar>* stagedPrimaryOverride,
                                              const QVector<uchar>* stagedSecondaryOverride,
                                              const PTTransitionPreset* stagedPresetOverride,
                                              double morphProgress,
                                              bool useMultiFx,
                                              int stateSlot)
{
    Q_UNUSED(timer);

    if (stateSlot < 0 && outputIdx >= 0)
    {
        const PTTransitionMode bucketMode =
                (preset.playbackMode == PTTransitionMode::Continuous)
                ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
        const int bucketPresetIndex = (bucketMode == PTTransitionMode::Continuous)
                ? liveContinuousPresetIndexLocked(outputIdx)
                : liveSweepPresetIndexLocked(outputIdx);
        PresetTableV2TransitionProviderIface* provider = transitionProviderLocked();
        if (provider && bucketPresetIndex >= 0
                && bucketPresetIndex < provider->transitionPresetCount(bucketMode))
        {
            QMap<int, QMap<QLCPoint, GroupHead>> buckets;
            bool hasCustomSelection = false;
            for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
            {
                if (!outputScopeAllowsPoint(out.scope, it.key(), out))
                    continue;
                const int key = provider->transitionSelectionKeyForPoint(
                        bucketMode, bucketPresetIndex, outputIdx, it.key());
                if (key >= 0)
                    hasCustomSelection = true;
                buckets[key].insert(it.key(), it.value());
            }

            if (hasCustomSelection)
            {
                for (auto bit = buckets.constBegin(); bit != buckets.constEnd(); ++bit)
                {
                    if (bit.value().isEmpty())
                        continue;
                    const int selectionKey = bit.key();
                    const PTTransitionPreset bucketPreset =
                            provider->effectiveTransitionPresetForSelection(
                                bucketMode, bucketPresetIndex, outputIdx, selectionKey);
                    PTTransitionPreset bucketStagedPreset;
                    const PTTransitionPreset* bucketStagedPresetPtr = stagedPresetOverride;
                    if (stagedPresetOverride
                            && bucketMode == PTTransitionMode::Continuous
                            && outputIdx < m_stagedContinuousValid.size()
                            && outputIdx < m_stagedContinuousPreset.size()
                            && m_stagedContinuousValid[outputIdx]
                            && m_stagedContinuousPreset[outputIdx] >= 0)
                    {
                        bucketStagedPreset =
                                provider->effectiveTransitionPresetForSelection(
                                    PTTransitionMode::Continuous,
                                    m_stagedContinuousPreset[outputIdx],
                                    outputIdx, selectionKey);
                        bucketStagedPresetPtr = &bucketStagedPreset;
                    }
                    const int bucketStateSlot =
                            matrixStateSlotForSelectionLocked(outputIdx, selectionKey);
                    if (selectionKey >= 0 && outputIdx < m_matrixState.size())
                    {
                        ensureMatrixState(bucketStateSlot);
                        const PTOutputMatrixState& src = m_matrixState[outputIdx];
                        PTOutputMatrixState& dst = m_matrixState[bucketStateSlot];
                        if (src.flashActive || dst.flashActive)
                        {
                            dst.flashActive = src.flashActive;
                            dst.flashPhase = src.flashPhase;
                            dst.flashWaveProgress = src.flashWaveProgress;
                            dst.flashReleaseProgress = src.flashReleaseProgress;
                            dst.flashElapsedMs = src.flashElapsedMs;
                            dst.flashLastCycleMs = src.flashLastCycleMs;
                            dst.flashSourceWidgetId = src.flashSourceWidgetId;
                            dst.flashToken = src.flashToken;
                            dst.flashRow = src.flashRow;
                            dst.flashReturnRow = src.flashReturnRow;
                            dst.flashValues = src.flashValues;
                            dst.flashReturnValues = src.flashReturnValues;
                            dst.flashPreset = src.flashPreset;
                            dst.flashTimeMultiplier = src.flashTimeMultiplier;
                        }
                        if (src.sweepRunning
                                && (!dst.sweepRunning
                                    || dst.sweepFromRow != src.sweepFromRow
                                    || dst.sweepToRow != src.sweepToRow
                                    || dst.sweepManualCrossfade != src.sweepManualCrossfade))
                        {
                            dst.sweepRunning = true;
                            dst.sweepManualCrossfade = src.sweepManualCrossfade;
                            dst.sweepManualPhase = src.sweepManualPhase;
                            dst.sweepManualPhasePrev = src.sweepManualPhasePrev;
                            dst.sweepProgress = src.sweepProgress;
                            dst.sweepFromRow = src.sweepFromRow;
                            dst.sweepToRow = src.sweepToRow;
                            dst.sweepElapsedMs = src.sweepElapsedMs;
                            dst.sweepLastCycleMs = src.sweepLastCycleMs;
                            dst.sweepPeakDimmer.clear();
                            dst.sweepHeldValues.clear();
                        }
                        if (src.sweepManualCrossfade)
                        {
                            dst.sweepManualCrossfade = true;
                            dst.sweepManualPhase = src.sweepManualPhase;
                        }
                        if (!src.sweepRunning && !dst.sweepRunning && dst.appliedRow != src.appliedRow)
                            dst.appliedRow = src.appliedRow;
                    }
                    writeMatrixSpatial(outputIdx, timer, universes, out, activeRow, secondaryRow,
                                       bucketPreset, global, gridSize, bit.value(),
                                       forceContinuousBlend, primaryOverride, secondaryOverride,
                                       stagedPrimaryOverride, stagedSecondaryOverride,
                                       bucketStagedPresetPtr, morphProgress, useMultiFx,
                                       bucketStateSlot);
                }
                return;
            }
        }
    }

    const int stateIdx = stateSlot >= 0 ? stateSlot : outputIdx;
    ensureMatrixState(stateIdx);
    PTOutputMatrixState& st = m_matrixState[stateIdx];

    const bool useSecondaryBlend = forceContinuousBlend
            || (preset.playbackMode == PTTransitionMode::Continuous
                && secondaryRow >= 0 && secondaryRow < m_rows.size());

    const int blendFromRow = st.sweepRunning ? st.sweepFromRow : activeRow;
    const int blendToRow = st.sweepRunning ? st.sweepToRow
            : (useSecondaryBlend ? secondaryRow : activeRow);

    const QVector<uchar> priVals = primaryOverride ? *primaryOverride
            : ((blendFromRow >= 0 && blendFromRow < m_rows.size())
                ? m_rows[blendFromRow].values : QVector<uchar>());
    const QVector<uchar> secVals = secondaryOverride ? *secondaryOverride
            : ((blendToRow >= 0 && blendToRow < m_rows.size())
                ? m_rows[blendToRow].values : priVals);
    const bool morphOutput = stagedPresetOverride && stagedPrimaryOverride && stagedSecondaryOverride;
    const PTTransitionPreset stagedPreset = morphOutput ? *stagedPresetOverride : preset;
    const PTTransitionPreset multiFxPreset = multiFxPresetForOutputLocked(outputIdx);
    const int stagedMultiFxIdx = stagedMultiFxPresetIndexLocked(outputIdx);
    const bool hasStagedMultiFx = hasStagedMultiFxPresetLocked(outputIdx);
    const PTTransitionPreset stagedMultiFxPreset = hasStagedMultiFx
            ? transitionPresetAtIndexLocked(PTTransitionMode::MultiFx, stagedMultiFxIdx, outputIdx)
            : multiFxPreset;
    const bool mixMultiFx = useMultiFx && m_multiFxBlend > 0
            && (multiFxPreset.enabled || stagedMultiFxPreset.enabled);

    const bool continuousFx = forceContinuousBlend
            || (!st.sweepRunning
                && preset.playbackMode == PTTransitionMode::Continuous);

    const quint32 cycleMs = qMax(quint32(1), cycleDurationMsLocked(global, preset));

    ensurePhaseStableCycleLocked(m_continuousElapsedMs, m_continuousLastCycleMs,
                                 stateIdx, cycleMs);
    const quint32 multiFxCycleForOutput = qMax(quint32(1),
            cycleDurationMsLocked(global, multiFxPreset));
    ensurePhaseStableCycleLocked(m_multiFxElapsedMs, m_multiFxLastCycleMs,
                                 stateIdx, multiFxCycleForOutput);
    if (hasStagedMultiFx)
    {
        const quint32 stagedMultiFxCycleMs = qMax(quint32(1),
                cycleDurationMsLocked(global, stagedMultiFxPreset));
        ensurePhaseStableCycleLocked(m_multiFxStagedElapsedMs, m_multiFxStagedLastCycleMs,
                                     stateIdx, stagedMultiFxCycleMs);
    }
    if (continuousFx && !st.flashActive)
    {
        m_continuousElapsedMs[stateIdx] += MasterTimer::tick();
        if (m_continuousElapsedMs[stateIdx] > cycleMs)
            m_continuousElapsedMs[stateIdx] = 0;
    }
    const quint32 elapsedMs = quint32(m_continuousElapsedMs[stateIdx]);
    const quint32 multiFxElapsedMs = quint32(m_multiFxElapsedMs[stateIdx]);
    const quint32 stagedMultiFxElapsedMs = hasStagedMultiFx
            ? quint32(m_multiFxStagedElapsedMs[stateIdx]) : multiFxElapsedMs;

    QList<QLCPoint> points;
    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        if (outputScopeAllowsPoint(out.scope, it.key(), out))
            points.append(it.key());
    }

    const PTTransitionPreset activeFlashPreset = st.flashActive ? st.flashPreset : preset;
    const PTSpatialFixturePlan spatialPlan = PTSpatialFixturePlan::build(
            points, preset, global, gridSize.width(), gridSize.height());
    const int serialCount = qMax(1, spatialPlan.count());
    const PTSpatialFixturePlan flashSpatialPlan = st.flashActive
            ? PTSpatialFixturePlan::build(points, activeFlashPreset, global,
                                          gridSize.width(), gridSize.height())
            : PTSpatialFixturePlan();
    const PTSpatialFixturePlan stagedSpatialPlan = morphOutput
            ? PTSpatialFixturePlan::build(points, stagedPreset, global,
                                          gridSize.width(), gridSize.height())
            : PTSpatialFixturePlan();
    const int stagedSerialCount = qMax(1, stagedSpatialPlan.count());
    const PTSpatialFixturePlan multiFxSpatialPlan = mixMultiFx
            ? PTSpatialFixturePlan::build(points, multiFxPreset, global,
                                          gridSize.width(), gridSize.height())
            : PTSpatialFixturePlan();
    const int multiFxSerialCount = qMax(1, multiFxSpatialPlan.count());
    const PTSpatialFixturePlan stagedMultiFxSpatialPlan = mixMultiFx && hasStagedMultiFx
            ? PTSpatialFixturePlan::build(points, stagedMultiFxPreset, global,
                                          gridSize.width(), gridSize.height())
            : PTSpatialFixturePlan();
    const int stagedMultiFxSerialCount = qMax(1, stagedMultiFxSpatialPlan.count());

    auto dimmerAtPoint = [&](const QLCPoint& pt, quint32 timeMs) -> float {
        const int serialIdx = spatialPlan.indexByPoint.value(pt, 0);
        return matrixDimmerAtPoint(pt, timeMs, cycleMs, preset, global, gridSize,
                                   serialIdx, serialCount);
    };
    auto stagedDimmerAtPoint = [&](const QLCPoint& pt, quint32 timeMs) -> float {
        const int serialIdx = stagedSpatialPlan.indexByPoint.value(pt, 0);
        const quint32 stagedCycleMs = qMax(quint32(1), cycleDurationMsLocked(global, stagedPreset));
        return matrixDimmerAtPoint(pt, timeMs, stagedCycleMs, stagedPreset, global,
                                   gridSize, serialIdx, stagedSerialCount);
    };
    auto multiFxDimmerAtPoint = [&](const QLCPoint& pt, quint32 timeMs) -> float {
        const int serialIdx = multiFxSpatialPlan.indexByPoint.value(pt, 0);
        const quint32 multiFxCycleMs = qMax(quint32(1), cycleDurationMsLocked(global, multiFxPreset));
        return matrixDimmerAtPoint(pt, timeMs, multiFxCycleMs, multiFxPreset, global,
                                   gridSize, serialIdx, multiFxSerialCount);
    };
    auto stagedMultiFxDimmerAtPoint = [&](const QLCPoint& pt, quint32 timeMs) -> float {
        const int serialIdx = stagedMultiFxSpatialPlan.indexByPoint.value(pt, 0);
        const quint32 stagedMultiFxCycleMs =
                qMax(quint32(1), cycleDurationMsLocked(global, stagedMultiFxPreset));
        return matrixDimmerAtPoint(pt, timeMs, stagedMultiFxCycleMs,
                                   stagedMultiFxPreset, global, gridSize, serialIdx,
                                   stagedMultiFxSerialCount);
    };

    const bool snapBlend = (preset.waveShape == 1);
    auto presetForPoint = [&](const QLCPoint& pt) -> PTTransitionPreset {
        const PTTransitionMode modeForPoint =
                (preset.playbackMode == PTTransitionMode::Continuous)
                ? PTTransitionMode::Continuous : PTTransitionMode::SweepOnly;
        const int presetIndex = (modeForPoint == PTTransitionMode::Continuous)
                ? liveContinuousPresetIndexLocked(outputIdx)
                : liveSweepPresetIndexLocked(outputIdx);
        if (presetIndex < 0)
            return preset;
        return transitionPresetAtIndexLocked(modeForPoint, presetIndex, outputIdx, &pt);
    };
    auto stagedPresetForPoint = [&](const QLCPoint& pt) -> PTTransitionPreset {
        if (!morphOutput)
            return presetForPoint(pt);
        if (preset.playbackMode == PTTransitionMode::Continuous
                && outputIdx >= 0
                && outputIdx < m_stagedContinuousValid.size()
                && outputIdx < m_stagedContinuousPreset.size()
                && m_stagedContinuousValid[outputIdx]
                && m_stagedContinuousPreset[outputIdx] >= 0)
            return transitionPresetAtIndexLocked(PTTransitionMode::Continuous,
                                                m_stagedContinuousPreset[outputIdx],
                                                outputIdx, &pt);
        return stagedPreset;
    };
    auto multiFxPresetForPoint = [&](const QLCPoint& pt, bool staged) -> PTTransitionPreset {
        if (staged && hasStagedMultiFx && stagedMultiFxIdx >= 0)
            return transitionPresetAtIndexLocked(PTTransitionMode::MultiFx,
                                                stagedMultiFxIdx, outputIdx, &pt);
        const int idx = liveMultiFxPresetIndexLocked(outputIdx);
        if (idx < 0)
            return multiFxPreset;
        return transitionPresetAtIndexLocked(PTTransitionMode::MultiFx, idx, outputIdx, &pt);
    };
    auto applySweepBlend = [&](const QVector<uchar>& fromRow, const QVector<uchar>& toRow,
                               float blend) -> QVector<uchar> {
        if (blend <= 0.0f)
            return PTParamMatrixEngine::blendWithIntensity(fromRow, global.intensity);
        if (blend >= 1.0f)
            return PTParamMatrixEngine::blendWithIntensity(toRow, global.intensity);
        QVector<uchar> blended = PresetTableV2SpatialEngine::blendValues(
                fromRow, toRow, double(blend), snapBlend);
        return PTParamMatrixEngine::blendWithIntensity(blended, global.intensity);
    };

    if (st.sweepRunning && !st.sweepManualCrossfade)
    {
        const quint32 sweepDurationMs = qMax(quint32(1), cycleMs * 2);
        if (st.sweepLastCycleMs > 0 && st.sweepLastCycleMs != cycleMs)
            rescaleElapsedForDurationChange(st.sweepElapsedMs,
                                            qMax(quint32(1), st.sweepLastCycleMs * 2),
                                            sweepDurationMs);
        st.sweepLastCycleMs = cycleMs;
        st.sweepElapsedMs += MasterTimer::tick();
    }

    const double flashTimeMultiplier = st.flashActive
            ? qBound(0.05, st.flashTimeMultiplier, 16.0) : 1.0;
    const quint32 baseFlashCycleMs = cycleDurationMsLocked(global, activeFlashPreset);
    const quint32 flashCycleMs = qMax(quint32(MasterTimer::tick()),
            quint32(qRound64(double(baseFlashCycleMs) * flashTimeMultiplier)));

    if (st.flashActive)
    {
        const quint32 activeFlashCycleMs = (st.flashPhase == PTFlashPhase::WaveOut)
                ? qMax(quint32(MasterTimer::tick()),
                       quint32(qRound64(double(flashCycleMs) * st.flashReleaseProgress)))
                : flashCycleMs;

        if (st.flashLastCycleMs > 0 && st.flashLastCycleMs != activeFlashCycleMs)
            rescaleElapsedForDurationChange(st.flashElapsedMs,
                                            st.flashLastCycleMs,
                                            activeFlashCycleMs);
        st.flashLastCycleMs = activeFlashCycleMs;
        st.flashElapsedMs += MasterTimer::tick();
        st.flashWaveProgress = qMin(1.0, double(st.flashElapsedMs)
                / double(activeFlashCycleMs));

        if (st.flashPhase == PTFlashPhase::WaveIn)
        {
            if (st.flashWaveProgress >= 1.0)
            {
                st.flashPhase = PTFlashPhase::Hold;
                st.flashWaveProgress = 0.0;
                st.flashElapsedMs = 0;
                st.flashLastCycleMs = 0;
            }
        }
        else if (st.flashPhase == PTFlashPhase::WaveOut)
        {
            if (st.flashWaveProgress >= 1.0)
            {
                st.flashActive = false;
                st.flashPhase = PTFlashPhase::Idle;
                st.flashWaveProgress = 0.0;
                st.flashReleaseProgress = 1.0;
                st.flashElapsedMs = 0;
                st.flashLastCycleMs = 0;
                st.flashSourceWidgetId = 0;
                st.flashToken = 0;
                st.flashRow = -1;
                st.flashReturnRow = -1;
                st.flashValues.clear();
                st.flashReturnValues.clear();
            }
        }
    }
    const quint32 fadeMs = st.flashActive ? qMax(quint32(1), activeFlashPreset.fadeMs) : 0;

    const double sweepTimedProgress01 = (st.sweepRunning && !st.sweepManualCrossfade && cycleMs > 0)
            ? qMin(1.0, double(st.sweepElapsedMs) / double(cycleMs * 2))
            : 0.0;

    int sweepScopeCount = 0;
    int sweepDoneCount = 0;
    QSet<quint32> writtenFixtures;

    for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
    {
        const QLCPoint& pt = it.key();
        if (!outputScopeAllowsPoint(out.scope, pt, out))
            continue;

        const GroupHead& head = it.value();
        Fixture* fxi = m_doc->fixture(head.fxi);
        if (!fxi)
            continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size())
            continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        auto applyRow = [&](const QVector<uchar>& rowVals) {
            if (writtenFixtures.contains(head.fxi))
                return;
            QVector<uchar> vals = PTParamMatrixEngine::blendWithIntensity(rowVals, global.intensity);
            applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, fadeMs);
            writtenFixtures.insert(head.fxi);
        };

        auto continuousValuesAtPoint = [&]() -> QVector<uchar> {
            const PTTransitionPreset pointPreset = presetForPoint(pt);
            const float dimmer = (pointPreset.axis == preset.axis
                    && pointPreset.offsetDirection == preset.offsetDirection
                    && pointPreset.offsetStep == preset.offsetStep
                    && pointPreset.wings == preset.wings
                    && pointPreset.blocks == preset.blocks
                    && pointPreset.wingsSymmetry == preset.wingsSymmetry
                    && pointPreset.propagation == preset.propagation)
                    ? dimmerAtPoint(pt, elapsedMs)
                    : matrixDimmerAtPoint(pt, elapsedMs,
                                          qMax(quint32(1), cycleDurationMsLocked(global, pointPreset)),
                                          pointPreset, global, gridSize,
                                          spatialPlan.indexByPoint.value(pt, 0), serialCount);
            QVector<uchar> finalValues;
            if (morphOutput)
            {
                const PTTransitionPreset pointStagedPreset = stagedPresetForPoint(pt);
                const float stagedDimmer = (pointStagedPreset.axis == stagedPreset.axis
                        && pointStagedPreset.offsetDirection == stagedPreset.offsetDirection
                        && pointStagedPreset.offsetStep == stagedPreset.offsetStep
                        && pointStagedPreset.wings == stagedPreset.wings
                        && pointStagedPreset.blocks == stagedPreset.blocks
                        && pointStagedPreset.wingsSymmetry == stagedPreset.wingsSymmetry
                        && pointStagedPreset.propagation == stagedPreset.propagation)
                        ? stagedDimmerAtPoint(pt, elapsedMs)
                        : matrixDimmerAtPoint(pt, elapsedMs,
                                              qMax(quint32(1), cycleDurationMsLocked(global, pointStagedPreset)),
                                              pointStagedPreset, global, gridSize,
                                              stagedSpatialPlan.indexByPoint.value(pt, 0),
                                              stagedSerialCount);
                const QVector<uchar> liveValues = continuousOutputValues(
                        m_columns, priVals, secVals, pointPreset, double(dimmer),
                        global.intensity);
                const QVector<uchar> stagedValues = continuousOutputValues(
                        m_columns, *stagedPrimaryOverride, *stagedSecondaryOverride,
                        pointStagedPreset, double(stagedDimmer), global.intensity);
                finalValues = blendRowValues(liveValues, stagedValues, morphProgress);
            }
            else
            {
                finalValues = continuousOutputValues(
                        m_columns, priVals, secVals, pointPreset, double(dimmer),
                        global.intensity);
            }
            if (mixMultiFx)
            {
                const PTTransitionPreset pointMultiFxPreset = multiFxPresetForPoint(pt, false);
                const float multiFxDimmer = multiFxDimmerAtPoint(pt, multiFxElapsedMs);
                const QVector<uchar> multiValues = pointMultiFxPreset.enabled
                        ? continuousColumnValues(
                            m_columns, priVals, secVals,
                            double(multiFxDimmer), pointMultiFxPreset.waveShape,
                            pointMultiFxPreset.waveFadeIn, pointMultiFxPreset.waveFadeOut,
                            global.intensity)
                        : QVector<uchar>(m_columns.size(), 0);
                QVector<uchar> effectiveMultiValues = multiValues;
                if (hasStagedMultiFx)
                {
                    const PTTransitionPreset pointStagedMultiFxPreset =
                            multiFxPresetForPoint(pt, true);
                    const float stagedMultiFxDimmer = stagedMultiFxDimmerAtPoint(
                            pt, stagedMultiFxElapsedMs);
                    const QVector<uchar>& stagedPri = (morphOutput && stagedPrimaryOverride)
                            ? *stagedPrimaryOverride : priVals;
                    const QVector<uchar>& stagedSec = (morphOutput && stagedSecondaryOverride)
                            ? *stagedSecondaryOverride : secVals;
                    const QVector<uchar> stagedMultiValues = pointStagedMultiFxPreset.enabled
                            ? continuousColumnValues(
                                m_columns, stagedPri, stagedSec,
                                double(stagedMultiFxDimmer), pointStagedMultiFxPreset.waveShape,
                                pointStagedMultiFxPreset.waveFadeIn,
                                pointStagedMultiFxPreset.waveFadeOut,
                                global.intensity)
                            : QVector<uchar>(m_columns.size(), 0);
                    effectiveMultiValues = blendRowValues(multiValues, stagedMultiValues,
                                                           morphProgress);
                }
                finalValues = blendRowValues(finalValues, effectiveMultiValues,
                                             double(m_multiFxBlend) / 255.0);
            }
            return finalValues;
        };

        auto applyContinuous = [&]() {
            if (writtenFixtures.contains(head.fxi))
                return;
            const QVector<uchar> finalValues = continuousValuesAtPoint();
            applyPointChannels(fader.data(), universes[uni], head, fxi, pt, finalValues, 0);
            writtenFixtures.insert(head.fxi);
        };

        if (st.flashActive)
        {
            if (st.flashPhase == PTFlashPhase::Hold)
                applyRow(st.flashValues);
            else if (st.flashPhase == PTFlashPhase::WaveIn)
            {
                const float blend = flashSpatialPlan.sweepBlend01(
                        st.flashWaveProgress, pt, activeFlashPreset, global);
                const QVector<uchar> fromValues = continuousFx
                        ? continuousValuesAtPoint()
                        : PTParamMatrixEngine::blendWithIntensity(
                            st.flashReturnValues.isEmpty() ? priVals : st.flashReturnValues,
                            global.intensity);
                const QVector<uchar> toValues =
                        PTParamMatrixEngine::blendWithIntensity(st.flashValues,
                                                                global.intensity);
                const QVector<uchar> finalValues = blendRowValues(fromValues, toValues, blend);
                applyPointChannels(fader.data(), universes[uni], head, fxi, pt, finalValues,
                                   fadeMs);
                writtenFixtures.insert(head.fxi);
            }
            else
            {
                const float waveOutBlend = flashSpatialPlan.sweepBlend01(
                        st.flashWaveProgress, pt, activeFlashPreset, global);
                const float peakBlend = flashSpatialPlan.sweepBlend01(
                        st.flashReleaseProgress, pt, activeFlashPreset, global);
                const float releaseBlend = peakBlend * (1.0f - waveOutBlend);
                const QVector<uchar> fromValues = continuousFx
                        ? continuousValuesAtPoint()
                        : PTParamMatrixEngine::blendWithIntensity(
                            st.flashReturnValues.isEmpty() ? priVals : st.flashReturnValues,
                            global.intensity);
                const QVector<uchar> toValues =
                        PTParamMatrixEngine::blendWithIntensity(st.flashValues,
                                                                global.intensity);
                const QVector<uchar> finalValues = blendRowValues(fromValues, toValues,
                                                                  releaseBlend);
                applyPointChannels(fader.data(), universes[uni], head, fxi, pt, finalValues,
                                   fadeMs);
                writtenFixtures.insert(head.fxi);
            }
        }
        else if (st.sweepRunning && st.sweepManualCrossfade)
        {
            if (st.sweepManualPhase + 0.002 < st.sweepManualPhasePrev)
            {
                st.sweepPeakDimmer.clear();
                st.sweepHeldValues.clear();
            }
            st.sweepManualPhasePrev = st.sweepManualPhase;

            if (!writtenFixtures.contains(head.fxi))
            {
                const float blend = spatialPlan.sweepBlend01(
                        st.sweepManualPhase, pt, preset, global);
                const QVector<uchar> vals = applySweepBlend(priVals, secVals, blend);
                applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, 0);
                writtenFixtures.insert(head.fxi);
            }
        }
        else if (st.sweepRunning)
        {
            ++sweepScopeCount;
            const float blend = spatialPlan.sweepBlend01(
                    sweepTimedProgress01, pt, preset, global);
            if (!writtenFixtures.contains(head.fxi))
            {
                const QVector<uchar> vals = applySweepBlend(priVals, secVals, blend);
                applyPointChannels(fader.data(), universes[uni], head, fxi, pt, vals, 0);
                writtenFixtures.insert(head.fxi);
            }
            if (blend >= 0.99f)
                ++sweepDoneCount;
        }
        else if (continuousFx)
        {
            applyContinuous();
        }
        else
        {
            applyRow(priVals);
        }
    }

    if (st.sweepRunning && !st.sweepManualCrossfade)
    {
        st.sweepProgress = sweepTimedProgress01;
        const bool allDone = (sweepScopeCount > 0 && sweepDoneCount >= sweepScopeCount);
        const bool timedOut = st.sweepElapsedMs >= cycleMs * 2;
        if (allDone || timedOut || sweepTimedProgress01 >= 1.0)
        {
            st.sweepRunning = false;
            st.sweepProgress = 0.0;
            st.appliedRow = st.sweepToRow;
            st.sweepPeakDimmer.clear();
            st.sweepHeldValues.clear();
        }
    }
    else if (!st.flashActive && activeRow >= 0 && !st.sweepManualCrossfade)
    {
        st.appliedRow = activeRow;
    }
}

void PresetTableV2Widget::writeDMXFixtureGroup(MasterTimer* timer, QList<Universe*>& universes,
                                                uchar xfEffective)
{
    if (m_mode == PTMode::Position)
    {
        writeDMXPositionFixtureGroup(timer, universes, xfEffective);
        return;
    }

    FixtureGroup* grp = m_doc->fixtureGroup(m_fixtureGroupId);
    if (!grp) return;

    const FixtureGroupMask docMask = m_doc->fixtureGroupMask(m_fixtureGroupId);
    const QMap<QLCPoint, GroupHead> maskedHeads = m_doc->effectiveHeadsMap(grp);
    const QMap<QLCPoint, GroupHead> fullHeads = grp->headsMap();

    const bool spatialOn = m_spatialEffects.enabled;
    const QSize gridSize = grp->size();

    if (m_spatialAppliedRow.size() != m_outputs.size())
        m_spatialAppliedRow.resize(m_outputs.size());
    if (m_spatialChase.size() != m_outputs.size())
        m_spatialChase.resize(m_outputs.size());
    if (m_matrixState.size() < m_outputs.size())
        m_matrixState.resize(m_outputs.size());
    if (m_flashInputHeldRow.size() != m_outputs.size())
        m_flashInputHeldRow.fill(-1, m_outputs.size());

    if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
    {
        m_cachedTransitionSweepCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
        m_cachedTransitionContinuousCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
        m_cachedTransitionMultiFxCount = provider->transitionPresetCount(PTTransitionMode::MultiFx);
    }

    const bool matrixReady = matrixProviderReadyLocked();
    const PTGlobalEffectSettings globalFx = globalEffectSettingsLocked();

    for (int o = 0; o < m_outputs.size(); ++o)
    {
        const int activeRow = (o < m_activeRow.size()) ? m_activeRow[o] : -1;
        const bool stagedPrimaryValid = o < m_stagedRowValid.size() && m_stagedRowValid[o];
        const int stagedRow = (stagedPrimaryValid && o < m_stagedRow.size()) ? m_stagedRow[o] : -1;

        const bool activeRowValid = activeRow >= 0 && activeRow < m_rows.size();
        const bool stagedRowValid = stagedRow >= 0 && stagedRow < m_rows.size();
        if (!activeRowValid && !stagedRowValid) continue;

        const PTOutput& out = m_outputs[o];
        if (out.scope == PTOutputScope::Mask && !docMask.isActive())
            continue;

        const QVector<uchar> offVals(m_columns.size(), uchar(0));
        const QVector<uchar>& aVals = activeRowValid ? m_rows[activeRow].values : offVals;
        const QVector<uchar>* bVals = stagedRowValid
            ? &m_rows[stagedRow].values : (stagedPrimaryValid ? &offVals : nullptr);

        const QMap<QLCPoint, GroupHead>& headsMap =
                (out.scope == PTOutputScope::Rows) ? fullHeads : maskedHeads;

        const bool hasStaged = stagedPrimaryValid && stagedRow != activeRow;
        bool sweepOn = false;
        bool contOn = false;
        int secRow = -1;
        bool crossfadeSweep = false;
        bool crossfadeCont = false;
        bool blockMatrixForStaged = false;
        bool matrixForOutput = false;
        if (activeRowValid)
        {
            const PTOutputPlaybackState playback = resolveOutputPlaybackStateLocked(
                    o, activeRow, hasStaged, matrixReady, spatialOn);
            sweepOn = playback.transitionOn;
            contOn = playback.continuousFxOn;
            secRow = playback.secondaryRow;
            crossfadeSweep = playback.crossfadeTransition;
            crossfadeCont = playback.crossfadeContinuous;
            blockMatrixForStaged = playback.blockMatrixForStaged;
            matrixForOutput = playback.matrixForOutput;
        }

        if (matrixForOutput && !blockMatrixForStaged)
        {
            ensureMatrixState(o);
            PTOutputMatrixState& st = m_matrixState[o];

            if (contOn || (multiFxActiveForOutputLocked(o) && m_multiFxBlend > 0))
            {
                const PTContinuousLayerState layer =
                        continuousLayerStateForOutputLocked(o, activeRow, xfEffective);
                if (layer.active)
                {
                    st.sweepRunning = false;
                    st.sweepManualCrossfade = false;
                    writeMatrixSpatial(o, timer, universes, out, activeRow, secRow,
                                       layer.livePreset, globalFx, gridSize, headsMap, true,
                                       &layer.livePrimaryValues, &layer.liveSecondaryValues,
                                       layer.hasStaged ? &layer.primaryValues : nullptr,
                                       layer.hasStaged ? &layer.secondaryValues : nullptr,
                                       layer.hasStaged ? &layer.preset : nullptr,
                                       crossfadeProgress01Locked(xfEffective),
                                       multiFxActiveForOutputLocked(o));
                    continue;
                }
            }

            if (crossfadeSweep && stagedRow >= 0 && stagedRow != activeRow)
            {
                PTTransitionPreset sweepPreset = sweepPresetForOutputLocked(o);
                if (sweepPreset.enabled)
                {
                    if (!st.sweepRunning
                            || st.sweepFromRow != activeRow || st.sweepToRow != stagedRow)
                        beginMatrixSweepLocked(o, activeRow, stagedRow, true);
                    st.sweepManualCrossfade = true;
                    st.sweepManualPhase = crossfadeProgress01Locked(xfEffective);
                    writeMatrixSpatial(o, timer, universes, out, activeRow, activeRow,
                                       sweepPreset, globalFx, gridSize, headsMap);
                    continue;
                }
            }

            st.sweepManualCrossfade = false;

            if (sweepOnPrimaryChangeLocked(o, activeRow) && !st.flashActive && !st.sweepRunning
                    && activeRow >= 0 && activeRow != st.appliedRow)
                beginMatrixSweepLocked(o, st.appliedRow, activeRow);

            if (sweepOn && st.sweepRunning)
            {
                PTTransitionPreset sweepPreset = sweepPresetForOutputLocked(o);
                if (sweepPreset.enabled)
                {
                    writeMatrixSpatial(o, timer, universes, out, activeRow, activeRow,
                                       sweepPreset, globalFx, gridSize, headsMap);
                    continue;
                }
            }

            if (sweepOn)
            {
                PTTransitionPreset sweepPreset = sweepPresetForOutputLocked(o);
                if (sweepPreset.enabled)
                {
                    writeMatrixSpatial(o, timer, universes, out, activeRow, activeRow,
                                       sweepPreset, globalFx, gridSize, headsMap);
                    continue;
                }
            }
        }

        if (crossfadeSweep && stagedRow >= 0 && stagedRow != activeRow)
        {
            const QList<PTOutputScopeFixture> scopeFixtures =
                    collectOutputScopeFixtures(headsMap, out);
            for (const PTOutputScopeFixture& sf : scopeFixtures)
            {
                Fixture* fxi = m_doc->fixture(sf.fxiId);
                if (!fxi)
                    continue;
                quint32 uni = fxi->universe();
                if ((int)uni >= universes.size())
                    continue;
                auto fader = m_faders.value(uni);
                if (fader.isNull())
                {
                    fader = universes[uni]->requestFader(Universe::Auto);
                    m_faders.insert(uni, fader);
                }
                QVector<uchar> vals = aVals;
                vals = PTParamMatrixEngine::blendWithIntensity(vals, globalFx.intensity);
                applyPointChannels(fader.data(), universes[uni], sf.head, fxi, sf.point, vals, 0);
            }
            continue;
        }

        const bool sweepActive = sweepOn;
        const bool contActive = contOn || (multiFxActiveForOutputLocked(o) && m_multiFxBlend > 0);

        const bool useContinuous = spatialOn && contActive
                && !blockMatrixForStaged
                && !matrixForOutput;

        if (useContinuous)
        {
            const PTContinuousLayerState layer =
                    continuousLayerStateForOutputLocked(o, activeRow, xfEffective);
            if (layer.active)
            {
                writeContinuousSpatial(o, timer, universes, out,
                                       layer.livePrimaryValues, layer.liveSecondaryValues,
                                       gridSize, headsMap, &layer.livePreset,
                                       layer.hasStaged ? &layer.primaryValues : nullptr,
                                       layer.hasStaged ? &layer.secondaryValues : nullptr,
                                       layer.hasStaged ? &layer.preset : nullptr,
                                       crossfadeProgress01Locked(xfEffective),
                                       multiFxActiveForOutputLocked(o));
            }
            continue;
        }

        PTTransitionPreset transPreset = transitionPresetForOutputLocked(o);
        const bool efxActive = sweepActive || contActive;
        const bool useSpatial = spatialOn && efxActive && !blockMatrixForStaged
                && transPreset.enabled && !matrixForOutput;
        const int appliedRow = (o < m_spatialAppliedRow.size()) ? m_spatialAppliedRow[o] : -1;

        if (useSpatial)
        {
            PTSpatialChaseOutput& chase = m_spatialChase[o];
            if (activeRow != appliedRow && (!chase.active || chase.targetRow != activeRow))
            {
                QList<QLCPoint> points;
                for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
                {
                    if (outputScopeAllowsPoint(out.scope, it.key(), out))
                        points.append(it.key());
                }
                startSpatialChase(o, activeRow, points, transPreset,
                                  gridSize.width(), gridSize.height());
            }

            if (m_spatialChase[o].active)
            {
                tickSpatialChase(o, timer, universes, out, aVals);
                continue;
            }
        }

        const QList<PTOutputScopeFixture> scopeFixtures =
                collectOutputScopeFixtures(headsMap, out);
        for (const PTOutputScopeFixture& sf : scopeFixtures)
        {
            Fixture* fxi = m_doc->fixture(sf.fxiId);
            if (!fxi)
                continue;

            QLCFixtureDef*  fxDef  = fxi->fixtureDef();
            QLCFixtureMode* fxMode = fxi->fixtureMode();
            if (!fxDef || !fxMode)
                continue;

            quint32 uni = fxi->universe();
            if ((int)uni >= universes.size())
                continue;

            auto fader = m_faders.value(uni);
            if (fader.isNull())
            {
                fader = universes[uni]->requestFader(Universe::Auto);
                m_faders.insert(uni, fader);
            }

            for (int c = 0; c < m_columns.size(); ++c)
            {
                const PTColumn& col = m_columns[c];
                uchar aVal = (c < aVals.size()) ? aVals[c] : 0;
                uchar bVal = (bVals && c < bVals->size()) ? (*bVals)[c] : aVal;
                const bool linearCrossfade = m_crossfadeEnabled && hasStaged
                        && !crossfadeSweep;

                for (const PTColumnTypeBinding& binding : col.bindings)
                {
                    if (!bindingMatchesFixture(binding, fxi))
                        continue;

                    const quint32 absChannel = quint32(binding.channelIndex);
                    applyFadeValue(fader.data(), m_doc, universes[uni],
                                   sf.head.fxi, absChannel,
                                   aVal, bVal,
                                   linearCrossfade, linearCrossfade && hasStaged,
                                   col.fade, xfEffective);
                }
            }
        }

        if (useSpatial && o < m_spatialAppliedRow.size() && !m_spatialChase[o].active)
            m_spatialAppliedRow[o] = activeRow;
    }
}

// ==========================================================================
// writeDMX (MasterTimer thread)
// ==========================================================================

void PresetTableV2Widget::slotFixtureGroupMaskChanged(quint32 groupId)
{
    if ((m_mode != PTMode::FixtureGroup && m_mode != PTMode::Position)
            || m_fixtureGroupId != groupId)
        return;

    {
        QMutexLocker lk(&m_stateMutex);
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
    }
    refreshRowHighlights();
}

void PresetTableV2Widget::writeDMX(MasterTimer* timer, QList<Universe*> universes)
{
    bool refreshPositionUi = false;
    {
    QMutexLocker lk(&m_stateMutex);

    if (m_crossfadeEnabled && timer)
        tickCrossfadeClockLocked(timer);
    if (m_crossfadeEnabled && timer)
        syncMultiFxPhaseOnCrossfadeMotionLocked();
    if (timer)
    {
        while (m_multiFxElapsedMs.size() < m_outputs.size())
            m_multiFxElapsedMs.append(0);
        while (m_multiFxStagedElapsedMs.size() < m_outputs.size())
            m_multiFxStagedElapsedMs.append(0);
        while (m_multiFxLastCycleMs.size() < m_outputs.size())
            m_multiFxLastCycleMs.append(0);
        while (m_multiFxStagedLastCycleMs.size() < m_outputs.size())
            m_multiFxStagedLastCycleMs.append(0);
        const PTGlobalEffectSettings global = globalEffectSettingsLocked();
        const bool holdStagedMultiFxClock = m_syncMultiFxPhaseToCrossfade
                && m_crossfadeEnabled
                && hasStagedMultiFxAnyLocked()
                && !m_multiFxXfPhaseAnchored;
        for (int o = 0; o < m_outputs.size(); ++o)
        {
            if (multiFxActiveForOutputLocked(o))
            {
                const PTTransitionPreset preset = multiFxPresetForOutputLocked(o);
                const quint32 cycleMs = qMax(quint32(1), cycleDurationMsLocked(global, preset));
                ensurePhaseStableCycleLocked(m_multiFxElapsedMs, m_multiFxLastCycleMs, o, cycleMs);
                m_multiFxElapsedMs[o] += MasterTimer::tick();
                // Match native EFXFixture wrap (strict >, reset to 0) so the loop
                // period equals the native EFX (loopDuration + 1 tick) and stays
                // frame-locked to a cue-list EFX of the same duration.
                if (m_multiFxElapsedMs[o] > cycleMs)
                    m_multiFxElapsedMs[o] = 0;
            }
            const int stagedMultiFxIdx = stagedMultiFxPresetIndexLocked(o);
            if (hasStagedMultiFxPresetLocked(o))
            {
                const PTTransitionPreset stagedPreset =
                        transitionPresetAtIndexLocked(PTTransitionMode::MultiFx, stagedMultiFxIdx, o);
                const quint32 stagedCycleMs = qMax(quint32(1),
                        cycleDurationMsLocked(global, stagedPreset));
                ensurePhaseStableCycleLocked(m_multiFxStagedElapsedMs, m_multiFxStagedLastCycleMs,
                                             o, stagedCycleMs);
                if (holdStagedMultiFxClock)
                {
                    m_multiFxStagedElapsedMs[o] = 0;
                }
                else
                {
                    m_multiFxStagedElapsedMs[o] += MasterTimer::tick();
                    if (m_multiFxStagedElapsedMs[o] > stagedCycleMs)
                        m_multiFxStagedElapsedMs[o] = 0;
                }
            }
        }
    }

    const uchar xfPos      = m_crossfadeGlobalPos;
    const uchar xfStartPos = m_crossfadeStartPos;
    const uchar xfEffective = crossfadeEffectiveLocked(xfPos, xfStartPos);

    if (m_mode == PTMode::FixtureGroup || m_mode == PTMode::Position)
        writeDMXFixtureGroup(timer, universes, xfEffective);
    else
        writeDMXLegacy(universes, xfEffective);

    if (m_positionPromoteUiRefresh)
    {
        refreshPositionUi = true;
        m_positionPromoteUiRefresh = false;
    }

    }

    if (refreshPositionUi)
        refreshOperatePositionChrome();
}

// ==========================================================================
// External input
// ==========================================================================

void PresetTableV2Widget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (!acceptsInput()) return;

    quint32 pagedCh = (page() << 16) | channel;

    QMutexLocker lk(&m_stateMutex);
    int numOutputs        = m_outputs.size();
    int numRows           = m_rows.size();
    bool xfEnabled        = m_crossfadeEnabled;
    bool initialSync      = m_initialInputSyncPending;
    lk.unlock();

    if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::kMultiFxBlend))
    {
        QMutexLocker lk2(&m_stateMutex);
        m_multiFxBlend = value;
        return;
    }

    if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::kMultiFxRestart))
    {
        if (value > 0)
        {
            QMutexLocker lk2(&m_stateMutex);
            m_multiFxElapsedMs.fill(0, m_outputs.size());
            m_multiFxStagedElapsedMs.fill(0, m_outputs.size());
            m_multiFxLastCycleMs.fill(0, m_outputs.size());
            m_multiFxStagedLastCycleMs.fill(0, m_outputs.size());
            resetMultiFxCrossfadePhaseAnchorLocked();
        }
        return;
    }

    if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::kWidgetFlashGate))
    {
        const bool active = value > 0;
        QMutexLocker lk2(&m_stateMutex);
        setWidgetFlashGateActiveLocked(active, value);
        return;
    }

    // Global crossfade position
    if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::kCrossfade))
    {
        QMutexLocker lk2(&m_stateMutex);
        m_crossfadeGlobalPos = value;

        if (initialSync)
        {
            const bool atEdge = crossfadeAtLowEdge(value) || crossfadeAtHighEdge(value);
            m_crossfadeStartPos = atEdge ? crossfadeNormalizedEdge(value) : value;
            m_crossfadeStagedAtLowSide = crossfadeLowSideFromPosition(value);
            m_crossfadeEditLaneStaged = true;
            m_crossfadeSessionActive = false;
            resetCrossfadeClockLocked();
        }
        else if (m_crossfadeEnabled && crossfadeManualControlEnabledLocked()
                && crossfadeAtTargetEdge(value, m_crossfadeStagedAtLowSide)
                && crossfadeHasStagedChangesLocked())
        {
            const uchar edge = crossfadeNormalizedEdge(value);
            promoteStagedToLiveLocked();
            m_crossfadeEditLaneStaged = true;
            m_crossfadeStagedAtLowSide = !m_crossfadeStagedAtLowSide;
            m_crossfadeStartPos = edge;
            resetCrossfadeClockLocked();
            resetMultiFxCrossfadePhaseAnchorLocked();
        }
        else if (m_crossfadeEnabled && crossfadeManualControlEnabledLocked()
                 && (crossfadeAtLowEdge(value) || crossfadeAtHighEdge(value))
                 && !crossfadeHasStagedChangesLocked())
        {
            const uchar edge = crossfadeNormalizedEdge(value);
            m_crossfadeEditLaneStaged = true;
            m_crossfadeStagedAtLowSide = (edge == 0);
            m_crossfadeStartPos = edge;
            m_crossfadeSessionActive = false;
            resetCrossfadeClockLocked();
            resetMultiFxCrossfadePhaseAnchorLocked();
        }

        syncMultiFxPhaseOnCrossfadeMotionLocked();
        m_crossfadePrevPos = value;
        lk2.unlock();
        refreshRowHighlights();
        flushPositionPromoteUiIfNeeded();
        return;
    }

    // Per-output row selector (ID = o)
    for (int o = 0; o < numOutputs; ++o)
    {
        if (o >= PTInputId::kMaxRoutableOutputs)
            break;

        if (checkInputSource(universe, pagedCh, value, sender(), quint8(o)))
        {
            const bool wasFlash = (o < m_flashInputHeldRow.size() && m_flashInputHeldRow[o] >= 0);
            const bool isFlash = (value >= 101);

            if (isFlash)
            {
                const int flashRow = int(value) - 101;
                if (flashRow >= 0 && flashRow < numRows)
                {
                    int effectIdx = 0;
                    {
                        QMutexLocker lk2(&m_stateMutex);
                        if (sweepEfxActiveForOutputLocked(o))
                            effectIdx = liveSweepPresetIndexLocked(o);
                        else if (continuousEfxActiveForOutputLocked(o))
                            effectIdx = liveContinuousPresetIndexLocked(o);
                        if (o < m_flashInputHeldRow.size())
                            m_flashInputHeldRow[o] = flashRow;
                    }
                    requestTableFlash(flashRow, effectIdx >= 0 ? effectIdx : 0);
                }
                refreshRowHighlights();
                return;
            }

            if (wasFlash)
            {
                QMutexLocker lk2(&m_stateMutex);
                if (o < m_flashInputHeldRow.size())
                    m_flashInputHeldRow[o] = -1;
                releaseMatrixFlashLocked(o);
            }

            int rowIdx = (value == 0) ? -1 : qMin<int>(int(value) - 1, numRows - 1);
            if (initialSync)
            {
                QMutexLocker lk2(&m_stateMutex);
                if (o < m_activeRow.size())
                {
                    m_activeRow[o] = rowIdx;
                    if (o < m_stagedRow.size())
                        m_stagedRow[o] = -1;
                    if (o < m_stagedRowValid.size())
                        m_stagedRowValid[o] = false;
                    bumpMultiButtonStateRevisionLocked(
                            o, PresetTableV2MultiButtonTargetIface::PrimaryRow);
                    syncCommittedPlaybackStateLocked(o, false);
                }
                lk2.unlock();
                refreshRowHighlights();
                sendFeedback(value, PTInputId::rowSelector(o));
            }
            else if (xfEnabled)
            {
                bool routeToStaged = false;
                {
                    QMutexLocker lk2(&m_stateMutex);
                    routeToStaged = crossfadeRoutesToStagedLocked();
                    if (!routeToStaged && o < m_stagedRow.size())
                        m_stagedRow[o] = -1;
                    if (!routeToStaged && o < m_stagedRowValid.size())
                        m_stagedRowValid[o] = false;
                }
                if (!routeToStaged)
                {
                    setActiveRow(o, rowIdx);
                    refreshRowHighlights();
                    return;
                }

                // Crossfade: staged primary follows the currently armed fader side.
                QMutexLocker lk2(&m_stateMutex);
                const int prevStaged = (o < m_stagedRow.size()) ? m_stagedRow[o] : -1;
                armCrossfadeStagingLocked();
                stagePrimaryRowLocked(o, rowIdx);
                if (rowIdx != prevStaged && !crossfadeManualControlEnabledLocked())
                    resetCrossfadeClockLocked();
                lk2.unlock();
                refreshRowHighlights();
                refreshOperatePositionChrome(o);
            }
            else
            {
                // Normal mode: selector sets current row directly
                setActiveRow(o, rowIdx);
            }
            return;
        }
    }

    int sweepPresetCount = 0;
    int continuousPresetCount = 0;
    int multiFxPresetCount = 0;
    {
        QMutexLocker lk3(&m_stateMutex);
        sweepPresetCount = m_cachedTransitionSweepCount;
        continuousPresetCount = m_cachedTransitionContinuousCount;
        multiFxPresetCount = m_cachedTransitionMultiFxCount;
        if (sweepPresetCount <= 0 && continuousPresetCount <= 0 && multiFxPresetCount <= 0)
        {
            if (PresetTableV2TransitionProviderIface* provider = transitionProviderLocked())
            {
                sweepPresetCount = provider->transitionPresetCount(PTTransitionMode::SweepOnly);
                continuousPresetCount = provider->transitionPresetCount(PTTransitionMode::Continuous);
                multiFxPresetCount = provider->transitionPresetCount(PTTransitionMode::MultiFx);
            }
        }
    }

    for (int o = 0; o < numOutputs; ++o)
    {
        if (o >= PTInputId::kMaxRoutableOutputs)
            break;

        if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::transSweep(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const int prevSweep = (o < m_liveSweepPreset.size()) ? m_liveSweepPreset[o] : -1;
            const int nextSweep = PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                    value, sweepPresetCount);
            if (o < m_liveSweepPreset.size())
            {
                m_liveSweepPreset[o] = nextSweep;
                if (o < m_stagedSweepValid.size())
                    m_stagedSweepValid[o] = false;
                if (o < m_stagedSweepPreset.size())
                    m_stagedSweepPreset[o] = -1;
                bumpMultiButtonStateRevisionLocked(
                        o, PresetTableV2MultiButtonTargetIface::TransitionPreset);

                if (!initialSync && nextSweep != prevSweep)
                    syncCommittedPlaybackStateLocked(o, false);
                else if (initialSync)
                    syncCommittedPlaybackStateLocked(o, false);
            }
            lk2.unlock();
            refreshTransitionPresetCache();
            sendFeedback(value, PTInputId::transSweep(o));
            return;
        }

        if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::transContinuousBank(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const bool toStaged = !initialSync && continuousFxSelectorToStagedLocked();
            const int presetIdx = PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                    value, continuousPresetCount);
            if (toStaged)
            {
                armCrossfadeStagingLocked();
                materializeContinuousRowsLocked(o, true);
                stageContinuousPresetLocked(o, presetIdx);
                resetCrossfadeClockLocked();
            }
            else if (o < m_liveContinuousPreset.size())
            {
                m_liveContinuousPreset[o] = presetIdx;
                if (o < m_stagedContinuousValid.size())
                    m_stagedContinuousValid[o] = false;
                if (o < m_stagedContinuousPreset.size())
                    m_stagedContinuousPreset[o] = -1;
                bumpMultiButtonStateRevisionLocked(
                        o, PresetTableV2MultiButtonTargetIface::ContinuousPreset);
                materializeContinuousRowsLocked(o, false);
                if (!initialSync && o < m_continuousElapsedMs.size())
                {
                    m_continuousElapsedMs[o] = 0;
                    if (o < m_continuousLastCycleMs.size())
                        m_continuousLastCycleMs[o] = 0;
                }
            }
            lk2.unlock();
            refreshTransitionPresetCache();
            if (!toStaged)
                sendFeedback(value, PTInputId::transContinuousBank(o));
            return;
        }

        if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::transSecondaryRow(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const bool toStaged = !initialSync && continuousFxSelectorToStagedLocked();
            const int rowIdx = PresetTableV2SpatialEngine::tableRowIndexFromInput(value, numRows);
            if (toStaged)
            {
                armCrossfadeStagingLocked();
                stageSecondaryRowLocked(o, rowIdx);
            }
            else
            {
                while (m_liveSecondaryRow.size() <= o)
                    m_liveSecondaryRow.append(-1);
                m_liveSecondaryRow[o] = rowIdx;
                if (o < m_stagedSecondaryValid.size())
                    m_stagedSecondaryValid[o] = false;
                if (o < m_stagedSecondaryRow.size())
                    m_stagedSecondaryRow[o] = -1;
                bumpMultiButtonStateRevisionLocked(
                        o, PresetTableV2MultiButtonTargetIface::SecondaryRow);
            }
            lk2.unlock();
            refreshTransitionPresetCache();
            if (!toStaged)
                sendFeedback(value, PTInputId::transSecondaryRow(o));
            return;
        }

        if (checkInputSource(universe, pagedCh, value, sender(), PTInputId::multiFxBank(o)))
        {
            QMutexLocker lk2(&m_stateMutex);
            const bool toStaged = !initialSync && continuousFxSelectorToStagedLocked();
            const int presetIdx = PresetTableV2SpatialEngine::transitionPresetIndexFromInput(
                    value, multiFxPresetCount);
            if (toStaged)
            {
                armCrossfadeStagingLocked();
                materializeContinuousRowsLocked(o, true);
                stageMultiFxPresetLocked(o, presetIdx);
                resetCrossfadeClockLocked();
            }
            else
            {
                while (m_liveMultiFxPreset.size() <= o)
                    m_liveMultiFxPreset.append(-1);
                m_liveMultiFxPreset[o] = presetIdx;
                if (o < m_stagedMultiFxValid.size())
                    m_stagedMultiFxValid[o] = false;
                if (o < m_stagedMultiFxPreset.size())
                    m_stagedMultiFxPreset[o] = -1;
                bumpMultiButtonStateRevisionLocked(
                        o, PresetTableV2MultiButtonTargetIface::MultiFxPreset);
            }
            lk2.unlock();
            if (!toStaged)
                sendFeedback(value, PTInputId::multiFxBank(o));
            return;
        }
    }
}

void PresetTableV2Widget::slotKeyPressed(const QKeySequence& keySequence)
{
    if (!acceptsInput())
        return;

    const QKeySequence key = stripKeySequence(keySequence);
    if (key.isEmpty())
        return;

    if (!m_widgetFlashGateKey.isEmpty()
            && stripKeySequence(m_widgetFlashGateKey) == key)
    {
        QMutexLocker lk(&m_stateMutex);
        setWidgetFlashGateActiveLocked(true, 255);
        return;
    }

    if (!m_multiFxRestartKey.isEmpty()
            && stripKeySequence(m_multiFxRestartKey) == key)
    {
        QMutexLocker lk(&m_stateMutex);
        m_multiFxElapsedMs.fill(0, m_outputs.size());
        m_multiFxStagedElapsedMs.fill(0, m_outputs.size());
        m_multiFxLastCycleMs.fill(0, m_outputs.size());
        m_multiFxStagedLastCycleMs.fill(0, m_outputs.size());
        resetMultiFxCrossfadePhaseAnchorLocked();
        return;
    }
}

void PresetTableV2Widget::slotKeyReleased(const QKeySequence& keySequence)
{
    if (!acceptsInput())
        return;

    const QKeySequence key = stripKeySequence(keySequence);
    if (key.isEmpty() || m_widgetFlashGateKey.isEmpty()
            || stripKeySequence(m_widgetFlashGateKey) != key)
        return;

    QMutexLocker lk(&m_stateMutex);
    setWidgetFlashGateActiveLocked(false, 0);
}

void PresetTableV2Widget::updateFeedback()
{
    QMutexLocker lk(&m_stateMutex);
    for (int o = 0; o < m_activeRow.size(); ++o)
    {
        sendLiveSelectorFeedbackLocked(o);
    }
    sendFeedback(m_multiFxBlend, PTInputId::kMultiFxBlend);
}

// ==========================================================================
// Properties dialog
// ==========================================================================

void PresetTableV2Widget::editProperties()
{
    syncAllDataFromTable();

    QVector<PTColumn> colsCopy;
    QVector<PTOutput>  outsCopy;
    PTMode   modeCopy;
    quint32  groupIdCopy;
    {
        QMutexLocker lk(&m_stateMutex);
        colsCopy    = m_columns;
        outsCopy    = m_outputs;
        modeCopy    = m_mode;
        groupIdCopy = m_fixtureGroupId;
    }

    // Collect current input sources per output
    QVector<QSharedPointer<QLCInputSource>> srcsCopy;
    for (int o = 0; o < outsCopy.size(); ++o)
        srcsCopy.append(o < PTInputId::kMaxRoutableOutputs
                        ? inputSource(PTInputId::rowSelector(o))
                        : QSharedPointer<QLCInputSource>());

    bool xfEnabled;
    bool syncMultiFxPhase;
    int multiFxSyncOffsetMs;
    PTContinuousFxSelectorMode contFxSelectorMode;
    PTSpatialEffectSettings spatialCopy;
    quint32 linkedTransitionId;
    QSharedPointer<QLCInputSource> xfSrc;
    QSharedPointer<QLCInputSource> multiFxBlendSrc;
    QSharedPointer<QLCInputSource> multiFxRestartSrc;
    QSharedPointer<QLCInputSource> widgetFlashGateSrc;
    QKeySequence multiFxRestartKey;
    QKeySequence widgetFlashGateKey;
    int widgetFlashTimeMultiplierIndex;
    PTWidgetFlashBehavior widgetFlashBehavior;
    bool positionConfirmDiscardDraft;
    bool positionShowStatusStrip;
    bool positionShowEditorHints;
    {
        QMutexLocker lk(&m_stateMutex);
        xfEnabled = m_crossfadeEnabled;
        syncMultiFxPhase = m_syncMultiFxPhaseToCrossfade;
        multiFxSyncOffsetMs = m_multiFxCrossfadeSyncOffsetMs;
        contFxSelectorMode = m_continuousFxSelectorMode;
        spatialCopy = m_spatialEffects;
        linkedTransitionId = m_linkedTransitionWidgetId;
        multiFxRestartKey = m_multiFxRestartKey;
        widgetFlashGateKey = m_widgetFlashGateKey;
        widgetFlashTimeMultiplierIndex = m_widgetFlashTimeMultiplierIndex;
        widgetFlashBehavior = m_widgetFlashBehavior;
        positionConfirmDiscardDraft = m_positionConfirmDiscardDraft;
        positionShowStatusStrip = m_positionShowStatusStrip;
        positionShowEditorHints = m_positionShowEditorHints;
    }
    xfSrc = inputSource(PTInputId::kCrossfade);
    multiFxBlendSrc = inputSource(PTInputId::kMultiFxBlend);
    multiFxRestartSrc = inputSource(PTInputId::kMultiFxRestart);
    widgetFlashGateSrc = inputSource(PTInputId::kWidgetFlashGate);

    PresetTableV2ConfigDialog dlg(m_doc, colsCopy, outsCopy, srcsCopy,
                                xfEnabled, syncMultiFxPhase, multiFxSyncOffsetMs,
                                xfSrc, multiFxBlendSrc, multiFxRestartSrc,
                                multiFxRestartKey, widgetFlashGateSrc,
                                widgetFlashGateKey, widgetFlashTimeMultiplierIndex,
                                widgetFlashBehavior, contFxSelectorMode, page(),
                                modeCopy, groupIdCopy, spatialCopy, linkedTransitionId,
                                positionConfirmDiscardDraft,
                                positionShowStatusStrip,
                                positionShowEditorHints,
                                this);

    if (dlg.exec() != QDialog::Accepted) return;

    QVector<PTColumn> newCols = dlg.columns();
    QVector<PTOutput>  newOuts = dlg.outputs();
    PTMode   newMode    = dlg.widgetMode();
    quint32  newGroupId = dlg.selectedFixtureGroupId();

    {
        QMutexLocker lk(&m_stateMutex);

        // Adjust row values if column count changed
        int oldNumCols = m_columns.size();
        int newNumCols = newCols.size();
        m_columns = newCols;
        if (newNumCols != oldNumCols)
        {
            for (PTRow& row : m_rows)
                row.values.resize(newNumCols, 0);
        }

        m_outputs = newOuts;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
        m_stagedRowValid.resize(m_outputs.size());
        m_stagedRowValid.fill(false);
        m_spatialAppliedRow.resize(m_outputs.size());
        m_spatialAppliedRow.fill(-1);

        m_mode          = newMode;
        m_fixtureGroupId = newGroupId;
        m_spatialEffects.enabled = dlg.spatialEffectsEnabled();
        m_linkedTransitionWidgetId = dlg.linkedTransitionWidgetId();
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        syncLiveTransitionFromOutputs();
    }

    refreshTransitionPresetCache();
    {
        QMutexLocker lk(&m_stateMutex);
        syncLiveTransitionFromOutputs();
    }

    // Apply new input sources
    for (int o = 0; o < newOuts.size(); ++o)
    {
        if (o >= PTInputId::kMaxRoutableOutputs)
            break;
        setInputSource(dlg.inputSource(o), PTInputId::rowSelector(o));
        if (newMode == PTMode::FixtureGroup || newMode == PTMode::Position)
        {
            setInputSource(dlg.transSweepInputSource(o), PTInputId::transSweep(o));
            setInputSource(dlg.transContinuousInputSource(o), PTInputId::transContinuousBank(o));
            setInputSource(dlg.multiFxInputSource(o), PTInputId::multiFxBank(o));
            setInputSource(dlg.transSecondaryInputSource(o), PTInputId::transSecondaryRow(o));
        }
    }

    // Global crossfade input + toggle
    setInputSource(dlg.crossfadeInputSource(), PTInputId::kCrossfade);
    setInputSource(dlg.multiFxBlendInputSource(), PTInputId::kMultiFxBlend);
    setInputSource(dlg.multiFxRestartInputSource(), PTInputId::kMultiFxRestart);
    setInputSource(dlg.widgetFlashGateInputSource(), PTInputId::kWidgetFlashGate);
    {
        QMutexLocker lk(&m_stateMutex);
        m_crossfadeEnabled = dlg.crossfadeEnabled();
        m_syncMultiFxPhaseToCrossfade = dlg.syncMultiFxPhaseToCrossfade();
        m_multiFxCrossfadeSyncOffsetMs = dlg.multiFxCrossfadeSyncOffsetMs();
        m_continuousFxSelectorMode = dlg.continuousFxSelectorMode();
        m_multiFxRestartKey = dlg.multiFxRestartKeySequence();
        m_widgetFlashGateKey = dlg.widgetFlashGateKeySequence();
        m_widgetFlashTimeMultiplierIndex =
                qBound(0, dlg.widgetFlashTimeMultiplierIndex(), kWidgetFlashTimeMultiplierMax);
        m_positionConfirmDiscardDraft = dlg.positionConfirmDiscardDraft();
        m_positionShowStatusStrip = dlg.positionShowStatusStrip();
        m_positionShowEditorHints = dlg.positionShowEditorHints();
        const PTWidgetFlashBehavior oldWidgetFlashBehavior = m_widgetFlashBehavior;
        m_widgetFlashBehavior = dlg.widgetFlashBehavior();
        if (oldWidgetFlashBehavior != m_widgetFlashBehavior && m_widgetFlashGateActive)
        {
            if (oldWidgetFlashBehavior == PTWidgetFlashBehavior::StagedRowTrigger)
                endWidgetStagedFlashLocked();
            else
            {
                for (int o = 0; o < m_matrixState.size(); ++o)
                    releaseMatrixFlashLocked(o);
            }
            if (m_widgetFlashBehavior == PTWidgetFlashBehavior::StagedRowTrigger)
                beginWidgetStagedFlashLocked();
            else
                m_widgetStagedFlashToken = 0;
        }
        if (!m_widgetFlashGateActive)
            m_widgetFlashGateLastValue = 0;
        if (!m_crossfadeEnabled)
        {
            m_stagedRow.fill(-1, m_stagedRow.size());
            m_stagedRowValid.fill(false, m_stagedRowValid.size());
            m_stagedSecondaryRow.fill(-1, m_stagedSecondaryRow.size());
            m_stagedSweepPreset.fill(-1, m_stagedSweepPreset.size());
            m_stagedContinuousPreset.fill(-1, m_stagedContinuousPreset.size());
            m_stagedMultiFxPreset.fill(-1, m_stagedMultiFxPreset.size());
            m_stagedSecondaryValid.fill(false, m_stagedSecondaryValid.size());
            m_stagedSweepValid.fill(false, m_stagedSweepValid.size());
            m_stagedContinuousValid.fill(false, m_stagedContinuousValid.size());
            m_stagedMultiFxValid.fill(false, m_stagedMultiFxValid.size());
            m_crossfadeGlobalPos = 0;
            m_crossfadeStartPos  = 0;
            m_crossfadePrevPos   = 0;
            m_crossfadeStagedAtLowSide = true;
            m_crossfadeSessionActive = false;
            m_crossfadeEditLaneStaged = true;
            resetCrossfadeClockLocked();
            resetMultiFxCrossfadePhaseAnchorLocked();
        }
    }

    rebuildTable();
    m_doc->setModified();
}

// ==========================================================================
// createCopy
// ==========================================================================

VCWidget* PresetTableV2Widget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    PresetTableV2Widget* copy = new PresetTableV2Widget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }

    QVector<PTColumn> colsCopy;
    QVector<PTRow>    rowsCopy;
    QVector<QHash<int, PTPositionOutputLayer>> positionOverridesCopy;
    QVector<PTOutput> outsCopy;
    QVector<int>      activeRowCopy;
    QVector<int>      stagedRowCopy;
    QVector<bool>     stagedRowValidCopy;
    bool              xfEnabledCopy;
    bool              syncMultiFxPhaseCopy;
    int               multiFxSyncOffsetMsCopy;
    uchar             xfPosCopy;
    uchar             xfStartPosCopy;
    bool              xfStagedAtLowSideCopy;
    bool              xfEditLaneStagedCopy;
    PTMode            modeCopy;
    quint32           groupIdCopy;
    PTSpatialEffectSettings spatialCopy;
    PTContinuousFxSelectorMode contFxSelectorModeCopy;
    quint32 linkedTransitionCopy;
    QKeySequence multiFxRestartKeyCopy;
    QKeySequence widgetFlashGateKeyCopy;
    int widgetFlashTimeMultiplierIndexCopy;
    PTWidgetFlashBehavior widgetFlashBehaviorCopy;
    bool positionConfirmDiscardDraftCopy;
    bool positionShowStatusStripCopy;
    bool positionShowEditorHintsCopy;

    {
        QMutexLocker lk(&m_stateMutex);
        colsCopy      = m_columns;
        rowsCopy      = m_rows;
        positionOverridesCopy = m_positionOverrides;
        outsCopy      = m_outputs;
        activeRowCopy = m_activeRow;
        stagedRowCopy = m_stagedRow;
        stagedRowValidCopy = m_stagedRowValid;
        xfEnabledCopy   = m_crossfadeEnabled;
        syncMultiFxPhaseCopy = m_syncMultiFxPhaseToCrossfade;
        multiFxSyncOffsetMsCopy = m_multiFxCrossfadeSyncOffsetMs;
        xfPosCopy       = m_crossfadeGlobalPos;
        xfStartPosCopy  = m_crossfadeStartPos;
        xfStagedAtLowSideCopy = m_crossfadeStagedAtLowSide;
        xfEditLaneStagedCopy = m_crossfadeEditLaneStaged;
        modeCopy        = m_mode;
        groupIdCopy     = m_fixtureGroupId;
        spatialCopy     = m_spatialEffects;
        contFxSelectorModeCopy = m_continuousFxSelectorMode;
        linkedTransitionCopy = m_linkedTransitionWidgetId;
        multiFxRestartKeyCopy = m_multiFxRestartKey;
        widgetFlashGateKeyCopy = m_widgetFlashGateKey;
        widgetFlashTimeMultiplierIndexCopy = m_widgetFlashTimeMultiplierIndex;
        widgetFlashBehaviorCopy = m_widgetFlashBehavior;
        positionConfirmDiscardDraftCopy = m_positionConfirmDiscardDraft;
        positionShowStatusStripCopy = m_positionShowStatusStrip;
        positionShowEditorHintsCopy = m_positionShowEditorHints;
    }

    {
        QMutexLocker lk2(&copy->m_stateMutex);
        copy->m_columns            = colsCopy;
        copy->m_rows               = rowsCopy;
        copy->m_positionOverrides  = positionOverridesCopy;
        copy->m_outputs            = outsCopy;
        copy->m_activeRow          = activeRowCopy;
        copy->m_stagedRow          = stagedRowCopy;
        copy->m_stagedRowValid     = stagedRowValidCopy;
        copy->m_crossfadeEnabled   = xfEnabledCopy;
        copy->m_syncMultiFxPhaseToCrossfade = syncMultiFxPhaseCopy;
        copy->m_multiFxCrossfadeSyncOffsetMs = multiFxSyncOffsetMsCopy;
        copy->m_crossfadeGlobalPos = xfPosCopy;
        copy->m_crossfadeStartPos  = xfStartPosCopy;
        copy->m_crossfadeStagedAtLowSide = xfStagedAtLowSideCopy;
        copy->m_crossfadeEditLaneStaged = xfEditLaneStagedCopy;
        copy->m_mode               = modeCopy;
        copy->m_fixtureGroupId     = groupIdCopy;
        copy->m_spatialEffects             = spatialCopy;
        copy->m_continuousFxSelectorMode   = contFxSelectorModeCopy;
        copy->m_linkedTransitionWidgetId   = linkedTransitionCopy;
        copy->m_multiFxRestartKey          = multiFxRestartKeyCopy;
        copy->m_widgetFlashGateKey         = widgetFlashGateKeyCopy;
        copy->m_widgetFlashTimeMultiplierIndex = widgetFlashTimeMultiplierIndexCopy;
        copy->m_widgetFlashBehavior        = widgetFlashBehaviorCopy;
        copy->m_positionConfirmDiscardDraft = positionConfirmDiscardDraftCopy;
        copy->m_positionShowStatusStrip = positionShowStatusStripCopy;
        copy->m_positionShowEditorHints = positionShowEditorHintsCopy;
        copy->m_widgetFlashGateActive      = false;
        copy->m_widgetFlashGateLastValue   = 0;
        copy->m_widgetStagedFlashToken     = 0;
        copy->m_spatialAppliedRow.resize(outsCopy.size());
        copy->m_spatialAppliedRow.fill(-1);
        copy->m_spatialChase.resize(outsCopy.size());
        copy->m_spatialChase.fill(PTSpatialChaseOutput(), outsCopy.size());
        copy->syncLiveTransitionFromOutputs();
    }

    // Copy input sources
    for (int o = 0; o < outsCopy.size(); ++o)
    {
        if (o >= PTInputId::kMaxRoutableOutputs)
            break;
        copy->setInputSource(inputSource(PTInputId::rowSelector(o)), PTInputId::rowSelector(o));
        if (modeCopy == PTMode::FixtureGroup || modeCopy == PTMode::Position)
        {
            copy->setInputSource(inputSource(PTInputId::transSweep(o)), PTInputId::transSweep(o));
            if (o < PTInputId::kMaxRoutableOutputs)
            {
                copy->setInputSource(inputSource(PTInputId::transContinuousBank(o)),
                                    PTInputId::transContinuousBank(o));
                copy->setInputSource(inputSource(PTInputId::multiFxBank(o)),
                                     PTInputId::multiFxBank(o));
            }
            copy->setInputSource(inputSource(PTInputId::transSecondaryRow(o)),
                                 PTInputId::transSecondaryRow(o));
        }
    }
    copy->setInputSource(inputSource(PTInputId::kCrossfade), PTInputId::kCrossfade);
    copy->setInputSource(inputSource(PTInputId::kMultiFxBlend), PTInputId::kMultiFxBlend);
    copy->setInputSource(inputSource(PTInputId::kMultiFxRestart), PTInputId::kMultiFxRestart);
    copy->setInputSource(inputSource(PTInputId::kWidgetFlashGate), PTInputId::kWidgetFlashGate);

    copy->rebuildTable();
    return copy;
}

// ==========================================================================
// Cross-project clipboard
// ==========================================================================

void PresetTableV2Widget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    /* Sync any pending UI edits to m_rows/m_columns (same as saveXML does) */
    const_cast<PresetTableV2Widget*>(this)->syncAllDataFromTable();

    QMutexLocker lk(const_cast<QMutex*>(&m_stateMutex));

    obj["crossfadeEnabled"] = m_crossfadeEnabled;
    obj["syncMultiFxPhaseToCrossfade"] = m_syncMultiFxPhaseToCrossfade;
    obj["multiFxCrossfadeSyncOffsetMs"] = m_multiFxCrossfadeSyncOffsetMs;
    obj["continuousFxSelectorMode"] = continuousFxSelectorModeToString(m_continuousFxSelectorMode);
    obj["widgetFlashTimeMultiplier"] = qBound(0, m_widgetFlashTimeMultiplierIndex,
                                              kWidgetFlashTimeMultiplierMax);
    obj["widgetFlashBehavior"] = widgetFlashBehaviorToString(m_widgetFlashBehavior);
    obj["positionConfirmDiscardDraft"] = m_positionConfirmDiscardDraft;
    obj["positionShowStatusStrip"] = m_positionShowStatusStrip;
    obj["positionShowEditorHints"] = m_positionShowEditorHints;
    if (!m_multiFxRestartKey.isEmpty())
        obj["multiFxRestartKey"] = m_multiFxRestartKey.toString(QKeySequence::PortableText);
    if (!m_widgetFlashGateKey.isEmpty())
        obj["widgetFlashGateKey"] = m_widgetFlashGateKey.toString(QKeySequence::PortableText);
    obj["mode"] = (m_mode == PTMode::Position) ? QStringLiteral("Position")
            : ((m_mode == PTMode::FixtureGroup) ? QStringLiteral("FixtureGroup")
                                                : QStringLiteral("Legacy"));
    if (m_mode == PTMode::FixtureGroup || m_mode == PTMode::Position)
    {
        FixtureGroup *grp = doc->fixtureGroup(m_fixtureGroupId);
        obj["fixtureGroupName"] = grp ? grp->name() : QString();
    }

    /* Columns */
    QJsonArray cols;
    for (const PTColumn &col : m_columns)
    {
        QJsonObject c;
        c["name"]  = col.name;
        c["type"]  = (col.type == PTColumn::Dropdown ? QStringLiteral("Dropdown") :
                      col.type == PTColumn::Scaler   ? QStringLiteral("Scaler")   :
                                                       QStringLiteral("Numeric"));
        c["fade"]  = col.fade;
        c["width"] = col.width;
        if (col.type == PTColumn::Scaler)
        {
            c["scalerMin"] = col.scalerMin;
            c["scalerMax"] = col.scalerMax;
            c["scalerSuffix"] = col.scalerSuffix;
        }
        QJsonArray bindArr;
        for (const PTColumnTypeBinding& binding : col.bindings)
        {
            if (!binding.isValid())
                continue;
            QJsonObject b;
            b["mfg"]  = binding.manufacturer;
            b["model"]= binding.model;
            b["mode"] = binding.modeName;
            b["chan"] = binding.channelIndex;
            bindArr.append(b);
        }
        if (!bindArr.isEmpty())
        {
            c["bindings"] = bindArr;
            c["binding"] = bindArr.first().toObject();
        }
        QJsonArray opts;
        for (const PTOption &opt : col.options)
        {
            QJsonObject o;
            o["name"]  = opt.name;
            o["value"] = (int)opt.value;
            o["resource"] = opt.resource;
            opts.append(o);
        }
        if (!opts.isEmpty())
            c["options"] = opts;
        cols.append(c);
    }
    obj["columns"] = cols;

    /* Rows */
    QJsonArray rows;
    for (const PTRow &row : m_rows)
    {
        QJsonObject r;
        r["name"] = row.name;
        QJsonArray vals;
        for (uchar v : row.values)
            vals.append((int)v);
        r["values"] = vals;
        rows.append(r);
    }
    obj["rows"] = rows;

    /* Outputs — fixture by name (Legacy) or groupRows (FixtureGroup) */
    QJsonArray outs;
    for (const PTOutput &out : m_outputs)
    {
        QJsonObject o;
        o["name"] = out.name;
        if (m_mode == PTMode::Legacy)
        {
            Fixture *fxi = doc->fixture(out.fixtureId);
            o["fixtureName"] = fxi ? fxi->name() : QString();
        }
        else
        {
            QJsonArray gRows;
            for (int r : out.groupRows)
                gRows.append(r);
            o["groupRows"] = gRows;
            o["outputScope"] = scopeToString(out.scope);
            o["sweepPresetIndex"] = out.sweepPresetIndex;
            o["continuousPresetIndex"] = out.continuousPresetIndex;
            o["multiFxPresetIndex"] = out.multiFxPresetIndex;
            o["secondaryRowIndex"] = out.secondaryRowIndex;
        }
        outs.append(o);
    }
    obj["outputs"] = outs;
}

void PresetTableV2Widget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    QMutexLocker lk(&m_stateMutex);

    m_crossfadeEnabled = obj["crossfadeEnabled"].toBool(false);
    m_syncMultiFxPhaseToCrossfade = obj["syncMultiFxPhaseToCrossfade"].toBool(false);
    m_multiFxCrossfadeSyncOffsetMs = qBound(0,
            obj.contains(QStringLiteral("multiFxCrossfadeSyncOffsetMs"))
                    ? obj["multiFxCrossfadeSyncOffsetMs"].toInt(40) : 40,
            200);
    m_continuousFxSelectorMode = continuousFxSelectorModeFromString(
            obj["continuousFxSelectorMode"].toString());
    m_widgetFlashTimeMultiplierIndex =
            qBound(0, obj["widgetFlashTimeMultiplier"].toInt(2), kWidgetFlashTimeMultiplierMax);
    m_widgetFlashBehavior = widgetFlashBehaviorFromString(
            obj["widgetFlashBehavior"].toString());
    m_positionConfirmDiscardDraft = obj.contains(QStringLiteral("positionConfirmDiscardDraft"))
            ? obj["positionConfirmDiscardDraft"].toBool(true)
            : true;
    m_positionShowStatusStrip = obj.contains(QStringLiteral("positionShowStatusStrip"))
            ? obj["positionShowStatusStrip"].toBool(true)
            : true;
    m_positionShowEditorHints = obj.contains(QStringLiteral("positionShowEditorHints"))
            ? obj["positionShowEditorHints"].toBool(true)
            : true;
    m_multiFxRestartKey = stripKeySequence(QKeySequence(obj["multiFxRestartKey"].toString()));
    m_widgetFlashGateKey = stripKeySequence(QKeySequence(obj["widgetFlashGateKey"].toString()));
    m_widgetFlashGateActive = false;
    m_widgetFlashGateLastValue = 0;
    m_widgetStagedFlashToken = 0;
    m_mode = (obj["mode"].toString() == QLatin1String("Position"))
             ? PTMode::Position
             : ((obj["mode"].toString() == QLatin1String("FixtureGroup"))
                ? PTMode::FixtureGroup : PTMode::Legacy);

    m_fixtureGroupId = UINT_MAX;
    if (m_mode == PTMode::FixtureGroup || m_mode == PTMode::Position)
    {
        const QString gName = obj["fixtureGroupName"].toString();
        if (!gName.isEmpty())
        {
            for (FixtureGroup *grp : doc->fixtureGroups())
            {
                if (grp && grp->name() == gName)
                {
                    m_fixtureGroupId = grp->id();
                    break;
                }
            }
        }
    }

    /* Columns */
    m_columns.clear();
    for (const QJsonValue &v : obj["columns"].toArray())
    {
        QJsonObject c = v.toObject();
        PTColumn col;
        col.name  = c["name"].toString();
        col.fade  = c["fade"].toBool(true);
        col.width = c["width"].toInt(-1);
        const QString typeStr = c["type"].toString();
        col.type = (typeStr == QLatin1String("Dropdown") ? PTColumn::Dropdown :
                    typeStr == QLatin1String("Scaler")   ? PTColumn::Scaler   :
                                                           PTColumn::Numeric);
        if (col.type == PTColumn::Scaler)
        {
            col.scalerMin    = c["scalerMin"].toInt(0);
            col.scalerMax    = c["scalerMax"].toInt(360);
            col.scalerSuffix = c["scalerSuffix"].toString();
        }
        auto appendBindingFromJson = [&](const QJsonObject& b) {
            PTColumnTypeBinding binding;
            binding.manufacturer = b["mfg"].toString();
            binding.model        = b["model"].toString();
            binding.modeName     = b["mode"].toString();
            binding.channelIndex = b["chan"].toInt(-1);
            if (binding.isValid())
                col.bindings.append(binding);
        };
        if (c.contains("bindings"))
        {
            for (const QJsonValue& bv : c["bindings"].toArray())
                appendBindingFromJson(bv.toObject());
        }
        else if (c.contains("binding"))
        {
            appendBindingFromJson(c["binding"].toObject());
        }
        for (const QJsonValue &ov : c["options"].toArray())
        {
            QJsonObject o = ov.toObject();
            PTOption opt;
            opt.name     = o["name"].toString();
            opt.value    = (uchar)o["value"].toInt(0);
            opt.resource = o["resource"].toString();
            col.options.append(opt);
        }
        m_columns.append(col);
    }

    /* Rows */
    m_rows.clear();
    for (const QJsonValue &v : obj["rows"].toArray())
    {
        QJsonObject r = v.toObject();
        PTRow row;
        row.name = r["name"].toString();
        for (const QJsonValue &val : r["values"].toArray())
            row.values.append((uchar)val.toInt(0));
        m_rows.append(row);
    }

    /* Outputs */
    m_outputs.clear();
    for (const QJsonValue &v : obj["outputs"].toArray())
    {
        QJsonObject o = v.toObject();
        PTOutput out;
        out.name = o["name"].toString();
        if (m_mode == PTMode::Legacy)
        {
            const QString fxName = o["fixtureName"].toString();
            out.fixtureId = UINT_MAX;
            if (!fxName.isEmpty())
            {
                for (Fixture *fxi : doc->fixtures())
                {
                    if (fxi && fxi->name() == fxName)
                    {
                        out.fixtureId = fxi->id();
                        break;
                    }
                }
            }
        }
        else
        {
            for (const QJsonValue &rv : o["groupRows"].toArray())
                out.groupRows.append(rv.toInt());
            out.scope = scopeFromString(o["outputScope"].toString());
            out.sweepPresetIndex = o["sweepPresetIndex"].toInt(-1);
            out.continuousPresetIndex = o["continuousPresetIndex"].toInt(-1);
            out.multiFxPresetIndex = o["multiFxPresetIndex"].toInt(-1);
            out.secondaryRowIndex = o["secondaryRowIndex"].toInt(-1);
        }
        m_outputs.append(out);
    }

    m_activeRow.fill(-1, m_outputs.size());
    m_stagedRow.fill(-1, m_outputs.size());
    m_stagedRowValid.fill(false, m_outputs.size());
    syncLiveTransitionFromOutputs();

    lk.unlock();
    rebuildTable();
}

// ==========================================================================
// paintEvent
// ==========================================================================

void PresetTableV2Widget::paintEvent(QPaintEvent* e)
{
    QPainter p(this);
    QColor bg = QColor(0x1c, 0x1c, 0x1c);
    p.fillRect(rect(), bg);
    p.end();
    VCWidget::paintEvent(e);
}

void PresetTableV2Widget::resizeEvent(QResizeEvent* e)
{
    VCWidget::resizeEvent(e);
    syncFrozenNameColumnLayout();
}

// ==========================================================================
// loadXML / saveXML
// ==========================================================================

bool PresetTableV2Widget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot) return false;

    loadXMLCommon(root);

    bool xfEnabled = (root.attributes().value(KXMLCrossfadeEn).toString() == QLatin1String("True"));
    bool syncMultiFxPhase = (root.attributes().value(KXMLSyncMultiFxPhaseToCrossfade).toString()
            == QLatin1String("True"));
    int multiFxSyncOffsetMs = root.attributes().hasAttribute(KXMLMultiFxCrossfadeSyncOffsetMs)
            ? root.attributes().value(KXMLMultiFxCrossfadeSyncOffsetMs).toInt() : 40;
    int widgetFlashTimeMultiplierIndex = root.attributes().hasAttribute(KXMLWidgetFlashTimeMultiplier)
            ? qBound(0, root.attributes().value(KXMLWidgetFlashTimeMultiplier).toInt(),
                      kWidgetFlashTimeMultiplierMax)
            : 2;
    PTWidgetFlashBehavior widgetFlashBehavior = widgetFlashBehaviorFromString(
            root.attributes().value(KXMLWidgetFlashBehavior).toString());
    int  nameColW  = root.attributes().value(KXMLNameColWidth).toInt();
    PTContinuousFxSelectorMode loadedContFxSelectorMode = continuousFxSelectorModeFromString(
            root.attributes().value(KXMLContinuousFxSelectorMode).toString());
    const bool loadedPositionConfirmDiscardDraft =
            root.attributes().hasAttribute(KXMLPositionConfirmDiscardDraft)
            ? (root.attributes().value(KXMLPositionConfirmDiscardDraft).toString()
               == QLatin1String("True"))
            : true;
    const bool loadedPositionShowStatusStrip =
            root.attributes().hasAttribute(KXMLPositionShowStatusStrip)
            ? (root.attributes().value(KXMLPositionShowStatusStrip).toString()
               == QLatin1String("True"))
            : true;
    const bool loadedPositionShowEditorHints =
            root.attributes().hasAttribute(KXMLPositionShowEditorHints)
            ? (root.attributes().value(KXMLPositionShowEditorHints).toString()
               == QLatin1String("True"))
            : true;

    // Mode (default = Legacy for backward compat)
    PTMode loadedMode = PTMode::Legacy;
    QString modeStr = root.attributes().value(KXMLMode).toString();
    if (modeStr == QLatin1String("Position"))
        loadedMode = PTMode::Position;
    else if (modeStr == QLatin1String("FixtureGroup"))
        loadedMode = PTMode::FixtureGroup;

    quint32 loadedGroupId = UINT_MAX;
    QString groupIdStr = root.attributes().value(KXMLFxGroupId).toString();
    if (!groupIdStr.isEmpty())
        loadedGroupId = groupIdStr.toUInt();

    PTSpatialEffectSettings loadedSpatial;
    if (root.attributes().hasAttribute(KXMLSpatialEn))
        loadedSpatial.enabled = (root.attributes().value(KXMLSpatialEn).toString() == QLatin1String("True"));
    loadedSpatial.order = PresetTableV2SpatialEngine::orderFromString(
            root.attributes().value(KXMLSpatialOrder).toString());
    if (root.attributes().hasAttribute(KXMLSpatialStepMs))
        loadedSpatial.stepDelayMs = root.attributes().value(KXMLSpatialStepMs).toUInt();
    if (root.attributes().hasAttribute(KXMLSpatialFadeMs))
        loadedSpatial.fadeMs = root.attributes().value(KXMLSpatialFadeMs).toUInt();
    loadedSpatial.reverse = (root.attributes().value(KXMLSpatialReverse).toString() == QLatin1String("True"));

    quint32 loadedLinkedTransition = VCWidget::invalidId();
    if (root.attributes().hasAttribute(KXMLLinkedTransition))
        loadedLinkedTransition = root.attributes().value(KXMLLinkedTransition).toUInt();

    QVector<PTColumn> cols;
    QVector<PTRow>    rows;
    QVector<QHash<int, PTPositionOutputLayer>> positionOverrides;
    QVector<PTOutput> outs;
    QKeySequence loadedMultiFxRestartKey;
    QKeySequence loadedWidgetFlashGateKey;
    QSharedPointer<QLCInputSource> loadedMultiFxRestartSource;
    QSharedPointer<QLCInputSource> loadedWidgetFlashGateSource;

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0; bool vis = true;
            loadXMLWindowState(root, &x, &y, &w, &h, &vis);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
        {
            loadXMLAppearance(root);
        }
        else if (root.name() == KXMLColumn)
        {
            auto attrs = root.attributes();
            PTColumn col;
            col.name  = attrs.value(KXMLColName).toString();
            {
                QString typeStr = attrs.value(KXMLColType).toString();
                if (typeStr == QLatin1String("Dropdown"))
                    col.type = PTColumn::Dropdown;
                else if (typeStr == QLatin1String("Scaler"))
                    col.type = PTColumn::Scaler;
                else
                    col.type = PTColumn::Numeric;
            }
            col.fade  = (attrs.value(KXMLColFade).toString() != QLatin1String("False"));
            col.width = attrs.value(KXMLColWidth).toInt();
            if (col.width <= 0) col.width = -1;

            // FixtureGroup binding — legacy attrs on Column + optional Binding children
            QString bindMfg  = attrs.value(KXMLBindMfg).toString();
            QString bindMod  = attrs.value(KXMLBindModel).toString();
            QString bindMode = attrs.value(KXMLBindMode).toString();
            int     bindChan = attrs.value(KXMLBindChan).toInt() - 1;  // stored as 1-based, 0 if absent

            // Scaler attributes (absent in older files → defaults kept)
            if (col.type == PTColumn::Scaler)
            {
                col.scalerMin = attrs.value(KXMLColScalerMin).toInt();  // 0 if absent
                QString maxStr = attrs.value(KXMLColScalerMax).toString();
                col.scalerMax = maxStr.isEmpty() ? 360 : maxStr.toInt();
                col.scalerSuffix = attrs.value(KXMLColScalerSfx).toString();
            }

            // Read child <Option> and <Binding> elements
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLOption)
                {
                    PTOption opt;
                    opt.name     = root.attributes().value(KXMLOptName).toString();
                    opt.value    = uchar(root.attributes().value(KXMLOptValue).toUInt());
                    opt.resource = root.attributes().value(KXMLOptResource).toString();
                    col.options.append(opt);
                    root.skipCurrentElement();
                }
                else if (root.name() == KXMLBinding)
                {
                    PTColumnTypeBinding binding;
                    binding.manufacturer = root.attributes().value(KXMLBindMfg).toString();
                    binding.model        = root.attributes().value(KXMLBindModel).toString();
                    binding.modeName     = root.attributes().value(KXMLBindMode).toString();
                    const int chan = root.attributes().value(KXMLBindChan).toInt() - 1;
                    if (!binding.manufacturer.isEmpty() && chan >= 0)
                    {
                        binding.channelIndex = chan;
                        col.bindings.append(binding);
                    }
                    root.skipCurrentElement();
                }
                else
                {
                    root.skipCurrentElement();
                }
            }
            if (col.bindings.isEmpty() && !bindMfg.isEmpty() && bindChan >= 0)
            {
                PTColumnTypeBinding binding;
                binding.manufacturer = bindMfg;
                binding.model        = bindMod;
                binding.modeName     = bindMode;
                binding.channelIndex = bindChan;
                col.bindings.append(binding);
            }
            cols.append(col);
        }
        else if (root.name() == KXMLRow)
        {
            auto attrs = root.attributes();
            PTRow row;
            row.name = attrs.value(KXMLRowName).toString();

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLV)
                    row.values.append(uchar(root.readElementText().toUInt()));
                else if (root.name() == KXMLPosition)
                {
                    PTPositionValue pos;
                    const auto pattrs = root.attributes();
                    const int x = pattrs.value(KXMLPositionX).toInt();
                    const int y = pattrs.value(KXMLPositionY).toInt();
                    const QLCPoint pt(x, y);
                    pos.valid = true;
                    if (pattrs.hasAttribute(KXMLPositionPanDeg))
                    {
                        pos.panDeg = pattrs.value(KXMLPositionPanDeg).toDouble();
                        pos.tiltDeg = pattrs.value(KXMLPositionTiltDeg).toDouble();
                    }
                    else
                    {
                        const quint16 pan16 = quint16(qBound(0, pattrs.value(KXMLPositionPan).toInt(), 65535));
                        const quint16 tilt16 = quint16(qBound(0, pattrs.value(KXMLPositionTilt).toInt(), 65535));
                        Fixture* fxi = nullptr;
                        int head = 0;
                        if (m_doc)
                        {
                            if (FixtureGroup* grp = m_doc->fixtureGroup(loadedGroupId))
                            {
                                const GroupHead gh = grp->headsMap().value(pt);
                                head = gh.head;
                                fxi = m_doc->fixture(gh.fxi);
                            }
                        }
                        pos.panDeg = PTPositionConverter::pan16ToPanDeg(fxi, head, pan16);
                        pos.tiltDeg = PTPositionConverter::tilt16ToTiltDeg(fxi, head, tilt16);
                    }
                    row.positions.insert(pt, pos);
                    root.skipCurrentElement();
                }
                else
                    root.skipCurrentElement();
            }
            rows.append(row);
        }
        else if (root.name() == KXMLPositionOverride)
        {
            const auto pattrs = root.attributes();
            const int rowIdx = pattrs.value(KXMLPosOvRow).toInt();
            const int outputIdx = pattrs.value(KXMLPosOvOutput).toInt();
            const int selectionIdx = pattrs.value(KXMLPosOvSelection).toInt();
            const QLCPoint pt(pattrs.value(KXMLPositionX).toInt(),
                              pattrs.value(KXMLPositionY).toInt());
            PTPositionValue pos;
            pos.valid = true;
            pos.panDeg = pattrs.value(KXMLPositionPanDeg).toDouble();
            pos.tiltDeg = pattrs.value(KXMLPositionTiltDeg).toDouble();
            while (positionOverrides.size() <= rowIdx)
                positionOverrides.append(QHash<int, PTPositionOutputLayer>());
            PTPositionOutputLayer& layer = positionOverrides[rowIdx][outputIdx];
            if (selectionIdx < 0)
            {
                layer.allOverrides.insert(pt, pos);
            }
            else
            {
                while (layer.selections.size() <= selectionIdx)
                    layer.selections.append(PTPositionSelectionLayer());
                PTPositionSelectionLayer& sel = layer.selections[selectionIdx];
                if (pattrs.hasAttribute(KXMLPosOvSelName))
                    sel.name = pattrs.value(KXMLPosOvSelName).toString();
                if (pattrs.hasAttribute(KXMLPosOvSelColor))
                    sel.color = QColor(pattrs.value(KXMLPosOvSelColor).toString());
                sel.cells.insert(pt);
                sel.overrides.insert(pt, pos);
            }
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLOutput)
        {
            auto attrs = root.attributes();
            int idx = attrs.value(KXMLOutIndex).toInt();
            PTOutput out;
            out.name      = attrs.value(KXMLOutName).toString();
            out.fixtureId = attrs.value(KXMLOutFxId).toUInt();

            // FixtureGroup rows (absent in legacy files)
            QString rowsStr = attrs.value(KXMLOutRows).toString();
            if (!rowsStr.isEmpty())
            {
                for (const QString& rStr : rowsStr.split(QLatin1Char(','), Qt::SkipEmptyParts))
                    out.groupRows.append(rStr.trimmed().toInt());
            }
            out.scope = scopeFromString(attrs.value(KXMLOutScope).toString());
            if (attrs.hasAttribute(KXMLOutSweepPreset))
                out.sweepPresetIndex = attrs.value(KXMLOutSweepPreset).toInt();
            else if (attrs.hasAttribute(KXMLOutTransitionPreset))
                out.sweepPresetIndex = attrs.value(KXMLOutTransitionPreset).toInt();
            if (attrs.hasAttribute(KXMLOutContinuousPreset))
                out.continuousPresetIndex = attrs.value(KXMLOutContinuousPreset).toInt();
            if (attrs.hasAttribute(KXMLOutMultiFxPreset))
                out.multiFxPresetIndex = attrs.value(KXMLOutMultiFxPreset).toInt();
            if (attrs.hasAttribute(KXMLOutSecondaryRow))
                out.secondaryRowIndex = attrs.value(KXMLOutSecondaryRow).toInt();
            else if (attrs.hasAttribute(KXMLOutTransitionSecondary))
                out.secondaryRowIndex = attrs.value(KXMLOutTransitionSecondary).toInt();

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLOutInput)
                {
                    if (idx < PTInputId::kMaxRoutableOutputs)
                        loadXMLSources(root, PTInputId::rowSelector(idx));
                    else
                        root.skipCurrentElement();
                }
                else if (root.name() == KXMLOutTransSweepInput)
                {
                    if (idx < PTInputId::kMaxRoutableOutputs)
                        loadXMLSources(root, PTInputId::transSweep(idx));
                    else
                        root.skipCurrentElement();
                }
                else if (root.name() == KXMLOutTransPrimaryInput)
                {
                    if (idx < PTInputId::kMaxRoutableOutputs)
                        loadXMLSources(root, PTInputId::transSweep(idx));
                    else
                        root.skipCurrentElement();
                }
                else if (root.name() == KXMLOutTransContinuousInput)
                {
                    if (idx < PTInputId::kMaxRoutableOutputs)
                        loadXMLSources(root, PTInputId::transContinuousBank(idx));
                    else
                        root.skipCurrentElement();
                }
                else if (root.name() == KXMLOutMultiFxInput)
                {
                    if (idx < PTInputId::kMaxRoutableOutputs)
                        loadXMLSources(root, PTInputId::multiFxBank(idx));
                    else
                        root.skipCurrentElement();
                }
                else if (root.name() == KXMLOutTransSecondaryInput)
                {
                    if (idx < PTInputId::kMaxRoutableOutputs)
                        loadXMLSources(root, PTInputId::transSecondaryRow(idx));
                    else
                        root.skipCurrentElement();
                }
                else
                    root.skipCurrentElement();
            }
            // Grow outs vector to fit index
            while (outs.size() <= idx) outs.append(PTOutput());
            outs[idx] = out;
        }
        else if (root.name() == KXMLCrossfadeInput)
        {
            loadXMLSources(root, PTInputId::kCrossfade);
        }
        else if (root.name() == KXMLMultiFxBlendInput)
        {
            loadXMLSources(root, PTInputId::kMultiFxBlend);
        }
        else if (root.name() == KXMLMultiFxRestartInput)
        {
            const PTInputBinding binding = readPTInputBlock(root, this);
            loadedMultiFxRestartSource = binding.source;
            loadedMultiFxRestartKey = binding.key;
            setInputSource(binding.source, PTInputId::kMultiFxRestart);
        }
        else if (root.name() == KXMLWidgetFlashGateInput)
        {
            const PTInputBinding binding = readPTInputBlock(root, this);
            loadedWidgetFlashGateSource = binding.source;
            loadedWidgetFlashGateKey = binding.key;
            setInputSource(binding.source, PTInputId::kWidgetFlashGate);
        }
        else if (root.name() == KXMLSelectorStateOutput)
        {
            root.skipCurrentElement();
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    {
        QMutexLocker lk(&m_stateMutex);
        m_columns = cols;
        m_rows    = rows;
        m_positionOverrides = positionOverrides;
        while (m_positionOverrides.size() < m_rows.size())
            m_positionOverrides.append(QHash<int, PTPositionOutputLayer>());
        // Ensure row values are correct size
        for (PTRow& r : m_rows)
            r.values.resize(m_columns.size(), 0);
        m_outputs = outs;
        m_activeRow.resize(m_outputs.size());
        m_activeRow.fill(-1);
        m_stagedRow.resize(m_outputs.size());
        m_stagedRow.fill(-1);
        m_stagedRowValid.resize(m_outputs.size());
        m_stagedRowValid.fill(false);
        m_crossfadeEnabled   = xfEnabled;
        m_syncMultiFxPhaseToCrossfade = syncMultiFxPhase;
        m_multiFxCrossfadeSyncOffsetMs = qBound(0, multiFxSyncOffsetMs, 200);
        m_continuousFxSelectorMode = loadedContFxSelectorMode;
        m_multiFxRestartKey = loadedMultiFxRestartKey;
        m_widgetFlashGateKey = loadedWidgetFlashGateKey;
        m_widgetFlashTimeMultiplierIndex = qBound(0, widgetFlashTimeMultiplierIndex,
                                                kWidgetFlashTimeMultiplierMax);
        m_widgetFlashBehavior = widgetFlashBehavior;
        m_positionConfirmDiscardDraft = loadedPositionConfirmDiscardDraft;
        m_positionShowStatusStrip = loadedPositionShowStatusStrip;
        m_positionShowEditorHints = loadedPositionShowEditorHints;
        m_widgetFlashGateActive = false;
        m_widgetFlashGateLastValue = 0;
        m_widgetStagedFlashToken = 0;
        m_crossfadeGlobalPos = 0;
        m_crossfadeStartPos  = 0;
        m_crossfadePrevPos   = 0;
        m_crossfadeStagedAtLowSide = true;
        m_crossfadeSessionActive = false;
        m_crossfadeEditLaneStaged = true;
        m_nameColWidth = (nameColW > 0) ? nameColW : -1;
        m_mode           = loadedMode;
        m_fixtureGroupId = loadedGroupId;
        m_spatialEffects = loadedSpatial;
        m_linkedTransitionWidgetId = loadedLinkedTransition;
        m_spatialAppliedRow.resize(m_outputs.size());
        m_spatialAppliedRow.fill(-1);
        m_spatialChase.resize(m_outputs.size());
        m_spatialChase.fill(PTSpatialChaseOutput(), m_spatialChase.size());
        m_liveMultiFxPreset.resize(m_outputs.size());
        m_multiFxElapsedMs.resize(m_outputs.size());
        m_multiFxStagedElapsedMs.resize(m_outputs.size());
        m_multiFxStagedLastCycleMs.resize(m_outputs.size());
    }

    refreshTransitionPresetCache();
    {
        QMutexLocker lk(&m_stateMutex);
        syncLiveTransitionFromOutputs();
    }
    rebuildTable();
    return true;
}

bool PresetTableV2Widget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    syncAllDataFromTable();

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);
    {
        QMutexLocker lk2(&m_stateMutex);
        doc->writeAttribute(KXMLCrossfadeEn, m_crossfadeEnabled ? QLatin1String("True") : QLatin1String("False"));
        doc->writeAttribute(KXMLSyncMultiFxPhaseToCrossfade,
                            m_syncMultiFxPhaseToCrossfade ? QLatin1String("True") : QLatin1String("False"));
        doc->writeAttribute(KXMLMultiFxCrossfadeSyncOffsetMs,
                            QString::number(qBound(0, m_multiFxCrossfadeSyncOffsetMs, 200)));
        doc->writeAttribute(KXMLContinuousFxSelectorMode,
                            continuousFxSelectorModeToString(m_continuousFxSelectorMode));
        doc->writeAttribute(KXMLWidgetFlashTimeMultiplier,
                            QString::number(qBound(0, m_widgetFlashTimeMultiplierIndex,
                                                   kWidgetFlashTimeMultiplierMax)));
        doc->writeAttribute(KXMLWidgetFlashBehavior,
                            widgetFlashBehaviorToString(m_widgetFlashBehavior));
        doc->writeAttribute(KXMLPositionConfirmDiscardDraft,
                            m_positionConfirmDiscardDraft ? QLatin1String("True")
                                                          : QLatin1String("False"));
        doc->writeAttribute(KXMLPositionShowStatusStrip,
                            m_positionShowStatusStrip ? QLatin1String("True")
                                                      : QLatin1String("False"));
        doc->writeAttribute(KXMLPositionShowEditorHints,
                            m_positionShowEditorHints ? QLatin1String("True")
                                                      : QLatin1String("False"));
        if (m_nameColWidth > 0)
            doc->writeAttribute(KXMLNameColWidth, QString::number(m_nameColWidth));

        // Write mode (only write explicit tag for non-legacy; Legacy is default for old files)
        if (m_mode == PTMode::FixtureGroup || m_mode == PTMode::Position)
        {
            doc->writeAttribute(KXMLMode, m_mode == PTMode::Position
                                ? QLatin1String("Position")
                                : QLatin1String("FixtureGroup"));
            doc->writeAttribute(KXMLFxGroupId, QString::number(m_fixtureGroupId));
        }

        doc->writeAttribute(KXMLSpatialEn, m_spatialEffects.enabled ? QLatin1String("True") : QLatin1String("False"));
        doc->writeAttribute(KXMLSpatialOrder,
                            PresetTableV2SpatialEngine::orderToString(m_spatialEffects.order));
        doc->writeAttribute(KXMLSpatialStepMs, QString::number(m_spatialEffects.stepDelayMs));
        doc->writeAttribute(KXMLSpatialFadeMs, QString::number(m_spatialEffects.fadeMs));
        doc->writeAttribute(KXMLSpatialReverse, m_spatialEffects.reverse ? QLatin1String("True") : QLatin1String("False"));
        if (m_linkedTransitionWidgetId != VCWidget::invalidId())
            doc->writeAttribute(KXMLLinkedTransition, QString::number(m_linkedTransitionWidgetId));
    }

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    QMutexLocker lk(&m_stateMutex);

    for (int c = 0; c < m_columns.size(); ++c)
    {
        const PTColumn& col = m_columns[c];
        doc->writeStartElement(KXMLColumn);
        doc->writeAttribute(KXMLColIndex, QString::number(c));
        doc->writeAttribute(KXMLColName,  col.name);
        doc->writeAttribute(KXMLColType,
            col.type == PTColumn::Dropdown ? QLatin1String("Dropdown") :
            col.type == PTColumn::Scaler   ? QLatin1String("Scaler")   :
                                             QLatin1String("Numeric"));
        if (col.type == PTColumn::Scaler)
        {
            doc->writeAttribute(KXMLColScalerMin, QString::number(col.scalerMin));
            doc->writeAttribute(KXMLColScalerMax, QString::number(col.scalerMax));
            if (!col.scalerSuffix.isEmpty())
                doc->writeAttribute(KXMLColScalerSfx, col.scalerSuffix);
        }
        doc->writeAttribute(KXMLColFade,  col.fade ? QLatin1String("True") : QLatin1String("False"));
        if (col.width > 0)
            doc->writeAttribute(KXMLColWidth, QString::number(col.width));

        // FixtureGroup bindings — legacy attrs mirror first entry for old readers
        const PTColumnTypeBinding* firstBinding = nullptr;
        for (const PTColumnTypeBinding& binding : col.bindings)
        {
            if (!binding.isValid())
                continue;
            if (firstBinding == nullptr)
                firstBinding = &binding;
        }
        if (firstBinding != nullptr)
        {
            doc->writeAttribute(KXMLBindMfg,   firstBinding->manufacturer);
            doc->writeAttribute(KXMLBindModel, firstBinding->model);
            doc->writeAttribute(KXMLBindMode,  firstBinding->modeName);
            doc->writeAttribute(KXMLBindChan,  QString::number(firstBinding->channelIndex + 1));
        }
        for (const PTColumnTypeBinding& binding : col.bindings)
        {
            if (!binding.isValid())
                continue;
            doc->writeStartElement(KXMLBinding);
            doc->writeAttribute(KXMLBindMfg,   binding.manufacturer);
            doc->writeAttribute(KXMLBindModel, binding.model);
            doc->writeAttribute(KXMLBindMode,  binding.modeName);
            doc->writeAttribute(KXMLBindChan,  QString::number(binding.channelIndex + 1));
            doc->writeEndElement();
        }

        for (const PTOption& opt : col.options)
        {
            doc->writeStartElement(KXMLOption);
            doc->writeAttribute(KXMLOptName,  opt.name);
            doc->writeAttribute(KXMLOptValue, QString::number(opt.value));
            if (!opt.resource.isEmpty())
                doc->writeAttribute(KXMLOptResource, opt.resource);
            doc->writeEndElement();
        }
        doc->writeEndElement();  // Column
    }

    for (int r = 0; r < m_rows.size(); ++r)
    {
        const PTRow& row = m_rows[r];
        doc->writeStartElement(KXMLRow);
        doc->writeAttribute(KXMLRowIndex, QString::number(r));
        doc->writeAttribute(KXMLRowName,  row.name);
        for (uchar v : row.values)
            doc->writeTextElement(KXMLV, QString::number(v));
        for (auto it = row.positions.constBegin(); it != row.positions.constEnd(); ++it)
        {
            if (!it.value().valid)
                continue;
            doc->writeStartElement(KXMLPosition);
            doc->writeAttribute(KXMLPositionX, QString::number(it.key().x()));
            doc->writeAttribute(KXMLPositionY, QString::number(it.key().y()));
            doc->writeAttribute(KXMLPositionPanDeg, QString::number(it.value().panDeg, 'f', 2));
            doc->writeAttribute(KXMLPositionTiltDeg, QString::number(it.value().tiltDeg, 'f', 2));
            doc->writeEndElement();
        }
        doc->writeEndElement();  // Row
    }

    for (int rowIdx = 0; rowIdx < m_positionOverrides.size(); ++rowIdx)
    {
        const QHash<int, PTPositionOutputLayer>& rowLayers = m_positionOverrides.at(rowIdx);
        for (auto outIt = rowLayers.constBegin(); outIt != rowLayers.constEnd(); ++outIt)
        {
            const int outputIdx = outIt.key();
            const PTPositionOutputLayer& layer = outIt.value();
            for (auto pit = layer.allOverrides.constBegin(); pit != layer.allOverrides.constEnd(); ++pit)
            {
                if (!pit.value().valid)
                    continue;
                doc->writeStartElement(KXMLPositionOverride);
                doc->writeAttribute(KXMLPosOvRow, QString::number(rowIdx));
                doc->writeAttribute(KXMLPosOvOutput, QString::number(outputIdx));
                doc->writeAttribute(KXMLPosOvSelection, QStringLiteral("-1"));
                doc->writeAttribute(KXMLPositionX, QString::number(pit.key().x()));
                doc->writeAttribute(KXMLPositionY, QString::number(pit.key().y()));
                doc->writeAttribute(KXMLPositionPanDeg, QString::number(pit.value().panDeg, 'f', 2));
                doc->writeAttribute(KXMLPositionTiltDeg, QString::number(pit.value().tiltDeg, 'f', 2));
                doc->writeEndElement();
            }
            for (int selIdx = 0; selIdx < layer.selections.size(); ++selIdx)
            {
                const PTPositionSelectionLayer& sel = layer.selections.at(selIdx);
                for (auto pit = sel.overrides.constBegin(); pit != sel.overrides.constEnd(); ++pit)
                {
                    if (!pit.value().valid)
                        continue;
                    doc->writeStartElement(KXMLPositionOverride);
                    doc->writeAttribute(KXMLPosOvRow, QString::number(rowIdx));
                    doc->writeAttribute(KXMLPosOvOutput, QString::number(outputIdx));
                    doc->writeAttribute(KXMLPosOvSelection, QString::number(selIdx));
                    if (!sel.name.isEmpty())
                        doc->writeAttribute(KXMLPosOvSelName, sel.name);
                    if (sel.color.isValid())
                        doc->writeAttribute(KXMLPosOvSelColor, sel.color.name());
                    doc->writeAttribute(KXMLPositionX, QString::number(pit.key().x()));
                    doc->writeAttribute(KXMLPositionY, QString::number(pit.key().y()));
                    doc->writeAttribute(KXMLPositionPanDeg, QString::number(pit.value().panDeg, 'f', 2));
                    doc->writeAttribute(KXMLPositionTiltDeg, QString::number(pit.value().tiltDeg, 'f', 2));
                    doc->writeEndElement();
                }
            }
        }
    }

    // Collect output data under lock, then write outside
    struct OutData {
        QString       name;
        quint32       fixtureId;
        QList<int>    groupRows;
        PTOutputScope scope;
        int           sweepPresetIndex;
        int           continuousPresetIndex;
        int           multiFxPresetIndex;
        int           secondaryRowIndex;
    };
    QVector<OutData> outData;
    outData.reserve(m_outputs.size());
    for (const PTOutput& out : m_outputs)
        outData.append({out.name, out.fixtureId, out.groupRows, out.scope,
                        out.sweepPresetIndex, out.continuousPresetIndex,
                        out.multiFxPresetIndex, out.secondaryRowIndex});

    bool isFGMode = (m_mode == PTMode::FixtureGroup || m_mode == PTMode::Position);
    lk.unlock();

    for (int o = 0; o < outData.size(); ++o)
    {
        doc->writeStartElement(KXMLOutput);
        doc->writeAttribute(KXMLOutIndex, QString::number(o));
        doc->writeAttribute(KXMLOutName,  outData[o].name);

        if (!isFGMode)
        {
            doc->writeAttribute(KXMLOutFxId, QString::number(outData[o].fixtureId));
        }
        else
        {
            doc->writeAttribute(KXMLOutScope, scopeToString(outData[o].scope));
            doc->writeAttribute(KXMLOutSweepPreset,
                                QString::number(outData[o].sweepPresetIndex));
            doc->writeAttribute(KXMLOutContinuousPreset,
                                QString::number(outData[o].continuousPresetIndex));
            doc->writeAttribute(KXMLOutMultiFxPreset,
                                QString::number(outData[o].multiFxPresetIndex));
            doc->writeAttribute(KXMLOutSecondaryRow,
                                QString::number(outData[o].secondaryRowIndex));
            if (!outData[o].groupRows.isEmpty())
            {
                QStringList rowParts;
                for (int row : outData[o].groupRows)
                    rowParts << QString::number(row);
                doc->writeAttribute(KXMLOutRows, rowParts.join(QLatin1Char(',')));
            }
        }

        auto src = (o < PTInputId::kMaxRoutableOutputs)
                ? inputSource(PTInputId::rowSelector(o)) : QSharedPointer<QLCInputSource>();
        if (!src.isNull() && src->isValid())
        {
            doc->writeStartElement(KXMLOutInput);
            saveXMLInput(doc, src);
            doc->writeEndElement();
        }

        if (isFGMode)
        {
            if (o >= PTInputId::kMaxRoutableOutputs)
            {
                doc->writeEndElement();  // Output
                continue;
            }

            auto sweepSrc = inputSource(PTInputId::transSweep(o));
            if (!sweepSrc.isNull() && sweepSrc->isValid())
            {
                doc->writeStartElement(KXMLOutTransSweepInput);
                saveXMLInput(doc, sweepSrc);
                doc->writeEndElement();
            }
            auto contSrc = inputSource(PTInputId::transContinuousBank(o));
            if (!contSrc.isNull() && contSrc->isValid())
            {
                doc->writeStartElement(KXMLOutTransContinuousInput);
                saveXMLInput(doc, contSrc);
                doc->writeEndElement();
            }
            auto multiFxSrc = inputSource(PTInputId::multiFxBank(o));
            if (!multiFxSrc.isNull() && multiFxSrc->isValid())
            {
                doc->writeStartElement(KXMLOutMultiFxInput);
                saveXMLInput(doc, multiFxSrc);
                doc->writeEndElement();
            }
            auto secSrc = inputSource(PTInputId::transSecondaryRow(o));
            if (!secSrc.isNull() && secSrc->isValid())
            {
                doc->writeStartElement(KXMLOutTransSecondaryInput);
                saveXMLInput(doc, secSrc);
                doc->writeEndElement();
            }
        }

        doc->writeEndElement();  // Output
    }

    // Global crossfade input
    auto xfSrc = inputSource(PTInputId::kCrossfade);
    if (!xfSrc.isNull() && xfSrc->isValid())
    {
        doc->writeStartElement(KXMLCrossfadeInput);
        saveXMLInput(doc, xfSrc);
        doc->writeEndElement();
    }

    auto multiFxBlendSrc = inputSource(PTInputId::kMultiFxBlend);
    if (!multiFxBlendSrc.isNull() && multiFxBlendSrc->isValid())
    {
        doc->writeStartElement(KXMLMultiFxBlendInput);
        saveXMLInput(doc, multiFxBlendSrc);
        doc->writeEndElement();
    }

    auto multiFxRestartSrc = inputSource(PTInputId::kMultiFxRestart);
    QKeySequence multiFxRestartKey;
    QKeySequence widgetFlashGateKey;
    {
        QMutexLocker keyLock(&m_stateMutex);
        multiFxRestartKey = m_multiFxRestartKey;
        widgetFlashGateKey = m_widgetFlashGateKey;
    }
    if ((!multiFxRestartSrc.isNull() && multiFxRestartSrc->isValid())
            || !multiFxRestartKey.isEmpty())
    {
        doc->writeStartElement(KXMLMultiFxRestartInput);
        savePTInputBlock(doc, multiFxRestartSrc, multiFxRestartKey);
        doc->writeEndElement();
    }

    auto widgetFlashGateSrc = inputSource(PTInputId::kWidgetFlashGate);
    if ((!widgetFlashGateSrc.isNull() && widgetFlashGateSrc->isValid())
            || !widgetFlashGateKey.isEmpty())
    {
        doc->writeStartElement(KXMLWidgetFlashGateInput);
        savePTInputBlock(doc, widgetFlashGateSrc, widgetFlashGateKey);
        doc->writeEndElement();
    }

    doc->writeEndElement();  // PluginWidget
    return true;
}

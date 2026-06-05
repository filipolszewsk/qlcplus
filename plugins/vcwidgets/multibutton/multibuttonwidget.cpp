/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonwidget.cpp — Apache 2.0 / public domain
*/

#include "multibuttonwidget.h"
#include "multibuttonconfigdialog.h"
#include "mbvalueexpr.h"

#include "functionparent.h"
#include "mastertimer.h"
#include "function.h"
#include "doc.h"
#include "qlcinputsource.h"
#include "inputoutputmap.h"
#include "vcframe.h"
#include "scene.h"
#include "universe.h"
#include "qlcfile.h"
#include "qlcconfig.h"
#include "fixture.h"
#include "genericfader.h"
#include "fadechannel.h"
#include "qlcchannel.h"
#include "scribbledialog.h"
#include "apputil.h"
#include "presettablev2multibuttoniface.h"
#include "virtualconsole.h"

#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileInfo>
#include <QImageReader>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QMap>
#include <QRandomGenerator>
#include <functional>
#include <QEventLoop>
#include <QApplication>
#include <QCursor>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOptionButton>

// ---- XML tag constants ----------------------------------------------------

static const QString KXMLRoot              = QStringLiteral("PluginWidget");
static const QString KXMLPluginId          = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal       = QStringLiteral("org.qlcplus.vcwidgets.multibutton");
static const QString KXMLWidgetMode        = QStringLiteral("WidgetMode");
static const QString KXMLWidgetLink        = QStringLiteral("WidgetLink");
static const QString KXMLWidgetLinkTarget  = QStringLiteral("TargetWidgetId");
static const QString KXMLWidgetLinkOutput  = QStringLiteral("OutputIndex");
static const QString KXMLWidgetLinkParam   = QStringLiteral("Parameter");
static const QString KXMLWidgetLiveInput   = QStringLiteral("WidgetLiveInput");
static const QString KXMLWidgetBusPolicy   = QStringLiteral("WidgetBusPolicy");
static const QString KXMLWidgetEntryAppearance = QStringLiteral("WidgetEntryAppearance");

static const int kWidgetBusPublishSuppressMs = 250;

static QString widgetBusPolicyToString(MultiButtonWidgetBusPolicy policy)
{
    return policy == MultiButtonWidgetBusPolicy::HoldOverride
            ? QStringLiteral("holdOverride") : QStringLiteral("shared");
}

static MultiButtonWidgetBusPolicy stringToWidgetBusPolicy(const QString& s)
{
    if (s.compare(QStringLiteral("holdOverride"), Qt::CaseInsensitive) == 0)
        return MultiButtonWidgetBusPolicy::HoldOverride;
    return MultiButtonWidgetBusPolicy::SharedBus;
}
static const QString KXMLCurrentIndex      = QStringLiteral("CurrentIndex");
static const QString KXMLLongPressMs       = QStringLiteral("LongPressMs");
static const QString KXMLFunction          = QStringLiteral("Function");
static const QString KXMLFunctionID        = QStringLiteral("ID");
static const QString KXMLFunctionLabel     = QStringLiteral("Label");
static const QString KXMLFunctionIconPath  = QStringLiteral("IconPath");
static const QString KXMLAddOffAtEnd       = QStringLiteral("AddOffAtEnd");
static const QString KXMLMonitorChannels   = QStringLiteral("MonitorChannelValues");
static const QString KXMLReceiveInputInactiveFramePage =
    QStringLiteral("ReceiveInputOnInactiveFramePage");
static const QString KXMLStageBeforeCommit = QStringLiteral("StageBeforeCommit");
static const QString KXMLEntrySelectAutoCommit = QStringLiteral("EntrySelectAutoCommit");
static const QString KXMLLogPresetChanges = QStringLiteral("LogPresetChanges");
static const QString KXMLCommitInput       = QStringLiteral("CommitInput");
static const QString KXMLTriggerInput      = QStringLiteral("TriggerInput");
static const QString KXMLPopupInput        = QStringLiteral("PopupInput");
static const QString KXMLAutomationTriggerInput = QStringLiteral("AutomationTriggerInput");
static const QString KXMLPresetChooseInput = QStringLiteral("PresetChooseInput");
static const QString KXMLEntrySelectInput  = QStringLiteral("EntrySelectInput");
static const QString KXMLSpreadPageInput   = QStringLiteral("SpreadPageInput");
static const QString KXMLEntryTriggerInput = QStringLiteral("EntryTriggerInput");
static const QString KXMLFunctionEntryInput = QStringLiteral("FunctionEntryInput");
static const QString KXMLFunctionEntryIndex = QStringLiteral("Index");
static const QString KXMLSpreadSlotInput   = QStringLiteral("SpreadSlotInput");
static const QString KXMLSpreadSlotIndex   = QStringLiteral("Index");
static const QString KXMLLevelFixture      = QStringLiteral("LevelFixture");
static const QString KXMLLevelFixtureID    = QStringLiteral("ID");
static const QString KXMLLevelChannels     = QStringLiteral("LevelChannels");
static const QString KXMLLevelChannel      = QStringLiteral("Channel");
static const QString KXMLLevelChannelIndex = QStringLiteral("Index");
static const QString KXMLLevelBindings     = QStringLiteral("LevelChannelBindings");
static const QString KXMLLevelBinding      = QStringLiteral("Binding");
static const QString KXMLLevelBindingFx    = QStringLiteral("FixtureID");
static const QString KXMLLevelBindingCh    = QStringLiteral("Channel");
static const QString KXMLLevelPreset       = QStringLiteral("LevelPreset");
static const QString KXMLLevelPresetLabel  = QStringLiteral("Label");
static const QString KXMLLevelPresetIcon   = QStringLiteral("IconPath");
static const QString KXMLLevelPresetColor  = QStringLiteral("Color");
static const QString KXMLLevelPresetLabelColor = QStringLiteral("LabelColor");
static const QString KXMLLevelPresetFlash  = QStringLiteral("Flash");
static const QString KXMLLevelPresetHideName = QStringLiteral("HideName");
static const QString KXMLFunctionFlash     = QStringLiteral("Flash");
static const QString KXMLFunctionLabelColor = QStringLiteral("LabelColor");
static const QString KXMLLevelPresetValues = QStringLiteral("Values");
static const QString KXMLLevelValueFormula = QStringLiteral("ValueFormula");
static const QString KXMLLevelValueFormulaIndex = QStringLiteral("Index");
static const QString KXMLLevelValueFormulaText = QStringLiteral("Text");
static const QString KXMLSpread            = QStringLiteral("Spread");
static const QString KXMLSpreadEnabled     = QStringLiteral("Enabled");
static const QString KXMLSpreadColumns     = QStringLiteral("Columns");
static const QString KXMLSpreadRows        = QStringLiteral("Rows");
static const QString KXMLSpreadHMargin       = QStringLiteral("HMargin");
static const QString KXMLSpreadVMargin       = QStringLiteral("VMargin");
static const QString KXMLSpreadTileW         = QStringLiteral("TileW");
static const QString KXMLSpreadTileH         = QStringLiteral("TileH");
static const QString KXMLSpreadPages         = QStringLiteral("Pages");
static const QString KXMLAutomation          = QStringLiteral("Automation");
static const QString KXMLAutomationEnabled   = QStringLiteral("Enabled");
// Legacy attributes kept for migration reading:
static const QString KXMLAutomationActive    = QStringLiteral("ActiveProfile");
static const QString KXMLAutomationProfile   = QStringLiteral("Profile");
static const QString KXMLAutomationName      = QStringLiteral("Name");
static const QString KXMLAutomationExcludeMask = QStringLiteral("ExcludeMask");
// Current attributes:
static const QString KXMLAutomationMode      = QStringLiteral("Mode");
static const QString KXMLAutomationStepMin   = QStringLiteral("StepMin");
static const QString KXMLAutomationStepMax   = QStringLiteral("StepMax");
static const QString KXMLAutomationMultiplier = QStringLiteral("Multiplier");
static const QString KXMLAutomationBeatOffset = QStringLiteral("BeatOffset");

static QString modeToString(MultiButtonMode mode)
{
    if (mode == MultiButtonMode::Widget)
        return QStringLiteral("Widget");
    return mode == MultiButtonMode::Level ? QStringLiteral("Level")
                                          : QStringLiteral("Function");
}

static MultiButtonMode stringToMode(const QString& s)
{
    if (s == QStringLiteral("Widget"))
        return MultiButtonMode::Widget;
    return s == QStringLiteral("Level") ? MultiButtonMode::Level
                                        : MultiButtonMode::Function;
}

static QString layoutToString(MultiButtonLayout layout)
{
    return layout == MultiButtonLayout::Spread ? QStringLiteral("Spread")
                                               : QStringLiteral("Single");
}

static MultiButtonLayout stringToLayout(const QString& s)
{
    return s == QStringLiteral("Spread") ? MultiButtonLayout::Spread
                                         : MultiButtonLayout::Single;
}

static QString automationModeToString(MultiButtonAutomationMode mode)
{
    switch (mode)
    {
        case MultiButtonAutomationMode::Random: return QStringLiteral("Random");
        case MultiButtonAutomationMode::Jump:   return QStringLiteral("Jump");
        default:                                return QStringLiteral("Next");
    }
}

static MultiButtonAutomationMode stringToAutomationMode(const QString& s)
{
    if (s == QStringLiteral("Random"))
        return MultiButtonAutomationMode::Random;
    if (s == QStringLiteral("Jump"))
        return MultiButtonAutomationMode::Jump;
    return MultiButtonAutomationMode::Next;
}

static QSharedPointer<QLCInputSource> cloneInputSource(const QSharedPointer<QLCInputSource>& src)
{
    if (src.isNull() || !src->isValid())
        return QSharedPointer<QLCInputSource>();

    QSharedPointer<QLCInputSource> copy(new QLCInputSource(src->universe(), src->channel()));
    copy->setFeedbackValue(QLCInputFeedback::LowerValue,
                          src->feedbackValue(QLCInputFeedback::LowerValue));
    copy->setFeedbackValue(QLCInputFeedback::UpperValue,
                          src->feedbackValue(QLCInputFeedback::UpperValue));
    copy->setFeedbackValue(QLCInputFeedback::MonitorValue,
                          src->feedbackValue(QLCInputFeedback::MonitorValue));
    copy->setFeedbackExtraParams(QLCInputFeedback::LowerValue,
                                 src->feedbackExtraParams(QLCInputFeedback::LowerValue));
    copy->setFeedbackExtraParams(QLCInputFeedback::UpperValue,
                                 src->feedbackExtraParams(QLCInputFeedback::UpperValue));
    copy->setFeedbackExtraParams(QLCInputFeedback::MonitorValue,
                                 src->feedbackExtraParams(QLCInputFeedback::MonitorValue));
    return copy;
}

struct EntryInputBinding
{
    QSharedPointer<QLCInputSource> source;
    QKeySequence                 key;
};

static EntryInputBinding readInputBlock(QXmlStreamReader& root, VCWidget* widget)
{
    EntryInputBinding binding;
    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCVCWidgetInput)
        {
            binding.source = widget->getXMLInput(root);
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCVCWidgetKey)
            binding.key = VCWidget::stripKeySequence(QKeySequence(root.readElementText()));
        else
            root.skipCurrentElement();
    }
    return binding;
}

static void saveInputBlock(QXmlStreamWriter* doc, const QSharedPointer<QLCInputSource>& src,
                           const QKeySequence& key)
{
    if (!src.isNull() && src->isValid())
        VCWidget::saveXMLInput(doc, src);
    if (!key.isEmpty())
        doc->writeTextElement(KXMLQLCVCWidgetKey, key.toString());
}

// ---- Construction ---------------------------------------------------------

MultiButtonWidget::MultiButtonWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(MultiButtonWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(QString());
    resize(QSize(120, 80));
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);
    connect(m_longPressTimer, &QTimer::timeout,
            this, &MultiButtonWidget::slotLongPressFired);

    m_entrySelectDismissTimer = new QTimer(this);
    m_entrySelectDismissTimer->setSingleShot(true);
    m_entrySelectDismissTimer->setInterval(500);
    connect(m_entrySelectDismissTimer, &QTimer::timeout, this, [this]() {
        closeEntrySelectPopup(true);
    });
}

MultiButtonWidget::~MultiButtonWidget()
{
    closeEntrySelectPopup(false);
    m_doc->masterTimer()->unregisterDMXSource(this);
    releaseLevelFaders();
    releaseWidgetLiveFaders();

    if (m_channelMonitorTimer)
    {
        m_channelMonitorTimer->stop();
        delete m_channelMonitorTimer;
        m_channelMonitorTimer = nullptr;
    }
    stopCurrent();
}

// ---- FunctionParent -------------------------------------------------------

FunctionParent MultiButtonWidget::functionParent() const
{
    return FunctionParent(FunctionParent::ManualVCWidget, id());
}

// ---- Mode -----------------------------------------------------------------

void MultiButtonWidget::setWidgetMode(MultiButtonMode mode)
{
    if (m_mode == mode)
        return;

    cancelEntrySelectPreview();
    closeEntrySelectPopup(false);

    if (m_mode == MultiButtonMode::Level)
        releaseLevelFaders();
    if (m_mode == MultiButtonMode::Widget)
        releaseWidgetLiveFaders();
    stopCurrent();
    m_mode = mode;
    m_iconCache.clear();
    m_currentIndex = -1;
    clearLocalStagedSelection();
    m_monitorMatchIndex = -1;
    m_lastMonitorMatchIdx = -1;
    m_lastResolvedEntryCount = -1;
    m_visualOnly   = false;
    updateDmxRegistration();
    update();
}

void MultiButtonWidget::setLevelConfig(const QList<LevelChannelBinding>& bindings,
                                       const QList<LevelPreset>& presets)
{
    const int savedIndex = m_currentIndex;

    if (m_mode == MultiButtonMode::Function && savedIndex >= 0 && !m_visualOnly)
    {
        Function* f = functionAt(savedIndex);
        if (f)
            f->stop(functionParent());
    }
    releaseLevelFaders();

    m_levelChannelBindings = bindings;
    m_levelPresets         = presets;

    for (LevelPreset& preset : m_levelPresets)
        alignLevelPresetArrays(preset, bindings.size());

    m_iconCache.clear();
    m_visualOnly = false;
    if (savedIndex >= 0 && savedIndex < m_levelPresets.size())
        m_currentIndex = savedIndex;
    else
        m_currentIndex = -1;

    reactivateLevelPreset();
    clampSpreadPageIndex();
    resizeSpreadSlotInputs();
    syncEntryInputSources();
    recalcLayoutSize();
    update();
}

void MultiButtonWidget::setWidgetEntryAppearance(const QList<LevelPreset>& appearance)
{
    m_widgetEntryAppearance = appearance;
    syncWidgetEntryAppearanceCount();
    m_iconCache.clear();
    syncEntryInputSources();
    update();
}

void MultiButtonWidget::updateDmxRegistration()
{
    m_doc->masterTimer()->unregisterDMXSource(this);

    if (m_doc->mode() != Doc::Operate)
        return;

    if (m_mode == MultiButtonMode::Level
        && !m_levelChannelBindings.isEmpty()
        && !m_levelPresets.isEmpty())
    {
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else if (m_mode == MultiButtonMode::Widget)
    {
        QSharedPointer<QLCInputSource> src = widgetLiveInputSourceResolved();
        if (!src.isNull() && src->isValid())
            m_doc->masterTimer()->registerDMXSource(this);
    }
}

void MultiButtonWidget::releaseWidgetLiveFaders()
{
    QMutexLocker lock(&m_dmxMutex);
    for (auto& fader : m_widgetLiveFaders)
    {
        if (!fader.isNull())
            fader->requestDelete();
    }
    m_widgetLiveFaders.clear();
    m_widgetLiveWriteDirty = true;
    m_widgetLiveLastUniverse = UINT_MAX;
    m_widgetLiveLastChannel = UINT_MAX;
    m_widgetLiveLastValue = 0;
    m_widgetSelectorLatchedValid = false;
    m_widgetSelectorLatchedIndex = -1;
    m_widgetBusCommittedValue = 0;
    m_widgetBusLastSeen = 0;
    m_widgetBusForceReassert = false;
}

void MultiButtonWidget::reactivateLevelPreset()
{
    updateDmxRegistration();

    if (m_doc->mode() != Doc::Operate || m_mode != MultiButtonMode::Level)
        return;

    if (m_currentIndex >= 0 && m_currentIndex < m_levelPresets.size())
        activate(m_currentIndex);
}

// ---- Function list management ---------------------------------------------

void MultiButtonWidget::setEntries(const QList<quint32>& ids,
                                   const QStringList&    labels,
                                   const QStringList&    iconPaths)
{
    stopCurrent();
    m_functionIds    = ids;
    m_functionLabels = labels;
    m_iconPaths      = iconPaths;

    while (m_functionLabels.size() < m_functionIds.size())
        m_functionLabels.append(QString());
    while (m_iconPaths.size() < m_functionIds.size())
        m_iconPaths.append(QString());

    m_iconCache.clear();
    m_currentIndex = -1;
    m_visualOnly   = false;
    cancelEntrySelectPreview();
    resetLevelWriteCache();

    while (m_functionEntryInputs.size() < m_functionIds.size())
        m_functionEntryInputs.append(QSharedPointer<QLCInputSource>());
    while (m_functionEntryInputs.size() > m_functionIds.size())
        m_functionEntryInputs.removeLast();
    while (m_functionEntryKeys.size() < m_functionIds.size())
        m_functionEntryKeys.append(QKeySequence());
    while (m_functionEntryKeys.size() > m_functionIds.size())
        m_functionEntryKeys.removeLast();
    while (m_functionEntryFlash.size() < m_functionIds.size())
        m_functionEntryFlash.append(false);
    while (m_functionEntryFlash.size() > m_functionIds.size())
        m_functionEntryFlash.removeLast();
    while (m_functionEntryLabelColors.size() < m_functionIds.size())
        m_functionEntryLabelColors.append(QColor());
    while (m_functionEntryLabelColors.size() > m_functionIds.size())
        m_functionEntryLabelColors.removeLast();

    rebuildSceneCache();
    clampSpreadPageIndex();
    syncEntryInputSources();
    recalcLayoutSize();
    update();
}

void MultiButtonWidget::setCurrentIndex(int idx)
{
    if (idx < 0 || idx >= entryCount())
        idx = -1;
    activate(idx);
}

void MultiButtonWidget::setLongPressMs(int ms)
{
    m_longPressMs = qBound(100, ms, 5000);
}

void MultiButtonWidget::setAddOffAtEnd(bool v)
{
    m_addOffAtEnd = v;
    cancelEntrySelectPreview();
    recalcLayoutSize();
    update();
}

void MultiButtonWidget::setWidgetLayout(MultiButtonLayout layout)
{
    if (m_layout == layout)
        return;
    m_layout = layout;
    recalcLayoutSize();
    update();
}

void MultiButtonWidget::setSpreadColumns(int columns)
{
    m_spreadColumns = qMax(0, columns);
    clampSpreadPageIndex();
    resizeSpreadSlotInputs();
    syncEntryInputSources();
    recalcLayoutSize();
}

void MultiButtonWidget::setSpreadRows(int rows)
{
    m_spreadRows = qMax(0, rows);
    clampSpreadPageIndex();
    resizeSpreadSlotInputs();
    syncEntryInputSources();
    recalcLayoutSize();
}

void MultiButtonWidget::setSpreadHMargin(int margin)
{
    m_spreadHMargin = qBound(0, margin, 64);
    recalcLayoutSize();
}

void MultiButtonWidget::setSpreadVMargin(int margin)
{
    m_spreadVMargin = qBound(0, margin, 64);
    recalcLayoutSize();
}

void MultiButtonWidget::setSpreadTileWidth(int width)
{
    m_spreadTileWidth = qBound(20, width, 400);
    recalcLayoutSize();
}

void MultiButtonWidget::setSpreadTileHeight(int height)
{
    m_spreadTileHeight = qBound(20, height, 400);
    recalcLayoutSize();
}

void MultiButtonWidget::setSpreadPages(int pages)
{
    m_spreadPages = qBound(0, pages, 32);
    clampSpreadPageIndex();
    resizeSpreadSlotInputs();
    syncEntryInputSources();
    recalcLayoutSize();
}

int MultiButtonWidget::totalSpreadSlots() const
{
    const int n = entryCount();
    if (n <= 0)
        return 0;
    return n + (m_addOffAtEnd ? 1 : 0);
}

int MultiButtonWidget::spreadSlotsPerPage(bool forPaging) const
{
    if (forPaging)
    {
        const int total = totalSpreadSlots();
        if (m_spreadColumns > 0 && m_spreadRows > 0)
            return m_spreadColumns * m_spreadRows;
        if (m_spreadColumns > 0 && m_spreadRows <= 0)
        {
            if (m_spreadPages > 0)
                return qMax(1, (total + m_spreadPages - 1) / m_spreadPages);
            return qMax(1, total);
        }
        if (m_spreadPages > 0)
            return qMax(1, (total + m_spreadPages - 1) / m_spreadPages);
        return qMax(1, total);
    }

    int cols = 0, rows = 0;
    resolveSpreadGrid(cols, rows);
    return cols * rows;
}

int MultiButtonWidget::spreadPageCount() const
{
    if (m_layout != MultiButtonLayout::Spread)
        return 1;

    const int total = totalSpreadSlots();
    if (m_spreadColumns > 0 && m_spreadRows > 0)
    {
        const int fixedSlots = qMax(1, m_spreadColumns * m_spreadRows);
        const int minPages = total > 0 ? qMax(1, (total + fixedSlots - 1) / fixedSlots) : 1;
        return m_spreadPages > 0 ? qMax(m_spreadPages, minPages) : minPages;
    }

    if (m_spreadPages > 0)
        return qMax(1, m_spreadPages);

    const int spp = spreadSlotsPerPage(true);
    if (total <= 0 || spp <= 0)
        return 1;
    return qMax(1, (total + spp - 1) / spp);
}

bool MultiButtonWidget::spreadPagingActive() const
{
    return m_layout == MultiButtonLayout::Spread && spreadPageCount() > 1;
}

void MultiButtonWidget::clampSpreadPageIndex()
{
    const int pc = spreadPageCount();
    m_spreadPageIndex = qBound(0, m_spreadPageIndex, qMax(0, pc - 1));
}

void MultiButtonWidget::resizeSpreadSlotInputs()
{
    const int n = qMin(MBInputId::kMaxSpreadSlots, spreadSlotsPerPage(true));
    while (m_spreadSlotInputs.size() < n)
        m_spreadSlotInputs.append(QSharedPointer<QLCInputSource>());
    while (m_spreadSlotInputs.size() > n)
        m_spreadSlotInputs.removeLast();
    while (m_spreadSlotKeys.size() < n)
        m_spreadSlotKeys.append(QKeySequence());
    while (m_spreadSlotKeys.size() > n)
        m_spreadSlotKeys.removeLast();
}

int MultiButtonWidget::entryInputLocalSlot(int globalRow) const
{
    if (!spreadPagingActive() || globalRow < 0)
        return globalRow;
    const int spp = spreadSlotsPerPage(true);
    if (spp <= 0)
        return globalRow;
    return globalRow % spp;
}

QSharedPointer<QLCInputSource> MultiButtonWidget::entryInputSource(int idx) const
{
    if (idx < 0)
        return QSharedPointer<QLCInputSource>();

    if (m_mode == MultiButtonMode::Level)
    {
        if (idx >= m_levelPresets.size())
            return QSharedPointer<QLCInputSource>();
        return m_levelPresets.at(idx).entryInput;
    }
    if (m_mode == MultiButtonMode::Widget)
    {
        if (idx >= m_widgetEntryAppearance.size())
            return QSharedPointer<QLCInputSource>();
        return m_widgetEntryAppearance.at(idx).entryInput;
    }

    if (idx >= m_functionEntryInputs.size())
        return QSharedPointer<QLCInputSource>();
    return m_functionEntryInputs.at(idx);
}

void MultiButtonWidget::setEntryInputSource(int idx, QSharedPointer<QLCInputSource> src)
{
    if (idx < 0)
        return;

    if (m_mode == MultiButtonMode::Level)
    {
        if (idx >= m_levelPresets.size())
            return;
        m_levelPresets[idx].entryInput = src;
    }
    else if (m_mode == MultiButtonMode::Widget)
    {
        syncWidgetEntryAppearanceCount();
        if (idx >= m_widgetEntryAppearance.size())
            return;
        m_widgetEntryAppearance[idx].entryInput = src;
    }
    else
    {
        while (m_functionEntryInputs.size() <= idx)
            m_functionEntryInputs.append(QSharedPointer<QLCInputSource>());
        m_functionEntryInputs[idx] = src;
    }

    if (idx < MBInputId::kMaxEntryInputs)
        assignInputSource(src, MBInputId::entryTrigger(idx));
}

void MultiButtonWidget::setSpreadSlotInputs(const QList<QSharedPointer<QLCInputSource>>& inputs)
{
    m_spreadSlotInputs = inputs;
    resizeSpreadSlotInputs();
    syncEntryInputSources();
}

QSharedPointer<QLCInputSource> MultiButtonWidget::spreadSlotInput(int localSlot) const
{
    if (localSlot < 0 || localSlot >= m_spreadSlotInputs.size())
        return QSharedPointer<QLCInputSource>();
    return m_spreadSlotInputs.at(localSlot);
}

void MultiButtonWidget::setSpreadSlotInput(int localSlot, QSharedPointer<QLCInputSource> src)
{
    if (localSlot < 0 || localSlot >= MBInputId::kMaxSpreadSlots)
        return;
    while (m_spreadSlotInputs.size() <= localSlot)
        m_spreadSlotInputs.append(QSharedPointer<QLCInputSource>());
    m_spreadSlotInputs[localSlot] = src;
    if (localSlot < m_spreadSlotInputs.size())
        assignInputSource(src, MBInputId::spreadSlot(localSlot));
}

QKeySequence MultiButtonWidget::entryKeySource(int idx) const
{
    if (idx < 0)
        return QKeySequence();

    if (m_mode == MultiButtonMode::Level)
    {
        if (idx >= m_levelPresets.size())
            return QKeySequence();
        return m_levelPresets.at(idx).entryKey;
    }
    if (m_mode == MultiButtonMode::Widget)
    {
        if (idx >= m_widgetEntryAppearance.size())
            return QKeySequence();
        return m_widgetEntryAppearance.at(idx).entryKey;
    }

    if (idx >= m_functionEntryKeys.size())
        return QKeySequence();
    return m_functionEntryKeys.at(idx);
}

void MultiButtonWidget::setEntryKeySource(int idx, const QKeySequence& key)
{
    if (idx < 0)
        return;

    const QKeySequence stripped = stripKeySequence(key);

    if (m_mode == MultiButtonMode::Level)
    {
        if (idx >= m_levelPresets.size())
            return;
        m_levelPresets[idx].entryKey = stripped;
    }
    else if (m_mode == MultiButtonMode::Widget)
    {
        syncWidgetEntryAppearanceCount();
        if (idx < m_widgetEntryAppearance.size())
            m_widgetEntryAppearance[idx].entryKey = stripped;
    }
    else
    {
        while (m_functionEntryKeys.size() <= idx)
            m_functionEntryKeys.append(QKeySequence());
        m_functionEntryKeys[idx] = stripped;
    }
}

void MultiButtonWidget::setSpreadSlotKeys(const QList<QKeySequence>& keys)
{
    m_spreadSlotKeys = keys;
    resizeSpreadSlotInputs();
}

void MultiButtonWidget::setFunctionEntryKeys(const QList<QKeySequence>& keys)
{
    m_functionEntryKeys = keys;
}

QKeySequence MultiButtonWidget::spreadSlotKey(int localSlot) const
{
    if (localSlot < 0 || localSlot >= m_spreadSlotKeys.size())
        return QKeySequence();
    return m_spreadSlotKeys.at(localSlot);
}

void MultiButtonWidget::setSpreadSlotKey(int localSlot, const QKeySequence& key)
{
    if (localSlot < 0 || localSlot >= MBInputId::kMaxSpreadSlots)
        return;
    while (m_spreadSlotKeys.size() <= localSlot)
        m_spreadSlotKeys.append(QKeySequence());
    m_spreadSlotKeys[localSlot] = stripKeySequence(key);
}

void MultiButtonWidget::syncEntryInputSources()
{
    for (int s = 0; s < MBInputId::kMaxSpreadSlots; ++s)
        setInputSource(QSharedPointer<QLCInputSource>(), MBInputId::spreadSlot(s));
    for (int i = 0; i < MBInputId::kMaxEntryInputs; ++i)
        setInputSource(QSharedPointer<QLCInputSource>(), MBInputId::entryTrigger(i));

    resizeSpreadSlotInputs();
    if (spreadPagingActive())
    {
        for (int s = 0; s < m_spreadSlotInputs.size(); ++s)
        {
            const QSharedPointer<QLCInputSource>& src = m_spreadSlotInputs.at(s);
            if (!src.isNull() && src->isValid())
                assignInputSource(src, MBInputId::spreadSlot(s));
        }
    }

    const int n = entryCount();
    for (int i = 0; i < n && i < MBInputId::kMaxEntryInputs; ++i)
    {
        const QSharedPointer<QLCInputSource> src = entryInputSource(i);
        if (!src.isNull() && src->isValid())
            assignInputSource(src, MBInputId::entryTrigger(i));
    }

    syncAllInputSourcePages();
}

void MultiButtonWidget::activateFromGlobalSlot(int globalSlot)
{
    const int total = totalSpreadSlots();
    if (globalSlot < 0 || globalSlot >= total)
        return;

    if (m_addOffAtEnd && globalSlot == total - 1)
    {
        if (stagingActive() || widgetLinkUsesInternalStaging())
            stageEntry(-1);
        else
        {
            stopCurrent();
            updateFeedback();
            update();
        }
        return;
    }

    if (globalSlot < entryCount())
    {
        if (stagingActive() || widgetLinkUsesInternalStaging())
            stageEntry(globalSlot);
        else
            activate(globalSlot);
    }
}

int MultiButtonWidget::spreadTileCount() const
{
    const int total = totalSpreadSlots();
    if (total <= 0)
        return 0;

    if (!spreadPagingActive())
        return total;

    const int spp = spreadSlotsPerPage(true);
    const int pageStart = m_spreadPageIndex * spp;
    if (pageStart >= total)
        return 0;
    return qMin(spp, total - pageStart);
}

void MultiButtonWidget::resolveSpreadGrid(int& cols, int& rows) const
{
    if (spreadPagingActive())
    {
        const int tilesOnPage = spreadTileCount();
        if (m_spreadColumns <= 0)
        {
            cols = qMax(1, tilesOnPage);
            rows = 1;
        }
        else
        {
            cols = m_spreadColumns;
            rows = m_spreadRows > 0 ? m_spreadRows
                                    : qMax(1, (tilesOnPage + cols - 1) / cols);
        }
        return;
    }

    const int total = totalSpreadSlots();
    if (total <= 0)
    {
        cols = 1;
        rows = 1;
        return;
    }

    int c = m_spreadColumns;
    int r = m_spreadRows;

    if (c <= 0 && r <= 0)
    {
        c = total;
        r = 1;
    }
    else if (c > 0 && r <= 0)
    {
        r = (total + c - 1) / c;
    }
    else if (c <= 0 && r > 0)
    {
        c = (total + r - 1) / r;
    }

    cols = qMax(1, c);
    rows = qMax(1, r);
}

QSize MultiButtonWidget::spreadTotalSize() const
{
    int cols = 0, rows = 0;
    resolveSpreadGrid(cols, rows);

    const int w = cols * m_spreadTileWidth + qMax(0, cols - 1) * m_spreadHMargin;
    const int h = rows * m_spreadTileHeight + qMax(0, rows - 1) * m_spreadVMargin;

    const int titleH = caption().isEmpty() ? 0 : 19;
    return QSize(qMax(40, w), qMax(40, h + titleH));
}

QSize MultiButtonWidget::singleButtonSize() const
{
    const int titleH = caption().isEmpty() ? 0 : 19;
    return QSize(qMax(40, m_spreadTileWidth),
                 qMax(40, m_spreadTileHeight + titleH));
}

void MultiButtonWidget::recalcLayoutSize()
{
    if (m_layout == MultiButtonLayout::Spread)
        resize(spreadTotalSize());
    else
        resize(singleButtonSize());
}

bool MultiButtonWidget::widgetLinkEntryCountDynamic() const
{
    return m_mode == MultiButtonMode::Widget
            && m_widgetTargetId != VCWidget::invalidId();
}

void MultiButtonWidget::syncDynamicEntryCountLayout()
{
    if (!widgetLinkEntryCountDynamic())
        return;

    const int count = entryCount();
    if (count == m_lastResolvedEntryCount)
        return;

    m_lastResolvedEntryCount = count;
    syncWidgetEntryAppearanceCount();

    if (m_currentIndex >= count)
        m_currentIndex = -1;
    if (m_stagedIndex >= count)
        clearLocalStagedSelection();
    if (m_monitorMatchIndex >= count)
        m_monitorMatchIndex = -1;
    if (m_lastMonitorMatchIdx >= count)
        m_lastMonitorMatchIdx = -1;
    if (m_entrySelectPreviewIndex >= count)
    {
        m_entrySelectPreviewIndex = -1;
        m_entrySelectPreviewActive = false;
    }

    clampSpreadPageIndex();
    resizeSpreadSlotInputs();
    syncEntryInputSources();
    recalcLayoutSize();
    updateFeedback();
    update();
}

QVector<SpreadTileInfo> MultiButtonWidget::computeSpreadTiles() const
{
    QVector<SpreadTileInfo> tiles;
    const int totalAll = totalSpreadSlots();
    if (totalAll <= 0)
        return tiles;

    int cols = 0, rows = 0;
    resolveSpreadGrid(cols, rows);

    const int maxSlots = cols * rows;
    const int titleH = caption().isEmpty() ? 0 : 19;
    const int y0 = titleH;

    if (spreadPagingActive())
    {
        const int spp = spreadSlotsPerPage(true);
        const int pageStart = m_spreadPageIndex * spp;

        for (int local = 0; local < spp && (pageStart + local) < totalAll; ++local)
        {
            const int globalSlot = pageStart + local;
            const int col = local % cols;
            const int row = local / cols;

            SpreadTileInfo info;
            const bool isOff = m_addOffAtEnd && (globalSlot == totalAll - 1);
            info.index = isOff ? -1 : globalSlot;
            info.rect = QRect(col * (m_spreadTileWidth + m_spreadHMargin),
                              y0 + row * (m_spreadTileHeight + m_spreadVMargin),
                              m_spreadTileWidth,
                              m_spreadTileHeight);
            tiles.append(info);
        }
        return tiles;
    }

    for (int slot = 0; slot < qMin(totalAll, maxSlots); ++slot)
    {
        const int col = slot % cols;
        const int row = slot / cols;

        SpreadTileInfo info;
        const bool isOff = m_addOffAtEnd && (slot == totalAll - 1);
        info.index = isOff ? -1 : slot;
        info.rect = QRect(col * (m_spreadTileWidth + m_spreadHMargin),
                          y0 + row * (m_spreadTileHeight + m_spreadVMargin),
                          m_spreadTileWidth,
                          m_spreadTileHeight);
        tiles.append(info);
    }

    return tiles;
}

int MultiButtonWidget::spreadHitTest(const QPoint& pos) const
{
    for (const SpreadTileInfo& tile : computeSpreadTiles())
    {
        if (tile.rect.contains(pos))
            return tile.index;
    }
    return -2;
}

QString MultiButtonWidget::tileCaption(int idx) const
{
    if (idx < 0)
        return tr("OFF");

    if (m_mode == MultiButtonMode::Level)
        return levelPresetDisplayName(idx);

    const QString lbl = entryLabel(idx);
    if (!lbl.isEmpty())
        return lbl;

    if (m_mode == MultiButtonMode::Widget)
        return tr("Preset %1").arg(idx + 1);

    Function* f = functionAt(idx);
    return f ? f->name() : tr("?");
}

void MultiButtonWidget::setIconForEntry(int idx, const QString& path)
{
    if (idx < 0 || idx >= entryCount())
        return;

    if (m_mode == MultiButtonMode::Function)
    {
        while (m_iconPaths.size() < m_functionIds.size())
            m_iconPaths.append(QString());
        m_iconPaths[idx] = path;
    }
    else
    {
        if (LevelPreset* appearance = mutableEntryAppearancePreset(idx))
            appearance->iconPath = path;
    }

    m_iconCache.remove(idx);
    update();
}

void MultiButtonWidget::setReceiveInputOnInactiveFramePage(bool enable)
{
    m_receiveInputOnInactiveFramePage = enable;
}

void MultiButtonWidget::setEntrySelectAutoCommit(bool enable)
{
    m_entrySelectAutoCommit = enable;
}

void MultiButtonWidget::setLogPresetChanges(bool enable)
{
    m_logPresetChanges = enable;
}

void MultiButtonWidget::setStageBeforeCommit(bool enable)
{
    if (m_stageBeforeCommit == enable)
        return;
    m_stageBeforeCommit = enable;
    if (!enable)
        clearLocalStagedSelection();
    updateChannelMonitorTimerInterval();
    update();
}

void MultiButtonWidget::updateChannelMonitorTimerInterval()
{
    if (!m_channelMonitorTimer)
        return;
    const int intervalMs = stagingActive() ? 100 : 200;
    m_channelMonitorTimer->setInterval(intervalMs);
}

void MultiButtonWidget::setMonitorChannelValues(bool enable)
{
    if (m_monitorChannelValues == enable) return;
    m_monitorChannelValues = enable;

    if (enable)
    {
        rebuildSceneCache();
        if (!m_channelMonitorTimer)
        {
            m_channelMonitorTimer = new QTimer(this);
            connect(m_channelMonitorTimer, &QTimer::timeout,
                    this, &MultiButtonWidget::slotCheckChannelValues);
        }
        updateChannelMonitorTimerInterval();
        if (mode() == Doc::Operate)
            m_channelMonitorTimer->start();
    }
    else
    {
        if (m_channelMonitorTimer)
        {
            m_channelMonitorTimer->stop();
            delete m_channelMonitorTimer;
            m_channelMonitorTimer = nullptr;
        }
    }
}

// ---- DMXSource ------------------------------------------------------------

void MultiButtonWidget::releaseLevelFaders()
{
    foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
    {
        if (!fader.isNull())
            fader->requestDelete();
    }
    m_fadersMap.clear();
    resetLevelWriteCache();
}

void MultiButtonWidget::resetLevelWriteCache()
{
    m_lastWrittenPresetIndex = -1;
    m_lastWrittenPresetValues.clear();
}

void MultiButtonWidget::alignLevelPresetArrays(LevelPreset& preset, int bindingCount)
{
    const int n = qMax(0, bindingCount);
    while (preset.values.size() < n)
        preset.values.append(0);
    while (preset.values.size() > n)
        preset.values.removeLast();
    while (preset.valueFormulas.size() < n)
        preset.valueFormulas.append(QString());
    while (preset.valueFormulas.size() > n)
        preset.valueFormulas.removeLast();
}

quint8 MultiButtonWidget::resolvedPresetChannelValue(const LevelPreset& preset, int channelIndex,
                                                     const QList<Universe*>& universes) const
{
    if (channelIndex >= 0 && channelIndex < preset.valueFormulas.size())
    {
        const QString formula = preset.valueFormulas.at(channelIndex).trimmed();
        if (!formula.isEmpty() && mbValueExprLooksLikeFormula(formula))
        {
            MbValueExpr expr;
            QString err;
            if (mbParseValueExpr(formula, expr, &err))
                return mbEvaluateValueExpr(expr, universes, &err);
        }
    }
    if (channelIndex >= 0 && channelIndex < preset.values.size())
        return preset.values.at(channelIndex);
    return 0;
}

QList<uchar> MultiButtonWidget::resolvedPresetValues(const LevelPreset& preset,
                                                     const QList<Universe*>& universes) const
{
    QList<uchar> out;
    const int n = m_levelChannelBindings.size();
    out.reserve(n);
    for (int i = 0; i < n; ++i)
        out.append(resolvedPresetChannelValue(preset, i, universes));
    return out;
}

PresetTableV2MultiButtonTargetIface* MultiButtonWidget::widgetLinkTarget() const
{
    if (m_widgetTargetId == VCWidget::invalidId())
        return nullptr;
    VirtualConsole* vc = VirtualConsole::instance();
    if (!vc)
        return nullptr;
    return qobject_cast<PresetTableV2MultiButtonTargetIface*>(vc->widget(m_widgetTargetId));
}

QSharedPointer<QLCInputSource> MultiButtonWidget::widgetLiveInputSourceResolved() const
{
    if (m_mode != MultiButtonMode::Widget)
        return QSharedPointer<QLCInputSource>();

    return (!m_widgetLiveInputSource.isNull() && m_widgetLiveInputSource->isValid())
            ? m_widgetLiveInputSource : QSharedPointer<QLCInputSource>();
}

void MultiButtonWidget::syncWidgetLiveInputSourceToTarget()
{
    setInputSource(cloneInputSource(m_widgetLiveInputSource), widgetRecallInputSourceId);
}

void MultiButtonWidget::markWidgetSelectorPublishPending()
{
    QMutexLocker lock(&m_dmxMutex);
    m_widgetLiveWriteDirty = true;
}

void MultiButtonWidget::setWidgetSelectorLatchedIndex(int idx)
{
    QMutexLocker lock(&m_dmxMutex);
    m_widgetSelectorLatchedValid = true;
    m_widgetSelectorLatchedIndex = idx;
    m_widgetLiveWriteDirty = true;
}

uchar MultiButtonWidget::selectorIndexToBusValue(int selectorIdx)
{
    return selectorIdx < 0 ? 0 : uchar(qBound(0, selectorIdx + 1, 255));
}

bool MultiButtonWidget::widgetBusInPublishGrace() const
{
    QMutexLocker lock(&m_dmxMutex);
    return m_publishSuppressTimer.isValid()
            && m_publishSuppressTimer.elapsed() < kWidgetBusPublishSuppressMs;
}

int MultiButtonWidget::widgetBusTargetIndex(PresetTableV2MultiButtonTargetIface* target) const
{
    if (!target)
        return -1;

    if (widgetLinkUsesInternalStaging())
    {
        if (target->multiButtonHasStagedIndex(m_widgetOutputIndex, m_widgetParameter))
            return target->multiButtonStagedIndex(m_widgetOutputIndex, m_widgetParameter);
    }
    return target->multiButtonLiveIndex(m_widgetOutputIndex, m_widgetParameter);
}

void MultiButtonWidget::commitWidgetBusFromUi(int selectorIdx)
{
    const uchar busValue = selectorIndexToBusValue(selectorIdx);
    QMutexLocker lock(&m_dmxMutex);
    m_lastPublishedValue = busValue;
    m_widgetBusCommittedValue = busValue;
    m_widgetBusLastSeen = busValue;
    m_widgetBusForceReassert = true;
    m_publishSuppressTimer.start();
    m_widgetLiveWriteDirty = true;
    m_widgetSelectorLatchedValid = false;
    m_widgetSelectorLatchedIndex = -1;
}

void MultiButtonWidget::publishSelectorToBus(int selectorIdx)
{
    if (m_mode != MultiButtonMode::Widget)
        return;

    QSharedPointer<QLCInputSource> src = widgetLiveInputSourceResolved();
    if (src.isNull() || !src->isValid() || !m_doc)
        return;

    commitWidgetBusFromUi(selectorIdx);

    const uchar busValue = selectorIndexToBusValue(selectorIdx);
    m_doc->inputOutputMap()->sendFeedBack(
            src->universe(), src->channel(), busValue,
            src->feedbackExtraParams(QLCInputFeedback::UpperValue));

    QSharedPointer<QLCInputSource> recall = inputSource(widgetRecallInputSourceId);
    if (!recall.isNull() && recall->isValid() && recall->needsUpdate())
        recall->updateOuputValue(busValue);
}

void MultiButtonWidget::writeWidgetBusChannel(QList<Universe*>& universes,
                                              quint32 universe, quint32 channel,
                                              uchar value, Universe::FaderPriority priority,
                                              bool forceLtp)
{
    if (int(universe) >= universes.size() || channel >= 512 || !universes[int(universe)])
        return;

    QMutexLocker lock(&m_dmxMutex);

    const bool sourceChanged = (m_widgetLiveLastUniverse != universe
                                || m_widgetLiveLastChannel != channel);

    const QList<quint32> liveFaderUniverses = m_widgetLiveFaders.keys();
    for (quint32 oldUniverse : liveFaderUniverses)
    {
        if (oldUniverse == universe)
            continue;
        QSharedPointer<GenericFader> oldFader = m_widgetLiveFaders.take(oldUniverse);
        if (!oldFader.isNull())
            oldFader->requestDelete();
    }

    QSharedPointer<GenericFader> fader =
            m_widgetLiveFaders.value(universe, QSharedPointer<GenericFader>());
    if (fader.isNull())
    {
        fader = universes[int(universe)]->requestFader(priority);
        fader->adjustIntensity(intensity());
        m_widgetLiveFaders.insert(universe, fader);
    }
    else if (fader->priority() != priority)
    {
        universes[int(universe)]->requestFaderPriority(fader, priority);
    }
    if (sourceChanged)
        fader->removeAll();

    FadeChannel* fc = fader->getChannelFader(m_doc, universes[int(universe)],
                                             Fixture::invalidId(), channel);
    if (fc->universe() == Universe::invalid())
    {
        fader->remove(fc);
        return;
    }

    fc->removeFlag(FadeChannel::AutoRemove);
    if (forceLtp)
        fc->addFlag(FadeChannel::Override);
    else
        fc->removeFlag(FadeChannel::Override);
    fc->setStart(value);
    fc->setCurrent(value);
    fc->setTarget(value);
    fc->setReady(false);
    fc->setElapsed(0);
    m_widgetLiveLastUniverse = universe;
    m_widgetLiveLastChannel = channel;
    m_widgetLiveLastValue = value;
    m_widgetLiveWriteDirty = false;
}

void MultiButtonWidget::applyWidgetRecallInput(uchar value)
{
    if (m_mode != MultiButtonMode::Widget)
        return;

    if (widgetBusInPublishGrace())
        return;

    PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
    if (!target)
        return;

    const int idx = value == 0 ? -1 : int(value) - 1;
    const bool ok = widgetLinkUsesInternalStaging()
            ? target->multiButtonActivateStaged(m_widgetOutputIndex, m_widgetParameter, idx)
            : target->multiButtonActivate(m_widgetOutputIndex, m_widgetParameter, idx);
    if (!ok)
        return;

    {
        QMutexLocker lock(&m_dmxMutex);
        m_widgetBusCommittedValue = value;
        m_widgetBusLastSeen = value;
    }

    syncWidgetLinkLiveStagedState();
    updateFeedback();
    update();
}

void MultiButtonWidget::writeDMX(MasterTimer* /*timer*/, QList<Universe*> universes)
{
    if (m_mode == MultiButtonMode::Widget)
    {
        PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
        if (!target)
            return;

        if (syncWidgetLinkLiveStagedState())
            update();

        QSharedPointer<QLCInputSource> src = widgetLiveInputSourceResolved();
        if (src.isNull() || !src->isValid())
        {
            releaseWidgetLiveFaders();
            return;
        }
        const quint32 universe = src->universe();
        const quint32 channel = src->channel();
        if (int(universe) >= universes.size() || channel >= 512 || !universes[int(universe)])
            return;

        const int selectorIdx = widgetBusTargetIndex(target);
        const uchar targetValue = selectorIndexToBusValue(selectorIdx);
        const uchar busValue = universes[int(universe)]->preGMValue(int(channel));

        if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
        {
            bool forceReassert = false;
            bool inGrace = false;
            uchar lastSeen = 0;
            {
                QMutexLocker lock(&m_dmxMutex);
                forceReassert = m_widgetBusForceReassert;
                inGrace = m_publishSuppressTimer.isValid()
                        && m_publishSuppressTimer.elapsed() < kWidgetBusPublishSuppressMs;
                lastSeen = m_widgetBusLastSeen;
            }

            if (forceReassert)
            {
                releaseWidgetLiveFaders();
                QMutexLocker lock(&m_dmxMutex);
                m_widgetBusForceReassert = false;
            }

            if (!inGrace && !forceReassert && busValue != lastSeen)
            {
                applyWidgetRecallInput(busValue);
                return;
            }

            writeWidgetBusChannel(universes, universe, channel, targetValue,
                                  Universe::Auto, false);
            {
                QMutexLocker lock(&m_dmxMutex);
                m_widgetBusCommittedValue = targetValue;
                m_widgetBusLastSeen = targetValue;
            }
            return;
        }

        QMutexLocker lock(&m_dmxMutex);

        int holdIdx = m_widgetSelectorLatchedValid
                ? m_widgetSelectorLatchedIndex : selectorIdx;
        const uchar value = selectorIndexToBusValue(holdIdx);
        lock.unlock();
        writeWidgetBusChannel(universes, universe, channel, value,
                              Universe::Override, true);
        return;
    }

    QMutexLocker lock(&m_dmxMutex);

    if (m_mode != MultiButtonMode::Level)
        return;
    const int dmxIdx = levelDmxPresetIndex();
    if (dmxIdx < 0 || dmxIdx >= m_levelPresets.size())
        return;
    if (m_levelChannelBindings.isEmpty())
        return;

    const LevelPreset& preset = m_levelPresets.at(dmxIdx);
    const QList<uchar> resolved = resolvedPresetValues(preset, universes);
    const bool liveFormulas = presetUsesLiveFormulas(preset);
    const bool presetChanged = (dmxIdx != m_lastWrittenPresetIndex
                                || (liveFormulas && m_lastWrittenPresetValues != resolved));
    if (!presetChanged)
        return;

    for (int i = 0; i < m_levelChannelBindings.size() && i < resolved.size(); ++i)
    {
        const LevelChannelBinding& b = m_levelChannelBindings.at(i);
        Fixture* fxi = m_doc->fixture(b.fixtureId);
        if (!fxi || b.channel >= fxi->channels())
            continue;

        quint32 universe = fxi->universe();
        if ((int) universe >= universes.size())
            continue;

        QSharedPointer<GenericFader> fader = m_fadersMap.value(universe, QSharedPointer<GenericFader>());
        if (fader.isNull())
        {
            fader = universes.at(universe)->requestFader(Universe::Auto);
            fader->adjustIntensity(intensity());
            m_fadersMap[universe] = fader;
        }

        FadeChannel* fc = fader->getChannelFader(m_doc, universes.at(universe),
                                                 b.fixtureId, b.channel);
        if (fc->universe() == Universe::invalid())
        {
            fader->remove(fc);
            continue;
        }

        const QLCChannel* qlcch = fxi->channel(b.channel);
        if (qlcch && qlcch->group() != QLCChannel::Intensity)
            fc->addFlag(FadeChannel::AutoRemove);

        const uchar target = resolved.at(i);
        fc->setStart(fc->current());
        fc->setTarget(target);
        fc->setReady(false);
        fc->setElapsed(0);
    }

    m_lastWrittenPresetIndex  = dmxIdx;
    m_lastWrittenPresetValues = resolved;
}

bool MultiButtonWidget::entryIsFlash(int idx) const
{
    if (idx < 0 || idx >= entryCount())
        return false;

    if (m_mode == MultiButtonMode::Level)
    {
        if (idx >= m_levelPresets.size())
            return false;
        return m_levelPresets.at(idx).flashOnActivate;
    }

    if (m_mode == MultiButtonMode::Widget)
        return false;

    return idx < m_functionEntryFlash.size() && m_functionEntryFlash.at(idx);
}

void MultiButtonWidget::beginFlashHold(int idx)
{
    if (idx < 0 || idx >= entryCount() || !entryIsFlash(idx))
        return;
    if (m_flashHoldIndex >= 0)
        return;

    m_restoreIndex   = m_currentIndex;
    m_flashHoldIndex = idx;

    if (m_mode == MultiButtonMode::Function)
    {
        Function* f = functionAt(idx);
        if (f != nullptr && m_doc->mode() == Doc::Operate)
            f->flash(m_doc->masterTimer(), false, false);
    }
    else
    {
        resetLevelWriteCache();
        updateDmxRegistration();
    }

    update();
}

void MultiButtonWidget::endFlashHold()
{
    if (m_flashHoldIndex < 0)
        return;

    const int held = m_flashHoldIndex;
    m_flashHoldIndex = -1;
    const int restore = m_restoreIndex;
    m_restoreIndex = -1;

    if (m_mode == MultiButtonMode::Function)
    {
        Function* f = functionAt(held);
        if (f != nullptr && m_doc->mode() == Doc::Operate)
            f->unFlash(m_doc->masterTimer());
    }
    else
    {
        resetLevelWriteCache();
        if (restore >= 0 && restore < entryCount())
            activate(restore);
        else
            stopCurrent();
    }

    update();
}

int MultiButtonWidget::levelDmxPresetIndex() const
{
    if (m_flashHoldIndex >= 0 && m_flashHoldIndex < m_levelPresets.size())
        return m_flashHoldIndex;
    return m_currentIndex;
}

/** Flash emblem (same asset as VCButton). Coordinates are local to the tile rect. */
static void drawFlashEmblem(QPainter& p, const QRect& rect)
{
    const QPixmap flashPx(QStringLiteral(":/flash.png"));
    if (flashPx.isNull())
        return;

    const int size = qBound(10, qMin(rect.width(), rect.height()) / 4, 16);
    const QPixmap scaled = flashPx.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    p.drawPixmap(rect.right() - scaled.width() - 2, rect.top() + 2, scaled);
}

/** VC Button style #3 borders (vcbutton.cpp paintEvent). Local coords (0,0) = top-left of tile. */
static void drawVcButtonStyle3Border(QPainter& painter, const QRect& rect,
                                     bool active, bool monitoring)
{
    painter.setBrush(Qt::NoBrush);
    const int w = rect.width();
    const int h = rect.height();

    if (!active)
    {
        painter.setPen(QPen(QColor(160, 160, 160, 255), 3));
        painter.drawRoundedRect(1, 1, w - 2, h - 2, 3, 3);
        return;
    }

    const int borderWidth = (w > 80) ? 3 : 2;
    painter.setPen(QPen(QColor(20, 20, 20, 255), borderWidth * 2));
    painter.drawRoundedRect(borderWidth, borderWidth,
                            w - borderWidth * 2, h - borderWidth * 2,
                            borderWidth + 1, borderWidth + 1);
    if (monitoring)
        painter.setPen(QPen(QColor(255, 170, 0, 255), borderWidth));
    else
        painter.setPen(QPen(QColor(0, 230, 0, 255), borderWidth));
    painter.drawRoundedRect(borderWidth, borderWidth,
                            w - borderWidth * 2, h - borderWidth * 2,
                            borderWidth, borderWidth);
}

/** Same paint sequence as VCButton::paintEvent — Fusion style + CE_PushButton + style #3. */
static void paintVcButtonSurface(QPainter& p, const QWidget* paletteHost,
                                 const QRect& rect, const QColor& buttonColorOverride,
                                 bool sunken, bool borderActive, bool liveBorder,
                                 bool drawStyle3Border)
{
    QStyleOptionButton opt;
    opt.initFrom(paletteHost);
    opt.rect = rect;
    opt.features = QStyleOptionButton::None;
    opt.state = QStyle::State_Enabled;
    opt.state |= sunken ? QStyle::State_Sunken : QStyle::State_Raised;

    if (buttonColorOverride.isValid())
    {
        QPalette pal = opt.palette;
        pal.setColor(QPalette::Button, buttonColorOverride);
        opt.palette = pal;
    }

    QStyle* const btnStyle = AppUtil::saneStyle();
    btnStyle->drawControl(QStyle::CE_PushButton, &opt, &p,
                          const_cast<QWidget*>(paletteHost));

    if (drawStyle3Border)
        drawVcButtonStyle3Border(p, rect, borderActive, liveBorder);
}

void MultiButtonWidget::paintTileBackground(QPainter& p, const QRect& rect, int tileIndex,
                                            bool isLive, bool isStaged, bool isPressed,
                                            QColor& outBg) const
{
    QColor buttonColor;
    bool hasColorOverride = false;

    if (tileIndex >= 0)
    {
        const LevelPreset* appearance = entryAppearancePreset(tileIndex);
        if (appearance && appearance->color.isValid())
        {
            buttonColor = appearance->color;
            hasColorOverride = true;
        }
    }
    else if (tileIndex < 0)
    {
        buttonColor = palette().mid().color().lighter(130);
        hasColorOverride = true;
    }

    if (!hasColorOverride)
        outBg = defaultTileBackground();
    else
        outBg = buttonColor;

    const bool sunken = isPressed || isLive || isStaged;
    const bool borderActive = isLive || isStaged;
    paintVcButtonSurface(p, this, rect,
                         hasColorOverride ? buttonColor : QColor(),
                         sunken, borderActive, isLive, true);
}

// ---- Helpers --------------------------------------------------------------

int MultiButtonWidget::entryCount() const
{
    if (m_mode == MultiButtonMode::Function)
        return m_functionIds.size();
    if (m_mode == MultiButtonMode::Widget)
    {
        if (PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget())
            return target->multiButtonEntryCount(m_widgetOutputIndex, m_widgetParameter);
        return 0;
    }
    return m_levelPresets.size();
}

QString MultiButtonWidget::entryLabel(int idx) const
{
    if (idx < 0)
        return QString();

    if (m_mode == MultiButtonMode::Function)
        return m_functionLabels.value(idx);

    if (m_mode == MultiButtonMode::Widget)
    {
        const LevelPreset* appearance = entryAppearancePreset(idx);
        if (appearance && appearance->hideName)
            return QString();
        if (appearance && !appearance->label.isEmpty())
            return appearance->label;
        return linkedWidgetEntryName(idx);
    }

    if (idx < m_levelPresets.size())
        return m_levelPresets.at(idx).label;

    return QString();
}

QString MultiButtonWidget::levelPresetDisplayName(int idx) const
{
    if (idx < 0 || idx >= m_levelPresets.size())
        return QString();

    const LevelPreset& preset = m_levelPresets.at(idx);
    if (preset.hideName)
        return QString();
    if (!preset.label.isEmpty())
        return preset.label;
    return tr("Preset %1").arg(idx + 1);
}

QString MultiButtonWidget::entryIconPath(int idx) const
{
    if (idx < 0)
        return QString();

    if (m_mode == MultiButtonMode::Function)
        return m_iconPaths.value(idx);

    if (const LevelPreset* appearance = entryAppearancePreset(idx))
        return appearance->iconPath;

    return QString();
}

Function* MultiButtonWidget::functionAt(int idx) const
{
    if (idx < 0 || idx >= m_functionIds.size()) return nullptr;
    return m_doc->function(m_functionIds.at(idx));
}

QPixmap MultiButtonWidget::iconForEntry(int idx) const
{
    const QString path = entryIconPath(idx);
    if (path.isEmpty()) return QPixmap();

    if (m_iconCache.contains(idx))
        return m_iconCache.value(idx);

    QPixmap px(path);
    if (!px.isNull())
        m_iconCache.insert(idx, px);
    return px;
}

static QColor contrastTextOn(const QColor& bg)
{
    return (bg.lightness() > 128) ? QColor(Qt::black) : QColor(Qt::white);
}

static QColor contrastRingOn(const QColor& bg)
{
    return contrastTextOn(bg);
}

/** Non-modal entry list for Operate + Single (replaces QMenu::popup on macOS). */
class EntrySelectOverlay : public QWidget
{
public:
    static const int kRowHeight  = 28;
    static const int kPad        = 4;
    static const int kTitleHeight = 22;
    static const int kIconSize   = 22;

    explicit EntrySelectOverlay(MultiButtonWidget* owner)
        : QWidget(nullptr)
        , m_owner(owner)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setPalette(owner->palette());
    }

    std::function<void(int)> onRowPicked;
    std::function<void()>    onPickCancelled;

    void beginDragPick()
    {
        m_trackingPick = true;
        grabMouse();
        setHoverFromPos(mapFromGlobal(QCursor::pos()));
    }

    QSize computedSize() const
    {
        QFontMetrics fm(font());
        int w = qMax(m_owner->width(), 180);

        const int n = m_owner->entryCount();
        for (int i = 0; i < n; ++i)
        {
            const QString text = rowLabel(i);
            w = qMax(w, fm.horizontalAdvance(text) + 72);
        }
        if (m_owner->m_addOffAtEnd)
            w = qMax(w, fm.horizontalAdvance(m_owner->tr("OFF (deactivate)")) + 24);

        const bool hasTitle = !m_owner->caption().isEmpty();
        int h = kPad * 2 + rowCount() * kRowHeight;
        if (hasTitle)
            h += kTitleHeight;

        return QSize(w, h);
    }

    void positionBelowOwner()
    {
        setFixedSize(computedSize());
        move(m_owner->mapToGlobal(QPoint(0, m_owner->height())));
    }

    void positionAtGlobal(const QPoint& globalPos)
    {
        setFixedSize(computedSize());
        move(globalPos);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), palette().window());

        const bool hasTitle = !m_owner->caption().isEmpty();
        int y = kPad;

        if (hasTitle)
        {
            QFont titleFont = font();
            titleFont.setBold(true);
            p.setFont(titleFont);
            p.setPen(palette().windowText().color());
            p.drawText(QRect(kPad, y, width() - 2 * kPad, kTitleHeight),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       m_owner->caption());
            y += kTitleHeight;
        }

        const int highlightIdx = highlightEntryIndex();
        const int rows = rowCount();
        for (int row = 0; row < rows; ++row)
        {
            const int entryIdx = entryIndexForRow(row);
            const QRect rowRect(kPad, y, width() - 2 * kPad, kRowHeight);
            paintRow(p, rowRect, entryIdx, highlightIdx);
            y += kRowHeight;
        }

        p.setPen(QPen(palette().mid().color(), 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() != Qt::LeftButton)
            return;

        m_trackingPick = true;
        setHoverFromPos(e->pos());
        e->accept();
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        setHoverFromPos(e->pos());
        e->accept();
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (e->button() != Qt::LeftButton)
            return;

        const int row = rowAt(e->pos());
        const int entryIdx = entryIndexForRow(row);
        const bool validRow = (row >= 0 && row < rowCount() && entryIdx >= -1);

        endDragPick();

        if (validRow && onRowPicked)
            onRowPicked(entryIdx);
        else if (onPickCancelled)
            onPickCancelled();

        e->accept();
    }

private:
    void setHoverFromPos(const QPoint& pos)
    {
        const int row = rowAt(pos);
        if (row == m_hoverRow)
            return;
        m_hoverRow = row;
        update();
    }

    void endDragPick()
    {
        if (mouseGrabber() == this)
            releaseMouse();
        m_trackingPick = false;
    }

    int highlightEntryIndex() const
    {
        if (m_trackingPick && m_hoverRow >= 0)
        {
            const int idx = entryIndexForRow(m_hoverRow);
            if (idx >= -1)
                return idx;
        }
        return m_owner->displayedEntryIndex();
    }

    int m_hoverRow = -1;
    bool m_trackingPick = false;

    int rowCount() const
    {
        const int n = m_owner->entryCount();
        if (n <= 0)
            return 0;
        return n + (m_owner->m_addOffAtEnd ? 1 : 0);
    }

    int contentTop() const
    {
        return kPad + (m_owner->caption().isEmpty() ? 0 : kTitleHeight);
    }

    int rowAt(const QPoint& pos) const
    {
        const int y = pos.y() - contentTop();
        if (y < 0)
            return -1;
        return y / kRowHeight;
    }

    int entryIndexForRow(int row) const
    {
        const int n = m_owner->entryCount();
        if (row < 0 || row >= rowCount())
            return -2;
        if (row < n)
            return row;
        return -1;
    }

    QString rowLabel(int entryIdx) const
    {
        if (entryIdx < 0)
            return m_owner->tr("OFF (deactivate)");
        return m_owner->popupMenuTextForEntry(entryIdx);
    }

    void paintRow(QPainter& p, const QRect& rowRect, int entryIdx, int displayIdx) const
    {
        const bool selected = (entryIdx < 0) ? (displayIdx < 0) : (entryIdx == displayIdx);

        QColor bg = m_owner->palette().button().color();
        QColor fg = m_owner->palette().buttonText().color();

        if (entryIdx >= 0)
        {
            const LevelPreset* appearance = m_owner->entryAppearancePreset(entryIdx);
            if (appearance && appearance->color.isValid())
            {
                bg = appearance->color;
                fg = contrastTextOn(bg);
            }
            if (appearance && appearance->labelColor.isValid())
                fg = appearance->labelColor;
        }

        p.fillRect(rowRect, bg);

        if (selected)
        {
            const QColor ring = contrastRingOn(bg);
            p.setPen(QPen(ring, 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(rowRect.adjusted(1, 1, -1, -1), 3, 3);
        }

        QRect inner = rowRect.adjusted(8, 0, -6, 0);

        if (selected)
        {
            QFont markFont = p.font();
            markFont.setBold(true);
            p.setFont(markFont);
            p.setPen(fg);
            p.drawText(QRect(inner.left(), inner.top(), 16, inner.height()),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QStringLiteral("\u2713"));
            inner.adjust(14, 0, 0, 0);
        }

        if (entryIdx >= 0 && !m_owner->entryIconPath(entryIdx).isEmpty())
        {
            const QPixmap px = m_owner->iconForEntry(entryIdx);
            if (!px.isNull())
            {
                const QPixmap scaled = px.scaled(kIconSize, kIconSize,
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
                const int ix = inner.right() - scaled.width();
                const int iy = inner.top() + (inner.height() - scaled.height()) / 2;
                p.drawPixmap(ix, iy, scaled);
                inner.adjust(0, 0, -(scaled.width() + 4), 0);
            }
        }

        const QString text = rowLabel(entryIdx);
        if (!text.isEmpty())
        {
            QFont textFont = p.font();
            textFont.setBold(selected);
            p.setFont(textFont);
            p.setPen(fg);
            p.drawText(inner, Qt::AlignVCenter | Qt::AlignLeft, text);
        }
    }

    MultiButtonWidget* m_owner;
};

QString MultiButtonWidget::popupMenuTextForEntry(int idx) const
{
    if (idx < 0 || idx >= entryCount())
        return QString();

    if (m_mode == MultiButtonMode::Function)
    {
        const QString lbl = entryLabel(idx);
        Function* f = functionAt(idx);
        return lbl.isEmpty() ? (f ? f->name() : tr("Function %1").arg(idx + 1)) : lbl;
    }

    if (m_mode == MultiButtonMode::Widget)
    {
        const LevelPreset* appearance = entryAppearancePreset(idx);
        if (appearance && !appearance->label.isEmpty())
            return appearance->label;
        const QString linked = linkedWidgetEntryName(idx);
        return linked.isEmpty() ? tr("Preset %1").arg(idx + 1) : linked;
    }

    if (idx < m_levelPresets.size() && m_levelPresets.at(idx).hideName)
        return QString();

    return levelPresetDisplayName(idx);
}

void MultiButtonWidget::rebuildSceneCache()
{
    m_cachedSceneValues.clear();
    for (quint32 fid : m_functionIds)
    {
        Function* f = m_doc->function(fid);
        Scene* sc = qobject_cast<Scene*>(f);
        if (sc)
            m_cachedSceneValues.append(sc->values());
        else
            m_cachedSceneValues.append(QList<SceneValue>());
    }
}

// ---- Core logic -----------------------------------------------------------

void MultiButtonWidget::cycleNext()
{
    if (entryCount() == 0) return;

    if (m_logPresetChanges)
        qDebug().nospace() << "MultiButton id=" << id() << " cycleNext";

    int total = entryCount() + (m_addOffAtEnd ? 1 : 0);
    if (widgetLinkUsesInternalStaging())
        syncWidgetLinkLiveStagedState();

    const int base = widgetLinkUsesInternalStaging() && hasLocalStagedSelection()
            ? m_stagedIndex : m_currentIndex;
    int next  = (base + 1) % total;

    if (m_addOffAtEnd && next == entryCount())
    {
        if (widgetLinkUsesInternalStaging() || stagingActive())
            stageEntry(-1);
        else
        {
            stopCurrent();
            updateFeedback();
            update();
        }
    }
    else if (widgetLinkUsesInternalStaging() || stagingActive())
        stageEntry(next);
    else
    {
        activate(next);
    }
}

void MultiButtonWidget::setAutomationEnabled(bool enable)
{
    m_automationEnabled = enable;
}

void MultiButtonWidget::setAutomationProfiles(const QList<MultiButtonAutomationProfile>& profiles,
                                            int activeIndex)
{
    m_automationProfiles = profiles;
    if (m_automationProfiles.isEmpty())
    {
        MultiButtonAutomationProfile def;
        def.name = tr("Profile %1").arg(1);
        m_automationProfiles.append(def);
    }
    m_activeAutomationProfile = qBound(0, activeIndex, m_automationProfiles.size() - 1);
    m_automationPulseCounter = 0;
}

void MultiButtonWidget::onAutomationTrigger()
{
    const MultiButtonAutomationProfile* profile = activeAutomationProfilePtr();
    if (!profile || entryCount() == 0)
        return;

    ++m_automationPulseCounter;
    const int N = qMax(1, profile->multiplier);
    const int phase = ((profile->beatOffset % N) + N) % N;
    if ((m_automationPulseCounter % N) != phase)
        return;

    advanceAutomation();
}

const MultiButtonAutomationProfile* MultiButtonWidget::activeAutomationProfilePtr() const
{
    if (m_automationProfiles.isEmpty())
        return nullptr;
    return &m_automationProfiles.at(qBound(0, m_activeAutomationProfile,
                                             m_automationProfiles.size() - 1));
}

QVector<int> MultiButtonWidget::buildAllowedAutomationSlots(
    const MultiButtonAutomationProfile& profile) const
{
    QVector<int> allowed;
    const int n = entryCount();
    for (int i = 0; i < n; ++i)
    {
        if (entryIsFlash(i))
            continue;
        if ((profile.excludeMask & (1u << i)) == 0)
            allowed.append(i);
    }
    if (m_addOffAtEnd)
        allowed.append(-1);
    return allowed;
}

void MultiButtonWidget::advanceAutomation()
{
    const MultiButtonAutomationProfile* profile = activeAutomationProfilePtr();
    if (!profile || entryCount() == 0)
        return;

    if (m_logPresetChanges)
        qDebug().nospace() << "MultiButton id=" << id() << " advanceAutomation";

    const QVector<int> allowed = buildAllowedAutomationSlots(*profile);
    if (allowed.isEmpty())
        return;

    int pos = 0;
    bool currentInAllowed = false;
    for (int i = 0; i < allowed.size(); ++i)
    {
        if (allowed.at(i) == m_currentIndex)
        {
            pos = i;
            currentInAllowed = true;
            break;
        }
    }
    if (!currentInAllowed)
        pos = allowed.size() - 1;

    int newPos = pos;
    switch (profile->mode)
    {
        case MultiButtonAutomationMode::Random:
            newPos = QRandomGenerator::global()->bounded(allowed.size());
            break;
        case MultiButtonAutomationMode::Jump:
        {
            int step = profile->stepMin;
            if (profile->stepMax > profile->stepMin)
            {
                step = QRandomGenerator::global()->bounded(profile->stepMin,
                                                           profile->stepMax + 1);
            }
            newPos = (pos + step) % allowed.size();
            break;
        }
        case MultiButtonAutomationMode::Next:
        default:
            newPos = (pos + 1) % allowed.size();
            break;
    }

    activateAutomationLive(allowed.at(newPos));
}

quint8 MultiButtonWidget::staticPresetChannelValue(const LevelPreset& preset, int channelIndex)
{
    if (channelIndex >= 0 && channelIndex < preset.values.size())
        return preset.values.at(channelIndex);
    return 0;
}

bool MultiButtonWidget::presetUsesLiveFormulas(const LevelPreset& preset) const
{
    for (const QString& formula : preset.valueFormulas)
    {
        if (mbValueExprLooksLikeFormula(formula.trimmed()))
            return true;
    }
    return false;
}

quint8 MultiButtonWidget::monitorExpectedChannelValue(const LevelPreset& preset, int channelIndex,
                                                      const QList<Universe*>& universes) const
{
    if (channelIndex >= 0 && channelIndex < preset.valueFormulas.size())
    {
        const QString formula = preset.valueFormulas.at(channelIndex).trimmed();
        if (!formula.isEmpty() && mbValueExprLooksLikeFormula(formula))
            return resolvedPresetChannelValue(preset, channelIndex, universes);
    }
    return staticPresetChannelValue(preset, channelIndex);
}

void MultiButtonWidget::activate(int idx)
{
    if (entryIsFlash(idx))
        return;

    const bool widgetInternalStage = widgetLinkUsesInternalStaging()
            && !m_widgetLiveActivationOverride;
    if (!widgetInternalStage && !m_visualOnly && idx == m_currentIndex) return;

    if (m_logPresetChanges)
    {
        qDebug().nospace() << "MultiButton id=" << id() << " activate idx=" << idx
                           << " (was " << m_currentIndex << ")";
    }

    stopCurrent();

    if (idx < 0 || idx >= entryCount())
    {
        if (m_mode == MultiButtonMode::Widget)
        {
            PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
            const bool ok = widgetInternalStage
                    ? (target && target->multiButtonActivateStaged(m_widgetOutputIndex,
                                                                   m_widgetParameter, -1))
                    : (target && target->multiButtonActivate(m_widgetOutputIndex,
                                                             m_widgetParameter, -1));
            if (ok)
            {
                syncWidgetLinkLiveStagedState();
                if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
                    publishSelectorToBus(-1);
                else
                    setWidgetSelectorLatchedIndex(-1);
                updateFeedback();
                update();
                return;
            }
        }
        m_currentIndex = -1;
        m_visualOnly   = false;
        updateFeedback();
        update();
        return;
    }

    if (m_mode == MultiButtonMode::Function)
    {
        Function* f = functionAt(idx);
        if (f)
            f->start(m_doc->masterTimer(), functionParent());
    }
    else if (m_mode == MultiButtonMode::Widget)
    {
        PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
        const bool ok = widgetInternalStage
                ? (target && target->multiButtonActivateStaged(m_widgetOutputIndex,
                                                               m_widgetParameter, idx))
                : (target && target->multiButtonActivate(m_widgetOutputIndex,
                                                         m_widgetParameter, idx));
        if (!ok)
        {
            m_currentIndex = -1;
            m_visualOnly = false;
            updateFeedback();
            update();
            return;
        }
        if (widgetInternalStage)
        {
            m_stagedIndex = idx;
            m_stagedValid = true;
            if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
                publishSelectorToBus(idx);
            else
                setWidgetSelectorLatchedIndex(idx);
            updateFeedback();
            update();
            return;
        }
    }

    m_currentIndex = idx;
    m_visualOnly   = false;
    m_stagedIndex         = m_currentIndex;
    m_stagedValid         = false;
    m_monitorMatchIndex   = idx;
    m_lastMonitorMatchIdx = idx;
    m_lastActivationTime.restart();
    if (m_mode == MultiButtonMode::Widget)
    {
        if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
            publishSelectorToBus(idx);
        else
            setWidgetSelectorLatchedIndex(idx);
    }
    updateFeedback();
    update();
}

void MultiButtonWidget::activateAutomationLive(int idx)
{
    clearLocalStagedSelection();

    if (m_mode == MultiButtonMode::Widget)
    {
        const bool previousOverride = m_widgetLiveActivationOverride;
        m_widgetLiveActivationOverride = true;

        if (idx < 0)
        {
            PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
            const bool ok = target && target->multiButtonActivate(m_widgetOutputIndex,
                                                                  m_widgetParameter, -1);
            if (ok)
                syncWidgetLinkLiveStagedState();
            else
            {
                m_currentIndex = -1;
                m_visualOnly = false;
                m_monitorMatchIndex = -1;
                m_lastMonitorMatchIdx = -1;
            }

            if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
                publishSelectorToBus(-1);
            else
                setWidgetSelectorLatchedIndex(-1);
            updateFeedback();
            update();
        }
        else
        {
            activate(idx);
        }

        m_widgetLiveActivationOverride = previousOverride;
        return;
    }

    if (idx < 0)
    {
        stopCurrent();
        clearLocalStagedSelection();
        updateFeedback();
        update();
        return;
    }

    activate(idx);
}

void MultiButtonWidget::stopCurrent()
{
    if (m_currentIndex < 0) return;

    if (m_mode == MultiButtonMode::Function && !m_visualOnly)
    {
        Function* f = functionAt(m_currentIndex);
        if (f) f->stop(functionParent());
    }
    else if (m_mode == MultiButtonMode::Level)
    {
        releaseLevelFaders();
    }

    m_currentIndex = -1;
    m_visualOnly   = false;
    clearLocalStagedSelection();
    m_lastActivationTime.restart();
}

bool MultiButtonWidget::stagingActive() const
{
    return m_stageBeforeCommit && mode() == Doc::Operate;
}

bool MultiButtonWidget::hasLocalStagedSelection() const
{
    return m_stagedValid;
}

void MultiButtonWidget::clearLocalStagedSelection()
{
    m_stagedIndex = -1;
    m_stagedValid = false;
}

void MultiButtonWidget::stageEntry(int idx)
{
    if (!stagingActive() && !widgetLinkUsesInternalStaging())
        return;
    if (idx < -1 || idx >= entryCount())
        return;
    if (idx >= 0 && entryIsFlash(idx))
        return;

    if (widgetLinkUsesInternalStaging())
    {
        PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
        if (!target || !target->multiButtonActivateStaged(m_widgetOutputIndex,
                                                          m_widgetParameter, idx))
            return;
    }
    m_stagedIndex = idx;
    m_stagedValid = true;
    syncWidgetLinkLiveStagedState();
    if (m_mode == MultiButtonMode::Widget)
    {
        if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
            publishSelectorToBus(idx);
        else
            setWidgetSelectorLatchedIndex(idx);
    }
    update();
}

void MultiButtonWidget::commitStaged()
{
    if (!stagingActive())
        return;

    if (widgetLinkUsesInternalStaging())
    {
        update();
        return;
    }

    if (!hasLocalStagedSelection())
        return;

    if (m_logPresetChanges)
    {
        qDebug().nospace() << "MultiButton id=" << id() << " commitStaged idx=" << m_stagedIndex;
    }

    const int idx = m_stagedIndex;
    if (idx < 0)
        activate(-1);
    else
        activate(idx);

    clearLocalStagedSelection();
    update();
}

void MultiButtonWidget::clearStagedOnExternalMonitorChange(int matchIdx)
{
    Q_UNUSED(matchIdx);
    if ((!stagingActive() && !widgetLinkUsesInternalStaging()) || !hasLocalStagedSelection())
        return;
    clearLocalStagedSelection();
}

// ---- Monitor channel values -----------------------------------------------

void MultiButtonWidget::slotCheckChannelValues()
{
    if (!m_monitorChannelValues) return;

    const int debounceMs = stagingActive() ? 100 : 500;
    if (m_lastActivationTime.isValid() && m_lastActivationTime.elapsed() < debounceMs)
        return;

    if (m_mode == MultiButtonMode::Widget)
    {
        syncDynamicEntryCountLayout();
        if (syncWidgetLinkLiveStagedState())
            update();
        return;
    }

    QList<Universe*> universes = m_doc->inputOutputMap()->claimUniverses();
    int matchIdx = -1;

    if (m_mode == MultiButtonMode::Function)
    {
        if (m_cachedSceneValues.isEmpty())
        {
            m_doc->inputOutputMap()->releaseUniverses(false);
            return;
        }

        if (m_cachedSceneValues.size() != m_functionIds.size())
            rebuildSceneCache();

        QVector<int> matches;
        matches.reserve(m_cachedSceneValues.size());

        for (int entry = 0; entry < m_cachedSceneValues.size(); ++entry)
        {
            const QList<SceneValue>& vals = m_cachedSceneValues.at(entry);
            if (vals.isEmpty()) continue;

            bool allMatch = true;
            for (const SceneValue& scv : vals)
            {
                Fixture* fixture = m_doc->fixture(scv.fxi);
                if (!fixture) { allMatch = false; break; }

                quint32 uni  = fixture->universe();
                quint32 addr = fixture->address() + scv.channel;

                if ((int) uni >= universes.count()) { allMatch = false; break; }
                if (universes.at(uni)->preGMValue(addr) != scv.value) { allMatch = false; break; }
            }

            if (allMatch)
                matches.append(entry);
        }

        if (!matches.isEmpty())
        {
            if (m_currentIndex >= 0 && matches.contains(m_currentIndex))
                matchIdx = m_currentIndex;
            else
                matchIdx = matches.first();
        }
    }
    else
    {
        if (m_levelChannelBindings.isEmpty())
        {
            m_doc->inputOutputMap()->releaseUniverses(false);
            return;
        }

        QVector<int> matches;
        matches.reserve(m_levelPresets.size());

        for (int entry = 0; entry < m_levelPresets.size(); ++entry)
        {
            const LevelPreset& preset = m_levelPresets.at(entry);
            bool allMatch = true;

            for (int i = 0; i < m_levelChannelBindings.size(); ++i)
            {
                const LevelChannelBinding& b = m_levelChannelBindings.at(i);
                Fixture* fxi = m_doc->fixture(b.fixtureId);
                if (!fxi) { allMatch = false; break; }

                quint32 addr = fxi->address() + b.channel;
                quint32 uni  = fxi->universe();
                const quint8 expected = monitorExpectedChannelValue(preset, i, universes);

                if ((int) uni >= universes.count()) { allMatch = false; break; }
                if (universes.at(uni)->preGMValue(addr) != expected) { allMatch = false; break; }
            }

            if (allMatch)
                matches.append(entry);
        }

        if (!matches.isEmpty())
        {
            if (m_currentIndex >= 0 && matches.contains(m_currentIndex))
                matchIdx = m_currentIndex;
            else
                matchIdx = matches.first();
        }
    }

    m_doc->inputOutputMap()->releaseUniverses(false);

    bool needUpdate = false;
    bool stagedCleared = false;
    const int prevMonitorMatch = m_monitorMatchIndex;

    if (matchIdx >= 0)
    {
        if (stagingActive() && hasLocalStagedSelection() && matchIdx != m_lastMonitorMatchIdx)
        {
            clearStagedOnExternalMonitorChange(matchIdx);
            stagedCleared = true;
        }

        m_monitorMatchIndex   = matchIdx;
        m_lastMonitorMatchIdx = matchIdx;
    }
    else
    {
        m_monitorMatchIndex = -1;
        if (m_lastMonitorMatchIdx >= 0)
            m_lastMonitorMatchIdx = -1;
    }

    if (m_monitorMatchIndex != prevMonitorMatch || stagedCleared)
        needUpdate = true;

    if (needUpdate)
        update();
}

// ---- Mode ----------------------------------------------------------------

void MultiButtonWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Design)
    {
        closeEntrySelectPopup(false);
        m_doc->masterTimer()->unregisterDMXSource(this);
        releaseLevelFaders();
        releaseWidgetLiveFaders();

        if (m_visualOnly)
        {
            m_currentIndex = -1;
            m_visualOnly   = false;
        }
        else
        {
            stopCurrent();
        }

        clearLocalStagedSelection();
        m_monitorMatchIndex   = -1;
        m_lastMonitorMatchIdx = -2;

        if (m_channelMonitorTimer)
            m_channelMonitorTimer->stop();
    }
    else if (mode == Doc::Operate)
    {
        syncAllInputSourcePages();
        syncAutomationSuspendDefault();
        m_triggerLastValue     = 0;
        m_automationLastValue  = 0;
        m_commitInputLastValue = 0;

        syncWidgetLiveInputSourceToTarget();
        updateDmxRegistration();
        if (m_mode == MultiButtonMode::Widget)
            markWidgetSelectorPublishPending();
        reactivateLevelPreset();
        updateChannelMonitorTimerInterval();

        if (m_monitorChannelValues)
        {
            rebuildSceneCache();
            if (m_channelMonitorTimer)
                m_channelMonitorTimer->start();
        }
    }

    VCWidget::slotModeChanged(mode);
    update();
}

// ---- Mouse interaction ---------------------------------------------------

void MultiButtonWidget::mousePressEvent(QMouseEvent* e)
{
    if (mode() == Doc::Operate && e->button() == Qt::LeftButton)
    {
        m_pressActive = true;
        m_longFired   = false;
        m_pressPos    = e->pos();

        if (m_layout == MultiButtonLayout::Spread)
        {
            m_pressTileIndex = spreadHitTest(e->pos());
            if (m_pressTileIndex >= 0 && entryIsFlash(m_pressTileIndex))
                beginFlashHold(m_pressTileIndex);
            else if (m_pressTileIndex >= -1)
                m_longPressTimer->start(m_longPressMs);
            update();
        }
        else
        {
            m_pressTileIndex = -2;
            m_longPressTimer->start(m_longPressMs);
            update();
        }

        e->accept();
        return;
    }
    VCWidget::mousePressEvent(e);
}

void MultiButtonWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (m_pressActive)
    {
        QPoint delta = e->pos() - m_pressPos;
        if (delta.manhattanLength() > 10)
        {
            m_longPressTimer->stop();
            m_longFired = true;
        }
        e->accept();
        return;
    }
    VCWidget::mouseMoveEvent(e);
}

void MultiButtonWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_pressActive && e->button() == Qt::LeftButton)
    {
        m_pressActive = false;
        m_longPressTimer->stop();

        if (m_layout == MultiButtonLayout::Spread)
        {
            if (m_flashHoldIndex >= 0)
            {
                endFlashHold();
            }
            else
            {
                const int hit = spreadHitTest(e->pos());
                if (!m_longFired && hit == m_pressTileIndex && hit != -2)
                {
                    if (stagingActive() || widgetLinkUsesInternalStaging())
                    {
                        if (hit < 0)
                            stageEntry(-1);
                        else if (!entryIsFlash(hit))
                            stageEntry(hit);
                    }
                    else if (hit < 0)
                    {
                        stopCurrent();
                    }
                    else if (!entryIsFlash(hit))
                    {
                        activate(hit);
                    }
                }
            }
            m_pressTileIndex = -2;
        }
        else if (m_longFired && m_entrySelectOverlay)
        {
            clearPressTracking();
        }
        else if (!m_longFired && rect().contains(e->pos()))
        {
            cycleNext();
        }

        update();
        e->accept();
        return;
    }
    VCWidget::mouseReleaseEvent(e);
}

void MultiButtonWidget::clearPressTracking()
{
    m_pressActive = false;
    m_longPressTimer->stop();
    m_pressTileIndex = -2;
}

void MultiButtonWidget::slotLongPressFired()
{
    if (m_layout != MultiButtonLayout::Single && m_layout != MultiButtonLayout::Spread)
        return;

    m_longFired = true;
    showPopupMenu(QCursor::pos());
    update();
}

void MultiButtonWidget::contextMenuEvent(QContextMenuEvent* e)
{
    if (mode() == Doc::Operate
        && (m_layout == MultiButtonLayout::Single || m_layout == MultiButtonLayout::Spread))
    {
        showPopupMenu(e->globalPos());
        e->accept();
        return;
    }
    m_contextMenuEntryIndex = -2;
    if (mode() == Doc::Design && m_layout == MultiButtonLayout::Spread)
    {
        const int hit = spreadHitTest(e->pos());
        if (hit >= 0)
            m_contextMenuEntryIndex = hit;
    }
    VCWidget::contextMenuEvent(e);
}

int MultiButtonWidget::selectableSlotCount() const
{
    const int n = entryCount();
    if (n <= 0)
        return 0;
    return n + (m_addOffAtEnd ? 1 : 0);
}

int MultiButtonWidget::slotFromInputValue(uchar value, const QLCInputSource* src) const
{
    const int slotCount = selectableSlotCount();
    if (slotCount <= 0)
        return 0;

    uchar lower = 0;
    uchar upper = 255;
    if (src != nullptr)
    {
        lower = src->feedbackValue(QLCInputFeedback::LowerValue);
        upper = src->feedbackValue(QLCInputFeedback::UpperValue);
        if (upper <= lower)
        {
            lower = 0;
            upper = 255;
        }
    }

    const int range = int(upper) - int(lower);
    const int v = qBound(int(lower), int(value), int(upper));
    return qMin(slotCount - 1, int((qint64(v - lower) * slotCount) / (range + 1)));
}

int MultiButtonWidget::slotToEntryIndex(int slot) const
{
    const int n = entryCount();
    if (n <= 0)
        return -1;

    if (m_addOffAtEnd && slot >= n)
        return -1;

    return qBound(0, slot, n - 1);
}

void MultiButtonWidget::syncWidgetEntryAppearanceCount()
{
    if (m_mode != MultiButtonMode::Widget)
        return;

    const int count = qMax(0, entryCount());
    while (m_widgetEntryAppearance.size() < count)
        m_widgetEntryAppearance.append(LevelPreset());
    while (m_widgetEntryAppearance.size() > count)
        m_widgetEntryAppearance.removeLast();
}

const LevelPreset* MultiButtonWidget::entryAppearancePreset(int idx) const
{
    if (idx < 0)
        return nullptr;

    if (m_mode == MultiButtonMode::Widget)
    {
        if (idx < m_widgetEntryAppearance.size())
            return &m_widgetEntryAppearance.at(idx);
        return nullptr;
    }

    if (m_mode == MultiButtonMode::Level && idx < m_levelPresets.size())
        return &m_levelPresets.at(idx);

    return nullptr;
}

LevelPreset* MultiButtonWidget::mutableEntryAppearancePreset(int idx)
{
    if (idx < 0)
        return nullptr;

    if (m_mode == MultiButtonMode::Widget)
    {
        syncWidgetEntryAppearanceCount();
        if (idx < m_widgetEntryAppearance.size())
            return &m_widgetEntryAppearance[idx];
        return nullptr;
    }

    if (m_mode == MultiButtonMode::Level && idx < m_levelPresets.size())
        return &m_levelPresets[idx];

    return nullptr;
}

QString MultiButtonWidget::linkedWidgetEntryName(int idx) const
{
    if (idx < 0)
        return QString();
    if (PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget())
        return target->multiButtonEntryName(m_widgetOutputIndex, m_widgetParameter, idx);
    return QString();
}

uchar MultiButtonWidget::entrySelectOutputValueForSlot(int slot) const
{
    const int slotCount = selectableSlotCount();
    if (slotCount <= 0)
        return 0;

    const QLCInputSource* src = inputSource(entrySelectInputSourceId).data();
    uchar lower = 0;
    uchar upper = 255;
    if (src != nullptr)
    {
        lower = src->feedbackValue(QLCInputFeedback::LowerValue);
        upper = src->feedbackValue(QLCInputFeedback::UpperValue);
        if (upper <= lower)
        {
            lower = 0;
            upper = 255;
        }
    }

    if (slotCount <= 1)
        return lower;

    const int range = int(upper) - int(lower);
    const int center = int(lower) + ((slot * 2 + 1) * (range + 1)) / (2 * slotCount);
    return uchar(qBound(int(lower), center, int(upper)));
}

void MultiButtonWidget::syncEntrySelectInputOutput(uchar rawValue)
{
    QSharedPointer<QLCInputSource> src = inputSource(entrySelectInputSourceId);
    if (src.isNull() || !src->isValid() || !src->needsUpdate())
        return;

    src->updateOuputValue(rawValue);
}

int MultiButtonWidget::displayedEntryIndex() const
{
    if (m_flashHoldIndex >= 0)
        return m_flashHoldIndex;
    if (m_entrySelectPreviewActive)
        return m_entrySelectPreviewIndex;
    if ((stagingActive() || widgetLinkUsesInternalStaging()) && hasLocalStagedSelection())
        return m_stagedIndex;
    return m_currentIndex;
}

bool MultiButtonWidget::syncWidgetLinkLiveStagedState()
{
    if (m_mode != MultiButtonMode::Widget)
        return false;

    PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
    const int liveIdx = target
            ? target->multiButtonLiveIndex(m_widgetOutputIndex, m_widgetParameter) : -1;
    const bool stagedValid = target && widgetLinkUsesInternalStaging()
            && target->multiButtonHasStagedIndex(m_widgetOutputIndex, m_widgetParameter);
    const int stagedIdx = stagedValid
            ? target->multiButtonStagedIndex(m_widgetOutputIndex, m_widgetParameter) : -1;

    const int prevLive = m_currentIndex;
    const int prevMonitor = m_monitorMatchIndex;
    const int prevLastMonitor = m_lastMonitorMatchIdx;
    const int prevStaged = m_stagedIndex;
    const bool prevStagedValid = m_stagedValid;

    m_currentIndex = liveIdx;
    m_monitorMatchIndex = liveIdx;
    m_lastMonitorMatchIdx = liveIdx;
    m_stagedIndex = stagedIdx;
    m_stagedValid = stagedValid;

    const bool changed = m_currentIndex != prevLive
            || m_monitorMatchIndex != prevMonitor
            || m_lastMonitorMatchIdx != prevLastMonitor
            || m_stagedIndex != prevStaged
            || m_stagedValid != prevStagedValid;

    if (changed && m_mode == MultiButtonMode::Widget)
    {
        QMutexLocker lock(&m_dmxMutex);
        m_widgetSelectorLatchedValid = false;
        m_widgetSelectorLatchedIndex = -1;
        if (m_widgetBusPolicy == MultiButtonWidgetBusPolicy::SharedBus)
            m_widgetLiveWriteDirty = true;
    }

    return changed;
}

int MultiButtonWidget::monitorHighlightIndex() const
{
    if (m_mode == MultiButtonMode::Widget)
    {
        if (PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget())
            return target->multiButtonLiveIndex(m_widgetOutputIndex, m_widgetParameter);
        return m_monitorMatchIndex >= 0 ? m_monitorMatchIndex : m_currentIndex;
    }

    if (!m_monitorChannelValues)
        return -1;

    // Staging: orange = bus match (external/internal), green = m_stagedIndex; DMX = m_currentIndex
    if (stagingActive() || widgetLinkUsesInternalStaging())
    {
        if (m_monitorMatchIndex >= 0)
            return m_monitorMatchIndex;
        if (m_currentIndex >= 0)
            return m_currentIndex;
        return -1;
    }

    // Monitor is display-only; does not change m_currentIndex / writeDMX
    return m_monitorMatchIndex;
}

int MultiButtonWidget::stagedHighlightIndex() const
{
    if (!stagedHighlightValid())
        return -1;

    if (widgetLinkUsesInternalStaging())
        return m_stagedIndex;

    return m_stagedIndex;
}

bool MultiButtonWidget::stagedHighlightValid() const
{
    if (!stagingActive() && !widgetLinkUsesInternalStaging())
        return false;

    if (widgetLinkUsesInternalStaging())
    {
        PresetTableV2MultiButtonTargetIface* target = widgetLinkTarget();
        return target
                && target->multiButtonHasStagedIndex(m_widgetOutputIndex, m_widgetParameter);
    }

    return hasLocalStagedSelection();
}

bool MultiButtonWidget::widgetLinkUsesInternalStaging() const
{
    return m_mode == MultiButtonMode::Widget
            && (m_widgetParameter == PresetTableV2MultiButtonTargetIface::PrimaryRow
                || m_widgetParameter == PresetTableV2MultiButtonTargetIface::SecondaryRow
                || m_widgetParameter == PresetTableV2MultiButtonTargetIface::ContinuousPreset
                || m_widgetParameter == PresetTableV2MultiButtonTargetIface::MultiFxPreset);
}

void MultiButtonWidget::cancelEntrySelectPreview()
{
    if (!m_entrySelectPreviewActive)
        return;

    m_entrySelectPreviewActive = false;
    m_entrySelectPreviewIndex  = -1;
    update();
}

void MultiButtonWidget::commitEntrySelectPreview()
{
    if (!m_entrySelectPreviewActive)
        return;

    m_entrySelectPreviewActive = false;
    const int idx = m_entrySelectPreviewIndex;
    m_entrySelectPreviewIndex = -1;

    if (stagingActive() || widgetLinkUsesInternalStaging())
    {
        stageEntry(idx);
        return;
    }

    if (idx < 0)
        stopCurrent();
    else
        activate(idx);
}

void MultiButtonWidget::destroyEntrySelectOverlay()
{
    EntrySelectOverlay* overlay = m_entrySelectOverlay.data();
    m_entrySelectOverlay.clear();
    if (overlay)
    {
        overlay->hide();
        overlay->deleteLater();
    }
}

void MultiButtonWidget::applyEntryPick(int idx)
{
    if (stagingActive() || widgetLinkUsesInternalStaging())
    {
        stageEntry(idx);
        return;
    }

    if (idx < 0)
    {
        stopCurrent();
        updateFeedback();
        update();
    }
    else
    {
        activate(idx);
    }
}

void MultiButtonWidget::showPopupMenu(const QPoint& globalPos)
{
    if (entryCount() == 0)
        return;

    closeEntrySelectPopup(false);

    auto* overlay = new EntrySelectOverlay(this);
    m_entrySelectOverlay = overlay;
    overlay->positionAtGlobal(globalPos);
    overlay->onRowPicked = [this](int idx) {
        if (m_entrySelectDismissTimer)
            m_entrySelectDismissTimer->stop();
        destroyEntrySelectOverlay();
        clearPressTracking();
        applyEntryPick(idx);
    };
    overlay->onPickCancelled = [this]() {
        destroyEntrySelectOverlay();
        clearPressTracking();
    };
    overlay->show();
    if (QApplication::mouseButtons() & Qt::LeftButton)
        overlay->beginDragPick();
}

int MultiButtonWidget::pickEntryIndexModal(const QPoint& globalPos)
{
    if (entryCount() == 0)
        return -2;

    int result = -2;
    QEventLoop loop;

    auto* overlay = new EntrySelectOverlay(this);
    overlay->positionAtGlobal(globalPos);
    overlay->onRowPicked = [overlay, &result, &loop](int idx) {
        result = idx;
        overlay->hide();
        overlay->deleteLater();
        loop.quit();
    };
    overlay->onPickCancelled = [overlay, &result, &loop]() {
        result = -2;
        overlay->hide();
        overlay->deleteLater();
        loop.quit();
    };
    overlay->show();
    loop.exec();

    return result;
}

void MultiButtonWidget::closeEntrySelectPopup(bool commitSelection)
{
    if (m_entrySelectDismissTimer)
        m_entrySelectDismissTimer->stop();

    destroyEntrySelectOverlay();

    if (commitSelection && m_entrySelectAutoCommit && acceptsOperationalInput())
        commitEntrySelectPreview();
    else
        cancelEntrySelectPreview();
}

void MultiButtonWidget::armEntrySelectPopupDismissTimer()
{
    if (mode() != Doc::Operate)
        return;

    if (!m_entrySelectAutoCommit)
        return;

    if (!isVisible() || !isEnabled())
        return;

    if (!m_entrySelectDismissTimer)
        return;

    m_entrySelectDismissTimer->stop();
    m_entrySelectDismissTimer->start();
}

void MultiButtonWidget::openEntrySelectPopup()
{
    if (entryCount() == 0 || m_entrySelectOverlay)
        return;

    auto* overlay = new EntrySelectOverlay(this);
    m_entrySelectOverlay = overlay;

    overlay->onRowPicked = [this](int idx) {
        if (m_entrySelectDismissTimer)
            m_entrySelectDismissTimer->stop();

        m_entrySelectPreviewActive = true;
        m_entrySelectPreviewIndex  = idx;
        closeEntrySelectPopup(true);
    };
    overlay->onPickCancelled = [this]() {
        closeEntrySelectPopup(false);
    };

    overlay->positionBelowOwner();
    overlay->show();
}

void MultiButtonWidget::updateEntrySelectPopupHighlight()
{
    if (m_entrySelectOverlay)
        m_entrySelectOverlay->update();
}

// ---- Custom context menu (Design mode) ------------------------------------

QMenu* MultiButtonWidget::customMenu(QMenu* parentMenu)
{
    if (entryCount() == 0) return nullptr;

    QMenu* appearanceMenu = new QMenu(tr("Entry appearance"), parentMenu);

    auto pickEntry = [this]() {
        if (m_contextMenuEntryIndex >= 0 && m_contextMenuEntryIndex < entryCount())
            return m_contextMenuEntryIndex;
        return pickEntryIndexModal(QCursor::pos());
    };

    auto updateAppearance = [this](int idx, const std::function<void(LevelPreset&)>& fn) {
        if (idx < 0 || idx >= entryCount())
            return;
        if (LevelPreset* appearance = mutableEntryAppearancePreset(idx))
        {
            fn(*appearance);
            m_iconCache.remove(idx);
            update();
            if (m_doc)
                m_doc->setModified();
        }
    };

    QAction* editLabelAct = appearanceMenu->addAction(tr("Custom label…"));
    connect(editLabelAct, &QAction::triggered, this, [this, pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        if (idx < 0)
            return;
        const QString current = entryAppearancePreset(idx)
                ? entryAppearancePreset(idx)->label : QString();
        bool ok = false;
        const QString text = QInputDialog::getText(
                this, tr("Custom label"),
                tr("Local label for entry %1 (blank = linked/default name):").arg(idx + 1),
                QLineEdit::Normal, current, &ok);
        if (!ok)
            return;
        updateAppearance(idx, [&text](LevelPreset& preset) {
            preset.label = text.trimmed();
            if (!preset.label.isEmpty())
                preset.hideName = false;
        });
    });

    QAction* resetLabelAct = appearanceMenu->addAction(tr("Reset local label"));
    connect(resetLabelAct, &QAction::triggered, this, [pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        updateAppearance(idx, [](LevelPreset& preset) {
            preset.label.clear();
        });
    });

    QAction* hideNameAct = appearanceMenu->addAction(tr("Hide/show name"));
    connect(hideNameAct, &QAction::triggered, this, [pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        updateAppearance(idx, [](LevelPreset& preset) {
            preset.hideName = !preset.hideName;
        });
    });

    appearanceMenu->addSeparator();

    QAction* scribbleAct = appearanceMenu->addAction(QIcon(":/edit.png"), tr("Scribble icon…"));
    connect(scribbleAct, &QAction::triggered, this, [this]() {
        const int idx = (m_contextMenuEntryIndex >= 0 && m_contextMenuEntryIndex < entryCount())
                ? m_contextMenuEntryIndex : pickEntryIndexModal(QCursor::pos());
        if (idx < 0)
            return;
        ScribbleDialog dlg(m_doc, this);
        if (dlg.exec() == QDialog::Accepted)
            setIconForEntry(idx, dlg.savedIconPath());
    });

    QAction* chooseAct = appearanceMenu->addAction(tr("Choose image…"));
    connect(chooseAct, &QAction::triggered, this, [this, pickEntry]() {
        const int idx = pickEntry();
        if (idx < 0)
            return;

        QString formats;
        for (const QByteArray& ba : QImageReader::supportedImageFormats())
            formats += QString("*.%1 ").arg(QString(ba).toLower());

        QString path = QFileDialog::getOpenFileName(
            this, tr("Select icon image"),
            entryIconPath(idx),
            tr("Images (%1)").arg(formats));
        if (!path.isEmpty())
            setIconForEntry(idx, path);
    });

    QAction* resetAct = appearanceMenu->addAction(tr("Reset icon"));
    connect(resetAct, &QAction::triggered, this, [this, pickEntry]() {
        const int idx = pickEntry();
        if (idx < 0)
            return;
        setIconForEntry(idx, QString());
    });

    appearanceMenu->addSeparator();

    QAction* colorAct = appearanceMenu->addAction(tr("Button color…"));
    connect(colorAct, &QAction::triggered, this, [this, pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        if (idx < 0)
            return;
        QColor initial = Qt::white;
        if (const LevelPreset* appearance = entryAppearancePreset(idx))
            if (appearance->color.isValid())
                initial = appearance->color;
        const QColor chosen = QColorDialog::getColor(initial, this, tr("Entry button color"));
        if (!chosen.isValid())
            return;
        updateAppearance(idx, [&chosen](LevelPreset& preset) {
            preset.color = chosen;
        });
    });

    QAction* resetColorAct = appearanceMenu->addAction(tr("Reset button color"));
    connect(resetColorAct, &QAction::triggered, this, [pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        updateAppearance(idx, [](LevelPreset& preset) {
            preset.color = QColor();
        });
    });

    QAction* labelColorAct = appearanceMenu->addAction(tr("Label color…"));
    connect(labelColorAct, &QAction::triggered, this, [this, pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        if (idx < 0)
            return;
        QColor initial = Qt::black;
        if (const LevelPreset* appearance = entryAppearancePreset(idx))
            if (appearance->labelColor.isValid())
                initial = appearance->labelColor;
        const QColor chosen = QColorDialog::getColor(initial, this, tr("Entry label color"));
        if (!chosen.isValid())
            return;
        updateAppearance(idx, [&chosen](LevelPreset& preset) {
            preset.labelColor = chosen;
        });
    });

    QAction* resetLabelColorAct = appearanceMenu->addAction(tr("Reset label color"));
    connect(resetLabelColorAct, &QAction::triggered, this, [pickEntry, updateAppearance]() {
        const int idx = pickEntry();
        updateAppearance(idx, [](LevelPreset& preset) {
            preset.labelColor = QColor();
        });
    });

    return appearanceMenu;
}

// ---- External input -------------------------------------------------------

void MultiButtonWidget::setPage(int pNum)
{
    VCWidget::setPage(pNum);
    syncAllInputSourcePages();
}

void MultiButtonWidget::showEvent(QShowEvent* event)
{
    VCWidget::showEvent(event);
    syncDynamicEntryCountLayout();
    recalcLayoutSize();
    if (m_doc != nullptr && m_doc->mode() == Doc::Operate)
        syncAllInputSourcePages();
}

void MultiButtonWidget::assignInputSource(const QSharedPointer<QLCInputSource>& src, quint8 id)
{
    if (!src.isNull() && src->isValid())
        src->setPage(quint16(page()));
    setInputSource(src, id);
}

void MultiButtonWidget::syncAllInputSourcePages()
{
    const QList<quint8> globalIds = {
        triggerInputSourceId,
        popupInputSourceId,
        automationInputSourceId,
        presetChooseInputSourceId,
        entrySelectInputSourceId,
        spreadPageInputSourceId
    };

    for (quint8 id : globalIds)
    {
        QSharedPointer<QLCInputSource> src = inputSource(id);
        if (!src.isNull() && src->isValid())
            src->setPage(quint16(page()));
    }

    for (int s = 0; s < MBInputId::kMaxSpreadSlots; ++s)
    {
        QSharedPointer<QLCInputSource> src = inputSource(MBInputId::spreadSlot(s));
        if (!src.isNull() && src->isValid())
            src->setPage(quint16(page()));
    }

    for (int i = 0; i < MBInputId::kMaxEntryInputs; ++i)
    {
        QSharedPointer<QLCInputSource> src = inputSource(MBInputId::entryTrigger(i));
        if (!src.isNull() && src->isValid())
            src->setPage(quint16(page()));
    }
}

void MultiButtonWidget::syncAutomationSuspendDefault()
{
    const QSharedPointer<QLCInputSource> src = inputSource(presetChooseInputSourceId);
    if (!src.isNull() && src->isValid())
        m_automationSuspended = true;
}

bool MultiButtonWidget::isOnInactiveFrameSubPage() const
{
    if (m_doc == nullptr || m_doc->mode() != Doc::Operate)
        return false;
    if (isEnabled() || m_disableState)
        return false;

    for (QWidget* w = parentWidget(); w != nullptr; w = w->parentWidget())
    {
        const VCFrame* frame = qobject_cast<const VCFrame*>(w);
        if (frame != nullptr && frame->multipageMode())
            return true;
    }
    return false;
}

bool MultiButtonWidget::acceptsBackgroundInput() const
{
    return m_receiveInputOnInactiveFramePage && isOnInactiveFrameSubPage();
}

bool MultiButtonWidget::acceptsOperationalInput() const
{
    if (m_doc == nullptr || m_doc->mode() != Doc::Operate)
        return false;
    if (isEnabled() && isVisible())
        return true;
    return acceptsBackgroundInput();
}

void MultiButtonWidget::sendInputFeedback(uchar value,
                                          const QSharedPointer<QLCInputSource>& src)
{
    if (src.isNull() || !src->isValid())
        return;

    if (src->needsUpdate())
        src->updateOuputValue(value);

    if (acceptsInput())
    {
        sendFeedback(value, src);
        return;
    }

    if (!acceptsBackgroundInput())
        return;

    const QVariant extra = src->feedbackExtraParams(QLCInputFeedback::UpperValue);
    m_doc->inputOutputMap()->sendFeedBack(
        src->universe(), src->channel(), value,
        extra.isValid() ? extra : src->feedbackExtraParams(QLCInputFeedback::UpperValue));
}

void MultiButtonWidget::sendPresetChooseFeedback(uchar value)
{
    sendInputFeedback(value, inputSource(presetChooseInputSourceId));
}

void MultiButtonWidget::handlePresetChooseInput(uchar value)
{
    if (value == 0)
    {
        m_automationSuspended = true;
        sendPresetChooseFeedback(0);
        return;
    }

    m_automationSuspended = false;

    if (m_automationProfiles.isEmpty())
        return;

    const int idx = qBound(0, int(value) - 1, m_automationProfiles.size() - 1);
    const bool profileChanged = (idx != m_activeAutomationProfile);

    m_activeAutomationProfile = idx;
    m_automationPulseCounter = 0;

    const QVector<int> allowed = buildAllowedAutomationSlots(m_automationProfiles.at(idx));
    if (!allowed.isEmpty() && profileChanged)
    {
        bool currentAllowed = false;
        for (int slot : allowed)
        {
            if (slot == m_currentIndex)
            {
                currentAllowed = true;
                break;
            }
        }

        if (!currentAllowed)
        {
            const int snap = allowed.first();
            activateAutomationLive(snap);
        }
    }

    sendPresetChooseFeedback(value);
    updateFeedback();
    update();
}

void MultiButtonWidget::handleSpreadPageInput(uchar value)
{
    if (!spreadPagingActive())
        return;

    const int pc = spreadPageCount();
    const int next = qBound(0, int(value), pc - 1);
    if (next == m_spreadPageIndex)
        return;

    m_spreadPageIndex = next;
    recalcLayoutSize();
    update();
}

void MultiButtonWidget::handleCommitInput(uchar value)
{
    if (!stagingActive())
        return;

    const QLCInputSource* src = inputSource(commitInputSourceId).data();
    uchar upper = 255;
    if (src != nullptr)
    {
        upper = src->feedbackValue(QLCInputFeedback::UpperValue);
        if (upper == 0)
            upper = 255;
    }

    if (value == 0)
    {
        m_commitInputLastValue = 0;
        return;
    }

    if (m_commitInputLastValue != upper && value == upper)
        commitStaged();

    m_commitInputLastValue = value;
}

void MultiButtonWidget::handleEntrySelectInput(uchar value)
{
    if (entryCount() == 0)
        return;

    QLCInputSource* src = inputSource(entrySelectInputSourceId).data();
    const int slot = slotFromInputValue(value, src);

    if (m_entrySelectDebounceTime.isValid()
        && m_entrySelectDebounceTime.elapsed() < 100
        && slot == m_entrySelectDebounceSlot)
    {
        syncEntrySelectInputOutput(value);
        return;
    }
    m_entrySelectDebounceSlot = slot;
    m_entrySelectDebounceTime.restart();

    if (stagingActive() || widgetLinkUsesInternalStaging())
    {
        if (value == 0)
            return;

        const int idx = slotToEntryIndex(slot);
        syncEntrySelectInputOutput(value);
        stageEntry(idx);
        return;
    }

    if (value == 0)
        return;

    const int idx = slotToEntryIndex(slot);

    syncEntrySelectInputOutput(value);

    const bool selectionChanged = !m_entrySelectPreviewActive
                                  || m_entrySelectPreviewIndex != idx;
    m_entrySelectPreviewActive = true;
    m_entrySelectPreviewIndex  = idx;

    if (selectionChanged)
        update();

    if (mode() == Doc::Operate)
    {
        if (m_layout == MultiButtonLayout::Single)
        {
            if (!m_entrySelectOverlay)
                openEntrySelectPopup();
            else if (selectionChanged)
                updateEntrySelectPopupHighlight();
        }
        armEntrySelectPopupDismissTimer();
    }
}

void MultiButtonWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    const bool visibleInput = acceptsInput();
    const bool backgroundInput = acceptsBackgroundInput();
    if (!visibleInput && !backgroundInput)
        return;

    const bool opInput = acceptsOperationalInput();
    const quint32 pagedCh = (quint32(page()) << 16) | (channel & 0xFFFF);

    if (checkInputSource(universe, pagedCh, value, sender(), widgetRecallInputSourceId))
    {
        if (opInput)
            applyWidgetRecallInput(value);
        return;
    }

    if (checkInputSource(universe, pagedCh, value, sender(), triggerInputSourceId))
    {
        if (opInput && m_triggerLastValue == 0 && value > 0)
            cycleNext();
        if (value == 0)
            m_triggerLastValue = 0;
        else if (value > 0)
            m_triggerLastValue = value;
        return;
    }
    if (checkInputSource(universe, pagedCh, value, sender(), automationInputSourceId))
    {
        if (opInput && m_automationLastValue == 0 && value > 0
            && m_automationEnabled && !m_automationSuspended)
        {
            onAutomationTrigger();
        }
        if (value == 0)
            m_automationLastValue = 0;
        else if (value > 0)
            m_automationLastValue = value;
        return;
    }
    if (checkInputSource(universe, pagedCh, value, sender(), presetChooseInputSourceId))
    {
        if (opInput)
            handlePresetChooseInput(value);
        return;
    }
    if (checkInputSource(universe, pagedCh, value, sender(), entrySelectInputSourceId))
    {
        if (opInput)
            handleEntrySelectInput(value);
        return;
    }
    if (checkInputSource(universe, pagedCh, value, sender(), spreadPageInputSourceId))
    {
        if (opInput)
            handleSpreadPageInput(value);
        return;
    }
    if (checkInputSource(universe, pagedCh, value, sender(), commitInputSourceId))
    {
        if (opInput)
            handleCommitInput(value);
        return;
    }

    if (checkInputSource(universe, pagedCh, value, sender(), popupInputSourceId))
    {
        if (value > 0)
            showPopupMenu(mapToGlobal(rect().center()));
        return;
    }

    if (!visibleInput)
        return;

    if (spreadPagingActive())
    {
        const int spp = spreadSlotsPerPage(true);
        for (int slot = 0; slot < spp && slot < m_spreadSlotInputs.size(); ++slot)
        {
            if (checkInputSource(universe, pagedCh, value, sender(), MBInputId::spreadSlot(slot)))
            {
                if (opInput && value > 0)
                {
                    if (stagingActive() || widgetLinkUsesInternalStaging())
                        stageEntry(m_spreadPageIndex * spp + slot);
                    else
                        activateFromGlobalSlot(m_spreadPageIndex * spp + slot);
                }
                return;
            }
        }
    }

    for (int i = 0; i < entryCount() && i < MBInputId::kMaxEntryInputs; ++i)
    {
        if (checkInputSource(universe, pagedCh, value, sender(), MBInputId::entryTrigger(i)))
        {
            if (entryIsFlash(i))
            {
                if (value > 0)
                    beginFlashHold(i);
                else
                    endFlashHold();
            }
            else if (opInput && value > 0)
            {
                if (stagingActive() || widgetLinkUsesInternalStaging())
                    stageEntry(i);
                else
                    activate(i);
            }
            return;
        }
    }

}

void MultiButtonWidget::slotKeyPressed(const QKeySequence& keySequence)
{
    if (!acceptsInput())
        return;

    const QKeySequence key = stripKeySequence(keySequence);
    if (key.isEmpty())
        return;

    if (spreadPagingActive())
    {
        const int spp = spreadSlotsPerPage(true);
        for (int slot = 0; slot < spp && slot < m_spreadSlotKeys.size(); ++slot)
        {
            if (stripKeySequence(m_spreadSlotKeys.at(slot)) == key)
            {
                activateFromGlobalSlot(m_spreadPageIndex * spp + slot);
                return;
            }
        }
    }

    for (int i = 0; i < entryCount() && i < MBInputId::kMaxEntryInputs; ++i)
    {
        if (stripKeySequence(entryKeySource(i)) == key)
        {
            activate(i);
            return;
        }
    }
}

void MultiButtonWidget::updateFeedback()
{
    sendFeedback(m_currentIndex >= 0 ? 255 : 0, triggerInputSourceId);

    if (spreadPagingActive())
        sendFeedback(uchar(m_spreadPageIndex), spreadPageInputSourceId);

    if (spreadPagingActive() && m_currentIndex >= 0)
    {
        const int spp = spreadSlotsPerPage(true);
        if (spp > 0)
        {
            const int local = m_currentIndex % spp;
            if (local >= 0 && local < m_spreadSlotInputs.size())
            {
                const auto slotSrc = m_spreadSlotInputs.at(local);
                if (!slotSrc.isNull() && slotSrc->isValid())
                    sendFeedback(255, MBInputId::spreadSlot(local));
            }
        }
    }

    for (int i = 0; i < entryCount() && i < MBInputId::kMaxEntryInputs; ++i)
    {
        if (i == m_currentIndex)
            sendFeedback(255, MBInputId::entryTrigger(i));
    }

    QSharedPointer<QLCInputSource> src = inputSource(entrySelectInputSourceId);
    if (src.isNull() || !src->isValid() || !src->needsUpdate())
        return;

    const int slotCount = selectableSlotCount();
    if (slotCount <= 0)
        return;

    int slot = 0;
    if (m_currentIndex >= 0)
        slot = m_currentIndex;
    else if (m_addOffAtEnd)
        slot = slotCount - 1;

    src->updateOuputValue(entrySelectOutputValueForSlot(slot));
}

// ---- Properties -----------------------------------------------------------

void MultiButtonWidget::editProperties()
{
    if (mode() != Doc::Design) return;

    MultiButtonConfigDialog dlg(
        m_doc,
        m_mode,
        m_functionIds,
        m_functionLabels,
        m_iconPaths,
        m_levelChannelBindings,
        m_levelPresets,
        m_widgetEntryAppearance,
        m_longPressMs,
        m_addOffAtEnd,
        m_monitorChannelValues,
        m_receiveInputOnInactiveFramePage,
        m_layout,
        m_spreadColumns,
        m_spreadRows,
        m_spreadHMargin,
        m_spreadVMargin,
        m_spreadTileWidth,
        m_spreadTileHeight,
        m_spreadPages,
        m_automationEnabled,
        m_automationProfiles,
        m_activeAutomationProfile,
        inputSource(triggerInputSourceId),
        inputSource(popupInputSourceId),
        inputSource(automationInputSourceId),
        inputSource(presetChooseInputSourceId),
        inputSource(entrySelectInputSourceId),
        inputSource(spreadPageInputSourceId),
        inputSource(commitInputSourceId),
        m_stageBeforeCommit,
        m_entrySelectAutoCommit,
        m_logPresetChanges,
        m_functionEntryInputs,
        m_functionEntryKeys,
        m_spreadSlotInputs,
        m_spreadSlotKeys,
        m_functionEntryFlash,
        m_functionEntryLabelColors,
        m_widgetTargetId,
        m_widgetOutputIndex,
        m_widgetParameter,
        m_widgetLiveInputSource,
        m_widgetBusPolicy,
        page(),
        this);

    if (dlg.exec() != QDialog::Accepted) return;

    setWidgetMode(dlg.widgetMode());
    setEntries(dlg.functionIds(), dlg.functionLabels(), dlg.iconPaths());
    m_functionEntryFlash = dlg.functionEntryFlash();
    m_functionEntryLabelColors = dlg.functionEntryLabelColors();
    while (m_functionEntryFlash.size() < m_functionIds.size())
        m_functionEntryFlash.append(false);
    while (m_functionEntryFlash.size() > m_functionIds.size())
        m_functionEntryFlash.removeLast();
    while (m_functionEntryLabelColors.size() < m_functionIds.size())
        m_functionEntryLabelColors.append(QColor());
    while (m_functionEntryLabelColors.size() > m_functionIds.size())
        m_functionEntryLabelColors.removeLast();
    m_functionEntryInputs = dlg.functionEntryInputs();
    m_functionEntryKeys     = dlg.functionEntryKeys();
    for (int i = 0; i < m_functionEntryInputs.size(); ++i)
        setEntryInputSource(i, m_functionEntryInputs.at(i));
    for (int i = 0; i < m_functionEntryKeys.size(); ++i)
        setEntryKeySource(i, m_functionEntryKeys.at(i));
    setLevelConfig(dlg.levelChannelBindings(), dlg.levelPresets());
    setWidgetEntryAppearance(dlg.widgetEntryAppearance());
    setSpreadSlotInputs(dlg.spreadSlotInputs());
    setSpreadSlotKeys(dlg.spreadSlotKeys());
    setLongPressMs(dlg.longPressMs());
    setAddOffAtEnd(dlg.addOffAtEnd());
    setMonitorChannelValues(dlg.monitorChannelValues());
    setReceiveInputOnInactiveFramePage(dlg.receiveInputOnInactiveFramePage());
    setStageBeforeCommit(dlg.stageBeforeCommit());
    setEntrySelectAutoCommit(dlg.entrySelectAutoCommit());
    setLogPresetChanges(dlg.logPresetChanges());
    m_widgetTargetId = dlg.widgetTargetId();
    m_widgetOutputIndex = dlg.widgetOutputIndex();
    m_widgetParameter = dlg.widgetParameter();
    m_widgetLiveInputSource = cloneInputSource(dlg.widgetLiveInputSource());
    m_widgetBusPolicy = dlg.widgetBusPolicy();
    releaseWidgetLiveFaders();
    syncWidgetLiveInputSourceToTarget();
    m_lastResolvedEntryCount = -1;
    setWidgetLayout(dlg.widgetLayout());
    setSpreadColumns(dlg.spreadColumns());
    setSpreadRows(dlg.spreadRows());
    setSpreadHMargin(dlg.spreadHMargin());
    setSpreadVMargin(dlg.spreadVMargin());
    setSpreadTileWidth(dlg.spreadTileWidth());
    setSpreadTileHeight(dlg.spreadTileHeight());
    setSpreadPages(dlg.spreadPages());
    setAutomationEnabled(dlg.automationEnabled());
    setAutomationProfiles(dlg.automationProfiles(), dlg.activeAutomationProfile());
    assignInputSource(dlg.triggerInputSource(), triggerInputSourceId);
    assignInputSource(dlg.popupInputSource(), popupInputSourceId);
    assignInputSource(dlg.automationInputSource(), automationInputSourceId);
    assignInputSource(dlg.presetChooseInputSource(), presetChooseInputSourceId);
    assignInputSource(dlg.entrySelectInputSource(), entrySelectInputSourceId);
    assignInputSource(dlg.spreadPageInputSource(), spreadPageInputSourceId);
    assignInputSource(dlg.commitInputSource(), commitInputSourceId);
    syncAutomationSuspendDefault();
    m_triggerLastValue     = 0;
    m_automationLastValue  = 0;
    m_commitInputLastValue = 0;
    updateDmxRegistration();
    m_doc->setModified();
    update();
}

// ---- Copy ----------------------------------------------------------------

VCWidget* MultiButtonWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    MultiButtonWidget* copy = new MultiButtonWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }
    copy->setWidgetMode(m_mode);
    copy->setEntries(m_functionIds, m_functionLabels, m_iconPaths);
    copy->m_functionEntryKeys = m_functionEntryKeys;
    for (int i = 0; i < m_functionEntryInputs.size(); ++i)
        copy->setEntryInputSource(i, cloneInputSource(m_functionEntryInputs.at(i)));
    for (int i = 0; i < m_functionEntryKeys.size(); ++i)
        copy->setEntryKeySource(i, m_functionEntryKeys.at(i));

    QList<LevelPreset> clonedLevelPresets = m_levelPresets;
    for (LevelPreset& preset : clonedLevelPresets)
        preset.entryInput = cloneInputSource(preset.entryInput);
    copy->setLevelConfig(m_levelChannelBindings, clonedLevelPresets);

    QList<QSharedPointer<QLCInputSource>> clonedSpreadInputs;
    for (const QSharedPointer<QLCInputSource>& src : m_spreadSlotInputs)
        clonedSpreadInputs.append(cloneInputSource(src));
    copy->setSpreadSlotInputs(clonedSpreadInputs);
    copy->setSpreadSlotKeys(m_spreadSlotKeys);
    copy->setLongPressMs(m_longPressMs);
    copy->setAddOffAtEnd(m_addOffAtEnd);
    copy->setMonitorChannelValues(m_monitorChannelValues);
    copy->setReceiveInputOnInactiveFramePage(m_receiveInputOnInactiveFramePage);
    copy->setStageBeforeCommit(m_stageBeforeCommit);
    copy->setEntrySelectAutoCommit(m_entrySelectAutoCommit);
    copy->setLogPresetChanges(m_logPresetChanges);
    copy->m_widgetTargetId = m_widgetTargetId;
    copy->m_widgetOutputIndex = m_widgetOutputIndex;
    copy->m_widgetParameter = m_widgetParameter;
    copy->m_widgetLiveInputSource = cloneInputSource(m_widgetLiveInputSource);
    copy->m_widgetBusPolicy = m_widgetBusPolicy;
    copy->m_widgetEntryAppearance = m_widgetEntryAppearance;
    for (LevelPreset& preset : copy->m_widgetEntryAppearance)
        preset.entryInput = cloneInputSource(preset.entryInput);
    copy->assignInputSource(cloneInputSource(inputSource(commitInputSourceId)),
                            commitInputSourceId);
    copy->setWidgetLayout(m_layout);
    copy->setSpreadColumns(m_spreadColumns);
    copy->setSpreadRows(m_spreadRows);
    copy->setSpreadHMargin(m_spreadHMargin);
    copy->setSpreadVMargin(m_spreadVMargin);
    copy->setSpreadTileWidth(m_spreadTileWidth);
    copy->setSpreadTileHeight(m_spreadTileHeight);
    copy->setSpreadPages(m_spreadPages);
    copy->m_spreadPageIndex = m_spreadPageIndex;
    copy->setAutomationEnabled(m_automationEnabled);
    copy->setAutomationProfiles(m_automationProfiles, m_activeAutomationProfile);
    copy->m_functionEntryFlash = m_functionEntryFlash;
    copy->m_functionEntryLabelColors = m_functionEntryLabelColors;
    copy->setPluginId(KXMLPluginIdVal);
    copy->syncWidgetLiveInputSourceToTarget();
    copy->syncEntryInputSources();
    copy->updateDmxRegistration();
    return copy;
}

void MultiButtonWidget::applyEntryNamesFrom(const MultiButtonWidget* src)
{
    if (!src)
        return;

    const int n = qMin(src->entryCount(), entryCount());
    if (n <= 0)
        return;

    if (m_mode == MultiButtonMode::Function)
    {
        while (m_functionLabels.size() < m_functionIds.size())
            m_functionLabels.append(QString());
        while (m_iconPaths.size() < m_functionIds.size())
            m_iconPaths.append(QString());

        for (int i = 0; i < n; ++i)
        {
            if (src->m_mode == MultiButtonMode::Function)
            {
                if (i < src->m_functionLabels.size())
                    m_functionLabels[i] = src->m_functionLabels.at(i);
                if (i < src->m_iconPaths.size())
                    m_iconPaths[i] = src->m_iconPaths.at(i);
            }
            else if (i < src->m_levelPresets.size())
            {
                m_functionLabels[i] = src->m_levelPresets.at(i).label;
                m_iconPaths[i]      = src->m_levelPresets.at(i).iconPath;
            }
        }
    }
    else if (m_mode == MultiButtonMode::Widget)
    {
        syncWidgetEntryAppearanceCount();
        for (int i = 0; i < n && i < m_widgetEntryAppearance.size(); ++i)
        {
            LevelPreset& tgt = m_widgetEntryAppearance[i];
            const LevelPreset* s = nullptr;
            if (src->m_mode == MultiButtonMode::Widget && i < src->m_widgetEntryAppearance.size())
                s = &src->m_widgetEntryAppearance.at(i);
            else if (src->m_mode == MultiButtonMode::Level && i < src->m_levelPresets.size())
                s = &src->m_levelPresets.at(i);

            if (s)
            {
                tgt.label = s->label;
                tgt.iconPath = s->iconPath;
                tgt.color = s->color;
                tgt.labelColor = s->labelColor;
                tgt.hideName = s->hideName;
            }
            else
            {
                tgt.label = src->m_functionLabels.value(i);
                tgt.iconPath = src->m_iconPaths.value(i);
                tgt.color = QColor();
                tgt.labelColor = QColor();
                tgt.hideName = false;
            }
        }
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            if (i >= m_levelPresets.size())
                break;

            LevelPreset& tgt = m_levelPresets[i];
            if (src->m_mode == MultiButtonMode::Level && i < src->m_levelPresets.size())
            {
                const LevelPreset& s = src->m_levelPresets.at(i);
                tgt.label    = s.label;
                tgt.iconPath = s.iconPath;
                tgt.color    = s.color;
                tgt.labelColor = s.labelColor;
                tgt.hideName = s.hideName;
            }
            else
            {
                tgt.label    = src->m_functionLabels.value(i);
                tgt.iconPath = src->m_iconPaths.value(i);
            }
        }
    }

    m_iconCache.clear();
}

void MultiButtonWidget::applyChannelBindingsFrom(const MultiButtonWidget* src)
{
    if (!src)
        return;

    m_levelChannelBindings = src->m_levelChannelBindings;
    const int bindCount = m_levelChannelBindings.size();
    for (LevelPreset& preset : m_levelPresets)
    {
        while (preset.values.size() < bindCount)
            preset.values.append(0);
        while (preset.values.size() > bindCount)
            preset.values.removeLast();
    }

    updateDmxRegistration();
}

void MultiButtonWidget::applyFunctionAssignmentsFrom(const MultiButtonWidget* src)
{
    if (!src)
        return;

    setEntries(src->m_functionIds, src->m_functionLabels, src->m_iconPaths);
}

void MultiButtonWidget::applyLevelValuesFrom(const MultiButtonWidget* src)
{
    if (!src)
        return;

    const int n = qMin(m_levelPresets.size(), src->m_levelPresets.size());
    const int bindCount = m_levelChannelBindings.size();
    for (int i = 0; i < n; ++i)
    {
        m_levelPresets[i].values        = src->m_levelPresets.at(i).values;
        m_levelPresets[i].valueFormulas = src->m_levelPresets.at(i).valueFormulas;
        alignLevelPresetArrays(m_levelPresets[i], bindCount);
    }
    resetLevelWriteCache();
}

QList<QPair<VCWidget::PastePropertyGroup, QString>>
MultiButtonWidget::pasteablePropertyGroups() const
{
    QList<QPair<PastePropertyGroup, QString>> groups = VCWidget::pasteablePropertyGroups();
    groups << qMakePair(PasteSpecific0, tr("Entries — Mode (Function / Level / Widget)"));
    groups << qMakePair(PasteSpecific1, tr("Entries — Names (label, icon, color, hide name)"));
    groups << qMakePair(PasteSpecific2, tr("Entries — Channels (level bindings)"));
    groups << qMakePair(PasteSpecific3, tr("Entries — Function assignments"));
    groups << qMakePair(PasteSpecific4, tr("Entries — Level DMX values"));
    groups << qMakePair(PasteSpecific5, tr("Layout (Single/Spread, columns, pages, tile size)"));
    groups << qMakePair(PasteSpecific6, tr("Automation (profiles, enabled, active profile)"));
    groups << qMakePair(PasteSpecific7, tr("General (long-press, off at end, monitor)"));
    return groups;
}

void MultiButtonWidget::applyPropertiesFrom(const VCWidget* source, PastePropertyGroups flags)
{
    const MultiButtonWidget* src = qobject_cast<const MultiButtonWidget*>(source);
    if (src == nullptr)
    {
        VCWidget::applyPropertiesFrom(source, flags);
        return;
    }

    if (flags & PasteSpecific0)
    {
        setWidgetMode(src->m_mode);
        m_widgetTargetId = src->m_widgetTargetId;
        m_widgetOutputIndex = src->m_widgetOutputIndex;
        m_widgetParameter = src->m_widgetParameter;
        m_widgetLiveInputSource = cloneInputSource(src->m_widgetLiveInputSource);
        m_widgetBusPolicy = src->m_widgetBusPolicy;
        releaseWidgetLiveFaders();
        syncWidgetLiveInputSourceToTarget();
        updateDmxRegistration();
    }

    if (flags & PasteSpecific1)
        applyEntryNamesFrom(src);

    if (flags & PasteSpecific2)
        applyChannelBindingsFrom(src);

    if (flags & PasteSpecific3)
        applyFunctionAssignmentsFrom(src);

    if (flags & PasteSpecific4)
        applyLevelValuesFrom(src);

    if (flags & PasteSpecific5)
    {
        setWidgetLayout(src->m_layout);
        setSpreadColumns(src->m_spreadColumns);
        setSpreadRows(src->m_spreadRows);
        setSpreadHMargin(src->m_spreadHMargin);
        setSpreadVMargin(src->m_spreadVMargin);
        setSpreadTileWidth(src->m_spreadTileWidth);
        setSpreadTileHeight(src->m_spreadTileHeight);
        setSpreadPages(src->m_spreadPages);
    }

    if (flags & PasteSpecific6)
    {
        setAutomationProfiles(src->m_automationProfiles, src->m_activeAutomationProfile);
        setAutomationEnabled(src->m_automationEnabled);
    }

    if (flags & PasteSpecific7)
    {
        setLongPressMs(src->m_longPressMs);
        setAddOffAtEnd(src->m_addOffAtEnd);
        setMonitorChannelValues(src->m_monitorChannelValues);
        setReceiveInputOnInactiveFramePage(src->m_receiveInputOnInactiveFramePage);
        setStageBeforeCommit(src->m_stageBeforeCommit);
        setEntrySelectAutoCommit(src->m_entrySelectAutoCommit);
        setLogPresetChanges(src->m_logPresetChanges);
        assignInputSource(cloneInputSource(src->inputSource(commitInputSourceId)),
                          commitInputSourceId);
    }

    VCWidget::applyPropertiesFrom(source, flags);
    m_doc->setModified();
    update();
}

// ---- Cross-project clipboard -----------------------------------------------

void MultiButtonWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    obj["widgetMode"]           = modeToString(m_mode);
    QJsonObject widgetLink;
    widgetLink["targetWidgetId"] = QString::number(m_widgetTargetId);
    widgetLink["outputIndex"] = m_widgetOutputIndex;
    widgetLink["parameter"] = m_widgetParameter;
    widgetLink["busPolicy"] = widgetBusPolicyToString(m_widgetBusPolicy);
    if (!m_widgetLiveInputSource.isNull() && m_widgetLiveInputSource->isValid())
    {
        widgetLink["liveInputUniverse"] = int(m_widgetLiveInputSource->universe());
        widgetLink["liveInputChannel"] = int(m_widgetLiveInputSource->channel());
    }
    obj["widgetLink"] = widgetLink;
    obj["longPressMs"]          = m_longPressMs;
    obj["addOffAtEnd"]          = m_addOffAtEnd;
    obj["monitorChannelValues"] = m_monitorChannelValues;
    obj["receiveInputOnInactiveFramePage"] = m_receiveInputOnInactiveFramePage;
    obj["stageBeforeCommit"] = m_stageBeforeCommit;
    obj["entrySelectAutoCommit"] = m_entrySelectAutoCommit;
    obj["logPresetChanges"] = m_logPresetChanges;

    QJsonObject spread;
    spread["enabled"]   = (m_layout == MultiButtonLayout::Spread);
    spread["columns"]   = m_spreadColumns;
    spread["rows"]      = m_spreadRows;
    spread["hMargin"]   = m_spreadHMargin;
    spread["vMargin"]   = m_spreadVMargin;
    spread["tileWidth"]  = m_spreadTileWidth;
    spread["tileHeight"] = m_spreadTileHeight;
    spread["pages"]      = m_spreadPages;
    if (!m_spreadSlotInputs.isEmpty() || !m_spreadSlotKeys.isEmpty())
    {
        QJsonArray slotArr;
        const int n = qMax(m_spreadSlotInputs.size(), m_spreadSlotKeys.size());
        for (int s = 0; s < n; ++s)
        {
            const QSharedPointer<QLCInputSource> src =
                (s < m_spreadSlotInputs.size()) ? m_spreadSlotInputs.at(s)
                                                : QSharedPointer<QLCInputSource>();
            const QKeySequence key = (s < m_spreadSlotKeys.size()) ? m_spreadSlotKeys.at(s)
                                                                   : QKeySequence();
            if ((src.isNull() || !src->isValid()) && key.isEmpty())
                continue;
            QJsonObject io;
            if (!src.isNull() && src->isValid())
            {
                io["universe"] = (int) src->universe();
                io["channel"]  = (int) src->channel();
            }
            if (!key.isEmpty())
                io["key"] = key.toString();
            slotArr.append(io);
        }
        if (!slotArr.isEmpty())
            spread["slotInputs"] = slotArr;
    }
    obj["spread"] = spread;

    obj["automationEnabled"]      = m_automationEnabled;
    obj["activeAutomationProfile"] = m_activeAutomationProfile;
    QJsonArray autoProfiles;
    for (const MultiButtonAutomationProfile& ap : m_automationProfiles)
    {
        QJsonObject po;
        po["name"]         = ap.name;
        po["mode"]         = automationModeToString(ap.mode);
        po["stepMin"]      = ap.stepMin;
        po["stepMax"]      = ap.stepMax;
        po["multiplier"]   = ap.multiplier;
        po["excludeMask"]  = QString::number(ap.excludeMask, 16);
        po["beatOffset"]   = ap.beatOffset;
        autoProfiles.append(po);
    }
    obj["automationProfiles"] = autoProfiles;

    QJsonArray funcs;
    for (int i = 0; i < m_functionIds.size(); ++i)
    {
        Function *f = doc->function(m_functionIds.at(i));
        QJsonObject entry;
        entry["name"]     = f ? f->name() : QString();
        entry["label"]    = m_functionLabels.value(i);
        entry["iconPath"] = m_iconPaths.value(i);
        if (i < m_functionEntryFlash.size() && m_functionEntryFlash.at(i))
            entry["flash"] = true;
        if (i < m_functionEntryLabelColors.size()
            && m_functionEntryLabelColors.at(i).isValid())
        {
            entry["labelColor"] = m_functionEntryLabelColors.at(i).name(QColor::HexRgb);
        }
        funcs.append(entry);
    }
    obj["entries"] = funcs;

    QJsonArray bindArr;
    for (const LevelChannelBinding& b : m_levelChannelBindings)
    {
        Fixture* fxi = doc->fixture(b.fixtureId);
        QJsonObject bo;
        bo["fixtureName"] = fxi ? fxi->name() : QString();
        bo["channel"]     = (int) b.channel;
        bindArr.append(bo);
    }
    obj["levelChannelBindings"] = bindArr;

    // Legacy keys for older clipboard format (ignored on load if bindings present)
    obj["levelFixtureName"] = QString();
    obj["levelChannels"]    = QJsonArray();

    QJsonArray presetArr;
    for (const LevelPreset& preset : m_levelPresets)
    {
        QJsonObject po;
        po["label"]    = preset.label;
        po["iconPath"] = preset.iconPath;
        if (preset.color.isValid())
            po["color"] = preset.color.name(QColor::HexRgb);
        if (preset.hideName)
            po["hideName"] = true;
        if (preset.flashOnActivate)
            po["flash"] = true;
        if (preset.labelColor.isValid())
            po["labelColor"] = preset.labelColor.name(QColor::HexRgb);
        QJsonArray vals;
        for (quint8 v : preset.values)
            vals.append(v);
        po["values"] = vals;
        QJsonArray formulas;
        for (const QString& f : preset.valueFormulas)
            formulas.append(f);
        if (!formulas.isEmpty())
            po["valueFormulas"] = formulas;
        if (!preset.entryInput.isNull() && preset.entryInput->isValid())
        {
            po["inputUniverse"] = (int) preset.entryInput->universe();
            po["inputChannel"]  = (int) preset.entryInput->channel();
        }
        if (!preset.entryKey.isEmpty())
            po["inputKey"] = preset.entryKey.toString();
        presetArr.append(po);
    }
    obj["levelPresets"] = presetArr;

    QJsonArray widgetAppearanceArr;
    for (const LevelPreset& preset : m_widgetEntryAppearance)
    {
        QJsonObject po;
        po["label"] = preset.label;
        po["iconPath"] = preset.iconPath;
        if (preset.color.isValid())
            po["color"] = preset.color.name(QColor::HexRgb);
        if (preset.hideName)
            po["hideName"] = true;
        if (preset.labelColor.isValid())
            po["labelColor"] = preset.labelColor.name(QColor::HexRgb);
        if (!preset.entryInput.isNull() && preset.entryInput->isValid())
        {
            po["inputUniverse"] = int(preset.entryInput->universe());
            po["inputChannel"] = int(preset.entryInput->channel());
        }
        if (!preset.entryKey.isEmpty())
            po["inputKey"] = preset.entryKey.toString();
        widgetAppearanceArr.append(po);
    }
    if (!widgetAppearanceArr.isEmpty())
        obj["widgetEntryAppearance"] = widgetAppearanceArr;

    QJsonArray funcInArr;
    for (int i = 0; i < m_functionEntryInputs.size(); ++i)
    {
        const QSharedPointer<QLCInputSource>& src = m_functionEntryInputs.at(i);
        const QKeySequence key = (i < m_functionEntryKeys.size())
                                     ? m_functionEntryKeys.at(i)
                                     : QKeySequence();
        if ((src.isNull() || !src->isValid()) && key.isEmpty())
            continue;
        QJsonObject io;
        if (!src.isNull() && src->isValid())
        {
            io["universe"] = (int) src->universe();
            io["channel"]  = (int) src->channel();
        }
        if (!key.isEmpty())
            io["key"] = key.toString();
        funcInArr.append(io);
    }
    if (!funcInArr.isEmpty())
        obj["functionEntryInputs"] = funcInArr;
}

void MultiButtonWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    setWidgetMode(stringToMode(obj["widgetMode"].toString()));
    const QJsonObject widgetLink = obj["widgetLink"].toObject();
    m_widgetTargetId = widgetLink["targetWidgetId"].toString(
            QString::number(VCWidget::invalidId())).toUInt();
    m_widgetOutputIndex = qMax(0, widgetLink["outputIndex"].toInt(0));
    m_widgetParameter = qMax(0, widgetLink["parameter"].toInt(0));
    if (widgetLink.contains(QStringLiteral("busPolicy")))
    {
        m_widgetBusPolicy = stringToWidgetBusPolicy(
                widgetLink["busPolicy"].toString());
    }
    if (widgetLink.contains(QStringLiteral("liveInputUniverse"))
            && widgetLink.contains(QStringLiteral("liveInputChannel")))
    {
        m_widgetLiveInputSource.reset(new QLCInputSource(
                quint32(qMax(0, widgetLink["liveInputUniverse"].toInt())),
                quint32(qMax(0, widgetLink["liveInputChannel"].toInt()))));
    }
    else
        m_widgetLiveInputSource.clear();
    syncWidgetLiveInputSourceToTarget();
    releaseWidgetLiveFaders();
    updateDmxRegistration();
    m_lastResolvedEntryCount = -1;
    m_longPressMs          = obj["longPressMs"].toInt(500);
    m_addOffAtEnd          = obj["addOffAtEnd"].toBool(false);
    m_monitorChannelValues = obj["monitorChannelValues"].toBool(false);
    m_receiveInputOnInactiveFramePage = obj["receiveInputOnInactiveFramePage"].toBool(false);
    setStageBeforeCommit(obj["stageBeforeCommit"].toBool(false));
    m_entrySelectAutoCommit = obj.contains(QStringLiteral("entrySelectAutoCommit"))
        ? obj["entrySelectAutoCommit"].toBool(true)
        : true;
    m_logPresetChanges = obj["logPresetChanges"].toBool(false);

    if (obj.contains("spread"))
    {
        const QJsonObject spread = obj["spread"].toObject();
        m_layout = spread["enabled"].toBool(false) ? MultiButtonLayout::Spread
                                                     : MultiButtonLayout::Single;
        m_spreadColumns    = spread["columns"].toInt(0);
        m_spreadRows       = spread["rows"].toInt(1);
        m_spreadHMargin    = spread["hMargin"].toInt(4);
        m_spreadVMargin    = spread["vMargin"].toInt(4);
        m_spreadTileWidth  = spread["tileWidth"].toInt(80);
        m_spreadTileHeight = spread["tileHeight"].toInt(60);
        m_spreadPages      = spread["pages"].toInt(0);
    }

    m_automationEnabled = obj["automationEnabled"].toBool(false);
    m_activeAutomationProfile = obj["activeAutomationProfile"].toInt(0);
    QList<MultiButtonAutomationProfile> loadedAuto;
    for (const QJsonValue& av : obj["automationProfiles"].toArray())
    {
        QJsonObject ao = av.toObject();
        MultiButtonAutomationProfile ap;
        ap.name         = ao["name"].toString();
        ap.mode         = stringToAutomationMode(ao["mode"].toString());
        ap.stepMin      = ao["stepMin"].toInt(1);
        ap.stepMax      = ao["stepMax"].toInt(1);
        ap.multiplier   = qMax(1, ao["multiplier"].toInt(1));
        ap.beatOffset   = ao["beatOffset"].toInt(0);
        ap.excludeMask  = ao["excludeMask"].toString().toUInt(nullptr, 16);
        const int maxOff = qMax(0, ap.multiplier - 1);
        ap.beatOffset = qBound(0, ap.beatOffset, maxOff);
        loadedAuto.append(ap);
    }
    setAutomationProfiles(loadedAuto, m_activeAutomationProfile);

    QList<quint32> ids;
    QStringList    labels;
    QStringList    icons;
    for (const QJsonValue &v : obj["entries"].toArray())
    {
        QJsonObject e = v.toObject();
        Function *f = resolveFunctionByName(e["name"].toString(), doc);
        ids    << (f ? f->id() : Function::invalidId());
        labels << e["label"].toString();
        icons  << e["iconPath"].toString();
    }
    setEntries(ids, labels, icons);

    QList<bool> loadedFlash;
    QList<QColor> loadedLabelColors;
    for (const QJsonValue& v : obj["entries"].toArray())
    {
        QJsonObject e = v.toObject();
        loadedFlash.append(e["flash"].toBool(false));
        const QString lc = e["labelColor"].toString();
        if (!lc.isEmpty())
        {
            const QColor c(lc);
            loadedLabelColors.append(c.isValid() ? c : QColor());
        }
        else
            loadedLabelColors.append(QColor());
    }
    m_functionEntryFlash = loadedFlash;
    m_functionEntryLabelColors = loadedLabelColors;
    while (m_functionEntryFlash.size() < m_functionIds.size())
        m_functionEntryFlash.append(false);
    while (m_functionEntryFlash.size() > m_functionIds.size())
        m_functionEntryFlash.removeLast();
    while (m_functionEntryLabelColors.size() < m_functionIds.size())
        m_functionEntryLabelColors.append(QColor());
    while (m_functionEntryLabelColors.size() > m_functionIds.size())
        m_functionEntryLabelColors.removeLast();

    QList<LevelChannelBinding> bindings;

    if (obj.contains("levelChannelBindings"))
    {
        for (const QJsonValue& bv : obj["levelChannelBindings"].toArray())
        {
            QJsonObject bo = bv.toObject();
            QString fixName = bo["fixtureName"].toString();
            quint32 ch      = (quint32) bo["channel"].toInt();

            for (Fixture* fxi : doc->fixtures())
            {
                if (fxi->name() == fixName)
                {
                    LevelChannelBinding b;
                    b.fixtureId = fxi->id();
                    b.channel   = ch;
                    bindings.append(b);
                    break;
                }
            }
        }
    }
    else
    {
        // Legacy single-fixture format
        quint32 levelFxId = UINT_MAX;
        for (Fixture* fxi : doc->fixtures())
        {
            if (fxi->name() == obj["levelFixtureName"].toString())
            {
                levelFxId = fxi->id();
                break;
            }
        }
        for (const QJsonValue& cv : obj["levelChannels"].toArray())
        {
            LevelChannelBinding b;
            b.fixtureId = levelFxId;
            b.channel   = (quint32) cv.toInt();
            bindings.append(b);
        }
    }

    QList<LevelPreset> presets;
    for (const QJsonValue& pv : obj["levelPresets"].toArray())
    {
        QJsonObject po = pv.toObject();
        LevelPreset preset;
        preset.label    = po["label"].toString();
        preset.iconPath = po["iconPath"].toString();
        const QString colorStr = po["color"].toString();
        if (!colorStr.isEmpty())
        {
            const QColor c(colorStr);
            if (c.isValid())
                preset.color = c;
        }
        preset.hideName = po["hideName"].toBool(false);
        preset.flashOnActivate = po["flash"].toBool(false);
        const QString labelColorStr = po["labelColor"].toString();
        if (!labelColorStr.isEmpty())
        {
            const QColor lc(labelColorStr);
            if (lc.isValid())
                preset.labelColor = lc;
        }
        for (const QJsonValue& vv : po["values"].toArray())
            preset.values.append((quint8) vv.toInt());
        for (const QJsonValue& fv : po["valueFormulas"].toArray())
            preset.valueFormulas.append(fv.toString());
        alignLevelPresetArrays(preset, bindings.size());
        if (po.contains("inputUniverse"))
        {
            preset.entryInput = QSharedPointer<QLCInputSource>(
                new QLCInputSource((quint32) po["inputUniverse"].toInt(),
                                   (quint32) po["inputChannel"].toInt()));
        }
        if (po.contains("inputKey"))
            preset.entryKey = stripKeySequence(QKeySequence(po["inputKey"].toString()));
        presets.append(preset);
    }

    setLevelConfig(bindings, presets);

    QList<LevelPreset> widgetAppearance;
    for (const QJsonValue& pv : obj["widgetEntryAppearance"].toArray())
    {
        QJsonObject po = pv.toObject();
        LevelPreset preset;
        preset.label = po["label"].toString();
        preset.iconPath = po["iconPath"].toString();
        const QString colorStr = po["color"].toString();
        if (!colorStr.isEmpty())
        {
            const QColor c(colorStr);
            if (c.isValid())
                preset.color = c;
        }
        preset.hideName = po["hideName"].toBool(false);
        const QString labelColorStr = po["labelColor"].toString();
        if (!labelColorStr.isEmpty())
        {
            const QColor lc(labelColorStr);
            if (lc.isValid())
                preset.labelColor = lc;
        }
        if (po.contains("inputUniverse"))
        {
            preset.entryInput = QSharedPointer<QLCInputSource>(
                    new QLCInputSource(quint32(po["inputUniverse"].toInt()),
                                       quint32(po["inputChannel"].toInt())));
        }
        if (po.contains("inputKey"))
            preset.entryKey = stripKeySequence(QKeySequence(po["inputKey"].toString()));
        widgetAppearance.append(preset);
    }
    setWidgetEntryAppearance(widgetAppearance);

    if (obj.contains("functionEntryInputs"))
    {
        const QJsonArray arr = obj["functionEntryInputs"].toArray();
        for (int i = 0; i < arr.size(); ++i)
        {
            QJsonObject io = arr.at(i).toObject();
            while (m_functionEntryInputs.size() <= i)
                m_functionEntryInputs.append(QSharedPointer<QLCInputSource>());
            while (m_functionEntryKeys.size() <= i)
                m_functionEntryKeys.append(QKeySequence());
            if (io.contains("universe"))
            {
                m_functionEntryInputs[i] = QSharedPointer<QLCInputSource>(
                    new QLCInputSource((quint32) io["universe"].toInt(),
                                       (quint32) io["channel"].toInt()));
            }
            if (io.contains("key"))
                m_functionEntryKeys[i] = stripKeySequence(QKeySequence(io["key"].toString()));
        }
    }

    if (obj.contains("spread"))
    {
        const QJsonArray slotArr = obj["spread"].toObject()["slotInputs"].toArray();
        m_spreadSlotInputs.clear();
        m_spreadSlotKeys.clear();
        for (const QJsonValue& sv : slotArr)
        {
            QJsonObject io = sv.toObject();
            if (io.contains("universe"))
            {
                m_spreadSlotInputs.append(QSharedPointer<QLCInputSource>(
                    new QLCInputSource((quint32) io["universe"].toInt(),
                                       (quint32) io["channel"].toInt())));
            }
            else
                m_spreadSlotInputs.append(QSharedPointer<QLCInputSource>());
            if (io.contains("key"))
                m_spreadSlotKeys.append(stripKeySequence(QKeySequence(io["key"].toString())));
            else
                m_spreadSlotKeys.append(QKeySequence());
        }
    }

    resizeSpreadSlotInputs();
    syncEntryInputSources();
    recalcLayoutSize();
    update();
}

// ---- Load & Save ---------------------------------------------------------

static QString resolveIconPath(const QString& stored, Doc* doc)
{
    if (stored.isEmpty()) return QString();

    if (!stored.contains('/') && !stored.contains('\\'))
    {
        QString scribbleDir = QLCFile::userDirectory(
            QString(USERSCRIBBLEDIR), QString(USERSCRIBBLEDIR),
            QStringList()).absolutePath();
        return scribbleDir + "/" + stored;
    }
    return doc->denormalizeComponentPath(stored);
}

static QString normalizeIconPath(const QString& path, Doc* doc)
{
    if (path.isEmpty()) return QString();

    QString scribbleDir = QLCFile::userDirectory(
        QString(USERSCRIBBLEDIR), QString(USERSCRIBBLEDIR),
        QStringList()).absolutePath();

    if (!scribbleDir.isEmpty() && path.startsWith(scribbleDir))
        return QFileInfo(path).fileName();

    return doc->normalizeComponentPath(path);
}

void MultiButtonWidget::postLoad()
{
    syncDynamicEntryCountLayout();
    recalcLayoutSize();
    update();
}

bool MultiButtonWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot)
        return false;

    if (!loadXMLCommon(root))
    {
        qWarning() << Q_FUNC_INFO << "loadXMLCommon failed for Multi Button id" << id();
        return false;
    }

    QList<quint32>       ids;
    QStringList          labels;
    QStringList          icons;
    MultiButtonMode           widgetMode = MultiButtonMode::Function;
    quint32                   legacyFxId = UINT_MAX;
    QList<quint32>            legacyChannels;
    QList<LevelChannelBinding> levelBindings;
    QList<LevelPreset>        levelPresets;
    QList<LevelPreset>        widgetEntryAppearance;
    quint32                   widgetTargetId = VCWidget::invalidId();
    int                       widgetOutputIndex = 0;
    int                       widgetParameter = 0;
    QSharedPointer<QLCInputSource> widgetLiveInputSource;
    QList<MultiButtonAutomationProfile> loadedAutomation;
    bool hasAutomationElement = false;
    QMap<int, EntryInputBinding> functionInputsLoaded;
    QMap<int, EntryInputBinding> spreadSlotsLoaded;
    QList<bool>  functionFlashLoaded;
    QList<QColor> functionLabelColorsLoaded;

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0;
            bool visible = false;
            loadXMLWindowState(root, &x, &y, &w, &h, &visible);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
        {
            loadXMLAppearance(root);
        }
        else if (root.name() == KXMLWidgetMode)
        {
            widgetMode = stringToMode(root.readElementText());
        }
        else if (root.name() == KXMLCurrentIndex)
        {
            m_currentIndex = root.readElementText().toInt();
        }
        else if (root.name() == KXMLWidgetLink)
        {
            const auto attrs = root.attributes();
            widgetTargetId = attrs.value(KXMLWidgetLinkTarget).toUInt();
            widgetOutputIndex = attrs.value(KXMLWidgetLinkOutput).toInt();
            widgetParameter = attrs.value(KXMLWidgetLinkParam).toInt();
            if (attrs.hasAttribute(KXMLWidgetBusPolicy))
            {
                m_widgetBusPolicy = stringToWidgetBusPolicy(
                        attrs.value(KXMLWidgetBusPolicy).toString());
            }
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLWidgetLiveInput)
                    widgetLiveInputSource = readInputBlock(root, this).source;
                else
                    root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLLongPressMs)
        {
            setLongPressMs(root.readElementText().toInt());
        }
        else if (root.name() == KXMLAddOffAtEnd)
        {
            setAddOffAtEnd(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLSpread)
        {
            const auto attrs = root.attributes();
            const bool spreadEnabled = attrs.value(KXMLSpreadEnabled).toInt() != 0;
            setSpreadColumns(attrs.value(KXMLSpreadColumns).toInt());
            setSpreadRows(attrs.value(KXMLSpreadRows).toInt());
            setSpreadHMargin(attrs.value(KXMLSpreadHMargin).toInt());
            setSpreadVMargin(attrs.value(KXMLSpreadVMargin).toInt());
            setSpreadTileWidth(attrs.value(KXMLSpreadTileW).toInt());
            setSpreadTileHeight(attrs.value(KXMLSpreadTileH).toInt());
            setSpreadPages(attrs.value(KXMLSpreadPages).toInt());
            setWidgetLayout(spreadEnabled ? MultiButtonLayout::Spread
                                          : MultiButtonLayout::Single);
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLAutomation)
        {
            hasAutomationElement = true;
            const auto attrs = root.attributes();
            m_automationEnabled = attrs.value(KXMLAutomationEnabled).toInt() != 0;
            m_activeAutomationProfile = attrs.value(KXMLAutomationActive).toInt();

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLAutomationProfile)
                {
                    MultiButtonAutomationProfile profile;
                    const auto pa = root.attributes();
                    profile.name = pa.value(KXMLAutomationName).toString();
                    profile.mode = stringToAutomationMode(pa.value(KXMLAutomationMode).toString());
                    profile.stepMin = pa.value(KXMLAutomationStepMin).toInt();
                    if (profile.stepMin < 1)
                        profile.stepMin = 1;
                    profile.stepMax = pa.value(KXMLAutomationStepMax).toInt();
                    if (profile.stepMax < profile.stepMin)
                        profile.stepMax = profile.stepMin;
                    profile.multiplier = qMax(1, pa.value(KXMLAutomationMultiplier).toInt());
                    profile.beatOffset = pa.value(KXMLAutomationBeatOffset).toInt();
                    profile.excludeMask = pa.value(KXMLAutomationExcludeMask)
                                              .toString()
                                              .toUInt(nullptr, 16);
                    const int maxOff = qMax(0, profile.multiplier - 1);
                    profile.beatOffset = qBound(0, profile.beatOffset, maxOff);
                    loadedAutomation.append(profile);
                    root.skipCurrentElement();
                }
                else
                {
                    root.skipCurrentElement();
                }
            }
        }
        else if (root.name() == KXMLMonitorChannels)
        {
            setMonitorChannelValues(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLReceiveInputInactiveFramePage)
        {
            setReceiveInputOnInactiveFramePage(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLStageBeforeCommit)
        {
            setStageBeforeCommit(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLEntrySelectAutoCommit)
        {
            setEntrySelectAutoCommit(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLLogPresetChanges)
        {
            setLogPresetChanges(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLLevelFixture)
        {
            legacyFxId = root.attributes().value(KXMLLevelFixtureID).toUInt();
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLLevelBindings)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLLevelBinding)
                {
                    LevelChannelBinding b;
                    b.fixtureId = root.attributes().value(KXMLLevelBindingFx).toUInt();
                    b.channel   = root.attributes().value(KXMLLevelBindingCh).toUInt();
                    levelBindings.append(b);
                }
                root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLLevelChannels)
        {
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLLevelChannel)
                    legacyChannels.append(root.attributes().value(KXMLLevelChannelIndex).toUInt());
                root.skipCurrentElement();
            }
        }
        else if (root.name() == KXMLLevelPreset)
        {
            LevelPreset preset;
            auto attrs = root.attributes();
            preset.label = attrs.value(KXMLLevelPresetLabel).toString();
            preset.iconPath = resolveIconPath(attrs.value(KXMLLevelPresetIcon).toString(), m_doc);
            const QString colorStr = attrs.value(KXMLLevelPresetColor).toString();
            if (!colorStr.isEmpty())
            {
                const QColor c(colorStr);
                if (c.isValid())
                    preset.color = c;
            }
            preset.hideName = attrs.value(KXMLLevelPresetHideName).toInt() != 0;
            preset.flashOnActivate = attrs.value(KXMLLevelPresetFlash).toInt() != 0;
            const QString labelColorStr = attrs.value(KXMLLevelPresetLabelColor).toString();
            if (!labelColorStr.isEmpty())
            {
                const QColor lc(labelColorStr);
                if (lc.isValid())
                    preset.labelColor = lc;
            }

            QString valuesStr = attrs.value(KXMLLevelPresetValues).toString();
            for (const QString& part : valuesStr.split(' ', Qt::SkipEmptyParts))
                preset.values.append((quint8) part.toUInt());

            while (root.readNextStartElement())
            {
                if (root.name() == KXMLEntryTriggerInput)
                {
                    const EntryInputBinding binding = readInputBlock(root, this);
                    preset.entryInput = binding.source;
                    preset.entryKey   = binding.key;
                }
                else if (root.name() == KXMLLevelValueFormula)
                {
                    const int fi = root.attributes().value(KXMLLevelValueFormulaIndex).toInt();
                    QString text = root.attributes().value(KXMLLevelValueFormulaText).toString();
                    while (root.readNext() != QXmlStreamReader::EndElement)
                    {
                        if (root.tokenType() == QXmlStreamReader::Characters)
                        {
                            const QString body = root.text().toString().trimmed();
                            if (!body.isEmpty())
                                text = body;
                        }
                    }
                    while (preset.valueFormulas.size() <= fi)
                        preset.valueFormulas.append(QString());
                    preset.valueFormulas[fi] = text;
                }
                else
                    root.skipCurrentElement();
            }

            levelPresets.append(preset);
        }
        else if (root.name() == KXMLWidgetEntryAppearance)
        {
            LevelPreset preset;
            const auto attrs = root.attributes();
            preset.label = attrs.value(KXMLLevelPresetLabel).toString();
            preset.iconPath = resolveIconPath(attrs.value(KXMLLevelPresetIcon).toString(), m_doc);
            const QString colorStr = attrs.value(KXMLLevelPresetColor).toString();
            if (!colorStr.isEmpty())
            {
                const QColor c(colorStr);
                if (c.isValid())
                    preset.color = c;
            }
            preset.hideName = attrs.value(KXMLLevelPresetHideName).toInt() != 0;
            const QString labelColorStr = attrs.value(KXMLLevelPresetLabelColor).toString();
            if (!labelColorStr.isEmpty())
            {
                const QColor lc(labelColorStr);
                if (lc.isValid())
                    preset.labelColor = lc;
            }
            while (root.readNextStartElement())
            {
                if (root.name() == KXMLEntryTriggerInput)
                {
                    const EntryInputBinding binding = readInputBlock(root, this);
                    preset.entryInput = binding.source;
                    preset.entryKey = binding.key;
                }
                else
                    root.skipCurrentElement();
            }
            widgetEntryAppearance.append(preset);
        }
        else if (root.name() == KXMLFunctionEntryInput)
        {
            const int idx = root.attributes().value(KXMLFunctionEntryIndex).toInt();
            functionInputsLoaded[idx] = readInputBlock(root, this);
        }
        else if (root.name() == KXMLSpreadSlotInput)
        {
            const int idx = root.attributes().value(KXMLSpreadSlotIndex).toInt();
            spreadSlotsLoaded[idx] = readInputBlock(root, this);
        }
        else if (root.name() == KXMLFunction)
        {
            auto attrs = root.attributes();
            ids.append(attrs.value(KXMLFunctionID).toUInt());
            labels.append(attrs.value(KXMLFunctionLabel).toString());
            icons.append(resolveIconPath(attrs.value(KXMLFunctionIconPath).toString(), m_doc));
            functionFlashLoaded.append(attrs.value(KXMLFunctionFlash).toInt() != 0);
            const QString fnLabelColor = attrs.value(KXMLFunctionLabelColor).toString();
            if (!fnLabelColor.isEmpty())
            {
                const QColor lc(fnLabelColor);
                functionLabelColorsLoaded.append(lc.isValid() ? lc : QColor());
            }
            else
                functionLabelColorsLoaded.append(QColor());
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLTriggerInput)
        {
            loadXMLSources(root, triggerInputSourceId);
        }
        else if (root.name() == KXMLPopupInput)
        {
            loadXMLSources(root, popupInputSourceId);
        }
        else if (root.name() == KXMLAutomationTriggerInput)
        {
            loadXMLSources(root, automationInputSourceId);
        }
        else if (root.name() == KXMLPresetChooseInput)
        {
            loadXMLSources(root, presetChooseInputSourceId);
        }
        else if (root.name() == KXMLEntrySelectInput)
        {
            loadXMLSources(root, entrySelectInputSourceId);
        }
        else if (root.name() == KXMLSpreadPageInput)
        {
            loadXMLSources(root, spreadPageInputSourceId);
        }
        else if (root.name() == KXMLCommitInput)
        {
            loadXMLSources(root, commitInputSourceId);
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    m_mode = widgetMode;
    m_widgetTargetId = widgetTargetId;
    m_widgetOutputIndex = qMax(0, widgetOutputIndex);
    m_widgetParameter = qMax(0, widgetParameter);
    m_widgetLiveInputSource = cloneInputSource(widgetLiveInputSource);
    syncWidgetLiveInputSourceToTarget();
    releaseWidgetLiveFaders();
    m_lastResolvedEntryCount = -1;
    m_functionIds    = ids;
    m_functionLabels = labels;
    m_iconPaths      = icons;
    m_levelChannelBindings = levelBindings;
    if (m_levelChannelBindings.isEmpty() && legacyFxId != UINT_MAX)
    {
        for (quint32 ch : legacyChannels)
        {
            LevelChannelBinding b;
            b.fixtureId = legacyFxId;
            b.channel   = ch;
            m_levelChannelBindings.append(b);
        }
    }
    m_levelPresets = levelPresets;
    m_widgetEntryAppearance = widgetEntryAppearance;
    for (LevelPreset& preset : m_widgetEntryAppearance)
        preset.entryInput = cloneInputSource(preset.entryInput);
    m_functionEntryFlash = functionFlashLoaded;
    m_functionEntryLabelColors = functionLabelColorsLoaded;
    while (m_functionEntryFlash.size() < m_functionIds.size())
        m_functionEntryFlash.append(false);
    while (m_functionEntryFlash.size() > m_functionIds.size())
        m_functionEntryFlash.removeLast();
    while (m_functionEntryLabelColors.size() < m_functionIds.size())
        m_functionEntryLabelColors.append(QColor());
    while (m_functionEntryLabelColors.size() > m_functionIds.size())
        m_functionEntryLabelColors.removeLast();

    while (m_functionLabels.size() < m_functionIds.size())
        m_functionLabels.append(QString());
    while (m_iconPaths.size() < m_functionIds.size())
        m_iconPaths.append(QString());

    m_functionEntryInputs.clear();
    m_functionEntryKeys.clear();
    for (int i = 0; i < m_functionIds.size(); ++i)
    {
        const EntryInputBinding binding = functionInputsLoaded.value(i);
        m_functionEntryInputs.append(cloneInputSource(binding.source));
        m_functionEntryKeys.append(binding.key);
    }

    m_spreadSlotInputs.clear();
    m_spreadSlotKeys.clear();
    int maxSpreadSlot = -1;
    for (auto it = spreadSlotsLoaded.constBegin(); it != spreadSlotsLoaded.constEnd(); ++it)
        maxSpreadSlot = qMax(maxSpreadSlot, it.key());
    for (int s = 0; s <= maxSpreadSlot && s < MBInputId::kMaxSpreadSlots; ++s)
    {
        const EntryInputBinding binding = spreadSlotsLoaded.value(s);
        m_spreadSlotInputs.append(cloneInputSource(binding.source));
        m_spreadSlotKeys.append(binding.key);
    }

    for (LevelPreset& preset : m_levelPresets)
    {
        while (preset.values.size() < m_levelChannelBindings.size())
            preset.values.append(0);
        while (preset.values.size() > m_levelChannelBindings.size())
            preset.values.removeLast();
    }

    if (m_currentIndex >= entryCount())
        m_currentIndex = -1;

    if (hasAutomationElement)
        setAutomationProfiles(loadedAutomation, m_activeAutomationProfile);

    m_iconCache.clear();
    rebuildSceneCache();
    clampSpreadPageIndex();
    resizeSpreadSlotInputs();
    syncEntryInputSources();
    syncAllInputSourcePages();
    syncAutomationSuspendDefault();
    m_triggerLastValue    = 0;
    m_automationLastValue = 0;
    recalcLayoutSize();

    if (root.hasError())
    {
        qWarning() << Q_FUNC_INFO << "XML reader warning after load for Multi Button id"
                   << id() << root.errorString();
    }

    return true;
}

bool MultiButtonWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    if (m_mode != MultiButtonMode::Function)
        doc->writeTextElement(KXMLWidgetMode, modeToString(m_mode));

    if (m_mode == MultiButtonMode::Widget)
    {
        doc->writeStartElement(KXMLWidgetLink);
        doc->writeAttribute(KXMLWidgetLinkTarget, QString::number(m_widgetTargetId));
        doc->writeAttribute(KXMLWidgetLinkOutput, QString::number(m_widgetOutputIndex));
        doc->writeAttribute(KXMLWidgetLinkParam, QString::number(m_widgetParameter));
        doc->writeAttribute(KXMLWidgetBusPolicy, widgetBusPolicyToString(m_widgetBusPolicy));
        if (!m_widgetLiveInputSource.isNull() && m_widgetLiveInputSource->isValid())
        {
            doc->writeStartElement(KXMLWidgetLiveInput);
            saveInputBlock(doc, m_widgetLiveInputSource, QKeySequence());
            doc->writeEndElement();
        }
        doc->writeEndElement();
    }

    doc->writeTextElement(KXMLCurrentIndex, QString::number(m_currentIndex));
    doc->writeTextElement(KXMLLongPressMs,  QString::number(m_longPressMs));
    doc->writeTextElement(KXMLAddOffAtEnd,  QString::number(m_addOffAtEnd ? 1 : 0));
    if (m_monitorChannelValues)
        doc->writeTextElement(KXMLMonitorChannels, QString::number(1));
    if (m_receiveInputOnInactiveFramePage)
        doc->writeTextElement(KXMLReceiveInputInactiveFramePage, QString::number(1));
    if (m_stageBeforeCommit)
        doc->writeTextElement(KXMLStageBeforeCommit, QString::number(1));
    if (!m_entrySelectAutoCommit)
        doc->writeTextElement(KXMLEntrySelectAutoCommit, QString::number(0));
    if (m_logPresetChanges)
        doc->writeTextElement(KXMLLogPresetChanges, QString::number(1));

    doc->writeStartElement(KXMLSpread);
    doc->writeAttribute(KXMLSpreadEnabled,
                        m_layout == MultiButtonLayout::Spread ? QStringLiteral("1")
                                                              : QStringLiteral("0"));
    doc->writeAttribute(KXMLSpreadColumns, QString::number(m_spreadColumns));
    doc->writeAttribute(KXMLSpreadRows, QString::number(m_spreadRows));
    doc->writeAttribute(KXMLSpreadHMargin, QString::number(m_spreadHMargin));
    doc->writeAttribute(KXMLSpreadVMargin, QString::number(m_spreadVMargin));
    doc->writeAttribute(KXMLSpreadTileW, QString::number(m_spreadTileWidth));
    doc->writeAttribute(KXMLSpreadTileH, QString::number(m_spreadTileHeight));
    doc->writeAttribute(KXMLSpreadPages, QString::number(m_spreadPages));
    doc->writeEndElement();

    doc->writeStartElement(KXMLAutomation);
    doc->writeAttribute(KXMLAutomationEnabled, m_automationEnabled ? QStringLiteral("1")
                                                                 : QStringLiteral("0"));
    doc->writeAttribute(KXMLAutomationActive, QString::number(m_activeAutomationProfile));
    for (const MultiButtonAutomationProfile& profile : m_automationProfiles)
    {
        doc->writeStartElement(KXMLAutomationProfile);
        doc->writeAttribute(KXMLAutomationName, profile.name);
        doc->writeAttribute(KXMLAutomationMode, automationModeToString(profile.mode));
        doc->writeAttribute(KXMLAutomationStepMin, QString::number(profile.stepMin));
        doc->writeAttribute(KXMLAutomationStepMax, QString::number(profile.stepMax));
        doc->writeAttribute(KXMLAutomationMultiplier, QString::number(profile.multiplier));
        doc->writeAttribute(KXMLAutomationBeatOffset, QString::number(profile.beatOffset));
        doc->writeAttribute(KXMLAutomationExcludeMask,
                            QString::number(profile.excludeMask, 16));
        doc->writeEndElement();
    }
    doc->writeEndElement();

    if (m_mode == MultiButtonMode::Level)
    {
        if (!m_levelChannelBindings.isEmpty())
        {
            doc->writeStartElement(KXMLLevelBindings);
            for (const LevelChannelBinding& b : m_levelChannelBindings)
            {
                doc->writeStartElement(KXMLLevelBinding);
                doc->writeAttribute(KXMLLevelBindingFx, QString::number(b.fixtureId));
                doc->writeAttribute(KXMLLevelBindingCh, QString::number(b.channel));
                doc->writeEndElement();
            }
            doc->writeEndElement();
        }

        for (const LevelPreset& preset : m_levelPresets)
        {
            doc->writeStartElement(KXMLLevelPreset);
            doc->writeAttribute(KXMLLevelPresetLabel, preset.label);
            doc->writeAttribute(KXMLLevelPresetIcon,
                                normalizeIconPath(preset.iconPath, m_doc));
            if (preset.color.isValid())
                doc->writeAttribute(KXMLLevelPresetColor, preset.color.name(QColor::HexRgb));
            if (preset.hideName)
                doc->writeAttribute(KXMLLevelPresetHideName, QStringLiteral("1"));
            if (preset.flashOnActivate)
                doc->writeAttribute(KXMLLevelPresetFlash, QStringLiteral("1"));
            if (preset.labelColor.isValid())
                doc->writeAttribute(KXMLLevelPresetLabelColor,
                                    preset.labelColor.name(QColor::HexRgb));

            QStringList parts;
            for (quint8 v : preset.values)
                parts.append(QString::number(v));
            doc->writeAttribute(KXMLLevelPresetValues, parts.join(' '));
            if ((!preset.entryInput.isNull() && preset.entryInput->isValid())
                || !preset.entryKey.isEmpty())
            {
                doc->writeStartElement(KXMLEntryTriggerInput);
                saveInputBlock(doc, preset.entryInput, preset.entryKey);
                doc->writeEndElement();
            }
            for (int fi = 0; fi < preset.valueFormulas.size(); ++fi)
            {
                const QString formula = preset.valueFormulas.at(fi).trimmed();
                if (formula.isEmpty())
                    continue;
                doc->writeStartElement(KXMLLevelValueFormula);
                doc->writeAttribute(KXMLLevelValueFormulaIndex, QString::number(fi));
                doc->writeCharacters(formula);
                doc->writeEndElement();
            }
            doc->writeEndElement();
        }
    }
    else if (m_mode == MultiButtonMode::Function)
    {
        for (int i = 0; i < m_functionIds.size(); ++i)
        {
            doc->writeStartElement(KXMLFunction);
            doc->writeAttribute(KXMLFunctionID,       QString::number(m_functionIds.at(i)));
            doc->writeAttribute(KXMLFunctionLabel,    m_functionLabels.value(i));
            doc->writeAttribute(KXMLFunctionIconPath, normalizeIconPath(m_iconPaths.value(i), m_doc));
            if (i < m_functionEntryFlash.size() && m_functionEntryFlash.at(i))
                doc->writeAttribute(KXMLFunctionFlash, QStringLiteral("1"));
            if (i < m_functionEntryLabelColors.size()
                && m_functionEntryLabelColors.at(i).isValid())
            {
                doc->writeAttribute(KXMLFunctionLabelColor,
                                    m_functionEntryLabelColors.at(i).name(QColor::HexRgb));
            }
            doc->writeEndElement();
        }
    }

    for (const LevelPreset& preset : m_widgetEntryAppearance)
    {
        doc->writeStartElement(KXMLWidgetEntryAppearance);
        doc->writeAttribute(KXMLLevelPresetLabel, preset.label);
        doc->writeAttribute(KXMLLevelPresetIcon,
                            normalizeIconPath(preset.iconPath, m_doc));
        if (preset.color.isValid())
            doc->writeAttribute(KXMLLevelPresetColor, preset.color.name(QColor::HexRgb));
        if (preset.hideName)
            doc->writeAttribute(KXMLLevelPresetHideName, QStringLiteral("1"));
        if (preset.labelColor.isValid())
            doc->writeAttribute(KXMLLevelPresetLabelColor,
                                preset.labelColor.name(QColor::HexRgb));
        if ((!preset.entryInput.isNull() && preset.entryInput->isValid())
            || !preset.entryKey.isEmpty())
        {
            doc->writeStartElement(KXMLEntryTriggerInput);
            saveInputBlock(doc, preset.entryInput, preset.entryKey);
            doc->writeEndElement();
        }
        doc->writeEndElement();
    }

    for (int i = 0; i < m_functionEntryInputs.size(); ++i)
    {
        const QSharedPointer<QLCInputSource>& src = m_functionEntryInputs.at(i);
        const QKeySequence key = (i < m_functionEntryKeys.size())
                                     ? m_functionEntryKeys.at(i)
                                     : QKeySequence();
        if ((src.isNull() || !src->isValid()) && key.isEmpty())
            continue;
        doc->writeStartElement(KXMLFunctionEntryInput);
        doc->writeAttribute(KXMLFunctionEntryIndex, QString::number(i));
        saveInputBlock(doc, src, key);
        doc->writeEndElement();
    }

    for (int s = 0; s < m_spreadSlotInputs.size(); ++s)
    {
        const QSharedPointer<QLCInputSource>& src = m_spreadSlotInputs.at(s);
        const QKeySequence key = (s < m_spreadSlotKeys.size()) ? m_spreadSlotKeys.at(s) : QKeySequence();
        if ((src.isNull() || !src->isValid()) && key.isEmpty())
            continue;
        doc->writeStartElement(KXMLSpreadSlotInput);
        doc->writeAttribute(KXMLSpreadSlotIndex, QString::number(s));
        saveInputBlock(doc, src, key);
        doc->writeEndElement();
    }

    auto trigSrc = inputSource(triggerInputSourceId);
    if (!trigSrc.isNull() && trigSrc->isValid())
    {
        doc->writeStartElement(KXMLTriggerInput);
        saveXMLInput(doc, trigSrc);
        doc->writeEndElement();
    }

    auto popSrc = inputSource(popupInputSourceId);
    if (!popSrc.isNull() && popSrc->isValid())
    {
        doc->writeStartElement(KXMLPopupInput);
        saveXMLInput(doc, popSrc);
        doc->writeEndElement();
    }

    auto autoSrc = inputSource(automationInputSourceId);
    if (!autoSrc.isNull() && autoSrc->isValid())
    {
        doc->writeStartElement(KXMLAutomationTriggerInput);
        saveXMLInput(doc, autoSrc);
        doc->writeEndElement();
    }

    auto presetSrc = inputSource(presetChooseInputSourceId);
    if (!presetSrc.isNull() && presetSrc->isValid())
    {
        doc->writeStartElement(KXMLPresetChooseInput);
        saveXMLInput(doc, presetSrc);
        doc->writeEndElement();
    }

    auto entrySelSrc = inputSource(entrySelectInputSourceId);
    if (!entrySelSrc.isNull() && entrySelSrc->isValid())
    {
        doc->writeStartElement(KXMLEntrySelectInput);
        saveXMLInput(doc, entrySelSrc);
        doc->writeEndElement();
    }

    auto spreadPageSrc = inputSource(spreadPageInputSourceId);
    if (!spreadPageSrc.isNull() && spreadPageSrc->isValid())
    {
        doc->writeStartElement(KXMLSpreadPageInput);
        saveXMLInput(doc, spreadPageSrc);
        doc->writeEndElement();
    }

    auto commitSrc = inputSource(commitInputSourceId);
    if (!commitSrc.isNull() && commitSrc->isValid())
    {
        doc->writeStartElement(KXMLCommitInput);
        saveXMLInput(doc, commitSrc);
        doc->writeEndElement();
    }

    doc->writeEndElement();
    return true;
}

// ---- Paint ---------------------------------------------------------------

QString MultiButtonWidget::activeFunctionCaption() const
{
    const int displayIdx = displayedEntryIndex();

    if (displayIdx < 0)
        return tr("—");

    if (m_mode == MultiButtonMode::Level)
        return levelPresetDisplayName(displayIdx);

    const QString lbl = entryLabel(displayIdx);
    if (!lbl.isEmpty())
        return lbl;

    if (m_mode == MultiButtonMode::Widget)
        return tr("Preset %1").arg(displayIdx + 1);

    Function* f = functionAt(displayIdx);
    return f ? f->name() : tr("?");
}

QColor MultiButtonWidget::defaultTileBackground() const
{
    if (hasCustomBackgroundColor())
        return backgroundColor();
    return palette().button().color();
}

QColor MultiButtonWidget::buttonTextColor(const QColor& tileBg) const
{
    if (hasCustomForegroundColor())
        return foregroundColor();
    return contrastTextOn(tileBg);
}

void MultiButtonWidget::drawTile(QPainter& p, const QRect& tileRect, int tileIndex,
                                 bool isLive, bool isStaged, bool isPressed) const
{
    p.save();
    p.translate(tileRect.topLeft());
    const QRect r(0, 0, tileRect.width(), tileRect.height());

    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bg;
    paintTileBackground(p, r, tileIndex, isLive, isStaged, isPressed, bg);

    QPixmap iconPx;
    if (tileIndex >= 0)
        iconPx = iconForEntry(tileIndex);

    const bool hasCustomLabel = (tileIndex >= 0) && !entryLabel(tileIndex).isEmpty();
    const LevelPreset* appearance = entryAppearancePreset(tileIndex);
    const bool levelNoText = (tileIndex >= 0
                              && appearance
                              && appearance->hideName);
    const bool showLabelText = tileIndex < 0
                               || (!levelNoText && (iconPx.isNull() || hasCustomLabel
                                                    || m_mode == MultiButtonMode::Level));

    QColor fg = buttonTextColor(bg);
    if (tileIndex >= 0)
    {
        if (appearance && appearance->labelColor.isValid())
        {
            fg = appearance->labelColor;
        }
        else if (m_mode == MultiButtonMode::Function
                 && tileIndex < m_functionEntryLabelColors.size()
                 && m_functionEntryLabelColors.at(tileIndex).isValid())
        {
            fg = m_functionEntryLabelColors.at(tileIndex);
        }
    }

    const int pad = 3;
    const QRect fillRect = r.adjusted(1, 1, -2, -2);
    QRect inner = fillRect.adjusted(pad, pad, -pad, -pad);

    int iconAreaH = 0;
    if (!iconPx.isNull())
    {
        int dim = qMin(inner.width(), qMax(inner.height(), 1)) * 7 / 10;
        if (hasCustomLabel && tileIndex >= 0)
            dim = dim * 4 / 10;
        dim = qMax(12, dim);
        QPixmap scaled = iconPx.scaled(dim, dim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int ix = inner.x() + (inner.width() - scaled.width()) / 2;
        const int iy = (tileIndex < 0 || !hasCustomLabel)
                       ? inner.y() + qMax(0, (inner.height() - scaled.height()) / 2)
                       : inner.y() + 1;
        p.drawPixmap(ix, iy, scaled);
        iconAreaH = (tileIndex >= 0 && hasCustomLabel) ? scaled.height() + 2 : inner.height();
    }

    const QString cap = tileCaption(tileIndex);
    if (showLabelText && !cap.isEmpty())
    {
        QFont mainFont = font();
        if ((isLive || isStaged) && tileIndex >= 0)
            mainFont.setBold(true);
        p.setFont(mainFont);
        p.setPen(fg);
        QRect textRect = inner.adjusted(0, iconAreaH, 0, 0);
        if (textRect.height() > 0)
            p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, cap);
    }

    if (tileIndex >= 0 && entryIsFlash(tileIndex))
        drawFlashEmblem(p, r);

    p.restore();
}

void MultiButtonWidget::paintSpread(QPainter& p)
{
    if (m_mode == MultiButtonMode::Widget)
        syncWidgetLinkLiveStagedState();

    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(QPen(palette().mid().color(), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    QColor fg = hasCustomForegroundColor()
                ? foregroundColor()
                : palette().buttonText().color();

    const bool hasTitle = !caption().isEmpty();
    if (hasTitle)
    {
        QFont titleFont = font();
        titleFont.setPointSize(qMax(6, font().pointSize() - 1));
        titleFont.setItalic(true);
        p.setFont(titleFont);
        p.setPen(fg.darker(140));
        p.drawText(QRect(4, 3, width() - 8, 14), Qt::AlignCenter, caption());
    }

    const int displayIdx = displayedEntryIndex();
    const int monitorIdx = monitorHighlightIndex();
    const int stagedIdx  = stagedHighlightIndex();
    const bool hasStagedHighlight = stagedHighlightValid();
    const bool hasLiveHighlight = m_mode == MultiButtonMode::Widget
            ? (widgetLinkTarget() != nullptr)
            : (m_monitorChannelValues || m_currentIndex >= 0);

    for (const SpreadTileInfo& tile : computeSpreadTiles())
    {
        const bool tileIsOff = tile.index < 0;
        const bool isLiveTile = hasLiveHighlight
                && (tileIsOff ? (monitorIdx < 0) : (tile.index == monitorIdx));
        const bool isStagedTile = hasStagedHighlight
                && (tileIsOff ? (stagedIdx < 0) : (tile.index == stagedIdx));
        const bool isPreviewTile = m_entrySelectPreviewActive
                && (tileIsOff ? (displayIdx < 0) : (tile.index == displayIdx));
        const bool isPressed = (m_pressActive && tile.index == m_pressTileIndex)
                || isPreviewTile;
        drawTile(p, tile.rect, tile.index, isLiveTile, isStagedTile, isPressed);
    }
}

void MultiButtonWidget::paintSingle(QPainter& p)
{
    if (m_mode == MultiButtonMode::Widget)
        syncWidgetLinkLiveStagedState();

    const int displayIdx = displayedEntryIndex();
    const int monitorIdx = monitorHighlightIndex();
    const bool isPressed = m_pressActive;

    QColor bg = defaultTileBackground();
    const LevelPreset* displayAppearance = entryAppearancePreset(displayIdx);
    if (displayIdx >= 0 && displayAppearance && displayAppearance->color.isValid())
    {
        bg = displayAppearance->color;
    }

    if (m_pressActive)
        bg = bg.darker(120);
    if (displayIdx >= 0)
        bg = bg.lighter(115);

    p.fillRect(rect(), bg);

    const int dotsReserve = (entryCount() > 0) ? 14 : 0;
    QColor fg = buttonTextColor(bg);
    if (displayIdx >= 0)
    {
        if (displayAppearance && displayAppearance->labelColor.isValid())
        {
            fg = displayAppearance->labelColor;
        }
        else if (m_mode == MultiButtonMode::Function
                 && displayIdx < m_functionEntryLabelColors.size()
                 && m_functionEntryLabelColors.at(displayIdx).isValid())
        {
            fg = m_functionEntryLabelColors.at(displayIdx);
        }
    }

    const bool hasTitle = !caption().isEmpty();
    const int  titleH   = hasTitle ? 16 : 0;
    const int  topPad   = hasTitle ? 3  : 4;

    if (hasTitle)
    {
        QFont titleFont = font();
        titleFont.setPointSize(qMax(6, font().pointSize() - 1));
        titleFont.setItalic(true);
        p.setFont(titleFont);
        p.setPen(fg.darker(140));
        p.drawText(QRect(4, topPad, width() - 8, titleH - 2), Qt::AlignCenter, caption());
    }

    const int iconTopPad = topPad + titleH;
    QPixmap icon;
    if (displayIdx >= 0)
        icon = iconForEntry(displayIdx);

    const bool hasCustomLabel = (displayIdx >= 0)
                                && !entryLabel(displayIdx).isEmpty();
    const bool levelNoText = displayIdx >= 0
                              && displayAppearance
                              && displayAppearance->hideName;
    const bool showLabelText = !levelNoText && (icon.isNull() || hasCustomLabel
                                                || m_mode == MultiButtonMode::Level);

    int iconAreaH = 0;
    if (!icon.isNull())
    {
        int availH = height() - iconTopPad - dotsReserve - 4;
        int availW = width() - 8;
        int dim;
        if (!hasCustomLabel)
            dim = qMin(availW, qMax(availH, 1));
        else
            dim = qMin(availW, qMax(availH, 1)) * 4 / 10;
        dim = qMax(16, dim);
        QPixmap scaled = icon.scaled(dim, dim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int ix = (width() - scaled.width()) / 2;
        int iy = !hasCustomLabel
                 ? iconTopPad + qMax(0, (availH - scaled.height()) / 2)
                 : iconTopPad + 2;
        p.drawPixmap(ix, iy, scaled);
        iconAreaH = hasCustomLabel ? scaled.height() + 4 : availH;
    }

    QString funcText = activeFunctionCaption();
    if (showLabelText)
    {
        QFont mainFont = font();
        if (displayIdx >= 0 && (m_entrySelectPreviewActive || !m_visualOnly
                                || widgetLinkUsesInternalStaging()))
            mainFont.setBold(true);
        p.setFont(mainFont);
        p.setPen(fg);
        QRect mainRect = rect().adjusted(4, iconTopPad + iconAreaH, -4, -(dotsReserve + 2));
        if (mainRect.height() > 0)
            p.drawText(mainRect, Qt::AlignCenter | Qt::TextWordWrap, funcText);
    }

    int n         = entryCount();
    int totalDots = n + (m_addOffAtEnd ? 1 : 0);
    if (totalDots > 0)
    {
        if (totalDots <= 12)
        {
            const int dotSize = 5;
            const int gap     = 4;
            const int totalW  = totalDots * dotSize + (totalDots - 1) * gap;
            int x0 = (width() - totalW) / 2;
            int y0 = height() - 10;

            for (int i = 0; i < totalDots; ++i)
            {
                bool isOffDot = (m_addOffAtEnd && i == n);
                bool active   = isOffDot ? (displayIdx < 0)
                                         : (i == displayIdx);
                const bool live = (m_mode == MultiButtonMode::Widget)
                        && (isOffDot ? (monitorIdx < 0)
                                     : (i == monitorIdx));
                const QColor dotColor = live ? QColor(255, 150, 0) : fg;

                if (isOffDot)
                {
                    p.setBrush((active || live) ? dotColor : fg.darker(200));
                    p.setPen(live ? QPen(dotColor.lighter(130), 1)
                                  : (active ? QPen(fg, 1) : Qt::NoPen));
                    p.drawRect(x0 + i * (dotSize + gap), y0, dotSize, dotSize);
                }
                else
                {
                    p.setBrush((active || live) ? dotColor : fg.darker(180));
                    p.setPen(live ? QPen(dotColor.lighter(130), 1) : Qt::NoPen);
                    p.drawEllipse(x0 + i * (dotSize + gap), y0, dotSize, dotSize);
                }
            }
        }
        else
        {
            QFont small = font();
            small.setPointSize(qMax(6, small.pointSize() - 2));
            p.setFont(small);
            p.setPen(fg.darker(130));
            QString idxStr = displayIdx < 0
                ? tr("OFF/%1").arg(n)
                : QString("%1/%2").arg(displayIdx + 1).arg(n);
            p.drawText(QRect(0, height() - 14, width(), 12),
                       Qt::AlignCenter, idxStr);
            if (m_mode == MultiButtonMode::Widget)
            {
                p.setPen(QPen(QColor(255, 150, 0), 2));
                p.setBrush(QColor(255, 150, 0));
                const int markerX = qMax(4, width() / 2 - 34);
                p.drawEllipse(QPoint(markerX, height() - 8), 3, 3);
                QFont liveFont = small;
                liveFont.setPointSize(qMax(6, small.pointSize() - 1));
                p.setFont(liveFont);
                p.drawText(QRect(markerX + 7, height() - 14, 42, 12),
                           Qt::AlignLeft | Qt::AlignVCenter,
                           monitorIdx < 0 ? tr("live OFF")
                                          : tr("live %1").arg(monitorIdx + 1));
            }
        }
    }

    if (displayIdx >= 0 && entryIsFlash(displayIdx))
        drawFlashEmblem(p, rect());
}

void MultiButtonWidget::paintEvent(QPaintEvent* e)
{
    syncDynamicEntryCountLayout();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (m_layout == MultiButtonLayout::Spread)
        paintSpread(p);
    else
        paintSingle(p);

    p.end();

    VCWidget::paintEvent(e);
}

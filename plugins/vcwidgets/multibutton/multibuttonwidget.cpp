/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonwidget.cpp — Apache 2.0 / public domain
*/

#include "multibuttonwidget.h"
#include "multibuttonconfigdialog.h"

#include "functionparent.h"
#include "mastertimer.h"
#include "function.h"
#include "doc.h"
#include "qlcinputsource.h"
#include "scene.h"
#include "universe.h"
#include "qlcfile.h"
#include "qlcconfig.h"
#include "fixture.h"
#include "genericfader.h"
#include "fadechannel.h"
#include "qlcchannel.h"
#include "scribbledialog.h"

#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

// ---- XML tag constants ----------------------------------------------------

static const QString KXMLRoot              = QStringLiteral("PluginWidget");
static const QString KXMLPluginId          = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal       = QStringLiteral("org.qlcplus.vcwidgets.multibutton");
static const QString KXMLWidgetMode        = QStringLiteral("WidgetMode");
static const QString KXMLCurrentIndex      = QStringLiteral("CurrentIndex");
static const QString KXMLLongPressMs       = QStringLiteral("LongPressMs");
static const QString KXMLFunction          = QStringLiteral("Function");
static const QString KXMLFunctionID        = QStringLiteral("ID");
static const QString KXMLFunctionLabel     = QStringLiteral("Label");
static const QString KXMLFunctionIconPath  = QStringLiteral("IconPath");
static const QString KXMLAddOffAtEnd       = QStringLiteral("AddOffAtEnd");
static const QString KXMLMonitorChannels   = QStringLiteral("MonitorChannelValues");
static const QString KXMLTriggerInput      = QStringLiteral("TriggerInput");
static const QString KXMLPopupInput        = QStringLiteral("PopupInput");
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
static const QString KXMLLevelPresetHideName = QStringLiteral("HideName");
static const QString KXMLLevelPresetValues = QStringLiteral("Values");
static const QString KXMLSpread            = QStringLiteral("Spread");
static const QString KXMLSpreadEnabled     = QStringLiteral("Enabled");
static const QString KXMLSpreadColumns     = QStringLiteral("Columns");
static const QString KXMLSpreadRows        = QStringLiteral("Rows");
static const QString KXMLSpreadHMargin       = QStringLiteral("HMargin");
static const QString KXMLSpreadVMargin       = QStringLiteral("VMargin");
static const QString KXMLSpreadTileW         = QStringLiteral("TileW");
static const QString KXMLSpreadTileH         = QStringLiteral("TileH");

static QString modeToString(MultiButtonMode mode)
{
    return mode == MultiButtonMode::Level ? QStringLiteral("Level")
                                          : QStringLiteral("Function");
}

static MultiButtonMode stringToMode(const QString& s)
{
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

// ---- Construction ---------------------------------------------------------

MultiButtonWidget::MultiButtonWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(MultiButtonWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(QString());
    resize(QSize(120, 80));

    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);
    connect(m_longPressTimer, &QTimer::timeout,
            this, &MultiButtonWidget::slotLongPressFired);
}

MultiButtonWidget::~MultiButtonWidget()
{
    m_doc->masterTimer()->unregisterDMXSource(this);
    releaseLevelFaders();

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

    if (m_mode == MultiButtonMode::Level)
        releaseLevelFaders();
    stopCurrent();
    m_mode = mode;
    m_iconCache.clear();
    m_currentIndex = -1;
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
    {
        while (preset.values.size() < bindings.size())
            preset.values.append(0);
        while (preset.values.size() > bindings.size())
            preset.values.removeLast();
    }

    m_iconCache.clear();
    m_visualOnly = false;
    if (savedIndex >= 0 && savedIndex < m_levelPresets.size())
        m_currentIndex = savedIndex;
    else
        m_currentIndex = -1;

    reactivateLevelPreset();
    recalcSpreadSize();
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

    rebuildSceneCache();
    recalcSpreadSize();
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
    recalcSpreadSize();
    update();
}

void MultiButtonWidget::setWidgetLayout(MultiButtonLayout layout)
{
    if (m_layout == layout)
        return;
    m_layout = layout;
    recalcSpreadSize();
    update();
}

void MultiButtonWidget::setSpreadColumns(int columns)
{
    m_spreadColumns = qMax(0, columns);
    recalcSpreadSize();
}

void MultiButtonWidget::setSpreadRows(int rows)
{
    m_spreadRows = qMax(0, rows);
    recalcSpreadSize();
}

void MultiButtonWidget::setSpreadHMargin(int margin)
{
    m_spreadHMargin = qBound(0, margin, 64);
    recalcSpreadSize();
}

void MultiButtonWidget::setSpreadVMargin(int margin)
{
    m_spreadVMargin = qBound(0, margin, 64);
    recalcSpreadSize();
}

void MultiButtonWidget::setSpreadTileWidth(int width)
{
    m_spreadTileWidth = qBound(20, width, 400);
    recalcSpreadSize();
}

void MultiButtonWidget::setSpreadTileHeight(int height)
{
    m_spreadTileHeight = qBound(20, height, 400);
    recalcSpreadSize();
}

int MultiButtonWidget::spreadTileCount() const
{
    const int n = entryCount();
    if (n <= 0)
        return 0;
    return n + (m_addOffAtEnd ? 1 : 0);
}

void MultiButtonWidget::resolveSpreadGrid(int& cols, int& rows) const
{
    const int total = spreadTileCount();
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

void MultiButtonWidget::recalcSpreadSize()
{
    if (m_layout != MultiButtonLayout::Spread)
        return;
    resize(spreadTotalSize());
}

QVector<SpreadTileInfo> MultiButtonWidget::computeSpreadTiles() const
{
    QVector<SpreadTileInfo> tiles;
    const int total = spreadTileCount();
    if (total <= 0)
        return tiles;

    int cols = 0, rows = 0;
    resolveSpreadGrid(cols, rows);

    const int maxSlots = cols * rows;
    const int titleH = caption().isEmpty() ? 0 : 19;
    const int y0 = titleH;

    for (int slot = 0; slot < qMin(total, maxSlots); ++slot)
    {
        const int col = slot % cols;
        const int row = slot / cols;

        SpreadTileInfo info;
        const bool isOff = m_addOffAtEnd && (slot == total - 1);
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
        if (idx < m_levelPresets.size())
            m_levelPresets[idx].iconPath = path;
    }

    m_iconCache.remove(idx);
    update();
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
            m_channelMonitorTimer->setInterval(200);
            connect(m_channelMonitorTimer, &QTimer::timeout,
                    this, &MultiButtonWidget::slotCheckChannelValues);
        }
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
}

void MultiButtonWidget::writeDMX(MasterTimer* /*timer*/, QList<Universe*> universes)
{
    QMutexLocker lock(&m_dmxMutex);

    if (m_mode != MultiButtonMode::Level)
        return;
    if (m_currentIndex < 0 || m_currentIndex >= m_levelPresets.size())
        return;
    if (m_levelChannelBindings.isEmpty())
        return;

    const LevelPreset& preset = m_levelPresets.at(m_currentIndex);
    for (int i = 0; i < m_levelChannelBindings.size() && i < preset.values.size(); ++i)
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

        const uchar target = preset.values.at(i);
        fc->setStart(fc->current());
        fc->setTarget(target);
        fc->setReady(false);
        fc->setElapsed(0);
    }
}

// ---- Helpers --------------------------------------------------------------

int MultiButtonWidget::entryCount() const
{
    return m_mode == MultiButtonMode::Function ? m_functionIds.size()
                                               : m_levelPresets.size();
}

QString MultiButtonWidget::entryLabel(int idx) const
{
    if (idx < 0)
        return QString();

    if (m_mode == MultiButtonMode::Function)
        return m_functionLabels.value(idx);

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

    if (idx < m_levelPresets.size())
        return m_levelPresets.at(idx).iconPath;

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

/** VC Button style #3 borders (see vcbutton.cpp). Coordinates are local to tile (0,0). */
static void drawVcButtonBorder(QPainter& painter, const QRect& rect,
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

static QColor contrastRingOn(const QColor& bg)
{
    return contrastTextOn(bg);
}

static QColor hoverAccentOn(const QColor& bg)
{
    return (bg.lightness() > 160) ? QColor(QStringLiteral("#f59e0b"))
                                  : QColor(QStringLiteral("#ffeb3b"));
}

/** Colored preset row for Level popup menus (macOS ignores native menu highlight). */
class LevelPresetMenuRow : public QFrame
{
public:
    explicit LevelPresetMenuRow(const QColor& bg, int index, QWidget* parent = nullptr)
        : QFrame(parent)
        , m_bg(bg)
        , m_index(index)
        , m_active(false)
        , m_hovered(false)
    {
        setFixedHeight(28);
        setMinimumWidth(140);
        updateBorderStyle();
    }

    int index() const { return m_index; }

    void setActive(bool active)
    {
        if (m_active == active)
            return;
        m_active = active;
        updateBorderStyle();
    }

    void setHovered(bool hovered)
    {
        if (m_hovered == hovered)
            return;
        m_hovered = hovered;
        updateBorderStyle();
    }

private:
    void updateBorderStyle()
    {
        const QColor ring = contrastRingOn(m_bg);
        const QColor hoverRing = hoverAccentOn(m_bg);

        QString border;
        QString extra;

        if (m_active)
        {
            const int width = m_hovered ? 4 : 3;
            border = QStringLiteral("%1px solid %2")
                         .arg(width)
                         .arg(ring.name(QColor::HexRgb));
            extra = QStringLiteral("border-left: 5px solid %1;")
                        .arg(ring.name(QColor::HexRgb));
        }
        else if (m_hovered)
        {
            border = QStringLiteral("2px solid %1").arg(hoverRing.name(QColor::HexRgb));
        }
        else
        {
            border = QStringLiteral("1px solid rgba(0, 0, 0, 0.45)");
        }

        setStyleSheet(QStringLiteral(
                          "QFrame { background-color: %1; border: %2; %3"
                          " border-radius: 3px; }"
                          "QLabel { background: transparent; border: none; }")
                          .arg(m_bg.name(QColor::HexRgb), border, extra));
    }

    QColor m_bg;
    int m_index = 0;
    bool m_active = false;
    bool m_hovered = false;
};

static void updateLevelMenuRowHover(QMenu* menu, QAction* hoveredAction)
{
    if (!menu)
        return;

    const int hoverIdx = hoveredAction ? hoveredAction->data().toInt() : -1;

    for (QAction* action : menu->actions())
    {
        QWidgetAction* widgetAction = qobject_cast<QWidgetAction*>(action);
        if (!widgetAction)
            continue;

        auto* row = static_cast<LevelPresetMenuRow*>(widgetAction->defaultWidget());
        if (!row)
            continue;

        row->setHovered(hoverIdx >= 0 && row->index() == hoverIdx);
    }
}

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

    if (idx < m_levelPresets.size() && m_levelPresets.at(idx).hideName)
        return QString();

    return levelPresetDisplayName(idx);
}

void MultiButtonWidget::addLevelPresetMenuRow(QMenu* menu, int index, bool selected)
{
    if (!menu || index < 0 || index >= m_levelPresets.size())
        return;

    const LevelPreset& preset = m_levelPresets.at(index);
    const QColor bg = preset.color.isValid() ? preset.color : palette().button().color();
    const QColor fg = contrastTextOn(bg);

    auto* row = new LevelPresetMenuRow(bg, index, menu);
    row->setActive(selected);

    QHBoxLayout* lay = new QHBoxLayout(row);
    lay->setContentsMargins(8, 2, 6, 2);
    lay->setSpacing(6);

    if (selected)
    {
        QLabel* mark = new QLabel(QStringLiteral("\u2713"), row);
        mark->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;")
                                .arg(fg.name(QColor::HexRgb)));
        lay->addWidget(mark);
    }

    if (!preset.hideName)
    {
        QLabel* lbl = new QLabel(levelPresetDisplayName(index), row);
        lbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;")
                               .arg(fg.name(QColor::HexRgb)));
        lay->addWidget(lbl, 1);
    }
    else
    {
        lay->addStretch(1);
        row->setToolTip(tr("Preset %1").arg(index + 1));
    }

    if (!entryIconPath(index).isEmpty())
    {
        QPixmap px = iconForEntry(index);
        if (!px.isNull())
        {
            QLabel* iconLbl = new QLabel(row);
            iconLbl->setPixmap(px.scaled(22, 22, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            iconLbl->setStyleSheet(QStringLiteral("background: transparent;"));
            lay->addWidget(iconLbl);
        }
    }

    QWidgetAction* action = new QWidgetAction(menu);
    action->setDefaultWidget(row);
    action->setData(index);
    menu->addAction(action);
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

    int total = entryCount() + (m_addOffAtEnd ? 1 : 0);
    int next  = (m_currentIndex + 1) % total;

    if (m_addOffAtEnd && next == entryCount())
    {
        stopCurrent();
        updateFeedback();
        update();
    }
    else
    {
        activate(next);
    }
}

void MultiButtonWidget::activate(int idx)
{
    if (!m_visualOnly && idx == m_currentIndex) return;

    stopCurrent();

    if (idx < 0 || idx >= entryCount())
    {
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

    m_currentIndex = idx;
    m_visualOnly   = false;
    m_lastActivationTime.restart();
    updateFeedback();
    update();
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
    m_lastActivationTime.restart();
}

// ---- Monitor channel values -----------------------------------------------

void MultiButtonWidget::slotCheckChannelValues()
{
    if (!m_monitorChannelValues) return;

    if (m_lastActivationTime.isValid() && m_lastActivationTime.elapsed() < 500)
        return;

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

            if (allMatch) { matchIdx = entry; break; }
        }
    }
    else
    {
        if (m_levelChannelBindings.isEmpty())
        {
            m_doc->inputOutputMap()->releaseUniverses(false);
            return;
        }

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
                quint8 expected = i < preset.values.size() ? preset.values.at(i) : 0;

                if ((int) uni >= universes.count()) { allMatch = false; break; }
                if (universes.at(uni)->preGMValue(addr) != expected) { allMatch = false; break; }
            }

            if (allMatch) { matchIdx = entry; break; }
        }
    }

    m_doc->inputOutputMap()->releaseUniverses(false);

    if (matchIdx >= 0 && matchIdx != m_currentIndex)
    {
        m_currentIndex = matchIdx;
        m_visualOnly   = true;
        updateFeedback();
        update();
    }
}

// ---- Mode ----------------------------------------------------------------

void MultiButtonWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Design)
    {
        m_doc->masterTimer()->unregisterDMXSource(this);
        releaseLevelFaders();

        if (m_visualOnly)
        {
            m_currentIndex = -1;
            m_visualOnly   = false;
        }
        else
        {
            stopCurrent();
        }

        if (m_channelMonitorTimer)
            m_channelMonitorTimer->stop();
    }
    else if (mode == Doc::Operate)
    {
        reactivateLevelPreset();

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
            const int hit = spreadHitTest(e->pos());
            if (!m_longFired && hit == m_pressTileIndex && hit != -2)
            {
                if (hit < 0)
                    stopCurrent();
                else
                    activate(hit);
            }
            m_pressTileIndex = -2;
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

void MultiButtonWidget::slotLongPressFired()
{
    if (m_layout != MultiButtonLayout::Single)
        return;

    m_longFired = true;
    showPopupMenu(QCursor::pos());
    update();
}

void MultiButtonWidget::contextMenuEvent(QContextMenuEvent* e)
{
    if (mode() == Doc::Operate && m_layout == MultiButtonLayout::Single)
    {
        showPopupMenu(e->globalPos());
        e->accept();
        return;
    }
    VCWidget::contextMenuEvent(e);
}

void MultiButtonWidget::showPopupMenu(const QPoint& globalPos)
{
    if (entryCount() == 0) return;

    QMenu menu(this);
    menu.setTitle(caption().isEmpty() ? tr("Multi Button") : caption());
    menu.setStyleSheet(QStringLiteral("QMenu { padding: 3px; }"));

    if (m_mode == MultiButtonMode::Level)
    {
        for (int i = 0; i < entryCount(); ++i)
            addLevelPresetMenuRow(&menu, i, i == m_currentIndex);

        connect(&menu, &QMenu::hovered, [&menu](QAction* action) {
            updateLevelMenuRowHover(&menu, action);
        });
    }
    else
    {
        for (int i = 0; i < entryCount(); ++i)
        {
            QAction* act = menu.addAction(popupMenuTextForEntry(i));
            act->setData(i);
            if (i == m_currentIndex)
                act->setFont(QFont(act->font().family(), act->font().pointSize(), QFont::Bold));
        }
    }

    if (m_addOffAtEnd)
    {
        menu.addSeparator();
        QAction* offAct = menu.addAction(tr("OFF (deactivate)"));
        offAct->setData(-1);
        if (m_currentIndex < 0)
            offAct->setFont(QFont(offAct->font().family(), offAct->font().pointSize(), QFont::Bold));
    }

    QAction* chosen = menu.exec(globalPos);
    if (chosen)
    {
        int idx = chosen->data().toInt();
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
}

// ---- Custom context menu (Design mode) ------------------------------------

QMenu* MultiButtonWidget::customMenu(QMenu* parentMenu)
{
    if (entryCount() == 0) return nullptr;

    QMenu* iconMenu = new QMenu(tr("Entry icon"), parentMenu);

    QMenu* pickMenu = new QMenu(tr("Select entry…"), iconMenu);
    pickMenu->setStyleSheet(QStringLiteral("QMenu { padding: 3px; }"));

    if (m_mode == MultiButtonMode::Level)
    {
        for (int i = 0; i < entryCount(); ++i)
            addLevelPresetMenuRow(pickMenu, i, false);
    }
    else
    {
        for (int i = 0; i < entryCount(); ++i)
        {
            const QString text = QString("%1: %2").arg(i + 1).arg(popupMenuTextForEntry(i));
            QAction* entryAct = pickMenu->addAction(text);
            entryAct->setData(i);
        }
    }

    QAction* scribbleAct = iconMenu->addAction(QIcon(":/edit.png"), tr("Scribble icon…"));
    connect(scribbleAct, &QAction::triggered, this, [this, pickMenu]() {
        QAction* chosen = pickMenu->exec(QCursor::pos());
        if (!chosen) return;
        ScribbleDialog dlg(m_doc, this);
        if (dlg.exec() == QDialog::Accepted)
            setIconForEntry(chosen->data().toInt(), dlg.savedIconPath());
    });

    QAction* chooseAct = iconMenu->addAction(tr("Choose image…"));
    connect(chooseAct, &QAction::triggered, this, [this, pickMenu]() {
        QAction* chosen = pickMenu->exec(QCursor::pos());
        if (!chosen) return;
        int idx = chosen->data().toInt();

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

    iconMenu->addSeparator();

    QAction* resetAct = iconMenu->addAction(tr("Reset icon"));
    connect(resetAct, &QAction::triggered, this, [this, pickMenu]() {
        QAction* chosen = pickMenu->exec(QCursor::pos());
        if (!chosen) return;
        setIconForEntry(chosen->data().toInt(), QString());
    });

    return iconMenu;
}

// ---- External input -------------------------------------------------------

void MultiButtonWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (!acceptsInput()) return;

    quint32 pagedCh = (page() << 16) | channel;

    if (checkInputSource(universe, pagedCh, value, sender(), triggerInputSourceId))
    {
        if (value > 0) cycleNext();
        return;
    }
    if (checkInputSource(universe, pagedCh, value, sender(), popupInputSourceId))
    {
        if (value > 0) showPopupMenu(mapToGlobal(rect().center()));
        return;
    }
}

void MultiButtonWidget::updateFeedback()
{
    sendFeedback(m_currentIndex >= 0 ? 255 : 0, triggerInputSourceId);
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
        m_longPressMs,
        m_addOffAtEnd,
        m_monitorChannelValues,
        m_layout,
        m_spreadColumns,
        m_spreadRows,
        m_spreadHMargin,
        m_spreadVMargin,
        m_spreadTileWidth,
        m_spreadTileHeight,
        inputSource(triggerInputSourceId),
        inputSource(popupInputSourceId),
        page(),
        this);

    if (dlg.exec() != QDialog::Accepted) return;

    setWidgetMode(dlg.widgetMode());
    setEntries(dlg.functionIds(), dlg.functionLabels(), dlg.iconPaths());
    setLevelConfig(dlg.levelChannelBindings(), dlg.levelPresets());
    setLongPressMs(dlg.longPressMs());
    setAddOffAtEnd(dlg.addOffAtEnd());
    setMonitorChannelValues(dlg.monitorChannelValues());
    setWidgetLayout(dlg.widgetLayout());
    setSpreadColumns(dlg.spreadColumns());
    setSpreadRows(dlg.spreadRows());
    setSpreadHMargin(dlg.spreadHMargin());
    setSpreadVMargin(dlg.spreadVMargin());
    setSpreadTileWidth(dlg.spreadTileWidth());
    setSpreadTileHeight(dlg.spreadTileHeight());
    setInputSource(dlg.triggerInputSource(), triggerInputSourceId);
    setInputSource(dlg.popupInputSource(), popupInputSourceId);
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
    copy->setLevelConfig(m_levelChannelBindings, m_levelPresets);
    copy->setLongPressMs(m_longPressMs);
    copy->setAddOffAtEnd(m_addOffAtEnd);
    copy->setMonitorChannelValues(m_monitorChannelValues);
    copy->setWidgetLayout(m_layout);
    copy->setSpreadColumns(m_spreadColumns);
    copy->setSpreadRows(m_spreadRows);
    copy->setSpreadHMargin(m_spreadHMargin);
    copy->setSpreadVMargin(m_spreadVMargin);
    copy->setSpreadTileWidth(m_spreadTileWidth);
    copy->setSpreadTileHeight(m_spreadTileHeight);
    copy->setInputSource(inputSource(triggerInputSourceId), triggerInputSourceId);
    copy->setInputSource(inputSource(popupInputSourceId),   popupInputSourceId);
    return copy;
}

// ---- Cross-project clipboard -----------------------------------------------

void MultiButtonWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    obj["widgetMode"]           = modeToString(m_mode);
    obj["longPressMs"]          = m_longPressMs;
    obj["addOffAtEnd"]          = m_addOffAtEnd;
    obj["monitorChannelValues"] = m_monitorChannelValues;

    QJsonObject spread;
    spread["enabled"]   = (m_layout == MultiButtonLayout::Spread);
    spread["columns"]   = m_spreadColumns;
    spread["rows"]      = m_spreadRows;
    spread["hMargin"]   = m_spreadHMargin;
    spread["vMargin"]   = m_spreadVMargin;
    spread["tileWidth"]  = m_spreadTileWidth;
    spread["tileHeight"] = m_spreadTileHeight;
    obj["spread"] = spread;

    QJsonArray funcs;
    for (int i = 0; i < m_functionIds.size(); ++i)
    {
        Function *f = doc->function(m_functionIds.at(i));
        QJsonObject entry;
        entry["name"]     = f ? f->name() : QString();
        entry["label"]    = m_functionLabels.value(i);
        entry["iconPath"] = m_iconPaths.value(i);
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
        QJsonArray vals;
        for (quint8 v : preset.values)
            vals.append(v);
        po["values"] = vals;
        presetArr.append(po);
    }
    obj["levelPresets"] = presetArr;
}

void MultiButtonWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    setWidgetMode(stringToMode(obj["widgetMode"].toString()));
    m_longPressMs          = obj["longPressMs"].toInt(500);
    m_addOffAtEnd          = obj["addOffAtEnd"].toBool(false);
    m_monitorChannelValues = obj["monitorChannelValues"].toBool(false);

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
    }

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
        for (const QJsonValue& vv : po["values"].toArray())
            preset.values.append((quint8) vv.toInt());
        while (preset.values.size() < bindings.size()) preset.values.append(0);
        while (preset.values.size() > bindings.size()) preset.values.removeLast();
        presets.append(preset);
    }

    setLevelConfig(bindings, presets);
    recalcSpreadSize();
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

bool MultiButtonWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot) return false;

    loadXMLCommon(root);

    QList<quint32>       ids;
    QStringList          labels;
    QStringList          icons;
    MultiButtonMode           widgetMode = MultiButtonMode::Function;
    quint32                   legacyFxId = UINT_MAX;
    QList<quint32>            legacyChannels;
    QList<LevelChannelBinding> levelBindings;
    QList<LevelPreset>        levelPresets;

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
            if (attrs.value(KXMLSpreadEnabled).toInt() != 0)
                m_layout = MultiButtonLayout::Spread;
            setSpreadColumns(attrs.value(KXMLSpreadColumns).toInt());
            setSpreadRows(attrs.value(KXMLSpreadRows).toInt());
            setSpreadHMargin(attrs.value(KXMLSpreadHMargin).toInt());
            setSpreadVMargin(attrs.value(KXMLSpreadVMargin).toInt());
            setSpreadTileWidth(attrs.value(KXMLSpreadTileW).toInt());
            setSpreadTileHeight(attrs.value(KXMLSpreadTileH).toInt());
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLMonitorChannels)
        {
            setMonitorChannelValues(root.readElementText().toInt() != 0);
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

            QString valuesStr = attrs.value(KXMLLevelPresetValues).toString();
            for (const QString& part : valuesStr.split(' ', Qt::SkipEmptyParts))
                preset.values.append((quint8) part.toUInt());

            levelPresets.append(preset);
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLFunction)
        {
            auto attrs = root.attributes();
            ids.append(attrs.value(KXMLFunctionID).toUInt());
            labels.append(attrs.value(KXMLFunctionLabel).toString());
            icons.append(resolveIconPath(attrs.value(KXMLFunctionIconPath).toString(), m_doc));
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
        else
        {
            root.skipCurrentElement();
        }
    }

    m_mode = widgetMode;
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

    while (m_functionLabels.size() < m_functionIds.size())
        m_functionLabels.append(QString());
    while (m_iconPaths.size() < m_functionIds.size())
        m_iconPaths.append(QString());

    for (LevelPreset& preset : m_levelPresets)
    {
        while (preset.values.size() < m_levelChannelBindings.size())
            preset.values.append(0);
        while (preset.values.size() > m_levelChannelBindings.size())
            preset.values.removeLast();
    }

    if (m_currentIndex >= entryCount())
        m_currentIndex = -1;

    m_iconCache.clear();
    rebuildSceneCache();
    recalcSpreadSize();

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

    if (m_mode == MultiButtonMode::Level)
        doc->writeTextElement(KXMLWidgetMode, modeToString(m_mode));

    doc->writeTextElement(KXMLCurrentIndex, QString::number(m_currentIndex));
    doc->writeTextElement(KXMLLongPressMs,  QString::number(m_longPressMs));
    doc->writeTextElement(KXMLAddOffAtEnd,  QString::number(m_addOffAtEnd ? 1 : 0));
    if (m_monitorChannelValues)
        doc->writeTextElement(KXMLMonitorChannels, QString::number(1));

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

            QStringList parts;
            for (quint8 v : preset.values)
                parts.append(QString::number(v));
            doc->writeAttribute(KXMLLevelPresetValues, parts.join(' '));
            doc->writeEndElement();
        }
    }
    else
    {
        for (int i = 0; i < m_functionIds.size(); ++i)
        {
            doc->writeStartElement(KXMLFunction);
            doc->writeAttribute(KXMLFunctionID,       QString::number(m_functionIds.at(i)));
            doc->writeAttribute(KXMLFunctionLabel,    m_functionLabels.value(i));
            doc->writeAttribute(KXMLFunctionIconPath, normalizeIconPath(m_iconPaths.value(i), m_doc));
            doc->writeEndElement();
        }
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

    doc->writeEndElement();
    return true;
}

// ---- Paint ---------------------------------------------------------------

QString MultiButtonWidget::activeFunctionCaption() const
{
    if (m_currentIndex < 0)
        return tr("—");

    if (m_mode == MultiButtonMode::Level)
        return levelPresetDisplayName(m_currentIndex);

    const QString lbl = entryLabel(m_currentIndex);
    if (!lbl.isEmpty())
        return lbl;

    Function* f = functionAt(m_currentIndex);
    return f ? f->name() : tr("?");
}

void MultiButtonWidget::drawTile(QPainter& p, const QRect& tileRect, int tileIndex,
                                 bool isActive, bool isPressed) const
{
    p.save();
    p.translate(tileRect.topLeft());
    const QRect r(0, 0, tileRect.width(), tileRect.height());

    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bg = backgroundColor().isValid()
                ? backgroundColor()
                : palette().button().color();

    if (tileIndex >= 0
        && m_mode == MultiButtonMode::Level
        && tileIndex < m_levelPresets.size())
    {
        const QColor presetColor = m_levelPresets.at(tileIndex).color;
        if (presetColor.isValid())
            bg = presetColor;
    }
    else if (tileIndex < 0)
    {
        bg = palette().mid().color().lighter(130);
    }

    if (isPressed)
        bg = bg.darker(120);
    else if (isActive)
        bg = bg.lighter(115);

    const QRect fillRect = r.adjusted(1, 1, -2, -2);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(fillRect, 3, 3);

    QPixmap iconPx;
    if (tileIndex >= 0)
        iconPx = iconForEntry(tileIndex);

    const bool hasCustomLabel = (tileIndex >= 0) && !entryLabel(tileIndex).isEmpty();
    const bool levelNoText = (tileIndex >= 0
                              && m_mode == MultiButtonMode::Level
                              && tileIndex < m_levelPresets.size()
                              && m_levelPresets.at(tileIndex).hideName);
    const bool showLabelText = tileIndex < 0
                               || (!levelNoText && (iconPx.isNull() || hasCustomLabel
                                                    || m_mode == MultiButtonMode::Level));

    const QColor fg = contrastTextOn(bg);
    const int pad = 3;
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
        if (isActive && tileIndex >= 0 && !m_visualOnly)
            mainFont.setBold(true);
        p.setFont(mainFont);
        p.setPen(fg);
        QRect textRect = inner.adjusted(0, iconAreaH, 0, 0);
        if (textRect.height() > 0)
            p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, cap);
    }

    const bool monitoring = isActive && m_visualOnly;
    drawVcButtonBorder(p, r, isActive, monitoring);

    p.restore();
}

void MultiButtonWidget::paintSpread(QPainter& p)
{
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor panelBg = backgroundColor().isValid()
                     ? backgroundColor()
                     : palette().button().color();
    p.fillRect(rect(), panelBg);

    p.setPen(QPen(palette().mid().color(), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    QColor fg = foregroundColor().isValid()
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

    for (const SpreadTileInfo& tile : computeSpreadTiles())
    {
        const bool isActive = (tile.index < 0) ? (m_currentIndex < 0)
                                               : (tile.index == m_currentIndex);
        const bool isPressed = m_pressActive && (tile.index == m_pressTileIndex);
        drawTile(p, tile.rect, tile.index, isActive, isPressed);
    }
}

void MultiButtonWidget::paintSingle(QPainter& p)
{
    QColor bg = backgroundColor().isValid()
                ? backgroundColor()
                : palette().button().color();

    if (m_mode == MultiButtonMode::Level
        && m_currentIndex >= 0
        && m_currentIndex < m_levelPresets.size())
    {
        const QColor presetColor = m_levelPresets.at(m_currentIndex).color;
        if (presetColor.isValid())
            bg = presetColor;
    }

    if (m_pressActive)   bg = bg.darker(120);
    if (m_currentIndex >= 0) bg = bg.lighter(115);

    p.fillRect(rect(), bg);

    p.setPen(QPen(palette().mid().color(), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    const int dotsReserve = (entryCount() > 0) ? 14 : 0;
    QColor fg = foregroundColor().isValid()
                ? foregroundColor()
                : palette().buttonText().color();

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
    if (m_currentIndex >= 0)
        icon = iconForEntry(m_currentIndex);

    const bool hasCustomLabel = (m_currentIndex >= 0)
                                && !entryLabel(m_currentIndex).isEmpty();
    const bool levelNoText = (m_mode == MultiButtonMode::Level
                              && m_currentIndex >= 0
                              && m_levelPresets.at(m_currentIndex).hideName);
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
        if (m_currentIndex >= 0 && !m_visualOnly) mainFont.setBold(true);
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
                bool active   = isOffDot ? (m_currentIndex < 0)
                                         : (i == m_currentIndex);

                if (isOffDot)
                {
                    p.setBrush(active ? fg : fg.darker(200));
                    p.setPen(active ? QPen(fg, 1) : Qt::NoPen);
                    p.drawRect(x0 + i * (dotSize + gap), y0, dotSize, dotSize);
                }
                else
                {
                    p.setBrush(active ? fg : fg.darker(180));
                    p.setPen(Qt::NoPen);
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
            QString idxStr = m_currentIndex < 0
                ? tr("OFF/%1").arg(n)
                : QString("%1/%2").arg(m_currentIndex + 1).arg(n);
            p.drawText(QRect(0, height() - 14, width(), 12),
                       Qt::AlignCenter, idxStr);
        }
    }
}

void MultiButtonWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (m_layout == MultiButtonLayout::Spread)
        paintSpread(p);
    else
        paintSingle(p);

    p.end();
}

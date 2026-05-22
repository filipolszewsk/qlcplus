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
#include "scribbledialog.h"

#include <QPainter>
#include <QPen>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

// ---- XML tag constants ----------------------------------------------------

static const QString KXMLRoot              = QStringLiteral("PluginWidget");
static const QString KXMLPluginId          = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal       = QStringLiteral("org.qlcplus.vcwidgets.multibutton");
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

// ---- Construction ---------------------------------------------------------

MultiButtonWidget::MultiButtonWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(MultiButtonWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(QString());   // empty by default — title row hidden
    resize(QSize(120, 80));

    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);
    connect(m_longPressTimer, &QTimer::timeout,
            this, &MultiButtonWidget::slotLongPressFired);
}

MultiButtonWidget::~MultiButtonWidget()
{
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
    update();
}

void MultiButtonWidget::setCurrentIndex(int idx)
{
    if (idx < 0 || idx >= m_functionIds.size())
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
    update();
}

void MultiButtonWidget::setIconForEntry(int idx, const QString& path)
{
    if (idx < 0 || idx >= m_functionIds.size()) return;
    while (m_iconPaths.size() < m_functionIds.size())
        m_iconPaths.append(QString());
    m_iconPaths[idx] = path;
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

// ---- Helpers --------------------------------------------------------------

Function* MultiButtonWidget::functionAt(int idx) const
{
    if (idx < 0 || idx >= m_functionIds.size()) return nullptr;
    return m_doc->function(m_functionIds.at(idx));
}

QPixmap MultiButtonWidget::iconForEntry(int idx) const
{
    if (idx < 0 || idx >= m_iconPaths.size()) return QPixmap();
    const QString& path = m_iconPaths.at(idx);
    if (path.isEmpty()) return QPixmap();

    if (m_iconCache.contains(idx))
        return m_iconCache.value(idx);

    QPixmap px(path);
    if (!px.isNull())
        m_iconCache.insert(idx, px);
    return px;
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
    if (m_functionIds.isEmpty()) return;

    int total = m_functionIds.size() + (m_addOffAtEnd ? 1 : 0);
    int next  = (m_currentIndex + 1) % total;

    if (m_addOffAtEnd && next == m_functionIds.size())
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

    if (idx < 0 || idx >= m_functionIds.size())
    {
        m_currentIndex = -1;
        m_visualOnly   = false;
        updateFeedback();
        update();
        return;
    }

    Function* f = functionAt(idx);
    if (f)
        f->start(m_doc->masterTimer(), functionParent());

    m_currentIndex = idx;
    m_visualOnly   = false;
    m_lastActivationTime.restart();
    updateFeedback();
    update();
}

void MultiButtonWidget::stopCurrent()
{
    if (m_currentIndex < 0) return;
    if (!m_visualOnly)
    {
        Function* f = functionAt(m_currentIndex);
        if (f) f->stop(functionParent());
    }
    m_currentIndex = -1;
    m_visualOnly   = false;
    m_lastActivationTime.restart();
}

// ---- Monitor channel values -----------------------------------------------

void MultiButtonWidget::slotCheckChannelValues()
{
    if (!m_monitorChannelValues) return;
    if (m_cachedSceneValues.isEmpty()) return;

    // Grace period: skip right after the user activated something manually
    if (m_lastActivationTime.isValid() && m_lastActivationTime.elapsed() < 500)
        return;

    // Rebuild cache if sizes are out of sync (defensive)
    if (m_cachedSceneValues.size() != m_functionIds.size())
        rebuildSceneCache();

    QList<Universe*> universes = m_doc->inputOutputMap()->claimUniverses();

    int matchIdx = -1;
    for (int entry = 0; entry < m_cachedSceneValues.size(); ++entry)
    {
        const QList<SceneValue>& vals = m_cachedSceneValues.at(entry);
        if (vals.isEmpty()) continue;   // non-Scene entry, skip

        bool allMatch = true;
        for (const SceneValue& scv : vals)
        {
            Fixture* fixture = m_doc->fixture(scv.fxi);
            if (!fixture) { allMatch = false; break; }

            quint32 uni  = fixture->universe();
            quint32 addr = fixture->address() + scv.channel;

            if ((int)uni >= universes.count()) { allMatch = false; break; }
            if (universes.at(uni)->preGMValue(addr) != scv.value) { allMatch = false; break; }
        }

        if (allMatch) { matchIdx = entry; break; }
    }

    m_doc->inputOutputMap()->releaseUniverses(false);

    if (matchIdx >= 0 && matchIdx != m_currentIndex)
    {
        // Reflect external state visually — do NOT call Function::start
        m_currentIndex = matchIdx;
        m_visualOnly   = true;
        updateFeedback();
        update();
    }
    // On no match: keep current index unchanged (per user requirement)
}

// ---- Mode ----------------------------------------------------------------

void MultiButtonWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Design)
    {
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
        if (m_monitorChannelValues)
        {
            // Refresh cache on Operate entry to pick up any Scene edits
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
        m_longPressTimer->start(m_longPressMs);
        update();
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

        if (!m_longFired && rect().contains(e->pos()))
            cycleNext();

        update();
        e->accept();
        return;
    }
    VCWidget::mouseReleaseEvent(e);
}

void MultiButtonWidget::slotLongPressFired()
{
    m_longFired = true;
    showPopupMenu(QCursor::pos());
    update();
}

void MultiButtonWidget::contextMenuEvent(QContextMenuEvent* e)
{
    if (mode() == Doc::Operate)
    {
        showPopupMenu(e->globalPos());
        e->accept();
        return;
    }
    VCWidget::contextMenuEvent(e);
}

void MultiButtonWidget::showPopupMenu(const QPoint& globalPos)
{
    if (m_functionIds.isEmpty()) return;

    QMenu menu(this);
    menu.setTitle(caption().isEmpty() ? tr("Multi Button") : caption());

    for (int i = 0; i < m_functionIds.size(); ++i)
    {
        const QString lbl = m_functionLabels.value(i);
        Function* f = functionAt(i);
        QString text = lbl.isEmpty() ? (f ? f->name() : tr("Function %1").arg(i + 1)) : lbl;

        QAction* act = menu.addAction(text);
        // Show icon thumbnail in popup if available
        QPixmap px = iconForEntry(i);
        if (!px.isNull())
            act->setIcon(QIcon(px.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        act->setCheckable(true);
        act->setChecked(i == m_currentIndex);
        act->setData(i);
    }

    if (m_addOffAtEnd)
    {
        menu.addSeparator();
        QAction* offAct = menu.addAction(tr("OFF (deactivate)"));
        offAct->setCheckable(true);
        offAct->setChecked(m_currentIndex < 0);
        offAct->setData(-1);
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
    if (m_functionIds.isEmpty()) return nullptr;

    QMenu* iconMenu = new QMenu(tr("Entry icon"), parentMenu);

    // Sub-menu to pick which entry to target
    QMenu* pickMenu = new QMenu(tr("Select entry…"), iconMenu);
    for (int i = 0; i < m_functionIds.size(); ++i)
    {
        const QString lbl = m_functionLabels.value(i);
        Function* f = functionAt(i);
        QString text = QString("%1: %2").arg(i + 1)
            .arg(lbl.isEmpty() ? (f ? f->name() : tr("?")) : lbl);
        QAction* entryAct = pickMenu->addAction(text);
        entryAct->setData(i);
    }

    // "Scribble icon…" action
    QAction* scribbleAct = iconMenu->addAction(QIcon(":/edit.png"), tr("Scribble icon…"));
    connect(scribbleAct, &QAction::triggered, this, [this, pickMenu]() {
        QAction* chosen = pickMenu->exec(QCursor::pos());
        if (!chosen) return;
        int idx = chosen->data().toInt();
        ScribbleDialog dlg(m_doc, this);
        if (dlg.exec() == QDialog::Accepted)
            setIconForEntry(idx, dlg.savedIconPath());
    });

    // "Choose image…" action
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
            m_iconPaths.value(idx),
            tr("Images (%1)").arg(formats));
        if (!path.isEmpty())
            setIconForEntry(idx, path);
    });

    iconMenu->addSeparator();

    // "Reset icon" action
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
        m_functionIds,
        m_functionLabels,
        m_iconPaths,
        m_longPressMs,
        m_addOffAtEnd,
        m_monitorChannelValues,
        inputSource(triggerInputSourceId),
        inputSource(popupInputSourceId),
        page(),
        this);

    if (dlg.exec() != QDialog::Accepted) return;

    setEntries(dlg.functionIds(), dlg.functionLabels(), dlg.iconPaths());
    setLongPressMs(dlg.longPressMs());
    setAddOffAtEnd(dlg.addOffAtEnd());
    setMonitorChannelValues(dlg.monitorChannelValues());
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
    copy->setEntries(m_functionIds, m_functionLabels, m_iconPaths);
    copy->setLongPressMs(m_longPressMs);
    copy->setAddOffAtEnd(m_addOffAtEnd);
    copy->setMonitorChannelValues(m_monitorChannelValues);
    copy->setInputSource(inputSource(triggerInputSourceId), triggerInputSourceId);
    copy->setInputSource(inputSource(popupInputSourceId),   popupInputSourceId);
    return copy;
}

// ---- Cross-project clipboard -----------------------------------------------

void MultiButtonWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    obj["longPressMs"]          = m_longPressMs;
    obj["addOffAtEnd"]          = m_addOffAtEnd;
    obj["monitorChannelValues"] = m_monitorChannelValues;

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
}

void MultiButtonWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    m_longPressMs          = obj["longPressMs"].toInt(500);
    m_addOffAtEnd          = obj["addOffAtEnd"].toBool(false);
    m_monitorChannelValues = obj["monitorChannelValues"].toBool(false);

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
        return QFileInfo(path).fileName();   // filename-only for portability

    return doc->normalizeComponentPath(path);
}

bool MultiButtonWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot) return false;

    loadXMLCommon(root);

    QList<quint32>  ids;
    QStringList     labels;
    QStringList     icons;

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
        else if (root.name() == KXMLMonitorChannels)
        {
            setMonitorChannelValues(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLFunction)
        {
            auto attrs = root.attributes();
            ids.append(attrs.value(KXMLFunctionID).toUInt());
            labels.append(attrs.value(KXMLFunctionLabel).toString());
            QString stored = attrs.value(KXMLFunctionIconPath).toString();
            icons.append(resolveIconPath(stored, m_doc));
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

    m_functionIds    = ids;
    m_functionLabels = labels;
    m_iconPaths      = icons;

    while (m_functionLabels.size() < m_functionIds.size())
        m_functionLabels.append(QString());
    while (m_iconPaths.size() < m_functionIds.size())
        m_iconPaths.append(QString());

    if (m_currentIndex >= m_functionIds.size())
        m_currentIndex = -1;

    m_iconCache.clear();
    rebuildSceneCache();

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

    doc->writeTextElement(KXMLCurrentIndex, QString::number(m_currentIndex));
    doc->writeTextElement(KXMLLongPressMs,  QString::number(m_longPressMs));
    doc->writeTextElement(KXMLAddOffAtEnd,  QString::number(m_addOffAtEnd ? 1 : 0));
    if (m_monitorChannelValues)
        doc->writeTextElement(KXMLMonitorChannels, QString::number(1));

    for (int i = 0; i < m_functionIds.size(); ++i)
    {
        doc->writeStartElement(KXMLFunction);
        doc->writeAttribute(KXMLFunctionID,       QString::number(m_functionIds.at(i)));
        doc->writeAttribute(KXMLFunctionLabel,    m_functionLabels.value(i));
        doc->writeAttribute(KXMLFunctionIconPath, normalizeIconPath(m_iconPaths.value(i), m_doc));
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

    doc->writeEndElement();
    return true;
}

// ---- Paint ---------------------------------------------------------------

QString MultiButtonWidget::activeFunctionCaption() const
{
    if (m_currentIndex < 0)
        return tr("—");
    const QString lbl = m_functionLabels.value(m_currentIndex);
    if (!lbl.isEmpty()) return lbl;
    Function* f = functionAt(m_currentIndex);
    return f ? f->name() : tr("?");
}

void MultiButtonWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- Background ---
    QColor bg = backgroundColor().isValid()
                ? backgroundColor()
                : palette().button().color();

    if (m_pressActive)   bg = bg.darker(120);
    if (m_currentIndex >= 0) bg = bg.lighter(115);

    p.fillRect(rect(), bg);

    // --- Border ---
    p.setPen(QPen(palette().mid().color(), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    const int dotsReserve = (m_functionIds.size() > 0) ? 14 : 0;
    QColor fg = foregroundColor().isValid()
                ? foregroundColor()
                : palette().buttonText().color();

    // --- Optional title row (only if caption is set) ---
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

    // --- Per-entry icon (active entry only) ---
    const int iconTopPad = topPad + titleH;
    QPixmap icon;
    if (m_currentIndex >= 0)
        icon = iconForEntry(m_currentIndex);

    // When there is an icon but NO custom label, the icon fills all available
    // space and no text is drawn below it. When there is a custom label, the
    // icon is kept small (~40%) and the label appears beneath it.
    const bool hasCustomLabel = (m_currentIndex >= 0)
                                && !m_functionLabels.value(m_currentIndex).isEmpty();
    const bool showLabelText  = icon.isNull() || hasCustomLabel;

    int iconAreaH = 0;
    if (!icon.isNull())
    {
        int availH = height() - iconTopPad - dotsReserve - 4;
        int availW = width() - 8;
        int dim;
        if (!hasCustomLabel)
            dim = qMin(availW, qMax(availH, 1));        // fill all available area
        else
            dim = qMin(availW, qMax(availH, 1)) * 4 / 10;  // small, label goes below
        dim = qMax(16, dim);
        QPixmap scaled = icon.scaled(dim, dim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int ix = (width() - scaled.width()) / 2;
        int iy = !hasCustomLabel
                 ? iconTopPad + qMax(0, (availH - scaled.height()) / 2)  // vertically centred
                 : iconTopPad + 2;                                         // top-aligned
        p.drawPixmap(ix, iy, scaled);
        iconAreaH = hasCustomLabel ? scaled.height() + 4 : availH;
    }

    // --- Center: active function name (only when no full-screen icon) ---
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

    // --- Monitor indicator dot (top-right corner) ---
    // green = monitoring, amber = currently showing externally-matched scene
    if (m_monitorChannelValues)
    {
        QColor dotColor = m_visualOnly ? QColor(255, 170, 0) : QColor(100, 200, 100);
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(width() - 9, 3, 6, 6);
    }

    // --- Dot indicators at the bottom ---
    int n         = m_functionIds.size();
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

    p.end();
}

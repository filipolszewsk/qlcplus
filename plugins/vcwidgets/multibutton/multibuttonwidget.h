/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonwidget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QTimer>
#include <QPoint>
#include <QList>
#include <QStringList>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPaintEvent>
#include <QElapsedTimer>
#include <QPixmap>
#include <QColor>
#include <QHash>
#include <QMap>
#include <QMenu>
#include <QMutex>

#include "vcwidget.h"
#include "genericfader.h"
#include "functionparent.h"
#include "scenevalue.h"
#include "dmxsource.h"

class Doc;
class Function;

enum class MultiButtonMode
{
    Function,
    Level
};

struct LevelChannelBinding
{
    quint32 fixtureId = 0;
    quint32 channel   = 0;

    bool operator==(const LevelChannelBinding& o) const
    { return fixtureId == o.fixtureId && channel == o.channel; }
};

struct LevelPreset
{
    QString       label;
    QString       iconPath;
    QColor        color;      // invalid = use default widget background
    bool          hideName = false;  // true = no text on button (user cleared name)
    QList<quint8> values;   // parallel to m_levelChannelBindings
};

class MultiButtonWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    static const quint8 triggerInputSourceId = 0;   // cycle-next
    static const quint8 popupInputSourceId   = 1;   // open popup

    explicit MultiButtonWidget(QWidget* parent, Doc* doc);
    ~MultiButtonWidget() override;

    // ---- Mode ------------------------------------------------------------
    MultiButtonMode widgetMode() const { return m_mode; }
    void setWidgetMode(MultiButtonMode mode);

    // ---- Function list management ----------------------------------------
    void           setEntries(const QList<quint32>& ids, const QStringList& labels,
                              const QStringList& iconPaths = QStringList());
    QList<quint32> functionIds()    const { return m_functionIds; }
    QStringList    functionLabels() const { return m_functionLabels; }
    QStringList    iconPaths()      const { return m_iconPaths; }

    // ---- Level mode ------------------------------------------------------
    void setLevelConfig(const QList<LevelChannelBinding>& bindings,
                        const QList<LevelPreset>& presets);
    QList<LevelChannelBinding> levelChannelBindings() const { return m_levelChannelBindings; }
    QList<LevelPreset>         levelPresets()         const { return m_levelPresets; }

    void setCurrentIndex(int idx);    // -1 = none active (calls activate internally)
    int  currentIndex()  const { return m_currentIndex; }

    void setIconForEntry(int idx, const QString& path);

    // ---- Settings --------------------------------------------------------
    int  longPressMs()   const { return m_longPressMs; }
    void setLongPressMs(int ms);

    bool addOffAtEnd()   const { return m_addOffAtEnd; }
    void setAddOffAtEnd(bool v);

    bool monitorChannelValues() const { return m_monitorChannelValues; }
    void setMonitorChannelValues(bool enable);

    // ---- FunctionParent (required for Function::start/stop) --------------
    FunctionParent functionParent() const;

    // ---- DMXSource -------------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

    // ---- VCWidget overrides ----------------------------------------------
    VCWidget* createCopy(VCWidget* parent) override;
    void      toClipboardJson(QJsonObject &obj, const Doc *doc) const override;
    void      fromClipboardJson(const QJsonObject &obj, Doc *doc) override;
    void      updateFeedback() override;
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      editProperties() override;

    QMenu*    customMenu(QMenu* parentMenu) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

private slots:
    void slotLongPressFired();
    void slotCheckChannelValues();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    void paintEvent(QPaintEvent* e) override;

private:
    void cycleNext();
    void activate(int idx);
    void stopCurrent();
    void showPopupMenu(const QPoint& globalPos);

    QString popupMenuTextForEntry(int idx) const;

    void addLevelPresetMenuRow(QMenu* menu, int index, bool selected);

    void rebuildSceneCache();
    void updateDmxRegistration();
    void releaseLevelFaders();
    void reactivateLevelPreset();

    int     entryCount() const;
    QString entryLabel(int idx) const;
    QString levelPresetDisplayName(int idx) const;
    QString entryIconPath(int idx) const;

    QString   activeFunctionCaption() const;
    Function* functionAt(int idx) const;
    QPixmap   iconForEntry(int idx) const;

    // ---- Mode ------------------------------------------------------------
    MultiButtonMode m_mode = MultiButtonMode::Function;

    // ---- Function list state --------------------------------------------
    QList<quint32> m_functionIds;
    QStringList    m_functionLabels;
    QStringList    m_iconPaths;

    // ---- Level mode state -----------------------------------------------
    QList<LevelChannelBinding> m_levelChannelBindings;
    QList<LevelPreset>         m_levelPresets;
    mutable QMutex     m_dmxMutex;
    QMap<quint32, QSharedPointer<GenericFader>> m_fadersMap;

    int            m_currentIndex = -1;
    bool           m_visualOnly   = false;

    // ---- Icon cache (keyed by entry index) ------------------------------
    mutable QHash<int, QPixmap> m_iconCache;

    // ---- Monitor --------------------------------------------------------
    bool                         m_monitorChannelValues = false;
    QTimer*                      m_channelMonitorTimer  = nullptr;
    QList<QList<SceneValue>>     m_cachedSceneValues;
    QElapsedTimer                m_lastActivationTime;

    // ---- Settings --------------------------------------------------------
    int  m_longPressMs  = 500;
    bool m_addOffAtEnd  = false;

    // ---- Press-tracking state (GUI thread only) --------------------------
    QTimer* m_longPressTimer = nullptr;
    bool    m_pressActive    = false;
    bool    m_longFired      = false;
    QPoint  m_pressPos;
};

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
#include <QHash>
#include <QMenu>

#include "vcwidget.h"
#include "functionparent.h"
#include "scenevalue.h"

class Doc;
class Function;

class MultiButtonWidget : public VCWidget
{
    Q_OBJECT

public:
    static const quint8 triggerInputSourceId = 0;   // cycle-next
    static const quint8 popupInputSourceId   = 1;   // open popup

    explicit MultiButtonWidget(QWidget* parent, Doc* doc);
    ~MultiButtonWidget() override;

    // ---- Function list management ----------------------------------------
    void           setEntries(const QList<quint32>& ids, const QStringList& labels,
                              const QStringList& iconPaths = QStringList());
    QList<quint32> functionIds()    const { return m_functionIds; }
    QStringList    functionLabels() const { return m_functionLabels; }
    QStringList    iconPaths()      const { return m_iconPaths; }

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
    void activate(int idx);   // stop prev, start new
    void stopCurrent();
    void showPopupMenu(const QPoint& globalPos);

    void rebuildSceneCache();   // rebuild m_cachedSceneValues from m_functionIds

    QString   activeFunctionCaption() const;   // current function name for display
    Function* functionAt(int idx) const;
    QPixmap   iconForEntry(int idx) const;     // from cache or loaded

    // ---- Function list state --------------------------------------------
    QList<quint32> m_functionIds;
    QStringList    m_functionLabels;   // parallel; empty string = use fn->name()
    QStringList    m_iconPaths;        // parallel; empty string = no icon
    int            m_currentIndex = -1;
    bool           m_visualOnly   = false;   // true if index set by monitor, not user action

    // ---- Icon cache (keyed by entry index) ------------------------------
    mutable QHash<int, QPixmap> m_iconCache;

    // ---- Monitor --------------------------------------------------------
    bool                         m_monitorChannelValues = false;
    QTimer*                      m_channelMonitorTimer  = nullptr;
    QList<QList<SceneValue>>     m_cachedSceneValues;   // parallel to m_functionIds
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

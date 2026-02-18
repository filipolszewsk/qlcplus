/*
  Q Light Controller Plus
  vcframe.h

  Copyright (c) Heikki Junnila
                Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef VCFRAME_H
#define VCFRAME_H

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QMainWindow>
#include <QComboBox>
#include <QWidget>
#include <QLabel>
#include <QList>
#include <QHash>


#include "vcwidget.h"

class VCFrame;

/*********************************************************************
 * DetachedVCFrame - Window for detached VCFrame widgets
 *********************************************************************/

class DetachedVCFrame : public QMainWindow
{
    Q_OBJECT

public:
    DetachedVCFrame(QWidget *parent, VCFrame *frame);
    VCFrame* frame() const { return m_frame; }

protected:
    void closeEvent(QCloseEvent *ev) override;

signals:
    void closing(VCFrame *frame);

private:
    VCFrame *m_frame;
};

/** @addtogroup ui_vc_widgets
 * @{
 */

#define KXMLQLCVCFrame                  QStringLiteral("Frame")
#define KXMLQLCVCFrameAllowChildren     QStringLiteral("AllowChildren")
#define KXMLQLCVCFrameAllowResize       QStringLiteral("AllowResize")
#define KXMLQLCVCFrameShowHeader        QStringLiteral("ShowHeader")
#define KXMLQLCVCFrameIsCollapsed       QStringLiteral("Collapsed")
#define KXMLQLCVCFrameIsDisabled        QStringLiteral("Disabled")
#define KXMLQLCVCFrameEnableSource      QStringLiteral("Enable")
#define KXMLQLCVCFrameShowEnableButton  QStringLiteral("ShowEnableButton")
#define KXMLQLCVCFrameShowBorder        QStringLiteral("ShowBorder")

#define KXMLQLCVCFrameMultipage   QStringLiteral("Multipage")
#define KXMLQLCVCFramePagesNumber QStringLiteral("PagesNum")
#define KXMLQLCVCFrameCurrentPage QStringLiteral("CurrentPage")
#define KXMLQLCVCFrameNext        QStringLiteral("Next")
#define KXMLQLCVCFramePrevious    QStringLiteral("Previous")
#define KXMLQLCVCFramePagesLoop   QStringLiteral("PagesLoop")
#define KXMLQLCVCFramePerPageSize QStringLiteral("PerPageSize")
#define KXMLQLCVCFramePageSize    QStringLiteral("PageSize")

#define KXMLQLCVCFrameDetached         QStringLiteral("Detached")
#define KXMLQLCVCFrameDetachedGeometry QStringLiteral("DetachedGeometry")

class VCFrameProperties;
class VCFramePageShortcut;

class VCFrame : public VCWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(VCFrame)

public:
    /** Default size for newly-created frames */
    static const QSize defaultSize;

    /** External input source IDs */
    static const quint8 nextPageInputSourceId;
    static const quint8 previousPageInputSourceId;
    static const quint8 enableInputSourceId;
    static const quint8 shortcutsBaseInputSourceId;

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    VCFrame(QWidget* parent, Doc* doc, bool canCollapse = false);
    virtual ~VCFrame();

    void init(bool bottomFrame = false);

    /* Check if this is the virtual console's draw area */
    bool isBottomFrame();

    /*********************************************************************
     * GUI
     *********************************************************************/
public:
    /** @reimp */
    void setDisableState(bool disable);

    /** @reimp */
    void setLiveEdit(bool liveEdit);

    /** @reimp */
    void setCaption(const QString& text) override;

    /** @reimp */
    void setFont(const QFont& font) override;

    /** @reimp */
    QFont font() const override;

    /** @reimp */
    void setForegroundColor(const QColor& color) override;

    /** @reimp */
    QColor foregroundColor() const override;

    void setHeaderVisible(bool enable);

    bool isHeaderVisible() const;

    void setEnableButtonVisible(bool enable);

    bool isEnableButtonVisible() const;

    void setShowBorder(bool enable);

    bool isShowBorder() const;

    bool isCollapsed() const;

    QSize originalSize() const;

signals:
    void disableStateChanged(bool disable);

protected slots:
    void slotCollapseButtonToggled(bool toggle);

    /**
     * When called, this method will set the disable state of
     * this frame and its chidren widget accordingly to the
     * toggle parameter
     *
     * @param toggle true means enable, false means disable
     */
    void slotEnableButtonClicked(bool checked);

    /** Detach frame to a separate window */
    void slotDetachButtonClicked();

public slots:
    /** Reattach frame to its original parent (called from DetachedVCFrame::closeEvent) */
    void slotReattachToParent();

public:
    /** Check if this frame is currently detached */
    bool isDetached() const;

    /** Returns the logical parent (original parent if detached, current parent otherwise) */
    QWidget* logicalParent() const;

    /** Detach frame programmatically (e.g., when loading from XML) */
    void detachToWindow(const QRect &windowGeometry = QRect());

    /** Get/set the saved detached window geometry */
    QRect detachedGeometry() const;
    void setDetachedGeometry(const QRect &geometry);

protected:
    void createHeader();

protected:
    QHBoxLayout *m_hbox;
    QToolButton *m_collapseButton;
    QToolButton *m_enableButton;
    QToolButton *m_detachButton;
    QLabel *m_label;
    bool m_collapsed;
    bool m_showHeader;
    bool m_showEnableButton;
    bool m_showBorder;
    int m_width, m_height;

    /** Detached window state */
    DetachedVCFrame *m_detachedWindow;
    QWidget *m_originalParent;
    QRect m_originalGeometry;  /** Original geometry within parent before detaching */
    int m_originalPage;
    QRect m_detachedGeometry;  /** Saved geometry of detached window for persistence */
    
    /** List of child frames that are currently detached from this frame */
    QList<VCFrame*> m_detachedChildren;

    /*********************************************************************
     * Pages
     *********************************************************************/
public:
    void setMultipageMode(bool enable);
    virtual bool multipageMode() const;

    QList<VCFramePageShortcut *> shortcuts() const;
    void addShortcut();
    void setShortcuts(QList<VCFramePageShortcut *> shortcuts);
    void resetShortcuts();
    void updatePageCombo();

    void setTotalPagesNumber(int num);
    int totalPagesNumber();

    virtual int currentPage();

    void setPagesLoop(bool pagesLoop);
    bool pagesLoop() const;

    void setPerPageSize(bool enable);
    bool isPerPageSize() const;
    void setPageSize(int page, const QSize &size);
    QSize pageSize(int page) const;

    virtual void addWidgetToPageMap(VCWidget *widget);
    virtual void removeWidgetFromPageMap(VCWidget *widget);

    /** @reimp */
    void remapInputSource(quint32 oldUni, quint32 oldAddr, 
                          quint32 newUni, quint32 newAddr, 
                          quint32 channels) override;

    /** @reimp */
    bool hasInputsInRange(quint32 universe, quint32 address, quint32 channels) const override;

public slots:
    void slotPreviousPage();
    void slotNextPage();
    void slotSetPage(int pageNum);

signals:
    void pageChanged(int pageNum);

protected:
    bool m_multiPageMode;
    ushort m_currentPage;
    ushort m_totalPagesNumber;
    QToolButton *m_nextPageBtn, *m_prevPageBtn;
    QComboBox *m_pageCombo;
    bool m_pagesLoop;
    bool m_perPageSize;
    QMap<int, QSize> m_pageSizes;
    QList<VCFramePageShortcut*> m_pageShortcuts;

    /** Here's where the magic takes place. This holds a map
     *  of pages/widgets to be shown/hidden when page is changed */
    QMap <VCWidget *, int> m_pagesMap;

    /*************************************************************************
     * QLC+ mode
     *************************************************************************/
protected slots:
    /** @reimp */
    void slotModeChanged(Doc::Mode mode) override;

    /*********************************************************************
     * Submasters
     *********************************************************************/
protected slots:
    void slotSubmasterValueChanged(qreal value);

public:
    void updateSubmasterValue();

    /*********************************************************************
     * Intensity
     *********************************************************************/
public:
    /** @reimp */
    void adjustIntensity(qreal val) override;

    /*************************************************************************
     * Key sequences
     *************************************************************************/
public:
    /** Set the keyboard key combination to enable/disable the frame */
    void setEnableKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination to enable/disable the frame */
    QKeySequence enableKeySequence() const;

    /** Set the keyboard key combination for skipping to the next page */
    void setNextPageKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for skipping to the next page */
    QKeySequence nextPageKeySequence() const;

    /** Set the keyboard key combination for skipping to the previous page */
    void setPreviousPageKeySequence(const QKeySequence& keySequence);

    /** Get the keyboard key combination for skipping to the previous page */
    QKeySequence previousPageKeySequence() const;

protected slots:
    /** @reimp */
    void slotKeyPressed(const QKeySequence& keySequence) override;

private:
    QKeySequence m_enableKeySequence;
    QKeySequence m_nextPageKeySequence;
    QKeySequence m_previousPageKeySequence;

    /*************************************************************************
     * External Input
     *************************************************************************/
public:
    /** @reimp */
    void updateFeedback() override;

protected slots:
    /** @reimp */
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

    /*********************************************************************
     * Clipboard
     *********************************************************************/
public:
    /** Create a copy of this widget into the given parent */
    VCWidget* createCopy(VCWidget* parent) override;

protected:
    /** Copy the contents for this widget from another widget */
    bool copyFrom(const VCWidget* widget) override;

    /*********************************************************************
     * Properties
     *********************************************************************/
protected:
    QList<VCWidget *> getChildren(VCWidget *obj);
    void applyProperties(VCFrameProperties const& prop);

public:
    /** @reimp */
    void editProperties() override;

    /*********************************************************************
     * Load & Save
     *********************************************************************/
public:
    bool loadXML(QXmlStreamReader &root) override;

    bool saveXML(QXmlStreamWriter *doc) override;

    /**
     * @reimp
     *
     * Propagates the postLoad() call to all children.
     */
    void postLoad() override;

protected:
    /** Can be overridden by subclasses */
    virtual QString xmlTagName() const;

    /*********************************************************************
     * Custom menu
     *********************************************************************/
public:
    /** Get a custom menu specific to this widget. Ownership is transferred
        to the caller, which must delete the returned menu pointer. */
    QMenu* customMenu(QMenu* parentMenu) override;

    /*********************************************************************
     * Event handlers
     *********************************************************************/
protected:
    void paintEvent(QPaintEvent* e) override;
    void handleWidgetSelection(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
};

/** @} */

#endif

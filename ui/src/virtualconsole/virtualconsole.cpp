/*
  Q Light Controller Plus
  virtualconsole.cpp

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QBuffer>
#include <QApplication>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QColorDialog>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QScrollArea>
#include <QSettings>
#include <QKeyEvent>
#include <QMenuBar>
#include <QToolBar>
#include <QString>
#include <QDebug>
#include <QMenu>
#include <QList>

#include "importvcwidgetsdialog.h"
#include "vcpastepropertiesdialog.h"
#include "vcmultipatcheditor.h"
#include "vcpropertieseditor.h"
#include "addvcbuttonmatrix.h"
#include "addvcslidermatrix.h"
#include "vcaudiotriggers.h"
#include "virtualconsole.h"
#include "vcproperties.h"
#include "vcspeeddial.h"
#include "vcsoloframe.h"
#include "vcdockarea.h"
#include "vccuelist.h"
#include "vcbutton.h"
#include "vcslider.h"
#include "vcmatrix.h"
#include "vcframe.h"
#include "vclabel.h"
#include "vcxypad.h"
#include "vcclock.h"
#include "functionwizard.h"
#include "collection.h"
#include "chaserstep.h"
#include "function.h"
#include "chaser.h"
#include "doc.h"
#include "vcwidgetpluginmanagerdialog.h"
#include "vcwidgetpluginmanager.h"
#include "vcwidgetplugininterface.h"

#define SETTINGS_VC_SIZE "virtualconsole/size"

VirtualConsole* VirtualConsole::s_instance = NULL;

/****************************************************************************
 * Initialization
 ****************************************************************************/

VirtualConsole::VirtualConsole(QWidget* parent, Doc* doc)
    : QWidget(parent)
    , m_doc(doc)
    , m_latestWidgetId(0)
    , m_latestGroupId(1)

    , m_editAction(EditNone)
    , m_toolbar(NULL)

    , m_addActionGroup(NULL)
    , m_editActionGroup(NULL)
    , m_bgActionGroup(NULL)
    , m_fgActionGroup(NULL)
    , m_fontActionGroup(NULL)
    , m_frameActionGroup(NULL)
    , m_stackingActionGroup(NULL)

    , m_addButtonAction(NULL)
    , m_addButtonMatrixAction(NULL)
    , m_addSliderAction(NULL)
    , m_addSliderMatrixAction(NULL)
    , m_addKnobAction(NULL)
    , m_addSpeedDialAction(NULL)
    , m_addXYPadAction(NULL)
    , m_addCueListAction(NULL)
    , m_addFrameAction(NULL)
    , m_addSoloFrameAction(NULL)
    , m_addLabelAction(NULL)
    , m_addAudioTriggersAction(NULL)
    , m_addClockAction(NULL)
    , m_addAnimationAction(NULL)

    , m_toolsSettingsAction(NULL)
    , m_functionWizardAction(NULL)

    , m_editCutAction(NULL)
    , m_editCopyAction(NULL)
    , m_editPasteAction(NULL)
    , m_editCopyToClipboardAction(NULL)
    , m_editPasteFromClipboardAction(NULL)
    , m_editDeleteAction(NULL)
    , m_editPropertiesAction(NULL)
    , m_editRenameAction(NULL)
    , m_multiPatchAction(NULL)

    , m_bgColorAction(NULL)
    , m_bgImageAction(NULL)
    , m_bgDefaultAction(NULL)

    , m_fgColorAction(NULL)
    , m_fgDefaultAction(NULL)

    , m_fontAction(NULL)
    , m_resetFontAction(NULL)

    , m_frameSunkenAction(NULL)
    , m_frameRaisedAction(NULL)
    , m_frameNoneAction(NULL)

    , m_stackingRaiseAction(NULL)
    , m_stackingLowerAction(NULL)

    , m_makeGroupAction(NULL)
    , m_ungroupAction(NULL)

    , m_alignTopAction(NULL)
    , m_alignBottomAction(NULL)
    , m_alignLeftAction(NULL)
    , m_alignRightAction(NULL)
    , m_alignCenterVAction(NULL)
    , m_alignCenterHAction(NULL)

    , m_customMenu(NULL)
    , m_editMenu(NULL)
    , m_addMenu(NULL)

    , m_dockArea(NULL)
    , m_contentsLayout(NULL)
    , m_scrollArea(NULL)
    , m_contents(NULL)

    , m_liveEdit(false)
{
    Q_ASSERT(s_instance == NULL);
    s_instance = this;

    Q_ASSERT(doc != NULL);

    /* Main layout */
    new QHBoxLayout(this);
    layout()->setContentsMargins(1, 1, 1, 1);
    layout()->setSpacing(1);

    initActions();
    initDockArea();
    m_contentsLayout = new QVBoxLayout;
    layout()->addItem(m_contentsLayout);
    initMenuBar();
    initContents();

    // Propagate mode changes to all widgets
    connect(m_doc, SIGNAL(modeChanged(Doc::Mode)),
            this, SLOT(slotModeChanged(Doc::Mode)));

    // Refresh Add menu when plugins are hot-loaded or updated
    connect(VCWidgetPluginManager::instance(), &VCWidgetPluginManager::pluginsChanged,
            this, &VirtualConsole::slotPluginsChanged);

    // Smart save/restore on hot-reload: direct connection is critical so
    // widget destruction happens synchronously before the library is unloaded.
    connect(VCWidgetPluginManager::instance(), &VCWidgetPluginManager::aboutToReloadPlugin,
            this, &VirtualConsole::slotAboutToReloadPlugin, Qt::DirectConnection);
    connect(VCWidgetPluginManager::instance(), &VCWidgetPluginManager::pluginReloaded,
            this, &VirtualConsole::slotPluginReloaded);

    // Use the initial mode
    slotModeChanged(m_doc->mode());

    // Nothing is selected
    updateActions();
}

VirtualConsole::~VirtualConsole()
{
    s_instance = NULL;
}

VirtualConsole* VirtualConsole::instance()
{
    return s_instance;
}

Doc *VirtualConsole::getDoc()
{
    return m_doc;
}

quint32 VirtualConsole::newWidgetId()
{
    /* This results in an endless loop if there are UINT_MAX-1 widgets. That,
       however, seems a bit unlikely. */
    while (m_widgetsMap.contains(m_latestWidgetId) ||
           m_latestWidgetId == VCWidget::invalidId())
    {
        m_latestWidgetId++;
    }

    return m_latestWidgetId;
}

quint32 VirtualConsole::newGroupId()
{
    while (m_widgetGroups.contains(m_latestGroupId) ||
           m_latestGroupId == VCWidget::invalidId())
    {
        m_latestGroupId++;
    }
    return m_latestGroupId;
}

/*****************************************************************************
 * Properties
 *****************************************************************************/

VCProperties VirtualConsole::properties() const
{
    return m_properties;
}

/*****************************************************************************
 * Selected widget
 *****************************************************************************/

void VirtualConsole::setEditAction(VirtualConsole::EditAction action)
{
    m_editAction = action;
}

VirtualConsole::EditAction VirtualConsole::editAction() const
{
    return m_editAction;
}

const QList <VCWidget*> VirtualConsole::selectedWidgets() const
{
    return m_selectedWidgets;
}

void VirtualConsole::setWidgetSelected(VCWidget* widget, bool select)
{
    Q_ASSERT(widget != NULL);

    if (select == false)
    {
        m_selectedWidgets.removeAll(widget);
        widget->update();

        /* If widget is in a group, deselect all group siblings too */
        quint32 gid = widget->groupId();
        if (gid != VCWidget::invalidId() && m_widgetGroups.contains(gid))
        {
            foreach (quint32 sibId, m_widgetGroups.value(gid))
            {
                VCWidget* sib = this->widget(sibId);
                if (sib && sib != widget)
                {
                    m_selectedWidgets.removeAll(sib);
                    sib->update();
                }
            }
        }
    }
    else if (select == true && m_selectedWidgets.indexOf(widget) == -1)
    {
        m_selectedWidgets.append(widget);
        widget->update();

        /* If widget is in a group, auto-select all group siblings too */
        quint32 gid = widget->groupId();
        if (gid != VCWidget::invalidId() && m_widgetGroups.contains(gid))
        {
            foreach (quint32 sibId, m_widgetGroups.value(gid))
            {
                VCWidget* sib = this->widget(sibId);
                if (sib && sib != widget && m_selectedWidgets.indexOf(sib) == -1)
                {
                    m_selectedWidgets.append(sib);
                    sib->update();
                }
            }
        }
    }

    /* Change the custom menu to the latest-selected widget's menu */
    updateCustomMenu();

    /* Enable or disable actions */
    updateActions();
}

bool VirtualConsole::isWidgetSelected(VCWidget* widget) const
{
    if (widget == NULL || m_selectedWidgets.indexOf(widget) == -1)
        return false;
    else
        return true;
}

void VirtualConsole::clearWidgetSelection()
{
    /* Get a copy of selected widget list */
    QList <VCWidget*> widgets(m_selectedWidgets);

    /* Clear the list so isWidgetSelected() returns false for all widgets */
    m_selectedWidgets.clear();

    /* Update all widgets to clear the selection frame around them */
    QListIterator <VCWidget*> it(widgets);
    while (it.hasNext() == true)
        it.next()->update();

    /* Change the custom menu to the latest-selected widget's menu */
    updateCustomMenu();

    /* Enable or disable actions */
    updateActions();
}

void VirtualConsole::reselectWidgets()
{
    QList <VCWidget*> widgets(m_selectedWidgets);
    clearWidgetSelection();
    foreach (VCWidget* w, widgets)
        setWidgetSelected(w, true);
}

/*****************************************************************************
 * Actions, menu- and toolbar
 *****************************************************************************/

QMenu* VirtualConsole::customMenu() const
{
    return m_customMenu;
}

QMenu* VirtualConsole::editMenu() const
{
    return m_editMenu;
}

QMenu* VirtualConsole::addMenu() const
{
    return m_addMenu;
}

void VirtualConsole::initActions()
{
    /* Add menu actions */
    m_addButtonAction = new QAction(QIcon(":/button.png"), tr("New Button"), this);
    connect(m_addButtonAction, SIGNAL(triggered(bool)), this, SLOT(slotAddButton()), Qt::QueuedConnection);

    m_addButtonMatrixAction = new QAction(QIcon(":/buttonmatrix.png"), tr("New Button Matrix"), this);
    connect(m_addButtonMatrixAction, SIGNAL(triggered(bool)), this, SLOT(slotAddButtonMatrix()), Qt::QueuedConnection);

    m_addSliderAction = new QAction(QIcon(":/slider.png"), tr("New Slider"), this);
    connect(m_addSliderAction, SIGNAL(triggered(bool)), this, SLOT(slotAddSlider()), Qt::QueuedConnection);

    m_addSliderMatrixAction = new QAction(QIcon(":/slidermatrix.png"), tr("New Slider Matrix"), this);
    connect(m_addSliderMatrixAction, SIGNAL(triggered(bool)), this, SLOT(slotAddSliderMatrix()), Qt::QueuedConnection);

    m_addKnobAction = new QAction(QIcon(":/knob.png"), tr("New Knob"), this);
    connect(m_addKnobAction, SIGNAL(triggered(bool)), this, SLOT(slotAddKnob()), Qt::QueuedConnection);

    m_addSpeedDialAction = new QAction(QIcon(":/speed.png"), tr("New Speed Dial"), this);
    connect(m_addSpeedDialAction, SIGNAL(triggered(bool)), this, SLOT(slotAddSpeedDial()), Qt::QueuedConnection);

    m_addXYPadAction = new QAction(QIcon(":/xypad.png"), tr("New XY pad"), this);
    connect(m_addXYPadAction, SIGNAL(triggered(bool)), this, SLOT(slotAddXYPad()), Qt::QueuedConnection);

    m_addCueListAction = new QAction(QIcon(":/cuelist.png"), tr("New Cue list"), this);
    connect(m_addCueListAction, SIGNAL(triggered(bool)), this, SLOT(slotAddCueList()), Qt::QueuedConnection);

    m_addFrameAction = new QAction(QIcon(":/frame.png"), tr("New Frame"), this);
    connect(m_addFrameAction, SIGNAL(triggered(bool)), this, SLOT(slotAddFrame()), Qt::QueuedConnection);

    m_addSoloFrameAction = new QAction(QIcon(":/soloframe.png"), tr("New Solo frame"), this);
    connect(m_addSoloFrameAction, SIGNAL(triggered(bool)), this, SLOT(slotAddSoloFrame()), Qt::QueuedConnection);

    m_addLabelAction = new QAction(QIcon(":/label.png"), tr("New Label"), this);
    connect(m_addLabelAction, SIGNAL(triggered(bool)), this, SLOT(slotAddLabel()), Qt::QueuedConnection);

    m_addAudioTriggersAction = new QAction(QIcon(":/audioinput.png"), tr("New Audio Triggers"), this);
    connect(m_addAudioTriggersAction, SIGNAL(triggered(bool)), this, SLOT(slotAddAudioTriggers()), Qt::QueuedConnection);

    m_addClockAction = new QAction(QIcon(":/clock.png"), tr("New Clock"), this);
    connect(m_addClockAction, SIGNAL(triggered(bool)), this, SLOT(slotAddClock()), Qt::QueuedConnection);

    m_addAnimationAction = new QAction(QIcon(":/animation.png"), tr("New Animation"), this);
    connect(m_addAnimationAction, SIGNAL(triggered(bool)), this, SLOT(slotAddAnimation()), Qt::QueuedConnection);

    /* Dynamic entries for installed VC widget plugins */
    const auto pluginList = VCWidgetPluginManager::instance()->plugins();
    for (VCWidgetPluginInterface* plugin : pluginList)
    {
        QAction* act = new QAction(plugin->icon(),
                                   tr("New %1").arg(plugin->name()), this);
        connect(act, &QAction::triggered, this, [this, plugin]() {
            slotAddPluginWidget(plugin);
        });
        m_addPluginActions.append(act);
    }

    m_pluginSectionSeparator = nullptr;

    /* "Get more widgets..." always appears last in the Add menu */
    m_managePluginsAction = new QAction(QIcon(":/plugin.png"),
                                        tr("Get more widgets..."), this);
    connect(m_managePluginsAction, &QAction::triggered,
            this, &VirtualConsole::slotManagePlugins);

    /* Put add actions under the same group */
    m_addActionGroup = new QActionGroup(this);
    m_addActionGroup->setExclusive(false);
    m_addActionGroup->addAction(m_addButtonAction);
    m_addActionGroup->addAction(m_addButtonMatrixAction);
    m_addActionGroup->addAction(m_addSliderAction);
    m_addActionGroup->addAction(m_addSliderMatrixAction);
    m_addActionGroup->addAction(m_addKnobAction);
    m_addActionGroup->addAction(m_addSpeedDialAction);
    m_addActionGroup->addAction(m_addXYPadAction);
    m_addActionGroup->addAction(m_addCueListAction);
    m_addActionGroup->addAction(m_addFrameAction);
    m_addActionGroup->addAction(m_addSoloFrameAction);
    m_addActionGroup->addAction(m_addLabelAction);
    m_addActionGroup->addAction(m_addAudioTriggersAction);
    m_addActionGroup->addAction(m_addClockAction);
    m_addActionGroup->addAction(m_addAnimationAction);
    for (QAction* act : m_addPluginActions)
        m_addActionGroup->addAction(act);

    /* Tools menu actions */
    m_toolsSettingsAction = new QAction(QIcon(":/configure.png"), tr("Virtual Console Settings"), this);
    connect(m_toolsSettingsAction, SIGNAL(triggered(bool)), this, SLOT(slotToolsSettings()));
    // Prevent this action from ending up to the application menu on OSX
    // and crashing the app after VC window is closed.
    m_toolsSettingsAction->setMenuRole(QAction::NoRole);

    m_functionWizardAction = new QAction(QIcon(":/wizard.png"), tr("VC Fixture Widget Wizard"), this);
    connect(m_functionWizardAction, SIGNAL(triggered(bool)), this, SLOT(slotWizard()));

    /* Edit menu actions */
    m_editCutAction = new QAction(QIcon(":/editcut.png"), tr("Cut"), this);
    connect(m_editCutAction, SIGNAL(triggered(bool)), this, SLOT(slotEditCut()));

    m_editCopyAction = new QAction(QIcon(":/editcopy.png"), tr("Copy"), this);
    connect(m_editCopyAction, SIGNAL(triggered(bool)), this, SLOT(slotEditCopy()));

    m_editPasteAction = new QAction(QIcon(":/editpaste.png"), tr("Paste"), this);
    m_editPasteAction->setEnabled(false);
    connect(m_editPasteAction, SIGNAL(triggered(bool)), this, SLOT(slotEditPaste()));

    m_editCopyToClipboardAction = new QAction(QIcon(":/copyproject.png"),
                                              tr("Copy to system clipboard (cross-project)"), this);
    m_editCopyToClipboardAction->setToolTip(
        tr("Serialize selected widgets to the system clipboard so they can be pasted into another QLC+ project"));
    connect(m_editCopyToClipboardAction, SIGNAL(triggered(bool)),
            this, SLOT(slotEditCopyToClipboard()));

    m_editPasteFromClipboardAction = new QAction(QIcon(":/pasteproject.png"),
                                                 tr("Paste from system clipboard (cross-project)"), this);
    m_editPasteFromClipboardAction->setToolTip(
        tr("Paste widgets previously copied from another QLC+ project"));
    connect(m_editPasteFromClipboardAction, SIGNAL(triggered(bool)),
            this, SLOT(slotEditPasteFromClipboard()));

    m_editDeleteAction = new QAction(QIcon(":/editdelete.png"), tr("Delete"), this);
    connect(m_editDeleteAction, SIGNAL(triggered(bool)), this, SLOT(slotEditDelete()));

    m_editPropertiesAction = new QAction(QIcon(":/edit.png"), tr("Widget Properties"), this);
    connect(m_editPropertiesAction, SIGNAL(triggered(bool)), this, SLOT(slotEditProperties()));

    m_editRenameAction = new QAction(QIcon(":/editclear.png"), tr("Rename Widget"), this);
    connect(m_editRenameAction, SIGNAL(triggered(bool)), this, SLOT(slotEditRename()));

    m_multiPatchAction = new QAction(QIcon(":/edit.png"), tr("Multi-Patch"), this);
    connect(m_multiPatchAction, SIGNAL(triggered(bool)), this, SLOT(slotMultiPatch()));

    /* Put edit actions under the same group */
    m_editActionGroup = new QActionGroup(this);
    m_editActionGroup->setExclusive(false);
    m_editActionGroup->addAction(m_editCutAction);
    m_editActionGroup->addAction(m_editCopyAction);
    m_editActionGroup->addAction(m_editPasteAction);
    m_editActionGroup->addAction(m_editDeleteAction);
    m_editActionGroup->addAction(m_editPropertiesAction);
    m_editActionGroup->addAction(m_editRenameAction);
    m_editActionGroup->addAction(m_multiPatchAction);

    /* Background menu actions */
    m_bgColorAction = new QAction(QIcon(":/color.png"), tr("Background Color"), this);
    connect(m_bgColorAction, SIGNAL(triggered(bool)), this, SLOT(slotBackgroundColor()));

    m_bgImageAction = new QAction(QIcon(":/image.png"), tr("Background Image"), this);
    connect(m_bgImageAction, SIGNAL(triggered(bool)), this, SLOT(slotBackgroundImage()));

    m_bgDefaultAction = new QAction(QIcon(":/undo.png"), tr("Default"), this);
    connect(m_bgDefaultAction, SIGNAL(triggered(bool)), this, SLOT(slotBackgroundNone()));

    /* Put BG actions under the same group */
    m_bgActionGroup = new QActionGroup(this);
    m_bgActionGroup->setExclusive(false);
    m_bgActionGroup->addAction(m_bgColorAction);
    m_bgActionGroup->addAction(m_bgImageAction);
    m_bgActionGroup->addAction(m_bgDefaultAction);

    /* Foreground menu actions */
    m_fgColorAction = new QAction(QIcon(":/fontcolor.png"), tr("Font Colour"), this);
    connect(m_fgColorAction, SIGNAL(triggered(bool)), this, SLOT(slotForegroundColor()));

    m_fgDefaultAction = new QAction(QIcon(":/undo.png"), tr("Default"), this);
    connect(m_fgDefaultAction, SIGNAL(triggered(bool)), this, SLOT(slotForegroundNone()));

    /* Put FG actions under the same group */
    m_fgActionGroup = new QActionGroup(this);
    m_fgActionGroup->setExclusive(false);
    m_fgActionGroup->addAction(m_fgColorAction);
    m_fgActionGroup->addAction(m_fgDefaultAction);

    /* Font menu actions */
    m_fontAction = new QAction(QIcon(":/fonts.png"), tr("Font"), this);
    connect(m_fontAction, SIGNAL(triggered(bool)), this, SLOT(slotFont()));

    m_resetFontAction = new QAction(QIcon(":/undo.png"), tr("Default"), this);
    connect(m_resetFontAction, SIGNAL(triggered(bool)), this, SLOT(slotResetFont()));

    /* Put font actions under the same group */
    m_fontActionGroup = new QActionGroup(this);
    m_fontActionGroup->setExclusive(false);
    m_fontActionGroup->addAction(m_fontAction);
    m_fontActionGroup->addAction(m_resetFontAction);

    /* Frame menu actions */
    m_frameSunkenAction = new QAction(QIcon(":/framesunken.png"), tr("Sunken"), this);
    connect(m_frameSunkenAction, SIGNAL(triggered(bool)), this, SLOT(slotFrameSunken()));

    m_frameRaisedAction = new QAction(QIcon(":/frameraised.png"), tr("Raised"), this);
    connect(m_frameRaisedAction, SIGNAL(triggered(bool)), this, SLOT(slotFrameRaised()));

    m_frameNoneAction = new QAction(QIcon(":/framenone.png"), tr("None"), this);
    connect(m_frameNoneAction, SIGNAL(triggered(bool)), this, SLOT(slotFrameNone()));

    /* Put frame actions under the same group */
    m_frameActionGroup = new QActionGroup(this);
    m_frameActionGroup->setExclusive(false);
    m_frameActionGroup->addAction(m_frameRaisedAction);
    m_frameActionGroup->addAction(m_frameSunkenAction);
    m_frameActionGroup->addAction(m_frameNoneAction);

    /* Stacking menu actions */
    m_stackingRaiseAction = new QAction(QIcon(":/up.png"), tr("Bring to front"), this);
    connect(m_stackingRaiseAction, SIGNAL(triggered(bool)), this, SLOT(slotStackingRaise()));

    m_stackingLowerAction = new QAction(QIcon(":/down.png"), tr("Send to back"), this);
    connect(m_stackingLowerAction, SIGNAL(triggered(bool)), this, SLOT(slotStackingLower()));

    /* Put stacking actions under the same group */
    m_stackingActionGroup = new QActionGroup(this);
    m_stackingActionGroup->setExclusive(false);
    m_stackingActionGroup->addAction(m_stackingRaiseAction);
    m_stackingActionGroup->addAction(m_stackingLowerAction);

    /* Position lock action */
    m_lockPositionAction = new QAction(QIcon(":/lock.png"), tr("Lock position"), this);
    m_lockPositionAction->setCheckable(true);
    m_lockPositionAction->setEnabled(false);
    connect(m_lockPositionAction, SIGNAL(triggered(bool)), this, SLOT(slotToggleLockPosition()));

    /* Group selection actions */
    m_makeGroupAction = new QAction(tr("Make Group"), this);
    m_makeGroupAction->setEnabled(false);
    connect(m_makeGroupAction, SIGNAL(triggered(bool)), this, SLOT(slotMakeGroup()));

    m_ungroupAction = new QAction(tr("Ungroup"), this);
    m_ungroupAction->setEnabled(false);
    connect(m_ungroupAction, SIGNAL(triggered(bool)), this, SLOT(slotUngroup()));

    /* Align actions */
    m_alignTopAction = new QAction(tr("Align Top"), this);
    m_alignTopAction->setEnabled(false);
    connect(m_alignTopAction, SIGNAL(triggered(bool)), this, SLOT(slotAlignTop()));

    m_alignBottomAction = new QAction(tr("Align Bottom"), this);
    m_alignBottomAction->setEnabled(false);
    connect(m_alignBottomAction, SIGNAL(triggered(bool)), this, SLOT(slotAlignBottom()));

    m_alignLeftAction = new QAction(tr("Align Left"), this);
    m_alignLeftAction->setEnabled(false);
    connect(m_alignLeftAction, SIGNAL(triggered(bool)), this, SLOT(slotAlignLeft()));

    m_alignRightAction = new QAction(tr("Align Right"), this);
    m_alignRightAction->setEnabled(false);
    connect(m_alignRightAction, SIGNAL(triggered(bool)), this, SLOT(slotAlignRight()));

    m_alignCenterVAction = new QAction(tr("Distribute Vertically"), this);
    m_alignCenterVAction->setEnabled(false);
    connect(m_alignCenterVAction, SIGNAL(triggered(bool)), this, SLOT(slotAlignCenterV()));

    m_alignCenterHAction = new QAction(tr("Distribute Horizontally"), this);
    m_alignCenterHAction->setEnabled(false);
    connect(m_alignCenterHAction, SIGNAL(triggered(bool)), this, SLOT(slotAlignCenterH()));
}

void VirtualConsole::initMenuBar()
{
    /* Add menu */
    m_addMenu = new QMenu(this);
    m_addMenu->setTitle(tr("&Add"));
    m_addMenu->addAction(m_addButtonAction);
    m_addMenu->addAction(m_addButtonMatrixAction);
    m_addMenu->addSeparator();
    m_addMenu->addAction(m_addSliderAction);
    m_addMenu->addAction(m_addSliderMatrixAction);
    m_addMenu->addAction(m_addKnobAction);
    m_addMenu->addAction(m_addSpeedDialAction);
    m_addMenu->addSeparator();
    m_addMenu->addAction(m_addXYPadAction);
    m_addMenu->addAction(m_addCueListAction);
    m_addMenu->addAction(m_addAnimationAction);
    m_addMenu->addAction(m_addAudioTriggersAction);
    m_addMenu->addSeparator();
    m_addMenu->addAction(m_addFrameAction);
    m_addMenu->addAction(m_addSoloFrameAction);
    m_addMenu->addAction(m_addLabelAction);
    m_addMenu->addAction(m_addClockAction);

    /* Plugin widgets section */
    if (!m_addPluginActions.isEmpty())
    {
        m_pluginSectionSeparator = m_addMenu->addSeparator();
        for (QAction* act : m_addPluginActions)
            m_addMenu->addAction(act);
    }
    m_addMenu->addSeparator();
    m_addMenu->addAction(m_managePluginsAction);

    /* Edit menu */
    m_editMenu = new QMenu(this);
    m_editMenu->setTitle(tr("&Edit"));
    m_editMenu->addAction(m_editCutAction);
    m_editMenu->addAction(m_editCopyAction);
    m_editMenu->addAction(m_editPasteAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_editCopyToClipboardAction);
    m_editMenu->addAction(m_editPasteFromClipboardAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_editDeleteAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_editPropertiesAction);
    m_editMenu->addAction(m_editRenameAction);
    m_editMenu->addAction(m_multiPatchAction);
    m_editMenu->addSeparator();

    /* Background Menu */
    QMenu* bgMenu = new QMenu(m_editMenu);
    bgMenu->setTitle(tr("&Background"));
    m_editMenu->addMenu(bgMenu);
    bgMenu->addAction(m_bgColorAction);
    bgMenu->addAction(m_bgImageAction);
    bgMenu->addAction(m_bgDefaultAction);

    /* Foreground menu */
    QMenu* fgMenu = new QMenu(m_editMenu);
    fgMenu->setTitle(tr("&Foreground"));
    m_editMenu->addMenu(fgMenu);
    fgMenu->addAction(m_fgColorAction);
    fgMenu->addAction(m_fgDefaultAction);

    /* Font menu */
    QMenu* fontMenu = new QMenu(m_editMenu);
    fontMenu->setTitle(tr("F&ont"));
    m_editMenu->addMenu(fontMenu);
    fontMenu->addAction(m_fontAction);
    fontMenu->addAction(m_resetFontAction);

    /* Frame menu */
    QMenu* frameMenu = new QMenu(m_editMenu);
    frameMenu->setTitle(tr("F&rame"));
    m_editMenu->addMenu(frameMenu);
    frameMenu->addAction(m_frameSunkenAction);
    frameMenu->addAction(m_frameRaisedAction);
    frameMenu->addAction(m_frameNoneAction);

    /* Stacking order menu */
    QMenu* stackMenu = new QMenu(m_editMenu);
    stackMenu->setTitle(tr("Stacking &order"));
    m_editMenu->addMenu(stackMenu);
    stackMenu->addAction(m_stackingRaiseAction);
    stackMenu->addAction(m_stackingLowerAction);

    /* Position lock */
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_lockPositionAction);

    /* Group selection */
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_makeGroupAction);
    m_editMenu->addAction(m_ungroupAction);

    /* Align submenu */
    QMenu* alignMenu = new QMenu(m_editMenu);
    alignMenu->setTitle(tr("&Align"));
    m_editMenu->addMenu(alignMenu);
    alignMenu->addAction(m_alignTopAction);
    alignMenu->addAction(m_alignBottomAction);
    alignMenu->addSeparator();
    alignMenu->addAction(m_alignLeftAction);
    alignMenu->addAction(m_alignRightAction);
    alignMenu->addSeparator();
    alignMenu->addAction(m_alignCenterVAction);
    alignMenu->addAction(m_alignCenterHAction);

    /* Add a separator that separates the common edit items from a custom
       widget menu that gets appended to the edit menu when a selected
       widget provides one. */
    m_editMenu->addSeparator();

    /* Toolbar */
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(26,26));
    m_contentsLayout->addWidget(m_toolbar);

    m_toolbar->addAction(m_addButtonAction);
    m_toolbar->addAction(m_addButtonMatrixAction);
    m_toolbar->addAction(m_addSliderAction);
    m_toolbar->addAction(m_addSliderMatrixAction);
    m_toolbar->addAction(m_addKnobAction);
    m_toolbar->addAction(m_addSpeedDialAction);
    m_toolbar->addAction(m_addXYPadAction);
    m_toolbar->addAction(m_addCueListAction);
    m_toolbar->addAction(m_addAnimationAction);
    m_toolbar->addAction(m_addFrameAction);
    m_toolbar->addAction(m_addSoloFrameAction);
    m_toolbar->addAction(m_addLabelAction);
    m_toolbar->addAction(m_addAudioTriggersAction);
    m_toolbar->addAction(m_addClockAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_editCutAction);
    m_toolbar->addAction(m_editCopyAction);
    m_toolbar->addAction(m_editPasteAction);
    m_toolbar->addAction(m_editCopyToClipboardAction);
    m_toolbar->addAction(m_editPasteFromClipboardAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_editDeleteAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_editPropertiesAction);
    m_toolbar->addAction(m_editRenameAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_stackingRaiseAction);
    m_toolbar->addAction(m_stackingLowerAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_bgColorAction);
    m_toolbar->addAction(m_bgImageAction);
    m_toolbar->addAction(m_fgColorAction);
    m_toolbar->addAction(m_fontAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_functionWizardAction);
    m_toolbar->addAction(m_toolsSettingsAction);
}

void VirtualConsole::updateCustomMenu()
{
    /* Get rid of the custom menu, but delete it later because this might
       be called from the very menu that is being deleted. */
    if (m_customMenu != NULL)
    {
        delete m_customMenu;
        m_customMenu = NULL;
    }

    if (m_selectedWidgets.size() > 0)
    {
        /* Change the custom menu to the last selected widget's menu */
        VCWidget* latestWidget = m_selectedWidgets.last();
        m_customMenu = latestWidget->customMenu(m_editMenu);
        if (m_customMenu != NULL)
            m_editMenu->addMenu(m_customMenu);
    }
    else
    {
        /* Change the custom menu to the bottom frame's menu */
        Q_ASSERT(contents() != NULL);
        m_customMenu = contents()->customMenu(m_editMenu);
        if (m_customMenu != NULL)
            m_editMenu->addMenu(m_customMenu);
    }
}

void VirtualConsole::updateActions()
{
    /* When selected widgets is empty, all actions go to main draw area. */
    if (m_selectedWidgets.isEmpty() == true)
    {
        /* Enable widget additions to draw area */
        m_addActionGroup->setEnabled(true);

        /* Disable edit actions that can't be allowed for draw area */
        m_editCutAction->setEnabled(false);
        m_editCopyAction->setEnabled(false);
        m_editDeleteAction->setEnabled(false);
        m_editRenameAction->setEnabled(false);
        m_editPropertiesAction->setEnabled(false);

        /* All the rest are disabled for draw area, except BG & font */
        m_frameActionGroup->setEnabled(false);
        m_stackingActionGroup->setEnabled(false);
        m_lockPositionAction->setEnabled(false);
        m_lockPositionAction->setChecked(false);
        m_makeGroupAction->setEnabled(false);
        m_ungroupAction->setEnabled(false);
        m_alignTopAction->setEnabled(false);
        m_alignBottomAction->setEnabled(false);
        m_alignLeftAction->setEnabled(false);
        m_alignRightAction->setEnabled(false);
        m_alignCenterVAction->setEnabled(false);
        m_alignCenterHAction->setEnabled(false);

        /* Enable paste to draw area if there's something to paste */
        if (m_clipboard.isEmpty() == true)
            m_editPasteAction->setEnabled(false);
        else
            m_editPasteAction->setEnabled(true);
    }
    else
    {
        /* Enable edit actions for other widgets */
        m_editCutAction->setEnabled(true);
        m_editCopyAction->setEnabled(true);
        m_editDeleteAction->setEnabled(true);
        m_editRenameAction->setEnabled(true);
        m_editPropertiesAction->setEnabled(true);

        /* Enable all common properties */
        m_bgActionGroup->setEnabled(true);
        m_fgActionGroup->setEnabled(true);
        m_fontActionGroup->setEnabled(true);
        m_frameActionGroup->setEnabled(true);
        m_stackingActionGroup->setEnabled(true);

        /* Check, whether the last selected widget can hold children */
        if (m_selectedWidgets.last()->allowChildren() == true)
        {
            /* Enable paste for widgets that can hold children */
            if (m_clipboard.isEmpty() == true)
                m_editPasteAction->setEnabled(false);
            else
                m_editPasteAction->setEnabled(true);

            /* Enable also new additions */
            m_addActionGroup->setEnabled(true);
        }
        else
        {
            /* Allow paste-as-properties when clipboard has a same-type widget.
               Also supports multi-selection: all selected widgets must be
               the same type as the clipboard source and none may hold children. */
            bool samePasteAvailable = false;
            if (!m_clipboard.isEmpty() && m_editAction == EditCopy)
            {
                int sourceType = m_clipboard.first()->type();
                samePasteAvailable = true;
                foreach (VCWidget* w, m_selectedWidgets)
                {
                    if (w->type() != sourceType || w->allowChildren())
                    {
                        samePasteAvailable = false;
                        break;
                    }
                }
            }
            m_editPasteAction->setEnabled(samePasteAvailable);
        }

        /* Lock position: enabled when widgets are selected; checked when ALL are locked */
        m_lockPositionAction->setEnabled(true);
        bool allLocked = true;
        foreach (VCWidget* w, m_selectedWidgets)
        {
            if (!w->positionLocked())
            {
                allLocked = false;
                break;
            }
        }
        m_lockPositionAction->setChecked(allLocked);

        /* Group actions:
           Make Group — enabled when >=2 selected and they do NOT already all share one group
           Ungroup    — enabled when at least one selected widget is in a group */
        bool anyGrouped = false;
        bool allSameGroup = (m_selectedWidgets.size() >= 2);
        quint32 firstGid = m_selectedWidgets.first()->groupId();
        foreach (VCWidget* w, m_selectedWidgets)
        {
            quint32 gid = w->groupId();
            if (gid != VCWidget::invalidId())
                anyGrouped = true;
            if (gid == VCWidget::invalidId() || gid != firstGid)
                allSameGroup = false;
        }
        m_makeGroupAction->setEnabled(m_selectedWidgets.size() >= 2 && !allSameGroup);
        m_ungroupAction->setEnabled(anyGrouped);

        /* Align: enabled when >= 2 widgets selected */
        bool canAlign = (m_selectedWidgets.size() >= 2);
        m_alignTopAction->setEnabled(canAlign);
        m_alignBottomAction->setEnabled(canAlign);
        m_alignLeftAction->setEnabled(canAlign);
        m_alignRightAction->setEnabled(canAlign);
        m_alignCenterVAction->setEnabled(canAlign);
        m_alignCenterHAction->setEnabled(canAlign);
    }

    if (contents()->children().count() == 0)
        m_latestWidgetId = 0;
}

/*****************************************************************************
 * Add menu callbacks
 *****************************************************************************/

VCWidget* VirtualConsole::closestParent() const
{
    /* If nothing is selected, return the bottom-most contents frame */
    if (m_selectedWidgets.isEmpty() == true)
        return contents();

    /* Find the next VCWidget in the hierarchy that accepts children */
    VCWidget* widget = m_selectedWidgets.last();
    while (widget != NULL)
    {
        if (widget->allowChildren() == true)
            return widget;
        else
            widget = qobject_cast<VCWidget*> (widget->parentWidget());
    }

    return NULL;
}

void VirtualConsole::connectWidgetToParent(VCWidget *widget, VCWidget *parent)
{
    if (parent->type() == VCWidget::FrameWidget
            || parent->type() == VCWidget::SoloFrameWidget)
    {
        VCFrame *frame = qobject_cast<VCFrame *>(parent);
        if (frame != NULL)
        {
            widget->setPage(frame->currentPage());
            frame->addWidgetToPageMap(widget);
        }
    }
    else
        widget->setPage(0);

    if (widget->type() == VCWidget::SliderWidget)
    {
        VCSlider *slider = qobject_cast<VCSlider *>(widget);
        if (slider != NULL)
        {
            connect(slider, SIGNAL(submasterValueChanged(qreal)),
                    parent, SLOT(slotSubmasterValueChanged(qreal)));
        }
    }
}

void VirtualConsole::disconnectWidgetFromParent(VCWidget *widget, VCWidget *parent)
{
    if (parent->type() == VCWidget::FrameWidget
            || parent->type() == VCWidget::SoloFrameWidget)
    {
        VCFrame *frame = qobject_cast<VCFrame *>(parent);
        if (frame != NULL)
            frame->removeWidgetFromPageMap(widget);
    }

    if (widget->type() == VCWidget::SliderWidget)
    {
        VCSlider *slider = qobject_cast<VCSlider *>(widget);
        if (slider != NULL)
        {
            disconnect(slider, SIGNAL(submasterValueChanged(qreal)),
                       parent, SLOT(slotSubmasterValueChanged(qreal)));
        }
    }
}

void VirtualConsole::slotAddButton()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCButton* button = new VCButton(parent, m_doc);
    setupWidget(button, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddButtonMatrix()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    AddVCButtonMatrix abm(this, m_doc);
    if (abm.exec() == QDialog::Rejected)
        return;

    int h = abm.horizontalCount();
    int v = abm.verticalCount();
    int sz = abm.buttonSize();

    VCFrame* frame = NULL;
    if (abm.frameStyle() == AddVCButtonMatrix::NormalFrame)
        frame = new VCFrame(parent, m_doc);
    else
        frame = new VCSoloFrame(parent, m_doc);
    Q_ASSERT(frame != NULL);
    addWidgetInMap(frame);
    frame->setHeaderVisible(false);
    connectWidgetToParent(frame, parent);

    // Resize the parent frame to fit the buttons nicely and toggle resizing off
    frame->resize(QSize((h * sz) + 20, (v * sz) + 20));
    frame->setAllowResize(false);

    for (int y = 0; y < v; y++)
    {
        for (int x = 0; x < h; x++)
        {
            VCButton* button = new VCButton(frame, m_doc);
            Q_ASSERT(button != NULL);
            addWidgetInMap(button);
            connectWidgetToParent(button, frame);
            button->move(QPoint(10 + (x * sz), 10 + (y * sz)));
            button->resize(QSize(sz, sz));
            button->show();

            int index = (y * h) + x;
            if (index < abm.functions().size())
            {
                quint32 fid = abm.functions().at(index);
                Function* function = m_doc->function(fid);
                if (function != NULL)
                {
                    button->setFunction(fid);
                    button->setCaption(function->name());
                }
            }
        }
    }

    // Show the frame after adding buttons to prevent flickering
    frame->show();
    frame->move(parent->lastClickPoint());
    frame->setAllowChildren(false); // Don't allow more children
    clearWidgetSelection();
    setWidgetSelected(frame, true);
    m_doc->setModified();
}

void VirtualConsole::slotAddSlider()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCSlider* slider = new VCSlider(parent, m_doc);
    setupWidget(slider, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddSliderMatrix()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    AddVCSliderMatrix avsm(this);
    if (avsm.exec() == QDialog::Rejected)
        return;

    int width = avsm.width();
    int height = avsm.height();
    int count = avsm.amount();

    VCFrame* frame = new VCFrame(parent, m_doc);
    Q_ASSERT(frame != NULL);
    addWidgetInMap(frame);
    frame->setHeaderVisible(false);
    connectWidgetToParent(frame, parent);

    // Resize the parent frame to fit the sliders nicely
    frame->resize(QSize((count * width) + 20, height + 20));
    frame->setAllowResize(false);

    for (int i = 0; i < count; i++)
    {
        VCSlider* slider = new VCSlider(frame, m_doc);
        Q_ASSERT(slider != NULL);
        addWidgetInMap(slider);
        connectWidgetToParent(slider, frame);
        slider->move(QPoint(10 + (width * i), 10));
        slider->resize(QSize(width, height));
        slider->show();
    }

    // Show the frame after adding buttons to prevent flickering
    frame->show();
    frame->move(parent->lastClickPoint());
    frame->setAllowChildren(false); // Don't allow more children
    clearWidgetSelection();
    setWidgetSelected(frame, true);
    m_doc->setModified();
}

void VirtualConsole::slotAddKnob()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCSlider* knob = new VCSlider(parent, m_doc);
    setupWidget(knob, parent);
    knob->resize(QSize(60, 90));
    knob->setWidgetStyle(VCSlider::WKnob);
    knob->setCaption(tr("Knob %1").arg(knob->id()));
    m_doc->setModified();
}

void VirtualConsole::slotAddSpeedDial()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCSpeedDial* dial = new VCSpeedDial(parent, m_doc);
    setupWidget(dial, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddXYPad()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCXYPad* xypad = new VCXYPad(parent, m_doc);
    setupWidget(xypad, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddCueList()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCCueList* cuelist = new VCCueList(parent, m_doc);
    setupWidget(cuelist, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddFrame()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCFrame* frame = new VCFrame(parent, m_doc, true);
    setupWidget(frame, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddSoloFrame()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCSoloFrame* soloframe = new VCSoloFrame(parent, m_doc, true);
    setupWidget(soloframe, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddLabel()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCLabel* label = new VCLabel(parent, m_doc);
    setupWidget(label, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddAudioTriggers()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCAudioTriggers* triggers = new VCAudioTriggers(parent, m_doc);
    setupWidget(triggers, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddClock()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCClock* clock = new VCClock(parent, m_doc);
    setupWidget(clock, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddAnimation()
{
    VCWidget* parent(closestParent());
    if (parent == NULL)
        return;

    VCMatrix* matrix = new VCMatrix(parent, m_doc);
    setupWidget(matrix, parent);
    m_doc->setModified();
}

void VirtualConsole::slotAddPluginWidget(VCWidgetPluginInterface* plugin)
{
    VCWidget* parent = closestParent();
    if (parent == nullptr || plugin == nullptr)
        return;

    VCWidget* widget = plugin->createWidget(parent, m_doc);
    if (widget == nullptr)
    {
        qWarning() << Q_FUNC_INFO << "Plugin" << plugin->pluginId()
                   << "returned null widget";
        return;
    }

    widget->setPluginId(plugin->pluginId());
    setupWidget(widget, parent);
    m_doc->setModified();
}

void VirtualConsole::slotManagePlugins()
{
    VCWidgetPluginManagerDialog dialog(this);
    dialog.exec();
}

void VirtualConsole::slotAboutToReloadPlugin(const QString& pluginId)
{
    if (!m_contents)
        return;

    // Find all live widget instances created by this plugin
    QList<VCWidget*> toSave = m_contents->findChildren<VCWidget*>();

    QList<SavedPluginWidget> savedWidgets;

    for (VCWidget* w : toSave)
    {
        if (w->pluginId() != pluginId)
            continue;

        // Serialize to XML
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        QXmlStreamWriter writer(&buf);
        writer.setAutoFormatting(false);
        bool ok = w->saveXML(&writer);
        buf.close();

        if (!ok)
        {
            qWarning() << Q_FUNC_INFO << "saveXML failed for plugin widget"
                       << pluginId << "— widget will not be restored";
            continue;
        }

        SavedPluginWidget saved;
        saved.xml      = buf.data();
        saved.parent   = qobject_cast<VCFrame*>(w->parent());
        saved.geometry = w->geometry();
        savedWidgets.append(saved);

        // Remove from parent's widget map before deletion
        if (saved.parent)
            saved.parent->removeWidgetFromPageMap(w);

        // Delete immediately — library will be unloaded after this slot returns
        delete w;
    }

    if (!savedWidgets.isEmpty())
        m_pendingPluginRestore[pluginId] = savedWidgets;
}

void VirtualConsole::slotPluginReloaded(const QString& pluginId)
{
    if (pluginId.isEmpty() || !m_pendingPluginRestore.contains(pluginId))
        return;

    VCWidgetPluginInterface* plugin =
        VCWidgetPluginManager::instance()->pluginById(pluginId);

    const QList<SavedPluginWidget> savedWidgets =
        m_pendingPluginRestore.take(pluginId);

    if (!plugin)
    {
        QMessageBox::warning(this, tr("Plugin reload failed"),
            tr("Plugin \"%1\" could not be loaded after update.\n\n"
               "%2 widget instance(s) could not be restored.\n"
               "Please restart QLC+ to try again.")
                .arg(pluginId)
                .arg(savedWidgets.size()));
        return;
    }

    int restored = 0;
    int failed   = 0;

    for (const SavedPluginWidget& saved : savedWidgets)
    {
        VCFrame* parent = saved.parent ? saved.parent : m_contents;

        VCWidget* newWidget = plugin->createWidget(parent, m_doc);
        if (!newWidget)
        {
            ++failed;
            continue;
        }

        newWidget->setPluginId(pluginId);

        QBuffer buf;
        buf.setData(saved.xml);
        buf.open(QIODevice::ReadOnly);
        QXmlStreamReader reader(&buf);

        // Advance to the first start element (the PluginWidget element)
        while (!reader.atEnd() && !reader.isStartElement())
            reader.readNext();

        if (reader.atEnd() || !newWidget->loadXML(reader))
        {
            // loadXML failed (plugin changed XML format) — fall back to
            // using saved geometry and showing the widget as-is
            newWidget->setGeometry(saved.geometry);
            qWarning() << Q_FUNC_INFO
                       << "loadXML failed for reloaded plugin" << pluginId
                       << "— widget restored with default state";
        }

        parent->addWidgetToPageMap(newWidget);
        newWidget->show();
        addWidgetInMap(newWidget);
        ++restored;
    }

    if (failed > 0)
    {
        QMessageBox::information(this, tr("Plugin reloaded"),
            tr("Plugin \"%1\" reloaded.\n"
               "%2 widget(s) restored, %3 could not be recreated.")
                .arg(plugin->name())
                .arg(restored)
                .arg(failed));
    }
}

void VirtualConsole::slotPluginsChanged()
{
    // Remove old plugin section separator (if any)
    if (m_pluginSectionSeparator)
    {
        m_addMenu->removeAction(m_pluginSectionSeparator);
        m_pluginSectionSeparator->deleteLater();
        m_pluginSectionSeparator = nullptr;
    }

    // Remove old plugin actions from menu and action group
    for (QAction* act : m_addPluginActions)
    {
        m_addMenu->removeAction(act);
        m_addActionGroup->removeAction(act);
        act->deleteLater();
    }
    m_addPluginActions.clear();

    // Rebuild plugin actions from current manager state
    const auto pluginList = VCWidgetPluginManager::instance()->plugins();
    for (VCWidgetPluginInterface* plugin : pluginList)
    {
        QAction* act = new QAction(plugin->icon(),
                                   tr("New %1").arg(plugin->name()), this);
        connect(act, &QAction::triggered, this, [this, plugin]() {
            slotAddPluginWidget(plugin);
        });
        m_addPluginActions.append(act);
        m_addActionGroup->addAction(act);
    }

    // Find the separator immediately before m_managePluginsAction and insert
    // plugin section before it (insert separator + actions before that separator)
    if (!m_addPluginActions.isEmpty())
    {
        QList<QAction*> menuActions = m_addMenu->actions();
        QAction* separatorBeforeManage = nullptr;
        for (int i = 0; i < menuActions.size(); ++i)
        {
            if (menuActions[i] == m_managePluginsAction && i > 0
                && menuActions[i - 1]->isSeparator())
            {
                separatorBeforeManage = menuActions[i - 1];
                break;
            }
        }

        QAction* anchor = separatorBeforeManage ? separatorBeforeManage
                                                : m_managePluginsAction;
        m_pluginSectionSeparator = m_addMenu->insertSeparator(anchor);
        for (QAction* act : m_addPluginActions)
            m_addMenu->insertAction(anchor, act);
    }
}

/*****************************************************************************
 * Tools menu callbacks
 *****************************************************************************/

void VirtualConsole::slotToolsSettings()
{
    VCPropertiesEditor vcpe(this, m_properties, m_doc->inputOutputMap());
    if (vcpe.exec() == QDialog::Accepted)
    {
        m_properties = vcpe.properties();
        contents()->resize(m_properties.size());
        m_doc->inputOutputMap()->setGrandMasterChannelMode(m_properties.grandMasterChannelMode());
        m_doc->inputOutputMap()->setGrandMasterValueMode(m_properties.grandMasterValueMode());
        if (m_dockArea != NULL)
        {
            m_dockArea->setGrandMasterVisible(m_properties.grandMasterVisible());
            m_dockArea->setGrandMasterInvertedAppearance(m_properties.grandMasterSliderMode());
        }

        QSettings settings;
        settings.setValue(SETTINGS_BUTTON_SIZE, vcpe.buttonSize());
        settings.setValue(SETTINGS_BUTTON_STATUSLED, vcpe.buttonStatusLED());
        settings.setValue(SETTINGS_SLIDER_SIZE, vcpe.sliderSize());
        settings.setValue(SETTINGS_SPEEDDIAL_SIZE, vcpe.speedDialSize());
        settings.setValue(SETTINGS_SPEEDDIAL_VALUE, vcpe.speedDialValue());
        settings.setValue(SETTINGS_XYPAD_SIZE, vcpe.xypadSize());
        settings.setValue(SETTINGS_CUELIST_SIZE, vcpe.cuelistSize());
        settings.setValue(SETTINGS_FRAME_SIZE, vcpe.frameSize());
        settings.setValue(SETTINGS_SOLOFRAME_SIZE, vcpe.soloFrameSize());
        settings.setValue(SETTINGS_AUDIOTRIGGERS_SIZE, vcpe.audioTriggersSize());
        settings.setValue(SETTINGS_RGBMATRIX_SIZE, vcpe.rgbMatrixSize());

        m_doc->setModified();
    }
}

void VirtualConsole::slotWizard()
{
    FunctionWizard fw(this, m_doc);
    if (fw.exec() == QDialog::Accepted){
        m_doc->setModified();
    }
}

/*****************************************************************************
 * Edit menu callbacks
 *****************************************************************************/

void VirtualConsole::slotEditCut()
{
    /* No need to delete widgets in clipboard because they are actually just
       MOVED to another parent during Paste when m_editAction == EditCut.
       Cutting the widgets does nothing to them unless Paste is invoked. */

    /* Make the edit action valid only if there's something to cut */
    if (m_selectedWidgets.size() == 0)
    {
        m_editAction = EditNone;
        m_clipboard.clear();
        m_editPasteAction->setEnabled(false);
    }
    else
    {
        m_editAction = EditCut;
        m_clipboard = m_selectedWidgets;
        m_editPasteAction->setEnabled(true);
    }

    updateActions();
}

void VirtualConsole::slotEditCopy()
{
    /* Make the edit action valid only if there's something to copy */
    if (m_selectedWidgets.size() == 0)
    {
        m_editAction = EditNone;
        m_clipboard.clear();
        m_editPasteAction->setEnabled(false);
    }
    else
    {
        m_editAction = EditCopy;
        m_clipboard = m_selectedWidgets;
        m_editPasteAction->setEnabled(true);
    }
}

void VirtualConsole::slotEditCopyToClipboard()
{
    if (m_selectedWidgets.isEmpty())
        return;

    QJsonArray widgets;
    for (VCWidget *w : m_selectedWidgets)
    {
        QJsonObject obj;
        w->toClipboardJson(obj, m_doc);
        widgets.append(obj);
    }

    QJsonObject root;
    root["type"]    = QString("qlc_vc_widgets");
    root["version"] = 1;
    root["widgets"] = widgets;

    QJsonDocument doc(root);
    QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Compact));
}

void VirtualConsole::slotEditPasteFromClipboard()
{
    QString text = QApplication::clipboard()->text();
    if (text.isEmpty())
        return;

    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return;

    QJsonObject root = jsonDoc.object();
    if (root["type"].toString() != "qlc_vc_widgets")
    {
        QMessageBox::information(this, tr("Paste from Clipboard"),
                                 tr("Clipboard does not contain VC widget data."));
        return;
    }

    const QJsonArray clipWidgets = root["widgets"].toArray();

    /* --- Paste Properties branch ---
     * Triggered when clipboard has exactly one widget whose type matches ALL
     * selected widgets (and none of them accept children). */
    if (clipWidgets.size() == 1
        && !m_selectedWidgets.isEmpty())
    {
        int srcType = clipWidgets.first().toObject()["widgetType"].toInt(-1);
        bool allSameLeaf = (srcType >= 0);
        for (VCWidget *w : m_selectedWidgets)
        {
            if (w->type() != srcType || w->allowChildren())
            {
                allSameLeaf = false;
                break;
            }
        }
        if (allSameLeaf)
        {
            VCWidget *ghost = createGhostFromJson(clipWidgets.first().toObject());
            if (ghost)
            {
                VCPastePropertiesDialog dlg(ghost, m_selectedWidgets.first(), this);
                if (dlg.exec() == QDialog::Accepted)
                {
                    VCWidget::PastePropertyGroups flags = dlg.selectedFlags();
                    if (flags != 0)
                    {
                        foreach (VCWidget *w, m_selectedWidgets)
                            w->applyPropertiesFrom(ghost, flags);
                        m_doc->setModified();
                    }
                }
                delete ghost;
                return;
            }
        }
    }

    /* --- Import Widgets branch (default: create new widgets) --- */
    VCFrame *targetFrame = nullptr;
    for (VCWidget *w : m_selectedWidgets)
    {
        VCFrame *f = qobject_cast<VCFrame*>(w);
        if (f)
        {
            targetFrame = f;
            break;
        }
    }
    if (!targetFrame)
        targetFrame = m_contents;

    ImportVCWidgetsDialog dlg(root, targetFrame, m_doc, this);
    dlg.exec();
}

VCWidget* VirtualConsole::createGhostFromJson(const QJsonObject &obj)
{
    int wType = obj["widgetType"].toInt(-1);
    if (wType < 0)
        return nullptr;

    VCWidget *widget = nullptr;
    switch (static_cast<VCWidget::WidgetType>(wType))
    {
        case VCWidget::ButtonWidget:    widget = new VCButton(nullptr, m_doc);    break;
        case VCWidget::SliderWidget:    widget = new VCSlider(nullptr, m_doc);    break;
        case VCWidget::LabelWidget:     widget = new VCLabel(nullptr, m_doc);     break;
        case VCWidget::FrameWidget:     widget = new VCFrame(nullptr, m_doc);     break;
        case VCWidget::SoloFrameWidget: widget = new VCSoloFrame(nullptr, m_doc); break;
        case VCWidget::XYPadWidget:     widget = new VCXYPad(nullptr, m_doc);     break;
        case VCWidget::CueListWidget:   widget = new VCCueList(nullptr, m_doc);   break;
        case VCWidget::SpeedDialWidget: widget = new VCSpeedDial(nullptr, m_doc); break;
        case VCWidget::AnimationWidget: widget = new VCMatrix(nullptr, m_doc);    break;
        case VCWidget::ClockWidget:     widget = new VCClock(nullptr, m_doc);     break;
        default: return nullptr;
    }

    widget->fromClipboardJson(obj, m_doc);
    return widget;
}

void VirtualConsole::importWidgetsFromJson(const QJsonArray &widgets,
                                            VCFrame *parentFrame, int &count)
{
    for (const QJsonValue &v : widgets)
    {
        QJsonObject obj = v.toObject();
        int wType = obj["widgetType"].toInt(-1);
        if (wType < 0)
            continue;

        VCWidget *widget = nullptr;

        switch (static_cast<VCWidget::WidgetType>(wType))
        {
            case VCWidget::ButtonWidget:    widget = new VCButton(parentFrame, m_doc);    break;
            case VCWidget::SliderWidget:    widget = new VCSlider(parentFrame, m_doc);    break;
            case VCWidget::LabelWidget:     widget = new VCLabel(parentFrame, m_doc);     break;
            case VCWidget::FrameWidget:     widget = new VCFrame(parentFrame, m_doc);     break;
            case VCWidget::SoloFrameWidget: widget = new VCSoloFrame(parentFrame, m_doc); break;
            case VCWidget::XYPadWidget:     widget = new VCXYPad(parentFrame, m_doc);     break;
            case VCWidget::CueListWidget:   widget = new VCCueList(parentFrame, m_doc);   break;
            case VCWidget::SpeedDialWidget: widget = new VCSpeedDial(parentFrame, m_doc); break;
            case VCWidget::AnimationWidget: widget = new VCMatrix(parentFrame, m_doc);    break;
            case VCWidget::ClockWidget:     widget = new VCClock(parentFrame, m_doc);     break;
            default: continue;
        }

        if (!widget)
            continue;

        /* Populate all properties (common + widget-specific) from JSON */
        widget->fromClipboardJson(obj, m_doc);

        addWidgetInMap(widget);
        connectWidgetToParent(widget, parentFrame);
        widget->show();
        count++;

        /* Recurse for frame children */
        if (obj.contains("children"))
        {
            VCFrame *childFrame = qobject_cast<VCFrame*>(widget);
            if (childFrame)
                importWidgetsFromJson(obj["children"].toArray(), childFrame, count);
        }
    }
}

void VirtualConsole::slotEditPaste()
{
    if (m_clipboard.size() == 0)
    {
        /* Invalidate the edit action if there's nothing to paste */
        m_editAction = EditNone;
        m_editPasteAction->setEnabled(false);
        return;
    }

    /* Selective paste: one or more targets selected, all the same type as the
       clipboard source and none allowing children.  The dialog is shown once
       and the chosen properties are applied to every selected widget. */
    if (m_editAction == EditCopy
        && !m_selectedWidgets.isEmpty()
        && !m_clipboard.isEmpty())
    {
        int sourceType = m_clipboard.first()->type();
        bool allSameType = true;
        foreach (VCWidget* w, m_selectedWidgets)
        {
            if (w->type() != sourceType || w->allowChildren())
            {
                allSameType = false;
                break;
            }
        }
        if (allSameType)
        {
            VCWidget* source = m_clipboard.first();
            VCPastePropertiesDialog dlg(source, m_selectedWidgets.first(), this);
            if (dlg.exec() == QDialog::Accepted)
            {
                VCWidget::PastePropertyGroups flags = dlg.selectedFlags();
                if (flags != 0)
                    foreach (VCWidget* w, m_selectedWidgets)
                        w->applyPropertiesFrom(source, flags);
            }
            return;
        }
    }

    VCWidget* parent;
    VCWidget* widget;
    QRect bounds;

    Q_ASSERT(contents() != NULL);

    /* Select the parent that gets the cut clipboard contents */
    parent = closestParent();

    /* Get the bounding rect for all selected widgets */
    QListIterator <VCWidget*> it(m_clipboard);
    while (it.hasNext() == true)
    {
        widget = it.next();
        Q_ASSERT(widget != NULL);
        bounds = bounds.united(widget->geometry());
    }

    /* Get the upcoming parent's last mouse click point */
    QPoint cp(parent->lastClickPoint());

    if (m_editAction == EditCut)
    {
        it.toFront();
        while (it.hasNext() == true)
        {
            widget = it.next();
            Q_ASSERT(widget != NULL);
            if (widget == parent)
                continue;

            VCWidget* prevParent = qobject_cast<VCWidget*> (widget->parentWidget());
            if (prevParent != NULL)
                disconnectWidgetFromParent(widget, prevParent);

            /* Get widget's relative pos to the bounding rect */
            QPoint p(widget->x() - bounds.x() + cp.x(),
                     widget->y() - bounds.y() + cp.y());

            /* Reparent and move to the correct place */
            widget->setParent(parent);
            connectWidgetToParent(widget, parent);
            widget->move(p);
            widget->show();
        }

        /* Clear clipboard after pasting stuff that was CUT */
        m_clipboard.clear();
        m_editPasteAction->setEnabled(false);
    }
    else if (m_editAction == EditCopy)
    {
        it.toFront();
        while (it.hasNext() == true)
        {
            widget = it.next();
            Q_ASSERT(widget != NULL);
            if (widget == parent)
                continue;

            /* Get widget's relative pos to the bounding rect */
            QPoint p(widget->x() - bounds.x() + cp.x(),
                     widget->y() - bounds.y() + cp.y());

            /* Create a copy and move to correct place */
            VCWidget* copy = widget->createCopy(parent);
            Q_ASSERT(copy != NULL);
            addWidgetInMap(copy);
            connectWidgetToParent(copy, parent);
            copy->move(p);
            copy->show();
        }
    }

    updateActions();
}

void VirtualConsole::slotEditDelete()
{
    QString msg(tr("Do you wish to delete the selected widgets?"));
    QString title(tr("Delete widgets"));
    int result = QMessageBox::question(this, title, msg,
                                       QMessageBox::Yes,
                                       QMessageBox::No);
    if (result == QMessageBox::Yes)
    {
        while (m_selectedWidgets.isEmpty() == false)
        {
            /* Consume the selected list until it is empty and
               delete each widget. */
            VCWidget* widget = m_selectedWidgets.takeFirst();

            /* Remove from group before deleting (may dissolve group if <2 remain) */
            removeFromGroup(widget);
            foreach (VCWidget* child, getChildren(widget))
                removeFromGroup(child);

            m_widgetsMap.remove(widget->id());
            foreach (VCWidget* child, getChildren(widget))
                m_widgetsMap.remove(child->id());
            VCWidget* parent = qobject_cast<VCWidget*> (widget->parentWidget());
            widget->deleteLater();

            if (parent != NULL)
                disconnectWidgetFromParent(widget, parent);

            /* Remove the widget from clipboard as well so that
               deleted widgets won't be pasted anymore anywhere */
            m_clipboard.removeAll(widget);
            m_editPasteAction->setEnabled(false);
        }

        updateActions();
    }
    m_doc->setModified();
}

void VirtualConsole::slotEditProperties()
{
    VCWidget* widget;

    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        widget = contents();
    else
        widget = m_selectedWidgets.last();

    if (widget != NULL)
        widget->editProperties();
}

void VirtualConsole::slotEditRename()
{
    if (m_selectedWidgets.isEmpty() == true)
        return;

    bool ok = false;
    QString text(m_selectedWidgets.last()->caption());
    text = QInputDialog::getText(this, tr("Rename widgets"), tr("Caption:"),
                                 QLineEdit::Normal, text, &ok);
    if (ok == true)
    {
        VCWidget* widget;
        foreach (widget, m_selectedWidgets)
            widget->setCaption(text);
    }
}

void VirtualConsole::slotMultiPatch()
{
    if (m_selectedWidgets.isEmpty())
        return;

    VCMultiPatchEditor editor(m_selectedWidgets, m_doc, this);
    editor.exec();
}

/*****************************************************************************
 * Background menu callbacks
 *****************************************************************************/

void VirtualConsole::slotBackgroundColor()
{
    QColor color;

    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        color = contents()->backgroundColor();
    else
        color = m_selectedWidgets.last()->backgroundColor();

    color = QColorDialog::getColor(color);
    if (color.isValid() == true)
    {
        if (m_selectedWidgets.isEmpty() == true)
        {
            contents()->setBackgroundColor(color);
        }
        else
        {
            VCWidget* widget;
            foreach (widget, m_selectedWidgets)
                widget->setBackgroundColor(color);
        }
    }
}

void VirtualConsole::slotBackgroundImage()
{
    QString path;

    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        path = contents()->backgroundImage();
    else
        path = m_selectedWidgets.last()->backgroundImage();

    path = QFileDialog::getOpenFileName(this,
                                        tr("Select background image"),
                                        path,
                                        QString("%1 (*.png *.bmp *.jpg *.jpeg *.gif)").arg(tr("Images")));
    if (path.isEmpty() == false)
    {
        if (m_selectedWidgets.isEmpty() == true)
        {
            contents()->setBackgroundImage(path);
        }
        else
        {
            VCWidget* widget;
            foreach (widget, m_selectedWidgets)
                widget->setBackgroundImage(path);
        }
    }
}

void VirtualConsole::slotBackgroundNone()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
    {
        contents()->resetBackgroundColor();
    }
    else
    {
        VCWidget* widget;
        foreach (widget, m_selectedWidgets)
            widget->resetBackgroundColor();
    }
}

/*****************************************************************************
 * Foreground menu callbacks
 *****************************************************************************/

void VirtualConsole::slotForegroundColor()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    QColor color(m_selectedWidgets.last()->foregroundColor());
    color = QColorDialog::getColor(color);
    if (color.isValid() == true)
    {
        VCWidget* widget;
        foreach (widget, m_selectedWidgets)
            widget->setForegroundColor(color);
    }
}

void VirtualConsole::slotForegroundNone()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    VCWidget* widget;
    foreach (widget, m_selectedWidgets)
        widget->resetForegroundColor();
}

/*****************************************************************************
 * Font menu callbacks
 *****************************************************************************/

void VirtualConsole::slotFont()
{
    bool ok = false;
    QFont font;

    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        font = contents()->font();
    else
        font = m_selectedWidgets.last()->font();

    /* This crashes with Qt 4.6.x on OSX. Upgrade to 4.7.x. */
    font = QFontDialog::getFont(&ok, font);
    if (ok == true)
    {
        if (m_selectedWidgets.isEmpty() == true)
        {
            contents()->setFont(font);
        }
        else
        {
            VCWidget* widget;
            foreach (widget, m_selectedWidgets)
                widget->setFont(font);
        }
    }
}

void VirtualConsole::slotResetFont()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
    {
        contents()->resetFont();
    }
    else
    {
        VCWidget* widget;
        foreach (widget, m_selectedWidgets)
            widget->resetFont();
    }
}

/*****************************************************************************
 * Stacking menu callbacks
 *****************************************************************************/

void VirtualConsole::slotStackingRaise()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    VCWidget* widget;
    foreach (widget, m_selectedWidgets)
        widget->raise();

    m_doc->setModified();
}

void VirtualConsole::slotStackingLower()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    VCWidget* widget;
    foreach (widget, m_selectedWidgets)
        widget->lower();

    m_doc->setModified();
}

/*****************************************************************************
 * Position lock callbacks
 *****************************************************************************/

void VirtualConsole::slotToggleLockPosition()
{
    if (m_selectedWidgets.isEmpty())
        return;

    bool allLocked = true;
    foreach (VCWidget* w, m_selectedWidgets)
    {
        if (!w->positionLocked())
        {
            allLocked = false;
            break;
        }
    }

    bool newState = !allLocked;
    foreach (VCWidget* w, m_selectedWidgets)
        w->setPositionLocked(newState);

    m_doc->setModified();
    updateActions();
}

/*****************************************************************************
 * Group selection callbacks
 *****************************************************************************/

void VirtualConsole::makeGroup(const QList<VCWidget*>& widgets)
{
    if (widgets.size() < 2)
        return;

    quint32 gid = newGroupId();
    QList<quint32> members;

    foreach (VCWidget* w, widgets)
    {
        /* Remove from any previous group first */
        removeFromGroup(w);
        w->setGroupId(gid);
        members.append(w->id());
    }
    m_widgetGroups.insert(gid, members);
}

void VirtualConsole::ungroup(const QList<VCWidget*>& widgets)
{
    QSet<quint32> toDissolve;
    foreach (VCWidget* w, widgets)
    {
        if (w->groupId() != VCWidget::invalidId())
            toDissolve.insert(w->groupId());
    }

    foreach (quint32 gid, toDissolve)
    {
        foreach (quint32 wid, m_widgetGroups.value(gid))
        {
            VCWidget* w = this->widget(wid);
            if (w)
                w->setGroupId(VCWidget::invalidId());
        }
        m_widgetGroups.remove(gid);
    }
}

void VirtualConsole::removeFromGroup(VCWidget* w)
{
    if (!w || w->groupId() == VCWidget::invalidId())
        return;

    quint32 gid = w->groupId();
    if (!m_widgetGroups.contains(gid))
    {
        w->setGroupId(VCWidget::invalidId());
        return;
    }

    m_widgetGroups[gid].removeAll(w->id());
    w->setGroupId(VCWidget::invalidId());

    /* If only one member left, dissolve the group entirely */
    if (m_widgetGroups[gid].size() < 2)
    {
        foreach (quint32 wid, m_widgetGroups[gid])
        {
            VCWidget* remaining = this->widget(wid);
            if (remaining)
                remaining->setGroupId(VCWidget::invalidId());
        }
        m_widgetGroups.remove(gid);
    }
}

void VirtualConsole::slotMakeGroup()
{
    if (m_selectedWidgets.size() < 2)
        return;

    makeGroup(m_selectedWidgets);
    m_doc->setModified();
    updateActions();
}

void VirtualConsole::slotUngroup()
{
    if (m_selectedWidgets.isEmpty())
        return;

    ungroup(m_selectedWidgets);
    m_doc->setModified();
    updateActions();
}

/*****************************************************************************
 * Align callbacks
 *****************************************************************************/

void VirtualConsole::slotAlignTop()
{
    if (m_selectedWidgets.size() < 2)
        return;

    int minY = INT_MAX;
    foreach (VCWidget* w, m_selectedWidgets)
        minY = qMin(minY, w->y());
    foreach (VCWidget* w, m_selectedWidgets)
        w->move(QPoint(w->x(), minY));

    m_doc->setModified();
}

void VirtualConsole::slotAlignBottom()
{
    if (m_selectedWidgets.size() < 2)
        return;

    int maxBottom = INT_MIN;
    foreach (VCWidget* w, m_selectedWidgets)
        maxBottom = qMax(maxBottom, w->y() + w->height());
    foreach (VCWidget* w, m_selectedWidgets)
        w->move(QPoint(w->x(), maxBottom - w->height()));

    m_doc->setModified();
}

void VirtualConsole::slotAlignLeft()
{
    if (m_selectedWidgets.size() < 2)
        return;

    int minX = INT_MAX;
    foreach (VCWidget* w, m_selectedWidgets)
        minX = qMin(minX, w->x());
    foreach (VCWidget* w, m_selectedWidgets)
        w->move(QPoint(minX, w->y()));

    m_doc->setModified();
}

void VirtualConsole::slotAlignRight()
{
    if (m_selectedWidgets.size() < 2)
        return;

    int maxRight = INT_MIN;
    foreach (VCWidget* w, m_selectedWidgets)
        maxRight = qMax(maxRight, w->x() + w->width());
    foreach (VCWidget* w, m_selectedWidgets)
        w->move(QPoint(maxRight - w->width(), w->y()));

    m_doc->setModified();
}

void VirtualConsole::slotAlignCenterV()
{
    if (m_selectedWidgets.size() < 2)
        return;

    QList<VCWidget*> sorted = m_selectedWidgets;
    std::sort(sorted.begin(), sorted.end(),
        [](VCWidget* a, VCWidget* b){ return a->y() < b->y(); });

    int totalSpan = (sorted.last()->y() + sorted.last()->height()) - sorted.first()->y();
    int sumHeights = 0;
    foreach (VCWidget* w, sorted) sumHeights += w->height();
    int gap = (sorted.size() > 1) ? (totalSpan - sumHeights) / (sorted.size() - 1) : 0;

    int curY = sorted.first()->y();
    foreach (VCWidget* w, sorted)
    {
        w->move(QPoint(w->x(), curY));
        curY += w->height() + gap;
    }
    m_doc->setModified();
}

void VirtualConsole::slotAlignCenterH()
{
    if (m_selectedWidgets.size() < 2)
        return;

    QList<VCWidget*> sorted = m_selectedWidgets;
    std::sort(sorted.begin(), sorted.end(),
        [](VCWidget* a, VCWidget* b){ return a->x() < b->x(); });

    int totalSpan = (sorted.last()->x() + sorted.last()->width()) - sorted.first()->x();
    int sumWidths = 0;
    foreach (VCWidget* w, sorted) sumWidths += w->width();
    int gap = (sorted.size() > 1) ? (totalSpan - sumWidths) / (sorted.size() - 1) : 0;

    int curX = sorted.first()->x();
    foreach (VCWidget* w, sorted)
    {
        w->move(QPoint(curX, w->y()));
        curX += w->width() + gap;
    }
    m_doc->setModified();
}

/*****************************************************************************
 * Frame menu callbacks
 *****************************************************************************/

void VirtualConsole::slotFrameSunken()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    VCWidget* widget;
    foreach (widget, m_selectedWidgets)
        widget->setFrameStyle(KVCFrameStyleSunken);
}

void VirtualConsole::slotFrameRaised()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    VCWidget* widget;
    foreach (widget, m_selectedWidgets)
        widget->setFrameStyle(KVCFrameStyleRaised);
}

void VirtualConsole::slotFrameNone()
{
    Q_ASSERT(contents() != NULL);

    if (m_selectedWidgets.isEmpty() == true)
        return;

    VCWidget* widget;
    foreach (widget, m_selectedWidgets)
        widget->setFrameStyle(KVCFrameStyleNone);
}

/*****************************************************************************
 * Dock area
 *****************************************************************************/

VCDockArea* VirtualConsole::dockArea() const
{
    return m_dockArea;
}

void VirtualConsole::initDockArea()
{
    if (m_dockArea != NULL)
        delete m_dockArea;

    m_dockArea = new VCDockArea(this, m_doc->inputOutputMap());
    m_dockArea->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

    // Add the dock area into the master horizontal layout
    layout()->addWidget(m_dockArea);

    /* Show the dock area by default */
    m_dockArea->show();
}

/*****************************************************************************
 * Contents
 *****************************************************************************/

VCFrame* VirtualConsole::contents() const
{
    return m_contents;
}

void VirtualConsole::resetContents()
{
    if (m_contents != NULL)
        delete m_contents;

    Q_ASSERT(m_scrollArea != NULL);
    m_contents = new VCFrame(m_scrollArea, m_doc);
    m_contents->setFrameStyle(0);

    // Get virtual console size from properties
    QSize size(m_properties.size());
    contents()->resize(size);
    contents()->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_scrollArea->setWidget(contents());

    /* Disconnect old key handlers to prevent duplicates */
    disconnect(this, SIGNAL(keyPressed(const QKeySequence&)),
               contents(), SLOT(slotKeyPressed(const QKeySequence&)));
    disconnect(this, SIGNAL(keyReleased(const QKeySequence&)),
               contents(), SLOT(slotKeyReleased(const QKeySequence&)));

    /* Connect new key handlers */
    connect(this, SIGNAL(keyPressed(const QKeySequence&)),
            contents(), SLOT(slotKeyPressed(const QKeySequence&)));
    connect(this, SIGNAL(keyReleased(const QKeySequence&)),
            contents(), SLOT(slotKeyReleased(const QKeySequence&)));

    /* Make the contents area take up all available space */
    contents()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_clipboard.clear();
    m_selectedWidgets.clear();
    m_latestWidgetId = 0;
    m_widgetsMap.clear();

    /* Update actions' enabled status */
    updateActions();

    /* Reset all properties but size */
    m_properties.setGrandMasterVisible(true);
    m_properties.setGrandMasterSliderMode(GrandMaster::Normal);
    m_properties.setGrandMasterChannelMode(GrandMaster::Intensity);
    m_properties.setGrandMasterValueMode(GrandMaster::Reduce);
    m_properties.setGrandMasterInputSource(InputOutputMap::invalidUniverse(), QLCChannel::invalid());

    m_dockArea->setGrandMasterVisible(m_properties.grandMasterVisible());
    m_dockArea->setGrandMasterInvertedAppearance(m_properties.grandMasterSliderMode());
}

void VirtualConsole::addWidgetInMap(VCWidget* widget)
{
    // Valid ID ?
    if (widget->id() != VCWidget::invalidId())
    {
        // Maybe we don't know this widget yet
        if (!m_widgetsMap.contains(widget->id()))
        {
            m_widgetsMap.insert(widget->id(), widget);
            return;
        }

        // Maybe we already know this widget
        if (m_widgetsMap[widget->id()] == widget)
        {
            qDebug() << Q_FUNC_INFO << "widget" << widget->id() << "already in map";
            return;
        }

        // This widget id conflicts with another one we have to change it.
        qDebug() << Q_FUNC_INFO << "widget id" << widget->id() << "conflicts, creating a new ID";
    }

    quint32 wid = newWidgetId();
    Q_ASSERT(!m_widgetsMap.contains(wid));
    qDebug() << Q_FUNC_INFO << "id=" << wid;
    widget->setID(wid);
    m_widgetsMap.insert(wid, widget);
}

void VirtualConsole::removeWidgetFromMap(VCWidget* widget)
{
    if (widget == NULL)
        return;
    m_widgetsMap.remove(widget->id());
    foreach (VCWidget* child, getChildren(widget))
        m_widgetsMap.remove(child->id());
}

void VirtualConsole::setupWidget(VCWidget *widget, VCWidget *parent)
{
    Q_ASSERT(widget != NULL);
    Q_ASSERT(parent != NULL);

    addWidgetInMap(widget);
    connectWidgetToParent(widget, parent);
    widget->show();
    widget->move(parent->lastClickPoint());
    clearWidgetSelection();
    setWidgetSelected(widget, true);
}

VCWidget *VirtualConsole::widget(quint32 id)
{
    if (id == VCWidget::invalidId())
        return NULL;

    return m_widgetsMap.value(id, NULL);
}

QSet<quint32> VirtualConsole::usedFunctionIDs() const
{
    QSet<quint32> result;

    // Collect directly referenced function IDs from all VC widgets
    QList<quint32> toVisit;
    for (VCWidget *w : m_widgetsMap)
    {
        for (quint32 fid : w->referencedFunctions())
        {
            if (!result.contains(fid))
            {
                result.insert(fid);
                toVisit.append(fid);
            }
        }
    }

    // BFS: expand into functions contained by Chasers/Sequences and Collections
    while (!toVisit.isEmpty())
    {
        quint32 fid = toVisit.takeFirst();
        Function *func = m_doc->function(fid);
        if (func == NULL)
            continue;

        QList<quint32> children;

        if (func->type() == Function::ChaserType || func->type() == Function::SequenceType)
        {
            Chaser *chaser = qobject_cast<Chaser *>(func);
            if (chaser)
            {
                foreach (const ChaserStep &step, chaser->steps())
                    children.append(step.fid);
            }
        }
        else if (func->type() == Function::CollectionType)
        {
            Collection *col = qobject_cast<Collection *>(func);
            if (col)
                children = col->functions();
        }

        foreach (quint32 childFid, children)
        {
            if (childFid != Function::invalidId() && !result.contains(childFid))
            {
                result.insert(childFid);
                toVisit.append(childFid);
            }
        }
    }

    return result;
}

void VirtualConsole::initContents()
{
    Q_ASSERT(layout() != NULL);

    m_scrollArea = new QScrollArea(this);
    m_contentsLayout->addWidget(m_scrollArea);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setWidgetResizable(false);

    resetContents();
}

/*****************************************************************************
 * Key press handler
 *****************************************************************************/

void VirtualConsole::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat() == true || event->key() == 0)
    {
        event->ignore();
        return;
    }

    QKeySequence seq(event->key() | (event->modifiers() & ~Qt::ControlModifier));
    emit keyPressed(seq);

    event->accept();
}

void VirtualConsole::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat() == true || event->key() == 0)
    {
        event->ignore();
        return;
    }

    QKeySequence seq(event->key() | event->modifiers());
    emit keyReleased(seq);

    event->accept();
}

/*****************************************************************************
 * Main application mode
 *****************************************************************************/

void VirtualConsole::toggleLiveEdit()
{
    // No live edit in Design Mode
    Q_ASSERT(m_doc->mode() == Doc::Operate);

    if (m_liveEdit)
    { // live edit was on, disable live edit
        m_liveEdit = false;
        disableEdit();
    }
    else
    { // live edit was off, enable live edit
        m_liveEdit = true;
        enableEdit();
    }

    // inform the widgets of the live edit status
    QHash<quint32, VCWidget*>::iterator widgetIt = m_widgetsMap.begin();
    while (widgetIt != m_widgetsMap.end())
    {
        VCWidget* widget = widgetIt.value();
        if (widget != NULL)
            widget->setLiveEdit(m_liveEdit);
        ++widgetIt;
    }
    m_contents->setLiveEdit(m_liveEdit);
}

bool VirtualConsole::liveEdit() const
{
    return m_liveEdit;
}

void VirtualConsole::enableEdit()
{
    // Allow editing and adding in design mode
    m_toolsSettingsAction->setEnabled(true);
    m_editActionGroup->setEnabled(true);
    m_addActionGroup->setEnabled(true);
    m_bgActionGroup->setEnabled(true);
    m_fgActionGroup->setEnabled(true);
    m_fontActionGroup->setEnabled(true);
    m_frameActionGroup->setEnabled(true);
    m_stackingActionGroup->setEnabled(true);
    m_functionWizardAction->setEnabled(true);

    // Set action shortcuts for design mode
    m_addButtonAction->setShortcut(QKeySequence("CTRL+SHIFT+B"));
    m_addButtonMatrixAction->setShortcut(QKeySequence("CTRL+SHIFT+M"));
    m_addSliderAction->setShortcut(QKeySequence("CTRL+SHIFT+S"));
    m_addSliderMatrixAction->setShortcut(QKeySequence("CTRL+SHIFT+I"));
    m_addKnobAction->setShortcut(QKeySequence("CTRL+SHIFT+K"));
    m_addSpeedDialAction->setShortcut(QKeySequence("CTRL+SHIFT+D"));
    m_addXYPadAction->setShortcut(QKeySequence("CTRL+SHIFT+X"));
    m_addCueListAction->setShortcut(QKeySequence("CTRL+SHIFT+C"));
    m_addFrameAction->setShortcut(QKeySequence("CTRL+SHIFT+F"));
    m_addSoloFrameAction->setShortcut(QKeySequence("CTRL+SHIFT+O"));
    m_addLabelAction->setShortcut(QKeySequence("CTRL+SHIFT+L"));
    m_addAudioTriggersAction->setShortcut(QKeySequence("CTRL+SHIFT+A"));
    m_addClockAction->setShortcut(QKeySequence("CTRL+SHIFT+T"));
    m_addAnimationAction->setShortcut(QKeySequence("CTRL+SHIFT+R"));

    m_editCutAction->setShortcut(QKeySequence("CTRL+X"));
    m_editCopyAction->setShortcut(QKeySequence("CTRL+C"));
    m_editPasteAction->setShortcut(QKeySequence("CTRL+V"));
    m_editDeleteAction->setShortcut(QKeySequence("Delete"));
    m_editPropertiesAction->setShortcut(QKeySequence("CTRL+E"));

    m_bgColorAction->setShortcut(QKeySequence("SHIFT+B"));
    m_bgImageAction->setShortcut(QKeySequence("SHIFT+I"));
    m_bgDefaultAction->setShortcut(QKeySequence("SHIFT+ALT+B"));
    m_fgColorAction->setShortcut(QKeySequence("SHIFT+F"));
    m_fgDefaultAction->setShortcut(QKeySequence("SHIFT+ALT+F"));
    m_fontAction->setShortcut(QKeySequence("SHIFT+O"));
    m_resetFontAction->setShortcut(QKeySequence("SHIFT+ALT+O"));
    m_frameSunkenAction->setShortcut(QKeySequence("SHIFT+S"));
    m_frameRaisedAction->setShortcut(QKeySequence("SHIFT+R"));
    m_frameNoneAction->setShortcut(QKeySequence("SHIFT+ALT+S"));

    m_stackingRaiseAction->setShortcut(QKeySequence("SHIFT+UP"));
    m_stackingLowerAction->setShortcut(QKeySequence("SHIFT+DOWN"));

    // Show toolbar
    m_toolbar->show();
}

void VirtualConsole::disableEdit()
{
    // Don't allow editing or adding in operate mode
    m_toolsSettingsAction->setEnabled(false);
    m_editActionGroup->setEnabled(false);
    m_addActionGroup->setEnabled(false);
    m_bgActionGroup->setEnabled(false);
    m_fgActionGroup->setEnabled(false);
    m_fontActionGroup->setEnabled(false);
    m_frameActionGroup->setEnabled(false);
    m_stackingActionGroup->setEnabled(false);
    m_functionWizardAction->setEnabled(false);

    // Disable action shortcuts in operate mode
    m_addButtonAction->setShortcut(QKeySequence());
    m_addButtonMatrixAction->setShortcut(QKeySequence());
    m_addSliderAction->setShortcut(QKeySequence());
    m_addSliderMatrixAction->setShortcut(QKeySequence());
    m_addKnobAction->setShortcut(QKeySequence());
    m_addSpeedDialAction->setShortcut(QKeySequence());
    m_addXYPadAction->setShortcut(QKeySequence());
    m_addCueListAction->setShortcut(QKeySequence());
    m_addFrameAction->setShortcut(QKeySequence());
    m_addSoloFrameAction->setShortcut(QKeySequence());
    m_addLabelAction->setShortcut(QKeySequence());
    m_addAudioTriggersAction->setShortcut(QKeySequence());
    m_addClockAction->setShortcut(QKeySequence());
    m_addAnimationAction->setShortcut(QKeySequence());

    m_editCutAction->setShortcut(QKeySequence());
    m_editCopyAction->setShortcut(QKeySequence());
    m_editPasteAction->setShortcut(QKeySequence());
    m_editDeleteAction->setShortcut(QKeySequence());
    m_editPropertiesAction->setShortcut(QKeySequence());

    m_bgColorAction->setShortcut(QKeySequence());
    m_bgImageAction->setShortcut(QKeySequence());
    m_bgDefaultAction->setShortcut(QKeySequence());
    m_fgColorAction->setShortcut(QKeySequence());
    m_fgDefaultAction->setShortcut(QKeySequence());
    m_fontAction->setShortcut(QKeySequence());
    m_resetFontAction->setShortcut(QKeySequence());
    m_frameSunkenAction->setShortcut(QKeySequence());
    m_frameRaisedAction->setShortcut(QKeySequence());
    m_frameNoneAction->setShortcut(QKeySequence());

    m_stackingRaiseAction->setShortcut(QKeySequence());
    m_stackingLowerAction->setShortcut(QKeySequence());

    // Hide toolbar; there's nothing usable there in operate mode
    m_toolbar->hide();

    // Make sure the virtual console contents has the focus.
    // Without this, key combinations don't work unless
    // the user clicks on some VC area
    m_contents->setFocus();
}

void VirtualConsole::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    { // Switch from Design mode to Operate mode
        // Hide edit tools
        disableEdit();
    }
    else
    { // Switch from Operate mode to Design mode
        if (m_liveEdit)
        {
            // Edit tools already shown,
            // inform the widgets that we are out of live edit mode
            m_liveEdit = false;
            QHash<quint32, VCWidget*>::iterator widgetIt = m_widgetsMap.begin();
            while (widgetIt != m_widgetsMap.end())
            {
                VCWidget* widget = widgetIt.value();
                if (widget != NULL)
                    widget->cancelLiveEdit();
                ++widgetIt;
            }
            m_contents->cancelLiveEdit();
        }
        else
        {
            // Show edit tools
            enableEdit();
        }
    }
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

bool VirtualConsole::loadXML(QXmlStreamReader &root)
{
    if (root.name() != KXMLQLCVirtualConsole)
    {
        qWarning() << Q_FUNC_INFO << "Virtual Console node not found";
        return false;
    }

    while (root.readNextStartElement())
    {
        //qDebug() << "VC tag:" << root.name();
        if (root.name() == KXMLQLCVCProperties)
        {
            /* Properties */
            m_properties.loadXML(root);
            QSize size(m_properties.size());
            contents()->resize(size);
            contents()->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        }
        else if (root.name() == KXMLQLCVCFrame)
        {
            /* Contents */
            Q_ASSERT(m_contents != NULL);
            m_contents->loadXML(root);
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unknown Virtual Console tag"
                       << root.name().toString();
            root.skipCurrentElement();
        }
    }

    return true;
}

bool VirtualConsole::saveXML(QXmlStreamWriter *doc)
{
    Q_ASSERT(doc != NULL);

    /* Virtual Console entry */
    doc->writeStartElement(KXMLQLCVirtualConsole);

    /* Contents */
    Q_ASSERT(m_contents != NULL);
    m_contents->saveXML(doc);

    /* Properties */
    m_properties.saveXML(doc);

    /* End the <VirtualConsole> tag */
    doc->writeEndElement();

    return true;
}

QList<VCWidget *> VirtualConsole::getChildren(VCWidget *obj)
{
    QList<VCWidget *> list;
    if (obj == NULL)
        return list;
    QListIterator <VCWidget*> it(obj->findChildren<VCWidget*>());
    while (it.hasNext() == true)
    {
        VCWidget* child = it.next();
        list.append(child);
        list.append(getChildren(child));
    }
    return list;
}

void VirtualConsole::postLoad()
{
    m_contents->postLoad();

    /* apply GM values
      this should probably be placed in another place, but at the moment m_properties
      is just loaded in VirtualConsole */
    m_doc->inputOutputMap()->setGrandMasterValue(255);
    m_doc->inputOutputMap()->setGrandMasterValueMode(m_properties.grandMasterValueMode());
    m_doc->inputOutputMap()->setGrandMasterChannelMode(m_properties.grandMasterChannelMode());

    /* Go through widgets, check IDs and register */
    /* widgets to the map */
    /* This code is the same as the one in addWidgetInMap() */
    /* We have to repeat it to limit conflicts if */
    /* one widget was not saved with a valid ID, */
    /* as addWidgetInMap ensures the widget WILL be added */
    QList<VCWidget *> widgetsList = getChildren(m_contents);
    QList<VCWidget *> invalidWidgetsList;
    foreach (VCWidget *widget, widgetsList)
    {
        quint32 wid = widget->id();
        if (wid != VCWidget::invalidId())
        {
            if (!m_widgetsMap.contains(wid))
                m_widgetsMap.insert(wid, widget);
            else if (m_widgetsMap[wid] != widget)
                invalidWidgetsList.append(widget);
        }
        else
            invalidWidgetsList.append(widget);
    }
    foreach (VCWidget *widget, invalidWidgetsList)
        addWidgetInMap(widget);

    m_dockArea->setGrandMasterVisible(m_properties.grandMasterVisible());
    m_dockArea->setGrandMasterInvertedAppearance(m_properties.grandMasterSliderMode());

    /* Rebuild m_widgetGroups from per-widget groupId persisted in XML */
    m_widgetGroups.clear();
    foreach (VCWidget* w, m_widgetsMap)
    {
        quint32 gid = w->groupId();
        if (gid != VCWidget::invalidId())
        {
            m_widgetGroups[gid].append(w->id());
            /* Keep m_latestGroupId ahead of any loaded group ID */
            if (gid >= m_latestGroupId)
                m_latestGroupId = gid + 1;
        }
    }

    m_contents->setFocus();

    emit loaded();
}

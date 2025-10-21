/*
  Q Light Controller
  vcxypadproperties.h

  Copyright (C) Stefan Krumm, Heikki Junnila

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

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QSettings>
#include <QDebug>
#include <QAction>

#include "qlcfixturemode.h"
#include "qlcchannel.h"
#include "qlcmacros.h"

#include "vcxypadfixtureeditor.h"
#include "inputselectionwidget.h"
#include "vcxypadproperties.h"
#include "functionselection.h"
#include "fixtureselection.h"
#include "vcxypadfixture.h"
#include "vcxypadpreset.h"
#include "vcxypadarea.h"
#include "vcxypad.h"
#include "fixturegroup.h"
#include "apputil.h"
#include "scene.h"
#include "doc.h"
#include "efx.h"

#define SETTINGS_GEOMETRY "vcxypad/geometry"

#define KColumnFixture   0
#define KColumnXAxis     1
#define KColumnYAxis     2

/****************************************************************************
 * Initialization
 ****************************************************************************/

VCXYPadProperties::VCXYPadProperties(VCXYPad* xypad, Doc* doc)
    : QDialog(xypad)
    , m_xypad(xypad)
    , m_doc(doc)
{
    Q_ASSERT(doc != NULL);
    Q_ASSERT(xypad != NULL);

    setupUi(this);

    // IDs 0-15 are reserved for XYPad base controls
    m_lastAssignedID = 15;

    QAction* action = new QAction(this);
    action->setShortcut(QKeySequence(QKeySequence::Close));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(reject()));
    addAction(action);

    /********************************************************************
     * General page
     ********************************************************************/

    m_nameEdit->setText(m_xypad->caption());

    if (m_xypad->invertedAppearance() == true)
        m_YInvertedRadio->setChecked(true);

    m_panInputWidget = new InputSelectionWidget(m_doc, this);
    m_panInputWidget->setTitle(tr("Pan / Horizontal Axis"));
    m_panInputWidget->setKeyInputVisibility(false);
    m_panInputWidget->setInputSource(m_xypad->inputSource(VCXYPad::panInputSourceId));
    m_panInputWidget->setWidgetPage(m_xypad->page());
    m_panInputWidget->emitOddValues(true);
    m_panInputWidget->show();
    m_extInputLayout->addWidget(m_panInputWidget);
    connect(m_panInputWidget, SIGNAL(autoDetectToggled(bool)),
            this, SLOT(slotPanAutoDetectToggled(bool)));
    connect(m_panInputWidget, SIGNAL(inputValueChanged(quint32,quint32)),
            this, SLOT(slotPanInputValueChanged(quint32,quint32)));

    m_panFineInputWidget = new InputSelectionWidget(m_doc, this);
    m_panFineInputWidget->setTitle(tr("Pan Fine"));
    m_panFineInputWidget->setKeyInputVisibility(false);
    m_panFineInputWidget->setInputSource(m_xypad->inputSource(VCXYPad::panFineInputSourceId));
    m_panFineInputWidget->setWidgetPage(m_xypad->page());
    m_panFineInputWidget->emitOddValues(true);
    m_panFineInputWidget->show();
    m_extFineInputLayout->addWidget(m_panFineInputWidget);
    connect(m_panFineInputWidget, SIGNAL(autoDetectToggled(bool)),
            this, SLOT(slotPanFineAutoDetectToggled(bool)));
    connect(m_panFineInputWidget, SIGNAL(inputValueChanged(quint32,quint32)),
            this, SLOT(slotPanFineInputValueChanged(quint32,quint32)));

    m_tiltInputWidget = new InputSelectionWidget(m_doc, this);
    m_tiltInputWidget->setTitle(tr("Tilt / Vertical Axis"));
    m_tiltInputWidget->setKeyInputVisibility(false);
    m_tiltInputWidget->setInputSource(m_xypad->inputSource(VCXYPad::tiltInputSourceId));
    m_tiltInputWidget->setWidgetPage(m_xypad->page());
    m_tiltInputWidget->emitOddValues(true);
    m_tiltInputWidget->show();
    m_extInputLayout->addWidget(m_tiltInputWidget);
    connect(m_tiltInputWidget, SIGNAL(autoDetectToggled(bool)),
            this, SLOT(slotTiltAutoDetectToggled(bool)));
    connect(m_tiltInputWidget, SIGNAL(inputValueChanged(quint32,quint32)),
            this, SLOT(slotTiltInputValueChanged(quint32,quint32)));

    m_tiltFineInputWidget = new InputSelectionWidget(m_doc, this);
    m_tiltFineInputWidget->setTitle(tr("Tilt Fine"));
    m_tiltFineInputWidget->setKeyInputVisibility(false);
    m_tiltFineInputWidget->setInputSource(m_xypad->inputSource(VCXYPad::tiltFineInputSourceId));
    m_tiltFineInputWidget->setWidgetPage(m_xypad->page());
    m_tiltFineInputWidget->emitOddValues(true);
    m_tiltFineInputWidget->show();
    m_extFineInputLayout->addWidget(m_tiltFineInputWidget);
    connect(m_tiltFineInputWidget, SIGNAL(autoDetectToggled(bool)),
            this, SLOT(slotTiltFineAutoDetectToggled(bool)));
    connect(m_tiltFineInputWidget, SIGNAL(inputValueChanged(quint32,quint32)),
            this, SLOT(slotTiltFineInputValueChanged(quint32,quint32)));

    m_widthInputWidget = new InputSelectionWidget(m_doc, this);
    m_widthInputWidget->setTitle(tr("Width"));
    m_widthInputWidget->setKeyInputVisibility(false);
    m_widthInputWidget->setInputSource(m_xypad->inputSource(VCXYPad::widthInputSourceId));
    m_widthInputWidget->setWidgetPage(m_xypad->page());
    m_widthInputWidget->show();
    m_sizeInputLayout->addWidget(m_widthInputWidget);

    m_heightInputWidget = new InputSelectionWidget(m_doc, this);
    m_heightInputWidget->setTitle(tr("Height"));
    m_heightInputWidget->setKeyInputVisibility(false);
    m_heightInputWidget->setInputSource(m_xypad->inputSource(VCXYPad::heightInputSourceId));
    m_heightInputWidget->setWidgetPage(m_xypad->page());
    m_heightInputWidget->show();
    m_sizeInputLayout->addWidget(m_heightInputWidget);

    /********************************************************************
     * Fixtures page
     ********************************************************************/

    slotSelectionChanged(NULL);
    fillFixturesTree();

    connect(m_percentageRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotPercentageRadioChecked()));
    connect(m_degreesRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotDegreesRadioChecked()));
    connect(m_dmxRadio, SIGNAL(clicked(bool)),
            this, SLOT(slotDMXRadioChecked()));

    // Fixture Group controls
    connect(m_useFixtureGroupCheck, SIGNAL(toggled(bool)),
            this, SLOT(slotUseFixtureGroupToggled(bool)));
    connect(m_fixtureGroupCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotFixtureGroupChanged(int)));
    connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)),
            this, SLOT(slotFixtureGroupRemoved(quint32)));

    updateFixtureGroupCombo();
    updateRowSelection();

    // Load current state
    if (m_xypad->isFixtureGroupMode())
    {
        m_useFixtureGroupCheck->setChecked(true);
        // Find and select current group in combo
        for (int i = 0; i < m_fixtureGroupCombo->count(); i++)
        {
            if (m_fixtureGroupCombo->itemData(i).toUInt() == m_xypad->fixtureGroupID())
            {
                m_fixtureGroupCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    /********************************************************************
     * Presets page
     ********************************************************************/

    m_presetInputWidget = new InputSelectionWidget(m_doc, this);
    m_presetInputWidget->setCustomFeedbackVisibility(true);
    m_presetInputWidget->setWidgetPage(m_xypad->page());
    m_presetInputWidget->show();
    m_presetInputLayout->addWidget(m_presetInputWidget);

    connect(m_presetInputWidget, SIGNAL(inputValueChanged(quint32,quint32)),
            this, SLOT(slotInputValueChanged(quint32,quint32)));
    connect(m_presetInputWidget, SIGNAL(keySequenceChanged(QKeySequence)),
            this, SLOT(slotKeySequenceChanged(QKeySequence)));

    connect(m_addPositionButton, SIGNAL(clicked(bool)),
            this, SLOT(slotAddPositionClicked()));
    connect(m_addEfxButton, SIGNAL(clicked(bool)),
            this, SLOT(slotAddEFXClicked()));
    connect(m_addSceneButton, SIGNAL(clicked(bool)),
            this, SLOT(slotAddSceneClicked()));
    connect(m_addFxGroupButton, SIGNAL(clicked(bool)),
            this, SLOT(slotAddFixtureGroupClicked()));
    connect(m_removePresetButton, SIGNAL(clicked()),
            this, SLOT(slotRemovePresetClicked()));
    connect(m_moveUpPresetButton, SIGNAL(clicked()),
            this, SLOT(slotMoveUpPresetClicked()));
    connect(m_moveDownPresetButton, SIGNAL(clicked()),
            this, SLOT(slotMoveDownPresetClicked()));
    connect(m_presetNameEdit, SIGNAL(textEdited(QString const&)),
            this, SLOT(slotPresetNameEdited(QString const&)));
    connect(m_presetsTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotPresetSelectionChanged()));

    m_xyArea = new VCXYPadArea(this);
    //m_xyArea->setFixedSize(140, 140);
    m_xyArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_xyArea->setMode(Doc::Operate);
    m_presetLayout->addWidget(m_xyArea);
    connect(m_xyArea, SIGNAL(positionChanged(QPointF)),
            this, SLOT(slotXYPadPositionChanged(QPointF)));

    foreach (const VCXYPadPreset *preset, m_xypad->presets())
    {
        m_presetList.append(new VCXYPadPreset(*preset));
        if (preset->m_id > m_lastAssignedID)
            m_lastAssignedID = preset->m_id;
    }

    updatePresetsTree();

    QSettings settings;
    QVariant var = settings.value(SETTINGS_GEOMETRY);
    if (var.isValid() == true)
        restoreGeometry(var.toByteArray());
    AppUtil::ensureWidgetIsVisible(this);

    m_doc->masterTimer()->registerDMXSource(this);
}

VCXYPadProperties::~VCXYPadProperties()
{
    QSettings settings;
    settings.setValue(SETTINGS_GEOMETRY, saveGeometry());
    m_doc->masterTimer()->unregisterDMXSource(this);
    foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
    {
        if (!fader.isNull())
            fader->requestDelete();
    }
    m_fadersMap.clear();

    delete m_presetInputWidget;
}

/****************************************************************************
 * Fixtures page
 ****************************************************************************/

void VCXYPadProperties::fillFixturesTree()
{
    m_tree->clear();

    if (m_xypad->isFixtureGroupMode())
    {
        // In fixture group mode, show columns instead of individual fixtures
        FixtureGroup* group = m_doc->fixtureGroup(m_xypad->fixtureGroupID());
        if (group == nullptr)
        {
            m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
            return;
        }
        
        int gridWidth = group->size().width();
        int gridHeight = group->size().height();
        
        if (gridWidth <= 0 || gridHeight <= 0)
        {
            m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
            return;
        }
        
        // Group fixtures by column
        QMap<int, QList<VCXYPadFixture>> columnFixtures;
        foreach (const VCXYPadFixture& fxi, m_xypad->fixtures())
        {
            // Find which column this fixture belongs to
            for (int col = 0; col < gridWidth; col++)
            {
                for (int row = 0; row < gridHeight; row++)
                {
                    if (!m_xypad->isRowSelected(row))
                        continue;
                        
                    GroupHead head = group->head(QLCPoint(col, row));
                    if (head.isValid() && head.fxi == fxi.head().fxi && head.head == fxi.head().head)
                    {
                        columnFixtures[col].append(fxi);
                        break;
                    }
                }
            }
        }
        
        // Create tree items for each column
        for (int col = 0; col < gridWidth; col++)
        {
            int fixtureCount = columnFixtures[col].size();
            
            QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
            item->setText(KColumnFixture, QString("Column %1 (%2 fixtures)").arg(col + 1).arg(fixtureCount));
            item->setData(KColumnFixture, Qt::UserRole, col); // Store column index
            item->setData(KColumnFixture, Qt::UserRole + 1, true); // Mark as column item
            
            if (fixtureCount > 0)
            {
                // Use first fixture as representative for X/Y display
                VCXYPadFixture firstFxi = columnFixtures[col].first();
                item->setText(KColumnXAxis, firstFxi.xBrief());
                item->setText(KColumnYAxis, firstFxi.yBrief());
            }
            else
            {
                item->setText(KColumnXAxis, "");
                item->setText(KColumnYAxis, "");
            }
        }
        
        // Disable Add, Remove, Edit buttons in group mode
        m_addButton->setEnabled(false);
        m_removeButton->setEnabled(false);
        m_editButton->setEnabled(false);
    }
    else
    {
        // Normal mode - show individual fixtures
        QListIterator <VCXYPadFixture> it(m_xypad->fixtures());
        while (it.hasNext() == true)
            updateFixtureItem(new QTreeWidgetItem(m_tree), it.next());
        
        // Enable buttons in normal mode
        m_addButton->setEnabled(true);
        // Remove and Edit will be enabled/disabled by slotSelectionChanged
    }
    
    m_tree->setCurrentItem(m_tree->topLevelItem(0));
    m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void VCXYPadProperties::updateFixturesTree(VCXYPadFixture::DisplayMode mode)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = m_tree->topLevelItem(i);
        QVariant var(item->data(KColumnFixture, Qt::UserRole));
        VCXYPadFixture fx = VCXYPadFixture(m_doc, var);
        fx.setDisplayMode(mode);
        updateFixtureItem(item, fx);
    }
}

void VCXYPadProperties::updateFixtureItem(QTreeWidgetItem* item,
                                          const VCXYPadFixture& fxi)
{
    Q_ASSERT(item != NULL);

    item->setText(KColumnFixture, fxi.name());
    item->setText(KColumnXAxis, fxi.xBrief());
    item->setText(KColumnYAxis, fxi.yBrief());
    item->setData(KColumnFixture, Qt::UserRole, QVariant(fxi));
}

QList <VCXYPadFixture> VCXYPadProperties::selectedFixtures() const
{
    QListIterator <QTreeWidgetItem*> it(m_tree->selectedItems());
    QList <VCXYPadFixture> list;

    /* Put all selected fixtures to a list and return it */
    while (it.hasNext() == true)
        list << VCXYPadFixture(m_doc, it.next()->data(KColumnFixture, Qt::UserRole));

    return list;
}

QTreeWidgetItem* VCXYPadProperties::fixtureItem(const VCXYPadFixture& fxi)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it != NULL)
    {
        QVariant var((*it)->data(KColumnFixture, Qt::UserRole));
        VCXYPadFixture another(m_doc, var);
        if (fxi.head() == another.head())
            return *it;
        else
            ++it;
    }

    return NULL;
}

void VCXYPadProperties::removeFixtureItem(GroupHead const & head)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it != NULL)
    {
        QVariant var((*it)->data(KColumnFixture, Qt::UserRole));
        VCXYPadFixture fxi(m_doc, var);
        if (fxi.head() == head)
        {
            delete (*it);
            break;
        }

        ++it;
    }
}

void VCXYPadProperties::slotAddClicked()
{
    // Block adding fixtures in fixture group mode
    if (m_useFixtureGroupCheck->isChecked())
    {
        QMessageBox::information(this, tr("Fixture Group Mode"),
            tr("Cannot add individual fixtures in Fixture Group mode.\n"
               "Disable 'Use Fixture Group' to add fixtures manually."));
        return;
    }
    
    /* Put all fixtures already present into a list of fixtures that
       will be disabled in the fixture selection dialog */
    QList <GroupHead> disabled;
    QTreeWidgetItemIterator twit(m_tree);
    while (*twit != NULL)
    {
        QVariant var((*twit)->data(KColumnFixture, Qt::UserRole));
        VCXYPadFixture fxi(m_doc, var);
        disabled << fxi.head();
        ++twit;
    }

    /* Disable all fixtures that don't have pan OR tilt channels */
    QListIterator <Fixture*> fxit(m_doc->fixtures());
    while (fxit.hasNext() == true)
    {
        Fixture* fixture(fxit.next());
        Q_ASSERT(fixture != NULL);

        // If a channel with pan or tilt group exists, don't disable this fixture
        if (fixture->channel(QLCChannel::Pan) == QLCChannel::invalid() &&
            fixture->channel(QLCChannel::Tilt) == QLCChannel::invalid())
        {
            // Disable all fixtures without pan or tilt channels
            disabled << fixture->id();
        }
        else
        {
            QVector <QLCFixtureHead> const& heads = fixture->fixtureMode()->heads();
            for (int i = 0; i < heads.size(); ++i)
            {
                if (heads[i].channelNumber(QLCChannel::Pan, QLCChannel::MSB) == QLCChannel::invalid() &&
                    heads[i].channelNumber(QLCChannel::Tilt, QLCChannel::MSB) == QLCChannel::invalid() &&
                    heads[i].channelNumber(QLCChannel::Pan, QLCChannel::LSB) == QLCChannel::invalid() &&
                    heads[i].channelNumber(QLCChannel::Tilt, QLCChannel::LSB) == QLCChannel::invalid())
                {
                    // Disable heads without pan or tilt channels
                    disabled << GroupHead(fixture->id(), i);
                }
            }
        }
    }

    /* Get a list of new fixtures to add to the pad */
    QTreeWidgetItem* item = NULL;
    FixtureSelection fs(this, m_doc);
    fs.setMultiSelection(true);
    fs.setSelectionMode(FixtureSelection::Heads);
    fs.setDisabledHeads(disabled);
    if (fs.exec() == QDialog::Accepted)
    {
        QListIterator <GroupHead> it(fs.selectedHeads());
        while (it.hasNext() == true)
        {
            VCXYPadFixture fxi(m_doc);
            fxi.setHead(it.next());
            item = new QTreeWidgetItem(m_tree);
            updateFixtureItem(item, fxi);
        }
    }

    if (item != NULL)
        m_tree->setCurrentItem(item);

    m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void VCXYPadProperties::slotRemoveClicked()
{
    int r = QMessageBox::question(
                this, tr("Remove fixtures"),
                tr("Do you want to remove the selected fixtures?"),
                QMessageBox::Yes, QMessageBox::No);

    if (r == QMessageBox::Yes)
    {
        QListIterator <QTreeWidgetItem*> it(m_tree->selectedItems());
        while (it.hasNext() == true)
            delete it.next();
    }
}

void VCXYPadProperties::slotEditClicked()
{
    /* Get a list of selected fixtures */
    QList <VCXYPadFixture> list(selectedFixtures());

    /* Start editor */
    VCXYPadFixtureEditor editor(this, list);
    if (editor.exec() == QDialog::Accepted)
    {
        QListIterator <VCXYPadFixture> it(editor.fixtures());
        while (it.hasNext() == true)
        {
            VCXYPadFixture fxi(it.next());
            QTreeWidgetItem* item = fixtureItem(fxi);

            updateFixtureItem(item, fxi);
        }
        m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}

void VCXYPadProperties::slotSelectionChanged(QTreeWidgetItem* item)
{
    if (item == NULL)
    {
        m_removeButton->setEnabled(false);
        m_editButton->setEnabled(false);
    }
    else
    {
        m_removeButton->setEnabled(true);
        m_editButton->setEnabled(true);
    }
}

void VCXYPadProperties::slotPercentageRadioChecked()
{
    updateFixturesTree(VCXYPadFixture::Percentage);
}

void VCXYPadProperties::slotDegreesRadioChecked()
{
    updateFixturesTree(VCXYPadFixture::Degrees);
}

void VCXYPadProperties::slotDMXRadioChecked()
{
    updateFixturesTree(VCXYPadFixture::DMX);
}

/****************************************************************************
 * Input page
 ****************************************************************************/

void VCXYPadProperties::stopAutodetection(quint8 sourceId)
{
    if (sourceId != VCXYPad::panInputSourceId)
        m_panInputWidget->stopAutoDetection();
    if (sourceId != VCXYPad::panFineInputSourceId)
        m_panFineInputWidget->stopAutoDetection();
    if (sourceId != VCXYPad::tiltInputSourceId)
        m_tiltInputWidget->stopAutoDetection();
    if (sourceId != VCXYPad::tiltFineInputSourceId)
        m_tiltFineInputWidget->stopAutoDetection();
    if (sourceId != VCXYPad::widthInputSourceId)
        m_widthInputWidget->stopAutoDetection();
    if (sourceId != VCXYPad::heightInputSourceId)
        m_heightInputWidget->stopAutoDetection();
}

void VCXYPadProperties::slotPanAutoDetectToggled(bool toggled)
{
    if (toggled == true)
        stopAutodetection(VCXYPad::panInputSourceId);
}

void VCXYPadProperties::slotPanInputValueChanged(quint32 uni, quint32 ch)
{
    QSharedPointer<QLCInputSource> tmpSource = m_panInputWidget->inputSource();
    if (tmpSource->universe() != uni || tmpSource->channel() != ch)
        m_tiltInputWidget->setInputSource(
                    QSharedPointer<QLCInputSource>(new QLCInputSource(uni, ch)));
}

void VCXYPadProperties::slotPanFineAutoDetectToggled(bool toggled)
{
    if (toggled == true)
        stopAutodetection(VCXYPad::panFineInputSourceId);
}

void VCXYPadProperties::slotPanFineInputValueChanged(quint32 uni, quint32 ch)
{
    QSharedPointer<QLCInputSource> tmpSource = m_panFineInputWidget->inputSource();
    if (tmpSource->universe() != uni || tmpSource->channel() != ch)
        m_tiltFineInputWidget->setInputSource(
            QSharedPointer<QLCInputSource>(new QLCInputSource(uni, ch)));
}

void VCXYPadProperties::slotTiltAutoDetectToggled(bool toggled)
{
    if (toggled == true)
        stopAutodetection(VCXYPad::tiltInputSourceId);
}

void VCXYPadProperties::slotTiltInputValueChanged(quint32 uni, quint32 ch)
{
    QSharedPointer<QLCInputSource> tmpSource = m_tiltInputWidget->inputSource();
    if (tmpSource->universe() != uni || tmpSource->channel() != ch)
        m_panInputWidget->setInputSource(
                    QSharedPointer<QLCInputSource>(new QLCInputSource(uni, ch)));
}

void VCXYPadProperties::slotTiltFineAutoDetectToggled(bool toggled)
{
    if (toggled == true)
        stopAutodetection(VCXYPad::tiltFineInputSourceId);
}

void VCXYPadProperties::slotTiltFineInputValueChanged(quint32 uni, quint32 ch)
{
    QSharedPointer<QLCInputSource> tmpSource = m_tiltFineInputWidget->inputSource();
    if (tmpSource->universe() != uni || tmpSource->channel() != ch)
        m_panFineInputWidget->setInputSource(
            QSharedPointer<QLCInputSource>(new QLCInputSource(uni, ch)));
}

void VCXYPadProperties::writeDMX(MasterTimer *timer, QList<Universe *> universes)
{
    Q_UNUSED(timer);

    if (m_tab->currentIndex() != 2 || m_xyArea->hasPositionChanged() == false)
        return;

    //qDebug() << Q_FUNC_INFO;

    // This call also resets the m_changed flag in m_area
    QPointF pt = m_xyArea->position();

    /* Scale XY coordinate values to 0.0 - 1.0 */
    qreal x = SCALE(pt.x(), qreal(0), qreal(256), qreal(0), qreal(1));
    qreal y = SCALE(pt.y(), qreal(0), qreal(256), qreal(0), qreal(1));

    if (m_YInvertedRadio->isChecked())
        y = qreal(1) - y;

    QTreeWidgetItemIterator it(m_tree);
    while (*it != NULL)
    {
        QVariant var((*it)->data(KColumnFixture, Qt::UserRole));
        VCXYPadFixture fixture(m_doc, var);
        fixture.arm();
        quint32 universe = fixture.universe();
        if (universe == Universe::invalid())
            continue;

        QSharedPointer<GenericFader> fader = m_fadersMap.value(universe, QSharedPointer<GenericFader>());
        if (fader.isNull())
        {
            fader = universes[universe]->requestFader();
            m_fadersMap[universe] = fader;
        }
        fixture.writeDMX(x, y, fader, universes[universe]);
        fixture.disarm();
        ++it;
    }
}

/********************************************************************
 * Presets
 ********************************************************************/

void VCXYPadProperties::updatePresetsTree()
{
    m_presetsTree->blockSignals(true);
    m_presetsTree->clear();

    for (int i = 0; i < m_presetList.count(); i++)
    {
        VCXYPadPreset *preset = m_presetList.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem(m_presetsTree);
        item->setData(0, Qt::UserRole, preset->m_id);
        item->setText(0, preset->m_name);
        if (preset->m_type == VCXYPadPreset::EFX)
            item->setIcon(0, QIcon(":/efx.png"));
        else if (preset->m_type == VCXYPadPreset::Scene)
            item->setIcon(0, QIcon(":/scene.png"));
        else if (preset->m_type == VCXYPadPreset::Position)
            item->setIcon(0, QIcon(":/xypad.png"));
        else if (preset->m_type == VCXYPadPreset::FixtureGroup)
            item->setIcon(0, QIcon(":/group.png"));
    }
    m_presetsTree->resizeColumnToContents(0);
    m_presetsTree->blockSignals(false);
}

void VCXYPadProperties::selectItemOnPresetsTree(quint8 presetId)
{
    m_presetsTree->blockSignals(true);

    for (int i = 0; i < m_presetsTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* treeItem = m_presetsTree->topLevelItem(i);
        if (treeItem->data(0, Qt::UserRole).toUInt() == presetId)
        {
            treeItem->setSelected(true);
            break;
        }
    }

    m_presetsTree->blockSignals(false);
}

void VCXYPadProperties::updateTreeItem(const VCXYPadPreset &preset)
{
    m_presetsTree->blockSignals(true);

    for (int i = 0; i < m_presetsTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* treeItem = m_presetsTree->topLevelItem(i);
        if (treeItem->data(0, Qt::UserRole).toUInt() == preset.m_id)
        {
            treeItem->setText(0, preset.m_name);
            m_presetsTree->resizeColumnToContents(0);
            m_presetsTree->blockSignals(false);
            return;
        }
    }
    Q_ASSERT(false);
}

VCXYPadPreset *VCXYPadProperties::getSelectedPreset()
{
    if (m_presetsTree->selectedItems().isEmpty())
        return NULL;

    QTreeWidgetItem* item = m_presetsTree->selectedItems().first();
    if (item != NULL)
    {
        quint8 presetID = item->data(0, Qt::UserRole).toUInt();
        foreach (VCXYPadPreset* preset, m_presetList)
        {
            if (preset->m_id == presetID)
                return preset;
        }
    }

    Q_ASSERT(false);
    return NULL;
}

void VCXYPadProperties::removePreset(quint8 id)
{
    for (int i = 0; i < m_presetList.count(); i++)
    {
        if (m_presetList.at(i)->m_id == id)
        {
            m_presetList.removeAt(i);
            return;
        }
    }
}

quint8 VCXYPadProperties::moveUpPreset(quint8 id)
{
    for (int i = 0; i < m_presetList.count(); i++)
    {
        if (m_presetList.at(i)->m_id == id)
        {
            if (i > 0)
            {
                //change order on hash preset structure.
                //presets are saved in hash and sort on id is used to create the preset list.
                //So swapping id change order every time that preset list is created (restore, dialog open, ...).
                quint8 dstPosID = m_presetList.at(i-1)->m_id;
                quint8 srcPosID = m_presetList.at(i)->m_id;

                m_presetList.at(i-1)->m_id = srcPosID;
                m_presetList.at(i)->m_id = dstPosID;

                //change order on current preset list...
                m_presetList.move(i, i-1);

                return dstPosID;
            }

            return id;
        }
    }

    return id;
}

quint8 VCXYPadProperties::moveDownPreset(quint8 id)
{
    for (int i = 0; i < m_presetList.count(); i++)
    {
        if (m_presetList.at(i)->m_id == id)
        {
            if (i < m_presetList.count() - 1)
            {
                //change order on hash preset structure.
                //presets are saved in hash and sort on id is used to create the preset list.
                //So swapping id change order every time that preset list is created (restore, dialog open, ...).
                quint8 dstPosID = m_presetList.at(i+1)->m_id;
                quint8 srcPosID = m_presetList.at(i)->m_id;

                m_presetList.at(i+1)->m_id = srcPosID;
                m_presetList.at(i)->m_id = dstPosID;

                //change order on current preset list...
                m_presetList.move(i, i+1);

                return dstPosID;
            }
            return id;
        }
    }

    return id;
}

void VCXYPadProperties::slotAddPositionClicked()
{
    VCXYPadPreset *newPreset = new VCXYPadPreset(++m_lastAssignedID);
    newPreset->m_type = VCXYPadPreset::Position;
    newPreset->m_dmxPos = m_xyArea->position();
    newPreset->m_name = QString("X:%1 - Y:%2").arg((int)newPreset->m_dmxPos.x()).arg((int)newPreset->m_dmxPos.y());
    m_presetList.append(newPreset);
    updatePresetsTree();
    selectItemOnPresetsTree(newPreset->m_id);
}

void VCXYPadProperties::slotAddEFXClicked()
{
    FunctionSelection fs(this, m_doc);
    fs.setMultiSelection(false);
    fs.setFilter(Function::EFXType, true);
    QList <quint32> ids;
    foreach (VCXYPadPreset *preset, m_presetList)
    {
        if (preset->m_type == VCXYPadPreset::EFX)
            ids.append(preset->m_funcID);
    }

    if (fs.exec() == QDialog::Accepted && fs.selection().size() > 0)
    {
        quint32 fID = fs.selection().first();
        Function *f = m_doc->function(fID);
        if (f == NULL || f->type() != Function::EFXType)
            return;
        VCXYPadPreset *newPreset = new VCXYPadPreset(++m_lastAssignedID);
        newPreset->m_type = VCXYPadPreset::EFX;
        newPreset->m_funcID = fID;
        newPreset->m_name = f->name();
        m_presetList.append(newPreset);
        updatePresetsTree();
        selectItemOnPresetsTree(newPreset->m_id);
    }
}

void VCXYPadProperties::slotAddSceneClicked()
{
    FunctionSelection fs(this, m_doc);
    fs.setMultiSelection(false);
    fs.setFilter(Function::SceneType, true);
    QList <quint32> ids;
    foreach (VCXYPadPreset *preset, m_presetList)
    {
        if (preset->m_type == VCXYPadPreset::Scene)
            ids.append(preset->m_funcID);
    }

    if (fs.exec() == QDialog::Accepted && fs.selection().size() > 0)
    {
        quint32 fID = fs.selection().first();
        Function *f = m_doc->function(fID);
        if (f == NULL || f->type() != Function::SceneType)
            return;
        Scene *scene = qobject_cast<Scene*>(f);
        bool panTiltFound = false;
        foreach (SceneValue scv, scene->values())
        {
            Fixture *fixture = m_doc->fixture(scv.fxi);
            if (fixture == NULL)
                continue;
            const QLCChannel *ch = fixture->channel(scv.channel);
            if (ch == NULL)
                continue;
            if (ch->group() == QLCChannel::Pan || ch->group() == QLCChannel::Tilt)
            {
                panTiltFound = true;
                break;
            }
        }
        if (panTiltFound == false)
        {
            QMessageBox::critical(this, tr("Error"),
                                  tr("The selected Scene does not include any Pan or Tilt channel.\n"
                                     "Please select one with such channels."),
                                  QMessageBox::Close);
            return;
        }
        VCXYPadPreset *newPreset = new VCXYPadPreset(++m_lastAssignedID);
        newPreset->m_type = VCXYPadPreset::Scene;
        newPreset->m_funcID = fID;
        newPreset->m_name = f->name();
        m_presetList.append(newPreset);
        updatePresetsTree();
        selectItemOnPresetsTree(newPreset->m_id);
    }
}

void VCXYPadProperties::slotAddFixtureGroupClicked()
{
    QList <GroupHead> enabled;
    QList <GroupHead> disabled;

    QTreeWidgetItemIterator it(m_tree);
    while (*it != NULL)
    {
        QVariant var((*it)->data(KColumnFixture, Qt::UserRole));
        VCXYPadFixture fxi(m_doc, var);
        enabled << fxi.head();
        ++it;
    }

    foreach (Fixture *fx, m_doc->fixtures())
    {
        for (int i = 0; i < fx->heads(); i++)
        {
            GroupHead gh(fx->id(), i);
            if (enabled.contains(gh) == false)
                disabled << gh;
        }
    }

    FixtureSelection fs(this, m_doc);
    fs.setMultiSelection(true);
    fs.setSelectionMode(FixtureSelection::Heads);
    fs.setDisabledHeads(disabled);
    if (fs.exec() == QDialog::Accepted)
    {
        QList<GroupHead> selectedGH = fs.selectedHeads();
        if (selectedGH.isEmpty())
        {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Please select at least one fixture or head to create this type of preset!"),
                                  QMessageBox::Close);
            return;
        }

        VCXYPadPreset *newPreset = new VCXYPadPreset(++m_lastAssignedID);
        newPreset->m_type = VCXYPadPreset::FixtureGroup;
        newPreset->m_name = tr("Fixture Group");
        newPreset->setFixtureGroup(selectedGH);
        m_presetList.append(newPreset);
        updatePresetsTree();
        selectItemOnPresetsTree(newPreset->m_id);
    }
}

void VCXYPadProperties::slotRemovePresetClicked()
{
    if (m_presetsTree->selectedItems().isEmpty())
        return;
    QTreeWidgetItem *selItem = m_presetsTree->selectedItems().first();
    quint8 ctlID = selItem->data(0, Qt::UserRole).toUInt();
    removePreset(ctlID);
    updatePresetsTree();
}

void VCXYPadProperties::slotMoveUpPresetClicked()
{
    if (m_presetsTree->selectedItems().isEmpty())
        return;
    QTreeWidgetItem *selItem = m_presetsTree->selectedItems().first();
    quint8 ctlID = selItem->data(0, Qt::UserRole).toUInt();
    quint8 newID = moveUpPreset(ctlID);
    updatePresetsTree();

    //select item on new position. User can make multiple move up/down without need to select item every time.
    selectItemOnPresetsTree(newID);
}

void VCXYPadProperties::slotMoveDownPresetClicked()
{
    if (m_presetsTree->selectedItems().isEmpty())
        return;
    QTreeWidgetItem *selItem = m_presetsTree->selectedItems().first();
    quint8 ctlID = selItem->data(0, Qt::UserRole).toUInt();
    quint8 newID =moveDownPreset(ctlID);
    updatePresetsTree();

    //select item on new position. User can make multiple move up/down without need to select item every time.
    selectItemOnPresetsTree(newID);
}

void VCXYPadProperties::slotPresetNameEdited(const QString &newName)
{
    VCXYPadPreset* preset = getSelectedPreset();

    if (preset != NULL)
    {
        preset->m_name = newName;

        updateTreeItem(*preset);
    }
}

void VCXYPadProperties::slotPresetSelectionChanged()
{
    VCXYPadPreset *preset = getSelectedPreset();

    if (preset != NULL)
    {
        m_presetNameEdit->setText(preset->m_name);
        m_presetInputWidget->setInputSource(preset->m_inputSource);
        m_presetInputWidget->setKeySequence(preset->m_keySequence.toString(QKeySequence::NativeText));
        if (preset->m_type == VCXYPadPreset::EFX)
        {
            Function *f = m_doc->function(preset->functionID());
            if (f == NULL || f->type() != Function::EFXType)
                return;
            EFX *efx = qobject_cast<EFX*>(f);
            QPolygonF polygon;
            efx->preview(polygon);

            QVector <QPolygonF> fixturePoints;
            efx->previewFixtures(fixturePoints);

            m_xyArea->enableEFXPreview(true);
            m_xyArea->setEnabled(false);
            m_xyArea->setEFXPolygons(polygon, fixturePoints);
            m_xyArea->setEFXInterval(efx->duration());
        }
        else if (preset->m_type == VCXYPadPreset::Position)
        {
            m_xyArea->enableEFXPreview(false);
            m_xyArea->setEnabled(true);
            m_xyArea->blockSignals(true);
            m_xyArea->setPosition(preset->m_dmxPos);
            m_xyArea->repaint();
            m_xyArea->blockSignals(false);
        }
        else if (preset->m_type == VCXYPadPreset::Scene)
        {
            m_xyArea->enableEFXPreview(false);
            m_xyArea->setEnabled(false);
        }
    }
}

void VCXYPadProperties::slotXYPadPositionChanged(const QPointF &pt)
{
    VCXYPadPreset *preset = getSelectedPreset();

    if (preset != NULL)
    {
        preset->m_dmxPos = pt;
        if (preset->m_type == VCXYPadPreset::Position &&
            preset->m_name.startsWith("X:"))
        {
            preset->m_name = QString("X:%1 - Y:%2").arg((int)pt.x()).arg((int)pt.y());
            m_presetNameEdit->blockSignals(true);
            m_presetNameEdit->setText(preset->m_name);
            m_presetNameEdit->blockSignals(false);
        }
        updateTreeItem(*preset);
    }
}

void VCXYPadProperties::slotInputValueChanged(quint32 universe, quint32 channel)
{
    Q_UNUSED(universe);
    Q_UNUSED(channel);

    VCXYPadPreset *preset = getSelectedPreset();

    if (preset != NULL)
        preset->m_inputSource = m_presetInputWidget->inputSource();
}

void VCXYPadProperties::slotKeySequenceChanged(QKeySequence key)
{
    VCXYPadPreset *preset = getSelectedPreset();

    if (preset != NULL)
        preset->m_keySequence = key;
}

/****************************************************************************
 * Fixture Group
 ****************************************************************************/

void VCXYPadProperties::updateFixtureGroupCombo()
{
    m_fixtureGroupCombo->clear();
    
    foreach (FixtureGroup* grp, m_doc->fixtureGroups())
    {
        m_fixtureGroupCombo->addItem(grp->name(), grp->id());
    }
}

void VCXYPadProperties::updateRowSelection()
{
    // Delete existing checkboxes
    foreach (QCheckBox* cb, m_rowCheckboxes)
    {
        m_rowSelectionLayout->removeWidget(cb);
        delete cb;
    }
    m_rowCheckboxes.clear();
    
    // Remove placeholder if it exists
    if (m_rowSelectionPlaceholder != nullptr)
    {
        m_rowSelectionLayout->removeWidget(m_rowSelectionPlaceholder);
        delete m_rowSelectionPlaceholder;
        m_rowSelectionPlaceholder = nullptr;
    }
    
    if (!m_useFixtureGroupCheck->isChecked() || m_fixtureGroupCombo->currentIndex() < 0)
    {
        // Add placeholder back
        m_rowSelectionPlaceholder = new QLabel(tr("Select a fixture group to enable row filtering"));
        m_rowSelectionLayout->addWidget(m_rowSelectionPlaceholder);
        m_rowSelectionGroup->setEnabled(false);
        return;
    }
    
    quint32 groupId = m_fixtureGroupCombo->currentData().toUInt();
    FixtureGroup* group = m_doc->fixtureGroup(groupId);
    if (group == nullptr)
    {
        m_rowSelectionPlaceholder = new QLabel(tr("Select a fixture group to enable row filtering"));
        m_rowSelectionLayout->addWidget(m_rowSelectionPlaceholder);
        m_rowSelectionGroup->setEnabled(false);
        return;
    }
    
    int gridHeight = group->size().height();
    if (gridHeight <= 0)
    {
        m_rowSelectionPlaceholder = new QLabel(tr("Fixture group is empty"));
        m_rowSelectionLayout->addWidget(m_rowSelectionPlaceholder);
        m_rowSelectionGroup->setEnabled(false);
        return;
    }
    
    m_rowSelectionGroup->setEnabled(true);
    
    // Create checkbox for each row
    QList<int> selectedRows = m_xypad->selectedRows();
    for (int row = 0; row < gridHeight; row++)
    {
        QCheckBox* cb = new QCheckBox(tr("Row %1").arg(row + 1));
        cb->setProperty("row_index", row);
        
        // Check if this row is selected (or all if empty)
        if (selectedRows.isEmpty() || selectedRows.contains(row))
            cb->setChecked(true);
        
        connect(cb, SIGNAL(toggled(bool)),
                this, SLOT(slotRowSelectionChanged()));
        
        m_rowSelectionLayout->addWidget(cb);
        m_rowCheckboxes.append(cb);
    }
}

void VCXYPadProperties::slotUseFixtureGroupToggled(bool checked)
{
    m_fixtureGroupCombo->setEnabled(checked);
    
    if (checked && m_fixtureGroupCombo->count() > 0)
    {
        // Update row selection UI
        updateRowSelection();
        
        // Clear existing fixtures
        m_tree->clear();
        
        // Validate combo data
        if (!m_fixtureGroupCombo->currentData().isValid())
            return;
        
        quint32 groupId = m_fixtureGroupCombo->currentData().toUInt();
        if (groupId == FixtureGroup::invalidId())
            return;
        
        // Create fixtures for the selected group
        FixtureGroup* group = m_doc->fixtureGroup(groupId);
        if (group != nullptr)
        {
            int gridWidth = group->size().width();
            int gridHeight = group->size().height();
            
            if (gridWidth <= 0 || gridHeight <= 0)
            {
                fillFixturesTree();
                return;
            }
            
            // Create VCXYPadFixture for each fixture in the group (respecting row filter)
            for (int col = 0; col < gridWidth; col++)
            {
                for (int row = 0; row < gridHeight; row++)
                {
                    // Check if this row is selected
                    if (!m_xypad->isRowSelected(row))
                        continue;
                    
                    GroupHead head = group->head(QLCPoint(col, row));
                    if (head.isValid())
                    {
                        VCXYPadFixture fxi(m_doc);
                        fxi.setHead(head);
                        
                        QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
                        updateFixtureItem(item, fxi);
                    }
                }
            }
        }
        
        fillFixturesTree();
    }
    else
    {
        m_fixtureGroupCombo->setCurrentIndex(-1);
        updateRowSelection();
        
        // Clear fixtures when disabling group mode
        m_tree->clear();
        fillFixturesTree();
    }
}

void VCXYPadProperties::slotFixtureGroupChanged(int index)
{
    if (!m_useFixtureGroupCheck->isChecked() || index < 0)
        return;
    
    updateRowSelection();
    
    // Clear existing fixtures
    m_tree->clear();
    
    // Validate combo data
    if (!m_fixtureGroupCombo->itemData(index).isValid())
        return;
    
    quint32 groupId = m_fixtureGroupCombo->itemData(index).toUInt();
    if (groupId == FixtureGroup::invalidId())
        return;
    
    // Recreate fixtures for new group
    FixtureGroup* group = m_doc->fixtureGroup(groupId);
    if (group != nullptr)
    {
        int gridWidth = group->size().width();
        int gridHeight = group->size().height();
        
        if (gridWidth <= 0 || gridHeight <= 0)
        {
            fillFixturesTree();
            return;
        }
        
        // Create VCXYPadFixture for each fixture in the group (respecting row filter)
        for (int col = 0; col < gridWidth; col++)
        {
            for (int row = 0; row < gridHeight; row++)
            {
                // Check if this row is selected
                if (!m_xypad->isRowSelected(row))
                    continue;
                
                GroupHead head = group->head(QLCPoint(col, row));
                if (head.isValid())
                {
                    VCXYPadFixture fxi(m_doc);
                    fxi.setHead(head);
                    
                    QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
                    updateFixtureItem(item, fxi);
                }
            }
        }
    }
    
    fillFixturesTree();
}

void VCXYPadProperties::slotRowSelectionChanged()
{
    // Prevent recursion
    static bool updating = false;
    if (updating)
        return;
    
    updating = true;
    
    // Collect selected rows from checkboxes
    QList<int> selectedRows;
    foreach (QCheckBox *cb, m_rowCheckboxes)
    {
        if (cb->isChecked())
        {
            int rowIndex = cb->property("row_index").toInt();
            selectedRows.append(rowIndex);
        }
    }
    
    // Ensure at least one row is selected
    if (selectedRows.isEmpty())
    {
        // Re-check the first checkbox to prevent empty selection
        if (!m_rowCheckboxes.isEmpty())
        {
            m_rowCheckboxes.first()->blockSignals(true);
            m_rowCheckboxes.first()->setChecked(true);
            m_rowCheckboxes.first()->blockSignals(false);
            selectedRows.append(0);
        }
    }
    
    m_xypad->setSelectedRows(selectedRows);
    
    // Clear tree
    m_tree->clear();
    
    // Recreate fixtures for currently selected group
    if (m_fixtureGroupCombo->currentIndex() >= 0)
    {
        quint32 groupId = m_fixtureGroupCombo->currentData().toUInt();
        FixtureGroup* group = m_doc->fixtureGroup(groupId);
        if (group != nullptr)
        {
            int gridWidth = group->size().width();
            int gridHeight = group->size().height();
            
            if (gridWidth > 0 && gridHeight > 0)
            {
                // Create VCXYPadFixture for each fixture (respecting row filter)
                for (int col = 0; col < gridWidth; col++)
                {
                    for (int row = 0; row < gridHeight; row++)
                    {
                        // Check if this row is selected
                        if (!m_xypad->isRowSelected(row))
                            continue;
                        
                        GroupHead head = group->head(QLCPoint(col, row));
                        if (head.isValid())
                        {
                            VCXYPadFixture fxi(m_doc);
                            fxi.setHead(head);
                            
                            QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
                            updateFixtureItem(item, fxi);
                        }
                    }
                }
            }
        }
    }
    
    fillFixturesTree();
    
    updating = false;
}

void VCXYPadProperties::slotFixtureGroupRemoved(quint32 id)
{
    // Find and remove the deleted group from combo
    for (int i = 0; i < m_fixtureGroupCombo->count(); i++)
    {
        if (m_fixtureGroupCombo->itemData(i).toUInt() == id)
        {
            m_fixtureGroupCombo->removeItem(i);
            break;
        }
    }
    
    // If current group was removed, uncheck
    if (m_xypad->fixtureGroupID() == id)
    {
        m_useFixtureGroupCheck->setChecked(false);
    }
}

/****************************************************************************
 * OK/Cancel
 ****************************************************************************/

void VCXYPadProperties::accept()
{
    m_xypad->clearFixtures();
    m_xypad->setCaption(m_nameEdit->text());
    m_xypad->setInputSource(m_panInputWidget->inputSource(), VCXYPad::panInputSourceId);
    m_xypad->setInputSource(m_panFineInputWidget->inputSource(), VCXYPad::panFineInputSourceId);
    m_xypad->setInputSource(m_tiltInputWidget->inputSource(), VCXYPad::tiltInputSourceId);
    m_xypad->setInputSource(m_tiltFineInputWidget->inputSource(), VCXYPad::tiltFineInputSourceId);
    m_xypad->setInputSource(m_widthInputWidget->inputSource(), VCXYPad::widthInputSourceId);
    m_xypad->setInputSource(m_heightInputWidget->inputSource(), VCXYPad::heightInputSourceId);
    if (m_YNormalRadio->isChecked())
        m_xypad->setInvertedAppearance(false);
    else
        m_xypad->setInvertedAppearance(true);

    // Save fixture group mode settings
    if (m_useFixtureGroupCheck->isChecked() && m_fixtureGroupCombo->currentIndex() >= 0)
    {
        m_xypad->setFixtureGroupID(m_fixtureGroupCombo->currentData().toUInt());
        
        // Collect selected rows
        QList<int> selectedRows;
        foreach (QCheckBox* cb, m_rowCheckboxes)
        {
            if (cb->isChecked())
            {
                int rowIndex = cb->property("row_index").toInt();
                selectedRows.append(rowIndex);
            }
        }
        m_xypad->setSelectedRows(selectedRows);
    }
    else
    {
        m_xypad->setFixtureGroupID(FixtureGroup::invalidId());
        m_xypad->setSelectedRows(QList<int>());
    }

    QTreeWidgetItemIterator it(m_tree);
    while (*it != NULL)
    {
        QVariant var((*it)->data(KColumnFixture, Qt::UserRole));
        m_xypad->appendFixture(VCXYPadFixture(m_doc, var));
        ++it;
    }

    /* Controls */
    m_xypad->resetPresets();
    for (int i = 0; i < m_presetList.count(); i++)
        m_xypad->addPreset(*m_presetList.at(i));

    QDialog::accept();
}

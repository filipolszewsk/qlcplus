/*
  Q Light Controller
  efxeditor.cpp

  Copyright (c) Heikki Junnila

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
#include <QPaintEvent>
#include <QSettings>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPainter>
#include <QLabel>
#include <QDebug>
#include <QPen>

#include "fixtureselection.h"
#include "speeddialwidget.h"
#include "efxpreviewarea.h"
#include "fixturegroup.h"
#include "efxeditor.h"
#include "fixture.h"
#include "doc.h"

#define SETTINGS_GEOMETRY "efxeditor/geometry"

#define KColumnNumber  0
#define KColumnName    1
#define KColumnMode    2
#define KColumnReverse 3
#define KColumnStartOffset 4

#define PROPERTY_FIXTURE "fixture"
#define UI_STATE_TAB_INDEX "tabIndex"
#define UI_STATE_SHOW_DIAL "showDial"

#define KTabGeneral 0
#define KTabMovement 1

/*****************************************************************************
 * Initialization
 *****************************************************************************/

EFXEditor::EFXEditor(QWidget* parent, EFX* efx, Doc* doc)
    : QWidget(parent)
    , m_doc(doc)
    , m_efx(efx)
    , m_previewArea(NULL)
    , m_points(NULL)
    , m_speedDials(NULL)
{
    Q_ASSERT(doc != NULL);
    Q_ASSERT(efx != NULL);

    setupUi(this);

    connect(m_speedDial, SIGNAL(toggled(bool)),
            this, SLOT(slotSpeedDialToggle(bool)));

    initGeneralPage();
    initMovementPage();

    QVariant tabIndex = efx->uiStateValue(UI_STATE_TAB_INDEX);
    if (tabIndex.isNull())
        m_tab->setCurrentIndex(0);
    else
        m_tab->setCurrentIndex(tabIndex.toInt());

    /* Tab widget */
    connect(m_tab, SIGNAL(currentChanged(int)),
            this, SLOT(slotTabChanged(int)));

    // Used for UI parameter changes
    m_testTimer.setSingleShot(true);
    m_testTimer.setInterval(500);
    connect(&m_testTimer, SIGNAL(timeout()), this, SLOT(slotRestartTest()));
    connect(m_doc, SIGNAL(modeChanged(Doc::Mode)), this, SLOT(slotModeChanged(Doc::Mode)));

    updateSpeedDials();

    QVariant showDial = efx->uiStateValue(UI_STATE_SHOW_DIAL);
    if (showDial.isNull() == false && showDial.toBool() == true)
        m_speedDial->setChecked(true);

    // Set focus to the editor
    m_nameEdit->setFocus();
}

EFXEditor::~EFXEditor()
{
    if (m_testButton->isChecked() == true)
        m_efx->stopAndWait();
}

void EFXEditor::stopTest()
{
    if (m_testButton->isChecked() == true)
        m_testButton->click();
}

void EFXEditor::slotFunctionManagerActive(bool active)
{
    if (active == true)
    {
        updateSpeedDials();
    }
    else
    {
        if (m_speedDials != NULL)
            m_speedDials->deleteLater();
        m_speedDials = NULL;
    }
}

void EFXEditor::initGeneralPage()
{
    // Doc
    connect(m_doc, SIGNAL(fixtureRemoved(quint32)), this, SLOT(slotFixtureRemoved()));
    connect(m_doc, SIGNAL(fixtureChanged(quint32)), this, SLOT(slotFixtureChanged()));
    connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)), this, SLOT(slotFixtureGroupRemoved(quint32)));

    /* Set the EFX's name to the name field */
    m_nameEdit->setText(m_efx->name());
    m_nameEdit->setSelection(0, m_nameEdit->text().length());

    /* Put all of the EFX's fixtures to the tree view */
    updateFixtureTree();

    /* Set propagation mode */
    if (m_efx->propagationMode() == EFX::Serial)
        m_serialRadio->setChecked(true);
    else if (m_efx->propagationMode() == EFX::Asymmetric)
        m_asymmetricRadio->setChecked(true);
    else
        m_parallelRadio->setChecked(true);

    /* Fixture Group Mode */
    m_useFixtureGroupCheck->setChecked(m_efx->isFixtureGroupMode());
    updateFixtureGroupCombo();
    if (m_efx->isFixtureGroupMode())
    {
        m_fixtureGroupCombo->setCurrentIndex(
            m_fixtureGroupCombo->findData(m_efx->fixtureGroupID()));
    }
    
    /* Offset Direction and Step */
    m_offsetDirectionCombo->setCurrentIndex((int)m_efx->offsetDirection());
    m_offsetStepSpin->setValue(m_efx->offsetStep());
    m_wingsSpin->setValue(m_efx->wings());
    m_offsetDirectionCombo->setEnabled(m_efx->isFixtureGroupMode());
    m_offsetStepSpin->setEnabled(m_efx->isFixtureGroupMode());
    m_wingsSpin->setEnabled(m_efx->isFixtureGroupMode());
    
    /* Initialize Row Selection checkboxes */
    updateRowSelection();

    /* Disable test button if we're in operate mode */
    if (m_doc->mode() == Doc::Operate)
        m_testButton->setEnabled(false);

    connect(m_nameEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(slotNameEdited(const QString&)));

    connect(m_tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(slotFixtureItemChanged(QTreeWidgetItem*,int)));

    connect(m_addFixtureButton, SIGNAL(clicked()),
            this, SLOT(slotAddFixtureClicked()));
    connect(m_removeFixtureButton, SIGNAL(clicked()),
            this, SLOT(slotRemoveFixtureClicked()));

    connect(m_raiseFixtureButton, SIGNAL(clicked()),
            this, SLOT(slotRaiseFixtureClicked()));
    connect(m_lowerFixtureButton, SIGNAL(clicked()),
            this, SLOT(slotLowerFixtureClicked()));

    connect(m_parallelRadio, SIGNAL(toggled(bool)),
            this, SLOT(slotParallelRadioToggled(bool)));
    connect(m_serialRadio, SIGNAL(toggled(bool)),
            this, SLOT(slotSerialRadioToggled(bool)));
    connect(m_asymmetricRadio, SIGNAL(toggled(bool)),
            this, SLOT(slotAsymmetricRadioToggled(bool)));

    connect(m_useFixtureGroupCheck, SIGNAL(toggled(bool)),
            this, SLOT(slotUseFixtureGroupToggled(bool)));
    connect(m_fixtureGroupCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotFixtureGroupChanged(int)));
    connect(m_offsetDirectionCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotOffsetDirectionChanged(int)));
    connect(m_offsetStepSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotOffsetStepChanged(int)));
    connect(m_wingsSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotWingsChanged(int)));

    // Test slots
    connect(m_testButton, SIGNAL(clicked()),
            this, SLOT(slotTestClicked()));
    connect(m_raiseFixtureButton, SIGNAL(clicked()),
            this, SLOT(slotRestartTest()));
    connect(m_lowerFixtureButton, SIGNAL(clicked()),
            this, SLOT(slotRestartTest()));
    connect(m_parallelRadio, SIGNAL(toggled(bool)),
            this, SLOT(slotRestartTest()));
    connect(m_serialRadio, SIGNAL(toggled(bool)),
            this, SLOT(slotRestartTest()));
    connect(m_asymmetricRadio, SIGNAL(toggled(bool)),
            this, SLOT(slotRestartTest()));
}

void EFXEditor::initMovementPage()
{
    new QHBoxLayout(m_previewFrame);
    m_previewArea = new EFXPreviewArea(m_previewFrame);
    m_previewFrame->layout()->setContentsMargins(0, 0, 0, 0);
    m_previewFrame->layout()->addWidget(m_previewArea);

    /* Get supported algorithms and fill the algorithm combo with them */
    m_algorithmCombo->addItems(EFX::algorithmList());

    QString algo(EFX::algorithmToString(m_efx->algorithm()));
    int algoIndex = 0;
    /* Select the EFX's algorithm from the algorithm combo */
    for (int i = 0; i < m_algorithmCombo->count(); i++)
    {
        if (m_algorithmCombo->itemText(i) == algo)
        {
            m_algorithmCombo->setCurrentIndex(i);
            algoIndex = i;
            break;
        }
    }

    /* Causes the EFX function to update the preview point array */
    slotAlgorithmSelected(algoIndex);

    /* Get the algorithm parameters */
    m_widthSpin->setValue(m_efx->width());
    m_heightSpin->setValue(m_efx->height());
    m_xOffsetSpin->setValue(m_efx->xOffset());
    m_yOffsetSpin->setValue(m_efx->yOffset());
    m_rotationSpin->setValue(m_efx->rotation());
    m_startOffsetSpin->setValue(m_efx->startOffset());
    m_isRelativeCheckbox->setChecked(m_efx->isRelative());

    m_xFrequencySpin->setValue(m_efx->xFrequency());
    m_yFrequencySpin->setValue(m_efx->yFrequency());
    m_xPhaseSpin->setValue(m_efx->xPhase());
    m_yPhaseSpin->setValue(m_efx->yPhase());

    m_waveWidthSpin->setValue(m_efx->waveWidth());
    m_waveShapeCombo->setCurrentIndex(m_efx->waveShape());
    m_waveFadeInSpin->setValue(m_efx->waveFadeIn());
    m_waveFadeOutSpin->setValue(m_efx->waveFadeOut());

    /* Running order */
    switch (m_efx->runOrder())
    {
        default:
        case Function::Loop:
            m_loop->setChecked(true);
        break;
        case Function::SingleShot:
            m_singleShot->setChecked(true);
        break;
        case Function::PingPong:
            m_pingPong->setChecked(true);
        break;
    }

    /* Direction */
    switch (m_efx->direction())
    {
        default:
        case Function::Forward:
            m_forward->setChecked(true);
        break;
        case Function::Backward:
            m_backward->setChecked(true);
        break;
    }

    connect(m_loop, SIGNAL(clicked()),
            this, SLOT(slotLoopClicked()));
    connect(m_singleShot, SIGNAL(clicked()),
            this, SLOT(slotSingleShotClicked()));
    connect(m_pingPong, SIGNAL(clicked()),
            this, SLOT(slotPingPongClicked()));

    connect(m_forward, SIGNAL(clicked()),
            this, SLOT(slotForwardClicked()));
    connect(m_backward, SIGNAL(clicked()),
            this, SLOT(slotBackwardClicked()));

    connect(m_algorithmCombo, SIGNAL(activated(int)),
            this, SLOT(slotAlgorithmSelected(int)));
    connect(m_widthSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotWidthSpinChanged(int)));
    connect(m_heightSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotHeightSpinChanged(int)));
    connect(m_xOffsetSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotXOffsetSpinChanged(int)));
    connect(m_yOffsetSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotYOffsetSpinChanged(int)));
    connect(m_rotationSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotRotationSpinChanged(int)));
    connect(m_startOffsetSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotStartOffsetSpinChanged(int)));
    connect(m_isRelativeCheckbox, SIGNAL(stateChanged(int)),
            this, SLOT(slotIsRelativeCheckboxChanged(int)));

    connect(m_xFrequencySpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotXFrequencySpinChanged(int)));
    connect(m_yFrequencySpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotYFrequencySpinChanged(int)));
    connect(m_xPhaseSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotXPhaseSpinChanged(int)));
    connect(m_yPhaseSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotYPhaseSpinChanged(int)));

    connect(m_waveWidthSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotWaveWidthSpinChanged(int)));
    connect(m_waveShapeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotWaveShapeComboChanged(int)));
    connect(m_waveFadeInSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotWaveFadeInSpinChanged(int)));
    connect(m_waveFadeOutSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotWaveFadeOutSpinChanged(int)));

    connect(m_colorCheck, SIGNAL(toggled(bool)),
            this, SLOT(slotSetColorBackground(bool)));

    redrawPreview();
}

void EFXEditor::slotTestClicked()
{
    if (m_testButton->isChecked() == true)
    {
        m_efx->start(m_doc->masterTimer(), functionParent());

        //Restart animation so preview it is in sync with real test
        m_previewArea->restart();
    }
    else
        m_efx->stopAndWait();
}

void EFXEditor::slotRestartTest()
{
    if (m_testButton->isChecked() == true)
    {
        // Toggle off, toggle on. Duh.
        m_testButton->click();
        m_testButton->click();
    }
}

void EFXEditor::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        m_efx->stopAndWait();
        m_testButton->setChecked(false);
        m_testButton->setEnabled(false);
    }
    else
    {
        m_testButton->setEnabled(true);
    }
}

void EFXEditor::slotTabChanged(int tab)
{
    m_efx->setUiStateValue(UI_STATE_TAB_INDEX, tab);

    //When preview animation is opened restart animation but avoid restart if test is running.
    if (tab == 1 && (m_testButton->isChecked () == false))
        m_previewArea->restart ();
}

void EFXEditor::slotSetColorBackground(bool checked)
{
    m_previewArea->showGradientBackground(checked);
}

bool EFXEditor::interruptRunning()
{
    if (m_testButton->isChecked() == true)
    {
        m_efx->stopAndWait();
        m_testButton->setChecked(false);
        return true;
    }
    else
    {
        return false;
    }
}

void EFXEditor::continueRunning(bool running)
{
    if (running == true)
    {
        if (m_doc->mode() == Doc::Operate)
            m_efx->start(m_doc->masterTimer(), functionParent());
        else
            m_testButton->click();
    }
}

FunctionParent EFXEditor::functionParent() const
{
    return FunctionParent::master();
}

/*****************************************************************************
 * General page
 *****************************************************************************/

void EFXEditor::updateFixtureTree()
{
    // Save current Mode settings from tree before clearing (for empty columns)
    QMap<int, QString> savedColumnModes;
    if (m_efx->isFixtureGroupMode())
    {
        for (int i = 0; i < m_tree->topLevelItemCount(); i++)
        {
            QTreeWidgetItem *item = m_tree->topLevelItem(i);
            int colIndex = item->data(0, Qt::UserRole).toInt();
            QComboBox *combo = qobject_cast<QComboBox*>(m_tree->itemWidget(item, KColumnMode));
            if (combo != nullptr)
            {
                savedColumnModes[colIndex] = combo->currentText();
            }
        }
    }
    
    m_tree->clear();
    
    if (m_efx->isFixtureGroupMode())
    {
        // Disable operations that don't work with columns
        m_removeFixtureButton->setEnabled(false);
        m_raiseFixtureButton->setEnabled(false);
        m_lowerFixtureButton->setEnabled(false);
        m_addFixtureButton->setEnabled(false);
        
        // Fixture Group Mode: show columns with ability to change offset for entire column
        FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
        if (group != nullptr)
        {
            int gridWidth = group->size().width();
            int gridHeight = group->size().height();
            
            if (gridWidth <= 0 || gridHeight <= 0)
            {
                // Empty grid, nothing to show
                m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
                return;
            }
            
            // Group fixtures by column based on their position in the grid
            QMap<int, QList<EFXFixture*>> columnFixtures;
            
            // Iterate through the grid and find which column each fixture belongs to
            for (int col = 0; col < gridWidth; col++)
            {
                for (int row = 0; row < gridHeight; row++)
                {
                    GroupHead gridHead = group->head(QLCPoint(col, row));
                    if (gridHead.isValid())
                    {
                        // Find the EFXFixture that corresponds to this grid position
                        foreach (EFXFixture *ef, m_efx->fixtures())
                        {
                            if (ef->head().fxi == gridHead.fxi && ef->head().head == gridHead.head)
                            {
                                columnFixtures[col].append(ef);
                                break;
                            }
                        }
                    }
                }
            }
            
            // Create tree items for each column (including empty ones)
            for (int col = 0; col < gridWidth; col++)
            {
                int fixtureCount = columnFixtures.contains(col) ? columnFixtures[col].size() : 0;
                
                QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
                item->setText(KColumnNumber, QString("%1").arg(col + 1, 3, 10, QChar('0')));
                item->setText(KColumnName, QString("Column %1 (%2 fixtures)").arg(col + 1).arg(fixtureCount));
                item->setData(0, Qt::UserRole, QVariant(col)); // Store column index
                item->setData(0, Qt::UserRole + 1, QVariant(true)); // Mark as column item
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                
                if (fixtureCount > 0)
                {
                    // Use the first fixture in column as representative
                    EFXFixture *firstFixture = columnFixtures[col].first();
                    
                    if (firstFixture->direction() == Function::Backward)
                        item->setCheckState(KColumnReverse, Qt::Checked);
                    else
                        item->setCheckState(KColumnReverse, Qt::Unchecked);
                    
                    updateModeColumn(item, firstFixture);
                    updateStartOffsetColumn(item, firstFixture);
                }
                else
                {
                    // Empty column - show with default values (can be used as template)
                    item->setCheckState(KColumnReverse, Qt::Unchecked);
                    
                    // Create widgets with defaults for empty columns (enabled for template use)
                    // Mode combo
                    QComboBox* combo = new QComboBox(m_tree);
                    combo->setAutoFillBackground(true);
                    // Use mode list from any fixture, or default list
                    if (!m_efx->fixtures().isEmpty())
                    {
                        combo->addItems(m_efx->fixtures().first()->modeList());
                    }
                    else
                    {
                        // If no fixtures at all, add basic modes
                        combo->addItem("PanTilt");
                        combo->addItem("Dimmer");
                    }
                    combo->setProperty(PROPERTY_FIXTURE, col);
                    // Note: widgets are enabled so user can set template values
                    // but they won't affect anything until fixtures are added to this column
                    m_tree->setItemWidget(item, KColumnMode, combo);
                    
                    // Restore mode for this column (backend first, then UI cache)
                    int backendMode = m_efx->columnMode(col);
                    if (backendMode != 0)  // 0 = PanTilt (default), any other value means it was explicitly set
                    {
                        // Use mode from backend (persistent, from XML)
                        QString modeStr = EFXFixture::modeToString((EFXFixture::Mode)backendMode);
                        int idx = combo->findText(modeStr);
                        if (idx >= 0)
                            combo->setCurrentIndex(idx);
                    }
                    else if (savedColumnModes.contains(col))
                    {
                        // Fallback to UI cache if backend has nothing
                        int idx = combo->findText(savedColumnModes[col]);
                        if (idx >= 0)
                            combo->setCurrentIndex(idx);
                    }
                    
                    // Start offset spin
                    QSpinBox* spin = new QSpinBox(m_tree);
                    spin->setAutoFillBackground(true);
                    spin->setRange(0, 359);
                    // Calculate default offset based on column position and direction
                    int defaultOffset = calculateColumnOffset(col, 0, gridWidth, gridHeight);
                    spin->setValue(defaultOffset);
                    spin->setSuffix(QChar(0x00b0)); // degree
                    spin->setProperty(PROPERTY_FIXTURE, col);
                    // Connect signal even for empty columns (for future use)
                    connect(spin, SIGNAL(valueChanged(int)),
                            this, SLOT(slotFixtureStartOffsetChanged(int)));
                    m_tree->setItemWidget(item, KColumnStartOffset, spin);
                }
            }
        }
    }
    else
    {
        // Re-enable buttons in normal mode
        m_removeFixtureButton->setEnabled(true);
        m_raiseFixtureButton->setEnabled(true);
        m_lowerFixtureButton->setEnabled(true);
        m_addFixtureButton->setEnabled(true);
        
        // Normal mode: show individual fixtures
        QListIterator <EFXFixture*> it(m_efx->fixtures());
        while (it.hasNext())
            addFixtureItem(it.next());
    }
    
    m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

QTreeWidgetItem* EFXEditor::fixtureItem(EFXFixture* ef)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it != NULL)
    {
        QTreeWidgetItem* item = *it;
        EFXFixture* ef_item = reinterpret_cast<EFXFixture*>
                              (item->data(0, Qt::UserRole).toULongLong());
        if (ef_item == ef)
            return item;
        ++it;
    }

    return NULL;
}

const QList <EFXFixture*> EFXEditor::selectedFixtures() const
{
    QListIterator <QTreeWidgetItem*> it(m_tree->selectedItems());
    QList <EFXFixture*> list;

    /* Put all selected fixture IDs to a list and return it */
    while (it.hasNext() == true)
    {
        EFXFixture* ef = reinterpret_cast <EFXFixture*>
                         (it.next()->data(0, Qt::UserRole).toULongLong());
        list << ef;
    }

    return list;
}

void EFXEditor::updateIndices(int from, int to)
{
    for (int i = from; i <= to; i++)
    {
        QTreeWidgetItem *item = m_tree->topLevelItem(i);
        Q_ASSERT(item != NULL);

        item->setText(KColumnNumber,
                      QString("%1").arg(i + 1, 3, 10, QChar('0')));
    }
}

void EFXEditor::addFixtureItem(EFXFixture* ef)
{
    QTreeWidgetItem* item;
    Fixture* fxi;

    Q_ASSERT(ef != NULL);

    fxi = m_doc->fixture(ef->head().fxi);
    if (fxi == NULL)
        return;

    item = new QTreeWidgetItem(m_tree);

    if (fxi->heads() > 1)
    {
        item->setText(KColumnName, QString("%1 [%2]").arg(fxi->name()).arg(ef->head().head));
    }
    else
    {
        item->setText(KColumnName, fxi->name());
    }
    item->setData(0, Qt::UserRole, QVariant(reinterpret_cast<qulonglong> (ef)));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

    if (ef->direction() == Function::Backward)
        item->setCheckState(KColumnReverse, Qt::Checked);
    else
        item->setCheckState(KColumnReverse, Qt::Unchecked);

    updateModeColumn(item, ef);
    updateStartOffsetColumn(item, ef);

    updateIndices(m_tree->indexOfTopLevelItem(item),
                  m_tree->topLevelItemCount() - 1);

    /* Select newly-added fixtures so that they can be moved quickly */
    m_tree->setCurrentItem(item);
}

void EFXEditor::updateModeColumn(QTreeWidgetItem* item, EFXFixture* ef)
{
    Q_ASSERT(item != NULL);
    Q_ASSERT(ef != NULL);

    if (m_tree->itemWidget(item, KColumnMode) == NULL)
    {
        QComboBox* combo = new QComboBox(m_tree);
        combo->setAutoFillBackground(true);
        combo->addItems(ef->modeList());
        
        if (m_efx->isFixtureGroupMode())
        {
            // In group mode, store column index
            int columnIndex = item->data(0, Qt::UserRole).toInt();
            combo->setProperty(PROPERTY_FIXTURE, columnIndex);
        }
        else
        {
            // Normal mode, store EFXFixture pointer
            combo->setProperty(PROPERTY_FIXTURE, (qulonglong) ef);
        }
        
        m_tree->setItemWidget(item, KColumnMode, combo);

        const int index = combo->findText(ef->modeToString(ef->mode()));
        combo->setCurrentIndex(index);

        connect(combo, SIGNAL(currentIndexChanged(int)),
                this, SLOT(slotFixtureModeChanged(int)));
    }
}

void EFXEditor::updateStartOffsetColumn(QTreeWidgetItem* item, EFXFixture* ef)
{
    Q_ASSERT(item != NULL);
    Q_ASSERT(ef != NULL);

    if (m_tree->itemWidget(item, KColumnStartOffset) == NULL)
    {
        QSpinBox* spin = new QSpinBox(m_tree);
        spin->setAutoFillBackground(true);
        spin->setRange(0, 359);
        spin->setValue(ef->startOffset());
        spin->setSuffix(QChar(0x00b0)); // degree
        m_tree->setItemWidget(item, KColumnStartOffset, spin);
        
        if (m_efx->isFixtureGroupMode())
        {
            // In group mode, store column index
            int columnIndex = item->data(0, Qt::UserRole).toInt();
            spin->setProperty(PROPERTY_FIXTURE, columnIndex);
        }
        else
        {
            // Normal mode, store EFXFixture pointer
            spin->setProperty(PROPERTY_FIXTURE, (qulonglong) ef);
        }
        
        connect(spin, SIGNAL(valueChanged(int)),
                this, SLOT(slotFixtureStartOffsetChanged(int)));
    }
}

void EFXEditor::removeFixtureItem(EFXFixture* ef)
{
    QTreeWidgetItem* item;
    int from;

    Q_ASSERT(ef != NULL);

    item = fixtureItem(ef);
    Q_ASSERT(item != NULL);

    from = m_tree->indexOfTopLevelItem(item);
    delete item;

    updateIndices(from, m_tree->topLevelItemCount() - 1);
    redrawPreview();

    m_tree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void EFXEditor::slotDialDestroyed(QObject *)
{
    m_speedDial->setChecked(false);
}

void EFXEditor::createSpeedDials()
{
    if (m_speedDials == NULL)
    {
        m_speedDials = new SpeedDialWidget(this);
        m_speedDials->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_speedDials, SIGNAL(fadeInChanged(int)), this, SLOT(slotFadeInChanged(int)));
        connect(m_speedDials, SIGNAL(fadeOutChanged(int)), this, SLOT(slotFadeOutChanged(int)));
        connect(m_speedDials, SIGNAL(holdChanged(int)), this, SLOT(slotHoldChanged(int)));
        connect(m_speedDials, SIGNAL(destroyed(QObject*)), this, SLOT(slotDialDestroyed(QObject*)));
    }

    m_speedDials->show();
}

void EFXEditor::updateSpeedDials()
{
   if (m_speedDial->isChecked() == false)
        return;

    createSpeedDials();

    m_speedDials->setWindowTitle(m_efx->name());
    m_speedDials->setFadeInSpeed(m_efx->fadeInSpeed());
    m_speedDials->setFadeOutSpeed(m_efx->fadeOutSpeed());
    if ((int)m_efx->duration() < 0)
        m_speedDials->setDuration(m_efx->duration());
    else
        m_speedDials->setDuration(m_efx->duration() - m_efx->fadeInSpeed() - m_efx->fadeOutSpeed());
}

void EFXEditor::slotNameEdited(const QString &text)
{
    m_efx->setName(text);
    if (m_speedDials)
        m_speedDials->setWindowTitle(text);
}

void EFXEditor::slotSpeedDialToggle(bool state)
{
    if (state == true)
    {
        updateSpeedDials();
    }
    else
    {
        if (m_speedDials != NULL)
            m_speedDials->deleteLater();
        m_speedDials = NULL;
    }

    m_efx->setUiStateValue(UI_STATE_SHOW_DIAL, state);
}

void EFXEditor::slotFixtureItemChanged(QTreeWidgetItem* item, int column)
{
    if (column == KColumnReverse)
    {
        if (m_efx->isFixtureGroupMode())
        {
            // In group mode, update all fixtures in this column
            int columnIndex = item->data(0, Qt::UserRole).toInt();
            
            FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
            if (group == nullptr)
                return;
            
            int gridWidth = group->size().width();
            if (columnIndex < 0 || columnIndex >= gridWidth)
                return;
            
            Function::Direction dir = (item->checkState(column) == Qt::Checked) ? Function::Backward : Function::Forward;
            int gridHeight = group->size().height();
            
            foreach (EFXFixture *ef, m_efx->fixtures())
            {
                for (int row = 0; row < gridHeight; row++)
                {
                    GroupHead head = group->head(QLCPoint(columnIndex, row));
                    if (head.isValid() && head.fxi == ef->head().fxi && head.head == ef->head().head)
                    {
                        ef->setDirection(dir);
                        break;
                    }
                }
            }
        }
        else
        {
            // Normal mode
            EFXFixture* ef = reinterpret_cast <EFXFixture*>
                             (item->data(0, Qt::UserRole).toULongLong());
            Q_ASSERT(ef != NULL);

            if (item->checkState(column) == Qt::Checked)
                ef->setDirection(Function::Backward);
            else
                ef->setDirection(Function::Forward);
        }

        redrawPreview();
    }
}

void EFXEditor::slotFixtureModeChanged(int index)
{
    QComboBox *combo = qobject_cast<QComboBox*>(QObject::sender());
    Q_ASSERT(combo != NULL);

    if (m_efx->isFixtureGroupMode())
    {
        // In group mode, update all fixtures in this column
        int columnIndex = combo->property(PROPERTY_FIXTURE).toInt();
        
        FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
        if (group == nullptr || m_efx->fixtures().isEmpty())
            return;
        
        int gridWidth = group->size().width();
        if (columnIndex < 0 || columnIndex >= gridWidth)
            return;
        
        // Get mode from first fixture (they should all use same mode)
        EFXFixture *firstEf = m_efx->fixtures().first();
        EFXFixture::Mode mode = firstEf->stringToMode(combo->itemText(index));
        
        // Save mode for this column in backend (persistent)
        m_efx->setColumnMode(columnIndex, (int)mode);
        
        int gridHeight = group->size().height();
        
        foreach (EFXFixture *ef, m_efx->fixtures())
        {
            for (int row = 0; row < gridHeight; row++)
            {
                GroupHead head = group->head(QLCPoint(columnIndex, row));
                if (head.isValid() && head.fxi == ef->head().fxi && head.head == ef->head().head)
                {
                    ef->setMode(mode);
                    break;
                }
            }
        }
    }
    else
    {
        // Normal mode
        EFXFixture *ef = (EFXFixture*) combo->property(PROPERTY_FIXTURE).toULongLong();
        Q_ASSERT(ef != NULL);

        ef->setMode(ef->stringToMode(combo->itemText(index)));
    }

    // Restart the test after the latest mode change, delayed
    m_testTimer.start();
}

void EFXEditor::slotFixtureStartOffsetChanged(int startOffset)
{
    QSpinBox *spin = qobject_cast<QSpinBox*>(QObject::sender());
    Q_ASSERT(spin != NULL);
    
    if (m_efx->isFixtureGroupMode())
    {
        // In group mode, the spin box stores column index instead of EFXFixture pointer
        int columnIndex = spin->property(PROPERTY_FIXTURE).toInt();
        
        // Update all fixtures in this column
        FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
        if (group == nullptr)
            return;
        
        int gridWidth = group->size().width();
        if (columnIndex < 0 || columnIndex >= gridWidth)
            return;
        
        int gridHeight = group->size().height();
        
        foreach (EFXFixture *ef, m_efx->fixtures())
        {
            // Check if this fixture is in the column
            for (int row = 0; row < gridHeight; row++)
            {
                GroupHead head = group->head(QLCPoint(columnIndex, row));
                if (head.isValid() && head.fxi == ef->head().fxi && head.head == ef->head().head)
                {
                    ef->setStartOffset(startOffset);
                    break;
                }
            }
        }
    }
    else
    {
        // Normal mode
        EFXFixture *ef = (EFXFixture*) spin->property(PROPERTY_FIXTURE).toULongLong();
        Q_ASSERT(ef != NULL);
        ef->setStartOffset(startOffset);
    }

    redrawPreview();

    // Restart the test after the latest offset change, delayed
    m_testTimer.start();
}

void EFXEditor::slotAddFixtureClicked()
{
    /* The following code is the original QLC+ code (EFX with only Pan-Tilt).
     * Now, with modes, the same fixture could be duplicated. */

    /* Put all fixtures already present into a list of fixtures that
       will be disabled in the fixture selection dialog */
    QList <GroupHead> disabled;
    QTreeWidgetItemIterator twit(m_tree);
    /*
    while (*twit != NULL)
    {
        EFXFixture* ef = reinterpret_cast <EFXFixture*>
                         ((*twit)->data(0, Qt::UserRole).toULongLong());
        Q_ASSERT(ef != NULL);

        disabled.append(ef->head());
        twit++;
    }
    */

    /* Disable all fixtures that don't have pan OR tilt, dimmer or RGB channels */
    /*
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
                if (heads[i].panMsbChannel() == QLCChannel::invalid() &&
                    heads[i].tiltMsbChannel() == QLCChannel::invalid() &&
                    heads[i].panLsbChannel() == QLCChannel::invalid() &&
                    heads[i].tiltLsbChannel() == QLCChannel::invalid())
                {
                    // Disable heads without pan or tilt channels
                    disabled << GroupHead(fixture->id(), i);
                }
            }
        }
    }
    */

    FixtureSelection fs(this, m_doc);
    fs.setMultiSelection(true);
    fs.setSelectionMode(FixtureSelection::Heads);
    fs.setDisabledHeads(disabled);
    if (fs.exec() == QDialog::Accepted)
    {
        // Stop running while adding fixtures
        bool running = interruptRunning();

        QListIterator <GroupHead> it(fs.selectedHeads());
        while (it.hasNext() == true)
        {
            EFXFixture* ef = new EFXFixture(m_efx);
            ef->setHead(it.next());

            if (m_efx->addFixture(ef) == true)
                addFixtureItem(ef);
            else
                delete ef;
        }

        m_tree->header()->resizeSections(QHeaderView::ResizeToContents);

        redrawPreview();

        // Continue running if appropriate
        continueRunning(running);
    }
}

void EFXEditor::slotRemoveFixtureClicked()
{
    int r = QMessageBox::question(
                this, tr("Remove fixtures"),
                tr("Do you want to remove the selected fixture(s)?"),
                QMessageBox::Yes, QMessageBox::No);

    if (r == QMessageBox::Yes)
    {
        // Stop running while removing fixtures
        bool running = interruptRunning();

        QListIterator <EFXFixture*> it(selectedFixtures());
        while (it.hasNext() == true)
        {
            EFXFixture* ef = it.next();
            Q_ASSERT(ef != NULL);

            removeFixtureItem(ef);
            if (m_efx->removeFixture(ef) == true)
                delete ef;
        }

        redrawPreview();

        // Continue if appropriate
        continueRunning(running);
    }
}

void EFXEditor::slotRaiseFixtureClicked()
{
    // Stop running while moving fixtures
    bool running = interruptRunning();

    QTreeWidgetItem* item = m_tree->currentItem();
    if (item != NULL)
    {
        int index = m_tree->indexOfTopLevelItem(item);
        if (index == 0)
            return;

        EFXFixture* ef = reinterpret_cast <EFXFixture*>
                         (item->data(0, Qt::UserRole).toULongLong());
        Q_ASSERT(ef != NULL);

        if (m_efx->raiseFixture(ef) == true)
        {
            item = m_tree->takeTopLevelItem(index);

            m_tree->insertTopLevelItem(index - 1, item);

            updateModeColumn(item, ef);
            updateStartOffsetColumn(item, ef);
            updateIndices(index - 1, index);
            m_tree->setCurrentItem(item);

            redrawPreview();
        }
    }

    // Continue running if appropriate
    continueRunning(running);
}

void EFXEditor::slotLowerFixtureClicked()
{
    // Stop running while moving fixtures
    bool running = interruptRunning();

    QTreeWidgetItem* item = m_tree->currentItem();
    if (item != NULL)
    {
        int index = m_tree->indexOfTopLevelItem(item);
        if (index == (m_tree->topLevelItemCount() - 1))
            return;

        EFXFixture* ef = reinterpret_cast <EFXFixture*>
                         (item->data(0, Qt::UserRole).toULongLong());
        Q_ASSERT(ef != NULL);

        if (m_efx->lowerFixture(ef) == true)
        {
            item = m_tree->takeTopLevelItem(index);
            m_tree->insertTopLevelItem(index + 1, item);

            updateModeColumn(item, ef);
            updateStartOffsetColumn(item, ef);
            updateIndices(index, index + 1);
            m_tree->setCurrentItem(item);

            redrawPreview();
        }
    }

    // Continue running if appropriate
    continueRunning(running);
}

void EFXEditor::slotParallelRadioToggled(bool state)
{
    Q_ASSERT(m_efx != NULL);
    if (state == true)
    {
        m_efx->setPropagationMode(EFX::Parallel);
        redrawPreview();
    }
}

void EFXEditor::slotSerialRadioToggled(bool state)
{
    Q_ASSERT(m_efx != NULL);
    if (state == true)
    {

        m_efx->setPropagationMode(EFX::Serial);
        redrawPreview();
    }
}

void EFXEditor::slotAsymmetricRadioToggled(bool state)
{
    Q_ASSERT(m_efx != NULL);
    if (state == true)
    {

        m_efx->setPropagationMode(EFX::Asymmetric);
        redrawPreview();
    }
}

void EFXEditor::slotFadeInChanged(int ms)
{
    m_efx->setFadeInSpeed(ms);
    slotRestartTest();
}

void EFXEditor::slotFadeOutChanged(int ms)
{
    m_efx->setFadeOutSpeed(ms);
}

void EFXEditor::slotHoldChanged(int ms)
{
    uint duration = 0;
    if (ms < 0)
        duration = ms;
    else
        duration = m_efx->fadeInSpeed() + ms + m_efx->fadeOutSpeed();
    m_efx->setDuration(duration);
    redrawPreview();
}

void EFXEditor::slotFixtureRemoved()
{
    // EFX already catches fixture removals so just update the list
    updateFixtureTree();
    redrawPreview();
}

void EFXEditor::slotFixtureChanged()
{
    // Update the tree in case fixture's name changes
    updateFixtureTree();
}

void EFXEditor::slotFixtureGroupRemoved(quint32 id)
{
    if (m_efx->fixtureGroupID() == id)
    {
        // Our group was deleted, uncheck and disable
        m_useFixtureGroupCheck->setChecked(false);
        updateFixtureTree();
        redrawPreview();
    }
    
    // Refresh combo list
    updateFixtureGroupCombo();
}

void EFXEditor::updateFixtureGroupCombo()
{
    m_fixtureGroupCombo->clear();
    
    foreach (FixtureGroup *grp, m_doc->fixtureGroups())
    {
        m_fixtureGroupCombo->addItem(grp->name(), grp->id());
    }
    
    m_fixtureGroupCombo->setEnabled(m_useFixtureGroupCheck->isChecked());
}

void EFXEditor::updateRowSelection()
{
    // Block signals to prevent recursion
    bool wasBlocked = m_rowSelectionGroup->signalsBlocked();
    m_rowSelectionGroup->blockSignals(true);
    
    // Clear existing checkboxes
    foreach (QCheckBox *cb, m_rowCheckboxes)
    {
        cb->blockSignals(true);
        m_rowSelectionLayout->removeWidget(cb);
        delete cb;
    }
    m_rowCheckboxes.clear();
    
    // Clear placeholder if exists
    QLayoutItem *child;
    while ((child = m_rowSelectionLayout->takeAt(0)) != nullptr)
    {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
    
    if (!m_efx->isFixtureGroupMode())
    {
        m_rowSelectionGroup->setEnabled(false);
        QLabel *placeholder = new QLabel(tr("Select a fixture group to see rows"));
        m_rowSelectionLayout->addWidget(placeholder);
        m_rowSelectionGroup->blockSignals(wasBlocked);
        return;
    }
    
    FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
    if (group == nullptr)
    {
        m_rowSelectionGroup->setEnabled(false);
        QLabel *placeholder = new QLabel(tr("Invalid fixture group"));
        m_rowSelectionLayout->addWidget(placeholder);
        m_rowSelectionGroup->blockSignals(wasBlocked);
        return;
    }
    
    int gridHeight = group->size().height();
    if (gridHeight <= 0)
    {
        m_rowSelectionGroup->setEnabled(false);
        QLabel *placeholder = new QLabel(tr("Empty grid"));
        m_rowSelectionLayout->addWidget(placeholder);
        m_rowSelectionGroup->blockSignals(wasBlocked);
        return;
    }
    
    m_rowSelectionGroup->setEnabled(true);
    
    // Create checkbox for each row
    QList<int> currentSelection = m_efx->selectedRows();
    for (int row = 0; row < gridHeight; row++)
    {
        QCheckBox *cb = new QCheckBox(QString("Row %1").arg(row + 1));
        cb->blockSignals(true);
        // If no selection saved, select all by default
        cb->setChecked(currentSelection.isEmpty() || currentSelection.contains(row));
        cb->setProperty("row_index", row);
        
        m_rowSelectionLayout->addWidget(cb);
        m_rowCheckboxes.append(cb);
        
        // Now connect signal after setting initial state
        cb->blockSignals(false);
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(slotRowSelectionChanged()));
    }
    
    m_rowSelectionGroup->blockSignals(wasBlocked);
}

void EFXEditor::slotRowSelectionChanged()
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
            m_rowCheckboxes.first()->setChecked(true);
            selectedRows.append(0);
        }
    }
    
    m_efx->setSelectedRows(selectedRows);
    
    // Save fixture settings before removing (mode, direction)
    QMap<QPair<quint32, int>, QPair<EFXFixture::Mode, Function::Direction>> fixtureSettings;
    foreach (EFXFixture *ef, m_efx->fixtures())
    {
        QPair<quint32, int> key(ef->head().fxi, ef->head().head);
        QPair<EFXFixture::Mode, Function::Direction> value(ef->mode(), ef->direction());
        fixtureSettings[key] = value;
    }
    
    // Recreate fixtures WITHOUT calling slotFixtureGroupChanged
    // (which would call updateRowSelection and destroy our checkboxes!)
    bool running = interruptRunning();
    
    m_efx->removeAllFixtures();
    
    FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
    if (group != nullptr)
    {
        int gridWidth = group->size().width();
        int gridHeight = group->size().height();
        
        if (gridWidth > 0 && gridHeight > 0)
        {
            for (int col = 0; col < gridWidth; col++)
            {
                for (int row = 0; row < gridHeight; row++)
                {
                    // Check if this row is selected
                    if (!m_efx->isRowSelected(row))
                        continue;
                    
                    GroupHead head = group->head(QLCPoint(col, row));
                    if (head.isValid())
                    {
                        EFXFixture *ef = new EFXFixture(m_efx);
                        ef->setHead(head);
                        int columnOffset = calculateColumnOffset(col, row, gridWidth, gridHeight);
                        ef->setStartOffset(columnOffset);
                        
                        // Restore mode from backend (column-specific, persistent)
                        EFXFixture::Mode columnMode = (EFXFixture::Mode)m_efx->columnMode(col);
                        ef->setMode(columnMode);
                        
                        // Restore direction from saved settings if available
                        QPair<quint32, int> key(head.fxi, head.head);
                        if (fixtureSettings.contains(key))
                        {
                            ef->setDirection(fixtureSettings[key].second);
                        }
                        
                        if (!m_efx->addFixture(ef))
                            delete ef;
                    }
                }
            }
        }
    }
    
    updateFixtureTree();  // Only update tree, NOT row selection!
    redrawPreview();
    continueRunning(running);
    
    updating = false;
}

void EFXEditor::slotUseFixtureGroupToggled(bool checked)
{
    m_fixtureGroupCombo->setEnabled(checked);
    m_offsetDirectionCombo->setEnabled(checked);
    m_offsetStepSpin->setEnabled(checked);
    m_wingsSpin->setEnabled(checked);
    
    if (checked && m_fixtureGroupCombo->count() > 0)
    {
        // Validate combo data
        if (!m_fixtureGroupCombo->currentData().isValid())
            return;
        
        // Enable group mode
        quint32 groupId = m_fixtureGroupCombo->currentData().toUInt();
        if (groupId == FixtureGroup::invalidId())
            return;
        
        m_efx->setFixtureGroupID(groupId);
        
        // Clear existing fixtures and create fixture entries for each fixture in group
        m_efx->removeAllFixtures();
        
        FixtureGroup *group = m_doc->fixtureGroup(groupId);
        if (group != nullptr)
        {
            int gridWidth = group->size().width();
            int gridHeight = group->size().height();
            
            if (gridWidth <= 0 || gridHeight <= 0)
            {
                // Empty grid, don't create fixtures
                updateFixtureTree();
                redrawPreview();
                return;
            }
            
            // Create EFXFixture for each fixture in the group
            for (int col = 0; col < gridWidth; col++)
            {
                for (int row = 0; row < gridHeight; row++)
                {
                    // Check if this row is selected
                    if (!m_efx->isRowSelected(row))
                        continue;
                    
                    GroupHead head = group->head(QLCPoint(col, row));
                    if (head.isValid())
                    {
                        EFXFixture *ef = new EFXFixture(m_efx);
                        ef->setHead(head);
                        // Calculate offset based on direction and step
                        int columnOffset = calculateColumnOffset(col, row, gridWidth, gridHeight);
                        ef->setStartOffset(columnOffset);
                        
                        // Restore mode from backend (column-specific, persistent)
                        EFXFixture::Mode columnMode = (EFXFixture::Mode)m_efx->columnMode(col);
                        ef->setMode(columnMode);
                        
                        if (!m_efx->addFixture(ef))
                            delete ef;
                    }
                }
            }
        }
    }
    else
    {
        // Disable group mode
        m_efx->setFixtureGroupID(FixtureGroup::invalidId());
        m_efx->removeAllFixtures();
    }
    
    updateFixtureTree();
    updateRowSelection();
    redrawPreview();
}

void EFXEditor::slotFixtureGroupChanged(int index)
{
    if (!m_useFixtureGroupCheck->isChecked())
        return;
    
    if (index < 0)
        return;
    
    // Validate combo data
    if (!m_fixtureGroupCombo->itemData(index).isValid())
        return;
    
    quint32 groupId = m_fixtureGroupCombo->itemData(index).toUInt();
    if (groupId == FixtureGroup::invalidId())
        return;
    
    m_efx->setFixtureGroupID(groupId);
    
    // Recreate fixture entries for new group
    bool running = interruptRunning();
    
    m_efx->removeAllFixtures();
    
    FixtureGroup *group = m_doc->fixtureGroup(groupId);
    if (group != nullptr)
    {
        int gridWidth = group->size().width();
        int gridHeight = group->size().height();
        
        if (gridWidth <= 0 || gridHeight <= 0)
        {
            // Empty grid
            updateFixtureTree();
            redrawPreview();
            continueRunning(running);
            return;
        }
        
        // Create EFXFixture for each fixture in the group
        for (int col = 0; col < gridWidth; col++)
        {
            for (int row = 0; row < gridHeight; row++)
            {
                // Check if this row is selected
                if (!m_efx->isRowSelected(row))
                    continue;
                
                GroupHead head = group->head(QLCPoint(col, row));
                if (head.isValid())
                {
                    EFXFixture *ef = new EFXFixture(m_efx);
                    ef->setHead(head);
                    // Calculate offset based on direction and step
                    int columnOffset = calculateColumnOffset(col, row, gridWidth, gridHeight);
                    ef->setStartOffset(columnOffset);
                    
                    // Restore mode from backend (column-specific, persistent)
                    EFXFixture::Mode columnMode = (EFXFixture::Mode)m_efx->columnMode(col);
                    ef->setMode(columnMode);
                    
                    if (!m_efx->addFixture(ef))
                        delete ef;
                }
            }
        }
    }
    
    updateFixtureTree();
    updateRowSelection();
    redrawPreview();
    continueRunning(running);
}

int EFXEditor::calculateColumnOffset(int col, int row, int gridWidth, int gridHeight)
{
    Q_UNUSED(row);
    Q_UNUSED(gridHeight);
    
    int wings = m_efx->wings();
    int step = m_efx->offsetStep();
    
    // Wings mode: divide columns into symmetric blocks
    if (wings > 1 && gridWidth > 0)
    {
        int blockSize = gridWidth / wings;
        if (blockSize < 1) blockSize = 1;
        
        int blockIndex = col / blockSize;  // Which block (0, 1, 2, ...)
        int posInBlock = col % blockSize;  // Position within block (0 to blockSize-1)
        
        // Calculate offset within the block based on direction
        int index = 0;
        switch (m_efx->offsetDirection())
        {
            case EFX::LeftToRight:
                index = posInBlock;
                break;
            case EFX::RightToLeft:
                index = (blockSize - 1) - posInBlock;
                break;
            case EFX::CenterToSides:
                {
                    int center = blockSize / 2;
                    index = abs(posInBlock - center);
                }
                break;
            case EFX::SidesToCenter:
                {
                    int center = blockSize / 2;
                    int distance = abs(posInBlock - center);
                    int maxDistance = (blockSize + 1) / 2;
                    index = maxDistance - distance - 1;
                }
                break;
            case EFX::Alternate:
                if (posInBlock % 2 == 0)
                    index = posInBlock / 2;
                else
                    index = (blockSize / 2) + ((posInBlock + 1) / 2);
                break;
            case EFX::Symmetric:
                {
                    int center = blockSize / 2;
                    if (posInBlock <= center)
                        index = posInBlock;
                    else
                        index = blockSize - 1 - posInBlock;
                }
                break;
        }
        
        return (step * index) % 360;
    }
    
    // No wings (wings = 1): normal propagation across all columns
    int index = 0;
    
    switch (m_efx->offsetDirection())
    {
        case EFX::LeftToRight:
            // 0, 1, 2, 3, ...
            index = col;
            break;
            
        case EFX::RightToLeft:
            // N-1, N-2, ..., 1, 0
            index = (gridWidth - 1) - col;
            break;
            
        case EFX::CenterToSides:
            // Center goes first, then alternating left/right
            {
                int center = gridWidth / 2;
                int distance = abs(col - center);
                index = distance;
            }
            break;
            
        case EFX::SidesToCenter:
            // Sides first, center last
            {
                int center = gridWidth / 2;
                int distance = abs(col - center);
                int maxDistance = (gridWidth + 1) / 2;
                index = maxDistance - distance - 1;
            }
            break;
            
        case EFX::Alternate:
            // Odd columns first (0,2,4...), then even (1,3,5...)
            if (col % 2 == 0)
                index = col / 2;
            else
                index = (gridWidth / 2) + ((col + 1) / 2);
            break;
            
        case EFX::Symmetric:
            // Mirror: 0 and N-1 together, 1 and N-2 together, etc.
            // For even width: middle two columns get same index
            {
                if (gridWidth % 2 == 0)
                {
                    // Even: middle two columns get same index
                    int centerLeft = (gridWidth / 2) - 1;
                    int centerRight = gridWidth / 2;
                    
                    if (col <= centerLeft)
                        index = col;
                    else
                        index = gridWidth - 1 - col;
                }
                else
                {
                    // Odd: normal mirror
                    int center = gridWidth / 2;
                    if (col <= center)
                        index = col;
                    else
                        index = gridWidth - 1 - col;
                }
            }
            break;
    }
    
    return (step * index) % 360;
}

void EFXEditor::slotOffsetDirectionChanged(int index)
{
    m_efx->setOffsetDirection((EFX::OffsetDirection)index);
    
    // Recreate fixtures with new offsets
    if (m_efx->isFixtureGroupMode())
    {
        bool running = interruptRunning();
        slotFixtureGroupChanged(m_fixtureGroupCombo->currentIndex());
        continueRunning(running);
    }
}

void EFXEditor::slotOffsetStepChanged(int value)
{
    m_efx->setOffsetStep(value);
    
    // Recreate fixtures with new offsets
    if (m_efx->isFixtureGroupMode())
    {
        bool running = interruptRunning();
        slotFixtureGroupChanged(m_fixtureGroupCombo->currentIndex());
        continueRunning(running);
    }
}

void EFXEditor::slotWingsChanged(int value)
{
    m_efx->setWings(value);
    
    // Recreate fixtures with new offsets
    if (m_efx->isFixtureGroupMode())
    {
        bool running = interruptRunning();
        slotFixtureGroupChanged(m_fixtureGroupCombo->currentIndex());
        continueRunning(running);
    }
}

/*****************************************************************************
 * Movement page
 *****************************************************************************/

void EFXEditor::slotAlgorithmSelected(int algoIndex)
{
    Q_ASSERT(m_efx != NULL);

    m_efx->setAlgorithm(EFX::Algorithm(algoIndex));

    if (m_efx->isFrequencyEnabled())
    {
        m_xFrequencyLabel->setEnabled(true);
        m_yFrequencyLabel->setEnabled(true);

        m_xFrequencySpin->setEnabled(true);
        m_yFrequencySpin->setEnabled(true);
    }
    else
    {
        m_xFrequencyLabel->setEnabled(false);
        m_yFrequencyLabel->setEnabled(false);

        m_xFrequencySpin->setEnabled(false);
        m_yFrequencySpin->setEnabled(false);
    }

    if (m_efx->isPhaseEnabled())
    {
        m_xPhaseLabel->setEnabled(true);
        m_yPhaseLabel->setEnabled(true);

        m_xPhaseSpin->setEnabled(true);
        m_yPhaseSpin->setEnabled(true);
    }
    else
    {
        m_xPhaseLabel->setEnabled(false);
        m_yPhaseLabel->setEnabled(false);

        m_xPhaseSpin->setEnabled(false);
        m_yPhaseSpin->setEnabled(false);
    }

    qDebug() << "DimmerWave check: algorithm =" << m_efx->algorithm() << ", isWaveParametersEnabled =" << m_efx->isWaveParametersEnabled();
    
    if (m_efx->isWaveParametersEnabled())
    {
        qDebug() << "ENABLING DimmerWave parameters!";
        m_waveWidthLabel->setEnabled(true);
        m_waveWidthSpin->setEnabled(true);
        m_waveShapeLabel->setEnabled(true);
        m_waveShapeCombo->setEnabled(true);
        m_waveFadeInLabel->setEnabled(true);
        m_waveFadeInSpin->setEnabled(true);
        m_waveFadeOutLabel->setEnabled(true);
        m_waveFadeOutSpin->setEnabled(true);
    }
    else
    {
        qDebug() << "DISABLING DimmerWave parameters";
        m_waveWidthLabel->setEnabled(false);
        m_waveWidthSpin->setEnabled(false);
        m_waveShapeLabel->setEnabled(false);
        m_waveShapeCombo->setEnabled(false);
        m_waveFadeInLabel->setEnabled(false);
        m_waveFadeInSpin->setEnabled(false);
        m_waveFadeOutLabel->setEnabled(false);
        m_waveFadeOutSpin->setEnabled(false);
    }

    redrawPreview();
}

void EFXEditor::slotWidthSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setWidth(value);
    redrawPreview();
}

void EFXEditor::slotHeightSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setHeight(value);
    redrawPreview();
}

void EFXEditor::slotRotationSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setRotation(value);
    redrawPreview();
}

void EFXEditor::slotStartOffsetSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setStartOffset(value);
    redrawPreview();
}

void EFXEditor::slotIsRelativeCheckboxChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setIsRelative(value == Qt::Checked);
}

void EFXEditor::slotXOffsetSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setXOffset(value);
    redrawPreview();
}

void EFXEditor::slotYOffsetSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setYOffset(value);
    redrawPreview();
}

void EFXEditor::slotXFrequencySpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setXFrequency(value);
    redrawPreview();
}

void EFXEditor::slotYFrequencySpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setYFrequency(value);
    redrawPreview();
}

void EFXEditor::slotXPhaseSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setXPhase(value);
    redrawPreview();
}

void EFXEditor::slotYPhaseSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setYPhase(value);
    redrawPreview();
}

void EFXEditor::slotWaveWidthSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setWaveWidth(value);
    redrawPreview();
}

void EFXEditor::slotWaveShapeComboChanged(int index)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setWaveShape(index);
    redrawPreview();
}

void EFXEditor::slotWaveFadeInSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setWaveFadeIn(value);
    redrawPreview();
}

void EFXEditor::slotWaveFadeOutSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setWaveFadeOut(value);
    redrawPreview();
}

/*****************************************************************************
 * Run order
 *****************************************************************************/

void EFXEditor::slotLoopClicked()
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setRunOrder(Function::Loop);
}

void EFXEditor::slotSingleShotClicked()
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setRunOrder(Function::SingleShot);
}

void EFXEditor::slotPingPongClicked()
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setRunOrder(Function::PingPong);
}

/*****************************************************************************
 * Direction
 *****************************************************************************/

void EFXEditor::slotForwardClicked()
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setDirection(Function::Forward);
    redrawPreview();
}

void EFXEditor::slotBackwardClicked()
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setDirection(Function::Backward);
    redrawPreview();
}

void EFXEditor::redrawPreview()
{
    if (m_previewArea == NULL)
        return;

    QPolygonF polygon;
    m_efx->preview(polygon);

    QVector <QPolygonF> fixturePoints;
    m_efx->previewFixtures(fixturePoints);

    m_previewArea->setPolygon(polygon);
    m_previewArea->setFixturePolygons(fixturePoints);

    m_previewArea->draw(m_efx->duration() / polygon.size());
}


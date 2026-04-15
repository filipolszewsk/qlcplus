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
#include <QSignalBlocker>
#include <QClipboard>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollArea>
#include <QGridLayout>
#include <QDialog>
#include <QVBoxLayout>
#include <QFrame>
#include <QDialogButtonBox>

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

    /* Wrap the tab widget in a scroll area so the editor scrolls when content
       doesn't fit instead of forcing the parent window to grow */
    QGridLayout *rootGrid = qobject_cast<QGridLayout*>(this->layout());
    if (rootGrid)
    {
        rootGrid->removeWidget(m_tab);
        QScrollArea *editorScrollArea = new QScrollArea(this);
        editorScrollArea->setWidgetResizable(true);
        editorScrollArea->setFrameShape(QFrame::NoFrame);
        editorScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        editorScrollArea->setWidget(m_tab);
        rootGrid->addWidget(editorScrollArea, 3, 0, 1, 2);
    }

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
    // Make the fixture tree row stretch to fill available space
    QGridLayout *layout = qobject_cast<QGridLayout*>(General->layout());
    if (layout)
        layout->setRowStretch(1, 1);  // Row 1 contains m_tree
    m_tree->setMinimumHeight(0);
    m_tree->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    
    // Doc
    connect(m_doc, SIGNAL(fixtureRemoved(quint32)), this, SLOT(slotFixtureRemoved()));
    connect(m_doc, SIGNAL(fixtureChanged(quint32)), this, SLOT(slotFixtureChanged()));
    connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)), this, SLOT(slotFixtureGroupRemoved(quint32)));
    connect(m_doc, SIGNAL(fixtureGroupChanged(quint32)), this, SLOT(slotFixtureGroupUpdated(quint32)));

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
    connect(m_copyButton, SIGNAL(clicked()),
            this, SLOT(slotCopyClicked()));
    connect(m_pasteButton, SIGNAL(clicked()),
            this, SLOT(slotPasteClicked()));

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
    connect(m_applyOffsetsButton, SIGNAL(clicked()),
            this, SLOT(slotApplyOffsetsClicked()));
    connect(m_autoApplyOffsetsCheck, SIGNAL(toggled(bool)),
            this, SLOT(slotAutoApplyOffsetsToggled(bool)));

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

    updateOffsetTemplateControls();
}

void EFXEditor::initMovementPage()
{
    new QHBoxLayout(m_previewFrame);
    m_previewArea = new EFXPreviewArea(m_previewFrame);
    m_previewFrame->layout()->setContentsMargins(0, 0, 0, 0);
    m_previewFrame->layout()->addWidget(m_previewArea);

    /* line_3 separator conflicts with m_waveShapeLabel/Combo at the same grid
       row 15 in m_parametersGroup — hide it to remove the crossing line */
    line_3->hide();

    /* Give the parameters group room to breathe — without row stretch the grid
       compresses all ~20 parameter rows into too little height, causing overlap */
    QGridLayout *outerGrid = qobject_cast<QGridLayout*>(Movement->layout());
    if (outerGrid)
    {
        outerGrid->setRowStretch(1, 1);

        /* Move Color Background out of the crowded Parameters group and into
           the outer layout as a full-width item at the bottom of the tab */
        QGridLayout *paramGrid = qobject_cast<QGridLayout*>(m_parametersGroup->layout());
        if (paramGrid)
            paramGrid->removeWidget(m_colorCheck);
        outerGrid->addWidget(m_colorCheck, 3, 0, 1, 3);
    }

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
    m_waveLengthSpin->setValue(m_efx->waveLength());
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
    connect(m_waveLengthSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotWaveLengthSpinChanged(int)));
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
            
            // Apply stored column modes to the corresponding fixtures before creating UI items
            for (int col = 0; col < gridWidth; col++)
            {
                if (columnFixtures.contains(col) == false)
                    continue;

                int modeValue = m_efx->columnMode(col);
                if (modeValue < EFXFixture::PanTilt || modeValue > EFXFixture::RGB)
                    modeValue = EFXFixture::PanTilt;

                EFXFixture::Mode mode = static_cast<EFXFixture::Mode>(modeValue);
                foreach (EFXFixture *ef, columnFixtures.value(col))
                    ef->forceMode(mode);
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
                    
                    if (m_efx->columnDirection(col) != firstFixture->direction())
                        m_efx->setColumnDirection(col, firstFixture->direction());
                    
                    updateModeColumn(item, firstFixture);
                    updateStartOffsetColumn(item, firstFixture);
                }
                else
                {
                    // Empty column - show with default values (can be used as template)
                    Function::Direction storedDir = m_efx->columnDirection(col);
                    item->setCheckState(KColumnReverse,
                                        storedDir == Function::Backward ? Qt::Checked : Qt::Unchecked);
                    
                    // Create widgets with defaults for empty columns (enabled for template use)
                    // Mode combo
                    QComboBox* combo = new QComboBox(m_tree);
                    combo->setAutoFillBackground(true);
                    // For empty columns, always show all available modes
                    // This allows user to select any mode regardless of fixture capabilities
                    combo->addItem("Position");
                    combo->addItem("Dimmer");
                    combo->addItem("RGB");
                    combo->setProperty(PROPERTY_FIXTURE, col);
                    // Note: widgets are enabled so user can set template values
                    // but they won't affect anything until fixtures are added to this column
                    
                    // Connect signal so Mode changes are saved to backend
                    connect(combo, SIGNAL(currentIndexChanged(int)),
                            this, SLOT(slotFixtureModeChanged(int)));
                    
                    m_tree->setItemWidget(item, KColumnMode, combo);
                    
                    // Restore mode for this column (backend first, then UI cache)
                    int backendMode = m_efx->columnMode(col);
                    if (backendMode < EFXFixture::PanTilt || backendMode > EFXFixture::RGB)
                        backendMode = EFXFixture::PanTilt;

                    QString backendModeStr = EFXFixture::modeToString(static_cast<EFXFixture::Mode>(backendMode));
                    int backendIdx = combo->findText(backendModeStr);
                    if (backendIdx >= 0)
                    {
                        combo->setCurrentIndex(backendIdx);
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
                    int defaultOffset = m_efx->hasColumnOffset(col)
                        ? m_efx->columnOffset(col)
                        : calculateColumnOffset(col, 0, gridWidth, gridHeight);
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
        // Always show all modes, regardless of fixture capabilities
        combo->addItem("Position");
        combo->addItem("Dimmer");
        combo->addItem("RGB");
        
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

    QString desiredMode = ef->modeToString(ef->mode());

    if (m_efx->isFixtureGroupMode())
    {
        int columnIndex = item->data(0, Qt::UserRole).toInt();
        const QMap<int, int> storedModes = m_efx->columnModes();
        if (storedModes.contains(columnIndex))
        {
            desiredMode = EFXFixture::modeToString(static_cast<EFXFixture::Mode>(storedModes.value(columnIndex)));
        }
    }

    const int index = combo->findText(desiredMode);
    if (index >= 0)
    {
        QSignalBlocker blocker(combo);
        combo->setCurrentIndex(index);
    }

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
        spin->setSuffix(QChar(0x00b0)); // degree

        if (m_efx->isFixtureGroupMode())
        {
            // In group mode, store column index
            int columnIndex = item->data(0, Qt::UserRole).toInt();
            spin->setProperty(PROPERTY_FIXTURE, columnIndex);
            int value = m_efx->hasColumnOffset(columnIndex) ?
                m_efx->columnOffset(columnIndex) : ef->startOffset();
            spin->setValue(value);
        }
        else
        {
            // Normal mode, store EFXFixture pointer
            spin->setProperty(PROPERTY_FIXTURE, (qulonglong) ef);
            spin->setValue(ef->startOffset());
        }
        
        m_tree->setItemWidget(item, KColumnStartOffset, spin);
        
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

            m_efx->setColumnDirection(columnIndex, dir);
            
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
                    ef->forceMode(mode);
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
        bool anyFixtureUpdated = false;
        
        foreach (EFXFixture *ef, m_efx->fixtures())
        {
            // Check if this fixture is in the column
            for (int row = 0; row < gridHeight; row++)
            {
                GroupHead head = group->head(QLCPoint(columnIndex, row));
                if (head.isValid() && head.fxi == ef->head().fxi && head.head == ef->head().head)
                {
                    ef->setStartOffset(startOffset);
                    anyFixtureUpdated = true;
                    break;
                }
            }
        }

        m_efx->setColumnOffset(columnIndex, startOffset);

        if (!anyFixtureUpdated)
            m_efx->markOffsetTemplateDirty();
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

void EFXEditor::slotCopyClicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Copy EFX Settings"));
    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QCheckBox *chkFixtures = new QCheckBox(tr("Fixtures"), &dlg);
    QCheckBox *chkGroupMode = new QCheckBox(tr("Fixture Group Mode"), &dlg);
    QCheckBox *chkRowSelection = new QCheckBox(tr("Row Selection"), &dlg);
    QCheckBox *chkFixtureOrder = new QCheckBox(tr("Fixture Order"), &dlg);
    QCheckBox *chkPattern = new QCheckBox(tr("Pattern"), &dlg);
    QCheckBox *chkParameters = new QCheckBox(tr("Parameters"), &dlg);
    QCheckBox *chkDirection = new QCheckBox(tr("Direction"), &dlg);
    QCheckBox *chkRunOrder = new QCheckBox(tr("Run Order"), &dlg);

    chkFixtures->setChecked(true);
    chkGroupMode->setChecked(true);
    chkRowSelection->setChecked(true);
    chkFixtureOrder->setChecked(true);
    chkPattern->setChecked(true);
    chkParameters->setChecked(true);
    chkDirection->setChecked(true);
    chkRunOrder->setChecked(true);

    layout->addWidget(chkFixtures);
    layout->addWidget(chkGroupMode);
    layout->addWidget(chkRowSelection);
    layout->addWidget(chkFixtureOrder);
    layout->addWidget(chkPattern);
    layout->addWidget(chkParameters);
    layout->addWidget(chkDirection);
    layout->addWidget(chkRunOrder);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QJsonObject root;
    root["type"] = QString("qlc_efx_settings");
    root["version"] = 2;

    if (chkFixtures->isChecked())
    {
        QJsonArray fixtureSettings;
        foreach (EFXFixture *ef, m_efx->fixtures())
        {
            QJsonObject fixture;
            fixture["fixtureId"] = static_cast<int>(ef->head().fxi);
            fixture["headIndex"] = ef->head().head;
            fixture["startOffset"] = ef->startOffset();
            fixture["direction"] = static_cast<int>(ef->direction());
            fixture["mode"] = static_cast<int>(ef->mode());
            fixtureSettings.append(fixture);
        }
        root["fixtures"] = fixtureSettings;
    }

    if (chkGroupMode->isChecked())
    {
        QJsonObject gm;
        gm["offsetDirection"] = static_cast<int>(m_efx->offsetDirection());
        gm["offsetStep"] = m_efx->offsetStep();
        gm["wings"] = m_efx->wings();

        QJsonObject columnOffsets;
        QJsonObject columnModes;
        QJsonObject columnDirections;

        FixtureGroup *group = m_doc->fixtureGroup(m_efx->fixtureGroupID());
        if (group != nullptr)
        {
            int gridWidth = group->size().width();
            int gridHeight = group->size().height();

            for (int col = 0; col < gridWidth; col++)
            {
                EFXFixture *representative = nullptr;
                for (int row = 0; row < gridHeight && representative == nullptr; row++)
                {
                    GroupHead head = group->head(QLCPoint(col, row));
                    if (!head.isValid())
                        continue;
                    foreach (EFXFixture *ef, m_efx->fixtures())
                    {
                        if (ef->head().fxi == head.fxi && ef->head().head == head.head)
                        {
                            representative = ef;
                            break;
                        }
                    }
                }

                if (representative != nullptr)
                {
                    int offset = m_efx->hasColumnOffset(col)
                        ? m_efx->columnOffset(col)
                        : representative->startOffset();
                    columnOffsets[QString::number(col)] = offset;
                    columnModes[QString::number(col)] = static_cast<int>(representative->mode());
                    columnDirections[QString::number(col)] = static_cast<int>(representative->direction());
                }
                else
                {
                    columnOffsets[QString::number(col)] = m_efx->columnOffset(col) >= 0
                        ? m_efx->columnOffset(col) : 0;
                    columnModes[QString::number(col)] = m_efx->columnMode(col);
                    columnDirections[QString::number(col)] = static_cast<int>(m_efx->columnDirection(col));
                }
            }
        }

        gm["columnOffsets"] = columnOffsets;
        gm["columnModes"] = columnModes;
        gm["columnDirections"] = columnDirections;

        root["fixtureGroupMode"] = gm;
    }

    if (chkRowSelection->isChecked())
    {
        QJsonArray rows;
        foreach (int r, m_efx->selectedRows())
            rows.append(r);
        root["rowSelection"] = rows;
    }

    if (chkFixtureOrder->isChecked())
    {
        root["fixtureOrder"] = EFX::propagationModeToString(m_efx->propagationMode());
    }

    if (chkPattern->isChecked())
    {
        root["pattern"] = EFX::algorithmToString(m_efx->algorithm());
    }

    if (chkParameters->isChecked())
    {
        QJsonObject params;
        params["width"] = m_efx->width();
        params["height"] = m_efx->height();
        params["xOffset"] = m_efx->xOffset();
        params["yOffset"] = m_efx->yOffset();
        params["rotation"] = m_efx->rotation();
        params["startOffset"] = m_efx->startOffset();
        params["isRelative"] = m_efx->isRelative();
        params["xFrequency"] = m_efx->xFrequency();
        params["yFrequency"] = m_efx->yFrequency();
        params["xPhase"] = m_efx->xPhase();
        params["yPhase"] = m_efx->yPhase();
        params["waveWidth"] = m_efx->waveWidth();
        params["waveLength"] = m_efx->waveLength();
        params["waveShape"] = m_efx->waveShape();
        params["waveFadeIn"] = m_efx->waveFadeIn();
        params["waveFadeOut"] = m_efx->waveFadeOut();
        root["parameters"] = params;
    }

    if (chkDirection->isChecked())
    {
        root["direction"] = static_cast<int>(m_efx->direction());
    }

    if (chkRunOrder->isChecked())
    {
        root["runOrder"] = static_cast<int>(m_efx->runOrder());
    }

    QJsonDocument doc(root);
    QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Compact));
}

void EFXEditor::slotPasteClicked()
{
    QString clipboardText = QApplication::clipboard()->text();
    if (clipboardText.isEmpty())
    {
        QMessageBox::warning(this, tr("Paste Error"),
                             tr("Clipboard is empty."));
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(clipboardText.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        QMessageBox::warning(this, tr("Paste Error"),
                             tr("Invalid clipboard data: %1").arg(error.errorString()));
        return;
    }

    QJsonObject root = doc.object();
    if (root["type"].toString() != "qlc_efx_settings")
    {
        QMessageBox::warning(this, tr("Paste Error"),
                             tr("Clipboard does not contain EFX settings."));
        return;
    }

    bool running = interruptRunning();

    m_efx->applySettingsFromJson(root, m_doc);

    /* Refresh UI from model state */

    /* Pattern */
    m_algorithmCombo->setCurrentIndex(static_cast<int>(m_efx->algorithm()));

    /* Parameters */
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
    m_waveLengthSpin->setValue(m_efx->waveLength());
    m_waveShapeCombo->setCurrentIndex(m_efx->waveShape());
    m_waveFadeInSpin->setValue(m_efx->waveFadeIn());
    m_waveFadeOutSpin->setValue(m_efx->waveFadeOut());

    /* Fixture Order */
    switch (m_efx->propagationMode())
    {
        default:
        case EFX::Parallel:   m_parallelRadio->setChecked(true); break;
        case EFX::Serial:     m_serialRadio->setChecked(true); break;
        case EFX::Asymmetric: m_asymmetricRadio->setChecked(true); break;
    }

    /* Run Order */
    switch (m_efx->runOrder())
    {
        default:
        case Function::Loop:       m_loop->setChecked(true); break;
        case Function::PingPong:   m_pingPong->setChecked(true); break;
        case Function::SingleShot: m_singleShot->setChecked(true); break;
    }

    /* Direction */
    switch (m_efx->direction())
    {
        default:
        case Function::Forward:  m_forward->setChecked(true); break;
        case Function::Backward: m_backward->setChecked(true); break;
    }

    /* Row selection */
    updateRowSelection();

    updateFixtureTree();
    redrawPreview();
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
    updateOffsetTemplateControls();
}

void EFXEditor::slotFixtureGroupUpdated(quint32 id)
{
    updateFixtureGroupCombo();

    if (!m_useFixtureGroupCheck->isChecked() || !m_efx->isFixtureGroupMode())
        return;

    if (m_efx->fixtureGroupID() != id)
        return;

    bool running = interruptRunning();
    updateFixtureTree();
    updateRowSelection();
    redrawPreview();
    continueRunning(running);
    updateOffsetTemplateControls();
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

    // Disconnect checkboxes — they live inside the scroll area's child widget,
    // which gets deleted by the takeAt loop below (cascade via Qt parent ownership)
    foreach (QCheckBox *cb, m_rowCheckboxes)
    {
        cb->blockSignals(true);
        cb->disconnect();
    }
    m_rowCheckboxes.clear();

    // Remove and delete all items from the layout (scroll area or placeholder label)
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

    // Build checkboxes in a multi-column grid inside a scroll area so the
    // window height does not grow when many rows are present
    const int NUM_COLS = 3;

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMaximumHeight(120);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *container = new QWidget();
    QGridLayout *grid = new QGridLayout(container);
    grid->setSpacing(4);
    grid->setContentsMargins(2, 2, 2, 2);

    QList<int> currentSelection = m_efx->selectedRows();
    for (int row = 0; row < gridHeight; row++)
    {
        QCheckBox *cb = new QCheckBox(QString("Row %1").arg(row + 1));
        cb->blockSignals(true);
        cb->setChecked(currentSelection.isEmpty() || currentSelection.contains(row));
        cb->setProperty("row_index", row);

        grid->addWidget(cb, row / NUM_COLS, row % NUM_COLS);
        m_rowCheckboxes.append(cb);

        cb->blockSignals(false);
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(slotRowSelectionChanged()));
    }

    scrollArea->setWidget(container);
    m_rowSelectionLayout->addWidget(scrollArea);

    m_rowSelectionGroup->blockSignals(wasBlocked);
}

void EFXEditor::updateOffsetTemplateControls()
{
    bool groupMode = m_useFixtureGroupCheck->isChecked() && m_efx->isFixtureGroupMode();

    m_applyOffsetsButton->setEnabled(groupMode && m_efx->isOffsetTemplateDirty());
    m_autoApplyOffsetsCheck->setEnabled(groupMode);

    QSignalBlocker blocker(m_autoApplyOffsetsCheck);
    m_autoApplyOffsetsCheck->setChecked(m_efx->autoApplyOffsetTemplate());
}

void EFXEditor::restoreColumnProperties(EFXFixture *ef, int columnIndex)
{
    if (ef == NULL)
        return;

    EFXFixture::Mode columnMode = (EFXFixture::Mode)m_efx->columnMode(columnIndex);
    ef->forceMode(columnMode);

    Function::Direction columnDir = m_efx->columnDirection(columnIndex);
    ef->setDirection(columnDir);
}

void EFXEditor::rebuildFixturesForGroup(quint32 groupId, bool preserveOffsets)
{
    Q_UNUSED(groupId);
    m_efx->rebuildFixtureGroup(preserveOffsets);
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
    
    if (m_useFixtureGroupCheck->isChecked())
    {
        bool running = interruptRunning();
        rebuildFixturesForGroup(m_efx->fixtureGroupID(), !m_efx->autoApplyOffsetTemplate());
        updateFixtureTree();  // Only update tree, NOT row selection!
        redrawPreview();
        continueRunning(running);
    }

    updateOffsetTemplateControls();
    updating = false;
}

void EFXEditor::slotUseFixtureGroupToggled(bool checked)
{
    m_fixtureGroupCombo->setEnabled(checked);
    m_offsetDirectionCombo->setEnabled(checked);
    m_offsetStepSpin->setEnabled(checked);
    m_wingsSpin->setEnabled(checked);
    m_autoApplyOffsetsCheck->setEnabled(checked);
    m_applyOffsetsButton->setEnabled(checked && m_efx->isOffsetTemplateDirty());

    if (checked && m_fixtureGroupCombo->count() > 0)
    {
        if (!m_fixtureGroupCombo->currentData().isValid())
            return;

        quint32 groupId = m_fixtureGroupCombo->currentData().toUInt();
        if (groupId == FixtureGroup::invalidId())
            return;

        m_efx->setFixtureGroupID(groupId);
        rebuildFixturesForGroup(groupId, !m_efx->autoApplyOffsetTemplate());
    }
    else
    {
        m_efx->setFixtureGroupID(FixtureGroup::invalidId());
        m_efx->removeAllFixtures();
    }

    updateFixtureTree();
    updateRowSelection();
    redrawPreview();
    updateOffsetTemplateControls();
}

void EFXEditor::slotApplyOffsetsClicked()
{
    if (!m_useFixtureGroupCheck->isChecked() || !m_efx->isFixtureGroupMode())
        return;

    bool running = interruptRunning();
    m_efx->applyOffsetTemplate();
    updateFixtureTree();
    redrawPreview();
    continueRunning(running);
    updateOffsetTemplateControls();
}

void EFXEditor::slotAutoApplyOffsetsToggled(bool checked)
{
    m_efx->setAutoApplyOffsetTemplate(checked);

    if (m_useFixtureGroupCheck->isChecked() && m_efx->isFixtureGroupMode() && checked)
    {
        bool running = interruptRunning();
        m_efx->applyOffsetTemplate();
        updateFixtureTree();
        redrawPreview();
        continueRunning(running);
    }

    updateOffsetTemplateControls();
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
    
    bool running = interruptRunning();
    rebuildFixturesForGroup(groupId, !m_efx->autoApplyOffsetTemplate());
    updateFixtureTree();
    updateRowSelection();
    redrawPreview();
    continueRunning(running);
    updateOffsetTemplateControls();
}

int EFXEditor::calculateColumnOffset(int col, int row, int gridWidth, int gridHeight)
{
    int stored = m_efx->columnOffset(col);
    if (stored >= 0)
        return stored;

    return m_efx->calculateTemplateOffset(col, row, gridWidth, gridHeight);
}

void EFXEditor::slotOffsetDirectionChanged(int index)
{
    m_efx->setOffsetDirection((EFX::OffsetDirection)index);

    if (m_efx->isFixtureGroupMode() && m_efx->autoApplyOffsetTemplate())
    {
        bool running = interruptRunning();
        updateFixtureTree();
        redrawPreview();
        continueRunning(running);
    }

    updateOffsetTemplateControls();
}

void EFXEditor::slotOffsetStepChanged(int value)
{
    m_efx->setOffsetStep(value);
    
    if (m_efx->isFixtureGroupMode() && m_efx->autoApplyOffsetTemplate())
    {
        bool running = interruptRunning();
        updateFixtureTree();
        redrawPreview();
        continueRunning(running);
    }

    updateOffsetTemplateControls();
}

void EFXEditor::slotWingsChanged(int value)
{
    m_efx->setWings(value);
    
    if (m_efx->isFixtureGroupMode() && m_efx->autoApplyOffsetTemplate())
    {
        bool running = interruptRunning();
        updateFixtureTree();
        redrawPreview();
        continueRunning(running);
    }

    updateOffsetTemplateControls();
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
        m_waveLengthLabel->setEnabled(true);
        m_waveLengthSpin->setEnabled(true);
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
        m_waveLengthLabel->setEnabled(false);
        m_waveLengthSpin->setEnabled(false);
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

void EFXEditor::slotWaveLengthSpinChanged(int value)
{
    Q_ASSERT(m_efx != NULL);
    m_efx->setWaveLength(value);
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


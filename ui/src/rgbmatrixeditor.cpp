/*
  Q Light Controller
  rgbmatrixeditor.cpp

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

#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsEffect>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QGradient>
#include <QSettings>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QMutex>

#include <QApplication>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QToolButton>

#include "fixtureselection.h"
#include "speeddialwidget.h"
#include "rgbmatrixeditor.h"
#include "qlcfixturehead.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "fixturegroup.h"
#include "fixture.h"
#include "qlcmacros.h"
#include "rgbimage.h"
#include "sequence.h"
#include "rgbitem.h"
#include "rgbtext.h"
#include "scene.h"

#define SETTINGS_GEOMETRY "rgbmatrixeditor/geometry"
#define RECT_SIZE 30
#define RECT_PADDING 0
#define ITEM_SIZE 28
#define ITEM_PADDING 2

/****************************************************************************
 * Initialization
 ****************************************************************************/

RGBMatrixEditor::RGBMatrixEditor(QWidget* parent, RGBMatrix* mtx, Doc* doc)
    : QWidget(parent)
    , m_doc(doc)
    , m_matrix(mtx)
    , m_previewHandler(new RGBMatrixStep())
    , m_speedDials(NULL)
    , m_scene(new QGraphicsScene(this))
    , m_previewTimer(new QTimer(this))
    , m_previewIterator(0)
    , m_channelMappingGroup(NULL)
    , m_channelMappingLayout(NULL)
{
    Q_ASSERT(doc != NULL);
    Q_ASSERT(mtx != NULL);

    setupUi(this);

    // Set a nice gradient backdrop
    m_scene->setBackgroundBrush(Qt::darkGray);
    QLinearGradient gradient(200, 200, 200, 2000);
    gradient.setSpread(QGradient::ReflectSpread);
    m_scene->setBackgroundBrush(gradient);

    connect(m_previewTimer, SIGNAL(timeout()), this, SLOT(slotPreviewTimeout()));
    connect(m_doc, SIGNAL(modeChanged(Doc::Mode)), this, SLOT(slotModeChanged(Doc::Mode)));
    connect(m_doc, SIGNAL(fixtureGroupAdded(quint32)), this, SLOT(slotFixtureGroupAdded()));
    connect(m_doc, SIGNAL(fixtureGroupRemoved(quint32)), this, SLOT(slotFixtureGroupRemoved()));
    connect(m_doc, SIGNAL(fixtureGroupChanged(quint32)), this, SLOT(slotFixtureGroupChanged(quint32)));

    init();

    slotModeChanged(m_doc->mode());

    // Set focus to the editor
    m_nameEdit->setFocus();
}

RGBMatrixEditor::~RGBMatrixEditor()
{
    m_previewTimer->stop();

    if (m_testButton->isChecked() == true)
        m_matrix->stopAndWait();

    delete m_previewHandler;

    qDeleteAll(m_previewHash);
}

void RGBMatrixEditor::stopTest()
{
    if (m_testButton->isChecked() == true)
        m_testButton->click();
}

void RGBMatrixEditor::slotFunctionManagerActive(bool active)
{
    if (active == true)
    {
        if (m_speedDials == NULL)
            updateSpeedDials();
    }
    else
    {
        if (m_speedDials != NULL)
            m_speedDials->deleteLater();
        m_speedDials = NULL;
    }
}

void RGBMatrixEditor::init()
{
    QSizePolicy sp = scrollArea->widget()->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Ignored);
    scrollArea->widget()->setSizePolicy(sp);

    /* Name */
    m_nameEdit->setText(m_matrix->name());
    m_nameEdit->setSelection(0, m_matrix->name().length());

    /* Running order */
    switch (m_matrix->runOrder())
    {
        default:
        case Function::Loop:
            m_loop->setChecked(true);
        break;
        case Function::PingPong:
            m_pingPong->setChecked(true);
        break;
        case Function::SingleShot:
            m_singleShot->setChecked(true);
        break;
    }

    /* Running direction */
    switch (m_matrix->direction())
    {
        default:
        case Function::Forward:
            m_forward->setChecked(true);
        break;
        case Function::Backward:
            m_backward->setChecked(true);
        break;
    }


    /* Blend mode */
    m_blendModeCombo->setCurrentIndex(m_matrix->blendMode());

    /* Color mode */
    m_controlModeCombo->setCurrentIndex(m_matrix->controlMode());

    /* Dimmer control */
    if (m_matrix->dimmerControl())
        m_dimmerControlCb->setChecked(m_matrix->dimmerControl());
    else
        m_intensityGroup->hide();

    fillPatternCombo();
    fillFixtureGroupCombo();
    fillAnimationCombo();
    fillImageAnimationCombo();

    QPixmap pm(50, 26);
    pm.fill(m_matrix->getColor(0));
    m_mtxColor1Button->setIcon(QIcon(pm));

    if (m_matrix->getColor(1).isValid())
        pm.fill(m_matrix->getColor(1));
    else
        pm.fill(Qt::transparent);
    m_mtxColor2Button->setIcon(QIcon(pm));

    if (m_matrix->getColor(2).isValid())
        pm.fill(m_matrix->getColor(2));
    else
        pm.fill(Qt::transparent);
    m_mtxColor3Button->setIcon(QIcon(pm));

    if (m_matrix->getColor(3).isValid())
        pm.fill(m_matrix->getColor(3));
    else
        pm.fill(Qt::transparent);
    m_mtxColor4Button->setIcon(QIcon(pm));

    if (m_matrix->getColor(4).isValid())
        pm.fill(m_matrix->getColor(4));
    else
        pm.fill(Qt::transparent);
    m_mtxColor5Button->setIcon(QIcon(pm));

    updateExtraOptions();
    updateSpeedDials();

    // Set column stretch for main grid layout (70% left, 30% right)
    QGridLayout *mainGrid = qobject_cast<QGridLayout*>(this->layout());
    if (mainGrid != NULL)
    {
        // Columns 0-3 (left side, preview + controls) get 70% total
        mainGrid->setColumnStretch(0, 18);  // 18/25 ≈ 70%
        mainGrid->setColumnStretch(1, 18);
        mainGrid->setColumnStretch(2, 17);
        mainGrid->setColumnStretch(3, 17);
        // Column 4 (right side, pattern + properties) gets 30%
        mainGrid->setColumnStretch(4, 30);  // 30/100 = 30%
    }
    
    // Find the bottom grid layout (gridLayout_6 from .ui file)
    QGridLayout *bottomGrid = this->findChild<QGridLayout*>("gridLayout_6");
    
    // Create row filtering UI (horizontal for space efficiency, scrollable)
    m_rowSelectionGroup = new QGroupBox(tr("Row Filter"));
    
    // Create scroll area for checkboxes (in case of many rows)
    QScrollArea *rowScrollArea = new QScrollArea(m_rowSelectionGroup);
    rowScrollArea->setWidgetResizable(true);
    rowScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    rowScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rowScrollArea->setMaximumHeight(60);  // Limit height for scrolling
    
    QWidget *rowScrollWidget = new QWidget();
    m_rowSelectionLayout = new QHBoxLayout(rowScrollWidget);  // HORIZONTAL!
    m_rowSelectionLayout->setContentsMargins(0, 0, 0, 0);
    rowScrollWidget->setLayout(m_rowSelectionLayout);
    rowScrollArea->setWidget(rowScrollWidget);
    
    QVBoxLayout *groupLayout = new QVBoxLayout(m_rowSelectionGroup);
    groupLayout->addWidget(rowScrollArea);
    m_rowSelectionGroup->setLayout(groupLayout);
    m_rowSelectionGroup->setVisible(false);
    m_rowSelectionGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    
    // Create multi-value mapping enable checkbox
    m_enablePerFixtureMappingCheck = new QCheckBox(tr("Enable Multi-Value Mapping"));
    m_enablePerFixtureMappingCheck->setChecked(m_matrix->enablePerFixtureMapping());
    m_enablePerFixtureMappingCheck->setToolTip(tr("Enable per-fixture-type parameter mapping (for scripts with multiple rows/parameters)"));

    // Create Override HTP checkbox
    m_overrideHTPCb = new QCheckBox(tr("Override HTP"));
    m_overrideHTPCb->setChecked(m_matrix->overrideHTP());
    m_overrideHTPCb->setToolTip(tr("Force this RGB Matrix to override HTP channels – the script value always wins, even if the channel is set to a higher value from another source"));

    // Create Zero Is Transparent checkbox
    m_zeroIsTransparentCb = new QCheckBox(tr("Zero Is Transparent"));
    m_zeroIsTransparentCb->setChecked(m_matrix->zeroIsTransparent());
    m_zeroIsTransparentCb->setToolTip(tr("When enabled, any value of 0 produced by the script releases the channel so other functions can take control. "
                                          "Useful together with Override HTP for scripts that output 0 in idle state."));

    // Create per-definition channel mapping UI
    m_channelMappingGroup = new QGroupBox(tr("Per-Fixture Channel Mapping"));
    m_channelMappingLayout = new QVBoxLayout(m_channelMappingGroup);
    m_channelMappingGroup->setLayout(m_channelMappingLayout);
    m_channelMappingGroup->setVisible(false);
    m_channelMappingGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    
    // Add to bottom grid layout (under Run Order & Direction)
    if (bottomGrid != NULL)
    {
        // gridLayout_6 structure:
        // Row 0: Run Order (col 0) + Direction (col 1)
        // Row 1: Row Filter (col 0-2, span 3)
        // Row 2: Enable Multi-Value (col 0) | Override HTP (col 1) | Zero Is Transparent (col 2)
        // Row 3: Per-Fixture Channel Mapping (col 0-2, span 3)
        bottomGrid->addWidget(m_rowSelectionGroup, 1, 0, 1, 3);
        bottomGrid->addWidget(m_enablePerFixtureMappingCheck, 2, 0);
        bottomGrid->addWidget(m_overrideHTPCb, 2, 1);
        bottomGrid->addWidget(m_zeroIsTransparentCb, 2, 2);
        bottomGrid->addWidget(m_channelMappingGroup, 3, 0, 1, 3);
    }
    
    connect(m_enablePerFixtureMappingCheck, SIGNAL(toggled(bool)),
            this, SLOT(slotEnablePerFixtureMappingToggled(bool)));
    connect(m_overrideHTPCb, SIGNAL(clicked()),
            this, SLOT(slotOverrideHTPClicked()));
    connect(m_zeroIsTransparentCb, SIGNAL(clicked()),
            this, SLOT(slotZeroIsTransparentClicked()));
    
    updateChannelMappingUI();
    updateRowSelection();

    connect(m_nameEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(slotNameEdited(const QString&)));
    connect(m_speedDialButton, SIGNAL(toggled(bool)),
            this, SLOT(slotSpeedDialToggle(bool)));
    connect(m_saveToSequenceButton, SIGNAL(clicked()),
            this, SLOT(slotSaveToSequenceClicked()));
    connect(m_shapeButton, SIGNAL(toggled(bool)),
            this, SLOT(slotShapeToggle(bool)));
    connect(m_patternCombo, SIGNAL(activated(int)),
            this, SLOT(slotPatternActivated(int)));
    connect(m_fixtureGroupCombo, SIGNAL(activated(int)),
            this, SLOT(slotFixtureGroupActivated(int)));
    connect(m_blendModeCombo, SIGNAL(activated(int)),
            this, SLOT(slotBlendModeChanged(int)));
    connect(m_controlModeCombo, SIGNAL(activated(int)),
            this, SLOT(slotControlModeChanged(int)));
    connect(m_mtxColor1Button, SIGNAL(clicked()),
            this, SLOT(slotMtxColor1ButtonClicked()));
    connect(m_mtxColor2Button, SIGNAL(clicked()),
            this, SLOT(slotMtxColor2ButtonClicked()));
    connect(m_resetMtxColor2Button, SIGNAL(clicked()),
            this, SLOT(slotResetMtxColor2ButtonClicked()));
    connect(m_mtxColor3Button, SIGNAL(clicked()),
            this, SLOT(slotMtxColor3ButtonClicked()));
    connect(m_resetMtxColor3Button, SIGNAL(clicked()),
            this, SLOT(slotResetMtxColor3ButtonClicked()));
    connect(m_mtxColor4Button, SIGNAL(clicked()),
            this, SLOT(slotMtxColor4ButtonClicked()));
    connect(m_resetMtxColor4Button, SIGNAL(clicked()),
            this, SLOT(slotResetMtxColor4ButtonClicked()));
    connect(m_mtxColor5Button, SIGNAL(clicked()),
            this, SLOT(slotMtxColor5ButtonClicked()));
    connect(m_resetMtxColor5Button, SIGNAL(clicked()),
            this, SLOT(slotResetMtxColor5ButtonClicked()));
    connect(m_textEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(slotTextEdited(const QString&)));
    connect(m_fontButton, SIGNAL(clicked()),
            this, SLOT(slotFontButtonClicked()));
    connect(m_animationCombo, SIGNAL(activated(int)),
            this, SLOT(slotAnimationActivated(int)));
    connect(m_imageEdit, SIGNAL(editingFinished()),
            this, SLOT(slotImageEdited()));
    connect(m_imageButton, SIGNAL(clicked()),
            this, SLOT(slotImageButtonClicked()));
    connect(m_imageAnimationCombo, SIGNAL(activated(int)),
            this, SLOT(slotImageAnimationActivated(int)));
    connect(m_xOffsetSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotOffsetSpinChanged()));
    connect(m_yOffsetSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotOffsetSpinChanged()));

    connect(m_loop, SIGNAL(clicked()), this, SLOT(slotLoopClicked()));
    connect(m_pingPong, SIGNAL(clicked()), this, SLOT(slotPingPongClicked()));
    connect(m_singleShot, SIGNAL(clicked()), this, SLOT(slotSingleShotClicked()));
    connect(m_forward, SIGNAL(clicked()), this, SLOT(slotForwardClicked()));
    connect(m_backward, SIGNAL(clicked()), this, SLOT(slotBackwardClicked()));
    connect(m_dimmerControlCb, SIGNAL(clicked()), this, SLOT(slotDimmerControlClicked()));

    // Test slots
    connect(m_testButton, SIGNAL(clicked(bool)),
            this, SLOT(slotTestClicked()));

    // Copy/Paste
    connect(m_copyButton, SIGNAL(clicked()), this, SLOT(slotCopyClicked()));
    connect(m_pasteButton, SIGNAL(clicked()), this, SLOT(slotPasteClicked()));

    m_preview->setScene(m_scene);
    if (createPreviewItems() == true)
        m_previewTimer->start(MasterTimer::tick());
}

void RGBMatrixEditor::updateSpeedDials()
{
    if (m_speedDialButton->isChecked() == false)
        return;

    if (m_speedDials != NULL)
        return;

    m_speedDials = new SpeedDialWidget(this);
    m_speedDials->setAttribute(Qt::WA_DeleteOnClose);
    m_speedDials->setWindowTitle(m_matrix->name());
    m_speedDials->show();
    m_speedDials->setFadeInSpeed(m_matrix->fadeInSpeed());
    m_speedDials->setFadeOutSpeed(m_matrix->fadeOutSpeed());
    if ((int)m_matrix->duration() < 0)
        m_speedDials->setDuration(m_matrix->duration());
    else
        m_speedDials->setDuration(m_matrix->duration() - m_matrix->fadeInSpeed());
    connect(m_speedDials, SIGNAL(fadeInChanged(int)), this, SLOT(slotFadeInChanged(int)));
    connect(m_speedDials, SIGNAL(fadeOutChanged(int)), this, SLOT(slotFadeOutChanged(int)));
    connect(m_speedDials, SIGNAL(holdChanged(int)), this, SLOT(slotHoldChanged(int)));
    connect(m_speedDials, SIGNAL(holdTapped()), this, SLOT(slotDurationTapped()));
    connect(m_speedDials, SIGNAL(destroyed(QObject*)), this, SLOT(slotDialDestroyed(QObject*)));
}

void RGBMatrixEditor::fillPatternCombo()
{
    m_patternCombo->addItems(RGBAlgorithm::algorithms(m_doc));
    if (m_matrix->algorithm() != NULL)
    {
        int index = m_patternCombo->findText(m_matrix->algorithm()->name());
        if (index >= 0)
            m_patternCombo->setCurrentIndex(index);
    }
}

void RGBMatrixEditor::fillFixtureGroupCombo()
{
    m_fixtureGroupCombo->clear();
    m_fixtureGroupCombo->addItem(tr("None"));

    QListIterator <FixtureGroup*> it(m_doc->fixtureGroups());
    while (it.hasNext() == true)
    {
        FixtureGroup* grp(it.next());
        Q_ASSERT(grp != NULL);
        m_fixtureGroupCombo->addItem(grp->name(), grp->id());
        if (m_matrix->fixtureGroup() == grp->id())
            m_fixtureGroupCombo->setCurrentIndex(m_fixtureGroupCombo->count() - 1);
    }
}

void RGBMatrixEditor::fillAnimationCombo()
{
    m_animationCombo->addItems(RGBText::animationStyles());
}

void RGBMatrixEditor::fillImageAnimationCombo()
{
    m_imageAnimationCombo->addItems(RGBImage::animationStyles());
}

void RGBMatrixEditor::updateExtraOptions()
{
    resetProperties(m_propertiesLayout->layout());
    m_propertiesGroup->hide();

    if (m_matrix->algorithm() == NULL ||
        m_matrix->algorithm()->type() == RGBAlgorithm::Script ||
        m_matrix->algorithm()->type() == RGBAlgorithm::Audio)
    {
        m_textGroup->hide();
        m_imageGroup->hide();
        m_offsetGroup->hide();

        if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Script)
        {
            RGBScript *script = static_cast<RGBScript*> (m_matrix->algorithm());
            displayProperties(script);
        }
    }
    else if (m_matrix->algorithm()->type() == RGBAlgorithm::Plain)
    {
        m_textGroup->hide();
        m_imageGroup->hide();
        m_offsetGroup->hide();
    }
    else if (m_matrix->algorithm()->type() == RGBAlgorithm::Image)
    {
        m_textGroup->hide();
        m_imageGroup->show();
        m_offsetGroup->show();

        RGBImage *image = static_cast<RGBImage*> (m_matrix->algorithm());
        Q_ASSERT(image != NULL);
        m_imageEdit->setText(image->filename());

        int index = m_imageAnimationCombo->findText(RGBImage::animationStyleToString(image->animationStyle()));
        if (index != -1)
            m_imageAnimationCombo->setCurrentIndex(index);

        m_xOffsetSpin->setValue(image->xOffset());
        m_yOffsetSpin->setValue(image->yOffset());

    }
    else if (m_matrix->algorithm()->type() == RGBAlgorithm::Text)
    {
        m_textGroup->show();
        m_offsetGroup->show();
        m_imageGroup->hide();

        RGBText *text = static_cast<RGBText*> (m_matrix->algorithm());
        Q_ASSERT(text != NULL);
        m_textEdit->setText(text->text());

        int index = m_animationCombo->findText(RGBText::animationStyleToString(text->animationStyle()));
        if (index != -1)
            m_animationCombo->setCurrentIndex(index);

        m_xOffsetSpin->setValue(text->xOffset());
        m_yOffsetSpin->setValue(text->yOffset());
    }

    updateColorOptions();
}

void RGBMatrixEditor::updateColorOptions()
{
    if (m_matrix->algorithm() != NULL)
    {
        int accColors = m_matrix->algorithm()->acceptColors();

        m_mtxColor1Button->setVisible(accColors == 0 ? false : true);
        m_mtxColor2Button->setVisible(accColors > 1 ? true : false);
        m_resetMtxColor2Button->setVisible(accColors > 1 ? true : false);
        m_mtxColor3Button->setVisible(accColors > 2 ? true : false);
        m_resetMtxColor3Button->setVisible(accColors > 2 ? true : false);
        m_mtxColor4Button->setVisible(accColors > 3 ? true : false);
        m_resetMtxColor4Button->setVisible(accColors > 3 ? true : false);
        m_mtxColor5Button->setVisible(accColors > 4 ? true : false);
        m_resetMtxColor5Button->setVisible(accColors > 4 ? true : false);

        m_blendModeLabel->setVisible(accColors == 0 ? false : true);
        m_blendModeCombo->setVisible(accColors == 0 ? false : true);
    }
}

void RGBMatrixEditor::updateColors()
{
    if (m_matrix->algorithm() != NULL)
    {
        int accColors = m_matrix->algorithm()->acceptColors();
        if (accColors > 0)
        {
            if (m_matrix->blendMode() == Universe::MaskBlend)
            {
                m_matrix->setColor(0, Qt::white);
                // Overwrite more colors only if applied.
                if (accColors <= 2)
                    m_matrix->setColor(1, QColor());
                if (accColors <= 3)
                    m_matrix->setColor(2, QColor());
                if (accColors <= 4)
                    m_matrix->setColor(3, QColor());
                if (accColors <= 5)
                    m_matrix->setColor(4, QColor());

                m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
                        m_matrix->algorithm());

                QPixmap pm(50, 26);
                pm.fill(Qt::white);
                m_mtxColor1Button->setIcon(QIcon(pm));

                pm.fill(Qt::transparent);
                m_mtxColor2Button->setIcon(QIcon(pm));
                m_mtxColor3Button->setIcon(QIcon(pm));
                m_mtxColor4Button->setIcon(QIcon(pm));
                m_mtxColor5Button->setIcon(QIcon(pm));
            }
            else if (m_controlModeCombo->currentIndex() != RGBMatrix::ControlModeRgb)
            {
                // Convert color 1 to grayscale for single color modes
                uchar gray = qGray(m_matrix->getColor(0).rgb());
                m_matrix->setColor(0, QColor(gray, gray, gray));
                QPixmap pm(50, 26);
                pm.fill(QColor(gray, gray, gray));
                m_mtxColor1Button->setIcon(QIcon(pm));

                // Convert color 2 and following to grayscale for single color modes
                if (accColors < 2)
                    m_matrix->setColor(1, QColor());
                if (m_matrix->getColor(1) == QColor())
                {
                    pm.fill(Qt::transparent);
                }
                else
                {
                    gray = qGray(m_matrix->getColor(1).rgb());
                    m_matrix->setColor(1, QColor(gray, gray, gray));
                    pm.fill(QColor(gray, gray, gray));
                }
                m_mtxColor2Button->setIcon(QIcon(pm));

                if (accColors < 3)
                    m_matrix->setColor(2, QColor());
                if (m_matrix->getColor(2) == QColor())
                {
                    pm.fill(Qt::transparent);
                }
                else
                {
                    gray = qGray(m_matrix->getColor(2).rgb());
                    m_matrix->setColor(2, QColor(gray, gray, gray));
                    pm.fill(QColor(gray, gray, gray));
                }
                m_mtxColor3Button->setIcon(QIcon(pm));

                if (accColors < 4)
                    m_matrix->setColor(3, QColor());
                if (m_matrix->getColor(3) == QColor())
                {
                    pm.fill(Qt::transparent);
                }
                else
                {
                    gray = qGray(m_matrix->getColor(3).rgb());
                    m_matrix->setColor(3, QColor(gray, gray, gray));
                    pm.fill(QColor(gray, gray, gray));
                }
                m_mtxColor4Button->setIcon(QIcon(pm));

                if (accColors < 5)
                    m_matrix->setColor(4, QColor());
                if (m_matrix->getColor(4) == QColor())
                {
                    pm.fill(Qt::transparent);
                }
                else
                {
                    gray = qGray(m_matrix->getColor(4).rgb());
                    m_matrix->setColor(4, QColor(gray, gray, gray));
                    pm.fill(QColor(gray, gray, gray));
                }
                m_mtxColor5Button->setIcon(QIcon(pm));

                m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
                        m_matrix->algorithm());
            }
            else 
            {
                QPixmap pm(50, 26);
                pm.fill(m_matrix->getColor(0));
                m_mtxColor1Button->setIcon(QIcon(pm));

                // Preserve the colors (do not set them to QColor().
                // RGBMatrixStep::calculateColorDelta will ensure correct color application
                if (m_matrix->getColor(1) == QColor())
                    pm.fill(Qt::transparent);
                else
                    pm.fill(m_matrix->getColor(1));
                m_mtxColor2Button->setIcon(QIcon(pm));

                if (m_matrix->getColor(2) == QColor())
                    pm.fill(Qt::transparent);
                else
                    pm.fill(m_matrix->getColor(2));
                m_mtxColor3Button->setIcon(QIcon(pm));

                if (m_matrix->getColor(3) == QColor())
                    pm.fill(Qt::transparent);
                else
                    pm.fill(m_matrix->getColor(3));
                m_mtxColor4Button->setIcon(QIcon(pm));

                if (m_matrix->getColor(4) == QColor())
                    pm.fill(Qt::transparent);
                else
                    pm.fill(m_matrix->getColor(4));
                m_mtxColor5Button->setIcon(QIcon(pm));

                m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
                        m_matrix->algorithm());
            }
        }
    }
}

/**
 * Helper function. Deletes all child widgets of the given layout @a item.
 */
void RGBMatrixEditor::resetProperties(QLayoutItem *item)
{
    if (item->layout()) 
    {
        // Process all child items recursively.
        for (int i = item->layout()->count() - 1; i >= 0; i--)
            resetProperties(item->layout()->itemAt(i));
    }
    delete item->widget();
}

void RGBMatrixEditor::displayProperties(RGBScript *script)
{
    if (script == NULL)
        return;

    int gridRowIdx = 0;

    QList<RGBScriptProperty> properties = script->properties();
    if (properties.count() > 0)
        m_propertiesGroup->show();

    foreach (RGBScriptProperty prop, properties)
    {
        switch(prop.m_type)
        {
            case RGBScriptProperty::List:
            {
                QLabel *propLabel = new QLabel(prop.m_displayName);
                m_propertiesLayout->addWidget(propLabel, gridRowIdx, 0);
                QComboBox *propCombo = new QComboBox(this);
                propCombo->addItems(prop.m_listValues);
                propCombo->setProperty("pName", prop.m_name);
                connect(propCombo, SIGNAL(currentIndexChanged(int)),
                        this, SLOT(slotPropertyComboChanged(int)));
                m_propertiesLayout->addWidget(propCombo, gridRowIdx, 1);
                if (m_matrix != NULL)
                {
                    QString pValue = m_matrix->property(prop.m_name);
                    if (!pValue.isEmpty())
                    {
                        propCombo->setCurrentText(pValue);
                    }
                    else
                    {
                        pValue = script->property(prop.m_name);
                        if (!pValue.isEmpty())
                            propCombo->setCurrentText(pValue);
                    }
                }
                gridRowIdx++;
            }
            break;
            case RGBScriptProperty::Range:
            {
                QLabel *propLabel = new QLabel(prop.m_displayName);
                m_propertiesLayout->addWidget(propLabel, gridRowIdx, 0);
                QSpinBox *propSpin = new QSpinBox(this);
                propSpin->setRange(prop.m_rangeMinValue, prop.m_rangeMaxValue);
                propSpin->setProperty("pName", prop.m_name);
                connect(propSpin, SIGNAL(valueChanged(int)),
                        this, SLOT(slotPropertySpinChanged(int)));
                m_propertiesLayout->addWidget(propSpin, gridRowIdx, 1);
                if (m_matrix != NULL)
                {
                    QString pValue = m_matrix->property(prop.m_name);
                    if (!pValue.isEmpty())
                        propSpin->setValue(pValue.toInt());
                    else
                    {
                        pValue = script->property(prop.m_name);
                        if (!pValue.isEmpty())
                            propSpin->setValue(pValue.toInt());
                    }
                }
                gridRowIdx++;
            }
            break;
            case RGBScriptProperty::Float:
            {
                QLabel *propLabel = new QLabel(prop.m_displayName);
                m_propertiesLayout->addWidget(propLabel, gridRowIdx, 0);
                QDoubleSpinBox *propSpin = new QDoubleSpinBox(this);
                propSpin->setDecimals(3);
                propSpin->setRange(-1000000, 1000000);
                propSpin->setProperty("pName", prop.m_name);
                connect(propSpin, SIGNAL(valueChanged(double)),
                        this, SLOT(slotPropertyDoubleSpinChanged(double)));
                m_propertiesLayout->addWidget(propSpin, gridRowIdx, 1);
                if (m_matrix != NULL)
                {
                    QString pValue = m_matrix->property(prop.m_name);
                    if (!pValue.isEmpty())
                        propSpin->setValue(pValue.toDouble());
                    else
                    {
                        pValue = script->property(prop.m_name);
                        if (!pValue.isEmpty())
                            propSpin->setValue(pValue.toDouble());
                    }
                }
                gridRowIdx++;
            }
            break;
            case RGBScriptProperty::String:
            {
                QLabel *propLabel = new QLabel(prop.m_displayName);
                m_propertiesLayout->addWidget(propLabel, gridRowIdx, 0);
                QLineEdit *propEdit = new QLineEdit(this);
                propEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                propEdit->setProperty("pName", prop.m_name);
                connect(propEdit, SIGNAL(textEdited(QString)),
                        this, SLOT(slotPropertyEditChanged(QString)));
                m_propertiesLayout->addWidget(propEdit, gridRowIdx, 1);
                if (m_matrix != NULL)
                {
                    QString pValue = m_matrix->property(prop.m_name);
                    if (!pValue.isEmpty())
                        propEdit->setText(pValue);
                    else
                    {
                        pValue = script->property(prop.m_name);
                        if (!pValue.isEmpty())
                            propEdit->setText(pValue);
                    }
                }
                gridRowIdx++;
            }
            break;
            default:
                qWarning() << "Type" << prop.m_type << "not handled yet";
            break;
        }
    }
}

bool RGBMatrixEditor::createPreviewItems()
{
    qDeleteAll(m_previewHash);
    m_previewHash.clear();
    m_scene->clear();

    FixtureGroup* grp = m_doc->fixtureGroup(m_matrix->fixtureGroup());
    if (grp == NULL)
    {
        QGraphicsTextItem* text = new QGraphicsTextItem(tr("No fixture group to control"));
        text->setDefaultTextColor(Qt::white);
        m_scene->addItem(text);
        return false;
    }

    m_previewHandler->initializeDirection(m_matrix->direction(), m_matrix->getColor(0),
                                      m_matrix->getColor(1), m_matrix->stepsCount(),
                                      m_matrix->algorithm());

    m_matrix->previewMap(m_previewHandler->currentStepIndex(), m_previewHandler);

    if (m_previewHandler->m_map.isEmpty())
        return false;

    for (int x = 0; x < grp->size().width(); x++)
    {
        for (int y = 0; y < grp->size().height(); y++)
        {
            QLCPoint pt(x, y);

            if (grp->headsMap().contains(pt) == true)
            {
                RGBItem *item;
                if (m_shapeButton->isChecked() == false)
                {
                    QGraphicsEllipseItem* circleItem = new QGraphicsEllipseItem();
                    circleItem->setRect(
                            x * RECT_SIZE + RECT_PADDING + ITEM_PADDING,
                            y * RECT_SIZE + RECT_PADDING + ITEM_PADDING,
                            ITEM_SIZE - (2 * ITEM_PADDING),
                            ITEM_SIZE - (2 * ITEM_PADDING));
                    item = new RGBItem(circleItem);
                }
                else
                {
                    QGraphicsRectItem *rectItem = new QGraphicsRectItem();
                    rectItem->setRect(
                            x * RECT_SIZE + RECT_PADDING + ITEM_PADDING,
                            y * RECT_SIZE + RECT_PADDING + ITEM_PADDING,
                            ITEM_SIZE - (2 * ITEM_PADDING),
                            ITEM_SIZE - (2 * ITEM_PADDING));
                    item = new RGBItem(rectItem);
                }

                item->setColor(rgbwToPreviewColor(m_previewHandler->m_map[y][x]));
                item->draw(0, 0);
                m_scene->addItem(item->graphicsItem());
                m_previewHash[pt] = item;
            }
        }
    }
    return true;
}

uint RGBMatrixEditor::rgbwToPreviewColor(uint col) const
{
    if (m_controlModeCombo->currentIndex() != RGBMatrix::ControlModeRgbw)
        return col;

    // Normalized RGBW→RGB: add W to each channel, then scale so the
    // brightest channel stays at 255.  This preserves hue while reducing
    // saturation as W increases — e.g. G=255 W=255 → (127,255,127).
    int w = qAlpha(col);
    int r = qRed(col)   + w;
    int g = qGreen(col) + w;
    int b = qBlue(col)  + w;
    int maxCh = qMax(qMax(r, g), b);
    if (maxCh > 255)
    {
        double scale = 255.0 / maxCh;
        r = qRound(r * scale);
        g = qRound(g * scale);
        b = qRound(b * scale);
    }
    return qRgb(r, g, b);
}

void RGBMatrixEditor::slotPreviewTimeout()
{
    if (m_matrix->duration() <= 0)
        return;

    m_previewIterator += MasterTimer::tick();
    uint elapsed = 0;
    while (m_previewIterator >= MAX(m_matrix->duration(), MasterTimer::tick()))
    {
        m_previewHandler->checkNextStep(m_matrix->runOrder(), m_matrix->getColor(0),
                                        m_matrix->getColor(1), m_matrix->stepsCount());

        m_matrix->previewMap(m_previewHandler->currentStepIndex(), m_previewHandler);

        m_previewIterator -= MAX(m_matrix->duration(), MasterTimer::tick());
        elapsed += MAX(m_matrix->duration(), MasterTimer::tick());
    }
    for (int y = 0; y < m_previewHandler->m_map.size(); y++)
    {
        for (int x = 0; x < m_previewHandler->m_map[y].size(); x++)
        {
            QLCPoint pt(x, y);
            RGBItem* shape = m_previewHash.value(pt, NULL);
            if (shape)
            {
                uint displayCol = rgbwToPreviewColor(m_previewHandler->m_map[y][x]);
                if (shape->color() != QColor(displayCol).rgb())
                    shape->setColor(displayCol);

                if (shape->color() == QColor(Qt::black).rgb())
                    shape->draw(elapsed, m_matrix->fadeOutSpeed());
                else
                    shape->draw(elapsed, m_matrix->fadeInSpeed());
            }
        }
    }
}

void RGBMatrixEditor::slotNameEdited(const QString& text)
{
    m_matrix->setName(text);
    if (m_speedDials != NULL)
        m_speedDials->setWindowTitle(text);
}

void RGBMatrixEditor::slotSpeedDialToggle(bool state)
{
    if (state == true)
        updateSpeedDials();
    else
    {
        if (m_speedDials != NULL)
            m_speedDials->deleteLater();
        m_speedDials = NULL;
    }
}

void RGBMatrixEditor::slotDialDestroyed(QObject *)
{
    m_speedDialButton->setChecked(false);
}

void RGBMatrixEditor::slotPatternActivated(int patternIndex)
{
    QString algoName = m_patternCombo->itemText(patternIndex);
    RGBAlgorithm *algo = RGBAlgorithm::algorithm(m_doc, algoName);
    m_matrix->setAlgorithm(algo);
    if (algo != NULL) {
        updateColors();
#if (5 != RGBAlgorithmColorDisplayCount)
#error "Further colors need to be displayed."
#endif
        QVector<QColor> colors = {
            m_matrix->getColor(0),
            m_matrix->getColor(1),
            m_matrix->getColor(2),
            m_matrix->getColor(3),
            m_matrix->getColor(4)
        };
        algo->setColors(colors);
        m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
                                              m_matrix->algorithm());
    }
    updateExtraOptions();
    updateChannelMappingUI();  // Refresh channel mapping UI to reflect new script's scriptHeight()

    slotRestartTest();
}

void RGBMatrixEditor::slotFixtureGroupActivated(int index)
{
    QVariant var = m_fixtureGroupCombo->itemData(index);
    if (var.isValid() == true)
    {
        m_matrix->setFixtureGroup(var.toUInt());
        slotRestartTest();
    }
    else
    {
        m_matrix->setFixtureGroup(FixtureGroup::invalidId());
        m_previewTimer->stop();
        qDeleteAll(m_previewHash);
        m_previewHash.clear();
        m_scene->clear();
    }
    
    // Update per-definition channel mapping UI
    updateChannelMappingUI();
    
    // Update row filtering UI
    updateRowSelection();
}

void RGBMatrixEditor::slotBlendModeChanged(int index)
{
    m_matrix->setBlendMode(Universe::BlendMode(index));

    if (index == Universe::MaskBlend)
    {
        m_mtxColor1Button->setEnabled(false);
    }
    else
    {
        m_mtxColor1Button->setEnabled(true);
    }
    updateExtraOptions();
    updateColors();
    slotRestartTest();
}

void RGBMatrixEditor::slotControlModeChanged(int index)
{
    RGBMatrix::ControlMode mode = RGBMatrix::ControlMode(index);
    m_matrix->setControlMode(mode);
    updateColors();
    slotRestartTest();
}

void RGBMatrixEditor::slotMtxColor1ButtonClicked()
{
    QColor col = QColorDialog::getColor(m_matrix->getColor(0));
    if (col.isValid() == true)
    {
        m_matrix->setColor(0, col);
        updateColors();
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotMtxColor2ButtonClicked()
{
    QColor col = QColorDialog::getColor(m_matrix->getColor(1));
    if (col.isValid() == true)
    {
        m_matrix->setColor(1, col);
        updateColors();
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotResetMtxColor2ButtonClicked()
{
    m_matrix->setColor(1, QColor());
    updateColors();
    slotRestartTest();
}

void RGBMatrixEditor::slotMtxColor3ButtonClicked()
{
    QColor col = QColorDialog::getColor(m_matrix->getColor(2));
    if (col.isValid() == true)
    {
        m_matrix->setColor(2, col);
        updateColors();
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotResetMtxColor3ButtonClicked()
{
    m_matrix->setColor(2, QColor());
    updateColors();
    slotRestartTest();
}

void RGBMatrixEditor::slotMtxColor4ButtonClicked()
{
    QColor col = QColorDialog::getColor(m_matrix->getColor(3));
    if (col.isValid() == true)
    {
        m_matrix->setColor(3, col);
        updateColors();
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotResetMtxColor4ButtonClicked()
{
    m_matrix->setColor(3, QColor());
    updateColors();
    slotRestartTest();
}

void RGBMatrixEditor::slotMtxColor5ButtonClicked()
{
    QColor col = QColorDialog::getColor(m_matrix->getColor(4));
    if (col.isValid() == true)
    {
        m_matrix->setColor(4, col);
        updateColors();
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotResetMtxColor5ButtonClicked()
{
    m_matrix->setColor(4, QColor());
    updateColors();
    slotRestartTest();
}

void RGBMatrixEditor::slotTextEdited(const QString& text)
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Text)
    {
        RGBText *algo = static_cast<RGBText*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);
        {
            QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
            algo->setText(text);
        }
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotFontButtonClicked()
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Text)
    {
        RGBText *algo = static_cast<RGBText*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);

        bool ok = false;
        QFont font = QFontDialog::getFont(&ok, algo->font(), this);
        if (ok == true)
        {
            {
                QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
                algo->setFont(font);
            }
            slotRestartTest();
        }
    }
}

void RGBMatrixEditor::slotAnimationActivated(int index)
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Text)
    {
        RGBText *algo = static_cast<RGBText*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);
        {
            QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
            QString text = m_animationCombo->itemText(index);
            algo->setAnimationStyle(RGBText::stringToAnimationStyle(text));
        }
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotImageEdited()
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Image)
    {
        RGBImage *algo = static_cast<RGBImage*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);
        {
            QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
            algo->setFilename(m_imageEdit->text());
        }
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotImageButtonClicked()
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Image)
    {
        RGBImage *algo = static_cast<RGBImage*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);

        QString path = algo->filename();
        path = QFileDialog::getOpenFileName(this,
                                            tr("Select image"),
                                            path,
                                            QString("%1 (*.png *.bmp *.jpg *.jpeg *.gif)").arg(tr("Images")));
        if (path.isEmpty() == false)
        {
            {
                QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
                algo->setFilename(path);
            }
            m_imageEdit->setText(path);
            slotRestartTest();
        }
    }
}

void RGBMatrixEditor::slotImageAnimationActivated(int index)
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Image)
    {
        RGBImage *algo = static_cast<RGBImage*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);
        {
            QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
            QString text = m_imageAnimationCombo->itemText(index);
            algo->setAnimationStyle(RGBImage::stringToAnimationStyle(text));
        }
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotOffsetSpinChanged()
{
    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Text)
    {
        RGBText *algo = static_cast<RGBText*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);
        {
            QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
            algo->setXOffset(m_xOffsetSpin->value());
            algo->setYOffset(m_yOffsetSpin->value());
        }
        slotRestartTest();
    }

    if (m_matrix->algorithm() != NULL && m_matrix->algorithm()->type() == RGBAlgorithm::Image)
    {
        RGBImage *algo = static_cast<RGBImage*> (m_matrix->algorithm());
        Q_ASSERT(algo != NULL);
        {
            QMutexLocker algorithmLocker(&m_matrix->algorithmMutex());
            algo->setXOffset(m_xOffsetSpin->value());
            algo->setYOffset(m_yOffsetSpin->value());
        }
        slotRestartTest();
    }
}

void RGBMatrixEditor::slotLoopClicked()
{
    m_matrix->setRunOrder(Function::Loop);
    m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
            m_matrix->algorithm());
    slotRestartTest();
}

void RGBMatrixEditor::slotPingPongClicked()
{
    m_matrix->setRunOrder(Function::PingPong);
    m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
            m_matrix->algorithm());
    slotRestartTest();
}

void RGBMatrixEditor::slotSingleShotClicked()
{
    m_matrix->setRunOrder(Function::SingleShot);
    m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
            m_matrix->algorithm());
    slotRestartTest();
}

void RGBMatrixEditor::slotForwardClicked()
{
    m_matrix->setDirection(Function::Forward);
    m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
            m_matrix->algorithm());
    slotRestartTest();
}

void RGBMatrixEditor::slotBackwardClicked()
{
    m_matrix->setDirection(Function::Backward);
    m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
            m_matrix->algorithm());
    slotRestartTest();
}

void RGBMatrixEditor::slotDimmerControlClicked()
{
    m_matrix->setDimmerControl(m_dimmerControlCb->isChecked());
    if (m_dimmerControlCb->isChecked() == false)
        m_dimmerControlCb->setEnabled(false);
}

void RGBMatrixEditor::slotOverrideHTPClicked()
{
    m_matrix->setOverrideHTP(m_overrideHTPCb->isChecked());
}

void RGBMatrixEditor::slotZeroIsTransparentClicked()
{
    m_matrix->setZeroIsTransparent(m_zeroIsTransparentCb->isChecked());
}

void RGBMatrixEditor::slotFadeInChanged(int ms)
{
    m_matrix->setFadeInSpeed(ms);
    uint duration = Function::speedAdd(ms, m_speedDials->duration());
    m_matrix->setDuration(duration);
}

void RGBMatrixEditor::slotFadeOutChanged(int ms)
{
    m_matrix->setFadeOutSpeed(ms);
}

void RGBMatrixEditor::slotHoldChanged(int ms)
{
    uint duration = Function::speedAdd(m_matrix->fadeInSpeed(), ms);
    m_matrix->setDuration(duration);
}

void RGBMatrixEditor::slotDurationTapped()
{
    m_matrix->tap();
}

void RGBMatrixEditor::slotTestClicked()
{
    if (m_testButton->isChecked() == true)
        m_matrix->start(m_doc->masterTimer(), functionParent());
    else
        m_matrix->stopAndWait();
}

void RGBMatrixEditor::slotRestartTest()
{
    m_previewTimer->stop();

    if (m_testButton->isChecked() == true)
    {
        // Toggle off, toggle on. Duh.
        m_testButton->click();
        m_testButton->click();
    }

    if (createPreviewItems() == true)
        m_previewTimer->start(MasterTimer::tick());

}

void RGBMatrixEditor::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        if (m_testButton->isChecked() == true)
            m_matrix->stopAndWait();
        m_testButton->setChecked(false);
        m_testButton->setEnabled(false);
    }
    else
    {
        m_testButton->setEnabled(true);
    }
}

void RGBMatrixEditor::slotFixtureGroupAdded()
{
    fillFixtureGroupCombo();
}

void RGBMatrixEditor::slotFixtureGroupRemoved()
{
    fillFixtureGroupCombo();
    slotFixtureGroupActivated(m_fixtureGroupCombo->currentIndex());
}

void RGBMatrixEditor::slotFixtureGroupChanged(quint32 id)
{
    if (id == m_matrix->fixtureGroup())
    {
        // Update the whole chain -> maybe the fixture layout has changed
        fillFixtureGroupCombo();
        slotFixtureGroupActivated(m_fixtureGroupCombo->currentIndex());
    }
    else
    {
        // Just change the name of the group, nothing else is interesting at this point
        int index = m_fixtureGroupCombo->findData(id);
        if (index != -1)
        {
            FixtureGroup* grp = m_doc->fixtureGroup(id);
            m_fixtureGroupCombo->setItemText(index, grp->name());
        }
    }
}

void RGBMatrixEditor::slotSaveToSequenceClicked()
{
    if (m_matrix == NULL || m_matrix->fixtureGroup() == FixtureGroup::invalidId())
        return;

    FixtureGroup* grp = m_doc->fixtureGroup(m_matrix->fixtureGroup());
    if (grp != NULL && m_matrix->algorithm() != NULL)
    {
        bool testRunning = false;

        if (m_testButton->isChecked() == true)
        {
            m_testButton->click();
            testRunning = true;
        }
        else
            m_previewTimer->stop();

        Scene *grpScene = new Scene(m_doc);
        grpScene->setName(grp->name());
        grpScene->setVisible(false);

        QList<GroupHead> headList = grp->headList();
        foreach (GroupHead head, headList)
        {
            Fixture *fxi = m_doc->fixture(head.fxi);
            if (fxi == NULL)
                continue;

            if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeRgb)
            {

                    QVector <quint32> rgb = fxi->rgbChannels(head.head);

                    // in case of CMY, dump those channels
                    if (rgb.count() == 0)
                        rgb = fxi->cmyChannels(head.head);

                    if (rgb.count() == 3)
                    {
                        grpScene->setValue(head.fxi, rgb.at(0), 0);
                        grpScene->setValue(head.fxi, rgb.at(1), 0);
                        grpScene->setValue(head.fxi, rgb.at(2), 0);
                    }
            }
            else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeDimmer)
            {
                quint32 channel = fxi->masterIntensityChannel();

                if (channel == QLCChannel::invalid())
                    channel = fxi->channelNumber(QLCChannel::Intensity, QLCChannel::MSB, head.head);

                if (channel != QLCChannel::invalid())
                    grpScene->setValue(head.fxi, channel, 0);
            }
            else
            {
                quint32 channel = QLCChannel::invalid();
                if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeWhite)
                    channel = fxi->channelNumber(QLCChannel::White, QLCChannel::MSB, head.head);
                else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeAmber)
                    channel = fxi->channelNumber(QLCChannel::Amber, QLCChannel::MSB, head.head);
                else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeUV)
                    channel = fxi->channelNumber(QLCChannel::UV, QLCChannel::MSB, head.head);
                else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeShutter)
                {
                    QLCFixtureHead fHead = fxi->head(head.head);
                    QVector <quint32> shutters = fHead.shutterChannels();
                    if (shutters.count())
                        channel = shutters.first();
                }

                if (channel != QLCChannel::invalid())
                    grpScene->setValue(head.fxi, channel, 0);
            }
        }
        m_doc->addFunction(grpScene);

        int totalSteps = m_matrix->stepsCount();
        int increment = 1;
        int currentStep = 0;
        m_previewHandler->setStepColor(m_matrix->getColor(0));

        if (m_matrix->direction() == Function::Backward)
        {
            currentStep = totalSteps - 1;
            increment = -1;
            if (m_matrix->getColor(1).isValid())
                m_previewHandler->setStepColor(m_matrix->getColor(1));
        }
        m_previewHandler->calculateColorDelta(m_matrix->getColor(0), m_matrix->getColor(1),
                m_matrix->algorithm());

        if (m_matrix->runOrder() == RGBMatrix::PingPong)
            totalSteps = (totalSteps * 2) - 1;

        Sequence *sequence = new Sequence(m_doc);
        sequence->setName(QString("%1 %2").arg(m_matrix->name()).arg(tr("Sequence")));
        sequence->setBoundSceneID(grpScene->id());
        sequence->setDurationMode(Chaser::PerStep);
        sequence->setDuration(m_matrix->duration());

        if (m_matrix->fadeInSpeed() != 0)
        {
            sequence->setFadeInMode(Chaser::PerStep);
            sequence->setFadeInSpeed(m_matrix->fadeInSpeed());
        }
        if (m_matrix->fadeOutSpeed() != 0)
        {
            sequence->setFadeOutMode(Chaser::PerStep);
            sequence->setFadeOutSpeed(m_matrix->fadeOutSpeed());
        }

        for (int i = 0; i < totalSteps; i++)
        {
            m_matrix->previewMap(currentStep, m_previewHandler);
            ChaserStep step;
            step.fid = grpScene->id();
            step.hold = m_matrix->duration() - m_matrix->fadeInSpeed();
            step.duration = m_matrix->duration();
            step.fadeIn = m_matrix->fadeInSpeed();
            step.fadeOut = m_matrix->fadeOutSpeed();

            for (int y = 0; y < m_previewHandler->m_map.size(); y++)
            {
                for (int x = 0; x < m_previewHandler->m_map[y].size(); x++)
                {
                    uint col = m_previewHandler->m_map[y][x];
                    GroupHead head = grp->head(QLCPoint(x, y));

                    Fixture *fxi = m_doc->fixture(head.fxi);
                    if (fxi == NULL)
                        continue;

                    if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeRgb)
                    {
                        QVector <quint32> rgb = fxi->rgbChannels(head.head);
                        QVector <quint32> cmy = fxi->cmyChannels(head.head);

                        if (rgb.count() == 3)
                        {
                            step.values.append(SceneValue(head.fxi, rgb.at(0), qRed(col)));
                            step.values.append(SceneValue(head.fxi, rgb.at(1), qGreen(col)));
                            step.values.append(SceneValue(head.fxi, rgb.at(2), qBlue(col)));
                        }

                        if (cmy.count() == 3)
                        {
                            QColor cmyCol(col);

                            step.values.append(SceneValue(head.fxi, cmy.at(0), cmyCol.cyan()));
                            step.values.append(SceneValue(head.fxi, cmy.at(1), cmyCol.magenta()));
                            step.values.append(SceneValue(head.fxi, cmy.at(2), cmyCol.yellow()));
                        }
                    }
                    else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeDimmer)
                    {
                        quint32 channel = fxi->masterIntensityChannel();

                        if (channel == QLCChannel::invalid())
                            channel = fxi->channelNumber(QLCChannel::Intensity, QLCChannel::MSB, head.head);

                        if (channel != QLCChannel::invalid())
                            step.values.append(SceneValue(head.fxi, channel, RGBMatrix::rgbToGrey(col)));
                    }
                    else
                    {
                        quint32 channel = QLCChannel::invalid();
                        if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeWhite)
                            channel = fxi->channelNumber(QLCChannel::White, QLCChannel::MSB, head.head);
                        else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeAmber)
                            channel = fxi->channelNumber(QLCChannel::Amber, QLCChannel::MSB, head.head);
                        else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeUV)
                            channel = fxi->channelNumber(QLCChannel::UV, QLCChannel::MSB, head.head);
                        else if (m_controlModeCombo->currentIndex() == RGBMatrix::ControlModeShutter)
                        {
                            QLCFixtureHead fHead = fxi->head(head.head);
                            QVector <quint32> shutters = fHead.shutterChannels();
                            if (shutters.count())
                                channel = shutters.first();
                        }

                        if (channel != QLCChannel::invalid())
                            step.values.append(SceneValue(head.fxi, channel, RGBMatrix::rgbToGrey(col)));
                    }
                }
            }
            // !! Important !! matrix's heads can be displaced randomly but in a sequence
            // we absolutely need ordered values. So do it now !
            std::sort(step.values.begin(), step.values.end());

            sequence->addStep(step);
            currentStep += increment;
            if (currentStep == totalSteps)
            {
                if (m_matrix->runOrder() == RGBMatrix::PingPong)
                {
                    currentStep = totalSteps - 2;
                    increment = -1;
                }
                else
                {
                    currentStep = 0;
                }
            }
            m_previewHandler->updateStepColor(currentStep, m_matrix->getColor(0), m_matrix->stepsCount());
        }

        m_doc->addFunction(sequence);

        if (testRunning == true)
            m_testButton->click();
        else if (createPreviewItems() == true)
            m_previewTimer->start(MasterTimer::tick());
    }
}

void RGBMatrixEditor::slotShapeToggle(bool)
{
    createPreviewItems();
}

void RGBMatrixEditor::slotCopyClicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Copy RGB Matrix Settings"));
    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QCheckBox *chkPattern = new QCheckBox(tr("Pattern"), &dlg);
    QCheckBox *chkProperties = new QCheckBox(tr("Properties"), &dlg);
    QCheckBox *chkRunOrder = new QCheckBox(tr("Run Order"), &dlg);
    QCheckBox *chkDirection = new QCheckBox(tr("Direction"), &dlg);
    QCheckBox *chkRowFilter = new QCheckBox(tr("Row Filter"), &dlg);
    QCheckBox *chkMultiValue = new QCheckBox(tr("Multi-Value Mapping"), &dlg);

    {
        QSettings settings;
        chkPattern->setChecked(settings.value("RGBMatrixEditor/copyPattern", true).toBool());
        chkProperties->setChecked(settings.value("RGBMatrixEditor/copyProperties", true).toBool());
        chkRunOrder->setChecked(settings.value("RGBMatrixEditor/copyRunOrder", true).toBool());
        chkDirection->setChecked(settings.value("RGBMatrixEditor/copyDirection", true).toBool());
        chkRowFilter->setChecked(settings.value("RGBMatrixEditor/copyRowFilter", true).toBool());
        chkMultiValue->setChecked(settings.value("RGBMatrixEditor/copyMultiValue", true).toBool());
    }

    layout->addWidget(chkPattern);
    layout->addWidget(chkProperties);
    layout->addWidget(chkRunOrder);
    layout->addWidget(chkDirection);
    layout->addWidget(chkRowFilter);
    layout->addWidget(chkMultiValue);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    {
        QSettings settings;
        settings.setValue("RGBMatrixEditor/copyPattern", chkPattern->isChecked());
        settings.setValue("RGBMatrixEditor/copyProperties", chkProperties->isChecked());
        settings.setValue("RGBMatrixEditor/copyRunOrder", chkRunOrder->isChecked());
        settings.setValue("RGBMatrixEditor/copyDirection", chkDirection->isChecked());
        settings.setValue("RGBMatrixEditor/copyRowFilter", chkRowFilter->isChecked());
        settings.setValue("RGBMatrixEditor/copyMultiValue", chkMultiValue->isChecked());
    }

    QJsonObject root;
    root["type"] = QString("qlc_rgb_matrix_settings");
    root["version"] = 1;

    if (chkPattern->isChecked() && m_matrix->algorithm() != NULL)
    {
        QJsonObject pattern;
        pattern["algorithmName"] = m_matrix->algorithm()->name();
        pattern["algorithmType"] = static_cast<int>(m_matrix->algorithm()->type());
        pattern["controlMode"] = static_cast<int>(m_matrix->controlMode());
        root["pattern"] = pattern;
    }

    if (chkProperties->isChecked() && m_matrix->algorithm() != NULL &&
        m_matrix->algorithm()->type() == RGBAlgorithm::Script)
    {
        RGBScript *script = static_cast<RGBScript*>(m_matrix->algorithm());
        QJsonObject props;
        foreach (RGBScriptProperty prop, script->properties())
        {
            QString val = m_matrix->property(prop.m_name);
            if (!val.isEmpty())
                props[prop.m_name] = val;
        }
        root["properties"] = props;
    }

    if (chkRunOrder->isChecked())
    {
        root["runOrder"] = static_cast<int>(m_matrix->runOrder());
    }

    if (chkDirection->isChecked())
    {
        root["direction"] = static_cast<int>(m_matrix->direction());
    }

    if (chkRowFilter->isChecked())
    {
        QJsonArray rows;
        foreach (int r, m_matrix->selectedRows())
            rows.append(r);
        root["rowFilter"] = rows;
    }

    if (chkMultiValue->isChecked())
    {
        QJsonObject mv;
        mv["enabled"] = m_matrix->enablePerFixtureMapping();

        QJsonObject fixtureMappings;
        FixtureGroup *grp = m_doc->fixtureGroup(m_matrix->fixtureGroup());
        if (grp != NULL)
        {
            QSet<QString> processedKeys;
            foreach (const GroupHead &head, grp->headList())
            {
                Fixture *fxi = m_doc->fixture(head.fxi);
                if (fxi == NULL || fxi->fixtureDef() == NULL)
                    continue;
                QString key = RGBMatrix::getFixtureDefKey(fxi->fixtureDef());
                if (processedKeys.contains(key))
                    continue;
                processedKeys.insert(key);

                QList<RGBMatrix::ChannelMapping> mappings = m_matrix->fixtureDefChannelMappings(key);
                QJsonArray mappingArr;
                foreach (const RGBMatrix::ChannelMapping &cm, mappings)
                {
                    QJsonObject obj;
                    obj["channelName"] = cm.channelName;
                    obj["valueIndex"] = cm.valueIndex;
                    mappingArr.append(obj);
                }
                fixtureMappings[key] = mappingArr;
            }
        }
        mv["fixtureMappings"] = fixtureMappings;
        root["multiValueMapping"] = mv;
    }

    QJsonDocument doc(root);
    QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Compact));
}

void RGBMatrixEditor::slotPasteClicked()
{
    QString clipboardText = QApplication::clipboard()->text();
    if (clipboardText.isEmpty())
    {
        QMessageBox::warning(this, tr("Paste Error"), tr("Clipboard is empty."));
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
    if (root["type"].toString() != "qlc_rgb_matrix_settings")
    {
        QMessageBox::warning(this, tr("Paste Error"),
                             tr("Clipboard does not contain RGB Matrix settings."));
        return;
    }

    m_matrix->applySettingsFromJson(root, m_doc);

    /* Refresh UI to match the new model state */

    /* Pattern */
    if (root.contains("pattern"))
    {
        QJsonObject pattern = root["pattern"].toObject();
        QString algoName = pattern["algorithmName"].toString();
        int index = m_patternCombo->findText(algoName);
        if (index >= 0)
        {
            m_patternCombo->setCurrentIndex(index);
            updateExtraOptions();
            updateChannelMappingUI();
        }
        if (pattern.contains("controlMode"))
            m_controlModeCombo->setCurrentIndex(pattern["controlMode"].toInt());
    }

    /* Properties */
    if (root.contains("properties") && m_matrix->algorithm() != NULL &&
        m_matrix->algorithm()->type() == RGBAlgorithm::Script)
    {
        RGBScript *script = static_cast<RGBScript*>(m_matrix->algorithm());
        resetProperties(m_propertiesLayout->layout());
        displayProperties(script);
    }

    /* Run Order */
    if (root.contains("runOrder"))
    {
        switch (m_matrix->runOrder())
        {
            default:
            case Function::Loop:       m_loop->setChecked(true); break;
            case Function::PingPong:   m_pingPong->setChecked(true); break;
            case Function::SingleShot: m_singleShot->setChecked(true); break;
        }
    }

    /* Direction */
    if (root.contains("direction"))
    {
        switch (m_matrix->direction())
        {
            default:
            case Function::Forward:  m_forward->setChecked(true); break;
            case Function::Backward: m_backward->setChecked(true); break;
        }
    }

    /* Row Filter */
    if (root.contains("rowFilter"))
        updateRowSelection();

    /* Multi-Value Mapping */
    if (root.contains("multiValueMapping"))
    {
        m_enablePerFixtureMappingCheck->setChecked(m_matrix->enablePerFixtureMapping());
        updateChannelMappingUI();
    }

    slotRestartTest();
}

void RGBMatrixEditor::slotPropertyComboChanged(int index)
{
    if (m_matrix->algorithm() == NULL ||
        m_matrix->algorithm()->type() == RGBAlgorithm::Script)
    {
        QComboBox *combo = qobject_cast<QComboBox *>(sender());
        QString pName = combo->property("pName").toString();
        QString pValue = combo->itemText(index);
        qDebug() << "Property combo changed to" << pValue;
        m_matrix->setProperty(pName, pValue);

        updateColorOptions();
        updateColors();
    }
}

void RGBMatrixEditor::slotPropertySpinChanged(int value)
{
    qDebug() << "Property spin changed to" << value;
    if (m_matrix->algorithm() == NULL ||
        m_matrix->algorithm()->type() == RGBAlgorithm::Script)
    {
        QSpinBox *spin = qobject_cast<QSpinBox *>(sender());
        QString pName = spin->property("pName").toString();
        m_matrix->setProperty(pName, QString::number(value));
    }
}

void RGBMatrixEditor::slotPropertyDoubleSpinChanged(double value)
{
    qDebug() << "Property float changed to" << value;
    if (m_matrix->algorithm() == NULL ||
        m_matrix->algorithm()->type() == RGBAlgorithm::Script)
    {
        QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox *>(sender());
        QString pName = spin->property("pName").toString();
        m_matrix->setProperty(pName, QString::number(value));
    }
}

void RGBMatrixEditor::slotPropertyEditChanged(QString text)
{
    qDebug() << "Property string changed to" << text;
    if (m_matrix->algorithm() == NULL ||
        m_matrix->algorithm()->type() == RGBAlgorithm::Script)
    {
        QLineEdit *edit = qobject_cast<QLineEdit *>(sender());
        QString pName = edit->property("pName").toString();
        m_matrix->setProperty(pName, text);
    }
}

/****************************************************************************
 * Per-Definition Channel Mapping
 ****************************************************************************/

QString RGBMatrixEditor::getFixtureDefKey(const QLCFixtureDef *def)
{
    if (def == NULL)
        return QString();

    return QString("%1|%2").arg(def->manufacturer()).arg(def->model());
}

int RGBMatrixEditor::getFixtureHeadCount(const QString &fixtureDefKey)
{
    // Parse fixtureDefKey to get manufacturer and model
    QStringList parts = fixtureDefKey.split("|");
    if (parts.size() != 2)
        return 1;  // Default to single head
    
    QString manufacturer = parts[0];
    QString model = parts[1];
    
    // Find the fixture definition
    QLCFixtureDef *def = m_doc->fixtureDefCache()->fixtureDef(manufacturer, model);
    if (def == NULL)
        return 1;
    
    // Find the fixture mode (use the first mode as representative)
    if (def->modes().isEmpty())
        return 1;
        
    QLCFixtureMode *mode = def->modes().first();
    if (mode == NULL)
        return 1;
    
    return mode->heads().size();
}

void RGBMatrixEditor::clearChannelMappingUI()
{
    // NAJPROSTSZE ROZWIĄZANIE: Usuń CAŁY groupbox i utwórz nowy!
    if (m_channelMappingGroup != NULL)
    {
        // Znajdź parent layout gdzie jest m_channelMappingGroup
        QWidget *parent = m_channelMappingGroup->parentWidget();
        QLayout *parentLayout = NULL;
        
        if (parent != NULL)
            parentLayout = parent->layout();
        
        // Usuń groupbox z parent layoutu
        if (parentLayout != NULL)
            parentLayout->removeWidget(m_channelMappingGroup);
        
        // Usuń cały groupbox wraz ze wszystkimi dziećmi
        m_channelMappingGroup->deleteLater();
        m_channelMappingGroup = NULL;
        m_channelMappingLayout = NULL;
    }
    
    // Wyczyść tracking list
    m_mappingWidgets.clear();
}

void RGBMatrixEditor::addChannelMappingRow(FixtureDefMappingWidget &widget, QLCFixtureMode *mode,
                                            const QList<RGBMatrix::ChannelMapping> &mappings, bool isFirstRow)
{
    ChannelMappingRow row;
    
    // Create horizontal layout for this row
    row.layout = new QHBoxLayout();
    row.layout->setSpacing(8);
    
    // Channel multi-select combo
    row.channelCombo = new MultiSelectChannelCombo();
    row.channelCombo->setProperty("fixtureDefKey", widget.fixtureDefKey);
    row.channelCombo->setProperty("rowIndex", widget.rows.size());
    row.channelCombo->addItem(tr("Auto (use control mode)"), QString());
    
    // Populate with channels from fixture mode
    for (int i = 0; i < mode->channels().size(); i++)
    {
        QLCChannel *ch = mode->channel(i);
        if (ch != NULL)
            row.channelCombo->addItem(ch->name(), ch->name());
    }
    
    // Add Virtual Dimmer option for fixtures that support it
    if (mode->virtualDimmer())
    {
        row.channelCombo->addItem(tr("Virtual Dimmer"), QString("__VIRTUAL_DIMMER__"));
    }
    
    // Set current selection from mappings (all mappings should have same valueIndex).
    // Empty channelName means "Auto (use control mode)" — include it so the Auto
    // item is visually checked when the project is reloaded.
    QVariantList selectedChannels;
    for (const RGBMatrix::ChannelMapping &mapping : mappings)
        selectedChannels.append(mapping.channelName);
    row.channelCombo->setSelectedData(selectedChannels);
    
    // Offset combo
    row.valueIndexCombo = new QComboBox();
    row.valueIndexCombo->setProperty("fixtureDefKey", widget.fixtureDefKey);
    row.valueIndexCombo->setProperty("rowIndex", widget.rows.size());
    
    // Get max offset from script's paramCount
    int maxOffset = 32;  // default fallback
    if (m_matrix->algorithm() != NULL)
    {
        int paramCount = m_matrix->algorithm()->paramCount();
        if (paramCount > 1)
            maxOffset = paramCount;
    }
    
    for (int i = 0; i < maxOffset; i++)
    {
        row.valueIndexCombo->addItem(QString::number(i + 1), i);
    }
    
    // Set current offset (all mappings should have same valueIndex)
    int valueIndex = mappings.isEmpty() ? 0 : mappings.first().valueIndex;
    row.valueIndexCombo->setCurrentIndex(valueIndex);
    
    // Head Mode combo (only if fixture has multiple heads)
    int headCount = getFixtureHeadCount(widget.fixtureDefKey);
    if (headCount > 1)
    {
        row.headModeCombo = new QComboBox();
        row.headModeCombo->setProperty("fixtureDefKey", widget.fixtureDefKey);
        row.headModeCombo->setProperty("rowIndex", widget.rows.size());
        row.headModeCombo->addItem(tr("Individual Heads"), "Individual");
        row.headModeCombo->addItem(tr("All Heads"), "All");
        
        // Set current value (use first mapping's head mode)
        QString currentMode = mappings.isEmpty() || mappings.first().headMode.isEmpty() ? "Individual" : mappings.first().headMode;
        int modeIdx = row.headModeCombo->findData(currentMode);
        if (modeIdx >= 0)
            row.headModeCombo->setCurrentIndex(modeIdx);
            
        connect(row.headModeCombo, SIGNAL(activated(int)), this, SLOT(slotChannelMappingChanged(int)));
    }
    else
    {
        row.headModeCombo = NULL;  // Single head fixture - no choice needed
    }
    
    // Remove button (ONLY for non-first rows)
    if (!isFirstRow)
    {
        row.removeButton = new QPushButton(QIcon(":/edit_remove.png"), QString());
        row.removeButton->setMaximumWidth(30);
        row.removeButton->setToolTip(tr("Remove channel mapping"));
        row.removeButton->setProperty("fixtureDefKey", widget.fixtureDefKey);
        row.removeButton->setProperty("rowIndex", widget.rows.size());
        connect(row.removeButton, &QPushButton::clicked, this, &RGBMatrixEditor::slotRemoveChannelMapping);
    }
    else
    {
        row.removeButton = NULL;  // Cannot remove first row
    }
    
    // Connect combo signals
    connect(row.channelCombo, SIGNAL(selectionChanged()), this, SLOT(slotChannelMappingChanged()));
    connect(row.valueIndexCombo, SIGNAL(activated(int)), this, SLOT(slotChannelMappingChanged(int)));
    
    // Build layout
    // In multi-row mode, add "Channel:" and "Offset:" labels
    if (!isFirstRow)
    {
        row.layout->addWidget(new QLabel(tr("Channel:")));
    }
    row.layout->addWidget(row.channelCombo, 1);
    
    if (!isFirstRow)
    {
        row.layout->addWidget(new QLabel(tr("Offset:")));
    }
    row.layout->addWidget(row.valueIndexCombo);
    
    if (row.headModeCombo)
    {
        if (!isFirstRow)
        {
            row.layout->addWidget(new QLabel(tr("Head Mode:")));
        }
        row.layout->addWidget(row.headModeCombo);
    }
    
    if (row.removeButton)
    {
        row.layout->addWidget(row.removeButton);
    }
    
    widget.rows.append(row);
}

void RGBMatrixEditor::updateChannelMappingUI()
{
    // ZAWSZE NAJPIERW WYCZYŚĆ (to usuwa groupbox!)
    clearChannelMappingUI();
    
    // ODTWÓRZ groupbox i layout z SCROLLABLE AREA
    if (m_channelMappingGroup == NULL)
    {
        m_channelMappingGroup = new QGroupBox(tr("Per-Fixture Channel Mapping"));
        
        // Create scrollable area for content (hybrid solution)
        QScrollArea *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setMaximumHeight(400);  // Max 400px height - scrolls if more fixture types
        
        // Inner widget for scrollable content
        QWidget *scrollWidget = new QWidget();
        m_channelMappingLayout = new QVBoxLayout(scrollWidget);
        m_channelMappingLayout->setContentsMargins(4, 4, 4, 4);
        m_channelMappingLayout->setSpacing(8);
        scrollArea->setWidget(scrollWidget);
        
        // GroupBox layout
        QVBoxLayout *groupLayout = new QVBoxLayout(m_channelMappingGroup);
        groupLayout->setContentsMargins(4, 4, 4, 4);
        groupLayout->addWidget(scrollArea);
        m_channelMappingGroup->setLayout(groupLayout);
        m_channelMappingGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        
        // Dodaj do bottom grid layout (ta sama pozycja co w init())
        QGridLayout *bottomGrid = this->findChild<QGridLayout*>("gridLayout_6");
        if (bottomGrid != NULL)
        {
            bottomGrid->addWidget(m_channelMappingGroup, 3, 0, 1, 3);  // Row 3, col 0-2, span 3 cols
        }
    }
    
    // Check if multi-value mapping is enabled
    if (!m_enablePerFixtureMappingCheck->isChecked())
    {
        m_channelMappingGroup->setVisible(false);
        return;
    }

    FixtureGroup *grp = m_doc->fixtureGroup(m_matrix->fixtureGroup());
    if (grp == NULL)
    {
        m_channelMappingGroup->setVisible(false);
        return;
    }

    // Collect unique fixture definitions
    QMap<QString, QPair<QLCFixtureDef*, QLCFixtureMode*>> uniqueDefs;
    foreach (quint32 fxiId, grp->fixtureList())
    {
        Fixture *fxi = m_doc->fixture(fxiId);
        if (fxi == NULL || fxi->fixtureDef() == NULL)
            continue;

        QString key = getFixtureDefKey(fxi->fixtureDef());
        if (!uniqueDefs.contains(key))
        {
            uniqueDefs[key] = qMakePair(fxi->fixtureDef(), fxi->fixtureMode());
        }
    }

    // Create UI for each unique fixture definition
    QMapIterator<QString, QPair<QLCFixtureDef*, QLCFixtureMode*>> it(uniqueDefs);
    while (it.hasNext())
    {
        it.next();
        QString key = it.key();
        QLCFixtureDef *def = it.value().first;
        QLCFixtureMode *mode = it.value().second;

        if (def == NULL || mode == NULL)
            continue;

        FixtureDefMappingWidget widget;
        widget.fixtureDefKey = key;
        widget.isCollapsed = false;  // Default: expanded

        // Create fixture name label
        widget.label = new QLabel(QString("<b>%1:</b>").arg(def->name()));
        
        // Create collapse/expand button
        widget.collapseButton = new QToolButton();
        widget.collapseButton->setArrowType(Qt::DownArrow);
        widget.collapseButton->setAutoRaise(true);
        widget.collapseButton->setToolTip(tr("Collapse/Expand channel mappings"));
        widget.collapseButton->setProperty("fixtureDefKey", key);
        widget.collapseButton->setCheckable(true);
        widget.collapseButton->setChecked(false);  // Not collapsed initially
        connect(widget.collapseButton, &QToolButton::toggled, this, &RGBMatrixEditor::slotToggleCollapse);

        // Get existing mappings from backend
        QList<RGBMatrix::ChannelMapping> existingMappings = m_matrix->fixtureDefChannelMappings(key);
        
        // If no mappings, create one default (Auto, offset 0)
        if (existingMappings.isEmpty())
        {
            existingMappings.append(RGBMatrix::ChannelMapping());
        }

        // Create container for multi-row layout (with indentation)
        widget.containerWidget = new QWidget();
        widget.containerLayout = new QVBoxLayout(widget.containerWidget);
        widget.containerLayout->setContentsMargins(20, 0, 0, 0);  // Indent rows
        widget.containerLayout->setSpacing(4);

        // Group mappings by valueIndex (offset)
        QMap<int, QList<RGBMatrix::ChannelMapping>> groupedMappings;
        foreach (const RGBMatrix::ChannelMapping &mapping, existingMappings)
        {
            groupedMappings[mapping.valueIndex].append(mapping);
        }
        
        // Create UI rows - one row per unique offset
        bool isFirstRow = true;
        QMapIterator<int, QList<RGBMatrix::ChannelMapping>> groupIt(groupedMappings);
        while (groupIt.hasNext())
        {
            groupIt.next();
            const QList<RGBMatrix::ChannelMapping> &mappingsForOffset = groupIt.value();
            addChannelMappingRow(widget, mode, mappingsForOffset, isFirstRow);
            isFirstRow = false;
        }

        // Create add-row button
        widget.addButton = new QPushButton(QIcon(":/edit_add.png"), QString());
        widget.addButton->setMaximumWidth(30);
        widget.addButton->setToolTip(tr("Add channel mapping"));
        widget.addButton->setProperty("fixtureDefKey", key);
        connect(widget.addButton, &QPushButton::clicked, this, &RGBMatrixEditor::slotAddChannelMapping);

        // LAYOUT LOGIC:
        // - Single row (default): [▼] [Label:] [Channel ▼] [Offset ▼] [➕] (all in one line)
        // - Multi row: [▼] [Label] on top, then indented rows below with ➕ at end
        // - Collapsed: [▶] [Label] only (rows hidden)
        
        if (widget.rows.size() == 1)
        {
            // SINGLE ROW MODE: everything in one horizontal line
            QHBoxLayout *singleRowLayout = new QHBoxLayout();
            singleRowLayout->addWidget(widget.collapseButton);
            singleRowLayout->addWidget(widget.label);
            singleRowLayout->addWidget(widget.rows[0].channelCombo, 1);
            singleRowLayout->addWidget(widget.rows[0].valueIndexCombo);
            singleRowLayout->addWidget(widget.addButton);
            singleRowLayout->addStretch();
            
            m_channelMappingLayout->addLayout(singleRowLayout);
        }
        else
        {
            // MULTI ROW MODE: collapse button + label on top, rows indented below
            QHBoxLayout *headerLayout = new QHBoxLayout();
            headerLayout->addWidget(widget.collapseButton);
            headerLayout->addWidget(widget.label);
            headerLayout->addStretch();
            m_channelMappingLayout->addLayout(headerLayout);
            
            // Add all rows to container
            for (int i = 0; i < widget.rows.size(); i++)
            {
                widget.containerLayout->addLayout(widget.rows[i].layout);
            }
            
            // Add ➕ button at end of last row
            QHBoxLayout *lastRowLayout = qobject_cast<QHBoxLayout*>(widget.rows.last().layout);
            if (lastRowLayout)
            {
                lastRowLayout->addWidget(widget.addButton);
            }
            
            // Container is visible/hidden based on collapse state
            widget.containerWidget->setVisible(!widget.isCollapsed);
            
            m_channelMappingLayout->addWidget(widget.containerWidget);
        }

        m_mappingWidgets.append(widget);
    }

    m_channelMappingGroup->setVisible(true);
}

void RGBMatrixEditor::slotChannelMappingChanged(int index)
{
    Q_UNUSED(index);

    QComboBox *combo = qobject_cast<QComboBox*>(sender());
    if (combo == NULL)
        return;

    QString key = combo->property("fixtureDefKey").toString();
    if (key.isEmpty())
        return;

    // Save all channel mappings for this fixture type
    saveAllChannelMappings(key);
    
    // Restart preview if running
    slotRestartTest();
}

void RGBMatrixEditor::slotChannelMappingChanged()
{
    MultiSelectChannelCombo *combo = qobject_cast<MultiSelectChannelCombo*>(sender());
    if (combo == NULL)
        return;

    QString key = combo->property("fixtureDefKey").toString();
    if (key.isEmpty())
        return;

    // Save all channel mappings for this fixture type
    saveAllChannelMappings(key);
    
    // Restart preview if running
    slotRestartTest();
}

void RGBMatrixEditor::saveAllChannelMappings(const QString &fixtureDefKey)
{
    // Find the widget for this fixture type
    const FixtureDefMappingWidget *widget = NULL;
    for (int i = 0; i < m_mappingWidgets.size(); i++)
    {
        if (m_mappingWidgets[i].fixtureDefKey == fixtureDefKey)
        {
            widget = &m_mappingWidgets[i];
            break;
        }
    }
    
    if (widget == NULL)
        return;
    
    // Collect all mappings from UI rows
    QList<RGBMatrix::ChannelMapping> mappings;
    foreach (const ChannelMappingRow &row, widget->rows)
    {
        // Get selected channels from multi-select combo
        QVariantList selectedChannels = row.channelCombo->selectedData();
        int valueIndex = row.valueIndexCombo->currentData().toInt();
        QString headMode = row.headModeCombo ? row.headModeCombo->currentData().toString() : "Individual";
        
        // Create separate mapping for each selected channel
        foreach (const QVariant &channelData, selectedChannels)
        {
            RGBMatrix::ChannelMapping m;
            m.channelName = channelData.toString();
            m.valueIndex = valueIndex;
            m.headMode = headMode;
            mappings.append(m);
        }
        
        // If no channels selected, create an "Auto" mapping
        if (selectedChannels.isEmpty())
        {
            RGBMatrix::ChannelMapping m;
            m.channelName = "";  // Empty = Auto
            m.valueIndex = valueIndex;
            m.headMode = headMode;
            mappings.append(m);
        }
    }
    
    // Save to backend
    m_matrix->setFixtureDefChannelMappings(fixtureDefKey, mappings);
}

void RGBMatrixEditor::slotAddChannelMapping()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn == NULL)
        return;
    
    QString key = btn->property("fixtureDefKey").toString();
    if (key.isEmpty())
        return;
    
    // Find next unused offset so the new row gets its own UI row
    QList<RGBMatrix::ChannelMapping> existing = m_matrix->fixtureDefChannelMappings(key);
    int nextOffset = 0;
    foreach (const RGBMatrix::ChannelMapping &m, existing)
        if (m.valueIndex >= nextOffset)
            nextOffset = m.valueIndex + 1;
    
    m_matrix->addChannelMapping(key, QString(), nextOffset);
    
    // Refresh UI (will switch to multi-row mode if this is second mapping)
    updateChannelMappingUI();
    slotRestartTest();
}

void RGBMatrixEditor::slotRemoveChannelMapping()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn == NULL)
        return;
    
    QString key = btn->property("fixtureDefKey").toString();
    int rowIndex = btn->property("rowIndex").toInt();
    
    if (key.isEmpty())
        return;
    
    // Find the offset (valueIndex) of the UI row being removed
    for (int i = 0; i < m_mappingWidgets.size(); i++)
    {
        if (m_mappingWidgets[i].fixtureDefKey == key)
        {
            if (rowIndex >= 0 && rowIndex < m_mappingWidgets[i].rows.size())
            {
                int offset = m_mappingWidgets[i].rows[rowIndex].valueIndexCombo->currentData().toInt();
                m_matrix->removeChannelMappingsByOffset(key, offset);
            }
            break;
        }
    }
    
    // Refresh UI
    updateChannelMappingUI();
    slotRestartTest();
}

void RGBMatrixEditor::slotToggleCollapse(bool collapsed)
{
    QToolButton *btn = qobject_cast<QToolButton*>(sender());
    if (btn == NULL)
        return;
    
    QString key = btn->property("fixtureDefKey").toString();
    if (key.isEmpty())
        return;
    
    // Find the corresponding widget
    for (int i = 0; i < m_mappingWidgets.size(); i++)
    {
        if (m_mappingWidgets[i].fixtureDefKey == key)
        {
            FixtureDefMappingWidget &widget = m_mappingWidgets[i];
            widget.isCollapsed = collapsed;
            
            // Update arrow direction
            btn->setArrowType(collapsed ? Qt::RightArrow : Qt::DownArrow);
            
            // Show/hide container widget (for multi-row mode)
            if (widget.containerWidget != NULL)
            {
                widget.containerWidget->setVisible(!collapsed);
            }
            
            // For single-row mode, show/hide the combos and add button
            if (widget.rows.size() == 1)
            {
                widget.rows[0].channelCombo->setVisible(!collapsed);
                widget.rows[0].valueIndexCombo->setVisible(!collapsed);
                widget.addButton->setVisible(!collapsed);
            }
            
            break;
        }
    }
}

FunctionParent RGBMatrixEditor::functionParent() const
{
    return FunctionParent::master();
}

/*****************************************************************************
 * Row filtering
 *****************************************************************************/

void RGBMatrixEditor::updateRowSelection()
{
    // Clear existing checkboxes
    foreach (QCheckBox *cb, m_rowCheckboxes)
    {
        m_rowSelectionLayout->removeWidget(cb);
        delete cb;
    }
    m_rowCheckboxes.clear();
    
    // Clear any remaining layout items
    QLayoutItem *child;
    while ((child = m_rowSelectionLayout->takeAt(0)) != nullptr)
    {
        if (child->widget())
            delete child->widget();
        delete child;
    }
    
    // Check if we have a fixture group
    FixtureGroup *grp = m_doc->fixtureGroup(m_matrix->fixtureGroup());
    if (grp == nullptr)
    {
        m_rowSelectionGroup->setVisible(false);
        return;
    }
    
    int gridHeight = grp->size().height();
    if (gridHeight <= 0)
    {
        m_rowSelectionGroup->setVisible(false);
        return;
    }
    
    m_rowSelectionGroup->setVisible(true);
    
    // Create checkbox for each row
    QList<int> currentSelection = m_matrix->selectedRows();
    for (int row = 0; row < gridHeight; row++)
    {
        QCheckBox *cb = new QCheckBox(QString("Row %1").arg(row + 1));
        // If no selection saved, select all by default
        cb->setChecked(currentSelection.isEmpty() || currentSelection.contains(row));
        cb->setProperty("row_index", row);
        
        m_rowSelectionLayout->addWidget(cb);
        m_rowCheckboxes.append(cb);
        
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(slotRowSelectionChanged()));
    }
}

void RGBMatrixEditor::slotRowSelectionChanged()
{
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
    if (selectedRows.isEmpty() && !m_rowCheckboxes.isEmpty())
    {
        // Re-check the first checkbox to prevent empty selection
        m_rowCheckboxes.first()->blockSignals(true);
        m_rowCheckboxes.first()->setChecked(true);
        m_rowCheckboxes.first()->blockSignals(false);
        selectedRows.append(0);
    }
    
    m_matrix->setSelectedRows(selectedRows);
    
    // Restart preview
    slotRestartTest();
}

/*****************************************************************************
 * Multi-Value Mapping
 *****************************************************************************/

void RGBMatrixEditor::slotEnablePerFixtureMappingToggled(bool checked)
{
    m_matrix->setEnablePerFixtureMapping(checked);
    
    // Update UI to show/hide per-fixture mapping section
    updateChannelMappingUI();
    
    // Restart preview
    slotRestartTest();
}

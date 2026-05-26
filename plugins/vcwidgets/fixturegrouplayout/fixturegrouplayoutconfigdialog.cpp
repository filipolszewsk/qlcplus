/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "fixturegrouplayoutconfigdialog.h"

#include "inputselectionwidget.h"
#include "doc.h"
#include "fixturegroup.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>
#include <QPushButton>

FixtureGroupLayoutConfigDialog::FixtureGroupLayoutConfigDialog(
        Doc* doc,
        quint32 currentGroupId,
        const MaskDisplayColors& maskColors,
        MaskEfxConflictPolicy efxConflictPolicy,
        QSharedPointer<QLCInputSource> presetSelectSrc,
        int widgetPage,
        QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_initGroupId(currentGroupId)
    , m_maskColors(maskColors)
    , m_maskEfxConflictPolicy(efxConflictPolicy)
{
    setWindowTitle(tr("Fixture Group Layout — Properties"));
    setMinimumWidth(420);

    QVBoxLayout* root = new QVBoxLayout(this);

    QFormLayout* form = new QFormLayout();
    m_groupCombo = new QComboBox(this);
    form->addRow(tr("Fixture group:"), m_groupCombo);
    root->addLayout(form);

    QGroupBox* colorGrp = new QGroupBox(tr("Mask colors"), this);
    QFormLayout* colorForm = new QFormLayout(colorGrp);

    m_visibleColorBtn = new QPushButton(tr("Choose…"), colorGrp);
    colorForm->addRow(tr("Visible cells:"), m_visibleColorBtn);
    connect(m_visibleColorBtn, &QPushButton::clicked,
            this, &FixtureGroupLayoutConfigDialog::slotPickVisibleColor);

    m_hiddenColorBtn = new QPushButton(tr("Choose…"), colorGrp);
    colorForm->addRow(tr("Hidden cells:"), m_hiddenColorBtn);
    connect(m_hiddenColorBtn, &QPushButton::clicked,
            this, &FixtureGroupLayoutConfigDialog::slotPickHiddenColor);

    m_hiddenTextColorBtn = new QPushButton(tr("Choose…"), colorGrp);
    colorForm->addRow(tr("Hidden cell text:"), m_hiddenTextColorBtn);
    connect(m_hiddenTextColorBtn, &QPushButton::clicked,
            this, &FixtureGroupLayoutConfigDialog::slotPickHiddenTextColor);

    m_headerColorBtn = new QPushButton(tr("Choose…"), colorGrp);
    colorForm->addRow(tr("Active column header:"), m_headerColorBtn);
    connect(m_headerColorBtn, &QPushButton::clicked,
            this, &FixtureGroupLayoutConfigDialog::slotPickHeaderColor);

    m_selectionBorderBtn = new QPushButton(tr("Choose…"), colorGrp);
    colorForm->addRow(tr("Selection border:"), m_selectionBorderBtn);
    connect(m_selectionBorderBtn, &QPushButton::clicked,
            this, &FixtureGroupLayoutConfigDialog::slotPickSelectionBorderColor);

    updateColorButton(m_visibleColorBtn, m_maskColors.visible);
    updateColorButton(m_hiddenColorBtn, m_maskColors.hidden);
    updateColorButton(m_hiddenTextColorBtn, m_maskColors.hiddenText);
    updateColorButton(m_headerColorBtn, m_maskColors.headerVisible);
    updateColorButton(m_selectionBorderBtn, m_maskColors.selectionBorder);

    root->addWidget(colorGrp);

    QFont hintFont = font();
    hintFont.setItalic(true);

    QGroupBox* efxGrp = new QGroupBox(tr("EFX in mask"), this);
    QFormLayout* efxForm = new QFormLayout(efxGrp);
    m_efxConflictCombo = new QComboBox(efxGrp);
    m_efxConflictCombo->addItem(tr("Off (standard HTP)"),
                                int(MaskEfxConflictPolicy::None));
    m_efxConflictCombo->addItem(tr("Wait for release"),
                                int(MaskEfxConflictPolicy::Wait));
    m_efxConflictCombo->addItem(tr("Override channel type"),
                                int(MaskEfxConflictPolicy::Override));
    const int policyIdx = m_efxConflictCombo->findData(int(m_maskEfxConflictPolicy));
    if (policyIdx >= 0)
        m_efxConflictCombo->setCurrentIndex(policyIdx);
    efxForm->addRow(tr("Conflict policy:"), m_efxConflictCombo);
    QLabel* efxHint = new QLabel(
        tr("When a new EFX starts in the mask: Override removes the same channel "
           "type (Position, Dimmer, or RGB) from other running EFX on that head. "
           "Wait holds the new EFX until the other stops."),
        efxGrp);
    efxHint->setWordWrap(true);
    efxHint->setFont(hintFont);
    efxForm->addRow(efxHint);
    root->addWidget(efxGrp);

    QGroupBox* inputGrp = new QGroupBox(tr("External Input"), this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGrp);
    QLabel* inputHint = new QLabel(
        tr("Channel value selects the preset list by index: 0 = All (clear mask), "
           "1 = first saved preset, 2 = second preset, etc."),
        inputGrp);
    inputHint->setWordWrap(true);
    inputHint->setFont(hintFont);
    inputLayout->addWidget(inputHint);

    m_presetSelectInputSel = new InputSelectionWidget(doc, inputGrp);
    m_presetSelectInputSel->setKeyInputVisibility(false);
    m_presetSelectInputSel->setWidgetPage(widgetPage);
    m_presetSelectInputSel->setInputSource(presetSelectSrc);
    inputLayout->addWidget(m_presetSelectInputSel);
    root->addWidget(inputGrp);

    QLabel* hint = new QLabel(
        tr("Drag fixtures on the grid to move them. Right-click drag removes cells from selection."),
        this);
    hint->setWordWrap(true);
    QFont f = hint->font();
    f.setItalic(true);
    hint->setFont(f);
    root->addWidget(hint);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    populateGroups();
}

void FixtureGroupLayoutConfigDialog::updateColorButton(QPushButton* button, const QColor& color)
{
    if (button == nullptr)
        return;
    button->setAutoFillBackground(true);
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, color);
    pal.setColor(QPalette::ButtonText,
                 color.lightness() > 128 ? Qt::black : Qt::white);
    button->setPalette(pal);
    button->setText(color.name());
}

void FixtureGroupLayoutConfigDialog::slotPickVisibleColor()
{
    const QColor c = QColorDialog::getColor(m_maskColors.visible, this, tr("Visible cells"));
    if (!c.isValid())
        return;
    m_maskColors.visible = c;
    updateColorButton(m_visibleColorBtn, c);
}

void FixtureGroupLayoutConfigDialog::slotPickHiddenColor()
{
    const QColor c = QColorDialog::getColor(m_maskColors.hidden, this, tr("Hidden cells"));
    if (!c.isValid())
        return;
    m_maskColors.hidden = c;
    updateColorButton(m_hiddenColorBtn, c);
}

void FixtureGroupLayoutConfigDialog::slotPickHiddenTextColor()
{
    const QColor c = QColorDialog::getColor(m_maskColors.hiddenText, this, tr("Hidden cell text"));
    if (!c.isValid())
        return;
    m_maskColors.hiddenText = c;
    updateColorButton(m_hiddenTextColorBtn, c);
}

void FixtureGroupLayoutConfigDialog::slotPickHeaderColor()
{
    const QColor c = QColorDialog::getColor(m_maskColors.headerVisible, this, tr("Active column header"));
    if (!c.isValid())
        return;
    m_maskColors.headerVisible = c;
    updateColorButton(m_headerColorBtn, c);
}

void FixtureGroupLayoutConfigDialog::slotPickSelectionBorderColor()
{
    const QColor c = QColorDialog::getColor(m_maskColors.selectionBorder, this, tr("Selection border"));
    if (!c.isValid())
        return;
    m_maskColors.selectionBorder = c;
    updateColorButton(m_selectionBorderBtn, c);
}

void FixtureGroupLayoutConfigDialog::populateGroups()
{
    m_groupCombo->clear();
    m_groupCombo->addItem(tr("(None)"), QVariant::fromValue(FixtureGroup::invalidId()));

    int preSelect = 0;
    int idx = 1;
    for (FixtureGroup* grp : m_doc->fixtureGroups())
    {
        if (grp == nullptr)
            continue;
        m_groupCombo->addItem(grp->name(), QVariant::fromValue(grp->id()));
        if (grp->id() == m_initGroupId)
            preSelect = idx;
        idx++;
    }

    m_groupCombo->setCurrentIndex(preSelect);
}

quint32 FixtureGroupLayoutConfigDialog::fixtureGroupId() const
{
    return m_groupCombo->currentData().toUInt();
}

MaskDisplayColors FixtureGroupLayoutConfigDialog::maskDisplayColors() const
{
    return m_maskColors;
}

MaskEfxConflictPolicy FixtureGroupLayoutConfigDialog::maskEfxConflictPolicy() const
{
    if (m_efxConflictCombo == nullptr)
        return m_maskEfxConflictPolicy;
    return static_cast<MaskEfxConflictPolicy>(m_efxConflictCombo->currentData().toInt());
}

QSharedPointer<QLCInputSource> FixtureGroupLayoutConfigDialog::presetSelectInputSource() const
{
    return m_presetSelectInputSel ? m_presetSelectInputSel->inputSource()
                                  : QSharedPointer<QLCInputSource>();
}

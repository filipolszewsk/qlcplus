/*
  QLC+ VC Widget Plugin — Preset Table v2
  presettablev2configdialog.cpp — Apache 2.0 / public domain
*/

#include "presettablev2configdialog.h"
#include "presettablev2widget.h"
#include "presettablev2inputids.h"
#include "presettablev2transitionprovideriface.h"
#include "virtualconsole.h"
#include "presettablev2columndialog.h"
#include "presettablev2vclookup.h"

#include "inputselectionwidget.h"
#include "fixtureselection.h"
#include "fixturegroup.h"
#include "fixturegroupmask.h"
#include "qlcpoint.h"
#include "grouphead.h"
#include "doc.h"
#include "fixture.h"
#include "qlcchannel.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "qlcfixturehead.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QHeaderView>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QSet>
#include <algorithm>

// ==========================================================================
// OutputEditorRow (Legacy mode)
// ==========================================================================

OutputEditorRow::OutputEditorRow(Doc* doc,
                                  const PTOutput& output,
                                  QSharedPointer<QLCInputSource> src,
                                  int requiredChannels,
                                  int widgetPage,
                                  QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_requiredChannels(requiredChannels)
    , m_fixtureId(output.fixtureId)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(4, 2, 4, 2);
    lay->setSpacing(6);

    // Name edit
    m_nameEdit = new QLineEdit(output.name, this);
    m_nameEdit->setPlaceholderText(tr("Output name"));
    m_nameEdit->setMinimumWidth(80);
    m_nameEdit->setMaximumWidth(120);
    lay->addWidget(m_nameEdit);

    // Fixture label + choose button
    m_fixtureLabel = new QLabel(this);
    m_fixtureLabel->setMinimumWidth(160);
    lay->addWidget(m_fixtureLabel, 1);

    m_chooseFixBtn = new QPushButton(tr("Choose fixture..."), this);
    m_chooseFixBtn->setMaximumWidth(130);
    lay->addWidget(m_chooseFixBtn);

    // Row selector input
    m_inputSel = new InputSelectionWidget(doc, this);
    m_inputSel->setKeyInputVisibility(false);
    m_inputSel->setWidgetPage(widgetPage);
    m_inputSel->setInputSource(src);
    lay->addWidget(m_inputSel);

    updateFixtureLabel();

    connect(m_chooseFixBtn, &QPushButton::clicked, this, &OutputEditorRow::slotChooseFixture);
    connect(m_nameEdit, &QLineEdit::textEdited, this, &OutputEditorRow::slotNameEdited);
}

PTOutput OutputEditorRow::output() const
{
    PTOutput out;
    out.name      = m_nameEdit->text().trimmed();
    out.fixtureId = m_fixtureId;
    return out;
}

QSharedPointer<QLCInputSource> OutputEditorRow::inputSource() const
{
    return m_inputSel ? m_inputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

void OutputEditorRow::slotChooseFixture()
{
    FixtureSelection fs(this, m_doc);
    fs.setMultiSelection(false);

    // Filter to only show fixtures with the required channel count
    if (m_requiredChannels > 0)
    {
        QList<quint32> disabled;
        for (Fixture* fx : m_doc->fixtures())
            if ((int)fx->channels() != m_requiredChannels)
                disabled.append(fx->id());
        fs.setDisabledFixtures(disabled);
    }

    if (fs.exec() == QDialog::Accepted)
    {
        QList<quint32> sel = fs.selection();
        if (!sel.isEmpty())
        {
            m_fixtureId = sel.first();
            updateFixtureLabel();
            emit changed();
            emit fixtureSelected(m_fixtureId);
        }
    }
}

void OutputEditorRow::setRequiredChannels(int n)
{
    m_requiredChannels = n;
    updateFixtureLabel();
}

void OutputEditorRow::slotNameEdited(const QString&)
{
    emit changed();
}

void OutputEditorRow::updateFixtureLabel()
{
    if (m_fixtureId == UINT_MAX)
    {
        m_fixtureLabel->setText(tr("<not assigned>"));
        m_fixtureLabel->setStyleSheet(QStringLiteral("color: #888;"));
        return;
    }

    Fixture* fx = m_doc->fixture(m_fixtureId);
    if (!fx)
    {
        m_fixtureLabel->setText(tr("<missing fixture>"));
        m_fixtureLabel->setStyleSheet(QStringLiteral("color: #ff4444;"));
        return;
    }

    bool ok = (m_requiredChannels <= 0 || (int)fx->channels() == m_requiredChannels);
    QString label = QString("U%1 CH%2 — %3 (%4ch)")
        .arg(fx->universe() + 1)
        .arg(fx->address() + 1)
        .arg(fx->name())
        .arg(fx->channels());
    m_fixtureLabel->setText(label);
    m_fixtureLabel->setStyleSheet(ok ? QString() : QStringLiteral("color: #ff4444;"));
}

static PresetTableV2TransitionProviderIface* transitionProviderForWidgetId(quint32 widgetId)
{
    return PresetTableV2VCLookup::transitionProviderByVcId(widgetId);
}

// ==========================================================================
// FGOutputEditorRow (FixtureGroup mode)
// ==========================================================================

FGOutputEditorRow::FGOutputEditorRow(Doc* doc,
                                       const PTOutput& output,
                                       QSharedPointer<QLCInputSource> rowSrc,
                                       QSharedPointer<QLCInputSource> transSweepSrc,
                                       QSharedPointer<QLCInputSource> transContinuousSrc,
                                       QSharedPointer<QLCInputSource> positionMotionSrc,
                                       QSharedPointer<QLCInputSource> channel1DSrc,
                                       QSharedPointer<QLCInputSource> multiFxSrc,
                                       QSharedPointer<QLCInputSource> transSecondarySrc,
                                       FixtureGroup* group,
                                       int widgetPage,
                                       PresetTableV2TransitionProviderIface* transitionProvider,
                                       PresetTableV2Widget* ptWidget,
                                       QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_group(group)
    , m_ptWidget(ptWidget)
    , m_transitionProvider(transitionProvider)
{
    const bool positionMode = m_ptWidget && m_ptWidget->widgetMode() == PTMode::Position;

    QVBoxLayout* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(4, 2, 4, 2);
    rootLay->setSpacing(4);

    QHBoxLayout* topLay = new QHBoxLayout;
    topLay->setSpacing(6);

    m_nameEdit = new QLineEdit(output.name, this);
    m_nameEdit->setPlaceholderText(tr("Output name"));
    m_nameEdit->setMinimumWidth(80);
    m_nameEdit->setMaximumWidth(120);
    topLay->addWidget(m_nameEdit);

    m_scopeCombo = new QComboBox(this);
    m_scopeCombo->addItem(tr("Rows"), int(PTOutputScope::Rows));
    m_scopeCombo->addItem(tr("Group mask"), int(PTOutputScope::Mask));
    m_scopeCombo->addItem(tr("Rows + mask"), int(PTOutputScope::RowsAndMask));
    const int scopeIdx = m_scopeCombo->findData(int(output.scope));
    if (scopeIdx >= 0)
        m_scopeCombo->setCurrentIndex(scopeIdx);
    m_scopeCombo->setToolTip(tr("Rows: grid rows only (ignores VC mask). "
                                 "Group mask: cells from Fixture Group Layout mask on this group. "
                                 "Rows + mask: both."));
    topLay->addWidget(m_scopeCombo);

    m_sweepPresetCombo = new QComboBox(this);
    m_sweepPresetCombo->setMinimumWidth(90);
    m_sweepPresetCombo->setToolTip(tr("Default Transition preset when the Transition selector has no DMX. Off = Instant."));
    topLay->addWidget(m_sweepPresetCombo);

    m_continuousPresetCombo = new QComboBox(this);
    m_continuousPresetCombo->setMinimumWidth(90);
    m_continuousPresetCombo->setToolTip(tr("Default Interpolation preset when the Interpolation selector has no DMX. Off = disabled."));
    topLay->addWidget(m_continuousPresetCombo);

    m_positionMotionPresetCombo = new QComboBox(this);
    m_positionMotionPresetCombo->setMinimumWidth(90);
    m_positionMotionPresetCombo->setToolTip(tr("Default Continuous Motion preset when the Continuous Motion selector has no DMX. Off = disabled."));
    m_positionMotionPresetCombo->setVisible(positionMode);
    topLay->addWidget(m_positionMotionPresetCombo);

    m_channel1DPresetCombo = new QComboBox(this);
    m_channel1DPresetCombo->setMinimumWidth(90);
    m_channel1DPresetCombo->setToolTip(tr("Default 1D Channel FX preset when the 1D Channel FX selector has no DMX. Off = disabled."));
    m_channel1DPresetCombo->setVisible(!positionMode);
    topLay->addWidget(m_channel1DPresetCombo);

    m_multiFxPresetCombo = new QComboBox(this);
    m_multiFxPresetCombo->setMinimumWidth(90);
    m_multiFxPresetCombo->setToolTip(tr("Default MultiFX background preset when the MultiFX selector has no DMX. Off = disabled."));
    topLay->addWidget(m_multiFxPresetCombo);

    m_secondaryRowCombo = new QComboBox(this);
    m_secondaryRowCombo->setMinimumWidth(90);
    m_secondaryRowCombo->setToolTip(tr("Default secondary row for Continuous FX when DMX secondary is 0."));
    topLay->addWidget(m_secondaryRowCombo);

    rebuildTransitionPresetCombos();
    rebuildSecondaryRowCombo();
    const int swIdx = m_sweepPresetCombo->findData(output.sweepPresetIndex);
    if (swIdx >= 0)
        m_sweepPresetCombo->setCurrentIndex(swIdx);
    const int ctIdx = m_continuousPresetCombo->findData(output.continuousPresetIndex);
    if (ctIdx >= 0)
        m_continuousPresetCombo->setCurrentIndex(ctIdx);
    const int pmIdx = m_positionMotionPresetCombo->findData(output.positionMotionPresetIndex);
    if (pmIdx >= 0)
        m_positionMotionPresetCombo->setCurrentIndex(pmIdx);
    const int chIdx = m_channel1DPresetCombo->findData(output.channel1DPresetIndex);
    if (chIdx >= 0)
        m_channel1DPresetCombo->setCurrentIndex(chIdx);
    const int mfIdx = m_multiFxPresetCombo->findData(output.multiFxPresetIndex);
    if (mfIdx >= 0)
        m_multiFxPresetCombo->setCurrentIndex(mfIdx);
    const int srIdx = m_secondaryRowCombo->findData(output.secondaryRowIndex);
    if (srIdx >= 0)
        m_secondaryRowCombo->setCurrentIndex(srIdx);

    connect(m_sweepPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::changed);
    connect(m_continuousPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::changed);
    connect(m_positionMotionPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::changed);
    connect(m_channel1DPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::changed);
    connect(m_multiFxPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::changed);
    connect(m_secondaryRowCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::changed);

    rootLay->addLayout(topLay);

    m_cbWidget = new QWidget(this);
    m_cbLayout = new QVBoxLayout(m_cbWidget);
    m_cbLayout->setContentsMargins(0, 0, 0, 0);
    m_cbLayout->setSpacing(1);
    m_cbWidget->setLayout(m_cbLayout);
    rootLay->addWidget(m_cbWidget, 1);

    QHBoxLayout* inLay = new QHBoxLayout;
    inLay->setSpacing(4);

    auto addInput = [&](const QString& label, InputSelectionWidget*& sel,
                        const QSharedPointer<QLCInputSource>& src, const QString& tip) -> QWidget* {
        QWidget* box = new QWidget(this);
        QVBoxLayout* vl = new QVBoxLayout(box);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(0);
        QLabel* lbl = new QLabel(label, box);
        QFont f = lbl->font();
        f.setPointSize(std::max(8, f.pointSize() - 1));
        lbl->setFont(f);
        lbl->setToolTip(tip);
        vl->addWidget(lbl);
        sel = new InputSelectionWidget(doc, box);
        sel->setKeyInputVisibility(false);
        sel->setWidgetPage(widgetPage);
        sel->setInputSource(src);
        sel->setToolTip(tip);
        vl->addWidget(sel);
        inLay->addWidget(box);
        return box;
    };

    addInput(tr("Primary live selector / snapshot"), m_inputSel, rowSrc,
             tr("Live primary row for snapshots/cuelists. DMX 1–N = table row, 0 = off. 101+ = flash that row."));
    addInput(tr("Transition live selector / snapshot"), m_transSweepInputSel, transSweepSrc,
             tr("Live transition preset for snapshots/cuelists. 0 = instant."));
    addInput(tr("Interpolation live selector / snapshot"), m_transContinuousInputSel, transContinuousSrc,
             tr("Live Interpolation preset for snapshots/cuelists. 0 = off."));
    QWidget* positionMotionInputBox = addInput(
            tr("Continuous Motion live selector / snapshot"), m_positionMotionInputSel,
            positionMotionSrc,
            tr("Live Continuous Motion preset for snapshots/cuelists. 0 = off."));
    positionMotionInputBox->setVisible(positionMode);
    QWidget* channel1DInputBox = addInput(
            tr("1D Channel FX live selector / snapshot"), m_channel1DInputSel,
            channel1DSrc,
            tr("Live 1D Channel FX preset for snapshots/cuelists. 0 = off."));
    channel1DInputBox->setVisible(!positionMode);
    addInput(tr("MultiFX live selector / snapshot"), m_multiFxInputSel, multiFxSrc,
             tr("Live MultiFX preset for snapshots/cuelists. MultiFX blend still controls how much is revealed."));
    addInput(tr("Secondary live selector / snapshot"), m_transSecondaryInputSel, transSecondarySrc,
             tr("Live secondary row for snapshots/cuelists. DMX 1 = table row 1, 2 = row 2, 0 = use Secondary combo below."));
    rootLay->addLayout(inLay);

    rebuildRowCheckboxes();
    for (QCheckBox* cb : m_rowCBs)
    {
        int y = cb->property("rowY").toInt();
        if (output.groupRows.contains(y))
            cb->setChecked(true);
    }

    connect(m_nameEdit, &QLineEdit::textEdited, this, &FGOutputEditorRow::changed);
    connect(m_scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FGOutputEditorRow::updateScopeUi);
    updateScopeUi();
}

void FGOutputEditorRow::updateScopeUi()
{
    const bool showRows = m_scopeCombo == nullptr
            || m_scopeCombo->currentData().toInt() != int(PTOutputScope::Mask);
    if (m_cbWidget != nullptr)
        m_cbWidget->setVisible(showRows);
}

void FGOutputEditorRow::setTransitionProvider(PresetTableV2TransitionProviderIface* provider)
{
    m_transitionProvider = provider;
    rebuildTransitionPresetCombos();
}

void FGOutputEditorRow::rebuildTransitionPresetCombos()
{
    auto fillCombo = [this](QComboBox* combo, PTTransitionMode mode) {
        if (!combo)
            return;
        const int prev = combo->currentData().toInt();
        combo->blockSignals(true);
        combo->clear();
        combo->addItem(tr("Instant"), -1);
        if (m_transitionProvider)
        {
            for (int i = 0; i < m_transitionProvider->transitionPresetCount(mode); ++i)
                combo->addItem(m_transitionProvider->transitionPresetName(mode, i), i);
        }
        const int idx = combo->findData(prev);
        combo->setCurrentIndex(idx >= 0 ? idx : 0);
        combo->blockSignals(false);
    };
    fillCombo(m_sweepPresetCombo, PTTransitionMode::SweepOnly);
    fillCombo(m_continuousPresetCombo, PTTransitionMode::Continuous);
    fillCombo(m_positionMotionPresetCombo, PTTransitionMode::PositionMotion);
    fillCombo(m_channel1DPresetCombo, PTTransitionMode::Channel1D);
    fillCombo(m_multiFxPresetCombo, PTTransitionMode::MultiFx);
}

void FGOutputEditorRow::rebuildSecondaryRowCombo()
{
    if (!m_secondaryRowCombo)
        return;
    const int prev = m_secondaryRowCombo->currentData().toInt();
    m_secondaryRowCombo->blockSignals(true);
    m_secondaryRowCombo->clear();
    m_secondaryRowCombo->addItem(tr("Off"), -1);
    if (m_ptWidget)
    {
        const QVector<PTRow>& rows = m_ptWidget->rows();
        for (int i = 0; i < rows.size(); ++i)
        {
            const QString label = rows[i].name.isEmpty()
                    ? tr("Row %1").arg(i + 1) : rows[i].name;
            m_secondaryRowCombo->addItem(label, i);
        }
    }
    const int idx = m_secondaryRowCombo->findData(prev);
    m_secondaryRowCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_secondaryRowCombo->blockSignals(false);
}

PTOutput FGOutputEditorRow::output() const
{
    PTOutput out;
    out.name = m_nameEdit ? m_nameEdit->text().trimmed() : QString();
    if (m_scopeCombo != nullptr)
        out.scope = static_cast<PTOutputScope>(m_scopeCombo->currentData().toInt());
    if (m_sweepPresetCombo != nullptr)
        out.sweepPresetIndex = m_sweepPresetCombo->currentData().toInt();
    if (m_continuousPresetCombo != nullptr)
        out.continuousPresetIndex = m_continuousPresetCombo->currentData().toInt();
    if (m_positionMotionPresetCombo != nullptr)
        out.positionMotionPresetIndex = m_positionMotionPresetCombo->currentData().toInt();
    if (m_channel1DPresetCombo != nullptr)
        out.channel1DPresetIndex = m_channel1DPresetCombo->currentData().toInt();
    if (m_multiFxPresetCombo != nullptr)
        out.multiFxPresetIndex = m_multiFxPresetCombo->currentData().toInt();
    if (m_secondaryRowCombo != nullptr)
        out.secondaryRowIndex = m_secondaryRowCombo->currentData().toInt();
    for (const QCheckBox* cb : m_rowCBs)
    {
        if (cb->isChecked())
            out.groupRows.append(cb->property("rowY").toInt());
    }
    return out;
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::inputSource() const
{
    return m_inputSel ? m_inputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::transSweepInputSource() const
{
    return m_transSweepInputSel ? m_transSweepInputSel->inputSource()
                                : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::transContinuousInputSource() const
{
    return m_transContinuousInputSel ? m_transContinuousInputSel->inputSource()
                                    : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::positionMotionInputSource() const
{
    return m_positionMotionInputSel ? m_positionMotionInputSel->inputSource()
                                    : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::channel1DInputSource() const
{
    return m_channel1DInputSel ? m_channel1DInputSel->inputSource()
                               : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::multiFxInputSource() const
{
    return m_multiFxInputSel ? m_multiFxInputSel->inputSource()
                             : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> FGOutputEditorRow::transSecondaryInputSource() const
{
    return m_transSecondaryInputSel ? m_transSecondaryInputSel->inputSource()
                                    : QSharedPointer<QLCInputSource>();
}

void FGOutputEditorRow::setFixtureGroup(FixtureGroup* group)
{
    m_group = group;
    // Preserve checked rows before rebuild
    QList<int> wasChecked;
    for (const QCheckBox* cb : m_rowCBs)
        if (cb->isChecked())
            wasChecked.append(cb->property("rowY").toInt());

    rebuildRowCheckboxes();

    for (QCheckBox* cb : m_rowCBs)
    {
        int y = cb->property("rowY").toInt();
        cb->setChecked(wasChecked.contains(y));
    }
}

void FGOutputEditorRow::rebuildRowCheckboxes()
{
    // Clear existing checkboxes
    for (QCheckBox* cb : m_rowCBs)
        delete cb;
    m_rowCBs.clear();

    if (!m_group) return;

    int gridH = m_group->size().height();
    if (gridH <= 0) return;

    // Count fixtures per row
    QMap<int, int> countPerRow;
    for (const GroupHead& head : m_group->headList())
    {
        // Find the y-coordinate for this head
        const auto& headsMap = m_group->headsMap();
        for (auto it = headsMap.constBegin(); it != headsMap.constEnd(); ++it)
        {
            if (it.value().fxi == head.fxi && it.value().head == head.head)
            {
                countPerRow[it.key().y()]++;
                break;
            }
        }
    }

    for (int y = 0; y < gridH; ++y)
    {
        int cnt = countPerRow.value(y, 0);
        QString label = tr("Row %1 (%2 fixture(s))").arg(y).arg(cnt);
        QCheckBox* cb = new QCheckBox(label, m_cbWidget);
        cb->setProperty("rowY", y);
        m_cbLayout->addWidget(cb);
        m_rowCBs.append(cb);
        connect(cb, &QCheckBox::toggled, this, &FGOutputEditorRow::changed);
    }
}

// ==========================================================================
// PresetTableV2ConfigDialog
// ==========================================================================

PresetTableV2ConfigDialog::PresetTableV2ConfigDialog(Doc* doc,
                                                   const QVector<PTColumn>& columns,
                                                   const QVector<PTOutput>&  outputs,
                                                   const QVector<QSharedPointer<QLCInputSource>>& sources,
                                                   bool crossfadeEnabled,
                                                   bool syncMultiFxPhaseToCrossfade,
                                                   int multiFxCrossfadeSyncOffsetMs,
                                                   QSharedPointer<QLCInputSource> crossfadeSrc,
                                                   QSharedPointer<QLCInputSource> multiFxBlendSrc,
                                                   QSharedPointer<QLCInputSource> multiFxRestartSrc,
                                                   const QKeySequence& multiFxRestartKey,
                                                   QSharedPointer<QLCInputSource> widgetFlashGateSrc,
                                                   const QKeySequence& widgetFlashGateKey,
                                                   int widgetFlashTimeMultiplierIndex,
                                                   PTWidgetFlashBehavior widgetFlashBehavior,
                                                   PTContinuousFxSelectorMode continuousFxSelectorMode,
                                                   int widgetPage,
                                                   PTMode mode,
                                                   quint32 fixtureGroupId,
                                                   const PTSpatialEffectSettings& spatialEffects,
                                                   quint32 linkedTransitionWidgetId,
                                                   bool positionConfirmDiscardDraft,
                                                   bool positionShowStatusStrip,
                                                   bool positionShowEditorHints,
                                                   QSharedPointer<QLCInputSource> positionBasePanSrc,
                                                   QSharedPointer<QLCInputSource> positionBaseTiltSrc,
                                                   QSharedPointer<QLCInputSource> positionSpreadPanSrc,
                                                   QSharedPointer<QLCInputSource> positionSpreadTiltSrc,
                                                   QSharedPointer<QLCInputSource> positionSpreadPanEnableSrc,
                                                   QSharedPointer<QLCInputSource> positionSpreadTiltEnableSrc,
                                                   QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_widgetPage(widgetPage)
    , m_ptWidget(qobject_cast<PresetTableV2Widget*>(parent))
    , m_columns(columns)
    , m_initSources(sources)
{
    setWindowTitle(tr("Preset Table v2 — Properties"));
    setMinimumSize(600, 520);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ============================
    // Mode + Group selection row
    // ============================
    QWidget* modeWidget = new QWidget(this);
    QHBoxLayout* modeRow = new QHBoxLayout(modeWidget);
    modeRow->setContentsMargins(0, 0, 0, 0);

    modeRow->addWidget(new QLabel(tr("Widget name:"), modeWidget));
    m_captionEdit = new QLineEdit(modeWidget);
    m_captionEdit->setPlaceholderText(tr("e.g. Front Wash"));
    if (m_ptWidget)
        m_captionEdit->setText(m_ptWidget->caption());
    m_captionEdit->setMinimumWidth(160);
    modeRow->addWidget(m_captionEdit);

    modeRow->addWidget(new QLabel(tr("Mode:"), modeWidget));
    m_modeCombo = new QComboBox(modeWidget);
    m_modeCombo->addItem(tr("Single Fixture (Legacy)"), int(PTMode::Legacy));
    m_modeCombo->addItem(tr("Fixture Group"),           int(PTMode::FixtureGroup));
    m_modeCombo->addItem(tr("Position Mode"),           int(PTMode::Position));
    {
        const int idx = m_modeCombo->findData(int(mode));
        m_modeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    modeRow->addWidget(m_modeCombo);

    // Fixture Group selector (hidden when Legacy)
    m_groupRow = new QWidget(modeWidget);
    QHBoxLayout* groupRowLay = new QHBoxLayout(m_groupRow);
    groupRowLay->setContentsMargins(12, 0, 0, 0);
    groupRowLay->addWidget(new QLabel(tr("Group:"), m_groupRow));
    m_groupCombo = new QComboBox(m_groupRow);
    m_groupCombo->setMinimumWidth(200);
    groupRowLay->addWidget(m_groupCombo, 1);
    modeRow->addWidget(m_groupRow, 1);

    modeRow->addStretch();
    root->addWidget(modeWidget);

    // Populate group combo
    rebuildGroupCombo();
    // Select the group that was active
    if (fixtureGroupId != UINT_MAX)
    {
        for (int i = 0; i < m_groupCombo->count(); ++i)
        {
            if (m_groupCombo->itemData(i).toUInt() == fixtureGroupId)
            {
                m_groupCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    m_groupRow->setVisible(mode == PTMode::FixtureGroup || mode == PTMode::Position);

    // ============================
    // Tabs
    // ============================
    m_configTabs = new QTabWidget(this);
    QTabWidget* tabs = m_configTabs;

    // ---- TAB: Outputs -------------------------------------------------------
    QWidget* outTab = new QWidget(tabs);
    QVBoxLayout* outLayout = new QVBoxLayout(outTab);

    m_outHintLabel = new QLabel(outTab);
    m_outHintLabel->setWordWrap(true);
    {
        QFont hf = m_outHintLabel->font();
        hf.setItalic(true);
        m_outHintLabel->setFont(hf);
    }
    outLayout->addWidget(m_outHintLabel);

    // Scrollable list of output editor rows
    m_outputList = new QListWidget(outTab);
    m_outputList->setUniformItemSizes(false);
    m_outputList->setSpacing(2);
    outLayout->addWidget(m_outputList, 1);

    QHBoxLayout* outBtnRow = new QHBoxLayout;
    m_addOutBtn = new QPushButton(tr("Add Output"), outTab);
    m_remOutBtn = new QPushButton(tr("Remove Output"), outTab);
    outBtnRow->addWidget(m_addOutBtn);
    outBtnRow->addWidget(m_remOutBtn);
    outBtnRow->addStretch();
    outLayout->addLayout(outBtnRow);

    tabs->addTab(outTab, tr("Outputs"));

    // ---- TAB: Columns -------------------------------------------------------
    QWidget* colTab = new QWidget(tabs);
    QVBoxLayout* colLayout = new QVBoxLayout(colTab);

    m_colTable = new QTableWidget(0, 5, colTab);
    m_colTable->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Fade"), tr("1D FX"), tr("Binding")});
    m_colTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_colTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_colTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_colTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_colTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_colTable->verticalHeader()->setVisible(false);
    m_colTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_colTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_colTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    colLayout->addWidget(m_colTable, 1);

    QHBoxLayout* colBtnRow = new QHBoxLayout;
    m_editColBtn        = new QPushButton(tr("Edit..."), colTab);
    m_addColBtn         = new QPushButton(tr("+ Column"), colTab);
    m_remColsBtn        = new QPushButton(tr("- Column"), colTab);
    m_addChFromGrpBtn   = new QPushButton(tr("+ Channels from Group..."), colTab);
    colBtnRow->addWidget(m_editColBtn);
    colBtnRow->addWidget(m_addColBtn);
    colBtnRow->addWidget(m_remColsBtn);
    colBtnRow->addWidget(m_addChFromGrpBtn);
    colBtnRow->addStretch();
    colLayout->addLayout(colBtnRow);

    m_colTabIndex = tabs->addTab(colTab, tr("Columns"));

    // ---- TAB: Position ------------------------------------------------------
    QWidget* posTab = new QWidget(tabs);
    QVBoxLayout* posTabLayout = new QVBoxLayout(posTab);

    QGroupBox* posEditorGrp = new QGroupBox(tr("Position editor"), posTab);
    QVBoxLayout* posEditorLayout = new QVBoxLayout(posEditorGrp);

    m_positionConfirmDiscardDraftChk = new QCheckBox(
            tr("Warn before discarding unsaved position edits"), posEditorGrp);
    m_positionConfirmDiscardDraftChk->setChecked(positionConfirmDiscardDraft);
    m_positionConfirmDiscardDraftChk->setToolTip(tr(
            "When enabled, switching presets or layers asks before dropping unsaved edits. "
            "When disabled, unsaved edits are discarded silently."));
    posEditorLayout->addWidget(m_positionConfirmDiscardDraftChk);

    m_positionShowStatusStripChk = new QCheckBox(
            tr("Show position status strip"), posEditorGrp);
    m_positionShowStatusStripChk->setChecked(positionShowStatusStrip);
    m_positionShowStatusStripChk->setToolTip(tr(
            "Shows the text bar under the fixture grid with layer name, live/staged state "
            "and fixture positions. Hide for a cleaner layout."));
    posEditorLayout->addWidget(m_positionShowStatusStripChk);

    m_positionShowEditorHintsChk = new QCheckBox(
            tr("Show editor hints"), posEditorGrp);
    m_positionShowEditorHintsChk->setChecked(positionShowEditorHints);
    m_positionShowEditorHintsChk->setToolTip(tr(
            "Shows short help text below the status strip (e.g. symmetric spread)."));
    posEditorLayout->addWidget(m_positionShowEditorHintsChk);

    auto addPositionInput = [&](const QString& label,
                                InputSelectionWidget*& selector,
                                const QSharedPointer<QLCInputSource>& src,
                                const QString& tip) {
        QWidget* rowWidget = new QWidget(posEditorGrp);
        QHBoxLayout* row = new QHBoxLayout(rowWidget);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(new QLabel(label, rowWidget));
        selector = new InputSelectionWidget(doc, rowWidget);
        selector->setKeyInputVisibility(false);
        selector->setWidgetPage(widgetPage);
        selector->setInputSource(src);
        selector->setToolTip(tip);
        row->addWidget(selector, 1);
        posEditorLayout->addWidget(rowWidget);
    };
    addPositionInput(tr("Base Pan input (0–255):"), m_positionBasePanInputSel,
                     positionBasePanSrc,
                     tr("Moves the current position editor base pan for selected fixtures."));
    addPositionInput(tr("Base Tilt input (0–255):"), m_positionBaseTiltInputSel,
                     positionBaseTiltSrc,
                     tr("Moves the current position editor base tilt for selected fixtures."));
    addPositionInput(tr("Spread Pan input (center catch):"), m_positionSpreadPanInputSel,
                     positionSpreadPanSrc,
                     tr("Controls Pan spread after the input passes through center."));
    addPositionInput(tr("Spread Tilt input (center catch):"), m_positionSpreadTiltInputSel,
                     positionSpreadTiltSrc,
                     tr("Controls Tilt spread after the input passes through center."));
    addPositionInput(tr("Spread Pan enable:"), m_positionSpreadPanEnableInputSel,
                     positionSpreadPanEnableSrc,
                     tr("Value > 127 enables Pan spread and arms center catch."));
    addPositionInput(tr("Spread Tilt enable:"), m_positionSpreadTiltEnableInputSel,
                     positionSpreadTiltEnableSrc,
                     tr("Value > 127 enables Tilt spread and arms center catch."));
    posEditorLayout->addStretch();
    posTabLayout->addWidget(posEditorGrp);
    posTabLayout->addStretch();

    m_positionTabIndex = tabs->addTab(posTab, tr("Position"));

    // ---- TAB: Crossfade / Transitions ---------------------------------------
    QWidget* xfTab = new QWidget(tabs);
    QVBoxLayout* xfTabLayout = new QVBoxLayout(xfTab);

    QGroupBox* xfGrp = new QGroupBox(tr("Crossfade"), xfTab);
    QVBoxLayout* xfLayout = new QVBoxLayout(xfGrp);

    m_crossfadeChk = new QCheckBox(tr("Enable crossfade (row selector sets staged, not current)"), xfGrp);
    m_crossfadeChk->setChecked(crossfadeEnabled);
    xfLayout->addWidget(m_crossfadeChk);

    m_syncMultiFxPhaseChk = new QCheckBox(
            tr("Sync MultiFX phase to crossfade start (match cue list EFX lazy-start)"), xfGrp);
    m_syncMultiFxPhaseChk->setChecked(syncMultiFxPhaseToCrossfade);
    m_syncMultiFxPhaseChk->setEnabled(crossfadeEnabled);
    xfLayout->addWidget(m_syncMultiFxPhaseChk);

    QWidget* multiFxSyncOffsetWidget = new QWidget(xfGrp);
    QHBoxLayout* multiFxSyncOffsetRow = new QHBoxLayout(multiFxSyncOffsetWidget);
    multiFxSyncOffsetRow->setContentsMargins(20, 0, 0, 0);
    multiFxSyncOffsetRow->addWidget(new QLabel(
            tr("MultiFX crossfade sync offset (ms):"), multiFxSyncOffsetWidget));
    m_multiFxSyncOffsetSpin = new QSpinBox(multiFxSyncOffsetWidget);
    m_multiFxSyncOffsetSpin->setRange(0, 200);
    m_multiFxSyncOffsetSpin->setSingleStep(20);
    m_multiFxSyncOffsetSpin->setSuffix(tr(" ms"));
    m_multiFxSyncOffsetSpin->setValue(qBound(0, multiFxCrossfadeSyncOffsetMs, 200));
    m_multiFxSyncOffsetSpin->setToolTip(tr(
            "Hold staged MultiFX at phase 0 for this long after crossfade starts, "
            "to match cue-list EFX lazy-start. Tune until DMX matches native EFX (default 40)."));
    m_multiFxSyncOffsetSpin->setEnabled(crossfadeEnabled && syncMultiFxPhaseToCrossfade);
    multiFxSyncOffsetRow->addWidget(m_multiFxSyncOffsetSpin);
    multiFxSyncOffsetRow->addStretch();
    xfLayout->addWidget(multiFxSyncOffsetWidget);

    QWidget* contFxModeWidget = new QWidget(xfGrp);
    QHBoxLayout* contFxModeRow = new QHBoxLayout(contFxModeWidget);
    contFxModeRow->setContentsMargins(0, 0, 0, 0);
    contFxModeRow->addWidget(new QLabel(tr("Continuous FX selector mode:"), contFxModeWidget));
    m_contFxModeCombo = new QComboBox(contFxModeWidget);
    m_contFxModeCombo->addItem(tr("Live"), int(PTContinuousFxSelectorMode::Live));
    m_contFxModeCombo->addItem(tr("Staged commit"), int(PTContinuousFxSelectorMode::StagedCommit));
    m_contFxModeCombo->addItem(tr("Smooth morph"), int(PTContinuousFxSelectorMode::SmoothMorph));
    const int contFxModeIdx = m_contFxModeCombo->findData(int(continuousFxSelectorMode));
    m_contFxModeCombo->setCurrentIndex(contFxModeIdx >= 0 ? contFxModeIdx : 1);
    m_contFxModeCombo->setToolTip(tr("Controls only Continuous FX preset selection. EFX parameters remain live."));
    contFxModeRow->addWidget(m_contFxModeCombo, 1);
    xfLayout->addWidget(contFxModeWidget);

    m_xfadeInputWidget = new QWidget(xfGrp);
    QHBoxLayout* xfInputRow = new QHBoxLayout(m_xfadeInputWidget);
    xfInputRow->setContentsMargins(0, 0, 0, 0);
    xfInputRow->addWidget(new QLabel(tr("Global crossfade input (0–255):"), m_xfadeInputWidget));
    m_xfadeInputSel = new InputSelectionWidget(doc, m_xfadeInputWidget);
    m_xfadeInputSel->setKeyInputVisibility(false);
    m_xfadeInputSel->setWidgetPage(widgetPage);
    m_xfadeInputSel->setInputSource(crossfadeSrc);
    xfInputRow->addWidget(m_xfadeInputSel, 1);
    xfLayout->addWidget(m_xfadeInputWidget);

    QWidget* multiFxInputWidget = new QWidget(xfGrp);
    QHBoxLayout* multiFxInputRow = new QHBoxLayout(multiFxInputWidget);
    multiFxInputRow->setContentsMargins(0, 0, 0, 0);
    multiFxInputRow->addWidget(new QLabel(tr("MultiFX blend input (0–255):"), multiFxInputWidget));
    m_multiFxBlendInputSel = new InputSelectionWidget(doc, multiFxInputWidget);
    m_multiFxBlendInputSel->setKeyInputVisibility(false);
    m_multiFxBlendInputSel->setWidgetPage(widgetPage);
    m_multiFxBlendInputSel->setInputSource(multiFxBlendSrc);
    multiFxInputRow->addWidget(m_multiFxBlendInputSel, 1);
    multiFxInputRow->addWidget(new QLabel(tr("Restart/start:"), multiFxInputWidget));
    m_multiFxRestartInputSel = new InputSelectionWidget(doc, multiFxInputWidget);
    m_multiFxRestartInputSel->setKeyInputVisibility(true);
    m_multiFxRestartInputSel->setWidgetPage(widgetPage);
    m_multiFxRestartInputSel->setInputSource(multiFxRestartSrc);
    m_multiFxRestartInputSel->setKeySequence(multiFxRestartKey);
    multiFxInputRow->addWidget(m_multiFxRestartInputSel, 1);
    xfLayout->addWidget(multiFxInputWidget);

    QGroupBox* widgetFlashGrp = new QGroupBox(tr("Widget flash"), xfTab);
    QVBoxLayout* widgetFlashLay = new QVBoxLayout(widgetFlashGrp);
    QLabel* widgetFlashHint = new QLabel(
            tr("Hold gate for Multi Button Widget links targeting Primary Row. "
               "0/released = normal mode, value > 0/held key = flash mode."),
            widgetFlashGrp);
    widgetFlashHint->setWordWrap(true);
    widgetFlashLay->addWidget(widgetFlashHint);
    QWidget* widgetFlashBehaviorWidget = new QWidget(widgetFlashGrp);
    QHBoxLayout* widgetFlashBehaviorRow = new QHBoxLayout(widgetFlashBehaviorWidget);
    widgetFlashBehaviorRow->setContentsMargins(0, 0, 0, 0);
    widgetFlashBehaviorRow->addWidget(new QLabel(tr("Behavior:"), widgetFlashBehaviorWidget));
    m_widgetFlashBehaviorCombo = new QComboBox(widgetFlashBehaviorWidget);
    m_widgetFlashBehaviorCombo->addItem(tr("Primary row modifier"),
                                        int(PTWidgetFlashBehavior::PrimaryRowModifier));
    m_widgetFlashBehaviorCombo->addItem(tr("Staged row trigger"),
                                        int(PTWidgetFlashBehavior::StagedRowTrigger));
    const int widgetFlashBehaviorIdx =
            m_widgetFlashBehaviorCombo->findData(int(widgetFlashBehavior));
    m_widgetFlashBehaviorCombo->setCurrentIndex(widgetFlashBehaviorIdx >= 0
            ? widgetFlashBehaviorIdx : 0);
    widgetFlashBehaviorRow->addWidget(m_widgetFlashBehaviorCombo, 1);
    widgetFlashLay->addWidget(widgetFlashBehaviorWidget);
    QWidget* widgetFlashInputWidget = new QWidget(widgetFlashGrp);
    QHBoxLayout* widgetFlashInputRow = new QHBoxLayout(widgetFlashInputWidget);
    widgetFlashInputRow->setContentsMargins(0, 0, 0, 0);
    widgetFlashInputRow->addWidget(new QLabel(tr("Flash hold gate:"), widgetFlashInputWidget));
    m_widgetFlashGateInputSel = new InputSelectionWidget(doc, widgetFlashInputWidget);
    m_widgetFlashGateInputSel->setKeyInputVisibility(true);
    m_widgetFlashGateInputSel->setWidgetPage(widgetPage);
    m_widgetFlashGateInputSel->setInputSource(widgetFlashGateSrc);
    m_widgetFlashGateInputSel->setKeySequence(widgetFlashGateKey);
    widgetFlashInputRow->addWidget(m_widgetFlashGateInputSel, 1);
    widgetFlashLay->addWidget(widgetFlashInputWidget);
    QWidget* widgetFlashTimeWidget = new QWidget(widgetFlashGrp);
    QHBoxLayout* widgetFlashTimeRow = new QHBoxLayout(widgetFlashTimeWidget);
    widgetFlashTimeRow->setContentsMargins(0, 0, 0, 0);
    widgetFlashTimeRow->addWidget(new QLabel(tr("Flash time multiplier:"), widgetFlashTimeWidget));
    m_widgetFlashTimeMultCombo = new QComboBox(widgetFlashTimeWidget);
    m_widgetFlashTimeMultCombo->addItem(tr("1/16x"), 6);
    m_widgetFlashTimeMultCombo->addItem(tr("1/8x"), 5);
    m_widgetFlashTimeMultCombo->addItem(tr("0.25x"), 0);
    m_widgetFlashTimeMultCombo->addItem(tr("0.5x"), 1);
    m_widgetFlashTimeMultCombo->addItem(tr("1x"), 2);
    m_widgetFlashTimeMultCombo->addItem(tr("2x"), 3);
    m_widgetFlashTimeMultCombo->addItem(tr("4x"), 4);
    {
        const int idx = m_widgetFlashTimeMultCombo->findData(widgetFlashTimeMultiplierIndex);
        const int defaultIdx = m_widgetFlashTimeMultCombo->findData(2);
        m_widgetFlashTimeMultCombo->setCurrentIndex(idx >= 0 ? idx
                : (defaultIdx >= 0 ? defaultIdx : 0));
    }
    widgetFlashTimeRow->addWidget(m_widgetFlashTimeMultCombo, 1);
    widgetFlashLay->addWidget(widgetFlashTimeWidget);
    xfTabLayout->addWidget(widgetFlashGrp);

    m_xfadeInputWidget->setVisible(crossfadeEnabled);
    xfTabLayout->addWidget(xfGrp);

    connect(m_crossfadeChk, &QCheckBox::toggled, m_xfadeInputWidget, &QWidget::setVisible);
    connect(m_crossfadeChk, &QCheckBox::toggled, m_syncMultiFxPhaseChk, &QWidget::setEnabled);
    auto updateMultiFxSyncOffsetEnabled = [this]() {
        if (!m_multiFxSyncOffsetSpin)
            return;
        const bool on = m_crossfadeChk && m_crossfadeChk->isChecked()
                && m_syncMultiFxPhaseChk && m_syncMultiFxPhaseChk->isChecked();
        m_multiFxSyncOffsetSpin->setEnabled(on);
    };
    connect(m_crossfadeChk, &QCheckBox::toggled, this, updateMultiFxSyncOffsetEnabled);
    connect(m_syncMultiFxPhaseChk, &QCheckBox::toggled, this, updateMultiFxSyncOffsetEnabled);

    // ---- Transition panel link ----------------------------------------------
    QGroupBox* spatialGrp = new QGroupBox(tr("Transitions + Continuous FX"), xfTab);
    QVBoxLayout* spatialLay = new QVBoxLayout(spatialGrp);

    m_spatialChk = new QCheckBox(tr("Enable spatial transition on row recall"), spatialGrp);
    m_spatialChk->setChecked(spatialEffects.enabled);
    spatialLay->addWidget(m_spatialChk);

    QFormLayout* spatialForm = new QFormLayout;
    m_transitionLinkCombo = new QComboBox(spatialGrp);
    m_transitionLinkCombo->addItem(tr("(None)"), QVariant::fromValue(quint32(VCWidget::invalidId())));
    for (VCWidget* w : PresetTableV2VCLookup::allTransitionWidgets())
        m_transitionLinkCombo->addItem(PresetTableV2VCLookup::vcWidgetLabel(w),
                                       QVariant::fromValue(w->id()));
    for (int i = 0; i < m_transitionLinkCombo->count(); ++i)
    {
        if (m_transitionLinkCombo->itemData(i).toUInt() == linkedTransitionWidgetId)
        {
            m_transitionLinkCombo->setCurrentIndex(i);
            break;
        }
    }
    spatialForm->addRow(tr("Transition widget:"), m_transitionLinkCombo);
    spatialLay->addLayout(spatialForm);

    QLabel* spatialHint = new QLabel(
            tr("Add a „Preset Table v2 Transition” widget on the VC and select it here. "
               "Each output chooses a Transition preset for row recall and a Continuous FX preset for live primary↔secondary interpolation."),
            spatialGrp);
    spatialHint->setWordWrap(true);
    {
        QFont hf = spatialHint->font();
        hf.setItalic(true);
        spatialHint->setFont(hf);
    }
    spatialLay->addWidget(spatialHint);

    QLabel* xfEfxHint = new QLabel(
            tr("Crossfade commits staged selections: primary row, secondary row, Transition preset and Continuous FX preset. "
               "EFX parameters are always live. Continuous FX wins over Transitions when both are on and a secondary row is active."),
            spatialGrp);
    xfEfxHint->setWordWrap(true);
  {
        QFont hf = xfEfxHint->font();
        hf.setItalic(true);
        xfEfxHint->setFont(hf);
    }
    spatialLay->addWidget(xfEfxHint);

    connect(m_transitionLinkCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        PresetTableV2TransitionProviderIface* provider =
                transitionProviderForWidgetId(m_transitionLinkCombo->currentData().toUInt());
        for (FGOutputEditorRow* row : m_fgOutputRows)
            row->setTransitionProvider(provider);
    });

    xfTabLayout->addWidget(spatialGrp);
    xfTabLayout->addStretch();
    tabs->addTab(xfTab, tr("Crossfade / Transitions"));
    root->addWidget(tabs);

    // ---- Error label --------------------------------------------------------
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #ff4444;"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    root->addWidget(m_errorLabel);

    // ---- Buttons ------------------------------------------------------------
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    // ---- Populate output rows -----------------------------------------------
    bool isFG = (mode == PTMode::FixtureGroup || mode == PTMode::Position);
    FixtureGroup* grp = isFG ? m_doc->fixtureGroup(fixtureGroupId) : nullptr;

    PresetTableV2TransitionProviderIface* transitionProvider =
            transitionProviderForWidgetId(linkedTransitionWidgetId);

    for (int i = 0; i < outputs.size(); ++i)
    {
        QSharedPointer<QLCInputSource> src = (i < sources.size()) ? sources[i] : QSharedPointer<QLCInputSource>();

        if (!isFG)
        {
            auto* row = new OutputEditorRow(m_doc, outputs[i], src, m_columns.size(), m_widgetPage, this);
            m_outputRows.append(row);
            connect(row, &OutputEditorRow::fixtureSelected,
                    this, &PresetTableV2ConfigDialog::slotAutoCreateColumns);
            QListWidgetItem* item = new QListWidgetItem(m_outputList);
            item->setSizeHint(row->sizeHint());
            m_outputList->addItem(item);
            m_outputList->setItemWidget(item, row);
        }
        else
        {
            QSharedPointer<QLCInputSource> sweepSrc;
            QSharedPointer<QLCInputSource> contSrc;
            QSharedPointer<QLCInputSource> motionSrc;
            QSharedPointer<QLCInputSource> channel1DSrc;
            QSharedPointer<QLCInputSource> multiFxSrc;
            QSharedPointer<QLCInputSource> secSrc;
            if (m_ptWidget)
            {
                if (i < PTInputId::kMaxRoutableOutputs)
                {
                    sweepSrc = m_ptWidget->inputSource(PTInputId::transSweep(i));
                    contSrc = m_ptWidget->inputSource(PTInputId::transContinuousBank(i));
                    motionSrc = m_ptWidget->inputSource(PTInputId::positionMotionBank(i));
                    channel1DSrc = m_ptWidget->inputSource(PTInputId::channel1DBank(i));
                    multiFxSrc = m_ptWidget->inputSource(PTInputId::multiFxBank(i));
                    secSrc = m_ptWidget->inputSource(PTInputId::transSecondaryRow(i));
                }
            }
            auto* row = new FGOutputEditorRow(m_doc, outputs[i], src, sweepSrc, contSrc,
                                              motionSrc, channel1DSrc,
                                              multiFxSrc, secSrc,
                                              grp, m_widgetPage, transitionProvider, m_ptWidget, this);
            m_fgOutputRows.append(row);
            QListWidgetItem* item = new QListWidgetItem(m_outputList);
            item->setSizeHint(row->sizeHint());
            m_outputList->addItem(item);
            m_outputList->setItemWidget(item, row);
        }
    }

    updateHintLabel();
    if (m_configTabs && m_colTabIndex >= 0)
        m_configTabs->setTabVisible(m_colTabIndex, mode != PTMode::Position);
    if (m_configTabs && m_positionTabIndex >= 0)
        m_configTabs->setTabVisible(m_positionTabIndex, mode == PTMode::Position);

    // ---- Populate column table ----------------------------------------------
    rebuildColumnTable();
    updateColumnButtons();

    // ---- Connections --------------------------------------------------------
    connect(m_addOutBtn,  &QPushButton::clicked, this, &PresetTableV2ConfigDialog::slotAddOutput);
    connect(m_remOutBtn,  &QPushButton::clicked, this, &PresetTableV2ConfigDialog::slotRemoveOutput);

    // Column tab
    connect(m_editColBtn,       &QPushButton::clicked,
            this, &PresetTableV2ConfigDialog::slotEditColumn);
    connect(m_addColBtn,        &QPushButton::clicked,
            this, &PresetTableV2ConfigDialog::slotColumnsAddBlank);
    connect(m_remColsBtn,       &QPushButton::clicked,
            this, &PresetTableV2ConfigDialog::slotColumnsRemoveSelected);
    connect(m_addChFromGrpBtn,  &QPushButton::clicked,
            this, &PresetTableV2ConfigDialog::slotColumnsAddFromChannels);
    connect(m_colTable, &QTableWidget::cellDoubleClicked,
            this, [this](int /*row*/, int /*col*/) { slotEditColumn(); });
    connect(m_colTable, &QTableWidget::cellChanged,
            this, &PresetTableV2ConfigDialog::slotColTableCellChanged);
    connect(m_colTable, &QTableWidget::itemChanged,
            this, &PresetTableV2ConfigDialog::slotColTableItemChanged);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &PresetTableV2ConfigDialog::slotValidate);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_modeCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresetTableV2ConfigDialog::slotModeComboChanged);
    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PresetTableV2ConfigDialog::slotGroupComboChanged);
}

// ==========================================================================
// Accessors
// ==========================================================================

QVector<PTOutput> PresetTableV2ConfigDialog::outputs() const
{
    QVector<PTOutput> result;
    PTMode curMode = widgetMode();

    if (curMode == PTMode::Legacy)
    {
        for (const auto* row : m_outputRows)
            result.append(row->output());
    }
    else
    {
        for (const auto* row : m_fgOutputRows)
            result.append(row->output());
    }
    return result;
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::inputSource(int outputIdx) const
{
    PTMode curMode = widgetMode();

    if (curMode == PTMode::Legacy)
    {
        if (outputIdx < 0 || outputIdx >= m_outputRows.size())
            return QSharedPointer<QLCInputSource>();
        return m_outputRows[outputIdx]->inputSource();
    }
    else
    {
        if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
            return QSharedPointer<QLCInputSource>();
        return m_fgOutputRows[outputIdx]->inputSource();
    }
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::transSweepInputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_fgOutputRows[outputIdx]->transSweepInputSource();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::transContinuousInputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_fgOutputRows[outputIdx]->transContinuousInputSource();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionMotionInputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_fgOutputRows[outputIdx]->positionMotionInputSource();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::channel1DInputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_fgOutputRows[outputIdx]->channel1DInputSource();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::multiFxInputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_fgOutputRows[outputIdx]->multiFxInputSource();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::transSecondaryInputSource(int outputIdx) const
{
    if (outputIdx < 0 || outputIdx >= m_fgOutputRows.size())
        return QSharedPointer<QLCInputSource>();
    return m_fgOutputRows[outputIdx]->transSecondaryInputSource();
}

bool PresetTableV2ConfigDialog::crossfadeEnabled() const
{
    return m_crossfadeChk ? m_crossfadeChk->isChecked() : false;
}

bool PresetTableV2ConfigDialog::syncMultiFxPhaseToCrossfade() const
{
    return m_syncMultiFxPhaseChk ? m_syncMultiFxPhaseChk->isChecked() : false;
}

int PresetTableV2ConfigDialog::multiFxCrossfadeSyncOffsetMs() const
{
    return m_multiFxSyncOffsetSpin ? m_multiFxSyncOffsetSpin->value() : 40;
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::crossfadeInputSource() const
{
    return m_xfadeInputSel ? m_xfadeInputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::multiFxBlendInputSource() const
{
    return m_multiFxBlendInputSel ? m_multiFxBlendInputSel->inputSource()
                                  : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::multiFxRestartInputSource() const
{
    return m_multiFxRestartInputSel ? m_multiFxRestartInputSel->inputSource()
                                    : QSharedPointer<QLCInputSource>();
}

QKeySequence PresetTableV2ConfigDialog::multiFxRestartKeySequence() const
{
    return m_multiFxRestartInputSel
            ? VCWidget::stripKeySequence(m_multiFxRestartInputSel->keySequence())
            : QKeySequence();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::widgetFlashGateInputSource() const
{
    return m_widgetFlashGateInputSel ? m_widgetFlashGateInputSel->inputSource()
                                     : QSharedPointer<QLCInputSource>();
}

QKeySequence PresetTableV2ConfigDialog::widgetFlashGateKeySequence() const
{
    return m_widgetFlashGateInputSel
            ? VCWidget::stripKeySequence(m_widgetFlashGateInputSel->keySequence())
            : QKeySequence();
}

int PresetTableV2ConfigDialog::widgetFlashTimeMultiplierIndex() const
{
    return m_widgetFlashTimeMultCombo ? m_widgetFlashTimeMultCombo->currentData().toInt()
                                      : 2;
}

PTWidgetFlashBehavior PresetTableV2ConfigDialog::widgetFlashBehavior() const
{
    const int value = m_widgetFlashBehaviorCombo
            ? m_widgetFlashBehaviorCombo->currentData().toInt()
            : int(PTWidgetFlashBehavior::PrimaryRowModifier);
    return value == int(PTWidgetFlashBehavior::StagedRowTrigger)
            ? PTWidgetFlashBehavior::StagedRowTrigger
            : PTWidgetFlashBehavior::PrimaryRowModifier;
}

PTContinuousFxSelectorMode PresetTableV2ConfigDialog::continuousFxSelectorMode() const
{
    if (!m_contFxModeCombo)
        return PTContinuousFxSelectorMode::StagedCommit;
    return static_cast<PTContinuousFxSelectorMode>(m_contFxModeCombo->currentData().toInt());
}

bool PresetTableV2ConfigDialog::spatialEffectsEnabled() const
{
    return m_spatialChk && m_spatialChk->isChecked();
}

quint32 PresetTableV2ConfigDialog::linkedTransitionWidgetId() const
{
    return m_transitionLinkCombo ? m_transitionLinkCombo->currentData().toUInt() : VCWidget::invalidId();
}

QString PresetTableV2ConfigDialog::widgetCaption() const
{
    return m_captionEdit ? m_captionEdit->text().trimmed() : QString();
}

PTMode PresetTableV2ConfigDialog::widgetMode() const
{
    if (!m_modeCombo) return PTMode::Legacy;
    return PTMode(m_modeCombo->currentData().toInt());
}

quint32 PresetTableV2ConfigDialog::selectedFixtureGroupId() const
{
    if (!m_groupCombo || m_groupCombo->count() == 0) return UINT_MAX;
    return m_groupCombo->currentData().toUInt();
}

bool PresetTableV2ConfigDialog::positionConfirmDiscardDraft() const
{
    return m_positionConfirmDiscardDraftChk
            ? m_positionConfirmDiscardDraftChk->isChecked()
            : true;
}

bool PresetTableV2ConfigDialog::positionShowStatusStrip() const
{
    return m_positionShowStatusStripChk
            ? m_positionShowStatusStripChk->isChecked()
            : true;
}

bool PresetTableV2ConfigDialog::positionShowEditorHints() const
{
    return m_positionShowEditorHintsChk
            ? m_positionShowEditorHintsChk->isChecked()
            : true;
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionBasePanInputSource() const
{
    return m_positionBasePanInputSel ? m_positionBasePanInputSel->inputSource()
                                     : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionBaseTiltInputSource() const
{
    return m_positionBaseTiltInputSel ? m_positionBaseTiltInputSel->inputSource()
                                      : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionSpreadPanInputSource() const
{
    return m_positionSpreadPanInputSel ? m_positionSpreadPanInputSel->inputSource()
                                       : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionSpreadTiltInputSource() const
{
    return m_positionSpreadTiltInputSel ? m_positionSpreadTiltInputSel->inputSource()
                                        : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionSpreadPanEnableInputSource() const
{
    return m_positionSpreadPanEnableInputSel
            ? m_positionSpreadPanEnableInputSel->inputSource()
            : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2ConfigDialog::positionSpreadTiltEnableInputSource() const
{
    return m_positionSpreadTiltEnableInputSel
            ? m_positionSpreadTiltEnableInputSel->inputSource()
            : QSharedPointer<QLCInputSource>();
}

// ==========================================================================
// Private helpers
// ==========================================================================

void PresetTableV2ConfigDialog::updateHintLabel()
{
    if (!m_outHintLabel) return;

    if (widgetMode() == PTMode::Legacy)
    {
        m_outHintLabel->setText(
            tr("Each output must be assigned to a fixture with exactly %n channel(s). "
               "External input: value 0 = off, value N = row N.", "", m_columns.size()));
    }
    else
    {
        FixtureGroup* grp = currentFixtureGroup();
        QString grpName = grp ? grp->name() : tr("<none selected>");
        m_outHintLabel->setText(
            tr("Fixture Group: %1. Each output covers one or more grid rows. "
               "External input: value 0 = off, value N = row N.").arg(grpName));
    }
}

void PresetTableV2ConfigDialog::rebuildGroupCombo()
{
    if (!m_groupCombo) return;
    m_groupCombo->clear();
    for (FixtureGroup* grp : m_doc->fixtureGroups())
        m_groupCombo->addItem(grp->name(), grp->id());
    if (m_groupCombo->count() == 0)
        m_groupCombo->addItem(tr("<no fixture groups>"), QVariant(UINT_MAX));
}

FixtureGroup* PresetTableV2ConfigDialog::currentFixtureGroup() const
{
    if (!m_groupCombo || m_groupCombo->count() == 0) return nullptr;
    quint32 id = m_groupCombo->currentData().toUInt();
    return (id != UINT_MAX) ? m_doc->fixtureGroup(id) : nullptr;
}

void PresetTableV2ConfigDialog::rebuildOutputList()
{
    // Clear all rows from the list widget
    m_outputList->clear();
    qDeleteAll(m_outputRows);
    m_outputRows.clear();
    qDeleteAll(m_fgOutputRows);
    m_fgOutputRows.clear();
}

// ==========================================================================
// Slots
// ==========================================================================

void PresetTableV2ConfigDialog::slotModeComboChanged(int /*index*/)
{
    PTMode newMode = widgetMode();
    bool isFG = (newMode == PTMode::FixtureGroup || newMode == PTMode::Position);

    m_groupRow->setVisible(isFG);
    if (m_configTabs && m_colTabIndex >= 0)
        m_configTabs->setTabVisible(m_colTabIndex, newMode != PTMode::Position);
    if (m_configTabs && m_positionTabIndex >= 0)
        m_configTabs->setTabVisible(m_positionTabIndex, newMode == PTMode::Position);
    updateHintLabel();
    updateColumnButtons();

    // Warn about clearing outputs when switching modes
    if (m_outputList->count() > 0)
    {
        int ret = QMessageBox::question(this,
            tr("Change Mode"),
            tr("Switching mode will clear all current output assignments. Continue?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
        {
            // Revert combo silently
            QSignalBlocker sb(m_modeCombo);
            const int legacyIdx = m_modeCombo->findData(int(isFG ? PTMode::Legacy : PTMode::FixtureGroup));
            m_modeCombo->setCurrentIndex(legacyIdx >= 0 ? legacyIdx : 0);
            m_groupRow->setVisible(!isFG);
            updateHintLabel();
            return;
        }
    }

    rebuildOutputList();
}

void PresetTableV2ConfigDialog::slotGroupComboChanged(int /*index*/)
{
    updateHintLabel();
    updateColumnButtons();

    // Update existing FG output rows with new group
    FixtureGroup* grp = currentFixtureGroup();
    for (FGOutputEditorRow* row : m_fgOutputRows)
        row->setFixtureGroup(grp);

    // Resize list items to fit new row heights
    for (int i = 0; i < m_outputList->count(); ++i)
    {
        QListWidgetItem* item = m_outputList->item(i);
        QWidget* w = m_outputList->itemWidget(item);
        if (w) item->setSizeHint(w->sizeHint());
    }
}

void PresetTableV2ConfigDialog::slotAddOutput()
{
    PTOutput out;
    PTMode curMode = widgetMode();
    const int currentOutputs = (curMode == PTMode::Legacy)
            ? m_outputRows.size() : m_fgOutputRows.size();
    if (currentOutputs >= PTInputId::kMaxRoutableOutputs)
    {
        QMessageBox::information(
                this, tr("Output limit"),
                tr("Preset Table v2 supports up to %1 outputs.")
                        .arg(PTInputId::kMaxRoutableOutputs));
        return;
    }

    if (curMode == PTMode::Legacy)
    {
        out.name = tr("Output %1").arg(m_outputRows.size() + 1);
        auto* row = new OutputEditorRow(m_doc, out, QSharedPointer<QLCInputSource>(),
                                         m_columns.size(), m_widgetPage, this);
        m_outputRows.append(row);
        connect(row, &OutputEditorRow::fixtureSelected,
                this, &PresetTableV2ConfigDialog::slotAutoCreateColumns);

        QListWidgetItem* item = new QListWidgetItem(m_outputList);
        item->setSizeHint(row->sizeHint());
        m_outputList->addItem(item);
        m_outputList->setItemWidget(item, row);
    }
    else
    {
        out.name = tr("Output %1").arg(m_fgOutputRows.size() + 1);
        FixtureGroup* grp = currentFixtureGroup();
        if (grp)
        {
            const int h = grp->size().height();
            for (int y = 0; y < h; ++y)
                out.groupRows.append(y);
        }
        PresetTableV2TransitionProviderIface* provider =
                transitionProviderForWidgetId(
                    m_transitionLinkCombo ? m_transitionLinkCombo->currentData().toUInt()
                                        : VCWidget::invalidId());
        QSharedPointer<QLCInputSource> sweepSrc;
        QSharedPointer<QLCInputSource> contSrc;
        QSharedPointer<QLCInputSource> motionSrc;
        QSharedPointer<QLCInputSource> channel1DSrc;
        QSharedPointer<QLCInputSource> multiFxSrc;
        QSharedPointer<QLCInputSource> secSrc;
        const int o = m_fgOutputRows.size();
        if (m_ptWidget)
        {
            if (o < PTInputId::kMaxRoutableOutputs)
            {
                sweepSrc = m_ptWidget->inputSource(PTInputId::transSweep(o));
                contSrc = m_ptWidget->inputSource(PTInputId::transContinuousBank(o));
                motionSrc = m_ptWidget->inputSource(PTInputId::positionMotionBank(o));
                channel1DSrc = m_ptWidget->inputSource(PTInputId::channel1DBank(o));
                multiFxSrc = m_ptWidget->inputSource(PTInputId::multiFxBank(o));
                secSrc = m_ptWidget->inputSource(PTInputId::transSecondaryRow(o));
            }
        }
        auto* row = new FGOutputEditorRow(m_doc, out, QSharedPointer<QLCInputSource>(),
                                           sweepSrc, contSrc, motionSrc, channel1DSrc,
                                           multiFxSrc, secSrc, grp, m_widgetPage, provider,
                                           m_ptWidget, this);
        m_fgOutputRows.append(row);

        QListWidgetItem* item = new QListWidgetItem(m_outputList);
        item->setSizeHint(row->sizeHint());
        m_outputList->addItem(item);
        m_outputList->setItemWidget(item, row);
    }

    m_outputList->scrollToBottom();
}

void PresetTableV2ConfigDialog::slotRemoveOutput()
{
    int idx = m_outputList->currentRow();
    if (idx < 0) return;

    PTMode curMode = widgetMode();
    if (curMode == PTMode::Legacy)
    {
        if (idx >= m_outputRows.size()) return;
        m_outputRows.removeAt(idx);
    }
    else
    {
        if (idx >= m_fgOutputRows.size()) return;
        m_fgOutputRows.removeAt(idx);
    }
    delete m_outputList->takeItem(idx);
}

void PresetTableV2ConfigDialog::slotEditColumn()
{
    int idx = m_colTable->currentRow();
    if (idx < 0 || idx >= m_columns.size()) return;

    PTMode   curMode = widgetMode();
    FixtureGroup* grp = (curMode == PTMode::FixtureGroup || curMode == PTMode::Position)
            ? currentFixtureGroup() : nullptr;

    PresetTableV2ColumnDialog dlg(m_doc, m_columns[idx], curMode, grp, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_columns[idx] = dlg.column();
    rebuildColumnTable();

    // Keep same row selected
    m_colTable->selectRow(idx);

    updateHintLabel();
    if (curMode == PTMode::Legacy)
    {
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
    }
}

// ---------------------------------------------------------------------------
// Column table helpers
// ---------------------------------------------------------------------------

void PresetTableV2ConfigDialog::rebuildColumnTable()
{
    if (!m_colTable) return;
    m_updatingColTable = true;
    m_colTable->blockSignals(true);
    m_colTable->setColumnHidden(3, widgetMode() != PTMode::FixtureGroup);

    m_colTable->setRowCount(0);
    for (int r = 0; r < m_columns.size(); ++r)
    {
        const PTColumn& col = m_columns[r];
        m_colTable->insertRow(r);

        // Col 0: Name — editable text
        QTableWidgetItem* nameItem = new QTableWidgetItem(col.name);
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        m_colTable->setItem(r, 0, nameItem);

        // Col 1: Type — inline QComboBox
        QComboBox* typeCombo = new QComboBox(m_colTable);
        typeCombo->addItem(tr("Numeric"),  int(PTColumn::Numeric));
        typeCombo->addItem(tr("Dropdown"), int(PTColumn::Dropdown));
        typeCombo->addItem(tr("Scaler"),   int(PTColumn::Scaler));
        typeCombo->setCurrentIndex(int(col.type));
        int capturedRow = r;
        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, capturedRow](int comboIdx) {
                    if (m_updatingColTable) return;
                    if (capturedRow < 0 || capturedRow >= m_columns.size()) return;
                    m_columns[capturedRow].type = PTColumn::Type(comboIdx);
                    // Refresh the Binding cell summary
                    if (auto* bi = m_colTable->item(capturedRow, 4))
                        bi->setText(bindingSummary(m_columns[capturedRow]));
                });
        m_colTable->setCellWidget(r, 1, typeCombo);

        // Col 2: Fade — checkable item
        QTableWidgetItem* fadeItem = new QTableWidgetItem();
        fadeItem->setFlags((fadeItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        fadeItem->setCheckState(col.fade ? Qt::Checked : Qt::Unchecked);
        m_colTable->setItem(r, 2, fadeItem);

        // Col 3: 1D FX — explicit effect target
        QTableWidgetItem* fxItem = new QTableWidgetItem();
        fxItem->setFlags((fxItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        fxItem->setCheckState(col.useFor1DFx ? Qt::Checked : Qt::Unchecked);
        fxItem->setToolTip(tr("1D Channel FX presets affect this column."));
        m_colTable->setItem(r, 3, fxItem);

        // Col 4: Binding — read-only summary
        QTableWidgetItem* bindItem = new QTableWidgetItem(bindingSummary(col));
        bindItem->setFlags(bindItem->flags() & ~Qt::ItemIsEditable);
        m_colTable->setItem(r, 4, bindItem);
    }

    m_colTable->blockSignals(false);
    m_updatingColTable = false;
}

QString PresetTableV2ConfigDialog::bindingSummary(const PTColumn& col) const
{
    if (!col.hasBindings())
        return tr("—");

    int validCount = 0;
    QSet<QString> typeKeys;
    QString firstLabel;
    for (const PTColumnTypeBinding& binding : col.bindings)
    {
        if (!binding.isValid())
            continue;
        ++validCount;
        typeKeys.insert(QStringLiteral("%1|%2|%3")
                .arg(binding.manufacturer, binding.model, binding.modeName));

        if (!firstLabel.isEmpty())
            continue;

        if (m_doc)
        {
            FixtureGroup* grp = currentFixtureGroup();
            if (grp)
            {
                for (quint32 fxiId : grp->fixtureList())
                {
                    Fixture* fxi = m_doc->fixture(fxiId);
                    if (!fxi) continue;
                    QLCFixtureDef*  def  = fxi->fixtureDef();
                    QLCFixtureMode* mode = fxi->fixtureMode();
                    if (!def || !mode) continue;
                    if (def->manufacturer() == binding.manufacturer &&
                        def->model()        == binding.model &&
                        mode->name()        == binding.modeName)
                    {
                        QLCChannel* ch = mode->channel(quint32(binding.channelIndex));
                        if (ch)
                            firstLabel = ch->name();
                        break;
                    }
                }
            }
        }
        if (firstLabel.isEmpty())
            firstLabel = QString("ch%1").arg(binding.channelIndex + 1);
    }

    if (validCount == 1)
        return firstLabel;
    if (validCount > 1 && typeKeys.size() == 1)
        return tr("%1 (+%2)").arg(firstLabel).arg(validCount - 1);
    return tr("%1 ch, %2 types").arg(validCount).arg(typeKeys.size());
}

void PresetTableV2ConfigDialog::updateColumnButtons()
{
    bool fgMode = (widgetMode() == PTMode::FixtureGroup || widgetMode() == PTMode::Position);
    bool hasGrp = (currentFixtureGroup() != nullptr);
    if (m_addChFromGrpBtn)
        m_addChFromGrpBtn->setEnabled(fgMode && hasGrp);
}

// ---------------------------------------------------------------------------
// Column table inline-edit slots
// ---------------------------------------------------------------------------

void PresetTableV2ConfigDialog::slotColTableCellChanged(int row, int col)
{
    if (m_updatingColTable) return;
    if (row < 0 || row >= m_columns.size()) return;
    if (col == 0)
    {
        auto* item = m_colTable->item(row, 0);
        if (item) m_columns[row].name = item->text();
    }
}

void PresetTableV2ConfigDialog::slotColTableItemChanged(QTableWidgetItem* item)
{
    if (m_updatingColTable) return;
    if (!item || (item->column() != 2 && item->column() != 3)) return;
    int row = item->row();
    if (row < 0 || row >= m_columns.size()) return;
    if (item->column() == 2)
        m_columns[row].fade = (item->checkState() == Qt::Checked);
    else
        m_columns[row].useFor1DFx = (item->checkState() == Qt::Checked);
}

// ---------------------------------------------------------------------------
// Column toolbar button slots
// ---------------------------------------------------------------------------

void PresetTableV2ConfigDialog::slotColumnsAddBlank()
{
    PTColumn col;
    col.name = tr("Column %1").arg(m_columns.size() + 1);
    col.type = PTColumn::Numeric;
    col.fade = true;
    m_columns.append(col);
    rebuildColumnTable();
    m_colTable->selectRow(m_columns.size() - 1);
    if (widgetMode() == PTMode::Legacy)
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
    updateHintLabel();
}

void PresetTableV2ConfigDialog::slotColumnsRemoveSelected()
{
    QList<int> rows;
    const auto selected = m_colTable->selectionModel()->selectedRows();
    for (const QModelIndex& idx : selected)
        rows.append(idx.row());
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        if (r >= 0 && r < m_columns.size())
            m_columns.removeAt(r);
    rebuildColumnTable();
    if (widgetMode() == PTMode::Legacy)
        for (auto* row : m_outputRows)
            row->setRequiredChannels(m_columns.size());
    updateHintLabel();
}

void PresetTableV2ConfigDialog::slotColumnsAddFromChannels()
{
    FixtureGroup* grp = currentFixtureGroup();
    if (!grp || !m_doc) return;

    // Build deduplicated fixture-type list from the group
    struct FxEntry {
        QString manufacturer, model, modeName;
        QLCFixtureMode* mode = nullptr;
    };
    QList<FxEntry> fxTypes;
    for (quint32 fxiId : grp->fixtureList())
    {
        Fixture* fxi = m_doc->fixture(fxiId);
        if (!fxi) continue;
        QLCFixtureDef*  def  = fxi->fixtureDef();
        QLCFixtureMode* mode = fxi->fixtureMode();
        if (!def || !mode) continue;
        bool found = false;
        for (const FxEntry& e : fxTypes)
            if (e.manufacturer == def->manufacturer() &&
                e.model        == def->model() &&
                e.modeName     == mode->name())
            { found = true; break; }
        if (!found)
            fxTypes.append({def->manufacturer(), def->model(), mode->name(), mode});
    }
    if (fxTypes.isEmpty())
    {
        QMessageBox::information(this, tr("No fixtures"),
            tr("The selected fixture group contains no fixture types with definitions."));
        return;
    }

    // ---- Build inline picker dialog ----------------------------------------
    QDialog picker(this);
    picker.setWindowTitle(tr("Add columns from group channels"));
    picker.resize(520, 440);
    QVBoxLayout* pl = new QVBoxLayout(&picker);

    // Fixture type row
    QHBoxLayout* typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel(tr("Fixture type:"), &picker));
    QComboBox* typeCombo = new QComboBox(&picker);
    for (const FxEntry& e : fxTypes)
        typeCombo->addItem(QString("%1 %2 — %3").arg(e.manufacturer, e.model, e.modeName));
    typeRow->addWidget(typeCombo, 1);
    pl->addLayout(typeRow);

    // Channel list
    QListWidget* chanList = new QListWidget(&picker);
    chanList->setSelectionMode(QAbstractItemView::NoSelection);
    pl->addWidget(chanList, 1);

    // Lambda: rebuild channel list for the chosen fixture type
    auto populateChannels = [&](int typeIdx)
    {
        chanList->clear();
        if (typeIdx < 0 || typeIdx >= fxTypes.size()) return;
        QLCFixtureMode* fxMode = fxTypes[typeIdx].mode;
        if (!fxMode) return;

        const QVector<QLCFixtureHead>& heads = fxMode->heads();
        int nCh = fxMode->channels().size();
        for (int ci = 0; ci < nCh; ++ci)
        {
            QLCChannel* ch = fxMode->channel(quint32(ci));

            // Determine head label
            int headIdx = -1;
            for (int hi = 0; hi < heads.size(); ++hi)
                if (heads[hi].channels().contains(quint32(ci))) { headIdx = hi; break; }

            QString prefix;
            if (!heads.isEmpty())
                prefix = (headIdx >= 0) ? QString("[H%1] ").arg(headIdx) : tr("[shared] ");

            QString label = prefix + (ch
                ? QString("%1: %2").arg(ci + 1).arg(ch->name())
                : tr("Ch %1").arg(ci + 1));

            QListWidgetItem* item = new QListWidgetItem(label, chanList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            item->setData(Qt::UserRole, ci);
        }
    };
    populateChannels(0);

    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &picker, [&](int idx) { populateChannels(idx); });

    // Select-all / Clear row
    QHBoxLayout* selRow = new QHBoxLayout;
    QPushButton* selAll  = new QPushButton(tr("Select all"), &picker);
    QPushButton* selNone = new QPushButton(tr("Clear"),      &picker);
    selRow->addWidget(selAll);
    selRow->addWidget(selNone);
    selRow->addStretch();
    pl->addLayout(selRow);

    connect(selAll,  &QPushButton::clicked, &picker, [&] {
        for (int i = 0; i < chanList->count(); ++i)
            chanList->item(i)->setCheckState(Qt::Checked);
    });
    connect(selNone, &QPushButton::clicked, &picker, [&] {
        for (int i = 0; i < chanList->count(); ++i)
            chanList->item(i)->setCheckState(Qt::Unchecked);
    });

    QDialogButtonBox* pb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &picker);
    pl->addWidget(pb);
    connect(pb, &QDialogButtonBox::accepted, &picker, &QDialog::accept);
    connect(pb, &QDialogButtonBox::rejected, &picker, &QDialog::reject);

    if (picker.exec() != QDialog::Accepted) return;

    // ---- Collect selected channels and append PTColumns --------------------
    int typeIdx = typeCombo->currentIndex();
    if (typeIdx < 0 || typeIdx >= fxTypes.size()) return;
    const FxEntry& fxEntry = fxTypes[typeIdx];
    QLCFixtureMode* fxMode = fxEntry.mode;

    bool anyAdded = false;
    for (int i = 0; i < chanList->count(); ++i)
    {
        QListWidgetItem* item = chanList->item(i);
        if (item->checkState() != Qt::Checked) continue;

        int chanIdx = item->data(Qt::UserRole).toInt();
        QLCChannel* ch = fxMode ? fxMode->channel(quint32(chanIdx)) : nullptr;

        PTColumn col;
        col.name               = ch ? ch->name() : tr("Ch %1").arg(chanIdx + 1);
        col.type               = PTColumn::Numeric;
        col.fade               = true;
        PTColumnTypeBinding binding;
        binding.manufacturer = fxEntry.manufacturer;
        binding.model        = fxEntry.model;
        binding.modeName     = fxEntry.modeName;
        binding.channelIndex = chanIdx;
        col.bindings.append(binding);
        m_columns.append(col);
        anyAdded = true;
    }

    if (anyAdded)
    {
        rebuildColumnTable();
        if (widgetMode() == PTMode::Legacy)
            for (auto* row : m_outputRows)
                row->setRequiredChannels(m_columns.size());
        updateHintLabel();
    }
}

void PresetTableV2ConfigDialog::slotAutoCreateColumns(quint32 fixtureId)
{
    // Only auto-create when in Legacy mode and there are no columns yet
    if (widgetMode() != PTMode::Legacy) return;
    if (!m_columns.isEmpty()) return;

    Fixture* fx = m_doc->fixture(fixtureId);
    if (!fx || fx->channels() == 0) return;

    m_columns.clear();
    for (quint32 ch = 0; ch < fx->channels(); ++ch)
    {
        PTColumn col;
        const QLCChannel* qlcCh = fx->channel(ch);
        col.name = qlcCh ? qlcCh->name() : tr("Ch %1").arg(ch + 1);
        col.type = PTColumn::Numeric;
        col.fade = true;
        m_columns.append(col);
    }

    rebuildColumnTable();
    updateHintLabel();

    int n = m_columns.size();
    for (auto* row : m_outputRows)
        row->setRequiredChannels(n);
}

void PresetTableV2ConfigDialog::slotValidate()
{
    PTMode curMode = widgetMode();
    QStringList errors;

    if (curMode == PTMode::Legacy)
    {
        int required = m_columns.size();
        for (int i = 0; i < m_outputRows.size(); ++i)
        {
            PTOutput out = m_outputRows[i]->output();
            if (out.fixtureId == UINT_MAX) continue;  // unassigned is ok

            Fixture* fx = m_doc->fixture(out.fixtureId);
            if (!fx)
            {
                errors.append(tr("Output \"%1\": fixture not found.").arg(out.name));
                continue;
            }
            if ((int)fx->channels() != required)
            {
                errors.append(tr("Output \"%1\": fixture has %2 channels but table has %3 value columns.")
                    .arg(out.name).arg(fx->channels()).arg(required));
            }
        }
    }
    else
    {
        // FixtureGroup mode validation
        FixtureGroup* grp = currentFixtureGroup();
        if (!grp)
        {
            errors.append(tr("No Fixture Group selected."));
        }
        else
        {
            for (int i = 0; i < m_fgOutputRows.size(); ++i)
            {
                PTOutput out = m_fgOutputRows[i]->output();
                if (out.scope == PTOutputScope::Mask)
                {
                    if (!m_doc->fixtureGroupMask(currentFixtureGroup()->id()).isActive())
                        errors.append(tr("Output \"%1\": group mask is not active — apply a mask in Fixture Group Layout.")
                                         .arg(out.name));
                }
                else if (out.groupRows.isEmpty())
                {
                    errors.append(tr("Output \"%1\": no grid rows selected.").arg(out.name));
                }
            }

            // Warn if any parameter column has no binding. Position mode resolves Pan/Tilt automatically.
            for (const PTColumn& col : (curMode == PTMode::Position ? QVector<PTColumn>() : m_columns))
            {
                if (!col.hasBindings())
                    errors.append(tr("Column \"%1\" has no fixture binding set.").arg(col.name));
            }
        }
    }

    if (!errors.isEmpty())
    {
        m_errorLabel->setText(errors.join(QLatin1Char('\n')));
        m_errorLabel->setVisible(true);
        return;
    }

    m_errorLabel->setVisible(false);
    accept();
}

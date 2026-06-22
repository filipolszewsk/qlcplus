#include "presettablev2transitionconfigdialog.h"
#include "presettablev2transitionwidget.h"
#include "presettablev2controliface.h"
#include "presettablev2vclookup.h"
#include "ptefxinputids.h"
#include "inputselectionwidget.h"
#include "qlcinputsource.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QCheckBox>

static QString bankLabel(PTTransitionMode mode)
{
    switch (mode)
    {
        case PTTransitionMode::Off:            break;
        case PTTransitionMode::SweepOnly:      return QObject::tr("Transition");
        case PTTransitionMode::Continuous:     return QObject::tr("Interpolation");
        case PTTransitionMode::Channel1D:      return QObject::tr("1D FX");
        case PTTransitionMode::PositionMotion: return QObject::tr("2D FX");
        case PTTransitionMode::MultiFx:        return QObject::tr("MultiFX");
    }
    return QObject::tr("Bank");
}

PresetTableV2TransitionConfigDialog::PresetTableV2TransitionConfigDialog(
        PresetTableV2TransitionWidget* widget, QWidget* parent)
    : QDialog(parent)
    , m_widget(widget)
{
    setWindowTitle(tr("Preset Table Engine — Properties"));
    setMinimumWidth(500);

    QVBoxLayout* root = new QVBoxLayout(this);

    QLabel* hint = new QLabel(
            tr("Global speed, intensity, position size and min/max cycle times are edited here only.\n"
               "Preset banks: Transition / Interpolation / 1D FX / 2D FX / MultiFX tabs on the widget.\n"
               "Per-column external inputs: double-click column headers on the widget."),
            this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    QFormLayout* form = new QFormLayout;

    m_captionEdit = new QLineEdit(this);
    m_captionEdit->setPlaceholderText(tr("e.g. Front Wash - Preset Table Engine"));
    if (m_widget)
        m_captionEdit->setText(m_widget->caption());
    form->addRow(tr("Widget name:"), m_captionEdit);

    m_logVisibleChk = new QCheckBox(tr("Show diagnostics/status text"), this);
    m_logVisibleChk->setChecked(m_widget ? m_widget->logVisible() : false);
    form->addRow(tr("Log:"), m_logVisibleChk);

    m_tableCombo = new QComboBox(this);
    form->addRow(tr("Preset Table v2:"), m_tableCombo);
    root->addLayout(form);

    rebuildTableCombo();

    auto* sourceBox = new QGroupBox(tr("Bank sources"), this);
    auto* sourceForm = new QFormLayout(sourceBox);
    for (PTTransitionMode mode : { PTTransitionMode::SweepOnly,
                                   PTTransitionMode::Continuous,
                                   PTTransitionMode::Channel1D,
                                   PTTransitionMode::PositionMotion,
                                   PTTransitionMode::MultiFx })
    {
        QComboBox* combo = new QComboBox(sourceBox);
        m_bankSourceCombos.insert(int(mode), combo);
        sourceForm->addRow(bankLabel(mode) + QStringLiteral(":"), combo);
    }
    root->addWidget(sourceBox);
    rebuildBankSourceCombos();

    const PTGlobalEffectSettings gs = m_widget ? m_widget->globalEffectSettings()
                                               : PTGlobalEffectSettings();

    auto* globalBox = new QGroupBox(tr("Global effect"), this);
    auto* globalForm = new QFormLayout(globalBox);

    m_speedSlider = new QSlider(Qt::Horizontal, globalBox);
    m_speedSlider->setRange(0, 255);
    m_speedSlider->setValue(gs.speed);
    m_speedValueLabel = new QLabel(QString::number(gs.speed), globalBox);
    m_speedValueLabel->setMinimumWidth(36);
    auto* speedRow = new QHBoxLayout;
    speedRow->addWidget(m_speedSlider, 1);
    speedRow->addWidget(m_speedValueLabel);
    globalForm->addRow(tr("Speed (255=fast):"), speedRow);

    m_intensitySlider = new QSlider(Qt::Horizontal, globalBox);
    m_intensitySlider->setRange(0, 255);
    m_intensitySlider->setValue(gs.intensity);
    m_intensityValueLabel = new QLabel(QString::number(gs.intensity), globalBox);
    m_intensityValueLabel->setMinimumWidth(36);
    auto* intRow = new QHBoxLayout;
    intRow->addWidget(m_intensitySlider, 1);
    intRow->addWidget(m_intensityValueLabel);
    globalForm->addRow(tr("Intensity:"), intRow);

    m_positionSizeSlider = new QSlider(Qt::Horizontal, globalBox);
    m_positionSizeSlider->setRange(0, 255);
    m_positionSizeSlider->setValue(gs.positionSize);
    m_positionSizeValueLabel = new QLabel(QString::number(gs.positionSize), globalBox);
    m_positionSizeValueLabel->setMinimumWidth(36);
    auto* posSizeRow = new QHBoxLayout;
    posSizeRow->addWidget(m_positionSizeSlider, 1);
    posSizeRow->addWidget(m_positionSizeValueLabel);
    globalForm->addRow(tr("Position size:"), posSizeRow);

    m_minDurationSpin = new QSpinBox(globalBox);
    m_minDurationSpin->setRange(20, 60000);
    m_minDurationSpin->setSuffix(tr(" ms"));
    m_minDurationSpin->setValue(int(gs.minDurationMs));
    globalForm->addRow(tr("Min duration (fast):"), m_minDurationSpin);

    m_maxDurationSpin = new QSpinBox(globalBox);
    m_maxDurationSpin->setRange(20, 60000);
    m_maxDurationSpin->setSuffix(tr(" ms"));
    m_maxDurationSpin->setValue(int(gs.maxDurationMs));
    globalForm->addRow(tr("Max duration (slow):"), m_maxDurationSpin);

    m_sizeSpeedCeilingChk = new QCheckBox(tr("Enable size-aware speed ceiling"), globalBox);
    m_sizeSpeedCeilingChk->setChecked(gs.sizeSpeedCeilingEnabled);
    m_sizeSpeedCeilingChk->setToolTip(
            tr("Allows the top end of the speed fader to use a faster cycle when Position Size is small."));
    globalForm->addRow(tr("Size speed ceiling:"), m_sizeSpeedCeilingChk);

    m_smallSizeMinDurationSpin = new QSpinBox(globalBox);
    m_smallSizeMinDurationSpin->setRange(20, 60000);
    m_smallSizeMinDurationSpin->setSuffix(tr(" ms"));
    m_smallSizeMinDurationSpin->setValue(int(gs.smallSizeMinDurationMs));
    m_smallSizeMinDurationSpin->setToolTip(
            tr("Fastest cycle allowed at Position Size 0 when speed is above the overdrive knee."));
    globalForm->addRow(tr("Small-size fastest:"), m_smallSizeMinDurationSpin);

    m_speedOverdriveKneeSlider = new QSlider(Qt::Horizontal, globalBox);
    m_speedOverdriveKneeSlider->setRange(1, 254);
    m_speedOverdriveKneeSlider->setValue(qBound(1, gs.speedOverdriveKnee, 254));
    m_speedOverdriveKneeValueLabel =
            new QLabel(QString::number(m_speedOverdriveKneeSlider->value()), globalBox);
    m_speedOverdriveKneeValueLabel->setMinimumWidth(36);
    auto* kneeRow = new QHBoxLayout;
    kneeRow->addWidget(m_speedOverdriveKneeSlider, 1);
    kneeRow->addWidget(m_speedOverdriveKneeValueLabel);
    globalForm->addRow(tr("Overdrive starts at:"), kneeRow);

    m_effectiveCyclePreviewLabel = new QLabel(globalBox);
    m_effectiveCyclePreviewLabel->setWordWrap(true);
    globalForm->addRow(tr("Current cycle:"), m_effectiveCyclePreviewLabel);

    root->addWidget(globalBox);

    connect(m_speedSlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotSpeedSliderChanged);
    connect(m_intensitySlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotIntensitySliderChanged);
    connect(m_positionSizeSlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotPositionSizeSliderChanged);
    connect(m_speedSlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    connect(m_positionSizeSlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    connect(m_minDurationSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    connect(m_maxDurationSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    connect(m_sizeSpeedCeilingChk, &QCheckBox::toggled,
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    connect(m_smallSizeMinDurationSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    connect(m_speedOverdriveKneeSlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotTimingControlChanged);
    updateEffectiveCyclePreview();

    auto* inputBox = new QGroupBox(tr("External inputs (global)"), this);
    auto* inputForm = new QFormLayout(inputBox);

    Doc* doc = m_widget ? m_widget->doc() : nullptr;
    const int widgetPage = m_widget ? m_widget->page() : 0;

    m_speedInputSel = new InputSelectionWidget(doc, inputBox);
    m_speedInputSel->setKeyInputVisibility(false);
    m_speedInputSel->setWidgetPage(widgetPage);
    if (m_widget)
        m_speedInputSel->setInputSource(m_widget->inputSource(PTEfxCol::InputGlobalSpeed));
    inputForm->addRow(tr("Global speed:"), m_speedInputSel);

    m_intensityInputSel = new InputSelectionWidget(doc, inputBox);
    m_intensityInputSel->setKeyInputVisibility(false);
    m_intensityInputSel->setWidgetPage(widgetPage);
    if (m_widget)
        m_intensityInputSel->setInputSource(m_widget->inputSource(PTEfxCol::InputGlobalIntensity));
    inputForm->addRow(tr("Global intensity:"), m_intensityInputSel);

    m_positionSizeInputSel = new InputSelectionWidget(doc, inputBox);
    m_positionSizeInputSel->setKeyInputVisibility(false);
    m_positionSizeInputSel->setWidgetPage(widgetPage);
    if (m_widget)
        m_positionSizeInputSel->setInputSource(
                m_widget->inputSource(PTEfxCol::InputGlobalPositionSize));
    inputForm->addRow(tr("Global position size:"), m_positionSizeInputSel);

    m_crossfadeManualInputSel = new InputSelectionWidget(doc, inputBox);
    m_crossfadeManualInputSel->setKeyInputVisibility(false);
    m_crossfadeManualInputSel->setWidgetPage(widgetPage);
    if (m_widget)
        m_crossfadeManualInputSel->setInputSource(m_widget->inputSource(PTEfxCol::InputCrossfadeManual));
    inputForm->addRow(tr("Crossfade manual (>127=ON):"), m_crossfadeManualInputSel);

    root->addWidget(inputBox);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &PresetTableV2TransitionConfigDialog::slotValidate);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PresetTableV2TransitionConfigDialog::rebuildBankSourceCombos()
{
    const quint32 selfId = m_widget ? m_widget->id() : VCWidget::invalidId();
    for (auto it = m_bankSourceCombos.begin(); it != m_bankSourceCombos.end(); ++it)
    {
        QComboBox* combo = it.value();
        if (!combo)
            continue;
        const PTTransitionMode mode = PTTransitionMode(it.key());
        combo->clear();
        combo->addItem(tr("Local"), QVariant::fromValue(quint32(VCWidget::invalidId())));

        const quint32 current = m_widget ? m_widget->bankSourceEngineId(mode)
                                         : VCWidget::invalidId();
        bool currentFound = current == VCWidget::invalidId();
        for (VCWidget* w : PresetTableV2VCLookup::allTransitionWidgets())
        {
            if (!w || w->id() == selfId)
                continue;
            if (!qobject_cast<PresetTableV2TransitionProviderIface*>(w))
                continue;
            if (m_widget && m_widget->bankSourceWouldCreateCycle(mode, w->id()))
                continue;
            combo->addItem(PresetTableV2VCLookup::vcWidgetLabel(w),
                           QVariant::fromValue(w->id()));
            if (w->id() == current)
                currentFound = true;
        }
        if (!currentFound && current != VCWidget::invalidId())
            combo->addItem(tr("Missing engine #%1").arg(current), QVariant::fromValue(current));

        for (int i = 0; i < combo->count(); ++i)
        {
            if (combo->itemData(i).toUInt() == current)
            {
                combo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void PresetTableV2TransitionConfigDialog::slotSpeedSliderChanged(int v)
{
    if (m_speedValueLabel)
        m_speedValueLabel->setText(QString::number(v));
}

void PresetTableV2TransitionConfigDialog::slotIntensitySliderChanged(int v)
{
    if (m_intensityValueLabel)
        m_intensityValueLabel->setText(QString::number(v));
}

void PresetTableV2TransitionConfigDialog::slotPositionSizeSliderChanged(int v)
{
    if (m_positionSizeValueLabel)
        m_positionSizeValueLabel->setText(QString::number(v));
}

void PresetTableV2TransitionConfigDialog::slotTimingControlChanged()
{
    if (m_speedOverdriveKneeValueLabel && m_speedOverdriveKneeSlider)
        m_speedOverdriveKneeValueLabel->setText(QString::number(m_speedOverdriveKneeSlider->value()));
    updateEffectiveCyclePreview();
}

void PresetTableV2TransitionConfigDialog::rebuildTableCombo()
{
    m_tableCombo->clear();
    m_tableCombo->addItem(tr("(None)"), QVariant::fromValue(quint32(VCWidget::invalidId())));

    for (VCWidget* w : PresetTableV2VCLookup::allVcWidgets())
    {
        if (!qobject_cast<PresetTableV2ControlIface*>(w))
            continue;
        m_tableCombo->addItem(PresetTableV2VCLookup::vcWidgetLabel(w),
                              QVariant::fromValue(w->id()));
    }

    const quint32 current = m_widget ? m_widget->targetTableId() : VCWidget::invalidId();
    for (int i = 0; i < m_tableCombo->count(); ++i)
    {
        if (m_tableCombo->itemData(i).toUInt() == current)
        {
            m_tableCombo->setCurrentIndex(i);
            break;
        }
    }
}

quint32 PresetTableV2TransitionConfigDialog::targetTableId() const
{
    return m_tableCombo ? m_tableCombo->currentData().toUInt() : VCWidget::invalidId();
}

QString PresetTableV2TransitionConfigDialog::widgetCaption() const
{
    return m_captionEdit ? m_captionEdit->text().trimmed() : QString();
}

PTGlobalEffectSettings PresetTableV2TransitionConfigDialog::globalSettings() const
{
    PTGlobalEffectSettings gs;
    if (m_speedSlider)
        gs.speed = uchar(m_speedSlider->value());
    if (m_intensitySlider)
        gs.intensity = uchar(m_intensitySlider->value());
    if (m_positionSizeSlider)
        gs.positionSize = uchar(m_positionSizeSlider->value());
    if (m_minDurationSpin)
        gs.minDurationMs = quint32(m_minDurationSpin->value());
    if (m_maxDurationSpin)
        gs.maxDurationMs = quint32(m_maxDurationSpin->value());
    if (gs.minDurationMs > gs.maxDurationMs)
        qSwap(gs.minDurationMs, gs.maxDurationMs);
    if (m_sizeSpeedCeilingChk)
        gs.sizeSpeedCeilingEnabled = m_sizeSpeedCeilingChk->isChecked();
    if (m_smallSizeMinDurationSpin)
        gs.smallSizeMinDurationMs = quint32(m_smallSizeMinDurationSpin->value());
    if (m_speedOverdriveKneeSlider)
        gs.speedOverdriveKnee = qBound(1, m_speedOverdriveKneeSlider->value(), 254);
    return gs;
}

bool PresetTableV2TransitionConfigDialog::logVisible() const
{
    return m_logVisibleChk && m_logVisibleChk->isChecked();
}

quint32 PresetTableV2TransitionConfigDialog::bankSourceEngineId(PTTransitionMode mode) const
{
    QComboBox* combo = m_bankSourceCombos.value(int(mode), nullptr);
    return combo ? combo->currentData().toUInt() : VCWidget::invalidId();
}

void PresetTableV2TransitionConfigDialog::updateEffectiveCyclePreview()
{
    if (!m_effectiveCyclePreviewLabel)
        return;

    PTGlobalEffectSettings gs = globalSettings();
    PTTransitionPreset preset;
    const quint32 ms = PTParamMatrixEngine::effectiveDurationMs(gs, preset);
    const bool active = gs.sizeSpeedCeilingEnabled
            && int(gs.speed) > qBound(1, gs.speedOverdriveKnee, 254)
            && gs.positionSize < 255
            && gs.smallSizeMinDurationMs < qMin(gs.minDurationMs, gs.maxDurationMs);
    m_effectiveCyclePreviewLabel->setText(
            tr("%1 ms at speed %2, position size %3%4")
                    .arg(ms)
                    .arg(gs.speed)
                    .arg(gs.positionSize)
                    .arg(active ? tr(" (overdrive active)") : QString()));
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionConfigDialog::globalSpeedInputSource() const
{
    return m_speedInputSel ? m_speedInputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionConfigDialog::globalIntensityInputSource() const
{
    return m_intensityInputSel ? m_intensityInputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionConfigDialog::globalPositionSizeInputSource() const
{
    return m_positionSizeInputSel ? m_positionSizeInputSel->inputSource()
                                  : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionConfigDialog::globalCrossfadeManualInputSource() const
{
    return m_crossfadeManualInputSel ? m_crossfadeManualInputSel->inputSource()
                                     : QSharedPointer<QLCInputSource>();
}

void PresetTableV2TransitionConfigDialog::slotValidate()
{
    if (targetTableId() == VCWidget::invalidId())
    {
        QMessageBox::warning(this, tr("No table"),
                             tr("Select a Preset Table v2 to control."));
        return;
    }
    accept();
}

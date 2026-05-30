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

PresetTableV2TransitionConfigDialog::PresetTableV2TransitionConfigDialog(
        PresetTableV2TransitionWidget* widget, QWidget* parent)
    : QDialog(parent)
    , m_widget(widget)
{
    setWindowTitle(tr("EFX Engine — Properties"));
    setMinimumWidth(500);

    QVBoxLayout* root = new QVBoxLayout(this);

    QLabel* hint = new QLabel(
            tr("Global speed, intensity and min/max cycle times are edited here only.\n"
               "Preset banks: Sweep / Continuous tabs on the widget.\n"
               "Per-column external inputs: double-click column headers on the widget."),
            this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    QFormLayout* form = new QFormLayout;

    m_captionEdit = new QLineEdit(this);
    m_captionEdit->setPlaceholderText(tr("e.g. EFX Engine"));
    if (m_widget)
        m_captionEdit->setText(m_widget->caption());
    form->addRow(tr("Widget name:"), m_captionEdit);

    m_tableCombo = new QComboBox(this);
    form->addRow(tr("Preset Table v2:"), m_tableCombo);
    root->addLayout(form);

    rebuildTableCombo();

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

    root->addWidget(globalBox);

    connect(m_speedSlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotSpeedSliderChanged);
    connect(m_intensitySlider, &QSlider::valueChanged,
            this, &PresetTableV2TransitionConfigDialog::slotIntensitySliderChanged);

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
    if (m_minDurationSpin)
        gs.minDurationMs = quint32(m_minDurationSpin->value());
    if (m_maxDurationSpin)
        gs.maxDurationMs = quint32(m_maxDurationSpin->value());
    if (gs.minDurationMs > gs.maxDurationMs)
        qSwap(gs.minDurationMs, gs.maxDurationMs);
    return gs;
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionConfigDialog::globalSpeedInputSource() const
{
    return m_speedInputSel ? m_speedInputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionConfigDialog::globalIntensityInputSource() const
{
    return m_intensityInputSel ? m_intensityInputSel->inputSource() : QSharedPointer<QLCInputSource>();
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

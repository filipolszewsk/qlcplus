#pragma once

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QSharedPointer>
#include <QHash>
#include "presettablev2effectengine.h"
#include "ptparammatrixengine.h"

class QLCInputSource;
class InputSelectionWidget;
class PresetTableV2TransitionWidget;

class PresetTableV2TransitionConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PresetTableV2TransitionConfigDialog(PresetTableV2TransitionWidget* widget,
                                                 QWidget* parent = nullptr);

    quint32 targetTableId() const;
    QString widgetCaption() const;
    PTGlobalEffectSettings globalSettings() const;
    bool logVisible() const;
    bool multiFxShowInheritedValues() const;
    quint32 bankSourceEngineId(PTTransitionMode mode) const;
    QSharedPointer<QLCInputSource> globalSpeedInputSource() const;
    QSharedPointer<QLCInputSource> globalIntensityInputSource() const;
    QSharedPointer<QLCInputSource> globalPositionSizeInputSource() const;
    QSharedPointer<QLCInputSource> globalCrossfadeManualInputSource() const;

private slots:
    void slotValidate();
    void slotSpeedSliderChanged(int v);
    void slotIntensitySliderChanged(int v);
    void slotPositionSizeSliderChanged(int v);
    void slotTimingControlChanged();

private:
    void rebuildTableCombo();
    void rebuildBankSourceCombos();
    void updateEffectiveCyclePreview();

    PresetTableV2TransitionWidget* m_widget = nullptr;
    QLineEdit*                     m_captionEdit = nullptr;
    QComboBox*                     m_tableCombo = nullptr;
    QCheckBox*                     m_logVisibleChk = nullptr;
    QCheckBox*                     m_multiFxShowInheritedChk = nullptr;
    QHash<int, QComboBox*>         m_bankSourceCombos;
    QSlider*                       m_speedSlider = nullptr;
    QLabel*                        m_speedValueLabel = nullptr;
    QSlider*                       m_intensitySlider = nullptr;
    QLabel*                        m_intensityValueLabel = nullptr;
    QSlider*                       m_positionSizeSlider = nullptr;
    QLabel*                        m_positionSizeValueLabel = nullptr;
    QSpinBox*                      m_minDurationSpin = nullptr;
    QSpinBox*                      m_maxDurationSpin = nullptr;
    QCheckBox*                     m_sizeSpeedCeilingChk = nullptr;
    QSpinBox*                      m_smallSizeMinDurationSpin = nullptr;
    QSlider*                       m_speedOverdriveKneeSlider = nullptr;
    QLabel*                        m_speedOverdriveKneeValueLabel = nullptr;
    QLabel*                        m_effectiveCyclePreviewLabel = nullptr;
    InputSelectionWidget*          m_speedInputSel = nullptr;
    InputSelectionWidget*          m_intensityInputSel = nullptr;
    InputSelectionWidget*          m_positionSizeInputSel = nullptr;
    InputSelectionWidget*          m_crossfadeManualInputSel = nullptr;
    QDialogButtonBox*              m_buttons = nullptr;
};

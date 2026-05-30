#pragma once

#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QSharedPointer>
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
    QSharedPointer<QLCInputSource> globalSpeedInputSource() const;
    QSharedPointer<QLCInputSource> globalIntensityInputSource() const;

private slots:
    void slotValidate();
    void slotSpeedSliderChanged(int v);
    void slotIntensitySliderChanged(int v);

private:
    void rebuildTableCombo();

    PresetTableV2TransitionWidget* m_widget = nullptr;
    QLineEdit*                     m_captionEdit = nullptr;
    QComboBox*                     m_tableCombo = nullptr;
    QSlider*                       m_speedSlider = nullptr;
    QLabel*                        m_speedValueLabel = nullptr;
    QSlider*                       m_intensitySlider = nullptr;
    QLabel*                        m_intensityValueLabel = nullptr;
    QSpinBox*                      m_minDurationSpin = nullptr;
    QSpinBox*                      m_maxDurationSpin = nullptr;
    InputSelectionWidget*          m_speedInputSel = nullptr;
    InputSelectionWidget*          m_intensityInputSel = nullptr;
    QDialogButtonBox*              m_buttons = nullptr;
};

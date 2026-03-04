/*
  Q Light Controller Plus
  ltctimecodewidget.h

  Copyright (c) Filip Olszewski

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

#ifndef LTCTIMECODEWIDGET_H
#define LTCTIMECODEWIDGET_H

#include <QDialog>
#include <QAudioDevice>

class LTCTimecodeEngine;
class QComboBox;
class QPushButton;
class QLabel;
class QSpinBox;
class QTimer;

/**
 * LTCTimecodeWidget — control panel for the LTC Timecode engine.
 *
 * Shows:
 *   - Audio input device selector
 *   - Start / Stop capture button
 *   - Lock status indicator (green = locked, red = no signal)
 *   - Current SMPTE timecode display (HH:MM:SS:FF)
 *   - Detected FPS
 *   - Latency offset spinner (ms)
 *   - Brief usage instructions
 */
class LTCTimecodeWidget : public QDialog
{
    Q_OBJECT

public:
    explicit LTCTimecodeWidget(LTCTimecodeEngine *engine, QWidget *parent = nullptr);
    ~LTCTimecodeWidget() override;

private slots:
    void slotStartStop();
    void slotRefreshDevices();
    void slotLockChanged(bool locked);
    void slotTimeCodeUpdated(const QString &smpte);
    void slotOffsetChanged(int ms);
    void slotUpdateStatus();

private:
    void updateStartStopButton();
    void setLockIndicator(bool locked);

    LTCTimecodeEngine *m_engine;

    QComboBox   *m_deviceCombo;
    QPushButton *m_startStopBtn;
    QLabel      *m_lockLabel;
    QLabel      *m_smpteLabel;
    QLabel      *m_fpsLabel;
    QSpinBox    *m_offsetSpin;
    QTimer      *m_statusTimer;

    QList<QAudioDevice> m_devices;
};

#endif // LTCTIMECODEWIDGET_H

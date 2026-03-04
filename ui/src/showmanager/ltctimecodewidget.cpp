/*
  Q Light Controller Plus
  ltctimecodewidget.cpp

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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QTimer>
#include <QFont>
#include <QDebug>

#include "ltctimecodewidget.h"
#include "ltctimecodeengine.h"

LTCTimecodeWidget::LTCTimecodeWidget(LTCTimecodeEngine *engine, QWidget *parent)
    : QDialog(parent)
    , m_engine(engine)
{
    setWindowTitle(tr("LTC Timecode Sync"));
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // --- Device selection ---
    QGroupBox *deviceGroup = new QGroupBox(tr("Audio Input"), this);
    QHBoxLayout *deviceRow = new QHBoxLayout(deviceGroup);

    m_deviceCombo = new QComboBox(deviceGroup);
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    deviceRow->addWidget(m_deviceCombo);

    QPushButton *refreshBtn = new QPushButton(tr("Refresh"), deviceGroup);
    connect(refreshBtn, &QPushButton::clicked, this, &LTCTimecodeWidget::slotRefreshDevices);
    deviceRow->addWidget(refreshBtn);

    mainLayout->addWidget(deviceGroup);

    // --- Status display ---
    QGroupBox *statusGroup = new QGroupBox(tr("Status"), this);
    QFormLayout *statusForm = new QFormLayout(statusGroup);

    m_lockLabel = new QLabel(tr("NO SIGNAL"), statusGroup);
    QFont bigFont = m_lockLabel->font();
    bigFont.setPointSize(11);
    bigFont.setBold(true);
    m_lockLabel->setFont(bigFont);
    setLockIndicator(false);
    statusForm->addRow(tr("Signal:"), m_lockLabel);

    m_smpteLabel = new QLabel("--:--:--:--", statusGroup);
    QFont smpteFont;
    smpteFont.setFamily("Courier");
    smpteFont.setPointSize(18);
    smpteFont.setBold(true);
    m_smpteLabel->setFont(smpteFont);
    m_smpteLabel->setAlignment(Qt::AlignCenter);
    statusForm->addRow(tr("Timecode:"), m_smpteLabel);

    m_fpsLabel = new QLabel("-- fps", statusGroup);
    statusForm->addRow(tr("Detected FPS:"), m_fpsLabel);

    mainLayout->addWidget(statusGroup);

    // --- Settings ---
    QGroupBox *settingsGroup = new QGroupBox(tr("Settings"), this);
    QFormLayout *settingsForm = new QFormLayout(settingsGroup);

    m_offsetSpin = new QSpinBox(settingsGroup);
    m_offsetSpin->setRange(-500, 500);
    m_offsetSpin->setValue(m_engine->offsetMs());
    m_offsetSpin->setSuffix(" ms");
    m_offsetSpin->setToolTip(tr("Add this offset to the decoded timecode.\n"
                                "Use positive values if the show lags behind audio,\n"
                                "negative values if it runs ahead."));
    connect(m_offsetSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &LTCTimecodeWidget::slotOffsetChanged);
    settingsForm->addRow(tr("Latency offset:"), m_offsetSpin);

    mainLayout->addWidget(settingsGroup);

    // --- Start / Stop button ---
    m_startStopBtn = new QPushButton(tr("Start Capture"), this);
    m_startStopBtn->setMinimumHeight(36);
    connect(m_startStopBtn, &QPushButton::clicked, this, &LTCTimecodeWidget::slotStartStop);
    mainLayout->addWidget(m_startStopBtn);

    // --- Instructions ---
    QLabel *hint = new QLabel(
        tr("<small>Connect LTC audio output (e.g. from a DAW or tape) to the selected "
           "audio input. Start capture, then play the show — the timeline will follow "
           "the incoming timecode automatically.</small>"), this);
    hint->setWordWrap(true);
    mainLayout->addWidget(hint);

    // --- Close button ---
    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    mainLayout->addLayout(btnRow);

    // Connect engine signals
    connect(m_engine, &LTCTimecodeEngine::lockChanged,
            this, &LTCTimecodeWidget::slotLockChanged);
    connect(m_engine, &LTCTimecodeEngine::timeCodeUpdated,
            this, &LTCTimecodeWidget::slotTimeCodeUpdated);

    // Periodic FPS label + status refresh
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &LTCTimecodeWidget::slotUpdateStatus);
    m_statusTimer->start(500);

    slotRefreshDevices();
}

LTCTimecodeWidget::~LTCTimecodeWidget()
{
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void LTCTimecodeWidget::slotRefreshDevices()
{
    m_devices = LTCTimecodeEngine::inputDevices();
    m_deviceCombo->clear();
    for (const QAudioDevice &dev : m_devices)
        m_deviceCombo->addItem(dev.description());
}

void LTCTimecodeWidget::slotStartStop()
{
    if (m_engine->isCapturing())
    {
        m_engine->stopCapture();
    }
    else
    {
        int idx = m_deviceCombo->currentIndex();
        if (idx < 0 || idx >= m_devices.count())
            return;
        m_engine->startCapture(m_devices.at(idx));
    }
    updateStartStopButton();
}

void LTCTimecodeWidget::slotLockChanged(bool locked)
{
    setLockIndicator(locked);
    if (!locked)
        m_smpteLabel->setText("--:--:--:--");
}

void LTCTimecodeWidget::slotTimeCodeUpdated(const QString &smpte)
{
    m_smpteLabel->setText(smpte);
}

void LTCTimecodeWidget::slotOffsetChanged(int ms)
{
    m_engine->setOffsetMs(ms);
}

void LTCTimecodeWidget::slotUpdateStatus()
{
    int fps = m_engine->detectedFps();
    m_fpsLabel->setText(fps > 0 ? QString("%1 fps").arg(fps) : tr("-- fps"));
    setLockIndicator(m_engine->isLocked());
    updateStartStopButton();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void LTCTimecodeWidget::updateStartStopButton()
{
    if (m_engine->isCapturing())
    {
        m_startStopBtn->setText(tr("Stop Capture"));
        m_deviceCombo->setEnabled(false);
    }
    else
    {
        m_startStopBtn->setText(tr("Start Capture"));
        m_deviceCombo->setEnabled(true);
    }
}

void LTCTimecodeWidget::setLockIndicator(bool locked)
{
    if (locked)
    {
        m_lockLabel->setText(tr("LOCKED"));
        m_lockLabel->setStyleSheet("color: #00CC44; font-weight: bold;");
    }
    else
    {
        m_lockLabel->setText(tr("NO SIGNAL"));
        m_lockLabel->setStyleSheet("color: #CC2200; font-weight: bold;");
    }
}

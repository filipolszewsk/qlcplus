/*
  Q Light Controller Plus
  ltctimecodeengine.cpp

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

#include <QDebug>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QMutexLocker>
#include <cstring>

#include "ltctimecodeengine.h"
#include "showmanager.h"
#include "function.h"
#include "doc.h"

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

LTCTimecodeEngine::LTCTimecodeEngine(Doc *doc, QObject *parent)
    : QObject(parent)
    , m_doc(doc)
    , m_audioSource(nullptr)
    , m_audioDevice(nullptr)
    , m_decoder(nullptr)
    , m_sampleRate(48000)
    , m_decoderOffset(0)
    , m_locked(false)
    , m_lastFrameTimeMs(0)
    , m_offsetMs(0)
    , m_consecutiveFrames(0)
    , m_detectedFps(25)
    , m_maxFrameSeen(0)
    , m_cursorTimer(nullptr)
{
    m_frameTimer.invalidate();

    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setInterval(20); // 50 Hz
    connect(m_cursorTimer, &QTimer::timeout, this, &LTCTimecodeEngine::slotCursorTimer);
}

LTCTimecodeEngine::~LTCTimecodeEngine()
{
    stopCapture();
    if (m_decoder)
    {
        ltc_decoder_free(m_decoder);
        m_decoder = nullptr;
    }
}

// ---------------------------------------------------------------------------
// TimeCodeSource — called from MasterTimer thread (50 Hz)
// ---------------------------------------------------------------------------

bool LTCTimecodeEngine::isLocked() const
{
    QMutexLocker lock(&m_mutex);
    if (!m_locked)
        return false;

    // Flywheel: declare unlock if no frame received for FLYWHEEL_TIMEOUT_MS
    if (m_frameTimer.isValid() && m_frameTimer.elapsed() > FLYWHEEL_TIMEOUT_MS)
    {
        m_consecutiveFrames = 0; // reset hysteresis counter on flywheel expiry
        return false;
    }

    return true;
}

quint32 LTCTimecodeEngine::currentTimeMs() const
{
    QMutexLocker lock(&m_mutex);

    // Interpolate between LTC frames using a high-res elapsed timer.
    // LTC frames arrive every ~40ms (25fps) but MasterTimer ticks every 20ms.
    quint32 interpolated = m_lastFrameTimeMs;
    if (m_frameTimer.isValid())
        interpolated += static_cast<quint32>(m_frameTimer.elapsed());

    // Apply user offset for latency compensation (clamp to 0)
    int result = static_cast<int>(interpolated) + m_offsetMs;
    return static_cast<quint32>(qMax(0, result));
}

// ---------------------------------------------------------------------------
// Capture control
// ---------------------------------------------------------------------------

bool LTCTimecodeEngine::startCapture(const QAudioDevice &device)
{
    stopCapture();

    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!device.isFormatSupported(format))
    {
        m_sampleRate = device.preferredFormat().sampleRate();
        format.setSampleRate(m_sampleRate);
        qDebug() << "LTC: preferred sample rate not supported, using" << m_sampleRate;
    }

    // libltc decoder — apv = samples per video frame (default 25fps)
    int apv = m_sampleRate / 25;
    if (m_decoder)
        ltc_decoder_free(m_decoder);
    m_decoder = ltc_decoder_create(apv, 32);
    m_decoderOffset = 0;
    m_maxFrameSeen = 0;
    m_detectedFps = 25;
    m_consecutiveFrames = 0;

    m_audioSource = new QAudioSource(device, format, this);
    m_audioSource->setBufferSize(4096);
    m_audioDevice = m_audioSource->start();

    if (!m_audioDevice)
    {
        qWarning() << "LTC: failed to open audio device";
        delete m_audioSource;
        m_audioSource = nullptr;
        ltc_decoder_free(m_decoder);
        m_decoder = nullptr;
        return false;
    }

    connect(m_audioDevice, &QIODevice::readyRead,
            this, &LTCTimecodeEngine::slotAudioDataReady);

    if (m_doc)
        m_doc->setTimeCodeSource(this);

    m_cursorTimer->start();

    qDebug() << "LTC: capture started on" << device.description()
             << "at" << m_sampleRate << "Hz";
    return true;
}

void LTCTimecodeEngine::stopCapture()
{
    m_cursorTimer->stop();

    if (m_doc)
        m_doc->setTimeCodeSource(nullptr);

    if (m_audioSource)
    {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioDevice = nullptr;
    }

    updateLock(false);
    emit captureStopped();
}

bool LTCTimecodeEngine::isCapturing() const
{
    return m_audioSource != nullptr &&
           m_audioSource->state() == QAudio::ActiveState;
}

// ---------------------------------------------------------------------------
// Cursor timer — UI thread, 50 Hz
// ---------------------------------------------------------------------------

void LTCTimecodeEngine::slotCursorTimer()
{
    // Only update cursor when no show is actively playing.
    // When a show is playing, ShowRunner drives the cursor via timeChanged() signal —
    // two concurrent updates cause visible flicker/jumps.
    if (m_doc)
    {
        for (Function *f : m_doc->functionsByType(Function::ShowType))
        {
            if (f->isRunning())
                return;
        }
    }

    ShowManager *sm = ShowManager::instance();
    if (sm && isLocked())
        sm->setTimecodePosition(currentTimeMs());
}

// ---------------------------------------------------------------------------
// Audio data processing
// ---------------------------------------------------------------------------

void LTCTimecodeEngine::slotAudioDataReady()
{
    if (!m_audioDevice || !m_decoder)
        return;

    QByteArray data = m_audioDevice->readAll();
    if (!data.isEmpty())
        processAudioBuffer(data);
}

void LTCTimecodeEngine::processAudioBuffer(const QByteArray &data)
{
    if (!m_decoder)
        return;

    const int16_t *samples = reinterpret_cast<const int16_t *>(data.constData());
    int numSamples = data.size() / static_cast<int>(sizeof(int16_t));

    ltc_decoder_write_s16(m_decoder, const_cast<short *>(samples),
                           static_cast<size_t>(numSamples), m_decoderOffset);
    m_decoderOffset += numSamples;

    LTCFrameExt frame;
    while (ltc_decoder_read(m_decoder, &frame))
    {
        SMPTETimecode tc;
        ltc_frame_to_time(&tc, &frame.ltc, LTC_USE_DATE);

        // Reliable FPS detection: watch for frame-number rollover.
        // When tc.frame drops back to 0 after seeing >= 23, we know the max frame.
        // e.g. max=24 → 25fps, max=23 → 24fps, max=29 → 30fps.
        if (tc.frame == 0 && m_maxFrameSeen >= 23)
        {
            if (m_maxFrameSeen >= 28)
                m_detectedFps = 30;
            else if (m_maxFrameSeen >= 24)
                m_detectedFps = 25;
            else
                m_detectedFps = 24;
            m_maxFrameSeen = 0;
        }
        else if (static_cast<int>(tc.frame) > m_maxFrameSeen)
        {
            m_maxFrameSeen = static_cast<int>(tc.frame);
        }

        // Floating-point ms per frame avoids rounding errors (e.g. 1000/25=40 exactly,
        // but 1000/24=41.666... which would accumulate to wrong values with integer math)
        double mspf = 1000.0 / static_cast<double>(m_detectedFps > 0 ? m_detectedFps : 25);
        quint32 frameMs = static_cast<quint32>(
            (static_cast<double>(tc.hours) * 3600000.0)
          + (static_cast<double>(tc.mins)  *   60000.0)
          + (static_cast<double>(tc.secs)  *    1000.0)
          + (static_cast<double>(tc.frame) *     mspf));

        bool wasLocked;
        {
            QMutexLocker lock(&m_mutex);
            wasLocked = m_locked;
            m_lastFrameTimeMs = frameMs;
            m_frameTimer.restart();

            // Hysteresis: only set m_locked after N consecutive valid frames.
            // This prevents a single stray frame from causing a jump after dropout.
            m_consecutiveFrames++;
            if (m_consecutiveFrames >= LOCK_HYSTERESIS_FRAMES)
                m_locked = true;
        }

        QString smpte = QString("%1:%2:%3:%4")
            .arg(tc.hours, 2, 10, QChar('0'))
            .arg(tc.mins,  2, 10, QChar('0'))
            .arg(tc.secs,  2, 10, QChar('0'))
            .arg(tc.frame, 2, 10, QChar('0'));
        m_smpteString = smpte;

        fprintf(stderr, "LTC FRAME: %s  ms=%u  consec=%d  wasLocked=%d\n",
                smpte.toUtf8().constData(), frameMs, m_consecutiveFrames, (int)wasLocked);

        emit timeCodeUpdated(smpte);

        if (m_consecutiveFrames >= LOCK_HYSTERESIS_FRAMES && !wasLocked)
            emit lockChanged(true);
    }
}

// ---------------------------------------------------------------------------
// State helpers
// ---------------------------------------------------------------------------

void LTCTimecodeEngine::updateLock(bool locked)
{
    bool changed = false;
    {
        QMutexLocker lock(&m_mutex);
        if (m_locked != locked)
        {
            m_locked = locked;
            m_consecutiveFrames = 0;
            changed = true;
        }
    }
    if (changed)
        emit lockChanged(locked);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

int LTCTimecodeEngine::offsetMs() const
{
    return m_offsetMs;
}

void LTCTimecodeEngine::setOffsetMs(int ms)
{
    m_offsetMs = ms;
}

QString LTCTimecodeEngine::smpteString() const
{
    return m_smpteString;
}

int LTCTimecodeEngine::detectedFps() const
{
    return m_detectedFps;
}

QList<QAudioDevice> LTCTimecodeEngine::inputDevices()
{
    return QMediaDevices::audioInputs();
}

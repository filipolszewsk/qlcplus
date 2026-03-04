/*
  Q Light Controller Plus
  ltctimecodeengine.h

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

#ifndef LTCTIMECODEENGINE_H
#define LTCTIMECODEENGINE_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QIODevice>

#include <ltc.h>

#include "timecodesource.h"

class Doc;
class Function;

/**
 * LTCTimecodeEngine — decodes SMPTE LTC (Linear Timecode) from an audio input
 * and drives the QLC+ Show timeline position in real-time.
 *
 * Architecture:
 *   - Qt audio capture (QAudioSource) feeds raw PCM samples to libltc.
 *   - libltc decodes each SMPTE frame and provides H:MM:SS:FF.
 *   - On startCapture() the engine registers itself as the Doc-level TimeCodeSource.
 *     ShowRunner reads currentTimeMs() on every MasterTimer tick without any
 *     per-show registration — no race conditions, no timing issues.
 *   - A 50 Hz UI timer pushes the cursor position to ShowManager even when no
 *     show is currently playing (design-mode cursor preview).
 *   - Flywheel: if the signal is lost, isLocked() returns false and ShowRunner
 *     freezes the timeline until signal returns.
 *   - Hysteresis: requires LOCK_HYSTERESIS_FRAMES consecutive valid frames
 *     before declaring lock (prevents jitter on reconnect).
 *   - FPS detection: monitors tc.frame rollover (e.g. 24→0) for reliable
 *     frame-rate detection without relying on wall-clock timing.
 */
class LTCTimecodeEngine : public QObject, public TimeCodeSource
{
    Q_OBJECT

public:
    explicit LTCTimecodeEngine(Doc *doc, QObject *parent = nullptr);
    ~LTCTimecodeEngine() override;

    // TimeCodeSource — called from MasterTimer thread (50 Hz)
    bool    isLocked()      const override;
    quint32 currentTimeMs() const override;

    // Capture control — called from UI thread
    bool startCapture(const QAudioDevice &device);
    void stopCapture();
    bool isCapturing() const;

    /** Offset (ms) added to the decoded time for latency compensation. */
    int  offsetMs() const;
    void setOffsetMs(int ms);

    /** Last decoded SMPTE position formatted as HH:MM:SS:FF. */
    QString smpteString() const;

    /** Detected FPS of the incoming signal (0 if not yet determined). */
    int detectedFps() const;

    /** Available audio input devices. */
    static QList<QAudioDevice> inputDevices();

signals:
    void lockChanged(bool locked);
    void timeCodeUpdated(QString smpte);

private slots:
    void slotAudioDataReady();
    /** Called at 50 Hz to push cursor position to ShowManager (design-mode preview). */
    void slotCursorTimer();

private:
    void processAudioBuffer(const QByteArray &data);
    void updateLock(bool locked);

    Doc    *m_doc;

    // Audio capture
    QAudioSource   *m_audioSource;
    QIODevice      *m_audioDevice;

    // libltc decoder
    LTCDecoder     *m_decoder;
    int             m_sampleRate;
    ltc_off_t       m_decoderOffset;

    // Timecode state — protected by m_mutex (accessed from audio + render threads)
    mutable QMutex  m_mutex;
    bool            m_locked;
    quint32         m_lastFrameTimeMs;
    QElapsedTimer   m_frameTimer;
    int             m_offsetMs;

    // Lock hysteresis: require N consecutive frames before declaring lock
    mutable int     m_consecutiveFrames;
    static const int LOCK_HYSTERESIS_FRAMES = 3;

    // Flywheel: how many ms without a valid frame before declaring unlock
    static const int FLYWHEEL_TIMEOUT_MS = 300;

    // FPS detection via frame-number rollover
    int             m_detectedFps;
    int             m_maxFrameSeen;

    // 50 Hz timer for ShowManager cursor update (UI thread only)
    QTimer         *m_cursorTimer;

    // Diagnostics
    QString         m_smpteString;
};

#endif // LTCTIMECODEENGINE_H

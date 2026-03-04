/*
  Q Light Controller Plus
  timecodesource.h

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

#ifndef TIMECODESOURCE_H
#define TIMECODESOURCE_H

#include <QtGlobal>

/**
 * TimeCodeSource is an abstract interface that an external timecode provider
 * (e.g. an LTC audio timecode plugin) can implement to drive the Show timeline.
 *
 * ShowRunner checks for a registered source on every tick:
 *   - If isLocked() returns true, ShowRunner uses currentTimeMs() as the
 *     timeline position instead of incrementing its internal counter.
 *   - If isLocked() returns false (signal dropout, plugin not ready, etc.),
 *     ShowRunner falls back to its own internal clock seamlessly.
 *
 * Usage:
 *   1. Plugin implements TimeCodeSource and calls
 *      Doc::setTimeCodeSource(this) on init.
 *   2. Plugin calls Doc::setTimeCodeSource(nullptr) on shutdown.
 *   3. ShowRunner reads the source automatically — no other changes needed.
 *
 * Thread safety:
 *   currentTimeMs() and isLocked() are called from the MasterTimer thread
 *   (50 Hz). Implementations must use atomic or mutex protection for any
 *   internal state updated from the audio capture thread.
 */
class TimeCodeSource
{
public:
    virtual ~TimeCodeSource() {}

    /**
     * Returns true when the source has a valid, locked timecode signal.
     * ShowRunner uses external time only while this returns true.
     */
    virtual bool isLocked() const = 0;

    /**
     * Current timecode position in milliseconds.
     * Must be monotonically increasing while playing forward.
     * May jump when the source rewinds or the signal changes position.
     * Called every MasterTimer tick (~20 ms) from the render thread.
     */
    virtual quint32 currentTimeMs() const = 0;
};

#endif // TIMECODESOURCE_H

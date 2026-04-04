/*
  Q Light Controller Plus
  fixturechannelresolver.cpp

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

#include "fixturechannelresolver.h"

#include "fixture.h"
#include "qlcfixturemode.h"
#include "qlcchannel.h"
#include "qlccapability.h"
#include "doc.h"

#include <QList>
#include <QString>
#include <climits>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/**
 * Return the target QLCChannel::Group and the ordered list of preferred
 * QLCCapability::Preset values for a given GRIDqlc preset type string.
 */
struct PresetProfile
{
    QLCChannel::Group          channelGroup;
    QList<QLCCapability::Preset> capabilityPresets; ///< ordered: first match wins
};

PresetProfile profileForType(const QString &presetType)
{
    if (presetType == QLatin1String("GOBO"))
    {
        return {
            QLCChannel::Gobo,
            {
                QLCCapability::GoboMacro,
                QLCCapability::GoboShakeMacro,
                QLCCapability::GenericPicture,
            }
        };
    }
    if (presetType == QLatin1String("STROBE"))
    {
        return {
            QLCChannel::Shutter,
            {
                QLCCapability::StrobeFreqRange,
                QLCCapability::StrobeFrequency,
                QLCCapability::StrobeSlowToFast,
                QLCCapability::StrobeFastToSlow,
                QLCCapability::StrobeRandom,
                QLCCapability::StrobeRandomSlowToFast,
                QLCCapability::StrobeRandomFastToSlow,
            }
        };
    }
    if (presetType == QLatin1String("PRYZM"))
    {
        return {
            QLCChannel::Prism,
            {
                QLCCapability::PrismEffectOn,
            }
        };
    }
    if (presetType == QLatin1String("GOBOROT"))
    {
        return {
            QLCChannel::Speed,
            {
                QLCCapability::RotationClockwiseSlowToFast,
                QLCCapability::RotationClockwiseFastToSlow,
                QLCCapability::RotationClockwise,
                QLCCapability::RotationCounterClockwiseSlowToFast,
                QLCCapability::RotationCounterClockwiseFastToSlow,
                QLCCapability::RotationCounterClockwise,
                QLCCapability::SlowToFast,
                QLCCapability::FastToSlow,
            }
        };
    }
    // Unknown type — return an empty profile
    return { QLCChannel::NoGroup, {} };
}

/**
 * Try to find the best matching capability in @p channel for any of
 * @p preferredPresets. Returns the matched capability or nullptr.
 * For GOBO type, skips "open" capabilities (first capability in the channel,
 * which conventionally represents the open position / no gobo).
 */
const QLCCapability *findBestCapability(const QLCChannel *channel,
                                        const QList<QLCCapability::Preset> &preferredPresets,
                                        bool skipFirstForGobo)
{
    const QList<QLCCapability *> &caps = channel->capabilities();
    if (caps.isEmpty())
        return nullptr;

    for (QLCCapability::Preset wantedPreset : preferredPresets)
    {
        for (int i = 0; i < caps.size(); ++i)
        {
            const QLCCapability *cap = caps.at(i);
            if (skipFirstForGobo && i == 0)
                continue; // skip open/no-gobo position
            if (cap->preset() == wantedPreset)
                return cap;
        }
    }
    return nullptr;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ResolvedChannel FixtureChannelResolver::resolve(Fixture *fixture,
                                                 const QString &presetType) const
{
    ResolvedChannel result;
    if (!fixture)
    {
        result.info = QStringLiteral("Brak fixture (nullptr)");
        return result;
    }

    QLCFixtureMode *mode = fixture->fixtureMode();
    if (!mode)
    {
        result.info = QStringLiteral("Fixture nie ma trybu (fixtureMode == nullptr)");
        return result;
    }

    const PresetProfile profile = profileForType(presetType);
    if (profile.channelGroup == QLCChannel::NoGroup)
    {
        result.info = QString("Nieznany typ presetu: %1").arg(presetType);
        return result;
    }

    const QVector<QLCChannel *> channels = mode->channels();
    const bool skipFirstForGobo = (presetType == QLatin1String("GOBO"));

    // First pass: find a channel matching the target group AND a preferred capability
    for (quint32 chIdx = 0; chIdx < static_cast<quint32>(channels.size()); ++chIdx)
    {
        QLCChannel *ch = channels.at(static_cast<int>(chIdx));
        if (!ch || ch->group() != profile.channelGroup)
            continue;

        const QLCCapability *bestCap = findBestCapability(ch, profile.capabilityPresets,
                                                           skipFirstForGobo);
        if (bestCap)
        {
            uchar midpoint = static_cast<uchar>((bestCap->min() + bestCap->max()) / 2);
            result.fixtureId     = fixture->id();
            result.channelIndex  = chIdx;
            result.suggestedValue = midpoint;
            result.channelName   = ch->name();
            result.info = QString("Kanal %1 \"%2\", capability \"%3\" [%4-%5], wartosc=%6")
                              .arg(chIdx)
                              .arg(ch->name())
                              .arg(QLCCapability::presetToString(bestCap->preset()))
                              .arg(bestCap->min())
                              .arg(bestCap->max())
                              .arg(midpoint);
            return result;
        }
    }

    // Second pass (fallback): channel group match only, no capability filter
    for (quint32 chIdx = 0; chIdx < static_cast<quint32>(channels.size()); ++chIdx)
    {
        QLCChannel *ch = channels.at(static_cast<int>(chIdx));
        if (!ch || ch->group() != profile.channelGroup)
            continue;

        result.fixtureId      = fixture->id();
        result.channelIndex   = chIdx;
        result.suggestedValue = 128;
        result.channelName    = ch->name();
        result.info = QString("Kanal %1 \"%2\" (tylko grupa %3, brak pasujacych capabilities — wartosc domyslna 128)")
                          .arg(chIdx)
                          .arg(ch->name())
                          .arg(QLCChannel::groupToString(profile.channelGroup));
        return result;
    }

    // Nothing found
    result.info = QString("Nie znaleziono kanalu grupy \"%1\" w trybie \"%2\" fixture \"%3\"")
                      .arg(QLCChannel::groupToString(profile.channelGroup))
                      .arg(mode->name())
                      .arg(fixture->name());
    return result;
}

QList<ResolvedChannel> FixtureChannelResolver::resolveGroup(
    const QList<quint32> &fixtureIds,
    const QString &presetType,
    Doc *doc) const
{
    QList<ResolvedChannel> results;
    for (quint32 fxId : fixtureIds)
    {
        Fixture *fx = doc ? doc->fixture(fxId) : nullptr;
        results.append(resolve(fx, presetType));
        if (fx && results.last().isValid())
            results.last().fixtureId = fxId;
    }
    return results;
}

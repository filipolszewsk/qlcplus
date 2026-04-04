/*
  Q Light Controller Plus
  fixturechannelresolver.h

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

#ifndef FIXTURECHANNELRESOLVER_H
#define FIXTURECHANNELRESOLVER_H

#include <climits>

#include <QString>
#include <QList>
#include <QtGlobal>

class Doc;
class Fixture;

/**
 * Result of resolving a single fixture's channel for a given preset type.
 */
struct ResolvedChannel
{
    quint32 fixtureId    = UINT_MAX; ///< QLC+ fixture ID
    quint32 channelIndex = UINT_MAX; ///< 0-based channel index in the fixture mode
    uchar   suggestedValue = 128;    ///< Midpoint of the best matching capability range
    QString channelName;             ///< Channel name from fixture definition, e.g. "Gobo 1"
    QString info;                    ///< Human-readable description of how value was chosen

    bool isValid() const { return channelIndex != UINT_MAX; }
};

/**
 * FixtureChannelResolver — auto-detects the correct DMX channel and a sensible
 * default value for a given GRIDqlc preset type (GOBO, STROBE, PRYZM, GOBOROT).
 *
 * Resolution strategy (in order of priority):
 *   1. Scan the fixture mode channels for the target QLCChannel::Group.
 *   2. Among matching channels, look for a specific QLCCapability::Preset.
 *   3. Suggest the midpoint of that capability's value range as default.
 *   4. Fall back to channel-group-only match with value 128 if no capability matches.
 *
 * Mapping table:
 *   GOBO    → Group::Gobo,    capability GoboMacro (first non-open entry)
 *   STROBE  → Group::Shutter, capability StrobeFreqRange / StrobeFrequency / StrobeSlowToFast
 *   PRYZM   → Group::Prism,   capability PrismEffectOn
 *   GOBOROT → Group::Speed,   capability RotationClockwiseSlowToFast / RotationClockwiseFastToSlow
 */
class FixtureChannelResolver
{
public:
    FixtureChannelResolver() = default;

    /**
     * Resolve channel mapping for a single fixture and preset type.
     *
     * @param fixture    Pointer to the QLC+ Fixture object.
     * @param presetType One of: "GOBO", "STROBE", "PRYZM", "GOBOROT".
     * @return ResolvedChannel; check isValid() before using.
     */
    ResolvedChannel resolve(Fixture *fixture, const QString &presetType) const;

    /**
     * Resolve channel mappings for a list of fixture IDs (one per group row).
     * Each entry in the returned list corresponds to the input fixtureIds entry.
     *
     * @param fixtureIds List of QLC+ fixture IDs (one per row: ALL, G1..G4).
     * @param presetType One of: "GOBO", "STROBE", "PRYZM", "GOBOROT".
     * @param doc        Pointer to the QLC+ Doc to look up Fixture objects.
     * @return List of ResolvedChannel, same length as fixtureIds.
     */
    QList<ResolvedChannel> resolveGroup(const QList<quint32> &fixtureIds,
                                        const QString &presetType,
                                        Doc *doc) const;
};

#endif // FIXTURECHANNELRESOLVER_H

/*
  Q Light Controller Plus
  qlcinputaddress.h

  Copyright (c) Massimo Callegari

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

#ifndef QLCINPUTADDRESS_H
#define QLCINPUTADDRESS_H

#include <QtGlobal>
#include <QString>

class QLCInputAddress
{
public:
  /** Parse a 1-based universe.channel string (e.g. "1.42"). Returns false on invalid input. */
  static bool parse(const QString &text, quint32 &universe, quint32 &channel);

  /** Format 0-based universe and channel as 1-based "U.C". */
  static QString format(quint32 universe, quint32 channel);

  /** Pack 0-based universe and channel into a single address word. */
  static quint32 encode(quint32 universe, quint32 channel);
};

#endif

/*
  Q Light Controller Plus
  qlcinputaddress.cpp

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

#include <QStringList>

#include "qlcinputaddress.h"

bool QLCInputAddress::parse(const QString &text, quint32 &universe, quint32 &channel)
{
  QStringList parts = text.trimmed().split('.');
  if (parts.length() < 2)
    return false;

  bool okU = false;
  bool okC = false;
  quint32 u = parts.at(0).toUInt(&okU);
  quint32 c = parts.at(1).toUInt(&okC);
  if (!okU || !okC || u == 0 || c == 0)
    return false;

  universe = u - 1;
  channel = c - 1;
  return true;
}

QString QLCInputAddress::format(quint32 universe, quint32 channel)
{
  return QString("%1.%2").arg(universe + 1).arg((channel & 0xFFFF) + 1);
}

quint32 QLCInputAddress::encode(quint32 universe, quint32 channel)
{
  return (universe << 16) | (channel & 0xFFFF);
}

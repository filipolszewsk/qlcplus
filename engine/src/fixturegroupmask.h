/*
  Q Light Controller Plus
  fixturegroupmask.h

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

#ifndef FIXTUREGROUPMASK_H
#define FIXTUREGROUPMASK_H

#include <QSet>

#include "qlcpoint.h"

/** Runtime column/row filter for a fixture group (not saved in workspace). */
class FixtureGroupMask
{
public:
    FixtureGroupMask() = default;

    /** True when at least one column or row filter is set. */
    bool isActive() const
    {
        return !m_columns.isEmpty() || !m_rows.isEmpty();
    }

    bool acceptsPoint(const QLCPoint& pt) const
    {
        if (!m_columns.isEmpty() && !m_columns.contains(pt.x()))
            return false;
        if (!m_rows.isEmpty() && !m_rows.contains(pt.y()))
            return false;
        return true;
    }

    bool isColumnEnabled(int column) const
    {
        return m_columns.isEmpty() || m_columns.contains(column);
    }

    bool isRowEnabled(int row) const
    {
        return m_rows.isEmpty() || m_rows.contains(row);
    }

    void setColumnEnabled(int column, bool enabled);
    void setRowEnabled(int row, bool enabled);
    void clear();

    QSet<int> columns() const { return m_columns; }
    QSet<int> rows() const { return m_rows; }

    void setColumns(const QSet<int>& columns) { m_columns = columns; }
    void setRows(const QSet<int>& rows) { m_rows = rows; }

private:
    /** Non-empty: only these columns are visible. Empty: all columns. */
    QSet<int> m_columns;
    /** Non-empty: only these rows are visible. Empty: all rows. */
    QSet<int> m_rows;
};

#endif

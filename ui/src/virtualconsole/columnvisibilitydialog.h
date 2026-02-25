/*
  Q Light Controller Plus
  columnvisibilitydialog.h

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

#ifndef COLUMNVISIBILITYDIALOG_H
#define COLUMNVISIBILITYDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

class QCheckBox;
class QDialogButtonBox;

/**
 * Dialog for showing/hiding fixed columns in VCCueList.
 * Displays a list of checkboxes — checked means visible.
 */
class ColumnVisibilityDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param columns  List of (column name, isHidden) pairs for each fixed column
     * @param parent   Parent widget
     */
    explicit ColumnVisibilityDialog(const QList<QPair<QString, bool>> &columns,
                                    QWidget *parent = nullptr);
    ~ColumnVisibilityDialog() = default;

    /**
     * Returns hidden state for each column in the same order as the input.
     * true = hidden, false = visible.
     */
    QList<bool> hiddenStates() const;

private:
    QList<QCheckBox *> m_checkboxes;
};

#endif // COLUMNVISIBILITYDIALOG_H

/*
  Q Light Controller Plus
  columnvisibilitydialog.cpp

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

#include "columnvisibilitydialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

ColumnVisibilityDialog::ColumnVisibilityDialog(const QList<QPair<QString, bool>> &columns,
                                               QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Show / Hide Columns"));
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *hint = new QLabel(tr("Select which columns to display:"), this);
    layout->addWidget(hint);

    for (const QPair<QString, bool> &col : columns)
    {
        QCheckBox *cb = new QCheckBox(col.first, this);
        cb->setChecked(!col.second);   // checked = visible
        layout->addWidget(cb);
        m_checkboxes.append(cb);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QList<bool> ColumnVisibilityDialog::hiddenStates() const
{
    QList<bool> result;
    for (QCheckBox *cb : m_checkboxes)
        result.append(!cb->isChecked());   // unchecked = hidden
    return result;
}

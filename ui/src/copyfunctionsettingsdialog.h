/*
  Q Light Controller Plus
  copyfunctionsettingsdialog.h

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

#ifndef COPYFUNCTIONSETTINGSDIALOG_H
#define COPYFUNCTIONSETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QCheckBox>

class QVBoxLayout;
class Function;
class Doc;

/** @addtogroup ui_functions
 * @{
 */

/**
 * Dialog that lets the user choose which parameter groups to copy from a
 * Function into the system clipboard as JSON, enabling cross-instance paste.
 */
class CopyFunctionSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param function  The function whose settings will be copied.
     * @param doc       The owning Doc (used for name resolution during serialization).
     * @param parent    Parent widget.
     */
    CopyFunctionSettingsDialog(Function *function, const Doc *doc, QWidget *parent = nullptr);
    ~CopyFunctionSettingsDialog() = default;

    /** Perform the copy to system clipboard. Returns true on success. */
    bool copyToClipboard();

    /** @reimp: serialize selected groups to clipboard and accept. */
    void accept() override;

private slots:
    void slotSelectAllToggled(bool checked);
    void slotCheckBoxToggled(bool checked);

private:
    void buildUI();
    int selectedFlags() const;

    Function *m_function;
    const Doc *m_doc;

    QCheckBox *m_selectAllBox;
    QList<QPair<int, QCheckBox*>> m_entries;
    bool m_blockSignals;
};

/** @} */

#endif // COPYFUNCTIONSETTINGSDIALOG_H

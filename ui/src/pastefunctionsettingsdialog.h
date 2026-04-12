/*
  Q Light Controller Plus
  pastefunctionsettingsdialog.h

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

#ifndef PASTEFUNCTIONSETTINGSDIALOG_H
#define PASTEFUNCTIONSETTINGSDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QCheckBox>

class QLabel;
class QVBoxLayout;
class Function;
class Doc;

/** @addtogroup ui_functions
 * @{
 */

/**
 * Dialog shown when the user pastes function settings from the system
 * clipboard onto one or more selected functions.  It shows a summary of
 * what is in the clipboard and lets the user de-select individual parameter
 * groups before applying.
 */
class PasteFunctionSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param clipboardJson  Parsed JSON root from the clipboard.
     * @param targets        Functions to apply the settings to.
     * @param doc            The owning Doc.
     * @param parent         Parent widget.
     */
    PasteFunctionSettingsDialog(const QJsonObject &clipboardJson,
                                const QList<Function*> &targets,
                                Doc *doc,
                                QWidget *parent = nullptr);
    ~PasteFunctionSettingsDialog() = default;

    /** @reimp: apply to all target functions and accept. */
    void accept() override;

    /**
     * Read the system clipboard and validate it.
     * @return true if the clipboard contains parseable function settings.
     */
    static bool clipboardHasFunctionSettings();

    /**
     * Parse the system clipboard into a JSON object.
     * @return empty object on failure.
     */
    static QJsonObject clipboardJson();

private slots:
    void slotSelectAllToggled(bool checked);
    void slotCheckBoxToggled(bool checked);

private:
    void buildUI();
    int selectedFlags() const;

    const QJsonObject m_json;
    QList<Function*> m_targets;
    Doc *m_doc;
    int m_storedFlags;

    QCheckBox *m_selectAllBox;
    QList<QPair<int, QCheckBox*>> m_entries;
    bool m_blockSignals;
};

/** @} */

#endif // PASTEFUNCTIONSETTINGSDIALOG_H

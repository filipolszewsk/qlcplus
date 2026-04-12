/*
  Q Light Controller Plus
  importvcwidgetsdialog.h

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

#ifndef IMPORTVCWIDGETSDIALOG_H
#define IMPORTVCWIDGETSDIALOG_H

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class VCWidget;
class VCFrame;
class Doc;

/** @addtogroup ui_vc_widgets
 * @{
 */

/**
 * Dialog shown when pasting VC widgets from the system clipboard into the
 * Virtual Console.  Shows the widget tree, highlights unresolved function
 * references, and lets the user confirm before creating the widgets.
 */
class ImportVCWidgetsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param clipJson   Parsed JSON root from clipboard (type = "qlc_vc_widgets").
     * @param targetFrame  Frame to paste widgets into.
     * @param doc          The owning Doc.
     * @param parent       Parent widget.
     */
    ImportVCWidgetsDialog(const QJsonObject &clipJson,
                          VCFrame *targetFrame,
                          Doc *doc,
                          QWidget *parent = nullptr);
    ~ImportVCWidgetsDialog() = default;

    /** Returns the number of widgets that were successfully created. */
    int importedCount() const { return m_importedCount; }

    /** @reimp: create widgets in target frame and accept. */
    void accept() override;

private:
    void buildUI();
    void populateTree(const QJsonArray &widgets, QTreeWidgetItem *parent);
    void createWidgets(const QJsonArray &widgets, VCFrame *parentFrame, int &count);

    const QJsonObject m_json;
    VCFrame *m_targetFrame;
    Doc *m_doc;
    int m_importedCount;

    QTreeWidget *m_tree;
    QLabel *m_statusLabel;
};

/** @} */

#endif // IMPORTVCWIDGETSDIALOG_H

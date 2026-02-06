/*
  Q Light Controller Plus
  importfunctionsdialog.h

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

#ifndef IMPORTFUNCTIONSDIALOG_H
#define IMPORTFUNCTIONSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QList>

class QXmlStreamReader;
class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QLineEdit;
class QLabel;
class Doc;
class Function;
class Fixture;
class FixtureGroup;

/** @addtogroup ui
 * @{
 */

class ImportFunctionsDialog : public QDialog
{
    Q_OBJECT

public:
    ImportFunctionsDialog(QWidget *parent, Doc *doc);
    ~ImportFunctionsDialog();

    /** Returns the number of functions that were successfully imported */
    int importedCount() const;

private:
    /** Load workspace contents from the given XML reader (Engine section only) */
    bool loadWorkspaceXML(QXmlStreamReader &xmlReader);

    /** Load another .qxw project file into the temporary import Doc */
    bool loadProject(const QString &fileName);

    /** Populate the function tree from the import Doc */
    void populateFunctionTree();

    /** Recursively check dependencies of a function and auto-select them */
    void checkFunctionDependency(quint32 fid);

    /** Recursively import a function and its dependencies first */
    void importFunctionID(quint32 funcID);

    /** Execute the full import of selected functions. Returns false if cancelled. */
    bool performImport();

    /** Update the status label with current selection info */
    void updateStatusLabel();

    /** Get a display icon for a function type */
    QIcon functionIcon(int type) const;

private slots:
    void slotBrowseClicked();
    void slotItemChanged(QTreeWidgetItem *item, int column);
    void slotSelectAll();
    void slotSelectNone();

private:
    /** The main project Doc */
    Doc *m_doc;

    /** Temporary Doc for the imported workspace */
    Doc *m_importDoc;

    /** UI Elements */
    QLineEdit *m_filePathEdit;
    QPushButton *m_browseButton;
    QTreeWidget *m_functionTree;
    QLabel *m_statusLabel;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_importButton;

    /** Selected function IDs from import Doc */
    QList<quint32> m_functionIDList;

    /** Fixture IDs needed by selected functions (from import Doc) */
    QList<quint32> m_fixtureIDList;

    /** Fixture group IDs needed by selected functions (from import Doc) */
    QList<quint32> m_fixtureGroupIDList;

    /** Remap maps: importDoc ID -> mainDoc ID */
    QMap<quint32, quint32> m_fixtureIDRemap;
    QMap<quint32, quint32> m_fixtureGroupIDRemap;
    QMap<quint32, quint32> m_functionIDRemap;

    /** Number of successfully imported functions */
    int m_importedCount;

    /** Flag to prevent recursive item-changed signals */
    bool m_updatingTree;
};

/** @} */

#endif /* IMPORTFUNCTIONSDIALOG_H */

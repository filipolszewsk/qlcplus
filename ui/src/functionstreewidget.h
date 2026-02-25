/*
  Q Light Controller Plus
  functionstreewidget.h

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


#ifndef FUNCTIONSTREEWIDGET_H
#define FUNCTIONSTREEWIDGET_H

#include <QTreeWidget>
#include <QSet>

class Function;
class Doc;

/** @addtogroup ui UI
 * @{
 */

/**
 * FunctionsTreeWidget represents the tree of QLC+ functions,
 * including categories and folders.
 * It can be used anywhere in QLC+ to display functions organized
 * by categories and folders.
 * If drag & drop flags are turned on, it becomes a full node
 * editor, accessing functions' properties. Basically this mode
 * has to be used only by FunctionManager
 *
 * Data is organized in the following way:
 *
 * |              COL_NAME                     |           COL_PATH             |
 *  ------------------------------------------- --------------------------------
 * | Text: Function/category/folder name       | Text: path of category/folder  |
 * | Data:                                     |       (not set for functions)  |
 * |   Qt::UserRole: function ID (or invalid)  |                                |
 * |   Qt::UserRole + 1: category type         |                                |
 * |                     (Function::Type)      |                                |
 *  ------------------------------------------- --------------------------------
 */

class FunctionsTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    FunctionsTreeWidget(Doc* doc, QWidget *parent = 0);

    /** Update all functions to function tree */
    void updateTree();

    void clearTree();

    void functionNameChanged(quint32 fid);

    /** Add the Function with the given ID and returns
     *  a pointer to the created item */
    QTreeWidgetItem* addFunction(quint32 fid);

    /** Return a suitable parent item for the $function's type */
    QTreeWidgetItem* parentItem(const Function* function);

    /** Get the ID of the function represented by $item. */
    quint32 itemFunctionId(const QTreeWidgetItem* item) const;

    /** Get the item that represents the given function. */
    QTreeWidgetItem* functionItem(const Function* function);

    /**
     * Provide the set of function IDs that are used by VC widgets.
     * When set, each matching function item will show a VC indicator icon.
     * Pass an empty set to clear the indicators.
     */
    void setUsedFunctionIDs(const QSet<quint32>& ids);

    /**
     * Filter the tree to show only functions that are NOT used in VC
     * (showOnlyUnused = true) or show all functions (false).
     * Category and folder items are hidden recursively when all their
     * descendants are used in VC.
     */
    void filterByVCUsage(bool showOnlyUnused);

private:
    /** Update $item's contents from the given $function */
    void updateFunctionItem(QTreeWidgetItem* item, const Function* function);

    /**
     * Recursively determines folder/category visibility based on children.
     * Returns true if the item (and all its descendants) are hidden.
     */
    bool updateFolderVisibility(QTreeWidgetItem *item);

private:
    Doc* m_doc;
    QSet<quint32> m_usedFunctionIDs;

    /*********************************************************************
     * Tree folders
     *********************************************************************/
public:
    void addFolder();

    void deleteFolder(QTreeWidgetItem *item);

private:
    QTreeWidgetItem *folderItem(QString name);

private slots:
    void slotItemChanged(QTreeWidgetItem *item);

    void slotUpdateChildrenPath(QTreeWidgetItem *root);

private:
    QHash <QString, QTreeWidgetItem *> m_foldersMap;

    /*********************************************************************
     * Drag & Drop events
     *********************************************************************/
protected:
    void mousePressEvent(QMouseEvent *event);

    void dropEvent(QDropEvent *event);

private:
    QList<QTreeWidgetItem *>m_draggedItems;
};

/** @} */

#endif // FUNCTIONSTREEWIDGET_H

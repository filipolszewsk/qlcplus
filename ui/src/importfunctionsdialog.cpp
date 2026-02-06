/*
  Q Light Controller Plus
  importfunctionsdialog.cpp

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

#include <QTreeWidgetItemIterator>
#include <QXmlStreamReader>
#include <QTreeWidgetItem>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>
#include <QDebug>
#include <QIcon>

#include "importfunctionsdialog.h"
#include "fixturemappingdialog.h"

#include "rgbscriptscache.h"
#include "fixturegroup.h"
#include "collection.h"
#include "rgbmatrix.h"
#include "sequence.h"
#include "qlcfile.h"
#include "chaser.h"
#include "script.h"
#include "scene.h"
#include "efx.h"
#include "doc.h"
#include "app.h"

#define COL_NAME    0
#define COL_TYPE    1

ImportFunctionsDialog::ImportFunctionsDialog(QWidget *parent, Doc *doc)
    : QDialog(parent)
    , m_doc(doc)
    , m_importDoc(nullptr)
    , m_importedCount(0)
    , m_updatingTree(false)
{
    Q_ASSERT(doc != nullptr);

    setWindowTitle(tr("Import Functions from Project"));
    setMinimumSize(700, 500);
    resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    /* File selection row */
    QHBoxLayout *fileLayout = new QHBoxLayout();
    QLabel *fileLabel = new QLabel(tr("Source project:"), this);
    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setReadOnly(true);
    m_filePathEdit->setPlaceholderText(tr("Select a .qxw project file..."));
    m_browseButton = new QPushButton(tr("Browse..."), this);
    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(m_filePathEdit, 1);
    fileLayout->addWidget(m_browseButton);
    mainLayout->addLayout(fileLayout);

    /* Function tree */
    QLabel *treeLabel = new QLabel(tr("Select functions to import:"), this);
    mainLayout->addWidget(treeLabel);

    m_functionTree = new QTreeWidget(this);
    m_functionTree->setHeaderLabels(QStringList() << tr("Function") << tr("Type"));
    m_functionTree->setRootIsDecorated(true);
    m_functionTree->setAllColumnsShowFocus(true);
    m_functionTree->setSortingEnabled(true);
    m_functionTree->sortByColumn(COL_NAME, Qt::AscendingOrder);
    m_functionTree->header()->setStretchLastSection(false);
    m_functionTree->header()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    m_functionTree->header()->setSectionResizeMode(COL_TYPE, QHeaderView::ResizeToContents);
    mainLayout->addWidget(m_functionTree, 1);

    /* Selection buttons row */
    QHBoxLayout *selLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton(tr("Select All"), this);
    m_selectNoneButton = new QPushButton(tr("Select None"), this);
    m_statusLabel = new QLabel(this);
    selLayout->addWidget(m_selectAllButton);
    selLayout->addWidget(m_selectNoneButton);
    selLayout->addStretch();
    selLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(selLayout);

    /* Dialog buttons */
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    m_importButton = buttonBox->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
    m_importButton->setEnabled(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    /* Connections */
    connect(m_browseButton, SIGNAL(clicked()), this, SLOT(slotBrowseClicked()));
    connect(m_functionTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(slotItemChanged(QTreeWidgetItem*,int)));
    connect(m_selectAllButton, SIGNAL(clicked()), this, SLOT(slotSelectAll()));
    connect(m_selectNoneButton, SIGNAL(clicked()), this, SLOT(slotSelectNone()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    /* When Import is clicked: run import first, only accept if it succeeds */
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (performImport())
            accept();
    });

    updateStatusLabel();
}

ImportFunctionsDialog::~ImportFunctionsDialog()
{
    if (m_importDoc != nullptr)
    {
        /* Invalidate the shared fixture cache to prevent double-free */
        m_importDoc->setFixtureDefinitionCache(nullptr);
        delete m_importDoc;
        m_importDoc = nullptr;
    }
}

int ImportFunctionsDialog::importedCount() const
{
    return m_importedCount;
}

/*****************************************************************************
 * Project loading
 *****************************************************************************/

bool ImportFunctionsDialog::loadWorkspaceXML(QXmlStreamReader &xmlReader)
{
    if (xmlReader.readNextStartElement() == false)
        return false;

    if (xmlReader.name() != KXMLQLCWorkspace)
    {
        qWarning() << Q_FUNC_INFO << "Workspace node not found";
        return false;
    }

    while (xmlReader.readNextStartElement())
    {
        if (xmlReader.name() == KXMLQLCEngine)
        {
            m_importDoc->loadXML(xmlReader, false);
        }
        else
        {
            /* Skip Virtual Console and other non-engine sections */
            qDebug() << "Import: Skipping tag:" << xmlReader.name().toString();
            xmlReader.skipCurrentElement();
        }
    }

    return true;
}

bool ImportFunctionsDialog::loadProject(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    /* Clean up previous import Doc if any */
    if (m_importDoc != nullptr)
    {
        m_importDoc->setFixtureDefinitionCache(nullptr);
        delete m_importDoc;
        m_importDoc = nullptr;
    }

    /* Create a temporary Doc for importing */
    m_importDoc = new Doc(this);

    /* Delete the default cache and share the original Doc's fixture cache */
    delete m_importDoc->fixtureDefCache();
    m_importDoc->setFixtureDefinitionCache(m_doc->fixtureDefCache());

    /* Load RGB scripts so RGBMatrix functions can resolve their algorithms */
    m_importDoc->rgbScriptsCache()->load(RGBScriptsCache::systemScriptsDirectory());
    m_importDoc->rgbScriptsCache()->load(RGBScriptsCache::userScriptsDirectory());

    QXmlStreamReader *doc = QLCFile::getXMLReader(fileName);
    if (doc == nullptr || doc->device() == nullptr || doc->hasError())
    {
        qWarning() << Q_FUNC_INFO << "Unable to read from" << fileName;
        return false;
    }

    while (!doc->atEnd())
    {
        if (doc->readNext() == QXmlStreamReader::DTD)
            break;
    }

    if (doc->hasError())
    {
        QLCFile::releaseXMLReader(doc);
        return false;
    }

    m_importDoc->clearContents();
    m_importDoc->setWorkspacePath(QFileInfo(fileName).absolutePath());

    bool retval = false;
    if (doc->dtdName() == KXMLQLCWorkspace)
    {
        retval = loadWorkspaceXML(*doc);
    }
    else
    {
        qWarning() << Q_FUNC_INFO << fileName << "is not a workspace file";
    }

    QLCFile::releaseXMLReader(doc);
    return retval;
}

/*****************************************************************************
 * Tree population
 *****************************************************************************/

QIcon ImportFunctionsDialog::functionIcon(int type) const
{
    switch (type)
    {
        case Function::SceneType:      return QIcon(":/scene.png");
        case Function::ChaserType:     return QIcon(":/chaser.png");
        case Function::SequenceType:   return QIcon(":/sequence.png");
        case Function::EFXType:        return QIcon(":/efx.png");
        case Function::CollectionType: return QIcon(":/collection.png");
        case Function::RGBMatrixType:  return QIcon(":/rgbmatrix.png");
        case Function::ScriptType:     return QIcon(":/script.png");
        case Function::ShowType:       return QIcon(":/show.png");
        case Function::AudioType:      return QIcon(":/audio.png");
        case Function::VideoType:      return QIcon(":/video.png");
        default:                       return QIcon(":/function.png");
    }
}

void ImportFunctionsDialog::populateFunctionTree()
{
    m_updatingTree = true;
    m_functionTree->clear();
    m_functionIDList.clear();
    m_fixtureIDList.clear();
    m_fixtureGroupIDList.clear();

    if (m_importDoc == nullptr)
    {
        m_updatingTree = false;
        return;
    }

    /* Group functions by their folder path */
    QMap<QString, QTreeWidgetItem *> folderMap;

    for (Function *func : m_importDoc->functions())
    {
        if (func == nullptr || func->isVisible() == false)
            continue;

        QString path = func->path(true);
        QTreeWidgetItem *parentItem = nullptr;

        if (!path.isEmpty())
        {
            /* Build folder hierarchy */
            QStringList parts = path.split("/");
            QString fullPath;

            for (const QString &part : parts)
            {
                QString prevPath = fullPath;
                if (!fullPath.isEmpty())
                    fullPath += "/";
                fullPath += part;

                if (!folderMap.contains(fullPath))
                {
                    QTreeWidgetItem *folderItem;
                    if (parentItem == nullptr)
                        folderItem = new QTreeWidgetItem(m_functionTree);
                    else
                        folderItem = new QTreeWidgetItem(parentItem);

                    folderItem->setText(COL_NAME, part);
                    folderItem->setText(COL_TYPE, tr("Folder"));
                    folderItem->setIcon(COL_NAME, QIcon(":/folder.png"));
                    folderItem->setFlags(folderItem->flags() | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
                    folderItem->setCheckState(COL_NAME, Qt::Unchecked);
                    folderItem->setExpanded(true);
                    folderMap[fullPath] = folderItem;
                }
                parentItem = folderMap[fullPath];
            }
        }

        QTreeWidgetItem *item;
        if (parentItem != nullptr)
            item = new QTreeWidgetItem(parentItem);
        else
            item = new QTreeWidgetItem(m_functionTree);

        item->setText(COL_NAME, func->name());
        item->setText(COL_TYPE, Function::typeToString(func->type()));
        item->setIcon(COL_NAME, functionIcon(func->type()));
        item->setData(COL_NAME, Qt::UserRole, func->id());
        item->setData(COL_NAME, Qt::UserRole + 1, true); /* Mark as function item (not folder) */
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(COL_NAME, Qt::Unchecked);
    }

    m_updatingTree = false;
    updateStatusLabel();
}

/*****************************************************************************
 * Dependency resolution
 *****************************************************************************/

void ImportFunctionsDialog::checkFunctionDependency(quint32 fid)
{
    Function *func = m_importDoc->function(fid);
    if (func == nullptr)
        return;

    QList<quint32> funcList;
    QList<quint32> fxList;
    QList<quint32> fxGroupList;

    switch (func->type())
    {
        case Function::SceneType:
        {
            Scene *scene = qobject_cast<Scene *>(func);
            fxList = scene->components();
            fxGroupList = scene->fixtureGroups();
        }
        break;

        case Function::EFXType:
            fxList = func->components();
        break;

        case Function::RGBMatrixType:
        {
            RGBMatrix *rgbm = qobject_cast<RGBMatrix *>(func);
            fxList = rgbm->components();
            quint32 groupID = rgbm->fixtureGroup();
            if (groupID != FixtureGroup::invalidId())
                fxGroupList.append(groupID);
        }
        break;

        case Function::ChaserType:
        case Function::SequenceType:
        case Function::CollectionType:
            funcList = func->components();
        break;

        case Function::ScriptType:
        {
            Script *script = qobject_cast<Script *>(func);
            funcList = script->functionList();
            fxList = script->fixtureList();
        }
        break;

        default:
        break;
    }

    /* Add required fixture groups and their fixtures */
    for (quint32 groupID : fxGroupList)
    {
        if (groupID != FixtureGroup::invalidId() &&
            !m_fixtureGroupIDList.contains(groupID))
        {
            FixtureGroup *group = m_importDoc->fixtureGroup(groupID);
            if (group != nullptr)
            {
                m_fixtureGroupIDList.append(groupID);
                for (quint32 id : group->fixtureList())
                {
                    if (!m_fixtureIDList.contains(id))
                        m_fixtureIDList.append(id);
                }
            }
        }
    }

    /* Add required fixtures */
    for (quint32 fixtureID : fxList)
    {
        if (!m_fixtureIDList.contains(fixtureID))
            m_fixtureIDList.append(fixtureID);
    }

    /* Add required functions and recurse */
    for (quint32 functionID : funcList)
    {
        if (!m_functionIDList.contains(functionID))
        {
            m_functionIDList.append(functionID);
            checkFunctionDependency(functionID);
        }
    }
}

/*****************************************************************************
 * Import logic
 *****************************************************************************/

void ImportFunctionsDialog::importFunctionID(quint32 funcID)
{
    Function *importFunction = m_importDoc->function(funcID);
    if (importFunction == nullptr)
    {
        m_functionIDList.removeOne(funcID);
        return;
    }

    QList<quint32> depFuncList;

    /* 1. Get dependent function IDs */
    switch (importFunction->type())
    {
        case Function::ChaserType:
        case Function::SequenceType:
        case Function::CollectionType:
            depFuncList = importFunction->components();
        break;

        case Function::ScriptType:
        {
            Script *script = qobject_cast<Script *>(importFunction);
            depFuncList = script->functionList();
        }
        break;
        default:
        break;
    }

    /* 2. Import dependencies first */
    for (quint32 depID : depFuncList)
    {
        if (m_functionIDList.contains(depID))
            importFunctionID(depID);
    }

    /* 3. Create a copy in the main Doc */
    Function *docFunction = importFunction->createCopy(m_doc, true);
    if (docFunction == nullptr)
    {
        qWarning() << "Failed to create copy of function" << importFunction->name();
        m_functionIDList.removeOne(funcID);
        return;
    }

    m_functionIDRemap[funcID] = docFunction->id();
    m_importedCount++;

    qDebug() << "Imported function" << docFunction->name()
             << "with ID" << docFunction->id();

    /* 4. Remap references based on function type */
    switch (docFunction->type())
    {
        case Function::SceneType:
        {
            Scene *scene = qobject_cast<Scene *>(docFunction);
            QList<SceneValue> sceneValues = scene->values();
            QList<quint32> fixtureGroupList = scene->fixtureGroups();

            scene->clear();

            /* Remap fixture groups */
            for (quint32 gid : fixtureGroupList)
            {
                if (m_fixtureGroupIDRemap.contains(gid))
                    scene->addFixtureGroup(m_fixtureGroupIDRemap[gid]);
            }

            /* Remap scene values against fixture remap */
            for (SceneValue scv : sceneValues)
            {
                if (m_fixtureIDRemap.contains(scv.fxi))
                {
                    scv.fxi = m_fixtureIDRemap[scv.fxi];
                    scene->setValue(scv);
                }
            }
        }
        break;

        case Function::CollectionType:
        {
            Collection *collection = qobject_cast<Collection *>(docFunction);
            QList<quint32> funcList = collection->functions();

            for (quint32 id : funcList)
                collection->removeFunction(id);

            for (quint32 id : funcList)
            {
                if (m_functionIDRemap.contains(id))
                    collection->addFunction(m_functionIDRemap[id]);
            }
        }
        break;

        case Function::ChaserType:
        {
            Chaser *chaser = qobject_cast<Chaser *>(docFunction);
            QList<quint32> removeList;

            for (int i = 0; i < chaser->stepsCount(); i++)
            {
                ChaserStep *step = chaser->stepAt(i);
                if (m_functionIDRemap.contains(step->fid))
                {
                    step->fid = m_functionIDRemap[step->fid];
                }
                else
                {
                    /* If the function is not in the remap and doesn't exist
                     * in the target doc, mark for removal */
                    if (m_doc->function(step->fid) == nullptr)
                        removeList.append(i);
                }
            }

            if (!removeList.isEmpty())
            {
                std::sort(removeList.begin(), removeList.end());
                for (int i = removeList.count() - 1; i >= 0; i--)
                    chaser->removeStep(removeList.at(i));
            }
        }
        break;

        case Function::SequenceType:
        {
            Sequence *sequence = qobject_cast<Sequence *>(docFunction);
            quint32 boundSceneID = sequence->boundSceneID();

            if (boundSceneID != Function::invalidId() &&
                m_functionIDRemap.contains(boundSceneID))
            {
                sequence->setBoundSceneID(m_functionIDRemap[boundSceneID]);
            }
        }
        break;

        case Function::EFXType:
        {
            EFX *efx = qobject_cast<EFX *>(docFunction);
            for (EFXFixture *efxFixture : efx->fixtures())
            {
                GroupHead head(efxFixture->head());
                if (m_fixtureIDRemap.contains(head.fxi))
                {
                    head.fxi = m_fixtureIDRemap[head.fxi];
                    efxFixture->setHead(head);
                }
            }
        }
        break;

        case Function::RGBMatrixType:
        {
            RGBMatrix *rgbm = qobject_cast<RGBMatrix *>(docFunction);
            if (rgbm->fixtureGroup() != FixtureGroup::invalidId() &&
                m_fixtureGroupIDRemap.contains(rgbm->fixtureGroup()))
            {
                rgbm->setFixtureGroup(m_fixtureGroupIDRemap[rgbm->fixtureGroup()]);
            }
        }
        break;

        default:
            qDebug() << "No remap needed for function type" << docFunction->type();
        break;
    }

    m_functionIDList.removeOne(funcID);
}

bool ImportFunctionsDialog::performImport()
{
    if (m_importDoc == nullptr || m_functionIDList.isEmpty())
        return false;

    m_importedCount = 0;

    /* Clear remap maps */
    m_fixtureIDRemap.clear();
    m_fixtureGroupIDRemap.clear();
    m_functionIDRemap.clear();

    /* Step 1: Show fixture mapping dialog if there are fixtures to handle */
    if (!m_fixtureIDList.isEmpty())
    {
        FixtureMappingDialog mappingDialog(this, m_doc, m_importDoc,
                                           m_fixtureIDList, m_fixtureGroupIDList);
        if (mappingDialog.exec() != QDialog::Accepted)
            return false; /* User cancelled */

        m_fixtureIDRemap = mappingDialog.fixtureIDRemap();
        m_fixtureGroupIDRemap = mappingDialog.fixtureGroupIDRemap();
    }

    /* Step 2: Import functions (respecting dependency order) */
    while (!m_functionIDList.isEmpty())
    {
        importFunctionID(m_functionIDList.first());
    }

    qDebug() << "Import complete." << m_importedCount << "functions imported.";
    return true;
}

/*****************************************************************************
 * UI helpers
 *****************************************************************************/

void ImportFunctionsDialog::updateStatusLabel()
{
    int funcCount = m_functionIDList.count();
    int fxCount = m_fixtureIDList.count();
    int groupCount = m_fixtureGroupIDList.count();

    if (funcCount == 0)
    {
        m_statusLabel->setText(tr("No functions selected"));
        m_importButton->setEnabled(false);
    }
    else
    {
        QString text = tr("%1 function(s) selected").arg(funcCount);
        if (fxCount > 0)
            text += tr(", %1 fixture(s)").arg(fxCount);
        if (groupCount > 0)
            text += tr(", %1 group(s)").arg(groupCount);
        m_statusLabel->setText(text);
        m_importButton->setEnabled(true);
    }
}

/*****************************************************************************
 * Slots
 *****************************************************************************/

void ImportFunctionsDialog::slotBrowseClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open QLC+ Project"),
        QString(),
        tr("QLC+ Workspace Files (*.qxw);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    m_filePathEdit->setText(fileName);

    if (loadProject(fileName))
    {
        populateFunctionTree();
    }
    else
    {
        QMessageBox::warning(this, tr("Error"),
            tr("Failed to load the project file.\n"
               "Make sure it is a valid QLC+ workspace file."));
        m_functionTree->clear();
    }

    updateStatusLabel();
}

void ImportFunctionsDialog::slotItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_updatingTree || column != COL_NAME)
        return;

    m_updatingTree = true;

    /* Rebuild the selected function list from all checked items */
    m_functionIDList.clear();
    m_fixtureIDList.clear();
    m_fixtureGroupIDList.clear();

    QTreeWidgetItemIterator it(m_functionTree);
    while (*it)
    {
        QTreeWidgetItem *treeItem = *it;
        bool isFunctionItem = treeItem->data(COL_NAME, Qt::UserRole + 1).toBool();

        /* Only consider actual function items (not folders) */
        if (isFunctionItem && treeItem->checkState(COL_NAME) == Qt::Checked)
        {
            quint32 fid = treeItem->data(COL_NAME, Qt::UserRole).toUInt();
            if (!m_functionIDList.contains(fid))
                m_functionIDList.append(fid);
        }
        ++it;
    }

    /* Resolve dependencies for all selected functions */
    QList<quint32> selectedFunctions = m_functionIDList;
    for (quint32 fid : selectedFunctions)
    {
        checkFunctionDependency(fid);
    }

    /* Auto-check dependent functions in the tree */
    QTreeWidgetItemIterator it2(m_functionTree);
    while (*it2)
    {
        QTreeWidgetItem *treeItem = *it2;
        bool isFunctionItem = treeItem->data(COL_NAME, Qt::UserRole + 1).toBool();

        if (isFunctionItem)
        {
            quint32 fid = treeItem->data(COL_NAME, Qt::UserRole).toUInt();
            if (m_functionIDList.contains(fid) &&
                treeItem->checkState(COL_NAME) != Qt::Checked)
            {
                treeItem->setCheckState(COL_NAME, Qt::Checked);
            }
        }
        ++it2;
    }

    m_updatingTree = false;

    updateStatusLabel();
}

void ImportFunctionsDialog::slotSelectAll()
{
    m_updatingTree = true;

    QTreeWidgetItemIterator it(m_functionTree);
    while (*it)
    {
        (*it)->setCheckState(COL_NAME, Qt::Checked);
        ++it;
    }

    m_updatingTree = false;

    /* Rebuild full selection */
    slotItemChanged(nullptr, COL_NAME);
}

void ImportFunctionsDialog::slotSelectNone()
{
    m_updatingTree = true;

    m_functionIDList.clear();
    m_fixtureIDList.clear();
    m_fixtureGroupIDList.clear();

    QTreeWidgetItemIterator it(m_functionTree);
    while (*it)
    {
        (*it)->setCheckState(COL_NAME, Qt::Unchecked);
        ++it;
    }

    m_updatingTree = false;

    updateStatusLabel();
}

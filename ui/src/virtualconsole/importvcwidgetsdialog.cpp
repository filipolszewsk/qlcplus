/*
  Q Light Controller Plus
  importvcwidgetsdialog.cpp

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

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "importvcwidgetsdialog.h"
#include "virtualconsole.h"
#include "vcwidget.h"
#include "vcframe.h"
#include "function.h"
#include "doc.h"

ImportVCWidgetsDialog::ImportVCWidgetsDialog(const QJsonObject &clipJson,
                                             VCFrame *targetFrame,
                                             Doc *doc,
                                             QWidget *parent)
    : QDialog(parent)
    , m_json(clipJson)
    , m_targetFrame(targetFrame)
    , m_doc(doc)
    , m_importedCount(0)
    , m_tree(nullptr)
    , m_statusLabel(nullptr)
{
    setWindowTitle(tr("Paste Widgets from Clipboard"));
    setMinimumSize(500, 400);
    buildUI();
}

void ImportVCWidgetsDialog::buildUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *info = new QLabel(
        tr("The following widgets will be pasted into the Virtual Console:"), this);
    info->setWordWrap(true);
    layout->addWidget(info);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels(QStringList() << tr("Widget") << tr("Type") << tr("Functions"));
    m_tree->setRootIsDecorated(true);
    m_tree->header()->setStretchLastSection(true);
    layout->addWidget(m_tree, 1);

    const QJsonArray widgets = m_json["widgets"].toArray();
    populateTree(widgets, nullptr);
    m_tree->expandAll();

    m_statusLabel = new QLabel(tr("%1 widget(s) to paste.").arg(widgets.size()), this);
    layout->addWidget(m_statusLabel);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    QPushButton *pasteBtn = buttons->addButton(tr("Paste"), QDialogButtonBox::AcceptRole);
    Q_UNUSED(pasteBtn)
    buttons->addButton(QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);
}

void ImportVCWidgetsDialog::populateTree(const QJsonArray &widgets, QTreeWidgetItem *parentItem)
{
    for (const QJsonValue &v : widgets)
    {
        QJsonObject w = v.toObject();
        QString typeName;
        int wType = w["widgetType"].toInt(-1);
        if (wType >= 0)
            typeName = VCWidget::typeToString(static_cast<VCWidget::WidgetType>(wType));

        QString caption = w["caption"].toString();
        if (caption.isEmpty())
            caption = tr("(no caption)");

        /* Collect function refs */
        QStringList funcRefs;
        if (w.contains("functionName"))
        {
            QString fName = w["functionName"].toString();
            if (!fName.isEmpty())
            {
                bool found = false;
                for (Function *f : m_doc->functions())
                {
                    if (f->name() == fName)
                    {
                        found = true;
                        break;
                    }
                }
                funcRefs << (found
                             ? QString("%1 [OK]").arg(fName)
                             : QString("%1 [NOT FOUND]").arg(fName));
            }
        }

        QTreeWidgetItem *item;
        if (parentItem)
            item = new QTreeWidgetItem(parentItem);
        else
            item = new QTreeWidgetItem(m_tree);

        item->setText(0, caption);
        item->setText(1, typeName);
        item->setText(2, funcRefs.join(", "));

        /* Recurse into children */
        if (w.contains("children"))
            populateTree(w["children"].toArray(), item);
    }
}

void ImportVCWidgetsDialog::accept()
{
    const QJsonArray widgets = m_json["widgets"].toArray();
    VirtualConsole *vc = VirtualConsole::instance();
    if (vc && m_targetFrame)
    {
        vc->importWidgetsFromJson(widgets, m_targetFrame, m_importedCount);
        m_doc->setModified();
    }
    QDialog::accept();
}

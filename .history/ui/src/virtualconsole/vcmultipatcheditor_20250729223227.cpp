/*
  Q Light Controller Plus
  vcmultipatcheditor.cpp

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

#include "vcmultipatcheditor.h"
#include "vcwidget.h"
#include "vcslider.h"
#include "vcbutton.h"
#include "doc.h"

#include <QTreeWidgetItem>
#include <QDebug>

VCMultiPatchEditor::VCMultiPatchEditor(QList<VCWidget *> widgets, Doc *doc, QWidget *parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_widgets(widgets)
{
    setupUi(this);
    fillTree();
}

VCMultiPatchEditor::~VCMultiPatchEditor()
{
}

void VCMultiPatchEditor::fillTree()
{
    m_tree->clear();

    foreach (VCWidget *widget, m_widgets)
    {
        QTreeWidgetItem *topItem = new QTreeWidgetItem(m_tree);
        topItem->setText(0, widget->caption());
        topItem->setData(0, Qt::UserRole, widget->id());

        if (widget->type() == VCWidget::SliderWidget)
        {
            VCSlider *slider = qobject_cast<VCSlider*>(widget);
            if (slider)
            {
                QTreeWidgetItem *sliderItem = new QTreeWidgetItem(topItem);
                sliderItem->setText(0, tr("Slider"));
                sliderItem->setData(0, Qt::UserRole, VCSlider::sliderInputSourceId);

                QTreeWidgetItem *resetItem = new QTreeWidgetItem(topItem);
                resetItem->setText(0, tr("Override Reset"));
                resetItem->setData(0, Qt::UserRole, VCSlider::overrideResetInputSourceId);

                QTreeWidgetItem *flashItem = new QTreeWidgetItem(topItem);
                flashItem->setText(0, tr("Flash"));
                flashItem->setData(0, Qt::UserRole, VCSlider::flashButtonInputSourceId);
            }
        }
        else if (widget->type() == VCWidget::ButtonWidget)
        {
            VCButton *button = qobject_cast<VCButton*>(widget);
            if (button)
            {
                QTreeWidgetItem *buttonItem = new QTreeWidgetItem(topItem);
                buttonItem->setText(0, tr("Button"));
                buttonItem->setData(0, Qt::UserRole, 0); // Use 0 for the main input source
            }
        }
    }
}

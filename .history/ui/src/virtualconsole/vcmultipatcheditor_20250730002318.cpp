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
#include "addressdelegate.h"
#include "qlcinputsource.h"
#include "vcwidget.h"
#include "vcslider.h"
#include "vcbutton.h"
#include "vccuelist.h"
#include "vcframe.h"
#include "vcxypad.h"
#include "doc.h"

#include <QTreeWidgetItem>
#include <QDebug>

VCMultiPatchEditor::VCMultiPatchEditor(QList<VCWidget *> widgets, Doc *doc, QWidget *parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_widgets(widgets)
{
    setupUi(this);
    m_tree->setItemDelegateForColumn(1, new AddressDelegate(this));
    fillTree();
}

VCMultiPatchEditor::~VCMultiPatchEditor()
{
}

void VCMultiPatchEditor::accept()
{
    QListIterator<QTreeWidgetItem*> it(m_tree->selectedItems());
    while (it.hasNext())
    {
        QTreeWidgetItem *item = it.next();
        if (item->childCount() > 0)
            continue;

        quint32 widgetId = item->parent()->data(0, Qt::UserRole).toUInt();
        VCWidget *widget = nullptr;
        foreach(VCWidget *w, m_widgets)
        {
            if (w->id() == widgetId)
            {
                widget = w;
                break;
            }
        }

        if (widget == nullptr)
            continue;

        quint32 sourceId = item->data(0, Qt::UserRole).toUInt();
        QString softPatch = item->data(1, Qt::DisplayRole).toString();
        QStringList parts = softPatch.split(".");
        if (parts.size() == 2)
        {
            quint32 universe = parts[0].toUInt() - 1;
            quint32 channel = parts[1].toUInt() - 1;
            widget->setInputSource(QSharedPointer<QLCInputSource>(new QLCInputSource(universe, channel)), sourceId);
        }
    }

    QDialog::accept();
}

void VCMultiPatchEditor::slotItemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(item)
    Q_UNUSED(column)
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
                sliderItem->setFlags(sliderItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCSlider::sliderInputSourceId))
                    sliderItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCSlider::sliderInputSourceId)->universe() + 1).arg(widget->inputSource(VCSlider::sliderInputSourceId)->channel() + 1));

                QTreeWidgetItem *resetItem = new QTreeWidgetItem(topItem);
                resetItem->setText(0, tr("Override Reset"));
                resetItem->setData(0, Qt::UserRole, VCSlider::overrideResetInputSourceId);
                resetItem->setFlags(resetItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCSlider::overrideResetInputSourceId))
                    resetItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCSlider::overrideResetInputSourceId)->universe() + 1).arg(widget->inputSource(VCSlider::overrideResetInputSourceId)->channel() + 1));

                QTreeWidgetItem *flashItem = new QTreeWidgetItem(topItem);
                flashItem->setText(0, tr("Flash"));
                flashItem->setData(0, Qt::UserRole, VCSlider::flashButtonInputSourceId);
                flashItem->setFlags(flashItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCSlider::flashButtonInputSourceId))
                    flashItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCSlider::flashButtonInputSourceId)->universe() + 1).arg(widget->inputSource(VCSlider::flashButtonInputSourceId)->channel() + 1));
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
                buttonItem->setFlags(buttonItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(0))
                    buttonItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(0)->universe() + 1).arg(widget->inputSource(0)->channel() + 1));
            }
        }
        else if (widget->type() == VCWidget::CueListWidget)
        {
            VCCueList *cueList = qobject_cast<VCCueList*>(widget);
            if (cueList)
            {
                QTreeWidgetItem *nextItem = new QTreeWidgetItem(topItem);
                nextItem->setText(0, tr("Next Cue"));
                nextItem->setData(0, Qt::UserRole, VCCueList::nextInputSourceId);
                nextItem->setFlags(nextItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCCueList::nextInputSourceId))
                    nextItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCCueList::nextInputSourceId)->universe() + 1).arg(widget->inputSource(VCCueList::nextInputSourceId)->channel() + 1));

                QTreeWidgetItem *prevItem = new QTreeWidgetItem(topItem);
                prevItem->setText(0, tr("Previous Cue"));
                prevItem->setData(0, Qt::UserRole, VCCueList::previousInputSourceId);
                prevItem->setFlags(prevItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCCueList::previousInputSourceId))
                    prevItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCCueList::previousInputSourceId)->universe() + 1).arg(widget->inputSource(VCCueList::previousInputSourceId)->channel() + 1));

                QTreeWidgetItem *playbackItem = new QTreeWidgetItem(topItem);
                playbackItem->setText(0, tr("Playback"));
                playbackItem->setData(0, Qt::UserRole, VCCueList::playbackInputSourceId);
                playbackItem->setFlags(playbackItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCCueList::playbackInputSourceId))
                    playbackItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCCueList::playbackInputSourceId)->universe() + 1).arg(widget->inputSource(VCCueList::playbackInputSourceId)->channel() + 1));

                QTreeWidgetItem *stopItem = new QTreeWidgetItem(topItem);
                stopItem->setText(0, tr("Stop"));
                stopItem->setData(0, Qt::UserRole, VCCueList::stopInputSourceId);
                stopItem->setFlags(stopItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCCueList::stopInputSourceId))
                    stopItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCCueList::stopInputSourceId)->universe() + 1).arg(widget->inputSource(VCCueList::stopInputSourceId)->channel() + 1));

                QTreeWidgetItem *faderItem = new QTreeWidgetItem(topItem);
                faderItem->setText(0, tr("Side Fader"));
                faderItem->setData(0, Qt::UserRole, VCCueList::sideFaderInputSourceId);
                faderItem->setFlags(faderItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCCueList::sideFaderInputSourceId))
                    faderItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCCueList::sideFaderInputSourceId)->universe() + 1).arg(widget->inputSource(VCCueList::sideFaderInputSourceId)->channel() + 1));
            }
        }
        else if (widget->type() == VCWidget::FrameWidget)
        {
            VCFrame *frame = qobject_cast<VCFrame*>(widget);
            if (frame)
            {
                QTreeWidgetItem *enableItem = new QTreeWidgetItem(topItem);
                enableItem->setText(0, tr("Enable"));
                enableItem->setData(0, Qt::UserRole, VCFrame::enableInputSourceId);
                enableItem->setFlags(enableItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCFrame::enableInputSourceId))
                    enableItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCFrame::enableInputSourceId)->universe() + 1).arg(widget->inputSource(VCFrame::enableInputSourceId)->channel() + 1));

                if (frame->multipageMode())
                {
                    QTreeWidgetItem *nextItem = new QTreeWidgetItem(topItem);
                    nextItem->setText(0, tr("Next Page"));
                    nextItem->setData(0, Qt::UserRole, VCFrame::nextPageInputSourceId);
                    nextItem->setFlags(nextItem->flags() | Qt::ItemIsEditable);
                    if (widget->inputSource(VCFrame::nextPageInputSourceId))
                        nextItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCFrame::nextPageInputSourceId)->universe() + 1).arg(widget->inputSource(VCFrame::nextPageInputSourceId)->channel() + 1));

                    QTreeWidgetItem *prevItem = new QTreeWidgetItem(topItem);
                    prevItem->setText(0, tr("Previous Page"));
                    prevItem->setData(0, Qt::UserRole, VCFrame::previousPageInputSourceId);
                    prevItem->setFlags(prevItem->flags() | Qt::ItemIsEditable);
                    if (widget->inputSource(VCFrame::previousPageInputSourceId))
                        prevItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCFrame::previousPageInputSourceId)->universe() + 1).arg(widget->inputSource(VCFrame::previousPageInputSourceId)->channel() + 1));
                }
            }
        }
        else if (widget->type() == VCWidget::XYPadWidget)
        {
            VCXYPad *pad = qobject_cast<VCXYPad*>(widget);
            if (pad)
            {
                QTreeWidgetItem *panItem = new QTreeWidgetItem(topItem);
                panItem->setText(0, tr("Pan"));
                panItem->setData(0, Qt::UserRole, VCXYPad::panInputSourceId);
                panItem->setFlags(panItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCXYPad::panInputSourceId))
                    panItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCXYPad::panInputSourceId)->universe() + 1).arg(widget->inputSource(VCXYPad::panInputSourceId)->channel() + 1));

                QTreeWidgetItem *tiltItem = new QTreeWidgetItem(topItem);
                tiltItem->setText(0, tr("Tilt"));
                tiltItem->setData(0, Qt::UserRole, VCXYPad::tiltInputSourceId);
                tiltItem->setFlags(tiltItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCXYPad::tiltInputSourceId))
                    tiltItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCXYPad::tiltInputSourceId)->universe() + 1).arg(widget->inputSource(VCXYPad::tiltInputSourceId)->channel() + 1));

                QTreeWidgetItem *widthItem = new QTreeWidgetItem(topItem);
                widthItem->setText(0, tr("Width"));
                widthItem->setData(0, Qt::UserRole, VCXYPad::widthInputSourceId);
                widthItem->setFlags(widthItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCXYPad::widthInputSourceId))
                    widthItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCXYPad::widthInputSourceId)->universe() + 1).arg(widget->inputSource(VCXYPad::widthInputSourceId)->channel() + 1));

                QTreeWidgetItem *heightItem = new QTreeWidgetItem(topItem);
                heightItem->setText(0, tr("Height"));
                heightItem->setData(0, Qt::UserRole, VCXYPad::heightInputSourceId);
                heightItem->setFlags(heightItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCXYPad::heightInputSourceId))
                    heightItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCXYPad::heightInputSourceId)->universe() + 1).arg(widget->inputSource(VCXYPad::heightInputSourceId)->channel() + 1));

                QTreeWidgetItem *panFineItem = new QTreeWidgetItem(topItem);
                panFineItem->setText(0, tr("Pan Fine"));
                panFineItem->setData(0, Qt::UserRole, VCXYPad::panFineInputSourceId);
                panFineItem->setFlags(panFineItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCXYPad::panFineInputSourceId))
                    panFineItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCXYPad::panFineInputSourceId)->universe() + 1).arg(widget->inputSource(VCXYPad::panFineInputSourceId)->channel() + 1));

                QTreeWidgetItem *tiltFineItem = new QTreeWidgetItem(topItem);
                tiltFineItem->setText(0, tr("Tilt Fine"));
                tiltFineItem->setData(0, Qt::UserRole, VCXYPad::tiltFineInputSourceId);
                tiltFineItem->setFlags(tiltFineItem->flags() | Qt::ItemIsEditable);
                if (widget->inputSource(VCXYPad::tiltFineInputSourceId))
                    tiltFineItem->setData(1, Qt::DisplayRole, QString("%1.%2").arg(widget->inputSource(VCXYPad::tiltFineInputSourceId)->universe() + 1).arg(widget->inputSource(VCXYPad::tiltFineInputSourceId)->channel() + 1));
            }
        }
        m_tree->resizeColumnToContents(0);
    }
}

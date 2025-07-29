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
#include "qlcioplugin.h"
#include "qlcinputsource.h"
#include "addressdelegate.h"
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
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *topItem = m_tree->topLevelItem(i);
        quint32 widgetId = topItem->data(0, Qt::UserRole).toUInt();
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

        for (int j = 0; j < topItem->childCount(); ++j)
        {
            QTreeWidgetItem *item = topItem->child(j);
            quint32 sourceId = item->data(0, Qt::UserRole).toUInt();
            QVariant data = item->data(1, Qt::EditRole);
            if (data.isValid())
            {
                quint32 address = data.toUInt();
                if (address == QLCIOPlugin::invalidLine())
                    widget->setInputSource(QSharedPointer<QLCInputSource>(), sourceId);
                else
                {
                    quint32 universe = address >> 16;
                    quint32 channel = address & 0xFFFF;
                    widget->setInputSource(QSharedPointer<QLCInputSource>(new QLCInputSource(universe, channel)), sourceId);
                }
            }
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
                QSharedPointer<QLCInputSource> src = slider->inputSource(VCSlider::sliderInputSourceId);
                if (src)
                    sliderItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                sliderItem->setFlags(sliderItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *resetItem = new QTreeWidgetItem(topItem);
                resetItem->setText(0, tr("Override Reset"));
                resetItem->setData(0, Qt::UserRole, VCSlider::overrideResetInputSourceId);
                src = slider->inputSource(VCSlider::overrideResetInputSourceId);
                if (src)
                    resetItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                resetItem->setFlags(resetItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *flashItem = new QTreeWidgetItem(topItem);
                flashItem->setText(0, tr("Flash"));
                flashItem->setData(0, Qt::UserRole, VCSlider::flashButtonInputSourceId);
                src = slider->inputSource(VCSlider::flashButtonInputSourceId);
                if (src)
                    flashItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                flashItem->setFlags(flashItem->flags() | Qt::ItemIsEditable);
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
                QSharedPointer<QLCInputSource> src = button->inputSource();
                if (src)
                    buttonItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                buttonItem->setFlags(buttonItem->flags() | Qt::ItemIsEditable);
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
                QSharedPointer<QLCInputSource> src = cueList->inputSource(VCCueList::nextInputSourceId);
                if (src)
                    nextItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                nextItem->setFlags(nextItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *prevItem = new QTreeWidgetItem(topItem);
                prevItem->setText(0, tr("Previous Cue"));
                prevItem->setData(0, Qt::UserRole, VCCueList::previousInputSourceId);
                src = cueList->inputSource(VCCueList::previousInputSourceId);
                if (src)
                    prevItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                prevItem->setFlags(prevItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *playbackItem = new QTreeWidgetItem(topItem);
                playbackItem->setText(0, tr("Playback"));
                playbackItem->setData(0, Qt::UserRole, VCCueList::playbackInputSourceId);
                src = cueList->inputSource(VCCueList::playbackInputSourceId);
                if (src)
                    playbackItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                playbackItem->setFlags(playbackItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *stopItem = new QTreeWidgetItem(topItem);
                stopItem->setText(0, tr("Stop"));
                stopItem->setData(0, Qt::UserRole, VCCueList::stopInputSourceId);
                src = cueList->inputSource(VCCueList::stopInputSourceId);
                if (src)
                    stopItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                stopItem->setFlags(stopItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *faderItem = new QTreeWidgetItem(topItem);
                faderItem->setText(0, tr("Side Fader"));
                faderItem->setData(0, Qt::UserRole, VCCueList::sideFaderInputSourceId);
                src = cueList->inputSource(VCCueList::sideFaderInputSourceId);
                if (src)
                    faderItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                faderItem->setFlags(faderItem->flags() | Qt::ItemIsEditable);
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
                QSharedPointer<QLCInputSource> src = frame->inputSource(VCFrame::enableInputSourceId);
                if (src)
                    enableItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                enableItem->setFlags(enableItem->flags() | Qt::ItemIsEditable);

                if (frame->multipageMode())
                {
                    QTreeWidgetItem *nextItem = new QTreeWidgetItem(topItem);
                    nextItem->setText(0, tr("Next Page"));
                    nextItem->setData(0, Qt::UserRole, VCFrame::nextPageInputSourceId);
                    src = frame->inputSource(VCFrame::nextPageInputSourceId);
                    if (src)
                        nextItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                    nextItem->setFlags(nextItem->flags() | Qt::ItemIsEditable);

                    QTreeWidgetItem *prevItem = new QTreeWidgetItem(topItem);
                    prevItem->setText(0, tr("Previous Page"));
                    prevItem->setData(0, Qt::UserRole, VCFrame::previousPageInputSourceId);
                    src = frame->inputSource(VCFrame::previousPageInputSourceId);
                    if (src)
                        prevItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                    prevItem->setFlags(prevItem->flags() | Qt::ItemIsEditable);
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
                QSharedPointer<QLCInputSource> src = pad->inputSource(VCXYPad::panInputSourceId);
                if (src)
                    panItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                panItem->setFlags(panItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *tiltItem = new QTreeWidgetItem(topItem);
                tiltItem->setText(0, tr("Tilt"));
                tiltItem->setData(0, Qt::UserRole, VCXYPad::tiltInputSourceId);
                src = pad->inputSource(VCXYPad::tiltInputSourceId);
                if (src)
                    tiltItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                tiltItem->setFlags(tiltItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *widthItem = new QTreeWidgetItem(topItem);
                widthItem->setText(0, tr("Width"));
                widthItem->setData(0, Qt::UserRole, VCXYPad::widthInputSourceId);
                src = pad->inputSource(VCXYPad::widthInputSourceId);
                if (src)
                    widthItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                widthItem->setFlags(widthItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *heightItem = new QTreeWidgetItem(topItem);
                heightItem->setText(0, tr("Height"));
                heightItem->setData(0, Qt::UserRole, VCXYPad::heightInputSourceId);
                src = pad->inputSource(VCXYPad::heightInputSourceId);
                if (src)
                    heightItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                heightItem->setFlags(heightItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *panFineItem = new QTreeWidgetItem(topItem);
                panFineItem->setText(0, tr("Pan Fine"));
                panFineItem->setData(0, Qt::UserRole, VCXYPad::panFineInputSourceId);
                src = pad->inputSource(VCXYPad::panFineInputSourceId);
                if (src)
                    panFineItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                panFineItem->setFlags(panFineItem->flags() | Qt::ItemIsEditable);

                QTreeWidgetItem *tiltFineItem = new QTreeWidgetItem(topItem);
                tiltFineItem->setText(0, tr("Tilt Fine"));
                tiltFineItem->setData(0, Qt::UserRole, VCXYPad::tiltFineInputSourceId);
                src = pad->inputSource(VCXYPad::tiltFineInputSourceId);
                if (src)
                    tiltFineItem->setData(1, Qt::EditRole, (src->universe() << 16) | src->channel());
                tiltFineItem->setFlags(tiltFineItem->flags() | Qt::ItemIsEditable);
            }
        }
        topItem->setExpanded(true);
    }
}

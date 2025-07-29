/*
  Q Light Controller Plus
  vcmultipatcheditor.h

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

#ifndef VCMULTIPATCHEDITOR_H
#define VCMULTIPATCHEDITOR_H

#include <QDialog>

#include "ui_vcmultipatcheditor.h"

class VCWidget;
class Doc;

class VCMultiPatchEditor : public QDialog, public Ui_VCMultiPatchEditor
{
    Q_OBJECT

public:
    VCMultiPatchEditor(QList<VCWidget *> widgets, Doc *doc, QWidget *parent = 0);
    ~VCMultiPatchEditor();

private slots:
    void slotItemChanged(QTreeWidgetItem *item, int column);

private:
    void fillTree();

private:
    Doc *m_doc;
    QList<VCWidget *> m_widgets;
};

#endif // VCMULTIPATCHEDITOR_H

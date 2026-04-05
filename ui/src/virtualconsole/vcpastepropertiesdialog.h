/*
  Q Light Controller Plus
  vcpastepropertiesdialog.h

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

#ifndef VCPASTEPROPERTIESDIALOG_H
#define VCPASTEPROPERTIESDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QCheckBox>

#include "vcwidget.h"

class QVBoxLayout;
class QFrame;

/** @addtogroup ui_vc_widgets
 * @{
 */

/**
 * Dialog that lets the user choose which property groups to paste from a
 * source VCWidget onto a target VCWidget of the same type.
 */
class VCPastePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param source  The widget whose properties will be read (clipboard widget).
     * @param target  The widget that will receive the chosen properties.
     * @param parent  Parent widget for the dialog.
     */
    VCPastePropertiesDialog(VCWidget* source, VCWidget* target,
                            QWidget* parent = nullptr);
    ~VCPastePropertiesDialog() = default;

    /** Returns the OR-combination of all checked property group flags. */
    VCWidget::PastePropertyGroups selectedFlags() const;

private slots:
    void slotSelectAllToggled(bool checked);
    void slotCheckBoxToggled(bool checked);

private:
    void buildUI(VCWidget* source, VCWidget* target);
    QFrame* makeSeparator(const QString& title);

    QCheckBox* m_selectAllBox;
    QList<QPair<VCWidget::PastePropertyGroup, QCheckBox*>> m_entries;
    bool m_blockSignals;
};

/** @} */

#endif // VCPASTEPROPERTIESDIALOG_H

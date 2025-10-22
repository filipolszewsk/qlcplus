/*
  Q Light Controller Plus
  pantiltrangeeditor.h

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

#ifndef PANTILTRANGEEDITOR_H
#define PANTILTRANGEEDITOR_H

#include <QDialog>
#include "ui_pantiltrangeeditor.h"
#include "fixture.h"

/** @addtogroup ui UI
 * @{
 */

/**
 * PanTiltRangeEditor is a dialog for editing custom Pan/Tilt ranges
 * for fixture heads. It allows users to limit the movement range of
 * moving head fixtures in degrees.
 */
class PanTiltRangeEditor : public QDialog, public Ui_PanTiltRangeEditor
{
    Q_OBJECT
    Q_DISABLE_COPY(PanTiltRangeEditor)

public:
    PanTiltRangeEditor(QWidget *parent = nullptr);
    ~PanTiltRangeEditor();

    /**
     * Set the physical limits from the fixture definition.
     * These are displayed as reference and used to constrain the input.
     *
     * @param panMax Maximum pan angle in degrees (e.g., 540)
     * @param tiltMax Maximum tilt angle in degrees (e.g., 270)
     */
    void setPhysicalLimits(qreal panMax, qreal tiltMax);

    /**
     * Set the current Pan/Tilt range to be edited.
     *
     * @param range The PanTiltRange struct to edit
     */
    void setRange(const PanTiltRange &range);

    /**
     * Get the edited Pan/Tilt range.
     *
     * @return The edited PanTiltRange struct
     */
    PanTiltRange getRange() const;

private slots:
    void slotEnabledChanged(bool enabled);

private:
    qreal m_physPanMax;
    qreal m_physTiltMax;
};

/** @} */

#endif // PANTILTRANGEEDITOR_H


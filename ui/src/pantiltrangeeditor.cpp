/*
  Q Light Controller Plus
  pantiltrangeeditor.cpp

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

#include "pantiltrangeeditor.h"

PanTiltRangeEditor::PanTiltRangeEditor(QWidget *parent)
    : QDialog(parent)
    , m_physPanMax(360)
    , m_physTiltMax(270)
{
    setupUi(this);

    connect(m_enabledCheck, SIGNAL(toggled(bool)), 
            this, SLOT(slotEnabledChanged(bool)));
    
    // Initially disable spin boxes until enabled checkbox is checked
    slotEnabledChanged(false);
}

PanTiltRangeEditor::~PanTiltRangeEditor()
{
}

void PanTiltRangeEditor::setPhysicalLimits(qreal panMax, qreal tiltMax)
{
    m_physPanMax = panMax;
    m_physTiltMax = tiltMax;
    
    // Update label to show physical limits
    m_physicalLimitsLabel->setText(
        QString("Physical limits: Pan 0-%1°, Tilt 0-%2°")
        .arg(panMax).arg(tiltMax));
    
    // Set maximum values for spin boxes
    m_panMinSpin->setMaximum(panMax);
    m_panMaxSpin->setMaximum(panMax);
    m_tiltMinSpin->setMaximum(tiltMax);
    m_tiltMaxSpin->setMaximum(tiltMax);
    
    // Set default max values
    m_panMaxSpin->setValue(panMax);
    m_tiltMaxSpin->setValue(tiltMax);
}

void PanTiltRangeEditor::setRange(const PanTiltRange &range)
{
    m_enabledCheck->setChecked(range.enabled);
    m_panMinSpin->setValue(range.panMin);
    m_panMaxSpin->setValue(range.panMax);
    m_tiltMinSpin->setValue(range.tiltMin);
    m_tiltMaxSpin->setValue(range.tiltMax);
    
    slotEnabledChanged(range.enabled);
}

PanTiltRange PanTiltRangeEditor::getRange() const
{
    PanTiltRange range;
    range.enabled = m_enabledCheck->isChecked();
    range.panMin = m_panMinSpin->value();
    range.panMax = m_panMaxSpin->value();
    range.tiltMin = m_tiltMinSpin->value();
    range.tiltMax = m_tiltMaxSpin->value();
    return range;
}

void PanTiltRangeEditor::slotEnabledChanged(bool enabled)
{
    m_panMinSpin->setEnabled(enabled);
    m_panMaxSpin->setEnabled(enabled);
    m_tiltMinSpin->setEnabled(enabled);
    m_tiltMaxSpin->setEnabled(enabled);
}

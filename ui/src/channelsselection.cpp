/*
  Q Light Controller Plus
  channelsselection.cpp

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

#include <QTreeWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QDebug>
#include <QSettings>

#include "channelmodifiereditor.h"
#include "pantiltrangeeditor.h"
#include "channelsselection.h"
#include "channelmodifier.h"
#include "qlcfixturedef.h"
#include "qlcfixturemode.h"
#include "qlcfixturehead.h"
#include "qlcphysical.h"
#include "universe.h"
#include "doc.h"

#define KColumnName         0
#define KColumnType         1
#define KColumnSelection    2
#define KColumnBehaviour    3
#define KColumnModifier     4
#define KColumnPanTiltRange 5
#define KColumnChIdx        6
#define KColumnID           7

#define SETTINGS_GEOMETRY "channelsselection/geometry"

ChannelsSelection::ChannelsSelection(Doc *doc, QWidget *parent, ChannelSelectionType mode)
    : QDialog(parent)
    , m_doc(doc)
    , m_mode(mode)
{
    Q_ASSERT(doc != NULL);

    setupUi(this);

    QStringList hdrLabels;
    hdrLabels << tr("Name") << tr("Type");

    if (mode == NormalMode)
    {
        hdrLabels << tr("Selected");
    }
    else if (mode == ConfigurationMode)
    {
        setWindowTitle(tr("Channel properties configuration"));
        setWindowIcon(QIcon(":/fade.png"));
        hdrLabels << tr("Can fade") << tr("Behaviour") << tr("Modifier") << tr("Pan/Tilt Range");
    }

    m_channelsTree->setHeaderLabels(hdrLabels);

    updateFixturesTree();

    QSettings settings;
    QVariant geometrySettings = settings.value(SETTINGS_GEOMETRY);
    if (geometrySettings.isValid() == true)
        restoreGeometry(geometrySettings.toByteArray());

    connect(m_channelsTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(slotItemChecked(QTreeWidgetItem*, int)));
    connect(m_channelsTree, SIGNAL(expanded(QModelIndex)),
            this, SLOT(slotItemExpanded()));
    connect(m_channelsTree, SIGNAL(collapsed(QModelIndex)),
            this, SLOT(slotItemExpanded()));
    connect(m_collapseButton, SIGNAL(clicked(bool)),
            m_channelsTree, SLOT(collapseAll()));
    connect(m_expandButton, SIGNAL(clicked(bool)),
            m_channelsTree, SLOT(expandAll()));
}

ChannelsSelection::~ChannelsSelection()
{
    QSettings settings;
    settings.setValue(SETTINGS_GEOMETRY, saveGeometry());
}

void ChannelsSelection::setChannelsList(QList<SceneValue> list)
{
    if (list.count() > 0)
    {
        m_channelsList = list;
        updateFixturesTree();
    }
}

QList<SceneValue> ChannelsSelection::channelsList()
{
    return m_channelsList;
}

void ChannelsSelection::updateFixturesTree()
{
    m_channelsTree->clear();
    m_channelsTree->setIconSize(QSize(24, 24));
    m_channelsTree->setAllColumnsShowFocus(true);

    foreach (Fixture *fxi, m_doc->fixtures())
    {
        QTreeWidgetItem *topItem = NULL;
        quint32 uni = fxi->universe();
        for (int i = 0; i < m_channelsTree->topLevelItemCount(); i++)
        {
            QTreeWidgetItem* tItem = m_channelsTree->topLevelItem(i);
            quint32 tUni = tItem->text(KColumnID).toUInt();
            if (tUni == uni)
            {
                topItem = tItem;
                break;
            }
        }
        // Haven't found this universe node ? Create it.
        if (topItem == NULL)
        {
            topItem = new QTreeWidgetItem(m_channelsTree);
            topItem->setText(KColumnName, m_doc->inputOutputMap()->universes().at(uni)->name());
            topItem->setText(KColumnID, QString::number(uni));
            topItem->setExpanded(true);
        }

        QTreeWidgetItem *fItem = new QTreeWidgetItem(topItem);
        fItem->setText(KColumnName, fxi->name());
        fItem->setIcon(KColumnName, fxi->getIconFromType());
        fItem->setText(KColumnID, QString::number(fxi->id()));
        
        // Make fixture items checkable in NormalMode
        if (m_mode == NormalMode)
        {
            fItem->setFlags(fItem->flags() | Qt::ItemIsUserCheckable);
            // Check if all channels of this fixture are selected
            bool allSelected = true;
            bool anySelected = false;
            for (quint32 c = 0; c < fxi->channels(); c++)
            {
                SceneValue scv(fxi->id(), c);
                if (m_channelsList.contains(scv))
                    anySelected = true;
                else
                    allSelected = false;
            }
            if (allSelected && fxi->channels() > 0)
                fItem->setCheckState(KColumnSelection, Qt::Checked);
            else if (anySelected)
                fItem->setCheckState(KColumnSelection, Qt::PartiallyChecked);
            else
                fItem->setCheckState(KColumnSelection, Qt::Unchecked);
        }

        QList<int> forcedHTP = fxi->forcedHTPChannels();
        QList<int> forcedLTP = fxi->forcedLTPChannels();

        for (quint32 c = 0; c < fxi->channels(); c++)
        {
            const QLCChannel* channel = fxi->channel(c);
            QTreeWidgetItem *item = new QTreeWidgetItem(fItem);
            item->setText(KColumnName, QString("%1:%2").arg(c + 1)
                          .arg(channel->name()));
            item->setIcon(KColumnName, channel->getIcon());
            if (channel->group() == QLCChannel::Intensity &&
                channel->colour() != QLCChannel::NoColour)
                item->setText(KColumnType, QLCChannel::colourToString(channel->colour()));
            else
                item->setText(KColumnType, QLCChannel::groupToString(channel->group()));

            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            if (m_mode == ConfigurationMode)
            {
                if (fxi->channelCanFade(c))
                    item->setCheckState(KColumnSelection, Qt::Checked);
                else
                    item->setCheckState(KColumnSelection, Qt::Unchecked);

                QComboBox *combo = new QComboBox();
                combo->addItem("HTP", false);
                combo->addItem("LTP", false);
                combo->setProperty("treeItem", QVariant::fromValue((void *)item));
                m_channelsTree->setItemWidget(item, KColumnBehaviour, combo);

                int bIdx = 1;

                if (forcedHTP.contains(int(c)))
                    bIdx = 0;
                else if (forcedLTP.contains(int(c)))
                    bIdx = 1;
                else if (channel->group() == QLCChannel::Intensity)
                    bIdx = 0;

                combo->setCurrentIndex(bIdx);
                // set the other behaviour as true
                combo->setItemData(bIdx == 0 ? 1 : 0, true, Qt::UserRole);

                QPushButton *button = new QPushButton();
                ChannelModifier *mod = fxi->channelModifier(c);
                if (mod == NULL)
                    button->setText("...");
                else
                    button->setText(mod->name());
                button->setProperty("treeItem", QVariant::fromValue((void *)item));
                m_channelsTree->setItemWidget(item, KColumnModifier, button);

                connect(combo, SIGNAL(currentIndexChanged(int)),
                        this, SLOT(slotComboChanged(int)));
                connect(button, SIGNAL(clicked()),
                        this, SLOT(slotModifierButtonClicked()));
                
                // Add Pan/Tilt range button for Pan/Tilt channels
                if (channel->group() == QLCChannel::Pan || channel->group() == QLCChannel::Tilt)
                {
                    QPushButton *ptButton = new QPushButton();
                    ptButton->setMaximumWidth(80);
                    
                    // Determine head index for this channel
                    int headIdx = -1;
                    for (int h = 0; h < fxi->heads(); h++)
                    {
                        quint32 chNum = fxi->channelNumber(
                            channel->group() == QLCChannel::Pan ? QLCChannel::Pan : QLCChannel::Tilt,
                            QLCChannel::MSB, h);
                        if (chNum == c)
                        {
                            headIdx = h;
                            break;
                        }
                    }
                    
                    if (headIdx >= 0)
                    {
                        ptButton->setProperty("fixtureID", fxi->id());
                        ptButton->setProperty("headIndex", headIdx);
                        ptButton->setProperty("treeItem", QVariant::fromValue((void *)item));
                        
                        if (fxi->hasPanTiltRange(headIdx))
                        {
                            PanTiltRange range = fxi->getPanTiltRange(headIdx);
                            ptButton->setText(QString("P:%1-%2 T:%3-%4")
                                             .arg((int)range.panMin).arg((int)range.panMax)
                                             .arg((int)range.tiltMin).arg((int)range.tiltMax));
                        }
                        else
                        {
                            ptButton->setText("...");
                        }
                        
                        connect(ptButton, SIGNAL(clicked()),
                                this, SLOT(slotPanTiltRangeButtonClicked()));
                        m_channelsTree->setItemWidget(item, KColumnPanTiltRange, ptButton);
                    }
                }
            }
            else
            {
                SceneValue scv(fxi->id(), c);
                if (m_channelsList.contains(scv))
                    item->setCheckState(KColumnSelection, Qt::Checked);
                else
                    item->setCheckState(KColumnSelection, Qt::Unchecked);
            }
            item->setText(KColumnID, QString::number(fxi->id()));
            item->setText(KColumnChIdx, QString::number(c));
        }
    }
    m_channelsTree->header()->resizeSections(QHeaderView::ResizeToContents);
}

QList<QTreeWidgetItem *> ChannelsSelection::getSameChannels(QTreeWidgetItem *item)
{
    QList<QTreeWidgetItem *> sameChannelsList;
    Fixture *fixture = m_doc->fixture(item->text(KColumnID).toUInt());
    if (fixture == NULL)
        return sameChannelsList;

    const QLCFixtureDef *def = fixture->fixtureDef();
    if (def == NULL)
        return sameChannelsList;

    QString manufacturer = def->manufacturer();
    QString model = def->model();
    int chIdx = item->text(KColumnChIdx).toInt();

    qDebug() << "Manuf:" << manufacturer << ", model:" << model << ", ch:" << chIdx;

    for (int t = 0; t < m_channelsTree->topLevelItemCount(); t++)
    {
        QTreeWidgetItem *uniItem = m_channelsTree->topLevelItem(t);
        for (int f = 0; f < uniItem->childCount(); f++)
        {
            QTreeWidgetItem *fixItem = uniItem->child(f);
            quint32 fxID = fixItem->text(KColumnID).toUInt();
            Fixture *fxi = m_doc->fixture(fxID);
            if (fxi != NULL)
            {
                const QLCFixtureDef *tmpDef = fxi->fixtureDef();
                if (tmpDef != NULL)
                {
                    QString tmpManuf = tmpDef->manufacturer();
                    QString tmpModel = tmpDef->model();
                    if (tmpManuf == manufacturer && tmpModel == model)
                    {
                        QTreeWidgetItem* chItem = fixItem->child(chIdx);
                        if (chItem != NULL)
                            sameChannelsList.append(chItem);
                    }
                }
            }
        }
    }

    return sameChannelsList;
}

void ChannelsSelection::slotItemChecked(QTreeWidgetItem *item, int col)
{
    if (col != KColumnSelection || item->text(KColumnID).isEmpty())
        return;

    m_channelsTree->blockSignals(true);

    Qt::CheckState enable = item->checkState(KColumnSelection);

    // If this is a fixture item (has children), select/deselect all its channels
    if (item->childCount() > 0 && m_mode == NormalMode)
    {
        for (int i = 0; i < item->childCount(); i++)
        {
            QTreeWidgetItem *chItem = item->child(i);
            if (chItem == NULL)
                continue;
            chItem->setCheckState(KColumnSelection, enable);
        }
    }
    // If this is a channel item, update parent fixture checkbox state
    else if (item->parent() != NULL && m_mode == NormalMode)
    {
        QTreeWidgetItem *parentItem = item->parent();
        // Check if parent is a fixture (has this item as child and has ID)
        if (parentItem->childCount() > 0 && !parentItem->text(KColumnID).isEmpty())
        {
            // Count checked channels
            int checkedCount = 0;
            int totalCount = parentItem->childCount();
            for (int i = 0; i < totalCount; i++)
            {
                QTreeWidgetItem *childItem = parentItem->child(i);
                if (childItem == NULL)
                    continue;
                if (childItem->checkState(KColumnSelection) == Qt::Checked)
                    checkedCount++;
            }
            
            // Update fixture checkbox state
            if (checkedCount == 0)
                parentItem->setCheckState(KColumnSelection, Qt::Unchecked);
            else if (checkedCount == totalCount)
                parentItem->setCheckState(KColumnSelection, Qt::Checked);
            else
                parentItem->setCheckState(KColumnSelection, Qt::PartiallyChecked);
        }
        
        // If apply all is checked, apply to same channels
        if (m_applyAllCheck->isChecked() == true)
        {
            foreach (QTreeWidgetItem *chItem, getSameChannels(item))
                chItem->setCheckState(KColumnSelection, enable);
        }
    }
    // If apply all is checked, apply to same channels
    else if (m_applyAllCheck->isChecked() == true)
    {
        foreach (QTreeWidgetItem *chItem, getSameChannels(item))
            chItem->setCheckState(KColumnSelection, enable);
    }

    m_channelsTree->blockSignals(false);
}

void ChannelsSelection::slotItemExpanded()
{
    m_channelsTree->header()->resizeSections(QHeaderView::ResizeToContents);
}

void ChannelsSelection::slotComboChanged(int idx)
{
    Q_UNUSED(idx)
    QComboBox *combo = (QComboBox *)sender();
    if (combo != NULL)
    {
        combo->setStyleSheet("QWidget {color:red}");
        if (m_applyAllCheck->isChecked() == true)
        {
            QVariant var = combo->property("treeItem");
            QTreeWidgetItem *item = (QTreeWidgetItem *) var.value<void *>();

            foreach (QTreeWidgetItem *chItem, getSameChannels(item))
            {
                QComboBox *chCombo = qobject_cast<QComboBox *>(m_channelsTree->itemWidget(chItem, KColumnBehaviour));
                if (chCombo != NULL)
                {
                    chCombo->blockSignals(true);
                    chCombo->setCurrentIndex(idx);
                    chCombo->setStyleSheet("QWidget {color:red}");
                    chCombo->blockSignals(false);
                }
            }
        }
    }
}

void ChannelsSelection::slotModifierButtonClicked()
{
    QPushButton *button = (QPushButton *)sender();
    if (button == NULL)
        return;

    ChannelModifierEditor cme(m_doc, button->text(), this);
    if (cme.exec() == QDialog::Rejected)
        return; // User pressed cancel

    QString displayName = "...";
    ChannelModifier *modif = cme.selectedModifier();
    if (modif != NULL)
        displayName = modif->name();

    button->setText(displayName);
    if (m_applyAllCheck->isChecked() == true)
    {
        QVariant var = button->property("treeItem");
        QTreeWidgetItem *item = (QTreeWidgetItem *) var.value<void *>();

        foreach (QTreeWidgetItem *chItem, getSameChannels(item))
        {
            QPushButton *chButton = qobject_cast<QPushButton *>(m_channelsTree->itemWidget(chItem, KColumnModifier));
            if (chButton != NULL)
                chButton->setText(displayName);
        }
    }
}

void ChannelsSelection::slotPanTiltRangeButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn == NULL)
        return;
    
    quint32 fxID = btn->property("fixtureID").toUInt();
    int headIdx = btn->property("headIndex").toInt();
    
    Fixture *fxi = m_doc->fixture(fxID);
    if (!fxi)
        return;
    
    QLCFixtureMode* mode = fxi->fixtureMode();
    if (!mode)
        return;
    
    QLCPhysical phy = mode->physical();
    qreal panMax = phy.focusPanMax() > 0 ? phy.focusPanMax() : 360;
    qreal tiltMax = phy.focusTiltMax() > 0 ? phy.focusTiltMax() : 270;
    
    PanTiltRangeEditor editor(this);
    editor.setPhysicalLimits(panMax, tiltMax);
    editor.setRange(fxi->getPanTiltRange(headIdx));
    
    if (editor.exec() == QDialog::Accepted)
    {
        PanTiltRange range = editor.getRange();
        fxi->setPanTiltRange(headIdx, range);
        
        // Update button text
        if (range.enabled)
        {
            btn->setText(QString("P:%1-%2 T:%3-%4")
                        .arg((int)range.panMin).arg((int)range.panMax)
                        .arg((int)range.tiltMin).arg((int)range.tiltMax));
        }
        else
        {
            btn->setText("...");
        }
        
        // Apply to same type fixtures if checkbox checked
        if (m_applyAllCheck->isChecked())
        {
            QVariant var = btn->property("treeItem");
            QTreeWidgetItem *item = (QTreeWidgetItem *)var.value<void *>();
            
            foreach (QTreeWidgetItem *chItem, getSameChannels(item))
            {
                quint32 chFxID = chItem->text(KColumnID).toUInt();
                Fixture *chFxi = m_doc->fixture(chFxID);
                if (chFxi && chFxi->id() != fxID &&
                    chFxi->fixtureDef() == fxi->fixtureDef() &&
                    chFxi->fixtureMode() == fxi->fixtureMode())
                {
                    chFxi->setPanTiltRange(headIdx, range);
                    
                    // Update button for this channel too
                    QPushButton *chButton = qobject_cast<QPushButton *>(
                        m_channelsTree->itemWidget(chItem, KColumnPanTiltRange));
                    if (chButton != NULL)
                    {
                        if (range.enabled)
                        {
                            chButton->setText(QString("P:%1-%2 T:%3-%4")
                                            .arg((int)range.panMin).arg((int)range.panMax)
                                            .arg((int)range.tiltMin).arg((int)range.tiltMax));
                        }
                        else
                        {
                            chButton->setText("...");
                        }
                    }
                }
            }
        }
        
        // Trigger Universe update to register/unregister Pan/Tilt pairs
        m_doc->updateFixtureChannelCapabilities(fxID, 
                                                fxi->forcedHTPChannels(),
                                                fxi->forcedLTPChannels());
    }
}

void ChannelsSelection::accept()
{
    QList<int> excludeList;
    QList<int> forcedHTPList;
    QList<int> forcedLTPList;
    m_channelsList.clear();

    for (int t = 0; t < m_channelsTree->topLevelItemCount(); t++)
    {
        QTreeWidgetItem *uniItem = m_channelsTree->topLevelItem(t);
        for (int f = 0; f < uniItem->childCount(); f++)
        {
            QTreeWidgetItem *fixItem = uniItem->child(f);
            quint32 fxID = fixItem->text(KColumnID).toUInt();
            Fixture *fxi = m_doc->fixture(fxID);
            if (fxi != NULL)
            {
                excludeList.clear();
                forcedHTPList.clear();
                forcedLTPList.clear();
                for (int c = 0; c < fixItem->childCount(); c++)
                {
                    QTreeWidgetItem *chanItem = fixItem->child(c);
                    const QLCChannel* channel = fxi->channel(c);

                    if (m_mode == ConfigurationMode)
                    {
                        if (chanItem->checkState(KColumnSelection) == Qt::Unchecked)
                            excludeList.append(c);

                        QComboBox *combo = (QComboBox *)m_channelsTree->itemWidget(chanItem, KColumnBehaviour);
                        if (combo != NULL)
                        {
                            if (combo->currentIndex() == 0) // HTP
                            {
                                // do not force a channel that is already HTP by nature
                                if (channel->group() != QLCChannel::Intensity)
                                    forcedHTPList.append(c);
                            }
                            else // LTP
                            {
                                // do not force a channel that is already LTP by nature
                                if (channel->group() == QLCChannel::Intensity)
                                    forcedLTPList.append(c);
                            }
                        }
                        QPushButton *button = (QPushButton *)m_channelsTree->itemWidget(chanItem, KColumnModifier);
                        if (button != NULL)
                        {
                            ChannelModifier *mod = m_doc->modifiersCache()->modifier(button->text());
                            fxi->setChannelModifier((quint32)c, mod);
                        }
                    }
                    else
                    {
                        if (chanItem->checkState(KColumnSelection) == Qt::Checked)
                            m_channelsList.append(SceneValue(fxID, c));
                    }
                }
                if (m_mode == ConfigurationMode)
                {
                    fxi->setExcludeFadeChannels(excludeList);
                    m_doc->updateFixtureChannelCapabilities(fxi->id(), forcedHTPList, forcedLTPList);
                }
            }
        }
    }

    /* Close dialog */
    QDialog::accept();
}

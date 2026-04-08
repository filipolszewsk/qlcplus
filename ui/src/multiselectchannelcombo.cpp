/*
  Q Light Controller Plus
  multiselectchannelcombo.cpp

  Copyright (c) Heikki Junnila
                Massimo Callegari

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

#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QDebug>

#include "multiselectchannelcombo.h"

MultiSelectChannelCombo::MultiSelectChannelCombo(QWidget *parent)
    : QWidget(parent)
    , m_button(nullptr)
    , m_listWidget(nullptr)
    , m_popupVisible(false)
{
    // Create main layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Create button
    m_button = new QPushButton(this);
    m_button->setText(tr("Select channels..."));
    m_button->setStyleSheet("QPushButton { text-align: left; padding: 4px 8px; }");
    connect(m_button, &QPushButton::clicked, this, &MultiSelectChannelCombo::slotButtonClicked);
    
    layout->addWidget(m_button);
    
    // Create popup list widget (initially hidden)
    m_listWidget = new QListWidget();
    m_listWidget->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->hide();
    
    connect(m_listWidget, &QListWidget::itemChanged, this, &MultiSelectChannelCombo::slotItemChanged);
    
    setFocusPolicy(Qt::StrongFocus);
}

MultiSelectChannelCombo::~MultiSelectChannelCombo()
{
    if (m_listWidget)
    {
        m_listWidget->hide();
        delete m_listWidget;
    }
}

void MultiSelectChannelCombo::addItem(const QString &text, const QVariant &userData)
{
    ChannelItem item(text, userData);
    m_items.append(item);
    
    // Add to list widget
    QListWidgetItem *listItem = new QListWidgetItem(text, m_listWidget);
    listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
    listItem->setCheckState(Qt::Unchecked);
    listItem->setData(Qt::UserRole, userData);
}

void MultiSelectChannelCombo::clear()
{
    m_items.clear();
    m_listWidget->clear();
    updateButtonText();
}

QStringList MultiSelectChannelCombo::selectedTexts() const
{
    QStringList result;
    for (const ChannelItem &item : m_items)
    {
        if (item.selected)
            result.append(item.text);
    }
    return result;
}

QVariantList MultiSelectChannelCombo::selectedData() const
{
    QVariantList result;
    for (const ChannelItem &item : m_items)
    {
        if (item.selected)
            result.append(item.data);
    }
    return result;
}

void MultiSelectChannelCombo::setSelectedData(const QVariantList &dataList)
{
    // Clear current selection
    for (int i = 0; i < m_items.size(); i++)
    {
        m_items[i].selected = false;
    }
    
    // Set new selection based on data
    for (const QVariant &data : dataList)
    {
        for (int i = 0; i < m_items.size(); i++)
        {
            if (m_items[i].data == data)
            {
                m_items[i].selected = true;
                break;
            }
        }
    }
    
    // Update list widget checkboxes
    for (int i = 0; i < m_listWidget->count(); i++)
    {
        QListWidgetItem *listItem = m_listWidget->item(i);
        if (listItem)
        {
            bool selected = (i < m_items.size()) ? m_items[i].selected : false;
            listItem->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
        }
    }
    
    updateButtonText();
}

void MultiSelectChannelCombo::setSelectedTexts(const QStringList &textList)
{
    // Clear current selection
    for (int i = 0; i < m_items.size(); i++)
    {
        m_items[i].selected = false;
    }
    
    // Set new selection based on text
    for (const QString &text : textList)
    {
        for (int i = 0; i < m_items.size(); i++)
        {
            if (m_items[i].text == text)
            {
                m_items[i].selected = true;
                break;
            }
        }
    }
    
    // Update list widget checkboxes
    for (int i = 0; i < m_listWidget->count(); i++)
    {
        QListWidgetItem *listItem = m_listWidget->item(i);
        if (listItem)
        {
            bool selected = (i < m_items.size()) ? m_items[i].selected : false;
            listItem->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
        }
    }
    
    updateButtonText();
}

int MultiSelectChannelCombo::count() const
{
    return m_items.size();
}

int MultiSelectChannelCombo::selectedCount() const
{
    int count = 0;
    for (const ChannelItem &item : m_items)
    {
        if (item.selected)
            count++;
    }
    return count;
}

bool MultiSelectChannelCombo::hasSelection() const
{
    return selectedCount() > 0;
}

void MultiSelectChannelCombo::slotButtonClicked()
{
    if (m_popupVisible)
        hidePopup();
    else
        showPopup();
}

void MultiSelectChannelCombo::slotItemChanged(QListWidgetItem *item)
{
    if (!item)
        return;
    
    // Find corresponding item in our list
    int index = m_listWidget->row(item);
    if (index >= 0 && index < m_items.size())
    {
        m_items[index].selected = (item->checkState() == Qt::Checked);
        updateButtonText();
        emit selectionChanged();
    }
}

void MultiSelectChannelCombo::updateButtonText()
{
    QStringList selected = selectedTexts();
    
    if (selected.isEmpty())
    {
        m_button->setText(tr("Select channels..."));
    }
    else if (selected.size() == 1)
    {
        m_button->setText(selected.first());
    }
    else if (selected.size() <= 3)
    {
        m_button->setText(selected.join(", "));
    }
    else
    {
        m_button->setText(tr("%1 channels selected").arg(selected.size()));
    }
}

void MultiSelectChannelCombo::showPopup()
{
    if (m_popupVisible || m_listWidget->count() == 0)
        return;
    
    // Position popup below the button
    QPoint globalPos = mapToGlobal(QPoint(0, height()));
    
    // Adjust size to fit content
    int itemHeight = 20;
    int maxHeight = qMin(200, m_listWidget->count() * itemHeight + 10);
    int width = qMax(200, m_button->width());
    
    m_listWidget->resize(width, maxHeight);
    m_listWidget->move(globalPos);
    
    // Show popup
    m_listWidget->show();
    m_listWidget->raise();
    m_popupVisible = true;
    
    // Set focus to enable key handling
    setFocus();
}

void MultiSelectChannelCombo::hidePopup()
{
    if (!m_popupVisible)
        return;
    
    m_listWidget->hide();
    m_popupVisible = false;
}

void MultiSelectChannelCombo::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    
    // Hide popup when focus is lost, but only if it's not going to the list widget
    QWidget *focusWidget = QApplication::focusWidget();
    if (focusWidget != m_listWidget)
    {
        hidePopup();
    }
    
    QWidget::focusOutEvent(event);
}

void MultiSelectChannelCombo::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        hidePopup();
        event->accept();
        return;
    }
    
    QWidget::keyPressEvent(event);
}
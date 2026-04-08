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
#include <QFrame>
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>

#include "multiselectchannelcombo.h"

MultiSelectChannelCombo::MultiSelectChannelCombo(QWidget *parent)
    : QWidget(parent)
    , m_button(nullptr)
    , m_popupFrame(nullptr)
    , m_listWidget(nullptr)
    , m_popupVisible(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_button = new QPushButton(this);
    m_button->setText(tr("Select channels..."));
    m_button->setStyleSheet("QPushButton { text-align: left; padding: 4px 8px; }");
    connect(m_button, &QPushButton::clicked, this, &MultiSelectChannelCombo::slotButtonClicked);
    layout->addWidget(m_button);

    // Popup frame - plain window, no Qt::Popup (avoids mouse grab issues with checkboxes)
    m_popupFrame = new QFrame(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    m_popupFrame->setFrameShape(QFrame::StyledPanel);
    m_popupFrame->setFrameShadow(QFrame::Raised);
    m_popupFrame->hide();

    QVBoxLayout *frameLayout = new QVBoxLayout(m_popupFrame);
    frameLayout->setContentsMargins(1, 1, 1, 1);
    frameLayout->setSpacing(0);

    m_listWidget = new QListWidget(m_popupFrame);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    frameLayout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemChanged, this, &MultiSelectChannelCombo::slotItemChanged);
}

MultiSelectChannelCombo::~MultiSelectChannelCombo()
{
    if (m_popupVisible)
        qApp->removeEventFilter(this);

    if (m_popupFrame)
    {
        m_popupFrame->hide();
        delete m_popupFrame;
    }
}

void MultiSelectChannelCombo::addItem(const QString &text, const QVariant &userData)
{
    m_items.append(ChannelItem(text, userData));

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
        if (item.selected)
            result.append(item.text);
    return result;
}

QVariantList MultiSelectChannelCombo::selectedData() const
{
    QVariantList result;
    for (const ChannelItem &item : m_items)
        if (item.selected)
            result.append(item.data);
    return result;
}

void MultiSelectChannelCombo::setSelectedData(const QVariantList &dataList)
{
    for (int i = 0; i < m_items.size(); i++)
        m_items[i].selected = false;

    for (const QVariant &data : dataList)
        for (int i = 0; i < m_items.size(); i++)
            if (m_items[i].data == data) { m_items[i].selected = true; break; }

    // Block signals to avoid emitting selectionChanged while restoring state
    m_listWidget->blockSignals(true);
    for (int i = 0; i < m_listWidget->count(); i++)
    {
        QListWidgetItem *item = m_listWidget->item(i);
        if (item)
            item->setCheckState((i < m_items.size() && m_items[i].selected) ? Qt::Checked : Qt::Unchecked);
    }
    m_listWidget->blockSignals(false);

    updateButtonText();
}

void MultiSelectChannelCombo::setSelectedTexts(const QStringList &textList)
{
    for (int i = 0; i < m_items.size(); i++)
        m_items[i].selected = false;

    for (const QString &text : textList)
        for (int i = 0; i < m_items.size(); i++)
            if (m_items[i].text == text) { m_items[i].selected = true; break; }

    m_listWidget->blockSignals(true);
    for (int i = 0; i < m_listWidget->count(); i++)
    {
        QListWidgetItem *item = m_listWidget->item(i);
        if (item)
            item->setCheckState((i < m_items.size() && m_items[i].selected) ? Qt::Checked : Qt::Unchecked);
    }
    m_listWidget->blockSignals(false);

    updateButtonText();
}

int MultiSelectChannelCombo::count() const
{
    return m_items.size();
}

int MultiSelectChannelCombo::selectedCount() const
{
    int c = 0;
    for (const ChannelItem &item : m_items)
        if (item.selected) c++;
    return c;
}

bool MultiSelectChannelCombo::hasSelection() const
{
    return selectedCount() > 0;
}

// ── slots ──────────────────────────────────────────────────────────────────

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

    int index = m_listWidget->row(item);
    if (index >= 0 && index < m_items.size())
    {
        m_items[index].selected = (item->checkState() == Qt::Checked);
        updateButtonText();
        emit selectionChanged();
    }
}

// ── private helpers ────────────────────────────────────────────────────────

void MultiSelectChannelCombo::updateButtonText()
{
    QStringList sel = selectedTexts();
    if (sel.isEmpty())
        m_button->setText(tr("Select channels..."));
    else if (sel.size() <= 3)
        m_button->setText(sel.join(", "));
    else
        m_button->setText(tr("%1 channels selected").arg(sel.size()));
}

void MultiSelectChannelCombo::showPopup()
{
    if (m_popupVisible || m_listWidget->count() == 0)
        return;

    // Size: match button width, limit height
    int itemH   = m_listWidget->sizeHintForRow(0) + 2;
    int rows    = m_listWidget->count();
    int height  = qMin(rows * itemH + 4, 220);
    int width   = qMax(m_button->width(), 180);

    m_popupFrame->resize(width, height);

    // Position below button, flip up if too close to screen bottom
    QPoint globalPos = m_button->mapToGlobal(QPoint(0, m_button->height()));
    QRect screen = QApplication::primaryScreen()
                       ? QApplication::primaryScreen()->availableGeometry()
                       : QRect(0, 0, 1920, 1080);

    if (globalPos.y() + height > screen.bottom())
        globalPos.setY(m_button->mapToGlobal(QPoint(0, 0)).y() - height);

    m_popupFrame->move(globalPos);
    m_popupFrame->show();
    m_popupFrame->raise();
    m_popupVisible = true;

    // Install event filter on the whole application to catch outside clicks
    qApp->installEventFilter(this);
}

void MultiSelectChannelCombo::hidePopup()
{
    if (!m_popupVisible)
        return;

    qApp->removeEventFilter(this);
    m_popupFrame->hide();
    m_popupVisible = false;
}

// ── event handling ─────────────────────────────────────────────────────────

bool MultiSelectChannelCombo::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (!m_popupVisible)
        return false;

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QPoint globalPos = me->globalPosition().toPoint();

        // Close if click is outside the popup frame
        if (!m_popupFrame->geometry().contains(globalPos) &&
            !m_button->rect().contains(m_button->mapFromGlobal(globalPos)))
        {
            hidePopup();
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape)
        {
            hidePopup();
            return true;
        }
    }

    return false;
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

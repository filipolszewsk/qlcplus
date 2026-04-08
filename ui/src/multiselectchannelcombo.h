/*
  Q Light Controller Plus
  multiselectchannelcombo.h

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

#ifndef MULTISELECTCHANNELCOMBO_H
#define MULTISELECTCHANNELCOMBO_H

#include <QWidget>
#include <QStringList>
#include <QVariantList>

class QPushButton;
class QListWidget;
class QListWidgetItem;
class QFrame;

/**
 * A widget that allows selecting multiple channels from a dropdown list with checkboxes.
 * Displays selected channels as comma-separated text on a button.
 * When clicked, shows a popup with checkable list items.
 * Uses application-level event filter to properly detect clicks outside and close the popup.
 */
class MultiSelectChannelCombo : public QWidget
{
    Q_OBJECT

public:
    explicit MultiSelectChannelCombo(QWidget *parent = nullptr);
    ~MultiSelectChannelCombo();

    /** Add an item to the list with display text and associated data */
    void addItem(const QString &text, const QVariant &userData = QVariant());

    /** Clear all items */
    void clear();

    /** Get list of selected item texts */
    QStringList selectedTexts() const;

    /** Get list of selected item data */
    QVariantList selectedData() const;

    /** Set selected items by their data values */
    void setSelectedData(const QVariantList &dataList);

    /** Set selected items by their text values */
    void setSelectedTexts(const QStringList &textList);

    /** Get count of total items */
    int count() const;

    /** Get count of selected items */
    int selectedCount() const;

    /** Check if any items are selected */
    bool hasSelection() const;

signals:
    /** Emitted when selection changes */
    void selectionChanged();

protected:
    /** Event filter to catch mouse presses outside the popup */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /** Handle key press events */
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void slotButtonClicked();
    void slotItemChanged(QListWidgetItem *item);

private:
    void updateButtonText();
    void showPopup();
    void hidePopup();

    QPushButton *m_button;
    QFrame      *m_popupFrame;
    QListWidget *m_listWidget;
    bool         m_popupVisible;

    struct ChannelItem {
        QString  text;
        QVariant data;
        bool     selected;

        ChannelItem() : selected(false) {}
        ChannelItem(const QString &t, const QVariant &d) : text(t), data(d), selected(false) {}
    };

    QList<ChannelItem> m_items;
};

#endif // MULTISELECTCHANNELCOMBO_H

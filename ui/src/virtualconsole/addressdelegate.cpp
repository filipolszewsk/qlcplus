/*
  Q Light Controller Plus
  addressdelegate.cpp
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

#include <QLineEdit>
#include <QDebug>
#include <QStyledItemDelegate>
#include <QStringList>

#include "addressdelegate.h"
#include "qlcioplugin.h"

AddressDelegate::AddressDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QWidget *AddressDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    QLineEdit *editor = new QLineEdit(parent);
    return editor;
}

void AddressDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    quint32 value = index.model()->data(index, Qt::EditRole).toUInt();

    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    if (value != QLCIOPlugin::invalidLine())
    {
        quint32 universe = value >> 16;
        quint32 channel = value & 0xFFFF;
        QVariant inputVal = index.model()->data(index, Qt::UserRole + 1);
        if (inputVal.isValid() && inputVal.toInt() >= 0)
            lineEdit->setText(QString("%1.%2.%3").arg(universe + 1).arg(channel + 1).arg(inputVal.toInt()));
        else
            lineEdit->setText(QString("%1.%2").arg(universe + 1).arg(channel + 1));
    }
    else
    {
        lineEdit->setText("");
    }
}

void AddressDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    QString text = lineEdit->text();
    quint32 address = QLCIOPlugin::invalidLine();

    if (text.isEmpty() == false)
    {
        QStringList parts = text.split('.');
        if (parts.length() >= 2)
        {
            quint32 universe = parts[0].toUInt();
            quint32 channel = parts[1].toUInt();
            if (universe > 0 && channel > 0)
                address = ((universe - 1) << 16) | (channel - 1);
        }
        if (parts.length() >= 3)
        {
            bool ok = false;
            int inputVal = parts[2].toInt(&ok);
            if (ok && inputVal >= 0 && inputVal <= 255)
                model->setData(index, inputVal, Qt::UserRole + 1);
            else
                model->setData(index, -1, Qt::UserRole + 1);
        }
        else
        {
            model->setData(index, -1, Qt::UserRole + 1);
        }
    }

    model->setData(index, address, Qt::EditRole);
}

void AddressDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

QString AddressDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale);
    quint32 address = value.toUInt();
    if (address != QLCIOPlugin::invalidLine())
    {
        quint32 universe = address >> 16;
        quint32 channel = address & 0xFFFF;
        return QString("%1.%2").arg(universe + 1).arg(channel + 1);
    }
    return QString();
}

QString AddressDelegate::displayTextWithValue(quint32 address, int inputValue)
{
    if (address == QLCIOPlugin::invalidLine())
        return QString();
    quint32 universe = address >> 16;
    quint32 channel = address & 0xFFFF;
    if (inputValue >= 0)
        return QString("%1.%2.%3").arg(universe + 1).arg(channel + 1).arg(inputValue);
    return QString("%1.%2").arg(universe + 1).arg(channel + 1);
}

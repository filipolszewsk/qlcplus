/*
  Q Light Controller Plus
  channelcolumneditor.h

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

#ifndef CHANNELCOLUMNEDITOR_H
#define CHANNELCOLUMNEDITOR_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QComboBox;
class QTableWidget;
class QDoubleSpinBox;
class QStackedWidget;
class QToolButton;
class QCheckBox;
class QPushButton;
class Doc;

struct ChannelColumnInfo;

/**
 * Dialog for editing channel column settings in VCCueList
 */
class ChannelColumnEditor : public QDialog
{
    Q_OBJECT

public:
    ChannelColumnEditor(ChannelColumnInfo &info, Doc *doc, QWidget *parent = nullptr);
    ~ChannelColumnEditor();

    /** Get the edited column info */
    ChannelColumnInfo columnInfo() const;

    /**
     * When true, the "Change Channel" button is hidden/disabled.
     * The channel address is managed automatically by the recording mask.
     */
    void setAddressReadOnly(bool readOnly);

public slots:
    void accept() override;

private slots:
    void slotChangeChannel();
    void slotDisplayModeChanged(int index);
    void slotAddMapping();
    void slotRemoveMapping();

private:
    void setupUi();
    void loadFromInfo();
    void saveToInfo();
    QString channelDisplayString(quint32 fixtureId, quint32 fixtureChannel, quint32 absAddress) const;

private:
    ChannelColumnInfo &m_info;
    Doc *m_doc;

    // Pending new channel assignment (applied on accept only)
    quint32 m_newFixtureId;
    quint32 m_newFixtureChannel;
    quint32 m_newAbsAddress;
    bool m_channelChanged;

    // Channel assignment section
    QLabel *m_channelLabel;
    QPushButton *m_changeChannelBtn;

    QLineEdit *m_nameEdit;
    QComboBox *m_modeCombo;
    QCheckBox *m_hiddenCheck;
    QStackedWidget *m_settingsStack;

    // Scaled mode widgets
    QDoubleSpinBox *m_scaleMinSpin;
    QDoubleSpinBox *m_scaleMaxSpin;
    QLineEdit *m_scaleSuffixEdit;

    // Dropdown mode widgets
    QTableWidget *m_mappingTable;
    QToolButton *m_addMappingBtn;
    QToolButton *m_removeMappingBtn;
};

#endif // CHANNELCOLUMNEDITOR_H

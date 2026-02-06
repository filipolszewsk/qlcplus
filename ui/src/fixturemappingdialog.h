/*
  Q Light Controller Plus
  fixturemappingdialog.h

  Copyright (c) Filip Olszewski

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

#ifndef FIXTUREMAPPINGDIALOG_H
#define FIXTUREMAPPINGDIALOG_H

#include <QDialog>
#include <QMap>
#include <QList>

class QTableWidget;
class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class Doc;
class Fixture;
class FixtureGroup;

/** @addtogroup ui
 * @{
 */

/** Per-fixture mapping decision made by the user */
struct FixtureMappingEntry
{
    quint32 importID;       /**< Fixture ID in the import Doc */
    QString name;           /**< Fixture name */
    QString manufacturer;   /**< Fixture manufacturer */
    QString model;          /**< Fixture model */
    QString modeName;       /**< Fixture mode name */
    quint32 channels;       /**< Number of channels */
    quint32 srcUniverse;    /**< Original universe */
    quint32 srcAddress;     /**< Original address */

    enum Action { MapToExisting, CreateNew, Skip };
    Action action;

    quint32 targetFixtureID;  /**< Used when action == MapToExisting */
    int targetUniverse;       /**< Used when action == CreateNew */
    int targetAddress;        /**< Used when action == CreateNew */
};

class FixtureMappingDialog : public QDialog
{
    Q_OBJECT

public:
    FixtureMappingDialog(QWidget *parent, Doc *doc, Doc *importDoc,
                         const QList<quint32> &fixtureIDList,
                         const QList<quint32> &fixtureGroupIDList);
    ~FixtureMappingDialog();

    /** After exec(), retrieve the fixture ID remap (importID -> docID) */
    QMap<quint32, quint32> fixtureIDRemap() const;

    /** After exec(), retrieve the fixture group ID remap */
    QMap<quint32, quint32> fixtureGroupIDRemap() const;

private:
    /** Populate the mapping table with fixture entries */
    void populateTable();

    /** Build the target widget for a given row based on its action */
    void updateTargetWidget(int row);

    /** Find the first available fixture address with $channels slots */
    void getAvailableFixtureAddress(int channels, int &universe, int &address);

    /** Collect all compatible existing fixtures for a given import fixture */
    QList<Fixture *> findCompatibleFixtures(Fixture *importFixture);

    /** Build a display string for a fixture: "Name (U/Addr)" */
    QString fixtureDisplayString(Fixture *fxi) const;

    /** Update the status label */
    void updateStatusLabel();

    /** Execute the actual fixture/group creation and build remap maps */
    void applyMapping();

private slots:
    void slotActionChanged(int row);
    void slotAutoMapByName();
    void slotAutoMapByAddress();

private:
    Doc *m_doc;
    Doc *m_importDoc;

    QList<quint32> m_fixtureIDList;
    QList<quint32> m_fixtureGroupIDList;

    QTableWidget *m_table;
    QLabel *m_statusLabel;
    QPushButton *m_autoNameButton;
    QPushButton *m_autoAddrButton;

    QList<FixtureMappingEntry> m_entries;

    QMap<quint32, quint32> m_fixtureIDRemap;
    QMap<quint32, quint32> m_fixtureGroupIDRemap;
};

/** @} */

#endif /* FIXTUREMAPPINGDIALOG_H */

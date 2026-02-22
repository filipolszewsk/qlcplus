/*
  Q Light Controller Plus
  licensedialog.h

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

#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class Doc;

class LicenseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LicenseDialog(Doc *doc, QWidget *parent = nullptr);

private slots:
    void slotRefresh();
    void slotCopyHwId();

private:
    void updateDisplay();

    Doc *m_doc;
    QString m_fullHwId;
    QLabel *m_statusLabel;
    QLabel *m_nameLabel;
    QLabel *m_emailLabel;
    QLabel *m_hwIdLabel;
    QLabel *m_buyLabel;
};

#endif // LICENSEDIALOG_H

/*
  Q Light Controller Plus
  vcwidgetpluginmanagerdialog.h

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

#ifndef VCWIDGETPLUGINMANAGERDIALOG_H
#define VCWIDGETPLUGINMANAGERDIALOG_H

#include <QDialog>

namespace Ui { class VCWidgetPluginManagerDialog; }

/**
 * Dialog for managing VC widget plugins: shows installed plugins with their
 * load status, allows installing from a .qlcvcw package or bare binary, and
 * lets the user open the plugins folder or remove a plugin.
 */
class VCWidgetPluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VCWidgetPluginManagerDialog(QWidget* parent = nullptr);
    ~VCWidgetPluginManagerDialog();

private slots:
    void slotInstallFromFile();
    void slotOpenPluginsFolder();
    void slotRemovePlugin();
    void slotSelectionChanged();

private:
    void populateList();

    Ui::VCWidgetPluginManagerDialog* m_ui;
};

#endif // VCWIDGETPLUGINMANAGERDIALOG_H

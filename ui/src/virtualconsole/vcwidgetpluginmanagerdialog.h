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
 * Dialog for managing VC widget plugins.
 *
 * Tab "Installed": shows all installed plugins with their load status,
 * allows installing from a .qlcvcw file or bare binary, reloading a plugin
 * from disk without restarting QLC+, and removing a plugin.
 *
 * Tab "Settings": controls for auto-load (file watcher), developer mode
 * (hot-reload on file change), and an extra watch folder for plugin developers.
 */
class VCWidgetPluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VCWidgetPluginManagerDialog(QWidget* parent = nullptr);
    ~VCWidgetPluginManagerDialog();

public slots:
    void populateList();

private slots:
    void slotInstallFromFile();
    void slotOpenPluginsFolder();
    void slotReloadPlugin();
    void slotRemovePlugin();
    void slotSelectionChanged();

    // Settings tab
    void slotAutoLoadToggled(bool enabled);
    void slotDevModeToggled(bool enabled);
    void slotBrowseWatchFolder();
    void slotWatchFolderChanged(const QString& path);

private:
    Ui::VCWidgetPluginManagerDialog* m_ui;
};

#endif // VCWIDGETPLUGINMANAGERDIALOG_H

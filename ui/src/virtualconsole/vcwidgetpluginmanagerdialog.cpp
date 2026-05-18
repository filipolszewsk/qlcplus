/*
  Q Light Controller Plus
  vcwidgetpluginmanagerdialog.cpp

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

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QUrl>
#include <QDir>

#include "ui_vcwidgetpluginmanagerdialog.h"
#include "vcwidgetpluginmanagerdialog.h"
#include "vcwidgetplugininstaller.h"
#include "vcwidgetpluginmanager.h"
#include "vcwidgetplugininterface.h"

VCWidgetPluginManagerDialog::VCWidgetPluginManagerDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::VCWidgetPluginManagerDialog)
{
    m_ui->setupUi(this);

    m_ui->pluginList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->pluginList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_ui->pluginList->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_ui->pluginList->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    connect(m_ui->installButton,    &QPushButton::clicked,
            this, &VCWidgetPluginManagerDialog::slotInstallFromFile);
    connect(m_ui->openFolderButton, &QPushButton::clicked,
            this, &VCWidgetPluginManagerDialog::slotOpenPluginsFolder);
    connect(m_ui->removeButton,     &QPushButton::clicked,
            this, &VCWidgetPluginManagerDialog::slotRemovePlugin);
    connect(m_ui->pluginList, &QTreeWidget::itemSelectionChanged,
            this, &VCWidgetPluginManagerDialog::slotSelectionChanged);

    populateList();
}

VCWidgetPluginManagerDialog::~VCWidgetPluginManagerDialog()
{
    delete m_ui;
}

void VCWidgetPluginManagerDialog::populateList()
{
    m_ui->pluginList->clear();

    const auto entries = VCWidgetPluginManager::instance()->entries();
    for (const VCWidgetPluginEntry& entry : entries)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->pluginList);
        item->setData(0, Qt::UserRole, entry.filePath);

        if (entry.plugin != nullptr)
        {
            item->setText(0, entry.plugin->name());
            item->setText(1, entry.plugin->version());
            item->setText(2, entry.plugin->author());
            item->setText(3, tr("Loaded"));
            item->setForeground(3, QColor(40, 160, 40));
        }
        else
        {
            QFileInfo fi(entry.filePath);
            item->setText(0, fi.fileName());
            item->setText(1, QString());
            item->setText(2, QString());
            item->setText(3, tr("Error"));
            item->setForeground(3, QColor(200, 40, 40));
            item->setToolTip(3, entry.errorString);
        }
    }

    slotSelectionChanged();
}

void VCWidgetPluginManagerDialog::slotSelectionChanged()
{
    QList<QTreeWidgetItem*> sel = m_ui->pluginList->selectedItems();

    if (sel.isEmpty())
    {
        m_ui->idLabel->clear();
        m_ui->pathLabel->clear();
        m_ui->errorLabel->clear();
        m_ui->removeButton->setEnabled(false);
        return;
    }

    QTreeWidgetItem* item = sel.first();
    const QString filePath = item->data(0, Qt::UserRole).toString();

    // Find matching entry
    for (const VCWidgetPluginEntry& entry : VCWidgetPluginManager::instance()->entries())
    {
        if (entry.filePath != filePath)
            continue;

        if (entry.plugin != nullptr)
        {
            m_ui->idLabel->setText(tr("ID: %1").arg(entry.plugin->pluginId()));
            m_ui->errorLabel->clear();

            QString desc = entry.plugin->description();
            QString url  = entry.plugin->homepage();
            if (!url.isEmpty())
                desc += QString(" <a href=\"%1\">%1</a>").arg(url);
            m_ui->errorLabel->setText(desc);
            m_ui->errorLabel->setTextFormat(Qt::RichText);
            m_ui->errorLabel->setOpenExternalLinks(true);
            m_ui->errorLabel->setStyleSheet(QString());
        }
        else
        {
            m_ui->idLabel->clear();
            m_ui->errorLabel->setText(tr("Load error: %1").arg(entry.errorString));
            m_ui->errorLabel->setStyleSheet(QStringLiteral("color: #cc3333;"));
        }

        m_ui->pathLabel->setText(tr("Path: %1").arg(filePath));
        break;
    }

    // Only allow removing plugins from the user directory
    const QString userDir = VCWidgetPluginManager::userPluginDirectory().absolutePath();
    m_ui->removeButton->setEnabled(filePath.startsWith(userDir));
}

void VCWidgetPluginManagerDialog::slotInstallFromFile()
{
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Install VC Widget Plugin"), QString(),
        tr("QLC+ VC Widget Plugin (*.qlcvcw);;"
           "Plugin library (*.dll *.so *.dylib);;All files (*)"));

    if (file.isEmpty())
        return;

    VCWidgetPluginInstaller installer;
    VCWidgetPluginInstaller::Result result = installer.install(file);

    switch (result)
    {
    case VCWidgetPluginInstaller::Ok:
        QMessageBox::information(this, tr("Plugin installed"),
            tr("Plugin installed successfully.\n"
               "Restart QLC+ to use it in the Virtual Console."));
        // Reload manager so the new entry appears in the list
        VCWidgetPluginManager::instance()->load(
            VCWidgetPluginManager::userPluginDirectory());
        populateList();
        break;

    case VCWidgetPluginInstaller::AlreadyInstalled:
        QMessageBox::information(this, tr("Already installed"),
            tr("A plugin with the same ID is already installed."));
        break;

    case VCWidgetPluginInstaller::IncompatibleQt:
        QMessageBox::critical(this, tr("Incompatible plugin"),
            tr("This plugin was built for a different Qt version or compiler "
               "and cannot be loaded.\n\n"
               "Ask the plugin author for a build matching your QLC+ version."));
        break;

    case VCWidgetPluginInstaller::InvalidPackage:
        QMessageBox::critical(this, tr("Invalid package"),
            tr("The selected file does not appear to be a valid QLC+ VC widget "
               "plugin package.\n\n%1").arg(installer.lastError()));
        break;

    case VCWidgetPluginInstaller::CopyFailed:
        QMessageBox::critical(this, tr("Installation failed"),
            tr("Could not copy the plugin to the plugins folder.\n\n%1")
                .arg(installer.lastError()));
        break;
    }
}

void VCWidgetPluginManagerDialog::slotOpenPluginsFolder()
{
    QDir dir = VCWidgetPluginManager::userPluginDirectory();
    if (!dir.exists())
        dir.mkpath(dir.absolutePath());

    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void VCWidgetPluginManagerDialog::slotRemovePlugin()
{
    QList<QTreeWidgetItem*> sel = m_ui->pluginList->selectedItems();
    if (sel.isEmpty())
        return;

    const QString filePath = sel.first()->data(0, Qt::UserRole).toString();
    const QFileInfo fi(filePath);

    if (QMessageBox::question(this, tr("Remove plugin"),
            tr("Are you sure you want to remove \"%1\"?\n\n"
               "Changes take effect after restarting QLC+.")
                .arg(fi.fileName()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (!QFile::remove(filePath))
    {
        QMessageBox::critical(this, tr("Remove failed"),
            tr("Could not remove the file:\n%1").arg(filePath));
        return;
    }

    populateList();
}

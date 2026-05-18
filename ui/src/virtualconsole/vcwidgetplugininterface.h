/*
  Q Light Controller Plus
  vcwidgetplugininterface.h

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

#ifndef VCWIDGETPLUGININTERFACE_H
#define VCWIDGETPLUGININTERFACE_H

#include <QtPlugin>
#include <QString>
#include <QIcon>

class QWidget;
class Doc;
class VCWidget;

/**
 * VCWidgetPluginInterface is the interface that every external VC widget
 * plugin must implement. Plugins are loaded as Qt plugin shared libraries
 * (.dll / .so / .dylib) from the vcwidgets/ directory at startup.
 *
 * A plugin provides a factory method (createWidget) that QLC+ calls to
 * instantiate the widget when the user adds it to the Virtual Console or
 * when a project containing the widget is opened.
 *
 * Plugin authors must:
 *   1. Subclass QObject AND VCWidgetPluginInterface.
 *   2. Add Q_OBJECT and Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid).
 *   3. Add Q_INTERFACES(VCWidgetPluginInterface).
 *   4. Implement all pure virtual methods.
 *   5. The widget returned by createWidget() must inherit VCWidget.
 *
 * Example:
 * @code
 * class MyPlugin : public QObject, public VCWidgetPluginInterface
 * {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID VCWidgetPlugin_iid)
 *     Q_INTERFACES(VCWidgetPluginInterface)
 * public:
 *     QString pluginId()   const override { return "com.example.mywidget"; }
 *     QString name()       const override { return "My Widget"; }
 *     QString version()    const override { return "1.0.0"; }
 *     QString author()     const override { return "Jane Doe"; }
 *     QString category()   const override { return "Custom"; }
 *     QIcon   icon()       const override { return QIcon(":/mywidget.png"); }
 *     VCWidget* createWidget(QWidget* parent, Doc* doc) override {
 *         return new MyWidget(parent, doc);
 *     }
 * };
 * @endcode
 */
class VCWidgetPluginInterface
{
public:
    virtual ~VCWidgetPluginInterface() {}

    /**
     * Globally unique reverse-domain identifier for this plugin.
     * Must never change between versions — this is the key used in
     * project XML to look up the plugin at load time.
     * Example: "com.gridqlc.superwidget"
     */
    virtual QString pluginId() const = 0;

    /**
     * Human-readable display name shown in the "Add" menu.
     * Example: "Super Widget"
     */
    virtual QString name() const = 0;

    /**
     * Plugin version string, for display in the Plugin Manager dialog.
     * Recommended format: "MAJOR.MINOR.PATCH" (semver).
     */
    virtual QString version() const = 0;

    /**
     * Plugin author name, shown in the Plugin Manager dialog and install
     * confirmation. Example: "Filip Olszewski"
     */
    virtual QString author() const = 0;

    /**
     * Optional short description shown in Plugin Manager.
     * Default implementation returns an empty string.
     */
    virtual QString description() const { return QString(); }

    /**
     * Optional homepage / source URL shown in Plugin Manager.
     * Default implementation returns an empty string.
     */
    virtual QString homepage() const { return QString(); }

    /**
     * Category label used to group the widget in the Add menu.
     * Suggested values: "Buttons", "Faders", "Pads", "Automation", "Custom".
     * Default implementation returns "Custom".
     */
    virtual QString category() const { return QStringLiteral("Custom"); }

    /**
     * Icon shown next to the widget name in the Add menu and Plugin Manager.
     * Should be 32x32 or 64x64. Return QIcon() if no icon is available.
     */
    virtual QIcon icon() const = 0;

    /**
     * Factory method. QLC+ calls this to create a new instance of the widget.
     * The returned widget MUST inherit VCWidget and must NOT be null.
     * Ownership is transferred to the VCFrame parent.
     *
     * @param parent  The QWidget parent (a VCFrame or similar container).
     * @param doc     The QLC+ Doc instance giving access to the full engine.
     */
    virtual VCWidget* createWidget(QWidget* parent, Doc* doc) = 0;
};

#define VCWidgetPlugin_iid "org.qlcplus.VCWidgetPlugin/4.14"

Q_DECLARE_INTERFACE(VCWidgetPluginInterface, VCWidgetPlugin_iid)

#endif // VCWIDGETPLUGININTERFACE_H

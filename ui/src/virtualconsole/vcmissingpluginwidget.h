/*
  Q Light Controller Plus
  vcmissingpluginwidget.h

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

#ifndef VCMISSINGPLUGINWIDGET_H
#define VCMISSINGPLUGINWIDGET_H

#include <QString>
#include "vcwidget.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QPaintEvent;
class Doc;

/** @addtogroup ui_vc_widgets
 * @{
 */

#define KXMLQLCVCPluginWidget QStringLiteral("PluginWidget")
#define KXMLQLCVCPluginWidgetId QStringLiteral("PluginId")

/**
 * VCMissingPluginWidget is a placeholder shown in the Virtual Console when
 * a project references a VC widget plugin that is not currently installed.
 *
 * It preserves the original XML verbatim so that the project is NOT corrupted
 * if saved while the plugin is absent — re-installing the plugin will restore
 * the widget fully.
 *
 * Visual appearance: hatched grey rectangle with a warning message.
 */
class VCMissingPluginWidget : public VCWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(VCMissingPluginWidget)

public:
    /**
     * Create a placeholder for a missing plugin.
     *
     * @param parent    Parent widget (VCFrame).
     * @param doc       QLC+ document.
     * @param pluginId  The plugin id string read from XML (e.g. "com.example.foo").
     */
    VCMissingPluginWidget(QWidget* parent, Doc* doc, const QString& pluginId);
    ~VCMissingPluginWidget();

    /** The plugin id that was not found at load time. */
    QString missingPluginId() const;

    /*********************************************************************
     * Clipboard — not supported for missing widgets
     *********************************************************************/
    VCWidget* createCopy(VCWidget* parent) override;

    /*********************************************************************
     * External input
     *********************************************************************/
    void updateFeedback() override {}

    /*********************************************************************
     * Load & Save
     *********************************************************************/
public:
    /**
     * Load widget state from XML. Reads geometry from the standard
     * WindowState element for display, and captures the full XML subtree
     * verbatim for lossless saveXML.
     *
     * On entry, root must be positioned at the StartElement of the
     * PluginWidget tag (before any readNextStartElement call).
     */
    bool loadXML(QXmlStreamReader& root) override;

    /**
     * Write the original XML back, unchanged. The project is never
     * modified just because the plugin is missing.
     */
    bool saveXML(QXmlStreamWriter* doc) override;

    /*********************************************************************
     * Painting
     *********************************************************************/
protected:
    void paintEvent(QPaintEvent* e) override;

private:
    QString m_pluginId;
    QString m_rawXml;   ///< Full serialized XML of the original element
};

/** @} */

#endif // VCMISSINGPLUGINWIDGET_H

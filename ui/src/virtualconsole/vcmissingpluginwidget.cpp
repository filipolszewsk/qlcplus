/*
  Q Light Controller Plus
  vcmissingpluginwidget.cpp

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>
#include <QPainter>
#include <QDebug>

#include "vcmissingpluginwidget.h"
#include "doc.h"

VCMissingPluginWidget::VCMissingPluginWidget(QWidget* parent, Doc* doc,
                                             const QString& pluginId)
    : VCWidget(parent, doc)
    , m_pluginId(pluginId)
{
    setObjectName(VCMissingPluginWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Missing plugin:\n%1").arg(pluginId));
    resize(QSize(200, 80));
    setEnabled(false);
}

VCMissingPluginWidget::~VCMissingPluginWidget()
{
}

QString VCMissingPluginWidget::missingPluginId() const
{
    return m_pluginId;
}

/*****************************************************************************
 * Clipboard
 *****************************************************************************/

VCWidget* VCMissingPluginWidget::createCopy(VCWidget* parent)
{
    Q_UNUSED(parent)
    // Missing plugin widgets cannot be meaningfully copied.
    return nullptr;
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

/**
 * Captures the entire current element (and all descendants) from @p reader
 * into a raw XML string, consuming the reader up to and including the
 * matching EndElement.
 *
 * On entry: reader is positioned at the StartElement.
 * On exit:  reader is positioned past the EndElement (token after it).
 */
static QString captureElementXml(QXmlStreamReader& reader)
{
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    QString buffer;
    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(false);

    // Write the opening tag with all its attributes.
    writer.writeCurrentToken(reader);

    int depth = 1;
    while (!reader.atEnd() && depth > 0)
    {
        reader.readNext();

        switch (reader.tokenType())
        {
        case QXmlStreamReader::StartElement:
            writer.writeCurrentToken(reader);
            depth++;
            break;

        case QXmlStreamReader::EndElement:
            writer.writeCurrentToken(reader);
            depth--;
            break;

        case QXmlStreamReader::Characters:
            writer.writeCurrentToken(reader);
            break;

        default:
            break;
        }
    }

    return buffer;
}

bool VCMissingPluginWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLQLCVCPluginWidget)
    {
        qWarning() << Q_FUNC_INFO << "PluginWidget node not found";
        return false;
    }

    // Capture common widget attributes (ID, caption, page, group) from
    // the element attributes before we consume the element.
    loadXMLCommon(root);

    // Serialize the full element (including all children) to raw XML so we
    // can write it back verbatim in saveXML.  captureElementXml advances
    // the reader past the EndElement.
    m_rawXml = captureElementXml(root);

    // Now parse the captured XML a second time to extract window geometry
    // so that the placeholder is positioned and sized correctly.
    QXmlStreamReader replay(m_rawXml);
    while (!replay.atEnd())
    {
        replay.readNextStartElement();

        if (replay.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0;
            bool visible = false;
            loadXMLWindowState(replay, &x, &y, &w, &h, &visible);
            setGeometry(x, y, w, h);
        }
        else if (replay.name() == KXMLQLCVCWidgetAppearance)
        {
            // Intentionally skip: we show our own placeholder appearance.
            replay.skipCurrentElement();
        }
        else if (!replay.name().isEmpty())
        {
            replay.skipCurrentElement();
        }
    }

    return true;
}

bool VCMissingPluginWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    if (m_rawXml.isEmpty())
    {
        // Nothing was loaded — write a minimal placeholder so the project
        // file is not left with an empty entry.
        doc->writeStartElement(KXMLQLCVCPluginWidget);
        doc->writeAttribute(KXMLQLCVCPluginWidgetId, m_pluginId);
        saveXMLWindowState(doc);
        doc->writeEndElement();
        return true;
    }

    // Replay the captured XML token by token into the output stream.
    QXmlStreamReader replay(m_rawXml);
    while (!replay.atEnd())
    {
        replay.readNext();

        switch (replay.tokenType())
        {
        case QXmlStreamReader::StartElement:
        case QXmlStreamReader::EndElement:
        case QXmlStreamReader::Characters:
            doc->writeCurrentToken(replay);
            break;

        default:
            break;
        }
    }

    return true;
}

/*****************************************************************************
 * Painting
 *****************************************************************************/

void VCMissingPluginWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    // Draw hatched background to signal "broken / missing"
    QColor bg(60, 60, 60);
    QColor stripe(90, 30, 30);
    painter.fillRect(rect(), bg);

    painter.setPen(stripe);
    const int spacing = 10;
    for (int x = -height(); x < width(); x += spacing)
        painter.drawLine(x, 0, x + height(), height());

    // Semi-transparent overlay
    painter.fillRect(rect(), QColor(0, 0, 0, 80));

    // Warning text
    painter.setPen(QColor(220, 80, 80));
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(8);
    painter.setFont(f);

    QString msg = tr("Missing plugin:\n%1").arg(m_pluginId);
    painter.drawText(rect().adjusted(4, 4, -4, -4),
                     Qt::AlignCenter | Qt::TextWordWrap, msg);

    painter.end();

    VCWidget::paintEvent(e);
}

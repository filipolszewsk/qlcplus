#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "vcmultiinput.h"
#include "virtualconsole.h"

VCMultiInput::VCMultiInput(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
    , m_vcWidget(NULL)
{
    setType(VCWidget::MultiInputWidget);
}

VCMultiInput::~VCMultiInput()
{
}

void VCMultiInput::setWidgetId(quint32 id)
{
    m_vcWidget = VirtualConsole::instance()->widget(id);
}

VCWidget* VCMultiInput::getWidget()
{
    return m_vcWidget;
}

void VCMultiInput::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (m_vcWidget)
    {
        for (int i = 0; i < m_vcWidget->inputSourcesCount(); ++i)
        {
            QSharedPointer<QLCInputSource> source = m_vcWidget->inputSource(i);
            if (source && source->universe() == universe && source->channel() == channel)
            {
                m_vcWidget->slotInputValueChanged(universe, channel, value);
                return;
            }
        }
    }
}
>>>>>>>

void VCMultiInput::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        if (m_vcWidget)
        {
            for (int i = 0; i < m_vcWidget->inputSourcesCount(); ++i)
            {
                setInputSource(m_vcWidget->inputSource(i), i);
            }
        }
    }
    VCWidget::slotModeChanged(mode);
}
>>>>>>>
>>>>>>>

VCWidget* VCMultiInput::createCopy(VCWidget* parent)
{
    VCMultiInput* multiInput = new VCMultiInput(parent, m_doc);
    if (multiInput->copyFrom(this) == false)
    {
        delete multiInput;
        multiInput = NULL;
    }

    return multiInput;
}

bool VCMultiInput::copyFrom(const VCWidget* widget)
{
    const VCMultiInput* multiInput = qobject_cast<const VCMultiInput*> (widget);
    if (multiInput == NULL)
        return false;

    if (multiInput->m_vcWidget)
        setWidgetId(multiInput->m_vcWidget->id());

    return VCWidget::copyFrom(widget);
}

bool VCMultiInput::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLQLCVCMultiInput)
    {
        qWarning() << Q_FUNC_INFO << "Multi Input node not found";
        return false;
    }

    QXmlStreamAttributes attrs = root.attributes();

    if (attrs.hasAttribute(KXMLQLCVCWidgetID))
        setWidgetId(attrs.value(KXMLQLCVCWidgetID).toString().toUInt());

    return true;
}

bool VCMultiInput::saveXML(QXmlStreamWriter* doc)
{
    doc->writeStartElement(KXMLQLCVCMultiInput);

    if (m_vcWidget)
        doc->writeAttribute(KXMLQLCVCWidgetID, QString::number(m_vcWidget->id()));

    doc->writeEndElement();

    return true;
}

void VCMultiInput::updateFeedback()
{
    if (m_vcWidget)
        m_vcWidget->updateFeedback();
}

QStringList VCMultiInput::inputSourceNames() const
{
    if (m_vcWidget)
        return m_vcWidget->inputSourceNames();
    return QStringList();
}
>>>>>>>

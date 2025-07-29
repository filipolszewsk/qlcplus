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

void VCMultiInput::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        if (m_vcWidget)
        {
            setInputSource(m_vcWidget->inputSource());
        }
    }
    VCWidget::slotModeChanged(mode);
}

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

    setWidgetId(multiInput->widgetId());

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

    VCWidget::loadXML(root);

    if (attrs.hasAttribute(KXMLQLCVCWidgetID))
        setWidgetId(attrs.value(KXMLQLCVCWidgetID).toString().toUInt());

    return true;
}

bool VCMultiInput::saveXML(QXmlStreamWriter* doc)
{
    doc->writeStartElement(KXMLQLCVCMultiInput);

    VCWidget::saveXML(doc);

    doc->writeAttribute(KXMLQLCVCWidgetID, QString::number(widgetId()));

    doc->writeEndElement();

    return true;
}

void VCMultiInput::updateFeedback()
{
    if (m_vcWidget)
        m_vcWidget->updateFeedback();
}

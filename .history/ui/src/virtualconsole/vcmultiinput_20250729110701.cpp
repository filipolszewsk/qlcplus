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

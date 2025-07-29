#include "vcmultiinputeditor.h"
#include "qlcinputchannel.h"
#include "doc.h"

VCMultiInputEditor::VCMultiInputEditor(QWidget* parent, const QLCInputSource* source)
    : QDialog(parent)
{
    Q_ASSERT(source != NULL);

    setupUi(this);

    for (quint32 i = 0; i < doc->inputOutputMap()->universes(); ++i)
        m_universeCombo->addItem(doc->inputOutputMap()->universeName(i));

    m_universeCombo->setCurrentIndex(source->universe());
    connect(m_universeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotUniverseChanged(int)));

    connect(m_channelCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotChannelChanged(int)));

    updateChannels();
    m_channelCombo->setCurrentIndex(source->channel());
}

VCMultiInputEditor::~VCMultiInputEditor()
{
}

QLCInputSource VCMultiInputEditor::externalInput() const
{
    return m_externalInput;
}

void VCMultiInputEditor::slotUniverseChanged(int idx)
{
    m_externalInput.setUniverse(idx);
    updateChannels();
}

void VCMultiInputEditor::slotChannelChanged(int idx)
{
    m_externalInput.setChannel(idx);
}

void VCMultiInputEditor::accept()
{
    QDialog::accept();
}

void VCMultiInputEditor::updateChannels()
{
    m_channelCombo->clear();
    for (quint32 i = 0; i < doc->inputOutputMap()->channels(m_externalInput.universe()); ++i)
    {
        const QLCInputChannel* ch = doc->inputOutputMap()->channel(m_externalInput.universe(), i);
        if (ch != NULL)
            m_channelCombo->addItem(ch->name());
    }
}

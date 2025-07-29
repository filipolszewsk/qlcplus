#include "vcmultiinputeditor.h"
#include "qlcinputchannel.h"
#include "virtualconsole.h"
#include "vcwidget.h"
#include "doc.h"

VCMultiInputEditor::VCMultiInputEditor(QWidget* parent, QList<VCWidget*> widgets, const QLCInputSource* source)
    : QDialog(parent)
    , m_widgets(widgets)
{
    Q_ASSERT(source != NULL);

    setupUi(this);

    Doc* doc = VirtualConsole::instance()->getDoc();

    connect(okButton, &QPushButton::clicked, this, &VCMultiInputEditor::accept);
    connect(cancelButton, &QPushButton::clicked, this, &VCMultiInputEditor::reject);

    for (quint32 i = 0; i < doc->inputOutputMap()->universes(); ++i)
        universeCombo->addItem(doc->inputOutputMap()->universeName(i));

    universeCombo->setCurrentIndex(source->universe());
    connect(universeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotUniverseChanged(int)));

    connect(channelCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotChannelChanged(int)));

    updateChannels();
    channelCombo->setCurrentIndex(source->channel());

    tableWidget->setRowCount(m_widgets.count());
    for (int i = 0; i < m_widgets.count(); ++i)
    {
        VCWidget* widget = m_widgets.at(i);
        Q_ASSERT(widget != NULL);

        QTableWidgetItem* item = new QTableWidgetItem(widget->caption());
        tableWidget->setItem(i, 0, item);

        QSharedPointer<QLCInputSource> src = widget->inputSource();
        if (src)
        {
            item = new QTableWidgetItem(QString::number(src->universe() + 1));
            tableWidget->setItem(i, 1, item);

            item = new QTableWidgetItem(QString::number(src->channel() + 1));
            tableWidget->setItem(i, 2, item);
        }
    }
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
    for (VCWidget* widget : m_widgets)
    {
        QSharedPointer<QLCInputSource> src = widget->inputSource();
        if (src)
        {
            src->setUniverse(m_externalInput.universe());
            src->setChannel(m_externalInput.channel());
            widget->setInputSource(src);
        }
        else
        {
            QSharedPointer<QLCInputSource> newSrc(new QLCInputSource(m_externalInput.universe(), m_externalInput.channel()));
            widget->setInputSource(newSrc);
        }
    }

    QDialog::accept();
}

void VCMultiInputEditor::updateChannels()
{
    Doc* doc = VirtualConsole::instance()->getDoc();
    channelCombo->clear();
    for (quint32 i = 0; i < doc->inputOutputMap()->channels(m_externalInput.universe()); ++i)
    {
        const QLCInputChannel* ch = doc->inputOutputMap()->channel(m_externalInput.universe(), i);
        if (ch != NULL)
            channelCombo->addItem(ch->name());
    }
}

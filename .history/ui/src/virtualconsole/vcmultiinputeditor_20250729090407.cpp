#include "vcmultiinputeditor.h"
#include "vcwidget.h"
#include "qlcinputsource.h"
#include "inputpatch.h"
#include "doc.h"

#include <QDialog>
#include <QKeySequence>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSpinBox>

VCMultiInputEditor::VCMultiInputEditor(const QList<VCWidget*>& widgets, QWidget* parent)
    : QDialog(parent)
    , m_widgets(widgets)
{
    setWindowTitle(tr("Batch Edit Inputs"));
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(3);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() << tr("Widget") << tr("Universe") << tr("Channel"));
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_tableWidget);

    fillTable();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("OK"), this);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &VCMultiInputEditor::accept);
    connect(cancelButton, &QPushButton::clicked, this, &VCMultiInputEditor::reject);
}

VCMultiInputEditor::~VCMultiInputEditor()
{
}

void VCMultiInputEditor::fillTable()
{
    m_tableWidget->setRowCount(m_widgets.size());
    for (int i = 0; i < m_widgets.size(); ++i)
    {
        VCWidget* widget = m_widgets.at(i);
        QTableWidgetItem* nameItem = new QTableWidgetItem(widget->caption());
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_tableWidget->setItem(i, 0, nameItem);

        QSpinBox* universeSpin = new QSpinBox(this);
        universeSpin->setRange(1, INT_MAX);
        m_tableWidget->setCellWidget(i, 1, universeSpin);

        QSpinBox* channelSpin = new QSpinBox(this);
        channelSpin->setRange(1, INT_MAX);
        m_tableWidget->setCellWidget(i, 2, channelSpin);

        QList<QSharedPointer<QLCInputSource>> sources = widget->inputSources();
        if (!sources.isEmpty())
        {
            universeSpin->setValue(sources.first()->universe() + 1);
            channelSpin->setValue(sources.first()->channel() + 1);
        }
    }
}

void VCMultiInputEditor::applyChanges()
{
    for (int i = 0; i < m_widgets.size(); ++i)
    {
        VCWidget* widget = m_widgets.at(i);
        QSpinBox* universeSpin = qobject_cast<QSpinBox*>(m_tableWidget->cellWidget(i, 1));
        QSpinBox* channelSpin = qobject_cast<QSpinBox*>(m_tableWidget->cellWidget(i, 2));

        if (universeSpin && channelSpin)
        {
            quint32 universe = universeSpin->value() - 1;
            quint32 channel = channelSpin->value() - 1;

            QList<QSharedPointer<QLCInputSource>> sources = widget->inputSources();
            if (!sources.isEmpty())
            {
                widget->updateInputSource(sources.first(), universe, channel);
            }
            else
            {
                QSharedPointer<QLCInputSource> newSource(new QLCInputSource(universe, channel));
                widget->addInputSource(newSource);
            }
        }
    }
}

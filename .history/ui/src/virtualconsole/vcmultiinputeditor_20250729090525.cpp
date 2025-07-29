#include "vcmultiinputeditor.h"
#include "ui_vcmultiinputeditor.h"
#include "vcwidget.h"
#include "qlcinputsource.h"
#include "inputpatch.h"
#include "doc.h"

#include <QSpinBox>

VCMultiInputEditor::VCMultiInputEditor(const QList<VCWidget*>& widgets, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::VCMultiInputEditor)
    , m_widgets(widgets)
{
    ui->setupUi(this);
    fillTable();
    connect(this, &VCMultiInputEditor::accepted, this, &VCMultiInputEditor::applyChanges);
}

VCMultiInputEditor::~VCMultiInputEditor()
{
}

void VCMultiInputEditor::fillTable()
{
    ui->tableWidget->setRowCount(m_widgets.size());
    for (int i = 0; i < m_widgets.size(); ++i)
    {
        VCWidget* widget = m_widgets.at(i);
        QTableWidgetItem* nameItem = new QTableWidgetItem(widget->caption());
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 0, nameItem);

        QSpinBox* universeSpin = new QSpinBox(this);
        universeSpin->setRange(1, INT_MAX);
        ui->tableWidget->setCellWidget(i, 1, universeSpin);

        QSpinBox* channelSpin = new QSpinBox(this);
        channelSpin->setRange(1, INT_MAX);
        ui->tableWidget->setCellWidget(i, 2, channelSpin);

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
        QSpinBox* universeSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(i, 1));
        QSpinBox* channelSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(i, 2));

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

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
    int row = 0;
    for (VCWidget* widget : m_widgets)
    {
        QList<QSharedPointer<QLCInputSource>> sources = widget->inputSources();
        QStringList names = widget->inputSourceNames();

        if (sources.isEmpty())
        {
            for (int i = 0; i < names.size(); ++i)
            {
                ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
                QString name = widget->caption();
                if (i < names.size())
                    name += " - " + names.at(i);

                QTableWidgetItem* nameItem = new QTableWidgetItem(name);
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(row, 0, nameItem);

                QSpinBox* universeSpin = new QSpinBox(this);
                universeSpin->setRange(1, INT_MAX);
                ui->tableWidget->setCellWidget(row, 1, universeSpin);

                QSpinBox* channelSpin = new QSpinBox(this);
                channelSpin->setRange(1, INT_MAX);
                ui->tableWidget->setCellWidget(row, 2, channelSpin);
                row++;
            }
        }
        else
        {
            for (int i = 0; i < sources.size(); ++i)
            {
                ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
                QString name = widget->caption();
                if (i < names.size())
                    name += " - " + names.at(i);

                QTableWidgetItem* nameItem = new QTableWidgetItem(name);
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->setItem(row, 0, nameItem);

                QSpinBox* universeSpin = new QSpinBox(this);
                universeSpin->setRange(1, INT_MAX);
                ui->tableWidget->setCellWidget(row, 1, universeSpin);

                QSpinBox* channelSpin = new QSpinBox(this);
                channelSpin->setRange(1, INT_MAX);
                ui->tableWidget->setCellWidget(row, 2, channelSpin);

                universeSpin->setValue(sources.at(i)->universe() + 1);
                channelSpin->setValue(sources.at(i)->channel() + 1);
                row++;
            }
        }
    }
}

void VCMultiInputEditor::applyChanges()
{
    int row = 0;
    for (VCWidget* widget : m_widgets)
    {
        QList<QSharedPointer<QLCInputSource>> sources = widget->inputSources();
        QStringList names = widget->inputSourceNames();

        if (sources.isEmpty())
        {
            for (int i = 0; i < names.size(); ++i)
            {
                QSpinBox* universeSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 1));
                QSpinBox* channelSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 2));
                if (universeSpin && channelSpin && universeSpin->value() > 0 && channelSpin->value() > 0)
                {
                    quint32 universe = universeSpin->value() - 1;
                    quint32 channel = channelSpin->value() - 1;
                    QSharedPointer<QLCInputSource> newSource(new QLCInputSource(universe, channel));
                    widget->setInputSource(newSource, i);
                }
                row++;
            }
        }
        else
        {
            for (int i = 0; i < sources.size(); ++i)
            {
                QSpinBox* universeSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 1));
                QSpinBox* channelSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 2));
                if (universeSpin && channelSpin)
                {
                    quint32 universe = universeSpin->value() - 1;
                    quint32 channel = channelSpin->value() - 1;
                    widget->updateInputSource(sources.at(i), universe, channel);
                }
                row++;
            }
        }
    }
}

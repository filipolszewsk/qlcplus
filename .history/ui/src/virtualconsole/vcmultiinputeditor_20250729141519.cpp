#include "vcmultiinputeditor.h"
#include "ui_vcmultiinputeditor.h"
#include "vcwidget.h"
#include "vccuelist.h"
#include "vcframe.h"
#include "vcbutton.h"
#include "vcslider.h"
#include "vcxypad.h"
#include "vcaudiotriggers.h"
#include "vcclock.h"
#include "qlcinputsource.h"
#include "inputpatch.h"
#include "doc.h"
>>>>>>>

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
        QStringList names = widget->inputSourceNames();
        for (int i = 0; i < names.size(); ++i)
        {
            ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
            QString name = widget->caption() + " - " + names.at(i);

            QTableWidgetItem* nameItem = new QTableWidgetItem(name);
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(row, 0, nameItem);

            QSpinBox* universeSpin = new QSpinBox(this);
            universeSpin->setRange(0, INT_MAX);
            ui->tableWidget->setCellWidget(row, 1, universeSpin);

            QSpinBox* channelSpin = new QSpinBox(this);
            channelSpin->setRange(0, INT_MAX);
            ui->tableWidget->setCellWidget(row, 2, channelSpin);

            QSharedPointer<QLCInputSource> source = widget->inputSource(i);
            if (source)
            {
                universeSpin->setValue(source->universe() + 1);
                channelSpin->setValue(source->channel() + 1);
            }
            row++;
        }
    }
}
>>>>>>>

void VCMultiInputEditor::applyChanges()
{
    int row = 0;
    for (VCWidget* widget : m_widgets)
    {
        QStringList names = widget->inputSourceNames();
        for (int i = 0; i < names.size(); ++i)
        {
            QSpinBox* universeSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 1));
            QSpinBox* channelSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 2));
            if (universeSpin && channelSpin)
            {
                if (universeSpin->value() > 0 && channelSpin->value() > 0)
                {
                    quint32 universe = universeSpin->value() - 1;
                    quint32 channel = channelSpin->value() - 1;

                    QSharedPointer<QLCInputSource> source = widget->inputSource(i);
                    if (source)
                    {
                        widget->updateInputSource(source, universe, channel);
                    }
                    else
                    {
                        QSharedPointer<QLCInputSource> newSource(new QLCInputSource(universe, channel));
                        widget->setInputSource(newSource, i);
                    }
                }
                else
                {
>>>>>>>
                    widget->setInputSource(QSharedPointer<QLCInputSource>(), i);
                }
            }
            row++;
        }
    }
}

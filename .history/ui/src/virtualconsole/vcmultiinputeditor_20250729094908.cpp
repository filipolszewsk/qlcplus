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
        int sourcesNum = sources.size();
        if (sourcesNum < names.size())
            sourcesNum = names.size();
        if (names.isEmpty() && sources.size() > 0)
            sourcesNum = sources.size();
        else if (names.isEmpty() && widget->inputSources().size() == 0)
             sourcesNum = 1;


        for (int i = 0; i < sourcesNum; ++i)
        {
            ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
            QString name = widget->caption();
            if (i < names.size())
                name += " - " + names.at(i);

            QTableWidgetItem* nameItem = new QTableWidgetItem(name);
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(row, 0, nameItem);

            QSpinBox* universeSpin = new QSpinBox(this);
            universeSpin->setRange(0, INT_MAX);
            ui->tableWidget->setCellWidget(row, 1, universeSpin);

            QSpinBox* channelSpin = new QSpinBox(this);
            channelSpin->setRange(0, INT_MAX);
            ui->tableWidget->setCellWidget(row, 2, channelSpin);

            if (i < sources.size() && sources.at(i))
            {
                universeSpin->setValue(sources.at(i)->universe() + 1);
                channelSpin->setValue(sources.at(i)->channel() + 1);
            }
            row++;
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
        int sourcesNum = sources.size();
        if (sourcesNum < names.size())
            sourcesNum = names.size();
        if (names.isEmpty() && widget->property("hasExternalInput").toBool())
            sourcesNum = 1;

        for (int i = 0; i < sourcesNum; ++i)
        {
            QSpinBox* universeSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 1));
            QSpinBox* channelSpin = qobject_cast<QSpinBox*>(ui->tableWidget->cellWidget(row, 2));
            if (universeSpin && channelSpin)
            {
                if (universeSpin->value() > 0 && channelSpin->value() > 0)
                {
                    quint32 universe = universeSpin->value() - 1;
                    quint32 channel = channelSpin->value() - 1;

                    if (i < sources.size() && sources.at(i))
                    {
                        widget->updateInputSource(sources.at(i), universe, channel);
                    }
                    else
                    {
                        QSharedPointer<QLCInputSource> newSource(new QLCInputSource(universe, channel));
                        widget->setInputSource(newSource, i);
                    }
                }
                else
                {
                    if (i < sources.size() && sources.at(i))
                        widget->setInputSource(QSharedPointer<QLCInputSource>(), i);
                }
            }
            row++;
        }
    }
}

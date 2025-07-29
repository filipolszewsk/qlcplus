#include "vcmultiinputeditor.h"
#include "ui_vcmultiinputeditor.h"

#include <QTreeWidgetItem>
#include <QSpinBox>

#include "vcwidget.h"
#include "qlcinputsource.h"
#include "inputpatch.h"

VCMultiInputEditor::VCMultiInputEditor(QList<VCWidget*> widgets, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::VCMultiInputEditor)
    , m_widgets(widgets)
{
    Q_ASSERT(m_widgets.isEmpty() == false);

    m_ui->setupUi(this);

    fillTree();
}

VCMultiInputEditor::~VCMultiInputEditor()
{
    delete m_ui;
}

void VCMultiInputEditor::fillTree()
{
    if (m_widgets.isEmpty())
        return;

    QHash<quint8, QSharedPointer<QLCInputSource>> commonInputs;
    VCWidget* firstWidget = m_widgets.first();
    commonInputs = firstWidget->inputSources();

    for (int i = 1; i < m_widgets.count(); ++i)
    {
        QHash<quint8, QSharedPointer<QLCInputSource>> widgetInputs = m_widgets.at(i)->inputSources();
        QMutableHashIterator<quint8, QSharedPointer<QLCInputSource>> it(commonInputs);
        while (it.hasNext())
        {
            it.next();
            if (!widgetInputs.contains(it.key()))
                it.remove();
        }
    }

    QHashIterator<quint8, QSharedPointer<QLCInputSource>> it(commonInputs);
    while (it.hasNext())
    {
        it.next();
        QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->m_treeWidget);
        item->setText(0, QString("Input %1").arg(it.key()));
        item->setData(0, Qt::UserRole, it.key());
        updateItem(item);
    }
}

void VCMultiInputEditor::updateItem(QTreeWidgetItem* item)
{
    if (item == nullptr)
        return;

    QString inputName = item->data(0, Qt::UserRole).toString();
    if (inputName.isEmpty())
        return;

    QSpinBox* universeSpin = new QSpinBox(this);
    universeSpin->setRange(0, 1000); // Assuming a reasonable range
    m_ui->m_treeWidget->setItemWidget(item, 1, universeSpin);

    QSpinBox* channelSpin = new QSpinBox(this);
    channelSpin->setRange(0, 511);
    m_ui->m_treeWidget->setItemWidget(item, 2, channelSpin);
}

void VCMultiInputEditor::accept()
{
    for (int i = 0; i < m_ui->m_treeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* item = m_ui->m_treeWidget->topLevelItem(i);
        quint8 inputId = item->data(0, Qt::UserRole).toUInt();

        QSpinBox* universeSpin = qobject_cast<QSpinBox*>(m_ui->m_treeWidget->itemWidget(item, 1));
        QSpinBox* channelSpin = qobject_cast<QSpinBox*>(m_ui->m_treeWidget->itemWidget(item, 2));

        if (universeSpin && channelSpin)
        {
            quint32 universe = universeSpin->value();
            quint32 channel = channelSpin->value();

            for (VCWidget* widget : m_widgets)
            {
                QSharedPointer<QLCInputSource> source(new QLCInputSource(universe, channel));
                widget->setInputSource(source, inputId);
            }
        }
    }

    QDialog::accept();
}

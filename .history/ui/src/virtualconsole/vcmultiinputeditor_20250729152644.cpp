#include <QTreeWidgetItem>
#include <QSpinBox>

#include "vcmultiinputeditor.h"
#include "vcwidget.h"
#include "qlcinputsource.h"
#include "inputpatch.h"

VCMultiInputEditor::VCMultiInputEditor(QList<VCWidget*> widgets, QWidget* parent)
    : QDialog(parent)
    , m_widgets(widgets)
{
    Q_ASSERT(m_widgets.isEmpty() == false);

    setupUi(this);
    fillTree();
}

VCMultiInputEditor::~VCMultiInputEditor()
{
}

void VCMultiInputEditor::fillTree()
{
    if (m_widgets.isEmpty())
        return;

    QStringList commonInputs;
    VCWidget* firstWidget = m_widgets.first();
    commonInputs = firstWidget->inputSourceNames();

    for (int i = 1; i < m_widgets.count(); ++i)
    {
        QStringList widgetInputs = m_widgets.at(i)->inputSourceNames();
        for (const QString& commonInput : commonInputs)
        {
            if (!widgetInputs.contains(commonInput))
                commonInputs.removeOne(commonInput);
        }
    }

    for (const QString& inputName : commonInputs)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, inputName);
        item->setData(0, Qt::UserRole, inputName);
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
    m_treeWidget->setItemWidget(item, 1, universeSpin);

    QSpinBox* channelSpin = new QSpinBox(this);
    channelSpin->setRange(0, 511);
    m_treeWidget->setItemWidget(item, 2, channelSpin);
}

void VCMultiInputEditor::accept()
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        QString inputName = item->data(0, Qt::UserRole).toString();

        QSpinBox* universeSpin = qobject_cast<QSpinBox*>(m_treeWidget->itemWidget(item, 1));
        QSpinBox* channelSpin = qobject_cast<QSpinBox*>(m_treeWidget->itemWidget(item, 2));

        if (universeSpin && channelSpin)
        {
            quint32 universe = universeSpin->value();
            quint32 channel = channelSpin->value();

            for (VCWidget* widget : m_widgets)
            {
                widget->setInputSource(QLCInputSource(universe, channel), inputName);
            }
        }
    }

    QDialog::accept();
}

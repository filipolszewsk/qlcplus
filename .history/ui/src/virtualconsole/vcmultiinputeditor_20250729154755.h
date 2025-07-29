#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>
#include <QList>

class QTreeWidgetItem;
class VCWidget;
class QTreeWidget;
class QDialogButtonBox;

namespace Ui {
class VCMultiInputEditor;
}

class VCMultiInputEditor : public QDialog
{
    Q_OBJECT

public:
    VCMultiInputEditor(QList<VCWidget*> widgets, QWidget* parent = nullptr);
    ~VCMultiInputEditor();

protected slots:
    void accept();

private:
    void fillTree();
    void updateItem(QTreeWidgetItem* item);

private:
    Ui::VCMultiInputEditor* m_ui;
    QList<VCWidget*> m_widgets;
    QTreeWidget* m_treeWidget;
    QDialogButtonBox* m_buttonBox;
};

#endif // VCMULTIINPUTEDITOR_H

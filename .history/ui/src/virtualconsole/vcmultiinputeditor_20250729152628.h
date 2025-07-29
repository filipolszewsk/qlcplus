#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>
#include "ui_vcmultiinputeditor.h"

class VCWidget;
class QTreeWidget;
class QTreeWidgetItem;

class VCMultiInputEditor : public QDialog, public Ui_VCMultiInputEditor
{
    Q_OBJECT

public:
    VCMultiInputEditor(QList<VCWidget*> widgets, QWidget* parent = 0);
    ~VCMultiInputEditor();

protected:
    void accept();

private:
    void fillTree();
    void updateItem(QTreeWidgetItem* item);

private:
    QList<VCWidget*> m_widgets;
};

#endif // VCMULTIINPUTEDITOR_H

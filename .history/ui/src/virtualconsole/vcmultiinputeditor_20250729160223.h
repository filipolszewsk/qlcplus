#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>
#include <QList>
#include <QHash>
#include <QSharedPointer>

class VCWidget;
class QLCInputSource;
class QTreeWidgetItem;

namespace Ui {
class VCMultiInputEditor;
}

class VCMultiInputEditor : public QDialog
{
    Q_OBJECT

public:
    VCMultiInputEditor(QList<VCWidget*> widgets, QWidget* parent = nullptr);
    ~VCMultiInputEditor();

private slots:
    void accept();

private:
    void fillTree();
    void updateItem(QTreeWidgetItem* item);

private:
    Ui::VCMultiInputEditor* m_ui;
    QList<VCWidget*> m_widgets;
};

#endif // VCMULTIINPUTEDITOR_H

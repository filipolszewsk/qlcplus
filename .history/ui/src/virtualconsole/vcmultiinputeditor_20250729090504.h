#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>

class VCWidget;

namespace Ui {
class VCMultiInputEditor;
}

class VCMultiInputEditor : public QDialog
{
    Q_OBJECT

public:
    VCMultiInputEditor(const QList<VCWidget*>& widgets, QWidget* parent = nullptr);
    ~VCMultiInputEditor();

    void applyChanges();

private:
    void fillTable();

private:
    Ui::VCMultiInputEditor* ui;
    QList<VCWidget*> m_widgets;
};

#endif // VCMULTIINPUTEDITOR_H

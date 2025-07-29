#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>
#include "ui_vcmultiinputeditor.h"

class VCWidget;
class Doc;

class VCMultiInputEditor : public QDialog, public Ui_VCMultiInputEditor
{
    Q_OBJECT

public:
    VCMultiInputEditor(QWidget* parent, const QList<VCWidget*>& widgets, Doc* doc);
    ~VCMultiInputEditor();

private slots:
    void accept();

private:
    void init();
    void updateInputSources();

private:
    QList<VCWidget*> m_widgets;
    Doc* m_doc;
};

#endif // VCMULTIINPUTEDITOR_H

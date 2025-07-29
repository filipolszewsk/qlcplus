#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>
#include "ui_vcmultiinputeditor.h"

class VCWidget;
class QLCInputSource;

class VCMultiInputEditor : public QDialog, public Ui_VCMultiInputEditor
{
    Q_OBJECT
    Q_DISABLE_COPY(VCMultiInputEditor)

public:
    VCMultiInputEditor(QWidget* parent, QList<VCWidget*> widgets, const QLCInputSource* source);
    ~VCMultiInputEditor();

    QLCInputSource externalInput() const;

protected slots:
    void slotUniverseChanged(int idx);
    void slotChannelChanged(int idx);
    void accept();

protected:
    void updateChannels();

protected:
    QLCInputSource m_externalInput;
    QList<VCWidget*> m_widgets;
};

#endif // VCMULTIINPUTEDITOR_H

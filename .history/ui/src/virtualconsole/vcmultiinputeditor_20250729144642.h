#ifndef VCMULTIINPUTEDITOR_H
#define VCMULTIINPUTEDITOR_H

#include <QDialog>
#include "ui_vcmultiinputeditor.h"
#include "qlcinputsource.h"

class VCMultiInputEditor : public QDialog, public Ui_VCMultiInputEditor
{
    Q_OBJECT
    Q_DISABLE_COPY(VCMultiInputEditor)

public:
    VCMultiInputEditor(QWidget* parent, const QLCInputSource* source);
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
};

#endif // VCMULTIINPUTEDITOR_H

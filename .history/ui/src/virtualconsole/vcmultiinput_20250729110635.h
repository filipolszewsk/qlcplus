#ifndef VCMULTIINPUT_H
#define VCMULTIINPUT_H

#include "vcwidget.h"

class VCMultiInput : public VCWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(VCMultiInput)

public:
    VCMultiInput(QWidget* parent, Doc* doc);
    ~VCMultiInput();

    void setWidgetId(quint32 id);

public slots:
    void slotModeChanged(Doc::Mode mode);

protected:
    VCWidget* m_vcWidget;
};

#endif // VCMULTIINPUT_H

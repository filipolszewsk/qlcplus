#ifndef VCMULTIINPUT_H
#define VCMULTIINPUT_H

#include <QKeySequence>
#include "vcwidget.h"

#define KXMLQLCVCMultiInput QStringLiteral("MultiInput")

class VCMultiInput : public VCWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(VCMultiInput)

public:
    VCMultiInput(QWidget* parent, Doc* doc);
    ~VCMultiInput();

    void setWidgetId(quint32 id);
    VCWidget* getWidget();

public slots:
    void slotModeChanged(Doc::Mode mode);

protected:
    VCWidget* m_vcWidget;
};

#endif // VCMULTIINPUT_H

#ifndef VCMULTIINPUT_H
#define VCMULTIINPUT_H

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

    /*********************************************************************
     * Copying
     *********************************************************************/
public:
    /** @reimp */
    VCWidget* createCopy(VCWidget* parent);

protected:
    /** @reimp */
    bool copyFrom(const VCWidget* widget);

    /*************************************************************************
     * Load & Save
     *************************************************************************/
public:
    /** @reimp */
    bool loadXML(QXmlStreamReader& root);

    /** @reimp */
    bool saveXML(QXmlStreamWriter* doc);

public slots:
    void slotModeChanged(Doc::Mode mode);

    /*********************************************************************
     * External input
     *********************************************************************/
public:
    /** @reimp */
    QStringList inputSourceNames() const;

    /** @reimp */
    void updateFeedback();

public slots:
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

protected:
    VCWidget* m_vcWidget;
};

#endif // VCMULTIINPUT_H

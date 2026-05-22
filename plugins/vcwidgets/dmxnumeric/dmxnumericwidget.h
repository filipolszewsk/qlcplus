/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericwidget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QMutex>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>

#include <QJsonObject>
#include "vcwidget.h"
#include "dmxsource.h"
#include "genericfader.h"

class Doc;

class DMXNumericWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    static const quint8 valueInputSourceId = 0;
    static const quint8 applyInputSourceId = 1;

    DMXNumericWidget(QWidget* parent, Doc* doc);
    ~DMXNumericWidget() override;

    // ---- Configuration -----------------------------------------------
    void    setFixtureChannel(quint32 fixtureId, quint32 channel);
    quint32 fixtureId() const { return m_fixtureId; }
    quint32 channel()   const { return m_channel; }

    // ---- Clipboard ---------------------------------------------------
    VCWidget* createCopy(VCWidget* parent) override;
    void      toClipboardJson(QJsonObject &obj, const Doc *doc) const override;
    void      fromClipboardJson(const QJsonObject &obj, Doc *doc) override;

    // ---- External input / feedback -----------------------------------
    void updateFeedback() override;

    // ---- DMXSource ---------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

    // ---- Load & Save -------------------------------------------------
    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;

    // ---- Properties dialog -------------------------------------------
    void editProperties() override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

private slots:
    void slotSpinChanged(int value);
    void slotApply();

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    void updateAddressLabel();
    void updateApplyButtonStyle();

    quint32 m_fixtureId = UINT_MAX;
    quint32 m_channel   = 0;

    // Applied DMX value — shared between GUI thread and MasterTimer thread
    QMutex m_valueMutex;
    uchar  m_dmxValue = 0;   // what is actually sent to output (after Apply)

    QSharedPointer<GenericFader> m_fader;

    // UI
    QVBoxLayout* m_layout      = nullptr;
    QLabel*      m_addrLabel   = nullptr;
    QSpinBox*    m_spinBox     = nullptr;  // pending value
    QPushButton* m_applyButton = nullptr;
};

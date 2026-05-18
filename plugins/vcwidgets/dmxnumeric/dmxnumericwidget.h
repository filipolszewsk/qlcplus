/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericwidget.h — Apache 2.0 / public domain

  Widget that lets the user type or scroll a DMX value (0–255)
  directly to a configured universe + channel.
*/

#pragma once

#include <QMutex>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>

#include "vcwidget.h"
#include "dmxsource.h"

class Doc;

/**
 * DMXNumericWidget — enter a DMX value (0-255) numerically for one channel.
 *
 * Implements DMXSource so it registers on MasterTimer and writes
 * the configured value to universe/channel on every tick while
 * in Operate mode.
 */
class DMXNumericWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    DMXNumericWidget(QWidget* parent, Doc* doc);
    ~DMXNumericWidget() override;

    // ---- Configuration -----------------------------------------------
    void setUniverse(int universe);   ///< 0-based universe index
    int  universe() const;

    void setAddress(int address);     ///< 0-based DMX address (0–511)
    int  address() const;

    // ---- Clipboard ---------------------------------------------------
    VCWidget* createCopy(VCWidget* parent) override;

    // ---- External input ----------------------------------------------
    void updateFeedback() override {}

    // ---- DMXSource ---------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

    // ---- Load & Save -------------------------------------------------
    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;

    // ---- Properties dialog -------------------------------------------
    void editProperties() override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;

private slots:
    void slotValueChanged(int value);

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    void updateAddressLabel();
    // Configuration (GUI thread only for reads from UI, also read in timer thread)
    int  m_universe = 0;
    int  m_address  = 0;

    // DMX value — written from GUI thread, read from timer thread
    QMutex  m_valueMutex;
    uchar   m_dmxValue      = 0;
    bool    m_valueChanged  = false;

    // UI elements embedded in the widget
    QVBoxLayout* m_layout   = nullptr;
    QLabel*      m_addrLabel = nullptr;
    QSpinBox*    m_spinBox   = nullptr;
};

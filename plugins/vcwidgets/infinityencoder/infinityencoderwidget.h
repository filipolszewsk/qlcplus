/*
  QLC+ VC Widget Plugin — Infinity Encoder
  infinityencoderwidget.h — Apache 2.0 / public domain
*/

#pragma once

#include <QMutex>
#include <QHash>
#include <QLabel>
#include <QRect>
#include <QPoint>
#include <QPushButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSharedPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <cmath>

#include "vcwidget.h"
#include "dmxsource.h"
#include "genericfader.h"

class Doc;

class InfinityEncoderWidget : public VCWidget, public DMXSource
{
    Q_OBJECT

public:
    static constexpr int NUM_BANKS = 4;

    // Input source IDs
    static const quint8 encoderInputSourceId = 0;
    static const quint8 bankAInputSourceId   = 1;
    static const quint8 bankBInputSourceId   = 2;
    static const quint8 bankCInputSourceId   = 3;
    static const quint8 bankDInputSourceId   = 4;

    enum Sensitivity { Fine = 0, Normal = 1, Coarse = 2 };

    struct Slot {
        quint32 fixtureId = UINT_MAX;
        quint32 channel   = 0;
        QString label;   // user label; falls back to channel name from fixture
    };

    explicit InfinityEncoderWidget(QWidget* parent, Doc* doc);
    ~InfinityEncoderWidget() override;

    // ---- Slot / bank configuration ----------------------------------------
    void        setSlot(int bank, const Slot& s);
    Slot        slot(int bank) const;
    void        setActiveBank(int bank);
    int         activeBank() const { return m_activeBank; }
    void        setSensitivity(Sensitivity s);
    Sensitivity sensitivity() const { return m_sensitivity; }

    // ---- VCWidget overrides ------------------------------------------------
    VCWidget* createCopy(VCWidget* parent) override;
    void      updateFeedback() override;
    bool      loadXML(QXmlStreamReader& root) override;
    bool      saveXML(QXmlStreamWriter* doc) override;
    void      editProperties() override;

    // ---- DMXSource ---------------------------------------------------------
    void writeDMX(MasterTimer* timer, QList<Universe*> universes) override;

protected slots:
    void slotModeChanged(Doc::Mode mode) override;
    void slotInputValueChanged(quint32 universe, quint32 channel, uchar value) override;

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private slots:
    void slotBankButtonClicked(int bank);
    void slotSensButtonClicked(int sens);

private:
    void applyDelta(int rawDelta);
    int  sensitivityMultiplier() const;
    void updateInfoLabel();
    void updateBankButtonStyles();
    void updateSensButtonStyles();
    void recalcKnobRect();

    static int decodeSignedBit(uchar value);

    // ---- Shared state (GUI thread + MasterTimer thread, mutex protected) ---
    mutable QMutex m_valueMutex;
    uchar   m_value[NUM_BANKS] = {0, 0, 0, 0};
    Slot    m_slots[NUM_BANKS];

    // ---- GUI-thread-only state ---------------------------------------------
    int          m_activeBank  = 0;
    Sensitivity  m_sensitivity = Normal;
    double       m_knobAngleDeg = 0.0;   // continuous angle for visual rotation
    QPoint       m_lastMousePos;
    bool         m_dragging = false;
    double       m_accumulatedDelta = 0.0;   // sub-tick accumulator for smooth circular drag

    // ---- UI widgets --------------------------------------------------------
    QVBoxLayout*  m_layout    = nullptr;
    QLabel*       m_infoLabel = nullptr;
    // Knob area spacer (the actual knob is drawn in paintEvent using m_knobRect)
    QWidget*      m_knobArea  = nullptr;
    QHBoxLayout*  m_bankRow   = nullptr;
    QHBoxLayout*  m_sensRow   = nullptr;
    QPushButton*  m_bankBtn[NUM_BANKS] = {nullptr};
    QPushButton*  m_sensBtn[3]         = {nullptr};
    QButtonGroup* m_bankGroup = nullptr;
    QButtonGroup* m_sensGroup = nullptr;
    QRect         m_knobRect;

    // ---- DMX output --------------------------------------------------------
    QHash<quint32, QSharedPointer<GenericFader>> m_faders;   // per universe, lazy

    static const QColor s_bankTint[NUM_BANKS];
};

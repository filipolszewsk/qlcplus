/*
  QLC+ VC Widget Plugin Example
  hellowidget.h

  This file is part of the HelloVCWidget example plugin and is
  placed in the public domain (CC0 1.0 Universal).
  Copy, modify, and redistribute freely.
*/

#pragma once

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>
#include <QTimer>

#include "vcwidget.h"

/**
 * HelloWidget — a minimal VC widget plugin example.
 *
 * What it demonstrates:
 *  - Inheriting VCWidget correctly.
 *  - Implementing loadXML / saveXML.
 *  - Handling Design vs. Operate mode via slotModeChanged.
 *  - Drawing a custom paintEvent.
 *  - Connecting to MasterTimer beat signal for animation.
 */
class HelloWidget : public VCWidget
{
    Q_OBJECT

public:
    HelloWidget(QWidget* parent, Doc* doc);
    ~HelloWidget() override;

    /*********************************************************************
     * Clipboard
     *********************************************************************/
    VCWidget* createCopy(VCWidget* parent) override;

    /*********************************************************************
     * External input
     *********************************************************************/
    void updateFeedback() override {}

    /*********************************************************************
     * Load & Save
     *********************************************************************/
    bool loadXML(QXmlStreamReader& root) override;
    bool saveXML(QXmlStreamWriter* doc) override;

    /*********************************************************************
     * Mode
     *********************************************************************/
protected slots:
    void slotModeChanged(Doc::Mode mode) override;

private slots:
    void slotBeat();

    /*********************************************************************
     * Painting
     *********************************************************************/
protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    int  m_beatCount  = 0;
    bool m_flashState = false;
};

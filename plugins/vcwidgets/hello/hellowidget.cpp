/*
  QLC+ VC Widget Plugin Example
  hellowidget.cpp — public domain (CC0 1.0)
*/

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QDebug>

#include "hellowidget.h"
#include "mastertimer.h"
#include "doc.h"

static const QString KXMLHelloWidget       = QStringLiteral("PluginWidget");
static const QString KXMLHelloWidgetId     = QStringLiteral("PluginId");
static const QString KXMLHelloPluginId     = QStringLiteral("org.qlcplus.example.hellowidget");
static const QString KXMLHelloBeatCount    = QStringLiteral("BeatCount");

HelloWidget::HelloWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(HelloWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Hello Widget"));
    resize(QSize(180, 100));
}

HelloWidget::~HelloWidget()
{
    // Disconnect beat signal if we connected it
    if (m_doc && m_doc->masterTimer())
        disconnect(m_doc->masterTimer(), nullptr, this, nullptr);
}

/*****************************************************************************
 * Clipboard
 *****************************************************************************/

VCWidget* HelloWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    HelloWidget* copy = new HelloWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }
    copy->m_beatCount = m_beatCount;
    return copy;
}

/*****************************************************************************
 * Mode
 *****************************************************************************/

void HelloWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        // Connect to beat signal for animation in Operate mode
        connect(m_doc->masterTimer(), &MasterTimer::beat,
                this, &HelloWidget::slotBeat);
    }
    else
    {
        disconnect(m_doc->masterTimer(), &MasterTimer::beat,
                   this, &HelloWidget::slotBeat);
    }
    VCWidget::slotModeChanged(mode);
    update();
}

void HelloWidget::slotBeat()
{
    m_beatCount++;
    m_flashState = !m_flashState;

    // Request repaint from the GUI thread via queued connection.
    // slotBeat is connected with Qt::AutoConnection and MasterTimer::beat
    // is emitted from the timer thread — Qt routes this as a queued call
    // to the GUI thread, so update() is safe here.
    update();
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

bool HelloWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLHelloWidget)
    {
        qWarning() << Q_FUNC_INFO << "PluginWidget node not found";
        return false;
    }

    loadXMLCommon(root);

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCWindowState)
        {
            int x = 0, y = 0, w = 0, h = 0;
            bool visible = false;
            loadXMLWindowState(root, &x, &y, &w, &h, &visible);
            setGeometry(x, y, w, h);
        }
        else if (root.name() == KXMLQLCVCWidgetAppearance)
        {
            loadXMLAppearance(root);
        }
        else if (root.name() == KXMLHelloBeatCount)
        {
            m_beatCount = root.readElementText().toInt();
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    return true;
}

bool HelloWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    doc->writeStartElement(KXMLHelloWidget);
    doc->writeAttribute(KXMLHelloWidgetId, KXMLHelloPluginId);

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    doc->writeTextElement(KXMLHelloBeatCount, QString::number(m_beatCount));

    doc->writeEndElement();
    return true;
}

/*****************************************************************************
 * Painting
 *****************************************************************************/

void HelloWidget::mousePressEvent(QMouseEvent* e)
{
    if (mode() == Doc::Operate)
    {
        m_flashState = !m_flashState;
        update();
    }
    VCWidget::mousePressEvent(e);
}

void HelloWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    // Background
    QColor bg = m_flashState ? QColor(60, 140, 200) : QColor(30, 60, 100);
    painter.fillRect(rect(), bg);

    // Text
    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(10);
    painter.setFont(f);

    QString msg;
    if (mode() == Doc::Operate)
        msg = tr("Hello!\nBeats: %1").arg(m_beatCount);
    else
        msg = tr("Hello Widget\n(click to configure)");

    painter.drawText(rect().adjusted(6, 6, -6, -6),
                     Qt::AlignCenter | Qt::TextWordWrap, msg);

    painter.end();
    VCWidget::paintEvent(e);
}

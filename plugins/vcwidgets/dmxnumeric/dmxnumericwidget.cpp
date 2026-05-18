/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericwidget.cpp — Apache 2.0 / public domain
*/

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QInputDialog>
#include <QPaintEvent>
#include <QMessageBox>
#include <QPainter>
#include <QMutexLocker>
#include <QDebug>

#include "dmxnumericwidget.h"
#include "inputoutputmap.h"
#include "mastertimer.h"
#include "universe.h"
#include "doc.h"

static const QString KXMLDMXNumeric    = QStringLiteral("PluginWidget");
static const QString KXMLDMXPluginId   = QStringLiteral("PluginId");
static const QString KXMLDMXPluginIdVal = QStringLiteral("org.qlcplus.dmxnumeric");
static const QString KXMLDMXUniverse   = QStringLiteral("Universe");
static const QString KXMLDMXAddress    = QStringLiteral("Address");
static const QString KXMLDMXValue      = QStringLiteral("Value");

DMXNumericWidget::DMXNumericWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(DMXNumericWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("DMX Numeric"));
    resize(QSize(120, 80));

    // Build embedded UI
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);

    m_addrLabel = new QLabel(tr("U1 / CH1"), this);
    m_addrLabel->setAlignment(Qt::AlignCenter);
    QFont f = m_addrLabel->font();
    f.setPointSize(8);
    m_addrLabel->setFont(f);
    m_layout->addWidget(m_addrLabel);

    m_spinBox = new QSpinBox(this);
    m_spinBox->setRange(0, 255);
    m_spinBox->setValue(0);
    m_spinBox->setAlignment(Qt::AlignCenter);
    m_spinBox->setEnabled(false);   // enabled only in Operate mode
    m_layout->addWidget(m_spinBox);

    setLayout(m_layout);

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DMXNumericWidget::slotValueChanged);

    updateAddressLabel();
}

DMXNumericWidget::~DMXNumericWidget()
{
    if (m_doc && m_doc->masterTimer())
        m_doc->masterTimer()->unregisterDMXSource(this);
}

// ---- Configuration -----------------------------------------------------------

void DMXNumericWidget::setUniverse(int universe)
{
    m_universe = universe;
    updateAddressLabel();
}

int DMXNumericWidget::universe() const
{
    return m_universe;
}

void DMXNumericWidget::setAddress(int address)
{
    m_address = address;
    updateAddressLabel();
}

int DMXNumericWidget::address() const
{
    return m_address;
}

// ---- Clipboard ---------------------------------------------------------------

VCWidget* DMXNumericWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    DMXNumericWidget* copy = new DMXNumericWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }
    copy->setUniverse(m_universe);
    copy->setAddress(m_address);
    {
        QMutexLocker lk(&m_valueMutex);
        copy->m_dmxValue = m_dmxValue;
    }
    copy->m_spinBox->setValue(m_spinBox->value());
    return copy;
}

// ---- Mode --------------------------------------------------------------------

void DMXNumericWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        m_spinBox->setEnabled(true);
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else
    {
        m_spinBox->setEnabled(false);
        m_doc->masterTimer()->unregisterDMXSource(this);
        // Release fader so the universe stops outputting our value
        m_fader.clear();
    }
    VCWidget::slotModeChanged(mode);
    update();
}

// ---- DMXSource ---------------------------------------------------------------

void DMXNumericWidget::slotValueChanged(int value)
{
    // Called from GUI thread. Update shared value and set dirty flag.
    QMutexLocker lk(&m_valueMutex);
    m_dmxValue     = static_cast<uchar>(value);
    m_valueChanged = true;
}

void DMXNumericWidget::writeDMX(MasterTimer* /*timer*/,
                                 QList<Universe*> universes)
{
    // Called from MasterTimer thread — do NOT touch any QWidget here.
    QMutexLocker lk(&m_valueMutex);

    if (!m_valueChanged)
        return;

    if (m_universe < 0 || m_universe >= universes.size())
        return;

    if (m_address < 0 || m_address >= UNIVERSE_SIZE)
        return;

    // Write directly to the universe at the raw address.
    // forceLTP = true so the value overrides HTP precedence.
    universes[m_universe]->write(m_address, m_dmxValue, true);

    m_valueChanged = false;
}

// ---- Properties --------------------------------------------------------------

void DMXNumericWidget::editProperties()
{
    if (mode() != Doc::Design)
        return;

    // Simple two-step dialog: universe then address
    bool ok = false;

    int u = QInputDialog::getInt(this,
        tr("DMX Numeric — Universe"),
        tr("Universe (1–%1):").arg(m_doc->inputOutputMap()->universesCount()),
        m_universe + 1, 1,
        m_doc->inputOutputMap()->universesCount(), 1, &ok);
    if (!ok) return;

    int ch = QInputDialog::getInt(this,
        tr("DMX Numeric — Channel"),
        tr("DMX channel (1–512):"),
        m_address + 1, 1, 512, 1, &ok);
    if (!ok) return;

    setUniverse(u - 1);
    setAddress(ch - 1);
    m_doc->setModified();
}

// ---- Load & Save -------------------------------------------------------------

bool DMXNumericWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLDMXNumeric)
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
        else if (root.name() == KXMLDMXUniverse)
        {
            setUniverse(root.readElementText().toInt());
        }
        else if (root.name() == KXMLDMXAddress)
        {
            setAddress(root.readElementText().toInt());
        }
        else if (root.name() == KXMLDMXValue)
        {
            const int v = root.readElementText().toInt();
            m_spinBox->blockSignals(true);
            m_spinBox->setValue(v);
            m_spinBox->blockSignals(false);
            QMutexLocker lk(&m_valueMutex);
            m_dmxValue = static_cast<uchar>(v);
            m_valueChanged = false;
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    return true;
}

bool DMXNumericWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    doc->writeStartElement(KXMLDMXNumeric);
    doc->writeAttribute(KXMLDMXPluginId, KXMLDMXPluginIdVal);

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    doc->writeTextElement(KXMLDMXUniverse, QString::number(m_universe));
    doc->writeTextElement(KXMLDMXAddress,  QString::number(m_address));
    doc->writeTextElement(KXMLDMXValue,    QString::number(m_spinBox->value()));

    doc->writeEndElement();
    return true;
}

// ---- Painting ----------------------------------------------------------------

void DMXNumericWidget::updateAddressLabel()
{
    if (m_addrLabel)
        m_addrLabel->setText(
            tr("U%1 / CH%2").arg(m_universe + 1).arg(m_address + 1));
}

void DMXNumericWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    QColor bg = (mode() == Doc::Operate)
                ? QColor(20, 40, 20)
                : QColor(30, 30, 30);
    painter.fillRect(rect(), bg);

    painter.end();
    VCWidget::paintEvent(e);
}

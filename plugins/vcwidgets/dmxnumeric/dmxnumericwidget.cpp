/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericwidget.cpp — Apache 2.0 / public domain
*/

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QPaintEvent>
#include <QPainter>
#include <QMutexLocker>
#include <QJsonObject>
#include <QDebug>

#include "dmxnumericconfigdialog.h"
#include "dmxnumericwidget.h"

#include "genericfader.h"
#include "fadechannel.h"
#include "inputoutputmap.h"
#include "mastertimer.h"
#include "universe.h"
#include "fixture.h"
#include "qlcinputsource.h"
#include "doc.h"

static const QString KXMLDMXNumeric     = QStringLiteral("PluginWidget");
static const QString KXMLDMXPluginId    = QStringLiteral("PluginId");
static const QString KXMLDMXPluginIdVal = QStringLiteral("org.qlcplus.dmxnumeric");
static const QString KXMLDMXFixtureID   = QStringLiteral("FixtureID");
static const QString KXMLDMXChannel     = QStringLiteral("Channel");
static const QString KXMLDMXValue       = QStringLiteral("Value");
static const QString KXMLDMXValueInput  = QStringLiteral("ValueInput");
static const QString KXMLDMXApplyInput  = QStringLiteral("ApplyInput");
// Legacy
static const QString KXMLDMXUniverse    = QStringLiteral("Universe");
static const QString KXMLDMXAddress     = QStringLiteral("Address");

// ---- Construction ------------------------------------------------------------

DMXNumericWidget::DMXNumericWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(DMXNumericWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("DMX Numeric"));
    resize(QSize(150, 100));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(3);

    m_addrLabel = new QLabel(tr("No channel"), this);
    m_addrLabel->setAlignment(Qt::AlignCenter);
    QFont f = m_addrLabel->font();
    f.setPointSize(8);
    m_addrLabel->setFont(f);
    m_layout->addWidget(m_addrLabel);

    m_spinBox = new QSpinBox(this);
    m_spinBox->setRange(0, 255);
    m_spinBox->setValue(0);
    m_spinBox->setAlignment(Qt::AlignCenter);
    m_spinBox->setEnabled(false);
    m_layout->addWidget(m_spinBox);

    m_applyButton = new QPushButton(tr("Apply"), this);
    m_applyButton->setEnabled(false);
    m_layout->addWidget(m_applyButton);

    setLayout(m_layout);

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DMXNumericWidget::slotSpinChanged);
    connect(m_applyButton, &QPushButton::clicked,
            this, &DMXNumericWidget::slotApply);

    updateAddressLabel();
}

DMXNumericWidget::~DMXNumericWidget()
{
    if (m_doc && m_doc->masterTimer())
        m_doc->masterTimer()->unregisterDMXSource(this);
    if (!m_fader.isNull())
        m_fader->requestDelete();
}

// ---- Configuration -----------------------------------------------------------

void DMXNumericWidget::setFixtureChannel(quint32 fixtureId, quint32 channel)
{
    m_fixtureId = fixtureId;
    m_channel   = channel;
    updateAddressLabel();
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
    copy->setFixtureChannel(m_fixtureId, m_channel);
    copy->setInputSource(inputSource(valueInputSourceId), valueInputSourceId);
    copy->setInputSource(inputSource(applyInputSourceId), applyInputSourceId);
    {
        QMutexLocker lk(&m_valueMutex);
        copy->m_dmxValue = m_dmxValue;
    }
    copy->m_spinBox->setValue(m_spinBox->value());
    return copy;
}

void DMXNumericWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    Fixture *fxi = doc->fixture(m_fixtureId);
    obj["fixtureName"] = fxi ? fxi->name() : QString();
    obj["channel"]     = (int)m_channel;
}

void DMXNumericWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    const QString fxName = obj["fixtureName"].toString();
    quint32 fxId = UINT_MAX;
    if (!fxName.isEmpty())
    {
        for (Fixture *fxi : doc->fixtures())
        {
            if (fxi && fxi->name() == fxName)
            {
                fxId = fxi->id();
                break;
            }
        }
    }
    setFixtureChannel(fxId, (quint32)obj["channel"].toInt(0));
    updateAddressLabel();
}

// ---- Mode --------------------------------------------------------------------

void DMXNumericWidget::slotModeChanged(Doc::Mode mode)
{
    if (mode == Doc::Operate)
    {
        m_spinBox->setEnabled(true);
        m_applyButton->setEnabled(true);
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else
    {
        m_spinBox->setEnabled(false);
        m_applyButton->setEnabled(false);
        m_doc->masterTimer()->unregisterDMXSource(this);
        if (!m_fader.isNull())
        {
            m_fader->requestDelete();
            m_fader.clear();
        }
    }
    VCWidget::slotModeChanged(mode);
    update();
}

// ---- UI slots ----------------------------------------------------------------

void DMXNumericWidget::slotSpinChanged(int value)
{
    // Spinbox changed — just update button highlight to show pending state.
    Q_UNUSED(value);
    updateApplyButtonStyle();
}

void DMXNumericWidget::slotApply()
{
    // Push pending spinbox value to the applied DMX value (read by timer thread).
    {
        QMutexLocker lk(&m_valueMutex);
        m_dmxValue = static_cast<uchar>(m_spinBox->value());
    }
    // Send feedback to any motorised/LED controller bound to Value input.
    sendFeedback(m_spinBox->value(), valueInputSourceId);
    updateApplyButtonStyle();
}

// ---- External input ----------------------------------------------------------

void DMXNumericWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (acceptsInput() == false)
        return;

    quint32 pagedCh = (page() << 16) | channel;

    if (checkInputSource(universe, pagedCh, value, sender(), valueInputSourceId))
    {
        // External device controls the pending value (spinbox) — does not apply yet.
        m_spinBox->setValue(int(value));
    }
    else if (checkInputSource(universe, pagedCh, value, sender(), applyInputSourceId))
    {
        // Any non-zero value on the apply input triggers Apply.
        if (value > 0)
            slotApply();
    }
}

void DMXNumericWidget::updateFeedback()
{
    // Called by QLC+ when the widget becomes visible / page changes.
    // Send current applied value back to motorised faders / LED indicators.
    QMutexLocker lk(&m_valueMutex);
    sendFeedback(int(m_dmxValue), valueInputSourceId);
}

// ---- DMXSource ---------------------------------------------------------------

void DMXNumericWidget::writeDMX(MasterTimer* /*timer*/, QList<Universe*> universes)
{
    QMutexLocker lk(&m_valueMutex);

    if (m_fixtureId == UINT_MAX)
        return;

    Fixture* fxi = m_doc->fixture(m_fixtureId);
    if (fxi == nullptr || m_channel >= fxi->channels())
        return;

    quint32 uni = fxi->universe();
    if ((int)uni >= universes.size())
        return;

    if (m_fader.isNull())
        m_fader = universes[uni]->requestFader(Universe::Auto);

    FadeChannel* fc = m_fader->getChannelFader(m_doc, universes[uni],
                                                m_fixtureId, m_channel);
    if (fc->universe() == Universe::invalid())
    {
        m_fader->remove(fc);
        return;
    }

    fc->setStart(fc->current());
    fc->setTarget(m_dmxValue);
    fc->setReady(false);
    fc->setElapsed(0);
}

// ---- Properties --------------------------------------------------------------

void DMXNumericWidget::editProperties()
{
    if (mode() != Doc::Design)
        return;

    DMXNumericConfigDialog dlg(m_doc,
                               m_fixtureId, m_channel,
                               inputSource(valueInputSourceId),
                               inputSource(applyInputSourceId),
                               page(),
                               this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    setFixtureChannel(dlg.fixtureId(), dlg.channel());
    setInputSource(dlg.valueInputSource(), valueInputSourceId);
    setInputSource(dlg.applyInputSource(), applyInputSourceId);
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

    int legacyUniverse = -1;
    int legacyAddress  = -1;

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
        else if (root.name() == KXMLDMXFixtureID)
        {
            m_fixtureId = root.readElementText().toUInt();
        }
        else if (root.name() == KXMLDMXChannel)
        {
            m_channel = root.readElementText().toUInt();
        }
        else if (root.name() == KXMLDMXValue)
        {
            const int v = root.readElementText().toInt();
            m_spinBox->blockSignals(true);
            m_spinBox->setValue(v);
            m_spinBox->blockSignals(false);
            QMutexLocker lk(&m_valueMutex);
            m_dmxValue = static_cast<uchar>(v);
        }
        else if (root.name() == KXMLDMXValueInput)
        {
            loadXMLSources(root, valueInputSourceId);
        }
        else if (root.name() == KXMLDMXApplyInput)
        {
            loadXMLSources(root, applyInputSourceId);
        }
        // Legacy
        else if (root.name() == KXMLDMXUniverse)
        {
            legacyUniverse = root.readElementText().toInt();
        }
        else if (root.name() == KXMLDMXAddress)
        {
            legacyAddress = root.readElementText().toInt();
        }
        else
        {
            root.skipCurrentElement();
        }
    }

    // Migrate legacy universe/address
    if (m_fixtureId == UINT_MAX && legacyUniverse >= 0 && legacyAddress >= 0)
    {
        for (Fixture* fx : m_doc->fixtures())
        {
            if ((int)fx->universe() == legacyUniverse
                && legacyAddress >= (int)fx->address()
                && legacyAddress < (int)(fx->address() + fx->channels()))
            {
                m_fixtureId = fx->id();
                m_channel   = quint32(legacyAddress - (int)fx->address());
                break;
            }
        }
    }

    updateAddressLabel();
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

    doc->writeTextElement(KXMLDMXFixtureID, QString::number(m_fixtureId));
    doc->writeTextElement(KXMLDMXChannel,   QString::number(m_channel));
    doc->writeTextElement(KXMLDMXValue,     QString::number(m_spinBox->value()));

    // Value input binding
    auto valueSrc = inputSource(valueInputSourceId);
    if (!valueSrc.isNull() && valueSrc->isValid())
    {
        doc->writeStartElement(KXMLDMXValueInput);
        saveXMLInput(doc, valueSrc);
        doc->writeEndElement();
    }

    // Apply input binding
    auto applySrc = inputSource(applyInputSourceId);
    if (!applySrc.isNull() && applySrc->isValid())
    {
        doc->writeStartElement(KXMLDMXApplyInput);
        saveXMLInput(doc, applySrc);
        doc->writeEndElement();
    }

    doc->writeEndElement();
    return true;
}

// ---- Painting ----------------------------------------------------------------

void DMXNumericWidget::updateAddressLabel()
{
    if (!m_addrLabel)
        return;

    if (m_fixtureId == UINT_MAX)
    {
        m_addrLabel->setText(tr("No channel"));
        return;
    }

    Fixture* fx = m_doc ? m_doc->fixture(m_fixtureId) : nullptr;
    if (fx == nullptr)
    {
        m_addrLabel->setText(tr("Missing fixture"));
        return;
    }

    const QLCChannel* ch = fx->channel(m_channel);
    QString chName = ch ? ch->name() : tr("ch%1").arg(m_channel + 1);
    m_addrLabel->setText(
        QString("U%1 | CH%2\n%3")
            .arg(fx->universe() + 1)
            .arg(fx->address() + m_channel + 1)
            .arg(chName));
}

void DMXNumericWidget::updateApplyButtonStyle()
{
    if (!m_applyButton || !m_spinBox)
        return;

    QMutexLocker lk(&m_valueMutex);
    bool pending = (static_cast<uchar>(m_spinBox->value()) != m_dmxValue);
    // Highlight when spinbox differs from what's actually on output
    m_applyButton->setStyleSheet(pending
        ? QStringLiteral("QPushButton { background-color: #cc6600; color: white; font-weight: bold; }")
        : QString());
}

void DMXNumericWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    QColor bg = (mode() == Doc::Operate) ? QColor(20, 40, 20) : QColor(30, 30, 30);
    painter.fillRect(rect(), bg);
    painter.end();
    VCWidget::paintEvent(e);
}

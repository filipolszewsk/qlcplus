/*
  QLC+ VC Widget Plugin — Infinity Encoder
  infinityencoderwidget.cpp — Apache 2.0 / public domain
*/

#include "infinityencoderwidget.h"
#include "infinityencoderconfigdialog.h"

#include "genericfader.h"
#include "fadechannel.h"
#include "mastertimer.h"
#include "universe.h"
#include "fixture.h"
#include "qlcchannel.h"
#include "qlcinputsource.h"
#include "doc.h"

#include <QPainter>
#include <QMutexLocker>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QtMath>

// ---- Static data -----------------------------------------------------------

const QColor InfinityEncoderWidget::s_bankTint[NUM_BANKS] = {
    QColor("#3a86ff"),   // A — blue
    QColor("#ff006e"),   // B — pink
    QColor("#ffbe0b"),   // C — amber
    QColor("#06ffa5"),   // D — mint
};

// ---- XML tag constants -----------------------------------------------------

static const QString KXMLRoot         = QStringLiteral("PluginWidget");
static const QString KXMLPluginId     = QStringLiteral("PluginId");
static const QString KXMLPluginIdVal  = QStringLiteral("org.qlcplus.vcwidgets.infinityencoder");
static const QString KXMLActiveBank   = QStringLiteral("ActiveBank");
static const QString KXMLSensitivity  = QStringLiteral("Sensitivity");
static const QString KXMLSlot         = QStringLiteral("Slot");
static const QString KXMLSlotBank     = QStringLiteral("Bank");
static const QString KXMLSlotFxId     = QStringLiteral("FixtureID");
static const QString KXMLSlotCh       = QStringLiteral("Channel");
static const QString KXMLSlotValue    = QStringLiteral("Value");
static const QString KXMLSlotLabel    = QStringLiteral("Label");
static const QString KXMLEncoderInput = QStringLiteral("EncoderInput");
static const QString KXMLBankInput[4] = {
    QStringLiteral("BankAInput"),
    QStringLiteral("BankBInput"),
    QStringLiteral("BankCInput"),
    QStringLiteral("BankDInput"),
};

// ---- Construction ----------------------------------------------------------

InfinityEncoderWidget::InfinityEncoderWidget(QWidget* parent, Doc* doc)
    : VCWidget(parent, doc)
{
    setObjectName(InfinityEncoderWidget::staticMetaObject.className());
    setType(VCWidget::UnknownWidget);
    setCaption(tr("Infinity Encoder"));
    resize(QSize(200, 240));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(3);

    // Info label at top
    m_infoLabel = new QLabel(tr("No channel"), this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    QFont labelFont = m_infoLabel->font();
    labelFont.setPointSize(7);
    labelFont.setItalic(true);
    m_infoLabel->setFont(labelFont);
    m_layout->addWidget(m_infoLabel);

    // Knob area — the actual knob is painted in paintEvent over m_knobRect
    m_knobArea = new QWidget(this);
    m_knobArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_knobArea->setMinimumSize(100, 100);
    // Transparent so parent's paintEvent covers it
    m_knobArea->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_layout->addWidget(m_knobArea, 1);

    // Bank buttons: A B C D
    QWidget* bankWidget = new QWidget(this);
    m_bankRow = new QHBoxLayout(bankWidget);
    m_bankRow->setContentsMargins(0, 0, 0, 0);
    m_bankRow->setSpacing(2);
    m_bankGroup = new QButtonGroup(this);
    m_bankGroup->setExclusive(true);

    static const char* bankLabels[NUM_BANKS] = {"A", "B", "C", "D"};
    for (int i = 0; i < NUM_BANKS; ++i)
    {
        m_bankBtn[i] = new QPushButton(QLatin1String(bankLabels[i]), bankWidget);
        m_bankBtn[i]->setCheckable(true);
        m_bankBtn[i]->setMinimumHeight(24);
        m_bankBtn[i]->setEnabled(false);
        m_bankGroup->addButton(m_bankBtn[i], i);
        m_bankRow->addWidget(m_bankBtn[i]);
    }
    m_bankBtn[0]->setChecked(true);
    m_layout->addWidget(bankWidget);

    // Sensitivity buttons: Fine Normal Coarse
    QWidget* sensWidget = new QWidget(this);
    m_sensRow = new QHBoxLayout(sensWidget);
    m_sensRow->setContentsMargins(0, 0, 0, 0);
    m_sensRow->setSpacing(2);
    m_sensGroup = new QButtonGroup(this);
    m_sensGroup->setExclusive(true);

    static const char* sensLabels[3] = {"Fine", "Normal", "Coarse"};
    for (int i = 0; i < 3; ++i)
    {
        m_sensBtn[i] = new QPushButton(QLatin1String(sensLabels[i]), sensWidget);
        m_sensBtn[i]->setCheckable(true);
        m_sensBtn[i]->setMinimumHeight(20);
        m_sensBtn[i]->setEnabled(false);
        m_sensGroup->addButton(m_sensBtn[i], i);
        m_sensRow->addWidget(m_sensBtn[i]);
    }
    m_sensBtn[Normal]->setChecked(true);
    m_layout->addWidget(sensWidget);

    setLayout(m_layout);

    connect(m_bankGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &InfinityEncoderWidget::slotBankButtonClicked);
    connect(m_sensGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &InfinityEncoderWidget::slotSensButtonClicked);

    updateBankButtonStyles();
    updateSensButtonStyles();
    updateInfoLabel();
}

InfinityEncoderWidget::~InfinityEncoderWidget()
{
    if (m_doc && m_doc->masterTimer())
        m_doc->masterTimer()->unregisterDMXSource(this);
    for (auto& fader : m_faders)
    {
        if (!fader.isNull())
            fader->requestDelete();
    }
    m_faders.clear();
}

// ---- Slot configuration ----------------------------------------------------

void InfinityEncoderWidget::setSlot(int bank, const Slot& s)
{
    if (bank < 0 || bank >= NUM_BANKS) return;
    m_slots[bank] = s;
    if (bank == m_activeBank)
        updateInfoLabel();
}

InfinityEncoderWidget::Slot InfinityEncoderWidget::slot(int bank) const
{
    if (bank < 0 || bank >= NUM_BANKS) return Slot{};
    return m_slots[bank];
}

void InfinityEncoderWidget::setActiveBank(int bank)
{
    if (bank < 0 || bank >= NUM_BANKS) return;
    m_activeBank = bank;
    m_bankBtn[bank]->setChecked(true);
    // Sync knob angle with new bank's current value
    {
        QMutexLocker lk(&m_valueMutex);
        m_knobAngleDeg = (m_value[bank] / 255.0) * 270.0 - 135.0;
    }
    updateBankButtonStyles();
    updateInfoLabel();
    update();
}

void InfinityEncoderWidget::setSensitivity(Sensitivity s)
{
    m_sensitivity = s;
    m_sensBtn[int(s)]->setChecked(true);
    updateSensButtonStyles();
}

// ---- Mode ------------------------------------------------------------------

void InfinityEncoderWidget::slotModeChanged(Doc::Mode mode)
{
    bool operate = (mode == Doc::Operate);

    for (int i = 0; i < NUM_BANKS; ++i)
        m_bankBtn[i]->setEnabled(operate);
    for (int i = 0; i < 3; ++i)
        m_sensBtn[i]->setEnabled(operate);

    if (operate)
    {
        m_doc->masterTimer()->registerDMXSource(this);
    }
    else
    {
        m_doc->masterTimer()->unregisterDMXSource(this);
        for (auto& fader : m_faders)
        {
            if (!fader.isNull())
                fader->requestDelete();
        }
        m_faders.clear();
    }

    VCWidget::slotModeChanged(mode);
    update();
}

// ---- Bank / sensitivity slot handlers --------------------------------------

void InfinityEncoderWidget::slotBankButtonClicked(int bank)
{
    setActiveBank(bank);
}

void InfinityEncoderWidget::slotSensButtonClicked(int sens)
{
    setSensitivity(static_cast<Sensitivity>(sens));
}

// ---- Mouse / wheel interaction ---------------------------------------------

void InfinityEncoderWidget::mousePressEvent(QMouseEvent* e)
{
    if (mode() == Doc::Operate && e->button() == Qt::LeftButton
        && m_knobRect.contains(e->pos()))
    {
        m_dragging = true;
        m_accumulatedDelta = 0.0;
        m_lastMousePos = e->pos();
        e->accept();
        return;
    }
    VCWidget::mousePressEvent(e);
}

void InfinityEncoderWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging)
    {
        QPointF center = QRectF(m_knobRect).center();
        QPointF lastVec = QPointF(m_lastMousePos) - center;
        QPointF curVec  = QPointF(e->pos())        - center;

        // Ignore if too close to center — avoids sign-flip jitter
        if (curVec.manhattanLength() >= 6.0 && lastVec.manhattanLength() >= 6.0)
        {
            double lastAngle = std::atan2(lastVec.y(), lastVec.x());
            double curAngle  = std::atan2(curVec.y(),  curVec.x());

            double angleDelta = curAngle - lastAngle;
            // Wrap to -π..π so we don't get a ±360° jump when crossing 0
            if (angleDelta >  M_PI) angleDelta -= 2.0 * M_PI;
            if (angleDelta < -M_PI) angleDelta += 2.0 * M_PI;

            // Scale: full 360° turn = 64 raw ticks (feels like a typical encoder)
            m_accumulatedDelta += (angleDelta / (2.0 * M_PI)) * 64.0;

            int intTicks = int(m_accumulatedDelta);
            if (intTicks != 0)
            {
                m_accumulatedDelta -= double(intTicks);
                applyDelta(intTicks);
            }
        }

        m_lastMousePos = e->pos();
        e->accept();
        return;
    }
    VCWidget::mouseMoveEvent(e);
}

void InfinityEncoderWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_dragging)
    {
        m_dragging = false;
        m_accumulatedDelta = 0.0;
        e->accept();
        return;
    }
    VCWidget::mouseReleaseEvent(e);
}

void InfinityEncoderWidget::wheelEvent(QWheelEvent* e)
{
    if (mode() == Doc::Operate)
    {
        int notches = e->angleDelta().y() / 120;
        if (notches != 0)
        {
            applyDelta(notches);
            e->accept();
            return;
        }
    }
    VCWidget::wheelEvent(e);
}

// ---- Delta application -----------------------------------------------------

int InfinityEncoderWidget::sensitivityMultiplier() const
{
    switch (m_sensitivity)
    {
        case Fine:   return 1;
        case Coarse: return 16;
        default:     return 4;   // Normal
    }
}

void InfinityEncoderWidget::applyDelta(int rawDelta)
{
    int delta  = rawDelta * sensitivityMultiplier();
    int newVal = 0;
    {
        QMutexLocker lk(&m_valueMutex);
        int v = int(m_value[m_activeBank]) + delta;
        v = qBound(0, v, 255);
        m_value[m_activeBank] = uchar(v);
        newVal = v;
    }

    // Visual: angle rotates continuously (infinity-style), not clamped
    m_knobAngleDeg = std::fmod(m_knobAngleDeg + rawDelta * 3.0, 360.0);
    if (m_knobAngleDeg < 0.0) m_knobAngleDeg += 360.0;

    update();
    updateInfoLabel();
    sendFeedback(newVal, encoderInputSourceId);
}

// ---- External input --------------------------------------------------------

void InfinityEncoderWidget::slotInputValueChanged(quint32 universe, quint32 channel, uchar value)
{
    if (!acceptsInput()) return;

    quint32 pagedCh = (page() << 16) | channel;

    if (checkInputSource(universe, pagedCh, value, sender(), encoderInputSourceId))
    {
        int delta = decodeSignedBit(value);
        if (delta != 0)
            applyDelta(delta);
        return;
    }

    for (int i = 0; i < NUM_BANKS; ++i)
    {
        quint8 id = quint8(bankAInputSourceId + i);
        if (checkInputSource(universe, pagedCh, value, sender(), id))
        {
            if (value > 0)
                setActiveBank(i);
            return;
        }
    }
}

void InfinityEncoderWidget::updateFeedback()
{
    QMutexLocker lk(&m_valueMutex);
    sendFeedback(int(m_value[m_activeBank]), encoderInputSourceId);
}

int InfinityEncoderWidget::decodeSignedBit(uchar value)
{
    if (value == 0 || value == 64) return 0;
    if (value < 64) return int(value);          // 1..63  = +1..+63
    return -int(value - 64);                    // 65..127 = -1..-63
}

// ---- DMX output (MasterTimer thread) ----------------------------------------

void InfinityEncoderWidget::writeDMX(MasterTimer* /*timer*/, QList<Universe*> universes)
{
    QMutexLocker lk(&m_valueMutex);

    for (int b = 0; b < NUM_BANKS; ++b)
    {
        const Slot& sl = m_slots[b];
        if (sl.fixtureId == UINT_MAX) continue;

        Fixture* fxi = m_doc->fixture(sl.fixtureId);
        if (!fxi || sl.channel >= fxi->channels()) continue;

        quint32 uni = fxi->universe();
        if ((int)uni >= universes.size()) continue;

        auto fader = m_faders.value(uni);
        if (fader.isNull())
        {
            fader = universes[uni]->requestFader(Universe::Auto);
            m_faders.insert(uni, fader);
        }

        FadeChannel* fc = fader->getChannelFader(
            m_doc, universes[uni], sl.fixtureId, sl.channel);

        if (fc->universe() == Universe::invalid())
        {
            fader->remove(fc);
            continue;
        }

        fc->setStart(fc->current());
        fc->setTarget(m_value[b]);
        fc->setReady(false);
        fc->setElapsed(0);
    }
}

// ---- Properties dialog ----------------------------------------------------

void InfinityEncoderWidget::editProperties()
{
    if (mode() != Doc::Design) return;

    // Copy current sources for dialog
    QSharedPointer<QLCInputSource> bankSrcs[NUM_BANKS];
    for (int i = 0; i < NUM_BANKS; ++i)
        bankSrcs[i] = inputSource(quint8(bankAInputSourceId + i));

    InfinityEncoderConfigDialog dlg(
        m_doc,
        m_slots,
        inputSource(encoderInputSourceId),
        bankSrcs,
        m_sensitivity,
        page(),
        this);

    if (dlg.exec() != QDialog::Accepted) return;

    for (int i = 0; i < NUM_BANKS; ++i)
        setSlot(i, dlg.slot(i));

    setInputSource(dlg.encoderInputSource(), encoderInputSourceId);
    for (int i = 0; i < NUM_BANKS; ++i)
        setInputSource(dlg.bankInputSource(i), quint8(bankAInputSourceId + i));

    setSensitivity(dlg.sensitivity());
    updateInfoLabel();
    m_doc->setModified();
}

// ---- Copy ------------------------------------------------------------------

VCWidget* InfinityEncoderWidget::createCopy(VCWidget* parent)
{
    Q_ASSERT(parent != nullptr);
    InfinityEncoderWidget* copy = new InfinityEncoderWidget(parent, m_doc);
    if (!copy->copyFrom(this))
    {
        delete copy;
        return nullptr;
    }

    for (int i = 0; i < NUM_BANKS; ++i)
        copy->setSlot(i, m_slots[i]);

    copy->setSensitivity(m_sensitivity);
    copy->setActiveBank(m_activeBank);

    copy->setInputSource(inputSource(encoderInputSourceId), encoderInputSourceId);
    for (int i = 0; i < NUM_BANKS; ++i)
        copy->setInputSource(inputSource(quint8(bankAInputSourceId + i)),
                             quint8(bankAInputSourceId + i));

    {
        QMutexLocker lk(&m_valueMutex);
        for (int i = 0; i < NUM_BANKS; ++i)
            copy->m_value[i] = m_value[i];
    }

    return copy;
}

void InfinityEncoderWidget::toClipboardJson(QJsonObject &obj, const Doc *doc) const
{
    VCWidget::toClipboardJson(obj, doc);

    obj["sensitivity"] = (int)m_sensitivity;
    obj["activeBank"]  = m_activeBank;

    QJsonArray slots;
    for (int b = 0; b < NUM_BANKS; ++b)
    {
        const Slot &s = m_slots[b];
        Fixture *fxi = doc->fixture(s.fixtureId);
        QJsonObject js;
        js["fixtureName"] = fxi ? fxi->name() : QString();
        js["channel"]     = (int)s.channel;
        js["label"]       = s.label;
        slots.append(js);
    }
    obj["slots"] = slots;
}

void InfinityEncoderWidget::fromClipboardJson(const QJsonObject &obj, Doc *doc)
{
    VCWidget::fromClipboardJson(obj, doc);

    m_sensitivity = static_cast<Sensitivity>(obj["sensitivity"].toInt(Normal));
    m_activeBank  = obj["activeBank"].toInt(0);

    const QJsonArray slots = obj["slots"].toArray();
    for (int b = 0; b < NUM_BANKS && b < slots.size(); ++b)
    {
        QJsonObject js = slots[b].toObject();
        Slot s;
        const QString fxName = js["fixtureName"].toString();
        s.fixtureId = UINT_MAX;
        if (!fxName.isEmpty())
        {
            for (Fixture *fxi : doc->fixtures())
            {
                if (fxi && fxi->name() == fxName)
                {
                    s.fixtureId = fxi->id();
                    break;
                }
            }
        }
        s.channel = (quint32)js["channel"].toInt(0);
        s.label   = js["label"].toString();
        setSlot(b, s);
    }

    updateInfoLabel();
    updateBankButtonStyles();
    updateSensButtonStyles();
}

// ---- Load & Save -----------------------------------------------------------

bool InfinityEncoderWidget::loadXML(QXmlStreamReader& root)
{
    if (root.name() != KXMLRoot) return false;

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
        else if (root.name() == KXMLActiveBank)
        {
            setActiveBank(root.readElementText().toInt());
        }
        else if (root.name() == KXMLSensitivity)
        {
            setSensitivity(static_cast<Sensitivity>(root.readElementText().toInt()));
        }
        else if (root.name() == KXMLSlot)
        {
            auto attrs = root.attributes();
            int bank = attrs.value(KXMLSlotBank).toInt();
            if (bank >= 0 && bank < NUM_BANKS)
            {
                Slot s;
                s.fixtureId = attrs.value(KXMLSlotFxId).toUInt();
                s.channel   = attrs.value(KXMLSlotCh).toUInt();
                s.label     = attrs.value(KXMLSlotLabel).toString();
                {
                    QMutexLocker lk(&m_valueMutex);
                    m_value[bank] = uchar(attrs.value(KXMLSlotValue).toUInt());
                }
                setSlot(bank, s);
            }
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLEncoderInput)
        {
            loadXMLSources(root, encoderInputSourceId);
        }
        else
        {
            // Bank inputs: BankAInput, BankBInput, BankCInput, BankDInput
            bool handled = false;
            for (int i = 0; i < NUM_BANKS; ++i)
            {
                if (root.name() == KXMLBankInput[i])
                {
                    loadXMLSources(root, quint8(bankAInputSourceId + i));
                    handled = true;
                    break;
                }
            }
            if (!handled)
                root.skipCurrentElement();
        }
    }

    updateInfoLabel();
    return true;
}

bool InfinityEncoderWidget::saveXML(QXmlStreamWriter* doc)
{
    Q_ASSERT(doc != nullptr);

    doc->writeStartElement(KXMLRoot);
    doc->writeAttribute(KXMLPluginId, KXMLPluginIdVal);

    saveXMLCommon(doc);
    saveXMLWindowState(doc);
    saveXMLAppearance(doc);

    doc->writeTextElement(KXMLActiveBank,  QString::number(m_activeBank));
    doc->writeTextElement(KXMLSensitivity, QString::number(int(m_sensitivity)));

    for (int b = 0; b < NUM_BANKS; ++b)
    {
        doc->writeStartElement(KXMLSlot);
        doc->writeAttribute(KXMLSlotBank,  QString::number(b));
        doc->writeAttribute(KXMLSlotFxId,  QString::number(m_slots[b].fixtureId));
        doc->writeAttribute(KXMLSlotCh,    QString::number(m_slots[b].channel));
        {
            QMutexLocker lk(&m_valueMutex);
            doc->writeAttribute(KXMLSlotValue, QString::number(m_value[b]));
        }
        doc->writeAttribute(KXMLSlotLabel, m_slots[b].label);
        doc->writeEndElement();
    }

    auto encSrc = inputSource(encoderInputSourceId);
    if (!encSrc.isNull() && encSrc->isValid())
    {
        doc->writeStartElement(KXMLEncoderInput);
        saveXMLInput(doc, encSrc);
        doc->writeEndElement();
    }

    for (int i = 0; i < NUM_BANKS; ++i)
    {
        auto src = inputSource(quint8(bankAInputSourceId + i));
        if (!src.isNull() && src->isValid())
        {
            doc->writeStartElement(KXMLBankInput[i]);
            saveXMLInput(doc, src);
            doc->writeEndElement();
        }
    }

    doc->writeEndElement();
    return true;
}

// ---- Paint -----------------------------------------------------------------

void InfinityEncoderWidget::recalcKnobRect()
{
    // Knob area = everything between infoLabel and bankWidget
    QRect areaRect = m_knobArea->geometry();

    int side = qMin(areaRect.width(), areaRect.height()) - 16;
    if (side < 30) side = 30;

    int x = areaRect.x() + (areaRect.width()  - side) / 2;
    int y = areaRect.y() + (areaRect.height() - side) / 2;
    m_knobRect = QRect(x, y, side, side);
}

void InfinityEncoderWidget::resizeEvent(QResizeEvent* e)
{
    VCWidget::resizeEvent(e);
    recalcKnobRect();
}

void InfinityEncoderWidget::paintEvent(QPaintEvent* e)
{
    QPainter p(this);

    // Background
    QColor bg = (mode() == Doc::Operate) ? QColor(18, 36, 18) : QColor(28, 28, 28);
    p.fillRect(rect(), bg);

    recalcKnobRect();

    if (!m_knobRect.isValid() || m_knobRect.width() < 10)
    {
        p.end();
        VCWidget::paintEvent(e);
        return;
    }

    p.setRenderHint(QPainter::Antialiasing);

    QPointF center  = QRectF(m_knobRect).center();
    qreal   radius  = m_knobRect.width() / 2.0 - 6.0;
    QColor  tint    = s_bankTint[m_activeBank];

    // Outer glow ring (subtle)
    QPen glowPen(tint, 10);
    glowPen.setCapStyle(Qt::RoundCap);
    QColor glowColor = tint;
    glowColor.setAlpha(40);
    p.setPen(QPen(glowColor, 10));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, radius + 4, radius + 4);

    // Main knob body
    p.setPen(QPen(tint, 3));
    p.setBrush(QColor(45, 45, 45));
    p.drawEllipse(center, radius, radius);

    // Value arc (shows current value 0..255 as arc from -135 to +135 degrees)
    {
        QMutexLocker lk(&m_valueMutex);
        qreal valueFraction = m_value[m_activeBank] / 255.0;
        qreal arcSpan = 270.0 * valueFraction;   // max arc is 270 degrees
        qreal startAngle = 225.0;                // start from bottom-left (Qt: 16ths CCW)
        // Draw in screen coords: Qt drawArc uses 1/16th of degree, CCW
        QRectF arcRect(center.x() - radius * 0.75, center.y() - radius * 0.75,
                       radius * 1.5, radius * 1.5);
        p.setPen(QPen(tint, 4, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        // Qt drawArc: startAngle in 1/16th degrees, CCW. We want CW from bottom-left.
        // bottom-left in screen = 225 deg (standard). For CW arc: negative spanAngle.
        p.drawArc(arcRect,
                  int((startAngle) * 16),
                  -int(arcSpan * 16));
    }

    // Indicator line (rotates with m_knobAngleDeg — infinity rotation)
    {
        qreal rad = qDegreesToRadians(m_knobAngleDeg - 90.0);
        QPointF end(center.x() + std::cos(rad) * (radius * 0.75),
                    center.y() + std::sin(rad) * (radius * 0.75));
        p.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(center, end);
    }

    // Center dot
    p.setBrush(tint);
    p.setPen(Qt::NoPen);
    p.drawEllipse(center, 5.0, 5.0);

    // Bank letter in center
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(qMax(8, int(radius * 0.3)));
    p.setFont(f);
    QString bankLetter = QString(QChar('A' + m_activeBank));
    p.drawText(QRectF(center.x() - radius * 0.3, center.y() - radius * 0.3,
                      radius * 0.6, radius * 0.6),
               Qt::AlignCenter, bankLetter);

    p.end();
    VCWidget::paintEvent(e);
}

// ---- Info label ------------------------------------------------------------

void InfinityEncoderWidget::updateInfoLabel()
{
    if (!m_infoLabel) return;

    const Slot& sl = m_slots[m_activeBank];
    if (sl.fixtureId == UINT_MAX)
    {
        m_infoLabel->setText(tr("Bank %1: not configured").arg(QChar('A' + m_activeBank)));
        return;
    }

    Fixture* fxi = m_doc ? m_doc->fixture(sl.fixtureId) : nullptr;
    if (!fxi)
    {
        m_infoLabel->setText(tr("Bank %1: missing fixture").arg(QChar('A' + m_activeBank)));
        return;
    }

    const QLCChannel* ch = fxi->channel(sl.channel);
    QString chName = sl.label.isEmpty()
        ? (ch ? ch->name() : tr("ch%1").arg(sl.channel + 1))
        : sl.label;

    uchar val;
    {
        QMutexLocker lk(&m_valueMutex);
        val = m_value[m_activeBank];
    }

    m_infoLabel->setText(
        QString("Bank %1: U%2/CH%3 %4  =  %5")
            .arg(QChar('A' + m_activeBank))
            .arg(fxi->universe() + 1)
            .arg(fxi->address() + sl.channel + 1)
            .arg(chName)
            .arg(val));
}

void InfinityEncoderWidget::updateBankButtonStyles()
{
    for (int i = 0; i < NUM_BANKS; ++i)
    {
        if (!m_bankBtn[i]) continue;
        if (i == m_activeBank)
        {
            m_bankBtn[i]->setStyleSheet(
                QString("QPushButton { background-color: %1; color: black; font-weight: bold; border-radius: 3px; }")
                    .arg(s_bankTint[i].name()));
        }
        else
        {
            m_bankBtn[i]->setStyleSheet(
                QString("QPushButton { background-color: %1; color: white; border-radius: 3px; }")
                    .arg(s_bankTint[i].darker(200).name()));
        }
    }
}

void InfinityEncoderWidget::updateSensButtonStyles()
{
    static const char* sensNames[3] = {"Fine", "Normal", "Coarse"};
    for (int i = 0; i < 3; ++i)
    {
        if (!m_sensBtn[i]) continue;
        bool active = (int(m_sensitivity) == i);
        m_sensBtn[i]->setStyleSheet(
            active
            ? QStringLiteral("QPushButton { background-color: #555; color: white; font-weight: bold; border-radius: 3px; }")
            : QStringLiteral("QPushButton { background-color: #2a2a2a; color: #888; border-radius: 3px; }"));
        Q_UNUSED(sensNames);
    }
}

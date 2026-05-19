/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "dmxnumericconfigdialog.h"

#include "inputselectionwidget.h"
#include "doc.h"
#include "fixture.h"
#include "qlcchannel.h"

#include <QFormLayout>
#include <algorithm>

DMXNumericConfigDialog::DMXNumericConfigDialog(Doc* doc,
                                               quint32 currentFxId,
                                               quint32 currentChannel,
                                               QSharedPointer<QLCInputSource> valueSrc,
                                               QSharedPointer<QLCInputSource> applySrc,
                                               int widgetPage,
                                               QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_selectedFixtureId(currentFxId)
    , m_selectedChannel(currentChannel)
    , m_initFixtureId(currentFxId)
    , m_initChannel(currentChannel)
{
    setWindowTitle(tr("DMX Numeric — Properties"));
    setMinimumWidth(420);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ---- Fixture / Channel ----
    QGroupBox* fxGrp = new QGroupBox(tr("Patched fixture channel"), this);
    QFormLayout* form = new QFormLayout(fxGrp);

    m_fixtureCb = new QComboBox(fxGrp);
    m_channelCb = new QComboBox(fxGrp);
    form->addRow(tr("Fixture:"), m_fixtureCb);
    form->addRow(tr("Channel:"), m_channelCb);
    root->addWidget(fxGrp);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    QFont f = m_infoLabel->font();
    f.setItalic(true);
    m_infoLabel->setFont(f);
    root->addWidget(m_infoLabel);

    // ---- External Input ----
    QGroupBox* inputGrp = new QGroupBox(tr("External Input"), this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGrp);

    QGroupBox* valueGrp = new QGroupBox(tr("Value (0–255)"), inputGrp);
    QVBoxLayout* valueLayout = new QVBoxLayout(valueGrp);
    m_valueInputSel = new InputSelectionWidget(doc, valueGrp);
    m_valueInputSel->setKeyInputVisibility(false);
    m_valueInputSel->setWidgetPage(widgetPage);
    m_valueInputSel->setInputSource(valueSrc);
    valueLayout->addWidget(m_valueInputSel);
    inputLayout->addWidget(valueGrp);

    QGroupBox* applyGrp = new QGroupBox(tr("Apply trigger"), inputGrp);
    QVBoxLayout* applyLayout = new QVBoxLayout(applyGrp);
    m_applyInputSel = new InputSelectionWidget(doc, applyGrp);
    m_applyInputSel->setKeyInputVisibility(false);
    m_applyInputSel->setWidgetPage(widgetPage);
    m_applyInputSel->setInputSource(applySrc);
    applyLayout->addWidget(m_applyInputSel);
    inputLayout->addWidget(applyGrp);

    root->addWidget(inputGrp);

    // ---- Buttons ----
    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_fixtureCb, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DMXNumericConfigDialog::slotFixtureChanged);
    connect(m_channelCb, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DMXNumericConfigDialog::slotChannelChanged);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    populateFixtures();
}

QSharedPointer<QLCInputSource> DMXNumericConfigDialog::valueInputSource() const
{
    return m_valueInputSel ? m_valueInputSel->inputSource()
                           : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> DMXNumericConfigDialog::applyInputSource() const
{
    return m_applyInputSel ? m_applyInputSel->inputSource()
                           : QSharedPointer<QLCInputSource>();
}

void DMXNumericConfigDialog::populateFixtures()
{
    m_fixtureCb->blockSignals(true);
    m_fixtureCb->clear();

    QList<Fixture*> sorted = m_doc->fixtures();
    std::sort(sorted.begin(), sorted.end(), [](Fixture* a, Fixture* b) {
        if (a->universe() != b->universe()) return a->universe() < b->universe();
        return a->address() < b->address();
    });

    int preSelectIdx = 0;
    for (Fixture* fx : std::as_const(sorted))
    {
        QString label = QString("U%1 | CH%2 — %3 (%4ch)")
            .arg(fx->universe() + 1)
            .arg(fx->address() + 1, 3, 10, QChar('0'))
            .arg(fx->name())
            .arg(fx->channels());
        m_fixtureCb->addItem(label, QVariant::fromValue<quint32>(fx->id()));

        if (fx->id() == m_initFixtureId)
            preSelectIdx = m_fixtureCb->count() - 1;
    }

    m_fixtureCb->blockSignals(false);
    m_fixtureCb->setCurrentIndex(preSelectIdx);
    slotFixtureChanged(preSelectIdx);
}

void DMXNumericConfigDialog::slotFixtureChanged(int index)
{
    m_channelCb->blockSignals(true);
    m_channelCb->clear();

    if (index < 0 || index >= m_fixtureCb->count())
    {
        m_channelCb->blockSignals(false);
        return;
    }

    quint32 fxId = m_fixtureCb->itemData(index).value<quint32>();
    Fixture* fx  = m_doc->fixture(fxId);
    if (!fx) { m_channelCb->blockSignals(false); return; }

    int preSelectCh = 0;
    for (quint32 ch = 0; ch < fx->channels(); ++ch)
    {
        const QLCChannel* qlcCh = fx->channel(ch);
        QString chName = qlcCh ? qlcCh->name() : tr("Channel %1").arg(ch + 1);
        QString label = QString("CH%1 — %2")
            .arg(fx->address() + ch + 1, 3, 10, QChar('0'))
            .arg(chName);
        m_channelCb->addItem(label, QVariant::fromValue<quint32>(ch));

        if (fxId == m_initFixtureId && ch == m_initChannel)
            preSelectCh = (int)ch;
    }

    m_channelCb->blockSignals(false);
    m_channelCb->setCurrentIndex(preSelectCh);
    slotChannelChanged(preSelectCh);
}

void DMXNumericConfigDialog::slotChannelChanged(int index)
{
    int fxIdx = m_fixtureCb->currentIndex();
    if (fxIdx < 0 || index < 0) return;

    quint32 fxId = m_fixtureCb->itemData(fxIdx).value<quint32>();
    Fixture* fx  = m_doc->fixture(fxId);
    if (!fx) return;

    m_selectedFixtureId = fxId;
    m_selectedChannel   = m_channelCb->itemData(index).value<quint32>();

    const QLCChannel* ch = fx->channel(m_selectedChannel);
    m_infoLabel->setText(
        tr("Universe %1  |  DMX address %2  (channel \"%3\" of %4)")
            .arg(fx->universe() + 1)
            .arg(fx->address() + m_selectedChannel + 1)
            .arg(ch ? ch->name() : tr("ch%1").arg(m_selectedChannel + 1))
            .arg(fx->name()));
}

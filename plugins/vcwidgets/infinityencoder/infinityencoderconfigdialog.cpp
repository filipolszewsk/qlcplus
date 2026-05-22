/*
  QLC+ VC Widget Plugin — Infinity Encoder
  infinityencoderconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "infinityencoderconfigdialog.h"
#include "inputselectionwidget.h"
#include "doc.h"
#include "fixture.h"
#include "qlcchannel.h"

#include <algorithm>
#include <QScrollArea>

static const QColor s_bankTint[InfinityEncoderWidget::NUM_BANKS] = {
    QColor("#3a86ff"), QColor("#ff006e"), QColor("#ffbe0b"), QColor("#06ffa5")
};

InfinityEncoderConfigDialog::InfinityEncoderConfigDialog(
    Doc*                                  doc,
    const InfinityEncoderWidget::Slot     initSlots[InfinityEncoderWidget::NUM_BANKS],
    QSharedPointer<QLCInputSource>        encoderSrc,
    QSharedPointer<QLCInputSource>        bankSrcs[InfinityEncoderWidget::NUM_BANKS],
    InfinityEncoderWidget::Sensitivity    sens,
    int                                   widgetPage,
    QWidget*                              parent)
    : QDialog(parent)
    , m_doc(doc)
{
    for (int i = 0; i < N; ++i)
    {
        m_initFxId[i] = initSlots[i].fixtureId;
        m_initCh[i]   = initSlots[i].channel;
        m_selFxId[i]  = initSlots[i].fixtureId;
        m_selCh[i]    = initSlots[i].channel;
        m_selLabel[i] = initSlots[i].label;
    }

    setWindowTitle(tr("Infinity Encoder — Properties"));
    setMinimumWidth(480);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ---- Channel Slots -------------------------------------------------------
    QGroupBox* slotsGrp = new QGroupBox(tr("Channel slots"), this);
    QGridLayout* slotsGrid = new QGridLayout(slotsGrp);
    slotsGrid->setColumnStretch(1, 3);
    slotsGrid->setColumnStretch(2, 3);
    slotsGrid->setColumnStretch(3, 2);

    // Header row
    slotsGrid->addWidget(new QLabel(tr("Bank")),    0, 0);
    slotsGrid->addWidget(new QLabel(tr("Fixture")), 0, 1);
    slotsGrid->addWidget(new QLabel(tr("Channel")), 0, 2);
    slotsGrid->addWidget(new QLabel(tr("Label")),   0, 3);

    static const char* bankLetters[N] = {"A", "B", "C", "D"};

    for (int i = 0; i < N; ++i)
    {
        QLabel* bankLbl = new QLabel(QLatin1String(bankLetters[i]), slotsGrp);
        bankLbl->setStyleSheet(
            QString("QLabel { color: %1; font-weight: bold; }").arg(s_bankTint[i].name()));
        slotsGrid->addWidget(bankLbl, i + 1, 0);

        m_fixtureCb[i] = new QComboBox(slotsGrp);
        slotsGrid->addWidget(m_fixtureCb[i], i + 1, 1);

        m_channelCb[i] = new QComboBox(slotsGrp);
        slotsGrid->addWidget(m_channelCb[i], i + 1, 2);

        m_labelEdit[i] = new QLineEdit(initSlots[i].label, slotsGrp);
        m_labelEdit[i]->setPlaceholderText(tr("optional"));
        slotsGrid->addWidget(m_labelEdit[i], i + 1, 3);

        // Populate fixtures for this bank
        populateFixtures(i);

        // Connect changes
        connect(m_fixtureCb[i], QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, i](int idx) { slotFixtureChanged(i, idx); });
        connect(m_channelCb[i], QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, i](int idx) { slotChannelChanged(i, idx); });
    }
    root->addWidget(slotsGrp);

    // ---- External Input ------------------------------------------------------
    QGroupBox* inputGrp = new QGroupBox(tr("External Input"), this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGrp);

    // Encoder input
    QGroupBox* encGrp = new QGroupBox(tr("Encoder (relative, signed-bit MIDI)"), inputGrp);
    QVBoxLayout* encLayout = new QVBoxLayout(encGrp);
    m_encoderInputSel = new InputSelectionWidget(doc, encGrp);
    m_encoderInputSel->setKeyInputVisibility(false);
    m_encoderInputSel->setWidgetPage(widgetPage);
    m_encoderInputSel->setInputSource(encoderSrc);
    encLayout->addWidget(m_encoderInputSel);
    inputLayout->addWidget(encGrp);

    // Bank triggers
    QGroupBox* banksGrp = new QGroupBox(tr("Bank selection triggers (A/B/C/D)"), inputGrp);
    QGridLayout* banksGrid = new QGridLayout(banksGrp);

    for (int i = 0; i < N; ++i)
    {
        QLabel* lbl = new QLabel(tr("Bank %1:").arg(QChar('A' + i)), banksGrp);
        lbl->setStyleSheet(
            QString("QLabel { color: %1; font-weight: bold; }").arg(s_bankTint[i].name()));
        banksGrid->addWidget(lbl, i, 0);

        m_bankInputSel[i] = new InputSelectionWidget(doc, banksGrp);
        m_bankInputSel[i]->setKeyInputVisibility(false);
        m_bankInputSel[i]->setWidgetPage(widgetPage);
        m_bankInputSel[i]->setInputSource(bankSrcs[i]);
        banksGrid->addWidget(m_bankInputSel[i], i, 1);
    }
    inputLayout->addWidget(banksGrp);
    root->addWidget(inputGrp);

    // ---- Default Sensitivity -------------------------------------------------
    QGroupBox* sensGrp = new QGroupBox(tr("Default sensitivity"), this);
    QHBoxLayout* sensLayout = new QHBoxLayout(sensGrp);

    static const char* sensLabels[3] = {"Fine (1:1)", "Normal (×4)", "Coarse (×16)"};
    QButtonGroup* sensGroup = new QButtonGroup(sensGrp);
    for (int i = 0; i < 3; ++i)
    {
        m_sensRadio[i] = new QRadioButton(QLatin1String(sensLabels[i]), sensGrp);
        sensGroup->addButton(m_sensRadio[i], i);
        sensLayout->addWidget(m_sensRadio[i]);
    }
    m_sensRadio[int(sens)]->setChecked(true);
    root->addWidget(sensGrp);

    // ---- Buttons -------------------------------------------------------------
    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ---- Results ---------------------------------------------------------------

InfinityEncoderWidget::Slot InfinityEncoderConfigDialog::slot(int bank) const
{
    if (bank < 0 || bank >= N) return {};
    InfinityEncoderWidget::Slot s;
    s.fixtureId = m_selFxId[bank];
    s.channel   = m_selCh[bank];
    s.label     = m_labelEdit[bank] ? m_labelEdit[bank]->text() : QString();
    return s;
}

QSharedPointer<QLCInputSource> InfinityEncoderConfigDialog::encoderInputSource() const
{
    return m_encoderInputSel ? m_encoderInputSel->inputSource()
                             : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> InfinityEncoderConfigDialog::bankInputSource(int bank) const
{
    if (bank < 0 || bank >= N || !m_bankInputSel[bank])
        return QSharedPointer<QLCInputSource>();
    return m_bankInputSel[bank]->inputSource();
}

InfinityEncoderWidget::Sensitivity InfinityEncoderConfigDialog::sensitivity() const
{
    for (int i = 0; i < 3; ++i)
        if (m_sensRadio[i] && m_sensRadio[i]->isChecked())
            return static_cast<InfinityEncoderWidget::Sensitivity>(i);
    return InfinityEncoderWidget::Normal;
}

// ---- Private helpers -------------------------------------------------------

void InfinityEncoderConfigDialog::populateFixtures(int bank)
{
    m_fixtureCb[bank]->blockSignals(true);
    m_fixtureCb[bank]->clear();

    // "— none —" entry
    m_fixtureCb[bank]->addItem(tr("— none —"), QVariant::fromValue<quint32>(UINT_MAX));

    QList<Fixture*> sorted = m_doc->fixtures();
    std::sort(sorted.begin(), sorted.end(), [](Fixture* a, Fixture* b) {
        if (a->universe() != b->universe()) return a->universe() < b->universe();
        return a->address() < b->address();
    });

    int preSelect = 0;
    for (Fixture* fx : std::as_const(sorted))
    {
        QString label = QString("U%1 | CH%2 — %3 (%4ch)")
            .arg(fx->universe() + 1)
            .arg(fx->address() + 1, 3, 10, QChar('0'))
            .arg(fx->name())
            .arg(fx->channels());
        m_fixtureCb[bank]->addItem(label, QVariant::fromValue<quint32>(fx->id()));

        if (fx->id() == m_initFxId[bank])
            preSelect = m_fixtureCb[bank]->count() - 1;
    }

    m_fixtureCb[bank]->blockSignals(false);
    m_fixtureCb[bank]->setCurrentIndex(preSelect);
    slotFixtureChanged(bank, preSelect);
}

void InfinityEncoderConfigDialog::slotFixtureChanged(int bank, int index)
{
    quint32 fxId = m_fixtureCb[bank]->itemData(index).value<quint32>();
    m_selFxId[bank] = fxId;

    quint32 preSelectCh = (fxId == m_initFxId[bank]) ? m_initCh[bank] : 0;
    populateChannels(bank, fxId, preSelectCh);
}

void InfinityEncoderConfigDialog::populateChannels(int bank, quint32 fixtureId, quint32 preSelectCh)
{
    m_channelCb[bank]->blockSignals(true);
    m_channelCb[bank]->clear();

    if (fixtureId == UINT_MAX)
    {
        m_channelCb[bank]->blockSignals(false);
        m_selCh[bank] = 0;
        return;
    }

    Fixture* fx = m_doc->fixture(fixtureId);
    if (!fx)
    {
        m_channelCb[bank]->blockSignals(false);
        return;
    }

    int preSelectIdx = 0;
    for (quint32 ch = 0; ch < fx->channels(); ++ch)
    {
        const QLCChannel* qlcCh = fx->channel(ch);
        QString chName = qlcCh ? qlcCh->name() : tr("Channel %1").arg(ch + 1);
        QString label = QString("CH%1 — %2")
            .arg(fx->address() + ch + 1, 3, 10, QChar('0'))
            .arg(chName);
        m_channelCb[bank]->addItem(label, QVariant::fromValue<quint32>(ch));

        if (ch == preSelectCh) preSelectIdx = (int)ch;
    }

    m_channelCb[bank]->blockSignals(false);
    m_channelCb[bank]->setCurrentIndex(preSelectIdx);
    slotChannelChanged(bank, preSelectIdx);
}

void InfinityEncoderConfigDialog::slotChannelChanged(int bank, int index)
{
    if (index < 0) return;
    m_selCh[bank] = m_channelCb[bank]->itemData(index).value<quint32>();
}

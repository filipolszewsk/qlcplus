/*
  QLC+ VC Widget Plugin — Infinity Encoder
  infinityencoderconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QSharedPointer>

#include "qlcinputsource.h"
#include "infinityencoderwidget.h"

class Doc;
class InputSelectionWidget;

class InfinityEncoderConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InfinityEncoderConfigDialog(
        Doc*                                  doc,
        const InfinityEncoderWidget::Slot     initSlots[InfinityEncoderWidget::NUM_BANKS],
        QSharedPointer<QLCInputSource>        encoderSrc,
        QSharedPointer<QLCInputSource>        bankSrcs[InfinityEncoderWidget::NUM_BANKS],
        InfinityEncoderWidget::Sensitivity    sens,
        int                                   widgetPage,
        QWidget*                              parent = nullptr);

    // ---- Results -----------------------------------------------------------
    InfinityEncoderWidget::Slot           slot(int bank) const;
    QSharedPointer<QLCInputSource>        encoderInputSource() const;
    QSharedPointer<QLCInputSource>        bankInputSource(int bank) const;
    InfinityEncoderWidget::Sensitivity    sensitivity() const;

private slots:
    void slotFixtureChanged(int bank, int index);
    void slotChannelChanged(int bank, int index);

private:
    void populateFixtures(int bank);
    void populateChannels(int bank, quint32 fixtureId, quint32 preSelectCh);

    Doc* m_doc;

    static constexpr int N = InfinityEncoderWidget::NUM_BANKS;

    // Slot rows
    QComboBox*  m_fixtureCb[N] = {nullptr};
    QComboBox*  m_channelCb[N] = {nullptr};
    QLineEdit*  m_labelEdit[N] = {nullptr};

    // Selections
    quint32     m_initFxId[N];
    quint32     m_initCh[N];
    quint32     m_selFxId[N];
    quint32     m_selCh[N];
    QString     m_selLabel[N];

    // External input
    InputSelectionWidget* m_encoderInputSel          = nullptr;
    InputSelectionWidget* m_bankInputSel[N]          = {nullptr};

    // Sensitivity
    QRadioButton*         m_sensRadio[3]             = {nullptr};

    QDialogButtonBox*     m_buttons = nullptr;
};

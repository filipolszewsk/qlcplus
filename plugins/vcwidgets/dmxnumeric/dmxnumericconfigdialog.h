/*
  QLC+ VC Widget Plugin — DMX Numeric Input
  dmxnumericconfigdialog.h — Apache 2.0 / public domain
*/

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSharedPointer>

#include "qlcinputsource.h"

class Doc;
class InputSelectionWidget;

class DMXNumericConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param doc            QLC+ document
     * @param currentFxId    Currently configured fixture ID (UINT_MAX = none)
     * @param currentChannel Currently configured channel within fixture (0-based)
     * @param valueSrc       Current Value input source (may be null)
     * @param applySrc       Current Apply input source (may be null)
     * @param widgetPage     Widget's page (used by InputSelectionWidget for paged channels)
     * @param parent         Parent widget
     */
    explicit DMXNumericConfigDialog(Doc* doc,
                                    quint32 currentFxId,
                                    quint32 currentChannel,
                                    QSharedPointer<QLCInputSource> valueSrc,
                                    QSharedPointer<QLCInputSource> applySrc,
                                    int widgetPage,
                                    QWidget* parent = nullptr);

    quint32 fixtureId() const { return m_selectedFixtureId; }
    quint32 channel()   const { return m_selectedChannel; }
    QSharedPointer<QLCInputSource> valueInputSource() const;
    QSharedPointer<QLCInputSource> applyInputSource() const;

private slots:
    void slotFixtureChanged(int index);
    void slotChannelChanged(int index);

private:
    void populateFixtures();

    Doc* m_doc;

    // Fixture / Channel
    QComboBox*        m_fixtureCb = nullptr;
    QComboBox*        m_channelCb = nullptr;
    QLabel*           m_infoLabel = nullptr;

    // External Input
    InputSelectionWidget* m_valueInputSel = nullptr;
    InputSelectionWidget* m_applyInputSel = nullptr;

    QDialogButtonBox* m_buttons = nullptr;

    quint32 m_selectedFixtureId;
    quint32 m_selectedChannel;
    quint32 m_initFixtureId;
    quint32 m_initChannel;
};

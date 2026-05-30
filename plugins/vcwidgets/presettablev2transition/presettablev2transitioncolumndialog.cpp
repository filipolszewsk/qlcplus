#include "presettablev2transitioncolumndialog.h"
#include "inputselectionwidget.h"
#include "doc.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

PresetTableV2TransitionColumnDialog::PresetTableV2TransitionColumnDialog(
        Doc* doc, const QString& columnTitle, QSharedPointer<QLCInputSource> src,
        int widgetPage, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("EFX column — %1").arg(columnTitle));
    setMinimumWidth(400);

    QVBoxLayout* root = new QVBoxLayout(this);

    QLabel* hint = new QLabel(
            tr("Map MIDI/OSC/DMX to override this column for all presets while in Operate mode."),
            this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    m_inputSel = new InputSelectionWidget(doc, this);
    m_inputSel->setKeyInputVisibility(false);
    m_inputSel->setWidgetPage(widgetPage);
    m_inputSel->setInputSource(src);
    root->addWidget(m_inputSel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QSharedPointer<QLCInputSource> PresetTableV2TransitionColumnDialog::inputSource() const
{
    return m_inputSel ? m_inputSel->inputSource() : QSharedPointer<QLCInputSource>();
}

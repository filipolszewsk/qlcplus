/*
  QLC+ VC Widget Plugin — Fixture Group Layout
  fixturegrouplayoutconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "fixturegrouplayoutconfigdialog.h"

#include "doc.h"
#include "fixturegroup.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>

FixtureGroupLayoutConfigDialog::FixtureGroupLayoutConfigDialog(Doc* doc,
                                                               quint32 currentGroupId,
                                                               QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_initGroupId(currentGroupId)
{
    setWindowTitle(tr("Fixture Group Layout — Properties"));
    setMinimumWidth(360);

    QVBoxLayout* root = new QVBoxLayout(this);

    QFormLayout* form = new QFormLayout();
    m_groupCombo = new QComboBox(this);
    form->addRow(tr("Fixture group:"), m_groupCombo);
    root->addLayout(form);

    QLabel* hint = new QLabel(
        tr("Drag fixtures on the grid to move them. Changes are saved in the workspace."),
        this);
    hint->setWordWrap(true);
    QFont f = hint->font();
    f.setItalic(true);
    hint->setFont(f);
    root->addWidget(hint);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    populateGroups();
}

void FixtureGroupLayoutConfigDialog::populateGroups()
{
    m_groupCombo->clear();
    m_groupCombo->addItem(tr("(None)"), QVariant::fromValue(FixtureGroup::invalidId()));

    int preSelect = 0;
    int idx = 1;
    for (FixtureGroup* grp : m_doc->fixtureGroups())
    {
        if (grp == nullptr)
            continue;
        m_groupCombo->addItem(grp->name(), QVariant::fromValue(grp->id()));
        if (grp->id() == m_initGroupId)
            preSelect = idx;
        idx++;
    }

    m_groupCombo->setCurrentIndex(preSelect);
}

quint32 FixtureGroupLayoutConfigDialog::fixtureGroupId() const
{
    return m_groupCombo->currentData().toUInt();
}

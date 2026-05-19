/*
  QLC+ VC Widget Plugin — Multi Button
  multibuttonconfigdialog.cpp — Apache 2.0 / public domain
*/

#include "multibuttonconfigdialog.h"

#include "inputselectionwidget.h"
#include "functionselection.h"
#include "function.h"
#include "doc.h"
#include "scribbledialog.h"

#include <QInputDialog>
#include <QFormLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>

MultiButtonConfigDialog::MultiButtonConfigDialog(
    Doc*                           doc,
    const QList<quint32>&          funcIds,
    const QStringList&             funcLabels,
    const QStringList&             iconPaths,
    int                            longPressMs,
    bool                           addOffAtEnd,
    bool                           monitorChannelValues,
    QSharedPointer<QLCInputSource> triggerSrc,
    QSharedPointer<QLCInputSource> popupSrc,
    int                            widgetPage,
    QWidget*                       parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_ids(funcIds)
    , m_labels(funcLabels)
    , m_icons(iconPaths)
{
    // Ensure parallel arrays are same length
    while (m_labels.size() < m_ids.size()) m_labels.append(QString());
    while (m_icons.size() < m_ids.size())  m_icons.append(QString());

    setWindowTitle(tr("Multi Button — Properties"));
    setMinimumWidth(480);

    QVBoxLayout* root = new QVBoxLayout(this);

    // ---- Function List --------------------------------------------------
    QGroupBox* listGrp = new QGroupBox(tr("Functions (cycle order)"), this);
    QHBoxLayout* listLayout = new QHBoxLayout(listGrp);

    m_listWidget = new QListWidget(listGrp);
    m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setIconSize(QSize(24, 24));
    m_listWidget->setMinimumHeight(160);
    listLayout->addWidget(m_listWidget, 1);

    // Side buttons
    QVBoxLayout* btnCol = new QVBoxLayout;
    m_addBtn        = new QPushButton(tr("Add…"),           listGrp);
    m_removeBtn     = new QPushButton(tr("Remove"),         listGrp);
    m_editLblBtn    = new QPushButton(tr("Custom label"),   listGrp);
    m_scribbleBtn   = new QPushButton(tr("Scribble icon…"), listGrp);
    m_chooseIconBtn = new QPushButton(tr("Choose icon…"),   listGrp);
    m_clearIconBtn  = new QPushButton(tr("Clear icon"),     listGrp);
    m_upBtn         = new QPushButton(tr("Move up"),        listGrp);
    m_downBtn       = new QPushButton(tr("Move down"),      listGrp);

    btnCol->addWidget(m_addBtn);
    btnCol->addWidget(m_removeBtn);
    btnCol->addSpacing(6);
    btnCol->addWidget(m_editLblBtn);
    btnCol->addSpacing(6);
    btnCol->addWidget(m_scribbleBtn);
    btnCol->addWidget(m_chooseIconBtn);
    btnCol->addWidget(m_clearIconBtn);
    btnCol->addStretch();
    btnCol->addWidget(m_upBtn);
    btnCol->addWidget(m_downBtn);
    listLayout->addLayout(btnCol);

    root->addWidget(listGrp);

    connect(m_addBtn,        &QPushButton::clicked, this, &MultiButtonConfigDialog::slotAdd);
    connect(m_removeBtn,     &QPushButton::clicked, this, &MultiButtonConfigDialog::slotRemove);
    connect(m_editLblBtn,    &QPushButton::clicked, this, &MultiButtonConfigDialog::slotEditLabel);
    connect(m_scribbleBtn,   &QPushButton::clicked, this, &MultiButtonConfigDialog::slotScribbleIcon);
    connect(m_chooseIconBtn, &QPushButton::clicked, this, &MultiButtonConfigDialog::slotChooseIcon);
    connect(m_clearIconBtn,  &QPushButton::clicked, this, &MultiButtonConfigDialog::slotClearIcon);
    connect(m_upBtn,         &QPushButton::clicked, this, &MultiButtonConfigDialog::slotMoveUp);
    connect(m_downBtn,       &QPushButton::clicked, this, &MultiButtonConfigDialog::slotMoveDown);
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &MultiButtonConfigDialog::slotSelectionChanged);

    // ---- Behavior -------------------------------------------------------
    QGroupBox* behaviorGrp = new QGroupBox(tr("Behavior"), this);
    QFormLayout* behaviorForm = new QFormLayout(behaviorGrp);

    m_longPressSpin = new QSpinBox(behaviorGrp);
    m_longPressSpin->setRange(200, 2000);
    m_longPressSpin->setSingleStep(50);
    m_longPressSpin->setSuffix(tr(" ms"));
    m_longPressSpin->setValue(longPressMs);
    m_longPressSpin->setToolTip(tr("How long to hold for popup menu (also activated by right-click)"));
    behaviorForm->addRow(tr("Long press threshold:"), m_longPressSpin);

    m_offAtEndCheck = new QCheckBox(tr("Add \"OFF\" step at end of cycle"), behaviorGrp);
    m_offAtEndCheck->setChecked(addOffAtEnd);
    m_offAtEndCheck->setToolTip(
        tr("When enabled, one extra step is added after the last function.\n"
           "Cycling will land on OFF (no function active) before wrapping back to the first."));
    behaviorForm->addRow(QString(), m_offAtEndCheck);

    m_monitorCheck = new QCheckBox(tr("Monitor channel values"), behaviorGrp);
    m_monitorCheck->setChecked(monitorChannelValues);
    m_monitorCheck->setToolTip(
        tr("When enabled, the button automatically highlights the entry whose Scene matches\n"
           "the current DMX output, even if it was started from another widget.\n"
           "Only applies to Scene entries. An amber dot in the corner indicates monitored state."));
    behaviorForm->addRow(QString(), m_monitorCheck);

    root->addWidget(behaviorGrp);

    // ---- External Input -------------------------------------------------
    QGroupBox* inputGrp = new QGroupBox(tr("External Input"), this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGrp);

    QGroupBox* trigGrp = new QGroupBox(tr("Cycle trigger (short press equivalent)"), inputGrp);
    QVBoxLayout* trigLayout = new QVBoxLayout(trigGrp);
    m_triggerInputSel = new InputSelectionWidget(doc, trigGrp);
    m_triggerInputSel->setKeyInputVisibility(false);
    m_triggerInputSel->setWidgetPage(widgetPage);
    m_triggerInputSel->setInputSource(triggerSrc);
    trigLayout->addWidget(m_triggerInputSel);
    inputLayout->addWidget(trigGrp);

    QGroupBox* popGrp = new QGroupBox(tr("Popup trigger (long press equivalent)"), inputGrp);
    QVBoxLayout* popLayout = new QVBoxLayout(popGrp);
    m_popupInputSel = new InputSelectionWidget(doc, popGrp);
    m_popupInputSel->setKeyInputVisibility(false);
    m_popupInputSel->setWidgetPage(widgetPage);
    m_popupInputSel->setInputSource(popupSrc);
    popLayout->addWidget(m_popupInputSel);
    inputLayout->addWidget(popGrp);

    root->addWidget(inputGrp);

    // ---- Buttons --------------------------------------------------------
    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    rebuildList();
    slotSelectionChanged();
}

// ---- Results getters -------------------------------------------------------

// Read back from list widget to capture any drag-drop reorders
static QList<quint32> listWidgetIds(const QListWidget* lw)
{
    QList<quint32> ids;
    for (int i = 0; i < lw->count(); ++i)
        ids.append(lw->item(i)->data(Qt::UserRole).toUInt());
    return ids;
}

static QStringList listWidgetLabels(const QListWidget* lw)
{
    QStringList labels;
    for (int i = 0; i < lw->count(); ++i)
        labels.append(lw->item(i)->data(Qt::UserRole + 1).toString());
    return labels;
}

static QStringList listWidgetIcons(const QListWidget* lw)
{
    QStringList icons;
    for (int i = 0; i < lw->count(); ++i)
        icons.append(lw->item(i)->data(Qt::UserRole + 2).toString());
    return icons;
}

QList<quint32> MultiButtonConfigDialog::functionIds() const
{
    return listWidgetIds(m_listWidget);
}

QStringList MultiButtonConfigDialog::functionLabels() const
{
    return listWidgetLabels(m_listWidget);
}

QStringList MultiButtonConfigDialog::iconPaths() const
{
    return listWidgetIcons(m_listWidget);
}

int MultiButtonConfigDialog::longPressMs() const
{
    return m_longPressSpin->value();
}

bool MultiButtonConfigDialog::addOffAtEnd() const
{
    return m_offAtEndCheck ? m_offAtEndCheck->isChecked() : false;
}

bool MultiButtonConfigDialog::monitorChannelValues() const
{
    return m_monitorCheck ? m_monitorCheck->isChecked() : false;
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::triggerInputSource() const
{
    return m_triggerInputSel ? m_triggerInputSel->inputSource()
                             : QSharedPointer<QLCInputSource>();
}

QSharedPointer<QLCInputSource> MultiButtonConfigDialog::popupInputSource() const
{
    return m_popupInputSel ? m_popupInputSel->inputSource()
                           : QSharedPointer<QLCInputSource>();
}

// ---- Private helpers -------------------------------------------------------

static QIcon thumbIcon(const QString& path)
{
    if (path.isEmpty()) return QIcon();
    QPixmap px(path);
    if (px.isNull()) return QIcon();
    return QIcon(px.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MultiButtonConfigDialog::rebuildList()
{
    m_listWidget->clear();
    for (int i = 0; i < m_ids.size(); ++i)
    {
        quint32 id    = m_ids.at(i);
        QString label = m_labels.value(i);
        QString icon  = m_icons.value(i);

        Function* f   = m_doc->function(id);
        QString display = label.isEmpty()
            ? (f ? f->name() : tr("ID %1 (missing)").arg(id))
            : QString("%1 (%2)").arg(label, f ? f->name() : tr("?"));

        QListWidgetItem* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole,     QVariant::fromValue<quint32>(id));
        item->setData(Qt::UserRole + 1, label);
        item->setData(Qt::UserRole + 2, icon);
        item->setIcon(thumbIcon(icon));
        if (!f)
            item->setForeground(Qt::red);
        m_listWidget->addItem(item);
    }
}

void MultiButtonConfigDialog::slotSelectionChanged()
{
    bool has = (m_listWidget->currentRow() >= 0);
    int  row = m_listWidget->currentRow();
    int  cnt = m_listWidget->count();

    m_removeBtn->setEnabled(has);
    m_editLblBtn->setEnabled(has);
    m_scribbleBtn->setEnabled(has);
    m_chooseIconBtn->setEnabled(has);
    m_clearIconBtn->setEnabled(has);
    m_upBtn->setEnabled(has && row > 0);
    m_downBtn->setEnabled(has && row < cnt - 1);
}

void MultiButtonConfigDialog::slotAdd()
{
    FunctionSelection fs(this, m_doc);
    fs.setMultiSelection(true);
    if (fs.exec() != QDialog::Accepted) return;

    for (quint32 fid : fs.selection())
    {
        if (m_ids.contains(fid)) continue;
        Function* f = m_doc->function(fid);

        m_ids.append(fid);
        m_labels.append(QString());
        m_icons.append(QString());

        QListWidgetItem* item = new QListWidgetItem(f ? f->name() : tr("ID %1").arg(fid));
        item->setData(Qt::UserRole,     QVariant::fromValue<quint32>(fid));
        item->setData(Qt::UserRole + 1, QString());
        item->setData(Qt::UserRole + 2, QString());
        m_listWidget->addItem(item);
    }
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotRemove()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    delete m_listWidget->takeItem(row);
    if (row < m_ids.size())
    {
        m_ids.removeAt(row);
        m_labels.removeAt(row);
        m_icons.removeAt(row);
    }
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotEditLabel()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    QListWidgetItem* item = m_listWidget->item(row);
    quint32 fid    = item->data(Qt::UserRole).toUInt();
    QString curLbl = item->data(Qt::UserRole + 1).toString();
    Function* f    = m_doc->function(fid);

    bool ok = false;
    QString newLbl = QInputDialog::getText(
        this, tr("Custom label"),
        tr("Label for \"%1\" (leave blank to use function name):").arg(f ? f->name() : tr("?")),
        QLineEdit::Normal, curLbl, &ok);

    if (!ok) return;

    item->setData(Qt::UserRole + 1, newLbl);
    QString display = newLbl.isEmpty()
        ? (f ? f->name() : tr("ID %1 (missing)").arg(fid))
        : QString("%1 (%2)").arg(newLbl, f ? f->name() : tr("?"));
    item->setText(display);

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotScribbleIcon()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    ScribbleDialog dlg(m_doc, this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString path = dlg.savedIconPath();
    if (path.isEmpty()) return;

    QListWidgetItem* item = m_listWidget->item(row);
    item->setData(Qt::UserRole + 2, path);
    item->setIcon(thumbIcon(path));

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotChooseIcon()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    QString formats;
    for (const QByteArray& ba : QImageReader::supportedImageFormats())
        formats += QString("*.%1 ").arg(QString(ba).toLower());

    QString cur = m_listWidget->item(row)->data(Qt::UserRole + 2).toString();
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select icon image"), cur,
        tr("Images (%1)").arg(formats));

    if (path.isEmpty()) return;

    QListWidgetItem* item = m_listWidget->item(row);
    item->setData(Qt::UserRole + 2, path);
    item->setIcon(thumbIcon(path));

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotClearIcon()
{
    int row = m_listWidget->currentRow();
    if (row < 0) return;

    QListWidgetItem* item = m_listWidget->item(row);
    item->setData(Qt::UserRole + 2, QString());
    item->setIcon(QIcon());

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
}

void MultiButtonConfigDialog::slotMoveUp()
{
    int row = m_listWidget->currentRow();
    if (row <= 0) return;

    QListWidgetItem* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row - 1, item);
    m_listWidget->setCurrentRow(row - 1);

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
    slotSelectionChanged();
}

void MultiButtonConfigDialog::slotMoveDown()
{
    int row = m_listWidget->currentRow();
    if (row < 0 || row >= m_listWidget->count() - 1) return;

    QListWidgetItem* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row + 1, item);
    m_listWidget->setCurrentRow(row + 1);

    m_ids    = listWidgetIds(m_listWidget);
    m_labels = listWidgetLabels(m_listWidget);
    m_icons  = listWidgetIcons(m_listWidget);
    slotSelectionChanged();
}

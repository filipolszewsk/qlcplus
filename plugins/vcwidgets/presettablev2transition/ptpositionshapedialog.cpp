/*
  ptpositionshapedialog.cpp
*/

#include "ptpositionshapedialog.h"

#include "ptdimmerwavecurvewidget.h"
#include "ptdimmerwaveengine.h"
#include "ptpositionpath2dwidget.h"
#include "ptshapesgallery.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

PTPositionShapeDialog::PTPositionShapeDialog(PTPositionMotion motion,
                                             const PTTransitionPreset& preset,
                                             const QVector<PTShapeGalleryItem>& gallery,
                                             QWidget* parent)
    : QDialog(parent)
    , m_motion(motion)
    , m_gallery(gallery.isEmpty() ? PTShapesGallery::defaultBuiltinItems() : gallery)
{
    setWindowTitle(motion == PTPositionMotion::Custom2D
            ? tr("Custom 2D motion path") : tr("Custom 1D motion curve"));
    resize(860, 520);

    QHBoxLayout* root = new QHBoxLayout(this);

    QGroupBox* galleryBox = new QGroupBox(tr("Shapes gallery"), this);
    QVBoxLayout* galleryLayout = new QVBoxLayout(galleryBox);
    m_galleryList = new QListWidget(galleryBox);
    m_galleryList->setViewMode(QListView::IconMode);
    m_galleryList->setIconSize(QSize(132, 58));
    m_galleryList->setGridSize(QSize(150, 86));
    m_galleryList->setResizeMode(QListView::Adjust);
    m_galleryList->setMovement(QListView::Static);
    galleryLayout->addWidget(m_galleryList, 1);
    QPushButton* useBtn = new QPushButton(tr("Use"), galleryBox);
    QPushButton* saveBtn = new QPushButton(tr("Save current..."), galleryBox);
    galleryLayout->addWidget(useBtn);
    galleryLayout->addWidget(saveBtn);
    root->addWidget(galleryBox);

    QVBoxLayout* editorCol = new QVBoxLayout();
    m_editorStack = new QStackedWidget(this);
    m_curve1D = new PTDimmerWaveCurveWidget(m_editorStack);
    m_curve1D->setEditable(true);
    PTDimmerWaveParams params;
    params.customCurveEnabled = true;
    params.customCurve = preset.positionMotionCurve.size() >= 2
            ? preset.positionMotionCurve : PTShapesGallery::defaultMotionCurve1D();
    m_curve1D->setParams(params);
    m_curve1D->setCustomCurve(params.customCurve);

    m_path2D = new PTPositionPath2DWidget(m_editorStack);
    m_path2D->setPath(preset.positionPath2D.size() >= 2
                              ? preset.positionPath2D
                              : PTShapesGallery::defaultMotionPath2D(),
                      preset.positionPath2DClosed);
    m_editorStack->addWidget(m_curve1D);
    m_editorStack->addWidget(m_path2D);
    editorCol->addWidget(m_editorStack, 1);

    m_closedChk = new QCheckBox(tr("Closed path"), this);
    m_closedChk->setChecked(preset.positionPath2DClosed);
    m_closedChk->setVisible(motion == PTPositionMotion::Custom2D);
    connect(m_closedChk, &QCheckBox::toggled, m_path2D, &PTPositionPath2DWidget::setPathClosed);
    editorCol->addWidget(m_closedChk);

    QDialogButtonBox* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    editorCol->addWidget(buttons);
    root->addLayout(editorCol, 1);

    if (motion == PTPositionMotion::Custom2D)
        m_editorStack->setCurrentWidget(m_path2D);
    else
        m_editorStack->setCurrentWidget(m_curve1D);

    connect(useBtn, &QPushButton::clicked, this, [this]() {
        const int idx = selectedGalleryIndex();
        if (idx >= 0 && idx < m_gallery.size())
            applyGalleryItem(m_gallery.at(idx));
    });
    connect(m_galleryList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        const int idx = selectedGalleryIndex();
        if (idx >= 0 && idx < m_gallery.size())
            applyGalleryItem(m_gallery.at(idx));
    });
    connect(saveBtn, &QPushButton::clicked, this, [this]() { saveCurrentToGallery(); });

    rebuildGalleryList();
}

QVector<PTCustomCurvePoint> PTPositionShapeDialog::motionCurve1D() const
{
    return m_curve1D ? m_curve1D->customCurve() : QVector<PTCustomCurvePoint>();
}

QVector<PTPositionPath2DPoint> PTPositionShapeDialog::motionPath2D() const
{
    return m_path2D ? m_path2D->path() : QVector<PTPositionPath2DPoint>();
}

bool PTPositionShapeDialog::path2DClosed() const
{
    return m_path2D ? m_path2D->pathClosed() : true;
}

void PTPositionShapeDialog::rebuildGalleryList()
{
    if (!m_galleryList)
        return;
    const int oldRow = m_galleryList->currentRow();
    m_galleryList->clear();

    const bool want2D = (m_motion == PTPositionMotion::Custom2D);
    for (const PTShapeGalleryItem& item : m_gallery)
    {
        if (want2D && item.kind != PTShapeGalleryKind::Path2D)
            continue;
        if (!want2D && item.kind != PTShapeGalleryKind::Curve1D)
            continue;

        const QPixmap thumb = item.kind == PTShapeGalleryKind::Path2D
                ? PTShapesGallery::renderPath2DThumbnail(item.path2D, item.path2DClosed,
                                                         QSize(132, 58))
                : PTShapesGallery::renderCurveThumbnail(item.curve1D, QSize(132, 58));
        new QListWidgetItem(QIcon(thumb), item.name, m_galleryList);
    }
    if (m_galleryList->count() > 0)
        m_galleryList->setCurrentRow(qBound(0, oldRow, m_galleryList->count() - 1));
}

int PTPositionShapeDialog::selectedGalleryIndex() const
{
    if (!m_galleryList || m_galleryList->currentRow() < 0)
        return -1;

    const bool want2D = (m_motion == PTPositionMotion::Custom2D);
    int visible = 0;
    for (int i = 0; i < m_gallery.size(); ++i)
    {
        const PTShapeGalleryItem& item = m_gallery.at(i);
        if (want2D && item.kind != PTShapeGalleryKind::Path2D)
            continue;
        if (!want2D && item.kind != PTShapeGalleryKind::Curve1D)
            continue;
        if (visible == m_galleryList->currentRow())
            return i;
        ++visible;
    }
    return -1;
}

void PTPositionShapeDialog::applyGalleryItem(const PTShapeGalleryItem& item)
{
    if (item.kind == PTShapeGalleryKind::Curve1D && m_curve1D)
        m_curve1D->setCustomCurve(item.curve1D);
    else if (item.kind == PTShapeGalleryKind::Path2D && m_path2D)
    {
        m_path2D->setPath(item.path2D, item.path2DClosed);
        if (m_closedChk)
            m_closedChk->setChecked(item.path2DClosed);
    }
}

void PTPositionShapeDialog::saveCurrentToGallery()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save shape"),
                                                 tr("Name:"), QLineEdit::Normal,
                                                 QString(), &ok).trimmed();
    if (!ok || name.isEmpty())
        return;

    PTShapeGalleryItem item;
    item.name = name;
    if (m_motion == PTPositionMotion::Custom2D)
    {
        item.kind = PTShapeGalleryKind::Path2D;
        item.path2D = motionPath2D();
        item.path2DClosed = path2DClosed();
    }
    else
    {
        item.kind = PTShapeGalleryKind::Curve1D;
        item.curve1D = motionCurve1D();
    }

    int existing = -1;
    for (int i = 0; i < m_gallery.size(); ++i)
    {
        if (m_gallery.at(i).name.compare(name, Qt::CaseInsensitive) == 0)
        {
            existing = i;
            break;
        }
    }
    if (existing >= 0)
    {
        if (QMessageBox::question(this, tr("Replace shape"),
                                  tr("Replace existing \"%1\"?").arg(m_gallery.at(existing).name))
                != QMessageBox::Yes)
            return;
        m_gallery[existing] = item;
    }
    else
    {
        m_gallery.append(item);
    }
    rebuildGalleryList();
}

#include "ptcustomcurvedialog.h"

#include "ptdimmerwavecurvewidget.h"
#include "ptdimmerwaveengine.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSize>
#include <QVBoxLayout>
#include <algorithm>

namespace {

QPointF thumbPoint(const QRectF& r, double xDeg, double yValue)
{
    return QPointF(r.left() + qBound(0.0, xDeg, 360.0) / 360.0 * r.width(),
                   r.bottom() - qBound(0.0, yValue, 255.0) / 255.0 * r.height());
}

QPixmap renderCurveThumbnail(const QVector<PTCustomCurvePoint>& points, const QSize& size)
{
    QPixmap pix(size);
    pix.fill(QColor(30, 30, 30));

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = pix.rect().adjusted(6, 6, -6, -6);
    p.setPen(QPen(QColor(70, 70, 70), 1));
    for (int i = 0; i <= 4; ++i)
    {
        const qreal x = r.left() + r.width() * i / 4.0;
        p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
    }
    for (int i = 0; i <= 2; ++i)
    {
        const qreal y = r.top() + r.height() * i / 2.0;
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }

    if (points.size() >= 2)
    {
        QVector<PTCustomCurvePoint> sorted = points;
        std::sort(sorted.begin(), sorted.end(),
                  [](const PTCustomCurvePoint& a, const PTCustomCurvePoint& b) {
                      return a.xDeg < b.xDeg;
                  });
        QPainterPath path;
        path.moveTo(thumbPoint(r, sorted.first().xDeg, sorted.first().yValue));
        for (int i = 0; i < sorted.size() - 1; ++i)
        {
            const PTCustomCurvePoint& a = sorted.at(i);
            const PTCustomCurvePoint& b = sorted.at(i + 1);
            if (a.segmentMode == PTCustomCurvePoint::Linear)
                path.lineTo(thumbPoint(r, b.xDeg, b.yValue));
            else
                path.cubicTo(thumbPoint(r, a.rightHandleXDeg, a.rightHandleYValue),
                             thumbPoint(r, b.leftHandleXDeg, b.leftHandleYValue),
                             thumbPoint(r, b.xDeg, b.yValue));
        }
        p.setPen(QPen(QColor(235, 235, 235), 3));
        p.drawPath(path);
    }
    p.setPen(QPen(QColor(115, 115, 115), 1));
    p.drawRect(pix.rect().adjusted(0, 0, -1, -1));
    return pix;
}

} // namespace

PTCustomCurveDialog::PTCustomCurveDialog(const PTTransitionPreset& preset,
                                         const QVector<PTCustomCurveGalleryItem>& gallery,
                                         QWidget* parent)
    : QDialog(parent)
    , m_gallery(gallery)
{
    setWindowTitle(tr("Custom wave shape"));
    setMinimumSize(900, 420);

    QVBoxLayout* root = new QVBoxLayout(this);

    QLabel* hint = new QLabel(tr("Double-click to add a point. Drag points or handles to shape the curve. "
                                 "Delete removes the selected middle point only."), this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    QHBoxLayout* content = new QHBoxLayout;

    m_curve = new PTDimmerWaveCurveWidget(this);
    m_curve->setMinimumHeight(260);
    PTTransitionPreset editablePreset = preset;
    editablePreset.customCurveEnabled = true;
    PTDimmerWaveParams params = PTDimmerWaveEngine::paramsFromPreset(editablePreset, nullptr);
    params.customCurveEnabled = true;
    m_curve->setParams(params);
    m_curve->setEditable(true);
    m_curve->setCustomCurve(editablePreset.customCurve);
    content->addWidget(m_curve, 1);

    QGroupBox* galleryBox = new QGroupBox(tr("Gallery"), this);
    QVBoxLayout* galleryLayout = new QVBoxLayout(galleryBox);
    m_galleryList = new QListWidget(galleryBox);
    m_galleryList->setViewMode(QListView::IconMode);
    m_galleryList->setIconSize(QSize(132, 58));
    m_galleryList->setGridSize(QSize(150, 86));
    m_galleryList->setResizeMode(QListView::Adjust);
    m_galleryList->setMovement(QListView::Static);
    m_galleryList->setUniformItemSizes(true);
    galleryLayout->addWidget(m_galleryList, 1);
    QPushButton* useButton = new QPushButton(tr("Use"), galleryBox);
    QPushButton* saveButton = new QPushButton(tr("Save current..."), galleryBox);
    QPushButton* deleteButton = new QPushButton(tr("Delete"), galleryBox);
    galleryLayout->addWidget(useButton);
    galleryLayout->addWidget(saveButton);
    galleryLayout->addWidget(deleteButton);
    content->addWidget(galleryBox);
    root->addLayout(content, 1);

    QHBoxLayout* commandRow = new QHBoxLayout;
    QPushButton* resetButton = new QPushButton(tr("Reset"), this);
    m_deletePointButton = new QPushButton(tr("Delete point"), this);
    commandRow->addWidget(resetButton);
    commandRow->addWidget(m_deletePointButton);
    commandRow->addStretch(1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    commandRow->addWidget(buttons);
    root->addLayout(commandRow);

    connect(resetButton, &QPushButton::clicked, m_curve, &PTDimmerWaveCurveWidget::resetCustomCurve);
    connect(m_deletePointButton, &QPushButton::clicked, m_curve, [this]() {
        m_curve->deleteSelectedPoint();
    });
    connect(useButton, &QPushButton::clicked, this, [this]() {
        const int idx = selectedGalleryIndex();
        if (idx >= 0 && idx < m_gallery.size())
            m_curve->setCustomCurve(m_gallery.at(idx).points);
    });
    connect(m_galleryList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        const int idx = selectedGalleryIndex();
        if (idx >= 0 && idx < m_gallery.size())
            m_curve->setCustomCurve(m_gallery.at(idx).points);
    });
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Save custom curve"),
                                                   tr("Name:"), QLineEdit::Normal,
                                                   QString(), &ok).trimmed();
        if (!ok || name.isEmpty())
            return;

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
            const int answer = QMessageBox::question(
                    this, tr("Overwrite custom curve"),
                    tr("Replace existing curve \"%1\"?").arg(m_gallery.at(existing).name),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
            m_gallery[existing].name = name;
            m_gallery[existing].points = m_curve->customCurve();
        }
        else
        {
            PTCustomCurveGalleryItem item;
            item.name = name;
            item.points = m_curve->customCurve();
            m_gallery.append(item);
        }
        rebuildGalleryList();
    });
    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        const int idx = selectedGalleryIndex();
        if (idx < 0 || idx >= m_gallery.size())
            return;
        m_gallery.remove(idx);
        rebuildGalleryList();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    rebuildGalleryList();
}

QVector<PTCustomCurvePoint> PTCustomCurveDialog::customCurve() const
{
    return m_curve ? m_curve->customCurve() : QVector<PTCustomCurvePoint>();
}

void PTCustomCurveDialog::rebuildGalleryList()
{
    if (!m_galleryList)
        return;
    const int oldRow = m_galleryList->currentRow();
    m_galleryList->clear();
    for (const PTCustomCurveGalleryItem& item : m_gallery)
    {
        QListWidgetItem* row = new QListWidgetItem(
                QIcon(renderCurveThumbnail(item.points, QSize(132, 58))), item.name, m_galleryList);
        row->setTextAlignment(Qt::AlignCenter);
    }
    if (!m_gallery.isEmpty())
        m_galleryList->setCurrentRow(qBound(0, oldRow, m_gallery.size() - 1));
}

int PTCustomCurveDialog::selectedGalleryIndex() const
{
    return m_galleryList ? m_galleryList->currentRow() : -1;
}

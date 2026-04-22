/*
  Q Light Controller Plus
  scribbledialog.cpp

  Copyright (c) QLC+ contributors

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QListWidget>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDateTime>
#include <QPainter>
#include <QSlider>
#include <QLabel>
#include <QFrame>
#include <QMenu>
#include <QFileInfo>
#include <QEvent>
#include <QFile>
#include <QDir>

#include "scribbledialog.h"
#include "doc.h"
#include "qlcfile.h"
#include "qlcconfig.h"

/*****************************************************************************
 * ScribbleArea
 *****************************************************************************/

ScribbleArea::ScribbleArea(QWidget *parent)
    : QWidget(parent)
    , m_penColor(Qt::white)
    , m_penWidth(4)
    , m_erasing(false)
    , m_drawing(false)
    , m_straightLineMode(false)
{
    /* Size is managed externally by the container via event filter */
    setCursor(Qt::CrossCursor);

    /* Create a transparent canvas at default size */
    m_image = QImage(DEFAULT_CANVAS_SIZE, DEFAULT_CANVAS_SIZE, QImage::Format_ARGB32);
    m_image.fill(Qt::transparent);
}

ScribbleArea::~ScribbleArea()
{
}

QImage ScribbleArea::image() const
{
    return m_image;
}

void ScribbleArea::loadImage(const QImage &img)
{
    pushUndoState();
    m_image = img.convertToFormat(QImage::Format_ARGB32);
    update();
}

void ScribbleArea::clear()
{
    pushUndoState();
    m_image.fill(Qt::transparent);
    update();
}

void ScribbleArea::undo()
{
    if (m_undoStack.isEmpty())
        return;

    m_image = m_undoStack.pop();
    update();
    emit canUndoChanged(!m_undoStack.isEmpty());
}

bool ScribbleArea::canUndo() const
{
    return !m_undoStack.isEmpty();
}

void ScribbleArea::setPenColor(const QColor &color)
{
    m_penColor = color;
}

QColor ScribbleArea::penColor() const
{
    return m_penColor;
}

void ScribbleArea::setPenWidth(int width)
{
    m_penWidth = width;
}

int ScribbleArea::penWidth() const
{
    return m_penWidth;
}

void ScribbleArea::setEraserMode(bool eraser)
{
    m_erasing = eraser;
}

bool ScribbleArea::isEraserMode() const
{
    return m_erasing;
}

QPoint ScribbleArea::mapToImage(const QPoint &widgetPos) const
{
    if (m_image.isNull() || width() == 0 || height() == 0)
        return widgetPos;

    qreal scaleX = (qreal)m_image.width() / (qreal)width();
    qreal scaleY = (qreal)m_image.height() / (qreal)height();
    return QPoint(qRound(widgetPos.x() * scaleX), qRound(widgetPos.y() * scaleY));
}

void ScribbleArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPoint imgPos = mapToImage(event->pos());

        pushUndoState();
        m_drawing = true;
        m_lastPoint = imgPos;
        m_strokeStartPoint = imgPos;

        /* Check if SHIFT is held for straight line mode */
        m_straightLineMode = (event->modifiers() & Qt::ShiftModifier);

        if (m_straightLineMode)
        {
            /* Save image state before stroke for rubber-band preview */
            m_imageBeforeStroke = m_image.copy();
        }
        else
        {
            /* Draw a dot at the click position */
            QPainter painter(&m_image);
            painter.setRenderHint(QPainter::Antialiasing, true);

            if (m_erasing)
            {
                painter.setCompositionMode(QPainter::CompositionMode_Clear);
                painter.setPen(QPen(Qt::transparent, m_penWidth, Qt::SolidLine, Qt::RoundCap));
            }
            else
            {
                painter.setPen(QPen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap));
            }
            painter.drawPoint(imgPos);
        }
        update();
    }
}

void ScribbleArea::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drawing && (event->buttons() & Qt::LeftButton))
    {
        QPoint imgPos = mapToImage(event->pos());

        if (m_straightLineMode)
            drawStraightLinePreview(imgPos);
        else
            drawLineTo(imgPos);
    }
}

void ScribbleArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_drawing)
    {
        QPoint imgPos = mapToImage(event->pos());

        if (m_straightLineMode)
        {
            /* Final commit of the straight line */
            drawStraightLinePreview(imgPos);
        }
        else
        {
            drawLineTo(imgPos);
        }
        m_drawing = false;
        m_straightLineMode = false;
    }
}

void ScribbleArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    /* Draw a dark checkerboard background so light colours (e.g. white) remain visible */
    const int gridSize = 8;
    for (int y = 0; y < height(); y += gridSize)
    {
        for (int x = 0; x < width(); x += gridSize)
        {
            bool light = ((x / gridSize) + (y / gridSize)) % 2 == 0;
            painter.fillRect(x, y, gridSize, gridSize,
                             light ? QColor(60, 60, 60) : QColor(45, 45, 45));
        }
    }

    /* Draw the image scaled to the widget size (zoom view) */
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRect(0, 0, width(), height()), m_image);

    /* Draw a border */
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void ScribbleArea::drawLineTo(const QPoint &endPoint)
{
    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_erasing)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setPen(QPen(Qt::transparent, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    else
    {
        painter.setPen(QPen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }

    painter.drawLine(m_lastPoint, endPoint);
    m_lastPoint = endPoint;
    update();
}

void ScribbleArea::drawStraightLinePreview(const QPoint &endPoint)
{
    /* Restore the image to state before this stroke, then draw a single line */
    m_image = m_imageBeforeStroke.copy();

    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_erasing)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setPen(QPen(Qt::transparent, m_penWidth, Qt::SolidLine, Qt::RoundCap));
    }
    else
    {
        painter.setPen(QPen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap));
    }

    painter.drawLine(m_strokeStartPoint, endPoint);
    update();
}

void ScribbleArea::pushUndoState()
{
    m_undoStack.push(m_image.copy());

    /* Limit undo stack size */
    while (m_undoStack.size() > MAX_UNDO)
        m_undoStack.removeFirst();

    emit canUndoChanged(true);
}

/*****************************************************************************
 * ScribbleDialog
 *****************************************************************************/

ScribbleDialog::ScribbleDialog(Doc *doc, QWidget *parent)
    : QDialog(parent)
    , m_doc(doc)
{
    setWindowTitle(tr("Scribble Icon"));
    setMinimumSize(400, 500);
    resize(520, 650);

    /* Force dark theme regardless of system light/dark mode */
    setStyleSheet(
        "QDialog { background-color: #2b2b2b; color: #e0e0e0; } "
        "QLabel { color: #e0e0e0; } "
        "QToolButton { color: #e0e0e0; background-color: #3c3c3c; border: 1px solid #555; "
        "              border-radius: 3px; padding: 2px 4px; } "
        "QToolButton:hover { background-color: #4c4c4c; } "
        "QToolButton:checked { background-color: #5a5a5a; border: 2px solid #888; } "
        "QPushButton { color: #e0e0e0; background-color: #3c3c3c; border: 1px solid #555; "
        "              border-radius: 3px; padding: 4px 8px; } "
        "QPushButton:hover { background-color: #4c4c4c; } "
        "QPushButton:default { border-color: #888; } "
        "QSlider::groove:horizontal { background: #555; height: 4px; border-radius: 2px; } "
        "QSlider::handle:horizontal { background: #aaa; width: 14px; height: 14px; "
        "                             border-radius: 7px; margin: -5px 0; } "
        "QListWidget { background-color: #1e1e1e; color: #e0e0e0; border: 1px solid #555; } "
        "QListWidget::item:selected { background-color: #3a5a8a; } "
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    /* ---- Toolbar ---- */
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    /* Color button */
    m_colorButton = new QToolButton(this);
    m_colorButton->setText(tr("Color..."));
    m_colorButton->setToolTip(tr("Choose custom pen color"));
    m_colorButton->setIconSize(QSize(24, 24));
    m_colorButton->setMinimumSize(70, 32);
    toolbarLayout->addWidget(m_colorButton);
    connect(m_colorButton, SIGNAL(clicked()), this, SLOT(slotChooseColor()));

    /* Width label */
    QLabel *widthTitle = new QLabel(tr("Width:"), this);
    toolbarLayout->addWidget(widthTitle);

    /* Width slider */
    m_widthSlider = new QSlider(Qt::Horizontal, this);
    m_widthSlider->setRange(1, 30);
    m_widthSlider->setValue(4);
    m_widthSlider->setMaximumWidth(120);
    toolbarLayout->addWidget(m_widthSlider);
    connect(m_widthSlider, SIGNAL(valueChanged(int)), this, SLOT(slotPenWidthChanged(int)));

    /* Width value label */
    m_widthLabel = new QLabel("4", this);
    m_widthLabel->setMinimumWidth(20);
    toolbarLayout->addWidget(m_widthLabel);

    /* Separator */
    toolbarLayout->addSpacing(8);

    /* Eraser button */
    m_eraserButton = new QToolButton(this);
    m_eraserButton->setText(tr("Eraser"));
    m_eraserButton->setToolTip(tr("Toggle eraser mode"));
    m_eraserButton->setCheckable(true);
    m_eraserButton->setMinimumSize(60, 32);
    toolbarLayout->addWidget(m_eraserButton);
    connect(m_eraserButton, SIGNAL(toggled(bool)), this, SLOT(slotEraserToggled(bool)));

    /* Clear button */
    m_clearButton = new QToolButton(this);
    m_clearButton->setText(tr("Clear"));
    m_clearButton->setToolTip(tr("Clear the canvas"));
    m_clearButton->setMinimumSize(50, 32);
    toolbarLayout->addWidget(m_clearButton);
    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(slotClear()));

    /* Undo button */
    m_undoButton = new QToolButton(this);
    m_undoButton->setText(tr("Undo"));
    m_undoButton->setToolTip(tr("Undo last stroke"));
    m_undoButton->setEnabled(false);
    m_undoButton->setMinimumSize(50, 32);
    toolbarLayout->addWidget(m_undoButton);
    connect(m_undoButton, SIGNAL(clicked()), this, SLOT(slotUndo()));

    toolbarLayout->addStretch();

    /* Shift hint label */
    QLabel *shiftHint = new QLabel(tr("Hold SHIFT for straight line"), this);
    shiftHint->setStyleSheet("color: #888; font-size: 10px;");
    toolbarLayout->addWidget(shiftHint);

    mainLayout->addLayout(toolbarLayout);

    /* ---- Quick colour swatches ---- */
    QHBoxLayout *swatchLayout = new QHBoxLayout();
    swatchLayout->setSpacing(4);

    QLabel *swatchLabel = new QLabel(tr("Color:"), this);
    swatchLabel->setStyleSheet("color: #aaa; font-size: 10px;");
    swatchLayout->addWidget(swatchLabel);

    const QList<QColor> presetColors = {
        Qt::white, QColor(192, 192, 192), QColor(128, 128, 128), Qt::black,
        Qt::red, QColor(255, 128, 0), Qt::yellow, Qt::green,
        Qt::cyan, Qt::blue, QColor(128, 0, 255), QColor(255, 0, 128)
    };

    for (const QColor &c : presetColors)
    {
        QToolButton *swatch = new QToolButton(this);
        swatch->setFixedSize(24, 24);
        swatch->setToolTip(c.name());
        swatch->setStyleSheet(
            QString("QToolButton { background-color: %1; border: 1px solid #222; border-radius: 3px; } "
                    "QToolButton:hover { border: 2px solid #fff; }").arg(c.name())
        );
        connect(swatch, &QToolButton::clicked, this, [this, c]() { slotColorSwatch(c); });
        swatchLayout->addWidget(swatch);
    }
    swatchLayout->addStretch();

    mainLayout->addLayout(swatchLayout);

    /* ---- Canvas in a square-enforcing container ---- */
    m_canvasContainer = new QWidget(this);
    m_canvasContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout *containerLayout = new QHBoxLayout(m_canvasContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setAlignment(Qt::AlignCenter);

    m_scribbleArea = new ScribbleArea(m_canvasContainer);
    m_scribbleArea->setFixedSize(256, 256);
    containerLayout->addWidget(m_scribbleArea);

    mainLayout->addWidget(m_canvasContainer, 1); /* stretch factor 1 = takes all available space */

    /* Event filter on the container enforces the square constraint */
    m_canvasContainer->installEventFilter(this);

    connect(m_scribbleArea, SIGNAL(canUndoChanged(bool)),
            this, SLOT(slotUndoAvailableChanged(bool)));

    /* ---- Scribble Library ---- */
    m_libraryLabel = new QLabel(tr("Previous scribbles (click = use, double-click = edit):"), this);
    mainLayout->addWidget(m_libraryLabel);

    m_libraryList = new QListWidget(this);
    m_libraryList->setViewMode(QListView::IconMode);
    m_libraryList->setIconSize(QSize(48, 48));
    m_libraryList->setFlow(QListView::LeftToRight);
    m_libraryList->setWrapping(true);
    m_libraryList->setResizeMode(QListView::Adjust);
    m_libraryList->setSpacing(4);
    m_libraryList->setFixedHeight(90);
    m_libraryList->setMovement(QListView::Static);
    m_libraryList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_libraryList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_libraryList);
    connect(m_libraryList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(slotLibraryItemClicked(QListWidgetItem*)));
    connect(m_libraryList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(slotLibraryItemDoubleClicked(QListWidgetItem*)));
    connect(m_libraryList, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotLibraryContextMenu(QPoint)));

    /* Populate library from workspace scribbles/ folder */
    populateLibrary();

    /* ---- Dialog buttons ---- */
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *saveIconBtn = new QPushButton(tr("Save as Icon"), this);
    saveIconBtn->setToolTip(tr("Save to project scribbles folder and set as button icon"));
    saveIconBtn->setDefault(true);
    buttonLayout->addWidget(saveIconBtn);
    connect(saveIconBtn, SIGNAL(clicked()), this, SLOT(slotSaveAsIcon()));

    QPushButton *saveFileBtn = new QPushButton(tr("Save to File..."), this);
    saveFileBtn->setToolTip(tr("Save to a custom location and set as button icon"));
    buttonLayout->addWidget(saveFileBtn);
    connect(saveFileBtn, SIGNAL(clicked()), this, SLOT(slotSaveToFile()));

    QPushButton *cancelBtn = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(cancelBtn);
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));

    mainLayout->addLayout(buttonLayout);

    /* Update color button appearance to show the initial pen color */
    updateColorButtonStyle();
}

ScribbleDialog::~ScribbleDialog()
{
}

bool ScribbleDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_canvasContainer && event->type() == QEvent::Resize)
    {
        QResizeEvent *re = static_cast<QResizeEvent*>(event);
        int side = qMin(re->size().width(), re->size().height());
        if (side < 128)
            side = 128;
        m_scribbleArea->setFixedSize(side, side);
    }
    return QDialog::eventFilter(obj, event);
}

QString ScribbleDialog::savedIconPath() const
{
    return m_savedPath;
}

void ScribbleDialog::slotChooseColor()
{
    QColor color = QColorDialog::getColor(m_scribbleArea->penColor(), this, tr("Pen Color"));
    if (color.isValid())
    {
        m_scribbleArea->setPenColor(color);
        updateColorButtonStyle();
    }
}

void ScribbleDialog::slotPenWidthChanged(int value)
{
    m_scribbleArea->setPenWidth(value);
    m_widthLabel->setText(QString::number(value));
}

void ScribbleDialog::slotEraserToggled(bool checked)
{
    m_scribbleArea->setEraserMode(checked);
}

void ScribbleDialog::slotClear()
{
    m_scribbleArea->clear();
}

void ScribbleDialog::slotUndo()
{
    m_scribbleArea->undo();
}

void ScribbleDialog::slotUndoAvailableChanged(bool available)
{
    m_undoButton->setEnabled(available);
}

void ScribbleDialog::slotSaveAsIcon()
{
    QString dir = scribbleDirectory();
    if (dir.isEmpty())
    {
        slotSaveToFile();
        return;
    }

    QString path = saveImage(dir);
    if (path.isEmpty() == false)
    {
        m_savedPath = path;
        accept();
    }
}

void ScribbleDialog::slotSaveToFile()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Scribble Icon"),
                                                 QString(), tr("PNG Image (*.png)"));
    if (path.isEmpty())
        return;

    /* Ensure .png extension */
    if (!path.endsWith(".png", Qt::CaseInsensitive))
        path.append(".png");

    QImage img = m_scribbleArea->image();
    if (img.save(path, "PNG"))
    {
        m_savedPath = path;
        accept();
    }
    else
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to save the image to:\n%1").arg(path));
    }
}

void ScribbleDialog::slotLibraryItemClicked(QListWidgetItem *item)
{
    if (item == nullptr)
        return;

    /* Single click = use this existing scribble as the icon directly */
    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty() == false)
    {
        m_savedPath = path;
        accept();
    }
}

void ScribbleDialog::slotLibraryItemDoubleClicked(QListWidgetItem *item)
{
    if (item == nullptr)
        return;

    /* Double click = load the image into the canvas for editing */
    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;

    QImage img(path);
    if (img.isNull() == false)
        m_scribbleArea->loadImage(img);
}

void ScribbleDialog::slotLibraryContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_libraryList->itemAt(pos);
    if (item == nullptr)
        return;

    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;

    QMenu menu(this);
    QAction *useAction = menu.addAction(QIcon(":/image.png"), tr("Use as Icon"));
    QAction *editAction = menu.addAction(QIcon(":/edit.png"), tr("Load into Canvas"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(QIcon(":/editdelete.png"), tr("Delete"));

    QAction *chosen = menu.exec(m_libraryList->viewport()->mapToGlobal(pos));
    if (chosen == nullptr)
        return;

    if (chosen == useAction)
    {
        m_savedPath = path;
        accept();
    }
    else if (chosen == editAction)
    {
        QImage img(path);
        if (img.isNull() == false)
            m_scribbleArea->loadImage(img);
    }
    else if (chosen == deleteAction)
    {
        QFileInfo fi(path);
        int ret = QMessageBox::question(this, tr("Delete Scribble"),
                    tr("Delete \"%1\" from disk?\nThis cannot be undone.").arg(fi.fileName()),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
        {
            if (QFile::remove(path))
            {
                delete m_libraryList->takeItem(m_libraryList->row(item));

                /* Hide library if empty */
                if (m_libraryList->count() == 0)
                {
                    m_libraryLabel->hide();
                    m_libraryList->hide();
                }
            }
            else
            {
                QMessageBox::warning(this, tr("Error"),
                                     tr("Failed to delete:\n%1").arg(path));
            }
        }
    }
}

QString ScribbleDialog::scribbleDirectory() const
{
    QDir dir = QLCFile::userDirectory(QString(USERSCRIBBLEDIR),
                                      QString(USERSCRIBBLEDIR),
                                      QStringList() << "*.png" << "*.PNG");
    return dir.absolutePath();
}

QString ScribbleDialog::legacyScribbleDirectory() const
{
    if (m_doc == nullptr)
        return QString();

    QString workspace = m_doc->workspacePath();
    if (workspace.isEmpty())
        return QString();

    return workspace + QDir::separator() + "scribbles";
}

QString ScribbleDialog::saveImage(const QString &directory)
{
    /* Create directory if it doesn't exist */
    QDir dir(directory);
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to create directory:\n%1").arg(directory));
            return QString();
        }
    }

    /* Generate unique filename */
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString filename = QString("scribble_%1.png").arg(timestamp);
    QString fullPath = directory + QDir::separator() + filename;

    QImage img = m_scribbleArea->image();
    if (img.save(fullPath, "PNG"))
        return fullPath;

    QMessageBox::warning(this, tr("Error"),
                         tr("Failed to save the image to:\n%1").arg(fullPath));
    return QString();
}

void ScribbleDialog::updateColorButtonStyle()
{
    QColor c = m_scribbleArea->penColor();
    double luminance = 0.299 * c.redF() + 0.587 * c.greenF() + 0.114 * c.blueF();
    QString textColor = (luminance > 0.45) ? "#000000" : "#ffffff";
    QString style = QString(
        "QToolButton { background-color: %1; color: %2; border: 2px solid #555; "
        "border-radius: 4px; min-width: 70px; min-height: 24px; } "
        "QToolButton:hover { border-color: #aaa; }")
        .arg(c.name()).arg(textColor);
    m_colorButton->setStyleSheet(style);
}

void ScribbleDialog::slotColorSwatch(const QColor &color)
{
    m_scribbleArea->setPenColor(color);
    updateColorButtonStyle();
}

void ScribbleDialog::populateLibrary()
{
    m_libraryList->clear();

    QStringList allFiles;
    QStringList filters;
    filters << "*.png" << "*.PNG";

    /* Primary: user-global scribbles directory */
    QString userDir = scribbleDirectory();
    if (!userDir.isEmpty())
    {
        QDir d(userDir);
        foreach (const QString &f, d.entryList(filters, QDir::Files, QDir::Time))
            allFiles << d.absoluteFilePath(f);
    }

    /* Legacy: project-relative scribbles/ folder (backward compatibility) */
    QString legacy = legacyScribbleDirectory();
    if (!legacy.isEmpty())
    {
        QDir d(legacy);
        if (d.exists())
        {
            foreach (const QString &f, d.entryList(filters, QDir::Files, QDir::Time))
            {
                QString fp = d.absoluteFilePath(f);
                if (!allFiles.contains(fp))
                    allFiles << fp;
            }
        }
    }

    if (allFiles.isEmpty())
    {
        m_libraryLabel->hide();
        m_libraryList->hide();
        return;
    }

    m_libraryLabel->show();
    m_libraryList->show();

    foreach (const QString &fullPath, allFiles)
    {
        QPixmap pix(fullPath);
        if (pix.isNull())
            continue;

        QListWidgetItem *item = new QListWidgetItem();
        item->setIcon(QIcon(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        item->setToolTip(QFileInfo(fullPath).fileName());
        item->setData(Qt::UserRole, fullPath);
        m_libraryList->addItem(item);
    }
}

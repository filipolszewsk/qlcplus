/*
  Q Light Controller Plus
  scribbledialog.h

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

#ifndef SCRIBBLEDIALOG_H
#define SCRIBBLEDIALOG_H

#include <QDialog>
#include <QImage>
#include <QStack>
#include <QColor>
#include <QPoint>

class QToolButton;
class QListWidget;
class QListWidgetItem;
class QSlider;
class QLabel;

class Doc;

/*****************************************************************************
 * ScribbleArea - drawing canvas widget
 *****************************************************************************/

class ScribbleArea : public QWidget
{
    Q_OBJECT

public:
    ScribbleArea(QWidget *parent = nullptr);
    ~ScribbleArea();

    /** Get the current canvas image */
    QImage image() const;

    /** Load an existing image into the canvas (for editing) */
    void loadImage(const QImage &img);

    /** Clear the canvas to transparent */
    void clear();

    /** Undo the last stroke */
    void undo();

    /** Check if undo is available */
    bool canUndo() const;

    /** Set the pen color */
    void setPenColor(const QColor &color);
    QColor penColor() const;

    /** Set the pen width */
    void setPenWidth(int width);
    int penWidth() const;

    /** Toggle eraser mode */
    void setEraserMode(bool eraser);
    bool isEraserMode() const;

signals:
    void canUndoChanged(bool canUndo);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    /** Map a widget-space point to image-space coordinates */
    QPoint mapToImage(const QPoint &widgetPos) const;

    void drawLineTo(const QPoint &endPoint);
    void drawStraightLinePreview(const QPoint &endPoint);
    void pushUndoState();

    QImage m_image;
    QStack<QImage> m_undoStack;
    QColor m_penColor;
    int m_penWidth;
    bool m_erasing;
    bool m_drawing;
    QPoint m_lastPoint;        /**< in image coordinates */

    /* Straight line mode (hold SHIFT) */
    bool m_straightLineMode;
    QPoint m_strokeStartPoint; /**< in image coordinates */
    QImage m_imageBeforeStroke;

    static const int DEFAULT_CANVAS_SIZE = 256;
    static const int MAX_UNDO = 30;
};

/*****************************************************************************
 * ScribbleDialog - main dialog
 *****************************************************************************/

class ScribbleDialog : public QDialog
{
    Q_OBJECT

public:
    ScribbleDialog(Doc *doc, QWidget *parent = nullptr);
    ~ScribbleDialog();

    /** Get the path where the icon was saved (empty if cancelled) */
    QString savedIconPath() const;

protected:
    /** Event filter to enforce square canvas inside its container */
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void slotChooseColor();
    void slotPenWidthChanged(int value);
    void slotEraserToggled(bool checked);
    void slotClear();
    void slotUndo();
    void slotUndoAvailableChanged(bool available);
    void slotSaveAsIcon();
    void slotSaveToFile();
    void slotLibraryItemClicked(QListWidgetItem *item);
    void slotLibraryItemDoubleClicked(QListWidgetItem *item);
    void slotLibraryContextMenu(const QPoint &pos);

private:
    /** Returns the user-global scribbles directory (creates it if needed) */
    QString scribbleDirectory() const;

    /** Returns the legacy project-relative scribbles directory (for backward compatibility) */
    QString legacyScribbleDirectory() const;

    /** Save the image and return the path, or empty on failure */
    QString saveImage(const QString &directory);

    /** Update the color button background to reflect current pen color */
    void updateColorButtonStyle();

    /** Scan scribbles/ directory and populate the library list widget */
    void populateLibrary();

    ScribbleArea *m_scribbleArea;
    QWidget *m_canvasContainer;
    Doc *m_doc;
    QString m_savedPath;

    QToolButton *m_colorButton;
    QToolButton *m_eraserButton;
    QToolButton *m_clearButton;
    QToolButton *m_undoButton;
    QSlider *m_widthSlider;
    QLabel *m_widthLabel;

    /* Scribble library */
    QLabel *m_libraryLabel;
    QListWidget *m_libraryList;
};

#endif // SCRIBBLEDIALOG_H

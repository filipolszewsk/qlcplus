#pragma once

#include "presettablev2effectengine.h"

#include <QDialog>

class PTDimmerWaveCurveWidget;
class QListWidget;
class QPushButton;

class PTCustomCurveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PTCustomCurveDialog(const PTTransitionPreset& preset,
                                 const QVector<PTCustomCurveGalleryItem>& gallery,
                                 QWidget* parent = nullptr);

    QVector<PTCustomCurvePoint> customCurve() const;
    QVector<PTCustomCurveGalleryItem> gallery() const { return m_gallery; }

private:
    void rebuildGalleryList();
    int selectedGalleryIndex() const;

    PTDimmerWaveCurveWidget* m_curve = nullptr;
    QListWidget* m_galleryList = nullptr;
    QPushButton* m_deletePointButton = nullptr;
    QVector<PTCustomCurveGalleryItem> m_gallery;
};

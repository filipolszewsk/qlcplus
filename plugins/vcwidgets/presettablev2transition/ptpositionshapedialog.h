/*
  ptpositionshapedialog.h — unified 1D/2D motion shape editor with gallery
*/

#pragma once

#include "presettablev2effectengine.h"

#include <QDialog>
#include <QVector>

class QListWidget;
class PTDimmerWaveCurveWidget;
class PTPositionPath2DWidget;
class QStackedWidget;
class QCheckBox;

class PTPositionShapeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PTPositionShapeDialog(PTPositionMotion motion,
                                   const PTTransitionPreset& preset,
                                   const QVector<PTShapeGalleryItem>& gallery,
                                   QWidget* parent = nullptr);

    QVector<PTCustomCurvePoint> motionCurve1D() const;
    QVector<PTPositionPath2DPoint> motionPath2D() const;
    bool path2DClosed() const;
    QVector<PTShapeGalleryItem> gallery() const { return m_gallery; }

private:
    void rebuildGalleryList();
    int selectedGalleryIndex() const;
    void applyGalleryItem(const PTShapeGalleryItem& item);
    void saveCurrentToGallery();

    PTPositionMotion m_motion;
    QListWidget* m_galleryList = nullptr;
    QStackedWidget* m_editorStack = nullptr;
    PTDimmerWaveCurveWidget* m_curve1D = nullptr;
    PTPositionPath2DWidget* m_path2D = nullptr;
    QCheckBox* m_closedChk = nullptr;
    QVector<PTShapeGalleryItem> m_gallery;
};

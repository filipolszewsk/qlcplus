/*
  ptshapesgallery.h — built-in 1D/2D shape presets and serialization helpers
*/

#pragma once

#include "presettablev2effectengine.h"

#include <QPixmap>
#include <QSize>
#include <QString>
#include <QVector>

class PTShapesGallery
{
public:
    static QVector<PTShapeGalleryItem> defaultBuiltinItems();
    static QVector<PTCustomCurvePoint> defaultMotionCurve1D();
    static QVector<PTPositionPath2DPoint> defaultMotionPath2D();

    static QString serializePath2D(const QVector<PTPositionPath2DPoint>& points);
    static QVector<PTPositionPath2DPoint> parsePath2D(const QString& text);

    static QPixmap renderCurveThumbnail(const QVector<PTCustomCurvePoint>& points, const QSize& size);
    static QPixmap renderPath2DThumbnail(const QVector<PTPositionPath2DPoint>& points,
                                         bool closed, const QSize& size);

    static QVector<PTPositionPath2DPoint> path2DForBuiltinMotion(PTPositionMotion motion);
    static QVector<PTPositionPath2DPoint> builtinFlyoutFan();
    static QVector<PTPositionPath2DPoint> builtinSpiralOut();
};

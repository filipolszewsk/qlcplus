/*
  ptshapesgallery.cpp
*/

#include "ptshapesgallery.h"
#include "ptpositionfxengine.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <algorithm>

namespace {

QPointF thumbCurvePoint(const QRectF& r, double xDeg, double yValue)
{
    return QPointF(r.left() + qBound(0.0, xDeg, 360.0) / 360.0 * r.width(),
                   r.bottom() - qBound(0.0, yValue, 255.0) / 255.0 * r.height());
}

QPointF thumbPathPoint(const QRectF& r, double pan01, double tilt01)
{
    return QPointF(r.center().x() + qBound(-1.0, pan01, 1.0) * r.width() * 0.45,
                   r.center().y() - qBound(-1.0, tilt01, 1.0) * r.height() * 0.45);
}

QPainterPath pathFromCurvePoints(const QVector<PTCustomCurvePoint>& points, const QRectF& r)
{
    QPainterPath path;
    if (points.size() < 2)
        return path;

    QVector<PTCustomCurvePoint> sorted = points;
    std::sort(sorted.begin(), sorted.end(),
              [](const PTCustomCurvePoint& a, const PTCustomCurvePoint& b) {
                  return a.xDeg < b.xDeg;
              });
    path.moveTo(thumbCurvePoint(r, sorted.first().xDeg, sorted.first().yValue));
    for (int i = 0; i < sorted.size() - 1; ++i)
    {
        const PTCustomCurvePoint& a = sorted.at(i);
        const PTCustomCurvePoint& b = sorted.at(i + 1);
        if (a.segmentMode == PTCustomCurvePoint::Linear)
            path.lineTo(thumbCurvePoint(r, b.xDeg, b.yValue));
        else
            path.cubicTo(thumbCurvePoint(r, a.rightHandleXDeg, a.rightHandleYValue),
                         thumbCurvePoint(r, b.leftHandleXDeg, b.leftHandleYValue),
                         thumbCurvePoint(r, b.xDeg, b.yValue));
    }
    return path;
}

QPainterPath pathFromPath2DPoints(const QVector<PTPositionPath2DPoint>& points,
                                  const QRectF& r, bool closed)
{
    QPainterPath path;
    if (points.isEmpty())
        return path;

    path.moveTo(thumbPathPoint(r, points.first().pan01, points.first().tilt01));
    for (int i = 0; i < points.size() - 1; ++i)
    {
        const PTPositionPath2DPoint& a = points.at(i);
        const PTPositionPath2DPoint& b = points.at(i + 1);
        if (a.segmentMode == PTCustomCurvePoint::Linear)
            path.lineTo(thumbPathPoint(r, b.pan01, b.tilt01));
        else
            path.cubicTo(thumbPathPoint(r, a.rightHandlePan01, a.rightHandleTilt01),
                         thumbPathPoint(r, b.leftHandlePan01, b.leftHandleTilt01),
                         thumbPathPoint(r, b.pan01, b.tilt01));
    }
    if (closed && points.size() >= 2)
    {
        const PTPositionPath2DPoint& a = points.last();
        const PTPositionPath2DPoint& b = points.first();
        if (a.segmentMode == PTCustomCurvePoint::Linear)
            path.lineTo(thumbPathPoint(r, b.pan01, b.tilt01));
        else
            path.cubicTo(thumbPathPoint(r, a.rightHandlePan01, a.rightHandleTilt01),
                         thumbPathPoint(r, b.leftHandlePan01, b.leftHandleTilt01),
                         thumbPathPoint(r, b.pan01, b.tilt01));
        path.closeSubpath();
    }
    return path;
}

QVector<PTPositionPath2DPoint> sampleCirclePath(int segments)
{
    QVector<PTPositionPath2DPoint> pts;
    for (int i = 0; i < segments; ++i)
    {
        const double t = double(i) / double(segments) * 2.0 * M_PI;
        PTPositionPath2DPoint p;
        p.pan01 = qSin(t);
        p.tilt01 = qCos(t);
        p.leftHandlePan01 = p.pan01;
        p.leftHandleTilt01 = p.tilt01;
        p.rightHandlePan01 = p.pan01;
        p.rightHandleTilt01 = p.tilt01;
        p.segmentMode = PTCustomCurvePoint::Linear;
        pts.append(p);
    }
    return pts;
}

} // namespace

QVector<PTCustomCurvePoint> PTShapesGallery::defaultMotionCurve1D()
{
    QVector<PTCustomCurvePoint> pts;
    PTCustomCurvePoint a;
    a.xDeg = 0; a.yValue = 128;
    PTCustomCurvePoint b;
    b.xDeg = 90; b.yValue = 255;
    PTCustomCurvePoint c;
    c.xDeg = 180; c.yValue = 128;
    PTCustomCurvePoint d;
    d.xDeg = 270; d.yValue = 0;
    PTCustomCurvePoint e;
    e.xDeg = 360; e.yValue = 128;
    pts << a << b << c << d << e;
    return pts;
}

QVector<PTPositionPath2DPoint> PTShapesGallery::defaultMotionPath2D()
{
    return sampleCirclePath(32);
}

QVector<PTPositionPath2DPoint> PTShapesGallery::builtinFlyoutFan()
{
    QVector<PTPositionPath2DPoint> pts;
    auto add = [&](double pan, double tilt) {
        PTPositionPath2DPoint p;
        p.pan01 = pan;
        p.tilt01 = tilt;
        p.segmentMode = PTCustomCurvePoint::Bezier;
        pts.append(p);
    };
    add(0, 0);
    add(0, 0.95);
    add(0.75, 0.75);
    add(0.95, 0);
    add(0.75, -0.75);
    add(0, -0.95);
    add(0, 0);
    return pts;
}

QVector<PTPositionPath2DPoint> PTShapesGallery::builtinSpiralOut()
{
    QVector<PTPositionPath2DPoint> pts;
    for (int i = 0; i <= 48; ++i)
    {
        const double t = double(i) / 48.0 * 3.0 * M_PI;
        const double r = 0.15 + 0.8 * double(i) / 48.0;
        PTPositionPath2DPoint p;
        p.pan01 = r * qCos(t);
        p.tilt01 = r * qSin(t);
        p.segmentMode = PTCustomCurvePoint::Linear;
        pts.append(p);
    }
    return pts;
}

QVector<PTPositionPath2DPoint> PTShapesGallery::path2DForBuiltinMotion(PTPositionMotion motion)
{
    QVector<PTPositionPath2DPoint> pts;
    const int segments = 64;
    for (int i = 0; i <= segments; ++i)
    {
        const double phase = double(i) / double(segments) * 2.0 * M_PI;
        qreal panOff = 0;
        qreal tiltOff = 0;
        const auto shape = PTPositionFxEngine::shapeFromPositionMotion(motion);
        PTPositionFxEngine::relativeOffset(shape, phase, 1.0, 1.0, panOff, tiltOff);
        PTPositionPath2DPoint p;
        p.pan01 = panOff;
        p.tilt01 = tiltOff;
        p.segmentMode = PTCustomCurvePoint::Linear;
        pts.append(p);
    }
    return pts;
}

QVector<PTShapeGalleryItem> PTShapesGallery::defaultBuiltinItems()
{
    QVector<PTShapeGalleryItem> items;

    auto add1D = [&](const QString& name, const QVector<PTCustomCurvePoint>& curve) {
        PTShapeGalleryItem item;
        item.name = name;
        item.kind = PTShapeGalleryKind::Curve1D;
        item.curve1D = curve;
        items.append(item);
    };
    auto add2D = [&](const QString& name, const QVector<PTPositionPath2DPoint>& path,
                     bool closed) {
        PTShapeGalleryItem item;
        item.name = name;
        item.kind = PTShapeGalleryKind::Path2D;
        item.path2D = path;
        item.path2DClosed = closed;
        items.append(item);
    };

    QVector<PTCustomCurvePoint> sine;
    for (int i = 0; i <= 4; ++i)
    {
        PTCustomCurvePoint p;
        p.xDeg = i * 90.0;
        p.yValue = 128.0 + 127.0 * qSin(i * M_PI_2);
        sine.append(p);
    }
    add1D(QStringLiteral("Sine"), sine);
    add1D(QStringLiteral("Triangle"), defaultMotionCurve1D());

    add2D(QStringLiteral("Circle"), path2DForBuiltinMotion(PTPositionMotion::Circle2D), true);
    add2D(QStringLiteral("Line"), path2DForBuiltinMotion(PTPositionMotion::Line2D), false);
    add2D(QStringLiteral("Figure-8"), path2DForBuiltinMotion(PTPositionMotion::Figure8_2D), true);
    add2D(QStringLiteral("Flyout fan"), builtinFlyoutFan(), false);
    add2D(QStringLiteral("Spiral out"), builtinSpiralOut(), false);
    return items;
}

QString PTShapesGallery::serializePath2D(const QVector<PTPositionPath2DPoint>& points)
{
    QStringList encoded;
    for (const PTPositionPath2DPoint& p : points)
    {
        encoded << QStringLiteral("%1,%2,%3,%4,%5,%6,%7")
                .arg(p.pan01, 0, 'f', 4)
                .arg(p.tilt01, 0, 'f', 4)
                .arg(p.leftHandlePan01, 0, 'f', 4)
                .arg(p.leftHandleTilt01, 0, 'f', 4)
                .arg(p.rightHandlePan01, 0, 'f', 4)
                .arg(p.rightHandleTilt01, 0, 'f', 4)
                .arg(qBound(0, p.segmentMode, 1));
    }
    return encoded.join(QLatin1Char(';'));
}

QVector<PTPositionPath2DPoint> PTShapesGallery::parsePath2D(const QString& text)
{
    QVector<PTPositionPath2DPoint> points;
    for (const QString& part : text.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        const QStringList values = part.split(QLatin1Char(','));
        if (values.size() != 6 && values.size() != 7)
            continue;
        PTPositionPath2DPoint p;
        p.pan01 = values.at(0).toDouble();
        p.tilt01 = values.at(1).toDouble();
        p.leftHandlePan01 = values.at(2).toDouble();
        p.leftHandleTilt01 = values.at(3).toDouble();
        p.rightHandlePan01 = values.at(4).toDouble();
        p.rightHandleTilt01 = values.at(5).toDouble();
        p.segmentMode = values.size() >= 7 ? qBound(0, values.at(6).toInt(), 1)
                                           : PTCustomCurvePoint::Bezier;
        points.append(p);
    }
    return points;
}

QPixmap PTShapesGallery::renderCurveThumbnail(const QVector<PTCustomCurvePoint>& points,
                                              const QSize& size)
{
    QPixmap pix(size);
    pix.fill(QColor(30, 30, 30));
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = pix.rect().adjusted(6, 6, -6, -6);
    p.setPen(QPen(QColor(235, 235, 235), 2));
    p.drawPath(pathFromCurvePoints(points, r));
    return pix;
}

QPixmap PTShapesGallery::renderPath2DThumbnail(const QVector<PTPositionPath2DPoint>& points,
                                               bool closed, const QSize& size)
{
    QPixmap pix(size);
    pix.fill(QColor(30, 30, 30));
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = pix.rect().adjusted(6, 6, -6, -6);
    p.setPen(QPen(QColor(80, 160, 255), 2));
    p.drawPath(pathFromPath2DPoints(points, r, closed));
    return pix;
}

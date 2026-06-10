/*
  ptpositionconverter.cpp
*/

#include "ptpositionconverter.h"

#include "fixture.h"
#include "qlcfixturemode.h"
#include "qlcphysical.h"

#include <QtMath>

namespace {

qreal physicalPanMax(Fixture* fxi)
{
    if (!fxi || !fxi->fixtureMode())
        return 360.0;
    const qreal v = fxi->fixtureMode()->physical().focusPanMax();
    return v > 0 ? v : 360.0;
}

qreal physicalTiltMax(Fixture* fxi)
{
    if (!fxi || !fxi->fixtureMode())
        return 270.0;
    const qreal v = fxi->fixtureMode()->physical().focusTiltMax();
    return v > 0 ? v : 270.0;
}

} // namespace

QRectF PTPositionConverter::degreesRange(Fixture* fxi, int head)
{
    if (!fxi)
        return QRectF(0, 0, 360, 270);
    const QRectF r = fxi->degreesRange(head);
    if (!r.isValid() || r.width() <= 0 || r.height() <= 0)
        return QRectF(0, 0, physicalPanMax(fxi), physicalTiltMax(fxi));
    return r;
}

PTPositionValue PTPositionConverter::centerPosition(Fixture* fxi, int head)
{
    PTPositionValue pos;
    const QRectF r = degreesRange(fxi, head);
    pos.valid = true;
    pos.panDeg = r.left() + r.width() / 2.0;
    pos.tiltDeg = r.top() + r.height() / 2.0;
    return pos;
}

qreal PTPositionConverter::clampPanDeg(Fixture* fxi, int head, qreal panDeg)
{
    const QRectF r = degreesRange(fxi, head);
    return qBound(r.left(), panDeg, r.left() + r.width());
}

qreal PTPositionConverter::clampTiltDeg(Fixture* fxi, int head, qreal tiltDeg)
{
    const QRectF r = degreesRange(fxi, head);
    return qBound(r.top(), tiltDeg, r.top() + r.height());
}

PTPositionValue PTPositionConverter::clampPosition(Fixture* fxi, int head,
                                                   const PTPositionValue& pos)
{
    if (!pos.valid)
        return pos;
    PTPositionValue out = pos;
    out.panDeg = clampPanDeg(fxi, head, out.panDeg);
    out.tiltDeg = clampTiltDeg(fxi, head, out.tiltDeg);
    return out;
}

quint16 PTPositionConverter::panDegToPan16(Fixture* fxi, int head, qreal panDeg)
{
    const qreal clamped = clampPanDeg(fxi, head, panDeg);
    qreal norm = clamped / physicalPanMax(fxi);
    if (fxi && fxi->hasPanTiltRange(head) && fxi->getPanTiltRange(head).panReverse)
        norm = 1.0 - norm;
    return quint16(qBound(0, int(qRound(norm * 65535.0)), 65535));
}

quint16 PTPositionConverter::tiltDegToTilt16(Fixture* fxi, int head, qreal tiltDeg)
{
    const qreal clamped = clampTiltDeg(fxi, head, tiltDeg);
    qreal norm = clamped / physicalTiltMax(fxi);
    if (fxi && fxi->hasPanTiltRange(head) && fxi->getPanTiltRange(head).tiltReverse)
        norm = 1.0 - norm;
    return quint16(qBound(0, int(qRound(norm * 65535.0)), 65535));
}

qreal PTPositionConverter::pan16ToPanDeg(Fixture* fxi, int head, quint16 pan16)
{
    qreal norm = double(pan16) / 65535.0;
    if (fxi && fxi->hasPanTiltRange(head) && fxi->getPanTiltRange(head).panReverse)
        norm = 1.0 - norm;
    return clampPanDeg(fxi, head, norm * physicalPanMax(fxi));
}

qreal PTPositionConverter::tilt16ToTiltDeg(Fixture* fxi, int head, quint16 tilt16)
{
    qreal norm = double(tilt16) / 65535.0;
    if (fxi && fxi->hasPanTiltRange(head) && fxi->getPanTiltRange(head).tiltReverse)
        norm = 1.0 - norm;
    return clampTiltDeg(fxi, head, norm * physicalTiltMax(fxi));
}

void PTPositionConverter::degreesToNormalized(Fixture* fxi, int head, qreal panDeg, qreal tiltDeg,
                                              qreal& xNorm, qreal& yNorm)
{
    const QRectF r = degreesRange(fxi, head);
    xNorm = r.width() > 0 ? (clampPanDeg(fxi, head, panDeg) - r.left()) / r.width() : 0.5;
    yNorm = r.height() > 0 ? (clampTiltDeg(fxi, head, tiltDeg) - r.top()) / r.height() : 0.5;
    xNorm = qBound(0.0, xNorm, 1.0);
    yNorm = qBound(0.0, yNorm, 1.0);
}

void PTPositionConverter::normalizedToDegrees(Fixture* fxi, int head, qreal xNorm, qreal yNorm,
                                              qreal& panDeg, qreal& tiltDeg)
{
    const QRectF r = degreesRange(fxi, head);
    panDeg = clampPanDeg(fxi, head, r.left() + qBound(0.0, xNorm, 1.0) * r.width());
    tiltDeg = clampTiltDeg(fxi, head, r.top() + qBound(0.0, yNorm, 1.0) * r.height());
}

QString PTPositionConverter::formatPosition(const PTPositionValue& pos, int decimals)
{
    if (!pos.valid)
        return QStringLiteral("-");
    return QStringLiteral("%1°, %2°")
            .arg(pos.panDeg, 0, 'f', decimals)
            .arg(pos.tiltDeg, 0, 'f', decimals);
}

PTPositionValue PTPositionConverter::parsePositionText(const QString& raw, Fixture* fxi, int head)
{
    PTPositionValue pos;
    QString text = raw.trimmed();
    if (text.isEmpty() || text == QLatin1String("-"))
        return pos;

    text.replace(QLatin1Char('/'), QLatin1Char(','));
    text.replace(QLatin1Char(';'), QLatin1Char(','));
    QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.size() < 2)
        parts = text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return pos;

    auto parsePart = [](const QString& part, qreal& out) -> bool {
        QString s = part.trimmed();
        s.remove(QStringLiteral("°"));
        bool ok = false;
        const double v = s.toDouble(&ok);
        if (!ok)
            return false;
        out = v;
        return true;
    };

    qreal pan = 0;
    qreal tilt = 0;
    if (!parsePart(parts.at(0), pan) || !parsePart(parts.at(1), tilt))
        return pos;

    pos.valid = true;
    pos.panDeg = clampPanDeg(fxi, head, pan);
    pos.tiltDeg = clampTiltDeg(fxi, head, tilt);
    return pos;
}

PTPositionValue PTPositionConverter::blendPositions(const PTPositionValue& a,
                                                    const PTPositionValue& b,
                                                    double t)
{
    if (!a.valid)
        return b;
    if (!b.valid)
        return a;
    t = qBound(0.0, t, 1.0);
    PTPositionValue out;
    out.valid = true;
    out.panDeg = a.panDeg + (b.panDeg - a.panDeg) * t;
    out.tiltDeg = a.tiltDeg + (b.tiltDeg - a.tiltDeg) * t;
    return out;
}

PTPositionValue PTPositionConverter::addRelativeOffset(const PTPositionValue& base, Fixture* fxi,
                                                       int head, qreal panOffsetDeg,
                                                       qreal tiltOffsetDeg)
{
    if (!base.valid)
        return base;
    PTPositionValue out = base;
    out.panDeg = clampPanDeg(fxi, head, base.panDeg + panOffsetDeg);
    out.tiltDeg = clampTiltDeg(fxi, head, base.tiltDeg + tiltOffsetDeg);
    return out;
}

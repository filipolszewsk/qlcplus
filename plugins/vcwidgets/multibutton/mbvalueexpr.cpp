/*
  QLC+ VC Widget Plugin — Multi Button
  mbvalueexpr.cpp
*/

#include "mbvalueexpr.h"

#include "universe.h"

#include <QRegularExpression>
#include <QList>

static QString trimmed(const QString& s)
{
    return s.trimmed();
}

bool mbValueExprLooksLikeFormula(const QString& text)
{
    const QString t = trimmed(text);
    return t.length() >= 3 && t.startsWith(QStringLiteral("IF("), Qt::CaseInsensitive);
}

static bool splitTopLevelCommas(const QString& inner, QStringList& parts)
{
    parts.clear();
    int depth = 0;
    int start = 0;
    for (int i = 0; i < inner.size(); ++i)
    {
        const QChar c = inner.at(i);
        if (c == QLatin1Char('('))
            ++depth;
        else if (c == QLatin1Char(')'))
        {
            if (depth > 0)
                --depth;
        }
        else if (c == QLatin1Char(',') && depth == 0)
        {
            parts.append(inner.mid(start, i - start).trimmed());
            start = i + 1;
        }
    }
    parts.append(inner.mid(start).trimmed());
    return parts.size() == 3;
}

static bool parseCondition(const QString& cond, MbValueExpr& out, QString* error)
{
    static const QRegularExpression re(
        QStringLiteral(R"(^u(\d+)\.ch(\d+)\s*(==|!=|<=|>=|<|>)\s*(-?\d+)$)"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch m = re.match(cond.trimmed());
    if (!m.hasMatch())
    {
        if (error)
            *error = QStringLiteral("Expected condition like u1.ch42 >= 128");
        return false;
    }

    const int uNum = m.captured(1).toInt();
    if (uNum < 1)
    {
        if (error)
            *error = QStringLiteral("Universe number must be >= 1 (u1, u2, …)");
        return false;
    }

    out.universe = quint32(uNum - 1);

    bool okCh = false;
    const uint chNum = m.captured(2).toUInt(&okCh);
    if (!okCh || chNum < 1 || chNum > 512)
    {
        if (error)
            *error = QStringLiteral("Channel must be 1–512 (ch1 = first DMX channel, like patch 1.1)");
        return false;
    }
    out.channel = chNum - 1;

    const QString opStr = m.captured(3);
    if (opStr == QLatin1String("=="))
        out.op = MbCompareOp::Eq;
    else if (opStr == QLatin1String("!="))
        out.op = MbCompareOp::Ne;
    else if (opStr == QLatin1String("<"))
        out.op = MbCompareOp::Lt;
    else if (opStr == QLatin1String("<="))
        out.op = MbCompareOp::Le;
    else if (opStr == QLatin1String(">"))
        out.op = MbCompareOp::Gt;
    else
        out.op = MbCompareOp::Ge;

    out.compareValue = m.captured(4).toInt();
    return true;
}

static bool parseValueLiteral(const QString& text, int& out, QString* error)
{
    bool ok = false;
    const int v = text.trimmed().toInt(&ok);
    if (!ok)
    {
        if (error)
            *error = QStringLiteral("Expected DMX value 0–255");
        return false;
    }
    if (v < 0 || v > 255)
    {
        if (error)
            *error = QStringLiteral("DMX value must be 0–255");
        return false;
    }
    out = v;
    return true;
}

bool mbParseValueExpr(const QString& text, MbValueExpr& out, QString* error)
{
    QString t = trimmed(text);
    if (!mbValueExprLooksLikeFormula(t))
    {
        if (error)
            *error = QStringLiteral("Formula must start with IF(");
        return false;
    }

    if (t.endsWith(QLatin1Char(')')))
        t.chop(1);
    t = t.mid(3).trimmed();   // after "IF("

    QStringList parts;
    if (!splitTopLevelCommas(t, parts))
    {
        if (error)
            *error = QStringLiteral("IF() requires three arguments: condition, then, else");
        return false;
    }

    MbValueExpr parsed;
    if (!parseCondition(parts.at(0), parsed, error))
        return false;
    if (!parseValueLiteral(parts.at(1), parsed.thenValue, error))
        return false;
    if (!parseValueLiteral(parts.at(2), parsed.elseValue, error))
        return false;

    out = parsed;
    return true;
}

static bool compare(int live, int expected, MbCompareOp op)
{
    switch (op)
    {
        case MbCompareOp::Eq: return live == expected;
        case MbCompareOp::Ne: return live != expected;
        case MbCompareOp::Lt: return live < expected;
        case MbCompareOp::Le: return live <= expected;
        case MbCompareOp::Gt: return live > expected;
        case MbCompareOp::Ge: return live >= expected;
    }
    return false;
}

quint8 mbEvaluateValueExpr(const MbValueExpr& expr, const QList<Universe*>& universes,
                           QString* error)
{
    if (int(expr.universe) >= universes.size())
    {
        if (error)
            *error = QStringLiteral("Universe u%1 is not available").arg(expr.universe + 1);
        return 0;
    }

    Universe* uni = universes.at(int(expr.universe));
    if (!uni)
    {
        if (error)
            *error = QStringLiteral("Universe u%1 is not available").arg(expr.universe + 1);
        return 0;
    }

    const int live = int(uni->preGMValue(expr.channel));
    const int out = compare(live, expr.compareValue, expr.op)
                        ? expr.thenValue : expr.elseValue;
    return quint8(qBound(0, out, 255));
}

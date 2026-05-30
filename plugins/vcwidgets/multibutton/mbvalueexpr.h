/*
  QLC+ VC Widget Plugin — Multi Button
  mbvalueexpr.h — Level preset IF() value expressions
*/

#pragma once

#include <QString>
#include <QtGlobal>

class Universe;

enum class MbCompareOp
{
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge
};

struct MbValueExpr
{
    quint32 universe = 0;   // 0-based universe index
    quint32 channel  = 0;   // absolute channel within universe
    MbCompareOp op = MbCompareOp::Eq;
    int compareValue = 0;
    int thenValue = 0;
    int elseValue = 0;
};

bool mbValueExprLooksLikeFormula(const QString& text);

bool mbParseValueExpr(const QString& text, MbValueExpr& out, QString* error = nullptr);

quint8 mbEvaluateValueExpr(const MbValueExpr& expr, const QList<Universe*>& universes,
                           QString* error = nullptr);

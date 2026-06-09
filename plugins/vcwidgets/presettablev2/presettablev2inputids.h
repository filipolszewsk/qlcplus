/*
  VCWidget input source IDs for Preset Table v2.
*/

#pragma once

#include <QtGlobal>

namespace PTInputId
{
static const int kMaxRoutableOutputs = 48;
static const quint8 kTransSweepBase = 48;
static const quint8 kTransSecondaryRowBase = 96;
static const quint8 kTransContinuousBankBase = 144;
static const quint8 kMultiFxBankBase = 192;
static const quint8 kCrossfade = 240;
static const quint8 kMultiFxBlend = 241;
static const quint8 kMultiFxRestart = 242;
static const quint8 kWidgetFlashGate = 243;

inline quint8 rowSelector(int outputIdx) { return quint8(outputIdx); }
/** Transition selector (legacy selector_sweep) — bank preset index (0 = instant). */
inline quint8 transSweep(int outputIdx) { return quint8(kTransSweepBase + outputIdx); }
/** Secondary table row for Continuous only (DMX 0 = Properties default; 1 = row 1, …). */
inline quint8 transSecondaryRow(int outputIdx) { return quint8(kTransSecondaryRowBase + outputIdx); }
/** Continuous FX selector (legacy selector_continuous) — bank preset index (0 = off). */
inline quint8 transContinuousBank(int outputIdx) { return quint8(kTransContinuousBankBase + outputIdx); }
/** MultiFX background bus selector — bank preset index (0 = off). */
inline quint8 multiFxBank(int outputIdx) { return quint8(kMultiFxBankBase + outputIdx); }

/** @deprecated Use transSweep */
inline quint8 transPrimary(int outputIdx) { return transSweep(outputIdx); }
/** @deprecated Use transSecondaryRow */
inline quint8 transSecondary(int outputIdx) { return transSecondaryRow(outputIdx); }
}

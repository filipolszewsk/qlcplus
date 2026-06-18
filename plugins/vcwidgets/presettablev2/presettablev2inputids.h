/*
  VCWidget input source IDs for Preset Table v2.
*/

#pragma once

#include <QtGlobal>

namespace PTInputId
{
static const int kMaxRoutableOutputs = 30;
static const quint8 kTransSweepBase = 30;
static const quint8 kTransSecondaryRowBase = 60;
static const quint8 kTransContinuousBankBase = 90;
static const quint8 kPositionMotionBankBase = 120;
static const quint8 kMultiFxBankBase = 150;
static const quint8 kChannel1DBankBase = 180;
static const quint8 kOutputIntensityBase = 210;
static const quint32 kCrossfade = 240;
static const quint32 kMultiFxBlend = 241;
static const quint32 kMultiFxRestart = 242;
static const quint32 kWidgetFlashGate = 243;
static const quint32 kPositionBasePan = 244;
static const quint32 kPositionBaseTilt = 245;
static const quint32 kPositionSpreadPan = 246;
static const quint32 kPositionSpreadTilt = 247;
static const quint32 kPositionSpreadPanEnable = 248;
static const quint32 kPositionSpreadTiltEnable = 249;

inline quint8 rowSelector(int outputIdx) { return quint8(outputIdx); }
/** Transition selector (legacy selector_sweep) — bank preset index (0 = instant). */
inline quint8 transSweep(int outputIdx) { return quint8(kTransSweepBase + outputIdx); }
/** Secondary table row for Continuous only (DMX 0 = Properties default; 1 = row 1, …). */
inline quint8 transSecondaryRow(int outputIdx) { return quint8(kTransSecondaryRowBase + outputIdx); }
/** Interpolation selector (legacy selector_continuous) — bank preset index (0 = off). */
inline quint8 transContinuousBank(int outputIdx) { return quint8(kTransContinuousBankBase + outputIdx); }
/** MultiFX background bus selector — bank preset index (0 = off). */
inline quint8 multiFxBank(int outputIdx) { return quint8(kMultiFxBankBase + outputIdx); }
/** Position-only 2D FX selector — bank preset index (0 = off). */
inline quint8 positionMotionBank(int outputIdx) { return quint8(kPositionMotionBankBase + outputIdx); }
/** Fixture Group only 1D FX selector — bank preset index (0 = off). */
inline quint8 channel1DBank(int outputIdx) { return quint8(kChannel1DBankBase + outputIdx); }
/** Fixture Group output intensity multiplier — 0..255, defaults to 255 when unmapped. */
inline quint8 outputIntensity(int outputIdx) { return quint8(kOutputIntensityBase + outputIdx); }

/** @deprecated Use transSweep */
inline quint8 transPrimary(int outputIdx) { return transSweep(outputIdx); }
/** @deprecated Use transSecondaryRow */
inline quint8 transSecondary(int outputIdx) { return transSecondaryRow(outputIdx); }
}

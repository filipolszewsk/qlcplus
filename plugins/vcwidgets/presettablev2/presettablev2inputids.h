/*
  VCWidget input source IDs for Preset Table v2.
*/

#pragma once

#include <QtGlobal>

namespace PTInputId
{
static const quint8 kCrossfade = 255;
static const quint8 kTransSweepBase = 64;
static const quint8 kTransSecondaryRowBase = 128;
static const quint8 kTransContinuousBankBase = 192;

inline quint8 rowSelector(int outputIdx) { return quint8(outputIdx); }
/** DMX selector_sweep — bank preset index (0 = off, 1 = first sweep preset). */
inline quint8 transSweep(int outputIdx) { return quint8(kTransSweepBase + outputIdx); }
/** Secondary table row for Continuous only (DMX 0 = Properties default; 1 = row 1, …). */
inline quint8 transSecondaryRow(int outputIdx) { return quint8(kTransSecondaryRowBase + outputIdx); }
/** DMX selector_continuous — bank preset index (0 = off). */
inline quint8 transContinuousBank(int outputIdx) { return quint8(kTransContinuousBankBase + outputIdx); }

/** @deprecated Use transSweep */
inline quint8 transPrimary(int outputIdx) { return transSweep(outputIdx); }
/** @deprecated Use transSecondaryRow */
inline quint8 transSecondary(int outputIdx) { return transSecondaryRow(outputIdx); }
}

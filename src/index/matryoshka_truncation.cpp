/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            matryoshka_truncation.cpp                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-24                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// The MatryoshkaTruncation and MatryoshkaTruncatedIndex implementations are
// fully header-only (inline) in include/index/matryoshka_truncation.h.
//
// This translation unit exists to:
//   1. Provide a named compilation unit for the linker (avoids ODR issues when
//      the header is included from multiple TUs in a non-header-only build).
//   2. Serve as an extension point for future non-inline logic (e.g., SIMD-
//      accelerated normalization, serialization helpers).

#include "index/matryoshka_truncation.h"

// Nothing to define here — all logic is inline in the header.
// Future additions such as SIMD-accelerated L2 normalisation or persistence
// helpers for MatryoshkaTruncatedIndex metadata should be placed here.

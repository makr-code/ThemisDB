/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            normalizer.h                                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:38:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     42                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4c030cf92  2025-11-06  Add AQL FULLTEXT integration and geo scaffolding ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>

namespace themis {
namespace utils {

class Normalizer {
public:
    // Normalize German umlauts and ß to ASCII equivalents.
    // ä->a, ö->o, ü->u, Ä->A, Ö->O, Ü->U, ß->ss
    // Input is expected to be UTF-8; returns normalized UTF-8 string.
    static std::string normalizeUmlauts(std::string_view text);
};

} // namespace utils
} // namespace themis

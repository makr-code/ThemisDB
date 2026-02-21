/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            normalizer.h                                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     18                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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

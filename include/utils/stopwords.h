/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stopwords.h                                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:38:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     45                                             ║
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
#include <unordered_set>
#include <vector>

namespace themis {
namespace utils {

class Stopwords {
public:
    // Returns a default stopword set for a given language code ("en", "de", "none").
    static std::unordered_set<std::string> defaults(const std::string& language);
    
    // Merge default stopwords with a custom list (both assumed lowercase)
    static std::unordered_set<std::string> merge(const std::unordered_set<std::string>& base,
                                                 const std::vector<std::string>& custom);
};

} // namespace utils
} // namespace themis

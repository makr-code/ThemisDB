/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stopwords.cpp                                      ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/stopwords.h"
#include <algorithm>

namespace themis {
namespace utils {

static std::unordered_set<std::string> make_set(std::initializer_list<const char*> list) {
    std::unordered_set<std::string> s;
    s.reserve(list.size() * 2);
    for (auto* w : list) s.emplace(w);
    return s;
}

std::unordered_set<std::string> Stopwords::defaults(const std::string& language) {
    if (language == "en" || language == "EN") {
        return make_set({
            "the","a","an","and","or","but","if","then","else","when","while",
            "is","are","was","were","be","been","being",
            "in","on","at","of","to","for","with","by","from","as","it","its",
            "this","that","these","those","not","no","do","does","did","done"
        });
    }
    if (language == "de" || language == "DE") {
        return make_set({
            "der","die","das","und","oder","aber","nicht",
            "ist","sind","war","waren",
            "im","in","am","an","auf","zu","von","mit","bei","aus",
            "dies","diese","dieser","diesen","dem","den",
            "ein","eine","einer","einem","einen",
            "als","es","sein","seine","seiner"
        });
    }
    return {};
}

std::unordered_set<std::string> Stopwords::merge(const std::unordered_set<std::string>& base,
                                                 const std::vector<std::string>& custom) {
    std::unordered_set<std::string> out = base;
    out.reserve(out.size() + custom.size());
    for (auto w : custom) {
        // ensure lowercase (defensive)
        std::string lw = w;
        std::transform(lw.begin(), lw.end(), lw.begin(), [](unsigned char c){ return std::tolower(c); });
        out.emplace(std::move(lw));
    }
    return out;
}

} // namespace utils
} // namespace themis

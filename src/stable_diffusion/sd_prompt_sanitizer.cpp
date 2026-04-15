/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sd_prompt_sanitizer.cpp                            ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-15 04:20:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     97                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1e348484ec  2026-04-07  feat(plugins): add stable_diffusion + llama_cpp plugins, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "stable_diffusion/sd_prompt_sanitizer.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace themis {
namespace imggen {

// ── helpers ───────────────────────────────────────────────────────────────────

std::string SDPromptSanitizer::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

// ── constructors ──────────────────────────────────────────────────────────────

SDPromptSanitizer::SDPromptSanitizer(const std::vector<std::string>& keywords) {
    blocked_keywords_.reserve(keywords.size());
    for (const auto& kw : keywords) {
        const std::string lower = toLower(kw);
        if (!lower.empty()) blocked_keywords_.push_back(lower);
    }
}

SDPromptSanitizer SDPromptSanitizer::fromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("SDPromptSanitizer: cannot open '" + path + "'");
    }
    std::vector<std::string> kws;
    std::string line;
    while (std::getline(f, line)) {
        // trim whitespace
        const auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;
        kws.push_back(line);
    }
    return SDPromptSanitizer(kws);
}

// ── isAllowed ─────────────────────────────────────────────────────────────────

bool SDPromptSanitizer::isAllowed(const std::string& prompt) const {
    const std::string lower = toLower(prompt);
    for (const auto& kw : blocked_keywords_) {
        if (lower.find(kw) != std::string::npos) return false;
    }
    return true;
}

// ── sanitize ──────────────────────────────────────────────────────────────────

std::string SDPromptSanitizer::sanitize(const std::string& prompt) const {
    if (blocked_keywords_.empty()) return prompt;
    std::string result = prompt;
    const std::string lower_result = toLower(result);
    // For each blocked keyword, remove all case-insensitive occurrences
    for (const auto& kw : blocked_keywords_) {
        std::string::size_type pos = 0;
        std::string lower_r = toLower(result);
        while ((pos = lower_r.find(kw, pos)) != std::string::npos) {
            result.erase(pos, kw.size());
            lower_r.erase(pos, kw.size());
        }
    }
    return result;
}

} // namespace imggen
} // namespace themis

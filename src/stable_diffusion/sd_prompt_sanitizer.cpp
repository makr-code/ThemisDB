/**
 * @file sd_prompt_sanitizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
        if (!lower.empty()) {
          blocked_keywords_.push_back(lower);
        }
    }
}

SDPromptSanitizer SDPromptSanitizer::fromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("SDPromptSanitizer: cannot open '" + path + "'");
    }
    std::vector<std::string> kws;
    std::string line = {};
    while (std::getline(f, line)) {
        // trim whitespace
        const auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
          continue;
        }
        line = line.substr(start);
        if (line.empty() || line[0] == '#') {
          continue;
        }
        kws.push_back(line);
    }
    return SDPromptSanitizer(kws);
}

// ── isAllowed ─────────────────────────────────────────────────────────────────

bool SDPromptSanitizer::isAllowed(const std::string& prompt) const {
    const std::string lower = toLower(prompt);
    for (const auto& kw : blocked_keywords_) {
        if (lower.find(kw) != std::string::npos) {
          return false;
        }
    }
    return true;
}

// ── sanitize ──────────────────────────────────────────────────────────────────

std::string SDPromptSanitizer::sanitize(const std::string& prompt) const {
    if (blocked_keywords_.empty()) {
      return prompt;
    }
    std::string result = prompt;
    const std::string lower_result = toLower(result);
    // For each blocked keyword, remove all case-insensitive occurrences
    for (const auto& kw : blocked_keywords_) {
        std::string::size_type pos = 0;
        std::string lower_r = toLower(result);
        while ((pos = lower_r.find(kw, pos)) != std::string::npos) {
            result.erase(pos,static_cast<int>(kw.size()));
            lower_r.erase(pos,static_cast<int>(kw.size()));
        }
    }
    return result;
}

} // namespace imggen
} // namespace themis

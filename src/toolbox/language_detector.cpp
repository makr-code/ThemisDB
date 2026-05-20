/*
 * ThemisDB | File: language_detector.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 86
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=8 | delta=5 | status=divergent
 * External Severity (v3): C=0, H=6, M=2
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/language_detector.h"
#include "utils/stopwords.h"
#include "utils/string_utils.h"

#include <sstream>
#include <unordered_map>

namespace themis {
namespace toolbox {

namespace {

/// Split @p text on whitespace and return lower-cased tokens.
std::vector<std::string> tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string buf(text);
    std::istringstream iss(buf);
    std::string word;
    while (iss >> word) {
        tokens.push_back(utils::toLower(word));
    }
    return tokens;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// DefaultLanguageDetector
// ─────────────────────────────────────────────────────────────────────────────

DefaultLanguageDetector::DefaultLanguageDetector()
    : min_ratio_(0.05)
{}

DefaultLanguageDetector::DefaultLanguageDetector(double min_ratio)
    : min_ratio_(min_ratio)
{}

std::string DefaultLanguageDetector::detect(std::string_view text) const {
    auto tokens = tokenize(text);
    if (tokens.size() < 3) {
        return "und";
    }

    // Candidate languages with their stopword sets
    static const std::vector<std::string> kCandidates{"en", "de"};

    std::string best_lang  = "und";
    double      best_ratio = min_ratio_;

    for (const auto& lang : kCandidates) {
        auto stopwords = utils::Stopwords::defaults(lang);
        std::size_t hits = 0;
        for (const auto& tok : tokens) {
            if (stopwords.count(tok)) {
                ++hits;
            }
        }
        double ratio = static_cast<double>(hits) / static_cast<double>(tokens.size());
        if (ratio > best_ratio) {
            best_ratio = ratio;
            best_lang  = lang;
        }
    }

    return best_lang;
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

std::string detectLanguage(std::string_view text) {
    return DefaultLanguageDetector{}.detect(text);
}

} // namespace toolbox
} // namespace themis

/*
 * ThemisDB | File: text_quality_scorer.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/text_quality_scorer.h"
#include "toolbox/language_detector.h"
#include "utils/normalizer.h"

#include <sstream>
#include <unordered_set>

namespace themis {
namespace toolbox {

namespace {

struct TokenStats {
    std::size_t total  = 0;
    std::size_t unique = 0;
    double      avg_word_len = 0.0;
};

TokenStats computeTokenStats(const std::string& normalized) {
    TokenStats stats;
    std::unordered_set<std::string> seen;
    std::istringstream iss(normalized);
    std::string word;
    std::size_t total_len = 0;

    while (iss >> word) {
        ++stats.total;
        total_len += word.size();
        seen.insert(word);
    }

    stats.unique = seen.size();
    stats.avg_word_len = (stats.total > 0)
        ? static_cast<double>(total_len) / static_cast<double>(stats.total)
        : 0.0;
    return stats;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// TextQualityScorer
// ─────────────────────────────────────────────────────────────────────────────

TextQualityScore TextQualityScorer::score(std::string_view text) const {
    TextQualityScore result;
    result.char_count = text.size();

    // Empty / whitespace check
    if (text.empty()) {
        result.is_empty = true;
        result.language = "und";
        return result;
    }

    // Check if all-whitespace
    bool all_ws = true;
    for (char c : text) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            all_ws = false;
            break;
        }
    }
    if (all_ws) {
        result.is_empty = true;
        result.language = "und";
        return result;
    }

    // Normalize umlauts for consistent tokenization
    const std::string normalized = utils::Normalizer::normalizeUmlauts(text);

    // Token statistics
    auto stats = computeTokenStats(normalized);
    result.token_count = stats.total;

    // Language detection (run on original text for better stopword matching)
    result.language = DefaultLanguageDetector{}.detect(text);

    // Boilerplate heuristics
    if (stats.total < 5) {
        result.has_boilerplate = true;
    } else if (stats.avg_word_len > 25.0) {
        // Likely garbled / binary data
        result.has_boilerplate = true;
    } else if (stats.total > 0) {
        double repetition_ratio =
            1.0 - static_cast<double>(stats.unique) / static_cast<double>(stats.total);
        if (repetition_ratio > 0.50) {
            result.has_boilerplate = true;
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

TextQualityScore scoreText(std::string_view text) {
    return TextQualityScorer{}.score(text);
}

} // namespace toolbox
} // namespace themis

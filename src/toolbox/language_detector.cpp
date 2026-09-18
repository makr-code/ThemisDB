/**
 * @file language_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/language_detector.h"
#include "utils/stopwords.h"
#include "utils/string_utils.h"

#include <sstream>
#include <unordered_map>

namespace themis {
namespace toolbox {

namespace {

/**
 * @brief Tokenize.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Calls: reserve(), buf(), iss(), push_back(), utils::toLower().
 */
std::vector<std::string> tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    // Pre-allocate estimated capacity to reduce push_back overhead
    tokens.reserve(32);
    
    std::string buf(text);
    std::istringstream iss(buf);
    std::string word = {};
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

/**
 * @brief Detect Language.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Calls: detect().
 */
std::string detectLanguage(std::string_view text) {
    return DefaultLanguageDetector{}.detect(text);
}

} // namespace toolbox
} // namespace themis

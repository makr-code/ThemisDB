/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stemmer.h                                          ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     93                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>

namespace themis {
namespace utils {

/**
 * @brief Minimal Stemming Implementation
 * 
 * Supports English (Porter Stemmer subset) and German (basic suffix removal)
 * 
 * Based on:
 * - Porter Stemmer Algorithm (1980): https://tartarus.org/martin/PorterStemmer/
 * - German Stemming: Snowball German Stemmer (simplified)
 * 
 * This is a minimal implementation for v1. For production use, consider
 * integrating full Snowball library: https://snowballstem.org/
 */
class Stemmer {
public:
    enum class Language {
        EN,  // English
        DE,  // German
        NONE // No stemming
    };

    /**
     * @brief Stem a single token
     * 
     * @param token Input token (should be lowercase)
     * @param lang Language for stemming rules
     * @return Stemmed token
     * 
     * Example:
     *   stem("running", Language::EN) -> "run"
     *   stem("laufen", Language::DE) -> "lauf"
     */
    static std::string stem(std::string_view token, Language lang);

    /**
     * @brief Parse language code string to enum
     * 
     * @param langCode "en", "de", or "none"
     * @return Language enum
     */
    static Language parseLanguage(std::string_view langCode);

    /**
     * @brief Convert language enum to string
     */
    static std::string languageToString(Language lang);

private:
    // English Porter Stemmer helpers (simplified)
    static std::string stemEnglish(std::string word);
    static bool endsWithDoubleConsonant(const std::string& word);
    static bool hasVowel(const std::string& word);
    static std::string replaceEnding(std::string word, std::string_view from, std::string_view to);
    
    // German stemmer helpers (simplified)
    static std::string stemGerman(std::string word);
};

} // namespace utils
} // namespace themis

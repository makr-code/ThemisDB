/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            language_detector.h                                ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-20                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file language_detector.h
 * @brief Toolbox language detection API (ISO 639-1).
 *
 * Provides a stable `ILanguageDetector` interface and a heuristic
 * `DefaultLanguageDetector` implementation.  The default implementation
 * detects language by counting stopword matches from `utils::Stopwords`.
 *
 * ## Free function (simplest usage)
 * @code
 * std::string lang = themis::toolbox::detectLanguage("Der Hund bellt laut.");
 * // -> "de"
 * @endcode
 *
 * ## Replacing the detector (tests or custom logic)
 * @code
 * class MyDetector : public themis::toolbox::ILanguageDetector { … };
 * @endcode
 */

#include <memory>
#include <string>
#include <string_view>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ILanguageDetector
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Interface for language detection implementations.
 *
 * Returning an ISO 639-1 code (e.g. "en", "de") or "und" (undetermined)
 * when the language cannot be identified with sufficient confidence.
 */
class ILanguageDetector {
public:
    virtual ~ILanguageDetector() = default;

    /**
     * @brief Detect the primary language of @p text.
     *
     * @param text UTF-8 text sample.  For reliable results, supply at
     *             least 100 characters.
     * @return ISO 639-1 language code (e.g. "en", "de") or "und" when the
     *         language cannot be determined.
     */
    virtual std::string detect(std::string_view text) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// DefaultLanguageDetector
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Heuristic language detector based on stopword frequency analysis.
 *
 * Tokenises @p text into lower-cased whitespace-separated words and counts
 * how many belong to the stopword sets for each candidate language (currently
 * English and German).  The language with the highest match ratio above a
 * configurable confidence threshold wins.
 *
 * Supported languages and their ISO 639-1 codes:
 *  - `"en"` — English (uses `utils::Stopwords::defaults("en")`)
 *  - `"de"` — German  (uses `utils::Stopwords::defaults("de")`)
 *
 * For text that matches neither set above the threshold, the result is `"und"`
 * (undetermined).
 *
 * ### Accuracy expectations
 *
 * | Condition                              | Expected result |
 * |----------------------------------------|-----------------|
 * | ≥ 10 English stopwords in a 100-word sample | `"en"`     |
 * | ≥ 10 German  stopwords in a 100-word sample | `"de"`     |
 * | Very short input (< 3 words)           | `"und"`         |
 * | Mixed language / technical text        | `"und"` or best match |
 *
 * ### Performance
 * < 1 ms for a 10 000-character text on a modern CPU.
 *
 * Thread-safety: all methods are stateless; an instance may be shared.
 */
class DefaultLanguageDetector : public ILanguageDetector {
public:
    /**
     * @brief Construct with default confidence threshold (0.05 = 5 % of words
     *        must be stopwords).
     */
    DefaultLanguageDetector();

    /**
     * @brief Construct with a custom confidence threshold.
     *
     * @param min_ratio Minimum ratio of stopword matches to total word count
     *                  required to assert a language.  Range [0.0, 1.0].
     */
    explicit DefaultLanguageDetector(double min_ratio);

    ~DefaultLanguageDetector() override = default;

    DefaultLanguageDetector(const DefaultLanguageDetector&)            = default;
    DefaultLanguageDetector& operator=(const DefaultLanguageDetector&) = default;

    /// @copydoc ILanguageDetector::detect
    std::string detect(std::string_view text) const override;

private:
    double min_ratio_;  ///< Minimum stopword-match ratio to assert a language.
};

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Detect the primary language of @p text using the default heuristic.
 *
 * Convenience free function.  Equivalent to
 * `DefaultLanguageDetector{}.detect(text)`.
 *
 * @param text UTF-8 text sample.
 * @return ISO 639-1 language code ("en", "de") or "und" when undetermined.
 */
std::string detectLanguage(std::string_view text);

} // namespace toolbox
} // namespace themis

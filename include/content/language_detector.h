/**
 * @file language_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace themis {
namespace content {

/**
 * @brief Language detection result for a text document.
 *
 * Produced by LanguageDetector::detect() and stored in
 * ExtractionResult::metadata["detected_language"].
 */
struct DetectedLanguage {
    /// BCP 47 / ISO 639-1 language code, e.g. "en", "de", "fr".
    /// "und" (undetermined) when detection is inconclusive.
    std::string code;

    /// Human-readable name, e.g. "English".
    std::string name;

    /// Confidence in [0.0, 1.0]. Values < 0.3 are considered low-confidence.
    float confidence = 0.0f;

    /// Number of indicator words that matched this language.
    uint32_t indicator_hits = 0;
};

/**
 * @brief Lightweight, dependency-free multi-language detector for the
 *        content ingestion pipeline.
 *
 * Uses script byte patterns and stop-word heuristics to identify the dominant
 * natural language of a UTF-8 text document.  No external libraries are
 * required; the detector is self-contained within the content module.
 *
 * ### Supported languages
 * | Code | Language  |
 * |------|-----------|
 * | en   | English   |
 * | de   | German    |
 * | fr   | French    |
 * | es   | Spanish   |
 * | it   | Italian   |
 * | nl   | Dutch     |
 * | pt   | Portuguese|
 * | pl   | Polish    |
 * | ru   | Russian   |
 * | ja   | Japanese  |
 * | zh   | Chinese   |
 * | ar   | Arabic    |
 *
 * ### Routing integration
 * The detected language code is written into
 * `ExtractionResult::metadata["detected_language"]` and
 * `ExtractionResult::metadata["language_confidence"]` by
 * `TextProcessor::extract()`.  Downstream pipeline stages (stop-word
 * filtering, stemming, OCR language selection) read these fields to apply
 * the correct language-specific processing.
 *
 * ### Thread safety
 * All public methods are const and thread-safe after construction.
 */
class LanguageDetector {
public:
    LanguageDetector() = default;

    /**
     * @brief Detect the dominant language of @p text.
     *
     * @param text  UTF-8 input text (any length; short texts yield lower
     *              confidence).
     * @return DetectedLanguage with code, name, confidence and hit count.
     *         Returns code="und" when no language scores above the minimum
     *         threshold.
     */
    DetectedLanguage detect(std::string_view text) const;

    /**
     * @brief Return the BCP 47 language code for the detected language.
     *
     * Convenience wrapper around detect().
     *
     * @param text UTF-8 input text.
     * @return ISO 639-1 code (e.g. "en") or "und".
     */
    std::string detectCode(std::string_view text) const;

    /**
     * @brief Map a language code to a routing hint for downstream processors.
     *
     * The routing hint is a coarse category used to select the correct
     * stop-word list, stemmer and OCR language pack:
     *
     * | Code(s)       | Routing hint  |
     * |---------------|---------------|
     * | en            | "latin-en"    |
     * | de            | "latin-de"    |
     * | fr            | "latin-fr"    |
     * | es            | "latin-es"    |
     * | it            | "latin-it"    |
     * | nl / pt / pl  | "latin-other" |
     * | ru            | "cyrillic"    |
     * | ar            | "arabic"      |
     * | ja            | "japanese"    |
     * | zh            | "chinese"     |
     * | und / other   | "unknown"     |
     *
     * @param language_code BCP 47 code as returned by detectCode().
     * @return Routing hint string.
     */
    static std::string routingHint(const std::string& language_code);
};

} // namespace content
} // namespace themis

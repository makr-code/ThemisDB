/*
 * ThemisDB | File: text_quality_scorer.h | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file text_quality_scorer.h
 * @brief Generic text quality scoring for NER pre-filtering.
 *
 * `TextQualityScorer` provides a lightweight quality gate that callers in
 * `training/`, `rag/`, and `aql/` can use **before** dispatching expensive
 * NER/LLM workflows.  Scoring a 10 000-character document takes < 1 ms.
 *
 * ## Free function (simplest usage)
 * @code
 * auto score = themis::toolbox::scoreText(text);
 * if (score.is_empty || score.token_count < 10) { skip(); }
 * @endcode
 *
 * ## Class usage (custom config)
 * @code
 * themis::toolbox::TextQualityScorer scorer;
 * auto score = scorer.score(text);
 * @endcode
 */

#include <cstddef>
#include <string>
#include <string_view>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// TextQualityScore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Quality metrics for a piece of text.
 *
 * Populated by `TextQualityScorer::score()`.
 */
struct TextQualityScore {
    /// Estimated token count (whitespace-split words).
    std::size_t token_count = 0;

    /// Raw byte length of the text.
    std::size_t char_count = 0;

    /// ISO 639-1 language code ("en", "de") or "und" when undetermined.
    std::string language;

    /// `true` when the text is empty or contains only whitespace.
    bool is_empty = false;

    /**
     * @brief `true` when the text is suspected boilerplate.
     *
     * Boilerplate heuristics (any of the following triggers `true`):
     *  - More than 50 % of tokens are repeated (unique_tokens / total_tokens).
     *  - Token count < 5.
     *  - Average word length > 25 characters (garbled / binary data).
     */
    bool has_boilerplate = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// TextQualityScorer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Lightweight text quality scorer.
 *
 * Computes `TextQualityScore` fields in a single linear pass over the input:
 *  1. Count bytes and whitespace-separated tokens.
 *  2. Detect language via `DefaultLanguageDetector`.
 *  3. Evaluate boilerplate heuristics.
 *
 * Uses `utils::Normalizer` internally to fold umlauts before tokenisation so
 * that German texts are handled correctly.
 *
 * Thread-safety: stateless; instances may be shared across threads.
 */
class TextQualityScorer {
public:
    TextQualityScorer()  = default;
    ~TextQualityScorer() = default;

    TextQualityScorer(const TextQualityScorer&)            = default;
    TextQualityScorer& operator=(const TextQualityScorer&) = default;

    /**
     * @brief Compute a quality score for @p text.
     *
     * @param text UTF-8 text to evaluate.
     * @return Populated `TextQualityScore`.
     */
    TextQualityScore score(std::string_view text) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Compute a quality score for @p text.
 *
 * Convenience free function.  Equivalent to `TextQualityScorer{}.score(text)`.
 *
 * @param text UTF-8 text to evaluate.
 * @return Populated `TextQualityScore`.
 */
TextQualityScore scoreText(std::string_view text);

} // namespace toolbox
} // namespace themis

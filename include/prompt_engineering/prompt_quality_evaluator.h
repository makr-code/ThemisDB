/**
 * @file prompt_quality_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "prompt_engineering/prompt_template_compiler.h"

#include <string>
#include <vector>

namespace themis {
namespace prompt_engineering {

// ── Quality check result ──────────────────────────────────────────────────────

/**
 * @brief Result of a single quality check applied to a prompt.
 */
struct QualityCheck {
    std::string id;          ///< Stable identifier, e.g. "INJ-001", "DIV-001".
    std::string description; ///< Human-readable description of what was checked.
    bool        passed;      ///< `true` if the check passed.
    std::string detail;      ///< Additional context when `!passed` (empty on pass).
};

// ── Quality report ────────────────────────────────────────────────────────────

/**
 * @brief Aggregated result of all quality checks applied to a prompt.
 *
 * `score` is the ratio of passed checks to total checks (0.0–1.0).
 * `passed()` returns `true` iff `score >= threshold` **and** no check failed.
 */
struct QualityReport {
    double                    score      = 1.0; ///< 0.0 (all fail) – 1.0 (all pass).
    double                    threshold  = 0.6; ///< Min score to be considered passing.
    std::vector<QualityCheck> failed_checks;    ///< Checks that did not pass.
    std::vector<std::string>  warnings;         ///< Non-fatal warnings.

    /// Returns `true` iff `score >= threshold` and `failed_checks` is empty.
    bool passed() const noexcept {
        return failed_checks.empty() && score >= threshold;
    }
};

// ── Configuration ─────────────────────────────────────────────────────────────

/**
 * @brief Configuration for `IPromptQualityEvaluator`.
 *
 * All thresholds are normalised to [0.0, 1.0] unless noted.
 */
struct QualityConfig {
    /**
     * @brief Patterns that must not appear in the rendered prompt text.
     *
     * Matching is case-insensitive substring search.  Each matched pattern
     * produces a separate `QualityCheck` failure with id `"INJ-<n>"`.
     * Recommended: populate from OWASP LLM Top 10 patterns.
     */
    std::vector<std::string> injection_blocklist;

    /**
     * @brief Minimum acceptable token diversity ratio.
     *
     * Token diversity = `distinct_words / total_words`.  Values below this
     * threshold fail check `"DIV-001"`.  Default: 0.30 (30 %).
     */
    double min_token_diversity = 0.30;

    /**
     * @brief Maximum acceptable consecutive-bigram repetition ratio.
     *
     * Repetition ratio = `repeated_bigrams / total_bigrams`.  Values above
     * this threshold fail check `"REP-001"`.  Default: 0.50 (50 %).
     */
    double max_repetition_ratio = 0.50;

    /**
     * @brief Minimum overall quality score for `QualityReport::passed()`.
     *
     * Default: 0.60 (60 %).
     */
    double min_score_threshold = 0.60;
};

// ── Abstract interface ────────────────────────────────────────────────────────

/**
 * @brief Stateless prompt quality regression evaluation interface.
 *
 * Implementations must not persist any raw prompt content after `evaluate()`
 * or `evaluateText()` returns.
 */
class IPromptQualityEvaluator {
public:
    virtual ~IPromptQualityEvaluator() = default;

    /**
     * @brief Evaluate quality of a compiled prompt template.
     *
     * The template is rendered against an empty `PromptContext`; slots that
     * are not filled are treated as empty strings.  Rendering errors are
     * captured as structural failures rather than propagated as exceptions.
     *
     * @param tmpl    Template to evaluate.
     * @param config  Quality configuration.
     * @return        Aggregated `QualityReport`; no internal state is retained.
     */
    virtual QualityReport evaluate(const IPromptTemplate& tmpl,
                                   const QualityConfig&   config) const = 0;

    /**
     * @brief Evaluate quality of a raw text string.
     *
     * Useful when the prompt text is already assembled and no `IPromptTemplate`
     * object is available.
     *
     * @param text    Raw prompt text.
     * @param config  Quality configuration.
     * @return        Aggregated `QualityReport`.
     */
    virtual QualityReport evaluateText(const std::string& text,
                                       const QualityConfig& config) const = 0;
};

// ── Concrete implementation ───────────────────────────────────────────────────

/**
 * @brief Concrete stateless implementation of `IPromptQualityEvaluator`.
 *
 * Applies three check classes in order: injection pattern detection, token
 * diversity, and bigram repetition.  The implementation holds no mutable
 * state; all methods are `const`.
 */
class PromptQualityEvaluator final : public IPromptQualityEvaluator {
public:
    QualityReport evaluate(const IPromptTemplate& tmpl,
                           const QualityConfig&   config) const override;

    QualityReport evaluateText(const std::string& text,
                               const QualityConfig& config) const override;

private:
    /// Perform injection pattern checks; appends to @p failed.
    void checkInjection(const std::string&              text,
                        const std::vector<std::string>& blocklist,
                        std::vector<QualityCheck>&      failed) const;

    /// Compute and check token diversity; appends to @p failed if below threshold.
    void checkTokenDiversity(const std::string&         text,
                             double                     min_diversity,
                             std::vector<QualityCheck>& failed) const;

    /// Compute and check bigram repetition; appends to @p failed if above threshold.
    void checkRepetition(const std::string&         text,
                         double                     max_repetition,
                         std::vector<QualityCheck>& failed) const;

    /// Tokenise @p text into lowercase words (splits on non-alphanumeric chars).
    static std::vector<std::string> tokenize(const std::string& text);

    /// Case-insensitive substring search.
    static bool containsIgnoreCase(const std::string& haystack,
                                   const std::string& needle);
};

} // namespace prompt_engineering
} // namespace themis

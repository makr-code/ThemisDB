/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_regression_runner.h                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 11:27:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2fa6355de3  2026-03-23  feat(prompt_engineering): Prompt Regression Runner — Prom... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_regression_runner.h
 * @brief Automated prompt quality regression detection (Phase 5 / v1.8.0).
 *
 * Provides a regression harness around `PromptEvaluator` that compares a
 * candidate (new) set of prompt outputs against a baseline (previously
 * published version) on a fixed golden-set of fixtures.  A `isRegression()`
 * flag is set when the mean quality delta drops more than
 * `RegressionConfig::max_regression_pct` (default: 5 %).
 *
 * Key types:
 *
 * - **`RegressionFixture`** — a single evaluation pair: prompt text,
 *   expected output, source tag (`"golden"` or `"feedback"`), and an
 *   optional template ID.
 * - **`RegressionConfig`** — tuning parameters: `max_regression_pct`,
 *   `min_fixtures`, `confidence_level`, `block_on_regression`.
 * - **`RegressionResult`** — immutable result of one run: per-fixture scores,
 *   mean new score, mean baseline score, `delta_pct`, `is_regression`,
 *   `blocked`, `toJson()`.
 * - **`PromptRegressionRunner`** — orchestrates the run; loads fixtures from
 *   a `FeedbackCollector` (human-feedback signal) or directly from a
 *   programmer-supplied vector; emits structured log entries via a
 *   pluggable `log_callback`.
 *
 * Grounded in:
 *   - Lin (2004) "ROUGE: A Package for Automatic Evaluation of Summaries"
 *     [ACL W04-1013]
 *   - Papineni et al. (2002) "BLEU: A Method for Automatic Evaluation of
 *     Machine Translation" [ACL 2002, DOI:10.3115/1073083.1073135]
 *   - Zheng et al. (NeurIPS 2023) "Judging LLM-as-a-Judge with MT-Bench and
 *     Chatbot Arena" [arXiv:2306.05685]
 *
 * Performance target: full regression suite (100 fixtures) < 60 s.
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "feedback_collector.h"
#include "prompt_evaluator.h"

namespace themis {
namespace prompt_engineering {

// ============================================================================
// RegressionFixture
// ============================================================================

/**
 * @brief A single evaluation fixture for regression testing.
 *
 * Fixtures originate from one of two sources:
 *  - `"golden"` — developer-curated, stable ground truth pairs
 *  - `"feedback"` — real-world human-positive feedback entries
 *
 * The `baseline_score` field is populated when `PromptRegressionRunner::run()`
 * computes the baseline pass to record old-version performance.
 */
struct RegressionFixture {
    std::string template_id;          ///< Template the fixture belongs to
    std::string prompt_text;          ///< Input prompt / query
    std::string expected_output;      ///< Ground-truth expected output
    std::string source;               ///< "golden" or "feedback"
    double      baseline_score = -1.0; ///< Score of baseline; -1 = unknown

    /** @brief Serialise to JSON. */
    nlohmann::json toJson() const;

    /** @brief Deserialise from JSON. */
    static RegressionFixture fromJson(const nlohmann::json& j);
};

// ============================================================================
// RegressionConfig
// ============================================================================

/**
 * @brief Tuning parameters for `PromptRegressionRunner`.
 */
struct RegressionConfig {
    /// Mean score may drop at most this many percentage points before the run
    /// is flagged as a regression (default: 5.0 %).
    double max_regression_pct = 5.0;

    /// Minimum number of fixtures required to produce a valid result.
    /// If fewer fixtures are available the result is flagged as inconclusive.
    std::size_t min_fixtures = 1;

    /// When `true`, `RegressionResult::blocked` is set to `true` whenever
    /// `is_regression` is `true`.  Callers can use this to gate a publish.
    bool block_on_regression = true;

    /// Confidence level used for the Welch t-test significance check
    /// (0.0–1.0, default: 0.95).
    double confidence_level = 0.95;
};

// ============================================================================
// FixtureDelta
// ============================================================================

/**
 * @brief Per-fixture score pair for detailed inspection.
 */
struct FixtureDelta {
    std::size_t index        = 0;    ///< Index in fixture vector
    std::string template_id;         ///< Template ID from fixture
    double      baseline_score = 0.0;///< Old-version score
    double      candidate_score = 0.0;///< New-version score
    double      delta          = 0.0;///< candidate - baseline
};

// ============================================================================
// RegressionResult
// ============================================================================

/**
 * @brief Immutable result of one `PromptRegressionRunner::run()` call.
 */
struct RegressionResult {
    /// Number of fixtures evaluated.
    std::size_t fixture_count = 0;

    /// Mean score of the candidate (new) version across all fixtures.
    double mean_candidate_score = 0.0;

    /// Mean score of the baseline (old) version across all fixtures.
    double mean_baseline_score = 0.0;

    /// (mean_candidate − mean_baseline) / mean_baseline × 100.
    /// Negative means regression; positive means improvement.
    double delta_pct = 0.0;

    /// `true` when `delta_pct < -max_regression_pct`.
    bool is_regression = false;

    /// `true` when `is_regression && RegressionConfig::block_on_regression`.
    bool blocked = false;

    /// `true` when fewer than `RegressionConfig::min_fixtures` were available.
    bool inconclusive = false;

    /// Whether the score delta is statistically significant (Welch t-test).
    bool statistically_significant = false;

    /// Per-fixture breakdown.
    std::vector<FixtureDelta> fixture_deltas;

    /** @brief Serialise to JSON for logging / audit. */
    nlohmann::json toJson() const;
};

// ============================================================================
// PromptRegressionRunner
// ============================================================================

/**
 * @brief Automated prompt quality regression runner.
 *
 * Usage — basic:
 * @code
 * PromptRegressionRunner runner;
 * runner.setFixtures(my_golden_fixtures);
 * auto result = runner.run(baseline_outputs, candidate_outputs);
 * if (result.blocked) { throw std::runtime_error("Regression detected"); }
 * @endcode
 *
 * Usage — with FeedbackCollector integration:
 * @code
 * runner.loadFeedbackFixtures(collector, "my_template_id", 50);
 * auto result = runner.run(baseline_outputs, candidate_outputs);
 * @endcode
 */
class PromptRegressionRunner {
public:
    /// Callback type for structured log events.
    using LogCallback = std::function<void(const nlohmann::json&)>;

    /**
     * @brief Construct with optional evaluator config and runner config.
     * @param eval_config  PromptEvaluator settings.
     * @param run_config   Regression runner settings.
     */
    explicit PromptRegressionRunner(
        const EvaluatorConfig&   eval_config = EvaluatorConfig{},
        const RegressionConfig&  run_config  = RegressionConfig{});

    // -------------------------------------------------------------------------
    // Fixture management
    // -------------------------------------------------------------------------

    /**
     * @brief Replace the entire fixture set.
     * @param fixtures  Fixtures to use for all subsequent `run()` calls.
     */
    void setFixtures(std::vector<RegressionFixture> fixtures);

    /**
     * @brief Append fixtures derived from positive human feedback.
     *
     * Pulls `limit` entries with `FeedbackType::USER_POSITIVE` for the given
     * `template_id` from @p collector.  Each entry becomes a fixture whose
     * `expected_output` is the recorded response and `source` is `"feedback"`.
     *
     * @param collector   FeedbackCollector to query (not owned).
     * @param template_id Template ID to filter feedback for.
     * @param limit       Maximum entries to import (0 = all).
     */
    void loadFeedbackFixtures(
        const FeedbackCollector& collector,
        const std::string&       template_id,
        std::size_t              limit = 100);

    /**
     * @brief Remove all fixtures.
     */
    void clearFixtures();

    /** @brief Return the current fixture count. */
    std::size_t fixtureCount() const noexcept;

    // -------------------------------------------------------------------------
    // Run
    // -------------------------------------------------------------------------

    /**
     * @brief Run the regression suite.
     *
     * Evaluates each fixture using `PromptEvaluator::evaluateSingle()`.
     * `baseline_outputs[i]` is the old-version output for `fixtures_[i]`;
     * `candidate_outputs[i]` is the new-version output.
     *
     * If vector sizes differ or fewer than `config_.min_fixtures` fixtures are
     * available, `RegressionResult::inconclusive` is set to `true`.
     *
     * @param baseline_outputs  Outputs from the current (published) version.
     * @param candidate_outputs Outputs from the candidate (new) version.
     * @return `RegressionResult` with all scores, delta_pct, and flags.
     */
    RegressionResult run(
        const std::vector<std::string>& baseline_outputs,
        const std::vector<std::string>& candidate_outputs) const;

    // -------------------------------------------------------------------------
    // Logging
    // -------------------------------------------------------------------------

    /**
     * @brief Attach a log callback.
     *
     * The callback is invoked once per `run()` with a structured JSON log
     * entry containing `template_id`, fixture count, scores, delta, and
     * regression flag.
     *
     * @param cb  Callback; pass an empty `std::function` to disable.
     */
    void setLogCallback(LogCallback cb);

    // -------------------------------------------------------------------------
    // Config accessors
    // -------------------------------------------------------------------------

    /** @brief Return the current evaluator config. */
    const EvaluatorConfig& evalConfig() const noexcept;

    /** @brief Return the current runner config. */
    const RegressionConfig& runConfig() const noexcept;

    /** @brief Update the runner config. */
    void setRunConfig(const RegressionConfig& cfg);

private:
    EvaluatorConfig  eval_config_;
    RegressionConfig run_config_;
    PromptEvaluator  evaluator_;

    std::vector<RegressionFixture> fixtures_;
    LogCallback                    log_callback_;

    /** @brief Emit a structured log entry (no-op when callback is empty). */
    void emitLog(const RegressionResult& result,
                 const std::string&       template_id) const;
};

} // namespace prompt_engineering
} // namespace themis

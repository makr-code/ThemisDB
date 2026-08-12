/**
 * @file prompt_regression_runner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_regression_runner.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// RegressionFixture
// ============================================================================

nlohmann::json RegressionFixture::toJson() const {
    return {
        {"template_id",      template_id},
        {"prompt_text",      prompt_text},
        {"expected_output",  expected_output},
        {"source",           source},
        {"baseline_score",   baseline_score}
    };
}

RegressionFixture RegressionFixture::fromJson(const nlohmann::json& j) {
    RegressionFixture f;
    f.template_id     = j.value("template_id",     std::string{});
    f.prompt_text     = j.value("prompt_text",     std::string{});
    f.expected_output = j.value("expected_output", std::string{});
    f.source          = j.value("source",          std::string{"golden"});
    f.baseline_score  = j.value("baseline_score",  -1.0);
    return f;
}

// ============================================================================
// RegressionResult::toJson
// ============================================================================

nlohmann::json RegressionResult::toJson() const {
    auto deltas = nlohmann::json::array();
    for (const auto& d : fixture_deltas) {
        deltas.push_back({
            {"index",            d.index},
            {"template_id",      d.template_id},
            {"baseline_score",   d.baseline_score},
            {"candidate_score",  d.candidate_score},
            {"delta",            d.delta}
        });
    }
    return {
        {"fixture_count",              fixture_count},
        {"mean_candidate_score",       mean_candidate_score},
        {"mean_baseline_score",        mean_baseline_score},
        {"delta_pct",                  delta_pct},
        {"is_regression",              is_regression},
        {"blocked",                    blocked},
        {"inconclusive",               inconclusive},
        {"statistically_significant",  statistically_significant},
        {"fixture_deltas",             deltas}
    };
}

// ============================================================================
// PromptRegressionRunner — constructor
// ============================================================================

PromptRegressionRunner::PromptRegressionRunner(
    const EvaluatorConfig&  eval_config,
    const RegressionConfig& run_config)
    : eval_config_(eval_config),
      run_config_(run_config),
      evaluator_(eval_config) {}

// ============================================================================
// Fixture management
// ============================================================================

void PromptRegressionRunner::setFixtures(
    std::vector<RegressionFixture> fixtures) {
    fixtures_ = std::move(fixtures);
}

void PromptRegressionRunner::loadFeedbackFixtures(
    const FeedbackCollector& collector,
    const std::string&       template_id,
    std::size_t              limit) {
    const auto entries =
        collector.getFeedback(template_id, limit,
                              FeedbackType::USER_POSITIVE);
    for (const auto& e : entries) {
        RegressionFixture f;
        f.template_id     = template_id;
        f.prompt_text     = e.query;
        f.expected_output = e.response;
        f.source          = "feedback";
        fixtures_.push_back(std::move(f));
    }
}

void PromptRegressionRunner::clearFixtures() {
    fixtures_.clear();
}

std::size_t PromptRegressionRunner::fixtureCount() const noexcept {
    return fixtures_.size();
}

// ============================================================================
// run()
// ============================================================================

RegressionResult PromptRegressionRunner::run(
    const std::vector<std::string>& baseline_outputs,
    const std::vector<std::string>& candidate_outputs) const {

    RegressionResult result;
    result.fixture_count = fixtures_.size();

    // Determine the active fixture count (limited by output vector sizes).
    const std::size_t n =
        std::min({fixtures_.size(),
                  baseline_outputs.size(),
                  candidate_outputs.size()});

    // Inconclusive: too few fixtures or mismatched vector sizes.
    if (n < run_config_.min_fixtures ||
        baseline_outputs.size() != candidate_outputs.size() ||
        n == 0) {
        result.inconclusive = true;
        return result;
    }

    result.fixture_count = n;

    std::vector<double> baseline_scores;
    std::vector<double> candidate_scores;
    baseline_scores.reserve(n);
    candidate_scores.reserve(n);
    result.fixture_deltas.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        const auto& fixture = fixtures_[i];

        const auto bm = evaluator_.evaluateSingle(
            baseline_outputs[i], fixture.expected_output);
        const auto cm = evaluator_.evaluateSingle(
            candidate_outputs[i], fixture.expected_output);

                const auto weighted_score = [this](const EvaluationMetrics& metrics) {
                        const double partial_fallback = metrics.partial_match;
                        const double semantic_component = std::max(metrics.semantic_similarity,
                                                                                                            partial_fallback);
                        const double exact_component = std::max(metrics.exact_match,
                                                                                                     partial_fallback);
                        const double relevance_component = std::max(metrics.relevance,
                                                                                                             partial_fallback);

                        return eval_config_.similarity_weight * semantic_component +
                                     eval_config_.exact_match_weight * exact_component +
                                     eval_config_.relevance_weight * relevance_component;
                };

                // overall_score = weighted combination.
                const double bs = weighted_score(bm);
                const double cs = weighted_score(cm);

        baseline_scores.push_back(bs);
        candidate_scores.push_back(cs);

        FixtureDelta d;
        d.index           = i;
        d.template_id     = fixture.template_id;
        d.baseline_score  = bs;
        d.candidate_score = cs;
        d.delta           = cs - bs;
        result.fixture_deltas.push_back(d);
    }

    // Compute means.
    const double mean_b =
        std::accumulate(baseline_scores.begin(),
                        baseline_scores.end(), 0.0) /
        static_cast<double>(n);

    const double mean_c =
        std::accumulate(candidate_scores.begin(),
                        candidate_scores.end(), 0.0) /
        static_cast<double>(n);

    result.mean_baseline_score  = mean_b;
    result.mean_candidate_score = mean_c;

    // delta_pct: (candidate - baseline) / baseline × 100.
    // When the baseline is exactly zero, treat any positive candidate score as
    // a positive lift from a degenerate baseline instead of flattening to 0.
    if (mean_b > 0.0) {
        result.delta_pct = (mean_c - mean_b) / mean_b * 100.0;
    } else if (mean_c > 0.0) {
        result.delta_pct = mean_c * 100.0;
    } else {
        result.delta_pct = 0.0;
    }

    // Regression flag.
    result.is_regression =
        (result.delta_pct < -run_config_.max_regression_pct);

    // Block flag.
    result.blocked =
        result.is_regression && run_config_.block_on_regression;

    // Statistical significance (Welch t-test via PromptEvaluator).
    result.statistically_significant =
        PromptEvaluator::isStatisticallySignificant(
            baseline_scores, candidate_scores,
            run_config_.confidence_level);

    // Emit structured log.
    std::string tid = fixtures_.empty() ? "" : fixtures_.front().template_id;
    emitLog(result, tid);

    return result;
}

// ============================================================================
// Logging
// ============================================================================

void PromptRegressionRunner::setLogCallback(LogCallback cb) {
    log_callback_ = std::move(cb);
}

void PromptRegressionRunner::emitLog(const RegressionResult& result,
                                     const std::string&       template_id) const {
    if (!log_callback_) { return; }
    try {
        const auto ts = static_cast<std::int64_t>(
            std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()));
        log_callback_({
            {"event",          "prompt_regression_run"},
            {"template_id",    template_id},
            {"fixture_count",  result.fixture_count},
            {"mean_candidate", result.mean_candidate_score},
            {"mean_baseline",  result.mean_baseline_score},
            {"delta_pct",      result.delta_pct},
            {"is_regression",  result.is_regression},
            {"blocked",        result.blocked},
            {"inconclusive",   result.inconclusive},
            {"timestamp",      ts}
        });
    } catch (...) {
        // Log callback must never break the caller.
    }
}

// ============================================================================
// Config accessors
// ============================================================================

const EvaluatorConfig& PromptRegressionRunner::evalConfig() const noexcept {
    return eval_config_;
}

const RegressionConfig& PromptRegressionRunner::runConfig() const noexcept {
    return run_config_;
}

void PromptRegressionRunner::setRunConfig(const RegressionConfig& cfg) {
    run_config_ = cfg;
}

} // namespace prompt_engineering
} // namespace themis


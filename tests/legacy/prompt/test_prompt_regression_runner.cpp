/**
 * @file test_prompt_regression_runner.cpp
 * @brief Unit tests for PromptRegressionRunner (v1.8.0).
 *
 * Acceptance criteria:
 *  AC-1   PromptRegressionRunner default-constructs without error.
 *  AC-2   fixtureCount() starts at 0.
 *  AC-3   setFixtures() replaces all fixtures.
 *  AC-4   clearFixtures() empties the fixture set.
 *  AC-5   run() with 0 fixtures returns inconclusive=true.
 *  AC-6   run() with mismatched output vector sizes returns inconclusive=true.
 *  AC-7   run() with fewer outputs than min_fixtures returns inconclusive=true.
 *  AC-8   run() with identical outputs returns delta_pct == 0 and is_regression=false.
 *  AC-9   run() with improved candidate outputs returns positive delta_pct.
 *  AC-10  run() with regressed candidate outputs returns negative delta_pct.
 *  AC-11  is_regression=true when delta_pct < -max_regression_pct.
 *  AC-12  is_regression=false when delta_pct within max_regression_pct.
 *  AC-13  blocked=true when is_regression=true and block_on_regression=true.
 *  AC-14  blocked=false when block_on_regression=false even if is_regression=true.
 *  AC-15  fixture_deltas has one entry per fixture.
 *  AC-16  FixtureDelta.delta == candidate_score - baseline_score.
 *  AC-17  mean_baseline_score is the arithmetic mean of baseline scores.
 *  AC-18  mean_candidate_score is the arithmetic mean of candidate scores.
 *  AC-19  delta_pct formula: (mean_c - mean_b) / mean_b * 100.
 *  AC-20  RegressionResult::toJson() contains all required keys.
 *  AC-21  RegressionFixture::toJson() / fromJson() round-trips correctly.
 *  AC-22  Log callback is invoked once per run() call.
 *  AC-23  Log callback receives correct template_id, delta_pct, is_regression.
 *  AC-24  Misbehaving log callback (throws) does not abort run().
 *  AC-25  loadFeedbackFixtures() imports USER_POSITIVE entries from FeedbackCollector.
 *  AC-26  loadFeedbackFixtures() appends to existing fixtures.
 *  AC-27  loadFeedbackFixtures() respects the limit parameter.
 *  AC-28  setRunConfig() updates the active runner config.
 *  AC-29  Multiple run() calls are independent (no state leakage between runs).
 *  AC-30  inconclusive=false for a valid run with sufficient fixtures.
 */

#include <gtest/gtest.h>

#include "prompt_engineering/prompt_regression_runner.h"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/** Build a list of N identical fixtures with a fixed expected_output. */
std::vector<RegressionFixture> makeFixtures(
    std::size_t       n,
    const std::string& template_id     = "tpl",
    const std::string& prompt_text     = "What is 2+2?",
    const std::string& expected_output = "four",
    const std::string& source          = "golden") {
    std::vector<RegressionFixture> out = {};

    for (std::size_t i = 0; i < n; ++i) {
        RegressionFixture f;
        f.template_id     = template_id;
        f.prompt_text     = prompt_text + " #" + std::to_string(i);
        f.expected_output = expected_output;
        f.source          = source;
        out.push_back(f);
    }
    return out;
}

/** Produce N copies of the same string. */
std::vector<std::string> repeat(const std::string& s, std::size_t n) {
    return std::vector<std::string>(n, s);
}

} // anonymous namespace

// ============================================================================
// AC-1  Default construction
// ============================================================================

TEST(PromptRegressionRunnerTest, DefaultConstructs) {
    EXPECT_NO_THROW(PromptRegressionRunner runner);
}

// ============================================================================
// AC-2  fixtureCount starts at 0
// ============================================================================

TEST(PromptRegressionRunnerTest, FixtureCountStartsAtZero) {
    PromptRegressionRunner runner;
    EXPECT_EQ(runner.fixtureCount(), 0u);
}

// ============================================================================
// AC-3  setFixtures replaces all fixtures
// ============================================================================

TEST(PromptRegressionRunnerTest, SetFixturesReplacesAll) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(5));
    EXPECT_EQ(runner.fixtureCount(), 5u);
    runner.setFixtures(makeFixtures(2));
    EXPECT_EQ(runner.fixtureCount(), 2u);
}

// ============================================================================
// AC-4  clearFixtures empties the fixture set
// ============================================================================

TEST(PromptRegressionRunnerTest, ClearFixturesEmpties) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3));
    runner.clearFixtures();
    EXPECT_EQ(runner.fixtureCount(), 0u);
}

// ============================================================================
// AC-5  run() with 0 fixtures returns inconclusive
// ============================================================================

TEST(PromptRegressionRunnerTest, ZeroFixturesInconclusive) {
    PromptRegressionRunner runner;
    const auto result = runner.run({}, {});
    EXPECT_TRUE(result.inconclusive);
}

// ============================================================================
// AC-6  run() with mismatched output vector sizes returns inconclusive
// ============================================================================

TEST(PromptRegressionRunnerTest, MismatchedOutputSizesInconclusive) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3));
    const auto result = runner.run({"a", "b"}, {"a", "b", "c"});
    EXPECT_TRUE(result.inconclusive);
}

// ============================================================================
// AC-7  run() with fewer outputs than min_fixtures returns inconclusive
// ============================================================================

TEST(PromptRegressionRunnerTest, BelowMinFixturesInconclusive) {
    RegressionConfig cfg;
    cfg.min_fixtures = 5;
    PromptRegressionRunner runner(EvaluatorConfig{}, cfg);
    runner.setFixtures(makeFixtures(3));
    const auto result =
        runner.run(repeat("four", 3), repeat("four", 3));
    EXPECT_TRUE(result.inconclusive);
}

// ============================================================================
// AC-8  Identical outputs → delta_pct == 0, is_regression=false
// ============================================================================

TEST(PromptRegressionRunnerTest, IdenticalOutputsNoDelta) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(5, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 5), repeat("four", 5));
    EXPECT_FALSE(result.inconclusive);
    EXPECT_DOUBLE_EQ(result.delta_pct, 0.0);
    EXPECT_FALSE(result.is_regression);
}

// ============================================================================
// AC-9  Improved candidate → positive delta_pct
// ============================================================================

TEST(PromptRegressionRunnerTest, ImprovedCandidatePositiveDelta) {
    PromptRegressionRunner runner;
    // expected_output = "four"
    runner.setFixtures(makeFixtures(4, "tpl", "q", "four"));
    // baseline: unrelated output → low score
    // candidate: matches expected → higher score
    const auto result =
        runner.run(repeat("xyz", 4), repeat("four", 4));
    EXPECT_FALSE(result.inconclusive);
    EXPECT_GT(result.delta_pct, 0.0);
    EXPECT_FALSE(result.is_regression);
}

// ============================================================================
// AC-10  Regressed candidate → negative delta_pct
// ============================================================================

TEST(PromptRegressionRunnerTest, RegressedCandidateNegativeDelta) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(4, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 4), repeat("xyz", 4));
    EXPECT_FALSE(result.inconclusive);
    EXPECT_LT(result.delta_pct, 0.0);
}

// ============================================================================
// AC-11  is_regression=true when delta_pct < -max_regression_pct
// ============================================================================

TEST(PromptRegressionRunnerTest, RegressionFlagSetOnBigDrop) {
    RegressionConfig cfg;
    cfg.max_regression_pct = 2.0;   // 2% tolerance
    cfg.block_on_regression = false;
    PromptRegressionRunner runner(EvaluatorConfig{}, cfg);
    runner.setFixtures(makeFixtures(5, "tpl", "q", "four"));
    // good baseline; broken candidate
    const auto result =
        runner.run(repeat("four", 5), repeat("zzz", 5));
    EXPECT_LT(result.delta_pct, -2.0);
    EXPECT_TRUE(result.is_regression);
}

// ============================================================================
// AC-12  is_regression=false within max_regression_pct
// ============================================================================

TEST(PromptRegressionRunnerTest, NoRegressionWithinTolerance) {
    RegressionConfig cfg;
    cfg.max_regression_pct = 50.0;  // very wide tolerance
    PromptRegressionRunner runner(EvaluatorConfig{}, cfg);
    runner.setFixtures(makeFixtures(4, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 4), repeat("foru", 4));  // minor degradation
    EXPECT_FALSE(result.is_regression);
}

// ============================================================================
// AC-13  blocked=true when is_regression && block_on_regression
// ============================================================================

TEST(PromptRegressionRunnerTest, BlockedOnRegressionWithFlag) {
    RegressionConfig cfg;
    cfg.max_regression_pct  = 2.0;
    cfg.block_on_regression = true;
    PromptRegressionRunner runner(EvaluatorConfig{}, cfg);
    runner.setFixtures(makeFixtures(3, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 3), repeat("zzz", 3));
    EXPECT_TRUE(result.is_regression);
    EXPECT_TRUE(result.blocked);
}

// ============================================================================
// AC-14  blocked=false when block_on_regression=false
// ============================================================================

TEST(PromptRegressionRunnerTest, NotBlockedWhenFlagDisabled) {
    RegressionConfig cfg;
    cfg.max_regression_pct  = 2.0;
    cfg.block_on_regression = false;
    PromptRegressionRunner runner(EvaluatorConfig{}, cfg);
    runner.setFixtures(makeFixtures(3, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 3), repeat("zzz", 3));
    EXPECT_TRUE(result.is_regression);
    EXPECT_FALSE(result.blocked);
}

// ============================================================================
// AC-15  fixture_deltas has one entry per fixture
// ============================================================================

TEST(PromptRegressionRunnerTest, FixtureDeltasCountMatchesFixtures) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(7));
    const auto result =
        runner.run(repeat("four", 7), repeat("four", 7));
    EXPECT_EQ(result.fixture_deltas.size(), 7u);
}

// ============================================================================
// AC-16  FixtureDelta.delta == candidate - baseline
// ============================================================================

TEST(PromptRegressionRunnerTest, FixtureDeltaEqualsScoreDiff) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3));
    const auto result =
        runner.run(repeat("four", 3), repeat("four", 3));
    for (const auto& d : result.fixture_deltas) {
        EXPECT_NEAR(d.delta,
                    d.candidate_score - d.baseline_score,
                    1e-9);
    }
}

// ============================================================================
// AC-17  mean_baseline_score is the arithmetic mean of baseline scores
// ============================================================================

TEST(PromptRegressionRunnerTest, MeanBaselineIsArithmeticMean) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(4, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 4), repeat("four", 4));
    double sum = 0.0;
    for (const auto& d : result.fixture_deltas) {
        sum += d.baseline_score;
    }
    const double expected_mean = sum / 4.0;
    EXPECT_NEAR(result.mean_baseline_score, expected_mean, 1e-9);
}

// ============================================================================
// AC-18  mean_candidate_score is arithmetic mean of candidate scores
// ============================================================================

TEST(PromptRegressionRunnerTest, MeanCandidateIsArithmeticMean) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(4, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 4), repeat("four", 4));
    double sum = 0.0;
    for (const auto& d : result.fixture_deltas) {
        sum += d.candidate_score;
    }
    const double expected_mean = sum / 4.0;
    EXPECT_NEAR(result.mean_candidate_score, expected_mean, 1e-9);
}

// ============================================================================
// AC-19  delta_pct == (mean_c - mean_b) / mean_b * 100
// ============================================================================

TEST(PromptRegressionRunnerTest, DeltaPctFormula) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(5, "tpl", "q", "four"));
    const auto result =
        runner.run(repeat("four", 5), repeat("four", 5));
    if (result.mean_baseline_score > 0.0) {
        const double expected =
            (result.mean_candidate_score - result.mean_baseline_score) /
            result.mean_baseline_score * 100.0;
        EXPECT_NEAR(result.delta_pct, expected, 1e-6);
    }
}

// ============================================================================
// AC-20  RegressionResult::toJson() contains all required keys
// ============================================================================

TEST(PromptRegressionRunnerTest, ResultToJsonContainsAllKeys) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(2));
    const auto result = runner.run(repeat("four", 2), repeat("four", 2));
    const auto j = result.toJson();
    EXPECT_TRUE(j.contains("fixture_count"));
    EXPECT_TRUE(j.contains("mean_candidate_score"));
    EXPECT_TRUE(j.contains("mean_baseline_score"));
    EXPECT_TRUE(j.contains("delta_pct"));
    EXPECT_TRUE(j.contains("is_regression"));
    EXPECT_TRUE(j.contains("blocked"));
    EXPECT_TRUE(j.contains("inconclusive"));
    EXPECT_TRUE(j.contains("statistically_significant"));
    EXPECT_TRUE(j.contains("fixture_deltas"));
}

// ============================================================================
// AC-21  RegressionFixture round-trip JSON
// ============================================================================

TEST(PromptRegressionRunnerTest, FixtureJsonRoundTrip) {
    RegressionFixture f;
    f.template_id     = "tpl-42";
    f.prompt_text     = "prompt text";
    f.expected_output = "expected";
    f.source          = "golden";
    f.baseline_score  = 0.87;

    const auto j    = f.toJson();
    const auto back = RegressionFixture::fromJson(j);

    EXPECT_EQ(back.template_id,     f.template_id);
    EXPECT_EQ(back.prompt_text,     f.prompt_text);
    EXPECT_EQ(back.expected_output, f.expected_output);
    EXPECT_EQ(back.source,          f.source);
    EXPECT_DOUBLE_EQ(back.baseline_score, f.baseline_score);
}

// ============================================================================
// AC-22  Log callback is invoked once per run() call
// ============================================================================

TEST(PromptRegressionRunnerTest, LogCallbackInvokedOnce) {
    int call_count = 0;
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3));
    runner.setLogCallback([&](const nlohmann::json&) { ++call_count; });

    runner.run(repeat("four", 3), repeat("four", 3));
    EXPECT_EQ(call_count, 1);

    runner.run(repeat("four", 3), repeat("four", 3));
    EXPECT_EQ(call_count, 2);
}

// ============================================================================
// AC-23  Log callback receives correct template_id, delta_pct, is_regression
// ============================================================================

TEST(PromptRegressionRunnerTest, LogCallbackReceivesCorrectFields) {
    nlohmann::json captured;
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3, "golden-tpl"));
    runner.setLogCallback([&](const nlohmann::json& j) { captured = j; });

    const auto result = runner.run(repeat("four", 3), repeat("four", 3));
    EXPECT_EQ(captured.value("template_id", std::string{}), "golden-tpl");
    EXPECT_NEAR(captured.value("delta_pct", -999.0), result.delta_pct, 1e-6);
    EXPECT_EQ(captured.value("is_regression", true), result.is_regression);
}

// ============================================================================
// AC-24  Misbehaving log callback (throws) does not abort run()
// ============================================================================

TEST(PromptRegressionRunnerTest, ThrowingLogCallbackDoesNotAbortRun) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3));
    runner.setLogCallback([](const nlohmann::json&) {
        throw std::runtime_error("log boom");
    });
    RegressionResult result;
    EXPECT_NO_THROW(result = runner.run(repeat("four", 3), repeat("four", 3)));
    EXPECT_FALSE(result.inconclusive);
}

// ============================================================================
// AC-25  loadFeedbackFixtures imports USER_POSITIVE entries
// ============================================================================

TEST(PromptRegressionRunnerTest, LoadFeedbackFixturesImportsPositiveEntries) {
    FeedbackCollector collector;
    collector.recordFeedback("tpl-A", "query1", "response1",
                             FeedbackType::USER_POSITIVE);
    collector.recordFeedback("tpl-A", "query2", "response2",
                             FeedbackType::USER_POSITIVE);
    collector.recordFeedback("tpl-A", "query3", "response3",
                             FeedbackType::USER_NEGATIVE);  // should be excluded

    PromptRegressionRunner runner;
    runner.loadFeedbackFixtures(collector, "tpl-A");
    EXPECT_EQ(runner.fixtureCount(), 2u);  // only the 2 positive entries
}

// ============================================================================
// AC-26  loadFeedbackFixtures appends to existing fixtures
// ============================================================================

TEST(PromptRegressionRunnerTest, LoadFeedbackFixturesAppends) {
    FeedbackCollector collector;
    collector.recordFeedback("tpl-B", "q", "r",
                             FeedbackType::USER_POSITIVE);

    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(3, "golden-tpl"));
    EXPECT_EQ(runner.fixtureCount(), 3u);

    runner.loadFeedbackFixtures(collector, "tpl-B");
    EXPECT_EQ(runner.fixtureCount(), 4u);
}

// ============================================================================
// AC-27  loadFeedbackFixtures respects the limit parameter
// ============================================================================

TEST(PromptRegressionRunnerTest, LoadFeedbackFixturesRespectsLimit) {
    FeedbackCollector collector;
    for (int i = 0; i < 10; ++i) {
        collector.recordFeedback("tpl-C",
                                 "q" + std::to_string(i),
                                 "r" + std::to_string(i),
                                 FeedbackType::USER_POSITIVE);
    }

    PromptRegressionRunner runner;
    runner.loadFeedbackFixtures(collector, "tpl-C", 4);
    EXPECT_LE(runner.fixtureCount(), 4u);
}

// ============================================================================
// AC-28  setRunConfig updates the active runner config
// ============================================================================

TEST(PromptRegressionRunnerTest, SetRunConfigUpdatesConfig) {
    PromptRegressionRunner runner;
    EXPECT_DOUBLE_EQ(runner.runConfig().max_regression_pct, 5.0);

    RegressionConfig cfg;
    cfg.max_regression_pct = 10.0;
    runner.setRunConfig(cfg);
    EXPECT_DOUBLE_EQ(runner.runConfig().max_regression_pct, 10.0);
}

// ============================================================================
// AC-29  Multiple run() calls are independent (no state leakage)
// ============================================================================

TEST(PromptRegressionRunnerTest, MultipleRunCallsAreIndependent) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(4, "tpl", "q", "four"));

    const auto r1 = runner.run(repeat("four", 4), repeat("four", 4));
    const auto r2 = runner.run(repeat("four", 4), repeat("zzz",  4));

    // r1 should show no regression; r2 should show regression.
    EXPECT_FALSE(r1.is_regression);
    EXPECT_LT(r2.delta_pct, r1.delta_pct);
    // Verify r1 is unaffected by r2.
    EXPECT_DOUBLE_EQ(r1.delta_pct, 0.0);
}

// ============================================================================
// AC-30  inconclusive=false for a valid run with sufficient fixtures
// ============================================================================

TEST(PromptRegressionRunnerTest, ValidRunNotInconclusive) {
    PromptRegressionRunner runner;
    runner.setFixtures(makeFixtures(5));
    const auto result =
        runner.run(repeat("four", 5), repeat("four", 5));
    EXPECT_FALSE(result.inconclusive);
}

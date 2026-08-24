/**
 * @file test_evaluation_ablation_framework_focused.cpp
 * @brief Group AF — Unit tests for the ablation study framework.
 *
 * Exercises AblationRunner::addExperiment(), run(), and reset(), as well
 * as the AblationReport best-result helpers.
 */

#include <gtest/gtest.h>
#include "ablation_framework.h"
#include "retrieval_metrics.h"
#include <vector>
#include <string>

using namespace themis::evaluation;

// ── helpers ───────────────────────────────────────────────────────────────────

static AblationQuery make_simple_query(const std::string& qid, bool relevant_first) {
    AblationQuery q;
    q.query_id    = qid;
    q.ground_truth = {"doc1", "doc2"};
    q.k           = 2;
    q.total_candidates = 10;

    if (relevant_first) {
        q.results = {{"doc1", 2.0}, {"doc2", 1.0}};
    } else {
        q.results = {{"docX", 2.0}, {"docY", 1.0}};
    }

    // Minimal valid snapshot
    q.snapshot.residual_error = 0.02;
    q.snapshot.rank_cap_used  = 4;
    q.snapshot.rank_cap_limit = 8;
    q.snapshot.graph_finalization_passed = true;
    return q;
}

static AblationConfig make_config(PathVariant path) {
    AblationConfig c;
    c.path_variant      = path;
    c.freshness_variant = FreshnessVariant::Fresh;
    c.update_variant    = UpdateVariant::Patch;
    c.compute_variant   = ComputeVariant::Cpu;
    return c;
}

// ── Group AF — Ablation Framework ─────────────────────────────────────────────

// AF1: Fresh runner has 0 experiments
TEST(EvaluationAblationFrameworkFocusedTests, AF1_InitialExperimentCount_IsZero) {
    AblationRunner runner;
    EXPECT_EQ(runner.experimentCount(), 0u);
}

// AF2: addExperiment increments count
TEST(EvaluationAblationFrameworkFocusedTests, AF2_AddExperiment_IncrementsCount) {
    AblationRunner runner;
    runner.addExperiment("exp1", make_config(PathVariant::AnnOnly));
    EXPECT_EQ(runner.experimentCount(), 1u);
}

// AF3: addExperiment with empty name throws AblationError
TEST(EvaluationAblationFrameworkFocusedTests, AF3_EmptyExperimentName_ThrowsAblationError) {
    AblationRunner runner;
    EXPECT_THROW(
        runner.addExperiment("", make_config(PathVariant::AnnOnly)),
        AblationError
    );
}

// AF4: duplicate experiment name throws AblationError
TEST(EvaluationAblationFrameworkFocusedTests, AF4_DuplicateExperimentName_ThrowsAblationError) {
    AblationRunner runner;
    runner.addExperiment("exp1", make_config(PathVariant::AnnOnly));
    EXPECT_THROW(
        runner.addExperiment("exp1", make_config(PathVariant::AnnTensor)),
        AblationError
    );
}

// AF5: run() with empty query batch throws AblationError
TEST(EvaluationAblationFrameworkFocusedTests, AF5_RunEmptyQueryBatch_ThrowsAblationError) {
    AblationRunner runner;
    runner.addExperiment("exp1", make_config(PathVariant::AnnOnly));
    EXPECT_THROW(runner.run({}), AblationError);
}

// AF6: run() with no registered experiments throws AblationError
TEST(EvaluationAblationFrameworkFocusedTests, AF6_RunNoExperiments_ThrowsAblationError) {
    AblationRunner runner;
    auto q = make_simple_query("q1", true);
    EXPECT_THROW(runner.run({q}), AblationError);
}

// AF7: run() returns a report with one result per experiment
TEST(EvaluationAblationFrameworkFocusedTests, AF7_Run_ReturnsOneResultPerExperiment) {
    AblationRunner runner;
    runner.addExperiment("ann-only",  make_config(PathVariant::AnnOnly));
    runner.addExperiment("ann+tensor", make_config(PathVariant::AnnTensor));

    auto q = make_simple_query("q1", true);
    auto report = runner.run({q});

    EXPECT_EQ(report.results.size(), 2u);
}

// AF8: mean_recall_at_k is 1.0 when all results are relevant
TEST(EvaluationAblationFrameworkFocusedTests, AF8_PerfectRecall_MeanRecallIsOne) {
    AblationRunner runner;
    runner.addExperiment("perf", make_config(PathVariant::AnnOnly));

    auto q = make_simple_query("q1", /*relevant_first=*/true);
    auto report = runner.run({q});

    ASSERT_EQ(report.results.size(), 1u);
    EXPECT_DOUBLE_EQ(report.results[0].mean_recall_at_k, 1.0);
}

// AF9: query_count equals number of queries submitted
TEST(EvaluationAblationFrameworkFocusedTests, AF9_QueryCount_MatchesInput) {
    AblationRunner runner;
    runner.addExperiment("e1", make_config(PathVariant::AnnOnly));

    std::vector<AblationQuery> queries = {
        make_simple_query("q1", true),
        make_simple_query("q2", false),
        make_simple_query("q3", true),
    };
    auto report = runner.run(queries);

    ASSERT_EQ(report.results.size(), 1u);
    EXPECT_EQ(report.results[0].query_count, 3u);
}

// AF10: reset() clears all experiments
TEST(EvaluationAblationFrameworkFocusedTests, AF10_Reset_ClearsExperiments) {
    AblationRunner runner;
    runner.addExperiment("e1", make_config(PathVariant::AnnOnly));
    runner.reset();
    EXPECT_EQ(runner.experimentCount(), 0u);
}

// AF11: Results name matches registered experiment name
TEST(EvaluationAblationFrameworkFocusedTests, AF11_ResultName_MatchesRegisteredName) {
    AblationRunner runner;
    runner.addExperiment("my-experiment", make_config(PathVariant::AnnOnly));
    auto q = make_simple_query("q1", true);
    auto report = runner.run({q});
    ASSERT_EQ(report.results.size(), 1u);
    EXPECT_EQ(report.results[0].name, "my-experiment");
}

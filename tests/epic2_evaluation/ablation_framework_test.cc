/**
 * @file ablation_framework_test.cc
 * @brief Unit tests for the ablation study framework (EPIC 2 Phase 4).
 *
 * Covers:
 *  - AF-01..AF-06: AblationRunner registration and error handling (empty name,
 *                  duplicate name, empty queries, no experiments, run output
 *                  structure, error_count accumulation).
 *  - AB-01..AB-08: Ablation variant behavior (ANN-only baseline, ANN+Tensor,
 *                  ANN+Tensor+Graph, stale freshness triggers exact fallback,
 *                  patch vs rebuild triggers rebuild flag, GPU requested but
 *                  unavailable records gpu_fallback, unrecovered false negatives
 *                  detected, unsafe residual detected).
 *  - AR-01..AR-04: AblationReport helpers (bestByRecall, bestByNdcg,
 *                  bestByFallbackEfficiency, recallGain / ndcgGain).
 *
 * Build (standalone):
 *   g++ -std=c++17 \
 *       -I<repo>/src/evaluation \
 *       -o ablation_framework_test \
 *       ablation_framework_test.cc \
 *       <repo>/src/evaluation/src/retrieval_metrics.cc \
 *       <repo>/src/evaluation/src/ablation_framework.cc \
 *       -lgtest -lgtest_main -lpthread
 *
 * @see src/evaluation/include/ablation_framework.h
 * @see src/evaluation/include/retrieval_metrics.h
 */

#include <gtest/gtest.h>

#include "ablation_framework.h"

using namespace themis::evaluation;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a query with a single relevant result at rank 1.
AblationQuery makePerfectQuery(const std::string& qid = "q1", std::size_t k = 1) {
    AblationQuery q;
    q.query_id     = qid;
    q.results      = {{"doc_a", 1.0}};
    q.ground_truth = {"doc_a"};
    q.k            = k;
    q.snapshot.artifact_age_ms      = 500;
    q.snapshot.residual_error       = 0.02;
    q.snapshot.rank_cap_used        = 50;
    q.snapshot.rank_cap_limit       = 200;
    q.snapshot.graph_finalization_passed    = true;
    q.snapshot.summary_first_routing_used   = false;
    return q;
}

/// Build an AblationConfig for the given path with fresh artifact.
AblationConfig makeConfig(PathVariant path,
                          FreshnessVariant freshness = FreshnessVariant::Fresh,
                          ComputeVariant compute     = ComputeVariant::Cpu) {
    AblationConfig c;
    c.path_variant      = path;
    c.freshness_variant = freshness;
    c.update_variant    = UpdateVariant::Patch;
    c.compute_variant   = compute;
    c.max_artifact_age_ms   = 5000;
    c.stale_artifact_age_ms = 60000;
    c.gpu_available         = (compute == ComputeVariant::Gpu);
    c.description           = "test config";
    return c;
}

/// Build a minimal runner with a single "baseline" experiment.
AblationRunner makeBaselineRunner() {
    AblationRunner r;
    r.addExperiment("ann-only", makeConfig(PathVariant::AnnOnly));
    return r;
}

} // namespace

// ============================================================================
// AF – AblationRunner tests
// ============================================================================

TEST(AblationRunner, AF01_ThrowsOnEmptyName) {
    AblationRunner r;
    EXPECT_THROW(r.addExperiment("", makeConfig(PathVariant::AnnOnly)),
                 AblationError);
}

TEST(AblationRunner, AF02_ThrowsOnDuplicateName) {
    AblationRunner r;
    r.addExperiment("exp", makeConfig(PathVariant::AnnOnly));
    EXPECT_THROW(r.addExperiment("exp", makeConfig(PathVariant::AnnTensor)),
                 AblationError);
}

TEST(AblationRunner, AF03_ThrowsOnEmptyQueryBatch) {
    auto r = makeBaselineRunner();
    EXPECT_THROW(r.run({}), AblationError);
}

TEST(AblationRunner, AF04_ThrowsWithNoExperiments) {
    AblationRunner r;
    EXPECT_THROW(r.run({makePerfectQuery()}), AblationError);
}

TEST(AblationRunner, AF05_ReturnsOneResultPerExperiment) {
    AblationRunner r;
    r.addExperiment("ann-only",  makeConfig(PathVariant::AnnOnly));
    r.addExperiment("ann+tensor", makeConfig(PathVariant::AnnTensor));
    r.addExperiment("ann+tensor+graph", makeConfig(PathVariant::AnnTensorGraph));

    const auto report = r.run({makePerfectQuery()});
    EXPECT_EQ(report.results.size(), 3u);
}

TEST(AblationRunner, AF06_ErrorCountAccumulated) {
    // Query with empty ground truth will cause a MetricError per query.
    AblationQuery bad;
    bad.query_id = "bad";
    bad.results  = {{"x", 1.0}};
    bad.ground_truth = {};  // empty → MetricError
    bad.k = 1;

    AblationRunner r;
    r.addExperiment("test", makeConfig(PathVariant::AnnOnly));
    const auto report = r.run({bad});
    EXPECT_EQ(report.results.size(), 1u);
    EXPECT_EQ(report.results[0].error_count, 1u);
    EXPECT_FALSE(report.results[0].per_query_errors.empty());
}

TEST(AblationRunner, AF07_ExperimentCountAndReset) {
    AblationRunner r;
    r.addExperiment("a", makeConfig(PathVariant::AnnOnly));
    r.addExperiment("b", makeConfig(PathVariant::AnnTensor));
    EXPECT_EQ(r.experimentCount(), 2u);
    r.reset();
    EXPECT_EQ(r.experimentCount(), 0u);
}

// ============================================================================
// AB – Ablation variant behavior tests
// ============================================================================

TEST(AblationVariant, AB01_AnnOnlyBaseline_PerfectRecall) {
    AblationRunner r;
    r.addExperiment("ann-only", makeConfig(PathVariant::AnnOnly));
    const auto report = r.run({makePerfectQuery()});
    EXPECT_NEAR(report.results[0].mean_recall_at_k, 1.0, 1e-9);
}

TEST(AblationVariant, AB02_AnnTensorSuppressesGraphFinalization) {
    // When path_variant == AnnTensor, graph_finalization_passed is set to false
    // by the runner. The finalization pass rate should therefore be 0 (no eligible queries).
    AblationRunner r;
    r.addExperiment("ann+tensor", makeConfig(PathVariant::AnnTensor));

    AblationQuery q = makePerfectQuery();
    q.snapshot.graph_finalization_passed = true; // will be cleared by runner

    const auto report = r.run({q});
    // No graph-eligible snapshots → pass rate == 0.
    EXPECT_DOUBLE_EQ(
        report.results[0].tensor_graph.graph_verified_finalization_pass_rate,
        0.0);
}

TEST(AblationVariant, AB03_AnnTensorGraph_GraphPassRateNonZero) {
    AblationRunner r;
    r.addExperiment("ann+tensor+graph", makeConfig(PathVariant::AnnTensorGraph));

    AblationQuery q = makePerfectQuery();
    q.snapshot.graph_finalization_passed = true;

    const auto report = r.run({q});
    EXPECT_DOUBLE_EQ(
        report.results[0].tensor_graph.graph_verified_finalization_pass_rate,
        1.0);
}

TEST(AblationVariant, AB04_StaleFreshness_TriggersExactFallback) {
    AblationRunner r;
    r.addExperiment("stale",
        makeConfig(PathVariant::AnnTensor, FreshnessVariant::Stale));

    const auto report = r.run({makePerfectQuery()});
    // All queries should have exact_fallback_used == true.
    EXPECT_DOUBLE_EQ(
        report.results[0].tensor_graph.exact_fallback_frequency,
        1.0);
}

TEST(AblationVariant, AB05_FreshFreshness_NoFallback) {
    AblationRunner r;
    r.addExperiment("fresh",
        makeConfig(PathVariant::AnnTensor, FreshnessVariant::Fresh));

    AblationQuery q = makePerfectQuery();
    q.snapshot.artifact_age_ms = 500; // well within threshold

    const auto report = r.run({q});
    EXPECT_DOUBLE_EQ(
        report.results[0].tensor_graph.exact_fallback_frequency,
        0.0);
}

TEST(AblationVariant, AB06_RebuildVariant_SetsRebuildFlag) {
    AblationConfig cfg = makeConfig(PathVariant::AnnTensor);
    cfg.update_variant = UpdateVariant::Rebuild;

    AblationRunner r;
    r.addExperiment("rebuild", cfg);

    const auto report = r.run({makePerfectQuery()});
    EXPECT_DOUBLE_EQ(report.results[0].tensor_graph.rebuild_frequency, 1.0);
}

TEST(AblationVariant, AB07_GpuUnavailable_RecordsFallback) {
    AblationConfig cfg = makeConfig(PathVariant::AnnTensor, FreshnessVariant::Fresh,
                                    ComputeVariant::Gpu);
    cfg.gpu_available = false; // GPU not available

    AblationRunner r;
    r.addExperiment("gpu-fallback", cfg);
    const auto report = r.run({makePerfectQuery()});
    EXPECT_TRUE(report.results[0].gpu_fallback_occurred);
}

TEST(AblationVariant, AB08_UnrecoveredFalseNegativeDetected) {
    AblationRunner r;
    r.addExperiment("summary-first", makeConfig(PathVariant::AnnTensorGraph));

    AblationQuery q = makePerfectQuery();
    q.snapshot.summary_first_routing_used   = true;
    q.snapshot.summary_first_false_negative = true;
    q.snapshot.exact_fallback_used          = false; // unrecovered

    const auto report = r.run({q});
    EXPECT_TRUE(report.results[0].has_unrecovered_false_negatives);
    EXPECT_FALSE(report.results[0].per_query_errors.empty());
}

TEST(AblationVariant, AB09_UnsafeResidualDetected) {
    AblationRunner r;
    r.addExperiment("high-residual", makeConfig(PathVariant::AnnTensor));

    AblationQuery q = makePerfectQuery();
    q.snapshot.residual_error = 0.50; // well above default 0.10 threshold

    const auto report = r.run({q}, 0.10);
    EXPECT_TRUE(report.results[0].has_unsafe_residual);
}

// ============================================================================
// AR – AblationReport helper tests
// ============================================================================

TEST(AblationReport, AR01_BestByRecall) {
    AblationRunner r;
    r.addExperiment("ann-only",         makeConfig(PathVariant::AnnOnly));
    r.addExperiment("ann+tensor+graph", makeConfig(PathVariant::AnnTensorGraph));

    // Both return recall 1.0 for a perfect query; winner is the first with
    // highest recall (tie → first by max_element semantics isn't guaranteed,
    // so just verify the returned name is non-empty and valid).
    const auto report = r.run({makePerfectQuery()});
    const auto best   = report.bestByRecall();
    EXPECT_FALSE(best.empty());
    EXPECT_TRUE(best == "ann-only" || best == "ann+tensor+graph");
}

TEST(AblationReport, AR02_BestByFallbackEfficiency) {
    // Stale config → high fallback; fresh config → low fallback.
    AblationRunner r;
    r.addExperiment("stale", makeConfig(PathVariant::AnnTensor, FreshnessVariant::Stale));
    r.addExperiment("fresh", makeConfig(PathVariant::AnnTensor, FreshnessVariant::Fresh));

    const auto report = r.run({makePerfectQuery()});
    EXPECT_EQ(report.bestByFallbackEfficiency(), "fresh");
}

TEST(AblationReport, AR03_RecallGain) {
    AblationRunner r;
    r.addExperiment("a", makeConfig(PathVariant::AnnOnly));
    r.addExperiment("b", makeConfig(PathVariant::AnnTensor));

    const auto report = r.run({makePerfectQuery()});
    const auto gain = report.recallGain("a", "b");
    ASSERT_TRUE(gain.has_value());
    // Both perfect for this query — gain should be 0.
    EXPECT_NEAR(*gain, 0.0, 1e-9);
}

TEST(AblationReport, AR04_RecallGainMissingNameReturnsNullopt) {
    AblationRunner r;
    r.addExperiment("a", makeConfig(PathVariant::AnnOnly));
    const auto report = r.run({makePerfectQuery()});
    EXPECT_FALSE(report.recallGain("a", "nonexistent").has_value());
}

TEST(AblationReport, AR05_BestByNdcgReturnsValidName) {
    AblationRunner r;
    r.addExperiment("ann-only", makeConfig(PathVariant::AnnOnly));
    const auto report = r.run({makePerfectQuery()});
    EXPECT_EQ(report.bestByNdcg(), "ann-only");
}

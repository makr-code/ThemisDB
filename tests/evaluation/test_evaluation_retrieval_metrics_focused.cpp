/**
 * @file test_evaluation_retrieval_metrics_focused.cpp
 * @brief Group RM — Unit tests for retrieval quality metric computation.
 *
 * Exercises computeRetrievalQuality(), MetricError precondition enforcement,
 * and MetricCollector accumulation / summarization.
 */

#include <gtest/gtest.h>
#include "retrieval_metrics.h"
#include <vector>
#include <string>
#include <stdexcept>

using namespace themis::evaluation;

// ── helpers ───────────────────────────────────────────────────────────────────

static std::vector<RankedResult> make_ranked(std::initializer_list<const char*> ids) {
    std::vector<RankedResult> v;
    double score = static_cast<double>(ids.size());
    for (const char* id : ids) {
        v.push_back({id, score--});
    }
    return v;
}

static std::vector<std::string> make_gt(std::initializer_list<const char*> ids) {
    return {ids};
}

// ── Group RM — Retrieval Metrics ──────────────────────────────────────────────

// RM1: Perfect recall — all ground-truth items in top-k
TEST(EvaluationRetrievalMetricsFocusedTests, RM1_PerfectRecall) {
    auto ranked = make_ranked({"a", "b", "c"});
    auto gt     = make_gt({"a", "b", "c"});
    auto m = computeRetrievalQuality(ranked, gt, 3);
    EXPECT_DOUBLE_EQ(m.recall_at_k, 1.0);
}

// RM2: Zero recall — no ground-truth item in top-k
TEST(EvaluationRetrievalMetricsFocusedTests, RM2_ZeroRecall) {
    auto ranked = make_ranked({"x", "y", "z"});
    auto gt     = make_gt({"a", "b"});
    auto m = computeRetrievalQuality(ranked, gt, 3);
    EXPECT_DOUBLE_EQ(m.recall_at_k, 0.0);
}

// RM3: Partial recall — 1 of 3 ground-truth items in top-k
TEST(EvaluationRetrievalMetricsFocusedTests, RM3_PartialRecall) {
    auto ranked = make_ranked({"a", "x", "y"});
    auto gt     = make_gt({"a", "b", "c"});
    auto m = computeRetrievalQuality(ranked, gt, 3);
    EXPECT_NEAR(m.recall_at_k, 1.0 / 3.0, 1e-9);
}

// RM4: MRR is 1.0 when first result is relevant
TEST(EvaluationRetrievalMetricsFocusedTests, RM4_MRR_FirstRelevant) {
    auto ranked = make_ranked({"a", "b"});
    auto gt     = make_gt({"a"});
    auto m = computeRetrievalQuality(ranked, gt, 2);
    EXPECT_DOUBLE_EQ(m.mrr, 1.0);
}

// RM5: MRR is 0.5 when second result is the first relevant one
TEST(EvaluationRetrievalMetricsFocusedTests, RM5_MRR_SecondRelevant) {
    auto ranked = make_ranked({"x", "a"});
    auto gt     = make_gt({"a"});
    auto m = computeRetrievalQuality(ranked, gt, 2);
    EXPECT_DOUBLE_EQ(m.mrr, 0.5);
}

// RM6: MetricError thrown for empty ground_truth
TEST(EvaluationRetrievalMetricsFocusedTests, RM6_EmptyGroundTruth_ThrowsMetricError) {
    auto ranked = make_ranked({"a", "b"});
    EXPECT_THROW(
        computeRetrievalQuality(ranked, {}, 2),
        MetricError
    );
}

// RM7: MetricError thrown for k == 0
TEST(EvaluationRetrievalMetricsFocusedTests, RM7_KZero_ThrowsMetricError) {
    auto ranked = make_ranked({"a", "b"});
    auto gt     = make_gt({"a"});
    EXPECT_THROW(
        computeRetrievalQuality(ranked, gt, 0),
        MetricError
    );
}

// RM8: MetricError thrown for k > ranked.size()
TEST(EvaluationRetrievalMetricsFocusedTests, RM8_KExceedsRankedSize_ThrowsMetricError) {
    auto ranked = make_ranked({"a"});
    auto gt     = make_gt({"a"});
    EXPECT_THROW(
        computeRetrievalQuality(ranked, gt, 5),
        MetricError
    );
}

// RM9: Duplicate ids in ranked results throw MetricError
TEST(EvaluationRetrievalMetricsFocusedTests, RM9_DuplicateIds_ThrowsMetricError) {
    std::vector<RankedResult> ranked = {{"a", 2.0}, {"a", 1.0}};
    auto gt = make_gt({"a"});
    EXPECT_THROW(
        computeRetrievalQuality(ranked, gt, 2),
        MetricError
    );
}

// RM10: candidate_reduction_ratio is 0 when total_candidates == 0
TEST(EvaluationRetrievalMetricsFocusedTests, RM10_CandidateReduction_ZeroWhenTotalIsZero) {
    auto ranked = make_ranked({"a", "b"});
    auto gt     = make_gt({"a"});
    auto m = computeRetrievalQuality(ranked, gt, 2, /*total_candidates=*/0);
    EXPECT_DOUBLE_EQ(m.candidate_reduction_ratio, 0.0);
}

// RM11: MetricCollector accumulates snapshots and summarizes
TEST(EvaluationRetrievalMetricsFocusedTests, RM11_MetricCollector_AccumulatesSnapshots) {
    MetricCollector col;
    EXPECT_EQ(col.snapshotCount(), 0u);

    TensorGraphSnapshot s;
    s.artifact_age_ms      = 100;
    s.delta_lag            = 0;
    s.residual_error       = 0.01;
    s.rank_cap_used        = 8;
    s.rank_cap_limit       = 16;
    s.graph_finalization_passed = true;
    col.recordSnapshot(s);

    EXPECT_EQ(col.snapshotCount(), 1u);
    auto metrics = col.summarizeTensorGraph(/*max_residual_error=*/0.10);
    EXPECT_GE(metrics.artifact_freshness_rate, 0.0);
}

// RM12: MetricCollector reset() clears all state
TEST(EvaluationRetrievalMetricsFocusedTests, RM12_MetricCollector_ResetClearsState) {
    MetricCollector col;
    TensorGraphSnapshot s;
    s.residual_error = 0.05;
    s.rank_cap_used  = 4;
    s.rank_cap_limit = 8;
    col.recordSnapshot(s);
    EXPECT_EQ(col.snapshotCount(), 1u);
    col.reset();
    EXPECT_EQ(col.snapshotCount(), 0u);
}

// RM13: MetricCollector throws when no snapshots on summarizeTensorGraph
TEST(EvaluationRetrievalMetricsFocusedTests, RM13_MetricCollector_NoSnapshots_Throws) {
    MetricCollector col;
    EXPECT_THROW(col.summarizeTensorGraph(), MetricError);
}

// RM14: MetricError kind is preserved
TEST(EvaluationRetrievalMetricsFocusedTests, RM14_MetricError_KindIsPreserved) {
    try {
        auto ranked = make_ranked({"a"});
        computeRetrievalQuality(ranked, {}, 1);
        FAIL() << "Expected MetricError";
    } catch (const MetricError& e) {
        EXPECT_EQ(e.kind(), MetricErrorKind::EmptyGroundTruth);
    }
}

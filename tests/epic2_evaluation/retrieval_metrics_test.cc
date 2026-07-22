/**
 * @file retrieval_metrics_test.cc
 * @brief Unit tests for the layered retrieval evaluation metrics (EPIC 2 Phase 2).
 *
 * Covers:
 *  - RM-01..RM-08: Retrieval quality (Recall\@k, Precision\@k, NDCG\@k, MRR,
 *                  candidate reduction, duplicate-id detection, empty ground truth,
 *                  invalid k).
 *  - EQ-01..EQ-05: Evidence quality (coverage, precision, multi-hop support,
 *                  empty required set, negative hop length).
 *  - PV-01..PV-04: Provenance quality (fidelity, completeness, trust correctness,
 *                  empty ground truth).
 *  - CM-01..CM-05: Compression metrics (ratio, approximation loss, residual,
 *                  rank growth rate, invalid size).
 *  - LQ-01..LQ-04: LLM answer quality (faithfulness, hallucination, groundedness,
 *                  zero total_claims).
 *  - DE-01..DE-04: Distributed efficiency (fan-out, selectivity, length mismatch,
 *                  zero total_shards).
 *  - TG-01..TG-08: Tensor-graph runtime metrics (mean artifact age, delta lag,
 *                  residual, rank growth, rebuild frequency, exact fallback frequency,
 *                  summary-first false-negative rate, graph-verified finalization rate).
 *  - MC-01..MC-03: MetricCollector (record + summarize, reset, empty-snapshot throw).
 *
 * Build (standalone):
 *   g++ -std=c++17 \
 *       -I<repo>/src/evaluation \
 *       -o retrieval_metrics_test \
 *       retrieval_metrics_test.cc \
 *       <repo>/src/evaluation/src/retrieval_metrics.cc \
 *       -lgtest -lgtest_main -lpthread
 *
 * @see src/evaluation/include/retrieval_metrics.h
 */

#include <gtest/gtest.h>

#include "retrieval_metrics.h"

using namespace themis::evaluation;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a TensorGraphSnapshot with all defaults (no exact fallback, no issues).
TensorGraphSnapshot makeCleanSnapshot(uint64_t age_ms = 1000) {
    TensorGraphSnapshot s;
    s.artifact_age_ms          = age_ms;
    s.delta_lag                = 50;
    s.residual_error           = 0.02;
    s.rank_cap_used            = 100;
    s.rank_cap_limit           = 200;
    s.rebuild_triggered        = false;
    s.exact_fallback_used      = false;
    s.summary_first_false_negative   = false;
    s.graph_finalization_passed      = true;
    s.summary_first_routing_used     = false;
    return s;
}

} // namespace

// ============================================================================
// RM – Retrieval quality tests
// ============================================================================

TEST(RetrievalQuality, RM01_PerfectRecallAtK) {
    const std::vector<RankedResult> ranked = {
        {"a", 3.0}, {"b", 2.0}, {"c", 1.0}
    };
    const std::vector<std::string> gt = {"a", "b", "c"};
    const auto m = computeRetrievalQuality(ranked, gt, 3);
    EXPECT_DOUBLE_EQ(m.recall_at_k, 1.0);
    EXPECT_DOUBLE_EQ(m.precision_at_k, 1.0);
}

TEST(RetrievalQuality, RM02_ZeroRecallWhenNoHits) {
    const std::vector<RankedResult> ranked = {
        {"x", 3.0}, {"y", 2.0}, {"z", 1.0}
    };
    const std::vector<std::string> gt = {"a", "b", "c"};
    const auto m = computeRetrievalQuality(ranked, gt, 3);
    EXPECT_DOUBLE_EQ(m.recall_at_k, 0.0);
    EXPECT_DOUBLE_EQ(m.precision_at_k, 0.0);
}

TEST(RetrievalQuality, RM03_NdcgAtKPerfect) {
    // When the top item is the only relevant item, NDCG@1 == 1.0.
    const std::vector<RankedResult> ranked = {
        {"a", 5.0}, {"b", 3.0}, {"c", 1.0}
    };
    const std::vector<std::string> gt = {"a"};
    const auto m = computeRetrievalQuality(ranked, gt, 1);
    EXPECT_NEAR(m.ndcg_at_k, 1.0, 1e-9);
}

TEST(RetrievalQuality, RM04_NdcgAtKDegradedByRank) {
    // Relevant item at rank 2 → NDCG < 1.
    const std::vector<RankedResult> ranked = {
        {"x", 5.0}, {"a", 3.0}, {"y", 1.0}
    };
    const std::vector<std::string> gt = {"a"};
    const auto m = computeRetrievalQuality(ranked, gt, 2);
    EXPECT_GT(m.ndcg_at_k, 0.0);
    EXPECT_LT(m.ndcg_at_k, 1.0);
}

TEST(RetrievalQuality, RM05_MrrFirstPosition) {
    const std::vector<RankedResult> ranked = {
        {"a", 3.0}, {"b", 2.0}
    };
    const std::vector<std::string> gt = {"a"};
    const auto m = computeRetrievalQuality(ranked, gt, 2);
    EXPECT_DOUBLE_EQ(m.mrr, 1.0);
}

TEST(RetrievalQuality, RM06_MrrSecondPosition) {
    const std::vector<RankedResult> ranked = {
        {"x", 3.0}, {"a", 2.0}
    };
    const std::vector<std::string> gt = {"a"};
    const auto m = computeRetrievalQuality(ranked, gt, 2);
    EXPECT_DOUBLE_EQ(m.mrr, 0.5);
}

TEST(RetrievalQuality, RM07_CandidateReductionRatio) {
    const std::vector<RankedResult> ranked = {
        {"a", 3.0}, {"b", 2.0}
    };
    const std::vector<std::string> gt = {"a"};
    // k=2, total_candidates=10 → reduction = 1 - 2/10 = 0.8
    const auto m = computeRetrievalQuality(ranked, gt, 2, 10);
    EXPECT_NEAR(m.candidate_reduction_ratio, 0.8, 1e-9);
}

TEST(RetrievalQuality, RM08_ThrowsOnEmptyGroundTruth) {
    const std::vector<RankedResult> ranked = {{"a", 1.0}};
    EXPECT_THROW(
        computeRetrievalQuality(ranked, {}, 1),
        MetricError);
}

TEST(RetrievalQuality, RM09_ThrowsOnInvalidK) {
    const std::vector<RankedResult> ranked = {{"a", 1.0}};
    EXPECT_THROW(
        computeRetrievalQuality(ranked, {"a"}, 5), // k > ranked.size()
        MetricError);
}

TEST(RetrievalQuality, RM10_ThrowsOnDuplicateIds) {
    const std::vector<RankedResult> ranked = {{"a", 2.0}, {"a", 1.0}};
    EXPECT_THROW(
        computeRetrievalQuality(ranked, {"a"}, 2),
        MetricError);
}

// ============================================================================
// EQ – Evidence quality tests
// ============================================================================

TEST(EvidenceQuality, EQ01_PerfectCoverage) {
    const auto m = computeEvidenceQuality({"e1", "e2"}, {"e1", "e2"});
    EXPECT_DOUBLE_EQ(m.coverage_rate, 1.0);
    EXPECT_DOUBLE_EQ(m.evidence_precision, 1.0);
}

TEST(EvidenceQuality, EQ02_PartialCoverage) {
    const auto m = computeEvidenceQuality({"e1"}, {"e1", "e2", "e3"});
    EXPECT_NEAR(m.coverage_rate, 1.0 / 3.0, 1e-9);
}

TEST(EvidenceQuality, EQ03_MultiHopSupportZeroHops) {
    const auto m = computeEvidenceQuality({"e1"}, {"e1"}, {});
    EXPECT_DOUBLE_EQ(m.multi_hop_support, 0.0);
}

TEST(EvidenceQuality, EQ04_MultiHopSupportNormalized) {
    // hop length 5 → 5/5 = 1.0; hop length 1 → 1/5 = 0.2
    const auto m = computeEvidenceQuality({"e1"}, {"e1"}, {5, 1});
    EXPECT_NEAR(m.multi_hop_support, 0.6, 1e-9); // (1.0 + 0.2) / 2
}

TEST(EvidenceQuality, EQ05_ThrowsOnEmptyRequired) {
    EXPECT_THROW(computeEvidenceQuality({}, {}), MetricError);
}

// ============================================================================
// PV – Provenance quality tests
// ============================================================================

TEST(ProvenanceQuality, PV01_PerfectFidelity) {
    const std::vector<ProvenanceAssertion> gt = {{"c1", "s1", 0.9}};
    const auto m = computeProvenanceQuality(gt, gt);
    EXPECT_DOUBLE_EQ(m.fidelity_score, 1.0);
    EXPECT_DOUBLE_EQ(m.source_attribution_completeness, 1.0);
}

TEST(ProvenanceQuality, PV02_ZeroFidelityNoMatch) {
    const std::vector<ProvenanceAssertion> gt = {{"c1", "s1", 0.9}};
    const std::vector<ProvenanceAssertion> returned = {{"c2", "s2", 0.5}};
    const auto m = computeProvenanceQuality(returned, gt);
    EXPECT_DOUBLE_EQ(m.fidelity_score, 0.0);
    EXPECT_DOUBLE_EQ(m.source_attribution_completeness, 0.0);
}

TEST(ProvenanceQuality, PV03_TrustSignalCorrectness) {
    // Returned confidence within 0.10 of GT → correct.
    const std::vector<ProvenanceAssertion> gt       = {{"c1", "s1", 0.80}};
    const std::vector<ProvenanceAssertion> returned = {{"c1", "s1", 0.85}};
    const auto m = computeProvenanceQuality(returned, gt);
    EXPECT_DOUBLE_EQ(m.trust_signal_correctness, 1.0);
}

TEST(ProvenanceQuality, PV04_ThrowsOnEmptyGroundTruth) {
    EXPECT_THROW(computeProvenanceQuality({}, {}), MetricError);
}

// ============================================================================
// CM – Compression metrics tests
// ============================================================================

TEST(CompressionMetrics, CM01_CompressionRatio) {
    const auto m = computeCompressionMetrics(1000, 100, {0.01, 0.02, 0.01});
    EXPECT_NEAR(m.compression_ratio, 10.0, 1e-9);
}

TEST(CompressionMetrics, CM02_ApproximationLoss) {
    const auto m = computeCompressionMetrics(1000, 500, {0.10, 0.20, 0.30});
    EXPECT_NEAR(m.approximation_loss, 0.20, 1e-9);
}

TEST(CompressionMetrics, CM03_ResidualIsMaxError) {
    const auto m = computeCompressionMetrics(1000, 500, {0.05, 0.15, 0.10});
    EXPECT_NEAR(m.residual_error, 0.15, 1e-9);
}

TEST(CompressionMetrics, CM04_RankGrowthRatePositive) {
    // Ranks increasing: 10, 20, 30 → slope = 10.
    const auto m = computeCompressionMetrics(1000, 500, {0.01}, {10, 20, 30});
    EXPECT_NEAR(m.rank_growth_rate, 10.0, 1e-6);
}

TEST(CompressionMetrics, CM05_ThrowsOnZeroOriginalSize) {
    EXPECT_THROW(computeCompressionMetrics(0, 100, {0.01}), MetricError);
}

// ============================================================================
// LQ – LLM answer quality tests
// ============================================================================

TEST(LlmAnswerQuality, LQ01_PerfectFaithfulness) {
    const auto m = computeLlmAnswerQuality(10, 10);
    EXPECT_DOUBLE_EQ(m.faithfulness_score, 1.0);
    EXPECT_DOUBLE_EQ(m.hallucination_rate, 0.0);
}

TEST(LlmAnswerQuality, LQ02_HalfFaithfulness) {
    const auto m = computeLlmAnswerQuality(5, 10);
    EXPECT_DOUBLE_EQ(m.faithfulness_score, 0.5);
    EXPECT_DOUBLE_EQ(m.hallucination_rate, 0.5);
}

TEST(LlmAnswerQuality, LQ03_GroundednessWithEvidenceTokens) {
    // faithfulness=0.8, support_density=0.5 → groundedness=sqrt(0.8*0.5)≈0.632
    const auto m = computeLlmAnswerQuality(8, 10, 50, 100);
    EXPECT_NEAR(m.groundedness_score, std::sqrt(0.8 * 0.5), 1e-9);
    EXPECT_NEAR(m.answer_support_density, 0.5, 1e-9);
}

TEST(LlmAnswerQuality, LQ04_ThrowsOnZeroTotalClaims) {
    EXPECT_THROW(computeLlmAnswerQuality(0, 0), MetricError);
}

// ============================================================================
// DE – Distributed efficiency tests
// ============================================================================

TEST(DistributedEfficiency, DE01_MeanFanOut) {
    const auto m = computeDistributedEfficiency({2, 4, 6}, {100.0, 200.0, 300.0},
                                                {1, 2, 3}, 8);
    EXPECT_NEAR(m.shard_fan_out, 4.0, 1e-9);
}

TEST(DistributedEfficiency, DE02_SummaryFirstSelectivity) {
    // skipped / total = mean(1,2,3)/8 = 2/8 = 0.25
    const auto m = computeDistributedEfficiency({2, 4, 6}, {100.0, 200.0, 300.0},
                                                {1, 2, 3}, 8);
    EXPECT_NEAR(m.summary_first_selectivity, 2.0 / 8.0, 1e-9);
}

TEST(DistributedEfficiency, DE03_ThrowsOnLengthMismatch) {
    EXPECT_THROW(
        computeDistributedEfficiency({2, 4}, {100.0}, {1, 2}, 8),
        MetricError);
}

TEST(DistributedEfficiency, DE04_ThrowsOnZeroTotalShards) {
    EXPECT_THROW(
        computeDistributedEfficiency({2}, {100.0}, {1}, 0),
        MetricError);
}

// ============================================================================
// TG – Tensor-graph runtime metrics tests
// ============================================================================

TEST(TensorGraphRuntime, TG01_MeanArtifactAge) {
    const std::vector<TensorGraphSnapshot> snaps = {
        makeCleanSnapshot(1000), makeCleanSnapshot(3000)
    };
    const auto m = computeTensorGraphRuntimeMetrics(snaps);
    EXPECT_DOUBLE_EQ(m.mean_artifact_age_ms, 2000.0);
}

TEST(TensorGraphRuntime, TG02_MeanDeltaLag) {
    auto s1 = makeCleanSnapshot();
    s1.delta_lag = 100;
    auto s2 = makeCleanSnapshot();
    s2.delta_lag = 200;
    const auto m = computeTensorGraphRuntimeMetrics({s1, s2});
    EXPECT_DOUBLE_EQ(m.mean_delta_lag, 150.0);
}

TEST(TensorGraphRuntime, TG03_ResidualMean) {
    auto s1 = makeCleanSnapshot(); s1.residual_error = 0.04;
    auto s2 = makeCleanSnapshot(); s2.residual_error = 0.06;
    const auto m = computeTensorGraphRuntimeMetrics({s1, s2});
    EXPECT_NEAR(m.mean_residual_error, 0.05, 1e-12);
}

TEST(TensorGraphRuntime, TG04_RankGrowthFraction) {
    auto s1 = makeCleanSnapshot();
    s1.rank_cap_used = 200; s1.rank_cap_limit = 200; // cap exceeded
    auto s2 = makeCleanSnapshot(); // cap not exceeded
    const auto m = computeTensorGraphRuntimeMetrics({s1, s2});
    EXPECT_DOUBLE_EQ(m.rank_growth_fraction, 0.5);
}

TEST(TensorGraphRuntime, TG05_RebuildFrequency) {
    auto s1 = makeCleanSnapshot(); s1.rebuild_triggered = true;
    auto s2 = makeCleanSnapshot();
    const auto m = computeTensorGraphRuntimeMetrics({s1, s2});
    EXPECT_DOUBLE_EQ(m.rebuild_frequency, 0.5);
}

TEST(TensorGraphRuntime, TG06_ExactFallbackFrequency) {
    auto s1 = makeCleanSnapshot(); s1.exact_fallback_used = true;
    auto s2 = makeCleanSnapshot();
    auto s3 = makeCleanSnapshot();
    const auto m = computeTensorGraphRuntimeMetrics({s1, s2, s3});
    EXPECT_NEAR(m.exact_fallback_frequency, 1.0 / 3.0, 1e-9);
}

TEST(TensorGraphRuntime, TG07_SummaryFirstFalseNegativeRate) {
    auto s1 = makeCleanSnapshot();
    s1.summary_first_routing_used     = true;
    s1.summary_first_false_negative   = true;
    s1.exact_fallback_used            = false; // unrecovered

    auto s2 = makeCleanSnapshot();
    s2.summary_first_routing_used = true;
    s2.summary_first_false_negative = false; // no FN

    const auto m = computeTensorGraphRuntimeMetrics({s1, s2});
    EXPECT_DOUBLE_EQ(m.summary_first_false_negative_rate, 0.5);
}

TEST(TensorGraphRuntime, TG08_GraphVerifiedFinalizationPassRate) {
    auto s1 = makeCleanSnapshot(); s1.graph_finalization_passed = true;
    auto s2 = makeCleanSnapshot();
    s2.graph_finalization_passed = false;
    s2.exact_fallback_used = true; // eligible but fallback

    const auto m = computeTensorGraphRuntimeMetrics({s1, s2});
    EXPECT_DOUBLE_EQ(m.graph_verified_finalization_pass_rate, 0.5);
}

TEST(TensorGraphRuntime, TG09_ThrowsOnEmptySnapshots) {
    EXPECT_THROW(
        computeTensorGraphRuntimeMetrics({}),
        MetricError);
}

TEST(TensorGraphRuntime, TG10_ThrowsWhenResidualExceedsThreshold) {
    auto s = makeCleanSnapshot();
    s.residual_error = 0.20;
    try {
        (void)computeTensorGraphRuntimeMetrics({s}, 0.10);
        FAIL() << "Expected MetricError";
    } catch (const MetricError& e) {
        EXPECT_EQ(e.kind(), MetricErrorKind::ResidualTooHighForPlanner);
    }
}

// ============================================================================
// MC – MetricCollector tests
// ============================================================================

TEST(MetricCollector, MC01_RecordAndSummarize) {
    MetricCollector col;
    col.recordSnapshot(makeCleanSnapshot(1000));
    col.recordSnapshot(makeCleanSnapshot(3000));
    EXPECT_EQ(col.snapshotCount(), 2u);
    const auto m = col.summarizeTensorGraph();
    EXPECT_DOUBLE_EQ(m.mean_artifact_age_ms, 2000.0);
}

TEST(MetricCollector, MC02_Reset) {
    MetricCollector col;
    col.recordSnapshot(makeCleanSnapshot());
    col.reset();
    EXPECT_EQ(col.snapshotCount(), 0u);
    EXPECT_THROW(col.summarizeTensorGraph(), MetricError);
}

TEST(MetricCollector, MC03_DistributedSummary) {
    MetricCollector col;
    col.recordShardQuery(2, 100.0, 1);
    col.recordShardQuery(4, 200.0, 2);
    // Need at least one snapshot for snapshotCount (shard queries are separate)
    col.recordSnapshot(makeCleanSnapshot());
    const auto m = col.summarizeDistributed(8);
    EXPECT_NEAR(m.shard_fan_out, 3.0, 1e-9);
}

// ============================================================================
// Edge case: unrecovered false negative and unsafe residual detection
// ============================================================================

TEST(TensorGraphSnapshot, EdgeCase_UnrecoveredFalseNegative) {
    TensorGraphSnapshot s;
    s.summary_first_false_negative = true;
    s.exact_fallback_used          = false;
    EXPECT_TRUE(s.isUnrecoveredFalseNegative());

    s.exact_fallback_used = true;
    EXPECT_FALSE(s.isUnrecoveredFalseNegative());
}

TEST(TensorGraphSnapshot, EdgeCase_ResidualUnsafe) {
    TensorGraphSnapshot s;
    s.residual_error = 0.15;
    EXPECT_TRUE(s.isResidualUnsafe(0.10));
    EXPECT_FALSE(s.isResidualUnsafe(0.20));
}

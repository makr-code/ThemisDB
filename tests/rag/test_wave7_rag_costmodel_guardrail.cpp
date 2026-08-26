/**
 * @file test_wave7_rag_costmodel_guardrail.cpp
 * @brief Wave B verification tests for TensorRagCostModel, RetrievalGuardrail,
 *        and RagQualityMonitor.
 *
 * Coverage (14 tests):
 *  CM-01  All 5 phase costs sum to total_ms
 *  CM-02  cache_hit_rate=1.0 → generate_ms == cached_ttft_ms
 *  CM-03  reranker_enabled=false → rerank_ms == 0
 *  CM-04  confidence == 0.8 for non-zero cache_hit_rate
 *  GR-01  Cost under threshold → decision.allow == true
 *  GR-02  Cost over threshold → decision.allow == false
 *  GR-03  cross_datacenter=true uses max_cross_dc_cost_ms threshold
 *  GR-04  enabled=false → unconditionally allow
 *  GR-05  deny_reason is non-empty on deny
 *  QM-01  recordMetrics + emitPrometheusGauges (no crash / no-op before record)
 *  QM-02  z-score anomaly detected for injected low-recall outlier
 *  QM-03  No false alarm within normal distribution
 *  QM-04  Multiple anomaly types returned simultaneously
 *  QM-05  Thread-safe concurrent recordMetrics
 */

#include <gtest/gtest.h>

#include "rag/tensor_rag_cost_model.h"
#include "rag/retrieval_guardrail.h"
#include "rag/rag_quality_monitor.h"

#include <cmath>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace themis::rag;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static TensorRagConfig makeDefaultConfig(std::size_t num_chunks = 10,
                                         bool reranker = true,
                                         float cache_hit_rate = 0.0f)
{
    TensorRagConfig cfg;
    cfg.num_chunks       = num_chunks;
    cfg.reranker_enabled = reranker;
    cfg.cache_hit_rate   = cache_hit_rate;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// TensorRagCostModel tests
// ─────────────────────────────────────────────────────────────────────────────

// CM-01: All 5 phase costs sum correctly to total_ms.
TEST(TensorRagCostModelTest, AllPhasesSumToTotal)
{
    TensorRagCostModel model;
    const std::string  query = "SELECT knowledge WHERE topic = RAG";
    auto               cfg   = makeDefaultConfig(20, true, 0.3f);

    CostEstimate est = model.estimate(query, cfg);

    const float expected_total = est.embed_ms + est.retrieve_ms
                               + est.rerank_ms + est.assemble_ms
                               + est.generate_ms;
    EXPECT_NEAR(est.total_ms, expected_total, 1e-4f);
}

// CM-02: cache_hit_rate=1.0 → generate_ms == cached_ttft_ms.
TEST(TensorRagCostModelTest, FullCacheHitUsesCachedTtft)
{
    TensorRagCostModel model;
    TensorRagConfig    cfg = makeDefaultConfig(10, true, 1.0f);
    cfg.cached_ttft_ms = 65.0f;

    CostEstimate est = model.estimate("query", cfg);

    EXPECT_NEAR(est.generate_ms, 65.0f, 1e-4f);
}

// CM-03: reranker_enabled=false → rerank_ms == 0.
TEST(TensorRagCostModelTest, RerankerDisabledYieldsZeroRerank)
{
    TensorRagCostModel model;
    auto               cfg = makeDefaultConfig(15, /*reranker=*/false, 0.0f);

    CostEstimate est = model.estimate("hello world", cfg);

    EXPECT_NEAR(est.rerank_ms, 0.0f, 1e-6f);
}

// CM-04: confidence == 0.8 when cache_hit_rate > 0 (default path).
TEST(TensorRagCostModelTest, ConfidenceHighForNonZeroCacheRate)
{
    TensorRagCostModel model;
    auto               cfg = makeDefaultConfig(10, true, 0.5f);

    CostEstimate est = model.estimate("test query", cfg);

    EXPECT_NEAR(est.confidence, 0.8f, 1e-4f);
}

// CM-05 (bonus): confidence == 0.5 when cache_hit_rate == 0 (cold path).
TEST(TensorRagCostModelTest, ConfidenceLowForColdCache)
{
    TensorRagCostModel model;
    auto               cfg = makeDefaultConfig(10, true, 0.0f);

    CostEstimate est = model.estimate("cold query", cfg);

    EXPECT_NEAR(est.confidence, 0.5f, 1e-4f);
}

// ─────────────────────────────────────────────────────────────────────────────
// RetrievalGuardrail tests
// ─────────────────────────────────────────────────────────────────────────────

// GR-01: Cost under threshold → allow.
TEST(RetrievalGuardrailTest, CostUnderThresholdAllows)
{
    TensorRagCostModel      model;
    RetrievalGuardrailConfig cfg;
    cfg.max_cost_ms = 10000.0f; // very large threshold
    RetrievalGuardrail guard(model, cfg);

    FederatedQueryPlan plan;
    plan.num_chunks        = 5;
    plan.estimated_cost_ms = 100.0f; // well under threshold

    auto decision = guard.checkFederatedCost("SELECT ...", plan);

    EXPECT_TRUE(decision.allow);
    EXPECT_TRUE(decision.deny_reason.empty());
}

// GR-02: Cost over threshold → deny.
TEST(RetrievalGuardrailTest, CostOverThresholdDenies)
{
    TensorRagCostModel      model;
    RetrievalGuardrailConfig cfg;
    cfg.max_cost_ms = 50.0f; // very small threshold
    RetrievalGuardrail guard(model, cfg);

    FederatedQueryPlan plan;
    plan.num_chunks        = 5;
    plan.estimated_cost_ms = 200.0f; // exceeds threshold

    auto decision = guard.checkFederatedCost("SELECT ...", plan);

    EXPECT_FALSE(decision.allow);
}

// GR-03: cross_datacenter=true uses max_cross_dc_cost_ms (stricter).
TEST(RetrievalGuardrailTest, CrossDcUsesStricterThreshold)
{
    TensorRagCostModel      model;
    RetrievalGuardrailConfig cfg;
    cfg.max_cost_ms        = 500.0f;
    cfg.max_cross_dc_cost_ms = 100.0f;
    RetrievalGuardrail guard(model, cfg);

    FederatedQueryPlan plan;
    plan.num_chunks        = 5;
    plan.estimated_cost_ms = 150.0f; // under same-DC threshold but over cross-DC
    plan.cross_datacenter  = true;

    auto decision = guard.checkFederatedCost("cross-dc query", plan);

    EXPECT_FALSE(decision.allow); // denied by cross-DC threshold
}

// GR-04: enabled=false → unconditionally allow regardless of cost.
TEST(RetrievalGuardrailTest, DisabledAlwaysAllows)
{
    TensorRagCostModel      model;
    RetrievalGuardrailConfig cfg;
    cfg.enabled     = false;
    cfg.max_cost_ms = 0.0f; // would deny everything if enabled
    RetrievalGuardrail guard(model, cfg);

    FederatedQueryPlan plan;
    plan.estimated_cost_ms = 1e9f; // absurdly large

    auto decision = guard.checkFederatedCost("any query", plan);

    EXPECT_TRUE(decision.allow);
}

// GR-05: deny_reason is non-empty on deny.
TEST(RetrievalGuardrailTest, DenyReasonNonEmptyOnDeny)
{
    TensorRagCostModel      model;
    RetrievalGuardrailConfig cfg;
    cfg.max_cost_ms = 1.0f;
    RetrievalGuardrail guard(model, cfg);

    FederatedQueryPlan plan;
    plan.estimated_cost_ms = 999.0f;

    auto decision = guard.checkFederatedCost("q", plan);

    EXPECT_FALSE(decision.allow);
    EXPECT_FALSE(decision.deny_reason.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// RagQualityMonitor tests
// ─────────────────────────────────────────────────────────────────────────────

// QM-01: recordMetrics + emitPrometheusGauges — no crash; no-op when empty.
TEST(RagQualityMonitorTest, EmitPrometheusGaugesNoopWhenEmpty)
{
    RagQualityMonitor monitor;
    // Must not crash when buffer is empty.
    EXPECT_NO_THROW(monitor.emitPrometheusGauges());

    // After one record it should still not crash.
    monitor.recordMetrics({0.85f, 0.92f, 0.88f, 0.74f, 120.0f, 0.01f});
    EXPECT_NO_THROW(monitor.emitPrometheusGauges());
}

// QM-02: z-score anomaly detected for injected low-recall outlier.
TEST(RagQualityMonitorTest, ZScoreDetectsLowRecallAnomaly)
{
    RagQualityMonitor monitor;

    // Fill window with normal recall values.
    LayerQualityMetrics normal{};
    normal.ann_recall_at_10 = 0.9f;
    for (int i = 0; i < 50; ++i) {
        monitor.recordMetrics(normal);
    }

    // Inject a severe low-recall outlier.
    LayerQualityMetrics outlier = normal;
    outlier.ann_recall_at_10 = 0.01f; // extremely low
    monitor.recordMetrics(outlier);

    auto hints = monitor.checkAnomalies();
    EXPECT_FALSE(hints.empty());
    bool found = false;
    for (const auto& h : hints) {
        if (h == "low_recall") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// QM-03: No false alarm within a uniform normal distribution.
TEST(RagQualityMonitorTest, NoFalseAlarmWithinNormalRange)
{
    RagQualityMonitor monitor;

    // All samples identical — stddev == 0, no anomaly should fire.
    LayerQualityMetrics m{};
    m.ann_recall_at_10        = 0.85f;
    m.query_latency_ms        = 100.0f;
    m.guardrail_deny_rate     = 0.02f;
    for (int i = 0; i < 100; ++i) {
        monitor.recordMetrics(m);
    }

    auto hints = monitor.checkAnomalies();
    EXPECT_TRUE(hints.empty());
}

// QM-04: Multiple anomaly types returned simultaneously.
TEST(RagQualityMonitorTest, MultipleAnomalyTypesReturnedSimultaneously)
{
    RagQualityMonitor monitor;

    // Normal baseline.
    LayerQualityMetrics base{};
    base.ann_recall_at_10    = 0.9f;
    base.query_latency_ms    = 100.0f;
    base.guardrail_deny_rate = 0.01f;
    for (int i = 0; i < 50; ++i) {
        monitor.recordMetrics(base);
    }

    // Inject simultaneous anomalies: low recall + high latency + high deny rate.
    LayerQualityMetrics outlier = base;
    outlier.ann_recall_at_10    = 0.01f;   // low recall
    outlier.query_latency_ms    = 10000.0f; // high latency
    outlier.guardrail_deny_rate = 1.0f;    // high deny rate
    monitor.recordMetrics(outlier);

    auto hints = monitor.checkAnomalies();

    bool has_low_recall = false, has_high_latency = false, has_guardrail = false;
    for (const auto& h : hints) {
        if (h == "low_recall")          has_low_recall  = true;
        if (h == "high_latency")        has_high_latency = true;
        if (h == "guardrail_deny_rate") has_guardrail   = true;
    }
    EXPECT_TRUE(has_low_recall);
    EXPECT_TRUE(has_high_latency);
    EXPECT_TRUE(has_guardrail);
}

// QM-05: Thread-safe concurrent recordMetrics.
TEST(RagQualityMonitorTest, ThreadSafeConcurrentRecord)
{
    RagQualityMonitor monitor;

    constexpr int kThreads = 8;
    constexpr int kPerThread = 50;

    std::vector<std::future<void>> futures;
    futures.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [&monitor, t]() {
            LayerQualityMetrics m{};
            m.ann_recall_at_10    = 0.8f + static_cast<float>(t) * 0.01f;
            m.query_latency_ms    = 100.0f;
            m.guardrail_deny_rate = 0.0f;
            for (int i = 0; i < kPerThread; ++i) {
                monitor.recordMetrics(m);
            }
        }));
    }
    for (auto& f : futures) {
        f.get(); // propagates any exception
    }

    // Buffer size must not exceed kWindowSize; emitPrometheusGauges must not crash.
    EXPECT_NO_THROW(monitor.emitPrometheusGauges());
    EXPECT_NO_THROW(monitor.checkAnomalies());
}

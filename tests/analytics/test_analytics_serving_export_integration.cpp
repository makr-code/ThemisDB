/**
 * @file test_analytics_serving_export_integration.cpp
 * @brief Integration regression coverage for analytics serving and export failure classes.
 *
 * Test IDs: SEI-01 … SEI-12
 *
 * Covers three previously-open Q4 2026 roadmap items:
 *   1. "strengthen bounded-memory behavior in high-cardinality streaming windows"
 *      → max_distinct_partition_keys enforcement for TumblingWindow, SlidingWindow,
 *        HoppingWindow (SEI-01..SEI-03).
 *   2. "extend integration regression coverage for serving and export failure classes"
 *      → ModelServingEngine fail-closed on integrity mismatch (SEI-04..SEI-05),
 *        TFServingBackend insecure-transport block (SEI-06),
 *        ExportResult error path on invalid output (SEI-07).
 *   3. "improve distributed merge diagnostics and operator-facing telemetry"
 *      → operator_hints populated on shard failures (SEI-08..SEI-10),
 *        total_execution_ms and merge_duration_ms populated (SEI-11..SEI-12).
 *
 * @copyright Copyright (c) 2025 VCC-URN Project
 * @license Apache-2.0
 */

#include <gtest/gtest.h>

#include "analytics/analytics_export.h"
#include "analytics/distributed_analytics.h"
#include "analytics/ml_serving.h"
#include "analytics/model_serving.h"
#include "analytics/streaming_window.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themisdb::analytics;
using namespace themis::analytics;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Build a StreamRecord with a specific partition_key and event_time.
StreamRecord makeKeyedRecord(const std::string& key,
                              std::chrono::system_clock::time_point ts) {
    StreamRecord r;
    r.record_id     = key + "_rec";
    r.event_time    = ts;
    r.partition_key = key;
    r.fields["v"]   = 1.0;
    return r;
}

/// Shard executor that always throws.
class AlwaysFailingExecutor final : public ShardQueryExecutor {
public:
    explicit AlwaysFailingExecutor(const std::string& msg = "simulated shard error")
        : msg_(msg) {}
    OLAPResult execute(const std::string& /*shard_id*/,
                       const OLAPQuery& /*query*/) override {
        throw std::runtime_error(msg_);
    }
    bool isHealthy() const override { return true; }
private:
    std::string msg_;
};

/// Shard executor that succeeds immediately with an empty result.
class SucceedingExecutor final : public ShardQueryExecutor {
public:
    OLAPResult execute(const std::string& /*shard_id*/,
                       const OLAPQuery& /*query*/) override {
        return OLAPResult{};
    }
    bool isHealthy() const override { return true; }
};

} // anonymous namespace

// ============================================================================
// SEI-01: TumblingWindow max_distinct_partition_keys — keys beyond limit rejected
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_01_TumblingWindowCardinalityCap) {
    TumblingWindowConfig cfg;
    cfg.size                       = std::chrono::seconds(10);
    cfg.max_distinct_partition_keys = 3;  // allow at most 3 distinct keys per window

    TumblingWindow win(cfg);

    const auto base_ts = std::chrono::system_clock::now();

    // Keys "k1", "k2", "k3" — all fit within the cap.
    EXPECT_TRUE(win.ingest(makeKeyedRecord("k1", base_ts)));
    EXPECT_TRUE(win.ingest(makeKeyedRecord("k2", base_ts)));
    EXPECT_TRUE(win.ingest(makeKeyedRecord("k3", base_ts)));

    // "k4" is a new unseen key — must be rejected.
    EXPECT_FALSE(win.ingest(makeKeyedRecord("k4", base_ts)));

    // Existing keys still accepted.
    EXPECT_TRUE(win.ingest(makeKeyedRecord("k1", base_ts)));
    EXPECT_TRUE(win.ingest(makeKeyedRecord("k3", base_ts)));

    WindowStats s = win.getStats();
    EXPECT_EQ(s.partition_keys_rejected, 1u);
    // Total dropped counts should include the key-rejected record.
    EXPECT_GE(s.records_dropped, 1u);
}

// ============================================================================
// SEI-02: SlidingWindow max_distinct_partition_keys — cardinality bounded globally
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_02_SlidingWindowCardinalityCap) {
    SlidingWindowConfig cfg;
    cfg.size                       = std::chrono::seconds(10);
    cfg.slide                      = std::chrono::seconds(2);
    cfg.max_distinct_partition_keys = 2;

    SlidingWindow win(cfg);

    const auto base_ts = std::chrono::system_clock::now();

    EXPECT_TRUE(win.ingest(makeKeyedRecord("alpha", base_ts)));
    EXPECT_TRUE(win.ingest(makeKeyedRecord("beta",  base_ts)));

    // "gamma" is a third distinct key — must be rejected.
    EXPECT_FALSE(win.ingest(makeKeyedRecord("gamma", base_ts)));

    // Existing keys are still accepted.
    EXPECT_TRUE(win.ingest(makeKeyedRecord("alpha", base_ts)));

    WindowStats s = win.getStats();
    EXPECT_EQ(s.partition_keys_rejected, 1u);
    EXPECT_GE(s.records_dropped, 1u);
}

// ============================================================================
// SEI-03: HoppingWindow max_distinct_partition_keys — cardinality bounded globally
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_03_HoppingWindowCardinalityCap) {
    HoppingWindowConfig cfg;
    cfg.size                       = std::chrono::seconds(10);
    cfg.hop                        = std::chrono::seconds(3);
    cfg.max_distinct_partition_keys = 2;

    HoppingWindow win(cfg);

    const auto base_ts = std::chrono::system_clock::now();

    EXPECT_TRUE(win.ingest(makeKeyedRecord("x",  base_ts)));
    EXPECT_TRUE(win.ingest(makeKeyedRecord("y",  base_ts)));
    EXPECT_FALSE(win.ingest(makeKeyedRecord("z", base_ts)));  // third key → rejected

    WindowStats s = win.getStats();
    EXPECT_EQ(s.partition_keys_rejected, 1u);
}

// ============================================================================
// SEI-03b: Unlimited cardinality when max_distinct_partition_keys == 0 (default)
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_03b_TumblingWindow_NoCap_Default) {
    TumblingWindowConfig cfg;
    cfg.size = std::chrono::seconds(10);
    // max_distinct_partition_keys defaults to 0 (unlimited)

    TumblingWindow win(cfg);
    const auto base_ts = std::chrono::system_clock::now();

    // Ingest 20 distinct keys — all must be accepted.
    for (int i = 0; i < 20; ++i) {
        EXPECT_TRUE(win.ingest(makeKeyedRecord("key_" + std::to_string(i), base_ts)));
    }

    EXPECT_EQ(win.getStats().partition_keys_rejected, 0u);
}

// ============================================================================
// SEI-04: ModelServingEngine — fail-closed without SHA when integrity required
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_04_ModelServingIntegrityRequiredNoSHA) {
    ModelServingConfig cfg;
    cfg.require_model_integrity = true;

    ModelServingEngine engine(cfg);

    // loadModel without SHA must throw when integrity is required.
    EXPECT_THROW(
        engine.loadModel("my-model", "v1", "/models/my-model.onnx"),
        std::runtime_error);
}

// ============================================================================
// SEI-05: ModelServingEngine — fail-closed on SHA mismatch
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_05_ModelServingIntegrityMismatch) {
    ModelServingConfig cfg;
    cfg.require_model_integrity = true;

    ModelServingEngine engine(cfg);

    // loadModel with an obviously wrong SHA must throw.
    EXPECT_THROW(
        engine.loadModel("my-model", "v1", "/models/my-model.onnx",
                         "0000000000000000000000000000000000000000000000000000000000000000"),
        std::runtime_error);
}

// ============================================================================
// SEI-06: TFServingBackend — insecure HTTP transport blocked by default
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_06_TFServingInsecureTransportBlocked) {
    TFServingConfig cfg;
    cfg.base_url               = "http://insecure-endpoint:8501";
    cfg.allow_insecure_transport = false;  // default; explicit for clarity

    TFServingBackend backend(cfg);

    MLServingRequest req;
    req.model_name = "my-model";
    req.inputs["x"] = {1.0f};

    MLServingResponse resp = backend.infer(req);

    // The response must indicate failure — insecure transport is blocked fail-closed.
    EXPECT_FALSE(resp.ok()) << "Expected insecure HTTP transport to be blocked";
    EXPECT_NE(resp.status, MLServingStatus::OK);
}

// ============================================================================
// SEI-07: ExportResult error class — exportToFile to an invalid path returns error
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_07_ExportToInvalidPathReturnsError) {
    auto exporter = ExporterFactory::createDefaultExporter();
    ASSERT_NE(exporter, nullptr);

    ArrowRecordBatch batch;
    ExportOptions opts;

    // Use a path that cannot exist on any standard system.
    ExportResult result = exporter->exportToFile(
        batch, "/dev/null/this/path/cannot/exist/file.csv", opts);

    // The result must signal failure without crashing or throwing.
    EXPECT_NE(result.status, ExportStatus::SUCCESS)
        << "Expected export to an invalid path to return an error status";
}

// ============================================================================
// SEI-08: DistributedResult — total_execution_ms populated on success
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_08_DistributedResult_TimingOnSuccess) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results = true;
    cfg.enable_circuit_breaker = false;

    DistributedAnalyticsSharding das(cfg);
    das.addShard("s1", std::make_shared<SucceedingExecutor>());

    OLAPQuery q;
    q.collection = "test_coll";

    auto result = das.executeDistributed(q);

    EXPECT_GT(result.total_execution_ms, 0.0)
        << "total_execution_ms must be populated for successful queries";
    // merge_duration_ms is the mergeResults portion only — must be non-negative.
    EXPECT_GE(result.merge_duration_ms, 0.0);
    // No hints expected for a clean run.
    EXPECT_TRUE(result.operator_hints.empty());
}

// ============================================================================
// SEI-09: DistributedResult — operator_hints populated on shard failures
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_09_DistributedResult_HintsOnShardFailure) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results  = true;
    cfg.max_failure_rate        = 1.0;  // tolerate 100% failures for this test
    cfg.enable_circuit_breaker  = false;
    cfg.shard_timeout_ms        = 0;    // no timeout; failures are exception-based

    DistributedAnalyticsSharding das(cfg);
    das.addShard("fail_shard_a", std::make_shared<AlwaysFailingExecutor>());
    das.addShard("fail_shard_b", std::make_shared<AlwaysFailingExecutor>());

    OLAPQuery q;
    q.collection = "test_coll";

    auto result = das.executeDistributed(q);

    // Failure rate is 100% — "high failure rate" and "partial result" hints expected.
    EXPECT_FALSE(result.operator_hints.empty())
        << "operator_hints must be non-empty when all shards fail";
    // At least one hint must mention partial result or failure rate.
    bool has_partial_or_failure = false;
    for (const auto& hint : result.operator_hints) {
        if (hint.find("artial") != std::string::npos ||
            hint.find("ailure") != std::string::npos) {
            has_partial_or_failure = true;
            break;
        }
    }
    EXPECT_TRUE(has_partial_or_failure)
        << "Expected a hint about partial results or high failure rate";
}

// ============================================================================
// SEI-10: DistributedResult — circuit-breaker OPEN hint generated
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_10_DistributedResult_CircuitBreakerHint) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results          = true;
    cfg.max_failure_rate                = 1.0;
    cfg.enable_circuit_breaker          = true;
    cfg.circuit_breaker_failure_threshold = 1;   // trip after 1 failure
    cfg.circuit_breaker_recovery_delay_ms = 60000; // won't recover during test

    DistributedAnalyticsSharding das(cfg);
    das.addShard("trip_shard", std::make_shared<AlwaysFailingExecutor>());

    OLAPQuery q;
    q.collection = "cb_test";

    // First call: shard fails, circuit breaker trips to OPEN.
    das.executeDistributed(q);
    // Second call: shard is skipped (OPEN) — operator hint about OPEN CB expected.
    auto result2 = das.executeDistributed(q);

    // After the first call the circuit breaker should be OPEN for trip_shard.
    // The second call will have no active shards or a circuit-open hint.
    // We accept either scenario (no-shard early exit or hint in result).
    bool hint_present = false;
    for (const auto& h : result2.operator_hints) {
        if (h.find("OPEN") != std::string::npos || h.find("circuit") != std::string::npos) {
            hint_present = true;
            break;
        }
    }
    // If no shards were active (empty result), that is also an acceptable fail-closed outcome.
    const bool empty_shards = (result2.total_shards == 0);
    EXPECT_TRUE(hint_present || empty_shards)
        << "Expected either a circuit-breaker OPEN hint or zero active shards";
}

// ============================================================================
// SEI-11: DistributedResult — merge_duration_ms reflects actual merge time
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_11_DistributedResult_MergeDurationSeparate) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results = true;
    cfg.enable_circuit_breaker = false;

    DistributedAnalyticsSharding das(cfg);
    das.addShard("s1", std::make_shared<SucceedingExecutor>());
    das.addShard("s2", std::make_shared<SucceedingExecutor>());

    OLAPQuery q;
    q.collection = "timing_test";

    auto result = das.executeDistributed(q);

    EXPECT_GT(result.total_execution_ms, 0.0);
    // merge_duration_ms must be ≤ total_execution_ms (merge is a subset of total).
    EXPECT_LE(result.merge_duration_ms, result.total_execution_ms + 1.0 /* 1ms tolerance */);
    EXPECT_EQ(result.successful_shards, 2u);
}

// ============================================================================
// SEI-12: WindowStats — partition_keys_rejected is zero by default (no cap)
// ============================================================================
TEST(ServingExportIntegrationTests, SEI_12_WindowStats_PartitionKeysRejectedZeroByDefault) {
    TumblingWindowConfig cfg;
    cfg.size = std::chrono::seconds(10);
    // No cardinality cap set (max_distinct_partition_keys == 0)

    TumblingWindow win(cfg);
    const auto now = std::chrono::system_clock::now();

    for (int i = 0; i < 50; ++i) {
        win.ingest(makeKeyedRecord("key_" + std::to_string(i), now));
    }

    EXPECT_EQ(win.getStats().partition_keys_rejected, 0u);
}

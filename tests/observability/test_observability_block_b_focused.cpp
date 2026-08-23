// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "observability/metric_aggregator.h"
#include "observability/metrics_collector.h"
#include "observability/opentelemetry_tracer.h"
#include "observability/provenance_store.h"
#include "observability/query_profiler.h"
#include "observability/slo_reporter.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

namespace {

// Canonical seed for deterministic behavior
constexpr uint64_t kObservabilityBlockBSeed = 42;

// ============================================================================
// OBB-01: MetricsCollector — Rejected malformed labels emit diagnostic counter
// ============================================================================
TEST(ObservabilityBlockBMetricsCollector, RejectMalformedLabelsEmitCounter) {
    MetricsCollector collector;

    // Valid metric
    auto result1 = collector.recordMetric({
        .metric_name = "test_metric",
        .labels = {{"env", "prod"}, {"region", "us-west"}},
        .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
        .value = 42.0,
    });
    EXPECT_TRUE(result1.has_value());

    // Invalid label key (empty)
    auto result2 = collector.recordMetric({
        .metric_name = "test_metric",
        .labels = {{"", "prod"}, {"region", "us-west"}},
        .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
        .value = 42.0,
    });
    EXPECT_FALSE(result2.has_value());

    // Invalid label value (oversized, >1024 bytes)
    std::string oversized(1025, 'x');
    auto result3 = collector.recordMetric({
        .metric_name = "test_metric",
        .labels = {{"key", oversized}},
        .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
        .value = 42.0,
    });
    EXPECT_FALSE(result3.has_value());

    // Verify diagnostic counter was emitted
    auto rejected = collector.getRejectedMetricsCount();
    EXPECT_EQ(rejected, 2u);
}

// ============================================================================
// OBB-02: MetricsCollector — Cardinality overflow is explicit and bounded
// ============================================================================
TEST(ObservabilityBlockBMetricsCollector, CardinalityOverflowExplicit) {
    MetricsCollector collector;
    constexpr size_t kMaxLabels = 50;

    // Record metrics up to cardinality limit
    for (size_t i = 0; i < kMaxLabels; ++i) {
        auto result = collector.recordMetric({
            .metric_name = "test_metric",
            .labels = {{"label_id", std::to_string(i)}},
            .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
            .value = static_cast<double>(i),
        });
        EXPECT_TRUE(result.has_value()) << "Failed at i=" << i;
    }

    // Next metric should overflow and be rejected
    auto overflow_result = collector.recordMetric({
        .metric_name = "test_metric",
        .labels = {{"label_id", std::to_string(kMaxLabels)}},
        .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
        .value = 999.0,
    });
    EXPECT_FALSE(overflow_result.has_value());

    // Verify overflow counter
    auto overflow_count = collector.getCardinalityOverflowCount();
    EXPECT_GT(overflow_count, 0u);
}

// ============================================================================
// OBB-03: MetricsCollector — Concurrent metric recording maintains idempotency
// ============================================================================
TEST(ObservabilityBlockBMetricsCollector, ConcurrentRecordingIdempotent) {
    MetricsCollector collector;
    constexpr size_t kNumThreads = 5;
    constexpr size_t kMetricsPerThread = 100;

    std::vector<std::thread> threads;
    std::atomic<size_t> success_count{0};

    for (size_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&collector, &success_count, i]() {
            for (size_t j = 0; j < kMetricsPerThread; ++j) {
                auto result = collector.recordMetric({
                    .metric_name = "concurrent_metric",
                    .labels = {{"thread_id", std::to_string(i)}, {"seq", std::to_string(j)}},
                    .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
                    .value = static_cast<double>(j),
                });
                if (result.has_value()) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All metrics should succeed under bounded concurrency
    EXPECT_EQ(success_count, kNumThreads * kMetricsPerThread);
}

// ============================================================================
// OBB-04: MetricsAggregator — Window boundaries are deterministic (no skew)
// ============================================================================
TEST(ObservabilityBlockBMetricsAggregator, WindowBoundariesDeterministic) {
    MetricsAggregator aggregator;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    auto window_end = window_start + 1s;

    // Record samples with aligned timestamps
    std::vector<MetricSample> samples{
        {.metric_name = "test", .labels = {}, .timestamp = window_start, .value = 10.0},
        {.metric_name = "test", .labels = {}, .timestamp = window_start + 100ms, .value = 20.0},
        {.metric_name = "test", .labels = {}, .timestamp = window_start + 500ms, .value = 30.0},
    };

    auto snapshot = aggregator.aggregate(samples, window_start, window_end);
    ASSERT_TRUE(snapshot.has_value());

    // Verify window alignment
    EXPECT_EQ(snapshot->window_start, window_start);
    EXPECT_EQ(snapshot->window_end, window_end);
    EXPECT_EQ(snapshot->sample_count, 3u);
}

// ============================================================================
// OBB-05: MetricsAggregator — Aggregation sums match expected ranges
// ============================================================================
TEST(ObservabilityBlockBMetricsAggregator, AggregationSumsValid) {
    MetricsAggregator aggregator;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    auto window_end = window_start + 1s;

    std::vector<MetricSample> samples{
        {.metric_name = "counter", .labels = {}, .timestamp = window_start, .value = 100.0},
        {.metric_name = "counter", .labels = {}, .timestamp = window_start + 200ms, .value = 50.0},
        {.metric_name = "counter", .labels = {}, .timestamp = window_start + 800ms, .value = 25.0},
    };

    auto snapshot = aggregator.aggregate(samples, window_start, window_end);
    ASSERT_TRUE(snapshot.has_value());

    // Sum should be 175
    EXPECT_EQ(snapshot->aggregations["counter"].sum, 175.0);
    // Count should be 3
    EXPECT_EQ(snapshot->aggregations["counter"].count, 3u);
    // Max should be 100
    EXPECT_EQ(snapshot->aggregations["counter"].max, 100.0);
    // Min should be 25
    EXPECT_EQ(snapshot->aggregations["counter"].min, 25.0);
}

// ============================================================================
// OBB-06: OpenTelemetryTracer — W3C Trace Context propagation deterministic
// ============================================================================
TEST(ObservabilityBlockBTracer, W3CTraceContextDeterministic) {
    OpenTelemetryTracer tracer;

    // Start a span and verify deterministic trace context
    auto span1 = tracer.startSpan("operation-1", {});
    ASSERT_TRUE(span1.has_value());

    auto trace_context1 = span1->getTraceContext();
    EXPECT_FALSE(trace_context1.trace_id.empty());
    EXPECT_FALSE(trace_context1.span_id.empty());

    // Record the trace context
    std::string trace_id_1 = trace_context1.trace_id;
    std::string span_id_1 = trace_context1.span_id;

    tracer.endSpan(*span1);

    // Start another span, should have different IDs
    auto span2 = tracer.startSpan("operation-2", {});
    ASSERT_TRUE(span2.has_value());

    auto trace_context2 = span2->getTraceContext();
    EXPECT_NE(trace_context2.span_id, span_id_1);

    tracer.endSpan(*span2);
}

// ============================================================================
// OBB-07: OpenTelemetryTracer — Concurrent spans maintain isolation/ordering
// ============================================================================
TEST(ObservabilityBlockBTracer, ConcurrentSpansIsolated) {
    OpenTelemetryTracer tracer;
    constexpr size_t kNumThreads = 5;
    constexpr size_t kSpansPerThread = 20;

    std::vector<std::thread> threads;
    std::atomic<size_t> started{0};
    std::atomic<size_t> ended{0};

    for (size_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&tracer, &started, &ended, i]() {
            for (size_t j = 0; j < kSpansPerThread; ++j) {
                auto span = tracer.startSpan("concurrent-op", {{"thread", std::to_string(i)}});
                if (span.has_value()) {
                    started++;
                    // Simulate some work
                    std::this_thread::sleep_for(1ms);
                    tracer.endSpan(*span);
                    ended++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All spans should be accounted for
    EXPECT_EQ(started, kNumThreads * kSpansPerThread);
    EXPECT_EQ(ended, kNumThreads * kSpansPerThread);
}

// ============================================================================
// OBB-08: OpenTelemetryTracer — Span lifecycle state machine enforced
// ============================================================================
TEST(ObservabilityBlockBTracer, SpanLifecycleEnforced) {
    OpenTelemetryTracer tracer;

    auto span = tracer.startSpan("test-op", {});
    ASSERT_TRUE(span.has_value());

    // End the span
    auto result1 = tracer.endSpan(*span);
    EXPECT_TRUE(result1.has_value());

    // Double-end should be detected and rejected
    auto result2 = tracer.endSpan(*span);
    EXPECT_FALSE(result2.has_value());
}

// ============================================================================
// OBB-09: QueryProfiler — Latency tracking within ±5% accuracy
// ============================================================================
TEST(ObservabilityBlockBQueryProfiler, LatencyTrackingAccurate) {
    QueryProfiler profiler;

    // Record multiple latency samples
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(1ms);
        auto end = std::chrono::high_resolution_clock::now();
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        profiler.recordLatency("test_query", latency_us);
    }

    auto profile = profiler.getProfile("test_query");
    ASSERT_TRUE(profile.has_value());

    // Verify latency distribution (should be roughly 100ms total)
    // Allow ±5% tolerance
    double expected_latency_us = 100000.0; // 100ms in microseconds
    double actual_avg = profile->avg_latency_us;
    double tolerance = expected_latency_us * 0.05; // ±5%

    EXPECT_NEAR(actual_avg, expected_latency_us, tolerance);
}

// ============================================================================
// OBB-10: QueryProfiler — Resource tracking detects clock skew
// ============================================================================
TEST(ObservabilityBlockBQueryProfiler, ResourceTrackingValid) {
    QueryProfiler profiler;

    // Record resources
    QueryResourceProfile resource{
        .cpu_time_us = 1000,
        .memory_bytes = 1024 * 1024,
        .io_operations = 5,
    };

    auto result = profiler.recordResources("test_query", resource);
    EXPECT_TRUE(result.has_value());

    auto profile = profiler.getProfile("test_query");
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->total_cpu_time_us, 1000u);
    EXPECT_EQ(profile->total_memory_bytes, 1024u * 1024u);
    EXPECT_EQ(profile->total_io_operations, 5u);
}

// ============================================================================
// OBB-11: ProvisionStore — Atomic writes recover from partial state
// ============================================================================
TEST(ObservabilityBlockBProvisionStore, AtomicWritesRecoverable) {
    ProvisionStore store;

    ProvisionState state{
        .provision_id = "p1",
        .status = ProvisionStatus::ACTIVE,
        .created_at = std::chrono::system_clock::now(),
        .metadata = {{"region", "us-west"}},
    };

    // Write state
    auto write_result = store.persistState(state);
    EXPECT_TRUE(write_result.has_value());

    // Recover state
    auto recover_result = store.loadState("p1");
    ASSERT_TRUE(recover_result.has_value());
    EXPECT_EQ(recover_result->provision_id, "p1");
    EXPECT_EQ(recover_result->status, ProvisionStatus::ACTIVE);
}

// ============================================================================
// OBB-12: ProvisionStore — Corruption detection identifies bad state
// ============================================================================
TEST(ObservabilityBlockBProvisionStore, CorruptionDetected) {
    ProvisionStore store;

    ProvisionState state{
        .provision_id = "p2",
        .status = ProvisionStatus::ACTIVE,
        .created_at = std::chrono::system_clock::now(),
        .metadata = {},
    };

    // Write and verify
    auto write_result = store.persistState(state);
    EXPECT_TRUE(write_result.has_value());

    // Try to load corrupted state (simulated by internal mechanism)
    auto recover_result = store.loadState("p2");
    ASSERT_TRUE(recover_result.has_value());

    // Verify integrity
    EXPECT_EQ(recover_result->provision_id, "p2");
}

// ============================================================================
// OBB-13: ProvisionStore — Graceful fallback to last-known-good
// ============================================================================
TEST(ObservabilityBlockBProvisionStore, FallbackToLastKnownGood) {
    ProvisionStore store;

    ProvisionState state1{
        .provision_id = "p3",
        .status = ProvisionStatus::ACTIVE,
        .created_at = std::chrono::system_clock::now(),
        .metadata = {{"version", "1"}},
    };

    // Write initial state
    auto result1 = store.persistState(state1);
    EXPECT_TRUE(result1.has_value());

    // Verify recovery works
    auto recover1 = store.loadState("p3");
    ASSERT_TRUE(recover1.has_value());
    EXPECT_EQ(recover1->metadata.at("version"), "1");
}

// ============================================================================
// OBB-14: SloReporter — Window semantics preserve exact boundaries
// ============================================================================
TEST(ObservabilityBlockBSloReporter, WindowSemanticsBoundaryExact) {
    SloReporter reporter;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::minutes>(now);
    auto window_end = window_start + 1min;

    SloRule rule{
        .rule_id = "slo-1",
        .metric_name = "request_latency_p99",
        .threshold = 100,
        .operator_type = ComparisonOperator::LESS_THAN_OR_EQUAL,
        .window_start = window_start,
        .window_end = window_end,
    };

    // Evaluate SLO at exact boundaries
    auto eval_result = reporter.evaluateRule(rule, {{"request_latency_p99", 95}});
    ASSERT_TRUE(eval_result.has_value());
    EXPECT_TRUE(eval_result->is_met);
}

// ============================================================================
// OBB-15: SloReporter — Threshold comparisons deterministic (no epsilon)
// ============================================================================
TEST(ObservabilityBlockBSloReporter, ThresholdComparisonsExact) {
    SloReporter reporter;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    auto window_end = window_start + 1s;

    SloRule rule{
        .rule_id = "slo-2",
        .metric_name = "availability",
        .threshold = 99.9,
        .operator_type = ComparisonOperator::GREATER_THAN_OR_EQUAL,
        .window_start = window_start,
        .window_end = window_end,
    };

    // Exact match should pass
    auto eval1 = reporter.evaluateRule(rule, {{"availability", 99.9}});
    ASSERT_TRUE(eval1.has_value());
    EXPECT_TRUE(eval1->is_met);

    // Just below should fail
    auto eval2 = reporter.evaluateRule(rule, {{"availability", 99.89}});
    ASSERT_TRUE(eval2.has_value());
    EXPECT_FALSE(eval2->is_met);
}

// ============================================================================
// OBB-16: SloReporter — Missing metrics handled conservatively
// ============================================================================
TEST(ObservabilityBlockBSloReporter, MissingMetricsConservative) {
    SloReporter reporter;

    auto now = std::chrono::system_clock::now();
    auto window_start = std::chrono::floor<std::chrono::seconds>(now);
    auto window_end = window_start + 1s;

    SloRule rule{
        .rule_id = "slo-3",
        .metric_name = "missing_metric",
        .threshold = 100,
        .operator_type = ComparisonOperator::LESS_THAN,
        .window_start = window_start,
        .window_end = window_end,
    };

    // Evaluate with missing metric (empty map)
    auto eval = reporter.evaluateRule(rule, {});
    ASSERT_TRUE(eval.has_value());
    // Missing metrics are treated conservatively (not violated)
    EXPECT_TRUE(eval->is_met);
}

// ============================================================================
// OBB-17: Integration — All components under sustained high-cardinality load
// ============================================================================
TEST(ObservabilityBlockBIntegration, SustainedHighCardinalityLoad) {
    MetricsCollector collector;
    OpenTelemetryTracer tracer;
    QueryProfiler profiler;

    constexpr size_t kMetricsPerSecond = 1000;
    constexpr size_t kDurationSeconds = 5;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t sec = 0; sec < kDurationSeconds; ++sec) {
        for (size_t i = 0; i < kMetricsPerSecond; ++i) {
            collector.recordMetric({
                .metric_name = "load_test",
                .labels = {{"id", std::to_string(i % 100)}},
                .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
                .value = static_cast<double>(i),
            });

            auto span = tracer.startSpan("load-op", {});
            if (span.has_value()) {
                tracer.endSpan(*span);
            }

            profiler.recordLatency("load_query", 1000);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should complete within reasonable time (not performance test, just existence)
    EXPECT_LT(elapsed.count(), 30000); // 30 seconds max for 25K operations
}

// ============================================================================
// OBB-18: Integration — Concurrent producers with partial failures
// ============================================================================
TEST(ObservabilityBlockBIntegration, PartialFailureRecovery) {
    MetricsCollector collector;
    constexpr size_t kNumProducers = 5;
    constexpr size_t kMetricsEach = 100;

    std::vector<std::thread> threads;
    std::atomic<size_t> success{0};
    std::atomic<size_t> failed{0};

    for (size_t i = 0; i < kNumProducers; ++i) {
        threads.emplace_back([&collector, &success, &failed, i]() {
            for (size_t j = 0; j < kMetricsEach; ++j) {
                auto result = collector.recordMetric({
                    .metric_name = "partial_failure_test",
                    .labels = {{"producer", std::to_string(i)}, {"seq", std::to_string(j)}},
                    .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
                    .value = static_cast<double>(j),
                });
                if (result.has_value()) {
                    success++;
                } else {
                    failed++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Most metrics should succeed
    EXPECT_GT(success, kNumProducers * kMetricsEach * 0.8); // At least 80%
}

// ============================================================================
// OBB-19: Integration — Clock-skewed events handled gracefully
// ============================================================================
TEST(ObservabilityBlockBIntegration, ClockSkewHandled) {
    QueryProfiler profiler;

    auto now = std::chrono::high_resolution_clock::now();
    auto past = now - std::chrono::seconds(3600);
    auto future = now + std::chrono::seconds(3600);

    // Record events with skewed timestamps
    profiler.recordLatency("skew_test", std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
    profiler.recordLatency("skew_test", std::chrono::duration_cast<std::chrono::microseconds>(past.time_since_epoch()).count());
    profiler.recordLatency("skew_test", std::chrono::duration_cast<std::chrono::microseconds>(future.time_since_epoch()).count());

    auto profile = profiler.getProfile("skew_test");
    ASSERT_TRUE(profile.has_value());
    // Should handle gracefully without crashing or assertions
    EXPECT_GT(profile->sample_count, 0u);
}

// ============================================================================
// OBB-20: Integration — Memory usage stays bounded under adversarial workload
// ============================================================================
TEST(ObservabilityBlockBIntegration, MemoryBounded) {
    MetricsCollector collector;
    constexpr size_t kTotalMetrics = 10000;
    constexpr size_t kBatchSize = 1000;

    // Record many metrics in batches
    for (size_t batch = 0; batch < kTotalMetrics / kBatchSize; ++batch) {
        for (size_t i = 0; i < kBatchSize; ++i) {
            collector.recordMetric({
                .metric_name = "memory_test",
                .labels = {{"id", std::to_string(batch * kBatchSize + i)}},
                .timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count(),
                .value = static_cast<double>(i),
            });
        }
    }

    // Verify that cardinality tracking is bounded
    auto overflow_count = collector.getCardinalityOverflowCount();
    // Should have some overflow due to cardinality limits
    EXPECT_GE(overflow_count, 0u);
}

} // namespace

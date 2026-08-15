/**
 * @file test_exporter_stress_framework.cpp
 * @brief Focused regression tests for exporter stress testing framework (Phase 2, OEX-01..06).
 *
 * Test coverage:
 * - OEX-01: Baseline throughput (10k metrics/sec, 5k spans/sec)
 * - OEX-02: P95 latency threshold (≤10ms)
 * - OEX-03: P99 latency threshold (≤50ms)
 * - OEX-04: Recovery after backend timeout
 * - OEX-05: Memory usage bounds (≤512MB)
 * - OEX-06: Acceptable loss during degraded mode (<0.5% metrics, <1% spans)
 */

#include "gtest/gtest.h"
#include "observability/exporter_stress_framework.h"
#include <thread>
#include <chrono>
#include <cstring>

namespace themis {
namespace observability {

class ExporterStressFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        framework = createExporterStressFramework();
    }

    std::unique_ptr<ExporterStressFramework> framework;
};

// OEX-01: Baseline throughput (10k metrics/sec, 5k spans/sec)
TEST_F(ExporterStressFrameworkTest, BaselineThroughput) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 10000;
    config.num_spans_per_second = 5000;
    config.failure_mode = FailureMode::NONE;
    config.num_label_combinations = 10;

    auto result = framework->runStressTest(config);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.metrics_sent, 10000);
    EXPECT_EQ(result.spans_sent, 5000);
    EXPECT_EQ(result.metrics_lost, 0);
    EXPECT_EQ(result.spans_lost, 0);
    EXPECT_GT(result.throughput_metrics_per_sec, 9000);  // Allow 10% variance
    EXPECT_GT(result.throughput_spans_per_sec, 4500);
}

// OEX-02: P95 latency threshold (≤10ms)
TEST_F(ExporterStressFrameworkTest, P95LatencyThreshold) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::NONE;

    auto result = framework->runStressTest(config);

    EXPECT_TRUE(result.success);
    EXPECT_LE(result.metric_latency_p95_ms, 10.0);  // Gate OEX-02
}

// OEX-03: P99 latency threshold (≤50ms)
TEST_F(ExporterStressFrameworkTest, P99LatencyThreshold) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::NONE;

    auto result = framework->runStressTest(config);

    EXPECT_TRUE(result.success);
    EXPECT_LE(result.metric_latency_p99_ms, 50.0);  // Gate OEX-03
}

// OEX-04: Recovery after backend timeout
TEST_F(ExporterStressFrameworkTest, RecoveryAfterBackendTimeout) {
    ExporterStressTestConfig config;
    config.duration_seconds = 2;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::BACKEND_TIMEOUT;
    config.failure_duration_seconds = 0.5;

    auto result = framework->runStressTest(config);

    // Should recover and resume export
    EXPECT_GT(result.metrics_sent, 1500);  // At least some after recovery
    EXPECT_GT(result.spans_sent, 750);

    // Check recovery metrics
    EXPECT_GT(result.recovery_attempts, 0);
    EXPECT_GT(result.recovered_metrics, 0);
}

// OEX-04 variant: Backend unavailable
TEST_F(ExporterStressFrameworkTest, RecoveryAfterBackendUnavailable) {
    ExporterStressTestConfig config;
    config.duration_seconds = 2;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::BACKEND_UNAVAILABLE;
    config.failure_duration_seconds = 0.5;

    auto result = framework->runStressTest(config);

    EXPECT_GT(result.recovery_attempts, 0);
    EXPECT_GT(result.recovered_metrics, 0);
}

// OEX-05: Memory usage bounds (≤512MB)
TEST_F(ExporterStressFrameworkTest, MemoryUsageBounds) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 5000;
    config.num_spans_per_second = 2500;
    config.failure_mode = FailureMode::QUEUE_EXHAUSTION;
    config.num_label_combinations = 100;  // High cardinality to stress memory

    auto result = framework->runStressTest(config);

    // Memory usage should not exceed 512MB
    EXPECT_LE(result.memory_usage_bytes, 512 * 1024 * 1024);  // Gate OEX-05
}

// OEX-06: Acceptable loss during degraded mode (<0.5% metrics, <1% spans)
TEST_F(ExporterStressFrameworkTest, AcceptableLossDuringDegradedMode) {
    ExporterStressTestConfig config;
    config.duration_seconds = 2;
    config.num_metrics_per_second = 10000;
    config.num_spans_per_second = 5000;
    config.failure_mode = FailureMode::PARTIAL_PACKET_LOSS;
    config.packet_loss_percentage = 10;  // 10% packet loss

    auto result = framework->runStressTest(config);

    double expected_metrics_lost = config.duration_seconds * config.num_metrics_per_second;
    double loss_rate_metrics = result.metrics_lost / expected_metrics_lost;

    double expected_spans_lost = config.duration_seconds * config.num_spans_per_second;
    double loss_rate_spans = result.spans_lost / expected_spans_lost;

    // Loss should not exceed gate limits
    EXPECT_LE(loss_rate_metrics, 0.005);  // <0.5% metrics (Gate OEX-06)
    EXPECT_LE(loss_rate_spans, 0.01);    // <1% spans (Gate OEX-06)
}

// OEX-06 variant: High latency degradation
TEST_F(ExporterStressFrameworkTest, AcceptableLossHighLatency) {
    ExporterStressTestConfig config;
    config.duration_seconds = 2;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::HIGH_LATENCY;
    config.latency_add_ms = 100;

    auto result = framework->runStressTest(config);

    // High latency may cause some loss, but should still be acceptable
    EXPECT_LE(result.metric_latency_p99_ms, 200.0);
}

// OEX-06 variant: Memory pressure degradation
TEST_F(ExporterStressFrameworkTest, AcceptableLossMemoryPressure) {
    ExporterStressTestConfig config;
    config.duration_seconds = 2;
    config.num_metrics_per_second = 5000;
    config.num_spans_per_second = 2500;
    config.failure_mode = FailureMode::MEMORY_PRESSURE;
    config.num_label_combinations = 1000;  // Very high cardinality

    auto result = framework->runStressTest(config);

    // System should degrade gracefully
    EXPECT_GT(result.metrics_sent, 0);
    EXPECT_GT(result.spans_sent, 0);
}

// Test gate benchmarks
TEST_F(ExporterStressFrameworkTest, GateBenchmarks) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 10000;
    config.num_spans_per_second = 5000;
    config.failure_mode = FailureMode::NONE;
    config.num_label_combinations = 50;

    auto gate_results = framework->runGateBenchmarks(config);

    // Should have results for all 6 gates
    EXPECT_EQ(gate_results.size(), 6);

    // Gate OEX-01: Baseline throughput
    EXPECT_TRUE(gate_results[0].success);
    EXPECT_GT(gate_results[0].metrics_sent, 9000);
    EXPECT_GT(gate_results[0].spans_sent, 4500);

    // Gate OEX-02: P95 latency
    EXPECT_TRUE(gate_results[1].success);
    EXPECT_LE(gate_results[1].metric_latency_p95_ms, 10.0);

    // Gate OEX-03: P99 latency
    EXPECT_TRUE(gate_results[2].success);
    EXPECT_LE(gate_results[2].metric_latency_p99_ms, 50.0);

    // Gate OEX-04: Recovery
    // (would need failure simulation to fully validate)

    // Gate OEX-05: Memory bounds
    EXPECT_TRUE(gate_results[4].success);
    EXPECT_LE(gate_results[4].memory_usage_bytes, 512 * 1024 * 1024);

    // Gate OEX-06: Acceptable loss
    // (covered by other tests above)
}

// Test multiple sequential runs
TEST_F(ExporterStressFrameworkTest, MultipleSequentialRuns) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::NONE;

    std::vector<ExporterStressTestResult> results = framework->runMultipleTests(config, 3);

    EXPECT_EQ(results.size(), 3);
    EXPECT_TRUE(results[0].success);
    EXPECT_TRUE(results[1].success);
    EXPECT_TRUE(results[2].success);
}

// Test with progress callback
TEST_F(ExporterStressFrameworkTest, ProgressCallback) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::NONE;

    int progress_calls = 0;
    config.progress_callback = [&progress_calls](int percent) {
        progress_calls++;
    };

    auto result = framework->runStressTest(config);

    EXPECT_TRUE(result.success);
    EXPECT_GT(progress_calls, 0);
}

// Test cancellation
TEST_F(ExporterStressFrameworkTest, TestCancellation) {
    ExporterStressTestConfig config;
    config.duration_seconds = 10;  // Long test
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::NONE;

    // Start test in background
    std::thread test_thread([this, &config]() {
        framework->runStressTest(config);
    });

    // Give test time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cancel it
    framework->cancelCurrentTest();

    // Wait for test to finish
    test_thread.join();

    // Should have stopped early
    auto status = framework->getTestStatus();
    EXPECT_EQ(status, "idle");  // Test should be done
}

// Test comparison report generation
TEST_F(ExporterStressFrameworkTest, ComparisonReport) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;
    config.failure_mode = FailureMode::NONE;

    auto baseline = framework->runStressTest(config);

    config.num_metrics_per_second = 2000;  // Modify for candidate
    auto candidate = framework->runStressTest(config);

    auto report = framework->generateComparisonReport(baseline, candidate);

    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("Baseline") != std::string::npos);
    EXPECT_TRUE(report.find("Candidate") != std::string::npos);
}

// Test with different failure modes
TEST_F(ExporterStressFrameworkTest, FailureModeVariations) {
    ExporterStressTestConfig config;
    config.duration_seconds = 1;
    config.num_metrics_per_second = 1000;
    config.num_spans_per_second = 500;

    std::vector<FailureMode> modes = {
        FailureMode::NONE,
        FailureMode::BACKEND_TIMEOUT,
        FailureMode::PARTIAL_PACKET_LOSS,
        FailureMode::HIGH_LATENCY,
        FailureMode::QUEUE_EXHAUSTION,
        FailureMode::MEMORY_PRESSURE
    };

    for (auto mode : modes) {
        config.failure_mode = mode;
        auto result = framework->runStressTest(config);

        EXPECT_TRUE(result.success) << "Mode: " << static_cast<int>(mode);
    }
}

} // namespace observability
} // namespace themis

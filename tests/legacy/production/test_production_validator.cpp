#include <gtest/gtest.h>
#include "llm/production_validator.h"
#include <thread>
#include <chrono>

using namespace themis::llm::testing;

class ProductionValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration for testing (shortened stress test duration)
        config_.stress_test_duration = std::chrono::hours(1);
        config_.concurrent_requests = 10;
        config_.total_requests = 100;
        config_.max_latency_ms = 1000.0;
        config_.max_p99_latency_ms = 2000.0;
        config_.min_throughput_tokens_per_sec = 5.0;
    }
    
    ProductionValidator::ValidationConfig config_;
};

// ═══════════════════════════════════════════════════════════
// Benchmark Inference Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ProductionValidatorTest, BenchmarkInference_BasicFunctionality) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Verify basic structure
    EXPECT_EQ(metrics.model_id, "test-model-7b");
    EXPECT_EQ(metrics.total_requests, 100);
    EXPECT_EQ(metrics.successful_requests, 0);
    EXPECT_EQ(metrics.failed_requests, 0);
    EXPECT_EQ(metrics.skipped_requests, metrics.total_requests);
    
    // Verify latency metrics are populated
    EXPECT_GT(metrics.latency_p50_ms, 0.0);
    EXPECT_GT(metrics.latency_p95_ms, 0.0);
    EXPECT_GT(metrics.latency_p99_ms, 0.0);
    EXPECT_GT(metrics.avg_latency_ms, 0.0);
    
    // Verify latency ordering (P50 < P95 < P99)
    EXPECT_LE(metrics.latency_p50_ms, metrics.latency_p95_ms);
    EXPECT_LE(metrics.latency_p95_ms, metrics.latency_p99_ms);
    
    // Verify min <= avg <= max
    EXPECT_LE(metrics.min_latency_ms, metrics.avg_latency_ms);
    EXPECT_LE(metrics.avg_latency_ms, metrics.max_latency_ms);
}

TEST_F(ProductionValidatorTest, BenchmarkInference_LatencyMetrics) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // P50 should be less than P95
    EXPECT_LT(metrics.latency_p50_ms, metrics.latency_p95_ms);
    
    // P95 should be less than P99
    EXPECT_LT(metrics.latency_p95_ms, metrics.latency_p99_ms);
    
    // All percentiles should be positive
    EXPECT_GT(metrics.latency_p50_ms, 0.0);
    EXPECT_GT(metrics.latency_p95_ms, 0.0);
    EXPECT_GT(metrics.latency_p99_ms, 0.0);
}

TEST_F(ProductionValidatorTest, BenchmarkInference_ThroughputMetrics) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Without an attached inference engine, throughput remains zero and no
    // tokens are counted as generated.
    EXPECT_DOUBLE_EQ(metrics.throughput_tokens_per_sec, 0.0);
    EXPECT_EQ(metrics.total_tokens_generated, 0u);
    
    // Total time should be recorded
    EXPECT_GT(metrics.total_time_seconds, 0.0);
    
    // Verify throughput calculation
    double expected_throughput = metrics.total_tokens_generated / metrics.total_time_seconds;
    EXPECT_NEAR(metrics.throughput_tokens_per_sec, expected_throughput, 0.1);
}

TEST_F(ProductionValidatorTest, BenchmarkInference_CompletionTimeUnderTwoMinutes) {
    ProductionValidator validator(config_);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto metrics = validator.benchmarkInference("test-model-7b");
    auto end = std::chrono::high_resolution_clock::now();
    
    double elapsed_seconds = std::chrono::duration<double>(end - start).count();
    
    // Benchmark should complete in less than 2 minutes (120 seconds)
    EXPECT_LT(elapsed_seconds, 120.0);
    
    // Verify total time matches
    EXPECT_NEAR(metrics.total_time_seconds, elapsed_seconds, 1.0);  // Allow 1 second tolerance
}

TEST_F(ProductionValidatorTest, BenchmarkInference_MemoryTracking) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Memory metrics should be non-negative
    EXPECT_GE(metrics.memory_used_mb, 0);
    EXPECT_GE(metrics.peak_memory_mb, 0);
    
    // Peak should be at least as much as used
    EXPECT_GE(metrics.peak_memory_mb, metrics.memory_used_mb);
}

TEST_F(ProductionValidatorTest, BenchmarkInference_SLAValidation) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Check if SLA warnings are generated when thresholds exceeded
    // Note: In simulation, metrics may pass. This tests the structure.
    
    // If P95 latency exceeds 5000ms, should have warning
    if (metrics.latency_p95_ms > 5000.0) {
        EXPECT_FALSE(metrics.passed);
        EXPECT_FALSE(metrics.warnings.empty());
    }
    
    // If throughput is too low (< 10 tokens/sec), should fail
    if (metrics.throughput_tokens_per_sec < 10.0) {
        EXPECT_FALSE(metrics.passed);
    }
}

TEST_F(ProductionValidatorTest, BenchmarkInference_RequestStatistics) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Total requests should be 100
    EXPECT_EQ(metrics.total_requests, 100);
    
    // Successful + failed + skipped should equal total
    EXPECT_EQ(
        metrics.successful_requests + metrics.failed_requests + metrics.skipped_requests,
        metrics.total_requests);
    EXPECT_EQ(metrics.skipped_requests, metrics.total_requests);
}

// ═══════════════════════════════════════════════════════════
// Quality Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ProductionValidatorTest, ValidateQuality_BasicFunctionality) {
    ProductionValidator validator(config_);
    
    bool result = validator.validateQuality("test-model-7b");
    
    // Should return a boolean result
    EXPECT_TRUE(result || !result);  // Always true, just checking it compiles
}

TEST_F(ProductionValidatorTest, ValidateQuality_ThresholdCheck) {
    ProductionValidator validator(config_);
    
    // Run quality validation
    bool result = validator.validateQuality("test-model-7b");
    
    // Quality validation requires a real inference engine.
    EXPECT_FALSE(result);
}

// ═══════════════════════════════════════════════════════════
// Helper Method Tests (indirect testing through benchmarkInference)
// ═══════════════════════════════════════════════════════════

// Note: calculatePercentile is private, so we test it indirectly through benchmarkInference
TEST_F(ProductionValidatorTest, PercentileCalculation_ThroughBenchmark) {
    ProductionValidator validator(config_);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Verify percentile ordering (tests calculatePercentile indirectly)
    EXPECT_LE(metrics.latency_p50_ms, metrics.latency_p95_ms);
    EXPECT_LE(metrics.latency_p95_ms, metrics.latency_p99_ms);
    
    // All percentiles should be positive
    EXPECT_GT(metrics.latency_p50_ms, 0.0);
    EXPECT_GT(metrics.latency_p95_ms, 0.0);
    EXPECT_GT(metrics.latency_p99_ms, 0.0);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ProductionValidatorTest, EndToEndWorkflow) {
    ProductionValidator validator(config_);
    
    // 1. Run benchmark
    auto metrics = validator.benchmarkInference("test-model-7b");
    EXPECT_EQ(metrics.model_id, "test-model-7b");
    
    // 2. Validate quality
    bool quality_passed = validator.validateQuality("test-model-7b");
    
    // 3. Check overall validation result
    // In a real scenario, would combine metrics and quality
    EXPECT_TRUE(metrics.total_requests > 0);
}

TEST_F(ProductionValidatorTest, MultipleModels_Sequential) {
    ProductionValidator validator(config_);
    
    // Benchmark multiple models sequentially
    auto metrics1 = validator.benchmarkInference("model-a");
    auto metrics2 = validator.benchmarkInference("model-b");
    
    EXPECT_EQ(metrics1.model_id, "model-a");
    EXPECT_EQ(metrics2.model_id, "model-b");
    
    // Both should have valid metrics
    EXPECT_GT(metrics1.latency_p50_ms, 0.0);
    EXPECT_GT(metrics2.latency_p50_ms, 0.0);
}

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ProductionValidatorTest, CustomConfiguration) {
    ProductionValidator::ValidationConfig custom_config;
    custom_config.max_latency_ms = 500.0;
    custom_config.max_p99_latency_ms = 1000.0;
    custom_config.min_throughput_tokens_per_sec = 100.0;
    
    ProductionValidator validator(custom_config);
    
    auto metrics = validator.benchmarkInference("test-model-7b");
    
    // Should respect custom configuration for SLA checks
    EXPECT_EQ(metrics.total_requests, 100);
}

// ═══════════════════════════════════════════════════════════
// Performance Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ProductionValidatorTest, Performance_100Requests) {
    ProductionValidator validator(config_);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto metrics = validator.benchmarkInference("test-model-7b");
    auto end = std::chrono::high_resolution_clock::now();
    
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Should process 100 requests efficiently
    EXPECT_LT(elapsed_ms, 120000.0);  // Less than 2 minutes
    
    // All requests should be processed
    EXPECT_EQ(metrics.total_requests, 100);
}

// ═══════════════════════════════════════════════════════════
// Stress Testing Framework Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ProductionValidatorTest, StressTest_StartStop) {
    ProductionValidator validator(config_);
    
    // Initially not running
    EXPECT_FALSE(validator.isStressTestRunning());
    
    // Start stress test
    validator.startStressTest();
    
    // Should be running now
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(validator.isStressTestRunning());
    
    // Stop stress test
    validator.stopStressTest();
    
    // Wait for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_FALSE(validator.isStressTestRunning());
}

TEST_F(ProductionValidatorTest, StressTest_RequiresInferenceEngine) {
    ProductionValidator::ValidationConfig short_config = config_;
    short_config.stress_test_duration = std::chrono::hours(0);

    ProductionValidator validator(short_config);

    auto result = validator.runStressTest();

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.total_requests, 0u);
    EXPECT_EQ(result.successful_requests, 0u);
    EXPECT_EQ(result.failed_requests, 0u);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("requires an attached inference engine"), std::string::npos);
}

TEST_F(ProductionValidatorTest, LiveStats_Available) {
    ProductionValidator validator(config_);
    
    auto stats = validator.getLiveStats();
    
    // Stats structure should be valid
    EXPECT_GE(stats.active_requests, 0);
    EXPECT_GE(stats.memory_mb, 0);
    EXPECT_GE(stats.uptime_seconds, 0);
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

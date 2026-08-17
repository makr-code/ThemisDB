/**
 * @file production_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/continuous_batch_scheduler.h"
#include "llm/gpu_memory_manager.h"
#include "llm/kernel_fusion.h"
#include "llm/inference_engine_enhanced.h"
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>

namespace themis {
namespace llm {
namespace testing {

/**
 * @brief Production Validation Framework
 * 
 * Week 13-14 Implementation: End-to-end system integration testing,
 * production validation, and stress testing for 72+ hours stability.
 * 
 * LOCK HIERARCHY (always acquire in this order to prevent deadlocks):
 * 1. validation_state_lock_ → State machine transitions (exclusive)
 *    └─ validation_queue_lock_ → Results queue access (exclusive)
 *       └─ metrics_lock_ → Telemetry updates (exclusive)
 *
 * Memory Ordering:
 * - stress_test_running_: std::memory_order_acquire/release
 * - total_requests_processed_, total_failures_: std::memory_order_relaxed
 * 
 * Thread Safety:
 * - All public methods are thread-safe via internal mutexes
 * - Validation state machine (IDLE → RUNNING → COMPLETE) uses atomics
 * - Latency samples protected by latency_mutex_
 */
class ProductionValidator {
public:
    virtual ~ProductionValidator() = default;
    struct ValidationConfig {
        // Stress test duration
        std::chrono::hours stress_test_duration{72};
        
        // Load testing
        size_t concurrent_requests = 100;
        size_t requests_per_second = 50;
        size_t total_requests = 100000;
        
        // Quality checks
        double max_latency_ms = 100.0;
        double max_p99_latency_ms = 200.0;
        double min_throughput_tokens_per_sec = 1000.0;
        double max_error_rate_pct = 0.1;
        
        // Memory checks
        double max_memory_growth_mb_per_hour = 10.0;
        size_t max_fragmentation_pct = 15;
        
        // Performance regression
        double max_regression_pct = 1.0;  // Max 1% regression allowed
    };
    
    struct ValidationResult {
        bool passed = false;
        std::string error_message;
        
        // Performance metrics
        double avg_latency_ms = 0.0;
        double p50_latency_ms = 0.0;
        double p95_latency_ms = 0.0;
        double p99_latency_ms = 0.0;
        double throughput_tokens_per_sec = 0.0;
        
        // Quality metrics
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        double error_rate_pct = 0.0;
        
        // Memory metrics
        size_t peak_memory_mb = 0;
        size_t final_memory_mb = 0;
        double memory_growth_mb = 0.0;
        size_t max_fragmentation_pct = 0;
        
        // Stability metrics
        size_t num_crashes = 0;
        double uptime_pct = 0.0;
        std::chrono::seconds total_uptime{0};
    };
    
    /**
     * @brief Production Metrics for LLM inference benchmarking
     * 
     * Returned by benchmarkInference() to provide detailed performance
     * metrics for a specific model.
     */
    struct ProductionMetrics {
        std::string model_id;
        bool passed = false;
        std::string error_message;
        std::vector<std::string> warnings;
        
        // Latency metrics (milliseconds)
        double latency_p50_ms = 0.0;
        double latency_p95_ms = 0.0;
        double latency_p99_ms = 0.0;
        double avg_latency_ms = 0.0;
        double max_latency_ms = 0.0;
        double min_latency_ms = 0.0;
        
        // Throughput metrics
        double throughput_tokens_per_sec = 0.0;
        size_t total_tokens_generated = 0;
        double total_time_seconds = 0.0;
        
        // Quality score (0-100%)
        double quality_score_pct = 0.0;
        size_t quality_tests_passed = 0;
        size_t quality_tests_total = 0;
        
        // Memory metrics
        size_t memory_used_mb = 0;
        size_t peak_memory_mb = 0;
        
        // Request statistics
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        size_t skipped_requests = 0;
    };
    
    explicit ProductionValidator(const ValidationConfig& config);
    
    // Main validation methods
    ValidationResult runEndToEndTests();
    ValidationResult runStressTest();
    ValidationResult runLoadTest();
    ValidationResult checkPerformanceRegression(
        const std::string& baseline_file
    );
    
    /**
     * @brief Benchmark LLM inference performance
     * 
     * Runs a comprehensive benchmark suite with 100 requests of varying lengths,
     * measures latency percentiles (P50, P95, P99), throughput, quality tests,
     * and memory usage. Validates against SLA thresholds.
     * 
     * @param model_id Identifier of the model to benchmark
     * @return ProductionMetrics with detailed performance and quality metrics
     */
    ProductionMetrics benchmarkInference(const std::string& model_id);
    
    /**
     * @brief Validate model quality with standard test suite
     * 
     * Runs math, knowledge, and reasoning tests to verify model quality.
     * Requires ≥80% pass rate to meet acceptance criteria.
     * 
     * @param model_id Identifier of the model to validate
     * @return true if quality score ≥ 80%, false otherwise
     */
    bool validateQuality(const std::string& model_id);
    
    // Individual test suites
    bool testModelLoading();
    bool testInferencePipeline();
    bool testBatchScheduling();
    bool testMemoryManagement();
    bool testGPUOffload();
    bool testQuantization();
    bool testContinuousBatching();
    bool testKernelFusion();
    
    // Stress testing
    void startStressTest();
    void stopStressTest();
    bool isStressTestRunning() const;
    
    // Monitoring
    struct LiveStats {
        size_t active_requests = 0;
        double current_latency_ms = 0.0;
        double current_throughput = 0.0;
        size_t memory_mb = 0;
        size_t uptime_seconds = 0;
    };
    
    LiveStats getLiveStats() const;

    /**
     * @brief Set the inference engine used by benchmark and stress test.
     *
     * When set, benchmarkInference() and runStressTest() route requests
     * through this engine.  Without an engine the benchmark logs a warning
     * and reports skipped requests.
     */
    void setInferenceEngine(std::shared_ptr<InferenceEngineEnhanced> engine);
    
private:
    ValidationConfig config_;
    std::shared_ptr<InferenceEngineEnhanced> inference_engine_;

    // LOCK HIERARCHY ENFORCEMENT (§3.4):
    // ┌─ validation_state_lock_ : std::mutex (state machine)
    // │  └─ validation_queue_lock_ : std::mutex (results queue)
    // │     └─ metrics_lock_ : std::mutex (telemetry)
    // └─ latency_mutex_ : std::mutex (independent statistics)
    
    /// Exclusive lock for validation state transitions (IDLE → RUNNING → COMPLETE)
    mutable std::mutex validation_state_lock_;
    
    /// Exclusive lock for validation results queue
    mutable std::mutex validation_queue_lock_;
    
    /// Exclusive lock for telemetry and statistics updates
    mutable std::mutex metrics_lock_;

    // Test state (protected by validation_state_lock_)
    /// Atomic flag: stress test is running (memory_order_acquire/release)
    std::atomic<bool> stress_test_running_{false};
    
    /// Stress test start time (protected by validation_state_lock_)
    std::chrono::system_clock::time_point stress_test_start_;
    
    /// Memory baseline in MB (protected by validation_state_lock_)
    size_t memory_baseline_mb_ = 0;   ///< Set on first checkMemoryLeaks() call or reset()
    
    // Statistics (protected by metrics_lock_ or latency_mutex_)
    
    /// Latency samples (protected by latency_mutex_; use deque for efficient removal)
    std::deque<double> latency_samples_;
    
    /// Protects latency_samples_ in const and non-const paths
    mutable std::mutex latency_mutex_;
    
    /// Total requests processed (protected by metrics_lock_; may use std::memory_order_relaxed)
    std::atomic<size_t> total_requests_processed_{0};
    
    /// Total failures (protected by metrics_lock_; may use std::memory_order_relaxed)
    std::atomic<size_t> total_failures_{0};
    
    // Helper methods
    double calculatePercentile(const std::vector<double>& data, double percentile);
    void recordLatency(double latency_ms);
    void checkMemoryLeaks();
    
    // Benchmark helpers
    std::string generateBenchmarkPrompt(int variant);
    size_t measureMemoryUsage();
    
    // Quality test helpers
    struct QualityTest {
        std::string category;
        std::string prompt;
        std::vector<std::string> expected_answers;
    };
    std::vector<QualityTest> getQualityTests();
    bool evaluateQualityTest(const QualityTest& test, const std::string& model_id);
};

/**
 * @brief Performance Regression Framework
 * 
 * Detects performance degradation by comparing against baselines.
 */
class PerformanceRegressionDetector {
public:
    virtual ~PerformanceRegressionDetector() = default;
    struct Baseline {
        double avg_latency_ms = 0.0;
        double p99_latency_ms = 0.0;
        double throughput_tokens_per_sec = 0.0;
        size_t memory_usage_mb = 0;
        
        std::string version;
        std::chrono::system_clock::time_point recorded_at;
    };
    
    struct RegressionReport {
        bool has_regression = false;
        
        double latency_change_pct = 0.0;
        double p99_latency_change_pct = 0.0;
        double throughput_change_pct = 0.0;
        double memory_change_pct = 0.0;
        
        std::vector<std::string> regressions;
        std::vector<std::string> improvements;
    };
    
    // Save/load baselines
    bool saveBaseline(const std::string& filepath, const Baseline& baseline);
    bool loadBaseline(const std::string& filepath, Baseline& baseline);
    
    // Compare current performance against baseline
    RegressionReport detectRegression(
        const Baseline& baseline,
        const ProductionValidator::ValidationResult& current,
        double threshold_pct = 1.0
    );
    
private:
    std::vector<Baseline> historical_baselines_;
};

/**
 * @brief Integration Test Suite
 * 
 * Tests all components working together.
 */
class IntegrationTestSuite {
public:
    // Component integration tests
    bool testLazyLoaderWithGPUMemory();
    bool testSchedulerWithPagedAttention();
    bool testKernelFusionWithInference();
    bool testFullPipelineE2E();
    
    // Multi-model scenarios
    bool testMultiModelServing();
    bool testModelSwitching();
    bool testLoRAAdapterManagement();
    
    // Failure scenarios
    bool testGPUOutOfMemory();
    bool testModelLoadFailure();
    bool testRequestCancellation();
    bool testPreemption();
    
    // Performance scenarios
    bool testHighConcurrency();
    bool testLongRunningRequests();
    bool testBurstTraffic();
    
    struct TestResult {
        std::string test_name;
        bool passed = false;
        std::string error_message;
        double duration_ms = 0.0;
    };
    
    std::vector<TestResult> runAllTests();
};

} // namespace testing
} // namespace llm
} // namespace themis

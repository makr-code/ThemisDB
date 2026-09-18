/**
 * @file production_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ProductionValidator {
public:
    /**
     * @brief Production Validator.
     * @return Return value.
     */
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
    
    /**
     * @brief Production Validator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ProductionValidator(const ValidationConfig& config);
    
    // Main validation methods
    /**
     * @brief Run End To End Tests.
     * @return Return value.
     */
    ValidationResult runEndToEndTests();
    /**
     * @brief Run Stress Test.
     * @return Return value.
     */
    ValidationResult runStressTest();
    /**
     * @brief Run Load Test.
     * @return Return value.
     */
    ValidationResult runLoadTest();
    /**
     * @brief Check Performance Regression.
     * @param[in] baseline_file Input parameter.
     * @return Return value.
     */
    ValidationResult checkPerformanceRegression(
        const std::string& baseline_file
    );
    
    /**
     * @brief Benchmark Inference.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    ProductionMetrics benchmarkInference(const std::string& model_id);
    
    /**
     * @brief Validate Quality.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool validateQuality(const std::string& model_id);
    
    // Individual test suites
    /**
     * @brief Test Model Loading.
     * @return True when the operation succeeds.
     */
    bool testModelLoading();
    /**
     * @brief Test Inference Pipeline.
     * @return True when the operation succeeds.
     */
    bool testInferencePipeline();
    /**
     * @brief Test Batch Scheduling.
     * @return True when the operation succeeds.
     */
    bool testBatchScheduling();
    /**
     * @brief Test Memory Management.
     * @return True when the operation succeeds.
     */
    bool testMemoryManagement();
    /**
     * @brief Test GPUOffload.
     * @return True when the operation succeeds.
     */
    bool testGPUOffload();
    /**
     * @brief Test Quantization.
     * @return True when the operation succeeds.
     */
    bool testQuantization();
    /**
     * @brief Test Continuous Batching.
     * @return True when the operation succeeds.
     */
    bool testContinuousBatching();
    /**
     * @brief Test Kernel Fusion.
     * @return True when the operation succeeds.
     */
    bool testKernelFusion();
    
    // Stress testing
    /**
     * @brief Start Stress Test.
     */
    void startStressTest();
    /**
     * @brief Stop Stress Test.
     */
    void stopStressTest();
    /**
     * @brief Is Stress Test Running.
     * @return True when the operation succeeds.
     */
    bool isStressTestRunning() const;
    
    // Monitoring
    struct LiveStats {
        size_t active_requests = 0;
        double current_latency_ms = 0.0;
        double current_throughput = 0.0;
        size_t memory_mb = 0;
        size_t uptime_seconds = 0;
    };
    
    /**
     * @brief Get Live Stats.
     * @return Return value.
     */
    LiveStats getLiveStats() const;

    /**
     * @brief Set Inference Engine.
     * @param[in] engine Input parameter.
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
    
    mutable std::mutex validation_state_lock_;
    
    mutable std::mutex validation_queue_lock_;
    
    mutable std::mutex metrics_lock_;

    // Test state (protected by validation_state_lock_)
    std::atomic<bool> stress_test_running_{false};
    
    std::chrono::system_clock::time_point stress_test_start_;
    
    size_t memory_baseline_mb_ = 0;   ///< Set on first checkMemoryLeaks() call or reset()
    
    // Statistics (protected by metrics_lock_ or latency_mutex_)
    
    std::deque<double> latency_samples_;
    
    mutable std::mutex latency_mutex_;
    
    std::atomic<size_t> total_requests_processed_{0};
    
    std::atomic<size_t> total_failures_{0};
    
    // Helper methods
    /**
     * @brief Calculate Percentile.
     * @param[in] data Input parameter.
     * @param[in] percentile Input parameter.
     * @return Return value.
     */
    double calculatePercentile(const std::vector<double>& data, double percentile);
    /**
     * @brief Record Latency.
     * @param[in] latency_ms Input parameter.
     */
    void recordLatency(double latency_ms);
    /**
     * @brief Check Memory Leaks.
     */
    void checkMemoryLeaks();
    
    // Benchmark helpers
    /**
     * @brief Generate Benchmark Prompt.
     * @param[in] variant Input parameter.
     * @return Return value.
     */
    std::string generateBenchmarkPrompt(int variant);
    /**
     * @brief Measure Memory Usage.
     * @return Return value.
     */
    size_t measureMemoryUsage();
    
    // Quality test helpers
    struct QualityTest {
        std::string category;
        std::string prompt;
        std::vector<std::string> expected_answers;
    };
    /**
     * @brief Get Quality Tests.
     * @return Return value.
     */
    std::vector<QualityTest> getQualityTests();
    /**
     * @brief Evaluate Quality Test.
     * @param[in] test Input parameter.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool evaluateQualityTest(const QualityTest& test, const std::string& model_id);
};

class PerformanceRegressionDetector {
public:
    /**
     * @brief Performance Regression Detector.
     * @return Return value.
     */
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
    /**
     * @brief Save Baseline.
     * @param[in] filepath Input parameter.
     * @param[in] baseline Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveBaseline(const std::string& filepath, const Baseline& baseline);
    /**
     * @brief Load Baseline.
     * @param[in] filepath Input parameter.
     * @param[in,out] baseline Input/output parameter.
     * @return True when the operation succeeds.
     */
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

class IntegrationTestSuite {
public:
    // Component integration tests
    /**
     * @brief Test Lazy Loader With GPUMemory.
     * @return True when the operation succeeds.
     */
    bool testLazyLoaderWithGPUMemory();
    /**
     * @brief Test Scheduler With Paged Attention.
     * @return True when the operation succeeds.
     */
    bool testSchedulerWithPagedAttention();
    /**
     * @brief Test Kernel Fusion With Inference.
     * @return True when the operation succeeds.
     */
    bool testKernelFusionWithInference();
    /**
     * @brief Test Full Pipeline E2 E.
     * @return True when the operation succeeds.
     */
    bool testFullPipelineE2E();
    
    // Multi-model scenarios
    /**
     * @brief Test Multi Model Serving.
     * @return True when the operation succeeds.
     */
    bool testMultiModelServing();
    /**
     * @brief Test Model Switching.
     * @return True when the operation succeeds.
     */
    bool testModelSwitching();
    /**
     * @brief Test Lo RAAdapter Management.
     * @return True when the operation succeeds.
     */
    bool testLoRAAdapterManagement();
    
    // Failure scenarios
    /**
     * @brief Test GPUOut Of Memory.
     * @return True when the operation succeeds.
     */
    bool testGPUOutOfMemory();
    /**
     * @brief Test Model Load Failure.
     * @return True when the operation succeeds.
     */
    bool testModelLoadFailure();
    /**
     * @brief Test Request Cancellation.
     * @return True when the operation succeeds.
     */
    bool testRequestCancellation();
    /**
     * @brief Test Preemption.
     * @return True when the operation succeeds.
     */
    bool testPreemption();
    
    // Performance scenarios
    /**
     * @brief Test High Concurrency.
     * @return True when the operation succeeds.
     */
    bool testHighConcurrency();
    /**
     * @brief Test Long Running Requests.
     * @return True when the operation succeeds.
     */
    bool testLongRunningRequests();
    /**
     * @brief Test Burst Traffic.
     * @return True when the operation succeeds.
     */
    bool testBurstTraffic();
    
    struct TestResult {
        std::string test_name;
        bool passed = false;
        std::string error_message;
        double duration_ms = 0.0;
    };
    
    /**
     * @brief Run All Tests.
     * @return Return value.
     */
    std::vector<TestResult> runAllTests();
};

} // namespace testing
} // namespace llm
} // namespace themis

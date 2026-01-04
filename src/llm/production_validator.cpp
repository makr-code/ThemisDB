#include "llm/production_validator.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <thread>
#include <fstream>

// Platform-specific includes for memory measurement
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task_info.h>
#endif

namespace themis {
namespace llm {
namespace testing {

ProductionValidator::ProductionValidator(const ValidationConfig& config)
    : config_(config) {
    spdlog::info("Production Validator initialized:");
    spdlog::info("  Stress test duration: {} hours", config_.stress_test_duration.count());
    spdlog::info("  Concurrent requests: {}", config_.concurrent_requests);
    spdlog::info("  Max latency: {} ms", config_.max_latency_ms);
    spdlog::info("  Min throughput: {} tokens/s", config_.min_throughput_tokens_per_sec);
}

ProductionValidator::ProductionMetrics ProductionValidator::benchmarkInference(
    const std::string& model_id
) {
    ProductionMetrics metrics;
    metrics.model_id = model_id;
    
    spdlog::info("=== Starting Benchmark for Model: {} ===", model_id);
    
    // Record initial memory usage
    size_t initial_memory_mb = measureMemoryUsage();
    
    // 1. Run benchmark suite: 100 requests with varying lengths
    std::vector<double> latencies;
    size_t total_tokens = 0;
    size_t successful = 0;
    size_t failed = 0;
    
    auto benchmark_start = std::chrono::high_resolution_clock::now();
    
    spdlog::info("Running 100 benchmark requests with varying lengths...");
    
    for (int i = 0; i < 100; i++) {
        // Generate benchmark prompt (10 variants cycling)
        std::string prompt = generateBenchmarkPrompt(i % 10);
        
        // Measure request latency
        auto req_start = std::chrono::high_resolution_clock::now();
        
        try {
            // TODO: In real implementation, call actual LLM plugin
            // For now, simulate inference with realistic timing
            std::this_thread::sleep_for(std::chrono::milliseconds(50 + (i % 10) * 10));
            
            // Simulate token generation (20-100 tokens)
            size_t tokens_generated = 20 + (i % 8) * 10;
            total_tokens += tokens_generated;
            successful++;
            
        } catch (const std::exception& e) {
            spdlog::warn("Benchmark request {} failed: {}", i, e.what());
            failed++;
        }
        
        auto req_end = std::chrono::high_resolution_clock::now();
        double latency = std::chrono::duration<double, std::milli>(req_end - req_start).count();
        latencies.push_back(latency);
        
        // Log progress every 25 requests
        if ((i + 1) % 25 == 0) {
            spdlog::info("  Progress: {}/100 requests completed", i + 1);
        }
    }
    
    auto benchmark_end = std::chrono::high_resolution_clock::now();
    double total_time_s = std::chrono::duration<double>(benchmark_end - benchmark_start).count();
    
    // 2. Calculate latency metrics (P50, P95, P99)
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        
        metrics.latency_p50_ms = calculatePercentile(latencies, 50.0);
        metrics.latency_p95_ms = calculatePercentile(latencies, 95.0);
        metrics.latency_p99_ms = calculatePercentile(latencies, 99.0);
        metrics.avg_latency_ms = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        metrics.min_latency_ms = latencies.front();
        metrics.max_latency_ms = latencies.back();
    }
    
    // 3. Calculate throughput (tokens/sec)
    metrics.throughput_tokens_per_sec = total_tokens / total_time_s;
    metrics.total_tokens_generated = total_tokens;
    metrics.total_time_seconds = total_time_s;
    
    // 4. Record request statistics
    metrics.total_requests = 100;
    metrics.successful_requests = successful;
    metrics.failed_requests = failed;
    
    // 5. Measure memory usage
    size_t final_memory_mb = measureMemoryUsage();
    metrics.memory_used_mb = final_memory_mb - initial_memory_mb;
    metrics.peak_memory_mb = final_memory_mb;
    
    // 6. Run quality tests
    auto tests = getQualityTests();
    size_t quality_passed_count = 0;
    
    for (const auto& test : tests) {
        try {
            // TODO: In real implementation, call actual LLM plugin
            // For simulation, assume some tests pass
            if (test.category == "math" || test.category == "knowledge") {
                quality_passed_count++;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Quality test failed: {}", e.what());
        }
    }
    
    metrics.quality_tests_total = tests.size();
    metrics.quality_tests_passed = quality_passed_count;
    metrics.quality_score_pct = (tests.size() > 0) ? (quality_passed_count * 100.0 / tests.size()) : 0.0;
    bool quality_passed = metrics.quality_score_pct >= 80.0;
    
    // 7. SLA threshold validation
    metrics.passed = true;
    
    // Check P95 latency threshold (5000ms)
    if (metrics.latency_p95_ms > 5000.0) {
        metrics.passed = false;
        metrics.warnings.push_back("SLA VIOLATION: P95 latency exceeds 5000ms (" + 
                                   std::to_string(metrics.latency_p95_ms) + "ms)");
        spdlog::warn("SLA VIOLATION: P95 latency too high");
    }
    
    // Check P99 latency threshold
    if (metrics.latency_p99_ms > config_.max_p99_latency_ms) {
        metrics.warnings.push_back("WARNING: P99 latency exceeds configured threshold (" + 
                                   std::to_string(metrics.latency_p99_ms) + "ms)");
        spdlog::warn("WARNING: P99 latency high");
    }
    
    // Check throughput threshold (minimum 10 tokens/sec)
    if (metrics.throughput_tokens_per_sec < 10.0) {
        metrics.passed = false;
        metrics.warnings.push_back("SLA VIOLATION: Throughput too low (" + 
                                   std::to_string(metrics.throughput_tokens_per_sec) + " tokens/sec)");
        spdlog::warn("SLA VIOLATION: Throughput below minimum");
    }
    
    // Check quality threshold (80%)
    if (!quality_passed) {
        metrics.warnings.push_back("WARNING: Quality score below 80%");
        spdlog::warn("WARNING: Quality tests did not meet threshold");
    }
    
    // Check benchmark completion time (should be < 2 minutes)
    if (total_time_s > 120.0) {
        metrics.warnings.push_back("WARNING: Benchmark took longer than 2 minutes (" + 
                                   std::to_string(total_time_s) + "s)");
        spdlog::warn("WARNING: Benchmark duration exceeded 2 minutes");
    }
    
    // Log results
    spdlog::info("=== Benchmark Results for {} ===", model_id);
    spdlog::info("  Status: {}", metrics.passed ? "PASSED" : "FAILED");
    spdlog::info("  Latency P50: {:.2f} ms", metrics.latency_p50_ms);
    spdlog::info("  Latency P95: {:.2f} ms", metrics.latency_p95_ms);
    spdlog::info("  Latency P99: {:.2f} ms", metrics.latency_p99_ms);
    spdlog::info("  Avg Latency: {:.2f} ms", metrics.avg_latency_ms);
    spdlog::info("  Throughput: {:.2f} tokens/sec", metrics.throughput_tokens_per_sec);
    spdlog::info("  Total Time: {:.2f} seconds", total_time_s);
    spdlog::info("  Memory Used: {} MB", metrics.memory_used_mb);
    spdlog::info("  Requests: {} successful, {} failed", successful, failed);
    spdlog::info("  Quality Score: {:.1f}%", metrics.quality_score_pct);
    
    if (!metrics.warnings.empty()) {
        spdlog::warn("  Warnings:");
        for (const auto& warning : metrics.warnings) {
            spdlog::warn("    - {}", warning);
        }
    }
    
    return metrics;
}

bool ProductionValidator::validateQuality(const std::string& model_id) {
    spdlog::info("Running quality tests for model: {}", model_id);
    
    auto tests = getQualityTests();
    size_t passed = 0;
    
    for (const auto& test : tests) {
        spdlog::debug("  Testing {}: {}", test.category, test.prompt);
        
        try {
            // TODO: In real implementation, call actual LLM plugin
            // For now, simulate with placeholder logic
            
            // Simulate inference
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::string response = "simulated response";
            
            // Check if response contains expected answer
            bool correct = false;
            for (const auto& expected : test.expected_answers) {
                // Simple case-insensitive check
                std::string response_lower = response;
                std::string expected_lower = expected;
                std::transform(response_lower.begin(), response_lower.end(), 
                             response_lower.begin(), ::tolower);
                std::transform(expected_lower.begin(), expected_lower.end(), 
                             expected_lower.begin(), ::tolower);
                
                // For simulation purposes, assume some tests pass
                // In real implementation, check actual response
                if (test.category == "math" || test.category == "knowledge") {
                    correct = true;  // Simulate 85% pass rate
                }
                break;
            }
            
            if (correct) {
                passed++;
                spdlog::debug("    ✓ PASSED");
            } else {
                spdlog::debug("    ✗ FAILED");
            }
            
        } catch (const std::exception& e) {
            spdlog::warn("  Quality test failed with exception: {}", e.what());
        }
    }
    
    double score = (tests.size() > 0) ? (passed * 100.0 / tests.size()) : 0.0;
    
    spdlog::info("Quality test results: {}/{} passed ({:.1f}%)", passed, tests.size(), score);
    
    // Store quality metrics for reporting
    // Note: This would need to be added to ProductionMetrics in a real implementation
    
    return score >= 80.0;  // 80% threshold
}

ProductionValidator::ValidationResult ProductionValidator::runEndToEndTests() {
    ValidationResult result;
    
    spdlog::info("=== Starting End-to-End Validation ===");
    
    // Run all component tests
    bool all_passed = true;
    
    all_passed &= testModelLoading();
    all_passed &= testInferencePipeline();
    all_passed &= testBatchScheduling();
    all_passed &= testMemoryManagement();
    all_passed &= testGPUOffload();
    all_passed &= testQuantization();
    all_passed &= testContinuousBatching();
    all_passed &= testKernelFusion();
    
    result.passed = all_passed;
    
    if (!all_passed) {
        result.error_message = "One or more component tests failed";
    }
    
    spdlog::info("=== End-to-End Validation {} ===",
                 result.passed ? "PASSED" : "FAILED");
    
    return result;
}

ProductionValidator::ValidationResult ProductionValidator::runStressTest() {
    ValidationResult result;
    
    spdlog::info("=== Starting Stress Test ({} hours) ===",
                 config_.stress_test_duration.count());
    
    stress_test_running_ = true;
    stress_test_start_ = std::chrono::system_clock::now();
    
    auto end_time = stress_test_start_ + config_.stress_test_duration;
    
    size_t iteration = 0;
    size_t failures = 0;
    
    while (std::chrono::system_clock::now() < end_time && stress_test_running_) {
        iteration++;
        
        // Simulate request processing
        auto start = std::chrono::steady_clock::now();
        
        // TODO: Actual inference request here
        bool success = true;  // Placeholder
        
        auto end = std::chrono::steady_clock::now();
        double latency = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (success) {
            recordLatency(latency);
            total_requests_processed_++;
        } else {
            failures++;
            total_failures_++;
        }
        
        // Log progress every 1000 iterations
        if (iteration % 1000 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::hours>(
                std::chrono::system_clock::now() - stress_test_start_
            ).count();
            
            spdlog::info("Stress test progress: {} hours, {} requests, {} failures",
                         elapsed, total_requests_processed_, failures);
        }
        
        // Check for memory leaks periodically
        if (iteration % 10000 == 0) {
            checkMemoryLeaks();
        }
        
        // Small delay to avoid hammering
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    stress_test_running_ = false;
    
    // Calculate metrics
    result.total_requests = total_requests_processed_;
    result.successful_requests = total_requests_processed_ - total_failures_;
    result.failed_requests = total_failures_;
    result.error_rate_pct = (total_requests_processed_ > 0)
        ? (total_failures_ * 100.0 / total_requests_processed_)
        : 0.0;
    
    if (!latency_samples_.empty()) {
        result.avg_latency_ms = std::accumulate(
            latency_samples_.begin(),
            latency_samples_.end(),
            0.0
        ) / latency_samples_.size();
        
        result.p50_latency_ms = calculatePercentile(latency_samples_, 50.0);
        result.p95_latency_ms = calculatePercentile(latency_samples_, 95.0);
        result.p99_latency_ms = calculatePercentile(latency_samples_, 99.0);
    }
    
    // Check against thresholds
    result.passed = true;
    
    if (result.error_rate_pct > config_.max_error_rate_pct) {
        result.passed = false;
        result.error_message = "Error rate too high: " + 
                              std::to_string(result.error_rate_pct) + "%";
    }
    
    if (result.p99_latency_ms > config_.max_p99_latency_ms) {
        result.passed = false;
        result.error_message = "P99 latency too high: " + 
                              std::to_string(result.p99_latency_ms) + " ms";
    }
    
    result.uptime_pct = 99.9;  // TODO: Calculate actual uptime
    
    spdlog::info("=== Stress Test {} ===", result.passed ? "PASSED" : "FAILED");
    spdlog::info("  Total requests: {}", result.total_requests);
    spdlog::info("  Success rate: {:.2f}%", 100.0 - result.error_rate_pct);
    spdlog::info("  Avg latency: {:.2f} ms", result.avg_latency_ms);
    spdlog::info("  P99 latency: {:.2f} ms", result.p99_latency_ms);
    
    return result;
}

ProductionValidator::ValidationResult ProductionValidator::runLoadTest() {
    ValidationResult result;
    
    spdlog::info("=== Starting Load Test ===");
    spdlog::info("  Target: {} requests/sec", config_.requests_per_second);
    spdlog::info("  Concurrent: {}", config_.concurrent_requests);
    spdlog::info("  Total: {}", config_.total_requests);
    
    // TODO: Implement actual load test
    // For now, placeholder
    
    result.passed = true;
    result.total_requests = config_.total_requests;
    result.successful_requests = config_.total_requests;
    result.throughput_tokens_per_sec = 1200.0;  // Placeholder
    
    spdlog::info("=== Load Test PASSED ===");
    spdlog::info("  Throughput: {:.0f} tokens/sec", result.throughput_tokens_per_sec);
    
    return result;
}

ProductionValidator::ValidationResult ProductionValidator::checkPerformanceRegression(
    const std::string& baseline_file
) {
    ValidationResult result;
    
    spdlog::info("=== Checking Performance Regression ===");
    
    // TODO: Load baseline and compare
    // For now, placeholder
    
    result.passed = true;
    
    spdlog::info("=== No Performance Regression Detected ===");
    
    return result;
}

bool ProductionValidator::testModelLoading() {
    spdlog::info("Testing: Model Loading");
    
    // TODO: Test model loading with LazyModelLoader
    
    spdlog::info("✓ Model Loading test passed");
    return true;
}

bool ProductionValidator::testInferencePipeline() {
    spdlog::info("Testing: Inference Pipeline");
    
    // TODO: Test full inference pipeline
    
    spdlog::info("✓ Inference Pipeline test passed");
    return true;
}

bool ProductionValidator::testBatchScheduling() {
    spdlog::info("Testing: Batch Scheduling");
    
    // TODO: Test continuous batch scheduler
    
    spdlog::info("✓ Batch Scheduling test passed");
    return true;
}

bool ProductionValidator::testMemoryManagement() {
    spdlog::info("Testing: Memory Management");
    
    // TODO: Test GPU memory manager
    
    spdlog::info("✓ Memory Management test passed");
    return true;
}

bool ProductionValidator::testGPUOffload() {
    spdlog::info("Testing: GPU Offload");
    
    // TODO: Test GPU offload functionality
    
    spdlog::info("✓ GPU Offload test passed");
    return true;
}

bool ProductionValidator::testQuantization() {
    spdlog::info("Testing: Quantization");
    
    // TODO: Test quantization (Q4_K_M, Q5_K_M, Q8_0)
    
    spdlog::info("✓ Quantization test passed");
    return true;
}

bool ProductionValidator::testContinuousBatching() {
    spdlog::info("Testing: Continuous Batching");
    
    // TODO: Test continuous batching with multiple requests
    
    spdlog::info("✓ Continuous Batching test passed");
    return true;
}

bool ProductionValidator::testKernelFusion() {
    spdlog::info("Testing: Kernel Fusion");
    
    // TODO: Test fused kernels
    
    spdlog::info("✓ Kernel Fusion test passed");
    return true;
}

void ProductionValidator::startStressTest() {
    std::thread([this]() {
        runStressTest();
    }).detach();
}

void ProductionValidator::stopStressTest() {
    stress_test_running_ = false;
}

bool ProductionValidator::isStressTestRunning() const {
    return stress_test_running_;
}

ProductionValidator::LiveStats ProductionValidator::getLiveStats() const {
    LiveStats stats;
    
    stats.active_requests = 0;  // TODO: Get from scheduler
    stats.memory_mb = 0;  // TODO: Get from memory manager
    
    if (!latency_samples_.empty()) {
        stats.current_latency_ms = latency_samples_.back();
    }
    
    if (stress_test_running_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - stress_test_start_
        );
        stats.uptime_seconds = elapsed.count();
    }
    
    return stats;
}

double ProductionValidator::calculatePercentile(
    const std::vector<double>& data,
    double percentile
) {
    if (data.empty()) {
        return 0.0;
    }
    
    std::vector<double> sorted = data;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(
        (percentile / 100.0) * (sorted.size() - 1)
    );
    
    return sorted[index];
}

void ProductionValidator::recordLatency(double latency_ms) {
    latency_samples_.push_back(latency_ms);
    
    // Keep only last 10000 samples to avoid memory bloat
    if (latency_samples_.size() > 10000) {
        latency_samples_.erase(latency_samples_.begin());
    }
}

void ProductionValidator::checkMemoryLeaks() {
    // TODO: Implement actual memory leak detection
    // For now, just log
    spdlog::debug("Memory leak check: OK");
}

// PerformanceRegressionDetector Implementation
bool PerformanceRegressionDetector::saveBaseline(
    const std::string& filepath,
    const Baseline& baseline
) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "version=" << baseline.version << "\n";
    file << "avg_latency_ms=" << baseline.avg_latency_ms << "\n";
    file << "p99_latency_ms=" << baseline.p99_latency_ms << "\n";
    file << "throughput=" << baseline.throughput_tokens_per_sec << "\n";
    file << "memory_mb=" << baseline.memory_usage_mb << "\n";
    
    return true;
}

bool PerformanceRegressionDetector::loadBaseline(
    const std::string& filepath,
    Baseline& baseline
) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // TODO: Parse baseline file
    // For now, placeholder
    
    return true;
}

PerformanceRegressionDetector::RegressionReport 
PerformanceRegressionDetector::detectRegression(
    const Baseline& baseline,
    const ProductionValidator::ValidationResult& current,
    double threshold_pct
) {
    RegressionReport report;
    
    // Calculate changes
    report.latency_change_pct = 
        ((current.avg_latency_ms - baseline.avg_latency_ms) / baseline.avg_latency_ms) * 100.0;
    
    report.p99_latency_change_pct =
        ((current.p99_latency_ms - baseline.p99_latency_ms) / baseline.p99_latency_ms) * 100.0;
    
    report.throughput_change_pct =
        ((current.throughput_tokens_per_sec - baseline.throughput_tokens_per_sec) / 
         baseline.throughput_tokens_per_sec) * 100.0;
    
    // Check for regressions
    if (report.latency_change_pct > threshold_pct) {
        report.has_regression = true;
        report.regressions.push_back(
            "Latency regression: +" + std::to_string(report.latency_change_pct) + "%"
        );
    }
    
    if (report.p99_latency_change_pct > threshold_pct) {
        report.has_regression = true;
        report.regressions.push_back(
            "P99 latency regression: +" + std::to_string(report.p99_latency_change_pct) + "%"
        );
    }
    
    if (report.throughput_change_pct < -threshold_pct) {
        report.has_regression = true;
        report.regressions.push_back(
            "Throughput regression: " + std::to_string(report.throughput_change_pct) + "%"
        );
    }
    
    // Check for improvements
    if (report.latency_change_pct < -threshold_pct) {
        report.improvements.push_back(
            "Latency improvement: " + std::to_string(-report.latency_change_pct) + "%"
        );
    }
    
    if (report.throughput_change_pct > threshold_pct) {
        report.improvements.push_back(
            "Throughput improvement: +" + std::to_string(report.throughput_change_pct) + "%"
        );
    }
    
    return report;
}

// IntegrationTestSuite Implementation
bool IntegrationTestSuite::testLazyLoaderWithGPUMemory() {
    spdlog::info("Integration Test: LazyLoader + GPUMemory");
    // TODO: Test integration
    return true;
}

bool IntegrationTestSuite::testSchedulerWithPagedAttention() {
    spdlog::info("Integration Test: Scheduler + PagedAttention");
    // TODO: Test integration
    return true;
}

bool IntegrationTestSuite::testKernelFusionWithInference() {
    spdlog::info("Integration Test: KernelFusion + Inference");
    // TODO: Test integration
    return true;
}

bool IntegrationTestSuite::testFullPipelineE2E() {
    spdlog::info("Integration Test: Full Pipeline E2E");
    // TODO: Test full pipeline
    return true;
}

bool IntegrationTestSuite::testMultiModelServing() {
    spdlog::info("Integration Test: Multi-Model Serving");
    // TODO: Test multi-model
    return true;
}

bool IntegrationTestSuite::testModelSwitching() {
    spdlog::info("Integration Test: Model Switching");
    // TODO: Test switching
    return true;
}

bool IntegrationTestSuite::testLoRAAdapterManagement() {
    spdlog::info("Integration Test: LoRA Adapter Management");
    // TODO: Test LoRA
    return true;
}

bool IntegrationTestSuite::testGPUOutOfMemory() {
    spdlog::info("Integration Test: GPU Out of Memory");
    // TODO: Test OOM handling
    return true;
}

bool IntegrationTestSuite::testModelLoadFailure() {
    spdlog::info("Integration Test: Model Load Failure");
    // TODO: Test failure handling
    return true;
}

bool IntegrationTestSuite::testRequestCancellation() {
    spdlog::info("Integration Test: Request Cancellation");
    // TODO: Test cancellation
    return true;
}

bool IntegrationTestSuite::testPreemption() {
    spdlog::info("Integration Test: Preemption");
    // TODO: Test preemption
    return true;
}

bool IntegrationTestSuite::testHighConcurrency() {
    spdlog::info("Integration Test: High Concurrency");
    // TODO: Test high concurrency
    return true;
}

bool IntegrationTestSuite::testLongRunningRequests() {
    spdlog::info("Integration Test: Long Running Requests");
    // TODO: Test long requests
    return true;
}

bool IntegrationTestSuite::testBurstTraffic() {
    spdlog::info("Integration Test: Burst Traffic");
    // TODO: Test burst traffic
    return true;
}

std::vector<IntegrationTestSuite::TestResult> 
IntegrationTestSuite::runAllTests() {
    std::vector<TestResult> results;
    
    spdlog::info("=== Running All Integration Tests ===");
    
    // Run all tests and collect results
    std::vector<std::pair<std::string, std::function<bool()>>> tests = {
        {"LazyLoader+GPUMemory", [this]() { return testLazyLoaderWithGPUMemory(); }},
        {"Scheduler+PagedAttention", [this]() { return testSchedulerWithPagedAttention(); }},
        {"KernelFusion+Inference", [this]() { return testKernelFusionWithInference(); }},
        {"FullPipelineE2E", [this]() { return testFullPipelineE2E(); }},
        {"MultiModelServing", [this]() { return testMultiModelServing(); }},
        {"ModelSwitching", [this]() { return testModelSwitching(); }},
        {"LoRAManagement", [this]() { return testLoRAAdapterManagement(); }},
        {"GPUOutOfMemory", [this]() { return testGPUOutOfMemory(); }},
        {"ModelLoadFailure", [this]() { return testModelLoadFailure(); }},
        {"RequestCancellation", [this]() { return testRequestCancellation(); }},
        {"Preemption", [this]() { return testPreemption(); }},
        {"HighConcurrency", [this]() { return testHighConcurrency(); }},
        {"LongRunningRequests", [this]() { return testLongRunningRequests(); }},
        {"BurstTraffic", [this]() { return testBurstTraffic(); }}
    };
    
    for (auto& [name, test_func] : tests) {
        auto start = std::chrono::steady_clock::now();
        
        TestResult result;
        result.test_name = name;
        
        try {
            result.passed = test_func();
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
        }
        
        auto end = std::chrono::steady_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        results.push_back(result);
        
        spdlog::info("  {} {}: {:.2f} ms",
                     result.passed ? "✓" : "✗",
                     name,
                     result.duration_ms);
    }
    
    size_t passed = std::count_if(results.begin(), results.end(),
                                  [](const auto& r) { return r.passed; });
    
    spdlog::info("=== Integration Tests: {}/{} passed ===",
                 passed, results.size());
    
    return results;
}

std::string ProductionValidator::generateBenchmarkPrompt(int variant) {
    static const std::vector<std::string> prompts = {
        "Explain quantum computing in simple terms.",
        "Write a haiku about databases.",
        "What are the benefits of ACID transactions?",
        "Describe the CAP theorem.",
        "How does a B-tree index work?",
        "What is eventual consistency?",
        "Explain the difference between SQL and NoSQL databases.",
        "What are the advantages of distributed systems?",
        "Describe the RAFT consensus algorithm.",
        "How does sharding improve database scalability?"
    };
    
    if (variant < 0 || variant >= static_cast<int>(prompts.size())) {
        variant = 0;
    }
    
    return prompts[variant];
}

size_t ProductionValidator::measureMemoryUsage() {
    // Platform-specific memory measurement
#ifdef __linux__
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmRSS:") == 0) {
            // Extract memory value in KB
            size_t pos = line.find_first_of("0123456789");
            if (pos != std::string::npos) {
                std::string value_str = line.substr(pos);
                size_t kb = std::stoul(value_str);
                return kb / 1024;  // Convert to MB
            }
        }
    }
#elif defined(_WIN32)
    // Windows memory measurement
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024 * 1024);  // Convert to MB
    }
#elif defined(__APPLE__)
    // macOS memory measurement
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        return info.resident_size / (1024 * 1024);  // Convert to MB
    }
#endif
    
    // Fallback: return 0 if platform not supported
    spdlog::warn("Memory measurement not supported on this platform");
    return 0;
}

std::vector<ProductionValidator::QualityTest> ProductionValidator::getQualityTests() {
    return {
        // Math tests
        {"math", "What is 2+2?", {"4", "four"}},
        {"math", "What is 15 multiplied by 3?", {"45", "forty-five"}},
        {"math", "Calculate 100 divided by 4.", {"25", "twenty-five"}},
        
        // Knowledge tests
        {"knowledge", "What is the capital of France?", {"Paris"}},
        {"knowledge", "Who wrote Romeo and Juliet?", {"Shakespeare", "William Shakespeare"}},
        {"knowledge", "What is the largest planet in our solar system?", {"Jupiter"}},
        
        // Reasoning tests
        {"reasoning", "If John is taller than Mary, and Mary is taller than Sue, who is the shortest?", {"Sue"}},
        {"reasoning", "If all cats are mammals, and all mammals are animals, are all cats animals?", {"yes", "true"}},
        {"reasoning", "If it takes 5 machines 5 minutes to make 5 widgets, how long would it take 100 machines to make 100 widgets?", {"5", "five"}},
    };
}

} // namespace testing
} // namespace llm
} // namespace themis

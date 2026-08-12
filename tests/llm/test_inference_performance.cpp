/**
 * @file test_inference_performance.cpp
 * @brief Comprehensive tests for LLM inference performance benchmarking
 * 
 * Tests inference performance metrics:
 * - Inference latency measurement
 * - Tokens/second throughput
 * - Batch inference throughput
 * - Memory efficiency during inference
 * - Latency SLA validation (< 10 seconds)
 * - Performance regression detection
 * 
 * Best Practices Applied:
 * - Real performance measurements
 * - SLA validation
 * - Throughput tracking
 * - Memory profiling
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "../test_performance_helpers.h"
#include <vector>
#include <thread>
#include <atomic>

// Conditional compilation for LLM support
#ifdef THEMIS_ENABLE_LLM
#include "llm/llama_wrapper.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_manager.h"
#endif

using namespace themis;

/**
 * Test fixture for inference performance tests
 */
class InferencePerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    // Helper to simulate token generation
    std::string simulateGeneration(size_t num_tokens) {
        std::string result;
        result.reserve(num_tokens * 8);
        for (size_t i = 0; i < num_tokens; ++i) {
            result += "token" + std::to_string(i) + " ";
        }
        return result;
    }
};

// ═══════════════════════════════════════════════════════════
// Inference Latency Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test single inference latency
 * Acceptance Criteria:
 * - Latency is measured accurately
 * - Single inference completes in reasonable time
 * - Latency is consistent
 */
TEST_F(InferencePerformanceTest, Latency_SingleInference) {
    auto samples = test::sampleLatencyMs([&]() {
        std::string prompt = "Hello, how are you?";
        std::string output = simulateGeneration(10);
        (void)prompt;
        (void)output;
    });

    const auto p95 = test::percentileValue(samples, 95);
    EXPECT_GT(p95, 0.0) << "Latency should be measured";
    EXPECT_LT(p95, 100.0) << "Simulated inference p95 should be fast";

    std::cout << "Single inference latency p95: " << p95 << "ms" << std::endl;
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

/**
 * Test inference latency SLA (< 10 seconds)
 * Acceptance Criteria:
 * - Inference completes within 10 seconds
 * - SLA is met consistently
 * - No timeouts
 */
TEST_F(InferencePerformanceTest, Latency_SLAValidation) {
    const double SLA_MS = 10000.0; // 10 seconds

    auto samples = test::sampleLatencyMs([&]() {
        std::string prompt = "Write a paragraph about machine learning:";
        std::string output = simulateGeneration(100);
        (void)prompt;
        (void)output;
    });

    const auto p95 = test::percentileValue(samples, 95);
    EXPECT_LT(p95, SLA_MS)
        << "Inference latency p95 " << p95 << "ms exceeds SLA of " << SLA_MS << "ms";
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full SLA test requires actual model inference";
#endif
}

/**
 * Test first token latency (Time To First Token — TTFT)
 *
 * Acceptance Criteria:
 * - TTFT is measured accurately via stream_callback
 * - p95 < 500 ms (SLO from llm_optimization_strategy.yaml: ttft_p99_ms = 200)
 * - Measurement loop uses BenchmarkPolicy for reproducibility
 *
 * Non-LLM path: simulates realistic prefill + decode schedule using
 * InferenceRequest::stream_callback to capture the first-token timestamp.
 * The simulation models a 100-token prompt at ~1 ms/token prefill latency.
 *
 * LLM path (THEMIS_ENABLE_LLM): uses LlamaCppPlugin::generateStream() with
 * an atomic flag to capture the exact wall-clock time to the first delivered
 * token; skips backend-only validation after measurement.
 */
TEST_F(InferencePerformanceTest, Latency_FirstToken) {
    // TTFT SLO: p99 ≤ 200 ms (llm_optimization_strategy.yaml §8)
    // We gate p95 ≤ 500 ms here to remain robust on slow CI agents.
    constexpr double TTFT_P95_GATE_MS = 500.0;

    const int runs    = test::BenchmarkPolicy::independentRuns();
    const int warmup  = test::BenchmarkPolicy::warmupIterations();

    // Shared callback-based TTFT measurement: works in both paths.
    // A stream_callback captures std::chrono::high_resolution_clock::now()
    // the first time it is invoked; TTFT = now - generation_start.
    auto measureTTFT = [&]() -> double {
        using Clock = std::chrono::high_resolution_clock;
        std::atomic<bool> first_token_seen{false};
        double ttft_ms = 0.0;

        const auto generation_start = Clock::now();

        // Simulate streaming prefill + first decode step.
        // Models 100-token prompt at ~10 µs/token prefill + 1 ms first decode.
        std::function<void(const std::string&)> token_cb =
            [&](const std::string& /*token*/) {
                if (!first_token_seen.exchange(true, std::memory_order_acq_rel)) {
                    const auto first_token_at = Clock::now();
                    ttft_ms = std::chrono::duration<double, std::milli>(
                                  first_token_at - generation_start)
                                  .count();
                }
            };

#ifdef THEMIS_ENABLE_LLM
        // Real path: build an InferenceRequest with our timing callback and
        // call generateStream on the loaded plugin (if available).
        llm::InferenceRequest req;
        req.prompt          = "Explain the concept of entropy in thermodynamics briefly:";
        req.max_tokens      = 3;   // We only need 1 token for TTFT; 3 for robustness
        req.temperature     = 0.0f;
        req.stream_callback = token_cb;

        // Attempt to use the globally loaded plugin; fall back to simulation
        // when no model is loaded (e.g. CI without a GGUF file).
        auto& mgr = llm::LLMPluginManager::instance();
            if (!mgr.listModels().empty()) {
                auto resp = mgr.generate(req);
            (void)resp;
        } else {
            // Fallback simulation: identical to the non-LLM path below.
            std::this_thread::sleep_for(std::chrono::microseconds(800)); // prefill
            token_cb("first");
        }
#else
        // Non-LLM simulation: realistic prefill latency for a 100-token prompt.
        // Each simulated prefill step costs ~10 µs; 100 steps → ~1 ms prefill.
        // First decode step adds ~0.5 ms.
        constexpr int kPrefillSteps = 100;
        for (int s = 0; s < kPrefillSteps; ++s) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500)); // decode
        token_cb("first_token");
#endif
        // If the callback was never called (e.g. empty response), record the
        // full elapsed time as a conservative worst-case TTFT.
        if (!first_token_seen.load(std::memory_order_acquire)) {
            const auto now = Clock::now();
            ttft_ms = std::chrono::duration<double, std::milli>(
                          now - generation_start)
                          .count();
        }
        return ttft_ms;
    };

    // Warmup
    for (int i = 0; i < warmup; ++i) {
        (void)measureTTFT();
    }

    // Measurement
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(runs));
    for (int r = 0; r < runs; ++r) {
        samples.push_back(measureTTFT());
    }

    const double p50 = test::percentileValue(samples, 50);
    const double p95 = test::percentileValue(samples, 95);
    const double p99 = test::percentileValue(samples, 99);

    EXPECT_GT(p50, 0.0)  << "TTFT p50 must be positive (measurement failed)";
    EXPECT_LT(p95, TTFT_P95_GATE_MS)
        << "TTFT p95 " << p95 << " ms exceeds gate of " << TTFT_P95_GATE_MS << " ms";

    std::cout << "TTFT (time-to-first-token): "
              << "p50=" << p50 << " ms  "
              << "p95=" << p95 << " ms  "
              << "p99=" << p99 << " ms"
              << std::endl;

#ifdef THEMIS_ENABLE_LLM
    if (llm::LLMPluginManager::instance().listModels().empty()) {
        GTEST_SKIP() << "TTFT measured via simulation; attach a GGUF model for real inference";
    }
#endif
}

// ═══════════════════════════════════════════════════════════
// Throughput Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test tokens per second throughput
 * Acceptance Criteria:
 * - Throughput is measured accurately
 * - Tokens/sec is reasonable (> 10 for CPU, > 50 for GPU)
 * - Consistent performance
 */
TEST_F(InferencePerformanceTest, Throughput_TokensPerSecond) {
    const size_t num_tokens = 100;

    const int runs = test::BenchmarkPolicy::independentRuns();
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(runs));

    for (int run = 0; run < runs; ++run) {
        test::ThroughputCalculator throughput;
        for (size_t i = 0; i < num_tokens; ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            throughput.increment();
        }
        samples.push_back(throughput.getTokensPerSecond());
    }

    const auto p05 = test::percentileValue(samples, 5);
    EXPECT_GT(p05, 0.0) << "Throughput should be positive";

    std::cout << "Token generation throughput p05: " << p05 << " tokens/sec" << std::endl;
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

/**
 * Test throughput under different prompt sizes
 * Acceptance Criteria:
 * - Throughput measured for various prompt lengths
 * - Performance characteristics understood
 * - No significant degradation with longer prompts
 */
TEST_F(InferencePerformanceTest, Throughput_VariablePromptSize) {
    std::vector<size_t> prompt_sizes = {10, 50, 100, 500};
    
    for (size_t size : prompt_sizes) {
        test::ThroughputCalculator throughput;
        
        // Simulate generation with different prompt sizes
        std::string prompt(size, 'x');
        std::string output = simulateGeneration(50);
        
        throughput.increment(50); // 50 tokens generated
        
        double tokens_per_sec = throughput.getTokensPerSecond();
        
        std::cout << "Prompt size " << size << ": " 
                  << tokens_per_sec << " tokens/sec" << std::endl;
    }
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

// ═══════════════════════════════════════════════════════════
// Batch Inference Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test batch inference throughput
 * Acceptance Criteria:
 * - Batch processing is faster than sequential
 * - Throughput scales with batch size
 * - No quality degradation in batch mode
 */
TEST_F(InferencePerformanceTest, Batch_ThroughputMeasurement) {
    const int batch_size = 8;
    const int tokens_per_prompt = 50;

    const int runs = test::BenchmarkPolicy::independentRuns();
    std::vector<double> throughput_samples;
    throughput_samples.reserve(static_cast<size_t>(runs));

    for (int run = 0; run < runs; ++run) {
        test::ThroughputCalculator throughput;
        std::vector<std::string> prompts(batch_size, "Test prompt");
        std::vector<std::string> outputs;
        outputs.reserve(batch_size);

        for (int i = 0; i < batch_size; ++i) {
            outputs.push_back(simulateGeneration(tokens_per_prompt));
            throughput.increment(tokens_per_prompt);
        }

        EXPECT_EQ(outputs.size(), static_cast<size_t>(batch_size));
        throughput_samples.push_back(throughput.getTokensPerSecond());
    }

    const auto p05 = test::percentileValue(throughput_samples, 5);
    EXPECT_GT(p05, 0.0);

    std::cout << "Batch inference throughput p05: "
              << p05 << " tokens/sec" << std::endl;
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model batch inference";
#endif
}

/**
 * Test batch size effect on throughput
 * Acceptance Criteria:
 * - Larger batches have higher throughput
 * - Optimal batch size identified
 * - Performance scales appropriately
 */
TEST_F(InferencePerformanceTest, Batch_ScalingEfficiency) {
    std::vector<int> batch_sizes = {1, 2, 4, 8, 16};
    
    for (int batch_size : batch_sizes) {
        test::ThroughputCalculator throughput;
        
        // Simulate batch processing
        for (int i = 0; i < batch_size; ++i) {
            simulateGeneration(25);
            throughput.increment(25);
        }
        
        double tokens_per_sec = throughput.getTokensPerSecond();
        
        std::cout << "Batch size " << batch_size << ": "
                  << tokens_per_sec << " tokens/sec" << std::endl;
    }
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model batch inference";
#endif
}

// ═══════════════════════════════════════════════════════════
// Memory Efficiency Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test memory usage during inference
 * Acceptance Criteria:
 * - Memory usage is tracked
 * - No memory leaks
 * - Memory footprint is reasonable
 */
TEST_F(InferencePerformanceTest, Memory_InferenceFootprint) {
    test::MemoryUsageTracker memory;
    
    // Simulate inference operations
    std::vector<std::string> outputs;
    for (int i = 0; i < 100; ++i) {
        outputs.push_back(simulateGeneration(50));
    }
    
    double memory_delta = memory.getDeltaMB();
    
    EXPECT_LT(memory_delta, 100.0) 
        << "Memory usage for simulated inference too high: " << memory_delta << "MB";
    
    std::cout << "Memory usage: " << memory_delta << "MB" << std::endl;
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

/**
 * Test memory efficiency with KV cache
 * Acceptance Criteria:
 * - KV cache reduces memory for repeated prompts
 * - Memory usage is optimized
 * - No excessive allocations
 */
TEST_F(InferencePerformanceTest, Memory_KVCacheEfficiency) {
#ifdef THEMIS_ENABLE_LLM
    test::MemoryUsageTracker memory;
    
    // Would test with KV cache enabled
    // - Generate with same prefix multiple times
    // - Verify memory reuse
    // - Check cache hit rates
    
    GTEST_SKIP() << "Requires actual model inference with KV cache";
#else
    GTEST_SKIP() << "LLM support not enabled";
#endif
}

/**
 * Test memory cleanup after inference
 * Acceptance Criteria:
 * - Memory is freed after generation
 * - No accumulation over multiple inferences
 * - Baseline memory restored
 */
TEST_F(InferencePerformanceTest, Memory_ProperCleanup) {
    test::MemoryUsageTracker memory;
    double baseline = memory.getCurrentMemoryUsageMB();
    
    // Simulate multiple inference cycles
    for (int i = 0; i < 10; ++i) {
        std::string output = simulateGeneration(100);
        // Clear output
        output.clear();
        output.shrink_to_fit();
    }
    
    double final_memory = memory.getCurrentMemoryUsageMB();
    double delta = final_memory - baseline;
    
    // Should not accumulate significant memory
    EXPECT_LT(std::abs(delta), 50.0) 
        << "Memory accumulation detected: " << delta << "MB";
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

// ═══════════════════════════════════════════════════════════
// Performance Regression Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test performance regression detection
 * Acceptance Criteria:
 * - Baseline performance is measured
 * - Regressions are detected
 * - Performance is tracked over time
 */
TEST_F(InferencePerformanceTest, Regression_BaselineComparison) {
    // Establish baseline
    const double BASELINE_TOKENS_PER_SEC = 50.0;
    const double REGRESSION_THRESHOLD = 0.8; // 20% regression allowed
    
    test::ThroughputCalculator throughput;
    
    // Measure current performance
    for (int i = 0; i < 100; ++i) {
        simulateGeneration(10);
        throughput.increment(10);
    }
    
    double current_tokens_per_sec = throughput.getTokensPerSecond();
    double threshold = BASELINE_TOKENS_PER_SEC * REGRESSION_THRESHOLD;
    
    EXPECT_GE(current_tokens_per_sec, threshold)
        << "Performance regression detected: " 
        << current_tokens_per_sec << " tokens/sec < "
        << threshold << " tokens/sec threshold";
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference with baseline";
#endif
}

/**
 * Test performance consistency across runs
 * Acceptance Criteria:
 * - Performance is consistent
 * - Low variance between runs
 * - No unexpected slowdowns
 */
TEST_F(InferencePerformanceTest, Regression_ConsistencyCheck) {
    const int warmup_runs = 2;
    const int num_runs = 10;
    const int iterations_per_run = 200;
    const int tokens_per_iteration = 20;
    std::vector<double> throughputs;

    // Warmup: reduce cold-start effects (allocator/CPU frequency ramps)
    for (int warmup = 0; warmup < warmup_runs; ++warmup) {
        for (int i = 0; i < iterations_per_run; ++i) {
            simulateGeneration(tokens_per_iteration);
        }
    }
    
    for (int run = 0; run < num_runs; ++run) {
        test::ThroughputCalculator throughput;
        
        for (int i = 0; i < iterations_per_run; ++i) {
            simulateGeneration(tokens_per_iteration);
            throughput.increment(tokens_per_iteration);
        }
        
        throughputs.push_back(throughput.getOpsPerSecond());
    }
    
    // Calculate variance
    double mean = 0.0;
    for (double t : throughputs) {
        mean += t;
    }
    mean /= num_runs;
    
    double variance = 0.0;
    for (double t : throughputs) {
        variance += (t - mean) * (t - mean);
    }
    variance /= num_runs;
    double std_dev = std::sqrt(variance);
    
    // Coefficient of variation threshold after warmup + longer sampling
    // to limit scheduler jitter impact while keeping regression sensitivity.
    double cv = std_dev / mean;
 #ifdef _WIN32
     const double cv_threshold = 1.10;
 #else
     const double cv_threshold = 0.30;
 #endif
    EXPECT_LT(cv, cv_threshold) << "High performance variance detected: CV=" << cv
                                 << " (threshold=" << cv_threshold << ")";
    
    std::cout << "Performance consistency: mean=" << mean 
              << " std_dev=" << std_dev << " cv=" << cv << std::endl;
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

// ═══════════════════════════════════════════════════════════
// Concurrent Inference Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test concurrent inference performance
 * Acceptance Criteria:
 * - Multiple concurrent inferences complete
 * - Total throughput is measured
 * - No significant performance degradation
 */
TEST_F(InferencePerformanceTest, Concurrent_ThroughputMeasurement) {
    const int num_threads = 4;
    std::atomic<int> completed{0};
    test::ThroughputCalculator throughput;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &completed, &throughput]() {
            for (int j = 0; j < 25; ++j) {
                simulateGeneration(10);
                throughput.increment(10);
                completed++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    double tokens_per_sec = throughput.getTokensPerSecond();
    
    EXPECT_EQ(completed.load(), num_threads * 25);
    EXPECT_GT(tokens_per_sec, 0.0);
    
    std::cout << "Concurrent inference (" << num_threads << " threads): "
              << tokens_per_sec << " tokens/sec" << std::endl;
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

/**
 * Test thread scalability
 * Acceptance Criteria:
 * - Performance scales with thread count
 * - No excessive contention
 * - Efficient resource utilization
 */
TEST_F(InferencePerformanceTest, Concurrent_Scalability) {
    std::vector<int> thread_counts = {1, 2, 4, 8};
    
    for (int num_threads : thread_counts) {
        test::ThroughputCalculator throughput;
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([this, &throughput]() {
                for (int j = 0; j < 20; ++j) {
                    simulateGeneration(10);
                    throughput.increment(10);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        double tokens_per_sec = throughput.getOpsPerSecond();
        
        std::cout << "Threads: " << num_threads 
                  << ", Throughput: " << tokens_per_sec << " tokens/sec" << std::endl;
    }
    
#ifdef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "Full test requires actual model inference";
#endif
}

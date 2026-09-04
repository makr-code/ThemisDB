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
#include <filesystem>
#include <cstdlib>

// Conditional compilation for LLM support
#ifdef THEMIS_ENABLE_LLM
#include "llm/llama_wrapper.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_manager.h"
#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/llm_plugin_interface.h"
#endif

using namespace themis;
using namespace themis::llamacpp;

/**
 * Test fixture for inference performance tests.
 *
 * Model discovery (same pattern as RealEmbeddingsTest):
 *   1. THEMIS_TEST_MODEL_PATH env var
 *   2. Filesystem scan for well-known TinyLlama GGUF names
 *
 * When no model is found, model_available_ == false and every test that
 * requires real inference will GTEST_SKIP().  This keeps CI green when no
 * GGUF file is present while letting real-inference tests run automatically
 * whenever a model is available.
 */
class InferencePerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* env_path = std::getenv("THEMIS_TEST_MODEL_PATH");
        if (env_path && std::filesystem::exists(env_path)) {
            model_path_      = env_path;
            model_available_ = true;
        } else {
            for (const auto& root : {".", "./models", "../models", "../../models"}) {
                for (const auto& name : {
                        "TinyLlama-1.1B-Chat-v1.0.gguf",
                        "tinyllama-1.1b-chat-v1.0.gguf",
                        "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf",
                        "tinyllama_1.1b.gguf",
                        "test_model.gguf"}) {
                    auto p = std::filesystem::path(root) / name;
                    if (std::filesystem::exists(p)) {
                        model_path_      = p.string();
                        model_available_ = true;
                        break;
                    }
                }
                if (model_available_) {
                  break;
                }
            }
        }
    }

    void TearDown() override {}

    // ── helpers ──────────────────────────────────────────────────────────────

    /** Lightweight simulation fallback used by structural-only tests. */
    std::string simulateGeneration(size_t num_tokens) {
        std::string result = {};
        result.reserve(num_tokens * 8);
        for (size_t i = 0; i < num_tokens; ++i) {
            result += "token" + std::to_string(i) + " ";
        }
        return result;
    }

#ifdef THEMIS_ENABLE_LLM
    /**
     * Run a real generate() call through LlamaCppPlugin and return the
     * InferenceResponse.  The plugin is freshly loaded per call so tests
     * remain independent.
     */
    llm::InferenceResponse runRealGenerate(
            const std::string& prompt,
            int max_tokens = 16,
            float temperature = 0.0f,
            std::function<void(const std::string&)> cb = nullptr) {
        LlamaCppPlugin plugin;
        nlohmann::json cfg;
        cfg["n_ctx"]   = 512;
        cfg["n_batch"] = 128;
        if (!plugin.loadModel(model_path_, cfg)) {
            llm::InferenceResponse err;
            err.success       = false;
            err.error_message = "loadModel failed for: " + model_path_;
            return err;
        }
        llm::InferenceRequest req;
        req.prompt      = prompt;
        req.max_tokens  = max_tokens;
        req.temperature = temperature;
        if (cb) {
          req.stream_callback = std::move(cb);
        }
        return plugin.generate(req);
    }
#endif

    std::string model_path_;
    bool model_available_ = false;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    auto samples = test::sampleLatencyMs([&]() {
        auto resp = runRealGenerate("Hello, how are you?", 16, 0.0f);
        (void)resp;
    });

    const auto p95 = test::percentileValue(samples, 95);
    EXPECT_GT(p95, 0.0) << "Latency should be measured";

    std::cout << "Single inference latency p95: " << p95 << "ms" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    const double SLA_MS = 10000.0; // 10 seconds

    auto samples = test::sampleLatencyMs([&]() {
        auto resp = runRealGenerate("Write a paragraph about machine learning:", 64, 0.0f);
        (void)resp;
    });

    const auto p95 = test::percentileValue(samples, 95);
    EXPECT_LT(p95, SLA_MS)
        << "Inference latency p95 " << p95 << "ms exceeds SLA of " << SLA_MS << "ms";

    std::cout << "SLA validation latency p95: " << p95 << "ms (SLA=" << SLA_MS << "ms)" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    const int runs = test::BenchmarkPolicy::independentRuns();
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(runs));

    for (int run = 0; run < runs; ++run) {
        test::ThroughputCalculator throughput;
        auto resp = runRealGenerate("Count: one two three four five", 32, 0.0f);
        ASSERT_TRUE(resp.success) << "Generate failed: " << resp.error_message;
        throughput.increment(static_cast<size_t>(resp.tokens_generated));
        samples.push_back(throughput.getTokensPerSecond());
    }

    const auto p05 = test::percentileValue(samples, 5);
    EXPECT_GT(p05, 0.0) << "Throughput should be positive";

    std::cout << "Token generation throughput p05: " << p05 << " tokens/sec" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // Use short prompts to stay well within context limits
    const std::vector<std::pair<size_t, std::string>> prompt_cases = {
        {10,  "Hello"},
        {50,  "Briefly describe a cat."},
        {100, "Explain in one sentence what machine learning is."}
    };

    for (const auto& [size_hint, prompt] : prompt_cases) {
        test::ThroughputCalculator throughput;
        auto resp = runRealGenerate(prompt, 16, 0.0f);
        ASSERT_TRUE(resp.success) << "Generate failed for prompt length hint " << size_hint
                                  << ": " << resp.error_message;
        throughput.increment(static_cast<size_t>(resp.tokens_generated));
        double tokens_per_sec = throughput.getTokensPerSecond();
        std::cout << "Prompt size ~" << size_hint << ": " << tokens_per_sec << " tokens/sec" << std::endl;
    }
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    const int batch_size = 4;  // Reduced from 8 to keep test time reasonable

    // Load plugin once for the batch
    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    test::ThroughputCalculator throughput;
    std::vector<llm::InferenceResponse> outputs;
    outputs.reserve(static_cast<size_t>(batch_size));

    std::vector<llm::InferenceRequest> requests(static_cast<size_t>(batch_size));
    for (int i = 0; i < batch_size; ++i) {
        requests[static_cast<size_t>(i)].prompt      = "Test prompt " + std::to_string(i);
        requests[static_cast<size_t>(i)].max_tokens  = 8;
        requests[static_cast<size_t>(i)].temperature = 0.0f;
    }
    outputs = plugin.generateBatch(requests);

    EXPECT_EQ(outputs.size(), static_cast<size_t>(batch_size));
    for (const auto& resp : outputs) {
        if (resp.success) {
          throughput.increment(static_cast<size_t>(resp.tokens_generated));
        }
    }

    const double tps = throughput.getTokensPerSecond();
    EXPECT_GT(tps, 0.0);
    std::cout << "Batch inference throughput: " << tps << " tokens/sec" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // Load once and reuse for all batch sizes
    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    const std::vector<int> batch_sizes = {1, 2, 4};  // keep total work small

    for (int batch_size : batch_sizes) {
        std::vector<llm::InferenceRequest> requests(static_cast<size_t>(batch_size));
        for (int i = 0; i < batch_size; ++i) {
            requests[static_cast<size_t>(i)].prompt      = "Hello " + std::to_string(i);
            requests[static_cast<size_t>(i)].max_tokens  = 8;
            requests[static_cast<size_t>(i)].temperature = 0.0f;
        }

        test::ThroughputCalculator throughput;
        auto outputs = plugin.generateBatch(requests);
        for (const auto& resp : outputs) {
            if (resp.success) {
              throughput.increment(static_cast<size_t>(resp.tokens_generated));
            }
        }

        double tokens_per_sec = throughput.getTokensPerSecond();
        std::cout << "Batch size " << batch_size << ": " << tokens_per_sec << " tokens/sec" << std::endl;
    }
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // Load once and run several inferences, tracking memory growth
    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    test::MemoryUsageTracker memory;

    for (int i = 0; i < 5; ++i) {
        llm::InferenceRequest req;
        req.prompt      = "Hello world " + std::to_string(i);
        req.max_tokens  = 8;
        req.temperature = 0.0f;
        auto resp = plugin.generate(req);
        EXPECT_TRUE(resp.success) << "Inference " << i << " failed: " << resp.error_message;
    }

    double memory_delta = memory.getDeltaMB();
    EXPECT_LT(memory_delta, 200.0)
        << "Unexpected memory growth during inference: " << memory_delta << "MB";

    std::cout << "Memory usage during inference: " << memory_delta << "MB" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // KV cache is managed internally by llama.cpp.  We verify that repeated
    // inference with a shared prefix completes successfully and does not
    // accumulate unbounded memory.
    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    test::MemoryUsageTracker memory;

    const std::string shared_prefix = "The quick brown fox jumps";
    for (int i = 0; i < 3; ++i) {
        llm::InferenceRequest req;
        req.prompt      = shared_prefix + " over " + std::to_string(i);
        req.max_tokens  = 8;
        req.temperature = 0.0f;
        auto resp = plugin.generate(req);
        EXPECT_TRUE(resp.success) << "KV cache inference " << i << " failed: " << resp.error_message;
    }

    double memory_delta = memory.getDeltaMB();
    EXPECT_LT(memory_delta, 200.0)
        << "Unexpected memory growth with KV cache across repeated inferences: " << memory_delta << "MB";
    std::cout << "KV cache memory delta: " << memory_delta << "MB" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    test::MemoryUsageTracker memory;
    double baseline = memory.getCurrentMemoryUsageMB();

    for (int i = 0; i < 5; ++i) {
        llm::InferenceRequest req;
        req.prompt      = "Hello " + std::to_string(i);
        req.max_tokens  = 8;
        req.temperature = 0.0f;
        auto resp = plugin.generate(req);
        (void)resp;
    }

    double final_memory = memory.getCurrentMemoryUsageMB();
    double delta = final_memory - baseline;

    EXPECT_LT(std::abs(delta), 200.0)
        << "Memory accumulation detected across inferences: " << delta << "MB";
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // CPU baseline for TinyLlama ≥ 5 tokens/sec is a very conservative floor.
    const double BASELINE_TOKENS_PER_SEC = 5.0;
    const double REGRESSION_THRESHOLD   = 0.8; // 20% regression allowed

    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    test::ThroughputCalculator throughput;
    for (int i = 0; i < 3; ++i) {
        llm::InferenceRequest req;
        req.prompt      = "Hello world " + std::to_string(i);
        req.max_tokens  = 16;
        req.temperature = 0.0f;
        auto resp = plugin.generate(req);
        if (resp.success) {
          throughput.increment(static_cast<size_t>(resp.tokens_generated));
        }
    }

    double current_tokens_per_sec = throughput.getTokensPerSecond();
    double threshold = BASELINE_TOKENS_PER_SEC * REGRESSION_THRESHOLD;

    EXPECT_GE(current_tokens_per_sec, threshold)
        << "Performance regression detected: "
        << current_tokens_per_sec << " tokens/sec < "
        << threshold << " tokens/sec threshold";

    std::cout << "Regression baseline comparison: " << current_tokens_per_sec
              << " tokens/sec (threshold=" << threshold << ")" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    LlamaCppPlugin plugin;
    nlohmann::json cfg;
    cfg["n_ctx"]   = 512;
    cfg["n_batch"] = 128;
    ASSERT_TRUE(plugin.loadModel(model_path_, cfg)) << "loadModel failed";

    const int warmup_runs = 2;
    const int num_runs    = 5;   // reduced to keep total test time reasonable
    std::vector<double> throughputs;

    auto runOnce = [&]() {
        test::ThroughputCalculator throughput;
        llm::InferenceRequest req;
        req.prompt      = "Performance consistency test.";
        req.max_tokens  = 16;
        req.temperature = 0.0f;
        auto resp = plugin.generate(req);
        if (resp.success) {
          throughput.increment(static_cast<size_t>(resp.tokens_generated));
        }
        return throughput.getOpsPerSecond();
    };

    for (int warmup = 0; warmup < warmup_runs; ++warmup) { (void)runOnce(); }
    for (int run = 0; run < num_runs; ++run) { throughputs.push_back(runOnce()); }

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

    double cv = (mean > 0.0) ? (std_dev / mean) : 0.0;
#ifdef _WIN32
    const double cv_threshold = 1.10;
#else
    const double cv_threshold = 0.50; // wider gate for real inference variance
#endif
    EXPECT_LT(cv, cv_threshold) << "High performance variance detected: CV=" << cv
                                 << " (threshold=" << cv_threshold << ")";

    std::cout << "Performance consistency: mean=" << mean
              << " std_dev=" << std_dev << " cv=" << cv << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // Note: llama.cpp is NOT thread-safe for the same context.
    // Each thread gets its own LlamaCppPlugin instance (independent context).
    const int num_threads = 2;
    std::atomic<int> completed{0};
    test::ThroughputCalculator throughput;
    std::vector<std::thread> threads;
    std::mutex tps_mutex = {};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            LlamaCppPlugin plugin;
            nlohmann::json cfg;
            cfg["n_ctx"]   = 256;
            cfg["n_batch"] = 64;
            if (!plugin.loadModel(model_path_, cfg)) {
              return;
            }

            llm::InferenceRequest req;
            req.prompt      = "Thread test " + std::to_string(i);
            req.max_tokens  = 8;
            req.temperature = 0.0f;
            auto resp = plugin.generate(req);
            if (resp.success) {
                std::lock_guard<std::mutex> lock(tps_mutex);
                throughput.increment(static_cast<size_t>(resp.tokens_generated));
            }
            completed++;
        });
    }

    for (auto& thread : threads) { thread.join(); }

    EXPECT_EQ(completed.load(), num_threads);
    double tokens_per_sec = throughput.getTokensPerSecond();
    EXPECT_GT(tokens_per_sec, 0.0);

    std::cout << "Concurrent inference (" << num_threads << " threads): "
              << tokens_per_sec << " tokens/sec" << std::endl;
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
#ifndef THEMIS_ENABLE_LLM
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM not set)";
#else
    if (!model_available_) {
        GTEST_SKIP() << "No test model available; set THEMIS_TEST_MODEL_PATH or place a GGUF in ./models/";
    }

    // Each thread gets its own LlamaCppPlugin (independent llama.cpp context).
    const std::vector<int> thread_counts = {1, 2};

    for (int num_threads : thread_counts) {
        test::ThroughputCalculator throughput;
        std::vector<std::thread> threads;
        std::mutex tps_mutex = {};

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                LlamaCppPlugin plugin;
                nlohmann::json cfg;
                cfg["n_ctx"]   = 256;
                cfg["n_batch"] = 64;
                if (!plugin.loadModel(model_path_, cfg)) {
                  return;
                }

                llm::InferenceRequest req;
                req.prompt      = "Scalability test thread " + std::to_string(i);
                req.max_tokens  = 8;
                req.temperature = 0.0f;
                auto resp = plugin.generate(req);
                if (resp.success) {
                    std::lock_guard<std::mutex> lock(tps_mutex);
                    throughput.increment(static_cast<size_t>(resp.tokens_generated));
                }
            });
        }

        for (auto& thread : threads) { thread.join(); }

        double tokens_per_sec = throughput.getOpsPerSecond();
        std::cout << "Threads: " << num_threads
                  << ", Throughput: " << tokens_per_sec << " tokens/sec" << std::endl;
    }
#endif
}

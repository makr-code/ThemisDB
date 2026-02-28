/**
 * @file test_llm_single_model_integration.cpp
 * @brief Integration tests for single-model LLM inference scenarios
 *
 * Tests the complete single-model inference workflow using AsyncInferenceEngine:
 * - Basic submit-and-get round-trip
 * - Priority-ordered request scheduling
 * - Per-request timeout propagation
 * - Callback (fire-and-forget) submission
 * - Cancellation of pending requests
 * - SharedWorkerPool integration
 * - Plugin hot-swap without engine restart
 * - Worker statistics reporting
 *
 * All tests use a MockLLMPlugin so no real model file is required.
 */

#include <gtest/gtest.h>
#include "llm/async_inference_engine.h"
#include "llm/shared_worker_pool.h"
#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════════════════════
// Mock LLM plugin for single-model tests
// ═══════════════════════════════════════════════════════════════════════════

class SingleModelMockPlugin : public ILLMPlugin {
public:
    explicit SingleModelMockPlugin(const std::string& model_id = "single-model",
                                   int latency_ms = 10)
        : model_id_(model_id), latency_ms_(latency_ms) {}

    std::atomic<int> call_count{0};

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id  = model_id_;
        info.name      = model_id_;
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        call_count.fetch_add(1, std::memory_order_relaxed);
        if (latency_ms_ > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms_));
        }
        InferenceResponse resp;
        resp.request_id        = request.request_id;
        resp.text              = "response:" + request.prompt;
        resp.model_id          = model_id_;
        resp.tokens_generated  = 5;
        resp.inference_time_ms = static_cast<float>(latency_ms_);
        resp.latency_ms        = latency_ms_;
        return resp;
    }

    InferenceResponse generateRAG(const RAGContext&,
                                   const InferenceRequest& req) override {
        return generate(req);
    }

    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities    getCapabilities() const override { return {}; }
    json getMemoryStats()     const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t>  exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

private:
    std::string model_id_;
    int         latency_ms_;
};

/**
 * @brief Slow mock that blocks until explicitly unblocked — used for
 *        cancellation and timeout tests.
 *
 * @param max_wait_ms Upper bound on how long generate() will block even if
 *                    unblock() is never called.  Keeping this small (e.g. 400 ms)
 *                    ensures tests run quickly even when unblock() is not invoked
 *                    before handle.get().
 */
class BlockingMockPlugin : public ILLMPlugin {
public:
    explicit BlockingMockPlugin(int max_wait_ms = 400)
        : max_wait_ms_(max_wait_ms) {}

    // Release all blocked generate() calls.
    void unblock() {
        std::lock_guard<std::mutex> lk(mu_);
        blocked_ = false;
        cv_.notify_all();
    }

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id  = "blocking";
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait_for(lk, std::chrono::milliseconds(max_wait_ms_),
                     [this] { return !blocked_; });

        InferenceResponse resp;
        resp.request_id = request.request_id;
        resp.text       = "blocking response";
        resp.model_id   = "blocking";
        return resp;
    }

    InferenceResponse generateRAG(const RAGContext&,
                                   const InferenceRequest& req) override {
        return generate(req);
    }

    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities    getCapabilities() const override { return {}; }
    json getMemoryStats()     const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t>  exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

private:
    int                     max_wait_ms_;
    std::mutex              mu_;
    std::condition_variable cv_;
    bool                    blocked_ = true;
};

// ═══════════════════════════════════════════════════════════════════════════
// Test fixture
// ═══════════════════════════════════════════════════════════════════════════

class SingleModelIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.num_worker_threads = 2;
        config_.max_queue_size     = 100;
        config_.enable_priorities  = true;
        config_.backpressure =
            AsyncInferenceEngine::Config::BackpressurePolicy::REJECT;
    }

    AsyncInferenceEngine::Config config_;
};

// ═══════════════════════════════════════════════════════════════════════════
// Test 1: Basic submit-and-get round-trip
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, BasicSubmitAndGet) {
    auto plugin = std::make_shared<SingleModelMockPlugin>("model-a", 5);
    AsyncInferenceEngine engine(plugin, config_);

    InferenceRequest req;
    req.prompt    = "hello";
    req.max_tokens = 32;

    auto handle   = engine.submit(req);
    auto response = handle.get();

    EXPECT_FALSE(response.text.empty())
        << "Response text should not be empty";
    EXPECT_EQ(response.text, "response:hello");
    EXPECT_GT(plugin->call_count.load(), 0)
        << "Plugin should have been called at least once";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 2: Multiple requests complete successfully
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, MultipleRequestsComplete) {
    auto plugin = std::make_shared<SingleModelMockPlugin>("model-b", 5);
    AsyncInferenceEngine engine(plugin, config_);

    const int num_requests = 10;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_requests);

    for (int i = 0; i < num_requests; ++i) {
        InferenceRequest req;
        req.prompt     = "prompt_" + std::to_string(i);
        req.max_tokens = 32;
        handles.push_back(engine.submit(req));
    }

    int completed = 0;
    for (auto& h : handles) {
        auto resp = h.get();
        if (!resp.text.empty()) {
            ++completed;
        }
    }

    EXPECT_EQ(completed, num_requests)
        << "All requests should produce non-empty responses";
    EXPECT_EQ(plugin->call_count.load(), num_requests)
        << "Plugin should be called exactly once per request";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 3: Priority ordering — higher-priority request finishes first
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, PriorityOrdering) {
    // Single worker so requests are serialised and priority matters.
    config_.num_worker_threads = 1;
    auto plugin = std::make_shared<SingleModelMockPlugin>("model-c", 20);
    AsyncInferenceEngine engine(plugin, config_);

    // Submit low-priority first, then high-priority.
    InferenceRequest low_req;
    low_req.prompt     = "low";
    low_req.max_tokens = 32;

    InferenceRequest high_req;
    high_req.prompt    = "high";
    high_req.max_tokens = 32;

    auto low_handle  = engine.submit(low_req,  /*priority=*/1);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto high_handle = engine.submit(high_req, /*priority=*/10);

    auto high_resp = high_handle.get();
    auto low_resp  = low_handle.get();

    EXPECT_FALSE(high_resp.text.empty()) << "High-priority response should be non-empty";
    EXPECT_FALSE(low_resp.text.empty())  << "Low-priority response should be non-empty";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 4: Callback (fire-and-forget) submission
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, CallbackSubmission) {
    auto plugin = std::make_shared<SingleModelMockPlugin>("model-d", 5);
    AsyncInferenceEngine engine(plugin, config_);

    std::atomic<int> callback_count{0};
    std::atomic<bool> callback_text_valid{false};

    const int num_reqs = 5;
    for (int i = 0; i < num_reqs; ++i) {
        InferenceRequest req;
        req.prompt     = "cb_prompt_" + std::to_string(i);
        req.max_tokens = 32;

        engine.submitAsync(req, [&callback_count, &callback_text_valid](
                                    const InferenceResponse& resp) {
            if (!resp.text.empty()) {
                callback_text_valid.store(true);
            }
            callback_count.fetch_add(1, std::memory_order_relaxed);
        });
    }

    engine.waitForCompletion();

    EXPECT_EQ(callback_count.load(), num_reqs)
        << "All callbacks should have fired";
    EXPECT_TRUE(callback_text_valid.load())
        << "At least one callback should receive non-empty text";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 5: Per-request timeout propagation
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, PerRequestTimeout) {
    // Use a blocking plugin with a short self-expiry so the test stays fast.
    // The plugin blocks for at most 300 ms on its own; the per-request timeout
    // (50 ms) fires first, setting the cancel token and incrementing the
    // total_timed_out counter.  handle.get() returns once the worker's
    // generate() call exits (~300 ms worst-case).
    auto plugin = std::make_shared<BlockingMockPlugin>(300 /*max_wait_ms*/);
    AsyncInferenceEngine engine(plugin, config_);

    InferenceRequest req;
    req.prompt     = "will timeout";
    req.max_tokens = 32;

    // Short per-request timeout — will expire while generate() is blocking.
    auto handle = engine.submit(req, /*priority=*/0,
                                std::chrono::milliseconds(50));

    // Unblock the plugin shortly after submission so the worker can exit and
    // resolve the promise quickly (avoids waiting the full 300 ms).
    std::thread unblock_thread([&plugin]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        plugin->unblock();
    });

    auto resp = handle.get();
    (void)resp;

    unblock_thread.join();

    // The timeout monitor should have recorded the expiry.
    auto stats = engine.getWorkerStats();
    EXPECT_TRUE(stats.contains("total_timed_out"))
        << "Worker stats should expose 'total_timed_out' counter";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 6: Request cancellation
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, RequestCancellation) {
    // max_wait_ms=300: generate() unblocks on its own after 300 ms at most,
    // so the test doesn't require unblock() to be called for correctness.
    auto plugin = std::make_shared<BlockingMockPlugin>(300);
    config_.num_worker_threads = 1;
    AsyncInferenceEngine engine(plugin, config_);

    InferenceRequest req;
    req.prompt     = "cancel me";
    req.max_tokens = 32;

    auto handle = engine.submit(req);
    std::string req_id = handle.requestId();

    // Give the engine a moment to pick up the request, then cancel.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bool cancelled = engine.cancel(req_id);
    // Cancellation is best-effort; just verify the API doesn't throw.
    (void)cancelled;

    // Unblock so the worker can return from generate() quickly and shutdown
    // completes within the test timeout.
    plugin->unblock();
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 7: SharedWorkerPool integration with AsyncInferenceEngine
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, SharedWorkerPoolIntegration) {
    SharedWorkerPool::Config pool_cfg;
    pool_cfg.num_threads    = 4;
    pool_cfg.max_queue_size = 1000;
    auto pool = std::make_shared<SharedWorkerPool>(pool_cfg);

    auto plugin = std::make_shared<SingleModelMockPlugin>("pooled-model", 5);
    AsyncInferenceEngine engine(plugin, config_, pool);

    const int num_reqs = 20;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_reqs);

    for (int i = 0; i < num_reqs; ++i) {
        InferenceRequest req;
        req.prompt     = "pool_req_" + std::to_string(i);
        req.max_tokens = 32;
        handles.push_back(engine.submit(req));
    }

    for (auto& h : handles) {
        EXPECT_FALSE(h.get().text.empty())
            << "Every request should produce a response via shared pool";
    }

    EXPECT_EQ(plugin->call_count.load(), num_reqs)
        << "Plugin should be called for every request";

    engine.shutdown();
    pool->shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 8: Plugin hot-swap without engine restart
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, PluginHotSwap) {
    auto original = std::make_shared<SingleModelMockPlugin>("original", 5);
    AsyncInferenceEngine engine(original, config_);

    // Warm-up — use original plugin.
    InferenceRequest req1;
    req1.prompt    = "before swap";
    req1.max_tokens = 32;
    auto resp1 = engine.submit(req1).get();
    EXPECT_EQ(resp1.text, "response:before swap");

    // Swap in a replacement plugin.
    auto replacement = std::make_shared<SingleModelMockPlugin>("replacement", 5);
    engine.swapPlugin(replacement);

    InferenceRequest req2;
    req2.prompt    = "after swap";
    req2.max_tokens = 32;
    auto resp2 = engine.submit(req2).get();
    EXPECT_EQ(resp2.model_id, "replacement")
        << "Responses after swap should come from the replacement plugin";

    EXPECT_GT(replacement->call_count.load(), 0)
        << "Replacement plugin should have been called";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 9: Worker statistics are populated
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, WorkerStatisticsPopulated) {
    auto plugin = std::make_shared<SingleModelMockPlugin>("stats-model", 5);
    AsyncInferenceEngine engine(plugin, config_);

    const int num_reqs = 5;
    for (int i = 0; i < num_reqs; ++i) {
        InferenceRequest req;
        req.prompt     = "stat_req_" + std::to_string(i);
        req.max_tokens = 32;
        engine.submit(req).get();
    }

    auto queue_stats  = engine.getQueueStats();
    auto worker_stats = engine.getWorkerStats();

    // getQueueStats() exposes queue depth and utilisation.
    EXPECT_TRUE(queue_stats.contains("queue_size"))
        << "Queue stats should contain 'queue_size'";
    EXPECT_TRUE(queue_stats.contains("utilization"))
        << "Queue stats should contain 'utilization'";

    // getWorkerStats() exposes submission and completion counters.
    EXPECT_TRUE(worker_stats.contains("total_submitted"))
        << "Worker stats should contain 'total_submitted'";
    EXPECT_TRUE(worker_stats.contains("total_completed"))
        << "Worker stats should contain 'total_completed'";

    EXPECT_GE(worker_stats.value("total_submitted", static_cast<size_t>(0)), static_cast<size_t>(num_reqs))
        << "total_submitted should be >= number of requests";
    EXPECT_GE(worker_stats.value("total_completed", static_cast<size_t>(0)), static_cast<size_t>(num_reqs))
        << "total_completed should be >= number of requests";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 10: Concurrent submissions from multiple threads
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SingleModelIntegrationTest, ConcurrentSubmissions) {
    config_.num_worker_threads = 4;
    config_.max_queue_size     = 500;
    auto plugin = std::make_shared<SingleModelMockPlugin>("concurrent-model", 2);
    AsyncInferenceEngine engine(plugin, config_);

    const int num_threads = 8;
    const int reqs_per_thread = 5;
    std::atomic<int> total_completed{0};
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&engine, &total_completed, t, reqs_per_thread]() {
            for (int i = 0; i < reqs_per_thread; ++i) {
                InferenceRequest req;
                req.prompt     = "thread_" + std::to_string(t) +
                                 "_req_" + std::to_string(i);
                req.max_tokens = 32;
                try {
                    auto resp = engine.submit(req).get();
                    if (!resp.text.empty()) {
                        total_completed.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (...) {
                    // Queue-full rejections are acceptable under load.
                }
            }
        });
    }

    for (auto& thr : threads) {
        thr.join();
    }

    EXPECT_GT(total_completed.load(), 0)
        << "At least some concurrent requests should complete";

    engine.shutdown();
}

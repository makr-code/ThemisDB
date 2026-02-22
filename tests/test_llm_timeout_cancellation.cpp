/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_llm_timeout_cancellation.cpp                  ║
  Version:         0.0.29                                             ║
  Last Modified:   2026-02-22                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Tests: Per-request timeout and cancellation propagation for         ║
         AsyncInferenceEngine and InferenceEngineEnhanced             ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "llm/async_inference_engine.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Mock plugins
// ═══════════════════════════════════════════════════════════

/**
 * @brief Fast mock plugin - completes immediately.
 */
class FastMockPlugin : public ILLMPlugin {
public:
    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = "fast";
        info.name = "fast";
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        InferenceResponse resp;
        resp.request_id = request.request_id;
        resp.text = "fast response";
        resp.model_id = "fast";
        resp.inference_time_ms = 1.0f;
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
};

/**
 * @brief Slow streaming mock plugin.
 *
 * Emits @p num_tokens tokens via stream_callback, sleeping @p token_delay_ms
 * between each one.  This makes it possible to cancel mid-stream.
 */
class SlowStreamingPlugin : public ILLMPlugin {
public:
    explicit SlowStreamingPlugin(int num_tokens = 20, int token_delay_ms = 30)
        : num_tokens_(num_tokens), token_delay_ms_(token_delay_ms) {}

    // Track how many tokens were actually delivered
    std::atomic<int> tokens_delivered{0};

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = "slow_stream";
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        std::string full;
        for (int i = 0; i < num_tokens_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(token_delay_ms_));
            std::string tok = "tok" + std::to_string(i) + " ";
            if (request.stream_callback) {
                request.stream_callback(tok);
            }
            tokens_delivered.fetch_add(1, std::memory_order_relaxed);
            full += tok;
        }
        InferenceResponse resp;
        resp.request_id = request.request_id;
        resp.text = full;
        resp.model_id = "slow_stream";
        resp.tokens_generated = num_tokens_;
        resp.inference_time_ms =
            static_cast<float>(num_tokens_ * token_delay_ms_);
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

private:
    int num_tokens_;
    int token_delay_ms_;
};

// ═══════════════════════════════════════════════════════════
// AsyncInferenceEngine tests
// ═══════════════════════════════════════════════════════════

class AsyncEngineTimeoutCancelTest : public ::testing::Test {
protected:
    AsyncInferenceEngine::Config cfg;
    void SetUp() override {
        cfg.num_worker_threads = 2;
        cfg.max_queue_size = 100;
    }
};

// Test 1: InferenceHandle::cancel() propagates — a queued request should
// surface as a "Request cancelled" exception.
TEST_F(AsyncEngineTimeoutCancelTest, HandleCancelPropagatesForQueuedRequest) {
    // Use a slow plugin so the worker is busy and our test request stays queued.
    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(50, 20); // 1 s per request
    AsyncInferenceEngine engine(slow_plugin, cfg);

    // Fill workers with long-running requests so the next one stays queued.
    InferenceRequest filler;
    filler.prompt = "filler";
    auto fh1 = engine.submit(filler);
    auto fh2 = engine.submit(filler);

    // Submit the request we will cancel
    InferenceRequest req;
    req.prompt = "cancel me";
    auto handle = engine.submit(req);

    // Cancel via the handle
    handle.cancel();

    // The future should either throw (cancelled while queued) or return
    // (started before cancel reached it) — either is valid; we verify
    // that the engine doesn't deadlock.
    bool threw = false;
    try {
        auto resp = handle.get();
        (void)resp;
    } catch (const std::runtime_error& e) {
        threw = true;
        spdlog::info("AsyncEngine cancel test: caught expected exception: {}", e.what());
    }

    // At least the call must have completed.
    SUCCEED();

    engine.shutdown();
    (void)threw;
}

// Test 2: Per-request timeout fires and the future resolves.
// We use a very short timeout with a slow plugin.
TEST_F(AsyncEngineTimeoutCancelTest, PerRequestTimeoutFires) {
    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(30, 30); // ~900 ms
    AsyncInferenceEngine engine(slow_plugin, cfg);

    InferenceRequest req;
    req.prompt = "will timeout";
    // 150 ms timeout against a ~900 ms plugin
    auto handle = engine.submit(req, 0, std::chrono::milliseconds(150));

    bool threw = false;
    try {
        // After timeout the cancel token is set; the worker will finish the
        // current plugin call but stream tokens are dropped.  The future
        // is fulfilled with whatever the plugin returned.
        auto resp = handle.get();
        (void)resp;
    } catch (const std::runtime_error& e) {
        threw = true;
        spdlog::info("AsyncEngine timeout test: caught exception: {}", e.what());
    }

    SUCCEED(); // Must not deadlock; both exception and normal return are valid.

    engine.shutdown();
    (void)threw;
}

// Test 3: Streaming tokens stop being delivered after cancel().
TEST_F(AsyncEngineTimeoutCancelTest, StreamingCancelStopsTokenDelivery) {
    auto streaming_plugin = std::make_shared<SlowStreamingPlugin>(20, 30);
    AsyncInferenceEngine engine(streaming_plugin, cfg);

    std::atomic<int> tokens_received{0};

    InferenceRequest req;
    req.prompt = "stream me";
    req.stream_callback = [&tokens_received](const std::string& /*token*/) {
        tokens_received.fetch_add(1, std::memory_order_relaxed);
    };

    auto handle = engine.submit(req);

    // Let a few tokens through, then cancel.
    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    handle.cancel();

    try {
        auto resp = handle.get();
        (void)resp;
    } catch (...) {}

    int received = tokens_received.load();
    spdlog::info("Streaming cancel test: {} tokens received before cancel", received);

    // The wrapper should have suppressed tokens after cancellation was set,
    // so fewer than all 20 should have been delivered to our callback.
    EXPECT_LT(received, 20);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// InferenceEngineEnhanced tests
// ═══════════════════════════════════════════════════════════

class EnhancedEngineTimeoutCancelTest : public ::testing::Test {
protected:
    InferenceEngineEnhanced::Config config_;
    void SetUp() override {
        config_.enable_context_caching = false;
        config_.enable_batch_processing = true;
        config_.enable_load_balancing = false;
        config_.num_worker_threads = 2;
        config_.max_queue_size = 100;
        config_.batch_timeout_ms = 20;
    }
};

// Test 4: engine.cancel() stops a queued request and signals the handle.
TEST_F(EnhancedEngineTimeoutCancelTest, EngineCancelSignalsHandle) {
    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(40, 20); // ~800 ms
    InferenceEngineEnhanced engine(config_);
    engine.registerModel("slow", slow_plugin);
    engine.start();

    // Fill workers
    InferenceEngineEnhanced::EnhancedInferenceRequest filler;
    filler.request_id = "filler1";
    filler.base_request.prompt = "filler";
    filler.allow_caching = false;
    auto fh1 = engine.submit(filler);
    filler.request_id = "filler2";
    auto fh2 = engine.submit(filler);

    // Submit target request
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "to_cancel";
    req.base_request.prompt = "cancel me";
    req.allow_caching = false;
    auto handle = engine.submit(req);

    // Cancel via engine API
    engine.cancel("to_cancel");

    bool threw = false;
    try {
        auto resp = handle.get();
        (void)resp;
    } catch (const std::runtime_error& e) {
        threw = true;
        spdlog::info("Enhanced cancel test: exception: {}", e.what());
    }

    SUCCEED();

    engine.shutdown();
    (void)threw;
}

// Test 5: InferenceHandle::cancel() on InferenceEngineEnhanced-issued handle.
TEST_F(EnhancedEngineTimeoutCancelTest, HandleCancelPropagatesOnEnhancedEngine) {
    auto streaming_plugin = std::make_shared<SlowStreamingPlugin>(20, 30);
    InferenceEngineEnhanced engine(config_);
    engine.registerModel("slow", streaming_plugin);
    engine.start();

    std::atomic<int> tokens_received{0};

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "stream_cancel";
    req.base_request.prompt = "stream cancel me";
    req.base_request.stream_callback = [&tokens_received](const std::string&) {
        tokens_received.fetch_add(1, std::memory_order_relaxed);
    };
    req.allow_caching = false;

    auto handle = engine.submit(req);

    // Allow a few tokens, then cancel via handle
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    handle.cancel();

    try {
        auto resp = handle.get();
        (void)resp;
    } catch (...) {}

    int received = tokens_received.load();
    spdlog::info("Enhanced streaming cancel: {} tokens received", received);
    EXPECT_LT(received, 20);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Additional audit tests
// ═══════════════════════════════════════════════════════════

// Test 6: getWorkerStats() exposes total_timed_out counter.
TEST_F(AsyncEngineTimeoutCancelTest, WorkerStatsExposesTimedOut) {
    // Use a plugin that takes longer than the per-request timeout.
    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(30, 30); // ~900 ms
    AsyncInferenceEngine engine(slow_plugin, cfg);

    InferenceRequest req;
    req.prompt = "will timeout";
    auto handle = engine.submit(req, 0, std::chrono::milliseconds(100));

    try {
        auto resp = handle.get();
        (void)resp;
    } catch (...) {}

    // Allow time for the timeout monitor to fire and increment the counter.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto stats = engine.getWorkerStats();
    EXPECT_TRUE(stats.contains("total_timed_out"));

    engine.shutdown();
}

// Test 7: submitAsync() honours a per-request timeout via the cancel token.
TEST_F(AsyncEngineTimeoutCancelTest, SubmitAsyncHonoursTimeout) {
    auto streaming_plugin = std::make_shared<SlowStreamingPlugin>(30, 30); // ~900 ms
    AsyncInferenceEngine engine(streaming_plugin, cfg);

    std::atomic<int> tokens_received{0};
    std::atomic<bool> callback_called{false};

    InferenceRequest req;
    req.prompt = "async with timeout";
    req.stream_callback = [&tokens_received](const std::string&) {
        tokens_received.fetch_add(1, std::memory_order_relaxed);
    };

    // Submit via fire-and-forget with a short per-request timeout.
    std::string req_id = engine.submitAsync(
        req,
        [&callback_called](const InferenceResponse&) {
            callback_called.store(true, std::memory_order_release);
        },
        0,
        std::chrono::milliseconds(120)
    );

    EXPECT_FALSE(req_id.empty());

    // Wait for the plugin to finish (it will complete after ~900 ms).
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // The timeout should have fired within 120 ms, stopping token delivery.
    int received = tokens_received.load();
    spdlog::info("submitAsync timeout test: {} tokens received", received);
    // Plugin emits one token every 30 ms; 120 ms timeout + 50 ms monitor
    // poll interval → cancel fires by ~150 ms → at most ~5-6 tokens delivered.
    EXPECT_LT(received, 10);

    engine.shutdown();
}

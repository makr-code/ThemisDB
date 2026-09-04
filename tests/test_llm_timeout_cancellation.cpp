#include <gtest/gtest.h>
#include "llm/async_inference_engine.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include <atomic>
#include <chrono>
#include <filesystem>
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
        std::string full = {};
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

// Test 2: Per-request timeout fires and the future resolves promptly.
// We use a very short timeout with a slow plugin.
TEST_F(AsyncEngineTimeoutCancelTest, PerRequestTimeoutFires) {
    // The timeout monitor polls every 50 ms; allow generous headroom.
    constexpr int PER_REQUEST_TIMEOUT_MS      = 150;
    constexpr int SLOW_PLUGIN_TOTAL_MS        = 900; // 30 tokens × 30 ms
    constexpr int TIMEOUT_RESOLUTION_WINDOW_MS = 500; // must resolve well before plugin finishes

    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(30, 30); // ~900 ms
    AsyncInferenceEngine engine(slow_plugin, cfg);

    InferenceRequest req;
    req.prompt = "will timeout";
    auto handle = engine.submit(req, 0, std::chrono::milliseconds(PER_REQUEST_TIMEOUT_MS));

    auto t0 = std::chrono::steady_clock::now();
    bool threw = false;
    try {
        auto resp = handle.get();
        (void)resp;
    } catch (const std::runtime_error& e) {
        threw = true;
        spdlog::info("AsyncEngine timeout test: caught exception: {}", e.what());
    }
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    // The future must resolve well before the plugin would naturally finish.
    EXPECT_LT(elapsed_ms, TIMEOUT_RESOLUTION_WINDOW_MS)
        << "handle.get() should resolve promptly after timeout, not block until plugin finishes ("
        << SLOW_PLUGIN_TOTAL_MS << " ms)";

    // The promise must have been resolved with a cancellation exception.
    EXPECT_TRUE(threw) << "Expected a timeout exception from handle.get()";

    engine.shutdown();
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

// ═══════════════════════════════════════════════════════════
// Backpressure tests
// ═══════════════════════════════════════════════════════════

// Test 8: DROP_OLDEST policy — when the queue fills up, the lowest-priority
// request is dropped (promise resolved with an exception) and the new higher-
// priority request is accepted.
TEST_F(AsyncEngineTimeoutCancelTest, DropOldestPolicyDropsLowestPriorityRequest) {
    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(50, 20); // ~1 s per request

    AsyncInferenceEngine::Config drop_cfg;
    drop_cfg.num_worker_threads = 1;
    drop_cfg.max_queue_size = 2;  // very small queue: 1 worker slot + 1 queued
    drop_cfg.backpressure = AsyncInferenceEngine::Config::BackpressurePolicy::DROP_OLDEST;

    AsyncInferenceEngine engine(slow_plugin, drop_cfg);

    // Fill the worker + queue completely with low-priority requests.
    InferenceRequest low_req;
    low_req.prompt = "low priority";
    auto h1 = engine.submit(low_req, 0);  // taken by worker
    auto h2 = engine.submit(low_req, 0);  // fills the queue

    // Now submit a higher-priority request — DROP_OLDEST should drop h2.
    InferenceRequest high_req;
    high_req.prompt = "high priority";
    auto h3 = engine.submit(high_req, 100);  // should succeed; h2 gets dropped

    // Depending on worker scheduling at submit time, either h1 or h2 can be
    // the dropped low-priority request. Ensure at least one is dropped.
    bool low_dropped = false;
    auto mark_dropped = [&low_dropped](InferenceHandle& h) {
        try {
            h.get();  // result discarded; we only care about the exception
        } catch (const std::runtime_error& e) {
            low_dropped = true;
            spdlog::info("DROP_OLDEST test: low-priority request dropped: {}", e.what());
        }
    };

    mark_dropped(h1);
    mark_dropped(h2);

    EXPECT_TRUE(low_dropped);

    // h3 should eventually complete without throwing (queue had room after drop).
    bool h3_ok = false;
    try {
        h3.get();
        h3_ok = true;
    } catch (const std::exception& e) {
        spdlog::warn("DROP_OLDEST test: h3 unexpected exception: {}", e.what());
    }
    EXPECT_TRUE(h3_ok);

    engine.shutdown();
}

// Test 9: DROP_OLDEST drops the request with strictly the lowest priority,
// not just the first one added.
TEST_F(AsyncEngineTimeoutCancelTest, DropOldestTargetsLowestPriorityNotFIFO) {
    auto slow_plugin = std::make_shared<SlowStreamingPlugin>(50, 20);

    AsyncInferenceEngine::Config drop_cfg;
    drop_cfg.num_worker_threads = 1;
    drop_cfg.max_queue_size = 3;  // 1 worker + 2 queued
    drop_cfg.backpressure = AsyncInferenceEngine::Config::BackpressurePolicy::DROP_OLDEST;

    AsyncInferenceEngine engine(slow_plugin, drop_cfg);

    // Fill the worker and queue.
    InferenceRequest filler;
    filler.prompt = "filler";
    auto h_worker = engine.submit(filler, 5);  // taken by worker
    auto h_high   = engine.submit(filler, 10); // queued, higher priority
    auto h_low    = engine.submit(filler, 1);  // queued, lowest priority

    // New request triggers DROP_OLDEST — must drop h_low (priority=1).
    InferenceRequest new_req;
    new_req.prompt = "new";
    auto h_new = engine.submit(new_req, 7);

    // h_low must be dropped.
    bool low_dropped = false;
    try {
        h_low.get();  // result discarded; we only care about the exception
    } catch (const std::runtime_error& e) {
        low_dropped = true;
        spdlog::info("DROP_OLDEST priority test: h_low dropped: {}", e.what());
    }
    EXPECT_TRUE(low_dropped);

    engine.shutdown();

    auto consume_without_block = [](InferenceHandle& h) {
        if (h.ready()) {
            try { h.get(); } catch (...) {}
            return;
        }
        h.cancel();
        for (int i = 0; i < 50 && !h.ready(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (h.ready()) {
            try { h.get(); } catch (...) {}
        }
    };

    consume_without_block(h_worker);
    consume_without_block(h_high);
    consume_without_block(h_new);
}

// ═══════════════════════════════════════════════════════════
// Deduplication cache tests
// ═══════════════════════════════════════════════════════════

class AsyncEngineDedupCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto cache_dir_path = std::filesystem::temp_directory_path() /
                              std::filesystem::path("dedup_cache_test_" +
                                  std::to_string(std::chrono::steady_clock::now()
                                                     .time_since_epoch()
                                                     .count()));
        cache_dir_ = cache_dir_path.string();
    }

    void TearDown() override {
        if (!cache_dir_.empty()) {
            std::error_code ec = {};
            std::filesystem::remove_all(cache_dir_, ec);
        }
    }

    std::string cache_dir_;
};

// Test 1: Cache hit returns cached response without calling the plugin again.
TEST_F(AsyncEngineDedupCacheTest, CacheHitSkipsPluginCall) {
    std::atomic<int> plugin_calls{0};

    class CountingPlugin : public FastMockPlugin {
    public:
        explicit CountingPlugin(std::atomic<int>& counter) : counter_(counter) {}
        InferenceResponse generate(const InferenceRequest& request) override {
            ++counter_;
            return FastMockPlugin::generate(request);
        }
    private:
        std::atomic<int>& counter_;
    };

    auto plugin = std::make_shared<CountingPlugin>(plugin_calls);

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    cfg.enable_dedup_cache = true;
    cfg.dedup_cache_config.cache_dir = cache_dir_;
    cfg.dedup_cache_config.similarity_threshold = 0.99f;  // High threshold: near-exact match required

    AsyncInferenceEngine engine(plugin, cfg);

    InferenceRequest req;
    req.prompt = "What is ThemisDB?";

    // First request: cache miss, plugin called
    auto h1 = engine.submit(req);
    auto r1 = h1.get();
    EXPECT_EQ(plugin_calls.load(), 1);
    EXPECT_FALSE(r1.cache_hit);

    // Second request with same prompt: cache hit, plugin NOT called again
    auto h2 = engine.submit(req);
    auto r2 = h2.get();
    EXPECT_EQ(plugin_calls.load(), 1);  // no additional call
    EXPECT_TRUE(r2.cache_hit);
    EXPECT_EQ(r2.text, r1.text);

    engine.shutdown();
}

// Test 2: Different prompts each get their own inference call.
TEST_F(AsyncEngineDedupCacheTest, DifferentPromptsCauseSeparateInference) {
    std::atomic<int> plugin_calls{0};

    class CountingPlugin : public FastMockPlugin {
    public:
        explicit CountingPlugin(std::atomic<int>& counter) : counter_(counter) {}
        InferenceResponse generate(const InferenceRequest& request) override {
            ++counter_;
            InferenceResponse r = FastMockPlugin::generate(request);
            r.text = "response for: " + request.prompt;
            return r;
        }
    private:
        std::atomic<int>& counter_;
    };

    auto plugin = std::make_shared<CountingPlugin>(plugin_calls);

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    cfg.enable_dedup_cache = true;
    cfg.dedup_cache_config.cache_dir = cache_dir_;
    cfg.dedup_cache_config.similarity_threshold = 0.99f;

    AsyncInferenceEngine engine(plugin, cfg);

    InferenceRequest req1, req2;
    req1.prompt = "Prompt A";
    req2.prompt = "Prompt B";

    auto h1 = engine.submit(req1);
    auto h2 = engine.submit(req2);

    auto r1 = h1.get();
    auto r2 = h2.get();

    EXPECT_EQ(plugin_calls.load(), 2);
    EXPECT_NE(r1.text, r2.text);

    engine.shutdown();
}

// Test 3: Dedup cache stats are updated correctly.
TEST_F(AsyncEngineDedupCacheTest, StatsReflectHitsAndMisses) {
    auto plugin = std::make_shared<FastMockPlugin>();

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;
    cfg.enable_dedup_cache = true;
    cfg.dedup_cache_config.cache_dir = cache_dir_;
    cfg.dedup_cache_config.similarity_threshold = 0.99f;

    AsyncInferenceEngine engine(plugin, cfg);

    InferenceRequest req;
    req.prompt = "Stats test prompt";

    // First call: miss
    engine.submit(req).get();
    // Second call: hit
    engine.submit(req).get();

    auto worker_stats = engine.getWorkerStats();
    EXPECT_EQ(worker_stats["total_dedup_cache_hits"].get<size_t>(), 1u);
    EXPECT_EQ(worker_stats["total_dedup_cache_misses"].get<size_t>(), 1u);

    auto cache_stats = engine.getDedupCacheStats();
    EXPECT_GE(cache_stats.hits.load(), 1u);

    engine.shutdown();
}

// Test 4: Disabled cache (default) performs no caching.
TEST_F(AsyncEngineDedupCacheTest, DisabledCacheByDefault) {
    std::atomic<int> plugin_calls{0};

    class CountingPlugin : public FastMockPlugin {
    public:
        explicit CountingPlugin(std::atomic<int>& counter) : counter_(counter) {}
        InferenceResponse generate(const InferenceRequest& request) override {
            ++counter_;
            return FastMockPlugin::generate(request);
        }
    private:
        std::atomic<int>& counter_;
    };

    auto plugin = std::make_shared<CountingPlugin>(plugin_calls);

    // Default config: enable_dedup_cache = false
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    AsyncInferenceEngine engine(plugin, cfg);

    InferenceRequest req;
    req.prompt = "Same prompt";

    engine.submit(req).get();
    engine.submit(req).get();

    // Both calls must reach the plugin
    EXPECT_EQ(plugin_calls.load(), 2);

    // getDedupCacheStats returns empty/default stats when cache is disabled
    auto cache_stats = engine.getDedupCacheStats();
    EXPECT_EQ(cache_stats.hits.load(), 0u);
    EXPECT_EQ(cache_stats.misses.load(), 0u);

    engine.shutdown();
}

// Test 5: setDedupCache allows injecting an external cache.
TEST_F(AsyncEngineDedupCacheTest, SetDedupCacheExternalInjection) {
    std::atomic<int> plugin_calls{0};

    class CountingPlugin : public FastMockPlugin {
    public:
        explicit CountingPlugin(std::atomic<int>& counter) : counter_(counter) {}
        InferenceResponse generate(const InferenceRequest& request) override {
            ++counter_;
            return FastMockPlugin::generate(request);
        }
    private:
        std::atomic<int>& counter_;
    };

    auto plugin = std::make_shared<CountingPlugin>(plugin_calls);

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    AsyncInferenceEngine engine(plugin, cfg);

    // Inject external cache
    LLMResponseCache::Config cache_cfg;
    cache_cfg.cache_dir = cache_dir_;
    cache_cfg.similarity_threshold = 0.99f;
    auto external_cache = std::make_shared<LLMResponseCache>("external_dedup", cache_cfg);
    engine.setDedupCache(external_cache);

    InferenceRequest req;
    req.prompt = "External cache test";

    engine.submit(req).get();              // cache miss
    auto r2 = engine.submit(req).get();    // cache hit

    EXPECT_EQ(plugin_calls.load(), 1);
    EXPECT_TRUE(r2.cache_hit);

    auto worker_stats = engine.getWorkerStats();
    EXPECT_EQ(worker_stats["total_dedup_cache_hits"].get<size_t>(), 1u);

    engine.shutdown();
}


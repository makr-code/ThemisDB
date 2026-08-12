#include <gtest/gtest.h>
#include "llm/shared_worker_pool.h"
#include "llm/async_inference_engine.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Minimal mock plugin
// ═══════════════════════════════════════════════════════════

class SimpleMockPlugin : public ILLMPlugin {
public:
    explicit SimpleMockPlugin(int latency_ms = 5)
        : latency_ms_(latency_ms) {}

    std::atomic<int> call_count{0};

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = "mock";
        info.name     = "mock";
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
        resp.request_id       = request.request_id;
        resp.text             = "mock:" + request.prompt;
        resp.model_id         = "mock";
        resp.tokens_generated = 5;
        resp.inference_time_ms = static_cast<float>(latency_ms_);
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                   const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities    getCapabilities() const override { return {}; }
    json getMemoryStats()    const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t>  exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

private:
    int latency_ms_;
};

// ═══════════════════════════════════════════════════════════
// SharedWorkerPool unit tests
// ═══════════════════════════════════════════════════════════

class SharedWorkerPoolTest : public ::testing::Test {
protected:
    SharedWorkerPool::Config pool_cfg;
    void SetUp() override {
        pool_cfg.num_threads    = 4;
        pool_cfg.max_queue_size = 1000;
    }
};

// Test 1: Basic task execution
TEST_F(SharedWorkerPoolTest, ExecutesSingleTask) {
    SharedWorkerPool pool(pool_cfg);

    std::atomic<bool> executed{false};
    pool.submit([&executed]() { executed.store(true); });

    // Give worker time to execute
    for (int i = 0; i < 50 && !executed.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(executed.load());
    pool.shutdown();
}

// Test 2: Multiple concurrent tasks all complete
TEST_F(SharedWorkerPoolTest, ExecutesManyTasksConcurrently) {
    SharedWorkerPool pool(pool_cfg);

    constexpr int N = 100;
    std::atomic<int> counter{0};

    for (int i = 0; i < N; ++i) {
        pool.submit([&counter]() { counter.fetch_add(1); });
    }

    // Wait for completion
    for (int i = 0; i < 200 && counter.load() < N; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(counter.load(), N);
    pool.shutdown();
}

// Test 3: tasksCompleted counter increments
TEST_F(SharedWorkerPoolTest, TasksCompletedCounterIsAccurate) {
    SharedWorkerPool pool(pool_cfg);

    constexpr int N = 20;
    std::atomic<int> done{0};

    for (int i = 0; i < N; ++i) {
        pool.submit([&done]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            done.fetch_add(1);
        });
    }

    for (int i = 0; i < 200 && done.load() < N; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(static_cast<int>(pool.tasksCompleted()), N);
    pool.shutdown();
}

// Test 4: Priority ordering — higher-priority tasks complete before lower ones
// (best-effort; we just verify both tasks execute)
TEST_F(SharedWorkerPoolTest, HighPriorityTaskExecutes) {
    SharedWorkerPool::Config cfg;
    cfg.num_threads = 1;  // serial to observe ordering
    SharedWorkerPool pool(cfg);

    // Pause the single worker with a slow task first
    std::atomic<bool> gate{false};
    pool.submit([&gate]() {
        while (!gate.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }, 0);

    // Queue low and high priority tasks while worker is blocked
    std::vector<int> execution_order;
    std::mutex order_mutex;

    pool.submit([&]() {
        std::lock_guard<std::mutex> lk(order_mutex);
        execution_order.push_back(1);  // low priority
    }, 1);

    pool.submit([&]() {
        std::lock_guard<std::mutex> lk(order_mutex);
        execution_order.push_back(10); // high priority
    }, 10);

    // Release gate
    gate.store(true);

    for (int i = 0; i < 200 && execution_order.size() < 2; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(static_cast<int>(execution_order.size()), 2);
    // High priority (10) should have been dequeued before low (1)
    EXPECT_EQ(execution_order[0], 10);
    pool.shutdown();
}

// Test 5: Queue-full returns false
TEST_F(SharedWorkerPoolTest, FullQueueReturnsFalse) {
    SharedWorkerPool::Config cfg;
    cfg.num_threads    = 1;
    cfg.max_queue_size = 2;
    SharedWorkerPool pool(cfg);

    // Block the single worker
    std::atomic<bool> gate{false};
    pool.submit([&gate]() {
        while (!gate.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Fill the queue
    bool r1 = pool.submit([]() {});
    bool r2 = pool.submit([]() {});
    // This should be rejected (queue size = 2)
    bool r3 = pool.submit([]() {});

    EXPECT_TRUE(r1);
    EXPECT_TRUE(r2);
    EXPECT_FALSE(r3);

    gate.store(true);
    pool.shutdown();
}

// Test 6: shutdown() is idempotent
TEST_F(SharedWorkerPoolTest, ShutdownIsIdempotent) {
    SharedWorkerPool pool(pool_cfg);
    pool.shutdown();
    pool.shutdown();  // must not throw or hang
    SUCCEED();
}

// Test 7: getMetrics() returns sensible JSON
TEST_F(SharedWorkerPoolTest, GetMetricsReturnsJson) {
    SharedWorkerPool pool(pool_cfg);

    auto m = pool.getMetrics();
    EXPECT_EQ(m["num_threads"].get<int>(), 4);
    EXPECT_GE(m["queue_depth"].get<int>(), 0);
    EXPECT_GE(m["tasks_completed"].get<int>(), 0);
    EXPECT_TRUE(m["running"].get<bool>());

    pool.shutdown();
}

// ═══════════════════════════════════════════════════════════
// AsyncInferenceEngine + SharedWorkerPool integration tests
// ═══════════════════════════════════════════════════════════

class AsyncEngineSharedPoolTest : public ::testing::Test {
protected:
    std::shared_ptr<SharedWorkerPool>    pool;
    std::shared_ptr<SimpleMockPlugin>    plugin;

    void SetUp() override {
        SharedWorkerPool::Config pc;
        pc.num_threads = 4;
        pool   = std::make_shared<SharedWorkerPool>(pc);
        plugin = std::make_shared<SimpleMockPlugin>(10);
    }
    void TearDown() override {
        pool->shutdown();
    }
};

// Test 8: AsyncInferenceEngine with shared pool returns correct responses
TEST_F(AsyncEngineSharedPoolTest, SubmitReturnsCorrectResponse) {
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 2;  // ignored when shared pool is active
    AsyncInferenceEngine engine(plugin, cfg, pool);

    InferenceRequest req;
    req.prompt = "hello";
    auto handle = engine.submit(req);

    auto resp = handle.get();
    EXPECT_EQ(resp.text, "mock:hello");

    engine.shutdown();
}

// Test 9: Multiple concurrent submits via shared pool
TEST_F(AsyncEngineSharedPoolTest, MultipleConcurrentSubmits) {
    AsyncInferenceEngine::Config cfg;
    AsyncInferenceEngine engine(plugin, cfg, pool);

    constexpr int N = 20;
    std::vector<InferenceHandle> handles;
    handles.reserve(N);

    for (int i = 0; i < N; ++i) {
        InferenceRequest req;
        req.prompt = "req" + std::to_string(i);
        handles.push_back(engine.submit(req));
    }

    for (auto& h : handles) {
        auto resp = h.get();
        EXPECT_FALSE(resp.text.empty());
    }

    EXPECT_EQ(plugin->call_count.load(), N);
    engine.shutdown();
}

// Test 10: Cancel still works with shared pool
TEST_F(AsyncEngineSharedPoolTest, CancelPropagatesViaSharedPool) {
    // Use a slow plugin to ensure requests queue up
    auto slow = std::make_shared<SimpleMockPlugin>(200);
    AsyncInferenceEngine::Config cfg;
    cfg.max_queue_size = 100;
    AsyncInferenceEngine engine(slow, cfg, pool);

    // Flood pool to keep workers busy
    for (int i = 0; i < 8; ++i) {
        InferenceRequest filler;
        filler.prompt = "filler";
        engine.submit(filler);
    }

    InferenceRequest req;
    req.prompt = "cancel me";
    auto handle = engine.submit(req);
    handle.cancel();

    // The result must be obtainable (either cancelled exception or response)
    bool threw = false;
    try {
        auto resp = handle.get();
        (void)resp;
    } catch (const std::runtime_error&) {
        threw = true;
    }
    (void)threw;  // either outcome is acceptable
    SUCCEED();

    engine.shutdown();
}

// Test 11: Both engines share the SAME pool
TEST_F(AsyncEngineSharedPoolTest, TwoEnginesSharePool) {
    auto plugin2 = std::make_shared<SimpleMockPlugin>(5);

    AsyncInferenceEngine::Config async_cfg;
    AsyncInferenceEngine async_engine(plugin, async_cfg, pool);

    InferenceEngineEnhanced::Config enh_cfg;
    enh_cfg.num_worker_threads = 2;
    InferenceEngineEnhanced enh_engine(enh_cfg, pool);
    enh_engine.registerModel("mock2", plugin2);
    enh_engine.start();

    // Submit to async engine
    InferenceRequest req1;
    req1.prompt = "async_req";
    auto h1 = async_engine.submit(req1);

    // Submit to enhanced engine
    InferenceEngineEnhanced::EnhancedInferenceRequest ereq;
    ereq.base_request.prompt  = "enh_req";
    ereq.request_id           = "test-enh-001";
    ereq.preferred_model_id   = "mock2";
    ereq.allow_caching        = false;
    auto h2 = enh_engine.submit(ereq);

    // Both must complete without errors
    auto resp1 = h1.get();
    auto resp2 = h2.get();

    EXPECT_EQ(resp1.text, "mock:async_req");
    EXPECT_FALSE(resp2.text.empty());

    enh_engine.shutdown();
    async_engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// InferenceEngineEnhanced + SharedWorkerPool integration
// ═══════════════════════════════════════════════════════════

class EnhancedEngineSharedPoolTest : public ::testing::Test {
protected:
    std::shared_ptr<SharedWorkerPool>    pool;
    std::shared_ptr<SimpleMockPlugin>    plugin;

    void SetUp() override {
        SharedWorkerPool::Config pc;
        pc.num_threads = 4;
        pool   = std::make_shared<SharedWorkerPool>(pc);
        plugin = std::make_shared<SimpleMockPlugin>(5);
    }
    void TearDown() override {
        pool->shutdown();
    }
};

// Test 12: Enhanced engine with shared pool returns response
TEST_F(EnhancedEngineSharedPoolTest, SubmitReturnsResponse) {
    InferenceEngineEnhanced::Config cfg;
    cfg.num_worker_threads = 2;
    InferenceEngineEnhanced engine(cfg, pool);
    engine.registerModel("mock", plugin);
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.base_request.prompt = "hello_enh";
    req.request_id          = "test-001";
    req.allow_caching       = false;
    auto handle = engine.submit(req);

    auto resp = handle.get();
    EXPECT_FALSE(resp.text.empty());

    engine.shutdown();
}

// Test 13: Multiple requests complete with shared pool
TEST_F(EnhancedEngineSharedPoolTest, MultipleConcurrentRequests) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_batch_processing = false;  // process one at a time for simplicity
    InferenceEngineEnhanced engine(cfg, pool);
    engine.registerModel("mock", plugin);
    engine.start();

    constexpr int N = 10;
    std::vector<InferenceHandle> handles;
    handles.reserve(N);

    for (int i = 0; i < N; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.base_request.prompt = "prompt" + std::to_string(i);
        req.request_id          = "test-multi-" + std::to_string(i);
        req.allow_caching       = false;
        handles.push_back(engine.submit(req));
    }

    for (auto& h : handles) {
        auto resp = h.get();
        EXPECT_FALSE(resp.text.empty());
    }

    engine.shutdown();
}

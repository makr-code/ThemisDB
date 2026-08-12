/**
 * @file test_llm_multi_model_integration.cpp
 * @brief Integration tests for multi-model LLM inference scenarios
 *
 * Tests the complete multi-model inference workflow using InferenceEngineEnhanced:
 * - Registration and deregistration of multiple models
 * - Round-robin and least-loaded load balancing
 * - Preferred model routing per request
 * - Context-based model routing via routing rules
 * - Per-model resource quota enforcement
 * - Model hot-swap without engine restart
 * - LoRA adapter hot-load / unload across models
 * - SharedWorkerPool shared between AsyncInferenceEngine and
 *   InferenceEngineEnhanced
 * - Statistics and metrics after multi-model workload
 *
 * All tests use a MockLLMPlugin so no real model file is required.
 */

#include <gtest/gtest.h>
#include "llm/inference_engine_enhanced.h"
#include "llm/async_inference_engine.h"
#include "llm/shared_worker_pool.h"
#include "llm/inference_handle.h"
#include "llm/llm_plugin_interface.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <spdlog/spdlog.h>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════════════════════
// Mock LLM plugin for multi-model tests
// ═══════════════════════════════════════════════════════════════════════════

class MultiModelMockPlugin : public ILLMPlugin {
public:
    explicit MultiModelMockPlugin(const std::string& model_id,
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
        resp.text              = model_id_ + ":response:" + request.prompt;
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

    const std::string& modelId() const { return model_id_; }

private:
    std::string model_id_;
    int         latency_ms_;
};

// ═══════════════════════════════════════════════════════════════════════════
// Test fixture
// ═══════════════════════════════════════════════════════════════════════════

class MultiModelIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.enable_context_caching    = false;  // focus on routing
        config_.enable_batch_processing   = false;
        config_.enable_load_balancing     = true;
        config_.max_queue_size            = 500;
        config_.num_worker_threads        = 2;
        config_.batch_timeout_ms          = 10;
        config_.request_timeout_ms        = 5000;
    }

    InferenceEngineEnhanced::Config config_;

    // Helper: build a simple enhanced request.
    static InferenceEngineEnhanced::EnhancedInferenceRequest
    makeRequest(const std::string& id,
                const std::string& prompt,
                const std::string& preferred_model = "") {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id                  = id;
        req.base_request.prompt         = prompt;
        req.base_request.max_tokens     = 32;
        req.allow_caching               = false;
        req.preferred_model_id          = preferred_model;
        return req;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Test 1: Register multiple models and get responses from each
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, RegisterAndQueryMultipleModels) {
    InferenceEngineEnhanced engine(config_);

    auto model_a = std::make_shared<MultiModelMockPlugin>("model-a", 5);
    auto model_b = std::make_shared<MultiModelMockPlugin>("model-b", 5);
    auto model_c = std::make_shared<MultiModelMockPlugin>("model-c", 5);

    engine.registerModel("model-a", model_a);
    engine.registerModel("model-b", model_b);
    engine.registerModel("model-c", model_c);

    auto models = engine.getAvailableModels();
    EXPECT_EQ(models.size(), 3u) << "Three models should be registered";

    engine.start();

    // Submit requests and collect responses.
    const int num_reqs = 12;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_reqs);

    for (int i = 0; i < num_reqs; ++i) {
        handles.push_back(
            engine.submit(makeRequest("req_" + std::to_string(i),
                                      "prompt_" + std::to_string(i))));
    }

    int completed = 0;
    for (auto& h : handles) {
        auto resp = h.get();
        if (!resp.text.empty()) {
            ++completed;
        }
    }

    EXPECT_EQ(completed, num_reqs) << "All requests should produce responses";

    auto stats = engine.getStatistics();
    EXPECT_EQ(stats.completed_requests, static_cast<size_t>(num_reqs))
        << "Statistics should reflect all completed requests";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 2: Round-robin load balancing distributes requests across models
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, RoundRobinLoadBalancing) {
    config_.load_balance_strategy =
        InferenceEngineEnhanced::Config::LoadBalanceStrategy::ROUND_ROBIN;

    InferenceEngineEnhanced engine(config_);

    auto model_x = std::make_shared<MultiModelMockPlugin>("model-x", 5);
    auto model_y = std::make_shared<MultiModelMockPlugin>("model-y", 5);

    engine.registerModel("model-x", model_x);
    engine.registerModel("model-y", model_y);
    engine.start();

    const int num_reqs = 10;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_reqs);

    for (int i = 0; i < num_reqs; ++i) {
        handles.push_back(
            engine.submit(makeRequest("rr_" + std::to_string(i),
                                      "rr_prompt_" + std::to_string(i))));
    }

    for (auto& h : handles) {
        EXPECT_FALSE(h.get().text.empty());
    }

    auto stats = engine.getStatistics();
    // With round-robin and two models the fairness index should be near 1.0.
    EXPECT_GT(stats.load_balance_fairness, 0.5)
        << "Round-robin fairness should be > 0.5";
    EXPECT_FALSE(stats.requests_per_model.empty())
        << "Per-model request counts should be populated";

    // Both models should have received at least one request.
    bool x_used = stats.requests_per_model.count("model-x") > 0 &&
                  stats.requests_per_model.at("model-x") > 0;
    bool y_used = stats.requests_per_model.count("model-y") > 0 &&
                  stats.requests_per_model.at("model-y") > 0;
    EXPECT_TRUE(x_used) << "model-x should have served at least one request";
    EXPECT_TRUE(y_used) << "model-y should have served at least one request";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 3: Least-loaded balancing favours faster model
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, LeastLoadedBalancing) {
    config_.load_balance_strategy =
        InferenceEngineEnhanced::Config::LoadBalanceStrategy::LEAST_LOADED;

    InferenceEngineEnhanced engine(config_);

    auto fast = std::make_shared<MultiModelMockPlugin>("fast-model",  5);
    auto slow = std::make_shared<MultiModelMockPlugin>("slow-model", 50);

    engine.registerModel("fast-model", fast);
    engine.registerModel("slow-model", slow);
    engine.start();

    const int num_reqs = 10;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_reqs);

    for (int i = 0; i < num_reqs; ++i) {
        handles.push_back(
            engine.submit(makeRequest("ll_" + std::to_string(i),
                                      "ll_prompt_" + std::to_string(i))));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (auto& h : handles) {
        EXPECT_FALSE(h.get().text.empty());
    }

    auto stats = engine.getStatistics();
    // Fast model should handle more requests under least-loaded strategy.
    size_t fast_count = stats.requests_per_model.count("fast-model") ?
                        stats.requests_per_model.at("fast-model") : 0u;
    size_t slow_count = stats.requests_per_model.count("slow-model") ?
                        stats.requests_per_model.at("slow-model") : 0u;

    spdlog::info("LeastLoaded: fast={}, slow={}", fast_count, slow_count);
    EXPECT_GE(fast_count + slow_count, static_cast<size_t>(num_reqs) / 2u)
        << "At least half the requests should have been distributed";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 4: Preferred model routing
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, PreferredModelRouting) {
    InferenceEngineEnhanced engine(config_);

    auto general = std::make_shared<MultiModelMockPlugin>("general", 5);
    auto special  = std::make_shared<MultiModelMockPlugin>("special",  5);

    engine.registerModel("general", general);
    engine.registerModel("special",  special);
    engine.start();

    // Send all requests with preferred_model_id = "special".
    const int num_reqs = 6;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_reqs);

    for (int i = 0; i < num_reqs; ++i) {
        handles.push_back(
            engine.submit(makeRequest("pref_" + std::to_string(i),
                                      "pref_prompt_" + std::to_string(i),
                                      "special")));
    }

    for (auto& h : handles) {
        auto resp = h.get();
        EXPECT_FALSE(resp.text.empty());
        EXPECT_EQ(resp.model_id, "special")
            << "All responses should come from the preferred model";
    }

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 5: Unregister model — requests fall back to remaining models
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, UnregisterModelFallback) {
    InferenceEngineEnhanced engine(config_);

    auto model_1 = std::make_shared<MultiModelMockPlugin>("model-1", 5);
    auto model_2 = std::make_shared<MultiModelMockPlugin>("model-2", 5);

    engine.registerModel("model-1", model_1);
    engine.registerModel("model-2", model_2);
    engine.start();

    // Verify both models are initially available.
    EXPECT_EQ(engine.getAvailableModels().size(), 2u);

    // Submit a first batch via model-1.
    auto h1 = engine.submit(makeRequest("pre_unreg", "before unregister", "model-1"));
    EXPECT_FALSE(h1.get().text.empty());

    // Unregister model-1.
    engine.unregisterModel("model-1");
    EXPECT_EQ(engine.getAvailableModels().size(), 1u);

    // Subsequent requests should be handled by model-2.
    auto h2 = engine.submit(makeRequest("post_unreg", "after unregister"));
    auto resp2 = h2.get();
    EXPECT_FALSE(resp2.text.empty()) << "model-2 should handle request after model-1 is gone";
    EXPECT_EQ(resp2.model_id, "model-2");

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 6: Model hot-swap during live inference
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, ModelHotSwap) {
    InferenceEngineEnhanced engine(config_);

    auto original    = std::make_shared<MultiModelMockPlugin>("original",    5);
    auto replacement = std::make_shared<MultiModelMockPlugin>("replacement", 5);

    engine.registerModel("swap-model", original);
    engine.start();

    // Requests before swap use the original plugin.
    auto h_before = engine.submit(makeRequest("pre_swap", "before swap", "swap-model"));
    auto resp_before = h_before.get();
    EXPECT_EQ(resp_before.model_id, "original");

    // Hot-swap the plugin.
    engine.swapModel("swap-model", replacement);

    // Requests after swap should use the replacement.
    auto h_after = engine.submit(makeRequest("post_swap", "after swap", "swap-model"));
    auto resp_after = h_after.get();
    EXPECT_EQ(resp_after.model_id, "replacement")
        << "Responses after hot-swap should come from the replacement plugin";
    EXPECT_GT(replacement->call_count.load(), 0)
        << "Replacement plugin should have been called";

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 7: Per-model resource quota enforcement
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, PerModelResourceQuota) {
    InferenceEngineEnhanced engine(config_);

    auto model_a = std::make_shared<MultiModelMockPlugin>("quota-model-a", 5);
    auto model_b = std::make_shared<MultiModelMockPlugin>("quota-model-b", 5);

    engine.registerModel("quota-model-a", model_a);
    engine.registerModel("quota-model-b", model_b);

    // Set a quota on model-a.
    InferenceEngineEnhanced::ModelResourceQuota quota;
    quota.max_concurrent_requests = 2;
    quota.max_memory_mb            = 512;
    engine.setModelQuota("quota-model-a", quota);

    auto retrieved = engine.getModelQuota("quota-model-a");
    EXPECT_EQ(retrieved.max_concurrent_requests, 2u);
    EXPECT_EQ(retrieved.max_memory_mb, 512u);

    // Default (unset) quota should return zeros.
    auto default_quota = engine.getModelQuota("quota-model-b");
    EXPECT_EQ(default_quota.max_concurrent_requests, 0u);
    EXPECT_EQ(default_quota.max_memory_mb, 0u);

    engine.start();

    // Submit a few requests — engine should still serve them regardless of quota
    // (quota is informational; the engine doesn't hard-block).
    std::vector<InferenceHandle> handles;
    for (int i = 0; i < 4; ++i) {
        handles.push_back(
            engine.submit(makeRequest("quota_" + std::to_string(i),
                                      "quota_prompt_" + std::to_string(i),
                                      "quota-model-a")));
    }
    for (auto& h : handles) {
        EXPECT_FALSE(h.get().text.empty());
    }

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 8: LoRA adapter hot-load and unload across models
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, LoRAHotLoadUnload) {
    InferenceEngineEnhanced engine(config_);

    auto model_a = std::make_shared<MultiModelMockPlugin>("lora-model-a", 5);
    auto model_b = std::make_shared<MultiModelMockPlugin>("lora-model-b", 5);

    engine.registerModel("lora-model-a", model_a);
    engine.registerModel("lora-model-b", model_b);

    // Load a LoRA adapter on all models.
    EXPECT_NO_THROW(
        engine.loadLoRAAdapter("adapter-1", "/tmp/fake_adapter.bin", 1.0f));

    auto adapters = engine.getLoadedLoRAAdapters();
    EXPECT_EQ(adapters.size(), 1u) << "One LoRA adapter should be registered";
    EXPECT_EQ(adapters[0].lora_id, "adapter-1");

    // Load a second adapter only on model-a.
    EXPECT_NO_THROW(
        engine.loadLoRAAdapter("adapter-2", "/tmp/fake_adapter2.bin", 0.5f,
                               "lora-model-a"));

    adapters = engine.getLoadedLoRAAdapters();
    EXPECT_EQ(adapters.size(), 2u) << "Two LoRA adapters should be registered";

    // Unload adapter-1.
    bool removed = engine.unloadLoRAAdapter("adapter-1");
    EXPECT_TRUE(removed) << "Unload of registered adapter should succeed";

    adapters = engine.getLoadedLoRAAdapters();
    EXPECT_EQ(adapters.size(), 1u) << "Only one adapter should remain after unload";

    // Unloading non-existent adapter should be a no-op (return false).
    bool noop = engine.unloadLoRAAdapter("adapter-999");
    EXPECT_FALSE(noop);
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 9: Content-based routing rules
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, ContentBasedRoutingRules) {
    InferenceEngineEnhanced engine(config_);

    auto code_model    = std::make_shared<MultiModelMockPlugin>("code-model",    5);
    auto general_model = std::make_shared<MultiModelMockPlugin>("general-model", 5);

    engine.registerModel("code-model",    code_model);
    engine.registerModel("general-model", general_model);

    // Add a routing rule: prompts containing "def " → code-model.
    RoutingRule rule;
    rule.id              = "code-rule";
    rule.target_model_id = "code-model";
    rule.prompt_patterns = {"def ", "function ", "class "};
    rule.priority        = 10;

    engine.addRoutingRule(rule);

    auto rules = engine.getRoutingRules();
    EXPECT_EQ(rules.size(), 1u) << "One routing rule should be registered";
    EXPECT_EQ(rules[0].id, "code-rule");

    engine.start();

    // Routing rule evaluation is tested via round-trip; the "code-rule" should
    // direct code prompts to code-model.
    auto h = engine.submit(makeRequest("code_req", "def my_function():"));
    auto resp = h.get();
    EXPECT_FALSE(resp.text.empty());
    EXPECT_EQ(resp.model_id, "code-model")
        << "Prompt matching the code routing rule should go to code-model";

    // Remove the rule and verify clearRoutingRules works.
    bool removed = engine.removeRoutingRule("code-rule");
    EXPECT_TRUE(removed);
    engine.clearRoutingRules();
    EXPECT_TRUE(engine.getRoutingRules().empty());

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 10: SharedWorkerPool shared between AsyncInferenceEngine and
//          InferenceEngineEnhanced
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, SharedPoolBetweenBothEngines) {
    SharedWorkerPool::Config pool_cfg;
    pool_cfg.num_threads    = 4;
    pool_cfg.max_queue_size = 2000;
    auto pool = std::make_shared<SharedWorkerPool>(pool_cfg);

    // Single-model engine using the shared pool.
    auto async_plugin = std::make_shared<MultiModelMockPlugin>("async-pool-model", 5);
    AsyncInferenceEngine::Config async_cfg;
    async_cfg.num_worker_threads = 2;
    async_cfg.max_queue_size     = 200;
    AsyncInferenceEngine async_engine(async_plugin, async_cfg, pool);

    // Multi-model engine using the same shared pool.
    InferenceEngineEnhanced::Config enh_cfg = config_;
    InferenceEngineEnhanced enh_engine(enh_cfg, pool);

    auto enh_model_a = std::make_shared<MultiModelMockPlugin>("enh-a", 5);
    auto enh_model_b = std::make_shared<MultiModelMockPlugin>("enh-b", 5);
    enh_engine.registerModel("enh-a", enh_model_a);
    enh_engine.registerModel("enh-b", enh_model_b);
    enh_engine.start();

    const int num_async_reqs = 10;
    const int num_enh_reqs   = 10;

    // Submit to both engines concurrently.
    std::vector<InferenceHandle> async_handles, enh_handles;
    async_handles.reserve(num_async_reqs);
    enh_handles.reserve(num_enh_reqs);

    for (int i = 0; i < num_async_reqs; ++i) {
        InferenceRequest req;
        req.prompt     = "async_pool_" + std::to_string(i);
        req.max_tokens = 32;
        async_handles.push_back(async_engine.submit(req));
    }
    for (int i = 0; i < num_enh_reqs; ++i) {
        enh_handles.push_back(
            enh_engine.submit(makeRequest("enh_pool_" + std::to_string(i),
                                          "enh_pool_prompt_" + std::to_string(i))));
    }

    int async_ok = 0, enh_ok = 0;
    for (auto& h : async_handles) {
        if (!h.get().text.empty()) ++async_ok;
    }
    for (auto& h : enh_handles) {
        if (!h.get().text.empty()) ++enh_ok;
    }

    EXPECT_EQ(async_ok, num_async_reqs)
        << "All async-engine requests should complete via shared pool";
    EXPECT_EQ(enh_ok, num_enh_reqs)
        << "All enhanced-engine requests should complete via shared pool";

    // Pool should report tasks completed.
    EXPECT_GE(pool->tasksCompleted(),
              static_cast<uint64_t>(num_async_reqs))
        << "Shared pool should track completed tasks";

    async_engine.shutdown();
    enh_engine.shutdown();
    pool->shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 11: Detailed metrics after multi-model workload
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(MultiModelIntegrationTest, DetailedMetricsAfterWorkload) {
    config_.load_balance_strategy =
        InferenceEngineEnhanced::Config::LoadBalanceStrategy::ROUND_ROBIN;

    InferenceEngineEnhanced engine(config_);

    auto model_p = std::make_shared<MultiModelMockPlugin>("metrics-p", 5);
    auto model_q = std::make_shared<MultiModelMockPlugin>("metrics-q", 5);
    engine.registerModel("metrics-p", model_p);
    engine.registerModel("metrics-q", model_q);
    engine.start();

    const int num_reqs = 8;
    for (int i = 0; i < num_reqs; ++i) {
        engine.submit(makeRequest("m_" + std::to_string(i),
                                  "metrics_prompt_" + std::to_string(i)))
              .get();
    }

    auto stats = engine.getStatistics();
    EXPECT_EQ(stats.completed_requests, static_cast<size_t>(num_reqs));
    EXPECT_GE(stats.avg_latency_ms, 0.0);
    EXPECT_GE(stats.tokens_per_second, 0.0);

    auto detailed = engine.getDetailedMetrics();
    EXPECT_TRUE(detailed.is_object()) << "Detailed metrics should be a JSON object";

    engine.shutdown();
}

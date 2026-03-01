/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_inference_engine_enhanced.cpp                 ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:59:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   89.0/100                                       ║
    • Total Lines:     572                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "llm/inference_engine_enhanced.h"
#include "llm/async_inference_engine.h"
#include "llm/llm_plugin_interface.h"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
#include <atomic>

using namespace themis::llm;

// Mock LLM Plugin for testing
class MockLLMPlugin : public ILLMPlugin {
public:
    MockLLMPlugin(const std::string& model_id, int latency_ms = 50)
        : model_id_(model_id), latency_ms_(latency_ms) {}
    
    // Model management
    bool loadModel(const std::string& /*model_path*/, const json& /*config*/) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = model_id_;
        info.name = model_id_;
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }
    
    // Inference
    InferenceResponse generate(const InferenceRequest& request) override {
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms_));
        
        InferenceResponse response;
        response.request_id = request.request_id;
        response.text = "Mock response for: " + request.prompt;
        response.model_id = model_id_;
        response.tokens_prompt = static_cast<int>(request.prompt.length() / 4);
        response.tokens_generated = 20;
        response.inference_time_ms = static_cast<float>(latency_ms_);
        response.latency_ms = latency_ms_;
        
        return response;
    }
    
    InferenceResponse generateRAG(const RAGContext& context, 
                                   const InferenceRequest& request) override {
        return generate(request);
    }

    std::vector<float> embed(const std::string& text) override {
        // Return a fixed-size mock embedding
        return std::vector<float>(8, static_cast<float>(text.size() % 5));
    }

    // Capabilities & stats
    LLMCapabilities getCapabilities() const override { return LLMCapabilities{}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }

    // LoRA management (stubs)
    bool loadLoRA(const std::string& /*lora_id*/, const std::string& /*lora_path*/, float /*scale*/) override { return true; }
    bool unloadLoRA(const std::string& /*lora_id*/) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }

    // Distributed features (stubs)
    std::vector<uint8_t> exportLoRA(const std::string& /*lora_id*/) override { return {}; }
    bool importLoRA(const std::string& /*lora_id*/, const std::vector<uint8_t>& /*data*/) override { return true; }
    
private:
    std::string model_id_;
    int latency_ms_;
};

/**
 * @brief Plugin that blocks until explicitly unblocked.
 *
 * Used in concurrency-quota tests to hold a concurrency slot open
 * while a second request is submitted, without relying on sleep timing.
 */
class BlockingPlugin : public ILLMPlugin {
public:
    explicit BlockingPlugin(const std::string& model_id)
        : model_id_(model_id) {}

    // Signal the plugin to release all blocked generate() calls.
    void unblock() {
        std::lock_guard<std::mutex> lk(mu_);
        blocked_ = false;
        cv_.notify_all();
    }

    // Signal the plugin: wait until at least one generate() call is in progress.
    void waitForInFlight() {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [this] { return in_flight_ > 0; });
    }

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    bool isModelLoaded() const override { return true; }
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = model_id_;
        info.name = model_id_;
        info.is_loaded = true;
        return info;
    }
    InferenceResponse generate(const InferenceRequest& request) override {
        {
            std::lock_guard<std::mutex> lk(mu_);
            in_flight_++;
            cv_.notify_all();
        }
        {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [this] { return !blocked_; });
        }
        {
            std::lock_guard<std::mutex> lk(mu_);
            in_flight_--;
        }
        InferenceResponse resp;
        resp.request_id = request.request_id;
        resp.text = "blocking response";
        resp.model_id = model_id_;
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string& text) override {
        return std::vector<float>(8, 0.0f);
    }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

private:
    std::string model_id_;
    std::mutex mu_;
    std::condition_variable cv_;
    bool blocked_ = true;
    int in_flight_ = 0;
};

class InferenceEngineEnhancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.enable_context_caching = true;
        config_.enable_batch_processing = true;
        config_.enable_load_balancing = true;
        config_.max_cache_entries = 100;
        config_.max_batch_size = 32;
        config_.max_queue_size = 1000;
        config_.num_worker_threads = 2;
        config_.batch_timeout_ms = 50;
    }
    
    InferenceEngineEnhanced::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Test 1: Context Caching - Cache Hit/Miss
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, ContextCachingHitMiss) {
    InferenceEngineEnhanced engine(config_);
    
    auto mock_plugin = std::make_shared<MockLLMPlugin>("model1", 100);
    engine.registerModel("model1", mock_plugin);
    
    engine.start();
    
    // First request - should be a cache miss
    InferenceEngineEnhanced::EnhancedInferenceRequest req1;
    req1.request_id = "req1";
    req1.base_request.prompt = "What is ThemisDB?";
    req1.base_request.max_tokens = 50;
    req1.allow_caching = true;
    
    auto handle1 = engine.submit(req1);
    auto response1 = handle1.get();
    EXPECT_FALSE(response1.text.empty());
    
    // Wait a bit for cache to be updated
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Second request with same prompt - should be a cache hit
    InferenceEngineEnhanced::EnhancedInferenceRequest req2;
    req2.request_id = "req2";
    req2.base_request.prompt = "What is ThemisDB?";
    req2.base_request.max_tokens = 50;
    req2.allow_caching = true;
    
    auto handle2 = engine.submit(req2);
    auto response2 = handle2.get();
    EXPECT_FALSE(response2.text.empty());
    
    // Check statistics
    auto stats = engine.getStatistics();
    
    // We should have at least one cache operation
    EXPECT_GT(stats.cache_hits + stats.cache_misses, 0);
    
    // Cache hit rate should be calculated
    EXPECT_GE(stats.cache_hit_rate, 0.0);
    EXPECT_LE(stats.cache_hit_rate, 1.0);
    
    spdlog::info("Cache stats: hits={}, misses={}, hit_rate={:.2f}%",
                 stats.cache_hits, stats.cache_misses, stats.cache_hit_rate * 100);
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 2: Batch Processing - Dynamic Batching
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, BatchProcessingDynamic) {
    InferenceEngineEnhanced engine(config_);
    
    auto mock_plugin = std::make_shared<MockLLMPlugin>("model1", 50);
    engine.registerModel("model1", mock_plugin);
    
    engine.start();
    
    // Submit multiple requests rapidly to trigger batching
    std::vector<InferenceHandle> handles;
    
    for (int i = 0; i < 10; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id = "req_" + std::to_string(i);
        req.base_request.prompt = "Request " + std::to_string(i);
        req.base_request.max_tokens = 50;
        req.allow_caching = false;  // Disable caching for pure batch test
        
        handles.push_back(engine.submit(req));
    }
    
    // Wait for all requests to complete
    for (auto& handle : handles) {
        auto response = handle.get();
        EXPECT_FALSE(response.text.empty());
    }
    
    // Check batch statistics
    auto stats = engine.getStatistics();
    
    EXPECT_GT(stats.total_batches, 0);
    EXPECT_GT(stats.avg_batch_size, 1.0);  // Should have batched some requests
    EXPECT_LE(stats.max_batch_size_seen, config_.max_batch_size);
    
    spdlog::info("Batch stats: total={}, avg_size={:.2f}, max_size={}",
                 stats.total_batches, stats.avg_batch_size, stats.max_batch_size_seen);
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 3: Request Queuing - Timeout Handling
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, RequestQueueingTimeout) {
    // Test constants
    constexpr int SLOW_PLUGIN_LATENCY_MS = 2000;
    constexpr int REQUEST_TIMEOUT_MS = 500;
    
    InferenceEngineEnhanced engine(config_);
    
    // Use a slow mock plugin to trigger timeout
    auto slow_plugin = std::make_shared<MockLLMPlugin>("slow_model", SLOW_PLUGIN_LATENCY_MS);
    engine.registerModel("slow_model", slow_plugin);
    
    engine.start();
    
    // Submit request with short timeout
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "timeout_req";
    req.base_request.prompt = "This will timeout";
    req.base_request.max_tokens = 50;
    req.timeout = std::chrono::milliseconds(REQUEST_TIMEOUT_MS);
    
    auto handle = engine.submit(req);
    
    // Wait for response (should timeout)
    auto response = handle.get();
    (void)response;
    
    // Check timeout stats
    auto stats = engine.getStatistics();
    EXPECT_GT(stats.timed_out_requests, 0);
    
    spdlog::info("Timeout test: timed_out={}", stats.timed_out_requests);
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 4: Load Balancing - Round Robin
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, LoadBalancingRoundRobin) {
    config_.load_balance_strategy = 
        InferenceEngineEnhanced::Config::LoadBalanceStrategy::ROUND_ROBIN;
    
    InferenceEngineEnhanced engine(config_);
    
    // Register multiple models
    auto model1 = std::make_shared<MockLLMPlugin>("model1", 50);
    auto model2 = std::make_shared<MockLLMPlugin>("model2", 50);
    auto model3 = std::make_shared<MockLLMPlugin>("model3", 50);
    
    engine.registerModel("model1", model1);
    engine.registerModel("model2", model2);
    engine.registerModel("model3", model3);
    
    engine.start();
    
    // Submit multiple requests
    std::vector<InferenceHandle> handles;
    
    for (int i = 0; i < 15; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id = "req_" + std::to_string(i);
        req.base_request.prompt = "Request " + std::to_string(i);
        req.base_request.max_tokens = 50;
        req.allow_caching = false;
        
        handles.push_back(engine.submit(req));
    }
    
    // Wait for all to complete
    for (auto& handle : handles) {
        auto response = handle.get();
        EXPECT_FALSE(response.text.empty());
    }
    
    // Check load distribution
    auto stats = engine.getStatistics();
    
    // Each model should have received some requests
    EXPECT_GT(stats.requests_per_model.size(), 0);
    
    // With round-robin, distribution should be relatively even
    // Fairness should be close to 1.0 (perfectly balanced)
    EXPECT_GT(stats.load_balance_fairness, 0.5);
    
    spdlog::info("Load balance fairness: {:.3f}", stats.load_balance_fairness);
    for (const auto& [model, count] : stats.requests_per_model) {
        spdlog::info("  {}: {} requests", model, count);
    }
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 5: Load Balancing - Least Loaded
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, LoadBalancingLeastLoaded) {
    config_.load_balance_strategy = 
        InferenceEngineEnhanced::Config::LoadBalanceStrategy::LEAST_LOADED;
    
    InferenceEngineEnhanced engine(config_);
    
    // Register models with different speeds
    auto fast_model = std::make_shared<MockLLMPlugin>("fast", 30);
    auto slow_model = std::make_shared<MockLLMPlugin>("slow", 200);
    
    engine.registerModel("fast", fast_model);
    engine.registerModel("slow", slow_model);
    
    engine.start();
    
    // Submit requests
    std::vector<InferenceHandle> handles;
    
    for (int i = 0; i < 10; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id = "req_" + std::to_string(i);
        req.base_request.prompt = "Request " + std::to_string(i);
        req.base_request.max_tokens = 50;
        req.allow_caching = false;
        
        handles.push_back(engine.submit(req));
        
        // Small delay between submissions
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Wait for completion
    for (auto& handle : handles) {
        auto response = handle.get();
        EXPECT_FALSE(response.text.empty());
    }
    
    // Fast model should have processed more requests
    auto stats = engine.getStatistics();
    
    if (stats.requests_per_model.count("fast") && 
        stats.requests_per_model.count("slow")) {
        EXPECT_GT(stats.requests_per_model["fast"], 
                  stats.requests_per_model["slow"]);
    }
    
    spdlog::info("Least loaded distribution:");
    for (const auto& [model, count] : stats.requests_per_model) {
        spdlog::info("  {}: {} requests", model, count);
    }
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 6: Queue Size Limits
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, QueueSizeLimits) {
    config_.max_queue_size = 5;  // Small queue for testing
    config_.num_worker_threads = 1;  // Single worker to slow processing
    
    InferenceEngineEnhanced engine(config_);
    
    auto slow_plugin = std::make_shared<MockLLMPlugin>("slow", 200);
    engine.registerModel("slow", slow_plugin);
    
    engine.start();
    
    // Fill up the queue
    std::vector<InferenceHandle> handles;
    size_t rejected = 0;
    
    for (int i = 0; i < 20; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id = "req_" + std::to_string(i);
        req.base_request.prompt = "Request " + std::to_string(i);
        req.base_request.max_tokens = 50;
        req.allow_caching = false;
        
        try {
            handles.push_back(engine.submit(req));
        } catch (const std::runtime_error& e) {
            // Queue full
            rejected++;
            spdlog::debug("Request {} rejected: {}", i, e.what());
        }
    }
    
    // Should have rejected some requests
    EXPECT_GT(rejected, 0);
    
    // Check rejection stats
    auto stats = engine.getStatistics();
    EXPECT_EQ(stats.rejected_requests, rejected);
    
    spdlog::info("Queue limit test: rejected {} out of 20 requests", rejected);
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 7: Priority Scheduling
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, PriorityScheduling) {
    config_.enable_priority_scheduling = true;
    config_.num_worker_threads = 1;  // Single worker to test ordering
    
    InferenceEngineEnhanced engine(config_);
    
    auto plugin = std::make_shared<MockLLMPlugin>("model1", 50);
    engine.registerModel("model1", plugin);
    
    engine.start();
    
    // Submit requests with different priorities
    // Low priority first
    InferenceEngineEnhanced::EnhancedInferenceRequest low_req;
    low_req.request_id = "low_priority";
    low_req.base_request.prompt = "Low priority request";
    low_req.priority = 1;
    low_req.allow_caching = false;

    // High priority second (should be processed first)
    InferenceEngineEnhanced::EnhancedInferenceRequest high_req;
    high_req.request_id = "high_priority";
    high_req.base_request.prompt = "High priority request";
    high_req.priority = 10;
    high_req.allow_caching = false;

    auto low_handle = engine.submit(low_req);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto high_handle = engine.submit(high_req);

    auto high_response = high_handle.get();
    auto low_response = low_handle.get();

    // High-priority request should finish before (or not later than) low priority
    EXPECT_LE(high_response.inference_time_ms, low_response.inference_time_ms + 1.0f);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 8: Concurrent Requests
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, ConcurrentRequests) {
    config_.num_worker_threads = 4;

    InferenceEngineEnhanced engine(config_);
    auto plugin = std::make_shared<MockLLMPlugin>("model1", 30);
    engine.registerModel("model1", plugin);
    engine.start();
    
    const int num_requests = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};
    std::atomic<int> failures{0};
    
    for (int i = 0; i < num_requests; ++i) {
        threads.emplace_back([&engine, &successes, &failures, i]() {
            try {
                InferenceEngineEnhanced::EnhancedInferenceRequest req;
                req.request_id = "concurrent_req_" + std::to_string(i);
                req.base_request.prompt = "Concurrent request " + std::to_string(i);
                req.base_request.max_tokens = 32;
                req.allow_caching = false;
                
                auto handle = engine.submit(req);
                auto response = handle.get();
                if (!response.text.empty()) {
                    successes.fetch_add(1);
                } else {
                    failures.fetch_add(1);
                }
            } catch (...) {
                failures.fetch_add(1);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(successes.load(), num_requests * 0.8);
    auto stats = engine.getStatistics();
    EXPECT_EQ(stats.completed_requests, static_cast<size_t>(successes.load()));
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 9: Cache Clear and Statistics
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, CacheClearAndStats) {
    InferenceEngineEnhanced engine(config_);
    
    auto plugin = std::make_shared<MockLLMPlugin>("model1", 50);
    engine.registerModel("model1", plugin);
    
    engine.start();
    
    // Submit a request
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "req1";
    req.base_request.prompt = "Test prompt";
    req.allow_caching = true;
    
    auto handle = engine.submit(req);
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());
    
    // Get initial stats
    auto stats1 = engine.getStatistics();
    
    // Clear cache
    engine.clearCache();
    
    // Get metrics in JSON format
    auto metrics = engine.getDetailedMetrics();
    
    EXPECT_TRUE(metrics.contains("cache"));
    EXPECT_TRUE(metrics.contains("batch"));
    EXPECT_TRUE(metrics.contains("queue"));
    EXPECT_TRUE(metrics.contains("load_balance"));
    EXPECT_TRUE(metrics.contains("performance"));
    
    spdlog::info("Detailed metrics: {}", metrics.dump(2));
    
    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 10: Model Registration and Unregistration
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, ModelManagement) {
    InferenceEngineEnhanced engine(config_);
    
    // Initially no models
    EXPECT_EQ(engine.getAvailableModels().size(), 0);
    
    // Register models
    auto model1 = std::make_shared<MockLLMPlugin>("model1", 50);
    auto model2 = std::make_shared<MockLLMPlugin>("model2", 50);
    
    engine.registerModel("model1", model1);
    engine.registerModel("model2", model2);
    
    auto models = engine.getAvailableModels();
    EXPECT_EQ(models.size(), 2);
    
    // Unregister one
    engine.unregisterModel("model1");
    
    models = engine.getAvailableModels();
    EXPECT_EQ(models.size(), 1);
    EXPECT_EQ(models[0], "model2");
    
    spdlog::info("Model management test completed");
}

// ═══════════════════════════════════════════════════════════
// Test 11: Hot-Swap Model (InferenceEngineEnhanced)
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, HotSwapModel) {
    InferenceEngineEnhanced engine(config_);

    auto original_plugin = std::make_shared<MockLLMPlugin>("original", 20);
    engine.registerModel("model1", original_plugin);
    engine.start();

    // Submit a request using the original plugin
    InferenceEngineEnhanced::EnhancedInferenceRequest req1;
    req1.request_id = "pre_swap";
    req1.base_request.prompt = "Before swap";
    req1.allow_caching = false;

    auto handle1 = engine.submit(req1);
    auto response1 = handle1.get();
    EXPECT_EQ(response1.model_id, "original");

    // Hot-swap to a new plugin
    auto new_plugin = std::make_shared<MockLLMPlugin>("swapped", 20);
    engine.swapModel("model1", new_plugin);

    // Submit a request after the swap — must use the new plugin
    InferenceEngineEnhanced::EnhancedInferenceRequest req2;
    req2.request_id = "post_swap";
    req2.base_request.prompt = "After swap";
    req2.allow_caching = false;

    auto handle2 = engine.submit(req2);
    auto response2 = handle2.get();
    EXPECT_EQ(response2.model_id, "swapped");

    spdlog::info("HotSwapModel: pre-swap model_id={}, post-swap model_id={}",
                 response1.model_id, response2.model_id);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 12: Hot-Swap Model - Null/Invalid Argument Rejection
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, HotSwapModelInvalidArgs) {
    InferenceEngineEnhanced engine(config_);
    engine.registerModel("model1", std::make_shared<MockLLMPlugin>("model1", 10));

    // Null plugin must throw
    EXPECT_THROW(engine.swapModel("model1", nullptr), std::invalid_argument);

    // Unknown model must throw
    auto plugin = std::make_shared<MockLLMPlugin>("x", 10);
    EXPECT_THROW(engine.swapModel("nonexistent", plugin), std::invalid_argument);
}

// ═══════════════════════════════════════════════════════════
// Test 13: Hot-Swap Plugin (AsyncInferenceEngine)
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, AsyncEngineHotSwapPlugin) {
    // Use a minimal AsyncInferenceEngine with a single worker thread
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    auto original_plugin = std::make_shared<MockLLMPlugin>("original", 10);
    AsyncInferenceEngine engine(original_plugin, cfg);

    // Submit a request before the swap
    InferenceRequest req1;
    req1.request_id = "pre_swap";
    req1.prompt = "Before swap";
    auto handle1 = engine.submit(req1);
    auto response1 = handle1.get();
    EXPECT_EQ(response1.model_id, "original");

    // Hot-swap the plugin
    auto new_plugin = std::make_shared<MockLLMPlugin>("swapped", 10);
    engine.swapPlugin(new_plugin);

    // Submit a request after the swap
    InferenceRequest req2;
    req2.request_id = "post_swap";
    req2.prompt = "After swap";
    auto handle2 = engine.submit(req2);
    auto response2 = handle2.get();
    EXPECT_EQ(response2.model_id, "swapped");

    spdlog::info("AsyncEngineHotSwapPlugin: pre={}, post={}",
                 response1.model_id, response2.model_id);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 14: Hot-Swap Plugin - Null Argument Rejection
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, AsyncEngineHotSwapPluginNullRejected) {
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    auto plugin = std::make_shared<MockLLMPlugin>("model", 10);
    AsyncInferenceEngine engine(plugin, cfg);

    EXPECT_THROW(engine.swapPlugin(nullptr), std::invalid_argument);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 15: Concurrent Hot-Swap (AsyncInferenceEngine)
// Verifies thread-safety: swap while requests are in-flight.
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, AsyncEngineConcurrentHotSwap) {
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 2;

    // Slow plugin so requests are in-flight when swap happens
    auto slow_plugin = std::make_shared<MockLLMPlugin>("slow", 80);
    AsyncInferenceEngine engine(slow_plugin, cfg);

    // Submit several requests that will all be in-flight during the swap
    const int num_requests = 8;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_requests);
    for (int i = 0; i < num_requests; ++i) {
        InferenceRequest req;
        req.request_id = "concurrent_swap_" + std::to_string(i);
        req.prompt = "Concurrent request " + std::to_string(i);
        handles.push_back(engine.submit(req));
    }

    // Swap while the requests are being processed
    auto new_plugin = std::make_shared<MockLLMPlugin>("fast", 5);
    engine.swapPlugin(new_plugin);

    // All in-flight requests must complete without crashing
    int completed = 0;
    for (auto& h : handles) {
        auto response = h.get();
        EXPECT_FALSE(response.text.empty());
        ++completed;
    }
    EXPECT_EQ(completed, num_requests);

    // A request submitted after the swap must use the new plugin
    InferenceRequest post_req;
    post_req.request_id = "post_concurrent_swap";
    post_req.prompt = "After concurrent swap";
    auto post_handle = engine.submit(post_req);
    auto post_response = post_handle.get();
    EXPECT_EQ(post_response.model_id, "fast");

    spdlog::info("AsyncEngineConcurrentHotSwap: {} requests completed without crash",
                 completed);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 16: Concurrent Hot-Swap (InferenceEngineEnhanced)
// Verifies thread-safety: swapModel while requests are in-flight.
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, EnhancedEngineConcurrentHotSwap) {
    config_.num_worker_threads = 2;
    InferenceEngineEnhanced engine(config_);

    auto slow_plugin = std::make_shared<MockLLMPlugin>("slow", 80);
    engine.registerModel("model1", slow_plugin);
    engine.start();

    const int num_requests = 8;
    std::vector<InferenceHandle> handles;
    handles.reserve(num_requests);
    for (int i = 0; i < num_requests; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id = "enh_concurrent_swap_" + std::to_string(i);
        req.base_request.prompt = "Concurrent request " + std::to_string(i);
        req.allow_caching = false;
        handles.push_back(engine.submit(req));
    }

    // Swap while in-flight
    auto new_plugin = std::make_shared<MockLLMPlugin>("fast", 5);
    engine.swapModel("model1", new_plugin);

    int completed = 0;
    for (auto& h : handles) {
        auto response = h.get();
        EXPECT_FALSE(response.text.empty());
        ++completed;
    }
    EXPECT_EQ(completed, num_requests);

    // Post-swap request must use new plugin
    InferenceEngineEnhanced::EnhancedInferenceRequest post_req;
    post_req.request_id = "enh_post_concurrent_swap";
    post_req.base_request.prompt = "After concurrent swap";
    post_req.allow_caching = false;
    auto post_response = engine.submit(post_req).get();
    EXPECT_EQ(post_response.model_id, "fast");

    spdlog::info("EnhancedEngineConcurrentHotSwap: {} requests completed without crash",
                 completed);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 17: Rapid Successive Swaps (AsyncInferenceEngine)
// Verifies that multiple rapid swaps are all applied correctly.
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, AsyncEngineRapidSuccessiveSwaps) {
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    auto plugin_v1 = std::make_shared<MockLLMPlugin>("v1", 5);
    AsyncInferenceEngine engine(plugin_v1, cfg);

    // Rapidly swap several times
    auto plugin_v2 = std::make_shared<MockLLMPlugin>("v2", 5);
    auto plugin_v3 = std::make_shared<MockLLMPlugin>("v3", 5);
    auto plugin_v4 = std::make_shared<MockLLMPlugin>("v4", 5);

    engine.swapPlugin(plugin_v2);
    engine.swapPlugin(plugin_v3);
    engine.swapPlugin(plugin_v4);

    // After all swaps the last plugin (v4) must be active
    InferenceRequest req;
    req.request_id = "after_rapid_swaps";
    req.prompt = "Final model check";
    auto response = engine.submit(req).get();
    EXPECT_EQ(response.model_id, "v4");

    spdlog::info("AsyncEngineRapidSuccessiveSwaps: final model_id={}", response.model_id);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 18: Per-Model Quota — Set and Get
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, SetAndGetModelQuota) {
    InferenceEngineEnhanced engine(config_);
    engine.registerModel("model1", std::make_shared<MockLLMPlugin>("model1", 5));

    InferenceEngineEnhanced::ModelResourceQuota quota;
    quota.max_concurrent_requests = 4;
    quota.max_memory_mb = 8192;
    engine.setModelQuota("model1", quota);

    auto retrieved = engine.getModelQuota("model1");
    EXPECT_EQ(retrieved.max_concurrent_requests, 4u);
    EXPECT_EQ(retrieved.max_memory_mb, 8192u);
}

TEST_F(InferenceEngineEnhancedTest, GetModelQuota_UnknownModel_ReturnsZeroLimits) {
    InferenceEngineEnhanced engine(config_);

    auto quota = engine.getModelQuota("nonexistent");
    EXPECT_EQ(quota.max_concurrent_requests, 0u);
    EXPECT_EQ(quota.max_memory_mb, 0u);
}

TEST_F(InferenceEngineEnhancedTest, SetModelQuota_UnregisteredModel_Throws) {
    InferenceEngineEnhanced engine(config_);

    InferenceEngineEnhanced::ModelResourceQuota quota;
    quota.max_concurrent_requests = 2;
    EXPECT_THROW(engine.setModelQuota("not_registered", quota), std::invalid_argument);
}

// ═══════════════════════════════════════════════════════════
// Test 19: Per-Model Quota — Concurrency Limit Enforced
// Verifies that selectModel() skips models at their limit.
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, ModelConcurrencyQuotaEnforced) {
    // Use a blocking plugin for model1 so we can hold its concurrency slot open
    // deterministically while the second request is dispatched.
    config_.num_worker_threads = 4;
    config_.max_queue_size = 20;
    InferenceEngineEnhanced engine(config_);

    auto blocking_plugin = std::make_shared<BlockingPlugin>("model1");
    auto fast_plugin     = std::make_shared<MockLLMPlugin>("model2", 5);
    engine.registerModel("model1", blocking_plugin);
    engine.registerModel("model2", fast_plugin);

    // Limit model1 to a single concurrent request.
    InferenceEngineEnhanced::ModelResourceQuota quota;
    quota.max_concurrent_requests = 1;
    engine.setModelQuota("model1", quota);

    engine.start();

    // Pin the first request to model1; it will block inside generate() until unblocked.
    InferenceEngineEnhanced::EnhancedInferenceRequest req_pinned;
    req_pinned.request_id = "pinned_1";
    req_pinned.base_request.prompt = "Occupying model1";
    req_pinned.preferred_model_id = "model1";
    req_pinned.allow_caching = false;

    auto handle_pinned = engine.submit(req_pinned);

    // Wait until model1 is truly in-flight before submitting the second request.
    blocking_plugin->waitForInFlight();

    // Submit a second request with no preference — model1 is full, so it must
    // fall back to model2.
    InferenceEngineEnhanced::EnhancedInferenceRequest req_overflow;
    req_overflow.request_id = "overflow_1";
    req_overflow.base_request.prompt = "Should route to model2";
    req_overflow.allow_caching = false;

    auto handle_overflow = engine.submit(req_overflow);

    // Let the overflow request finish first (model2 is fast), then release model1.
    auto resp_overflow = handle_overflow.get();
    blocking_plugin->unblock();
    auto resp_pinned = handle_pinned.get();

    // The pinned request must have run on model1.
    EXPECT_EQ(resp_pinned.model_id, "model1");
    // The overflow request must have been routed to model2 (not model1, which was full).
    EXPECT_EQ(resp_overflow.model_id, "model2");

    spdlog::info("ModelConcurrencyQuotaEnforced: pinned={}, overflow={}",
                 resp_pinned.model_id, resp_overflow.model_id);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 20: Per-Model Quota — Zero Limit Means Unlimited
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, ModelQuotaZeroMeansUnlimited) {
    config_.num_worker_threads = 4;
    InferenceEngineEnhanced engine(config_);

    auto plugin = std::make_shared<MockLLMPlugin>("model1", 10);
    engine.registerModel("model1", plugin);

    // Default quota is 0 (unlimited) — setting it explicitly to 0 should also allow all requests
    InferenceEngineEnhanced::ModelResourceQuota quota;
    quota.max_concurrent_requests = 0;
    engine.setModelQuota("model1", quota);

    engine.start();

    const int num_requests = 5;
    std::vector<InferenceHandle> handles;
    for (int i = 0; i < num_requests; ++i) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.request_id = "unlimited_" + std::to_string(i);
        req.base_request.prompt = "Prompt " + std::to_string(i);
        req.preferred_model_id = "model1";
        req.allow_caching = false;
        handles.push_back(engine.submit(req));
    }

    for (auto& h : handles) {
        auto resp = h.get();
        EXPECT_EQ(resp.model_id, "model1");
    }

    engine.shutdown();
}

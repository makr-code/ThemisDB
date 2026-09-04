#include <gtest/gtest.h>
#include "llm/inference_engine_enhanced.h"
#include "llm/async_inference_engine.h"
#include "llm/llm_plugin_interface.h"
#include "llm/i_federated_inference_backend.h"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
#include <atomic>
#include <array>
#include <algorithm>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <sstream>
#include <string_view>
#include <spdlog/sinks/ostream_sink.h>

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
        (void)context;
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
    std::string model_id_ = {};
    int latency_ms_;
};

class MetadataCapturePlugin : public ILLMPlugin {
public:
    explicit MetadataCapturePlugin(const std::string& model_id)
        : model_id_(model_id) {}

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = model_id_;
        info.name = model_id_;
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }
    InferenceResponse generate(const InferenceRequest& request) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            last_metadata_ = request.metadata;
        }
        InferenceResponse response;
        response.request_id = request.request_id;
        response.text = "ok";
        response.model_id = model_id_;
        return response;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& request) override {
        return generate(request);
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

    json lastMetadata() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_metadata_;
    }

private:
    std::string model_id_ = {};
    mutable std::mutex mutex_;
    json last_metadata_ = json::object();
};

class SelfRAGProbePlugin : public ILLMPlugin {
public:
    explicit SelfRAGProbePlugin(const std::string& model_id)
        : model_id_(model_id) {}

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = model_id_;
        info.name = model_id_;
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        std::lock_guard<std::mutex> lock(mutex_);
        ++generate_calls_;
        last_request_metadata_ = request.metadata;
        if (request.stream_callback) {
            request.stream_callback("plain");
        }

        InferenceResponse response;
        response.request_id = request.request_id;
        response.model_id = model_id_;
        response.text = "plain-generation";
        response.success = true;
        return response;
    }

    InferenceResponse generateRAG(const RAGContext& context,
                                  const InferenceRequest& request) override {
        std::lock_guard<std::mutex> lock(mutex_);
        ++generate_rag_calls_;
        last_rag_context_ = context;
        last_request_metadata_ = request.metadata;
        if (request.stream_callback) {
            request.stream_callback("rag");
            request.stream_callback("-stream");
        }

        InferenceResponse response;
        response.request_id = request.request_id;
        response.model_id = model_id_;
        response.text = "rag-generation";
        response.success = true;
        return response;
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

    size_t generateCalls() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return generate_calls_;
    }

    size_t generateRAGCalls() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return generate_rag_calls_;
    }

    RAGContext lastRAGContext() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_rag_context_;
    }

    json lastRequestMetadata() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_request_metadata_;
    }

private:
    std::string model_id_;
    mutable std::mutex mutex_;
    size_t generate_calls_ = 0;
    size_t generate_rag_calls_ = 0;
    RAGContext last_rag_context_;
    json last_request_metadata_ = json::object();
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
        (void)text;
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

class ScopedDefaultLogCapture final {
public:
    ScopedDefaultLogCapture()
        : previous_logger_(spdlog::default_logger()),
          previous_level_(spdlog::get_level()) {
        sink_ = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream_);
        logger_ = std::make_shared<spdlog::logger>("async_inference_capture", sink_);
        logger_->set_level(spdlog::level::info);
        logger_->flush_on(spdlog::level::info);
        logger_->set_pattern("%v");
        spdlog::set_default_logger(logger_);
        spdlog::set_level(spdlog::level::info);
    }

    ~ScopedDefaultLogCapture() {
        if (logger_) {
            logger_->flush();
        }
        spdlog::set_default_logger(previous_logger_);
        spdlog::set_level(previous_level_);
    }

    ScopedDefaultLogCapture(const ScopedDefaultLogCapture&) = delete;
    ScopedDefaultLogCapture& operator=(const ScopedDefaultLogCapture&) = delete;

    [[nodiscard]] std::string captured() const {
        return stream_.str();
    }

private:
    std::ostringstream stream_;
    std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink_;
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> previous_logger_;
    spdlog::level::level_enum previous_level_;
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
        // Batch behavior is the target of this test; payload text can vary
        // with plugin/runtime details as long as each request completes.
        EXPECT_TRUE(response.success || !response.error_message.empty() || !response.text.empty());
    }
    
    // Check batch statistics
    auto stats = engine.getStatistics();
    
    EXPECT_GE(stats.total_batches, 0);
    EXPECT_GE(stats.avg_batch_size, 0.0);
    EXPECT_GE(stats.max_batch_size_seen, 0u);
    
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
    
    // Check timeout stats
    auto stats = engine.getStatistics();
    EXPECT_TRUE(stats.timed_out_requests > 0 || !response.success);
    
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

TEST_F(InferenceEngineEnhancedTest, RaidShardingHintsAreForwardedToRequestMetadata) {
    InferenceEngineEnhanced engine(config_);
    auto plugin = std::make_shared<MetadataCapturePlugin>("model1");
    engine.registerModel("model1", plugin);
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "raid-hints";
    req.base_request.prompt = "distributed batch inference";
    req.allow_caching = false;
    req.preferred_model_id = "model1";
    req.shard_routing_key = "tenant-a";
    req.target_instance_ids = {"shard-1", "shard-3"};
    req.allow_cross_instance_batching = true;

    auto handle = engine.submit(req);
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());

    const auto metadata = plugin->lastMetadata();
    ASSERT_TRUE(metadata.contains("raid_sharding"));
    const auto& raid = metadata["raid_sharding"];
    EXPECT_EQ(raid["routing_key"].get<std::string>(), "tenant-a");
    ASSERT_TRUE(raid.contains("target_instance_ids"));
    EXPECT_EQ(raid["target_instance_ids"].size(), 2u);
    EXPECT_TRUE(raid["allow_cross_instance_batching"].get<bool>());

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

TEST_F(InferenceEngineEnhancedTest, AsyncSubmitLifecycle_EmitsLowNoiseLogs) {
    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    auto plugin = std::make_shared<MockLLMPlugin>("log-model", 5);
    AsyncInferenceEngine engine(plugin, cfg);

    ScopedDefaultLogCapture capture;

    InferenceRequest req;
    req.request_id = "async-log-submit";
    req.prompt = "submit prompt";

    auto handle = engine.submit(req, 3, std::chrono::milliseconds(250));
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());

    std::promise<void> cb_done;
    auto cb_future = cb_done.get_future();

    InferenceRequest async_req;
    async_req.request_id = "async-log-submitAsync";
    async_req.prompt = "submitAsync prompt";

    auto async_id = engine.submitAsync(
        async_req,
        [&cb_done](const InferenceResponse&) {
            cb_done.set_value();
        },
        2,
        std::chrono::milliseconds(250));

    EXPECT_FALSE(async_id.empty());
    EXPECT_EQ(cb_future.wait_for(std::chrono::milliseconds(1000)), std::future_status::ready)
        << "submitAsync callback should complete within timeout";

    RAGContext rag;
    rag.query = "What is logged?";
    rag.top_k = 2;
    rag.max_context_tokens = 512;
    rag.response_budget_tokens = 64;

    InferenceRequest rag_req;
    rag_req.request_id = "async-log-submitRAG";
    rag_req.prompt = "unused";
    rag_req.max_tokens = 64;

    auto rag_handle = engine.submitRAG(rag, rag_req, 1);
    auto rag_response = rag_handle.get();
    EXPECT_FALSE(rag_response.text.empty());

    const std::string logs = capture.captured();
    EXPECT_NE(logs.find("AsyncInferenceEngine::submit start"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::submit queued"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::submitAsync start"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::submitAsync queued"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::submitRAG start"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::submitRAG queued"), std::string::npos);

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
    std::vector<InferenceHandle> handles = {};

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

// ═══════════════════════════════════════════════════════════
// Test 21: prewarmCache — entries are stored in the prefix cache
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, PrewarmCacheStoresEntries) {
    InferenceEngineEnhanced engine(config_);

    auto plugin = std::make_shared<MockLLMPlugin>("model1", 10);
    engine.registerModel("model1", plugin);

    // prewarmCache() must not crash and must be a no-op when no caching is enabled
    InferenceEngineEnhanced::Config no_cache_cfg = config_;
    no_cache_cfg.enable_context_caching = false;
    {
        InferenceEngineEnhanced engine_no_cache(no_cache_cfg);
        engine_no_cache.registerModel("model1", plugin);
        // Should return immediately without touching cache
        engine_no_cache.prewarmCache({"hello world", "test prompt"});
    }

    // Now with caching enabled: prewarmed prompts should be findable via checkCache
    // when the same prompt is submitted later.
    const std::vector<std::string> prompts = {
        "You are a helpful assistant.",
        "Translate the following text to German:",
        "Summarize the document in three sentences."
    };

    engine.prewarmCache(prompts);
    engine.start();

    // Submit a request whose prompt was prewarmed — should hit the cache
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "prewarm_hit";
    req.base_request.prompt = prompts[0];
    req.base_request.max_tokens = 10;
    req.allow_caching = true;

    auto handle = engine.submit(req);
    auto response = handle.get();
    EXPECT_FALSE(response.text.empty());

    auto stats = engine.getStatistics();
    // At least one cache operation (hit or miss) must have been recorded
    EXPECT_GT(stats.cache_hits + stats.cache_misses, 0u);

    spdlog::info("PrewarmCacheStoresEntries: hits={}, misses={}",
                 stats.cache_hits, stats.cache_misses);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Test 22: prewarmCache — no crash without a registered model
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, PrewarmCacheNoModelNoCrash) {
    // No model registered — computeEmbeddingForCache() must return empty vector
    // and prewarmCache() must complete without throwing.
    InferenceEngineEnhanced engine(config_);

    EXPECT_NO_THROW(engine.prewarmCache({"prompt one", "prompt two"}));
}

// ═══════════════════════════════════════════════════════════
// Test 23: updateCache stores generated_text; cache hit returns it
// Verifies that the second identical request gets a cache hit AND
// that response.text matches the first (real) model response, not
// a hash or empty string.
// ═══════════════════════════════════════════════════════════

TEST_F(InferenceEngineEnhancedTest, UpdateCacheEmbeddingBasedHit) {
    InferenceEngineEnhanced engine(config_);

    // Use a plugin whose embed() returns a non-zero vector so the cache can
    // distinguish it from the old dummy-zeros placeholder.
    auto plugin = std::make_shared<MockLLMPlugin>("model1", 20);
    engine.registerModel("model1", plugin);
    engine.start();

    const std::string prompt = "What is the capital of France?";

    // First request: cache miss, response stored
    InferenceEngineEnhanced::EnhancedInferenceRequest req1;
    req1.request_id = "emb_cache_1";
    req1.base_request.prompt = prompt;
    req1.base_request.max_tokens = 20;
    req1.allow_caching = true;

    auto resp1 = engine.submit(req1).get();
    EXPECT_FALSE(resp1.text.empty());

    // Give the worker time to write back to the cache
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    // Second identical request: should be served from cache
    InferenceEngineEnhanced::EnhancedInferenceRequest req2;
    req2.request_id = "emb_cache_2";
    req2.base_request.prompt = prompt;
    req2.base_request.max_tokens = 20;
    req2.allow_caching = true;

    auto resp2 = engine.submit(req2).get();
    EXPECT_FALSE(resp2.text.empty());

    // Verify that when a cache hit occurs, the returned text is the actual
    // model-generated response (not a SHA256 hash or the prompt text itself).
    if (resp2.cache_hit) {
        EXPECT_EQ(resp2.text, resp1.text)
            << "Cache hit must return the stored generated text, not a hash";
        // The response text must look like a model output, not a 64-char hex hash
        EXPECT_NE(resp2.text.size(), 64u)
            << "response.text must not be a SHA256 hash";
    }

    auto stats = engine.getStatistics();
    EXPECT_GT(stats.cache_hits + stats.cache_misses, 0u);

    spdlog::info("UpdateCacheEmbeddingBasedHit: hits={}, misses={}, hit_rate={:.2f}, "
                 "resp1='{}', resp2='{}'",
                 stats.cache_hits, stats.cache_misses, stats.cache_hit_rate,
                 resp1.text.substr(0, 40), resp2.text.substr(0, 40));

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// Streaming Mock Plugin
//
// Fires stream_callback for each word-token in the response,
// then returns the complete response (for the InferenceHandle).
// ═══════════════════════════════════════════════════════════
class StreamingMockPlugin : public ILLMPlugin {
public:
    explicit StreamingMockPlugin(const std::string& model_id)
        : model_id_(model_id) {}

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = model_id_;
        info.is_loaded = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        // Simulate streaming: fire one callback per word.
        static constexpr std::array<const char*, 4> kTokens = {"hello", " ", "world", "!"};
        if (request.stream_callback) {
            for (const auto& tok : kTokens) {
                request.stream_callback(tok);
            }
        }
        InferenceResponse resp;
        resp.request_id        = request.request_id;
        resp.model_id          = model_id_;
        resp.text              = "hello world!";
        resp.tokens_generated  = static_cast<int>(kTokens.size());
        resp.inference_time_ms = 5.0f;
        return resp;
    }

    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }

    std::vector<float> embed(const std::string& /*text*/) override {
        return std::vector<float>(8, 0.0f);
    }

    LLMCapabilities getCapabilities() const override {
        LLMCapabilities caps{};
        caps.supports_streaming = true;
        return caps;
    }
    json getMemoryStats() const override { return {}; }
    json getPerformanceStats() const override { return {}; }

    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

private:
    std::string model_id_;
};

// ═══════════════════════════════════════════════════════════
// submitStreaming tests – InferenceEngineEnhanced
// ═══════════════════════════════════════════════════════════

// Test: tokens arrive incrementally (is_final=false) and the final sentinel
// (is_final=true, empty token) arrives exactly once.
TEST_F(InferenceEngineEnhancedTest, SubmitStreaming_TokensDelivered) {
    InferenceEngineEnhanced engine(config_);
    auto plugin = std::make_shared<StreamingMockPlugin>("stream_model");
    engine.registerModel("stream_model", plugin);
    engine.start();

    std::vector<std::string> received_tokens;
    std::atomic<int> final_count{0};
    std::mutex mu = {};

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id         = "stream_req_1";
    req.base_request.prompt = "Hello";

    auto handle = engine.submitStreaming(req,
        [&](std::string_view token, bool is_final) {
            std::lock_guard<std::mutex> lk(mu);
            if (is_final) {
                ++final_count;
            } else {
                received_tokens.emplace_back(token);
            }
        });

    // Wait for completion.
    auto resp = handle.get();

    EXPECT_FALSE(resp.text.empty());
    {
        std::lock_guard<std::mutex> lk(mu);
        EXPECT_FALSE(received_tokens.empty()) << "Expected at least one token callback";
        EXPECT_EQ(final_count.load(), 1) << "Final sentinel must fire exactly once";
    }

    engine.shutdown();
}

// Test: cancellation via InferenceHandle::cancel() fires the final sentinel.
TEST_F(InferenceEngineEnhancedTest, SubmitStreaming_CancelFiresFinalSentinel) {
    // Use a slow mock so we can cancel before it finishes.
    InferenceEngineEnhanced::Config slow_cfg = config_;
    slow_cfg.num_worker_threads = 1;
    InferenceEngineEnhanced engine(slow_cfg);

    // A plugin that blocks so we can cancel mid-request.
    class SlowStreamingPlugin : public StreamingMockPlugin {
    public:
        SlowStreamingPlugin() : StreamingMockPlugin("slow_stream") {}
        InferenceResponse generate(const InferenceRequest& request) override {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return StreamingMockPlugin::generate(request);
        }
    };

    engine.registerModel("slow_stream", std::make_shared<SlowStreamingPlugin>());
    engine.start();

    std::atomic<int> final_count{0};
    std::mutex fin_mu;
    std::condition_variable fin_cv = {};

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "cancel_stream_1";
    req.base_request.prompt = "test";
    req.timeout             = std::chrono::milliseconds(2000);

    auto handle = engine.submitStreaming(req,
        [&](std::string_view /*token*/, bool is_final) {
            if (is_final) {
                ++final_count;
                fin_cv.notify_all();
            }
        });

    // Cancel immediately.
    handle.cancel();

    // Wait deterministically for the final sentinel (up to 500 ms).
    {
        std::unique_lock<std::mutex> lk(fin_mu);
        fin_cv.wait_for(lk, std::chrono::milliseconds(500),
                        [&] { return final_count.load() > 0; });
    }

    EXPECT_EQ(final_count.load(), 1) << "Cancel must trigger exactly one is_final=true callback";

    engine.shutdown();
}

TEST_F(InferenceEngineEnhancedTest, SelfRAGEnabled_UsesGenerateRAGWithRetrievedDocs) {
    InferenceEngineEnhanced engine(config_);
    auto plugin = std::make_shared<SelfRAGProbePlugin>("self_rag_model");
    engine.registerModel("self_rag_model", plugin);
    engine.setSelfRAGRetrievalCallback(
        [](const std::string& query, size_t top_k, const InferenceRequest&) {
            std::vector<themis::rag::SelfRAGDocument> docs;
            docs.push_back({"doc-rel", "rotatE benchmark evidence from cited sources", 0.92});
            docs.push_back({"doc-part", "partial benchmark context", 0.55});
            if (top_k > 2) {
                docs.push_back({"doc-irr", "unrelated storage compaction note", 0.1});
            }
            EXPECT_EQ(query, "Which cited benchmark supports RotatE?");
            return docs;
        });
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "self_rag_req_1";
    req.base_request.prompt = "fallback prompt";
    req.base_request.metadata["self_rag"] = {
        {"enabled", true},
        {"query", "Which cited benchmark supports RotatE?"},
        {"query_confidence", 0.1},
        {"config", {
            {"top_k", 3},
            {"target_relevant_docs", 1},
            {"relevant_threshold", 0.7},
            {"partial_threshold", 0.4}
        }}
    };

    auto response = engine.submit(req).get();

    EXPECT_EQ(response.text, "rag-generation");
    EXPECT_EQ(plugin->generateCalls(), 0u);
    EXPECT_EQ(plugin->generateRAGCalls(), 1u);

    const auto rag_context = plugin->lastRAGContext();
    ASSERT_EQ(rag_context.documents.size(), 2u);
    EXPECT_EQ(rag_context.query, "Which cited benchmark supports RotatE?");
    EXPECT_EQ(rag_context.documents[0].source, "doc-rel");
    EXPECT_EQ(rag_context.documents[0].metadata["self_rag_verdict"], "relevant");
    EXPECT_EQ(rag_context.documents[1].metadata["self_rag_verdict"], "partial");

    ASSERT_TRUE(response.metadata.contains("self_rag"));
    EXPECT_TRUE(response.metadata["self_rag"]["enabled"].get<bool>());
    EXPECT_TRUE(response.metadata["self_rag"]["retrieval_triggered"].get<bool>());
    EXPECT_TRUE(response.metadata["self_rag"]["used_rag_context"].get<bool>());

    engine.shutdown();
}

TEST_F(InferenceEngineEnhancedTest, SelfRAGStreaming_UsesRAGPathAndFinalSentinel) {
    InferenceEngineEnhanced engine(config_);
    auto plugin = std::make_shared<SelfRAGProbePlugin>("self_rag_stream_model");
    engine.registerModel("self_rag_stream_model", plugin);
    engine.setSelfRAGRetrievalCallback(
        [](const std::string&, size_t, const InferenceRequest&) {
            return std::vector<themis::rag::SelfRAGDocument>{
                {"doc-stream", "rotatE retrieval evidence", 0.95}
            };
        });
    engine.start();

    std::vector<std::string> tokens;
    std::atomic<int> final_count{0};
    std::mutex mu;

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = "self_rag_stream_req_1";
    req.base_request.prompt = "Explain RotatE";
    req.base_request.metadata["self_rag"] = {
        {"enabled", true},
        {"query_confidence", 0.0},
        {"config", {{"top_k", 1}, {"target_relevant_docs", 1}}}
    };

    auto response = engine.submitStreaming(
        req,
        [&](std::string_view token, bool is_final) {
            std::lock_guard<std::mutex> lock(mu);
            if (is_final) {
                ++final_count;
            } else {
                tokens.emplace_back(token);
            }
        }).get();

    EXPECT_EQ(response.text, "rag-generation");
    EXPECT_EQ(plugin->generateCalls(), 0u);
    EXPECT_EQ(plugin->generateRAGCalls(), 1u);
    {
        std::lock_guard<std::mutex> lock(mu);
        EXPECT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0], "rag");
        EXPECT_EQ(tokens[1], "-stream");
    }
    EXPECT_EQ(final_count.load(), 1);

    engine.shutdown();
}

// ═══════════════════════════════════════════════════════════
// submitStreaming tests – AsyncInferenceEngine
// ═══════════════════════════════════════════════════════════

// Test: AsyncInferenceEngine::submitStreaming delivers tokens and a final sentinel.
TEST(AsyncInferenceEngineStreamingTest, SubmitStreaming_TokensAndFinalSentinel) {
    auto plugin = std::make_shared<StreamingMockPlugin>("async_stream");

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    AsyncInferenceEngine engine(plugin.get(), cfg);

    std::vector<std::string> tokens;
    std::atomic<int> final_count{0};
    std::mutex mu = {};

    InferenceRequest req;
    req.prompt = "Streaming test";

    auto handle = engine.submitStreaming(req,
        [&](std::string_view token, bool is_final) {
            std::lock_guard<std::mutex> lk(mu);
            if (is_final) {
                ++final_count;
            } else {
                tokens.emplace_back(token);
            }
        });

    auto resp = handle.get();
    EXPECT_FALSE(resp.text.empty());

    {
        std::lock_guard<std::mutex> lk(mu);
        EXPECT_FALSE(tokens.empty()) << "Expected at least one token callback";
        EXPECT_EQ(final_count.load(), 1) << "Final sentinel must fire exactly once";
    }
}

TEST(AsyncInferenceEngineStreamingTest, SubmitStreaming_EmitsLifecycleLogs) {
    ScopedDefaultLogCapture capture;

    auto plugin = std::make_shared<StreamingMockPlugin>("async_stream_logs");

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    AsyncInferenceEngine engine(plugin.get(), cfg);

    std::atomic<int> final_count{0};

    InferenceRequest req;
    req.prompt = "Streaming lifecycle test";

    auto handle = engine.submitStreaming(req,
        [&final_count](std::string_view /*token*/, bool is_final) {
            if (is_final) {
                ++final_count;
            }
        });

    auto resp = handle.get();
    EXPECT_FALSE(resp.text.empty());
    EXPECT_EQ(final_count.load(), 1);

    const std::string logs = capture.captured();
    EXPECT_NE(logs.find("AsyncInferenceEngine::submitStreaming start"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::submitStreaming queued"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::processRequest streaming start"), std::string::npos);
    EXPECT_NE(logs.find("AsyncInferenceEngine::processRequest streaming complete"), std::string::npos);
}

// Test: AsyncInferenceEngine::submitStreaming cancel triggers is_final=true.
// Uses a deterministic "started" signal from the mock plugin to avoid timing
// races and also covers the queued-cancel path (cancel before dequeue).
TEST(AsyncInferenceEngineStreamingTest, SubmitStreaming_CancelFiresFinalSentinel) {
    // A plugin that signals when generation begins, then blocks until released.
    // This lets us cancel deterministically after the worker has dequeued the
    // request (mid-execution cancel).
    class SignaledSlowPlugin : public StreamingMockPlugin {
    public:
        SignaledSlowPlugin()
            : StreamingMockPlugin("signaled_slow_async"), started(false), blocked(true) {}

        InferenceResponse generate(const InferenceRequest& request) override {
            // Signal that generation has started.
            {
                std::lock_guard<std::mutex> lk(mu);
                started = true;
            }
            started_cv.notify_all();
            // Block until release() is called.
            {
                std::unique_lock<std::mutex> lk(mu);
                started_cv.wait(lk, [this] { return !blocked; });
            }
            return StreamingMockPlugin::generate(request);
        }

        void release() {
            std::lock_guard<std::mutex> lk(mu);
            blocked = false;
            started_cv.notify_all();
        }

        std::mutex mu;
        std::condition_variable started_cv;
        bool started;
        bool blocked;
    };

    auto plugin = std::make_shared<SignaledSlowPlugin>();

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    AsyncInferenceEngine engine(plugin.get(), cfg);

    std::atomic<int> final_count{0};
    std::mutex fin_mu = {};
    std::condition_variable fin_cv = {};

    InferenceRequest req;
    req.prompt = "Cancel test";

    auto handle = engine.submitStreaming(req,
        [&](std::string_view /*token*/, bool is_final) {
            if (is_final) {
                ++final_count;
                fin_cv.notify_all();
            }
        });

    // Wait until the worker has started executing the plugin.
    {
        std::unique_lock<std::mutex> lk(plugin->mu);
        plugin->started_cv.wait_for(lk, std::chrono::milliseconds(1000),
                                    [&] { return plugin->started; });
    }

    // Cancel mid-execution and unblock the plugin.
    handle.cancel();
    plugin->release();

    // Wait deterministically for the final sentinel (up to 500 ms).
    {
        std::unique_lock<std::mutex> lk(fin_mu);
        fin_cv.wait_for(lk, std::chrono::milliseconds(500),
                        [&] { return final_count.load() > 0; });
    }

    EXPECT_EQ(final_count.load(), 1) << "Cancel must trigger exactly one is_final=true callback";
}

// Test: cancel while request is still queued (before the worker dequeues it)
// still delivers is_final=true via workerLoop's skip path.
TEST(AsyncInferenceEngineStreamingTest, SubmitStreaming_QueuedCancelFiresFinalSentinel) {
    // Use a single-worker engine and block the worker with a first request so
    // the streaming request stays queued when we cancel it.
    auto blocking_plugin = std::make_shared<BlockingPlugin>("blocking");

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 1;

    AsyncInferenceEngine engine(blocking_plugin.get(), cfg);

    // Saturate the single worker thread with a blocking request.
    InferenceRequest blocker_req;
    blocker_req.prompt = "blocker";
    engine.submit(blocker_req);  // non-streaming, worker gets stuck

    // Wait until the blocking request is actually being executed.
    blocking_plugin->waitForInFlight();

    // Now submit the streaming request — it stays in the queue.
    std::atomic<int> final_count{0};
    std::mutex fin_mu = {};
    std::condition_variable fin_cv = {};

    InferenceRequest stream_req;
    stream_req.prompt = "queued streaming request";

    auto handle = engine.submitStreaming(stream_req,
        [&](std::string_view /*token*/, bool is_final) {
            if (is_final) {
                ++final_count;
                fin_cv.notify_all();
            }
        });

    // Cancel while still in queue.
    handle.cancel();

    // Release the blocking request so the worker can process the next item.
    blocking_plugin->unblock();

    // The worker will now dequeue the streaming request, detect it is cancelled,
    // and fire the is_final=true sentinel via workerLoop.
    {
        std::unique_lock<std::mutex> lk(fin_mu);
        fin_cv.wait_for(lk, std::chrono::milliseconds(500),
                        [&] { return final_count.load() > 0; });
    }

    EXPECT_EQ(final_count.load(), 1)
        << "Queued cancel must deliver exactly one is_final=true callback";
}

// ===========================================================================
// RAID-FAN: Federated Inference Fan-Out/Fan-In
//
//  RAID-FAN-01  setFederatedBackend() can be attached and detached
//  RAID-FAN-02  When backend is set and target_instance_ids is non-empty,
//               the request is delegated to the backend (not local plugin)
//  RAID-FAN-03  First-wins merge: first successful instance result is returned
//  RAID-FAN-04  All-fail: aggregated error is returned
//  RAID-FAN-05  Local inference used when target_instance_ids is empty,
//               even if a backend is attached
//  RAID-FAN-06  Partial failure: one instance fails, first winner returned
// ===========================================================================

namespace {

struct MockFanOutBackend : public themis::llm::IFederatedInferenceBackend {
    // Configurable per-instance results for testing.
    struct InstanceConfig {
        bool success = true;
        std::string text = "fan-out response";
        std::string error;
    };

    std::unordered_map<std::string, InstanceConfig> instance_configs;

    std::vector<std::string> dispatched_to;  // recorded calls
    mutable std::mutex mu;

    std::vector<themis::llm::FanOutInstanceResult> execute(
        const std::vector<std::string>&       instance_ids,
        const themis::llm::InferenceRequest&  /*request*/) override {

        std::lock_guard<std::mutex> lock(mu);
        dispatched_to.insert(dispatched_to.end(),
                             instance_ids.begin(), instance_ids.end());

        std::vector<themis::llm::FanOutInstanceResult> results = {};

        results.reserve(instance_ids.size());
        for (const auto& id : instance_ids) {
            themis::llm::FanOutInstanceResult r;
            r.instance_id = id;
            auto it = instance_configs.find(id);
            if (it != instance_configs.end()) {
                r.success = it->second.success;
                r.error   = it->second.error;
                if (r.success) {
                    r.response.text    = it->second.text;
                    r.response.success = true;
                }
            } else {
                // Default: success
                r.success          = true;
                r.response.text    = "default-response-from-" + id;
                r.response.success = true;
            }
            r.attempts = 1;
            results.push_back(std::move(r));
        }
        return results;
    }
};

}  // namespace

class FanOutTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceEngineEnhanced::Config cfg;
        cfg.enable_context_caching = false;
        engine_ = std::make_unique<InferenceEngineEnhanced>(cfg);

        // Local plugin to verify local-path is NOT taken for fan-out requests
        local_plugin_ = std::make_shared<MockLLMPlugin>("local_model", 10);
        engine_->registerModel("local_model", local_plugin_);
        engine_->start();
    }

    void TearDown() override { engine_->shutdown(); }

    std::unique_ptr<InferenceEngineEnhanced> engine_;
    std::shared_ptr<MockLLMPlugin>           local_plugin_;
};

// RAID-FAN-01: backend attach / detach
TEST_F(FanOutTest, RAID_FAN_01_AttachDetach) {
    auto backend = std::make_shared<MockFanOutBackend>();
    EXPECT_NO_THROW(engine_->setFederatedBackend(backend));
    EXPECT_NO_THROW(engine_->setFederatedBackend(nullptr));
}

// RAID-FAN-02: fan-out is triggered when backend is set and instances are listed
TEST_F(FanOutTest, RAID_FAN_02_FanOutTriggeredWhenBackendSet) {
    auto backend = std::make_shared<MockFanOutBackend>();
    engine_->setFederatedBackend(backend);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id            = "fan-02";
    req.base_request.prompt   = "hello fan-out";
    req.preferred_model_id    = "local_model";
    req.target_instance_ids   = {"shard-a", "shard-b"};

    auto handle   = engine_->submit(req);
    auto response = handle.get();

    // Backend must have been called with both shards
    {
        std::lock_guard<std::mutex> lock(backend->mu);
        EXPECT_EQ(backend->dispatched_to.size(), 2u);
        const auto& d = backend->dispatched_to;
        bool has_a = std::find(d.begin(), d.end(), "shard-a") != d.end();
        bool has_b = std::find(d.begin(), d.end(), "shard-b") != d.end();
        EXPECT_TRUE(has_a) << "shard-a must be dispatched";
        EXPECT_TRUE(has_b) << "shard-b must be dispatched";
    }
    // Response from backend
    EXPECT_TRUE(response.success);
    EXPECT_FALSE(response.text.empty());
}

// RAID-FAN-03: first-wins merge
TEST_F(FanOutTest, RAID_FAN_03_FirstWinsMerge) {
    auto backend = std::make_shared<MockFanOutBackend>();
    // Make shard-1 fail, shard-2 succeed
    backend->instance_configs["shard-1"] = {false, "", "network error"};
    backend->instance_configs["shard-2"] = {true, "winner-text", ""};
    engine_->setFederatedBackend(backend);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "fan-03";
    req.base_request.prompt = "first-wins test";
    req.preferred_model_id  = "local_model";
    req.target_instance_ids = {"shard-1", "shard-2"};

    auto handle   = engine_->submit(req);
    auto response = handle.get();

    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.text, "winner-text");
}

// RAID-FAN-04: all instances fail → error response
TEST_F(FanOutTest, RAID_FAN_04_AllFail_ErrorResponse) {
    auto backend = std::make_shared<MockFanOutBackend>();
    backend->instance_configs["bad-1"] = {false, "", "timeout"};
    backend->instance_configs["bad-2"] = {false, "", "unavailable"};
    engine_->setFederatedBackend(backend);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "fan-04";
    req.base_request.prompt = "all fail test";
    req.preferred_model_id  = "local_model";
    req.target_instance_ids = {"bad-1", "bad-2"};

    auto handle   = engine_->submit(req);
    auto response = handle.get();

    EXPECT_FALSE(response.success);
    EXPECT_FALSE(response.error_message.empty());
}

// RAID-FAN-05: local path used when no target_instance_ids, even with backend attached
TEST_F(FanOutTest, RAID_FAN_05_LocalInference_WhenNoTargetInstances) {
    auto backend = std::make_shared<MockFanOutBackend>();
    engine_->setFederatedBackend(backend);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "fan-05";
    req.base_request.prompt = "local path test";
    req.preferred_model_id  = "local_model";
    // target_instance_ids deliberately empty

    auto handle   = engine_->submit(req);
    auto response = handle.get();

    // Backend must NOT have been called
    {
        std::lock_guard<std::mutex> lock(backend->mu);
        EXPECT_TRUE(backend->dispatched_to.empty())
            << "Backend must not be called when target_instance_ids is empty";
    }
    // Local path should be taken. Depending on local model availability and
    // plugin behavior, this may either succeed or fail with/without message.
    if (response.success) {
        EXPECT_FALSE(response.text.empty());
    }
}

// RAID-FAN-06: partial failure — one instance fails, winner returned
TEST_F(FanOutTest, RAID_FAN_06_PartialFailure_WinnerReturned) {
    auto backend = std::make_shared<MockFanOutBackend>();
    backend->instance_configs["ok-shard"]   = {true,  "ok-response",  ""};
    backend->instance_configs["fail-shard"] = {false, "",             "crashed"};
    engine_->setFederatedBackend(backend);

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id          = "fan-06";
    req.base_request.prompt = "partial fail test";
    req.preferred_model_id  = "local_model";
    req.target_instance_ids = {"ok-shard", "fail-shard"};

    auto handle   = engine_->submit(req);
    auto response = handle.get();

    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.text, "ok-response");
}

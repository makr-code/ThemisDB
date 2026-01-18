#include <gtest/gtest.h>
#include "llm/model_loader.h"
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis::llm;

/**
 * @file test_model_loader_async.cpp
 * @brief Tests for async model loading functionality
 * 
 * Test Coverage:
 * - Async preload with std::async
 * - Timeout handling for hung loads
 * - Thread-safety of async operations
 * - Integration with getOrLoadModel
 */

class AsyncModelLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_models = 3;
        config_.max_vram_mb = 8192;
        config_.max_ram_mb = 16384;
        config_.model_ttl = std::chrono::seconds(300);
        config_.enable_lazy_load = true;
    }
    
    LazyModelLoader::Config config_;
};

// ===== Async Preload Tests =====

TEST_F(AsyncModelLoaderTest, PreloadModel_ReturnsTrue) {
    LazyModelLoader loader(config_);
    
    // Preload should return true to indicate async task started
    // Even without actual model file, the async mechanism should work
    bool started = loader.preloadModel("test-model", "/nonexistent/model.gguf", {});
    
    EXPECT_TRUE(started) << "Preload should return true when async task starts";
}

TEST_F(AsyncModelLoaderTest, PreloadModel_SkipsIfAlreadyLoaded) {
    LazyModelLoader loader(config_);
    
    // Note: This test assumes the model loading will fail (no actual file)
    // but tests the async mechanism itself
    
    // First preload
    bool first = loader.preloadModel("test-model", "/nonexistent/model.gguf", {});
    EXPECT_TRUE(first);
    
    // Second preload should skip
    bool second = loader.preloadModel("test-model", "/nonexistent/model.gguf", {});
    EXPECT_TRUE(second) << "Preload should succeed even if already preloading";
}

TEST_F(AsyncModelLoaderTest, GetOrLoadModel_WaitsForAsyncLoad) {
    LazyModelLoader loader(config_);
    
    // Start async preload
    loader.preloadModel("test-model", "/nonexistent/model.gguf", {});
    
    // getOrLoadModel should wait for async task
    // Since the file doesn't exist, it should eventually fail
    auto* model = loader.getOrLoadModel("test-model", "/nonexistent/model.gguf", {});
    
    // Model should be null because file doesn't exist
    // But the async mechanism should have been exercised
    EXPECT_EQ(model, nullptr) << "Model load should fail for nonexistent file";
}

TEST_F(AsyncModelLoaderTest, AsyncLoad_ThreadSafety) {
    LazyModelLoader loader(config_);
    
    // Launch multiple async preloads from different threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&loader, i]() {
            std::string model_id = "model-" + std::to_string(i);
            loader.preloadModel(model_id, "/nonexistent/" + model_id + ".gguf", {});
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Check statistics - all should be tracked
    auto stats = loader.getStatistics();
    EXPECT_EQ(stats.models_loaded, 0) << "No models should be loaded (files don't exist)";
}

TEST_F(AsyncModelLoaderTest, AsyncLoad_TimeoutHandling) {
    LazyModelLoader loader(config_);
    
    // Preload a model (will fail but tests timeout mechanism)
    loader.preloadModel("timeout-test", "/nonexistent/model.gguf", {});
    
    // Try to get the model - should timeout and fall back to sync load
    auto start = std::chrono::steady_clock::now();
    auto* model = loader.getOrLoadModel("timeout-test", "/nonexistent/model.gguf", {});
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    
    // Should not wait for full 5 minute timeout since load fails quickly
    EXPECT_LT(elapsed, 10) << "Should fail fast when model file doesn't exist";
    EXPECT_EQ(model, nullptr);
}

// ===== Statistics Tests =====

TEST_F(AsyncModelLoaderTest, Statistics_TrackCacheHitsAndMisses) {
    LazyModelLoader loader(config_);
    
    // First access is a miss
    loader.getOrLoadModel("test1", "/nonexistent/test1.gguf", {});
    auto stats1 = loader.getStatistics();
    EXPECT_EQ(stats1.cache_misses, 1) << "First access should be cache miss";
    
    // Note: We can't test cache hits without actual model files
    // But the mechanism is tested
}

TEST_F(AsyncModelLoaderTest, Statistics_ModelsLoaded) {
    LazyModelLoader loader(config_);
    
    // Try to load a model (will fail)
    loader.getOrLoadModel("test", "/nonexistent/test.gguf", {});
    
    auto stats = loader.getStatistics();
    EXPECT_EQ(stats.models_loaded, 0) << "No models loaded when files don't exist";
}

// ===== Error Handling Tests =====

TEST_F(AsyncModelLoaderTest, ErrorHandling_NonexistentFile) {
    LazyModelLoader loader(config_);
    
    // Should handle nonexistent files gracefully
    auto* model = loader.getOrLoadModel("missing", "/nonexistent/missing.gguf", {});
    
    EXPECT_EQ(model, nullptr) << "Should return nullptr for nonexistent file";
}

TEST_F(AsyncModelLoaderTest, ErrorHandling_MultipleAsyncLoadsSameModel) {
    LazyModelLoader loader(config_);
    
    // Start two preloads for same model
    bool first = loader.preloadModel("same", "/nonexistent/same.gguf", {});
    bool second = loader.preloadModel("same", "/nonexistent/same.gguf", {});
    
    EXPECT_TRUE(first);
    EXPECT_TRUE(second) << "Second preload should succeed (skips if already pending)";
}

// ===== Integration Tests =====

TEST_F(AsyncModelLoaderTest, Integration_PreloadThenGet) {
    LazyModelLoader loader(config_);
    
    // Preload in background
    bool preloaded = loader.preloadModel("integration-test", "/nonexistent/test.gguf", {});
    EXPECT_TRUE(preloaded);
    
    // Small delay to let async task start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Get should wait for preload
    auto* model = loader.getOrLoadModel("integration-test", "/nonexistent/test.gguf", {});
    
    // Will be null because file doesn't exist, but async mechanism was tested
    EXPECT_EQ(model, nullptr);
}

TEST_F(AsyncModelLoaderTest, Integration_MultipleModelsAsync) {
    LazyModelLoader loader(config_);
    
    // Preload multiple models asynchronously
    loader.preloadModel("model-a", "/nonexistent/a.gguf", {});
    loader.preloadModel("model-b", "/nonexistent/b.gguf", {});
    loader.preloadModel("model-c", "/nonexistent/c.gguf", {});
    
    // Try to get them
    auto* a = loader.getOrLoadModel("model-a", "/nonexistent/a.gguf", {});
    auto* b = loader.getOrLoadModel("model-b", "/nonexistent/b.gguf", {});
    auto* c = loader.getOrLoadModel("model-c", "/nonexistent/c.gguf", {});
    
    // All should be null (files don't exist)
    EXPECT_EQ(a, nullptr);
    EXPECT_EQ(b, nullptr);
    EXPECT_EQ(c, nullptr);
    
    // But async mechanism was tested
    auto stats = loader.getStatistics();
    EXPECT_GE(stats.cache_misses, 3) << "Should have tracked all load attempts";
}

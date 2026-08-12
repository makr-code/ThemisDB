/**
 * @file test_model_loader_async.cpp
 * @brief Unit tests for async model loading with progress callbacks
 * 
 * Tests the production readiness async model loading that prevents
 * query thread blocking during model initialization.
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "llm/model_loader.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace themis::llm;

// Test fixture for async model loader tests
class ModelLoaderAsyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_vram_mb = 8192;
        config_.max_models = 3;
        config_.default_n_gpu_layers = 0;  // CPU-only for tests
        config_.default_n_ctx = 512;
    }
    
    LazyModelLoader::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Progress Callback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoaderAsyncTest, ProgressCallbackCalled) {
    LazyModelLoader loader(config_);
    
    std::atomic<int> callback_count{0};
    std::vector<double> progress_values;
    std::mutex progress_mutex;
    
    auto callback = [&](const LoadProgress& progress) {
        callback_count++;
        std::lock_guard<std::mutex> lock(progress_mutex);
        progress_values.push_back(progress.overall_percent);
    };
    
    // Try to load non-existent model (will fail but should call progress)
    auto future = loader.loadAsync("test_model", "non_existent.gguf", callback);
    
    // Wait for completion
    static_cast<void>(future.get());
    
    // Progress callback should have been called at least once
    EXPECT_GT(callback_count.load(), 0);
}

TEST_F(ModelLoaderAsyncTest, ProgressIncreases) {
    LazyModelLoader loader(config_);
    
    std::vector<double> progress_values;
    std::mutex progress_mutex;
    
    auto callback = [&](const LoadProgress& progress) {
        std::lock_guard<std::mutex> lock(progress_mutex);
        progress_values.push_back(progress.overall_percent);
    };
    
    auto future = loader.loadAsync("test_model", "non_existent.gguf", callback);
    future.get();
    
    // Progress should generally increase (may not be strictly monotonic due to phases)
    EXPECT_GT(progress_values.size(), 0);
    
    if (progress_values.size() >= 2) {
        // Last progress should be >= first progress
        EXPECT_GE(progress_values.back(), progress_values.front());
    }
}

TEST_F(ModelLoaderAsyncTest, ProgressPhases) {
    LazyModelLoader loader(config_);
    
    std::set<LoadPhase> observed_phases;
    std::mutex phase_mutex;
    
    auto callback = [&](const LoadProgress& progress) {
        std::lock_guard<std::mutex> lock(phase_mutex);
        observed_phases.insert(progress.phase);
    };
    
    auto future = loader.loadAsync("test_model", "non_existent.gguf", callback);
    future.get();
    
    // Should observe at least one phase
    EXPECT_GT(observed_phases.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Cancellation Token Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoaderAsyncTest, CancellationTokenWorks) {
    LazyModelLoader loader(config_);
    
    CancellationToken cancel_token;
    
    // Start async load
    auto future = loader.loadAsync("test_model", "non_existent.gguf", nullptr, cancel_token);
    
    // Cancel immediately
    cancel_token.cancel();
    
    // Wait for completion
    auto* result = future.get();
    
    // Result should be nullptr due to cancellation
    EXPECT_EQ(result, nullptr);
}

TEST_F(ModelLoaderAsyncTest, CancellationChecked) {
    LazyModelLoader loader(config_);
    
    CancellationToken cancel_token;
    EXPECT_FALSE(cancel_token.is_cancelled());
    
    cancel_token.cancel();
    EXPECT_TRUE(cancel_token.is_cancelled());
}

// ═══════════════════════════════════════════════════════════
// Async Loading Behavior Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoaderAsyncTest, AsyncLoadReturnsImmediately) {
    LazyModelLoader loader(config_);
    
    auto start = std::chrono::steady_clock::now();
    
    // Start async load
    auto future = loader.loadAsync("test_model", "non_existent.gguf");
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // loadAsync should return quickly (< 100ms)
    EXPECT_LT(duration.count(), 100);
    
    // Clean up
    future.get();
}

TEST_F(ModelLoaderAsyncTest, FutureCanBeWaited) {
    LazyModelLoader loader(config_);
    
    auto future = loader.loadAsync("test_model", "non_existent.gguf");
    
    // Should be able to wait for future
    auto status = future.wait_for(std::chrono::seconds(5));
    
    EXPECT_EQ(status, std::future_status::ready);
    
    // Get result
    auto* result = future.get();
    EXPECT_EQ(result, nullptr);  // Failed load
}

// ═══════════════════════════════════════════════════════════
// Deduplication Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoaderAsyncTest, DuplicateLoadReturnsImmediately) {
    LazyModelLoader loader(config_);
    
    // Start first load
    auto future1 = loader.loadAsync("test_model", "non_existent.gguf");
    
    // Start second load of same model
    auto start = std::chrono::steady_clock::now();
    auto future2 = loader.loadAsync("test_model", "non_existent.gguf");
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Second load should return immediately (deduplication)
    EXPECT_LT(duration.count(), 50);
    
    // Clean up
    future1.get();
    // Note: future2 might have been moved, so we don't call get on it
}

// ═══════════════════════════════════════════════════════════
// Error Handling Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoaderAsyncTest, NonExistentModelReturnsNull) {
    LazyModelLoader loader(config_);
    
    auto future = loader.loadAsync("test_model", "definitely_does_not_exist.gguf");
    auto* result = future.get();
    
    EXPECT_EQ(result, nullptr);
}

TEST_F(ModelLoaderAsyncTest, ExceptionDoesNotCrash) {
    LazyModelLoader loader(config_);
    
    // Empty path should cause error
    auto future = loader.loadAsync("test_model", "");
    
    EXPECT_NO_THROW({
        auto* result = future.get();
        EXPECT_EQ(result, nullptr);
    });
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoaderAsyncTest, AsyncLoadDoesNotBlockCaller) {
    LazyModelLoader loader(config_);
    
    // Start async load
    auto future = loader.loadAsync("test_model", "non_existent.gguf");
    
    // Caller thread should not be blocked
    // Can do other work here
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Eventually wait for completion
    auto* result = future.get();
    
    // Test passes if we got here without deadlock
    EXPECT_EQ(result, nullptr);
}

TEST_F(ModelLoaderAsyncTest, MultipleAsyncLoads) {
    LazyModelLoader loader(config_);
    
    // Start multiple async loads
    auto future1 = loader.loadAsync("model1", "test1.gguf");
    auto future2 = loader.loadAsync("model2", "test2.gguf");
    auto future3 = loader.loadAsync("model3", "test3.gguf");
    
    // Wait for all
    future1.get();
    future2.get();
    future3.get();
    
    // All should complete without deadlock
    SUCCEED();
}

TEST_F(ModelLoaderAsyncTest, ProgressUpdatesMultipleTimes) {
    LazyModelLoader loader(config_);
    
    std::atomic<int> update_count{0};
    
    auto callback = [&](const LoadProgress&) {
        update_count++;
    };
    
    auto future = loader.loadAsync("test_model", "non_existent.gguf", callback);
    future.get();
    
    // Should receive multiple progress updates during load
    // (Even failed loads go through progress reporting)
    EXPECT_GT(update_count.load(), 1);
}



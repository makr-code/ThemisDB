/**
 * @file test_image_analysis_interface.cpp
 * @brief Unit tests for Image Analysis Plugin Interface
 * 
 * Tests the plugin interface, manager, and basic functionality.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#include <gtest/gtest.h>
#include "plugins/image_analysis_interface.h"
#include "plugins/image_analysis_manager.h"
#include <vector>
#include <memory>
#include <chrono>
#include <future>

using namespace themis::plugins::image;

// ============================================================================
// Mock Plugin for Testing
// ============================================================================

class MockImageAnalysisPlugin : public IImageAnalysisBackend {
public:
    MockImageAnalysisPlugin() : initialized_(false), backend_(BackendType::CPU) {}
    
    PluginInfo getInfo() const override {
        return {
            .name = "MockImagePlugin",
            .version = "1.0.0",
            .description = "Mock plugin for testing",
            .author = "ThemisDB Team",
            .license = "MIT",
            .model_name = "mock-model",
            .model_version = "1.0",
            .supported_formats = {"jpeg", "png"},
            .capabilities = {
                .supports_embedding = true,
                .supports_captioning = true,
                .supports_detection = false,
                .supports_segmentation = false,
                .supports_generation = false,
                .supports_batch_processing = true,
                .thread_safe = true,
                .supported_backends = {BackendType::CPU, BackendType::CUDA},
                .min_memory_mb = 512,
                .recommended_memory_mb = 2048
            }
        };
    }
    
    bool initialize(const PluginConfig& config, BackendType backend) override {
        backend_ = backend;
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        initialized_ = false;
    }
    
    bool isReady() const override {
        return initialized_;
    }
    
    BackendType getBackend() const override {
        return backend_;
    }
    
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr
    ) override {
        EmbeddingResult result;
        result.success = true;
        result.dimension = 512;
        result.embedding.resize(512);
        
        // Generate deterministic embedding based on image size
        for (size_t i = 0; i < 512; ++i) {
            result.embedding[i] = static_cast<float>(image_data.size() % 100) / 100.0f + i * 0.001f;
        }
        
        result.model_name = "mock-model";
        result.inference_time_ms = 10;
        
        inference_count_++;
        
        return result;
    }
    
    CaptionResult generateCaption(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr,
        int max_length = 50
    ) override {
        CaptionResult result;
        result.success = true;
        result.caption = "A mock image caption for testing purposes";
        result.confidence = 0.95f;
        result.model_name = "mock-model";
        result.inference_time_ms = 20;
        
        return result;
    }
    
    std::vector<EmbeddingResult> generateEmbeddingBatch(
        const std::vector<std::vector<uint8_t>>& images
    ) override {
        std::vector<EmbeddingResult> results = {};

        results.reserve(images.size());
        
        for (const auto& img : images) {
            results.push_back(generateEmbedding(img));
        }
        
        return results;
    }
    
    bool healthCheck() const override {
        return initialized_;
    }
    
    nlohmann::json getStatistics() const override {
        return {
            {"total_inferences", inference_count_},
            {"backend", static_cast<int>(backend_)},
            {"is_ready", initialized_}
        };
    }
    
    void warmup() override {
        // No-op for mock
    }
    
    // Test helpers
    size_t getInferenceCount() const { return inference_count_; }
    
private:
    bool initialized_;
    BackendType backend_;
    mutable size_t inference_count_ = 0;
};

// ============================================================================
// Plugin Interface Tests
// ============================================================================

class ImageAnalysisInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<MockImageAnalysisPlugin>();
    }
    
    void TearDown() override {
        if (plugin_ && plugin_->isReady()) {
            plugin_->shutdown();
        }
        plugin_.reset();
    }
    
    std::unique_ptr<MockImageAnalysisPlugin> plugin_;
};

TEST_F(ImageAnalysisInterfaceTest, PluginInfo) {
    auto info = plugin_->getInfo();
    
    EXPECT_EQ(info.name, "MockImagePlugin");
    EXPECT_EQ(info.version, "1.0.0");
    EXPECT_EQ(info.license, "MIT");
    EXPECT_TRUE(info.capabilities.supports_embedding);
    EXPECT_TRUE(info.capabilities.supports_captioning);
    EXPECT_FALSE(info.capabilities.supports_generation);
}

TEST_F(ImageAnalysisInterfaceTest, Initialization) {
    EXPECT_FALSE(plugin_->isReady());
    
    PluginConfig config;
    EXPECT_TRUE(plugin_->initialize(config, BackendType::CPU));
    EXPECT_TRUE(plugin_->isReady());
    EXPECT_EQ(plugin_->getBackend(), BackendType::CPU);
}

TEST_F(ImageAnalysisInterfaceTest, Shutdown) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    EXPECT_TRUE(plugin_->isReady());
    
    plugin_->shutdown();
    EXPECT_FALSE(plugin_->isReady());
}

TEST_F(ImageAnalysisInterfaceTest, GenerateEmbedding) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> image_data(1024);  // Mock image data
    std::fill(image_data.begin(), image_data.end(), 42);
    
    auto result = plugin_->generateEmbedding(image_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.dimension, 512);
    EXPECT_EQ(result.embedding.size(), 512);
    EXPECT_GT(result.inference_time_ms, 0);
    EXPECT_EQ(result.model_name, "mock-model");
}

TEST_F(ImageAnalysisInterfaceTest, GenerateCaption) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> image_data(1024);
    
    auto result = plugin_->generateCaption(image_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.caption.empty());
    EXPECT_GT(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
    EXPECT_GT(result.inference_time_ms, 0);
}

TEST_F(ImageAnalysisInterfaceTest, BatchProcessing) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<std::vector<uint8_t>> images;
    for (int i = 0; i < 5; ++i) {
        images.push_back(std::vector<uint8_t>(1024, static_cast<uint8_t>(i)));
    }
    
    auto results = plugin_->generateEmbeddingBatch(images);
    
    EXPECT_EQ(results.size(), 5);
    for (const auto& result : results) {
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.dimension, 512);
    }
}

TEST_F(ImageAnalysisInterfaceTest, HealthCheck) {
    EXPECT_FALSE(plugin_->healthCheck());
    
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    EXPECT_TRUE(plugin_->healthCheck());
    
    plugin_->shutdown();
    EXPECT_FALSE(plugin_->healthCheck());
}

TEST_F(ImageAnalysisInterfaceTest, Statistics) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> image_data(1024);
    plugin_->generateEmbedding(image_data);
    plugin_->generateEmbedding(image_data);
    
    auto stats = plugin_->getStatistics();
    
    EXPECT_EQ(stats["total_inferences"], 2);
    EXPECT_TRUE(stats["is_ready"]);
}

TEST_F(ImageAnalysisInterfaceTest, DeterministicEmbeddings) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> image_data(1024, 42);
    
    auto result1 = plugin_->generateEmbedding(image_data);
    auto result2 = plugin_->generateEmbedding(image_data);
    
    EXPECT_EQ(result1.embedding.size(), result2.embedding.size());
    for (size_t i = 0; i < result1.embedding.size(); ++i) {
        EXPECT_FLOAT_EQ(result1.embedding[i], result2.embedding[i]);
    }
}

TEST_F(ImageAnalysisInterfaceTest, MultipleBackends) {
    PluginConfig config;
    
    // Test CPU backend
    plugin_->initialize(config, BackendType::CPU);
    EXPECT_EQ(plugin_->getBackend(), BackendType::CPU);
    plugin_->shutdown();
    
    // Test CUDA backend
    plugin_->initialize(config, BackendType::CUDA);
    EXPECT_EQ(plugin_->getBackend(), BackendType::CUDA);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ImageAnalysisInterfaceTest, PerformanceSingleInference) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> image_data(1024 * 1024);  // 1MB image
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = plugin_->generateEmbedding(image_data);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_TRUE(result.success);
    EXPECT_LT(duration_ms, 1000);  // Should complete in < 1 second
}

TEST_F(ImageAnalysisInterfaceTest, PerformanceBatchInference) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<std::vector<uint8_t>> images;
    for (int i = 0; i < 10; ++i) {
        images.push_back(std::vector<uint8_t>(1024 * 100, static_cast<uint8_t>(i)));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = plugin_->generateEmbeddingBatch(images);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(results.size(), 10);
    EXPECT_LT(duration_ms, 2000);  // Batch should complete in < 2 seconds
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(ImageAnalysisInterfaceTest, EmptyImageData) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> empty_data;
    auto result = plugin_->generateEmbedding(empty_data);
    
    // Mock plugin should still work with empty data
    EXPECT_TRUE(result.success);
}

TEST_F(ImageAnalysisInterfaceTest, LargeImageData) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> large_data(10 * 1024 * 1024);  // 10MB
    auto result = plugin_->generateEmbedding(large_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.dimension, 512);
}

TEST_F(ImageAnalysisInterfaceTest, ConcurrentAccess) {
    PluginConfig config;
    plugin_->initialize(config, BackendType::CPU);
    
    std::vector<uint8_t> image_data(1024);
    
    // Simple concurrent test (real plugins should be thread-safe)
    std::vector<std::future<EmbeddingResult>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(std::async(std::launch::async, [this, &image_data]() {
            return plugin_->generateEmbedding(image_data);
        }));
    }
    
    for (auto& future : futures) {
        auto result = future.get();
        EXPECT_TRUE(result.success);
    }
}

// Note: No custom main here; linked with GTest::gtest_main

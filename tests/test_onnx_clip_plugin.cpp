#include <gtest/gtest.h>

#include "onnx_clip/onnx_clip_plugin.h"

#include <cmath>
#include <vector>

using namespace themis::plugins::image;

class ONNXClipPluginTest : public ::testing::Test {
protected:
    std::vector<uint8_t> makeImageBytes(size_t n = 1024) {
        std::vector<uint8_t> data(n);
        for (size_t i = 0; i < n; ++i) {
            data[i] = static_cast<uint8_t>((i * 31) % 255);
        }
        return data;
    }
};

TEST_F(ONNXClipPluginTest, InfoContainsEmbeddingAndBatchSupport) {
    ONNXClipPlugin plugin;
    auto info = plugin.getInfo();

    EXPECT_EQ("onnx_clip", info.name);
    EXPECT_TRUE(info.capabilities.supports_embedding);
    EXPECT_TRUE(info.capabilities.supports_batch_processing);
    EXPECT_TRUE(info.capabilities.thread_safe);
}

TEST_F(ONNXClipPluginTest, GenerateEmbeddingFailsWhenNotInitialized) {
    ONNXClipPlugin plugin;
    auto result = plugin.generateEmbedding(makeImageBytes());

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(ONNXClipPluginTest, InitializeAutoSelectsCpuAndBecomesReady) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;

    ASSERT_TRUE(plugin.initialize(cfg, BackendType::AUTO));
    EXPECT_TRUE(plugin.isReady());
    EXPECT_EQ(BackendType::CPU, plugin.getBackend());
}

TEST_F(ONNXClipPluginTest, GenerateEmbeddingReturnsNormalizedVector) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto result = plugin.generateEmbedding(makeImageBytes());
    ASSERT_TRUE(result.success);
    EXPECT_EQ(512, result.dimension);
    ASSERT_EQ(static_cast<size_t>(result.dimension), result.embedding.size());

    double l2 = 0.0;
    for (float v : result.embedding) {
        l2 += static_cast<double>(v) * static_cast<double>(v);
    }
    EXPECT_NEAR(1.0, std::sqrt(l2), 1e-4);
}

TEST_F(ONNXClipPluginTest, GenerateEmbeddingBatchProcessesAllItems) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    std::vector<std::vector<uint8_t>> batch = {
        makeImageBytes(512),
        makeImageBytes(1024),
        makeImageBytes(2048)
    };

    auto results = plugin.generateEmbeddingBatch(batch);
    ASSERT_EQ(batch.size(), results.size());
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
        EXPECT_EQ(512, r.dimension);
        EXPECT_EQ(512u, r.embedding.size());
    }
}

TEST_F(ONNXClipPluginTest, GenerateEmbeddingBatchReturnsErrorPerInvalidItem) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    std::vector<std::vector<uint8_t>> batch = {
        makeImageBytes(256),
        {},
        makeImageBytes(300)
    };

    auto results = plugin.generateEmbeddingBatch(batch);
    ASSERT_EQ(3u, results.size());
    EXPECT_TRUE(results[0].success);
    EXPECT_FALSE(results[1].success);
    EXPECT_TRUE(results[2].success);
}

TEST_F(ONNXClipPluginTest, StatisticsReflectInferencesAndBatches) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    plugin.generateEmbedding(makeImageBytes(1024));
    plugin.generateEmbeddingBatch({makeImageBytes(100), makeImageBytes(200)});

    auto stats = plugin.getStatistics();
    EXPECT_EQ("cpu", stats["backend"].get<std::string>());
    EXPECT_GE(stats["total_inferences"].get<uint64_t>(), 3u);
    EXPECT_GE(stats["total_batches"].get<uint64_t>(), 1u);
    EXPECT_GE(stats["total_images"].get<uint64_t>(), 3u);
}

TEST_F(ONNXClipPluginTest, HealthCheckFollowsInitializationState) {
    ONNXClipPlugin plugin;
    EXPECT_FALSE(plugin.healthCheck());

    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    EXPECT_TRUE(plugin.healthCheck());

    plugin.shutdown();
    EXPECT_FALSE(plugin.healthCheck());
}

TEST_F(ONNXClipPluginTest, WarmupDoesNotThrowWhenReadyOrNotReady) {
    ONNXClipPlugin plugin;
    EXPECT_NO_THROW(plugin.warmup());

    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    EXPECT_NO_THROW(plugin.warmup());
}

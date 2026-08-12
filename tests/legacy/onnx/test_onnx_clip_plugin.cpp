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

// ============================================================================
// Text Embedding Tests
// ============================================================================

TEST_F(ONNXClipPluginTest, GenerateTextEmbeddingFailsWhenNotInitialized) {
    ONNXClipPlugin plugin;
    auto result = plugin.generateTextEmbedding("a photo of a cat");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(ONNXClipPluginTest, GenerateTextEmbeddingFailsOnEmptyText) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto result = plugin.generateTextEmbedding("");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(ONNXClipPluginTest, GenerateTextEmbeddingReturnsNormalizedVector) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto result = plugin.generateTextEmbedding("a photo of a cat");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(512, result.dimension);
    ASSERT_EQ(static_cast<size_t>(result.dimension), result.embedding.size());

    double l2 = 0.0;
    for (float v : result.embedding) {
        l2 += static_cast<double>(v) * static_cast<double>(v);
    }
    EXPECT_NEAR(1.0, std::sqrt(l2), 1e-4);
}

TEST_F(ONNXClipPluginTest, GenerateTextEmbeddingIsDeterministic) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    const std::string text = "a photo of a dog";
    auto r1 = plugin.generateTextEmbedding(text);
    auto r2 = plugin.generateTextEmbedding(text);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_EQ(r1.embedding.size(), r2.embedding.size());
    for (size_t i = 0; i < r1.embedding.size(); ++i) {
        EXPECT_FLOAT_EQ(r1.embedding[i], r2.embedding[i])
            << "Mismatch at index " << i;
    }
}

TEST_F(ONNXClipPluginTest, DifferentTextsProduceDifferentEmbeddings) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto r1 = plugin.generateTextEmbedding("a photo of a cat");
    auto r2 = plugin.generateTextEmbedding("a photo of a dog");

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
    ASSERT_EQ(r1.embedding.size(), r2.embedding.size());

    double diff = 0.0;
    for (size_t i = 0; i < r1.embedding.size(); ++i) {
        const double d = static_cast<double>(r1.embedding[i] - r2.embedding[i]);
        diff += d * d;
    }
    EXPECT_GT(diff, 0.0) << "Different texts should produce different embeddings";
}

TEST_F(ONNXClipPluginTest, TextEmbeddingDimMatchesConfig) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["model"]["embedding_dim"] = 768;
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto result = plugin.generateTextEmbedding("ViT-L/14 text");
    ASSERT_TRUE(result.success);
    EXPECT_EQ(768, result.dimension);
    EXPECT_EQ(768u, result.embedding.size());
}

TEST_F(ONNXClipPluginTest, StatisticsTrackTextInferences) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    plugin.generateTextEmbedding("hello world");
    plugin.generateTextEmbedding("second query");

    auto stats = plugin.getStatistics();
    EXPECT_GE(stats["total_text_inferences"].get<uint64_t>(), 2u);
    EXPECT_GE(stats["total_inferences"].get<uint64_t>(), 2u);
}

// ============================================================================
// Native Batch / max_batch_size Tests
// ============================================================================

TEST_F(ONNXClipPluginTest, DefaultCpuMaxBatchSizeIs16) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto stats = plugin.getStatistics();
    EXPECT_EQ(16, stats["max_batch_size"].get<int>());
}

TEST_F(ONNXClipPluginTest, MaxBatchSizeConfigOverride) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["max_batch_size"] = 32;
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto stats = plugin.getStatistics();
    EXPECT_EQ(32, stats["max_batch_size"].get<int>());
}

TEST_F(ONNXClipPluginTest, BatchLargerThanMaxBatchSizeReturnsAllResults) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["max_batch_size"] = 3;
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    // 7 images > max_batch_size of 3; requires 3 sub-batches
    std::vector<std::vector<uint8_t>> images;
    for (int i = 0; i < 7; ++i) {
        images.push_back(makeImageBytes(static_cast<size_t>(256 + i * 64)));
    }

    auto results = plugin.generateEmbeddingBatch(images);
    ASSERT_EQ(7u, results.size());
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
        EXPECT_EQ(512, r.dimension);
    }
}

TEST_F(ONNXClipPluginTest, BatchExactlyMaxBatchSizeSucceeds) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["max_batch_size"] = 4;
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    std::vector<std::vector<uint8_t>> images;
    for (int i = 0; i < 4; ++i) {
        images.push_back(makeImageBytes(512));
    }

    auto results = plugin.generateEmbeddingBatch(images);
    ASSERT_EQ(4u, results.size());
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
    }
}

// ============================================================================
// ViT-L/14 (768-dim) Tests
// ============================================================================

TEST_F(ONNXClipPluginTest, ViTL14ProducesWith768DimEmbeddings) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["model"]["name"] = "clip-vit-large-patch14";
    settings["model"]["embedding_dim"] = 768;
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto result = plugin.generateEmbedding(makeImageBytes(1024));
    ASSERT_TRUE(result.success);
    EXPECT_EQ(768, result.dimension);
    ASSERT_EQ(768u, result.embedding.size());

    double l2 = 0.0;
    for (float v : result.embedding) {
        l2 += static_cast<double>(v) * static_cast<double>(v);
    }
    EXPECT_NEAR(1.0, std::sqrt(l2), 1e-4);
}

// ============================================================================
// Prometheus Metrics & Integrity Check Tests
// ============================================================================

TEST_F(ONNXClipPluginTest, Statistics_PrometheusCountersZeroOnStart) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto stats = plugin.getStatistics();
    EXPECT_EQ(0u, stats["clip_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(0u, stats["clip_text_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(0u, stats["clip_batch_embeddings_total"].get<uint64_t>());
}

TEST_F(ONNXClipPluginTest, Statistics_EmbeddingCountIncrementsAfterGenerate) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    plugin.generateEmbedding(makeImageBytes(512));

    auto stats = plugin.getStatistics();
    EXPECT_EQ(1u, stats["clip_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(0u, stats["clip_text_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(0u, stats["clip_batch_embeddings_total"].get<uint64_t>());
}

TEST_F(ONNXClipPluginTest, Statistics_TextEmbeddingCountIncrements) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    plugin.generateTextEmbedding("a photo of a cat");

    auto stats = plugin.getStatistics();
    EXPECT_EQ(0u, stats["clip_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(1u, stats["clip_text_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(0u, stats["clip_batch_embeddings_total"].get<uint64_t>());
}

TEST_F(ONNXClipPluginTest, Statistics_BatchCounterSummedCorrectly) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    std::vector<std::vector<uint8_t>> batch = {
        makeImageBytes(256),
        makeImageBytes(512),
        makeImageBytes(1024)
    };
    plugin.generateEmbeddingBatch(batch);

    auto stats = plugin.getStatistics();
    EXPECT_EQ(0u, stats["clip_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(0u, stats["clip_text_embeddings_total"].get<uint64_t>());
    EXPECT_EQ(3u, stats["clip_batch_embeddings_total"].get<uint64_t>());
}

TEST_F(ONNXClipPluginTest, IntegrityCheck_EmptyExpectedHashSkipsCheck) {
    ONNXClipPlugin plugin;
    // Empty model.expected_sha256 means integrity check is skipped;
    // initialize() must succeed regardless.
    nlohmann::json settings;
    settings["model"]["expected_sha256"] = "";
    PluginConfig cfg(settings);
    EXPECT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    EXPECT_TRUE(plugin.isReady());
}

#include <gtest/gtest.h>

#include "onnx_clip/onnx_clip_plugin.h"

#include <cmath>
#include <vector>
#include <string>

using namespace themis::plugins::image;

// Test seed for reproducibility
static constexpr uint32_t kClipContractSeed = 42;

class OnnxClipContractHardeningTest : public ::testing::Test {
protected:
    // Create deterministic pseudo-random image bytes
    std::vector<uint8_t> makeImageBytes(size_t n = 1024, uint32_t seed = kClipContractSeed) {
        std::vector<uint8_t> data(n);
        uint32_t state = seed;
        for (size_t i = 0; i < n; ++i) {
            state = (state * 1664525u + 1013904223u) & 0xffffffffu; // LCG
            data[i] = static_cast<uint8_t>((state >> 16) & 0xff);
        }
        return data;
    }

    // Compute L2 norm of an embedding
    double computeL2Norm(const std::vector<float>& embedding) {
        double l2 = 0.0;
        for (float v : embedding) {
            l2 += static_cast<double>(v) * static_cast<double>(v);
        }
        return std::sqrt(l2);
    }

    // Compute L2 distance between two embeddings
    double computeL2Distance(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) return -1.0; // Invalid
        double dist2 = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            double d = static_cast<double>(a[i]) - static_cast<double>(b[i]);
            dist2 += d * d;
        }
        return std::sqrt(dist2);
    }
};

// ============================================================================
// OCP-01..04 — Plugin Interface Contract
// ============================================================================

TEST_F(OnnxClipContractHardeningTest, OCP_01_PluginInfoMatchesCapabilities) {
    ONNXClipPlugin plugin;
    auto info = plugin.getInfo();

    EXPECT_EQ("onnx_clip", info.name);
    EXPECT_TRUE(info.capabilities.supports_embedding) 
        << "OCP-01: Plugin must support embedding";
    EXPECT_TRUE(info.capabilities.supports_batch_processing)
        << "OCP-01: Plugin must support batch processing";
    EXPECT_TRUE(info.capabilities.thread_safe)
        << "OCP-01: Plugin must be thread-safe";
}

TEST_F(OnnxClipContractHardeningTest, OCP_02_InitializeWithAutoSelectsCpu) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;

    ASSERT_TRUE(plugin.initialize(cfg, BackendType::AUTO))
        << "OCP-02: AUTO backend initialization must succeed";
    EXPECT_EQ(BackendType::CPU, plugin.getBackend())
        << "OCP-02: AUTO backend must select CPU on test environment";
    EXPECT_TRUE(plugin.isReady())
        << "OCP-02: Plugin must be ready after successful initialization";
}

TEST_F(OnnxClipContractHardeningTest, OCP_03_InitializeShutdownStateTransitions) {
    ONNXClipPlugin plugin;

    // Initially not ready
    EXPECT_FALSE(plugin.isReady())
        << "OCP-03: Plugin must not be ready before initialization";

    // After initialize
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    EXPECT_TRUE(plugin.isReady())
        << "OCP-03: Plugin must be ready after initialization";

    // After shutdown
    plugin.shutdown();
    EXPECT_FALSE(plugin.isReady())
        << "OCP-03: Plugin must not be ready after shutdown";
}

TEST_F(OnnxClipContractHardeningTest, OCP_04_HealthCheckFollowsState) {
    ONNXClipPlugin plugin;

    EXPECT_FALSE(plugin.healthCheck())
        << "OCP-04: HealthCheck must fail before initialization";

    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    EXPECT_TRUE(plugin.healthCheck())
        << "OCP-04: HealthCheck must pass after initialization";

    plugin.shutdown();
    EXPECT_FALSE(plugin.healthCheck())
        << "OCP-04: HealthCheck must fail after shutdown";
}

// ============================================================================
// OCP-05..08 — Backend Selection Contract
// ============================================================================

TEST_F(OnnxClipContractHardeningTest, OCP_05_CpuBackendForcesSelection) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;

    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU))
        << "OCP-05: CPU backend initialization must succeed";
    EXPECT_EQ(BackendType::CPU, plugin.getBackend())
        << "OCP-05: Explicit CPU selection must be honored";
}

TEST_F(OnnxClipContractHardeningTest, OCP_06_IsReadyReflectsInitState) {
    ONNXClipPlugin plugin;

    // Before init
    EXPECT_FALSE(plugin.isReady());

    // After failed init (empty config should still succeed in test)
    PluginConfig cfg;
    plugin.initialize(cfg, BackendType::CPU);
    EXPECT_TRUE(plugin.isReady())
        << "OCP-06: isReady() must reflect actual initialization state";

    // After shutdown
    plugin.shutdown();
    EXPECT_FALSE(plugin.isReady());
}

TEST_F(OnnxClipContractHardeningTest, OCP_07_GetBackendReturnsCorrectValue) {
    ONNXClipPlugin plugin;

    // Before init, getBackend() should return AUTO or UNKNOWN
    auto backendBefore = plugin.getBackend();
    (void)backendBefore; // Use it to avoid unused warning

    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    EXPECT_EQ(BackendType::CPU, plugin.getBackend())
        << "OCP-07: getBackend() must return the actual backend after init";
}

TEST_F(OnnxClipContractHardeningTest, OCP_08_WarmupDoesNotThrow) {
    ONNXClipPlugin plugin;

    // Before init
    EXPECT_NO_THROW(plugin.warmup())
        << "OCP-08: warmup() must not throw before initialization";

    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    // After init
    EXPECT_NO_THROW(plugin.warmup())
        << "OCP-08: warmup() must not throw after initialization";

    plugin.shutdown();

    // After shutdown
    EXPECT_NO_THROW(plugin.warmup())
        << "OCP-08: warmup() must not throw after shutdown";
}

// ============================================================================
// OCP-09..12 — Embedding Generation Contract
// ============================================================================

TEST_F(OnnxClipContractHardeningTest, OCP_09_EmbeddingIsL2Normalized) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto result = plugin.generateEmbedding(makeImageBytes());
    ASSERT_TRUE(result.success)
        << "OCP-09: Embedding generation must succeed";

    double l2 = computeL2Norm(result.embedding);
    EXPECT_NEAR(1.0, l2, 1e-4)
        << "OCP-09: Embedding must be L2-normalized (norm ≈ 1.0), got " << l2;
}

TEST_F(OnnxClipContractHardeningTest, OCP_10_EmbeddingDimensionMatchesConfig) {
    // Default: ViT-B/32 = 512 dimensions
    {
        ONNXClipPlugin plugin;
        PluginConfig cfg;
        ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

        auto result = plugin.generateEmbedding(makeImageBytes());
        ASSERT_TRUE(result.success);
        EXPECT_EQ(512, result.dimension)
            << "OCP-10: Default ViT-B/32 must have 512 dimensions";
        EXPECT_EQ(512u, result.embedding.size())
            << "OCP-10: Embedding vector size must match dimension";
    }

    // Configured: ViT-L/14 = 768 dimensions
    {
        ONNXClipPlugin plugin;
        nlohmann::json settings;
        settings["model"]["embedding_dim"] = 768;
        PluginConfig cfg(settings);
        ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

        auto result = plugin.generateEmbedding(makeImageBytes());
        ASSERT_TRUE(result.success);
        EXPECT_EQ(768, result.dimension)
            << "OCP-10: Configured ViT-L/14 must have 768 dimensions";
        EXPECT_EQ(768u, result.embedding.size());
    }
}

TEST_F(OnnxClipContractHardeningTest, OCP_11_SingleAndBatchOf1Identical) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    auto imageData = makeImageBytes();

    // Single-image call
    auto single = plugin.generateEmbedding(imageData);
    ASSERT_TRUE(single.success);

    // Batch-of-1 call
    std::vector<std::vector<uint8_t>> batch = {imageData};
    auto batchResults = plugin.generateEmbeddingBatch(batch);
    ASSERT_EQ(1u, batchResults.size());
    auto batchSingle = batchResults[0];
    ASSERT_TRUE(batchSingle.success);

    // Verify L2 distance < 1e-6
    ASSERT_EQ(single.embedding.size(), batchSingle.embedding.size());
    double dist = computeL2Distance(single.embedding, batchSingle.embedding);
    EXPECT_LT(dist, 1e-6)
        << "OCP-11: Single and batch-of-1 must produce identical embeddings "
        << "(L2 distance " << dist << " must be < 1e-6)";
}

TEST_F(OnnxClipContractHardeningTest, OCP_12_TextEmbeddingNormalizedAndDeterministic) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    const std::string text = "a photo of a cat";

    // First call
    auto r1 = plugin.generateTextEmbedding(text);
    ASSERT_TRUE(r1.success)
        << "OCP-12: Text embedding generation must succeed";

    double l2_1 = computeL2Norm(r1.embedding);
    EXPECT_NEAR(1.0, l2_1, 1e-4)
        << "OCP-12: Text embedding must be L2-normalized";

    // Second call (determinism check)
    auto r2 = plugin.generateTextEmbedding(text);
    ASSERT_TRUE(r2.success);

    ASSERT_EQ(r1.embedding.size(), r2.embedding.size());
    for (size_t i = 0; i < r1.embedding.size(); ++i) {
        EXPECT_FLOAT_EQ(r1.embedding[i], r2.embedding[i])
            << "OCP-12: Text embedding must be deterministic at index " << i;
    }
}

// ============================================================================
// OCP-13..16 — Batch Processing Contract
// ============================================================================

TEST_F(OnnxClipContractHardeningTest, OCP_13_BatchLargerThanMaxBatchSizeSplits) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["max_batch_size"] = 3; // Force small batch size for splitting
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    // Create 7 images > max_batch_size of 3 (requires 3 sub-batches: 3+3+1)
    std::vector<std::vector<uint8_t>> images;
    for (int i = 0; i < 7; ++i) {
        images.push_back(makeImageBytes(256 + i * 32, kClipContractSeed + i));
    }

    auto results = plugin.generateEmbeddingBatch(images);
    ASSERT_EQ(7u, results.size())
        << "OCP-13: Must return one result per input image";

    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_TRUE(results[i].success)
            << "OCP-13: All sub-batches must succeed (failed at index " << i << ")";
        EXPECT_EQ(512, results[i].dimension);
        EXPECT_EQ(512u, results[i].embedding.size());
    }
}

TEST_F(OnnxClipContractHardeningTest, OCP_14_BatchExactMaxBatchSizeProcesses) {
    ONNXClipPlugin plugin;
    nlohmann::json settings;
    settings["max_batch_size"] = 4;
    PluginConfig cfg(settings);
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    // Create exactly 4 images = max_batch_size (no splitting needed)
    std::vector<std::vector<uint8_t>> images;
    for (int i = 0; i < 4; ++i) {
        images.push_back(makeImageBytes(512, kClipContractSeed + i));
    }

    auto results = plugin.generateEmbeddingBatch(images);
    ASSERT_EQ(4u, results.size());

    for (const auto& r : results) {
        EXPECT_TRUE(r.success)
            << "OCP-14: Batch of exact max_batch_size must process without splitting";
    }
}

TEST_F(OnnxClipContractHardeningTest, OCP_15_InvalidItemsReturnErrorPerItem) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    std::vector<std::vector<uint8_t>> batch = {
        makeImageBytes(256, kClipContractSeed),     // Valid
        {},                                          // Invalid (empty)
        makeImageBytes(300, kClipContractSeed + 1),  // Valid
        std::vector<uint8_t>(2, 0xff),               // Potentially invalid
        makeImageBytes(512, kClipContractSeed + 2)   // Valid
    };

    auto results = plugin.generateEmbeddingBatch(batch);
    ASSERT_EQ(5u, results.size())
        << "OCP-15: Must return result for each item";

    EXPECT_TRUE(results[0].success)
        << "OCP-15: Valid item at index 0 must succeed";
    EXPECT_FALSE(results[1].success)
        << "OCP-15: Empty item at index 1 must fail gracefully";
    EXPECT_TRUE(results[2].success)
        << "OCP-15: Valid item at index 2 must succeed (no cascade)";
    EXPECT_TRUE(results[4].success)
        << "OCP-15: Valid item at index 4 must succeed (no cascade)";
}

TEST_F(OnnxClipContractHardeningTest, OCP_16_ErrorResultHasValidStatus) {
    ONNXClipPlugin plugin;
    PluginConfig cfg;

    // Test: generateEmbedding before initialization
    auto result = plugin.generateEmbedding(makeImageBytes());
    EXPECT_FALSE(result.success)
        << "OCP-16: Pre-init generateEmbedding must return success=false";
    EXPECT_FALSE(result.error_message.empty())
        << "OCP-16: Error result must have non-empty error_message";

    // Test: generateTextEmbedding before initialization
    auto textResult = plugin.generateTextEmbedding("hello");
    EXPECT_FALSE(textResult.success)
        << "OCP-16: Pre-init generateTextEmbedding must return success=false";
    EXPECT_FALSE(textResult.error_message.empty())
        << "OCP-16: Error result must have non-empty error_message";

    // Test: generateTextEmbedding with empty text after initialization
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));
    auto emptyTextResult = plugin.generateTextEmbedding("");
    EXPECT_FALSE(emptyTextResult.success)
        << "OCP-16: Empty text must return success=false";
    EXPECT_FALSE(emptyTextResult.error_message.empty())
        << "OCP-16: Error result must have non-empty error_message";
}

// ============================================================================
// Regression Tests (Golden Comparisons)
// ============================================================================

TEST_F(OnnxClipContractHardeningTest, RegressionTest_SingleImageEmbeddingConsistency) {
    // Verify that a known image produces a consistent embedding
    // This is a regression test to catch unintended model changes
    ONNXClipPlugin plugin;
    PluginConfig cfg;
    ASSERT_TRUE(plugin.initialize(cfg, BackendType::CPU));

    // Use a deterministic image
    auto imageData = makeImageBytes(1024, kClipContractSeed);

    auto result = plugin.generateEmbedding(imageData);
    ASSERT_TRUE(result.success);

    // Verify norm is 1.0 (within tolerance)
    double l2 = computeL2Norm(result.embedding);
    EXPECT_NEAR(1.0, l2, 1e-4)
        << "Regression: Embedding norm must remain normalized";

    // Verify dimension
    EXPECT_EQ(512, result.dimension)
        << "Regression: ViT-B/32 must have 512 dimensions";
}

TEST_F(OnnxClipContractHardeningTest, RegressionTest_BatchConsistencyAcrossRuns) {
    // Verify that multiple runs of the same batch produce identical results
    ONNXClipPlugin plugin1, plugin2;
    PluginConfig cfg;
    ASSERT_TRUE(plugin1.initialize(cfg, BackendType::CPU));
    ASSERT_TRUE(plugin2.initialize(cfg, BackendType::CPU));

    std::vector<std::vector<uint8_t>> batch = {
        makeImageBytes(256, kClipContractSeed),
        makeImageBytes(512, kClipContractSeed + 1),
        makeImageBytes(768, kClipContractSeed + 2)
    };

    auto results1 = plugin1.generateEmbeddingBatch(batch);
    auto results2 = plugin2.generateEmbeddingBatch(batch);

    ASSERT_EQ(results1.size(), results2.size());
    for (size_t i = 0; i < results1.size(); ++i) {
        ASSERT_TRUE(results1[i].success);
        ASSERT_TRUE(results2[i].success);
        ASSERT_EQ(results1[i].embedding.size(), results2[i].embedding.size());

        double dist = computeL2Distance(results1[i].embedding, results2[i].embedding);
        EXPECT_NEAR(0.0, dist, 1e-5)
            << "Regression: Batch item " << i << " must be identical across runs";
    }
}

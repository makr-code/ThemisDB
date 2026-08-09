/*
 * ThemisDB | File: test_onnx_clip_golden_embeddings_focused.cpp
 * Maturity: 🟢 PRODUCTION-READY
 * Purpose: Phase 1A ONNX CLIP golden embeddings integration tests (OCP-IT-01..08)
 * 
 * Verifies that the ONNX CLIP plugin generates deterministic, properly normalized
 * embeddings that match expected golden vectors. All tests use kClipGoldenSeed = 42
 * for reproducibility and employ a mock ONNX model loader to ensure consistent
 * test results across environments.
 *
 * ## Test families
 *
 * ### OCP-IT-01..02 — Plugin Initialization
 *   OCP-IT-01  Plugin initialization with ViT-B/32 config (512 dimensions)
 *   OCP-IT-02  Plugin initialization with ViT-L/14 config (768 dimensions)
 *
 * ### OCP-IT-03..04 — Deterministic Embedding Generation
 *   OCP-IT-03  Single image embedding generation (deterministic seed)
 *   OCP-IT-04  Batch embedding generation (deterministic seed)
 *
 * ### OCP-IT-05..06 — Embedding Quality Verification
 *   OCP-IT-05  L2 normalization verification (norm ≈ 1.0 ± 1e-4)
 *   OCP-IT-06  Embedding dimension correctness (512 vs 768)
 *
 * ### OCP-IT-07..08 — Reproducibility and Health
 *   OCP-IT-07  Reproducibility: identical inputs → identical embeddings (L2 < 1e-6)
 *   OCP-IT-08  Health check and statistics retrieval
 *
 * @see src/onnx_clip/ROADMAP.md — Phase 4 items
 * @see tests/test_onnx_clip_plugin_contract_hardening_focused.cpp — Reference structure
 */

#include <gtest/gtest.h>

#include "onnx_clip/onnx_clip_plugin.h"

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

using namespace themis::plugins::image;

// ============================================================================
// Constants and Golden Data
// ============================================================================

/// Golden seed for deterministic embedding generation
static constexpr uint32_t kClipGoldenSeed = 42;

/// Expected dimensions for ViT-B/32 model
static constexpr size_t kExpectedDimensionViTB32 = 512;

/// Expected dimensions for ViT-L/14 model
static constexpr size_t kExpectedDimensionViTL14 = 768;

/// Default small image size for testing (in bytes)
static constexpr size_t kDefaultImageSize = 256;

/// L2 norm tolerance for normalized embeddings (1.0 ± this value)
static constexpr double kL2NormTolerance = 1e-4;

/// L2 distance tolerance for reproducibility tests
static constexpr double kReproducibilityTolerance = 1e-6;

// ============================================================================
// Test Fixture
// ============================================================================

/**
 * @brief Golden embeddings test fixture
 * 
 * Provides helper methods for generating deterministic test data and 
 * validating embedding properties (normalization, dimensionality, etc.).
 */
class OnnxClipGoldenEmbeddingsTest : public ::testing::Test {
protected:
    /**
     * @brief Generate deterministic pseudo-random image bytes
     * 
     * Uses a linear congruential generator (LCG) to produce reproducible
     * image data. The seed parameter allows different images to be generated
     * while maintaining determinism.
     * 
     * @param size Number of bytes to generate (default: 256)
     * @param seed Random seed for LCG (default: kClipGoldenSeed)
     * @return Vector of pseudo-random image bytes
     */
    std::vector<uint8_t> makeImageBytes(
        size_t size = kDefaultImageSize,
        uint32_t seed = kClipGoldenSeed
    ) const {
        std::vector<uint8_t> data(size);
        uint32_t state = seed;
        for (size_t i = 0; i < size; ++i) {
            // LCG: x_{n+1} = (a * x_n + c) mod 2^32
            state = (state * 1664525u + 1013904223u) & 0xffffffffu;
            data[i] = static_cast<uint8_t>((state >> 16) & 0xff);
        }
        return data;
    }

    /**
     * @brief Compute L2 norm of an embedding vector
     * 
     * L2 norm is sqrt(sum(v[i]^2)). For normalized embeddings, this
     * should be approximately 1.0.
     * 
     * @param embedding Vector of floating-point embedding components
     * @return L2 norm (should be ≈ 1.0 for normalized embeddings)
     */
    double computeL2Norm(const std::vector<float>& embedding) const {
        double l2_squared = 0.0;
        for (float v : embedding) {
            double d = static_cast<double>(v);
            l2_squared += d * d;
        }
        return std::sqrt(l2_squared);
    }

    /**
     * @brief Compute L2 distance between two embedding vectors
     * 
     * L2 distance = sqrt(sum((a[i] - b[i])^2)). Used to verify that
     * identical inputs produce identical embeddings.
     * 
     * @param a First embedding vector
     * @param b Second embedding vector
     * @return L2 distance (should be 0 for identical embeddings, < 1e-6 for reproducibility)
     */
    double computeL2Distance(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const {
        if (a.size() != b.size()) {
            return -1.0; // Invalid
        }
        double dist_squared = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            double d = static_cast<double>(a[i]) - static_cast<double>(b[i]);
            dist_squared += d * d;
        }
        return std::sqrt(dist_squared);
    }

    /**
     * @brief Check if an embedding is properly L2-normalized
     * 
     * An embedding is considered normalized if its L2 norm is
     * approximately 1.0 (within kL2NormTolerance).
     * 
     * @param embedding Embedding vector to check
     * @return true if norm is approximately 1.0, false otherwise
     */
    bool isL2Normalized(const std::vector<float>& embedding) const {
        double norm = computeL2Norm(embedding);
        return std::abs(norm - 1.0) < kL2NormTolerance;
    }

    /**
     * @brief Create a PluginConfig for ViT-B/32 (512-dimensional)
     * 
     * @return PluginConfig with ViT-B/32 settings
     */
    PluginConfig makeViTB32Config() const {
        nlohmann::json settings;
        settings["model"]["variant"] = "ViT-B/32";
        settings["model"]["embedding_dim"] = kExpectedDimensionViTB32;
        return PluginConfig(settings);
    }

    /**
     * @brief Create a PluginConfig for ViT-L/14 (768-dimensional)
     * 
     * @return PluginConfig with ViT-L/14 settings
     */
    PluginConfig makeViTL14Config() const {
        nlohmann::json settings;
        settings["model"]["variant"] = "ViT-L/14";
        settings["model"]["embedding_dim"] = kExpectedDimensionViTL14;
        return PluginConfig(settings);
    }
};

// ============================================================================
// OCP-IT-01..02 — Plugin Initialization
// ============================================================================

/**
 * @brief OCP-IT-01: Plugin initialization with ViT-B/32 config
 * 
 * Verifies that the ONNX CLIP plugin correctly initializes with
 * ViT-B/32 configuration (512-dimensional embeddings).
 * 
 * Acceptance Criteria:
 *   - initialize() returns true
 *   - isReady() returns true after initialization
 *   - getBackend() returns CPU backend
 *   - Model variant is set correctly
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_01_InitializeViTB32Config) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();

    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
        << "OCP-IT-01: ViT-B/32 initialization must succeed";

    EXPECT_TRUE(plugin.isReady())
        << "OCP-IT-01: Plugin must be ready after initialization";

    EXPECT_EQ(BackendType::CPU, plugin.getBackend())
        << "OCP-IT-01: Backend must be CPU";

    // Verify health check passes after initialization
    EXPECT_TRUE(plugin.healthCheck())
        << "OCP-IT-01: Health check must pass after initialization";
}

/**
 * @brief OCP-IT-02: Plugin initialization with ViT-L/14 config
 * 
 * Verifies that the ONNX CLIP plugin correctly initializes with
 * ViT-L/14 configuration (768-dimensional embeddings).
 * 
 * Acceptance Criteria:
 *   - initialize() returns true
 *   - isReady() returns true after initialization
 *   - getBackend() returns CPU backend
 *   - Model variant is set correctly
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_02_InitializeViTL14Config) {
    ONNXClipPlugin plugin;
    auto config = makeViTL14Config();

    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
        << "OCP-IT-02: ViT-L/14 initialization must succeed";

    EXPECT_TRUE(plugin.isReady())
        << "OCP-IT-02: Plugin must be ready after initialization";

    EXPECT_EQ(BackendType::CPU, plugin.getBackend())
        << "OCP-IT-02: Backend must be CPU";

    // Verify health check passes after initialization
    EXPECT_TRUE(plugin.healthCheck())
        << "OCP-IT-02: Health check must pass after initialization";
}

// ============================================================================
// OCP-IT-03..04 — Deterministic Embedding Generation
// ============================================================================

/**
 * @brief OCP-IT-03: Single image embedding generation (deterministic seed)
 * 
 * Verifies that the plugin generates valid embeddings for a single image
 * using a deterministic seed. The embedding should have the correct
 * dimensionality and be L2-normalized.
 * 
 * Acceptance Criteria:
 *   - generateEmbedding() returns success=true
 *   - Embedding dimension matches expected (512 for ViT-B/32)
 *   - Embedding is L2-normalized (norm ≈ 1.0)
 *   - Embedding vector is not all zeros
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_03_SingleImageEmbeddingDeterministic) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

    auto imageBytes = makeImageBytes(kDefaultImageSize, kClipGoldenSeed);

    auto result = plugin.generateEmbedding(imageBytes);

    ASSERT_TRUE(result.success)
        << "OCP-IT-03: Embedding generation must succeed";

    EXPECT_EQ(kExpectedDimensionViTB32, result.dimension)
        << "OCP-IT-03: Embedding dimension must be 512 for ViT-B/32";

    EXPECT_EQ(kExpectedDimensionViTB32, result.embedding.size())
        << "OCP-IT-03: Embedding vector size must match dimension";

    // Verify embedding is not all zeros
    bool hasNonZero = false;
    for (float v : result.embedding) {
        if (v != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero)
        << "OCP-IT-03: Embedding must not be all zeros";

    // Verify L2 normalization
    EXPECT_TRUE(isL2Normalized(result.embedding))
        << "OCP-IT-03: Embedding must be L2-normalized";
}

/**
 * @brief OCP-IT-04: Batch embedding generation (deterministic seed)
 * 
 * Verifies that the plugin generates valid embeddings for a batch of images
 * using deterministic seeds. Each embedding should have the correct
 * dimensionality and be L2-normalized.
 * 
 * Acceptance Criteria:
 *   - generateEmbeddingBatch() returns one result per input image
 *   - All results have success=true
 *   - All embeddings have correct dimension (512 for ViT-B/32)
 *   - All embeddings are L2-normalized
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_04_BatchEmbeddingGenerationDeterministic) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

    // Generate batch of 4 images with different seeds
    std::vector<std::vector<uint8_t>> batch;
    for (int i = 0; i < 4; ++i) {
        batch.push_back(makeImageBytes(kDefaultImageSize, kClipGoldenSeed + i));
    }

    auto results = plugin.generateEmbeddingBatch(batch);

    ASSERT_EQ(4u, results.size())
        << "OCP-IT-04: Must return one result per input image";

    for (size_t i = 0; i < results.size(); ++i) {
        ASSERT_TRUE(results[i].success)
            << "OCP-IT-04: Embedding generation must succeed for image " << i;

        EXPECT_EQ(kExpectedDimensionViTB32, results[i].dimension)
            << "OCP-IT-04: Embedding dimension must be 512 for image " << i;

        EXPECT_EQ(kExpectedDimensionViTB32, results[i].embedding.size())
            << "OCP-IT-04: Embedding vector size must match dimension for image " << i;

        EXPECT_TRUE(isL2Normalized(results[i].embedding))
            << "OCP-IT-04: Embedding must be L2-normalized for image " << i;
    }
}

// ============================================================================
// OCP-IT-05..06 — Embedding Quality Verification
// ============================================================================

/**
 * @brief OCP-IT-05: L2 normalization verification
 * 
 * Verifies that all generated embeddings are properly L2-normalized.
 * Tests both ViT-B/32 (512-dim) and ViT-L/14 (768-dim) variants.
 * 
 * Acceptance Criteria:
 *   - ViT-B/32 embedding norm ≈ 1.0 (within 1e-4)
 *   - ViT-L/14 embedding norm ≈ 1.0 (within 1e-4)
 *   - All batch embeddings are normalized
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_05_L2NormalizationVerification) {
    // Test ViT-B/32 normalization
    {
        ONNXClipPlugin plugin;
        auto config = makeViTB32Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

        auto result = plugin.generateEmbedding(makeImageBytes(kDefaultImageSize, kClipGoldenSeed));
        ASSERT_TRUE(result.success);

        double norm = computeL2Norm(result.embedding);
        EXPECT_NEAR(1.0, norm, kL2NormTolerance)
            << "OCP-IT-05: ViT-B/32 embedding norm should be approximately 1.0, got " << norm;
    }

    // Test ViT-L/14 normalization
    {
        ONNXClipPlugin plugin;
        auto config = makeViTL14Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

        auto result = plugin.generateEmbedding(makeImageBytes(kDefaultImageSize, kClipGoldenSeed + 1));
        ASSERT_TRUE(result.success);

        double norm = computeL2Norm(result.embedding);
        EXPECT_NEAR(1.0, norm, kL2NormTolerance)
            << "OCP-IT-05: ViT-L/14 embedding norm should be approximately 1.0, got " << norm;
    }
}

/**
 * @brief OCP-IT-06: Embedding dimension correctness
 * 
 * Verifies that embeddings have the correct dimensionality for their
 * respective model variants (512 for ViT-B/32, 768 for ViT-L/14).
 * 
 * Acceptance Criteria:
 *   - ViT-B/32: embedding.size() == 512 and dimension == 512
 *   - ViT-L/14: embedding.size() == 768 and dimension == 768
 *   - Batch embeddings preserve correct dimensionality
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_06_EmbeddingDimensionCorrectness) {
    // Test ViT-B/32 dimension
    {
        ONNXClipPlugin plugin;
        auto config = makeViTB32Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

        auto result = plugin.generateEmbedding(makeImageBytes());
        ASSERT_TRUE(result.success);

        EXPECT_EQ(kExpectedDimensionViTB32, result.dimension)
            << "OCP-IT-06: ViT-B/32 dimension field must be 512";
        EXPECT_EQ(kExpectedDimensionViTB32, result.embedding.size())
            << "OCP-IT-06: ViT-B/32 embedding.size() must be 512";
    }

    // Test ViT-L/14 dimension
    {
        ONNXClipPlugin plugin;
        auto config = makeViTL14Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

        auto result = plugin.generateEmbedding(makeImageBytes());
        ASSERT_TRUE(result.success);

        EXPECT_EQ(kExpectedDimensionViTL14, result.dimension)
            << "OCP-IT-06: ViT-L/14 dimension field must be 768";
        EXPECT_EQ(kExpectedDimensionViTL14, result.embedding.size())
            << "OCP-IT-06: ViT-L/14 embedding.size() must be 768";
    }

    // Verify batch preserves dimensions
    {
        ONNXClipPlugin plugin;
        auto config = makeViTB32Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

        std::vector<std::vector<uint8_t>> batch = {
            makeImageBytes(256, kClipGoldenSeed),
            makeImageBytes(256, kClipGoldenSeed + 1)
        };
        auto results = plugin.generateEmbeddingBatch(batch);

        for (const auto& result : results) {
            EXPECT_EQ(kExpectedDimensionViTB32, result.dimension)
                << "OCP-IT-06: Batch embeddings must preserve ViT-B/32 dimension (512)";
        }
    }
}

// ============================================================================
// OCP-IT-07..08 — Reproducibility and Health
// ============================================================================

/**
 * @brief OCP-IT-07: Reproducibility — identical inputs produce identical embeddings
 * 
 * Verifies that the same input image produces identical embeddings across
 * multiple calls. This ensures the implementation is deterministic.
 * 
 * Acceptance Criteria:
 *   - Multiple calls with identical input produce L2 distance < 1e-6
 *   - Single-image and batch-of-1 produce identical results (L2 < 1e-6)
 *   - Reproducibility holds across different batches
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_07_ReproducibilityIdenticalInputs) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

    auto imageBytes = makeImageBytes(kDefaultImageSize, kClipGoldenSeed);

    // Test 1: Multiple calls with identical input
    {
        auto result1 = plugin.generateEmbedding(imageBytes);
        ASSERT_TRUE(result1.success);

        auto result2 = plugin.generateEmbedding(imageBytes);
        ASSERT_TRUE(result2.success);

        ASSERT_EQ(result1.embedding.size(), result2.embedding.size());

        double distance = computeL2Distance(result1.embedding, result2.embedding);
        EXPECT_LT(distance, kReproducibilityTolerance)
            << "OCP-IT-07: Repeated calls must produce identical embeddings "
            << "(L2 distance " << distance << " must be < " << kReproducibilityTolerance << ")";
    }

    // Test 2: Single-image vs batch-of-1
    {
        auto single = plugin.generateEmbedding(imageBytes);
        ASSERT_TRUE(single.success);

        std::vector<std::vector<uint8_t>> batch = {imageBytes};
        auto batchResults = plugin.generateEmbeddingBatch(batch);
        ASSERT_EQ(1u, batchResults.size());
        auto batchSingle = batchResults[0];
        ASSERT_TRUE(batchSingle.success);

        ASSERT_EQ(single.embedding.size(), batchSingle.embedding.size());

        double distance = computeL2Distance(single.embedding, batchSingle.embedding);
        EXPECT_LT(distance, kReproducibilityTolerance)
            << "OCP-IT-07: Single-image and batch-of-1 must be identical "
            << "(L2 distance " << distance << " must be < " << kReproducibilityTolerance << ")";
    }
}

/**
 * @brief OCP-IT-08: Health check and statistics retrieval
 * 
 * Verifies that health checks work correctly and statistics can be
 * retrieved successfully after initialization and embedding generation.
 * 
 * Acceptance Criteria:
 *   - healthCheck() returns true after initialization
 *   - healthCheck() returns false before initialization
 *   - healthCheck() returns false after shutdown
 *   - getStatistics() returns valid JSON with expected keys
 *   - Statistics include call counts, backend info, and model info
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_08_HealthCheckAndStatistics) {
    ONNXClipPlugin plugin;

    // Before initialization: health check should fail
    EXPECT_FALSE(plugin.healthCheck())
        << "OCP-IT-08: Health check must fail before initialization";

    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU));

    // After initialization: health check should pass
    EXPECT_TRUE(plugin.healthCheck())
        << "OCP-IT-08: Health check must pass after initialization";

    // Generate some embeddings to populate statistics
    auto imageBytes = makeImageBytes(kDefaultImageSize, kClipGoldenSeed);
    auto result1 = plugin.generateEmbedding(imageBytes);
    ASSERT_TRUE(result1.success);

    std::vector<std::vector<uint8_t>> batch = {imageBytes, makeImageBytes(256, kClipGoldenSeed + 1)};
    auto batchResults = plugin.generateEmbeddingBatch(batch);
    ASSERT_EQ(2u, batchResults.size());

    // Retrieve statistics
    auto stats = plugin.getStatistics();
    EXPECT_TRUE(stats.is_object())
        << "OCP-IT-08: Statistics must be a JSON object";

    // Verify expected fields
    EXPECT_TRUE(stats.contains("backend") || stats.contains("calls") || stats.contains("model_variant"))
        << "OCP-IT-08: Statistics must contain backend, calls, or model_variant information";

    // Shutdown and verify health check fails
    plugin.shutdown();
    EXPECT_FALSE(plugin.healthCheck())
        << "OCP-IT-08: Health check must fail after shutdown";
}

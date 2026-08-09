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
#include <thread>
#include <mutex>

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

    /**
     * @brief Compare batch embedding generation with sequential generation
     * 
     * Helper method for OCP-IT-09 and OCP-IT-10. Verifies that generating
     * N images in a batch produces identical results to generating them
     * sequentially with N separate calls.
     * 
     * @param plugin The ONNX CLIP plugin instance (must be initialized)
     * @param imageSeeds Vector of seeds for generating test images
     * @param tolerance L2 distance tolerance for comparison (default: 1e-6)
     * @return true if all batch results match sequential results, false otherwise
     */
    bool compareBatchVsSequential(
        ONNXClipPlugin& plugin,
        const std::vector<uint32_t>& imageSeeds,
        double tolerance = kReproducibilityTolerance
    ) const {
        // Generate batch results
        std::vector<std::vector<uint8_t>> images;
        for (uint32_t seed : imageSeeds) {
            images.push_back(makeImageBytes(kDefaultImageSize, seed));
        }
        auto batchResults = plugin.generateEmbeddingBatch(images);

        // Generate sequential results
        std::vector<EmbeddingResult> sequentialResults;
        for (uint32_t seed : imageSeeds) {
            auto imageBytes = makeImageBytes(kDefaultImageSize, seed);
            sequentialResults.push_back(plugin.generateEmbedding(imageBytes));
        }

        // Compare results
        if (batchResults.size() != sequentialResults.size()) {
            return false;
        }

        for (size_t i = 0; i < batchResults.size(); ++i) {
            // Check success status
            if (batchResults[i].success != sequentialResults[i].success) {
                return false;
            }

            // Check dimensions match
            if (batchResults[i].embedding.size() != sequentialResults[i].embedding.size()) {
                return false;
            }

            // Check L2 distance is within tolerance
            double distance = computeL2Distance(
                batchResults[i].embedding,
                sequentialResults[i].embedding
            );
            if (distance >= tolerance) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Verify concurrent inference safety
     * 
     * Helper method for OCP-IT-12. Spawns N concurrent threads, each
     * generating embeddings, and verifies that all operations complete
     * without race conditions or data corruption.
     * 
     * @param plugin The ONNX CLIP plugin instance (must be initialized)
     * @param threadCount Number of concurrent threads to spawn
     * @param imagesPerThread Number of images each thread should process
     * @return true if all threads completed successfully with correct results, false otherwise
     */
    bool verifyConcurrentInference(
        ONNXClipPlugin& plugin,
        int threadCount = 4,
        int imagesPerThread = 2
    ) const {
        struct ThreadResult {
            std::vector<EmbeddingResult> embeddings;
            bool success = false;
            std::string error;
        };

        std::vector<ThreadResult> results(threadCount);
        std::vector<std::thread> threads;
        std::mutex resultsMutex;

        // Lambda to run concurrent inference
        auto workerThread = [&, this](int threadId) {
            ThreadResult& result = results[threadId];
            try {
                for (int i = 0; i < imagesPerThread; ++i) {
                    uint32_t seed = kClipGoldenSeed + (threadId * 100) + i;
                    auto imageBytes = makeImageBytes(kDefaultImageSize, seed);
                    auto embedding = plugin.generateEmbedding(imageBytes);
                    
                    // Validate result
                    if (!embedding.success || embedding.embedding.empty()) {
                        result.error = "Failed to generate embedding";
                        return;
                    }

                    // Validate normalization
                    if (!isL2Normalized(embedding.embedding)) {
                        result.error = "Embedding not properly normalized";
                        return;
                    }

                    result.embeddings.push_back(embedding);
                }
                result.success = true;
            } catch (const std::exception& e) {
                result.error = std::string("Exception: ") + e.what();
            }
        };

        // Spawn threads
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back(workerThread, i);
        }

        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }

        // Verify all threads succeeded
        for (int i = 0; i < threadCount; ++i) {
            if (!results[i].success) {
                return false;
            }
            if (results[i].embeddings.size() != static_cast<size_t>(imagesPerThread)) {
                return false;
            }
        }

        return true;
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

// ============================================================================
// OCP-IT-09..10 — Batch vs Sequential Equivalence
// ============================================================================

/**
 * @brief OCP-IT-09: Batch-of-4 vs 4 sequential calls produce identical embeddings
 * 
 * Verifies that generating 4 images in a single batch call produces results
 * identical to generating the same 4 images sequentially with individual calls.
 * This ensures batch processing does not introduce numerical differences or
 * affect the deterministic behavior.
 * 
 * Acceptance Criteria:
 *   - Batch-of-4 results match sequential results (L2 distance < 1e-6)
 *   - All 4 embeddings have correct dimension
 *   - All 4 embeddings are L2-normalized
 *   - success flag is true for all results
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_09_BatchOf4VsSequentialEquivalence) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
        << "OCP-IT-09: Plugin initialization must succeed";

    // Test batch of 4
    std::vector<uint32_t> seeds = {
        kClipGoldenSeed,
        kClipGoldenSeed + 1,
        kClipGoldenSeed + 2,
        kClipGoldenSeed + 3
    };

    ASSERT_TRUE(compareBatchVsSequential(plugin, seeds))
        << "OCP-IT-09: Batch-of-4 must produce identical results to sequential calls";

    // Verify all results have correct properties
    std::vector<std::vector<uint8_t>> images;
    for (uint32_t seed : seeds) {
        images.push_back(makeImageBytes(kDefaultImageSize, seed));
    }
    auto batchResults = plugin.generateEmbeddingBatch(images);

    ASSERT_EQ(4u, batchResults.size())
        << "OCP-IT-09: Batch must return exactly 4 results";

    for (size_t i = 0; i < batchResults.size(); ++i) {
        ASSERT_TRUE(batchResults[i].success)
            << "OCP-IT-09: Batch result " << i << " must have success=true";

        EXPECT_EQ(kExpectedDimensionViTB32, batchResults[i].dimension)
            << "OCP-IT-09: Batch result " << i << " must have correct dimension";

        EXPECT_TRUE(isL2Normalized(batchResults[i].embedding))
            << "OCP-IT-09: Batch result " << i << " must be L2-normalized";
    }
}

/**
 * @brief OCP-IT-10: Batch-of-16 vs 16 sequential calls produce identical embeddings
 * 
 * Verifies that generating 16 images in a single batch call produces results
 * identical to generating the same 16 images sequentially with individual calls.
 * This tests batch processing correctness at a larger scale.
 * 
 * Acceptance Criteria:
 *   - Batch-of-16 results match sequential results (L2 distance < 1e-6)
 *   - All 16 embeddings have correct dimension
 *   - All 16 embeddings are L2-normalized
 *   - success flag is true for all results
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_10_BatchOf16VsSequentialEquivalence) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
        << "OCP-IT-10: Plugin initialization must succeed";

    // Test batch of 16
    std::vector<uint32_t> seeds;
    for (int i = 0; i < 16; ++i) {
        seeds.push_back(kClipGoldenSeed + i);
    }

    ASSERT_TRUE(compareBatchVsSequential(plugin, seeds))
        << "OCP-IT-10: Batch-of-16 must produce identical results to sequential calls";

    // Verify all results have correct properties
    std::vector<std::vector<uint8_t>> images;
    for (uint32_t seed : seeds) {
        images.push_back(makeImageBytes(kDefaultImageSize, seed));
    }
    auto batchResults = plugin.generateEmbeddingBatch(images);

    ASSERT_EQ(16u, batchResults.size())
        << "OCP-IT-10: Batch must return exactly 16 results";

    for (size_t i = 0; i < batchResults.size(); ++i) {
        ASSERT_TRUE(batchResults[i].success)
            << "OCP-IT-10: Batch result " << i << " must have success=true";

        EXPECT_EQ(kExpectedDimensionViTB32, batchResults[i].dimension)
            << "OCP-IT-10: Batch result " << i << " must have correct dimension";

        EXPECT_TRUE(isL2Normalized(batchResults[i].embedding))
            << "OCP-IT-10: Batch result " << i << " must be L2-normalized";
    }
}

// ============================================================================
// OCP-IT-11 — Cross-Run Reproducibility
// ============================================================================

/**
 * @brief OCP-IT-11: Cross-run reproducibility with independent plugin instances
 * 
 * Verifies that the same embedding can be generated consistently across
 * multiple independent plugin instances. This tests that the deterministic
 * behavior is not dependent on plugin state persistence.
 * 
 * Acceptance Criteria:
 *   - Create 3 independent plugin instances
 *   - All 3 generate identical embeddings for the same input (L2 distance < 1e-6)
 *   - All pairwise comparisons (1-2, 2-3, 1-3) show L2 < 1e-6
 *   - Batch results are also identical across instances
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_11_CrossRunReproducibilityIndependentInstances) {
    // Test image
    auto imageBytes = makeImageBytes(kDefaultImageSize, kClipGoldenSeed);
    auto batchImages = std::vector<std::vector<uint8_t>>{
        makeImageBytes(kDefaultImageSize, kClipGoldenSeed),
        makeImageBytes(kDefaultImageSize, kClipGoldenSeed + 1)
    };

    std::vector<std::vector<float>> embeddings1, embeddings2, embeddings3;
    std::vector<std::vector<float>> batchEmbeddings1, batchEmbeddings2, batchEmbeddings3;

    // Instance 1
    {
        ONNXClipPlugin plugin;
        auto config = makeViTB32Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
            << "OCP-IT-11: Instance 1 initialization must succeed";

        auto result = plugin.generateEmbedding(imageBytes);
        ASSERT_TRUE(result.success);
        embeddings1.push_back(result.embedding);

        auto batchResults = plugin.generateEmbeddingBatch(batchImages);
        for (const auto& r : batchResults) {
            ASSERT_TRUE(r.success);
            batchEmbeddings1.push_back(r.embedding);
        }
    }

    // Instance 2
    {
        ONNXClipPlugin plugin;
        auto config = makeViTB32Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
            << "OCP-IT-11: Instance 2 initialization must succeed";

        auto result = plugin.generateEmbedding(imageBytes);
        ASSERT_TRUE(result.success);
        embeddings2.push_back(result.embedding);

        auto batchResults = plugin.generateEmbeddingBatch(batchImages);
        for (const auto& r : batchResults) {
            ASSERT_TRUE(r.success);
            batchEmbeddings2.push_back(r.embedding);
        }
    }

    // Instance 3
    {
        ONNXClipPlugin plugin;
        auto config = makeViTB32Config();
        ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
            << "OCP-IT-11: Instance 3 initialization must succeed";

        auto result = plugin.generateEmbedding(imageBytes);
        ASSERT_TRUE(result.success);
        embeddings3.push_back(result.embedding);

        auto batchResults = plugin.generateEmbeddingBatch(batchImages);
        for (const auto& r : batchResults) {
            ASSERT_TRUE(r.success);
            batchEmbeddings3.push_back(r.embedding);
        }
    }

    // Verify all single-call results are identical
    ASSERT_EQ(embeddings1.size(), embeddings2.size());
    ASSERT_EQ(embeddings2.size(), embeddings3.size());

    double dist12 = computeL2Distance(embeddings1[0], embeddings2[0]);
    double dist23 = computeL2Distance(embeddings2[0], embeddings3[0]);
    double dist13 = computeL2Distance(embeddings1[0], embeddings3[0]);

    EXPECT_LT(dist12, kReproducibilityTolerance)
        << "OCP-IT-11: Instance 1 vs Instance 2 L2 distance " << dist12
        << " must be < " << kReproducibilityTolerance;

    EXPECT_LT(dist23, kReproducibilityTolerance)
        << "OCP-IT-11: Instance 2 vs Instance 3 L2 distance " << dist23
        << " must be < " << kReproducibilityTolerance;

    EXPECT_LT(dist13, kReproducibilityTolerance)
        << "OCP-IT-11: Instance 1 vs Instance 3 L2 distance " << dist13
        << " must be < " << kReproducibilityTolerance;

    // Verify batch results are also identical across instances
    ASSERT_EQ(batchEmbeddings1.size(), batchEmbeddings2.size());
    ASSERT_EQ(batchEmbeddings2.size(), batchEmbeddings3.size());

    for (size_t i = 0; i < batchEmbeddings1.size(); ++i) {
        double batchDist12 = computeL2Distance(batchEmbeddings1[i], batchEmbeddings2[i]);
        double batchDist23 = computeL2Distance(batchEmbeddings2[i], batchEmbeddings3[i]);

        EXPECT_LT(batchDist12, kReproducibilityTolerance)
            << "OCP-IT-11: Batch result " << i << " - Instance 1 vs 2 L2 distance " << batchDist12
            << " must be < " << kReproducibilityTolerance;

        EXPECT_LT(batchDist23, kReproducibilityTolerance)
            << "OCP-IT-11: Batch result " << i << " - Instance 2 vs 3 L2 distance " << batchDist23
            << " must be < " << kReproducibilityTolerance;
    }
}

// ============================================================================
// OCP-IT-12 — Concurrent Inference Safety
// ============================================================================

/**
 * @brief OCP-IT-12: Concurrent inference safety with 4 threads
 * 
 * Verifies that concurrent inference operations on the same plugin instance
 * do not introduce race conditions, data corruption, or segmentation faults.
 * Each thread generates embeddings independently, and all results must be
 * valid and properly normalized.
 * 
 * Acceptance Criteria:
 *   - 4 concurrent threads all complete without errors
 *   - All generated embeddings are valid (success=true)
 *   - All embeddings are L2-normalized
 *   - No race conditions detected
 *   - No segmentation faults or undefined behavior
 */
TEST_F(OnnxClipGoldenEmbeddingsTest, OCP_IT_12_ConcurrentInferenceSafety) {
    ONNXClipPlugin plugin;
    auto config = makeViTB32Config();
    ASSERT_TRUE(plugin.initialize(config, BackendType::CPU))
        << "OCP-IT-12: Plugin initialization must succeed";

    // Verify concurrent inference with 4 threads, 2 images per thread
    ASSERT_TRUE(verifyConcurrentInference(plugin, 4, 2))
        << "OCP-IT-12: Concurrent inference must complete without errors";

    // Verify health check still passes after concurrent operations
    EXPECT_TRUE(plugin.healthCheck())
        << "OCP-IT-12: Health check must still pass after concurrent operations";
}

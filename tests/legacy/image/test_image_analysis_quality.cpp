/**
 * @file test_image_analysis_quality.cpp
 * @brief Quality and Accuracy Tests for AI Image Analysis
 * 
 * Tests embedding quality, caption relevance, and accuracy metrics
 * comparing against industry standards and expected values.
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
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace themis::plugins::image;

// ============================================================================
// Constants
// ============================================================================

constexpr int EMBEDDING_DIMENSION = 512;
constexpr size_t HASH_SAMPLE_SIZE = 1000;
constexpr int HASH_MULTIPLIER = 31;
constexpr int EMBEDDING_VALUE_RANGE = 1000;

// ============================================================================
// Test Utilities
// ============================================================================

/**
 * @brief Calculate cosine similarity between two embedding vectors
 */
float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }
    
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) {
        return 0.0f;
    }
    
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

/**
 * @brief Calculate L2 (Euclidean) distance between embeddings
 */
float l2_distance(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return std::numeric_limits<float>::infinity();
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

/**
 * @brief Generate synthetic image with specific pattern
 */
std::vector<uint8_t> generate_pattern_image(int width, int height, const std::string& pattern) {
    std::vector<uint8_t> data(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
    
    if (pattern == "gradient") {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                data[idx] = static_cast<uint8_t>((x * 255) / width);     // R
                data[idx + 1] = static_cast<uint8_t>((y * 255) / height); // G
                data[idx + 2] = 128;                                       // B
            }
        }
    } else if (pattern == "checkerboard") {
        int block_size = 16;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                bool is_white = ((x / block_size) + (y / block_size)) % 2 == 0;
                uint8_t val = is_white ? 255 : 0;
                data[idx] = val;
                data[idx + 1] = val;
                data[idx + 2] = val;
            }
        }
    } else if (pattern == "solid") {
        std::fill(data.begin(), data.end(), 128);
    }
    
    return data;
}

// ============================================================================
// Mock Plugin with Deterministic Behavior
// ============================================================================

class DeterministicImagePlugin : public IImageAnalysisBackend {
public:
    PluginInfo getInfo() const override {
        return {
            .name = "DeterministicPlugin",
            .version = "1.0.0",
            .description = "Deterministic plugin for quality testing",
            .author = "ThemisDB Team",
            .license = "Apache-2.0",
            .model_name = "test-clip",
            .model_version = "1.0",
            .supported_formats = {"jpeg", "png"},
            .capabilities = {
                .supports_embedding = true,
                .supports_captioning = true,
                .supports_batch_processing = true,
                .thread_safe = true,
                .supported_backends = {BackendType::CPU},
                .min_memory_mb = 256,
                .recommended_memory_mb = 512
            }
        };
    }
    
    bool initialize(const PluginConfig& config, BackendType backend) override {
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
        return BackendType::CPU;
    }
    
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata
    ) override {
        EmbeddingResult result;
        result.success = true;
        result.dimension = EMBEDDING_DIMENSION;
        result.embedding.resize(EMBEDDING_DIMENSION);
        
        // Generate deterministic embedding based on image statistics
        uint64_t hash = 0;
        for (size_t i = 0; i < std::min(image_data.size(), HASH_SAMPLE_SIZE); i += 10) {
            hash = hash * HASH_MULTIPLIER + image_data[i];
        }
        
        // Use hash to seed embedding generation
        for (size_t i = 0; i < EMBEDDING_DIMENSION; ++i) {
            result.embedding[i] = static_cast<float>((hash + i) % EMBEDDING_VALUE_RANGE) / 
                                 static_cast<float>(EMBEDDING_VALUE_RANGE) - 0.5f;
        }
        
        // Normalize to unit length
        float norm = 0.0f;
        for (float v : result.embedding) {
            norm += v * v;
        }
        norm = std::sqrt(norm);
        
        if (norm > 0) {
            for (float& v : result.embedding) {
                v /= norm;
            }
        }
        
        result.model_name = "test-clip";
        result.inference_time_ms = 10;
        
        return result;
    }
    
    CaptionResult generateCaption(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata,
        int max_length
    ) override {
        CaptionResult result;
        result.success = true;
        
        // Generate caption based on image size
        size_t size_kb = image_data.size() / 1024;
        if (size_kb < 50) {
            result.caption = "A small image with minimal detail";
        } else if (size_kb < 200) {
            result.caption = "A medium-sized image with moderate detail";
        } else {
            result.caption = "A large high-resolution image with rich detail";
        }
        
        result.confidence = 0.90f;
        result.model_name = "test-caption";
        result.inference_time_ms = 20;
        
        return result;
    }
    
    std::vector<EmbeddingResult> generateEmbeddingBatch(
        const std::vector<std::vector<uint8_t>>& images
    ) override {
        std::vector<EmbeddingResult> results;
        for (const auto& img : images) {
            results.push_back(generateEmbedding(img, nullptr));
        }
        return results;
    }
    
    bool healthCheck() const override {
        return initialized_;
    }
    
    nlohmann::json getStatistics() const override {
        return {{"is_ready", initialized_}};
    }
    
    void warmup() override {}
    
private:
    bool initialized_ = false;
};

// ============================================================================
// Quality Tests
// ============================================================================

class ImageAnalysisQualityTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<DeterministicImagePlugin>();
        PluginConfig config;
        plugin_->initialize(config, BackendType::CPU);
    }
    
    void TearDown() override {
        if (plugin_) {
            plugin_->shutdown();
        }
    }
    
    std::unique_ptr<DeterministicImagePlugin> plugin_;
};

// Test: Embedding Reproducibility
TEST_F(ImageAnalysisQualityTest, EmbeddingReproducibility) {
    auto image = generate_pattern_image(224, 224, "gradient");
    
    auto result1 = plugin_->generateEmbedding(image, nullptr);
    auto result2 = plugin_->generateEmbedding(image, nullptr);
    
    ASSERT_TRUE(result1.success);
    ASSERT_TRUE(result2.success);
    ASSERT_EQ(result1.embedding.size(), result2.embedding.size());
    
    // Embeddings should be identical for same input
    for (size_t i = 0; i < result1.embedding.size(); ++i) {
        EXPECT_FLOAT_EQ(result1.embedding[i], result2.embedding[i]);
    }
}

// Test: Duplicate Detection (High Similarity)
TEST_F(ImageAnalysisQualityTest, DuplicateDetectionHighSimilarity) {
    auto image = generate_pattern_image(224, 224, "gradient");
    
    auto result1 = plugin_->generateEmbedding(image, nullptr);
    auto result2 = plugin_->generateEmbedding(image, nullptr);
    
    float similarity = cosine_similarity(result1.embedding, result2.embedding);
    
    // Industry standard: >0.90 for duplicate detection
    EXPECT_GT(similarity, 0.90f) << "Duplicate images should have cosine similarity >0.90";
    EXPECT_NEAR(similarity, 1.0f, 0.05f) << "Identical images should have similarity ~1.0";
}

// Test: Similar Image Detection (Moderate Similarity)
TEST_F(ImageAnalysisQualityTest, SimilarImageDetectionModerateSimilarity) {
    auto image1 = generate_pattern_image(224, 224, "gradient");
    auto image2 = generate_pattern_image(224, 224, "checkerboard");
    
    auto result1 = plugin_->generateEmbedding(image1, nullptr);
    auto result2 = plugin_->generateEmbedding(image2, nullptr);
    
    float similarity = cosine_similarity(result1.embedding, result2.embedding);
    
    // Different patterns should have lower similarity
    EXPECT_LT(similarity, 0.90f) << "Different images should have similarity <0.90";
}

// Test: Dissimilar Image Detection (Low Similarity)
TEST_F(ImageAnalysisQualityTest, DissimilarImageDetectionLowSimilarity) {
    auto image1 = generate_pattern_image(224, 224, "gradient");
    auto image2 = generate_pattern_image(224, 224, "solid");
    
    auto result1 = plugin_->generateEmbedding(image1, nullptr);
    auto result2 = plugin_->generateEmbedding(image2, nullptr);
    
    float similarity = cosine_similarity(result1.embedding, result2.embedding);
    
    // Very different images should have even lower similarity
    EXPECT_LT(similarity, 0.80f) << "Very different images should have low similarity";
}

// Test: Embedding Normalization
TEST_F(ImageAnalysisQualityTest, EmbeddingNormalization) {
    auto image = generate_pattern_image(224, 224, "gradient");
    auto result = plugin_->generateEmbedding(image, nullptr);
    
    ASSERT_TRUE(result.success);
    
    // Calculate L2 norm
    float norm = 0.0f;
    for (float v : result.embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    
    // Industry standard: embeddings should be unit-normalized
    EXPECT_NEAR(norm, 1.0f, 0.01f) << "Embeddings should be unit-normalized (L2 norm = 1)";
}

// Test: Embedding Dimension Consistency
TEST_F(ImageAnalysisQualityTest, EmbeddingDimensionConsistency) {
    std::vector<std::vector<uint8_t>> images;
    images.push_back(generate_pattern_image(224, 224, "gradient"));
    images.push_back(generate_pattern_image(384, 384, "checkerboard"));
    images.push_back(generate_pattern_image(512, 512, "solid"));
    
    std::vector<int> dimensions;
    for (const auto& img : images) {
        auto result = plugin_->generateEmbedding(img, nullptr);
        ASSERT_TRUE(result.success);
        dimensions.push_back(result.dimension);
    }
    
    // All embeddings should have same dimension regardless of input size
    EXPECT_TRUE(std::all_of(dimensions.begin(), dimensions.end(), 
                            [&](int d) { return d == dimensions[0]; }))
        << "All embeddings should have consistent dimension";
}

// Test: Batch Embedding Consistency
TEST_F(ImageAnalysisQualityTest, BatchEmbeddingConsistency) {
    auto image = generate_pattern_image(224, 224, "gradient");
    
    // Generate individual embedding
    auto individual_result = plugin_->generateEmbedding(image, nullptr);
    
    // Generate batch embedding with same image
    std::vector<std::vector<uint8_t>> batch = {image};
    auto batch_results = plugin_->generateEmbeddingBatch(batch);
    
    ASSERT_EQ(batch_results.size(), 1);
    ASSERT_TRUE(individual_result.success);
    ASSERT_TRUE(batch_results[0].success);
    
    // Individual and batch embeddings should be identical
    float similarity = cosine_similarity(individual_result.embedding, batch_results[0].embedding);
    EXPECT_NEAR(similarity, 1.0f, 0.001f) 
        << "Individual and batch embeddings should be identical";
}

// Test: Caption Quality Metrics
TEST_F(ImageAnalysisQualityTest, CaptionQualityMetrics) {
    auto image = generate_pattern_image(512, 512, "gradient");
    auto result = plugin_->generateCaption(image, nullptr, 50);
    
    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.caption.empty()) << "Caption should not be empty";
    EXPECT_GT(result.caption.length(), 10) << "Caption should have reasonable length";
    EXPECT_LT(result.caption.length(), 200) << "Caption should not be too long";
    
    // Confidence should be reasonable
    EXPECT_GE(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
    
    // Industry target: confidence >0.70 for good captions
    EXPECT_GT(result.confidence, 0.70f) << "Caption confidence should be >0.70";
}

// Test: Caption Consistency
TEST_F(ImageAnalysisQualityTest, CaptionConsistency) {
    auto image = generate_pattern_image(224, 224, "gradient");
    
    auto result1 = plugin_->generateCaption(image, nullptr, 50);
    auto result2 = plugin_->generateCaption(image, nullptr, 50);
    
    ASSERT_TRUE(result1.success);
    ASSERT_TRUE(result2.success);
    
    // Captions for same image should be identical (deterministic)
    EXPECT_EQ(result1.caption, result2.caption);
    EXPECT_FLOAT_EQ(result1.confidence, result2.confidence);
}

// Test: Embedding Distance Metrics
TEST_F(ImageAnalysisQualityTest, EmbeddingDistanceMetrics) {
    auto image1 = generate_pattern_image(224, 224, "gradient");
    auto image2 = generate_pattern_image(224, 224, "checkerboard");
    
    auto result1 = plugin_->generateEmbedding(image1, nullptr);
    auto result2 = plugin_->generateEmbedding(image2, nullptr);
    
    float cosine_sim = cosine_similarity(result1.embedding, result2.embedding);
    float l2_dist = l2_distance(result1.embedding, result2.embedding);
    
    // Relationship: high cosine similarity = low L2 distance
    // For normalized vectors: L2 = sqrt(2 - 2*cosine_similarity)
    float expected_l2 = std::sqrt(2.0f - 2.0f * cosine_sim);
    EXPECT_NEAR(l2_dist, expected_l2, 0.1f) 
        << "L2 distance should match expected value from cosine similarity";
}

// Test: Performance vs Quality Tradeoff
TEST_F(ImageAnalysisQualityTest, PerformanceQualityTradeoff) {
    std::vector<std::pair<int, std::string>> test_cases = {
        {224, "small"},
        {384, "medium"},
        {512, "large"}
    };
    
    for (const auto& [size, label] : test_cases) {
        auto image = generate_pattern_image(size, size, "gradient");
        
        auto start = std::chrono::high_resolution_clock::now();
        auto result = plugin_->generateEmbedding(image, nullptr);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        ASSERT_TRUE(result.success) << "Failed for " << label << " image";
        
        // Verify embedding quality is maintained regardless of input size
        float norm = 0.0f;
        for (float v : result.embedding) {
            norm += v * v;
        }
        norm = std::sqrt(norm);
        
        EXPECT_NEAR(norm, 1.0f, 0.01f) 
            << "Embedding quality (normalization) should be maintained for " << label << " images";
    }
}

// Test: Batch Processing Quality
TEST_F(ImageAnalysisQualityTest, BatchProcessingQuality) {
    std::vector<std::vector<uint8_t>> batch;
    for (int i = 0; i < 10; ++i) {
        batch.push_back(generate_pattern_image(224, 224, "gradient"));
    }
    
    auto results = plugin_->generateEmbeddingBatch(batch);
    
    ASSERT_EQ(results.size(), batch.size());
    
    // Verify all embeddings meet quality standards
    for (size_t i = 0; i < results.size(); ++i) {
        ASSERT_TRUE(results[i].success) << "Batch item " << i << " failed";
        EXPECT_EQ(results[i].dimension, 512) << "Batch item " << i << " has wrong dimension";
        
        // Check normalization
        float norm = 0.0f;
        for (float v : results[i].embedding) {
            norm += v * v;
        }
        norm = std::sqrt(norm);
        
        EXPECT_NEAR(norm, 1.0f, 0.01f) 
            << "Batch item " << i << " embedding not properly normalized";
    }
    
    // Verify all embeddings are identical (same input pattern)
    for (size_t i = 1; i < results.size(); ++i) {
        float sim = cosine_similarity(results[0].embedding, results[i].embedding);
        EXPECT_NEAR(sim, 1.0f, 0.001f) 
            << "Batch embeddings for identical images should be identical";
    }
}

// Test: Statistical Properties of Embeddings
TEST_F(ImageAnalysisQualityTest, EmbeddingStatisticalProperties) {
    auto image = generate_pattern_image(224, 224, "gradient");
    auto result = plugin_->generateEmbedding(image, nullptr);
    
    ASSERT_TRUE(result.success);
    
    // Calculate mean and variance
    float mean = std::accumulate(result.embedding.begin(), result.embedding.end(), 0.0f) 
                 / result.embedding.size();
    
    float variance = 0.0f;
    for (float v : result.embedding) {
        float diff = v - mean;
        variance += diff * diff;
    }
    variance /= result.embedding.size();
    float std_dev = std::sqrt(variance);
    
    // For unit-normalized embeddings, mean should be close to 0
    EXPECT_NEAR(mean, 0.0f, 0.2f) << "Mean of normalized embeddings should be near 0";
    
    // Standard deviation should be reasonable
    EXPECT_GT(std_dev, 0.1f) << "Embeddings should have reasonable variance";
    EXPECT_LT(std_dev, 1.0f) << "Standard deviation should not be too large";
}

// No custom main; we use gtest_main provided by build

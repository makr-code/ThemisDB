/**
 * @file test_binary_quantizer.cpp
 * @brief Unit tests for Binary Quantization implementation
 * 
 * Tests binary quantization for maximum vector compression (Issue #914)
 */

#include <gtest/gtest.h>
#include "index/binary_quantizer.h"
#include <random>
#include <cmath>

using namespace themis;

class BinaryQuantizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate random training vectors
        std::mt19937 gen(42);  // Fixed seed for reproducibility
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        training_vectors_.clear();
        for (int i = 0; i < num_training_vectors_; ++i) {
            std::vector<float> vec(dimension_);
            for (int j = 0; j < dimension_; ++j) {
                vec[j] = dis(gen);
            }
            training_vectors_.push_back(std::move(vec));
        }
    }

    int dimension_ = 128;
    int num_training_vectors_ = 1000;
    std::vector<std::vector<float>> training_vectors_;
};

TEST_F(BinaryQuantizerTest, ConstructorValidDimension) {
    BinaryQuantizer::Config config;
    EXPECT_NO_THROW({
        BinaryQuantizer bq(128, config);
    });
}

TEST_F(BinaryQuantizerTest, ConstructorInvalidDimension) {
    BinaryQuantizer::Config config;
    EXPECT_THROW({
        BinaryQuantizer bq(0, config);
    }, std::invalid_argument);
    
    EXPECT_THROW({
        BinaryQuantizer bq(-1, config);
    }, std::invalid_argument);
}

TEST_F(BinaryQuantizerTest, TrainWithValidData) {
    BinaryQuantizer::Config config;
    config.center_values = true;
    BinaryQuantizer bq(dimension_, config);
    
    auto status = bq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(bq.isTrained());
}

TEST_F(BinaryQuantizerTest, TrainWithEmptyData) {
    BinaryQuantizer::Config config;
    BinaryQuantizer bq(dimension_, config);
    
    std::vector<std::vector<float>> empty_vectors;
    auto status = bq.train(empty_vectors);
    EXPECT_FALSE(status.ok);
    EXPECT_FALSE(bq.isTrained());
}

TEST_F(BinaryQuantizerTest, TrainWithMismatchedDimension) {
    BinaryQuantizer::Config config;
    BinaryQuantizer bq(dimension_, config);
    
    std::vector<std::vector<float>> wrong_dim_vectors;
    wrong_dim_vectors.push_back(std::vector<float>(64));
    
    auto status = bq.train(wrong_dim_vectors);
    EXPECT_FALSE(status.ok);
}

TEST_F(BinaryQuantizerTest, EncodeDecodeRoundTrip) {
    BinaryQuantizer::Config config;
    config.center_values = true;
    BinaryQuantizer bq(dimension_, config);
    
    // Train quantizer
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Test encode/decode
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = bq.encode(test_vector);
    EXPECT_EQ(codes.size(), bq.getEncodedSize());
    
    auto decoded = bq.decode(codes);
    EXPECT_EQ(decoded.size(), dimension_);
}

TEST_F(BinaryQuantizerTest, BackendSelection) {
    // Test backend reporting
    BinaryQuantizer::Config config;
    config.prefer_faiss = true;
    BinaryQuantizer bq(dimension_, config);
    
    const char* backend = bq.getBackend();
    EXPECT_TRUE(strcmp(backend, "faiss") == 0 || strcmp(backend, "custom") == 0);
    
#ifdef THEMIS_HAS_FAISS
    if (config.prefer_faiss) {
        // Should use FAISS if available and preferred
        EXPECT_STREQ(backend, "faiss");
    }
#else
    // Should always use custom if FAISS not available
    EXPECT_STREQ(backend, "custom");
#endif
}

TEST_F(BinaryQuantizerTest, ForceCustomBackend) {
    // Test forcing custom backend even when FAISS available
    BinaryQuantizer::Config config;
    config.prefer_faiss = false;
    BinaryQuantizer bq(dimension_, config);
    
    EXPECT_STREQ(bq.getBackend(), "custom");
    
    // Train and verify it works
    auto status = bq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
}

TEST_F(BinaryQuantizerTest, CompressionRatio) {
    BinaryQuantizer::Config config;
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    float compression_ratio = bq.getCompressionRatio();
    
    // For 128D float32: 512 bytes → 16 bytes = 32x compression
    EXPECT_NEAR(compression_ratio, 32.0f, 1.0f);
}

TEST_F(BinaryQuantizerTest, HammingDistance) {
    BinaryQuantizer::Config config;
    config.center_values = false;  // Simpler for testing
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Encode two vectors
    auto codes_a = bq.encode(training_vectors_[0]);
    auto codes_b = bq.encode(training_vectors_[1]);
    
    // Compute Hamming distance
    float hamming_dist = bq.hammingDistance(codes_a, codes_b);
    
    // Should be a valid distance (0 to dimension)
    EXPECT_GE(hamming_dist, 0.0f);
    EXPECT_LE(hamming_dist, static_cast<float>(dimension_));
}

TEST_F(BinaryQuantizerTest, HammingDistanceIdentical) {
    BinaryQuantizer::Config config;
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    auto codes = bq.encode(training_vectors_[0]);
    float hamming_dist = bq.hammingDistance(codes, codes);
    
    // Distance to itself should be 0
    EXPECT_EQ(hamming_dist, 0.0f);
}

TEST_F(BinaryQuantizerTest, AsymmetricDistance) {
    BinaryQuantizer::Config config;
    config.center_values = true;
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Encode database vector
    auto codes = bq.encode(training_vectors_[0]);
    
    // Compute asymmetric distance
    float distance = bq.asymmetricDistance(training_vectors_[1], codes);
    
    // Should be a valid distance
    EXPECT_GE(distance, 0.0f);
    EXPECT_LT(distance, std::numeric_limits<float>::infinity());
}

TEST_F(BinaryQuantizerTest, BitPacking) {
    // Test that 128 bits fit in 16 bytes
    BinaryQuantizer::Config config;
    BinaryQuantizer bq(128, config);
    
    EXPECT_EQ(bq.getEncodedSize(), 16);
    
    // Test non-multiple of 8
    BinaryQuantizer bq_130(130, config);
    EXPECT_EQ(bq_130.getEncodedSize(), 17);  // Rounds up to nearest byte
}

TEST_F(BinaryQuantizerTest, CenteredVsNonCentered) {
    // Test with centering
    BinaryQuantizer::Config config_centered;
    config_centered.center_values = true;
    BinaryQuantizer bq_centered(dimension_, config_centered);
    bq_centered.train(training_vectors_);
    
    // Test without centering
    BinaryQuantizer::Config config_non_centered;
    config_non_centered.center_values = false;
    BinaryQuantizer bq_non_centered(dimension_, config_non_centered);
    bq_non_centered.train(training_vectors_);
    
    // Both should work but produce different codes
    auto codes_centered = bq_centered.encode(training_vectors_[0]);
    auto codes_non_centered = bq_non_centered.encode(training_vectors_[0]);
    
    EXPECT_EQ(codes_centered.size(), codes_non_centered.size());
    // Codes should likely be different
    // (can't guarantee without analyzing the data, so just check they're valid)
    EXPECT_GT(codes_centered.size(), 0);
    EXPECT_GT(codes_non_centered.size(), 0);
}

TEST_F(BinaryQuantizerTest, NormalizedInput) {
    BinaryQuantizer::Config config;
    config.normalize_input = true;
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Create unnormalized vector
    std::vector<float> vec(dimension_, 10.0f);  // Large values
    
    auto codes = bq.encode(vec);
    EXPECT_GT(codes.size(), 0);
}

TEST_F(BinaryQuantizerTest, MemoryUsage) {
    BinaryQuantizer::Config config;
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    size_t memory = bq.getMemoryUsage();
    
    // Should be relatively small (just metadata + means)
    EXPECT_GT(memory, 0);
    EXPECT_LT(memory, 10000);  // Less than 10KB
}

TEST_F(BinaryQuantizerTest, ScaleLearning) {
    BinaryQuantizer::Config config;
    config.scale_factor = 0.0f;  // Auto-learn
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    float scale = bq.getScale();
    EXPECT_GT(scale, 0.0f);
    EXPECT_LT(scale, 10.0f);  // Reasonable scale for normalized data
}

TEST_F(BinaryQuantizerTest, FixedScale) {
    BinaryQuantizer::Config config;
    config.scale_factor = 2.5f;  // Fixed scale
    BinaryQuantizer bq(dimension_, config);
    
    auto train_status = bq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    float scale = bq.getScale();
    EXPECT_FLOAT_EQ(scale, 2.5f);
}

// Main removed - using GTest's main from themis_tests.exe

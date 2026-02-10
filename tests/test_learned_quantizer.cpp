/**
 * @file test_learned_quantizer.cpp
 * @brief Unit tests for Learned Quantization implementation
 * 
 * Tests learned quantization with adaptive threshold learning (Issue #914)
 */

#include <gtest/gtest.h>
#include "index/learned_quantizer.h"
#include <random>
#include <cmath>

using namespace themis;

class LearnedQuantizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate random training vectors
        std::mt19937 gen(42);
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

TEST_F(LearnedQuantizerTest, ConstructorValidDimension) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    
    EXPECT_NO_THROW({
        LearnedQuantizer lq(128, config);
    });
}

TEST_F(LearnedQuantizerTest, ConstructorInvalidDimension) {
    LearnedQuantizer::Config config;
    
    EXPECT_THROW({
        LearnedQuantizer lq(0, config);
    }, std::invalid_argument);
    
    EXPECT_THROW({
        LearnedQuantizer lq(-1, config);
    }, std::invalid_argument);
}

TEST_F(LearnedQuantizerTest, ConstructorInvalidBits) {
    LearnedQuantizer::Config config;
    
    // Too few bits
    config.bits_per_dimension = 0;
    EXPECT_THROW({
        LearnedQuantizer lq(128, config);
    }, std::invalid_argument);
    
    // Too many bits
    config.bits_per_dimension = 9;
    EXPECT_THROW({
        LearnedQuantizer lq(128, config);
    }, std::invalid_argument);
}

TEST_F(LearnedQuantizerTest, TrainWithValidDataPerDimension) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto status = lq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(lq.isTrained());
}

TEST_F(LearnedQuantizerTest, TrainWithValidDataPerBlock) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = false;
    config.block_size = 64;
    LearnedQuantizer lq(dimension_, config);
    
    auto status = lq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(lq.isTrained());
}

TEST_F(LearnedQuantizerTest, TrainWithEmptyData) {
    LearnedQuantizer::Config config;
    LearnedQuantizer lq(dimension_, config);
    
    std::vector<std::vector<float>> empty_vectors;
    auto status = lq.train(empty_vectors);
    EXPECT_FALSE(status.ok);
    EXPECT_FALSE(lq.isTrained());
}

TEST_F(LearnedQuantizerTest, TrainWithMismatchedDimension) {
    LearnedQuantizer::Config config;
    LearnedQuantizer lq(dimension_, config);
    
    std::vector<std::vector<float>> wrong_dim_vectors;
    wrong_dim_vectors.push_back(std::vector<float>(64));
    
    auto status = lq.train(wrong_dim_vectors);
    EXPECT_FALSE(status.ok);
}

TEST_F(LearnedQuantizerTest, EncodeDecodeRoundTrip4Bit) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = lq.encode(test_vector);
    EXPECT_EQ(codes.size(), lq.getEncodedSize());
    
    auto decoded = lq.decode(codes);
    EXPECT_EQ(decoded.size(), test_vector.size());
    
    // Decoded should approximate original
    float mse = 0.0f;
    for (size_t i = 0; i < test_vector.size(); i++) {
        float diff = test_vector[i] - decoded[i];
        mse += diff * diff;
    }
    mse /= test_vector.size();
    
    // MSE should be reasonably small
    EXPECT_LT(mse, 2.0f);
}

TEST_F(LearnedQuantizerTest, EncodeDecodeRoundTrip8Bit) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 8;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = lq.encode(test_vector);
    auto decoded = lq.decode(codes);
    
    // 8-bit should be more accurate than 4-bit
    float mse = 0.0f;
    for (size_t i = 0; i < test_vector.size(); i++) {
        float diff = test_vector[i] - decoded[i];
        mse += diff * diff;
    }
    mse /= test_vector.size();
    
    EXPECT_LT(mse, 0.5f);  // Better accuracy than 4-bit
}

TEST_F(LearnedQuantizerTest, CompressionRatio4Bit) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    float compression_ratio = lq.getCompressionRatio();
    
    // For 128D float32 → 4-bit: 512 bytes → 128 bytes = 4x compression
    // But we use 8-bit codes, so it's still 128 bytes
    EXPECT_NEAR(compression_ratio, 4.0f, 0.5f);
}

TEST_F(LearnedQuantizerTest, CompressionRatio8Bit) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 8;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    float compression_ratio = lq.getCompressionRatio();
    
    // For 128D float32 → 8-bit: 512 bytes → 128 bytes = 4x compression
    EXPECT_NEAR(compression_ratio, 4.0f, 0.5f);
}

TEST_F(LearnedQuantizerTest, AsymmetricDistance) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    auto codes = lq.encode(training_vectors_[0]);
    float distance = lq.asymmetricDistance(training_vectors_[1], codes);
    
    EXPECT_GE(distance, 0.0f);
    EXPECT_LT(distance, std::numeric_limits<float>::infinity());
}

TEST_F(LearnedQuantizerTest, PercentilesVsUniformInit) {
    // Test with percentiles
    LearnedQuantizer::Config config_perc;
    config_perc.use_percentiles = true;
    config_perc.bits_per_dimension = 4;
    LearnedQuantizer lq_perc(dimension_, config_perc);
    lq_perc.train(training_vectors_);
    
    // Test with uniform
    LearnedQuantizer::Config config_uniform;
    config_uniform.use_percentiles = false;
    config_uniform.bits_per_dimension = 4;
    LearnedQuantizer lq_uniform(dimension_, config_uniform);
    lq_uniform.train(training_vectors_);
    
    // Both should train successfully
    EXPECT_TRUE(lq_perc.isTrained());
    EXPECT_TRUE(lq_uniform.isTrained());
    
    // Test encoding
    auto codes_perc = lq_perc.encode(training_vectors_[0]);
    auto codes_uniform = lq_uniform.encode(training_vectors_[0]);
    
    EXPECT_GT(codes_perc.size(), 0);
    EXPECT_GT(codes_uniform.size(), 0);
}

TEST_F(LearnedQuantizerTest, DifferentBitWidths) {
    for (int bits = 2; bits <= 8; bits++) {
        LearnedQuantizer::Config config;
        config.bits_per_dimension = bits;
        config.per_dimension = true;
        LearnedQuantizer lq(dimension_, config);
        
        auto status = lq.train(training_vectors_);
        EXPECT_TRUE(status.ok) << "Failed for " << bits << " bits";
        
        EXPECT_EQ(lq.getBitsPerDimension(), bits);
        EXPECT_EQ(lq.getNumBins(), 1 << bits);
        
        auto codes = lq.encode(training_vectors_[0]);
        auto decoded = lq.decode(codes);
        EXPECT_EQ(decoded.size(), training_vectors_[0].size());
    }
}

TEST_F(LearnedQuantizerTest, PerBlockMode) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = false;
    config.block_size = 32;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    auto codes = lq.encode(training_vectors_[0]);
    auto decoded = lq.decode(codes);
    
    EXPECT_EQ(decoded.size(), training_vectors_[0].size());
    
    // Check reconstruction quality
    float mse = 0.0f;
    for (size_t i = 0; i < decoded.size(); i++) {
        float diff = training_vectors_[0][i] - decoded[i];
        mse += diff * diff;
    }
    mse /= decoded.size();
    
    EXPECT_LT(mse, 2.0f);
}

TEST_F(LearnedQuantizerTest, MemoryUsage) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.per_dimension = true;
    LearnedQuantizer lq(dimension_, config);
    
    auto train_status = lq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    size_t memory = lq.getMemoryUsage();
    EXPECT_GT(memory, 0);
    
    // Per-dimension mode stores thresholds and centroids per dimension
    // Should be more than per-block but still reasonable
    EXPECT_LT(memory, 100000);  // Less than 100KB
}

TEST_F(LearnedQuantizerTest, ConvergenceIterations) {
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    config.training_iterations = 10;  // Few iterations
    config.convergence_threshold = 0.1f;  // Relaxed threshold
    LearnedQuantizer lq(dimension_, config);
    
    auto status = lq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
}

TEST_F(LearnedQuantizerTest, LargeDataset) {
    // Test with more training data
    std::mt19937 gen(42);
    std::normal_distribution<float> dis(0.0f, 1.0f);
    
    std::vector<std::vector<float>> large_training;
    for (int i = 0; i < 10000; ++i) {
        std::vector<float> vec(dimension_);
        for (int j = 0; j < dimension_; ++j) {
            vec[j] = dis(gen);
        }
        large_training.push_back(std::move(vec));
    }
    
    LearnedQuantizer::Config config;
    config.bits_per_dimension = 4;
    LearnedQuantizer lq(dimension_, config);
    
    auto status = lq.train(large_training);
    EXPECT_TRUE(status.ok);
}

// Main removed - using GTest's main from themis_tests.exe

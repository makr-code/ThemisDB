/**
 * @file test_product_quantizer.cpp
 * @brief Unit tests for Product Quantization implementation
 * 
 * Tests vector compression using Product Quantization (Feature #7)
 */

#include <gtest/gtest.h>
#include "index/product_quantizer.h"
#include <cmath>
#include <limits>
#include <random>

using namespace themis;

class ProductQuantizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate random training vectors
        std::random_device rd = {};
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

TEST_F(ProductQuantizerTest, ConstructorValidDimension) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    
    EXPECT_NO_THROW({
        ProductQuantizer pq(128, config);
    });
}

TEST_F(ProductQuantizerTest, ConstructorInvalidDimension) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    
    // 127 is not divisible by 8
    EXPECT_THROW({
        ProductQuantizer pq(127, config);
    }, std::invalid_argument);
}

TEST_F(ProductQuantizerTest, ConstructorRejectsZeroSubquantizers) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 0;

    EXPECT_THROW({
        ProductQuantizer pq(128, config);
    }, std::invalid_argument);
}

TEST_F(ProductQuantizerTest, TrainWithValidData) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    ProductQuantizer pq(dimension_, config);
    
    auto status = pq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(pq.isTrained());
}

TEST_F(ProductQuantizerTest, TrainWithEmptyData) {
    ProductQuantizer::Config config;
    ProductQuantizer pq(dimension_, config);
    
    std::vector<std::vector<float>> empty_vectors;
    auto status = pq.train(empty_vectors);
    EXPECT_FALSE(status.ok);
    EXPECT_FALSE(pq.isTrained());
}

TEST_F(ProductQuantizerTest, TrainWithMismatchedDimension) {
    ProductQuantizer::Config config;
    ProductQuantizer pq(dimension_, config);
    
    std::vector<std::vector<float>> wrong_dim_vectors;
    wrong_dim_vectors.push_back(std::vector<float>(64));  // Wrong dimension
    
    auto status = pq.train(wrong_dim_vectors);
    EXPECT_FALSE(status.ok);
}

TEST_F(ProductQuantizerTest, TrainWithMixedDimensionsFails) {
    ProductQuantizer::Config config;
    ProductQuantizer pq(dimension_, config);

    auto mixed_vectors = training_vectors_;
    mixed_vectors.back().push_back(1.0f);

    auto status = pq.train(mixed_vectors);
    EXPECT_FALSE(status.ok);
}

TEST_F(ProductQuantizerTest, EncodeDecodeRoundTrip) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    ProductQuantizer pq(dimension_, config);
    
    // Train quantizer
    auto train_status = pq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Test encode/decode
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = pq.encode(test_vector);
    
    EXPECT_EQ(codes.size(), static_cast<size_t>(config.num_subquantizers));
    
    auto reconstructed = pq.decode(codes);
    EXPECT_EQ(reconstructed.size(), test_vector.size());
}

TEST_F(ProductQuantizerTest, DecodeRejectsOutOfRangeCode) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    config.num_centroids = 16;
    ProductQuantizer pq(dimension_, config);

    auto train_status = pq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);

    std::vector<uint8_t> codes(static_cast<size_t>(config.num_subquantizers), 0);
    codes[0] = static_cast<uint8_t>(config.num_centroids);

    auto reconstructed = pq.decode(codes);
    EXPECT_TRUE(reconstructed.empty());
}

TEST_F(ProductQuantizerTest, CompressionRatio) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    ProductQuantizer pq(dimension_, config);
    
    // For 128D vectors: 128 * 4 bytes = 512 bytes
    // Compressed: 8 * 1 byte = 8 bytes
    // Ratio: 512 / 8 = 64
    float expected_ratio = (dimension_ * sizeof(float)) / 
                          (static_cast<float>(config.num_subquantizers) * sizeof(uint8_t));
    
    EXPECT_FLOAT_EQ(pq.getCompressionRatio(), expected_ratio);
}

TEST_F(ProductQuantizerTest, AsymmetricDistance) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    ProductQuantizer pq(dimension_, config);
    
    // Train quantizer
    auto train_status = pq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Test asymmetric distance
    std::vector<float> query = training_vectors_[0];
    auto codes = pq.encode(query);
    
    float distance = pq.computeAsymmetricDistance(query, codes);
    
    // Distance to itself should be small (due to quantization error)
    EXPECT_LT(distance, 5.0f);  // Reasonable threshold
}

TEST_F(ProductQuantizerTest, AsymmetricDistanceRejectsOutOfRangeCode) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    config.num_centroids = 16;
    ProductQuantizer pq(dimension_, config);

    auto train_status = pq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);

    std::vector<float> query = training_vectors_[0];
    std::vector<uint8_t> codes(static_cast<size_t>(config.num_subquantizers), 0);
    codes[1] = static_cast<uint8_t>(config.num_centroids);

    float distance = pq.computeAsymmetricDistance(query, codes);
    EXPECT_EQ(distance, std::numeric_limits<float>::max());
}

TEST_F(ProductQuantizerTest, QuantizationError) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    ProductQuantizer pq(dimension_, config);
    
    // Train quantizer
    auto train_status = pq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Test quantization error
    float total_error = 0.0f;
    int num_tests = 100;
    
    for (int i = 0; i < num_tests; ++i) {
        const auto& original = training_vectors_[i];
        auto codes = pq.encode(original);
        auto reconstructed = pq.decode(codes);
        
        // Compute L2 error
        float error = 0.0f;
        for (size_t j = 0; j < original.size(); ++j) {
            float diff = original[j] - reconstructed[j];
            error += diff * diff;
        }
        error = std::sqrt(error);
        total_error += error;
    }
    
    float avg_error = total_error / num_tests;
    
    // Average reconstruction error should be reasonable
    // For 8-bit quantization, we expect some error but not too much
    EXPECT_LT(avg_error, 10.0f);  // Reasonable threshold for normalized vectors
}

TEST_F(ProductQuantizerTest, MemoryUsage) {
    ProductQuantizer::Config config;
    config.num_subquantizers = 8;
    config.num_centroids = 256;
    ProductQuantizer pq(dimension_, config);
    
    // Expected memory: num_subquantizers * num_centroids * subvector_dim * sizeof(float)
    int subvector_dim = dimension_ / config.num_subquantizers;
    size_t expected_memory = config.num_subquantizers * config.num_centroids * 
                            subvector_dim * sizeof(float);
    
    EXPECT_EQ(pq.getMemoryUsage(), expected_memory);
}

TEST_F(ProductQuantizerTest, Different1024DimensionWith16Subquantizers) {
    // Test larger dimension (typical for embeddings like OpenAI ada-002 is 1536D)
    int large_dim = 1024;
    int num_subq = 16;
    
    ProductQuantizer::Config config;
    config.num_subquantizers = num_subq;
    ProductQuantizer pq(large_dim, config);
    
    // Generate training data
    std::random_device rd = {};
    std::mt19937 gen(42);
    std::normal_distribution<float> dis(0.0f, 1.0f);
    
    std::vector<std::vector<float>> large_training;
    for (int i = 0; i < 500; ++i) {
        std::vector<float> vec(large_dim);
        for (int j = 0; j < large_dim; ++j) {
            vec[j] = dis(gen);
        }
        large_training.push_back(std::move(vec));
    }
    
    auto status = pq.train(large_training);
    EXPECT_TRUE(status.ok);
    
    // Test compression ratio (1024 * 4 / 16 = 256)
    float expected_ratio = (large_dim * sizeof(float)) / 
                          (static_cast<float>(num_subq) * sizeof(uint8_t));
    EXPECT_FLOAT_EQ(pq.getCompressionRatio(), expected_ratio);
}



TEST_F(ProductQuantizerTest, BackendSelection) {
    ProductQuantizer::Config config;
    config.prefer_faiss = true;
    ProductQuantizer pq(dimension_, config);
    
    const char* backend = pq.getBackend();
    EXPECT_TRUE(strcmp(backend, "faiss") == 0 || strcmp(backend, "custom") == 0);
}

TEST_F(ProductQuantizerTest, ForceCustomBackend) {
    ProductQuantizer::Config config;
    config.prefer_faiss = false;
    ProductQuantizer pq(dimension_, config);
    
    EXPECT_STREQ(pq.getBackend(), "custom");
    auto status = pq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
}

// Main function
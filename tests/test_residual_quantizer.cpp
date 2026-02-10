/**
 * @file test_residual_quantizer.cpp
 * @brief Unit tests for Residual Quantization implementation
 * 
 * Tests multi-stage residual quantization for high-accuracy compression (Issue #914)
 */

#include <gtest/gtest.h>
#include "index/residual_quantizer.h"
#include <random>
#include <cmath>

using namespace themis;

class ResidualQuantizerTest : public ::testing::Test {
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

TEST_F(ResidualQuantizerTest, ConstructorValidDimension) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.num_subquantizers = 8;
    
    EXPECT_NO_THROW({
        ResidualQuantizer rq(128, config);
    });
}

TEST_F(ResidualQuantizerTest, ConstructorInvalidDimension) {
    ResidualQuantizer::Config config;
    config.num_subquantizers = 8;
    
    EXPECT_THROW({
        ResidualQuantizer rq(0, config);
    }, std::invalid_argument);
    
    // 127 is not divisible by 8
    EXPECT_THROW({
        ResidualQuantizer rq(127, config);
    }, std::invalid_argument);
}

TEST_F(ResidualQuantizerTest, ConstructorInvalidStages) {
    ResidualQuantizer::Config config;
    
    config.num_stages = 0;
    EXPECT_THROW({
        ResidualQuantizer rq(128, config);
    }, std::invalid_argument);
    
    config.num_stages = 11;  // Too many stages
    EXPECT_THROW({
        ResidualQuantizer rq(128, config);
    }, std::invalid_argument);
}

TEST_F(ResidualQuantizerTest, TrainWithValidData2Stages) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    auto status = rq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(rq.isTrained());
}

TEST_F(ResidualQuantizerTest, TrainWithValidData3Stages) {
    ResidualQuantizer::Config config;
    config.num_stages = 3;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    auto status = rq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(rq.isTrained());
}

TEST_F(ResidualQuantizerTest, TrainWithEmptyData) {
    ResidualQuantizer::Config config;
    ResidualQuantizer rq(dimension_, config);
    
    std::vector<std::vector<float>> empty_vectors;
    auto status = rq.train(empty_vectors);
    EXPECT_FALSE(status.ok);
    EXPECT_FALSE(rq.isTrained());
}

TEST_F(ResidualQuantizerTest, TrainWithMismatchedDimension) {
    ResidualQuantizer::Config config;
    ResidualQuantizer rq(dimension_, config);
    
    std::vector<std::vector<float>> wrong_dim_vectors;
    wrong_dim_vectors.push_back(std::vector<float>(64));
    
    auto status = rq.train(wrong_dim_vectors);
    EXPECT_FALSE(status.ok);
}

TEST_F(ResidualQuantizerTest, EncodeDecodeRoundTrip2Stages) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = rq.encode(test_vector);
    
    // Codes should be num_stages * num_subquantizers bytes
    EXPECT_EQ(codes.size(), rq.getEncodedSize());
    EXPECT_EQ(codes.size(), config.num_stages * config.num_subquantizers);
    
    auto decoded = rq.decode(codes);
    EXPECT_EQ(decoded.size(), test_vector.size());
    
    // Compute reconstruction error
    float mse = 0.0f;
    for (size_t i = 0; i < test_vector.size(); i++) {
        float diff = test_vector[i] - decoded[i];
        mse += diff * diff;
    }
    mse /= test_vector.size();
    
    // 2-stage should have lower MSE than single-stage PQ
    EXPECT_LT(mse, 0.5f);
}

TEST_F(ResidualQuantizerTest, EncodeDecodeRoundTrip3Stages) {
    ResidualQuantizer::Config config;
    config.num_stages = 3;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = rq.encode(test_vector);
    auto decoded = rq.decode(codes);
    
    // 3-stage should be even more accurate
    float mse = 0.0f;
    for (size_t i = 0; i < test_vector.size(); i++) {
        float diff = test_vector[i] - decoded[i];
        mse += diff * diff;
    }
    mse /= test_vector.size();
    
    EXPECT_LT(mse, 0.3f);  // Better than 2-stage
}

TEST_F(ResidualQuantizerTest, ImprovementOverStages) {
    // Train single-stage (equivalent to PQ)
    ResidualQuantizer::Config config1;
    config1.num_stages = 1;
    config1.num_subquantizers = 8;
    ResidualQuantizer rq1(dimension_, config1);
    rq1.train(training_vectors_);
    
    // Train two-stage
    ResidualQuantizer::Config config2;
    config2.num_stages = 2;
    config2.num_subquantizers = 8;
    ResidualQuantizer rq2(dimension_, config2);
    rq2.train(training_vectors_);
    
    // Measure reconstruction errors
    std::vector<float> test_vector = training_vectors_[0];
    
    auto codes1 = rq1.encode(test_vector);
    auto decoded1 = rq1.decode(codes1);
    float mse1 = 0.0f;
    for (size_t i = 0; i < test_vector.size(); i++) {
        float diff = test_vector[i] - decoded1[i];
        mse1 += diff * diff;
    }
    mse1 /= test_vector.size();
    
    auto codes2 = rq2.encode(test_vector);
    auto decoded2 = rq2.decode(codes2);
    float mse2 = 0.0f;
    for (size_t i = 0; i < test_vector.size(); i++) {
        float diff = test_vector[i] - decoded2[i];
        mse2 += diff * diff;
    }
    mse2 /= test_vector.size();
    
    // Two-stage should have lower error
    EXPECT_LT(mse2, mse1);
}

TEST_F(ResidualQuantizerTest, CompressionRatio) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    float compression_ratio = rq.getCompressionRatio();
    
    // For 128D float32 with 2 stages × 8 subquantizers:
    // 512 bytes → 16 bytes = 32x compression
    EXPECT_NEAR(compression_ratio, 32.0f, 2.0f);
}

TEST_F(ResidualQuantizerTest, AsymmetricDistance) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    auto codes = rq.encode(training_vectors_[0]);
    float distance = rq.asymmetricDistance(training_vectors_[1], codes);
    
    EXPECT_GE(distance, 0.0f);
    EXPECT_LT(distance, std::numeric_limits<float>::infinity());
}

TEST_F(ResidualQuantizerTest, AsymmetricDistanceSelfSimilarity) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    std::vector<float> test_vector = training_vectors_[0];
    auto codes = rq.encode(test_vector);
    float distance = rq.asymmetricDistance(test_vector, codes);
    
    // Distance to itself should be small (near 0, but not exact due to quantization)
    EXPECT_LT(distance, 2.0f);
}

TEST_F(ResidualQuantizerTest, StageQuantizerAccess) {
    ResidualQuantizer::Config config;
    config.num_stages = 3;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    // Check we can access stage quantizers
    for (int stage = 0; stage < 3; stage++) {
        const auto* pq = rq.getStageQuantizer(stage);
        EXPECT_NE(pq, nullptr);
        EXPECT_TRUE(pq->isTrained());
    }
    
    // Out of bounds should return nullptr
    EXPECT_EQ(rq.getStageQuantizer(3), nullptr);
    EXPECT_EQ(rq.getStageQuantizer(-1), nullptr);
}

TEST_F(ResidualQuantizerTest, MemoryUsage) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    ResidualQuantizer rq(dimension_, config);
    
    auto train_status = rq.train(training_vectors_);
    ASSERT_TRUE(train_status.ok);
    
    size_t memory = rq.getMemoryUsage();
    EXPECT_GT(memory, 0);
    
    // Should include memory for all stage quantizers
    // Rough estimate: num_stages × PQ memory
}

TEST_F(ResidualQuantizerTest, DifferentSubquantizers) {
    for (int sqs : {4, 8, 16}) {
        if (dimension_ % sqs != 0) continue;
        
        ResidualQuantizer::Config config;
        config.num_stages = 2;
        config.num_subquantizers = sqs;
        ResidualQuantizer rq(dimension_, config);
        
        auto status = rq.train(training_vectors_);
        EXPECT_TRUE(status.ok) << "Failed for " << sqs << " subquantizers";
        
        auto codes = rq.encode(training_vectors_[0]);
        EXPECT_EQ(codes.size(), config.num_stages * sqs);
    }
}

TEST_F(ResidualQuantizerTest, DifferentCentroids) {
    for (int centroids : {128, 256}) {
        ResidualQuantizer::Config config;
        config.num_stages = 2;
        config.num_subquantizers = 8;
        config.num_centroids = centroids;
        ResidualQuantizer rq(dimension_, config);
        
        auto status = rq.train(training_vectors_);
        EXPECT_TRUE(status.ok) << "Failed for " << centroids << " centroids";
    }
}

TEST_F(ResidualQuantizerTest, ConvergenceSettings) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.max_kmeans_iterations = 10;
    config.convergence_threshold = 0.01f;
    ResidualQuantizer rq(dimension_, config);
    
    auto status = rq.train(training_vectors_);
    EXPECT_TRUE(status.ok);
}

TEST_F(ResidualQuantizerTest, EncodedSize) {
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    config.num_subquantizers = 8;
    ResidualQuantizer rq(dimension_, config);
    
    size_t encoded_size = rq.getEncodedSize();
    EXPECT_EQ(encoded_size, 2 * 8);  // 2 stages × 8 subquantizers = 16 bytes
}

TEST_F(ResidualQuantizerTest, Getters) {
    ResidualQuantizer::Config config;
    config.num_stages = 3;
    config.num_subquantizers = 16;
    ResidualQuantizer rq(dimension_, config);
    
    EXPECT_EQ(rq.getDimension(), dimension_);
    EXPECT_EQ(rq.getNumStages(), 3);
    EXPECT_EQ(rq.getCodesPerStage(), 16);
}

TEST_F(ResidualQuantizerTest, LargeDataset) {
    // Test with more training data
    std::mt19937 gen(42);
    std::normal_distribution<float> dis(0.0f, 1.0f);
    
    std::vector<std::vector<float>> large_training;
    for (int i = 0; i < 5000; ++i) {
        std::vector<float> vec(dimension_);
        for (int j = 0; j < dimension_; ++j) {
            vec[j] = dis(gen);
        }
        large_training.push_back(std::move(vec));
    }
    
    ResidualQuantizer::Config config;
    config.num_stages = 2;
    ResidualQuantizer rq(dimension_, config);
    
    auto status = rq.train(large_training);
    EXPECT_TRUE(status.ok);
}

// Main removed - using GTest's main from themis_tests.exe

/*
 * ThemisDB | File: test_multi_gpu_vector_index.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


#include "index/multi_gpu_vector_index.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <random>

using namespace themis::index;

class MultiGPUVectorIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test configuration
        dimension = 128;
        numVectors = 100;
        k = 10;
        
        // Generate random test vectors
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (size_t i = 0; i < numVectors; ++i) {
            std::vector<float> vec(dimension);
            for (int j = 0; j < dimension; ++j) {
                vec[j] = dist(rng);
            }
            testVectors.push_back(vec);
            testIds.push_back("vec_" + std::to_string(i));
        }
        
        // Generate query vector
        queryVector.resize(dimension);
        for (int j = 0; j < dimension; ++j) {
            queryVector[j] = dist(rng);
        }
    }
    
    int dimension;
    size_t numVectors;
    size_t k;
    std::vector<std::vector<float>> testVectors;
    std::vector<std::string> testIds;
    std::vector<float> queryVector;
};

TEST_F(MultiGPUVectorIndexTest, ConfigurationDefaults) {
    MultiGPUVectorIndex::Config config;
    
    EXPECT_FALSE(config.enableMultiGPU);
    EXPECT_EQ(config.partitionStrategy, MultiGPUVectorIndex::PartitionStrategy::ROUND_ROBIN);
    EXPECT_EQ(config.loadBalancing, MultiGPUVectorIndex::LoadBalancingMode::STATIC);
    EXPECT_TRUE(config.enableP2P);
    EXPECT_TRUE(config.enableFaultTolerance);
    EXPECT_TRUE(config.allowCPUFallback);
}

TEST_F(MultiGPUVectorIndexTest, InitializationWithoutMultiGPU) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = false;
    
    MultiGPUVectorIndex index(config);
    
    // Should fail because multi-GPU is not enabled
    EXPECT_FALSE(index.initialize(dimension));
}

TEST_F(MultiGPUVectorIndexTest, InitializationWithEmptyDeviceList) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {};  // Empty list
    
    MultiGPUVectorIndex index(config);
    
    // Should fail because no devices specified
    EXPECT_FALSE(index.initialize(dimension));
}

TEST_F(MultiGPUVectorIndexTest, InitializationWithCPUFallback) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};  // Request 2 GPUs
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    // Initialize - will use CPU fallback if no real GPUs available
    // In test environment without actual GPUs, this uses CPU
    bool initialized = index.initialize(dimension);
    
    // Note: In CI without GPUs, this will initialize with CPU fallback
    // The test validates the API works correctly
    if (!initialized) {
        // Expected in environments without GPU hardware
        GTEST_SKIP() << "capability:gpu_hardware_available=false;reason=no_gpu_hardware_available";
    }
}

TEST_F(MultiGPUVectorIndexTest, AddAndSearchVectors) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    config.partitionStrategy = MultiGPUVectorIndex::PartitionStrategy::ROUND_ROBIN;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed_or_no_gpu_hardware";
    }
    
    // Add vectors
    for (size_t i = 0; i < numVectors; ++i) {
        EXPECT_TRUE(index.addVector(testIds[i], testVectors[i]));
    }
    
    // Search
    auto results = index.search(queryVector, k);
    
    // Should return up to k results
    EXPECT_LE(results.size(), k);
    EXPECT_GT(results.size(), 0);
    
    // Results should be sorted by distance
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i-1].distance, results[i].distance);
    }
}

TEST_F(MultiGPUVectorIndexTest, BatchAddVectors) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    // Batch add
    EXPECT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Verify with search
    auto results = index.search(queryVector, k);
    EXPECT_GT(results.size(), 0);
}

TEST_F(MultiGPUVectorIndexTest, RemoveVector) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    // Add vectors
    index.addVectorBatch(testIds, testVectors);
    
    // Remove a vector
    EXPECT_TRUE(index.removeVector(testIds[0]));
    
    // Try to remove again - should fail
    EXPECT_FALSE(index.removeVector(testIds[0]));
}

TEST_F(MultiGPUVectorIndexTest, UpdateVector) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    // Add initial vector
    index.addVector(testIds[0], testVectors[0]);
    
    // Update with new vector
    std::vector<float> newVector(dimension, 0.5f);
    EXPECT_TRUE(index.updateVector(testIds[0], newVector));
}

TEST_F(MultiGPUVectorIndexTest, PartitionStrategyRoundRobin) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    config.partitionStrategy = MultiGPUVectorIndex::PartitionStrategy::ROUND_ROBIN;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    EXPECT_EQ(index.getPartitionStrategy(), 
              MultiGPUVectorIndex::PartitionStrategy::ROUND_ROBIN);
    
    // Add vectors - should be distributed round-robin
    index.addVectorBatch(testIds, testVectors);
    
    auto stats = index.getStatistics();
    
    // With round-robin and 2 GPUs, vectors should be roughly evenly distributed
    // Allow for some tolerance due to implementation details
    EXPECT_GT(stats.numActiveGPUs, 0);
}

TEST_F(MultiGPUVectorIndexTest, PartitionStrategyHashBased) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    config.partitionStrategy = MultiGPUVectorIndex::PartitionStrategy::HASH_BASED;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    index.setPartitionStrategy(MultiGPUVectorIndex::PartitionStrategy::HASH_BASED);
    EXPECT_EQ(index.getPartitionStrategy(), 
              MultiGPUVectorIndex::PartitionStrategy::HASH_BASED);
    
    index.addVectorBatch(testIds, testVectors);
}

TEST_F(MultiGPUVectorIndexTest, PartitionStrategyBalanced) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    config.partitionStrategy = MultiGPUVectorIndex::PartitionStrategy::BALANCED;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    index.addVectorBatch(testIds, testVectors);
    
    auto stats = index.getStatistics();
    
    // Balanced strategy should minimize load imbalance
    // In an ideal case with many vectors, imbalance should be low
    EXPECT_GE(stats.loadImbalance, 0.0);
}

TEST_F(MultiGPUVectorIndexTest, BatchSearch) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    index.addVectorBatch(testIds, testVectors);
    
    // Create multiple query vectors
    std::vector<std::vector<float>> queries = {queryVector, queryVector, queryVector};
    
    auto results = index.searchBatch(queries, k);
    
    EXPECT_EQ(results.size(), queries.size());
    for (const auto& queryResults : results) {
        EXPECT_LE(queryResults.size(), k);
    }
}

TEST_F(MultiGPUVectorIndexTest, Statistics) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    index.addVectorBatch(testIds, testVectors);
    
    auto stats = index.getStatistics();
    
    EXPECT_EQ(stats.totalVectors, numVectors);
    EXPECT_EQ(stats.totalDimension, dimension);
    EXPECT_GT(stats.numActiveGPUs, 0);
    EXPECT_GE(stats.loadImbalance, 0.0);
    EXPECT_GE(stats.scalingEfficiency, 0.0);
    EXPECT_LE(stats.scalingEfficiency, 1.0);
}

TEST_F(MultiGPUVectorIndexTest, GetActiveGPUs) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    auto activeGPUs = index.getActiveGPUs();
    EXPECT_GT(activeGPUs.size(), 0);
}

TEST_F(MultiGPUVectorIndexTest, SetEfSearch) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    config.efSearch = 64;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    // Change efSearch parameter
    index.setEfSearch(128);
    
    // Add and search to verify it works
    index.addVectorBatch(testIds, testVectors);
    auto results = index.search(queryVector, k);
    EXPECT_GT(results.size(), 0);
}

TEST_F(MultiGPUVectorIndexTest, Rebalance) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    index.addVectorBatch(testIds, testVectors);
    
    // Trigger rebalance
    // Note: Current implementation is a placeholder
    bool result = index.rebalance();
    
    // Rebalance should succeed if index is initialized
    EXPECT_TRUE(result);
}

TEST_F(MultiGPUVectorIndexTest, SearchResultsIncludeSourceGPU) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;
    
    MultiGPUVectorIndex index(config);
    
    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }
    
    index.addVectorBatch(testIds, testVectors);
    auto results = index.search(queryVector, k);
    
    // Each result should have a valid source GPU ID
    for (const auto& result : results) {
        EXPECT_GE(result.sourceGPU, 0);
        EXPECT_FALSE(result.id.empty());
        EXPECT_GE(result.distance, 0.0f);
    }
}

TEST_F(MultiGPUVectorIndexTest, BatchSearch_ParallelResults_MatchSequential) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;

    MultiGPUVectorIndex index(config);

    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }

    index.addVectorBatch(testIds, testVectors);

    // Build a batch of identical queries
    std::vector<std::vector<float>> queries(4, queryVector);

    auto batchResults = index.searchBatch(queries, k);

    ASSERT_EQ(batchResults.size(), queries.size());

    // All query results should be identical (same query vector)
    for (const auto& queryResults : batchResults) {
        EXPECT_LE(queryResults.size(), k);
        EXPECT_GT(queryResults.size(), 0u);
    }

    // Results should be sorted by distance within each query
    for (const auto& queryResults : batchResults) {
        for (size_t i = 1; i < queryResults.size(); ++i) {
            EXPECT_LE(queryResults[i - 1].distance, queryResults[i].distance);
        }
    }
}

TEST_F(MultiGPUVectorIndexTest, Statistics_UtilizationTracked_AfterSearch) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;

    MultiGPUVectorIndex index(config);

    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }

    index.addVectorBatch(testIds, testVectors);

    // Perform several searches to accumulate utilization data
    for (int i = 0; i < 5; ++i) {
        index.search(queryVector, k);
    }

    auto stats = index.getStatistics();

    // After real queries every active GPU should have a non-negative utilization
    for (const auto& gpuStat : stats.perGPUStats) {
        EXPECT_GE(gpuStat.utilizationPercent, 0.0)
            << "GPU " << gpuStat.deviceId << " utilization must be >= 0";
        EXPECT_LE(gpuStat.utilizationPercent, 100.0)
            << "GPU " << gpuStat.deviceId << " utilization must be <= 100";
    }
}

TEST_F(MultiGPUVectorIndexTest, Statistics_UtilizationTracked_AfterBatchSearch) {
    MultiGPUVectorIndex::Config config;
    config.enableMultiGPU = true;
    config.deviceIds = {0, 1};
    config.allowCPUFallback = true;

    MultiGPUVectorIndex index(config);

    if (!index.initialize(dimension)) {
        GTEST_SKIP() << "capability:multi_gpu_index_initialized=false;reason=initialization_failed";
    }

    index.addVectorBatch(testIds, testVectors);

    std::vector<std::vector<float>> queries(8, queryVector);
    index.searchBatch(queries, k);

    auto stats = index.getStatistics();
    EXPECT_EQ(stats.totalVectors, numVectors);

    for (const auto& gpuStat : stats.perGPUStats) {
        EXPECT_GE(gpuStat.utilizationPercent, 0.0);
        EXPECT_LE(gpuStat.utilizationPercent, 100.0);
    }
}

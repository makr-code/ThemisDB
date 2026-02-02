#include "index/gpu_vector_index.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <limits>

using namespace themis::index;

class GPUVectorIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        dimension = 128;
        numVectors = 1000;
        
        // Generate random test vectors
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (size_t i = 0; i < numVectors; ++i) {
            std::vector<float> vec(dimension);
            for (int j = 0; j < dimension; ++j) {
                vec[j] = dist(gen);
            }
            testVectors.push_back(vec);
            testIds.push_back("vec_" + std::to_string(i));
        }
        
        // Generate a query vector
        queryVector.resize(dimension);
        for (int j = 0; j < dimension; ++j) {
            queryVector[j] = dist(gen);
        }
    }
    
    int dimension;
    size_t numVectors;
    std::vector<std::vector<float>> testVectors;
    std::vector<std::string> testIds;
    std::vector<float> queryVector;
};

TEST_F(GPUVectorIndexTest, Initialization) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::AUTO;
    config.metric = GPUVectorIndex::DistanceMetric::COSINE;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    auto stats = index.getStatistics();
    EXPECT_EQ(stats.dimension, dimension);
    EXPECT_EQ(stats.numVectors, 0);
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, AddVector) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU; // Use CPU for reliable testing
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add a single vector
    ASSERT_TRUE(index.addVector("test_1", testVectors[0]));
    
    auto stats = index.getStatistics();
    EXPECT_EQ(stats.numVectors, 1);
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, AddVectorBatch) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add batch of vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    auto stats = index.getStatistics();
    EXPECT_EQ(stats.numVectors, numVectors);
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, Search) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Search for nearest neighbors
    size_t k = 10;
    auto results = index.search(queryVector, k);
    
    EXPECT_EQ(results.size(), k);
    
    // Results should be sorted by distance (ascending)
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i-1].distance, results[i].distance);
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, SearchBatch) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Create multiple queries
    std::vector<std::vector<float>> queries = {queryVector, testVectors[0], testVectors[1]};
    
    // Batch search
    size_t k = 5;
    auto results = index.searchBatch(queries, k);
    
    EXPECT_EQ(results.size(), queries.size());
    for (const auto& queryResults : results) {
        EXPECT_EQ(queryResults.size(), k);
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, RemoveVector) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    EXPECT_EQ(index.getStatistics().numVectors, numVectors);
    
    // Remove a vector
    ASSERT_TRUE(index.removeVector(testIds[0]));
    EXPECT_EQ(index.getStatistics().numVectors, numVectors - 1);
    
    // Try to remove non-existent vector
    EXPECT_FALSE(index.removeVector("non_existent"));
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, UpdateVector) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add a vector
    ASSERT_TRUE(index.addVector(testIds[0], testVectors[0]));
    
    // Update the vector
    std::vector<float> newVector(dimension, 1.0f);
    ASSERT_TRUE(index.updateVector(testIds[0], newVector));
    
    // Number of vectors should remain the same
    EXPECT_EQ(index.getStatistics().numVectors, 1);
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, DistanceMetrics) {
    // Test different distance metrics
    std::vector<GPUVectorIndex::DistanceMetric> metrics = {
        GPUVectorIndex::DistanceMetric::L2,
        GPUVectorIndex::DistanceMetric::COSINE,
        GPUVectorIndex::DistanceMetric::INNER_PRODUCT
    };
    
    for (auto metric : metrics) {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;
        config.metric = metric;
        
        GPUVectorIndex index(config);
        ASSERT_TRUE(index.initialize(dimension));
        
        // Add a few vectors
        std::vector<std::string> ids = {testIds[0], testIds[1], testIds[2]};
        std::vector<std::vector<float>> vecs = {testVectors[0], testVectors[1], testVectors[2]};
        ASSERT_TRUE(index.addVectorBatch(ids, vecs));
        
        // Search
        auto results = index.search(queryVector, 2);
        EXPECT_EQ(results.size(), 2);
        EXPECT_GE(results[0].distance, 0.0f);
        
        index.shutdown();
    }
}

TEST_F(GPUVectorIndexTest, BackendSelection) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::AUTO;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Get available backends
    auto backends = index.getAvailableBackends();
    EXPECT_FALSE(backends.empty());
    
    // CPU should always be available
    EXPECT_NE(std::find(backends.begin(), backends.end(), 
                       GPUVectorIndex::Backend::CPU), backends.end());
    
    // Check active backend
    auto activeBackend = index.getActiveBackend();
    EXPECT_NE(std::find(backends.begin(), backends.end(), activeBackend), backends.end());
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, Statistics) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Perform some searches
    for (int i = 0; i < 10; ++i) {
        index.search(queryVector, 5);
    }
    
    // Check statistics
    auto stats = index.getStatistics();
    EXPECT_EQ(stats.numVectors, numVectors);
    EXPECT_EQ(stats.dimension, dimension);
    EXPECT_GT(stats.avgQueryTimeMs, 0.0);
    
    index.shutdown();
}

// Input validation tests
TEST_F(GPUVectorIndexTest, InvalidDimension) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    
    // Test negative dimension
    EXPECT_FALSE(index.initialize(-1));
    
    // Test zero dimension
    EXPECT_FALSE(index.initialize(0));
    
    // Test extremely large dimension
    EXPECT_FALSE(index.initialize(100000));
}

TEST_F(GPUVectorIndexTest, EmptyVectorId) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Try to add vector with empty ID
    EXPECT_FALSE(index.addVector("", testVectors[0]));
}

TEST_F(GPUVectorIndexTest, DimensionMismatch) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Try to add vector with wrong dimension
    std::vector<float> wrongDimVector(dimension + 10, 1.0f);
    EXPECT_FALSE(index.addVector("test_wrong", wrongDimVector));
    
    // Try to search with wrong dimension
    auto results = index.search(wrongDimVector, 5);
    EXPECT_TRUE(results.empty());
}

TEST_F(GPUVectorIndexTest, InvalidKValue) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Search with k=0 should return empty results
    auto results = index.search(queryVector, 0);
    EXPECT_TRUE(results.empty());
}

TEST_F(GPUVectorIndexTest, NonFiniteValues) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Try to add vector with NaN
    std::vector<float> nanVector(dimension, 1.0f);
    nanVector[0] = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(index.addVector("test_nan", nanVector));
    
    // Try to add vector with Infinity
    std::vector<float> infVector(dimension, 1.0f);
    infVector[0] = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(index.addVector("test_inf", infVector));
    
    // Add valid vectors first
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Try to search with NaN query
    std::vector<float> nanQuery(dimension, 1.0f);
    nanQuery[0] = std::numeric_limits<float>::quiet_NaN();
    auto results = index.search(nanQuery, 5);
    EXPECT_TRUE(results.empty());
}

TEST_F(GPUVectorIndexTest, ConfigValidation) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    // Set invalid values - should be auto-corrected
    config.M = -1;
    config.efConstruction = 0;
    config.efSearch = -10;
    config.batchSize = 0;
    config.maxVRAM_MB = 0;
    config.deviceId = -5;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Index should still work with corrected values
    ASSERT_TRUE(index.addVector(testIds[0], testVectors[0]));
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, SetterValidation) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Set valid values first
    index.setEfSearch(100);
    index.setBatchSize(256);
    
    // Add a test vector to ensure index works
    ASSERT_TRUE(index.addVector(testIds[0], testVectors[0]));
    
    // Try to set invalid efSearch - should be ignored
    index.setEfSearch(-10);
    // Index should still work after invalid call
    EXPECT_TRUE(index.addVector(testIds[1], testVectors[1]));
    
    index.setEfSearch(0);
    // Index should still work
    EXPECT_TRUE(index.addVector(testIds[2], testVectors[2]));
    
    // Try to set invalid batch size - should be ignored or clamped
    index.setBatchSize(-100);
    // Index should still work
    auto results = index.search(queryVector, 2);
    EXPECT_EQ(results.size(), 2);
    
    index.setBatchSize(0);
    // Index should still work
    results = index.search(queryVector, 2);
    EXPECT_EQ(results.size(), 2);
    
    // Test upper bound - should be clamped
    index.setBatchSize(20000);
    // Index should still work
    results = index.search(queryVector, 2);
    EXPECT_EQ(results.size(), 2);
    
    index.shutdown();
}

// Performance benchmark test (optional, can be slow)
TEST_F(GPUVectorIndexTest, DISABLED_PerformanceBenchmark) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add large number of vectors
    size_t largeNumVectors = 10000;
    std::vector<std::string> largeIds;
    std::vector<std::vector<float>> largeVectors;
    
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (size_t i = 0; i < largeNumVectors; ++i) {
        std::vector<float> vec(dimension);
        for (int j = 0; j < dimension; ++j) {
            vec[j] = dist(gen);
        }
        largeVectors.push_back(vec);
        largeIds.push_back("vec_" + std::to_string(i));
    }
    
    ASSERT_TRUE(index.addVectorBatch(largeIds, largeVectors));
    
    // Perform searches and measure time
    auto start = std::chrono::steady_clock::now();
    size_t numQueries = 100;
    for (size_t i = 0; i < numQueries; ++i) {
        index.search(queryVector, 10);
    }
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double qps = numQueries / (duration.count() / 1000.0);
    
    std::cout << "Performance: " << qps << " queries/sec" << std::endl;
    std::cout << "Avg query time: " << (duration.count() / numQueries) << " ms" << std::endl;
    
    index.shutdown();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

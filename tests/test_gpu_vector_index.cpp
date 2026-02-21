/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gpu_vector_index.cpp                          ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:57:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     597                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/gpu_vector_index.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <random>
#include <cmath>

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

// ============================================================================
// CUDA Backend Tests (v2.1+)
// ============================================================================

TEST_F(GPUVectorIndexTest, CUDABackendAvailability) {
    // Check if CUDA backend is available
    GPUVectorIndex tempIndex(GPUVectorIndex::Config{});
    tempIndex.initialize(dimension);
    auto backends = tempIndex.getAvailableBackends();
    
    // CPU must always be available
    EXPECT_NE(std::find(backends.begin(), backends.end(), 
                       GPUVectorIndex::Backend::CPU), backends.end());
    
    bool cudaAvailable = std::find(backends.begin(), backends.end(), 
                                   GPUVectorIndex::Backend::CUDA) != backends.end();
    
    if (cudaAvailable) {
        std::cout << "CUDA backend is available for testing" << std::endl;
        
        // If CUDA is listed as available, switching to it should succeed
        EXPECT_TRUE(tempIndex.switchBackend(GPUVectorIndex::Backend::CUDA));
    } else {
        std::cout << "CUDA backend not available (expected in CI without GPU)" << std::endl;
        
        // If CUDA is not available, switching to it should fail (unless fallback is allowed)
        // This is acceptable behavior
    }
    
    tempIndex.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDABackendInitialization) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.allowCPUFallback = true;  // Allow fallback in environments without GPU
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Check active backend (could be CPU if CUDA not available)
    auto activeBackend = index.getActiveBackend();
    EXPECT_TRUE(activeBackend == GPUVectorIndex::Backend::CUDA || 
                activeBackend == GPUVectorIndex::Backend::CPU);
    
    auto stats = index.getStatistics();
    std::cout << "Active backend: " << (stats.isGPUActive ? "GPU" : "CPU") << std::endl;
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDASearch) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.metric = GPUVectorIndex::DistanceMetric::L2;
    config.allowCPUFallback = true;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Search
    size_t k = 10;
    auto results = index.search(queryVector, k);
    
    EXPECT_LE(results.size(), k);
    if (!results.empty()) {
        // Results should be sorted by distance
        for (size_t i = 1; i < results.size(); ++i) {
            EXPECT_LE(results[i-1].distance, results[i].distance);
        }
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDACPUResultComparison) {
    // Compare CUDA and CPU results for consistency
    GPUVectorIndex::Config cpuConfig;
    cpuConfig.backend = GPUVectorIndex::Backend::CPU;
    cpuConfig.metric = GPUVectorIndex::DistanceMetric::L2;
    
    GPUVectorIndex::Config cudaConfig;
    cudaConfig.backend = GPUVectorIndex::Backend::CUDA;
    cudaConfig.metric = GPUVectorIndex::DistanceMetric::L2;
    cudaConfig.allowCPUFallback = true;
    
    GPUVectorIndex cpuIndex(cpuConfig);
    GPUVectorIndex cudaIndex(cudaConfig);
    
    ASSERT_TRUE(cpuIndex.initialize(dimension));
    ASSERT_TRUE(cudaIndex.initialize(dimension));
    
    // Add same vectors to both
    ASSERT_TRUE(cpuIndex.addVectorBatch(testIds, testVectors));
    ASSERT_TRUE(cudaIndex.addVectorBatch(testIds, testVectors));
    
    // Search with both backends
    size_t k = 5;
    auto cpuResults = cpuIndex.search(queryVector, k);
    auto cudaResults = cudaIndex.search(queryVector, k);
    
    // If CUDA is active, results should match (within floating point tolerance)
    if (cudaIndex.getActiveBackend() == GPUVectorIndex::Backend::CUDA) {
        EXPECT_EQ(cpuResults.size(), cudaResults.size());
        
        for (size_t i = 0; i < std::min(cpuResults.size(), cudaResults.size()); ++i) {
            EXPECT_EQ(cpuResults[i].id, cudaResults[i].id);
            EXPECT_NEAR(cpuResults[i].distance, cudaResults[i].distance, 1e-3f);  // Relaxed tolerance for GPU/CPU diff
        }
    }
    
    cpuIndex.shutdown();
    cudaIndex.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDABackendSwitching) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::AUTO;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Try switching to CPU
    EXPECT_TRUE(index.switchBackend(GPUVectorIndex::Backend::CPU));
    EXPECT_EQ(index.getActiveBackend(), GPUVectorIndex::Backend::CPU);
    
    // Search should work with CPU
    auto cpuResults = index.search(queryVector, 5);
    EXPECT_FALSE(cpuResults.empty());
    
    // Try switching to CUDA (may not be available)
    bool cudaSwitch = index.switchBackend(GPUVectorIndex::Backend::CUDA);
    if (cudaSwitch) {
        EXPECT_EQ(index.getActiveBackend(), GPUVectorIndex::Backend::CUDA);
        
        // Search should work with CUDA
        auto cudaResults = index.search(queryVector, 5);
        EXPECT_FALSE(cudaResults.empty());
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDABatchSearch) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.allowCPUFallback = true;
    
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
        EXPECT_LE(queryResults.size(), k);
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDACosineMetric) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.metric = GPUVectorIndex::DistanceMetric::COSINE;
    config.allowCPUFallback = true;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Search with COSINE metric
    size_t k = 5;
    auto results = index.search(queryVector, k);
    
    EXPECT_LE(results.size(), k);
    if (!results.empty()) {
        // Results should be sorted by distance
        for (size_t i = 1; i < results.size(); ++i) {
            EXPECT_LE(results[i-1].distance, results[i].distance);
        }
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDAInnerProductFallback) {
    // Test that INNER_PRODUCT falls back to CPU when CUDA is requested
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.metric = GPUVectorIndex::DistanceMetric::INNER_PRODUCT;
    config.allowCPUFallback = true;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Search with INNER_PRODUCT metric
    // This should work but will use CPU fallback internally
    size_t k = 5;
    auto results = index.search(queryVector, k);
    
    EXPECT_LE(results.size(), k);
    if (!results.empty()) {
        // Results should be sorted by distance
        for (size_t i = 1; i < results.size(); ++i) {
            EXPECT_LE(results[i-1].distance, results[i].distance);
        }
    }
    
    index.shutdown();
}

TEST_F(GPUVectorIndexTest, CUDACosineCPUComparison) {
    // Compare CUDA and CPU results for COSINE metric
    GPUVectorIndex::Config cpuConfig;
    cpuConfig.backend = GPUVectorIndex::Backend::CPU;
    cpuConfig.metric = GPUVectorIndex::DistanceMetric::COSINE;
    
    GPUVectorIndex::Config cudaConfig;
    cudaConfig.backend = GPUVectorIndex::Backend::CUDA;
    cudaConfig.metric = GPUVectorIndex::DistanceMetric::COSINE;
    cudaConfig.allowCPUFallback = true;
    
    GPUVectorIndex cpuIndex(cpuConfig);
    GPUVectorIndex cudaIndex(cudaConfig);
    
    ASSERT_TRUE(cpuIndex.initialize(dimension));
    ASSERT_TRUE(cudaIndex.initialize(dimension));
    
    // Add same vectors to both
    ASSERT_TRUE(cpuIndex.addVectorBatch(testIds, testVectors));
    ASSERT_TRUE(cudaIndex.addVectorBatch(testIds, testVectors));
    
    // Search with both backends
    size_t k = 5;
    auto cpuResults = cpuIndex.search(queryVector, k);
    auto cudaResults = cudaIndex.search(queryVector, k);
    
    // If CUDA is active, results should match
    if (cudaIndex.getActiveBackend() == GPUVectorIndex::Backend::CUDA) {
        EXPECT_EQ(cpuResults.size(), cudaResults.size());
        
        for (size_t i = 0; i < std::min(cpuResults.size(), cudaResults.size()); ++i) {
            EXPECT_EQ(cpuResults[i].id, cudaResults[i].id);
            EXPECT_NEAR(cpuResults[i].distance, cudaResults[i].distance, 1e-3f);
        }
    }
    
    cpuIndex.shutdown();
    cudaIndex.shutdown();
}
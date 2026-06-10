/*
 * ThemisDB | File: test_gpu_vector_index.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "index/gpu_vector_index.h"
#include "themis/gpu/memory_manager.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <optional>

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

TEST_F(GPUVectorIndexTest, FailedBackendSwitchPreservesExistingIndexState) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));

    const auto baselineResults = index.search(queryVector, 5);
    const auto baselineStats = index.getStatistics();

    const auto available = index.getAvailableBackends();
    const GPUVectorIndex::Backend candidates[] = {
        GPUVectorIndex::Backend::CUDA,
        GPUVectorIndex::Backend::HIP,
        GPUVectorIndex::Backend::VULKAN
    };

    std::optional<GPUVectorIndex::Backend> unavailableBackend;
    for (auto candidate : candidates) {
        if (std::find(available.begin(), available.end(), candidate) == available.end()) {
            unavailableBackend = candidate;
            break;
        }
    }

    if (!unavailableBackend.has_value()) {
        GTEST_SKIP() << "All non-CPU GPU backends are available in this environment";
    }

    EXPECT_FALSE(index.switchBackend(*unavailableBackend));
    EXPECT_EQ(index.getActiveBackend(), GPUVectorIndex::Backend::CPU);

    const auto afterStats = index.getStatistics();
    EXPECT_EQ(afterStats.numVectors, baselineStats.numVectors);

    const auto afterResults = index.search(queryVector, 5);
    EXPECT_EQ(afterResults.size(), baselineResults.size());
    for (size_t i = 0; i < afterResults.size(); ++i) {
        EXPECT_EQ(afterResults[i].id, baselineResults[i].id);
    }

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, SuccessfulNonCpuBackendSwitchPreservesVectorCount) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.allowCPUFallback = true;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));

    const auto baselineResults = index.search(queryVector, 5);
    const size_t baselineCount = index.getStatistics().numVectors;

    const auto available = index.getAvailableBackends();
    auto targetIt = std::find_if(
        available.begin(),
        available.end(),
        [](GPUVectorIndex::Backend backend) {
            return backend != GPUVectorIndex::Backend::CPU;
        });

    if (targetIt == available.end()) {
        GTEST_SKIP() << "No non-CPU backend available in this environment";
    }

    ASSERT_TRUE(index.switchBackend(*targetIt));
    EXPECT_EQ(index.getStatistics().numVectors, baselineCount);

    const auto afterResults = index.search(queryVector, 5);
    EXPECT_EQ(afterResults.size(), baselineResults.size());
    for (size_t i = 0; i < afterResults.size(); ++i) {
        EXPECT_EQ(afterResults[i].id, baselineResults[i].id);
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

TEST_F(GPUVectorIndexTest, CUDAInnerProductSearch) {
    // Test that INNER_PRODUCT metric works with the CUDA backend
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CUDA;
    config.metric = GPUVectorIndex::DistanceMetric::INNER_PRODUCT;
    config.allowCPUFallback = true;
    
    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    
    // Add vectors
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    
    // Search with INNER_PRODUCT metric
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

// ============================================================================
// GPU Memory Budget Tests (maxVRAM_MB per index)
// ============================================================================

TEST_F(GPUVectorIndexTest, VRAMBudget_ZeroMeansNoLimit) {
    // maxVRAM_MB == 0 (default) must not block any addition
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.maxVRAM_MB = 0;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
    EXPECT_EQ(index.getStatistics().numVectors, numVectors);

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, VRAMBudget_EnforcedOnAddVector) {
    if (themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=vram_budget_requires_gpu_capable_edition";
    }
    // Choose a budget that holds exactly 1 vector and verify the 2nd is rejected
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    // 1 MB budget; capacity = 1 MB / (dimension * sizeof(float)) vectors.
    config.maxVRAM_MB = 1;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    // Adding up to the budget limit should succeed
    const size_t bytesPerVec = static_cast<size_t>(dimension) * sizeof(float);
    const size_t maxVectors = (1ULL * 1024ULL * 1024ULL) / bytesPerVec;

    std::vector<std::string> ids;
    std::vector<std::vector<float>> vecs;
    for (size_t i = 0; i < maxVectors; ++i) {
        ids.push_back("v_" + std::to_string(i));
        vecs.push_back(testVectors[i % testVectors.size()]);
    }
    EXPECT_TRUE(index.addVectorBatch(ids, vecs));
    EXPECT_EQ(index.getStatistics().numVectors, maxVectors);

    // One more vector must be rejected because the budget is exhausted
    EXPECT_FALSE(index.addVector("overflow", testVectors[0]));
    EXPECT_EQ(index.getStatistics().numVectors, maxVectors);  // count unchanged

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, VRAMBudget_UpdateDoesNotConsumeExtraBudget) {
    if (themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=vram_budget_requires_gpu_capable_edition";
    }
    // Updating an existing vector must not allocate additional budget
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.maxVRAM_MB = 1;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    ASSERT_TRUE(index.addVector("vec0", testVectors[0]));
    EXPECT_EQ(index.getStatistics().numVectors, 1u);

    // Fill to budget
    const size_t bytesPerVec = static_cast<size_t>(dimension) * sizeof(float);
    const size_t maxVectors = (1ULL * 1024ULL * 1024ULL) / bytesPerVec;
    for (size_t i = 1; i < maxVectors; ++i) {
        ASSERT_TRUE(index.addVector("v_" + std::to_string(i),
                                    testVectors[i % testVectors.size()]));
    }

    // Budget is now full; updating "vec0" should still succeed (no new allocation)
    EXPECT_TRUE(index.updateVector("vec0", testVectors[1]));
    EXPECT_EQ(index.getStatistics().numVectors, maxVectors);

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, VRAMBudget_ReleasedOnRemoveVector) {
    if (themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=vram_budget_requires_gpu_capable_edition";
    }
    // After removing a vector the freed budget slot must be reusable
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.maxVRAM_MB = 1;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    // Fill to budget
    const size_t bytesPerVec = static_cast<size_t>(dimension) * sizeof(float);
    const size_t maxVectors = (1ULL * 1024ULL * 1024ULL) / bytesPerVec;
    for (size_t i = 0; i < maxVectors; ++i) {
        ASSERT_TRUE(index.addVector("v_" + std::to_string(i),
                                    testVectors[i % testVectors.size()]));
    }

    // Budget exhausted — adding one more must fail
    EXPECT_FALSE(index.addVector("overflow", testVectors[0]));

    // Remove one vector, then re-add should succeed
    ASSERT_TRUE(index.removeVector("v_0"));
    EXPECT_TRUE(index.addVector("new_vec", testVectors[0]));
    EXPECT_EQ(index.getStatistics().numVectors, maxVectors);

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, VRAMBudget_ReleasedOnShutdown) {
    if (themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=vram_budget_requires_gpu_capable_edition";
    }
    // After shutdown() the GPUMemoryManager tenant usage must be zero
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.maxVRAM_MB = 1;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    ASSERT_TRUE(index.addVector("v0", testVectors[0]));

    // Record global VRAM usage before shutdown
    const uint64_t usedBefore =
        themis::gpu::GPUMemoryManager::GetInstance().GetGPUMemoryUsed();

    index.shutdown();

    // In CPU/fallback paths global GPU usage may already be zero.
    // Verify that shutdown never increases usage and requires a decrease
    // only when there was measurable usage before shutdown.
    const uint64_t usedAfter =
        themis::gpu::GPUMemoryManager::GetInstance().GetGPUMemoryUsed();
    EXPECT_LE(usedAfter, usedBefore);
    if (usedBefore > 0) {
        EXPECT_LT(usedAfter, usedBefore);
    }
}

TEST_F(GPUVectorIndexTest, VRAMBudget_StatisticsReflectsUsage) {
    if (themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=vram_budget_requires_gpu_capable_edition";
    }
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.maxVRAM_MB = 1;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    // Initially no VRAM used
    EXPECT_EQ(index.getStatistics().vramUsageBytes, 0u);

    // Add a vector and check that vramUsageBytes increases accordingly
    ASSERT_TRUE(index.addVector("v0", testVectors[0]));
    const uint64_t expectedBytes =
        static_cast<uint64_t>(dimension) * sizeof(float);
    EXPECT_EQ(index.getStatistics().vramUsageBytes, expectedBytes);

    // Remove the vector and check that vramUsageBytes decreases back to zero
    ASSERT_TRUE(index.removeVector("v0"));
    EXPECT_EQ(index.getStatistics().vramUsageBytes, 0u);

    index.shutdown();
}

// ============================================================================
// buildIndex() Tests
// ============================================================================

TEST_F(GPUVectorIndexTest, BuildIndex_EmptyIndex) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    // buildIndex() on an empty index must succeed
    EXPECT_TRUE(index.buildIndex());

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, BuildIndex_NotInitialized) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    // Do NOT call initialize()

    // buildIndex() before initialize() must return false
    EXPECT_FALSE(index.buildIndex());
}

TEST_F(GPUVectorIndexTest, BuildIndex_SearchAfterBuild) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric  = GPUVectorIndex::DistanceMetric::L2;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));

    // buildIndex() must succeed and leave the index usable for search
    ASSERT_TRUE(index.buildIndex());

    size_t k = 5;
    auto results = index.search(queryVector, k);
    EXPECT_EQ(results.size(), k);

    // Results must be sorted by ascending distance
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i - 1].distance, results[i].distance);
    }

    index.shutdown();
}

TEST_F(GPUVectorIndexTest, BuildIndex_MultipleCalls) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));
    ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));

    // Calling buildIndex() multiple times must always succeed
    EXPECT_TRUE(index.buildIndex());
    EXPECT_TRUE(index.buildIndex());

    index.shutdown();
}

// ============================================================================
// saveIndex() / loadIndex() Tests
// ============================================================================

TEST_F(GPUVectorIndexTest, SaveIndex_NotInitialized) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    // No initialize() call

    EXPECT_FALSE(index.saveIndex("/tmp/test_gpu_index.bin"));
}

TEST_F(GPUVectorIndexTest, SaveIndex_EmptyPath) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    ASSERT_TRUE(index.initialize(dimension));

    EXPECT_FALSE(index.saveIndex(""));
}

TEST_F(GPUVectorIndexTest, LoadIndex_EmptyPath) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    EXPECT_FALSE(index.loadIndex(""));
}

TEST_F(GPUVectorIndexTest, LoadIndex_NonExistentFile) {
    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;

    GPUVectorIndex index(config);
    EXPECT_FALSE(index.loadIndex("/tmp/nonexistent_index_file_xyz.bin"));
}

TEST_F(GPUVectorIndexTest, SaveAndLoadIndex_RoundTrip) {
    const std::string indexPath = "/tmp/test_gpu_index_roundtrip.bin";

    // Build and save
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;
        config.metric  = GPUVectorIndex::DistanceMetric::L2;

        GPUVectorIndex index(config);
        ASSERT_TRUE(index.initialize(dimension));
        ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
        ASSERT_TRUE(index.saveIndex(indexPath));
        index.shutdown();
    }

    // Load and verify
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;
        config.metric  = GPUVectorIndex::DistanceMetric::L2;

        GPUVectorIndex index(config);
        ASSERT_TRUE(index.loadIndex(indexPath));

        auto stats = index.getStatistics();
        EXPECT_EQ(stats.numVectors, numVectors);
        EXPECT_EQ(stats.dimension,  static_cast<size_t>(dimension));

        // Search must return plausible results after load
        size_t k = 5;
        auto results = index.search(queryVector, k);
        EXPECT_EQ(results.size(), k);

        // All returned IDs must have been part of the original set
        for (const auto& r : results) {
            bool found = false;
            for (const auto& id : testIds) {
                if (id == r.id) { found = true; break; }
            }
            EXPECT_TRUE(found) << "Unexpected ID after load: " << r.id;
        }

        index.shutdown();
    }

    std::remove(indexPath.c_str());
}

TEST_F(GPUVectorIndexTest, SaveAndLoadIndex_SearchResultsMatch) {
    const std::string indexPath = "/tmp/test_gpu_index_match.bin";

    GPUVectorIndex::Config config;
    config.backend = GPUVectorIndex::Backend::CPU;
    config.metric  = GPUVectorIndex::DistanceMetric::L2;

    // Compute reference results before save
    std::vector<GPUVectorIndex::SearchResult> refResults;
    {
        GPUVectorIndex refIndex(config);
        ASSERT_TRUE(refIndex.initialize(dimension));
        ASSERT_TRUE(refIndex.addVectorBatch(testIds, testVectors));
        refResults = refIndex.search(queryVector, 5);
        ASSERT_TRUE(refIndex.saveIndex(indexPath));
        refIndex.shutdown();
    }

    // Load and compare search results
    {
        GPUVectorIndex loadedIndex(config);
        ASSERT_TRUE(loadedIndex.loadIndex(indexPath));

        auto loadedResults = loadedIndex.search(queryVector, 5);
        ASSERT_EQ(refResults.size(), loadedResults.size());

        for (size_t i = 0; i < refResults.size(); ++i) {
            EXPECT_EQ(refResults[i].id, loadedResults[i].id);
            EXPECT_NEAR(refResults[i].distance, loadedResults[i].distance, 1e-5f);
        }

        loadedIndex.shutdown();
    }

    std::remove(indexPath.c_str());
}

TEST_F(GPUVectorIndexTest, LoadIndex_OverwritesExistingVectors) {
    const std::string indexPath = "/tmp/test_gpu_index_overwrite.bin";

    // Save a small index
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;

        GPUVectorIndex index(config);
        ASSERT_TRUE(index.initialize(dimension));
        // Add only first 10 vectors
        std::vector<std::string> smallIds(testIds.begin(), testIds.begin() + 10);
        std::vector<std::vector<float>> smallVecs(testVectors.begin(), testVectors.begin() + 10);
        ASSERT_TRUE(index.addVectorBatch(smallIds, smallVecs));
        ASSERT_TRUE(index.saveIndex(indexPath));
        index.shutdown();
    }

    // Load into an already-populated index — old vectors must be replaced
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;

        GPUVectorIndex index(config);
        ASSERT_TRUE(index.initialize(dimension));
        ASSERT_TRUE(index.addVectorBatch(testIds, testVectors));
        EXPECT_EQ(index.getStatistics().numVectors, numVectors);

        ASSERT_TRUE(index.loadIndex(indexPath));
        EXPECT_EQ(index.getStatistics().numVectors, 10u);

        index.shutdown();
    }

    std::remove(indexPath.c_str());
}

// ============================================================================
// Audit: VRAM accounting correctness after loadIndex() overwrite
// ============================================================================

TEST_F(GPUVectorIndexTest, VRAMBudget_CorrectAfterLoadIndexOverwrite) {
    if (themis::gpu::GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=vram_budget_requires_gpu_capable_edition";
    }

    const std::string indexPath = "/tmp/test_gpu_vram_overwrite.bin";

    // Save a small (10-vector) index
    {
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;

        GPUVectorIndex saveIdx(config);
        ASSERT_TRUE(saveIdx.initialize(dimension));
        std::vector<std::string> smallIds(testIds.begin(), testIds.begin() + 10);
        std::vector<std::vector<float>> smallVecs(testVectors.begin(), testVectors.begin() + 10);
        ASSERT_TRUE(saveIdx.addVectorBatch(smallIds, smallVecs));
        ASSERT_TRUE(saveIdx.saveIndex(indexPath));
        saveIdx.shutdown();
    }

    // Load the small index into an already-populated (large) index
    {
        constexpr size_t kTestVRAMBudgetMB = 1;
        GPUVectorIndex::Config config;
        config.backend = GPUVectorIndex::Backend::CPU;
        config.maxVRAM_MB = kTestVRAMBudgetMB;

        GPUVectorIndex index(config);
        ASSERT_TRUE(index.initialize(dimension));

        // Populate with some vectors first
        const size_t bytesPerVec = static_cast<size_t>(dimension) * sizeof(float);
        const size_t fillCount = (kTestVRAMBudgetMB * 1024ULL * 1024ULL) / bytesPerVec / 2;
        for (size_t i = 0; i < fillCount; ++i) {
            ASSERT_TRUE(index.addVector("pre_" + std::to_string(i),
                                        testVectors[i % testVectors.size()]));
        }
        EXPECT_GT(index.getStatistics().vramUsageBytes, 0u);

        // loadIndex() must release pre-load allocations and re-account correctly
        ASSERT_TRUE(index.loadIndex(indexPath));
        EXPECT_EQ(index.getStatistics().numVectors, 10u);

        // After load, vramUsageBytes must reflect only the 10 loaded vectors
        const uint64_t expectedAfter = 10u * static_cast<uint64_t>(bytesPerVec);
        EXPECT_EQ(index.getStatistics().vramUsageBytes, expectedAfter);

        // The freed budget headroom should be usable
        EXPECT_TRUE(index.addVector("extra", testVectors[0]));

        index.shutdown();
    }

    std::remove(indexPath.c_str());
}
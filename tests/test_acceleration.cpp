#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/cuda_backend.h"
#include "acceleration/ai_hardware_dispatcher.h"
#include <vector>
#include <cmath>
#include <limits>

using namespace themis::acceleration;

class AccelerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register CPU backend (always available)
        auto cpuVector = std::make_unique<CPUVectorBackend>();
        BackendRegistry::instance().registerBackend(std::move(cpuVector));
        
        auto cpuGraph = std::make_unique<CPUGraphBackend>();
        BackendRegistry::instance().registerBackend(std::move(cpuGraph));
        
        auto cpuGeo = std::make_unique<CPUGeoBackend>();
        BackendRegistry::instance().registerBackend(std::move(cpuGeo));
    }
    
    void TearDown() override {
        BackendRegistry::instance().shutdownAll();
    }
};

TEST_F(AccelerationTest, BackendRegistryBasics) {
    auto& registry = BackendRegistry::instance();
    
    // CPU backend should always be available
    auto* cpuBackend = registry.getBackend(BackendType::CPU);
    ASSERT_NE(cpuBackend, nullptr);
    EXPECT_TRUE(cpuBackend->isAvailable());
    EXPECT_STREQ(cpuBackend->name(), "CPU");
}

TEST_F(AccelerationTest, CPUVectorBackend) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->initialize());
    
    // Test data: 3 vectors in 2D space
    std::vector<float> vectors = {
        1.0f, 0.0f,  // Vector 0
        0.0f, 1.0f,  // Vector 1
        1.0f, 1.0f   // Vector 2
    };
    
    // Query vector
    std::vector<float> query = {0.5f, 0.5f};
    
    // Compute distances
    auto distances = backend->computeDistances(
        query.data(), 1, 2,
        vectors.data(), 3,
        true  // Use L2
    );
    
    ASSERT_EQ(distances.size(), 3);
    
    // Verify distances are reasonable
    for (float dist : distances) {
        EXPECT_GE(dist, 0.0f);
        EXPECT_LT(dist, 10.0f);
    }
    
    backend->shutdown();
}

TEST_F(AccelerationTest, CPUVectorKnnSearch) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->initialize());
    
    // Test data: 5 vectors in 3D space
    std::vector<float> vectors = {
        1.0f, 0.0f, 0.0f,  // Vector 0
        0.0f, 1.0f, 0.0f,  // Vector 1
        0.0f, 0.0f, 1.0f,  // Vector 2
        1.0f, 1.0f, 0.0f,  // Vector 3
        1.0f, 1.0f, 1.0f   // Vector 4
    };
    
    // Query: origin
    std::vector<float> query = {0.0f, 0.0f, 0.0f};
    
    // Find 3 nearest neighbors
    auto results = backend->batchKnnSearch(
        query.data(), 1, 3,
        vectors.data(), 5,
        3,  // k=3
        true  // Use L2
    );
    
    ASSERT_EQ(results.size(), 1);
    ASSERT_EQ(results[0].size(), 3);
    
    // Results should be sorted by distance (ascending)
    for (size_t i = 1; i < results[0].size(); ++i) {
        EXPECT_LE(results[0][i-1].second, results[0][i].second);
    }
    
    backend->shutdown();
}

TEST_F(AccelerationTest, CPUGeoBackend) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestGeoBackend();
    
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->initialize());
    
    // Test data: distances between cities
    std::vector<double> lats1 = {51.5074};  // London
    std::vector<double> lons1 = {-0.1278};
    std::vector<double> lats2 = {48.8566};  // Paris
    std::vector<double> lons2 = {2.3522};
    
    auto distances = backend->batchDistances(
        lats1.data(), lons1.data(),
        lats2.data(), lons2.data(),
        1, true  // Use Haversine
    );
    
    ASSERT_EQ(distances.size(), 1);
    
    // London-Paris distance should be around 340-350 km
    EXPECT_GT(distances[0], 300.0f);
    EXPECT_LT(distances[0], 400.0f);
    
    backend->shutdown();
}

TEST_F(AccelerationTest, CPUGeoPointInPolygon) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestGeoBackend();
    
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->initialize());
    
    // Simple square polygon: (0,0), (10,0), (10,10), (0,10)
    std::vector<double> polygon = {
        0.0, 0.0,
        10.0, 0.0,
        10.0, 10.0,
        0.0, 10.0
    };
    
    // Test points: inside, outside, on edge
    std::vector<double> pointLats = {5.0, 15.0, 0.0};
    std::vector<double> pointLons = {5.0, 5.0, 0.0};
    
    auto results = backend->batchPointInPolygon(
        pointLats.data(), pointLons.data(), 3,
        polygon.data(), 4
    );
    
    ASSERT_EQ(results.size(), 3);
    EXPECT_TRUE(results[0]);   // (5,5) is inside
    EXPECT_FALSE(results[1]);  // (15,5) is outside
    // Point on edge behavior is implementation-specific
    
    backend->shutdown();
}

TEST_F(AccelerationTest, BackendCapabilities) {
    auto& registry = BackendRegistry::instance();
    
    auto* vectorBackend = registry.getBestVectorBackend();
    ASSERT_NE(vectorBackend, nullptr);
    
    auto caps = vectorBackend->getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_FALSE(caps.deviceName.empty());
}

TEST_F(AccelerationTest, GracefulDegradation) {
    auto& registry = BackendRegistry::instance();
    
    // Try to get CUDA backend (will not be available in CPU-only builds)
    auto* cudaBackend = registry.getBackend(BackendType::CUDA);
    
    // Should fall back to best available (CPU)
    auto* bestBackend = registry.getBestVectorBackend();
    ASSERT_NE(bestBackend, nullptr);
    
    // Should work regardless of which backend is used
    EXPECT_TRUE(bestBackend->initialize());
    EXPECT_TRUE(bestBackend->isAvailable());
}

// Benchmark test (disabled by default)
TEST_F(AccelerationTest, DISABLED_VectorSearchBenchmark) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    
    ASSERT_NE(backend, nullptr);
    backend->initialize();
    
    const size_t numVectors = 10000;
    const size_t dim = 128;
    const size_t numQueries = 100;
    const size_t k = 10;
    
    // Generate random vectors
    std::vector<float> vectors(numVectors * dim);
    for (auto& v : vectors) {
        v = static_cast<float>(rand()) / RAND_MAX;
    }
    
    std::vector<float> queries(numQueries * dim);
    for (auto& q : queries) {
        q = static_cast<float>(rand()) / RAND_MAX;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto results = backend->batchKnnSearch(
        queries.data(), numQueries, dim,
        vectors.data(), numVectors,
        k, true
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Backend: " << backend->name() << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Queries/sec: " << (numQueries * 1000.0 / duration.count()) << std::endl;
    
    EXPECT_EQ(results.size(), numQueries);
    
    backend->shutdown();
}

// ============================================================================
// ANNKernelDispatch and Capability Tests
// ============================================================================

TEST_F(AccelerationTest, CPUDispatchTableFullyPopulated) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);

    auto dispatch = backend->populateANNDispatch();
    EXPECT_NE(dispatch.launchL2Distance,   nullptr);
    EXPECT_NE(dispatch.launchCosine,       nullptr);
    EXPECT_NE(dispatch.launchInnerProduct, nullptr);
    EXPECT_NE(dispatch.launchTopK,         nullptr);
}

TEST_F(AccelerationTest, CPUDispatchMetricSelector) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);

    auto dispatch = backend->populateANNDispatch();
    EXPECT_NE(dispatch.distanceLauncherFor(DistanceMetric::L2),            nullptr);
    EXPECT_NE(dispatch.distanceLauncherFor(DistanceMetric::COSINE),        nullptr);
    EXPECT_NE(dispatch.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), nullptr);
}

TEST_F(AccelerationTest, CPUCapabilitiesMetricBitmask) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);

    auto caps = backend->getCapabilities();
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST_F(AccelerationTest, CPUCapabilitiesPrecisionMode) {
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);

    auto caps = backend->getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST_F(AccelerationTest, CUDAVectorCapabilitiesMetricBitmask) {
    // CUDAVectorBackend::getCapabilities() is a pure compile-time/device-query
    // method that does NOT require initialize() to have been called.  It reports
    // capabilities based on compile-time flags and optional device queries
    // (falling back to "Not Available" when no GPU is present).
    CUDAVectorBackend cudaBackend;
    auto caps = cudaBackend.getCapabilities();

#ifdef THEMIS_ENABLE_CUDA
    EXPECT_NE(caps.supportedMetrics, 0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
#else
    // No CUDA compile support — caps are empty by design
    EXPECT_EQ(caps.supportedMetrics, 0u);
#endif
}

TEST_F(AccelerationTest, CUDAVectorDispatchTablePopulated) {
    // CUDAVectorBackend::populateANNDispatch() returns compile-time function
    // pointers and does NOT require initialize() to have been called.  This
    // separation is intentional: callers can inspect available kernels before
    // deciding whether to initialize the backend.
    CUDAVectorBackend cudaBackend;
    auto dispatch = cudaBackend.populateANNDispatch();

#ifdef THEMIS_ENABLE_CUDA
    EXPECT_NE(dispatch.launchL2Distance,   nullptr);
    EXPECT_NE(dispatch.launchCosine,       nullptr);
    EXPECT_NE(dispatch.launchInnerProduct, nullptr);
    EXPECT_NE(dispatch.launchTopK,         nullptr);
#else
    // Without CUDA, all slots remain null so BackendRegistry falls back to CPU
    EXPECT_EQ(dispatch.launchL2Distance,   nullptr);
    EXPECT_EQ(dispatch.launchCosine,       nullptr);
    EXPECT_EQ(dispatch.launchInnerProduct, nullptr);
    EXPECT_EQ(dispatch.launchTopK,         nullptr);
#endif
}

TEST_F(AccelerationTest, BatchKnnSearchKClampsToNumVectors) {
    // k larger than numVectors must not crash; results must have at most numVectors entries
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    std::vector<float> vectors = {
        1.0f, 0.0f,
        0.0f, 1.0f,
    };
    std::vector<float> query = {0.5f, 0.5f};

    auto results = backend->batchKnnSearch(
        query.data(), 1, 2,
        vectors.data(), 2,
        100,  // k > numVectors
        true);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_LE(results[0].size(), 2u);

    backend->shutdown();
}

// =============================================================================
// Strict input validation tests — CPU backends (no GPU required)
// These tests cover all three CPU backend types and verify that unsafe batches
// are rejected early with an appropriate error set.
// =============================================================================

// --- CPUVectorBackend ---------------------------------------------------------

TEST_F(AccelerationTest, CPUVector_ComputeDistances_NullQueriesReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float v[] = {1.f, 0.f};
    auto result = backend.computeDistances(nullptr, 1, 2, v, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUVector_ComputeDistances_NullVectorsReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float q[] = {1.f, 0.f};
    auto result = backend.computeDistances(q, 1, 2, nullptr, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUVector_ComputeDistances_ZeroQueriesReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.computeDistances(q, 0, 2, v, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUVector_ComputeDistances_ZeroDimReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.computeDistances(q, 1, 0, v, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUVector_BatchKnnSearch_NullQueriesReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(nullptr, 1, 2, v, 1, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUVector_BatchKnnSearch_ZeroKReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(q, 1, 2, v, 1, 0, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUVector_BatchKnnSearch_ZeroDimReturnsEmpty) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(q, 1, 0, v, 1, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

// --- CPUGeoBackend ------------------------------------------------------------

TEST_F(AccelerationTest, CPUGeo_BatchDistances_NullLatsReturnsEmpty) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());
    const double lons1[] = {0.0};
    const double lats2[] = {10.0};
    const double lons2[] = {10.0};
    auto result = backend.batchDistances(nullptr, lons1, lats2, lons2, 1, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUGeo_BatchDistances_ZeroCountReturnsEmpty) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());
    const double lats1[] = {0.0};
    const double lons1[] = {0.0};
    const double lats2[] = {10.0};
    const double lons2[] = {10.0};
    auto result = backend.batchDistances(lats1, lons1, lats2, lons2, 0, true);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUGeo_BatchPointInPolygon_NullPointsReturnsEmpty) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());
    const double polyCoords[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    const double lons[] = {0.5};
    auto result = backend.batchPointInPolygon(nullptr, lons, 1, polyCoords, 4);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUGeo_BatchPointInPolygon_TooFewVerticesReturnsEmpty) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());
    const double lats[] = {0.5};
    const double lons[] = {0.5};
    const double polyCoords[] = {0.0,0.0, 1.0,0.0}; // only 2 vertices
    auto result = backend.batchPointInPolygon(lats, lons, 1, polyCoords, 2);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

// --- CPUGraphBackend ---------------------------------------------------------

TEST_F(AccelerationTest, CPUGraph_BatchBFS_NullAdjacencyReturnsEmpty) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());
    const uint32_t starts[] = {0u};
    auto result = backend.batchBFS(nullptr, 4, starts, 1, 2);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUGraph_BatchBFS_ZeroNumStartsReturnsEmpty) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());
    const uint32_t adj[] = {0u};
    const uint32_t starts[] = {0u};
    auto result = backend.batchBFS(adj, 4, starts, 0, 2);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUGraph_BatchShortestPath_NullAdjacencyReturnsEmpty) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());
    const float weights[] = {1.f};
    const uint32_t starts[] = {0u};
    const uint32_t ends[] = {1u};
    auto result = backend.batchShortestPath(nullptr, weights, 4, starts, ends, 1);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST_F(AccelerationTest, CPUGraph_BatchShortestPath_ZeroNumPairsReturnsEmpty) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());
    const uint32_t adj[] = {0u};
    const float weights[] = {1.f};
    const uint32_t starts[] = {0u};
    const uint32_t ends[] = {1u};
    auto result = backend.batchShortestPath(adj, weights, 4, starts, ends, 0);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

// ============================================================================
// Deterministic Tie-Breaking Tests (Issue #1388)
// ============================================================================

TEST_F(AccelerationTest, TieBreakingDeterminism_LowerIndexWins) {
    // When two vectors are equidistant from the query, the one with the lower
    // vector index must appear first in the results (deterministic ordering).
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    // Three vectors all at the same distance (1.0 squared L2) from the origin query.
    // Indices 0, 1, and 2 all have squared L2 distance = 1.0 from (0,0).
    std::vector<float> vectors = {
        1.0f, 0.0f,  // index 0 — dist^2 = 1.0
        0.0f, 1.0f,  // index 1 — dist^2 = 1.0
        -1.0f, 0.0f, // index 2 — dist^2 = 1.0
    };
    std::vector<float> query = {0.0f, 0.0f};

    auto results = backend->batchKnnSearch(
        query.data(), 1, 2,
        vectors.data(), 3,
        3,  // k=3, all tie
        true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 3u);

    // All distances must be equal
    EXPECT_FLOAT_EQ(results[0][0].second, results[0][1].second);
    EXPECT_FLOAT_EQ(results[0][1].second, results[0][2].second);

    // Tie-breaking: indices must appear in ascending order (lower index first)
    EXPECT_LT(results[0][0].first, results[0][1].first);
    EXPECT_LT(results[0][1].first, results[0][2].first);

    backend->shutdown();
}

TEST_F(AccelerationTest, TieBreakingDeterminism_PartialTie) {
    // Mixed case: nearest neighbour is unique; second and third tie.
    // Confirms tie-breaking is applied only to equal-distance pairs.
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    std::vector<float> vectors = {
        0.1f, 0.0f,  // index 0 — dist^2 = 0.01 (closest, unique)
        1.0f, 0.0f,  // index 1 — dist^2 = 1.0  (tie for 2nd/3rd)
        0.0f, 1.0f,  // index 2 — dist^2 = 1.0  (tie for 2nd/3rd)
    };
    std::vector<float> query = {0.0f, 0.0f};

    auto results = backend->batchKnnSearch(
        query.data(), 1, 2,
        vectors.data(), 3,
        3,
        true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 3u);

    // First result: index 0 (closest)
    EXPECT_EQ(results[0][0].first, 0u);

    // Second and third share the same distance; lower index (1) must come before 2
    EXPECT_FLOAT_EQ(results[0][1].second, results[0][2].second);
    EXPECT_EQ(results[0][1].first, 1u);
    EXPECT_EQ(results[0][2].first, 2u);

    backend->shutdown();
}

// ============================================================================
// Partial Failure Handling Tests (Issue #1388)
// ============================================================================

TEST_F(AccelerationTest, PartialFailure_AllValid) {
    // When all query vectors are valid, all queries succeed.
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    std::vector<float> vectors = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> queries = {0.5f, 0.5f, 0.0f, 0.0f};

    auto result = backend->batchKnnSearchSafe(
        queries.data(), 2, 2,
        vectors.data(), 2,
        1, true);

    ASSERT_EQ(result.queryResults.size(), 2u);
    EXPECT_EQ(result.successCount, 2u);
    EXPECT_EQ(result.failureCount, 0u);

    for (size_t i = 0; i < 2; ++i) {
        EXPECT_EQ(result.queryResults[i].status, AccelerationErrorCode::Success);
        EXPECT_FALSE(result.queryResults[i].neighbors.empty());
    }

    backend->shutdown();
}

TEST_F(AccelerationTest, PartialFailure_NaNQueryFails) {
    // A query vector containing NaN must receive InputRangeViolation status
    // while the other valid query succeeds.
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    std::vector<float> vectors = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> queries = {
        std::numeric_limits<float>::quiet_NaN(), 0.5f,  // query 0: NaN → should fail
        0.0f, 0.0f                                       // query 1: valid
    };

    auto result = backend->batchKnnSearchSafe(
        queries.data(), 2, 2,
        vectors.data(), 2,
        1, true);

    ASSERT_EQ(result.queryResults.size(), 2u);
    EXPECT_EQ(result.successCount, 1u);
    EXPECT_EQ(result.failureCount, 1u);

    // Query 0 failed
    EXPECT_EQ(result.queryResults[0].status, AccelerationErrorCode::InputRangeViolation);
    EXPECT_TRUE(result.queryResults[0].neighbors.empty());

    // Query 1 succeeded
    EXPECT_EQ(result.queryResults[1].status, AccelerationErrorCode::Success);
    EXPECT_FALSE(result.queryResults[1].neighbors.empty());

    backend->shutdown();
}

TEST_F(AccelerationTest, PartialFailure_InfQueryFails) {
    // A query vector containing Inf must receive InputRangeViolation status.
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    std::vector<float> vectors = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> queries = {
        std::numeric_limits<float>::infinity(), 0.5f,  // query 0: Inf → should fail
        0.0f, 0.0f                                      // query 1: valid
    };

    auto result = backend->batchKnnSearchSafe(
        queries.data(), 2, 2,
        vectors.data(), 2,
        1, true);

    ASSERT_EQ(result.queryResults.size(), 2u);
    EXPECT_EQ(result.successCount, 1u);
    EXPECT_EQ(result.failureCount, 1u);

    EXPECT_EQ(result.queryResults[0].status, AccelerationErrorCode::InputRangeViolation);
    EXPECT_TRUE(result.queryResults[0].neighbors.empty());
    EXPECT_FALSE(result.queryResults[0].errorMessage.empty());

    EXPECT_EQ(result.queryResults[1].status, AccelerationErrorCode::Success);

    backend->shutdown();
}

TEST_F(AccelerationTest, PartialFailure_AllInvalidQueriesReturnZeroSuccess) {
    // When every query vector is invalid, successCount must be 0 and
    // failureCount must equal numQueries.
    auto& registry = BackendRegistry::instance();
    auto* backend = registry.getBestVectorBackend();
    ASSERT_NE(backend, nullptr);
    ASSERT_TRUE(backend->initialize());

    std::vector<float> vectors = {1.0f, 0.0f};
    const float kNaN = std::numeric_limits<float>::quiet_NaN();
    std::vector<float> queries = {kNaN, kNaN,   // query 0
                                   kNaN, kNaN};  // query 1

    auto result = backend->batchKnnSearchSafe(
        queries.data(), 2, 2,
        vectors.data(), 1,
        1, true);

    EXPECT_EQ(result.successCount, 0u);
    EXPECT_EQ(result.failureCount, 2u);

    for (size_t i = 0; i < 2; ++i) {
        EXPECT_EQ(result.queryResults[i].status, AccelerationErrorCode::InputRangeViolation);
        EXPECT_TRUE(result.queryResults[i].neighbors.empty());
    }

    backend->shutdown();
}

// ============================================================================
// CUDAGraphBackend — structural and validation tests
//
// These tests exercise CUDAGraphBackend without requiring GPU hardware.
// Hardware-dependent paths are skipped gracefully when no CUDA device is
// present (consistent with the rest of the CUDA test suite).
// ============================================================================

TEST(CUDAGraphBackendTest, IsAvailable_ReturnsCorrectType) {
    CUDAGraphBackend backend;
    EXPECT_EQ(backend.type(), BackendType::CUDA);
    EXPECT_STREQ(backend.name(), "CUDA");
    // isAvailable() returns true iff a CUDA device is present — just ensure
    // it does not crash regardless of hardware.
    (void)backend.isAvailable();
}

TEST(CUDAGraphBackendTest, GetCapabilities_SupportsGraphOps) {
#ifdef THEMIS_ENABLE_CUDA
    CUDAGraphBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGraphOps);
#else
    GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled";
#endif
}

TEST(CUDAGraphBackendTest, Initialize_FailsWithoutHardware) {
#ifdef THEMIS_ENABLE_CUDA
    CUDAGraphBackend backend = {};
    if (backend.isAvailable()) {
        GTEST_SKIP() << "capability:no_cuda_device_path_exercisable=false;reason=cuda_hardware_present";
    }
    EXPECT_FALSE(backend.initialize());
    EXPECT_FALSE(backend.getLastError().isSuccess());
#else
    GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled";
#endif
}

TEST(CUDAGraphBackendTest, BatchBFS_NullAdjacencyReturnsEmpty) {
    CUDAGraphBackend backend;
    const uint32_t starts[] = {0u};
    // Validation must reject null adjacency even before GPU dispatch
    auto result = backend.batchBFS(nullptr, 4, starts, 1, 2);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST(CUDAGraphBackendTest, BatchBFS_ZeroNumStartsReturnsEmpty) {
    CUDAGraphBackend backend;
    const uint32_t adj[] = {0u};
    const uint32_t starts[] = {0u};
    auto result = backend.batchBFS(adj, 4, starts, 0, 2);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST(CUDAGraphBackendTest, BatchBFS_ZeroNumVerticesReturnsEmpty) {
    CUDAGraphBackend backend;
    const uint32_t adj[] = {0u};
    const uint32_t starts[] = {0u};
    auto result = backend.batchBFS(adj, 0, starts, 1, 2);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST(CUDAGraphBackendTest, BatchShortestPath_NullAdjacencyReturnsEmpty) {
    CUDAGraphBackend backend;
    const float weights[] = {1.f};
    const uint32_t starts[] = {0u};
    const uint32_t ends[]   = {1u};
    auto result = backend.batchShortestPath(nullptr, weights, 4, starts, ends, 1);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST(CUDAGraphBackendTest, BatchShortestPath_NullWeightsReturnsEmpty) {
    CUDAGraphBackend backend;
    const uint32_t adj[] = {0u};
    const uint32_t starts[] = {0u};
    const uint32_t ends[]   = {1u};
    auto result = backend.batchShortestPath(adj, nullptr, 4, starts, ends, 1);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST(CUDAGraphBackendTest, BatchShortestPath_ZeroNumPairsReturnsEmpty) {
    CUDAGraphBackend backend;
    const uint32_t adj[] = {0u};
    const float weights[] = {1.f};
    const uint32_t starts[] = {0u};
    const uint32_t ends[]   = {1u};
    auto result = backend.batchShortestPath(adj, weights, 4, starts, ends, 0);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

TEST(CUDAGraphBackendTest, BatchBFS_WithHardware_StartVertexAlwaysVisited) {
    // If a CUDA GPU is available and the backend initialises successfully,
    // every start vertex must appear in the corresponding BFS result.
#ifdef THEMIS_ENABLE_CUDA
    CUDAGraphBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // 4-vertex graph: 0→1, 1→2, 2→3 (linear chain)
    // Represented as dense 4×4 adjacency matrix (row = src, col = dst)
    const size_t N = 4;
    uint32_t adj[N * N] = {};
    adj[0 * N + 1] = 1;  // 0 → 1
    adj[1 * N + 2] = 1;  // 1 → 2
    adj[2 * N + 3] = 1;  // 2 → 3

    const uint32_t starts[] = {0u, 2u};
    auto results = backend.batchBFS(adj, N, starts, 2, 3);

    ASSERT_EQ(results.size(), 2u);

    // BFS from vertex 0: should visit 0, 1, 2, 3 (within depth 3)
    EXPECT_FALSE(results[0].empty());
    EXPECT_NE(std::find(results[0].begin(), results[0].end(), 0u),
              results[0].end());  // start vertex always present

    // BFS from vertex 2: should visit 2, 3 (within depth 1)
    EXPECT_FALSE(results[1].empty());
    EXPECT_NE(std::find(results[1].begin(), results[1].end(), 2u),
              results[1].end());

    backend.shutdown();
#else
    GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled";
#endif
}

TEST(CUDAGraphBackendTest, BatchShortestPath_WithHardware_LinearChain) {
    // On a linear chain 0→1→2→3 with unit weights, the shortest path from
    // vertex 0 to vertex 3 must be [0, 1, 2, 3].
#ifdef THEMIS_ENABLE_CUDA
    CUDAGraphBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const size_t N = 4;
    uint32_t adj[N * N] = {};
    float wgt[N * N]    = {};
    adj[0 * N + 1] = 1;  wgt[0 * N + 1] = 1.0f;
    adj[1 * N + 2] = 1;  wgt[1 * N + 2] = 1.0f;
    adj[2 * N + 3] = 1;  wgt[2 * N + 3] = 1.0f;

    const uint32_t starts[] = {0u};
    const uint32_t ends[]   = {3u};
    auto results = backend.batchShortestPath(adj, wgt, N, starts, ends, 1);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 4u);
    EXPECT_EQ(results[0][0], 0u);
    EXPECT_EQ(results[0][1], 1u);
    EXPECT_EQ(results[0][2], 2u);
    EXPECT_EQ(results[0][3], 3u);

    backend.shutdown();
#else
    GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled";
#endif
}

TEST(CUDAGraphBackendTest, BatchBFS_WithHardware_GraphCaptureReusesSameResults) {
    // Calling batchBFS twice with the same shape must produce identical results
    // (graph cache replay must be functionally equivalent to first capture).
#ifdef THEMIS_ENABLE_CUDA
    CUDAGraphBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const size_t N = 3;
    uint32_t adj[N * N] = {};
    adj[0 * N + 1] = 1;
    adj[1 * N + 2] = 1;

    const uint32_t starts[] = {0u};

    auto results1 = backend.batchBFS(adj, N, starts, 1, 2);
    auto results2 = backend.batchBFS(adj, N, starts, 1, 2);

    ASSERT_EQ(results1.size(), results2.size());
    for (size_t s = 0; s < results1.size(); ++s) {
        EXPECT_EQ(results1[s], results2[s]);
    }

    backend.shutdown();
#else
    GTEST_SKIP() << "capability:cuda_compiled=false;reason=cuda_not_compiled";
#endif
}

// ---------------------------------------------------------------------------
// AiHardwareDispatcher::logCapabilities — minimum coverage
// (UNUSED_FUNCTIONS_REPORT KEEP → NUR_TESTS, Target v1.4.0)
// ---------------------------------------------------------------------------

// LC-01: logCapabilities() does not throw and returns successfully.
TEST(AiHardwareDispatcherTest, LC01_LogCapabilitiesDoesNotThrow) {
    EXPECT_NO_THROW({
        themis::acceleration::AiHardwareDispatcher::instance().logCapabilities();
    });
}

// LC-02: logCapabilities() can be called multiple times without crashing.
TEST(AiHardwareDispatcherTest, LC02_LogCapabilitiesIdempotent) {
    auto& disp = themis::acceleration::AiHardwareDispatcher::instance();
    EXPECT_NO_THROW({
        disp.logCapabilities();
        disp.logCapabilities();
    });
}

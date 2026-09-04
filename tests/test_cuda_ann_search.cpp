// Test: CUDA-Accelerated ANN Search
//
// Validates the CUDA-accelerated Approximate Nearest Neighbor (ANN) search
// implementation in CUDAVectorBackend.  All structural tests run on any
// platform (no GPU required); hardware-dependent paths are skipped gracefully
// when CUDA is unavailable.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/kernel_invocation.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#include <cmath>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// Structural / compile-time tests (no GPU required)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CudaAnnSearch, DispatchTableStructure_L2AndTopKSlotsDefined) {
    CUDAVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();
    // Under THEMIS_ENABLE_CUDA the L2, cosine, inner-product, and top-K slots
    // must all be populated; GPU hardware is not required to verify this.
    EXPECT_NE(d.launchL2Distance,   nullptr);
    EXPECT_NE(d.launchCosine,       nullptr);
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.launchTopK,         nullptr);
}

TEST(CudaAnnSearch, DispatchTableStructure_DistanceLauncherForRoutesAllMetrics) {
    CUDAVectorBackend backend;
    ANNKernelDispatch d = backend.populateANNDispatch();

    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::L2),            d.launchL2Distance);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::COSINE),        d.launchCosine);
    EXPECT_EQ(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), d.launchInnerProduct);
}

TEST(CudaAnnSearch, Capabilities_SupportedMetricsFlagsSet) {
    CUDAVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    // After fix the backend must advertise all three metrics so that
    // BackendRegistry::selectVectorBackendFor() can select it.
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(CudaAnnSearch, Capabilities_SupportedPrecisionsFP32Set) {
    CUDAVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(CudaAnnSearch, Capabilities_SupportsVectorAndBatch) {
    CUDAVectorBackend backend;
    BackendCapabilities caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
}

// =============================================================================
// Input-validation tests (no GPU required — validate guard paths)
// =============================================================================

TEST(CudaAnnSearch, ComputeDistances_NullQueryReturnsEmpty) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float vectors[] = {1.f, 0.f};
    auto result = backend.computeDistances(nullptr, 1, 2, vectors, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaAnnSearch, ComputeDistances_ZeroDimReturnsEmpty) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float q[] = {1.f};
    const float v[] = {1.f};
    auto result = backend.computeDistances(q, 1, 0, v, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_NullQueryReturnsEmpty) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float vectors[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(nullptr, 1, 2, vectors, 1, 1, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_ZeroKReturnsEmpty) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearch(q, 1, 2, v, 1, 0, true);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

// =============================================================================
// GPU-hardware-dependent end-to-end tests
// =============================================================================

TEST(CudaAnnSearch, ComputeDistances_L2_CorrectResults) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // Query [1,0] vs vectors [1,0] and [0,1]
    // Expected squared-L2: 0 and 2
    const float queries[]  = {1.f, 0.f};
    const float vectors[]  = {1.f, 0.f,  0.f, 1.f};

    auto dists = backend.computeDistances(queries, 1, 2, vectors, 2, /*useL2=*/true);

    ASSERT_EQ(dists.size(), 2u);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_NEAR(dists[1], 2.f, 1e-5f);

    backend.shutdown();
}

TEST(CudaAnnSearch, ComputeDistances_Cosine_SameVectorIsZeroDistance) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const float queries[]  = {1.f, 0.f};
    const float vectors[]  = {1.f, 0.f};

    auto dists = backend.computeDistances(queries, 1, 2, vectors, 1, /*useL2=*/false);

    ASSERT_EQ(dists.size(), 1u);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);  // cosine distance of identical vectors = 0

    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_L2_ReturnsCorrectTopK) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // 4 vectors; query = [1,0]; nearest should be idx 0 (dist 0), then idx 3 (dist 0.25)
    const float queries[] = {1.f, 0.f};
    const float vectors[] = {
        1.f, 0.f,   // idx 0: dist² = 0
        0.f, 1.f,   // idx 1: dist² = 2
        0.f, 0.f,   // idx 2: dist² = 1
        0.5f, 0.f,  // idx 3: dist² = 0.25
    };

    auto results = backend.batchKnnSearch(queries, 1, 2, vectors, 4, 2, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 2u);
    EXPECT_EQ(results[0][0].first, 0u);   // closest: idx 0
    EXPECT_NEAR(results[0][0].second, 0.f, 1e-5f);
    EXPECT_EQ(results[0][1].first, 3u);   // 2nd closest: idx 3
    EXPECT_NEAR(results[0][1].second, 0.25f, 1e-5f);

    backend.shutdown();
}

TEST(CudaAnnSearch, BatchKnnSearch_KLargerThanVectors_ClampsK) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,  0.f, 1.f};  // 2 vectors

    // k=5 but only 2 vectors — should return 2 results, not crash
    auto results = backend.batchKnnSearch(queries, 1, 2, vectors, 2, 5, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_LE(results[0].size(), 2u);

    backend.shutdown();
}

// =============================================================================
// Dispatch-table ANN search: inner-product kernel round-trip
// =============================================================================

TEST(CudaAnnSearch, ANNDispatch_InnerProduct_SlotIsNonNullOnGPU) {
    CUDAVectorBackend backend = {};
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    ANNKernelDispatch d = backend.populateANNDispatch();
    // Inner-product slot must be wired when CUDA hardware is present
    EXPECT_NE(d.launchInnerProduct, nullptr);
    EXPECT_NE(d.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), nullptr);

    backend.shutdown();
}

#endif // THEMIS_ENABLE_CUDA

// =============================================================================
// HNSW ANN wiring tests — no GPU required (CPU fallback in CudaHnswTraversalEngine)
// =============================================================================
//
// These tests validate that the HNSW traversal engine is correctly wired into
// CUDAVectorBackend via buildHnswAnnIndex() / annBatchSearch() / batchKnnSearch().
// They run in any environment because CudaHnswTraversalEngine transparently falls
// back to CPU when no CUDA device is available.

#include "acceleration/cuda_backend.h"
#include "index/cuda_hnsw_graph_traversal.h"

namespace {

/// Build a fully-connected CSR graph (every node is a neighbour of every other).
static themis::HnswLayerGraph makeTestFullGraph(uint32_t num_nodes) {
    themis::HnswLayerGraph g;
    g.num_nodes      = num_nodes;
    g.max_neighbours = num_nodes > 0 ? num_nodes - 1 : 0;
    g.offsets.resize(num_nodes + 1);
    g.offsets[0] = 0;
    for (uint32_t i = 0; i < num_nodes; ++i) {
        for (uint32_t j = 0; j < num_nodes; ++j) {
            if (j != i) {
              g.neighbours.push_back(static_cast<int32_t>(j));
            }
        }
        g.offsets[i + 1] = static_cast<int32_t>(g.neighbours.size());
    }
    return g;
}

} // anonymous namespace

TEST(CudaAnnHnswWiring, BuildHnswAnnIndex_ReturnsTrueOnValidData) {
    constexpr uint32_t N   = 6;
    constexpr uint32_t DIM = 4;

    themis::acceleration::CUDAVectorBackend backend;

    // Build a trivial 1-layer HNSW graph
    auto g = makeTestFullGraph(N);
    std::vector<float> vecs(N * DIM, 1.0f);

    EXPECT_FALSE(backend.isHnswIndexBuilt());  // not built yet
    bool ok = backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(backend.isHnswIndexBuilt());
}

TEST(CudaAnnHnswWiring, BuildHnswAnnIndex_ReturnsFalseOnNullVectors) {
    themis::acceleration::CUDAVectorBackend backend;
    auto g = makeTestFullGraph(4);
    bool ok = backend.buildHnswAnnIndex({g}, nullptr, 4, 2);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(backend.isHnswIndexBuilt());
}

TEST(CudaAnnHnswWiring, BuildHnswAnnIndex_ReturnsFalseOnZeroDim) {
    themis::acceleration::CUDAVectorBackend backend;
    auto g = makeTestFullGraph(4);
    std::vector<float> vecs(4 * 2, 1.0f);
    bool ok = backend.buildHnswAnnIndex({g}, vecs.data(), 4, 0u);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(backend.isHnswIndexBuilt());
}

TEST(CudaAnnHnswWiring, AnnBatchSearch_ReturnsEmptyBeforeIndexBuilt) {
    themis::acceleration::CUDAVectorBackend backend;
    const float q[] = {0.f, 0.f};
    auto results = backend.annBatchSearch(q, 1, 1);
    EXPECT_TRUE(results.empty());
}

TEST(CudaAnnHnswWiring, AnnBatchSearch_ReturnsKResultsAfterBuild) {
    constexpr uint32_t N   = 8;
    constexpr uint32_t DIM = 3;
    constexpr uint32_t K   = 3;

    themis::acceleration::CUDAVectorBackend backend;

    // Vectors: i-th vector = (i*0.1, i*0.1, i*0.1)
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i)
        for (uint32_t d = 0; d < DIM; ++d)
            vecs.push_back(static_cast<float>(i) * 0.1f);

    auto g = makeTestFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query(DIM, 0.0f);
    auto results = backend.annBatchSearch(query.data(), 1, K);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), K);
}

TEST(CudaAnnHnswWiring, AnnBatchSearch_ResultsSortedAscendingByScore) {
    constexpr uint32_t N   = 6;
    constexpr uint32_t DIM = 2;

    themis::acceleration::CUDAVectorBackend backend;

    // Vectors at distances 0, 1, 2, 3, 4, 5 from origin (along x-axis)
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i));
        vecs.push_back(0.0f);
    }

    auto g = makeTestFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query(DIM, 0.0f);
    auto results = backend.annBatchSearch(query.data(), 1, N);
    ASSERT_EQ(results.size(), 1u);
    ASSERT_FALSE(results[0].empty());

    for (size_t i = 1; i < results[0].size(); ++i) {
        EXPECT_LE(results[0][i - 1].second, results[0][i].second)
            << "Results not sorted ascending at index " << i;
    }
}

TEST(CudaAnnHnswWiring, AnnBatchSearch_NearestNeighbourIsOriginForOriginQuery) {
    constexpr uint32_t N   = 5;
    constexpr uint32_t DIM = 2;

    themis::acceleration::CUDAVectorBackend backend;

    // Vector 0 = (0,0), vector 1 = (1,0), ...
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i));
        vecs.push_back(0.0f);
    }

    auto g = makeTestFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.0f, 0.0f};
    auto results = backend.annBatchSearch(query.data(), 1, 1);
    ASSERT_EQ(results.size(), 1u);
    ASSERT_FALSE(results[0].empty());
    EXPECT_EQ(results[0][0].first, 0u);         // nearest is vector 0
    EXPECT_NEAR(results[0][0].second, 0.0f, 1e-5f);
}

TEST(CudaAnnHnswWiring, BatchKnnSearch_UsesHnswWhenIndexBuilt) {
    // Verify that batchKnnSearch() delegates to HNSW when an index is pre-built.
    // We use a query = origin and expect vector 0 (at origin) to be the top hit.
    constexpr uint32_t N   = 5;
    constexpr uint32_t DIM = 2;

    themis::acceleration::CUDAVectorBackend backend;

    // Vector 0 = (0,0), vector 1 = (1,0), ...
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i));
        vecs.push_back(0.0f);
    }

    auto g = makeTestFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    // batchKnnSearch should detect the built index and use HNSW traversal
    std::vector<float> query = {0.0f, 0.0f};
    auto results = backend.batchKnnSearch(
        query.data(), 1, DIM,
        vecs.data(),  N,
        1, /*useL2=*/true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_FALSE(results[0].empty());
    EXPECT_EQ(results[0][0].first, 0u);
    EXPECT_NEAR(results[0][0].second, 0.0f, 1e-5f);
}

TEST(CudaAnnHnswWiring, AnnBatchSearch_MultipleQueries) {
    constexpr uint32_t N   = 8;
    constexpr uint32_t DIM = 4;
    constexpr uint32_t K   = 2;
    constexpr uint32_t NQ  = 3;

    themis::acceleration::CUDAVectorBackend backend;

    std::vector<float> vecs(N * DIM, 1.0f);
    auto g = makeTestFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> queries(NQ * DIM, 0.5f);
    auto results = backend.annBatchSearch(queries.data(), NQ, K);
    ASSERT_EQ(results.size(), NQ);
    for (const auto& r : results) {
        EXPECT_EQ(r.size(), K);
    }
}

// =============================================================================
// CPU-backend parity checks (always run, no GPU required)
// =============================================================================

TEST(CudaAnnSearch, CpuParity_L2DistancesMatch) {
    // Verify CPU backend dispatch produces expected squared-L2 distances.
    // This serves as a reference baseline against which the CUDA results are compared.
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchL2Distance, nullptr);

    const float queries[] = {1.f, 0.f, 0.f};
    const float vectors[] = {1.f, 0.f, 0.f,  0.f, 1.f, 0.f};
    float dists[2] = {};

    const int rc = d.launchL2Distance(queries, vectors, dists,
                                      1, 2, 3, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_NEAR(dists[1], 2.f, 1e-5f);

    backend.shutdown();
}

TEST(CudaAnnSearch, CpuParity_InnerProductDispatch_NegativeDot) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());

    ANNKernelDispatch d = backend.populateANNDispatch();
    ASSERT_NE(d.launchInnerProduct, nullptr);

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,  0.f, 1.f};
    float dists[2] = {};

    const int rc = d.launchInnerProduct(queries, vectors, dists,
                                        1, 2, 2, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dists[0], -1.f, 1e-5f);  // -dot([1,0],[1,0]) = -1
    EXPECT_NEAR(dists[1],  0.f, 1e-5f);  // -dot([1,0],[0,1]) =  0

    backend.shutdown();
}

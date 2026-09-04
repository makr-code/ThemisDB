// test_cuda_hnsw_large_k.cpp
//
// Tests for CUDA HNSW large-k search (k=257, k=512, k=1024).
//
// Acceptance criteria (Issue #132, v1.8.0):
//   - k > kMaxK no longer silently truncates results.
//   - k=257, k=512, k=1024 each return exactly the requested number of results.
//   - The multi-pass host-merge strategy is exercised for k > 1024.
//   - BackendHealthStatus::makeDegraded() is surfaced in release builds when
//     k > kHnswSinglePassMaxK (1024).
//
// All tests run on any platform.  CudaHnswTraversalEngine transparently falls
// back to the CPU path when no CUDA device is available, so no GPU is required.

#include <gtest/gtest.h>
#include "acceleration/cuda_backend.h"
#include "index/cuda_hnsw_graph_traversal.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace themis::acceleration;

namespace {

/// Build a fully-connected CSR graph where every node is a neighbour of every
/// other (maximum recall for any k ≤ N).
static themis::HnswLayerGraph makeFullGraph(uint32_t num_nodes) {
    themis::HnswLayerGraph g;
    g.num_nodes      = num_nodes;
    g.max_neighbours = num_nodes > 0u ? num_nodes - 1u : 0u;
    g.offsets.resize(num_nodes + 1u);
    g.offsets[0] = 0;
    for (uint32_t i = 0; i < num_nodes; ++i) {
        for (uint32_t j = 0; j < num_nodes; ++j) {
            if (j != i) {
              g.neighbours.push_back(static_cast<int32_t>(j));
            }
        }
        g.offsets[i + 1u] = static_cast<int32_t>(g.neighbours.size());
    }
    return g;
}

/// Return a set of N distinct 1-D vectors equally spaced in [0, 1].
static std::vector<float> makeLinearVectors(uint32_t n) {
    std::vector<float> v(n);
    for (uint32_t i = 0; i < n; ++i) {
        v[i] = static_cast<float>(i) / static_cast<float>(n > 1u ? n - 1u : 1u);
    }
    return v;
}

}  // namespace

// =============================================================================
// k = 257  — first value that previously exceeded the old kMaxK=256 limit
// =============================================================================

TEST(CudaHnswLargeK, K257_ResultCountEqualsRequestedK) {
    constexpr uint32_t N   = 512u;  // enough nodes to satisfy k=257
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 257u;

    CUDAVectorBackend backend;
    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.5f};
    auto results = backend.annBatchSearch(query.data(), 1u, K);

    ASSERT_EQ(results.size(), 1u)
        << "annBatchSearch must return one result row per query";
    EXPECT_EQ(results[0].size(), static_cast<size_t>(K))
        << "k=257 must yield exactly 257 results (no silent truncation)";
}

// =============================================================================
// k = 512  — former kMaxK boundary (previously the ceiling)
// =============================================================================

TEST(CudaHnswLargeK, K512_ResultCountEqualsRequestedK) {
    constexpr uint32_t N   = 600u;
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 512u;

    CUDAVectorBackend backend;
    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.0f};
    auto results = backend.annBatchSearch(query.data(), 1u, K);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), static_cast<size_t>(K))
        << "k=512 must yield exactly 512 results";
}

// =============================================================================
// k = 1024  — new kMaxK (single-pass upper boundary)
// =============================================================================

TEST(CudaHnswLargeK, K1024_ResultCountEqualsRequestedK) {
    constexpr uint32_t N   = 1200u;
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 1024u;

    CUDAVectorBackend backend;
    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.0f};
    auto results = backend.annBatchSearch(query.data(), 1u, K);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), static_cast<size_t>(K))
        << "k=1024 must yield exactly 1024 results (single-pass boundary)";
}

// =============================================================================
// k > 1024  — multi-pass strategy territory
// =============================================================================

TEST(CudaHnswLargeK, K1025_MultiPassReturnsRequestedK) {
    constexpr uint32_t N   = 1500u;
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 1025u;

    CUDAVectorBackend backend;
    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.0f};
    auto results = backend.annBatchSearch(query.data(), 1u, K);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), static_cast<size_t>(K))
        << "k=1025 must yield exactly 1025 results via multi-pass strategy";
}

// =============================================================================
// k > 1024 — BackendHealthStatus should be degraded in release builds
// =============================================================================

TEST(CudaHnswLargeK, K1025_HealthStatusDegradedInReleaseBuild) {
    constexpr uint32_t N   = 1500u;
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 1025u;

    CUDAVectorBackend backend;
    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.0f};
    /* result is intentionally unused here; we care only about health status */
    backend.annBatchSearch(query.data(), 1u, K);

#if defined(NDEBUG)
    // Release: multi-pass degrades the backend health status
    auto health = backend.getHealthStatus();
    EXPECT_NE(health.status, "healthy")
        << "k > 1024 in release builds should report a degraded or error status";
#else
    // Debug: kernel __trap() prevents reaching multi-pass, so no health check
    // is meaningful here.  Just skip.
    GTEST_SKIP() << "Debug build: k > 1024 is caught by __trap() in kernel";
#endif
}

// =============================================================================
// Result ordering: top result should be the closest for k=512
// =============================================================================

TEST(CudaHnswLargeK, K512_ResultsSortedByScore) {
    constexpr uint32_t N   = 600u;
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 512u;

    CUDAVectorBackend backend;
    // Vectors at positions 0, 1, 2, ..., N-1
    std::vector<float> vecs(N);
    for (uint32_t i = 0; i < N; ++i) {
      vecs[i] = static_cast<float>(i);
    }

    auto g = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    // Query from origin; expect results sorted ascending by distance
    std::vector<float> query = {0.0f};
    auto results = backend.annBatchSearch(query.data(), 1u, K);
    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), static_cast<size_t>(K));

    for (size_t i = 1u; i < results[0].size(); ++i) {
        EXPECT_LE(results[0][i - 1u].second, results[0][i].second)
            << "Results must be sorted ascending by score at index " << i;
    }
}

// =============================================================================
// Multiple queries with k=257 — each row must have exactly K results
// =============================================================================

TEST(CudaHnswLargeK, K257_MultipleQueriesAllReturnK) {
    constexpr uint32_t N   = 512u;
    constexpr uint32_t DIM = 2u;
    constexpr uint32_t K   = 257u;
    constexpr uint32_t NQ  = 4u;

    CUDAVectorBackend backend;
    std::vector<float> vecs;
    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i) * 0.01f);
        vecs.push_back(static_cast<float>(i) * 0.01f);
    }

    auto g = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> queries(NQ * DIM);
    for (uint32_t qi = 0; qi < NQ; ++qi) {
        queries[qi * DIM + 0] = static_cast<float>(qi) * 0.5f;
        queries[qi * DIM + 1] = 0.0f;
    }

    auto results = backend.annBatchSearch(queries.data(), NQ, K);
    ASSERT_EQ(results.size(), static_cast<size_t>(NQ));
    for (uint32_t qi = 0; qi < NQ; ++qi) {
        EXPECT_EQ(results[qi].size(), static_cast<size_t>(K))
            << "Query " << qi << " must return exactly k=" << K << " results";
    }
}

// =============================================================================
// CUDAVectorBackend::setMaxBatchSize / pool tuning API
// =============================================================================

TEST(CudaHnswLargeK, SetMaxBatchSizeDefaultIs512) {
    CUDAVectorBackend backend;
    EXPECT_EQ(backend.maxBatchSize(), 512u);
}

TEST(CudaHnswLargeK, SetMaxBatchSizeUpdatesValue) {
    CUDAVectorBackend backend;
    backend.setMaxBatchSize(128);
    EXPECT_EQ(backend.maxBatchSize(), 128u);
}

TEST(CudaHnswLargeK, SetMaxBatchSizeZeroClampedToOne) {
    CUDAVectorBackend backend;
    backend.setMaxBatchSize(0);
    EXPECT_GE(backend.maxBatchSize(), 1u);
}

TEST(CudaHnswLargeK, SetMaxBatchSizeBeforeBuildPropagated) {
    // setMaxBatchSize() called before buildHnswAnnIndex() is the recommended
    // pattern; the value must be honoured by the engine created inside build.
    constexpr uint32_t N   = 32u;
    constexpr uint32_t DIM = 1u;
    constexpr uint32_t K   = 1u;

    CUDAVectorBackend backend;
    backend.setMaxBatchSize(16);

    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    // Search must still work after setting batch size
    std::vector<float> query = {0.5f};
    auto results = backend.annBatchSearch(query.data(), 1u, K);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), static_cast<size_t>(K));
}

// =============================================================================
// Pool-based visited array: degraded health status when pool allocation fails
// =============================================================================

TEST(CudaHnswLargeK, HealthStatusDegradedWhenPoolFails) {
    // Simulate pool allocation failure by requesting an absurdly large
    // batch size so that even in a non-CUDA build the engine cannot satisfy
    // the pool request via CUDA malloc.  On CPU-only builds the pool is never
    // allocated (THEMIS_ENABLE_CUDA absent), so hasVisitedPool() is false and
    // the backend sets a degraded error after buildHnswAnnIndex().
    //
    // We test the health-status branch by checking that after a successful
    // build with a very large (and therefore failing) pool request, the
    // backend's health status is NOT "healthy" in a release build, or that
    // the build still succeeds (CPU fallback remains operational).
    constexpr uint32_t N   = 8u;
    constexpr uint32_t DIM = 1u;

    CUDAVectorBackend backend;
    // Request a pool so large it cannot be satisfied on any real device
    // (2^60 queries × 1 byte ≈ 1 EiB).  Pool allocation will fail, backend
    // stays operational in CPU-fallback mode.
    backend.setMaxBatchSize(static_cast<size_t>(1) << 60);

    auto vecs = makeLinearVectors(N);
    auto g    = makeFullGraph(N);
    // Build must not crash and CPU search must still produce results
    ASSERT_TRUE(backend.buildHnswAnnIndex({g}, vecs.data(), N, DIM));

    std::vector<float> query = {0.5f};
    auto results = backend.annBatchSearch(query.data(), 1u, 1u);
    ASSERT_EQ(results.size(), 1u)
        << "Search must succeed even when visited pool allocation fails";
    EXPECT_EQ(results[0].size(), 1u);

#if defined(NDEBUG)
    // In release builds the pool failure is surfaced via the health status
    auto health = backend.getHealthStatus();
    // After a successful build with a failed pool, status should be degraded
    // (not healthy) because setError() was called for the alloc failure.
    EXPECT_NE(health.status, "healthy")
        << "Backend must be degraded when visited pool allocation failed";
#endif
}


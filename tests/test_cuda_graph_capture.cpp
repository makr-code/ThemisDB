// Test: CUDA Graph Capture for Recurring Query Workloads
//
// Validates the CUDAGraphCache implementation and the CUDA-graph-accelerated
// batchKnnSearchWithGraph() path on CUDAVectorBackend.
//
// Test organization:
//   1. Structural / compile-time tests — QueryShape and CUDAGraphCache API.
//      These tests compile only when THEMIS_ENABLE_CUDA is defined but do NOT
//      require GPU hardware.
//   2. Hardware tests — end-to-end graph capture and replay.
//      These tests are skipped gracefully when no CUDA GPU is present.
//   3. CPU parity — batchKnnSearchWithGraph results must match batchKnnSearch.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#include <vector>
#include <cmath>

using namespace themis::acceleration;

// =============================================================================
// QueryShape — equality and hash correctness (THEMIS_ENABLE_CUDA required)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CudaGraphCapture, QueryShape_EqualityReflexive) {
    QueryShape s{1, 100, 128, 10, DistanceMetric::L2};
    EXPECT_TRUE(s == s);
}

TEST(CudaGraphCapture, QueryShape_EqualitySymmetric) {
    QueryShape a{1, 100, 128, 10, DistanceMetric::L2};
    QueryShape b{1, 100, 128, 10, DistanceMetric::L2};
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
}

TEST(CudaGraphCapture, QueryShape_InequalityOnEachField) {
    QueryShape base{2, 50, 64, 5, DistanceMetric::COSINE};

    EXPECT_FALSE((QueryShape{3, 50, 64, 5, DistanceMetric::COSINE}) == base); // numQueries
    EXPECT_FALSE((QueryShape{2, 99, 64, 5, DistanceMetric::COSINE}) == base); // numVectors
    EXPECT_FALSE((QueryShape{2, 50, 32, 5, DistanceMetric::COSINE}) == base); // dim
    EXPECT_FALSE((QueryShape{2, 50, 64, 7, DistanceMetric::COSINE}) == base); // topK
    EXPECT_FALSE((QueryShape{2, 50, 64, 5, DistanceMetric::L2})     == base); // metric
}

TEST(CudaGraphCapture, QueryShapeHash_EqualShapesHaveSameHash) {
    QueryShapeHash hasher;
    QueryShape a{4, 200, 256, 20, DistanceMetric::INNER_PRODUCT};
    QueryShape b{4, 200, 256, 20, DistanceMetric::INNER_PRODUCT};
    EXPECT_EQ(hasher(a), hasher(b));
}

// =============================================================================
// CUDAGraphCache — structural tests (no GPU hardware required)
// =============================================================================

TEST(CudaGraphCapture, GraphCache_EmptyOnConstruction) {
    CUDAGraphCache cache;
    EXPECT_EQ(cache.size(), 0u);
}

TEST(CudaGraphCapture, GraphCache_GetOnEmptyCacheReturnsNull) {
    CUDAGraphCache cache;
    QueryShape s{1, 10, 4, 2, DistanceMetric::L2};
    EXPECT_EQ(cache.get(s), nullptr);
}

TEST(CudaGraphCapture, GraphCache_PutThenGetReturnsEntry) {
    CUDAGraphCache cache;
    QueryShape s{1, 10, 4, 2, DistanceMetric::L2};

    CUDAGraphEntry entry;
    // graph and exec remain null (no GPU call needed for this test)
    cache.put(s, std::move(entry));

    EXPECT_EQ(cache.size(), 1u);
    EXPECT_NE(cache.get(s), nullptr);
}

TEST(CudaGraphCapture, GraphCache_GetMissOnDifferentShape) {
    CUDAGraphCache cache;
    QueryShape s1{1, 10, 4, 2, DistanceMetric::L2};
    QueryShape s2{1, 10, 4, 2, DistanceMetric::COSINE}; // only metric differs

    CUDAGraphEntry entry;
    cache.put(s1, std::move(entry));

    EXPECT_NE(cache.get(s1), nullptr);
    EXPECT_EQ(cache.get(s2), nullptr);
}

TEST(CudaGraphCapture, GraphCache_LRUEvictsWhenFull) {
    CUDAGraphCache cache;
    const size_t maxEntries = CUDAGraphCache::kMaxEntries;

    // Fill the cache to capacity with distinct shapes
    for (size_t i = 0; i < maxEntries; ++i) {
        QueryShape s{static_cast<int>(i + 1), 10, 4, 2, DistanceMetric::L2};
        CUDAGraphEntry e;
        cache.put(s, std::move(e));
    }
    ASSERT_EQ(cache.size(), maxEntries);

    // Access entry with numQueries==2 so it becomes the most-recently-used
    QueryShape mru{2, 10, 4, 2, DistanceMetric::L2};
    ASSERT_NE(cache.get(mru), nullptr); // updates lastAccess

    // Adding one more entry should evict the LRU (numQueries==1, which was
    // inserted first and never accessed after)
    QueryShape newShape{static_cast<int>(maxEntries + 1), 10, 4, 2, DistanceMetric::L2};
    CUDAGraphEntry extra;
    cache.put(newShape, std::move(extra));

    EXPECT_EQ(cache.size(), maxEntries); // size unchanged after eviction
    EXPECT_NE(cache.get(mru), nullptr);  // MRU entry survived
}

TEST(CudaGraphCapture, GraphCache_ClearEmptiesCache) {
    CUDAGraphCache cache;
    for (int i = 0; i < 3; ++i) {
        QueryShape s{i + 1, 10, 4, 2, DistanceMetric::L2};
        CUDAGraphEntry e;
        cache.put(s, std::move(e));
    }
    ASSERT_EQ(cache.size(), 3u);

    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
}

TEST(CudaGraphCapture, GraphCache_ReplaceExistingEntry) {
    CUDAGraphCache cache;
    QueryShape s{1, 10, 4, 2, DistanceMetric::L2};

    CUDAGraphEntry e1;
    cache.put(s, std::move(e1));
    ASSERT_EQ(cache.size(), 1u);

    CUDAGraphEntry e2;
    cache.put(s, std::move(e2)); // same shape — should replace, not grow
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_NE(cache.get(s), nullptr);
}

// =============================================================================
// Hardware tests — skipped when no CUDA GPU is present
// =============================================================================

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_MatchesBatchKnnSearch_L2) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    // Query: [1, 0, 0]  Vectors: unit basis
    const float queries[] = {1.f, 0.f, 0.f};
    const float vectors[] = {
        1.f, 0.f, 0.f,  // idx 0 — identical to query (dist² = 0)
        0.f, 1.f, 0.f,  // idx 1 — dist² = 2
        0.f, 0.f, 1.f,  // idx 2 — dist² = 2
    };

    auto refResults = backend.batchKnnSearch(
        queries, 1, 3, vectors, 3, 2, /*useL2=*/true);

    auto graphResults = backend.batchKnnSearchWithGraph(
        queries, 1, 3, vectors, 3, 2, DistanceMetric::L2);

    ASSERT_EQ(refResults.size(),  graphResults.size());
    ASSERT_EQ(refResults[0].size(), graphResults[0].size());

    for (size_t i = 0; i < refResults[0].size(); ++i) {
        EXPECT_EQ(refResults[0][i].first,  graphResults[0][i].first)
            << "Index mismatch at position " << i;
        EXPECT_NEAR(refResults[0][i].second, graphResults[0][i].second, 1e-5f)
            << "Distance mismatch at position " << i;
    }

    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_MatchesBatchKnnSearch_Cosine) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {
        1.f, 0.f,       // idx 0 — cosine dist = 0 (same direction as query)
        0.f, 1.f,       // idx 1 — cosine dist = 1 (orthogonal)
        0.707f, 0.707f, // idx 2 — cosine dist ≈ 0.293 (≈45° from query; nearly unit-length)
    };

    auto refResults = backend.batchKnnSearch(
        queries, 1, 2, vectors, 3, 2, /*useL2=*/false);

    auto graphResults = backend.batchKnnSearchWithGraph(
        queries, 1, 2, vectors, 3, 2, DistanceMetric::COSINE);

    ASSERT_EQ(refResults.size(),  graphResults.size());
    ASSERT_EQ(refResults[0].size(), graphResults[0].size());

    for (size_t i = 0; i < refResults[0].size(); ++i) {
        EXPECT_EQ(refResults[0][i].first,  graphResults[0][i].first)
            << "Index mismatch at position " << i;
        EXPECT_NEAR(refResults[0][i].second, graphResults[0][i].second, 1e-4f)
            << "Distance mismatch at position " << i;
    }

    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_GraphCachedOnSecondCall) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,  0.f, 1.f,  0.5f, 0.5f};

    // First call — populates cache
    ASSERT_EQ(backend.graphCache().size(), 0u);
    auto r1 = backend.batchKnnSearchWithGraph(
        queries, 1, 2, vectors, 3, 2, DistanceMetric::L2);
    EXPECT_FALSE(r1.empty());
    EXPECT_EQ(backend.graphCache().size(), 1u);

    // Second call — should hit the cache (size stays at 1)
    auto r2 = backend.batchKnnSearchWithGraph(
        queries, 1, 2, vectors, 3, 2, DistanceMetric::L2);
    EXPECT_FALSE(r2.empty());
    EXPECT_EQ(backend.graphCache().size(), 1u); // no new entry

    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_DifferentShapeAddsEntry) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const float q1[] = {1.f, 0.f};
    const float v1[] = {1.f, 0.f,  0.f, 1.f};
    backend.batchKnnSearchWithGraph(q1, 1, 2, v1, 2, 1, DistanceMetric::L2);
    ASSERT_EQ(backend.graphCache().size(), 1u);

    // Same data but k=2 → different shape
    backend.batchKnnSearchWithGraph(q1, 1, 2, v1, 2, 2, DistanceMetric::L2);
    EXPECT_EQ(backend.graphCache().size(), 2u);

    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_NullQueryReturnsEmpty) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float vectors[] = {1.f, 0.f};
    auto result = backend.batchKnnSearchWithGraph(
        nullptr, 1, 2, vectors, 1, 1, DistanceMetric::L2);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_ZeroDimReturnsEmpty) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float data[] = {1.f};
    auto result = backend.batchKnnSearchWithGraph(
        data, 1, 0, data, 1, 1, DistanceMetric::L2);
    EXPECT_TRUE(result.empty());
    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_KLargerThanVectorsClamped) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }
    const float queries[] = {1.f, 0.f};
    const float vectors[] = {1.f, 0.f,  0.f, 1.f}; // 2 vectors

    auto result = backend.batchKnnSearchWithGraph(
        queries, 1, 2, vectors, 2, 10, DistanceMetric::L2); // k=10 > numVectors=2

    ASSERT_EQ(result.size(), 1u);
    EXPECT_LE(result[0].size(), 2u); // clamped to numVectors

    backend.shutdown();
}

TEST(CudaGraphCapture, BatchKnnSearchWithGraph_ReplayProducesSameResultAsFirstCall) {
    CUDAVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_hardware_not_available";
    }

    const float queries[] = {0.5f, 0.5f};
    const float vectors[] = {
        1.f, 0.f,
        0.f, 1.f,
        0.5f, 0.5f,
        0.1f, 0.9f,
    };

    auto r1 = backend.batchKnnSearchWithGraph(
        queries, 1, 2, vectors, 4, 2, DistanceMetric::L2);
    // Second call replays the graph with the same data → must produce identical results
    auto r2 = backend.batchKnnSearchWithGraph(
        queries, 1, 2, vectors, 4, 2, DistanceMetric::L2);

    ASSERT_EQ(r1.size(), r2.size());
    ASSERT_EQ(r1[0].size(), r2[0].size());
    for (size_t i = 0; i < r1[0].size(); ++i) {
        EXPECT_EQ(r1[0][i].first, r2[0][i].first);
        EXPECT_NEAR(r1[0][i].second, r2[0][i].second, 1e-6f);
    }

    backend.shutdown();
}

// =============================================================================
// Backend-state guard — no GPU hardware required
// =============================================================================

// Calling batchKnnSearchWithGraph on an uninitialized backend must return an
// empty result without crashing or invoking any CUDA runtime API.  This test
// runs without GPU hardware whenever THEMIS_ENABLE_CUDA is defined.
TEST(CudaGraphCapture, BatchKnnSearchWithGraph_NotInitializedReturnsEmpty) {
    CUDAVectorBackend backend; // constructed but NOT initialized
    const float data[] = {1.f, 0.f};
    auto result = backend.batchKnnSearchWithGraph(
        data, 1, 2, data, 1, 1, DistanceMetric::L2);
    EXPECT_TRUE(result.empty());
    // Last error should be set (not Success) — BackendNotInitialized
    EXPECT_FALSE(backend.getLastError().isSuccess());
}

#endif // THEMIS_ENABLE_CUDA

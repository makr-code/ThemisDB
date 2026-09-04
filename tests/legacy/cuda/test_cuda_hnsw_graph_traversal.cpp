#include <gtest/gtest.h>
#include "index/cuda_hnsw_graph_traversal.h"
#include <cmath>
#include <numeric>

namespace themis {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<float> makeVector(uint32_t dim, float fill) {
    return std::vector<float>(dim, fill);
}

/// Build a trivial CSR graph where every node is connected to every other.
static HnswLayerGraph makeFullGraph(uint32_t num_nodes) {
    HnswLayerGraph g;
    g.num_nodes      = num_nodes;
    g.max_neighbours = num_nodes - 1;
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

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(CudaHnswEngine, DefaultConstruction) {
    CudaHnswConfig cfg;
    cfg.dim = 4;
    CudaHnswTraversalEngine engine(cfg);
    EXPECT_FALSE(engine.isBuilt());
    // deviceInfo() should return a readable string even before build
    EXPECT_FALSE(engine.deviceInfo().empty());
}

TEST(CudaHnswEngine, InvalidDeviceIdThrows) {
    // Device 9999 should not exist on any test machine
    // → constructor should not throw but isCudaAvailable() should be false
    CudaHnswConfig cfg;
    cfg.device_id = 9999;
    cfg.dim       = 4;
    // Constructor should NOT throw; it gracefully falls back to CPU
    EXPECT_NO_THROW({
        CudaHnswTraversalEngine engine(cfg);
        EXPECT_FALSE(engine.isCudaAvailable());
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// buildIndex
// ─────────────────────────────────────────────────────────────────────────────

TEST(CudaHnswEngine, BuildIndexWithValidData) {
    constexpr uint32_t N   = 5;
    constexpr uint32_t DIM = 4;

    CudaHnswConfig cfg;
    cfg.dim = DIM;
    CudaHnswTraversalEngine engine(cfg);

    // 5 vectors in R^4
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i)
        for (uint32_t d = 0; d < DIM; ++d)
            vecs.push_back(static_cast<float>(i));

    auto g = makeFullGraph(N);
    bool ok = engine.buildIndex({g}, vecs.data(), N);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(engine.isBuilt());
}

TEST(CudaHnswEngine, BuildWithNullVectorsReturnsFalse) {
    CudaHnswConfig cfg;
    cfg.dim = 4;
    CudaHnswTraversalEngine engine(cfg);
    auto g = makeFullGraph(3);
    bool ok = engine.buildIndex({g}, nullptr, 3);
    EXPECT_FALSE(ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// search (CPU fallback guaranteed without GPU)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CudaHnswEngine, SearchReturnsKResults) {
    constexpr uint32_t N   = 8;
    constexpr uint32_t DIM = 3;
    constexpr uint32_t K   = 3;

    CudaHnswConfig cfg;
    cfg.dim       = DIM;
    cfg.ef_search = 8;
    CudaHnswTraversalEngine engine(cfg);

    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i)
        for (uint32_t d = 0; d < DIM; ++d)
            vecs.push_back(static_cast<float>(i) * 0.1f);

    auto g = makeFullGraph(N);
    ASSERT_TRUE(engine.buildIndex({g}, vecs.data(), N));

    std::vector<float> query(DIM, 0.0f);
    auto results = engine.search(query.data(), K);
    EXPECT_EQ(results.size(), K);
}

TEST(CudaHnswEngine, SearchResultsSortedByScore) {
    constexpr uint32_t N   = 6;
    constexpr uint32_t DIM = 2;

    CudaHnswConfig cfg;
    cfg.dim    = DIM;
    cfg.metric = HnswDistanceMetric::L2;
    CudaHnswTraversalEngine engine(cfg);

    // Vectors at distances 0, 1, 2, 3, 4, 5 from origin
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i));
        vecs.push_back(0.0f);
    }

    auto g = makeFullGraph(N);
    ASSERT_TRUE(engine.buildIndex({g}, vecs.data(), N));

    std::vector<float> query(DIM, 0.0f);
    auto results = engine.search(query.data(), N);
    ASSERT_EQ(results.size(), N);

    // Scores should be non-decreasing (sorted ascending)
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i - 1].score, results[i].score);
    }
}

TEST(CudaHnswEngine, NearestNeighbourIsOriginWhenQueryIsOrigin) {
    constexpr uint32_t N   = 5;
    constexpr uint32_t DIM = 2;

    CudaHnswConfig cfg;
    cfg.dim    = DIM;
    cfg.metric = HnswDistanceMetric::L2;
    CudaHnswTraversalEngine engine(cfg);

    // Vector 0 = (0,0), vector 1 = (1,0), ...
    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i));
        vecs.push_back(0.0f);
    }

    auto g = makeFullGraph(N);
    ASSERT_TRUE(engine.buildIndex({g}, vecs.data(), N));

    std::vector<float> query = {0.0f, 0.0f};
    auto results = engine.search(query.data(), 1);
    ASSERT_FALSE(results.empty());
    // Best match should be vector 0 (at distance 0)
    EXPECT_EQ(results[0].id, 0);
    EXPECT_NEAR(results[0].score, 0.0f, 1e-5f);
}

// ─────────────────────────────────────────────────────────────────────────────
// batchSearch
// ─────────────────────────────────────────────────────────────────────────────

TEST(CudaHnswEngine, BatchSearchReturnsResultsForEachQuery) {
    constexpr uint32_t N   = 8;
    constexpr uint32_t DIM = 4;
    constexpr uint32_t K   = 2;
    constexpr uint32_t NQ  = 3;

    CudaHnswConfig cfg;
    cfg.dim = DIM;
    CudaHnswTraversalEngine engine(cfg);

    std::vector<float> vecs(N * DIM, 1.0f);
    auto g = makeFullGraph(N);
    ASSERT_TRUE(engine.buildIndex({g}, vecs.data(), N));

    std::vector<float> queries(NQ * DIM, 0.5f);
    auto results = engine.batchSearch(queries.data(), NQ, K);
    ASSERT_EQ(results.size(), NQ);
    for (const auto& r : results) {
        EXPECT_EQ(r.size(), K);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// setMaxBatchSize / visited pool
// ─────────────────────────────────────────────────────────────────────────────

TEST(CudaHnswEngine, SetMaxBatchSizeDefaultIs512) {
    CudaHnswConfig cfg;
    cfg.dim = 4;
    CudaHnswTraversalEngine engine(cfg);
    EXPECT_EQ(engine.maxBatchSize(), 512u);
}

TEST(CudaHnswEngine, SetMaxBatchSizeUpdatesValue) {
    CudaHnswConfig cfg;
    cfg.dim = 4;
    CudaHnswTraversalEngine engine(cfg);
    engine.setMaxBatchSize(128);
    EXPECT_EQ(engine.maxBatchSize(), 128u);
}

TEST(CudaHnswEngine, SetMaxBatchSizeZeroClampedToOne) {
    CudaHnswConfig cfg;
    cfg.dim = 4;
    CudaHnswTraversalEngine engine(cfg);
    engine.setMaxBatchSize(0);
    EXPECT_GE(engine.maxBatchSize(), 1u);
}

TEST(CudaHnswEngine, HasVisitedPoolFalseBeforeBuild) {
    CudaHnswConfig cfg;
    cfg.dim = 4;
    CudaHnswTraversalEngine engine(cfg);
    // Pool not allocated until buildIndex() is called
    EXPECT_FALSE(engine.hasVisitedPool());
}

// ─────────────────────────────────────────────────────────────────────────────
// Chunked batch processing (max_batch_size < num_queries)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CudaHnswEngine, ChunkedBatchSearchSmallPool) {
    // Build an engine with max_batch_size=2 so that a 5-query batch triggers
    // chunked processing (5 > 2).  Results must still be correct.
    constexpr uint32_t N   = 8;
    constexpr uint32_t DIM = 2;
    constexpr uint32_t K   = 2;
    constexpr uint32_t NQ  = 5;  // More than max_batch_size

    CudaHnswConfig cfg;
    cfg.dim = DIM;
    CudaHnswTraversalEngine engine(cfg);
    engine.setMaxBatchSize(2);  // Force chunking

    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
        vecs.push_back(static_cast<float>(i));
        vecs.push_back(0.0f);
    }

    auto g = makeFullGraph(N);
    ASSERT_TRUE(engine.buildIndex({g}, vecs.data(), N));

    std::vector<float> queries(NQ * DIM, 0.0f);
    auto results = engine.batchSearch(queries.data(), NQ, K);

    ASSERT_EQ(results.size(), static_cast<size_t>(NQ))
        << "Chunked batch search must return results for every query";
    for (size_t qi = 0; qi < NQ; ++qi) {
        EXPECT_EQ(results[qi].size(), static_cast<size_t>(K))
            << "Query " << qi << " must return exactly K=" << K << " results";
    }
}

TEST(CudaHnswEngine, ChunkedBatchSearchResultsCorrect) {
    // Verify that chunked processing (pool smaller than batch) returns results
    // consistent with a single-batch search.
    constexpr uint32_t N   = 6;
    constexpr uint32_t DIM = 1;
    constexpr uint32_t K   = 1;
    constexpr uint32_t NQ  = 4;

    CudaHnswConfig cfg;
    cfg.dim    = DIM;
    cfg.metric = HnswDistanceMetric::L2;

    // Engine A: default batch size (no chunking for NQ=4)
    CudaHnswTraversalEngine engineA(cfg);
    engineA.setMaxBatchSize(512);

    // Engine B: max_batch_size=1 (one query per kernel launch)
    CudaHnswTraversalEngine engineB(cfg);
    engineB.setMaxBatchSize(1);

    std::vector<float> vecs = {};

    for (uint32_t i = 0; i < N; ++i) {
      vecs.push_back(static_cast<float>(i));
    }

    auto g = makeFullGraph(N);
    ASSERT_TRUE(engineA.buildIndex({g}, vecs.data(), N));
    ASSERT_TRUE(engineB.buildIndex({g}, vecs.data(), N));

    // Queries at 0, 1, 2, 3
    std::vector<float> queries = {0.0f, 1.0f, 2.0f, 3.0f};

    auto resA = engineA.batchSearch(queries.data(), NQ, K);
    auto resB = engineB.batchSearch(queries.data(), NQ, K);

    ASSERT_EQ(resA.size(), static_cast<size_t>(NQ));
    ASSERT_EQ(resB.size(), static_cast<size_t>(NQ));

    for (size_t qi = 0; qi < NQ; ++qi) {
        ASSERT_FALSE(resA[qi].empty());
        ASSERT_FALSE(resB[qi].empty());
        // Both engines must agree on the nearest neighbour
        EXPECT_EQ(resA[qi][0].id, resB[qi][0].id)
            << "Chunked and non-chunked must agree on nearest neighbour for query "
            << qi;
    }
}

} // anonymous namespace
} // namespace themis

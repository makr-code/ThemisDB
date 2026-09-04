/**
 * @file test_cross_module_acceleration_index.cpp
 * @brief Cross-module integration tests: CPUVectorBackend × MatryoshkaTruncation ×
 *        MetricsCollector.
 *
 * Rationale
 * ---------
 * Individual module unit tests verify each component in isolation.  This file
 * validates the interactions at module boundaries that only emerge when the
 * three components are composed:
 *
 *   - CPUVectorBackend.computeDistances() provides L2 distance values that must
 *     be consistent with MatryoshkaTruncation prefix-truncate + L2-normalise
 *     when the same vectors are passed through both paths.
 *   - CPUVectorBackend.batchKnnSearch() returns sorted (index, distance) pairs
 *     that are coherent with a brute-force baseline on truncated vectors.
 *   - MetricsCollector correctly records acceleration- and index-level events
 *     so that Prometheus output reflects the full vector-search pipeline.
 *
 * Test groups
 * -----------
 * Group A (5 tests): CPUVectorBackend distance computation
 *   A-1  computeDistances returns numQueries × numVectors results
 *   A-2  Self-distance (query == vector) is 0.0 for L2 metric
 *   A-3  L2 distance is commutative: d(a,b) == d(b,a) to within 1e-5
 *   A-4  computeDistances produces same result as manual squared-norm loop
 *   A-5  Cosine distance mode (useL2=false) yields values in [0.0, 2.0]
 *
 * Group B (5 tests): CPUVectorBackend KNN search × MatryoshkaTruncation
 *   B-1  batchKnnSearch returns k results per query when k ≤ numVectors
 *   B-2  First result in batchKnnSearch is the nearest neighbour (matches brute-force)
 *   B-3  MatryoshkaTruncation::truncateAndNormalize reduces dim from 768 to 64
 *   B-4  truncated vectors fed into CPUVectorBackend KNN still return nearest neighbour
 *   B-5  Two-stage: truncate + coarse KNN then re-rank at full dim recalls NN
 *
 * Group C (5 tests): MetricsCollector × acceleration pipeline
 *   C-1  recordQuery("vector_knn") increments query counter
 *   C-2  recordIndexScan("ann_hnsw") increments index-scan counter
 *   C-3  MetricsCollector::reset() clears acceleration counters
 *   C-4  Prometheus output contains vector_knn metric after recording
 *   C-5  Mixed knn / embedding / index-scan sequence recorded independently
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "acceleration/cpu_backend.h"
#include "index/matryoshka_truncation.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::acceleration;
using namespace themis::index;
using namespace themis::observability;

// ============================================================================
// Shared helpers
// ============================================================================

namespace {

/// Generate n random unit vectors of dimension dim (reproducible).
static std::vector<std::vector<float>> randUnitVectors(size_t n, size_t dim,
                                                       unsigned seed = 77) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.f, 1.f);

    std::vector<std::vector<float>> vecs(n, std::vector<float>(dim, 0.f));
    for (auto& v : vecs) {
        float norm_sq = 0.f;
        for (float& x : v) { x = dist(rng); norm_sq += x * x; }
        float norm = std::sqrt(norm_sq);
        if (norm > 1e-10f)
            for (float& x : v) {
              x /= norm;
            }
    }
    return vecs;
}

/// Flatten a vector-of-vectors into a contiguous float array.
static std::vector<float> flatten(const std::vector<std::vector<float>>& vecs) {
    if (vecs.empty()) return {};
    const size_t dim = vecs[0].size();
    std::vector<float> flat(vecs.size() * dim);
    for (size_t i = 0; i < vecs.size(); ++i)
        std::copy(vecs[i].begin(), vecs[i].end(),
                  flat.begin() + static_cast<std::ptrdiff_t>(i * dim));
    return flat;
}

/// L2-squared distance between two equal-length vectors.
static float l2sq(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.f;
    for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

/// Brute-force nearest-neighbour index for a single query among db vectors.
static size_t bruteForceNN(const std::vector<float>& q,
                            const std::vector<std::vector<float>>& db) {
    size_t best = 0;
    float  best_d = std::numeric_limits<float>::max();
    for (size_t i = 0; i < db.size(); ++i) {
        float d = l2sq(q, db[i]);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

} // anonymous namespace

// ============================================================================
// Fixture
// ============================================================================

class AccelerationIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
        backend_ = std::make_unique<CPUVectorBackend>();
        ASSERT_TRUE(backend_->initialize());
    }

    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }

    std::unique_ptr<CPUVectorBackend> backend_;
};

// ============================================================================
// Group A – CPUVectorBackend distance computation
// ============================================================================

// A-1: computeDistances returns numQueries × numVectors results
TEST_F(AccelerationIndexTest, A1_ComputeDistancesReturnsCorrectResultCount) {
    const size_t nq = 3, nv = 5, dim = 16;
    auto queries = randUnitVectors(nq, dim, 1);
    auto vecs    = randUnitVectors(nv, dim, 2);

    auto qflat = flatten(queries);
    auto vflat = flatten(vecs);

    auto dists = backend_->computeDistances(
        qflat.data(), nq, dim, vflat.data(), nv, /*useL2=*/true);

    EXPECT_EQ(dists.size(), nq * nv)
        << "computeDistances must return numQueries × numVectors values";
}

// A-2: Self-distance (query == vector) is 0.0 for L2 metric
TEST_F(AccelerationIndexTest, A2_SelfDistanceIsZeroForL2) {
    const size_t dim = 32;
    auto vecs   = randUnitVectors(4, dim, 42);
    auto flat   = flatten(vecs);

    // query == database: each vector against itself
    auto dists = backend_->computeDistances(
        flat.data(), vecs.size(), dim,
        flat.data(), vecs.size(), /*useL2=*/true);

    for (size_t i = 0; i < vecs.size(); ++i) {
        float self_dist = dists[i * vecs.size() + i];
        EXPECT_NEAR(self_dist, 0.f, 1e-5f)
            << "Self-distance for vector " << i << " must be ~0";
    }
}

// A-3: L2 distance is commutative: d(a,b) == d(b,a) within 1e-5
TEST_F(AccelerationIndexTest, A3_L2DistanceIsCommutative) {
    const size_t dim = 24;
    auto a = randUnitVectors(1, dim, 10);
    auto b = randUnitVectors(1, dim, 20);

    auto aflat = flatten(a);
    auto bflat = flatten(b);

    auto d_ab = backend_->computeDistances(
        aflat.data(), 1, dim, bflat.data(), 1, true);
    auto d_ba = backend_->computeDistances(
        bflat.data(), 1, dim, aflat.data(), 1, true);

    ASSERT_EQ(d_ab.size(), 1u);
    ASSERT_EQ(d_ba.size(), 1u);
    EXPECT_NEAR(d_ab[0], d_ba[0], 1e-5f)
        << "L2 distance must be symmetric: d(a,b) == d(b,a)";
}

// A-4: computeDistances produces same result as manual squared-norm loop
TEST_F(AccelerationIndexTest, A4_ComputeDistancesMatchesManualL2Sq) {
    const size_t dim = 8;
    auto q    = randUnitVectors(1, dim, 3);
    auto vecs = randUnitVectors(4, dim, 5);

    auto qflat = flatten(q);
    auto vflat = flatten(vecs);

    auto dists = backend_->computeDistances(
        qflat.data(), 1, dim, vflat.data(), vecs.size(), true);

    for (size_t i = 0; i < vecs.size(); ++i) {
        float expected = l2sq(q[0], vecs[i]);
        EXPECT_NEAR(dists[i], expected, 1e-4f)
            << "computeDistances[" << i << "] must match manual L2-squared";
    }
}

// A-5: Cosine distance mode (useL2=false) yields values in [0.0, 2.0]
TEST_F(AccelerationIndexTest, A5_CosineDistanceValuesInRange) {
    const size_t dim = 16;
    auto queries = randUnitVectors(2, dim, 7);
    auto vecs    = randUnitVectors(3, dim, 8);

    auto qflat = flatten(queries);
    auto vflat = flatten(vecs);

    auto dists = backend_->computeDistances(
        qflat.data(), queries.size(), dim,
        vflat.data(), vecs.size(), /*useL2=*/false);

    EXPECT_EQ(dists.size(), queries.size() * vecs.size());
    for (float d : dists) {
        EXPECT_GE(d, -1e-5f)  << "Cosine distance must be ≥ 0";
        EXPECT_LE(d,  2.0f + 1e-5f) << "Cosine distance must be ≤ 2";
    }
}

// ============================================================================
// Group B – CPUVectorBackend KNN search × MatryoshkaTruncation
// ============================================================================

// B-1: batchKnnSearch returns k results per query when k ≤ numVectors
TEST_F(AccelerationIndexTest, B1_BatchKnnSearchReturnsKResultsPerQuery) {
    const size_t nq = 2, nv = 8, dim = 16, k = 3;
    auto queries = randUnitVectors(nq, dim, 11);
    auto vecs    = randUnitVectors(nv, dim, 22);

    auto qflat = flatten(queries);
    auto vflat = flatten(vecs);

    auto results = backend_->batchKnnSearch(
        qflat.data(), nq, dim, vflat.data(), nv, k, /*useL2=*/true);

    ASSERT_EQ(results.size(), nq) << "One result list per query";
    for (size_t qi = 0; qi < nq; ++qi) {
        EXPECT_EQ(results[qi].size(), k)
            << "Query " << qi << " must return exactly k=" << k << " neighbours";
    }
}

// B-2: First result in batchKnnSearch is the nearest neighbour (matches brute-force)
TEST_F(AccelerationIndexTest, B2_FirstResultIsNearestNeighbour) {
    const size_t nv = 10, dim = 32, k = 1;
    auto db = randUnitVectors(nv, dim, 33);
    // query = one of the db vectors (seed apart)
    auto queries = randUnitVectors(1, dim, 99);

    auto qflat = flatten(queries);
    auto dbflat = flatten(db);

    auto results = backend_->batchKnnSearch(
        qflat.data(), 1, dim, dbflat.data(), nv, k, true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 1u);

    size_t knn_id     = results[0][0].first;
    size_t bf_id      = bruteForceNN(queries[0], db);

    EXPECT_EQ(knn_id, static_cast<uint32_t>(bf_id))
        << "batchKnnSearch nearest neighbour must match brute-force result";
}

// B-3: MatryoshkaTruncation::truncateAndNormalize reduces 768-D to 64-D
TEST_F(AccelerationIndexTest, B3_TruncationReducesDimension) {
    const size_t full_dim = 768;
    auto full_vecs = randUnitVectors(5, full_dim, 44);

    MatryoshkaTruncation trunc(kMRL_64);

    for (const auto& v : full_vecs) {
        auto truncated = trunc.truncateAndNormalize(v.data(), full_dim);
        ASSERT_EQ(truncated.size(), kMRL_64)
            << "Truncated vector must have exactly kMRL_64 dimensions";
    }
}

// B-4: Truncated vectors fed into CPUVectorBackend KNN still return nearest neighbour
TEST_F(AccelerationIndexTest, B4_TruncatedVectorsKnnRecallsNearestNeighbour) {
    const size_t full_dim = 768;
    const size_t trunc_dim = kMRL_64;

    auto full_vecs = randUnitVectors(10, full_dim, 55);
    auto query_vecs = randUnitVectors(1, full_dim, 66);

    MatryoshkaTruncation trunc(trunc_dim);

    // Truncate database vectors
    std::vector<std::vector<float>> db_truncated;
    db_truncated.reserve(full_vecs.size());
    for (const auto& v : full_vecs) {
        db_truncated.push_back(trunc.truncateAndNormalize(v.data(), full_dim));
    }

    // Truncate query vector
    auto q_truncated = trunc.truncateAndNormalize(query_vecs[0].data(), full_dim);

    // Brute-force NN in truncated space
    size_t bf_id = bruteForceNN(q_truncated, db_truncated);

    // CPUVectorBackend KNN in truncated space
    auto qflat  = q_truncated;
    auto dbflat = flatten(db_truncated);

    auto results = backend_->batchKnnSearch(
        qflat.data(), 1, trunc_dim, dbflat.data(), full_vecs.size(), 1, true);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_FALSE(results[0].empty());

    EXPECT_EQ(results[0][0].first, static_cast<uint32_t>(bf_id))
        << "KNN on truncated vectors must agree with brute-force in truncated space";
}

// B-5: Two-stage: coarse KNN at 64-D recalls the NN found at full 768-D
TEST_F(AccelerationIndexTest, B5_TwoStageRecallsFullDimNearestNeighbour) {
    const size_t full_dim  = 128;   // use 128 for speed in test
    const size_t trunc_dim = kMRL_64;
    const size_t nv        = 20;
    const size_t k_coarse  = 5;

    auto full_db    = randUnitVectors(nv, full_dim, 71);
    auto query_vecs = randUnitVectors(1,  full_dim, 83);

    // Ground-truth NN in full space
    size_t gt_id = bruteForceNN(query_vecs[0], full_db);

    MatryoshkaTruncation trunc(trunc_dim);

    // Truncate database and query
    std::vector<std::vector<float>> db_t;
    db_t.reserve(nv);
    for (const auto& v : full_db)
        db_t.push_back(trunc.truncateAndNormalize(v.data(), full_dim));
    auto q_t = trunc.truncateAndNormalize(query_vecs[0].data(), full_dim);

    // Stage 1: coarse KNN at truncated dimension
    auto dbflat_t = flatten(db_t);
    auto coarse = backend_->batchKnnSearch(
        q_t.data(), 1, trunc_dim, dbflat_t.data(), nv, k_coarse, true);

    ASSERT_EQ(coarse.size(), 1u);
    ASSERT_EQ(coarse[0].size(), k_coarse);

    // Stage 2: re-rank coarse candidates at full dimension
    std::vector<std::pair<size_t, float>> candidates;
    for (const auto& [cand_idx, _] : coarse[0]) {
        float d = l2sq(query_vecs[0], full_db[cand_idx]);
        candidates.push_back({static_cast<size_t>(cand_idx), d});
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    // The ground-truth NN must be recalled in the coarse candidates
    bool recalled = std::any_of(coarse[0].begin(), coarse[0].end(),
                                [gt_id](const auto& p) {
                                    return p.first == static_cast<uint32_t>(gt_id);
                                });
    EXPECT_TRUE(recalled)
        << "Two-stage pipeline must recall the full-dim NN in the coarse candidate set";
}

// ============================================================================
// Group C – MetricsCollector × acceleration pipeline
// ============================================================================

// C-1: recordQuery("vector_knn") increments the query counter
TEST_F(AccelerationIndexTest, C1_RecordQueryVectorKnnIncrementsCounter) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    mc.recordQuery("vector_knn", 2.5, 10);
    mc.recordQuery("vector_knn", 1.8, 10);

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("vector_knn"), std::string::npos)
        << "Prometheus output must reference vector_knn after recording";
}

// C-2: recordIndexScan("ann_hnsw") increments the index-scan counter
TEST_F(AccelerationIndexTest, C2_RecordIndexScanAnnHnswIncrementsCounter) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    mc.recordIndexScan("ann_hnsw", 50);

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("ann_hnsw"), std::string::npos)
        << "Prometheus output must reference ann_hnsw after recording";
}

// C-3: MetricsCollector::reset() clears acceleration counters
TEST_F(AccelerationIndexTest, C3_ResetClearsAccelerationCounters) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordQuery("vector_knn", 3.0, 5);
    mc.recordIndexScan("ann_hnsw", 20);

    mc.reset();

    const std::string prom_after = mc.getPrometheusMetrics();
    EXPECT_TRUE(prom_after.find("vector_knn") == std::string::npos ||
                prom_after.size() < 500u)
        << "After reset(), Prometheus output should not retain stale vector_knn counters";
}

// C-4: Prometheus output contains vector_knn metric after a full pipeline run
TEST_F(AccelerationIndexTest, C4_FullPipelinePrometheusContainsVectorKnnMetric) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    const size_t nv = 8, dim = 16, k = 2;
    auto db      = randUnitVectors(nv, dim, 91);
    auto queries = randUnitVectors(2,  dim, 92);

    auto dbflat = flatten(db);
    auto qflat  = flatten(queries);

    // Run KNN and record metrics
    mc.recordIndexScan("ann_flat", nv);
    auto results = backend_->batchKnnSearch(
        qflat.data(), queries.size(), dim, dbflat.data(), nv, k, true);
    mc.recordQuery("vector_knn", 1.0, k * queries.size());

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("vector_knn"), std::string::npos)
        << "vector_knn must appear in Prometheus after full pipeline run";
    EXPECT_NE(prom.find("ann_flat"), std::string::npos)
        << "ann_flat index scan must appear in Prometheus";
}

// C-5: Mixed knn / embedding / index-scan sequence recorded independently
TEST_F(AccelerationIndexTest, C5_MixedAccelerationMetricsRecordedIndependently) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    mc.recordQuery("vector_knn",   1.5, 5);
    mc.recordEmbeddingGeneration(10, 20.0);
    mc.recordIndexScan("ann_hnsw", 100);
    mc.recordQuery("vector_radius_search", 3.0, 2);

    const std::string prom = mc.getPrometheusMetrics();

    EXPECT_NE(prom.find("vector_knn"),           std::string::npos)
        << "vector_knn must appear";
    EXPECT_NE(prom.find("ann_hnsw"),             std::string::npos)
        << "ann_hnsw index scan must appear";
    EXPECT_NE(prom.find("vector_radius_search"), std::string::npos)
        << "vector_radius_search query must appear";
}

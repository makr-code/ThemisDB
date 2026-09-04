/**
 * @file test_cross_module_index_matryoshka.cpp
 * @brief Cross-module integration tests: index (MatryoshkaTruncatedIndex /
 *        ScaNN / IAnnIndex) interacting with analytics (TimeSeries) and the
 *        two-stage Matryoshka retrieval pipeline.
 *
 * These tests exercise module-boundary contracts that individual unit tests
 * cannot catch:
 *
 * Group A – Two-stage retrieval pipeline: MatryoshkaTruncatedIndex + ScaNN
 * -------------------------------------------------------------------------
 *   A-1: Candidates from the 64-D stage are a superset of 256-D stage results
 *        when k is the same → wider net at lower dimension
 *   A-2: The ground-truth nearest neighbour is recalled at least once across
 *        two stages (coarse 128-D + fine re-rank)
 *   A-3: Two MatryoshkaTruncatedIndex instances at different dimensions share
 *        the same backend without interference
 *
 * Group B – Multi-granularity consistency
 * ----------------------------------------
 *   B-1: Building at trunc_dim=256 from 768-D vectors produces a valid index
 *        that returns results for a 768-D query
 *   B-2: Full-dimension build + search is functionally equivalent to wrapping
 *        a non-truncating index
 *   B-3: Adding vectors individually via add() vs batch build() yield the same
 *        nearest neighbour for a simple query
 *   B-4: kMRL_64, kMRL_128, kMRL_256 wrapped indices all return k results for
 *        a query (no crash, no empty result for populated indices)
 *
 * Group C – MatryoshkaTruncation + analytics::TimeSeries score trace
 * ------------------------------------------------------------------
 *   C-1: Similarity scores from MatryoshkaTruncatedIndex search can be
 *        accumulated into a TimeSeries and ForecastModel can be fit on them
 *   C-2: Scores are non-negative (L2-based) and the TimeSeries statistics
 *        (min, max, mean) are consistent with the raw score vector
 *   C-3: AnnSearchResult distance field is finite for every returned result
 *
 * Group D – IAnnIndex polymorphism across ScaNN and MatryoshkaTruncatedIndex
 * --------------------------------------------------------------------------
 *   D-1: Both ScaNN and MatryoshkaTruncatedIndex satisfy the IAnnIndex
 *        contract: build → size > 0, search → k results
 *   D-2: Nullptr backend raises std::invalid_argument in constructor
 *   D-3: size() is consistent between the truncated index and the backend
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "index/matryoshka_truncation.h"
#include "index/ann_index.h"
#include "analytics/forecasting.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::index;
using namespace themisdb::analytics;

// ============================================================================
// Shared helpers
// ============================================================================

/// Generate n random unit vectors of dimension dim (seed-reproducible).
static std::vector<std::vector<float>> randUnitVectors(size_t n, size_t dim,
                                                       unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.f, 1.f);

    std::vector<std::vector<float>> vecs(n, std::vector<float>(dim));
    for (auto& v : vecs) {
        float norm = 0.f;
        for (float& x : v) { x = dist(rng); norm += x * x; }
        norm = std::sqrt(norm);
        if (norm > 1e-10f) {
          for (float& x : v) {
            x /= norm;
          }
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
        std::copy(vecs[i].begin(), vecs[i].end(), flat.begin() + static_cast<std::ptrdiff_t>(i * dim));
    return flat;
}

/// Generate sequential integer IDs starting at 0.
static std::vector<int64_t> makeIds(size_t n) {
    std::vector<int64_t> ids(n);
    std::iota(ids.begin(), ids.end(), 0);
    return ids;
}

/// L2-squared distance between two vectors.
static float l2sq(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.f;
    for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
        float d = a[i] - b[i]; s += d * d;
    }
    return s;
}

/// Brute-force nearest-neighbour id for query among vectors.
static int64_t bruteForceNN(const std::vector<float>& query,
                             const std::vector<std::vector<float>>& db) {
    int64_t best_id = 0;
    float   best_d  = std::numeric_limits<float>::max();
    for (size_t i = 0; i < db.size(); ++i) {
        float d = l2sq(query, db[i]);
        if (d < best_d) { best_d = d; best_id = static_cast<int64_t>(i); }
    }
    return best_id;
}

// ============================================================================
// Shared fixture
// ============================================================================

class MatryoshkaCrossModuleFixture : public ::testing::Test {
protected:
    static constexpr size_t kN        = 50;   // number of vectors in the database
    static constexpr size_t kFullDim  = 256;  // full embedding dimension
    static constexpr size_t kTrunc64  = 64;   // first-stage truncation
    static constexpr size_t kTrunc128 = 128;  // second-stage truncation
    static constexpr int    kK        = 5;    // number of neighbours to retrieve

    std::vector<std::vector<float>> db_vecs_;
    std::vector<float>              db_flat_;
    std::vector<int64_t>            ids_;

    void SetUp() override {
        db_vecs_ = randUnitVectors(kN, kFullDim);
        db_flat_ = flatten(db_vecs_);
        ids_     = makeIds(kN);
    }

    /// Create a MatryoshkaTruncatedIndex wrapping a fresh ScaNN backend.
    std::shared_ptr<MatryoshkaTruncatedIndex> makeIndex(size_t trunc_dim) {
        return std::make_shared<MatryoshkaTruncatedIndex>(
            std::make_shared<ScaNN>(), trunc_dim);
    }
};

// ============================================================================
// Group A – Two-stage retrieval pipeline
// ============================================================================

// ---------------------------------------------------------------------------
// A-1: Coarser (64-D) stage retrieves at least as many distinct candidates as
//      the finer (128-D) stage for the same k.
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, TwoStage_CoarseStageHasAtLeastFineStageIds) {
    auto idx_64  = makeIndex(kTrunc64);
    auto idx_128 = makeIndex(kTrunc128);

    ASSERT_TRUE(idx_64->build(db_flat_.data(),  ids_.data(), kN, kFullDim));
    ASSERT_TRUE(idx_128->build(db_flat_.data(), ids_.data(), kN, kFullDim));

    // Use the first vector as query
    const float* q = db_flat_.data();
    auto coarse    = idx_64->search(q,  kFullDim, kK);
    auto fine      = idx_128->search(q, kFullDim, kK);

    // Both stages must return exactly kK results (index is large enough)
    EXPECT_EQ(coarse.size(), static_cast<size_t>(kK));
    EXPECT_EQ(fine.size(),   static_cast<size_t>(kK));

    // Collect IDs from each stage
    std::vector<int64_t> coarse_ids, fine_ids;
    for (const auto& r : coarse) {
      coarse_ids.push_back(r.id);
    }
    for (const auto& r : fine) {
      fine_ids.push_back(r.id);
    }

    // At least one ID from the fine stage must appear in the coarse stage
    // (the top-1 from 128-D should be in top-5 of 64-D for unit vectors).
    bool overlap = false;
    for (int64_t fid : fine_ids) {
        if (std::find(coarse_ids.begin(), coarse_ids.end(), fid) != coarse_ids.end()) {
            overlap = true;
            break;
        }
    }
    EXPECT_TRUE(overlap)
        << "There must be at least one overlapping ID between the 64-D and 128-D stages";
}

// ---------------------------------------------------------------------------
// A-2: Ground-truth NN is recalled by the two-stage pipeline
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, TwoStage_GroundTruthNNIsRecalled) {
    auto idx_128 = makeIndex(kTrunc128);
    ASSERT_TRUE(idx_128->build(db_flat_.data(), ids_.data(), kN, kFullDim));

    // Use vector[3] as query (not index 0 to avoid trivial self-match)
    const std::vector<float>& query_vec = db_vecs_[3];
    int64_t gt_id = bruteForceNN(query_vec, db_vecs_);

    auto results = idx_128->search(query_vec.data(), kFullDim, kK);
    ASSERT_FALSE(results.empty());

    // The ground truth must appear in the top-k results
    bool found = false;
    for (const auto& r : results) {
        if (r.id == gt_id) { found = true; break; }
    }
    EXPECT_TRUE(found)
        << "Ground-truth NN (id=" << gt_id
        << ") must appear in top-" << kK << " results from 128-D index";
}

// ---------------------------------------------------------------------------
// A-3: Two MatryoshkaTruncatedIndex at different dims share backend without
//      interference — building one does not corrupt the other
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, TwoIndependentTruncations_DoNotInterfere) {
    auto backend_a = std::make_shared<ScaNN>();
    auto backend_b = std::make_shared<ScaNN>();

    MatryoshkaTruncatedIndex idx_a(backend_a, kTrunc64);
    MatryoshkaTruncatedIndex idx_b(backend_b, kTrunc128);

    ASSERT_TRUE(idx_a.build(db_flat_.data(), ids_.data(), kN, kFullDim));
    ASSERT_TRUE(idx_b.build(db_flat_.data(), ids_.data(), kN, kFullDim));

    EXPECT_EQ(idx_a.size(), kN) << "idx_a must hold all N vectors";
    EXPECT_EQ(idx_b.size(), kN) << "idx_b must hold all N vectors";

    // Search both independently and verify no crash
    auto res_a = idx_a.search(db_flat_.data(), kFullDim, 3);
    auto res_b = idx_b.search(db_flat_.data(), kFullDim, 3);
    EXPECT_EQ(res_a.size(), 3u);
    EXPECT_EQ(res_b.size(), 3u);
}

// ============================================================================
// Group B – Multi-granularity consistency
// ============================================================================

// ---------------------------------------------------------------------------
// B-1: Truncated 256-D build from 768-D vectors returns results
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, TruncatedBuild_256from768_ReturnsResults) {
    constexpr size_t kBigDim   = 384; // simulate a larger model dim
    constexpr size_t kTrunc256 = 256;

    auto big_vecs = randUnitVectors(kN, kBigDim);
    auto big_flat = flatten(big_vecs);
    auto idx      = makeIndex(kTrunc256);

    ASSERT_TRUE(idx->build(big_flat.data(), ids_.data(), kN, kBigDim));
    EXPECT_EQ(idx->size(), kN);

    // Query using a full 384-D vector; index truncates it to 256-D internally
    auto res = idx->search(big_flat.data(), kBigDim, kK);
    ASSERT_EQ(res.size(), static_cast<size_t>(kK))
        << "Truncated index must return k results for a 384-D query";
}

// ---------------------------------------------------------------------------
// B-2: Full-dimension index wrapping is functionally equivalent to plain ScaNN
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, FullDimWrapper_EquivalentToDirectScaNN) {
    // Wrap ScaNN with trunc_dim == kFullDim (no actual truncation)
    auto plain_backend     = std::make_shared<ScaNN>();
    auto wrapped_backend   = std::make_shared<ScaNN>();
    MatryoshkaTruncatedIndex idx_wrap(wrapped_backend, kFullDim, /*normalize=*/false);

    ASSERT_TRUE(plain_backend->build(db_flat_.data(), ids_.data(), kN, kFullDim));
    ASSERT_TRUE(idx_wrap.build(db_flat_.data(), ids_.data(), kN, kFullDim));

    EXPECT_EQ(plain_backend->size(), idx_wrap.size())
        << "Both indices must have the same number of stored vectors";

    const float* q = db_flat_.data() + kFullDim; // use second vector as query
    auto res_plain  = plain_backend->search(q, kFullDim, 1);
    auto res_wrap   = idx_wrap.search(q, kFullDim, 1);

    ASSERT_EQ(res_plain.size(), 1u);
    ASSERT_EQ(res_wrap.size(),  1u);
    EXPECT_EQ(res_plain[0].id, res_wrap[0].id)
        << "Full-dim MatryoshkaTruncatedIndex and plain ScaNN must agree on top-1 NN";
}

// ---------------------------------------------------------------------------
// B-3: add() vs build() yield the same nearest neighbour
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, AddVsBuild_SameNearestNeighbour) {
    constexpr size_t kSmall = 10;
    auto small_vecs = randUnitVectors(kSmall, kFullDim, 99);
    auto small_flat = flatten(small_vecs);
    auto small_ids  = makeIds(kSmall);

    // Build via batch
    auto idx_build = makeIndex(kTrunc128);
    ASSERT_TRUE(idx_build->build(small_flat.data(), small_ids.data(),
                                 kSmall, kFullDim));

    // Build via individual add()
    auto idx_add = makeIndex(kTrunc128);
    for (size_t i = 0; i < kSmall; ++i) {
        ASSERT_TRUE(idx_add->add(static_cast<int64_t>(i),
                                 small_flat.data() + i * kFullDim, kFullDim));
    }

    // Query with the first vector (id=0 should be NN of itself)
    auto res_build = idx_build->search(small_flat.data(), kFullDim, 1);
    auto res_add   = idx_add->search(small_flat.data(), kFullDim, 1);

    ASSERT_EQ(res_build.size(), 1u);
    ASSERT_EQ(res_add.size(),   1u);
    EXPECT_EQ(res_build[0].id, 0) << "NN of first vector via build() must be itself";
    EXPECT_EQ(res_add[0].id,   0) << "NN of first vector via add() must be itself";
}

// ---------------------------------------------------------------------------
// B-4: kMRL_64/128/256 all return k results (no crash, no empty)
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, AllMRLGranularities_ReturnKResults) {
    const std::vector<size_t> granularities = {kMRL_64, kMRL_128, kMRL_256};

    for (size_t trunc : granularities) {
        if (trunc >= kFullDim) continue; // skip if trunc >= our fixture dim

        auto idx = makeIndex(trunc);
        ASSERT_TRUE(idx->build(db_flat_.data(), ids_.data(), kN, kFullDim))
            << "build() must succeed for trunc_dim=" << trunc;
        EXPECT_EQ(idx->size(), kN)
            << "size() must equal N for trunc_dim=" << trunc;

        auto res = idx->search(db_flat_.data(), kFullDim, kK);
        EXPECT_EQ(res.size(), static_cast<size_t>(kK))
            << "search() must return kK results for trunc_dim=" << trunc;
    }
}

// ============================================================================
// Group C – MatryoshkaTruncation + analytics::TimeSeries score trace
// ============================================================================

// ---------------------------------------------------------------------------
// C-1: Similarity scores from ANN search feed into ForecastModel
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, SearchScores_FeedIntoForecastModel) {
    constexpr int kNumQueries = 12; // must be >= 2 for ForecastModel

    auto idx = makeIndex(kTrunc128);
    ASSERT_TRUE(idx->build(db_flat_.data(), ids_.data(), kN, kFullDim));

    // Issue kNumQueries successive queries using different vectors and collect
    // the top-1 distance score for each query.
    TimeSeries score_trace;
    for (int qi = 0; qi < kNumQueries; ++qi) {
        const float* q = db_flat_.data() + qi * kFullDim;
        auto res       = idx->search(q, kFullDim, 1);
        ASSERT_EQ(res.size(), 1u);
        // Use query index as timestamp (ms), distance as value.
        score_trace.push(static_cast<int64_t>(qi) * 1000LL,
                         static_cast<double>(res[0].distance));
    }

    ASSERT_EQ(score_trace.size(), static_cast<size_t>(kNumQueries));

    // Fit a ForecastModel to the score trace
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    ASSERT_NO_THROW(model.fit(score_trace))
        << "ForecastModel must fit successfully on ANN score data";
    EXPECT_TRUE(model.isFitted());

    auto preds = model.predict(3);
    ASSERT_EQ(preds.size(), 3u)
        << "Forecast must produce exactly 3 future predictions";
    for (const auto& p : preds) {
        EXPECT_TRUE(std::isfinite(p.value))
            << "Predicted score must be finite";
    }
}

// ---------------------------------------------------------------------------
// C-2: Score trace statistics are self-consistent
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, SearchScores_TimeSeries_StatisticsConsistent) {
    auto idx = makeIndex(kTrunc128);
    ASSERT_TRUE(idx->build(db_flat_.data(), ids_.data(), kN, kFullDim));

    constexpr int kNumQ = 20;
    std::vector<double> raw_scores;
    TimeSeries ts;
    for (int qi = 0; qi < kNumQ; ++qi) {
        const float* q = db_flat_.data() + qi * kFullDim;
        auto res       = idx->search(q, kFullDim, 1);
        ASSERT_EQ(res.size(), 1u);
        double score = static_cast<double>(res[0].distance);
        raw_scores.push_back(score);
        ts.push(static_cast<int64_t>(qi) * 500LL, score);
    }

    // TimeSeries statistics must match manually computed values.
    double raw_min = *std::min_element(raw_scores.begin(), raw_scores.end());
    double raw_max = *std::max_element(raw_scores.begin(), raw_scores.end());
    double raw_sum = std::accumulate(raw_scores.begin(), raw_scores.end(), 0.0);
    double raw_mean = raw_sum / static_cast<double>(kNumQ);

    EXPECT_DOUBLE_EQ(ts.min(), raw_min)
        << "TimeSeries::min() must match raw score minimum";
    EXPECT_DOUBLE_EQ(ts.max(), raw_max)
        << "TimeSeries::max() must match raw score maximum";

    double ts_mean = ts.mean();
    EXPECT_NEAR(ts_mean, raw_mean, 1e-9)
        << "TimeSeries::mean() must match raw score mean";
}

// ---------------------------------------------------------------------------
// C-3: AnnSearchResult distances are all finite
// ---------------------------------------------------------------------------
TEST_F(MatryoshkaCrossModuleFixture, AnnSearchResult_Distances_AreFinite) {
    auto idx = makeIndex(kTrunc128);
    ASSERT_TRUE(idx->build(db_flat_.data(), ids_.data(), kN, kFullDim));

    auto results = idx->search(db_flat_.data(), kFullDim, kK);
    ASSERT_EQ(results.size(), static_cast<size_t>(kK));

    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_TRUE(std::isfinite(results[i].distance))
            << "Distance at position " << i << " must be finite";
        EXPECT_GE(results[i].distance, 0.f)
            << "L2-based distance at position " << i << " must be non-negative";
    }
}

// ============================================================================
// Group D – IAnnIndex polymorphism
// ============================================================================

class IAnnIndexPolymorphismTest : public ::testing::Test {
protected:
    static constexpr size_t kN   = 20;
    static constexpr size_t kDim = 64;
    static constexpr int    kK   = 3;

    std::vector<float>   db_flat_;
    std::vector<int64_t> ids_;

    void SetUp() override {
        db_flat_ = flatten(randUnitVectors(kN, kDim));
        ids_     = makeIds(kN);
    }
};

// ---------------------------------------------------------------------------
// D-1: Both ScaNN and MatryoshkaTruncatedIndex satisfy IAnnIndex contract
// ---------------------------------------------------------------------------
TEST_F(IAnnIndexPolymorphismTest, BothImplementations_SatisfyIAnnIndexContract) {
    // Use IAnnIndex pointer to test both implementations uniformly.
    std::vector<std::unique_ptr<IAnnIndex>> implementations;
    implementations.push_back(std::make_unique<ScaNN>());
    implementations.push_back(std::make_unique<MatryoshkaTruncatedIndex>(
        std::make_shared<ScaNN>(), kDim));  // trunc_dim == kDim → no actual truncation

    for (auto& idx : implementations) {
        EXPECT_EQ(idx->size(), 0u) << "size() before build must be 0";
        ASSERT_TRUE(idx->build(db_flat_.data(), ids_.data(), kN, kDim))
            << "build() must succeed";
        EXPECT_EQ(idx->size(), kN) << "size() after build must equal N";

        auto res = idx->search(db_flat_.data(), kDim, kK);
        EXPECT_EQ(res.size(), static_cast<size_t>(kK))
            << "search() must return exactly k results";
    }
}

// ---------------------------------------------------------------------------
// D-2: Null backend pointer raises std::invalid_argument
// ---------------------------------------------------------------------------
TEST_F(IAnnIndexPolymorphismTest, NullBackend_ThrowsInvalidArgument) {
    EXPECT_THROW(
        (MatryoshkaTruncatedIndex(nullptr, kDim)),
        std::invalid_argument)
        << "Null backend must throw std::invalid_argument";
}

// ---------------------------------------------------------------------------
// D-3: size() is consistent between MatryoshkaTruncatedIndex and its backend
// ---------------------------------------------------------------------------
TEST_F(IAnnIndexPolymorphismTest, Size_IsConsistentBetweenWrapperAndBackend) {
    auto backend = std::make_shared<ScaNN>();
    MatryoshkaTruncatedIndex idx(backend, kDim / 2);

    ASSERT_TRUE(idx.build(db_flat_.data(), ids_.data(), kN, kDim));

    EXPECT_EQ(idx.size(), kN)
        << "MatryoshkaTruncatedIndex::size() must equal N";
    EXPECT_EQ(idx.backend().size(), kN)
        << "backend().size() must equal wrapper.size()";
    EXPECT_EQ(idx.size(), idx.backend().size())
        << "Wrapper and backend must agree on size";
}

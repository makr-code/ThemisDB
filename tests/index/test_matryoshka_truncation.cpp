// Unit tests for MatryoshkaTruncation and MatryoshkaTruncatedIndex
// (index module — Issue: Matryoshka truncation partially planned v1.4.1)
//
// Test coverage:
//   MatryoshkaTruncation
//     1.  Truncate shorter than full_dim keeps first trunc_dim elements
//     2.  Truncate equal to full_dim is a no-op (modulo normalisation)
//     3.  Truncate > full_dim zero-pads output to trunc_dim
//     4.  Normalisation: output has unit L2 norm
//     5.  Normalisation disabled: raw values preserved
//     6.  Zero vector → no division by zero; output stays zero
//     7.  trunc_dim=1 edge case
//     8.  Standard granularities constant values are correct
//     9.  Constructor rejects trunc_dim=0
//
//   MatryoshkaTruncatedIndex (wrapping ScaNN)
//    10.  size() returns 0 before build
//    11.  build() populates index; size() reflects count
//    12.  search() returns non-empty results after build
//    13.  Nearest to a query that equals a stored vector has id of that vector
//    14.  Recall: top-k from truncated index contains most brute-force top-k
//    15.  add() increments size
//    16.  build() on empty dataset returns true; size() == 0
//    17.  search() on empty index returns empty vector (no crash)
//    18.  Null backend pointer raises exception in constructor
//    19.  save() / load() roundtrip preserves size (via ScaNN save/load)
//    20.  trunc_dim > full_dim: build and search still work (zero padding)
//    21.  Polymorphic IAnnIndex pointer works with MatryoshkaTruncatedIndex
//    22.  truncation() accessor returns correct trunc_dim
//    23.  backend() accessor returns the wrapped index
//    24.  k larger than index size returns as many as available (no crash)
//    25.  Multiple add() calls followed by search() works

#include "index/matryoshka_truncation.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::index;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float l2norm(const std::vector<float>& v) {
    float s = 0.f;
    for (float x : v) {
      s += x * x;
    }
    return std::sqrt(s);
}

static float l2dist(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i]; s += d * d;
    }
    return std::sqrt(s);
}

static std::vector<std::vector<float>> rand_vecs(
        size_t n, size_t dim, unsigned seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    std::vector<std::vector<float>> out(n, std::vector<float>(dim));
    for (auto& v : out)
        for (auto& x : v) {
          x = dist(rng);
        }
    return out;
}

static std::vector<float> flatten(const std::vector<std::vector<float>>& vv) {
    std::vector<float> out = {};

    out.reserve(vv.size() * (vv.empty() ? 0 : vv[0].size()));
    for (const auto& v : vv)
        out.insert(out.end(), v.begin(), v.end());
    return out;
}

static std::vector<int64_t> brute_force_knn(
        const std::vector<std::vector<float>>& db,
        const std::vector<float>& q, int k)
{
    std::vector<std::pair<float, int64_t>> scored;
    scored.reserve(db.size());
    for (size_t i = 0; i < db.size(); ++i)
        scored.emplace_back(l2dist(db[i], q), static_cast<int64_t>(i));
    std::sort(scored.begin(), scored.end());
    scored.resize(std::min<size_t>(scored.size(), static_cast<size_t>(k)));
    std::vector<int64_t> ids = {};

    for (auto& p : scored) {
      ids.push_back(p.second);
    }
    return ids;
}

// ---------------------------------------------------------------------------
// MatryoshkaTruncation unit tests
// ---------------------------------------------------------------------------

TEST(MatryoshkaTruncationTest, TruncateShorterKeepsFirstNDims) {
    std::vector<float> v = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    MatryoshkaTruncation t(3, /*normalize=*/false);
    auto out = t.truncate(v);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_FLOAT_EQ(out[0], 1.f);
    EXPECT_FLOAT_EQ(out[1], 2.f);
    EXPECT_FLOAT_EQ(out[2], 3.f);
}

TEST(MatryoshkaTruncationTest, TruncateEqualDimIsNoOp_NoNorm) {
    std::vector<float> v = {1.f, 2.f, 3.f};
    MatryoshkaTruncation t(3, false);
    auto out = t.truncate(v);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_FLOAT_EQ(out[0], 1.f);
    EXPECT_FLOAT_EQ(out[1], 2.f);
    EXPECT_FLOAT_EQ(out[2], 3.f);
}

TEST(MatryoshkaTruncationTest, TruncateLargerZeroPads) {
    std::vector<float> v = {1.f, 2.f};
    MatryoshkaTruncation t(4, false);
    auto out = t.truncate(v);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_FLOAT_EQ(out[0], 1.f);
    EXPECT_FLOAT_EQ(out[1], 2.f);
    EXPECT_FLOAT_EQ(out[2], 0.f);
    EXPECT_FLOAT_EQ(out[3], 0.f);
}

TEST(MatryoshkaTruncationTest, NormalizeProducesUnitNorm) {
    std::vector<float> v = {3.f, 4.f};  // |v| = 5
    MatryoshkaTruncation t(2, true);
    auto out = t.truncate(v);
    EXPECT_NEAR(l2norm(out), 1.f, 1e-6f);
    EXPECT_NEAR(out[0], 3.f / 5.f, 1e-6f);
    EXPECT_NEAR(out[1], 4.f / 5.f, 1e-6f);
}

TEST(MatryoshkaTruncationTest, NoNormalize_RawValuesPreserved) {
    std::vector<float> v = {3.f, 4.f};
    MatryoshkaTruncation t(2, false);
    auto out = t.truncate(v);
    EXPECT_FLOAT_EQ(out[0], 3.f);
    EXPECT_FLOAT_EQ(out[1], 4.f);
}

TEST(MatryoshkaTruncationTest, ZeroVector_NoDivisionByZero) {
    std::vector<float> v = {0.f, 0.f, 0.f};
    MatryoshkaTruncation t(3, true);
    auto out = t.truncate(v);
    for (float x : out) {
      EXPECT_FLOAT_EQ(x, 0.f);
    }
}

TEST(MatryoshkaTruncationTest, TruncDim1_EdgeCase) {
    std::vector<float> v = {5.f, 10.f, 15.f};
    MatryoshkaTruncation t(1, false);
    auto out = t.truncate(v);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_FLOAT_EQ(out[0], 5.f);
}

TEST(MatryoshkaTruncationTest, StandardGranularityConstants) {
    EXPECT_EQ(kMRL_64,    64u);
    EXPECT_EQ(kMRL_128,  128u);
    EXPECT_EQ(kMRL_256,  256u);
    EXPECT_EQ(kMRL_512,  512u);
    EXPECT_EQ(kMRL_768,  768u);
    EXPECT_EQ(kMRL_1024, 1024u);
    EXPECT_EQ(kMRL_1536, 1536u);
}

TEST(MatryoshkaTruncationTest, Constructor_RejectsTruncDimZero) {
    EXPECT_THROW(MatryoshkaTruncation(0), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// MatryoshkaTruncatedIndex fixtures and tests
// ---------------------------------------------------------------------------

class MatryoshkaTruncatedIndexTest : public ::testing::Test {
protected:
    static constexpr size_t N        = 300;
    static constexpr size_t FULL_DIM = 32;
    static constexpr size_t TRUNC    = 8;
    static constexpr int    K        = 5;

    std::vector<std::vector<float>> db_;
    std::vector<float>              flat_db_;
    std::vector<int64_t>            ids_;
    fs::path                        tmp_dir_;

    void SetUp() override {
        db_      = rand_vecs(N, FULL_DIM, 99);
        flat_db_ = flatten(db_);
        ids_.resize(N);
        std::iota(ids_.begin(), ids_.end(), 0);

        tmp_dir_ = fs::temp_directory_path() / "themis_mrl_test";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    std::shared_ptr<MatryoshkaTruncatedIndex> make_idx(
            size_t trunc = TRUNC, bool norm = true) const
    {
        return std::make_shared<MatryoshkaTruncatedIndex>(
            std::make_shared<ScaNN>(), trunc, norm);
    }
};

TEST_F(MatryoshkaTruncatedIndexTest, SizeZeroBeforeBuild) {
    auto idx = make_idx();
    EXPECT_EQ(idx->size(), 0u);
}

TEST_F(MatryoshkaTruncatedIndexTest, BuildPopulatesSize) {
    auto idx = make_idx();
    ASSERT_TRUE(idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM));
    EXPECT_EQ(idx->size(), N);
}

TEST_F(MatryoshkaTruncatedIndexTest, SearchReturnsNonEmpty) {
    auto idx = make_idx();
    idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM);
    auto res = idx->search(flat_db_.data(), FULL_DIM, K);
    EXPECT_FALSE(res.empty());
}

TEST_F(MatryoshkaTruncatedIndexTest, SearchNearestToSelf) {
    // Query identical to first stored vector → nearest must be id=0
    auto idx = make_idx(TRUNC, /*normalize=*/false);
    idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM);
    auto res = idx->search(flat_db_.data(), FULL_DIM, 1);
    ASSERT_FALSE(res.empty());
    EXPECT_EQ(res[0].id, 0);
}

TEST_F(MatryoshkaTruncatedIndexTest, Recall_TopK) {
    // Build truncated index with 8-D prefix.
    // Build truncated brute-force reference with the same prefix.
    auto idx = make_idx(TRUNC, false);
    idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM);

    // Compute truncated brute-force neighbours for query 0.
    std::vector<std::vector<float>> trunc_db(N, std::vector<float>(TRUNC));
    for (size_t i = 0; i < N; ++i)
        std::copy(db_[i].begin(), db_[i].begin() + TRUNC, trunc_db[i].begin());

    std::vector<float> q_trunc(db_[0].begin(), db_[0].begin() + TRUNC);
    auto bf_ids = brute_force_knn(trunc_db, q_trunc, K);

    auto res = idx->search(flat_db_.data(), FULL_DIM, K);
    std::vector<int64_t> res_ids = {};

    for (auto& r : res) {
      res_ids.push_back(r.id);
    }

    // Recall: at least 3 out of K should overlap
    int overlap = 0;
    for (auto id : bf_ids)
        if (std::find(res_ids.begin(), res_ids.end(), id) != res_ids.end())
            ++overlap;
    EXPECT_GE(overlap, 3);
}

TEST_F(MatryoshkaTruncatedIndexTest, AddIncrementsSize) {
    auto idx = make_idx();
    idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM);
    size_t before = idx->size();
    ASSERT_TRUE(idx->add(static_cast<int64_t>(N + 10), flat_db_.data(), FULL_DIM));
    EXPECT_EQ(idx->size(), before + 1);
}

TEST_F(MatryoshkaTruncatedIndexTest, BuildEmptyDataset) {
    auto idx = make_idx();
    EXPECT_TRUE(idx->build(nullptr, nullptr, 0, FULL_DIM));
    EXPECT_EQ(idx->size(), 0u);
}

TEST_F(MatryoshkaTruncatedIndexTest, SearchOnEmptyIndex_NoCrash) {
    auto idx = make_idx();
    auto res = idx->search(flat_db_.data(), FULL_DIM, K);
    EXPECT_TRUE(res.empty());
}

TEST_F(MatryoshkaTruncatedIndexTest, NullBackend_ThrowsInvalidArgument) {
    EXPECT_THROW(
        (MatryoshkaTruncatedIndex(nullptr, TRUNC)),
        std::invalid_argument);
}

TEST_F(MatryoshkaTruncatedIndexTest, SaveLoad_Roundtrip) {
    auto idx = make_idx();
    idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM);

    fs::path save_path = tmp_dir_ / "mrl_scann";
    ASSERT_TRUE(idx->save(save_path.string()));

    auto idx2 = make_idx();
    ASSERT_TRUE(idx2->load(save_path.string()));
    EXPECT_EQ(idx2->size(), N);
}

TEST_F(MatryoshkaTruncatedIndexTest, TruncDimLargerThanFullDim_StillWorks) {
    // Use a truncation dimension larger than the actual vector dimension →
    // remaining components should be zero-padded.
    auto idx = std::make_shared<MatryoshkaTruncatedIndex>(
        std::make_shared<ScaNN>(), FULL_DIM * 2, false);
    EXPECT_TRUE(idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM));
    EXPECT_EQ(idx->size(), N);
    auto res = idx->search(flat_db_.data(), FULL_DIM, K);
    EXPECT_FALSE(res.empty());
}

TEST_F(MatryoshkaTruncatedIndexTest, PolymorphicIAnnIndexPointer) {
    std::shared_ptr<IAnnIndex> idx = make_idx();
    ASSERT_TRUE(idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM));
    EXPECT_EQ(idx->size(), N);
    auto res = idx->search(flat_db_.data(), FULL_DIM, K);
    EXPECT_FALSE(res.empty());
}

TEST_F(MatryoshkaTruncatedIndexTest, TruncationAccessor) {
    auto idx = make_idx(kMRL_128, true);
    EXPECT_EQ(idx->truncation().trunc_dim(), kMRL_128);
    EXPECT_TRUE(idx->truncation().normalize());
}

TEST_F(MatryoshkaTruncatedIndexTest, BackendAccessor) {
    auto backend = std::make_shared<ScaNN>();
    auto idx     = std::make_shared<MatryoshkaTruncatedIndex>(backend, TRUNC);
    // backend() should return the same underlying object
    EXPECT_EQ(&idx->backend(), backend.get());
}

TEST_F(MatryoshkaTruncatedIndexTest, KLargerThanSize_NoCrash) {
    // k > N should not crash — backend returns what it has
    auto idx = make_idx();
    idx->build(flat_db_.data(), ids_.data(), N, FULL_DIM);
    int oversized_k = static_cast<int>(N) * 2;
    EXPECT_NO_THROW(idx->search(flat_db_.data(), FULL_DIM, oversized_k));
}

TEST_F(MatryoshkaTruncatedIndexTest, MultipleAdds_ThenSearch) {
    auto idx = make_idx(TRUNC, false);
    for (size_t i = 0; i < N; ++i)
        ASSERT_TRUE(idx->add(static_cast<int64_t>(i), db_[i].data(), FULL_DIM));

    // Query same as first vector
    auto res = idx->search(db_[0].data(), FULL_DIM, 1);
    ASSERT_FALSE(res.empty());
    EXPECT_EQ(res[0].id, 0);
}

// ---------------------------------------------------------------------------
// MatryoshkaTruncation raw-pointer overload
// ---------------------------------------------------------------------------

TEST(MatryoshkaTruncationRawPtrTest, RawPtrAndVectorOverloadAgree) {
    const float arr[] = {1.f, 2.f, 3.f, 4.f, 5.f};
    std::vector<float> vec(arr, arr + 5);
    MatryoshkaTruncation t(3, true);
    auto out_ptr = t.truncate(arr, 5);
    auto out_vec = t.truncate(vec);
    ASSERT_EQ(out_ptr.size(), out_vec.size());
    for (size_t i = 0; i < out_ptr.size(); ++i)
        EXPECT_FLOAT_EQ(out_ptr[i], out_vec[i]);
}

// ---------------------------------------------------------------------------
// Normalisation correctness over multiple random vectors
// ---------------------------------------------------------------------------

TEST(MatryoshkaTruncationNormTest, AllOutputsHaveUnitNorm) {
    MatryoshkaTruncation t(kMRL_64, true);
    auto vecs = rand_vecs(50, kMRL_256, 1234);
    for (const auto& v : vecs) {
        auto out = t.truncate(v);
        float n = l2norm(out);
        // zero vectors are the only exception
        if (l2norm(v) > 1e-10f)
            EXPECT_NEAR(n, 1.f, 1e-5f);
    }
}

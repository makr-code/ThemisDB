// Unit and integration tests for the ScaNN / DiskANN alternative ANN backends
// (index module – Phase 3, Issue #1865/#1876)
//
// Covers:
//   - ScaNN: build, search, recall quality, add(), save/load
//   - DiskAnnAdapter (compile-time guarded by THEMIS_ENABLE_DISKANN)
//   - IAnnIndex interface contract

#include "index/ann_index.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numeric>
#include <random>
#include <set>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::index;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<std::vector<float>> make_random_vectors(
        size_t n, size_t dim, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    std::vector<std::vector<float>> vecs(n, std::vector<float>(dim));
    for (auto& v : vecs)
        for (auto& x : v) x = dist(rng);
    return vecs;
}

static float l2(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return std::sqrt(s);
}

// Compute brute-force k nearest neighbours; returns sorted ids.
static std::vector<int64_t> brute_force_knn(
        const std::vector<std::vector<float>>& db,
        const std::vector<float>& query, int k) {
    std::vector<std::pair<float, int64_t>> scored;
    scored.reserve(db.size());
    for (size_t i = 0; i < db.size(); ++i)
        scored.emplace_back(l2(db[i], query), static_cast<int64_t>(i));
    std::sort(scored.begin(), scored.end());
    std::vector<int64_t> ids;
    ids.reserve(k);
    for (int i = 0; i < k && i < static_cast<int>(scored.size()); ++i)
        ids.push_back(scored[i].second);
    return ids;
}

// recall@k: fraction of true top-k results found in returned results
static float recall_at_k(const std::vector<int64_t>& expected,
                          const std::vector<AnnSearchResult>& got) {
    std::set<int64_t> expected_set(expected.begin(), expected.end());
    size_t hits = 0;
    for (auto& r : got)
        if (expected_set.count(r.id)) ++hits;
    return static_cast<float>(hits) / static_cast<float>(expected.size());
}

// Flatten a vector-of-vectors into a contiguous float array
static std::vector<float> flatten(const std::vector<std::vector<float>>& vecs) {
    if (vecs.empty()) return {};
    std::vector<float> out;
    out.reserve(vecs.size() * vecs[0].size());
    for (const auto& v : vecs)
        out.insert(out.end(), v.begin(), v.end());
    return out;
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class ScaNNTest : public ::testing::Test {
protected:
    static constexpr size_t N   = 2000;
    static constexpr size_t DIM = 32;
    static constexpr int    K   = 10;

    std::vector<std::vector<float>> db_;
    std::vector<float>              flat_db_;
    std::vector<int64_t>            ids_;
    std::vector<float>              query_;
    fs::path                        tmp_dir_;

    void SetUp() override {
        db_     = make_random_vectors(N, DIM);
        flat_db_ = flatten(db_);
        ids_.resize(N);
        std::iota(ids_.begin(), ids_.end(), 0);

        // pick a query that is one of the database vectors (perfect recall possible)
        query_ = db_[0];

        tmp_dir_ = fs::temp_directory_path() / "themis_ann_test";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

// ---------------------------------------------------------------------------
// IAnnIndex contract
// ---------------------------------------------------------------------------

TEST_F(ScaNNTest, IAnnIndex_Build_And_Size) {
    ScaNN idx;
    EXPECT_EQ(idx.size(), 0u);

    bool ok = idx.build(flat_db_.data(), ids_.data(), N, DIM);
    ASSERT_TRUE(ok);
    EXPECT_EQ(idx.size(), N);
}

TEST_F(ScaNNTest, IAnnIndex_Search_Returns_K_Results) {
    ScaNN idx;
    idx.build(flat_db_.data(), ids_.data(), N, DIM);

    auto results = idx.search(query_.data(), DIM, K);
    EXPECT_EQ(static_cast<int>(results.size()), K);
}

TEST_F(ScaNNTest, IAnnIndex_Search_Distances_NonNegative) {
    ScaNN idx;
    idx.build(flat_db_.data(), ids_.data(), N, DIM);

    auto results = idx.search(query_.data(), DIM, K);
    for (const auto& r : results)
        EXPECT_GE(r.distance, 0.f);
}

TEST_F(ScaNNTest, IAnnIndex_Search_Distances_Ascending) {
    ScaNN idx;
    idx.build(flat_db_.data(), ids_.data(), N, DIM);

    auto results = idx.search(query_.data(), DIM, K);
    for (size_t i = 1; i < results.size(); ++i)
        EXPECT_LE(results[i - 1].distance, results[i].distance);
}

// ---------------------------------------------------------------------------
// Recall quality
// ---------------------------------------------------------------------------

TEST_F(ScaNNTest, Recall_At_K_Acceptable) {
    ScaNNConfig cfg;
    cfg.num_leaves            = 50;
    cfg.num_leaves_to_search  = 20;
    cfg.reorder_num_neighbors = 100;
    ScaNN idx(cfg);
    idx.build(flat_db_.data(), ids_.data(), N, DIM);

    // Compute recall over 20 random queries
    float total_recall = 0.f;
    int num_queries = 20;
    auto queries = make_random_vectors(static_cast<size_t>(num_queries), DIM, 99);
    for (auto& q : queries) {
        auto true_nn = brute_force_knn(db_, q, K);
        auto results = idx.search(q.data(), DIM, K);
        total_recall += recall_at_k(true_nn, results);
    }
    float avg_recall = total_recall / static_cast<float>(num_queries);
    // Expect at least 50% recall@10 (generous threshold for small dataset / many leaves)
    EXPECT_GE(avg_recall, 0.5f) << "Average recall@" << K << " = " << avg_recall;
}

// ---------------------------------------------------------------------------
// Add() after build
// ---------------------------------------------------------------------------

TEST_F(ScaNNTest, Add_After_Build_Increases_Size) {
    ScaNN idx;
    idx.build(flat_db_.data(), ids_.data(), N, DIM);
    size_t before = idx.size();

    std::vector<float> new_vec = make_random_vectors(1, DIM, 777)[0];
    idx.add(static_cast<int64_t>(N), new_vec.data(), DIM);

    EXPECT_EQ(idx.size(), before + 1);
}

TEST_F(ScaNNTest, Add_Without_Build_Then_Search) {
    ScaNN idx;
    // Add 100 vectors without building first
    auto vecs = make_random_vectors(100, DIM, 55);
    for (size_t i = 0; i < 100; ++i)
        idx.add(static_cast<int64_t>(i), vecs[i].data(), DIM);

    // Search should trigger lazy build
    auto results = idx.search(vecs[0].data(), DIM, 5);
    EXPECT_LE(static_cast<int>(results.size()), 5);
    EXPECT_GT(static_cast<int>(results.size()), 0);
}

// ---------------------------------------------------------------------------
// Save / Load
// ---------------------------------------------------------------------------

TEST_F(ScaNNTest, Save_And_Load_Roundtrip) {
    ScaNN idx;
    idx.build(flat_db_.data(), ids_.data(), N, DIM);
    auto results_before = idx.search(query_.data(), DIM, K);

    std::string path = (tmp_dir_ / "scann.bin").string();
    ASSERT_TRUE(idx.save(path));

    ScaNN loaded;
    ASSERT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.size(), idx.size());

    auto results_after = loaded.search(query_.data(), DIM, K);
    ASSERT_FALSE(results_after.empty());
    // Top-1 result should match
    EXPECT_EQ(results_after[0].id, results_before[0].id);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(ScaNNTest, Build_Empty_Returns_False) {
    ScaNN idx;
    EXPECT_FALSE(idx.build(nullptr, nullptr, 0, DIM));
}

TEST_F(ScaNNTest, Build_RejectsZeroLeaves) {
    ScaNNConfig cfg;
    cfg.num_leaves = 0;
    ScaNN idx(cfg);

    EXPECT_FALSE(idx.build(flat_db_.data(), ids_.data(), N, DIM));
}

TEST_F(ScaNNTest, Build_RejectsZeroPQSubspacesWhenAHEnabled) {
    ScaNNConfig cfg;
    cfg.enable_ah = true;
    cfg.pq_num_subspaces = 0;
    ScaNN idx(cfg);

    EXPECT_FALSE(idx.build(flat_db_.data(), ids_.data(), N, DIM));
}

TEST_F(ScaNNTest, Search_Empty_Index_Returns_Empty) {
    ScaNN idx;
    auto results = idx.search(query_.data(), DIM, K);
    EXPECT_TRUE(results.empty());
}

TEST_F(ScaNNTest, Add_RejectsDimensionMismatch) {
    ScaNN idx;
    ASSERT_TRUE(idx.add(1, query_.data(), DIM));

    EXPECT_FALSE(idx.add(2, query_.data(), DIM + 1));
}

TEST_F(ScaNNTest, Search_NullQuery_Returns_Empty) {
    ScaNN idx;
    ASSERT_TRUE(idx.build(flat_db_.data(), ids_.data(), N, DIM));

    auto results = idx.search(nullptr, DIM, K);
    EXPECT_TRUE(results.empty());
}

TEST_F(ScaNNTest, Small_Dataset_Single_Vector) {
    ScaNN idx;
    std::vector<float> v = {1.f, 2.f, 3.f, 4.f};
    int64_t id = 42;
    ASSERT_TRUE(idx.build(v.data(), &id, 1, 4));
    EXPECT_EQ(idx.size(), 1u);

    auto res = idx.search(v.data(), 4, 1);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].id, 42);
    EXPECT_NEAR(res[0].distance, 0.f, 1e-4f);
}

TEST_F(ScaNNTest, Config_Respected) {
    ScaNNConfig cfg;
    cfg.num_leaves           = 10;
    cfg.num_leaves_to_search = 5;
    ScaNN idx(cfg);
    EXPECT_EQ(idx.config().num_leaves, 10u);
    EXPECT_EQ(idx.config().num_leaves_to_search, 5u);
}

// ---------------------------------------------------------------------------
// DiskANN adapter (guarded by compile-time flag)
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_DISKANN
class DiskAnnAdapterTest : public ::testing::Test {
protected:
    static constexpr size_t N   = 200;
    static constexpr size_t DIM = 16;
    static constexpr int    K   = 5;

    std::vector<std::vector<float>> db_;
    std::vector<float>              flat_db_;
    std::vector<int64_t>            ids_;
    fs::path                        tmp_dir_;

    void SetUp() override {
        db_      = make_random_vectors(N, DIM, 7);
        flat_db_ = flatten(db_);
        ids_.resize(N);
        std::iota(ids_.begin(), ids_.end(), 0);

        tmp_dir_ = fs::temp_directory_path() / "themis_diskann_adapter_test";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(DiskAnnAdapterTest, Build_And_Search) {
    std::string path = (tmp_dir_ / "diskann.graph").string();
    DiskAnnAdapter adapter(path);

    ASSERT_TRUE(adapter.build(flat_db_.data(), ids_.data(), N, DIM));
    EXPECT_EQ(adapter.size(), N);

    auto results = adapter.search(flat_db_.data(), DIM, K);
    ASSERT_FALSE(results.empty());
    EXPECT_LE(static_cast<int>(results.size()), K);
}

TEST_F(DiskAnnAdapterTest, Add_Single_Vector) {
    std::string path = (tmp_dir_ / "diskann_add.graph").string();
    DiskAnnAdapter adapter(path);
    adapter.build(flat_db_.data(), ids_.data(), N, DIM);

    std::vector<float> new_vec = make_random_vectors(1, DIM, 888)[0];
    EXPECT_TRUE(adapter.add(static_cast<int64_t>(N + 1), new_vec.data(), DIM));
}

TEST_F(DiskAnnAdapterTest, Save_And_Load_Metadata_Roundtrip) {
    std::string graph_path = (tmp_dir_ / "diskann_save.graph").string();
    DiskAnnAdapter adapter(graph_path);
    ASSERT_TRUE(adapter.build(flat_db_.data(), ids_.data(), N, DIM));

    // save(path) creates path + ".meta" on disk
    std::string save_path = (tmp_dir_ / "diskann_save").string();
    ASSERT_TRUE(adapter.save(save_path));

    // A new adapter loading the saved metadata should report same size
    DiskAnnAdapter loaded(graph_path, 64);
    ASSERT_TRUE(loaded.load(save_path));
    EXPECT_EQ(loaded.size(), N);
}
#endif // THEMIS_ENABLE_DISKANN

// ---------------------------------------------------------------------------
// IAnnIndex polymorphism via unique_ptr
// ---------------------------------------------------------------------------

TEST(IAnnIndexPolymorphismStandaloneTest, UniquePtr_ScaNN) {
    constexpr size_t N = 500, DIM = 16;
    auto vecs = make_random_vectors(N, DIM);
    auto flat = flatten(vecs);
    std::vector<int64_t> ids(N);
    std::iota(ids.begin(), ids.end(), 0);

    std::unique_ptr<IAnnIndex> idx = std::make_unique<ScaNN>();
    ASSERT_TRUE(idx->build(flat.data(), ids.data(), N, DIM));
    EXPECT_EQ(idx->size(), N);

    auto q = vecs[0];
    auto res = idx->search(q.data(), DIM, 5);
    ASSERT_FALSE(res.empty());
    EXPECT_EQ(res[0].id, 0); // nearest to vecs[0] should be itself
    EXPECT_NEAR(res[0].distance, 0.f, 1e-3f);
}

// ---------------------------------------------------------------------------
// ScaNN add() before build – lazy build triggered on search
// ---------------------------------------------------------------------------

TEST(ScaNNLazyBuildTest, AddBeforeBuild_TopResult_IsFirst) {
    constexpr size_t N = 100, DIM = 8;
    auto vecs = make_random_vectors(N, DIM, 12);
    ScaNN idx;
    for (size_t i = 0; i < N; ++i)
        idx.add(static_cast<int64_t>(i), vecs[i].data(), DIM);

    // query is identical to first vector → top-1 must be id=0
    auto res = idx.search(vecs[0].data(), DIM, 1);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].id, 0);
}

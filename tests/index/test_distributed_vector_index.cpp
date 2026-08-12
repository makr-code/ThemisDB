// Unit and integration tests for DistributedVectorIndex (Issue #1879)
//
// Covers:
//   - Single-shard degenerate case
//   - Hash-based partitioning distribution
//   - Consistent-hash partitioning and ring wrap-around
//   - Scatter-gather KNN correctness (recall@k)
//   - Insert / remove / re-insert lifecycle
//   - Aggregated and per-shard statistics
//   - Custom shard injection (using ScaNN)

#include "index/distributed_vector_index.h"
#include "index/ann_index.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

using namespace themis::index;

// ---------------------------------------------------------------------------
// Test helpers
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
    assert(a.size() == b.size() && "l2: vectors must have the same dimension");
    float s = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return std::sqrt(s);
}

static std::vector<size_t> brute_force_knn(
        const std::vector<std::vector<float>>& db,
        const std::vector<float>& query, int k) {
    std::vector<std::pair<float, size_t>> scored;
    scored.reserve(db.size());
    for (size_t i = 0; i < db.size(); ++i)
        scored.emplace_back(l2(db[i], query), i);
    std::sort(scored.begin(), scored.end());
    std::vector<size_t> ids;
    ids.reserve(static_cast<size_t>(k));
    for (int i = 0; i < k && i < static_cast<int>(scored.size()); ++i)
        ids.push_back(scored[i].second);
    return ids;
}

// recall@k: fraction of true top-k in returned results
static float recall_at_k(const std::vector<size_t>& expected,
                          const std::vector<AnnSearchResult>& got) {
    if (expected.empty()) return 1.f;
    std::set<size_t> expected_set(expected.begin(), expected.end());
    size_t hits = 0;
    for (auto& r : got) {
        if (expected_set.count(static_cast<size_t>(r.id))) ++hits;
    }
    return static_cast<float>(hits) / static_cast<float>(expected.size());
}

class DeterministicAnnIndex final : public IAnnIndex {
public:
    explicit DeterministicAnnIndex(bool fail_after_first_add = false)
        : fail_after_first_add_(fail_after_first_add) {}

    bool build(const float*, const int64_t*, size_t, size_t) override {
        return true;
    }

    bool add(int64_t id, const float* vector, size_t dim) override {
        if (fail_after_first_add_ && add_calls_ >= 1) {
            ++add_calls_;
            return false;
        }
        ++add_calls_;
        data_[id] = std::vector<float>(vector, vector + dim);
        return true;
    }

    std::vector<AnnSearchResult> search(const float* query, size_t dim, int k) const override {
        std::vector<AnnSearchResult> out;
        out.reserve(data_.size());
        for (const auto& [id, vec] : data_) {
            float d = 0.f;
            const size_t dmax = std::min(dim, vec.size());
            for (size_t i = 0; i < dmax; ++i) {
                const float diff = vec[i] - query[i];
                d += diff * diff;
            }
            out.push_back({id, d});
        }
        std::sort(out.begin(), out.end(), [](const AnnSearchResult& a, const AnnSearchResult& b) {
            if (a.distance != b.distance) return a.distance < b.distance;
            return a.id < b.id;
        });
        if (k < static_cast<int>(out.size())) {
            out.resize(static_cast<size_t>(k));
        }
        return out;
    }

    size_t size() const override {
        return data_.size();
    }

private:
    bool fail_after_first_add_ = false;
    size_t add_calls_ = 0;
    std::unordered_map<int64_t, std::vector<float>> data_;
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class DistributedVectorIndexTest : public ::testing::Test {
protected:
    static constexpr size_t N   = 500;
    static constexpr size_t DIM = 16;
    static constexpr int    K   = 10;

    std::vector<std::vector<float>> db_;
    std::vector<std::string>        pks_;

    void SetUp() override {
        db_  = make_random_vectors(N, DIM, 42);
        pks_.reserve(N);
        for (size_t i = 0; i < N; ++i)
            pks_.push_back("vec_" + std::to_string(i));
    }

    /// Build a DistributedVectorIndex with @p cfg and insert all db_ vectors.
    DistributedVectorIndex buildIndex(const DistributedVectorIndexConfig& cfg) {
        DistributedVectorIndex idx(cfg);
        for (size_t i = 0; i < N; ++i)
            EXPECT_TRUE(idx.insert(pks_[i], db_[i]));
        return idx;
    }};

// ---------------------------------------------------------------------------
// Basic construction
// ---------------------------------------------------------------------------

TEST(DistributedVectorIndexBasic, DefaultConstruct) {
    DistributedVectorIndex idx;
    EXPECT_EQ(idx.numShards(), 4u);
    EXPECT_EQ(idx.size(), 0u);
}

TEST(DistributedVectorIndexBasic, InvalidNumShards) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 0;
    EXPECT_THROW(DistributedVectorIndex{cfg}, std::invalid_argument);
}

TEST(DistributedVectorIndexBasic, SingleShard) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    DistributedVectorIndex idx(cfg);
    EXPECT_EQ(idx.numShards(), 1u);
}

// ---------------------------------------------------------------------------
// Insert / remove / re-insert
// ---------------------------------------------------------------------------

TEST(DistributedVectorIndexLifecycle, InsertIncreasesSize) {
    DistributedVectorIndex idx;
    const std::vector<float> v(8, 1.f);
    EXPECT_TRUE(idx.insert("k1", v));
    EXPECT_EQ(idx.size(), 1u);
    EXPECT_TRUE(idx.insert("k2", v));
    EXPECT_EQ(idx.size(), 2u);
}
TEST(DistributedVectorIndexLifecycle, RemoveKnownKey) {
    DistributedVectorIndex idx;
    const std::vector<float> v(8, 1.f);
    (void)idx.insert("k1", v);
    EXPECT_TRUE(idx.remove("k1"));
}

TEST(DistributedVectorIndexLifecycle, RemoveUnknownKeyReturnsFalse) {
    DistributedVectorIndex idx;
    EXPECT_FALSE(idx.remove("no_such_key"));
}

// Regression for audit finding #1: size() must reflect logical (alive) count.
TEST(DistributedVectorIndexLifecycle, RemoveDecreasesSizeCorrectly) {
    DistributedVectorIndex idx;
    const std::vector<float> v(8, 1.f);
    (void)idx.insert("k1", v);
    (void)idx.insert("k2", v);
    ASSERT_EQ(idx.size(), 2u);
    (void)idx.remove("k1");
    EXPECT_EQ(idx.size(), 1u);
}

// Regression for audit finding #2: removed vector must not appear in search().
TEST(DistributedVectorIndexLifecycle, RemovedVectorNotInSearchResults) {
    // Use a single shard so there's no ambiguity about which shard owns the key.
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    DistributedVectorIndex idx(cfg);

    // "target" vector at (100, 0, 0, …) – very far from query (0,…)
    // "other" vectors cluster near the origin (closer to query)
    const size_t dim = 8;
    std::vector<float> target(dim, 0.f);
    target[0] = 100.f; // far from query

    std::vector<float> near(dim, 0.f);
    near[0] = 0.01f; // close to query

    (void)idx.insert("far_key", target);
    for (int i = 0; i < 5; ++i)
        (void)idx.insert("near_" + std::to_string(i), near);

    // Remove the far vector.
    (void)idx.remove("far_key");

    // Search should return only near vectors (never far_key's ID).
    std::vector<float> query(dim, 0.f);
    auto results = idx.search(query, 6);

    // far_key's ID was in shard 0; after remove, none of the results' IDs
    // should correspond to the ghost entry.  We verify indirectly: since
    // near vectors have distance ~0.01 and far_key had distance ~100, any
    // result with distance >= 50 is the ghost.
    for (const auto& r : results) {
        EXPECT_LT(r.distance, 50.f) << "Ghost (removed) vector appeared in results";
    }
}

// Regression for audit finding #3: re-inserting the same key should not
// create duplicate search results.
TEST(DistributedVectorIndexLifecycle, ReInsertDoesNotProduceDuplicateIds) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    DistributedVectorIndex idx(cfg);

    std::vector<float> v(4, 1.f);
    (void)idx.insert("k1", v);
    (void)idx.insert("k1", v); // update with same data

    // Should still count as one logical vector.
    EXPECT_EQ(idx.size(), 1u);

    // Search should return at most 1 result for k1.
    auto results = idx.search(v, 10);
    // All returned IDs should be unique.
    std::set<int64_t> seen_ids;
    for (const auto& r : results) {
        EXPECT_EQ(seen_ids.count(r.id), 0u) << "Duplicate ID in search results";
        seen_ids.insert(r.id);
    }
}

TEST(DistributedVectorIndexLifecycle, ReInsertSameKey) {
    DistributedVectorIndex idx;
    std::vector<float> v1(8, 1.f);
    std::vector<float> v2(8, 2.f);
    EXPECT_TRUE(idx.insert("k1", v1));
    EXPECT_TRUE(idx.insert("k1", v2)); // update
}

TEST(DistributedVectorIndexLifecycle, FailedUpdateRestoresExistingMapping) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    std::vector<std::unique_ptr<IAnnIndex>> shards;
    shards.push_back(std::make_unique<DeterministicAnnIndex>(true));
    DistributedVectorIndex idx(cfg, std::move(shards));

    const std::vector<float> v1{1.f, 0.f, 0.f, 0.f};
    const std::vector<float> v2{0.f, 1.f, 0.f, 0.f};
    ASSERT_TRUE(idx.insert("k1", v1));
    ASSERT_FALSE(idx.insert("k1", v2));
    EXPECT_EQ(idx.size(), 1u);
    EXPECT_TRUE(idx.remove("k1"));
    EXPECT_EQ(idx.size(), 0u);
}

TEST(DistributedVectorIndexLifecycle, SearchDeduplicatesGlobalIdCollisions) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    std::vector<std::unique_ptr<IAnnIndex>> shards;
    shards.push_back(std::make_unique<DeterministicAnnIndex>());
    DistributedVectorIndex idx(cfg, std::move(shards));

    ASSERT_TRUE(idx.insert("user_7", std::vector<float>{0.f, 0.f, 0.f, 0.f}));
    ASSERT_TRUE(idx.insert("other_7", std::vector<float>{2.f, 0.f, 0.f, 0.f}));

    auto results = idx.search(std::vector<float>{0.f, 0.f, 0.f, 0.f}, 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results.front().id, 7);
}

TEST(DistributedVectorIndexLifecycle, SearchUsesLatestVersionForGlobalIdCollision) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    std::vector<std::unique_ptr<IAnnIndex>> shards;
    shards.push_back(std::make_unique<DeterministicAnnIndex>());
    DistributedVectorIndex idx(cfg, std::move(shards));

    ASSERT_TRUE(idx.insert("user_7", std::vector<float>{0.f, 0.f, 0.f, 0.f}));
    ASSERT_TRUE(idx.insert("other_7", std::vector<float>{10.f, 0.f, 0.f, 0.f}));

    auto results = idx.search(std::vector<float>{0.f, 0.f, 0.f, 0.f}, 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results.front().id, 7);
    EXPECT_FLOAT_EQ(results.front().distance, 100.f);
}

TEST(DistributedVectorIndexLifecycle, InsertNullVectorReturnsFalse) {
    DistributedVectorIndex idx;
    EXPECT_FALSE(idx.insert("k1", nullptr, 8));
}

// ---------------------------------------------------------------------------
// shardFor determinism
// ---------------------------------------------------------------------------

TEST(DistributedVectorIndexRouting, HashShardDeterministic) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    cfg.strategy   = ShardingStrategy::HASH;
    DistributedVectorIndex idx(cfg);

    size_t shard1 = idx.shardFor("my_key");
    size_t shard2 = idx.shardFor("my_key");
    EXPECT_EQ(shard1, shard2);
    EXPECT_LT(shard1, 4u);
}

TEST(DistributedVectorIndexRouting, ConsistentHashDeterministic) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    cfg.strategy   = ShardingStrategy::CONSISTENT_HASH;
    DistributedVectorIndex idx(cfg);

    size_t s1 = idx.shardFor("alpha");
    size_t s2 = idx.shardFor("alpha");
    EXPECT_EQ(s1, s2);
    EXPECT_LT(s1, 4u);
}

TEST(DistributedVectorIndexRouting, HashDistributesAcrossShards) {
    // With enough keys, all shards should receive at least one vector.
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    cfg.strategy   = ShardingStrategy::HASH;
    DistributedVectorIndex idx(cfg);

    std::vector<size_t> counts(4, 0);
    for (int i = 0; i < 200; ++i) {
        size_t s = idx.shardFor("key_" + std::to_string(i));
        counts[s]++;
    }
    for (size_t c : counts) EXPECT_GT(c, 0u);
}

// ---------------------------------------------------------------------------
// KNN correctness – scatter-gather recall
// ---------------------------------------------------------------------------

TEST_F(DistributedVectorIndexTest, SingleShardExactRecall) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    auto idx = buildIndex(cfg);

    const auto& query = db_[0];
    auto expected     = brute_force_knn(db_, query, K);
    auto results      = idx.search(query, K);

    ASSERT_EQ(results.size(), static_cast<size_t>(K));
    float recall = recall_at_k(expected, results);
    // ScaNN approximate – expect at least 70% recall on small N=500
    EXPECT_GE(recall, 0.7f);
}

TEST_F(DistributedVectorIndexTest, HashStrategy_4Shards_Recall) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    cfg.strategy   = ShardingStrategy::HASH;
    auto idx = buildIndex(cfg);

    const auto& query = db_[0];
    auto expected     = brute_force_knn(db_, query, K);
    auto results      = idx.search(query, K);

    ASSERT_EQ(results.size(), static_cast<size_t>(K));
    float recall = recall_at_k(expected, results);
    EXPECT_GE(recall, 0.5f); // scatter-gather from all 4 shards
}

TEST_F(DistributedVectorIndexTest, ConsistentHash_4Shards_Recall) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    cfg.strategy   = ShardingStrategy::CONSISTENT_HASH;
    auto idx = buildIndex(cfg);

    const auto& query = db_[0];
    auto expected     = brute_force_knn(db_, query, K);
    auto results      = idx.search(query, K);

    ASSERT_EQ(results.size(), static_cast<size_t>(K));
    float recall = recall_at_k(expected, results);
    EXPECT_GE(recall, 0.5f);
}

TEST_F(DistributedVectorIndexTest, SearchResultsSortedByDistance) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 3;
    auto idx = buildIndex(cfg);

    auto results = idx.search(db_[42], K);
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_LE(results[i - 1].distance, results[i].distance);
    }
}

TEST_F(DistributedVectorIndexTest, TotalSizeEqualsInsertCount) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    auto idx = buildIndex(cfg);
    EXPECT_EQ(idx.size(), N);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST_F(DistributedVectorIndexTest, ShardStatsCount) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    auto idx = buildIndex(cfg);

    auto stats = idx.getShardStats();
    ASSERT_EQ(stats.size(), 4u);
    size_t total = 0;
    for (const auto& s : stats) total += s.vector_count;
    EXPECT_EQ(total, N);
}

TEST_F(DistributedVectorIndexTest, AggregatedStats) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 4;
    auto idx = buildIndex(cfg);

    auto stats = idx.getStats();
    EXPECT_EQ(stats.num_shards, 4u);
    EXPECT_EQ(stats.total_vectors, N);
    EXPECT_GE(stats.max_shard_size, stats.min_shard_size);
    EXPECT_GE(stats.load_imbalance, 0.0);
}

// ---------------------------------------------------------------------------
// Custom shard injection
// ---------------------------------------------------------------------------

TEST(DistributedVectorIndexCustomShards, InjectScaNNShards) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 2;
    cfg.strategy   = ShardingStrategy::HASH;

    std::vector<std::unique_ptr<IAnnIndex>> shards;
    shards.push_back(std::make_unique<ScaNN>());
    shards.push_back(std::make_unique<ScaNN>());

    DistributedVectorIndex idx(cfg, std::move(shards));
    EXPECT_EQ(idx.numShards(), 2u);

    const std::vector<float> v{1.f, 0.f, 0.f, 0.f};
    EXPECT_TRUE(idx.insert("k1", v));
    EXPECT_EQ(idx.size(), 1u);}

TEST(DistributedVectorIndexCustomShards, WrongShardCountThrows) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 3;

    std::vector<std::unique_ptr<IAnnIndex>> shards;
    shards.push_back(std::make_unique<ScaNN>());
    // Only 1 shard provided, but cfg says 3
    EXPECT_THROW(DistributedVectorIndex(cfg, std::move(shards)),
                 std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST(DistributedVectorIndexEdge, SearchEmptyIndexReturnsEmpty) {
    DistributedVectorIndex idx;
    const std::vector<float> q(8, 0.f);
    auto results = idx.search(q, 5);
    EXPECT_TRUE(results.empty());
}

TEST(DistributedVectorIndexEdge, SearchKLargerThanSize) {
    DistributedVectorIndex idx;
    for (int i = 0; i < 3; ++i) {
        (void)idx.insert("k" + std::to_string(i), std::vector<float>(8, static_cast<float>(i)));
    }
    auto results = idx.search(std::vector<float>(8, 0.f), 100);
    // Should return at most 3 results (number of inserted vectors)
    EXPECT_LE(results.size(), 3u);
}

TEST(DistributedVectorIndexEdge, MoveConstruct) {
    DistributedVectorIndex idx1;
    (void)idx1.insert("k1", std::vector<float>(8, 1.f));
    DistributedVectorIndex idx2(std::move(idx1));
    EXPECT_EQ(idx2.size(), 1u);
}

TEST(DistributedVectorIndexEdge, MoveConstructPreservesGlobalIdState) {
    DistributedVectorIndexConfig cfg;
    cfg.num_shards = 1;
    std::vector<std::unique_ptr<IAnnIndex>> shards;
    shards.push_back(std::make_unique<DeterministicAnnIndex>());
    DistributedVectorIndex idx1(cfg, std::move(shards));

    ASSERT_TRUE(idx1.insert("user_42", std::vector<float>{0.f, 0.f, 0.f, 0.f}));
    DistributedVectorIndex idx2(std::move(idx1));
    ASSERT_TRUE(idx2.insert("plain", std::vector<float>{1.f, 0.f, 0.f, 0.f}));

    auto results = idx2.search(std::vector<float>{0.f, 0.f, 0.f, 0.f}, 10);
    std::set<int64_t> ids;
    for (const auto& r : results) {
        ids.insert(r.id);
    }
    EXPECT_TRUE(ids.count(42));
    EXPECT_TRUE(ids.count(43));
}

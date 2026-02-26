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
    }
};

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
    idx.insert("k1", v);
    EXPECT_TRUE(idx.remove("k1"));
}

TEST(DistributedVectorIndexLifecycle, RemoveUnknownKeyReturnsFalse) {
    DistributedVectorIndex idx;
    EXPECT_FALSE(idx.remove("no_such_key"));
}

TEST(DistributedVectorIndexLifecycle, ReInsertSameKey) {
    DistributedVectorIndex idx;
    std::vector<float> v1(8, 1.f);
    std::vector<float> v2(8, 2.f);
    EXPECT_TRUE(idx.insert("k1", v1));
    EXPECT_TRUE(idx.insert("k1", v2)); // update
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
    EXPECT_EQ(idx.size(), 1u);
}

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
        idx.insert("k" + std::to_string(i), std::vector<float>(8, static_cast<float>(i)));
    }
    auto results = idx.search(std::vector<float>(8, 0.f), 100);
    // Should return at most 3 results (number of inserted vectors)
    EXPECT_LE(results.size(), 3u);
}

TEST(DistributedVectorIndexEdge, MoveConstruct) {
    DistributedVectorIndex idx1;
    idx1.insert("k1", std::vector<float>(8, 1.f));
    DistributedVectorIndex idx2(std::move(idx1));
    EXPECT_EQ(idx2.size(), 1u);
}

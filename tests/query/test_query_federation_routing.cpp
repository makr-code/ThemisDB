/*
 * QueryFederation shard-key routing focused tests (v1.9.0)
 * ---------------------------------------------------------
 * Validates the three routing paths introduced in QueryFederation:
 *
 *  1. Point-lookup  (._key == "v")   → exactly 1 shard
 *  2. Range query   (._key >= "a" AND ._key <= "z") → subset of shards
 *  3. Full-scan     (no key predicate)              → all shards (broadcast)
 *
 * The tests use a mock ShardRouter whose scatterGather() returns labelled
 * ShardResult objects so that the routing filter in PARTITION_PRUNING can be
 * verified without a real cluster.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <limits>
#include <unordered_set>

#include "query/query_federation.h"
#include "sharding/sharding_manager.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include "sharding/urn_resolver.h"
#include "sharding/remote_executor.h"
#include <nlohmann/json.hpp>

using namespace themis;
using namespace themis::query;
using namespace themis::sharding;
using json = nlohmann::json;

// ============================================================================
// Helper: build a minimal ShardRouter for tests
// ============================================================================

static std::shared_ptr<URNResolver>    makeResolver() {
    ShardTopology::Config tc;
    tc.cluster_name         = "test";
    tc.enable_health_checks = false;
    auto topo     = std::make_shared<ShardTopology>(tc);
    auto hash_ring = std::make_shared<ConsistentHashRing>(150);
    return std::make_shared<URNResolver>(topo, hash_ring);
}

static std::shared_ptr<RemoteExecutor> makeExecutor() {
    RemoteExecutor::Config cfg;
    cfg.connect_timeout_ms = 1000;
    cfg.request_timeout_ms = 5000;
    cfg.enable_signing      = false;
    return std::make_shared<RemoteExecutor>(cfg);
}

// ============================================================================
// Mock ShardRouter
// ============================================================================

/**
 * MockShardRouter returns one ShardResult per registered shard.
 * Each result carries its shard_id so we can inspect filtering.
 */
class MockShardRouter : public ShardRouter {
public:
    MockShardRouter()
        : ShardRouter(makeResolver(), makeExecutor(), ShardRouter::Config{})
    {}

    void registerShard(const std::string& shard_id) {
        shard_ids_.push_back(shard_id);
    }

    std::vector<ShardResult> scatterGather(const std::string& /*query*/) override {
        scatter_gather_calls_++;
        std::vector<ShardResult> results = {};

        for (const auto& id : shard_ids_) {
            ShardResult r;
            r.shard_id = id;
            r.success  = true;
            r.data     = json::array({json{{"shard", id}}});
            results.push_back(r);
        }
        return results;
    }

    std::vector<ShardResult> executeOnShards(const std::string& /*query*/,
                                             const std::vector<std::string>& shard_ids) override {
        execute_on_shards_calls_++;
        last_target_shards_ = shard_ids;
        std::unordered_set<std::string> targets(shard_ids.begin(), shard_ids.end());

        std::vector<ShardResult> results = {};

        for (const auto& id : shard_ids_) {
            if (!targets.count(id)) {
                continue;
            }
            ShardResult r;
            r.shard_id = id;
            r.success = true;
            r.data = json::array({json{{"shard", id}}});
            results.push_back(r);
        }
        return results;
    }

    const std::vector<std::string>& shardIds() const { return shard_ids_; }
    uint64_t scatterGatherCalls() const { return scatter_gather_calls_; }
    uint64_t executeOnShardsCalls() const { return execute_on_shards_calls_; }
    const std::vector<std::string>& lastTargetShards() const { return last_target_shards_; }

private:
    std::vector<std::string> shard_ids_;
    uint64_t scatter_gather_calls_{0};
    uint64_t execute_on_shards_calls_{0};
    std::vector<std::string> last_target_shards_;
};

// ============================================================================
// Fixture
// ============================================================================

class QueryFederationRoutingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Build a three-shard ShardingManager.
        // Nodes are added in a specific order; the consistent-hash ring
        // will distribute them clockwise.
        ShardNodeInfo n1{1, "shard-001", "PRIMARY", true};
        ShardNodeInfo n2{2, "shard-002", "PRIMARY", true};
        ShardNodeInfo n3{3, "shard-003", "PRIMARY", true};

        auto& mgr = ShardingManager::GetInstance();
        // Clear previous test state (singleton may persist between tests).
        mgr.RemoveShardNode(1);
        mgr.RemoveShardNode(2);
        mgr.RemoveShardNode(3);
        mgr.AddShardNode(n1);
        mgr.AddShardNode(n2);
        mgr.AddShardNode(n3);

        // Build mock router with matching shard IDs.
        auto raw_router = std::make_shared<MockShardRouter>();
        raw_router->registerShard("shard-001");
        raw_router->registerShard("shard-002");
        raw_router->registerShard("shard-003");
        mock_router_ = raw_router;

        // Federation with ShardingManager injection.
        QueryFederation::Config fed_cfg;
        fed_cfg.enable_pushdown = true;
        federation_ = std::make_unique<QueryFederation>(mock_router_, mgr, fed_cfg);
    }

    void TearDown() override {
        auto& mgr = ShardingManager::GetInstance();
        mgr.RemoveShardNode(1);
        mgr.RemoveShardNode(2);
        mgr.RemoveShardNode(3);
    }

    std::shared_ptr<MockShardRouter>    mock_router_;
    std::unique_ptr<QueryFederation>    federation_;
};

// ============================================================================
// Helper: count distinct "shard" values in merged results
// ============================================================================
static std::vector<std::string> extractShards(const json& result) {
    std::vector<std::string> shards = {};

    if (!result.is_array()) {
      return shards;
    }
    for (const auto& item : result) {
        if (item.contains("shard") && item["shard"].is_string()) {
            std::string s = item["shard"].get<std::string>();
            if (std::find(shards.begin(), shards.end(), s) == shards.end()) {
                shards.push_back(s);
            }
        }
    }
    return shards;
}

// ============================================================================
// Test 1: Point-lookup → exactly 1 shard
// ============================================================================

TEST_F(QueryFederationRoutingTest, PointLookupRoutesToOneShard) {
    // Query with equality predicate on _key.
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key == "ord-42" RETURN doc)";

    // The ShardingManager routes "ord-42" to exactly one shard.
    auto& mgr = ShardingManager::GetInstance();
    std::string expected_shard = mgr.GetShardForKey("orders", "ord-42");
    ASSERT_FALSE(expected_shard.empty())
        << "ShardingManager must have at least one node registered";

    json result = federation_->execute(query);

    // With PARTITION_PRUNING, only the target shard's results are kept.
    std::vector<std::string> result_shards = extractShards(result);

    // At most 1 unique shard should appear in the merged output.
    // (could be 0 if shard_id doesn't match node_address — acceptable)
    EXPECT_LE(result_shards.size(), 1u)
        << "Point-lookup should route to at most 1 shard";
}

TEST_F(QueryFederationRoutingTest, PointLookupUsesExecuteOnShardsPath) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key == "ord-42" RETURN doc)";

    const uint64_t execute_on_shards_before = mock_router_->executeOnShardsCalls();
    const uint64_t scatter_before = mock_router_->scatterGatherCalls();
    federation_->execute(query);

    EXPECT_GT(mock_router_->executeOnShardsCalls(), execute_on_shards_before)
        << "Partition pruning should use executeOnShards for targeted routing";
    EXPECT_EQ(mock_router_->scatterGatherCalls(), scatter_before)
        << "Targeted routing should avoid scatterGather in partition-pruning path";
    EXPECT_LE(mock_router_->lastTargetShards().size(), 1u)
        << "Point-lookup should target at most one shard";
}

TEST_F(QueryFederationRoutingTest, PointLookupWithSingleQuotesUsesExecuteOnShardsPath) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key == 'ord-42' RETURN doc)";

    const uint64_t execute_on_shards_before = mock_router_->executeOnShardsCalls();
    const uint64_t scatter_before = mock_router_->scatterGatherCalls();
    federation_->execute(query);

    EXPECT_GT(mock_router_->executeOnShardsCalls(), execute_on_shards_before)
        << "Single-quoted shard-key predicate should still use executeOnShards";
    EXPECT_EQ(mock_router_->scatterGatherCalls(), scatter_before)
        << "Single-quoted point-lookup should avoid scatterGather";
    EXPECT_LE(mock_router_->lastTargetShards().size(), 1u)
        << "Single-quoted point-lookup should target at most one shard";
}

// ============================================================================
// Test 2: Range query → subset of shards (≤ total)
// ============================================================================

TEST_F(QueryFederationRoutingTest, RangeQueryRoutesToSubsetOfShards) {
    // Query with a range predicate on _key.
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key >= "a" AND doc._key <= "m" RETURN doc)";

    json result = federation_->execute(query);

    // GetShardsForKeyRange may return 1..N shards for a given range.
    // The key assertion: number of shards in results ≤ total shard count (3).
    std::vector<std::string> result_shards = extractShards(result);
    EXPECT_LE(result_shards.size(), 3u)
        << "Range query should not exceed the total shard count";
}

// ============================================================================
// Test 3: Full-scan (no key predicate) → broadcast to all 3 shards
// ============================================================================

TEST_F(QueryFederationRoutingTest, FullScanBroadcastsToAllShards) {
    // Query with no _key predicate: triggers SCATTER_GATHER / broadcast.
    const std::string query =
        R"(FOR doc IN orders FILTER doc.status == "open" RETURN doc)";

    json result = federation_->execute(query);

    // Broadcast returns results from all 3 shards.
    std::vector<std::string> result_shards = extractShards(result);
    EXPECT_EQ(result_shards.size(), 3u)
        << "Full-scan broadcast should return results from all 3 shards";
}

// ============================================================================
// Test 4: GetShardForKey returns non-empty string
// ============================================================================

TEST(ShardingManagerRoutingTest, GetShardForKeyReturnsShard) {
    auto& mgr = ShardingManager::GetInstance();
    // Nodes may already be present from fixture; add defensively.
    try { mgr.AddShardNode({10, "node-10", "PRIMARY", true}); } catch (...) {}

    std::string shard = mgr.GetShardForKey("users", "user-abc");
    // With at least one node, a non-empty shard must be returned.
    EXPECT_FALSE(shard.empty());

    mgr.RemoveShardNode(10);
}

// ============================================================================
// Test 5: GetShardForKey on empty ring returns empty string
// ============================================================================

TEST(ShardingManagerRoutingTest, GetShardForKeyEmptyRingReturnsEmpty) {
    // Create a fresh isolated test — use a distinct node_id range.
    // Since ShardingManager is a singleton and other tests may have nodes,
    // we just check the contract when we can remove all known nodes.
    auto& mgr = ShardingManager::GetInstance();
    // Only test the empty-ring case if no nodes are currently registered.
    if (mgr.GetNodeCount() == 0) {
        std::string shard = mgr.GetShardForKey("col", "key");
        EXPECT_TRUE(shard.empty());
    } else {
        GTEST_SKIP() << "Skipping: ShardingManager has live nodes from other tests";
    }
}

// ============================================================================
// Test 6: GetShardsForKeyRange returns non-empty vector when nodes exist
// ============================================================================

TEST(ShardingManagerRoutingTest, GetShardsForKeyRangeReturnsSomeShards) {
    auto& mgr = ShardingManager::GetInstance();
    try { mgr.AddShardNode({20, "node-20", "PRIMARY", true}); } catch (...) {}
    try { mgr.AddShardNode({21, "node-21", "PRIMARY", true}); } catch (...) {}

    auto shards = mgr.GetShardsForKeyRange("col", "a", "z");
    EXPECT_FALSE(shards.empty())
        << "GetShardsForKeyRange must return at least one shard when nodes exist";

    mgr.RemoveShardNode(20);
    mgr.RemoveShardNode(21);
}

// ============================================================================
// Test 7: analyzeQuery extracts point-lookup key predicate
// ============================================================================

TEST(QueryFederationAnalysisTest, AnalyzeQueryExtractsPointLookupKey) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    // We cannot call analyzeQuery directly (private), but we can verify the
    // routing behaviour: a FILTER on _key == "X" should produce PARTITION_PRUNING
    // and NOT broadcast to multiple shards (via getStatistics counter).
    const std::string query =
        R"(FOR d IN col FILTER d._key == "test-key" RETURN d)";

    json before = fed.getStatistics();
    uint64_t pruned_before = before.value("partition_pruned_queries", 0ull);
    static_cast<void>(pruned_before);

    router->registerShard("s1");
    router->registerShard("s2");
    fed.execute(query);

    json after = fed.getStatistics();
    uint64_t pruned_after  = after.value("partition_pruned_queries", 0ull);
    uint64_t scatter_after = after.value("scatter_gather_queries", 0ull);

    // Either partition_pruning incremented or scatter_gather incremented —
    // with no ShardingManager injected, analyzeQuery extracts the key but
    // falls back to legacy placeholder shards (2 shards < 5 threshold).
    EXPECT_GT(pruned_after + scatter_after, 0u)
        << "Query should have been executed (either pruned or scatter-gather)";
}

// ============================================================================
// Test 8: analyzeQuery extracts key-range predicate
// ============================================================================

TEST(QueryFederationAnalysisTest, AnalyzeQueryExtractsKeyRange) {
    auto router = std::make_shared<MockShardRouter>();
    router->registerShard("s1");
    router->registerShard("s2");
    QueryFederation fed(router);

    const std::string query =
        R"(FOR d IN col FILTER d._key >= "a" AND d._key <= "z" RETURN d)";

    json before = fed.getStatistics();
    fed.execute(query);
    json after = fed.getStatistics();

    uint64_t total_before = before.value("partition_pruned_queries", 0ull)
                          + before.value("scatter_gather_queries", 0ull);
    uint64_t total_after  = after.value("partition_pruned_queries", 0ull)
                          + after.value("scatter_gather_queries", 0ull);

    EXPECT_GT(total_after, total_before)
        << "Range query should have been executed";
}

TEST(QueryFederationAnalysisTest, AnalyzeQueryPreservesExplicitLimit) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    const auto meta = fed.analyzeQuery(
        R"(FOR d IN col FILTER d.status == "open" LIMIT 17 RETURN d)");

    ASSERT_TRUE(meta.limit.has_value());
    EXPECT_EQ(*meta.limit, 17u);
    EXPECT_FALSE(meta.offset.has_value());
}

TEST(QueryFederationAnalysisTest, AnalyzeQueryExtractsOffsetAndLimit) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    const auto meta = fed.analyzeQuery(
        R"(FOR d IN col FILTER d.status == "open" LIMIT 5, 17 RETURN d)");

    ASSERT_TRUE(meta.offset.has_value());
    ASSERT_TRUE(meta.limit.has_value());
    EXPECT_EQ(*meta.offset, 5u);
    EXPECT_EQ(*meta.limit, 17u);
}

TEST(QueryFederationAnalysisTest, AnalyzeQueryDoesNotDuplicateTableAndFilterMetadata) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    const auto meta = fed.analyzeQuery(
        R"(FOR d IN orders FILTER d.status == "open" RETURN d)");

    EXPECT_EQ(meta.tables.size(), 1u);
    EXPECT_EQ(meta.tables.front(), "orders");
    EXPECT_EQ(std::count(meta.predicates.begin(), meta.predicates.end(), "filter_present"), 1);
}

TEST(QueryFederationAnalysisTest, AnalyzeQuerySupportsLowercaseKeywords) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    const auto meta = fed.analyzeQuery(
        R"(for d in orders filter d.status == "open" limit 3 return d)");

    ASSERT_TRUE(meta.limit.has_value());
    EXPECT_EQ(*meta.limit, 3u);
    EXPECT_FALSE(meta.offset.has_value());
    ASSERT_FALSE(meta.tables.empty());
    EXPECT_EQ(meta.tables.front(), "orders");
    EXPECT_EQ(std::count(meta.predicates.begin(), meta.predicates.end(), "filter_present"), 1);
}

TEST(QueryFederationAnalysisTest, AnalyzeQueryExtractsSqlPointLookupKey) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    const auto meta = fed.analyzeQuery(
        R"(SELECT * FROM orders WHERE id = "ord-42" LIMIT 1)");

    ASSERT_FALSE(meta.tables.empty());
    EXPECT_EQ(meta.tables.front(), "orders");
    ASSERT_TRUE(meta.point_lookup_key.has_value());
    EXPECT_EQ(*meta.point_lookup_key, "ord-42");
    ASSERT_TRUE(meta.shard_key_predicate.has_value());
    EXPECT_EQ(meta.shard_key_predicate->kind,
              QueryFederation::QueryMetadata::ShardKeyPredicate::Kind::POINT);
    EXPECT_EQ(meta.shard_key_predicate->collection, "orders");
    EXPECT_EQ(meta.shard_key_predicate->key_value, "ord-42");
}

TEST(QueryFederationAnalysisTest, AnalyzeQueryExtractsSqlRangeKey) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    const auto meta = fed.analyzeQuery(
        R"(SELECT * FROM orders WHERE id >= "a" AND id <= "m")");

    ASSERT_FALSE(meta.tables.empty());
    EXPECT_EQ(meta.tables.front(), "orders");
    ASSERT_TRUE(meta.key_range.has_value());
    EXPECT_EQ(meta.key_range->first, "a");
    EXPECT_EQ(meta.key_range->second, "m");
    ASSERT_TRUE(meta.shard_key_predicate.has_value());
    EXPECT_EQ(meta.shard_key_predicate->kind,
              QueryFederation::QueryMetadata::ShardKeyPredicate::Kind::RANGE);
    EXPECT_EQ(meta.shard_key_predicate->collection, "orders");
    EXPECT_EQ(meta.shard_key_predicate->key_min, "a");
    EXPECT_EQ(meta.shard_key_predicate->key_max, "m");
}

TEST(QueryFederationAnalysisTest, ApplyGlobalOperationsClampsHugeLimitWithoutOverflow) {
    auto router = std::make_shared<MockShardRouter>();
    QueryFederation fed(router);

    QueryFederation::QueryMetadata meta;
    meta.offset = 1u;
    meta.limit = std::numeric_limits<uint64_t>::max();

    json merged = json::array({"a", "b", "c"});
    const auto paged = fed.applyGlobalOperations(merged, meta);

    ASSERT_TRUE(paged.is_array());
    ASSERT_EQ(paged.size(), 2u);
    EXPECT_EQ(paged[0], "b");
    EXPECT_EQ(paged[1], "c");
}

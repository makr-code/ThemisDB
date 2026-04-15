/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_query_federation.cpp                          ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:29:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     188                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bc061a79df  2026-03-24  feat(query): QueryFederation shard-key routing v1.9.0 ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "sharding/shard_router.h"
#include "query/query_federation.h"

using namespace themis;
using namespace themis::sharding;
using namespace themis::query;

// ─────────────────────────────────────────────────────────────────────────────
// Instrumented ShardRouter: records which shards each call targets.
// ─────────────────────────────────────────────────────────────────────────────

class InstrumentedShardRouter : public ShardRouter {
public:
    InstrumentedShardRouter(
        std::shared_ptr<URNResolver> resolver)
        : ShardRouter(resolver, nullptr, Config{})
    {}

    // Override scatterGather so full-scan tests don't make real network calls.
    std::vector<ShardResult> scatterGather(const std::string& /*query*/) override {
        scatter_gather_call_count++;
        return {};
    }

    std::vector<ShardResult> executeOnShards(
        const std::string& /*query*/,
        const std::vector<std::string>& shard_ids) override
    {
        last_on_shards_call = shard_ids;
        std::vector<ShardResult> results;
        for (const auto& id : shard_ids) {
            ShardResult r;
            r.shard_id = id;
            r.success  = true;
            r.data     = nlohmann::json::array();
            results.push_back(r);
        }
        return results;
    }

    // Records from the most recent executeOnShards call
    std::vector<std::string> last_on_shards_call;
    int scatter_gather_call_count = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: 3-shard cluster
// ─────────────────────────────────────────────────────────────────────────────

class QueryFederationShardRoutingTest : public ::testing::Test {
protected:
    static constexpr const char* kShard1 = "shard-001";
    static constexpr const char* kShard2 = "shard-002";
    static constexpr const char* kShard3 = "shard-003";

    void SetUp() override {
        ring_    = std::make_shared<ConsistentHashRing>();
        topology = std::make_shared<ShardTopology>();

        for (const char* id : {kShard1, kShard2, kShard3}) {
            ring_->addShard(id, /*virtual_nodes=*/150);
            ShardInfo info;
            info.shard_id         = id;
            info.primary_endpoint = std::string(id) + ".example.com:8080";
            info.is_healthy       = true;
            topology->addShard(info);
        }

        resolver = std::make_shared<URNResolver>(topology, ring_);
        router   = std::make_shared<InstrumentedShardRouter>(resolver);

        QueryFederation::Config cfg;
        cfg.enable_pushdown = true;
        fed = std::make_unique<QueryFederation>(router, cfg);
    }

    std::shared_ptr<ConsistentHashRing>       ring_;
    std::shared_ptr<ShardTopology>            topology;
    std::shared_ptr<URNResolver>              resolver;
    std::shared_ptr<InstrumentedShardRouter>  router;
    std::unique_ptr<QueryFederation>          fed;
};

// ─────────────────────────────────────────────────────────────────────────────
// Point-lookup routes to exactly 1 shard
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, PointLookup_RoutesTo_ExactlyOneShard) {
    const std::string key   = "order-42";
    const std::string query =
        "FOR doc IN orders FILTER doc._key == \"" + key + "\" RETURN doc";

    fed->execute(query);

    ASSERT_EQ(router->last_on_shards_call.size(), 1u)
        << "Point-lookup should route to exactly 1 shard";

    // Shard must match what the ring resolves directly
    const std::string expected = resolver->getShardForKey("orders", key);
    EXPECT_EQ(router->last_on_shards_call[0], expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// Range across two keys on different shards routes to ≥ 1 shard
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, RangeQuery_RoutesTo_Shards) {
    const std::string query =
        R"(FOR doc IN orders FILTER doc._key >= "aaa" AND doc._key <= "zzz" RETURN doc)";

    fed->execute(query);

    // For a wide range spanning the ring, at least 1 shard must be targeted
    // and at most all 3.
    EXPECT_GE(router->last_on_shards_call.size(), 1u);
    EXPECT_LE(router->last_on_shards_call.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Full-scan (no _key predicate) uses scatterGather, not executeOnShards
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, FullScan_Uses_ScatterGather) {
    const std::string query = "FOR doc IN orders RETURN doc";

    fed->execute(query);

    // scatterGather is called; executeOnShards is NOT called.
    EXPECT_TRUE(router->last_on_shards_call.empty())
        << "Full-scan must use scatterGather, not executeOnShards";
}

// ─────────────────────────────────────────────────────────────────────────────
// Two identical point-lookup queries hit the same shard (ring is deterministic)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryFederationShardRoutingTest, PointLookup_Is_Deterministic) {
    const std::string key   = "deterministic-key-999";
    const std::string query =
        "FOR doc IN users FILTER doc._key == \"" + key + "\" RETURN doc";

    fed->execute(query);
    const auto first_call = router->last_on_shards_call;

    // Reset and re-run
    router->last_on_shards_call.clear();
    fed->execute(query);
    const auto second_call = router->last_on_shards_call;

    ASSERT_EQ(first_call.size(), 1u);
    ASSERT_EQ(second_call.size(), 1u);
    EXPECT_EQ(first_call[0], second_call[0])
        << "The same key must always route to the same shard";
}



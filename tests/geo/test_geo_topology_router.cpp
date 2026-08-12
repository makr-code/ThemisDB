// Unit tests for GeoTopologyRouter (include/network/geo_topology_router.h).
//
// Validates configuration defaults, routing strategy selection, locality
// scoring, cross-region fallback, per-region endpoint selection, statistics
// tracking, and edge cases (null topology, no healthy shards).

#include <gtest/gtest.h>

#include "network/geo_topology_router.h"
#include "sharding/shard_topology.h"

#include <set>
#include <string>

using namespace themis::network;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Test constants
// ─────────────────────────────────────────────────────────────────────────────

/// Wire protocol port used in test endpoints.
constexpr uint16_t kTestPort = 8766;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static ShardInfo makeShard(const std::string& id,
                           const std::string& region,
                           const std::string& zone,
                           const std::string& datacenter,
                           bool               healthy    = true,
                           const std::string& endpoint   = "")
{
    ShardInfo s;
    s.shard_id        = id;
    s.region          = region;
    s.zone            = zone;
    s.datacenter      = datacenter;
    s.is_healthy      = healthy;
    s.primary_endpoint = endpoint.empty()
                             ? (id + ":" + std::to_string(kTestPort))
                             : endpoint;
    return s;
}

// Build a topology pre-populated with a representative multi-region cluster.
//
//   us-east-1a / dc1 : shard-ue1 (healthy)
//   us-east-1b / dc1 : shard-ue2 (healthy)
//   eu-west-1a / dc2 : shard-ew1 (healthy)
//   ap-south-1a/ dc3 : shard-ap1 (healthy)
//   us-east-1a / dc1 : shard-ue3 (unhealthy)
static std::shared_ptr<ShardTopology> buildTopology()
{
    auto topo = std::make_shared<ShardTopology>();
    topo->addShard(makeShard("shard-ue1", "us-east", "us-east-1a", "dc1", true,
                             "us-east-node1:8766"));
    topo->addShard(makeShard("shard-ue2", "us-east", "us-east-1b", "dc1", true,
                             "us-east-node2:8766"));
    topo->addShard(makeShard("shard-ew1", "eu-west", "eu-west-1a", "dc2", true,
                             "eu-west-node1:8766"));
    topo->addShard(makeShard("shard-ap1", "ap-south", "ap-south-1a", "dc3", true,
                             "ap-south-node1:8766"));
    topo->addShard(makeShard("shard-ue3", "us-east", "us-east-1a", "dc1", false,
                             "us-east-node3:8766"));
    return topo;
}

// ─────────────────────────────────────────────────────────────────────────────
// Configuration defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, DefaultConfigEmptyRegion) {
    GeoTopologyRouter::Config cfg;
    EXPECT_TRUE(cfg.local_region.empty());
}

TEST(GeoTopologyRouterTest, DefaultConfigEmptyZone) {
    GeoTopologyRouter::Config cfg;
    EXPECT_TRUE(cfg.local_zone.empty());
}

TEST(GeoTopologyRouterTest, DefaultConfigEmptyDatacenter) {
    GeoTopologyRouter::Config cfg;
    EXPECT_TRUE(cfg.local_datacenter.empty());
}

TEST(GeoTopologyRouterTest, DefaultConfigStrategyPreferLocal) {
    GeoTopologyRouter::Config cfg;
    EXPECT_EQ(cfg.strategy, GeoTopologyRouter::Strategy::PREFER_LOCAL);
}

TEST(GeoTopologyRouterTest, DefaultConfigFallbackEnabled) {
    GeoTopologyRouter::Config cfg;
    EXPECT_TRUE(cfg.fallback_cross_region);
}

TEST(GeoTopologyRouterTest, DefaultConfigNoLatencyHints) {
    GeoTopologyRouter::Config cfg;
    EXPECT_TRUE(cfg.region_latency_hints.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Initial statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, InitialStatsAllZero) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    GeoTopologyRouter router(cfg, buildTopology());

    const auto s = router.getStats();
    EXPECT_EQ(s.requests_routed,        0u);
    EXPECT_EQ(s.local_region_hits,      0u);
    EXPECT_EQ(s.cross_region_fallbacks, 0u);
    EXPECT_EQ(s.routing_failures,       0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: PREFER_LOCAL
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, PreferLocalReturnsLocalRegionEndpoint) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.strategy     = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, buildTopology());
    const std::string ep = router.selectEndpoint();

    // Should return one of the two healthy us-east nodes.
    EXPECT_TRUE(ep == "us-east-node1:8766" || ep == "us-east-node2:8766")
        << "Unexpected endpoint: " << ep;
}

TEST(GeoTopologyRouterTest, PreferLocalFallsBackToRemoteWhenLocalUnavailable) {
    auto topo = std::make_shared<ShardTopology>();
    topo->addShard(makeShard("shard-ew1", "eu-west", "eu-west-1a", "dc2", true,
                             "eu-west-node1:8766"));

    GeoTopologyRouter::Config cfg;
    cfg.local_region         = "us-east";  // no us-east shards in this topology
    cfg.fallback_cross_region = true;
    cfg.strategy             = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, topo);
    const std::string ep = router.selectEndpoint();
    EXPECT_EQ(ep, "eu-west-node1:8766");

    const auto s = router.getStats();
    EXPECT_EQ(s.requests_routed,        1u);
    EXPECT_EQ(s.local_region_hits,      0u);
    EXPECT_EQ(s.cross_region_fallbacks, 1u);
}

TEST(GeoTopologyRouterTest, FallbackDisabledBlocksCrossRegionRouting) {
    // Only remote shards available; fallback_cross_region = false must
    // cause selectEndpoint() to return empty and count a routing_failure.
    auto topo = std::make_shared<ShardTopology>();
    topo->addShard(makeShard("shard-ew1", "eu-west", "eu-west-1a", "dc2", true,
                             "eu-west-node1:8766"));

    GeoTopologyRouter::Config cfg;
    cfg.local_region          = "us-east";
    cfg.fallback_cross_region = false;
    cfg.strategy              = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, topo);
    const std::string ep = router.selectEndpoint();
    EXPECT_TRUE(ep.empty());

    const auto s = router.getStats();
    EXPECT_EQ(s.requests_routed,   0u);
    EXPECT_EQ(s.routing_failures,  1u);
}

TEST(GeoTopologyRouterTest, FallbackDisabledDoesNotBlockLocalRouting) {
    // Local shards are available; fallback_cross_region = false must not
    // interfere with normal local routing.
    GeoTopologyRouter::Config cfg;
    cfg.local_region          = "us-east";
    cfg.fallback_cross_region = false;
    cfg.strategy              = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, buildTopology());
    const std::string ep = router.selectEndpoint();

    EXPECT_TRUE(ep == "us-east-node1:8766" || ep == "us-east-node2:8766")
        << "Unexpected endpoint: " << ep;

    const auto s = router.getStats();
    EXPECT_EQ(s.requests_routed,   1u);
    EXPECT_EQ(s.local_region_hits, 1u);
    EXPECT_EQ(s.routing_failures,  0u);
}

TEST(GeoTopologyRouterTest, PreferLocalZoneAffinityChoosesSameZone) {
    auto topo = std::make_shared<ShardTopology>();
    // Two us-east shards in different zones; local node is in zone us-east-1b.
    topo->addShard(makeShard("shard-a", "us-east", "us-east-1a", "dc1", true,
                             "node-a:8766"));
    topo->addShard(makeShard("shard-b", "us-east", "us-east-1b", "dc1", true,
                             "node-b:8766"));

    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.local_zone   = "us-east-1b";
    cfg.strategy     = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, topo);
    EXPECT_EQ(router.selectEndpoint(), "node-b:8766");
}

TEST(GeoTopologyRouterTest, PreferLocalDatacenterAffinityChoosesSameDc) {
    auto topo = std::make_shared<ShardTopology>();
    // Two us-east shards in different datacenters; no zone info.
    topo->addShard(
        makeShard("shard-a", "us-east", "", "dc1", true, "node-a:8766"));
    topo->addShard(
        makeShard("shard-b", "us-east", "", "dc2", true, "node-b:8766"));

    GeoTopologyRouter::Config cfg;
    cfg.local_region      = "us-east";
    cfg.local_datacenter  = "dc2";
    cfg.strategy          = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, topo);
    EXPECT_EQ(router.selectEndpoint(), "node-b:8766");
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: LOWEST_LATENCY
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, LowestLatencySelectsRegionWithSmallestHint) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.strategy     = GeoTopologyRouter::Strategy::LOWEST_LATENCY;
    cfg.region_latency_hints = {
        {"us-east",  5},
        {"eu-west", 80},
        {"ap-south", 200},
    };

    GeoTopologyRouter router(cfg, buildTopology());
    const std::string ep = router.selectEndpoint();

    // us-east has the lowest latency; both us-east nodes are valid.
    EXPECT_TRUE(ep == "us-east-node1:8766" || ep == "us-east-node2:8766")
        << "Unexpected endpoint: " << ep;
}

TEST(GeoTopologyRouterTest, LowestLatencySelectsRemoteWhenItHasLowerHint) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.strategy     = GeoTopologyRouter::Strategy::LOWEST_LATENCY;
    cfg.region_latency_hints = {
        {"us-east", 150},
        {"eu-west",  20},  // eu-west is "closer" according to hints
    };

    GeoTopologyRouter router(cfg, buildTopology());
    const std::string ep = router.selectEndpoint();
    EXPECT_EQ(ep, "eu-west-node1:8766");

    const auto s = router.getStats();
    EXPECT_EQ(s.cross_region_fallbacks, 1u);
}

TEST(GeoTopologyRouterTest, LowestLatencyFallsBackToPreferLocalWithNoHints) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.strategy     = GeoTopologyRouter::Strategy::LOWEST_LATENCY;
    // No latency hints – must fall back to PREFER_LOCAL behaviour.

    GeoTopologyRouter router(cfg, buildTopology());
    const std::string ep = router.selectEndpoint();
    // PREFER_LOCAL would pick a us-east shard.
    EXPECT_TRUE(ep == "us-east-node1:8766" || ep == "us-east-node2:8766")
        << "Unexpected endpoint: " << ep;
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: ROUND_ROBIN
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, RoundRobinDistributesAcrossAllHealthyShards) {
    GeoTopologyRouter::Config cfg;
    cfg.strategy = GeoTopologyRouter::Strategy::ROUND_ROBIN;

    auto topo = std::make_shared<ShardTopology>();
    topo->addShard(makeShard("s1", "us-east", "", "", true, "node1:8766"));
    topo->addShard(makeShard("s2", "eu-west", "", "", true, "node2:8766"));
    topo->addShard(makeShard("s3", "ap-south", "", "", true, "node3:8766"));

    GeoTopologyRouter router(cfg, topo);

    // After exactly 3 calls each shard should have been selected once.
    std::set<std::string> seen;
    for (int i = 0; i < 3; ++i) {
        seen.insert(router.selectEndpoint());
    }
    EXPECT_EQ(seen.size(), 3u);
    EXPECT_TRUE(seen.count("node1:8766"));
    EXPECT_TRUE(seen.count("node2:8766"));
    EXPECT_TRUE(seen.count("node3:8766"));
}

TEST(GeoTopologyRouterTest, RoundRobinIgnoresLocalRegionConfig) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.strategy     = GeoTopologyRouter::Strategy::ROUND_ROBIN;

    auto topo = std::make_shared<ShardTopology>();
    topo->addShard(makeShard("s1", "eu-west", "", "", true, "eu-node:8766"));

    GeoTopologyRouter router(cfg, topo);
    EXPECT_EQ(router.selectEndpoint(), "eu-node:8766");
}

// ─────────────────────────────────────────────────────────────────────────────
// selectEndpointInRegion
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, SelectEndpointInRegionReturnsCorrectRegion) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    GeoTopologyRouter router(cfg, buildTopology());

    EXPECT_EQ(router.selectEndpointInRegion("eu-west"), "eu-west-node1:8766");
}

TEST(GeoTopologyRouterTest, SelectEndpointInRegionReturnsEmptyForUnknownRegion) {
    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, buildTopology());

    EXPECT_TRUE(router.selectEndpointInRegion("unknown-region").empty());
    EXPECT_EQ(router.getStats().routing_failures, 1u);
}

TEST(GeoTopologyRouterTest, SelectEndpointInLocalRegionCountsAsLocalHit) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    GeoTopologyRouter router(cfg, buildTopology());

    const std::string ep = router.selectEndpointInRegion("us-east");
    EXPECT_FALSE(ep.empty());

    const auto s = router.getStats();
    EXPECT_EQ(s.local_region_hits,      1u);
    EXPECT_EQ(s.cross_region_fallbacks, 0u);
}

TEST(GeoTopologyRouterTest, SelectEndpointInRemoteRegionCountsAsCrossRegion) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    GeoTopologyRouter router(cfg, buildTopology());

    const std::string ep = router.selectEndpointInRegion("eu-west");
    EXPECT_FALSE(ep.empty());

    const auto s = router.getStats();
    EXPECT_EQ(s.cross_region_fallbacks, 1u);
    EXPECT_EQ(s.local_region_hits,      0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// getRankedShards
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, GetRankedShardsOnlyHealthyShards) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    GeoTopologyRouter router(cfg, buildTopology());

    const auto ranked = router.getRankedShards();

    // shard-ue3 is unhealthy and must not appear.
    for (const auto& s : ranked) {
        EXPECT_TRUE(s.is_healthy) << "Unhealthy shard in ranked list: " << s.shard_id;
        EXPECT_NE(s.shard_id, "shard-ue3");
    }
}

TEST(GeoTopologyRouterTest, GetRankedShardsPreferLocalPlacesLocalFirst) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "eu-west";
    cfg.strategy     = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, buildTopology());
    const auto ranked = router.getRankedShards();

    ASSERT_FALSE(ranked.empty());
    EXPECT_EQ(ranked.front().region, "eu-west");
}

TEST(GeoTopologyRouterTest, GetRankedShardsLowestLatencyOrdersByHint) {
    GeoTopologyRouter::Config cfg;
    cfg.strategy = GeoTopologyRouter::Strategy::LOWEST_LATENCY;
    cfg.region_latency_hints = {
        {"ap-south",  10},
        {"us-east",   50},
        {"eu-west",  120},
    };

    GeoTopologyRouter router(cfg, buildTopology());
    const auto ranked = router.getRankedShards();

    ASSERT_FALSE(ranked.empty());
    EXPECT_EQ(ranked.front().region, "ap-south");
}

TEST(GeoTopologyRouterTest, GetRankedShardsRoundRobinNoGuaranteedOrder) {
    GeoTopologyRouter::Config cfg;
    cfg.strategy = GeoTopologyRouter::Strategy::ROUND_ROBIN;

    GeoTopologyRouter router(cfg, buildTopology());
    const auto ranked = router.getRankedShards();

    // All 4 healthy shards must be present; order is unspecified.
    EXPECT_EQ(ranked.size(), 4u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge cases
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, SelectEndpointWithNullTopologyReturnsEmpty) {
    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, nullptr);

    EXPECT_TRUE(router.selectEndpoint().empty());
    EXPECT_EQ(router.getStats().routing_failures, 1u);
}

TEST(GeoTopologyRouterTest, SelectEndpointWithNoHealthyShardsReturnsEmpty) {
    auto topo = std::make_shared<ShardTopology>();
    topo->addShard(makeShard("s1", "us-east", "", "", false, "node1:8766"));

    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, topo);

    EXPECT_TRUE(router.selectEndpoint().empty());
    EXPECT_EQ(router.getStats().routing_failures, 1u);
}

TEST(GeoTopologyRouterTest, SelectEndpointInRegionWithNullTopologyReturnsEmpty) {
    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, nullptr);

    EXPECT_TRUE(router.selectEndpointInRegion("us-east").empty());
    EXPECT_EQ(router.getStats().routing_failures, 1u);
}

TEST(GeoTopologyRouterTest, GetRankedShardsWithNullTopologyReturnsEmpty) {
    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, nullptr);

    EXPECT_TRUE(router.getRankedShards().empty());
}

TEST(GeoTopologyRouterTest, GetRankedShardsEmptyTopologyReturnsEmpty) {
    auto topo = std::make_shared<ShardTopology>();

    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, topo);

    EXPECT_TRUE(router.getRankedShards().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics accumulation
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, StatsAccumulateAcrossMultipleSelections) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region = "us-east";
    cfg.strategy     = GeoTopologyRouter::Strategy::PREFER_LOCAL;

    GeoTopologyRouter router(cfg, buildTopology());

    for (int i = 0; i < 5; ++i) {
        router.selectEndpoint();
    }

    const auto s = router.getStats();
    EXPECT_EQ(s.requests_routed,   5u);
    EXPECT_EQ(s.local_region_hits, 5u);  // all land in local us-east region
}

TEST(GeoTopologyRouterTest, StatsFailuresDoNotCountAsRouted) {
    GeoTopologyRouter::Config cfg;
    GeoTopologyRouter router(cfg, nullptr);

    router.selectEndpoint();
    router.selectEndpoint();

    const auto s = router.getStats();
    EXPECT_EQ(s.requests_routed,   0u);
    EXPECT_EQ(s.routing_failures,  2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// getConfig accessor
// ─────────────────────────────────────────────────────────────────────────────

TEST(GeoTopologyRouterTest, GetConfigReturnsConstructionConfig) {
    GeoTopologyRouter::Config cfg;
    cfg.local_region      = "ap-south";
    cfg.local_zone        = "ap-south-1a";
    cfg.local_datacenter  = "dc3";
    cfg.strategy          = GeoTopologyRouter::Strategy::LOWEST_LATENCY;
    cfg.fallback_cross_region = false;

    GeoTopologyRouter router(cfg, buildTopology());

    EXPECT_EQ(router.getConfig().local_region,      "ap-south");
    EXPECT_EQ(router.getConfig().local_zone,        "ap-south-1a");
    EXPECT_EQ(router.getConfig().local_datacenter,  "dc3");
    EXPECT_EQ(router.getConfig().strategy,
              GeoTopologyRouter::Strategy::LOWEST_LATENCY);
    EXPECT_FALSE(router.getConfig().fallback_cross_region);
}

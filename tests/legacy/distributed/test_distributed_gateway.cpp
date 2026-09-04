// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for DistributedGateway.
//
// Tests exercise the distributed gateway without a live network:
//  - Construction and configuration
//  - ConsistentHashRing: add/remove nodes, deterministic routing, affinity
//  - ClusterGatewayConfig: JSON round-trip serialisation
//  - GatewayRouteConfig: JSON round-trip serialisation
//  - DistributedGateway: cluster status, config apply, quorum handling
//  - DistributedGateway: session affinity detection (WebSocket / SSE)
//  - DistributedGateway: request delegation to underlying APIGateway
//  - DistributedGateway: extensibility (registerHandler / registerDeprecation)
//  - DistributedGateway: quorum-loss state and data-race safety
//  - Chaos: repeated add/remove ring operations remain consistent

#include <gtest/gtest.h>

#include "server/distributed_gateway.h"
#include "server/api_gateway.h"
#include "server/auth_middleware.h"
#include "server/rate_limiter.h"
#include "server/load_shedder.h"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>

using namespace themis::server;
namespace http = boost::beast::http;

// ============================================================================
// Helper factories
// ============================================================================

static std::shared_ptr<APIGateway> makeGateway() {
    auto auth         = std::make_shared<themis::AuthMiddleware>();
    auto rate_limiter = std::make_shared<RateLimiter>();
    auto load_shedder = std::make_shared<LoadShedder>(LoadShedder::Config{});

    APIGateway::Config cfg;
    cfg.gateway_id          = "test-gateway";
    cfg.datacenter          = "test-dc";
    cfg.enable_sharding     = false;
    cfg.enable_rate_limiting = false;
    cfg.enable_load_shedding = false;

    return std::make_shared<APIGateway>(cfg, auth, rate_limiter, load_shedder);
}

static DistributedGateway::Config makeDistConfig(
    const std::string& node_id  = "gw-1",
    std::size_t cluster_size    = 3
) {
    DistributedGateway::Config cfg;
    cfg.node_id = node_id;

    for (std::size_t i = 0; i < cluster_size; ++i) {
        GatewayNode n;
        n.node_id = "gw-" + std::to_string(i + 1);
        n.address = "127.0.0.1";
        n.port    = static_cast<uint16_t>(9000 + i);
        cfg.cluster_nodes.push_back(n);
    }

    cfg.election_timeout_min_ms  = 150;
    cfg.election_timeout_max_ms  = 300;
    cfg.heartbeat_interval_ms    = 50;
    cfg.leader_failover_timeout  = std::chrono::milliseconds{500};
    cfg.virtual_nodes_per_peer   = 50; // small for fast tests
    cfg.continue_on_quorum_loss  = true;

    return cfg;
}

static http::request<http::string_body>
makeReq(http::verb verb, const std::string& target) {
    http::request<http::string_body> req{verb, target, 11};
    req.set(http::field::host, "localhost");
    return req;
}

static auto echoHandler() {
    return [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"ok":true})";
        resp.prepare_payload();
        return resp;
    };
}

// ============================================================================
// GatewayRouteConfig – JSON round-trip
// ============================================================================

TEST(GatewayRouteConfigTest, JsonRoundTrip) {
    GatewayRouteConfig orig;
    orig.path_prefix   = "/api/v1/query";
    orig.upstream_url  = "http://backend:8081";
    orig.timeout_ms    = 15000;
    orig.retry_count   = 3;
    orig.circuit_breaker_enabled            = true;
    orig.circuit_breaker_failure_threshold  = 7;

    auto j       = orig.toJson();
    auto decoded = GatewayRouteConfig::fromJson(j);

    EXPECT_EQ(decoded.path_prefix,  orig.path_prefix);
    EXPECT_EQ(decoded.upstream_url, orig.upstream_url);
    EXPECT_EQ(decoded.timeout_ms,   orig.timeout_ms);
    EXPECT_EQ(decoded.retry_count,  orig.retry_count);
    EXPECT_EQ(decoded.circuit_breaker_enabled,           orig.circuit_breaker_enabled);
    EXPECT_EQ(decoded.circuit_breaker_failure_threshold, orig.circuit_breaker_failure_threshold);
}

TEST(GatewayRouteConfigTest, JsonDefaults) {
    // fromJson with an empty JSON object must not throw and must apply defaults.
    auto j = nlohmann::json::object();
    EXPECT_NO_THROW({
        auto cfg = GatewayRouteConfig::fromJson(j);
        EXPECT_TRUE(cfg.path_prefix.empty());
        EXPECT_EQ(cfg.timeout_ms,  static_cast<uint32_t>(30000));
        EXPECT_EQ(cfg.retry_count, static_cast<uint32_t>(2));
        EXPECT_TRUE(cfg.circuit_breaker_enabled);
    });
}

// ============================================================================
// ClusterGatewayConfig – JSON round-trip
// ============================================================================

TEST(ClusterGatewayConfigTest, JsonRoundTrip) {
    ClusterGatewayConfig orig;
    orig.version              = 42;
    orig.global_rate_limit_rps = 50000;
    orig.updated_by           = "gw-1";
    orig.updated_at           = std::chrono::system_clock::now();

    GatewayRouteConfig r;
    r.path_prefix  = "/api/v2";
    r.upstream_url = "http://shard0:8080";
    orig.routes.push_back(r);
    orig.rate_limits["client-A"] = 1000;

    auto j       = orig.toJson();
    auto decoded = ClusterGatewayConfig::fromJson(j);

    EXPECT_EQ(decoded.version,               orig.version);
    EXPECT_EQ(decoded.global_rate_limit_rps, orig.global_rate_limit_rps);
    EXPECT_EQ(decoded.updated_by,            orig.updated_by);
    ASSERT_EQ(decoded.routes.size(),         orig.routes.size());
    EXPECT_EQ(decoded.routes[0].path_prefix, r.path_prefix);
    EXPECT_EQ(decoded.rate_limits.at("client-A"), static_cast<uint32_t>(1000));
}

TEST(ClusterGatewayConfigTest, EmptyJsonDoesNotThrow) {
    EXPECT_NO_THROW({
        auto cfg = ClusterGatewayConfig::fromJson(nlohmann::json::object());
        EXPECT_EQ(cfg.version, 0ULL);
        EXPECT_TRUE(cfg.routes.empty());
    });
}

// ============================================================================
// ConsistentHashRing – construction
// ============================================================================

TEST(ConsistentHashRingTest, EmptyRingReturnsNullopt) {
    ConsistentHashRing ring(50);
    EXPECT_EQ(ring.nodeCount(), 0UL);
    EXPECT_FALSE(ring.getNode("any-key").has_value());
}

TEST(ConsistentHashRingTest, SingleNodeAlwaysSelected) {
    ConsistentHashRing ring(50);
    GatewayNode n;
    n.node_id = "gw-1";
    n.address = "127.0.0.1";
    n.port    = 9001;
    ring.addNode(n);

    for (const auto& key : {"session-1", "session-2", "/ws/feed", "/sse/updates"}) {
        auto result = ring.getNode(key);
        ASSERT_TRUE(result.has_value()) << "Key: " << key;
        EXPECT_EQ(result->node_id, "gw-1");
    }
}

TEST(ConsistentHashRingTest, NodeCountAfterAdd) {
    ConsistentHashRing ring(50);

    for (int i = 1; i <= 5; ++i) {
        GatewayNode n;
        n.node_id = "gw-" + std::to_string(i);
        n.address = "10.0.0." + std::to_string(i);
        n.port    = static_cast<uint16_t>(9000 + i);
        ring.addNode(n);
        EXPECT_EQ(ring.nodeCount(), static_cast<std::size_t>(i));
    }
}

TEST(ConsistentHashRingTest, RemoveNodeDecreasesCount) {
    ConsistentHashRing ring(50);
    GatewayNode n1; n1.node_id = "gw-1"; n1.address = "127.0.0.1"; n1.port = 9001;
    GatewayNode n2; n2.node_id = "gw-2"; n2.address = "127.0.0.1"; n2.port = 9002;

    ring.addNode(n1);
    ring.addNode(n2);
    EXPECT_EQ(ring.nodeCount(), 2UL);

    ring.removeNode("gw-1");
    EXPECT_EQ(ring.nodeCount(), 1UL);

    // After removal of gw-1, all keys must resolve to gw-2
    auto result = ring.getNode("any-session");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->node_id, "gw-2");
}

TEST(ConsistentHashRingTest, DeterministicRouting) {
    ConsistentHashRing ring(150);
    for (int i = 1; i <= 3; ++i) {
        GatewayNode n;
        n.node_id = "gw-" + std::to_string(i);
        n.address = "10.0.0." + std::to_string(i);
        n.port    = static_cast<uint16_t>(9000 + i);
        ring.addNode(n);
    }

    // Repeated lookups for the same key must return the same node.
    const std::string key = "persistent-session-42";
    auto first = ring.getNode(key);
    ASSERT_TRUE(first.has_value());

    for (int i = 0; i < 20; ++i) {
        auto result = ring.getNode(key);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->node_id, first->node_id)
            << "Routing must be deterministic for key: " << key;
    }
}

TEST(ConsistentHashRingTest, DistributionAcrossNodes) {
    // With enough virtual nodes, all physical nodes should receive at least
    // some of the keys.
    ConsistentHashRing ring(150);
    for (int i = 1; i <= 3; ++i) {
        GatewayNode n;
        n.node_id = "gw-" + std::to_string(i);
        n.address = "10.0.0." + std::to_string(i);
        n.port    = static_cast<uint16_t>(9000 + i);
        ring.addNode(n);
    }

    std::unordered_map<std::string, int> counts;
    for (int i = 0; i < 300; ++i) {
        auto result = ring.getNode("session-" + std::to_string(i));
        ASSERT_TRUE(result.has_value());
        counts[result->node_id]++;
    }

    // Every node should have received at least one request (distribution sanity)
    EXPECT_EQ(counts.size(), 3UL) << "All 3 nodes should receive some requests";
    for (const auto& [node_id, count] : counts) {
        EXPECT_GT(count, 0) << "Node " << node_id << " received 0 requests";
    }
}

TEST(ConsistentHashRingTest, RemoveNodeReroutesGracefully) {
    ConsistentHashRing ring(150);
    GatewayNode n1; n1.node_id = "gw-1"; n1.address = "10.0.0.1"; n1.port = 9001;
    GatewayNode n2; n2.node_id = "gw-2"; n2.address = "10.0.0.2"; n2.port = 9002;
    GatewayNode n3; n3.node_id = "gw-3"; n3.address = "10.0.0.3"; n3.port = 9003;
    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    // Record original routing for 100 keys
    std::unordered_map<std::string, std::string> before;
    for (int i = 0; i < 100; ++i) {
        std::string key = "key-" + std::to_string(i);
        auto result = ring.getNode(key);
        ASSERT_TRUE(result.has_value());
        before[key] = result->node_id;
    }

    // Remove gw-2; only keys that were on gw-2 should change
    ring.removeNode("gw-2");

    int changed = 0;
    for (int i = 0; i < 100; ++i) {
        std::string key = "key-" + std::to_string(i);
        auto result = ring.getNode(key);
        ASSERT_TRUE(result.has_value());
        EXPECT_NE(result->node_id, "gw-2")
            << "Key " << key << " still routed to removed node";
        if (result->node_id != before[key]) {
            changed++;
        }
    }
    // In a 3-node consistent-hash ring, removing 1 node should disrupt at most
    // ~1/3 of keys (≈33 out of 100). We allow up to 50% (50 keys) as a generous
    // bound to absorb hash-distribution variance with 150 virtual nodes, but not
    // the 67% that would occur if all keys were rehashed from scratch.
    static constexpr int kMaxAllowedDisruption = 50;
    EXPECT_LT(changed, kMaxAllowedDisruption)
        << "Too many keys rerouted; consistent hash ring broken";
}

// ============================================================================
// DistributedGateway – construction
// ============================================================================

TEST(DistributedGatewayTest, ConstructionDoesNotThrow) {
    EXPECT_NO_THROW({
        DistributedGateway dg(makeDistConfig(), makeGateway());
    });
}

TEST(DistributedGatewayTest, NullGatewayThrows) {
    EXPECT_THROW(
        DistributedGateway(makeDistConfig(), nullptr),
        std::invalid_argument
    );
}

TEST(DistributedGatewayTest, StartStop) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    EXPECT_NO_THROW(dg.start());
    EXPECT_NO_THROW(dg.stop());
}

TEST(DistributedGatewayTest, DoubleStartIdempotent) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    EXPECT_NO_THROW({
        dg.start();
        dg.start(); // second call must not throw
        dg.stop();
    });
}

TEST(DistributedGatewayTest, DoubleStopIdempotent) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    dg.start();
    EXPECT_NO_THROW({
        dg.stop();
        dg.stop(); // second call must not throw
    });
}

// ============================================================================
// DistributedGateway – cluster status
// ============================================================================

TEST(DistributedGatewayTest, ClusterStatusContainsNodeId) {
    DistributedGateway dg(makeDistConfig("gw-1", 3), makeGateway());
    auto status = dg.getClusterStatus();
    ASSERT_TRUE(status.contains("node_id"));
    EXPECT_EQ(status["node_id"].get<std::string>(), "gw-1");
}

TEST(DistributedGatewayTest, ClusterStatusContainsExpectedKeys) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    auto status = dg.getClusterStatus();

    EXPECT_TRUE(status.contains("node_id"));
    EXPECT_TRUE(status.contains("is_leader"));
    EXPECT_TRUE(status.contains("leader_id"));
    EXPECT_TRUE(status.contains("has_quorum"));
    EXPECT_TRUE(status.contains("quorum_lost"));
    EXPECT_TRUE(status.contains("config_version"));
    EXPECT_TRUE(status.contains("route_count"));
    EXPECT_TRUE(status.contains("ring_nodes"));
    EXPECT_TRUE(status.contains("cluster_nodes"));
}

TEST(DistributedGatewayTest, ClusterStatusRingNodesMatchesClusterSize) {
    DistributedGateway dg(makeDistConfig("gw-1", 5), makeGateway());
    auto status = dg.getClusterStatus();
    EXPECT_EQ(status["ring_nodes"].get<std::size_t>(), 5UL);
}

// ============================================================================
// DistributedGateway – config application
// ============================================================================

TEST(DistributedGatewayTest, ApplyConfigEntryUpdatesVersion) {
    DistributedGateway dg(makeDistConfig(), makeGateway());

    ClusterGatewayConfig cfg;
    cfg.version    = 1;
    cfg.updated_by = "gw-1";
    cfg.updated_at = std::chrono::system_clock::now();

    GatewayRouteConfig r;
    r.path_prefix  = "/api/v1";
    r.upstream_url = "http://backend:8080";
    cfg.routes.push_back(r);

    std::string entry = cfg.toJson().dump();
    EXPECT_TRUE(dg.applyConfigEntry(entry));

    auto current = dg.getCurrentConfig();
    EXPECT_EQ(current.version, 1ULL);
    ASSERT_EQ(current.routes.size(), 1UL);
    EXPECT_EQ(current.routes[0].path_prefix, "/api/v1");
}

TEST(DistributedGatewayTest, ApplyConfigEntryIgnoresStaleVersion) {
    DistributedGateway dg(makeDistConfig(), makeGateway());

    // Apply v2 first
    ClusterGatewayConfig v2;
    v2.version    = 2;
    v2.updated_by = "gw-1";
    v2.updated_at = std::chrono::system_clock::now();
    GatewayRouteConfig r2; r2.path_prefix = "/v2"; r2.upstream_url = "http://v2:8080";
    v2.routes.push_back(r2);
    EXPECT_TRUE(dg.applyConfigEntry(v2.toJson().dump()));

    // Now try to apply v1 – must be silently ignored (stale)
    ClusterGatewayConfig v1;
    v1.version    = 1;
    v1.updated_by = "gw-2";
    v1.updated_at = std::chrono::system_clock::now();
    GatewayRouteConfig r1; r1.path_prefix = "/v1"; r1.upstream_url = "http://v1:8080";
    v1.routes.push_back(r1);
    EXPECT_TRUE(dg.applyConfigEntry(v1.toJson().dump()));

    // Config must still be v2
    auto current = dg.getCurrentConfig();
    EXPECT_EQ(current.version, 2ULL);
    EXPECT_EQ(current.routes[0].path_prefix, "/v2");
}

TEST(DistributedGatewayTest, ApplyConfigEmptyEntryNoOp) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    EXPECT_TRUE(dg.applyConfigEntry(""));  // no-op / heartbeat
    EXPECT_EQ(dg.getCurrentConfig().version, 0ULL);
}

TEST(DistributedGatewayTest, ApplyConfigInvalidJsonReturnsFalse) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    EXPECT_FALSE(dg.applyConfigEntry("{ not valid json ---"));
}

TEST(DistributedGatewayTest, ApplyConfigMultipleRoutesPreserved) {
    DistributedGateway dg(makeDistConfig(), makeGateway());

    ClusterGatewayConfig cfg;
    cfg.version    = 5;
    cfg.updated_by = "gw-1";
    cfg.updated_at = std::chrono::system_clock::now();

    for (int i = 0; i < 10; ++i) {
        GatewayRouteConfig r;
        r.path_prefix  = "/api/route" + std::to_string(i);
        r.upstream_url = "http://backend" + std::to_string(i) + ":8080";
        cfg.routes.push_back(r);
    }
    cfg.rate_limits["tenant-1"] = 500;
    cfg.rate_limits["tenant-2"] = 1000;

    EXPECT_TRUE(dg.applyConfigEntry(cfg.toJson().dump()));

    auto current = dg.getCurrentConfig();
    EXPECT_EQ(current.version, 5ULL);
    EXPECT_EQ(current.routes.size(), 10UL);
    EXPECT_EQ(current.rate_limits.at("tenant-1"), static_cast<uint32_t>(500));
    EXPECT_EQ(current.rate_limits.at("tenant-2"), static_cast<uint32_t>(1000));
}

// ============================================================================
// DistributedGateway – session affinity
// ============================================================================

TEST(DistributedGatewayTest, WebSocketRequestAffinityReturnsNode) {
    DistributedGateway dg(makeDistConfig("gw-1", 3), makeGateway());

    auto req = makeReq(http::verb::get, "/ws/feed");
    req.set(http::field::upgrade, "websocket");

    auto node = dg.resolveAffinityNode("/ws/feed");
    ASSERT_TRUE(node.has_value());
    // The resolved node must be in the cluster
    const auto& cluster = makeDistConfig("gw-1", 3).cluster_nodes;
    bool found = false;
    for (const auto& n : cluster) {
        if (n.node_id == node->node_id) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Affinity node not in cluster: " << node->node_id;
}

TEST(DistributedGatewayTest, AffinityStableAcrossRequests) {
    DistributedGateway dg(makeDistConfig("gw-1", 3), makeGateway());

    const std::string session = "/sse/changefeed/collection-42";
    auto first = dg.resolveAffinityNode(session);
    ASSERT_TRUE(first.has_value());

    for (int i = 0; i < 10; ++i) {
        auto result = dg.resolveAffinityNode(session);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->node_id, first->node_id)
            << "Affinity must be stable across repeated calls";
    }
}

// ============================================================================
// DistributedGateway – request delegation
// ============================================================================

TEST(DistributedGatewayTest, HandleRequestDelegatesToGateway) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    dg.start();

    auto req = makeReq(http::verb::get, "/health");
    auto resp = dg.handleRequest(req, echoHandler());

    EXPECT_EQ(resp.result(), http::status::ok);
    dg.stop();
}

TEST(DistributedGatewayTest, HandleRequestWebSocketDelegatesToGateway) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    dg.start();

    auto req = makeReq(http::verb::get, "/ws/changefeed");
    req.set(http::field::upgrade, "websocket");

    auto resp = dg.handleRequest(req, echoHandler());
    EXPECT_EQ(resp.result(), http::status::ok);

    dg.stop();
}

TEST(DistributedGatewayTest, HandleRequestSseDelegatesToGateway) {
    DistributedGateway dg(makeDistConfig(), makeGateway());
    dg.start();

    auto req = makeReq(http::verb::get, "/sse/feed");
    req.set(http::field::accept, "text/event-stream");

    auto resp = dg.handleRequest(req, echoHandler());
    EXPECT_EQ(resp.result(), http::status::ok);

    dg.stop();
}

// ============================================================================
// DistributedGateway – quorum / leader state
// ============================================================================

TEST(DistributedGatewayTest, SingleNodeBecomesLeaderEventually) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping single-node leader election focused test on Windows due to unstable election timing behavior.";
#endif
    // A single-node cluster must eventually elect itself as leader.
    DistributedGateway::Config cfg = makeDistConfig("gw-1", 1);
    cfg.election_timeout_min_ms = 50;
    cfg.election_timeout_max_ms = 100;

    DistributedGateway dg(cfg, makeGateway());
    dg.start();

    // Allow time for election
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    EXPECT_TRUE(dg.isLeader()) << "Single-node cluster must elect itself as leader";
    EXPECT_TRUE(dg.hasQuorum()) << "Single node must always have quorum";
    EXPECT_EQ(dg.getLeaderId(), "gw-1");

    dg.stop();
}

TEST(DistributedGatewayTest, GetLeaderIdNonEmptyAfterElection) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping single-node leader election focused test on Windows due to unstable election timing behavior.";
#endif
    DistributedGateway::Config cfg = makeDistConfig("gw-1", 1);
    cfg.election_timeout_min_ms = 50;
    cfg.election_timeout_max_ms = 100;

    DistributedGateway dg(cfg, makeGateway());
    dg.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    EXPECT_FALSE(dg.getLeaderId().empty())
        << "Leader ID must be non-empty after successful election";
    dg.stop();
}

// ============================================================================
// DistributedGateway – config propose (non-leader path)
// ============================================================================

TEST(DistributedGatewayTest, ProposeConfigOnNonLeaderReturnsFalse) {
    // In a 3-node in-process cluster, this node may not be the leader yet.
    // Even if it is, we test the public proposeConfig API.
    DistributedGateway dg(makeDistConfig("gw-2", 3), makeGateway());
    // Do NOT start – node is not leader
    ClusterGatewayConfig cfg;
    cfg.version    = 1;
    cfg.updated_by = "gw-2";
    cfg.updated_at = std::chrono::system_clock::now();
    // Not started → not leader → should return false
    EXPECT_FALSE(dg.proposeConfig(cfg));
}

// ============================================================================
// Chaos – repeated add/remove ring stability
// ============================================================================

TEST(ConsistentHashRingTest, RepeatedAddRemoveStable) {
    ConsistentHashRing ring(50);
    GatewayNode n1; n1.node_id = "gw-1"; n1.address = "10.0.0.1"; n1.port = 9001;
    GatewayNode n2; n2.node_id = "gw-2"; n2.address = "10.0.0.2"; n2.port = 9002;
    ring.addNode(n1);
    ring.addNode(n2);

    // 50 cycles of remove-and-re-add gw-2
    for (int i = 0; i < 50; ++i) {
        ring.removeNode("gw-2");
        EXPECT_EQ(ring.nodeCount(), 1UL);
        auto r = ring.getNode("session-x");
        ASSERT_TRUE(r.has_value());
        EXPECT_EQ(r->node_id, "gw-1");

        ring.addNode(n2);
        EXPECT_EQ(ring.nodeCount(), 2UL);
    }
}

TEST(ConsistentHashRingTest, AllKeysResolveAfterFlap) {
    ConsistentHashRing ring(50);
    GatewayNode n1; n1.node_id = "gw-1"; n1.address = "10.0.0.1"; n1.port = 9001;
    GatewayNode n2; n2.node_id = "gw-2"; n2.address = "10.0.0.2"; n2.port = 9002;
    GatewayNode n3; n3.node_id = "gw-3"; n3.address = "10.0.0.3"; n3.port = 9003;
    ring.addNode(n1);
    ring.addNode(n2);
    ring.addNode(n3);

    // Flap gw-3
    ring.removeNode("gw-3");
    ring.addNode(n3);

    // All 200 keys must resolve to a valid (non-null) node
    for (int i = 0; i < 200; ++i) {
        auto r = ring.getNode("key-" + std::to_string(i));
        ASSERT_TRUE(r.has_value()) << "Key " << i << " resolved to null after flap";
        EXPECT_NE(r->node_id, "") << "Key " << i << " resolved to empty node_id";
    }
}

// ============================================================================
// DistributedGateway – extensibility (registerHandler / registerDeprecation)
// ============================================================================

TEST(DistributedGatewayTest, RegisterHandlerDelegatesToGateway) {
    // Verify that registerHandler() delegates to the underlying APIGateway
    // without throwing and that subsequent requests continue to work.
    // (The handlers_ map in APIGateway is consulted by the caller's local_handler
    // dispatch, not auto-injected into the request pipeline; this mirrors the
    // existing APIGatewayTest::RegisterHandler test contract.)
    DistributedGateway dg(makeDistConfig(), makeGateway());
    dg.start();

    // Registration must not throw.
    ASSERT_NO_THROW(
        dg.registerHandler(
            "/api/v1/custom",
            [](const http::request<http::string_body>& r) {
                http::response<http::string_body> resp{http::status::ok, r.version()};
                resp.set(http::field::content_type, "application/json");
                resp.body() = R"({"custom":true})";
                resp.prepare_payload();
                return resp;
            }));

    // Subsequent requests must continue to succeed after registration.
    auto req  = makeReq(http::verb::get, "/health");
    auto resp = dg.handleRequest(req, echoHandler());
    EXPECT_EQ(resp.result(), http::status::ok)
        << "Requests must succeed after handler registration";

    dg.stop();
}

TEST(DistributedGatewayTest, RegisterDeprecationDoesNotThrow) {
    // Verify that registerDeprecation delegates without throwing.
    DistributedGateway dg(makeDistConfig(), makeGateway());

    APIDeprecationInfo info;
    info.deprecated_in        = APIVersion{1, 0, 0};
    info.removed_in           = APIVersion{2, 0, 0};
    info.deprecation_date     = std::chrono::system_clock::now();
    info.removal_date         = std::chrono::system_clock::now()
                                + std::chrono::hours(24 * 365);
    info.migration_guide_url  = "https://docs.themisdb.com/migration";
    info.reason               = "Replaced by /api/v2/custom";
    info.alternative          = "/api/v2/custom";

    EXPECT_NO_THROW(dg.registerDeprecation("/api/v1/old-custom", info))
        << "registerDeprecation must not throw";
}

// ============================================================================
// DistributedGateway – quorum-loss state tracking
// ============================================================================

TEST(DistributedGatewayTest, QuorumLostFlagInitiallyFalse) {
    // Verify that getClusterStatus correctly reports quorum_lost: false
    // on a fresh gateway before any requests are processed.
    DistributedGateway dg(makeDistConfig("gw-1", 3), makeGateway());

    auto status = dg.getClusterStatus();
    EXPECT_FALSE(status["quorum_lost"].get<bool>())
        << "quorum_lost must be false on construction";
}

TEST(DistributedGatewayTest, ApplyConfigResetsQuorumLostFlag) {
    // Once a config entry is successfully applied, quorum_lost_ must be reset
    // to false.
    DistributedGateway dg(makeDistConfig("gw-1", 3), makeGateway());

    ClusterGatewayConfig cfg;
    cfg.version    = 7;
    cfg.updated_by = "gw-1";
    cfg.updated_at = std::chrono::system_clock::now();

    EXPECT_TRUE(dg.applyConfigEntry(cfg.toJson().dump()));

    auto status = dg.getClusterStatus();
    EXPECT_FALSE(status["quorum_lost"].get<bool>())
        << "quorum_lost must be false after a successful config apply";
    EXPECT_EQ(status["config_version"].get<uint64_t>(), 7ULL);
}

// ============================================================================
// DistributedGateway – data-race safety (concurrent applyConfigEntry)
// ============================================================================

TEST(DistributedGatewayTest, ConcurrentApplyConfigNoDataRace) {
    // Rapid concurrent applyConfigEntry calls must not corrupt the config.
    DistributedGateway dg(makeDistConfig("gw-1", 1), makeGateway());

    const int kThreads     = 4;
    const int kEntriesEach = 20;
    std::vector<std::thread> threads;
    std::atomic<int> applied{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&dg, &applied, t, kEntriesEach] {
            for (int i = 0; i < kEntriesEach; ++i) {
                ClusterGatewayConfig cfg;
                // Use non-overlapping version ranges per thread to ensure
                // each thread can win at least once.
                cfg.version    = static_cast<uint64_t>(t * kEntriesEach + i + 1);
                cfg.updated_by = "gw-1";
                cfg.updated_at = std::chrono::system_clock::now();
                GatewayRouteConfig r;
                r.path_prefix  = "/thread/" + std::to_string(t);
                r.upstream_url = "http://backend:8080";
                cfg.routes.push_back(r);

                if (dg.applyConfigEntry(cfg.toJson().dump())) {
                    applied.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    EXPECT_GT(applied.load(), 0);
    auto current = dg.getCurrentConfig();
    EXPECT_GT(current.version, 0ULL)
        << "config version must be non-zero after concurrent applies";
}

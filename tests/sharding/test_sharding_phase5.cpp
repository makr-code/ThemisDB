/**
 * Sharding Phase 5 unit tests:
 *   - Admin API  POST /api/v1/shards/{id}/migrate-hardware  (SHP5-01..04)
 *   - Raft peer-address update  updatePeerAddress()          (SHP5-05..08)
 *   - Drain-period enforcement                               (SHP5-09..12)
 */

#include <gtest/gtest.h>
#include "sharding/admin_api.h"
#include "sharding/hardware_migration_manager.h"
#include "sharding/raft_consensus.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::sharding;
using namespace themisdb::sharding;

// ============================================================================
// Helpers
// ============================================================================

static std::shared_ptr<ConsistentHashRing> makeRing() {
    return std::make_shared<ConsistentHashRing>(3);
}

static std::shared_ptr<ShardTopology> makeTopologyWithShard(
    const std::string& shard_id, const std::string& endpoint) {
    auto topo = std::make_shared<ShardTopology>();
    ShardInfo info;
    info.shard_id         = shard_id;
    info.primary_endpoint = endpoint;
    topo->addShard(info);
    return topo;
}

static std::shared_ptr<HardwareMigrationManager> makeManager(
    std::shared_ptr<ShardTopology> topo,
    std::shared_ptr<ConsistentHashRing> ring,
    std::chrono::seconds drain = std::chrono::seconds{0}) {
    HardwareMigrationConfig cfg;
    cfg.identity_file_path = "/tmp/test_node_identity_phase5.json";
    cfg.drain_period        = drain;
    cfg.verify_ring_stability = false;
    return std::make_shared<HardwareMigrationManager>(cfg, topo, ring);
}

static AdminAPI makeAdminApiForTests() {
    AdminAPI::Config cfg{};
    cfg.require_signatures = false;
    return AdminAPI(cfg);
}

// ============================================================================
// SHP5-01: migrate-hardware returns 501 when no manager is attached
// ============================================================================
TEST(ShardingPhase5, SHP501_MigrateHardwareNoManager) {
    auto api = makeAdminApiForTests();
    auto resp = api.handleRequest("POST",
        "/api/v1/shards/shard-1/migrate-hardware",
        {{"new_endpoint", "10.0.0.2:9090"}},
        "operator-cert");

    EXPECT_FALSE(resp.value("success", true));
    EXPECT_EQ(501, resp["error"]["code"].get<int>());
}

// ============================================================================
// SHP5-02: migrate-hardware returns 400 when new_endpoint is missing
// ============================================================================
TEST(ShardingPhase5, SHP502_MigrateHardwareMissingEndpoint) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");
    ring->addShard("shard-1");

    auto api = makeAdminApiForTests();
    api.setMigrationManager(makeManager(topo, ring));

    auto resp = api.handleRequest("POST",
        "/api/v1/shards/shard-1/migrate-hardware",
        nlohmann::json::object(),   // no new_endpoint
        "operator-cert");

    EXPECT_FALSE(resp.value("success", true));
    EXPECT_EQ(400, resp["error"]["code"].get<int>());
}

// ============================================================================
// SHP5-03: migrate-hardware succeeds and returns old + new endpoint
// ============================================================================
TEST(ShardingPhase5, SHP503_MigrateHardwareSuccess) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");
    ring->addShard("shard-1");

    auto api = makeAdminApiForTests();
    api.setMigrationManager(makeManager(topo, ring));

    auto resp = api.handleRequest("POST",
        "/api/v1/shards/shard-1/migrate-hardware",
        {{"new_endpoint", "10.0.0.2:9090"}},
        "operator-cert");

    ASSERT_TRUE(resp.value("success", false))
        << "Response: " << resp.dump();
    EXPECT_EQ("shard-1",        resp["shard_id"].get<std::string>());
    EXPECT_EQ("10.0.0.1:9090",  resp["old_endpoint"].get<std::string>());
    EXPECT_EQ("10.0.0.2:9090",  resp["new_endpoint"].get<std::string>());
}

// ============================================================================
// SHP5-04: custom handler overrides built-in manager path
// ============================================================================
TEST(ShardingPhase5, SHP504_MigrateHardwareCustomHandler) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");

    auto api = makeAdminApiForTests();
    api.setMigrationManager(makeManager(topo, ring));
    api.registerMigrateHardwareHandler([](const nlohmann::json& body) {
        return nlohmann::json{
            {"success", true},
            {"custom", true},
            {"shard_id", body.value("shard_id", "")}
        };
    });

    auto resp = api.handleRequest("POST",
        "/api/v1/shards/shard-1/migrate-hardware",
        {{"new_endpoint", "10.0.0.2:9090"}},
        "operator-cert");

    EXPECT_TRUE(resp.value("success", false));
    EXPECT_TRUE(resp.value("custom", false));
}

// ============================================================================
// SHP5-05: updatePeerAddress updates endpoint of existing peer
// ============================================================================
TEST(ShardingPhase5, SHP505_UpdatePeerAddressExistingPeer) {
    RaftConsensus::Config cfg;
    cfg.raft_config.node_id = "leader-1";
    RaftConsensus raft(cfg);
    raft.addReplicaNode("peer-1");

    // Verify initial state has empty endpoint
    auto states = raft.getReplicaStates();
    ASSERT_FALSE(states.empty());
    auto it = std::find_if(states.begin(), states.end(),
        [](const ReplicaState& s) { return s.node_id == "peer-1"; });
    ASSERT_NE(states.end(), it);
    EXPECT_EQ("", it->endpoint);

    // Update the endpoint
    raft.updatePeerAddress("peer-1", "10.0.1.2:5000");

    states = raft.getReplicaStates();
    it = std::find_if(states.begin(), states.end(),
        [](const ReplicaState& s) { return s.node_id == "peer-1"; });
    ASSERT_NE(states.end(), it);
    EXPECT_EQ("10.0.1.2:5000", it->endpoint);
}

// ============================================================================
// SHP5-06: updatePeerAddress is a no-op for unknown peer (no crash)
// ============================================================================
TEST(ShardingPhase5, SHP506_UpdatePeerAddressUnknownPeer) {
    RaftConsensus::Config cfg;
    cfg.raft_config.node_id = "leader-1";
    RaftConsensus raft(cfg);

    // Must not throw/crash
    EXPECT_NO_THROW(raft.updatePeerAddress("does-not-exist", "10.0.0.99:1234"));
}

// ============================================================================
// SHP5-07: updatePeerAddress updates only the named peer
// ============================================================================
TEST(ShardingPhase5, SHP507_UpdatePeerAddressIsolated) {
    RaftConsensus::Config cfg;
    cfg.raft_config.node_id = "leader-1";
    RaftConsensus raft(cfg);
    raft.addReplicaNode("peer-A");
    raft.addReplicaNode("peer-B");

    raft.updatePeerAddress("peer-A", "new-host:7000");

    auto states = raft.getReplicaStates();
    for (const auto& s : states) {
        if (s.node_id == "peer-A") {
            EXPECT_EQ("new-host:7000", s.endpoint);
        } else if (s.node_id == "peer-B") {
            EXPECT_EQ("", s.endpoint) << "peer-B should be untouched";
        }
    }
}

// ============================================================================
// SHP5-08: updatePeerAddress survives consecutive updates
// ============================================================================
TEST(ShardingPhase5, SHP508_UpdatePeerAddressConsecutive) {
    RaftConsensus::Config cfg;
    cfg.raft_config.node_id = "leader-1";
    RaftConsensus raft(cfg);
    raft.addReplicaNode("peer-1");

    raft.updatePeerAddress("peer-1", "addr-v1:1111");
    raft.updatePeerAddress("peer-1", "addr-v2:2222");

    auto states = raft.getReplicaStates();
    auto it = std::find_if(states.begin(), states.end(),
        [](const ReplicaState& s) { return s.node_id == "peer-1"; });
    ASSERT_NE(states.end(), it);
    EXPECT_EQ("addr-v2:2222", it->endpoint);
}

// ============================================================================
// SHP5-09: inFlightCount starts at 0
// ============================================================================
TEST(ShardingPhase5, SHP509_DrainCountInitiallyZero) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");
    auto mgr  = makeManager(topo, ring);

    EXPECT_EQ(0u, mgr->inFlightCount("shard-1"));
    EXPECT_EQ(0u, mgr->inFlightCount("non-existent"));
}

// ============================================================================
// SHP5-10: addInFlightRequest / releaseInFlightRequest balance correctly
// ============================================================================
TEST(ShardingPhase5, SHP510_DrainCountIncrementDecrement) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");
    auto mgr  = makeManager(topo, ring);

    mgr->addInFlightRequest("shard-1");
    mgr->addInFlightRequest("shard-1");
    EXPECT_EQ(2u, mgr->inFlightCount("shard-1"));

    mgr->releaseInFlightRequest("shard-1");
    EXPECT_EQ(1u, mgr->inFlightCount("shard-1"));

    mgr->releaseInFlightRequest("shard-1");
    EXPECT_EQ(0u, mgr->inFlightCount("shard-1"));
}

// ============================================================================
// SHP5-11: DrainGuard RAII correctly decrements on destruction
// ============================================================================
TEST(ShardingPhase5, SHP511_DrainGuardRAII) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");
    auto mgr  = makeManager(topo, ring);

    {
        auto guard = mgr->makeRequestGuard("shard-1");
        EXPECT_EQ(1u, mgr->inFlightCount("shard-1"));
    } // guard destroyed here
    EXPECT_EQ(0u, mgr->inFlightCount("shard-1"));
}

// ============================================================================
// SHP5-12: waitForDrain returns true when timeout=0 (no-wait mode)
// ============================================================================
TEST(ShardingPhase5, SHP512_WaitForDrainZeroTimeout) {
    auto ring = makeRing();
    auto topo = makeTopologyWithShard("shard-1", "10.0.0.1:9090");
    auto mgr  = makeManager(topo, ring);

    mgr->addInFlightRequest("shard-1");
    // timeout=0 → skip waiting entirely
    bool drained = mgr->waitForDrain("shard-1", std::chrono::seconds{0});
    EXPECT_TRUE(drained);
    mgr->releaseInFlightRequest("shard-1");
}

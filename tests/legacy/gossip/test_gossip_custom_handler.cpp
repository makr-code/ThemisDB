// Copyright 2026 ThemisDB — Licensed under MIT License
// DK-2 / S-5: GossipProtocol::registerCustomHandler unit tests
// CC-4: GossipProtocol::setRaftMembershipGateFn unit tests
//
// Tests:
//   GP-CUSTOM-01  registerCustomHandler() dispatches correctly for a matching message_type
//   GP-CUSTOM-02  handleMessage() does NOT invoke a handler for an unregistered message_type
//   GP-CUSTOM-03  Duplicate registration overwrites the previous handler (last-wins)
//   GP-GATE-01    Without a gate, a gossip-discovered peer is always added to the topology
//   GP-GATE-02    With a gate that returns true, the peer is added to the topology
//   GP-GATE-03    With a gate that returns false, the peer is NOT added to the topology
//   GP-GATE-04    Removing the gate (nullptr) reverts to the legacy warn+add behaviour

#include <gtest/gtest.h>
#include "sharding/gossip_protocol.h"
#include "sharding/shard_topology.h"
#include <atomic>
#include <chrono>
#include <future>
#include <memory>

using namespace themis::sharding;

// ---------------------------------------------------------------------------
// Helper – build a GossipMessage with a current timestamp so that the
// replay-protection check in handleMessage() passes.
// ---------------------------------------------------------------------------
static GossipMessage makeMsg(const std::string& type, const std::string& sender = "peer-test") {
    GossipMessage m;
    m.message_id   = "test-msg-id";
    m.sender_id    = sender;
    m.message_type = type;
    m.payload      = nlohmann::json::object();
    m.timestamp    = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return m;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
class GossipCustomHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ShardTopology::Config topo_config;
        topo_config.cluster_name       = "test_cluster";
        topo_config.enable_health_checks = false;
        topology = std::make_shared<ShardTopology>(topo_config);

        GossipConfig config;
        config.enabled               = false;  // don't start background threads
        config.validate_certificates = false;  // skip signature check in tests
        config.max_message_age_sec   = 300;    // generous age window for tests
        config.local_peer_id         = "local-test-peer";
        config.require_mtls          = false;

        protocol = std::make_unique<GossipProtocol>(config, topology);
    }

    std::shared_ptr<ShardTopology>  topology;
    std::unique_ptr<GossipProtocol> protocol;
};

// ---------------------------------------------------------------------------
// GP-CUSTOM-01: handler is invoked for the registered message_type
// ---------------------------------------------------------------------------
TEST_F(GossipCustomHandlerTest, GP_CUSTOM_01_DispatchesForMatchingType) {
    std::atomic<int> call_count{0};
    std::string received_type;

    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage& msg) {
            ++call_count;
            received_type = msg.message_type;
        });

    auto msg = makeMsg("adapter_capability");
    protocol->handleMessage(msg);

    EXPECT_EQ(call_count.load(), 1);
    EXPECT_EQ(received_type, "adapter_capability");
}

// ---------------------------------------------------------------------------
// GP-CUSTOM-02: handler is NOT invoked for an unregistered message_type
// ---------------------------------------------------------------------------
TEST_F(GossipCustomHandlerTest, GP_CUSTOM_02_NoDispatchForUnknownType) {
    std::atomic<int> call_count{0};

    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage&) { ++call_count; });

    auto msg = makeMsg("some_other_type");
    protocol->handleMessage(msg);

    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// GP-CUSTOM-03: duplicate registration overwrites the previous handler
// ---------------------------------------------------------------------------
TEST_F(GossipCustomHandlerTest, GP_CUSTOM_03_DuplicateOverwritesPrevious) {
    std::atomic<int> first_calls{0};
    std::atomic<int> second_calls{0};

    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage&) { ++first_calls; });

    // Second registration for the same type — last handler wins
    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage&) { ++second_calls; });

    auto msg = makeMsg("adapter_capability");
    protocol->handleMessage(msg);

    EXPECT_EQ(first_calls.load(),  0) << "First (overwritten) handler must NOT be called";
    EXPECT_EQ(second_calls.load(), 1) << "Second (active) handler must be called exactly once";
}

// ===========================================================================
// CC-4: Raft Membership Gate tests
//
// These tests verify that gossip-driven topology mutations are properly gated
// behind the Raft membership change protocol via setRaftMembershipGateFn().
// ===========================================================================

// Helper — build a minimal heartbeat GossipMessage from a new peer whose
// peer_id differs from the local node so addPeer() and syncWithTopologyLocked()
// are triggered.
static GossipMessage makeHeartbeatMsg(const std::string& sender_id,
                                      const std::string& endpoint) {
    GossipMessage m;
    m.message_id   = "hb-" + sender_id;
    m.sender_id    = sender_id;
    m.message_type = "heartbeat";
    m.payload      = {
        {"endpoint",    endpoint},
        {"datacenter",  "dc1"},
        {"region",      "us-east"},
        {"version",     uint64_t{1}}
    };
    m.timestamp    = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return m;
}

// ---------------------------------------------------------------------------
// Fixture that creates a fresh protocol + topology per CC-4 test.
// ---------------------------------------------------------------------------
class GossipMembershipGateTest : public ::testing::Test {
protected:
    void SetUp() override {
        ShardTopology::Config topo_config;
        topo_config.cluster_name         = "gate_test_cluster";
        topo_config.enable_health_checks = false;
        topology = std::make_shared<ShardTopology>(topo_config);

        GossipConfig config;
        config.enabled               = false;
        config.validate_certificates = false;
        config.max_message_age_sec   = 300;
        config.local_peer_id         = "local-node";
        config.require_mtls          = false;

        protocol = std::make_unique<GossipProtocol>(config, topology);
    }

    std::shared_ptr<ShardTopology>  topology;
    std::unique_ptr<GossipProtocol> protocol;
};

// GP-GATE-01: Without a gate, any gossip-discovered peer is added to topology.
TEST_F(GossipMembershipGateTest, GP_GATE_01_NoGate_PeerAddedToTopology) {
    // No gate registered — legacy warn+add path.
    protocol->handleMessage(makeHeartbeatMsg("peer-A", "host-a:9000"));
    EXPECT_TRUE(topology->hasShard("peer-A"))
        << "Without a gate, gossip-discovered peers must be added to the topology";
}

// GP-GATE-02: When the gate returns true, the peer IS added to the topology.
TEST_F(GossipMembershipGateTest, GP_GATE_02_GateApproves_PeerAddedToTopology) {
    std::atomic<int> gate_calls{0};
    protocol->setRaftMembershipGateFn([&](const std::string& /*peer_id*/,
                                          const std::string& /*endpoint*/) -> bool {
        ++gate_calls;
        return true;  // Raft approves this peer
    });

    protocol->handleMessage(makeHeartbeatMsg("peer-B", "host-b:9000"));

    EXPECT_EQ(gate_calls.load(), 1) << "Gate must be called exactly once for the new peer";
    EXPECT_TRUE(topology->hasShard("peer-B"))
        << "Gate-approved peers must be added to the topology";
}

// GP-GATE-03: When the gate returns false, the peer is NOT added to topology.
TEST_F(GossipMembershipGateTest, GP_GATE_03_GateDenies_PeerNotAddedToTopology) {
    std::atomic<int> gate_calls{0};
    protocol->setRaftMembershipGateFn([&](const std::string& /*peer_id*/,
                                          const std::string& /*endpoint*/) -> bool {
        ++gate_calls;
        return false;  // Raft denies this rogue peer
    });

    protocol->handleMessage(makeHeartbeatMsg("rogue-peer", "rogue:9000"));

    EXPECT_EQ(gate_calls.load(), 1) << "Gate must be called exactly once for the denied peer";
    EXPECT_FALSE(topology->hasShard("rogue-peer"))
        << "Gate-denied peers must NOT appear in the routing topology";
    // The peer is still tracked for health monitoring inside gossip peers_ map.
    EXPECT_EQ(protocol->getPeerCount(), 1u)
        << "Denied peer must still appear in the internal gossip peer table";
}

// GP-GATE-04: After removing the gate (nullptr), legacy warn+add behaviour is restored.
TEST_F(GossipMembershipGateTest, GP_GATE_04_RemoveGate_LegacyBehaviourRestored) {
    // Install a deny-all gate.
    protocol->setRaftMembershipGateFn(
        [](const std::string&, const std::string&) { return false; });

    // Verify the gate denies peer-C.
    protocol->handleMessage(makeHeartbeatMsg("peer-C", "host-c:9000"));
    EXPECT_FALSE(topology->hasShard("peer-C")) << "Gate must deny peer-C";

    // Remove the gate.
    protocol->setRaftMembershipGateFn(nullptr);

    // A fresh peer (different id) must now be added without the gate.
    protocol->handleMessage(makeHeartbeatMsg("peer-D", "host-d:9000"));
    EXPECT_TRUE(topology->hasShard("peer-D"))
        << "After removing the gate, gossip-discovered peers are added via legacy path";
}

TEST_F(GossipMembershipGateTest, PeerDiscoveryCallbackCanReenterProtocolWithoutDeadlock) {
    protocol->onPeerDiscovered([&](const PeerInfo&) {
        (void)protocol->getPeerCount();
    });

    PeerInfo peer;
    peer.peer_id = "peer-reentrant";
    peer.endpoint = "host-reentrant:9000";
    peer.datacenter = "dc1";
    peer.region = "us-east";
    peer.version = 1;
    peer.last_seen = std::chrono::system_clock::now();
    peer.first_seen = std::chrono::system_clock::now();
    peer.is_healthy = true;

    auto add_future = std::async(std::launch::async, [&]() {
        protocol->addPeer(peer);
    });

    EXPECT_EQ(add_future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    add_future.get();
}

TEST_F(GossipMembershipGateTest, PeerLostCallbackCanReenterProtocolWithoutDeadlock) {
    PeerInfo peer;
    peer.peer_id = "peer-to-remove";
    peer.endpoint = "host-remove:9000";
    peer.datacenter = "dc1";
    peer.region = "us-east";
    peer.version = 1;
    peer.last_seen = std::chrono::system_clock::now();
    peer.first_seen = std::chrono::system_clock::now();
    peer.is_healthy = true;
    protocol->addPeer(peer);

    protocol->onPeerLost([&](const std::string&) {
        (void)protocol->getPeerCount();
    });

    auto remove_future = std::async(std::launch::async, [&]() {
        protocol->removePeer("peer-to-remove");
    });

    EXPECT_EQ(remove_future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    remove_future.get();
}

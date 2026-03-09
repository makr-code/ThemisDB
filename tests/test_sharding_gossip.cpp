/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sharding_gossip.cpp                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-09 04:07:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f24988a56  2026-03-03  test: Wave 3 - add tests for query cache manager, transac... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_sharding_gossip.cpp
 * @brief Tests for gossip-based consensus and discovery modules:
 *
 *   - sharding/gossip_protocol.cpp  : GossipProtocol (construction, config
 *                                     defaults, disabled-mode start/stop)
 *   - sharding/gossip_consensus_adapter.cpp : GossipConsensusAdapter
 *                                     (construction, getState, start/stop)
 *
 * Neither module requires network connectivity:
 * - GossipProtocol is constructed in disabled mode (config.enabled = false)
 * - GossipConsensusAdapter is constructed with a minimal ConsensusConfig;
 *   getState() is tested before start() to validate default state.
 */

#include <gtest/gtest.h>
#include "sharding/gossip_protocol.h"
#include "sharding/gossip_consensus_adapter.h"
#include "sharding/shard_topology.h"
#include "sharding/consensus_module.h"
#include <memory>
#include <string>

using namespace themisdb::sharding;

// ============================================================================
// GossipConfig defaults
// ============================================================================

TEST(GossipConfigTest, Defaults_AreReasonable) {
    GossipConfig cfg;
    EXPECT_FALSE(cfg.enabled);                  // off by default
    EXPECT_GT(cfg.gossip_interval_sec, 0u);
    EXPECT_GT(cfg.max_peers,           0u);
    EXPECT_GT(cfg.fanout,              0u);
    EXPECT_GT(cfg.peer_timeout_sec,    0u);
    EXPECT_TRUE(cfg.seed_nodes.empty());
    EXPECT_TRUE(cfg.require_mtls);
    EXPECT_TRUE(cfg.validate_certificates);
}

// ============================================================================
// GossipProtocol construction
// ============================================================================

class GossipProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        topology_ = std::make_shared<ShardTopology>();

        GossipConfig cfg;
        cfg.enabled = false; // no real network needed
        cfg.gossip_interval_sec = 30;
        cfg.max_peers = 10;

        protocol_ = std::make_unique<GossipProtocol>(cfg, topology_);
    }

    std::shared_ptr<ShardTopology> topology_;
    std::unique_ptr<GossipProtocol> protocol_;
};

TEST_F(GossipProtocolTest, Construction_Succeeds) {
    EXPECT_NE(protocol_, nullptr);
}

TEST_F(GossipProtocolTest, GetPeerCount_InitiallyZero) {
    EXPECT_EQ(protocol_->getPeerCount(), 0u);
}

TEST_F(GossipProtocolTest, IsRunning_BeforeStart_ReturnsFalse) {
    EXPECT_FALSE(protocol_->isRunning());
}

TEST_F(GossipProtocolTest, StartStop_InDisabledMode_DoesNotCrash) {
    // start() with enabled=false should be a no-op
    EXPECT_NO_THROW(protocol_->start());
    EXPECT_NO_THROW(protocol_->stop());
}

// ============================================================================
// GossipConsensusAdapter
// ============================================================================

class GossipConsensusAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConsensusConfig cfg;
        cfg.type          = ConsensusType::GOSSIP;
        cfg.node_id       = "test-node-1";
        cfg.cluster_nodes = {"test-node-1", "test-node-2"};

        adapter_ = std::make_unique<GossipConsensusAdapter>(cfg);
    }

    std::unique_ptr<GossipConsensusAdapter> adapter_;
};

TEST_F(GossipConsensusAdapterTest, Construction_Succeeds) {
    EXPECT_NE(adapter_, nullptr);
}

TEST_F(GossipConsensusAdapterTest, GetState_BeforeStart_IsFollowerOrObserver) {
    auto state = adapter_->getState();
    // Before start() the adapter should be in a passive state
    EXPECT_TRUE(state == ConsensusState::FOLLOWER ||
                state == ConsensusState::OBSERVER);
}

TEST_F(GossipConsensusAdapterTest, StartStop_DoesNotCrash) {
    // start() may fail gracefully when there is no real cluster,
    // but it must not throw
    EXPECT_NO_THROW(adapter_->start());
    EXPECT_NO_THROW(adapter_->stop());
}

TEST_F(GossipConsensusAdapterTest, Stop_WithoutStart_DoesNotCrash) {
    EXPECT_NO_THROW(adapter_->stop());
}

// ============================================================================
// ConsensusConfig defaults
// ============================================================================

TEST(ConsensusConfigTest, Defaults_AreReasonable) {
    ConsensusConfig cfg;
    EXPECT_EQ(cfg.type, ConsensusType::RAFT);
    EXPECT_TRUE(cfg.node_id.empty());
    EXPECT_TRUE(cfg.cluster_nodes.empty());
    EXPECT_GT(cfg.heartbeat_interval.count(),    0);
    EXPECT_GT(cfg.election_timeout_min.count(),  0);
    EXPECT_GT(cfg.election_timeout_max.count(),  0);
    EXPECT_GE(cfg.election_timeout_max.count(),
              cfg.election_timeout_min.count());
}

// ============================================================================
// ConsensusType / ConsensusState enum completeness
// ============================================================================

TEST(ConsensusTypeTest, AllValuesDistinct) {
    EXPECT_NE(ConsensusType::RAFT,       ConsensusType::GOSSIP);
    EXPECT_NE(ConsensusType::RAFT,       ConsensusType::PAXOS);
    EXPECT_NE(ConsensusType::RAFT,       ConsensusType::MULTI_PAXOS);
    EXPECT_NE(ConsensusType::GOSSIP,     ConsensusType::PAXOS);
    EXPECT_NE(ConsensusType::GOSSIP,     ConsensusType::MULTI_PAXOS);
    EXPECT_NE(ConsensusType::PAXOS,      ConsensusType::MULTI_PAXOS);
}

TEST(ConsensusStateTest, AllValuesDistinct) {
    EXPECT_NE(ConsensusState::FOLLOWER,  ConsensusState::CANDIDATE);
    EXPECT_NE(ConsensusState::FOLLOWER,  ConsensusState::LEADER);
    EXPECT_NE(ConsensusState::FOLLOWER,  ConsensusState::OBSERVER);
    EXPECT_NE(ConsensusState::CANDIDATE, ConsensusState::LEADER);
    EXPECT_NE(ConsensusState::CANDIDATE, ConsensusState::OBSERVER);
    EXPECT_NE(ConsensusState::LEADER,    ConsensusState::OBSERVER);
}

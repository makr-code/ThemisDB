// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_raid_paxos.cpp
 * @brief Tests for RAID-Paxos consensus implementation
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * @note Tests RAID-mode-specific quorum calculation and failure tolerance
 */

#include "sharding/raid_paxos_consensus.h"
#include "sharding/raid_paxos_config.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <vector>
#include <string>

namespace themisdb { namespace sharding { 

class RAIDPaxosTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// RAID Configuration Tests
// ============================================================================

TEST_F(RAIDPaxosTest, RAIDModeEnumValues) {
    EXPECT_EQ(static_cast<int>(RAIDMode::STRIPE), 0);
    EXPECT_EQ(static_cast<int>(RAIDMode::MIRROR), 1);
    EXPECT_EQ(static_cast<int>(RAIDMode::PARITY), 5);
    EXPECT_EQ(static_cast<int>(RAIDMode::HYBRID), 10);
}

TEST_F(RAIDPaxosTest, RAIDModeToString) {
    EXPECT_EQ(raidModeToString(RAIDMode::STRIPE), "RAID_0");
    EXPECT_EQ(raidModeToString(RAIDMode::MIRROR), "RAID_1");
    EXPECT_EQ(raidModeToString(RAIDMode::PARITY), "RAID_5");
    EXPECT_EQ(raidModeToString(RAIDMode::HYBRID), "RAID_10");
    EXPECT_EQ(raidModeToString(static_cast<RAIDMode>(99)), "UNKNOWN");
}

TEST_F(RAIDPaxosTest, StringToRAIDMode) {
    EXPECT_EQ(stringToRAIDMode("RAID_0"), RAIDMode::STRIPE);
    EXPECT_EQ(stringToRAIDMode("STRIPE"), RAIDMode::STRIPE);
    EXPECT_EQ(stringToRAIDMode("0"), RAIDMode::STRIPE);
    
    EXPECT_EQ(stringToRAIDMode("RAID_1"), RAIDMode::MIRROR);
    EXPECT_EQ(stringToRAIDMode("MIRROR"), RAIDMode::MIRROR);
    EXPECT_EQ(stringToRAIDMode("1"), RAIDMode::MIRROR);
    
    EXPECT_EQ(stringToRAIDMode("RAID_5"), RAIDMode::PARITY);
    EXPECT_EQ(stringToRAIDMode("PARITY"), RAIDMode::PARITY);
    EXPECT_EQ(stringToRAIDMode("5"), RAIDMode::PARITY);
    
    EXPECT_EQ(stringToRAIDMode("RAID_10"), RAIDMode::HYBRID);
    EXPECT_EQ(stringToRAIDMode("HYBRID"), RAIDMode::HYBRID);
    EXPECT_EQ(stringToRAIDMode("10"), RAIDMode::HYBRID);
    
    // Default to safe mode
    EXPECT_EQ(stringToRAIDMode("INVALID"), RAIDMode::MIRROR);
}

// ============================================================================
// Quorum Calculation Tests
// ============================================================================

TEST_F(RAIDPaxosTest, RAID0QuorumCalculation) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::STRIPE;
    config.allow_raid0 = true;
    
    // RAID 0: All shards must respond (no redundancy)
    EXPECT_EQ(config.calculateQuorumSize(1), 1);
    EXPECT_EQ(config.calculateQuorumSize(2), 2);
    EXPECT_EQ(config.calculateQuorumSize(4), 4);
    EXPECT_EQ(config.calculateQuorumSize(8), 8);
}

TEST_F(RAIDPaxosTest, RAID1QuorumCalculation) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 2;
    
    // RAID 1: Need at least one mirror set
    // With mirror_factor=2: need at least 2 shards
    EXPECT_EQ(config.calculateQuorumSize(2), 2);  // 2/2 + 1 = 2
    EXPECT_EQ(config.calculateQuorumSize(4), 3);  // 4/2 + 1 = 3
    EXPECT_EQ(config.calculateQuorumSize(6), 4);  // 6/2 + 1 = 4
}

TEST_F(RAIDPaxosTest, RAID5QuorumCalculation) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    
    // RAID 5: Need majority including parity
    // For N data + 1 parity: quorum = (N+1)/2 + 1
    EXPECT_EQ(config.calculateQuorumSize(2), 2);   // 1 data + 1 parity: (2/2)+1 = 2
    EXPECT_EQ(config.calculateQuorumSize(4), 3);   // 3 data + 1 parity: (4/2)+1 = 3
    EXPECT_EQ(config.calculateQuorumSize(6), 4);   // 5 data + 1 parity: (6/2)+1 = 4
}

TEST_F(RAIDPaxosTest, RAID10QuorumCalculation) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::HYBRID;
    config.mirror_factor = 2;
    config.stripe_width = 2;
    
    // RAID 10: Need at least one complete mirror set per stripe
    // With stripe_width=2, mirror_factor=2: 4 shards total
    // Need: (4 / (2*2)) + 1 = 2
    EXPECT_EQ(config.calculateQuorumSize(4), 2);
    
    // With stripe_width=2, mirror_factor=2: 8 shards total
    // Need: (8 / (2*2)) + 1 = 3
    EXPECT_EQ(config.calculateQuorumSize(8), 3);
}

// ============================================================================
// Configuration Validation Tests
// ============================================================================

TEST_F(RAIDPaxosTest, RAID0RequiresExplicitAllowance) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::STRIPE;
    config.allow_raid0 = false;
    
    EXPECT_FALSE(config.isValidConfiguration());
    
    config.allow_raid0 = true;
    EXPECT_TRUE(config.isValidConfiguration());
}

TEST_F(RAIDPaxosTest, RAID1ValidConfiguration) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 2;
    config.stripe_width = 1;
    
    EXPECT_TRUE(config.isValidConfiguration());
}

TEST_F(RAIDPaxosTest, RAID1InvalidMirrorFactor) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 1;  // Must be >= 2
    
    EXPECT_FALSE(config.isValidConfiguration());
}

TEST_F(RAIDPaxosTest, RAID5ValidConfiguration) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    config.data_shard_indices = {0, 1, 2};
    config.parity_shard_index = 3;
    
    EXPECT_TRUE(config.isValidConfiguration());
}

TEST_F(RAIDPaxosTest, RAID5InvalidWithoutParity) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    config.data_shard_indices = {0, 1, 2};
    config.parity_shard_index = -1;  // Not configured
    
    EXPECT_FALSE(config.isValidConfiguration());
}

TEST_F(RAIDPaxosTest, RAID5InvalidWithoutDataShards) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    config.data_shard_indices = {};  // Empty
    config.parity_shard_index = 3;
    
    EXPECT_FALSE(config.isValidConfiguration());
}

TEST_F(RAIDPaxosTest, RAID10ValidConfiguration) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::HYBRID;
    config.mirror_factor = 2;
    config.stripe_width = 2;
    
    EXPECT_TRUE(config.isValidConfiguration());
}

// ============================================================================
// Failure Tolerance Tests
// ============================================================================

TEST_F(RAIDPaxosTest, RAID0NoFailureTolerance) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::STRIPE;
    
    EXPECT_EQ(config.getMaxTolerableFailures(4), 0);
    EXPECT_EQ(config.getMaxTolerableFailures(8), 0);
}

TEST_F(RAIDPaxosTest, RAID1FailureTolerance) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 2;
    
    // Can tolerate mirror_factor - 1 failures
    EXPECT_EQ(config.getMaxTolerableFailures(4), 1);
    
    config.mirror_factor = 3;
    EXPECT_EQ(config.getMaxTolerableFailures(6), 2);
}

TEST_F(RAIDPaxosTest, RAID5FailureTolerance) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    
    // RAID 5: Can tolerate 1 failure
    EXPECT_EQ(config.getMaxTolerableFailures(4), 1);
    EXPECT_EQ(config.getMaxTolerableFailures(8), 1);
}

TEST_F(RAIDPaxosTest, RAID10FailureTolerance) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::HYBRID;
    config.mirror_factor = 2;
    
    // Can tolerate mirror_factor - 1 failures per stripe
    EXPECT_EQ(config.getMaxTolerableFailures(4), 1);
    
    config.mirror_factor = 3;
    EXPECT_EQ(config.getMaxTolerableFailures(6), 2);
}

TEST_F(RAIDPaxosTest, CanRecoverFromFailures) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    config.data_shard_indices = {0, 1, 2, 3, 4};
    config.parity_shard_index = 5;
    
    // Can recover from 1 failure
    EXPECT_TRUE(config.canRecoverFromFailures({3}));
    EXPECT_TRUE(config.canRecoverFromFailures({5}));
    
    // Cannot recover from 2 failures
    EXPECT_FALSE(config.canRecoverFromFailures({3, 5}));
}

// ============================================================================
// RAIDPaxosConsensus Tests
// ============================================================================

TEST_F(RAIDPaxosTest, RAIDPaxosConstruction) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 2;
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    EXPECT_NE(consensus, nullptr);
    
    // Should not be initialized yet
    EXPECT_FALSE(consensus->isLeader());
}

TEST_F(RAIDPaxosTest, RAIDPaxosGetType) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    EXPECT_EQ(consensus->getType(), ConsensusType::RAID_PAXOS);
}

TEST_F(RAIDPaxosTest, RAIDPaxosInvalidConfigFailsInitialization) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::STRIPE;
    config.allow_raid0 = false;  // Invalid for RAID 0
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    
    std::vector<std::string> nodes = {"node-1", "node-2", "node-3"};
    EXPECT_FALSE(consensus->initialize("node-1", nodes));
}

TEST_F(RAIDPaxosTest, RAIDPaxosValidConfigInitialization) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 2;
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    
    std::vector<std::string> nodes = {"node-1", "node-2", "node-3"};
    EXPECT_TRUE(consensus->initialize("node-1", nodes));
}

TEST_F(RAIDPaxosTest, RAIDPaxosHasQuorumRAID0) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::STRIPE;
    config.allow_raid0 = true;
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    std::vector<std::string> nodes = {"node-1", "node-2", "node-3", "node-4"};
    consensus->initialize("node-1", nodes);
    
    // RAID 0: Need all shards
    std::set<std::string> all_responses = {"node-1", "node-2", "node-3", "node-4"};
    EXPECT_TRUE(consensus->hasQuorum(all_responses));
    
    std::set<std::string> partial_responses = {"node-1", "node-2"};
    EXPECT_FALSE(consensus->hasQuorum(partial_responses));
}

TEST_F(RAIDPaxosTest, RAIDPaxosHasQuorumRAID1) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 2;
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    std::vector<std::string> nodes = {"node-1", "node-2", "node-3", "node-4"};
    consensus->initialize("node-1", nodes);
    
    // RAID 1 with 4 nodes, mirror_factor=2: need 3 responses (4/2 + 1)
    std::set<std::string> three_responses = {"node-1", "node-2", "node-3"};
    EXPECT_TRUE(consensus->hasQuorum(three_responses));
    
    std::set<std::string> two_responses = {"node-1", "node-2"};
    EXPECT_FALSE(consensus->hasQuorum(two_responses));
}

TEST_F(RAIDPaxosTest, RAIDPaxosHasQuorumRAID5) {
    RAIDPaxosConfig config;
    config.raid_mode = RAIDMode::PARITY;
    
    auto consensus = std::make_unique<RAIDPaxosConsensus>(config);
    std::vector<std::string> nodes = {"node-1", "node-2", "node-3", "node-4", "node-5"};
    consensus->initialize("node-1", nodes);
    
    // RAID 5 with 5 nodes: need 3 responses ((5/2) + 1)
    std::set<std::string> three_responses = {"node-1", "node-2", "node-3"};
    EXPECT_TRUE(consensus->hasQuorum(three_responses));
    
    std::set<std::string> two_responses = {"node-1", "node-2"};
    EXPECT_FALSE(consensus->hasQuorum(two_responses));
}

// ============================================================================
// Description Tests
// ============================================================================

TEST_F(RAIDPaxosTest, RAIDModeDescription) {
    RAIDPaxosConfig config;
    
    config.raid_mode = RAIDMode::STRIPE;
    EXPECT_EQ(config.getRAIDModeDescription(), 
        "RAID 0 (Striping) - No redundancy, maximum performance");
    
    config.raid_mode = RAIDMode::MIRROR;
    config.mirror_factor = 3;
    EXPECT_EQ(config.getRAIDModeDescription(), 
        "RAID 1 (Mirroring) - Full redundancy, 3x replication");
    
    config.raid_mode = RAIDMode::PARITY;
    config.data_shard_indices = {0, 1, 2, 3};
    EXPECT_EQ(config.getRAIDModeDescription(), 
        "RAID 5 (Distributed Parity) - 4 data shards + 1 parity");
    
    config.raid_mode = RAIDMode::HYBRID;
    config.stripe_width = 4;
    config.mirror_factor = 2;
    EXPECT_EQ(config.getRAIDModeDescription(), 
        "RAID 10 (Stripe+Mirror) - 4 stripe width x 2 mirrors");
}
} } // namespace themisdb::sharding

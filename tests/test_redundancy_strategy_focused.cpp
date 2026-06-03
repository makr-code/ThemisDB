/*
 * ThemisDB | File: test_redundancy_strategy_focused.cpp | Version: 0.0.47
 * Maturity: 🟡 FOCUSED TESTS | Focused Tests for W2-S02 Input Validation
 * Status: New
 */

/**
 * ThemisDB Redundancy Strategy - Input Validation & Distributed Consistency Tests (W2-S02)
 * 
 * Tests for:
 * - Empty document_id and data validation
 * - Invalid write_quorum guards
 * - Distributed consensus precondition checks
 */

#include <gtest/gtest.h>
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <vector>
#include <map>
#include <string>

using namespace themis::sharding;

// ═══════════════════════════════════════════════════════════
// Mock Implementations
// ═══════════════════════════════════════════════════════════

class MockShardStorage {
public:
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> shard_data;
    
    bool write(const std::string& shard_id, const std::string& doc_id, 
               const std::vector<uint8_t>& data) {
        shard_data[shard_id][doc_id] = data;
        return true;
    }
    
    std::optional<std::vector<uint8_t>> read(const std::string& shard_id, 
                                              const std::string& doc_id) {
        if (shard_data.count(shard_id) && shard_data[shard_id].count(doc_id)) {
            return shard_data[shard_id][doc_id];
        }
        return std::nullopt;
    }
};

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class RedundancyStrategyFocusedTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsistentHashRing> ring;
    std::unique_ptr<ShardTopology> topology;
    std::unique_ptr<MockShardStorage> storage;
    std::unique_ptr<RedundancyStrategy> strategy;
    
    void SetUp() override {
        ring = std::make_unique<ConsistentHashRing>(100);
        for (int i = 0; i < 6; ++i) {
            ring->addNode("shard-" + std::to_string(i));
        }
        
        topology = std::make_unique<ShardTopology>();
        storage = std::make_unique<MockShardStorage>();
        
        RedundancyConfig config;
        config.mode = RedundancyMode::MIRROR;
        config.replication_factor = 3;
        config.write_concern = WriteConcern::MAJORITY;
        config.read_concern = ReadConcern::QUORUM;
        
        strategy = std::make_unique<RedundancyStrategy>(config);
    }
};

// ═══════════════════════════════════════════════════════════
// W2-S02: Input Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyFocusedTest, WriteRejectsEmptyDocumentId) {
    std::vector<uint8_t> data = {1, 2, 3, 4};
    
    auto write_handler = [this](const std::string& shard_id, const std::string& doc_id,
                                const std::vector<uint8_t>& data) {
        return storage->write(shard_id, doc_id, data);
    };
    
    // Should fail with empty document_id
    auto result = strategy->write("", data, "test_collection", *ring, *topology, write_handler);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "document_id is empty");
}

TEST_F(RedundancyStrategyFocusedTest, WriteRejectsEmptyData) {
    std::vector<uint8_t> empty_data;
    
    auto write_handler = [this](const std::string& shard_id, const std::string& doc_id,
                                const std::vector<uint8_t>& data) {
        return storage->write(shard_id, doc_id, data);
    };
    
    // Should fail with empty data
    auto result = strategy->write("doc123", empty_data, "test_collection", *ring, *topology, write_handler);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "data is empty");
}

TEST_F(RedundancyStrategyFocusedTest, ReadRejectsEmptyDocumentId) {
    auto read_handler = [this](const std::string& shard_id, const std::string& doc_id) 
        -> std::optional<std::vector<uint8_t>> {
        return storage->read(shard_id, doc_id);
    };
    
    // Should fail with empty document_id
    auto result = strategy->read("", "test_collection", *ring, *topology, read_handler);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "document_id is empty");
}

TEST_F(RedundancyStrategyFocusedTest, WriteAcceptsValidInputs) {
    std::vector<uint8_t> data = {1, 2, 3, 4};
    
    auto write_handler = [this](const std::string& shard_id, const std::string& doc_id,
                                const std::vector<uint8_t>& data) {
        return storage->write(shard_id, doc_id, data);
    };
    
    // Should succeed with valid inputs
    auto result = strategy->write("doc123", data, "test_collection", *ring, *topology, write_handler);
    // Note: May fail due to insufficient replicas in test environment, but should pass input validation
    // Expected: success OR error related to replication, NOT input validation
    EXPECT_NE(result.error_message, "document_id is empty");
    EXPECT_NE(result.error_message, "data is empty");
}

// ═══════════════════════════════════════════════════════════
// W2-S02: Distributed Consensus Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyFocusedTest, WriteMirrorRejectsZeroQuorumWithReplicas) {
    // Create a strategy with invalid QUORUM configuration (write_quorum = 0)
    RedundancyConfig bad_config;
    bad_config.mode = RedundancyMode::MIRROR;
    bad_config.replication_factor = 3;
    bad_config.write_concern = WriteConcern::QUORUM;
    bad_config.write_quorum = 0;  // Invalid: 0 quorum with replicas
    
    auto bad_strategy = std::make_unique<RedundancyStrategy>(bad_config);
    
    std::vector<uint8_t> data = {1, 2, 3, 4};
    auto write_handler = [this](const std::string& shard_id, const std::string& doc_id,
                                const std::vector<uint8_t>& data) {
        return storage->write(shard_id, doc_id, data);
    };
    
    // Should fail due to invalid quorum configuration
    auto result = bad_strategy->write("doc123", data, "test_collection", *ring, *topology, write_handler);
    EXPECT_FALSE(result.success);
    EXPECT_THAT(result.error_message, 
                ::testing::HasSubstr("write_quorum"));
}

// ═══════════════════════════════════════════════════════════
// W2-S02: Raft Write Validation Tests
// ═══════════════════════════════════════════════════════════

class RaftWriteValidationTest : public RedundancyStrategyFocusedTest {
protected:
    void SetUp() override {
        RedundancyStrategyFocusedTest::SetUp();
        // Configure for Raft writes
        RedundancyConfig raft_config;
        raft_config.mode = RedundancyMode::MIRROR;
        raft_config.replication_factor = 3;
        raft_config.use_raft = true;
        strategy = std::make_unique<RedundancyStrategy>(raft_config);
    }
};

TEST_F(RaftWriteValidationTest, ProposeRaftWriteRejectsEmptyShardId) {
    std::vector<uint8_t> data = {1, 2, 3, 4};
    
    // Should fail: empty shard_id
    bool result = strategy->proposeRaftWrite("", "doc123", data);
    EXPECT_FALSE(result);
}

TEST_F(RaftWriteValidationTest, ProposeRaftWriteRejectsEmptyDocumentId) {
    std::vector<uint8_t> data = {1, 2, 3, 4};
    
    // Should fail: empty document_id
    bool result = strategy->proposeRaftWrite("shard-0", "", data);
    EXPECT_FALSE(result);
}

TEST_F(RaftWriteValidationTest, ProposeRaftWriteRejectsEmptyData) {
    std::vector<uint8_t> empty_data;
    
    // Should fail: empty data
    bool result = strategy->proposeRaftWrite("shard-0", "doc123", empty_data);
    EXPECT_FALSE(result);
}

TEST_F(RaftWriteValidationTest, ProposeRaftWriteRejectsDelimiterInDocumentId) {
    std::vector<uint8_t> data = {1, 2, 3, 4};
    
    // Should fail: document_id contains reserved delimiter '|'
    bool result = strategy->proposeRaftWrite("shard-0", "doc|with|pipes", data);
    EXPECT_FALSE(result);
}

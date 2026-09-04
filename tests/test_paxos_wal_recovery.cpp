// Copyright 2026 ThemisDB
// Licensed under MIT License
// Phase 2.1: WAL and Recovery Tests

#include <gtest/gtest.h>
#include "sharding/paxos_consensus.h"
#include "sharding/paxos_wal.h"
#include "sharding/paxos_snapshot.h"
#include "sharding/consensus_factory.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;
using namespace themis::sharding;

class PaxosWALTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = "/tmp/paxos_wal_test_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_dir_);
        std::filesystem::create_directories(test_dir_ + "/wal");
        std::filesystem::create_directories(test_dir_ + "/snapshots");
        
        // Configure Paxos with persistence enabled
        config_.type = ConsensusType::PAXOS;
        config_.node_id = "test_node_1";
        config_.cluster_nodes = {"test_node_1"};  // Single node for testing
        config_.enable_persistence = true;
        config_.data_dir = test_dir_;
        config_.heartbeat_interval = std::chrono::milliseconds(100);
        config_.election_timeout_min = std::chrono::milliseconds(300);
        config_.election_timeout_max = std::chrono::milliseconds(600);
        config_.paxos_prepare_timeout = std::chrono::milliseconds(1000);
        config_.paxos_accept_timeout = std::chrono::milliseconds(500);
    }
    
    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_ = {};
    ConsensusConfig config_;
};

// Test 1: WAL Infrastructure Initialization
TEST_F(PaxosWALTest, WALInitialization) {
    PaxosWALConfig wal_config;
    wal_config.wal_directory = test_dir_ + "/wal";
    wal_config.snapshot_directory = test_dir_ + "/snapshots";
    
    PaxosWAL wal(wal_config);
    EXPECT_TRUE(wal.initialize());
    
    // Verify directories created
    EXPECT_TRUE(std::filesystem::exists(wal_config.wal_directory));
    EXPECT_TRUE(std::filesystem::exists(wal_config.snapshot_directory));
}

// Test 2: WAL Entry Logging
TEST_F(PaxosWALTest, LogPaxosOperations) {
    PaxosWALConfig wal_config;
    wal_config.wal_directory = test_dir_ + "/wal";
    wal_config.snapshot_directory = test_dir_ + "/snapshots";
    
    PaxosWAL wal(wal_config);
    ASSERT_TRUE(wal.initialize());
    
    // Log PREPARE
    LSN lsn1 = wal.logPrepare(1, 1, "test_node_1");
    EXPECT_GT(lsn1.segment, 0);
    
    // Log ACCEPT
    ConsensusLogEntry value;
    value.index = 1;
    value.term = 1;
    value.operation = "test_op";
    value.data = nlohmann::json{{"key", "value"}};
    
    LSN lsn2 = wal.logAccept(1, 1, "test_node_1", value);
    EXPECT_GT(lsn2.segment, 0);
    
    // Log COMMIT
    LSN lsn3 = wal.logCommit(1, value);
    EXPECT_GT(lsn3.segment, 0);
    
    // Verify LSNs are increasing
    EXPECT_TRUE(lsn2 > lsn1);
    EXPECT_TRUE(lsn3 > lsn2);
}

// Test 3: WAL Entry Reading
TEST_F(PaxosWALTest, ReadWALEntries) {
    PaxosWALConfig wal_config;
    wal_config.wal_directory = test_dir_ + "/wal";
    wal_config.snapshot_directory = test_dir_ + "/snapshots";
    
    PaxosWAL wal(wal_config);
    ASSERT_TRUE(wal.initialize());
    
    // Write entries
    LSN start_lsn = wal.logPrepare(1, 1, "test_node_1");
    
    ConsensusLogEntry value;
    value.index = 1;
    value.term = 1;
    value.operation = "test_op";
    value.data = nlohmann::json{{"key", "value"}};
    
    wal.logAccept(1, 1, "test_node_1", value);
    wal.logCommit(1, value);
    
    wal.flush();  // Ensure written
    
    // Read entries
    auto entries = wal.readEntries(start_lsn);
    EXPECT_GE(entries.size(), 3);  // At least PREPARE, ACCEPT, COMMIT
    
    // Verify first entry is PREPARE
    EXPECT_EQ(entries[0].type, PaxosWALEntryType::PREPARE);
    EXPECT_EQ(entries[0].slot, 1);
}

// Test 4: Snapshot Creation
TEST_F(PaxosWALTest, SnapshotCreation) {
    PaxosSnapshotManager snapshot_mgr(test_dir_ + "/snapshots", 10);
    
    // Create mock Paxos state
    std::map<uint64_t, PaxosInstance> instances;
    std::map<uint64_t, ConsensusLogEntry> committed_log;
    
    PaxosInstance instance;
    instance.slot = 1;
    instance.is_committed = true;
    instances[1] = instance;
    
    ConsensusLogEntry entry;
    entry.index = 1;
    entry.term = 1;
    entry.operation = "test";
    entry.data = nlohmann::json{{"test", "data"}};
    committed_log[1] = entry;
    
    // Create snapshot
    auto snapshot_id = snapshot_mgr.createSnapshot(
        "test_node_1",
        LSN(1, 100),
        1,  // last_committed_slot
        1,  // current_round
        instances,
        committed_log
    );
    
    ASSERT_TRUE(snapshot_id.has_value());
    EXPECT_GT(snapshot_id.value(), 0);
    
    // Verify snapshot file exists
    auto snapshots = snapshot_mgr.listSnapshots();
    EXPECT_EQ(snapshots.size(), 1);
    EXPECT_EQ(snapshots[0], snapshot_id.value());
}

// Test 5: Snapshot Loading
TEST_F(PaxosWALTest, SnapshotLoading) {
    PaxosSnapshotManager snapshot_mgr(test_dir_ + "/snapshots", 10);
    
    // Create mock Paxos state
    std::map<uint64_t, PaxosInstance> instances;
    std::map<uint64_t, ConsensusLogEntry> committed_log;
    
    PaxosInstance instance;
    instance.slot = 1;
    instance.is_committed = true;
    instances[1] = instance;
    
    ConsensusLogEntry entry;
    entry.index = 1;
    entry.term = 1;
    entry.operation = "test_operation";
    entry.data = nlohmann::json{{"test_key", "test_value"}};
    committed_log[1] = entry;
    
    // Create snapshot
    auto snapshot_id = snapshot_mgr.createSnapshot(
        "test_node_1",
        LSN(1, 100),
        1,
        1,
        instances,
        committed_log
    );
    ASSERT_TRUE(snapshot_id.has_value());
    
    // Load snapshot
    auto loaded_snapshot = snapshot_mgr.loadLatestSnapshot();
    ASSERT_TRUE(loaded_snapshot.has_value());
    
    // Verify snapshot contents
    EXPECT_EQ(loaded_snapshot->snapshot_id, snapshot_id.value());
    EXPECT_EQ(loaded_snapshot->last_committed_slot, 1);
    EXPECT_EQ(loaded_snapshot->current_round, 1);
    EXPECT_EQ(loaded_snapshot->node_id, "test_node_1");
    EXPECT_EQ(loaded_snapshot->instances.size(), 1);
    EXPECT_EQ(loaded_snapshot->committed_log.size(), 1);
}

// Test 6: PaxosConsensus with WAL Integration
TEST_F(PaxosWALTest, PaxosConsensusWithWAL) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    auto* paxos = dynamic_cast<PaxosConsensus*>(module.get());
    ASSERT_NE(paxos, nullptr);
    
    // Initialize with persistence
    EXPECT_TRUE(paxos->initialize(config_.node_id, config_.cluster_nodes));
    EXPECT_TRUE(paxos->start());
    
    // Give it time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Propose a value
    nlohmann::json data = {{"test", "value"}};
    auto log_index = paxos->propose("test_operation", data);
    
    ASSERT_TRUE(log_index.has_value());
    EXPECT_GT(log_index.value(), 0);
    
    // Wait for commit
    bool committed = paxos->waitForCommit(log_index.value(), std::chrono::seconds(2));
    EXPECT_TRUE(committed);
    
    // Give time for WAL to flush
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify WAL directory has entries
    std::string wal_dir = test_dir_ + "/wal";
    EXPECT_TRUE(std::filesystem::exists(wal_dir));
    
    // Check if any WAL files were created
    int file_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(wal_dir)) {
        if (entry.is_regular_file()) {
            file_count++;
        }
    }
    EXPECT_GT(file_count, 0) << "WAL files should be created";
    
    paxos->stop();
}

// Test 7: Snapshot Threshold
TEST_F(PaxosWALTest, SnapshotThreshold) {
    PaxosWALConfig wal_config;
    wal_config.wal_directory = test_dir_ + "/wal";
    wal_config.snapshot_directory = test_dir_ + "/snapshots";
    wal_config.snapshot_interval = 5;  // Snapshot every 5 operations
    
    PaxosWAL wal(wal_config);
    ASSERT_TRUE(wal.initialize());
    
    // Test threshold detection
    EXPECT_FALSE(wal.shouldCreateSnapshot(0));
    EXPECT_FALSE(wal.shouldCreateSnapshot(4));
    EXPECT_TRUE(wal.shouldCreateSnapshot(5));
    EXPECT_TRUE(wal.shouldCreateSnapshot(10));
}

// Test 8: Multiple Snapshots and Cleanup
TEST_F(PaxosWALTest, SnapshotCleanup) {
    PaxosSnapshotManager snapshot_mgr(test_dir_ + "/snapshots", 3);  // Keep only 3
    
    std::map<uint64_t, PaxosInstance> instances;
    std::map<uint64_t, ConsensusLogEntry> committed_log;
    
    // Create 5 snapshots
    for (int i = 1; i <= 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Ensure unique IDs
        
        auto snapshot_id = snapshot_mgr.createSnapshot(
            "test_node_1",
            LSN(1, i * 100),
            i,
            i,
            instances,
            committed_log
        );
        ASSERT_TRUE(snapshot_id.has_value());
    }
    
    // Should only have 3 snapshots (old ones cleaned up)
    auto snapshots = snapshot_mgr.listSnapshots();
    EXPECT_LE(snapshots.size(), 3);
}

// Test 9: WAL Entry Serialization
TEST_F(PaxosWALTest, WALEntrySerialization) {
    PaxosWALEntry entry;
    entry.type = PaxosWALEntryType::ACCEPT;
    entry.slot = 42;
    entry.round = 10;
    entry.node_id = "test_node";
    entry.timestamp = 1234567890;
    entry.data = nlohmann::json{{"key", "value"}};
    
    // Convert to WAL entry
    WALEntry wal_entry = entry.toWALEntry();
    
    // Verify conversion
    EXPECT_EQ(wal_entry.timestamp, entry.timestamp);
    EXPECT_TRUE(wal_entry.data.contains("slot"));
    EXPECT_EQ(wal_entry.data["slot"].get<uint64_t>(), 42);
    
    // Convert back
    PaxosWALEntry restored = PaxosWALEntry::fromWALEntry(wal_entry);
    
    // Verify restoration
    EXPECT_EQ(restored.type, entry.type);
    EXPECT_EQ(restored.slot, entry.slot);
    EXPECT_EQ(restored.round, entry.round);
    EXPECT_EQ(restored.node_id, entry.node_id);
}

// Test 10: Checksum Verification
TEST_F(PaxosWALTest, SnapshotChecksumVerification) {
    PaxosSnapshot snapshot;
    snapshot.snapshot_id = 123;
    snapshot.last_applied_lsn = LSN(1, 100);
    snapshot.last_committed_slot = 10;
    snapshot.current_round = 5;
    snapshot.node_id = "test_node";
    snapshot.timestamp = 1234567890;
    
    // Calculate checksum
    std::string checksum1 = snapshot.calculateChecksum();
    EXPECT_FALSE(checksum1.empty());
    EXPECT_EQ(checksum1.length(), 64);  // SHA-256 hex = 64 chars
    
    // Same data should produce same checksum
    std::string checksum2 = snapshot.calculateChecksum();
    EXPECT_EQ(checksum1, checksum2);
    
    // Set checksum and verify
    snapshot.checksum = checksum1;
    EXPECT_TRUE(snapshot.verifyChecksum());
    
    // Tamper with data
    snapshot.last_committed_slot = 999;
    EXPECT_FALSE(snapshot.verifyChecksum());
}


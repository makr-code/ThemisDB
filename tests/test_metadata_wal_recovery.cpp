// Copyright 2026 ThemisDB
// Licensed under MIT License
// Phase 2.2: Metadata Shard Durability Tests

#include <gtest/gtest.h>
#include "sharding/metadata_shard.h"
#include "sharding/metadata_wal.h"
#include "sharding/metadata_snapshot.h"
#include <filesystem>
#include <thread>

#include "utils/test_stability.h"

using namespace themisdb::sharding;

class MetadataWALTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = "/tmp/metadata_wal_test_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_dir_);
        std::filesystem::create_directories(test_dir_ + "/wal");
        std::filesystem::create_directories(test_dir_ + "/snapshots");
    }
    
    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_;
};

// Test 1: WAL Infrastructure Initialization
TEST_F(MetadataWALTest, WALInitialization) {
    MetadataWALConfig config;
    config.wal_directory = test_dir_ + "/wal";
    config.snapshot_directory = test_dir_ + "/snapshots";
    
    MetadataWAL wal(config);
    EXPECT_TRUE(wal.initialize());
    
    // Verify directories created
    EXPECT_TRUE(std::filesystem::exists(config.wal_directory));
    EXPECT_TRUE(std::filesystem::exists(config.snapshot_directory));
}

// Test 2: Log Metadata Operations
TEST_F(MetadataWALTest, LogMetadataOperations) {
    MetadataWALConfig config;
    config.wal_directory = test_dir_ + "/wal";
    config.snapshot_directory = test_dir_ + "/snapshots";
    
    MetadataWAL wal(config);
    ASSERT_TRUE(wal.initialize());
    
    // Log PUT
    nlohmann::json value = {{"name", "test_table"}, {"columns", 5}};
    LSN lsn1 = wal.logPut(MetadataPartitionKey::SCHEMA, "table1", value, 1);
    EXPECT_GT(lsn1.segment, 0);
    
    // Log UPDATE
    nlohmann::json value2 = {{"name", "test_table_updated"}, {"columns", 6}};
    LSN lsn2 = wal.logUpdate(MetadataPartitionKey::SCHEMA, "table1", value2, 2);
    EXPECT_GT(lsn2.segment, 0);
    
    // Log DELETE
    LSN lsn3 = wal.logDelete(MetadataPartitionKey::SCHEMA, "table1", 2);
    EXPECT_GT(lsn3.segment, 0);
    
    // Verify LSNs are increasing
    EXPECT_TRUE(lsn2 > lsn1);
    EXPECT_TRUE(lsn3 > lsn2);
}

// Test 3: Read WAL Entries
TEST_F(MetadataWALTest, ReadWALEntries) {
    MetadataWALConfig config;
    config.wal_directory = test_dir_ + "/wal";
    config.snapshot_directory = test_dir_ + "/snapshots";
    
    MetadataWAL wal(config);
    ASSERT_TRUE(wal.initialize());
    
    // Write entries
    nlohmann::json value = {{"name", "test_table"}};
    LSN start_lsn = wal.logPut(MetadataPartitionKey::SCHEMA, "table1", value, 1);
    wal.logUpdate(MetadataPartitionKey::SCHEMA, "table1", value, 2);
    wal.logDelete(MetadataPartitionKey::INDEX, "index1", 1);
    
    wal.flush();  // Ensure written
    
    // Read entries
    auto entries = wal.readEntries(start_lsn);
    EXPECT_GE(entries.size(), 3);  // At least PUT, UPDATE, DELETE
    
    // Verify first entry is PUT
    EXPECT_EQ(entries[0].type, MetadataWALEntryType::PUT);
    EXPECT_EQ(entries[0].partition, MetadataPartitionKey::SCHEMA);
    EXPECT_EQ(entries[0].key, "table1");
}

// Test 4: Snapshot Creation
TEST_F(MetadataWALTest, SnapshotCreation) {
    MetadataSnapshotManager snapshot_mgr(test_dir_ + "/snapshots", 10);
    
    // Create mock metadata storage
    std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>> storage;
    
    MetadataEntry entry;
    entry.key = "table1";
    entry.value = {{"name", "test_table"}};
    entry.version = 1;
    entry.partition = MetadataPartitionKey::SCHEMA;
    entry.created_at = std::chrono::system_clock::now();
    entry.updated_at = entry.created_at;
    
    storage[MetadataPartitionKey::SCHEMA]["table1"] = entry;
    
    // Create snapshot
    auto snapshot_id = snapshot_mgr.createSnapshot(
        "test_shard_0",
        LSN(1, 100),
        storage
    );
    
    ASSERT_TRUE(snapshot_id.has_value());
    EXPECT_GT(snapshot_id.value(), 0);
    
    // Verify snapshot file exists
    auto snapshots = snapshot_mgr.listSnapshots();
    EXPECT_EQ(snapshots.size(), 1);
    EXPECT_EQ(snapshots[0], snapshot_id.value());
}

// Test 5: Snapshot Loading
TEST_F(MetadataWALTest, SnapshotLoading) {
    MetadataSnapshotManager snapshot_mgr(test_dir_ + "/snapshots", 10);
    
    // Create mock metadata storage
    std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>> storage;
    
    MetadataEntry entry;
    entry.key = "table1";
    entry.value = {{"name", "test_table"}, {"columns", 5}};
    entry.version = 1;
    entry.partition = MetadataPartitionKey::SCHEMA;
    entry.created_at = std::chrono::system_clock::now();
    entry.updated_at = entry.created_at;
    
    storage[MetadataPartitionKey::SCHEMA]["table1"] = entry;
    
    // Create snapshot
    auto snapshot_id = snapshot_mgr.createSnapshot(
        "test_shard_0",
        LSN(1, 100),
        storage
    );
    ASSERT_TRUE(snapshot_id.has_value());
    
    // Load snapshot
    auto loaded_snapshot = snapshot_mgr.loadLatestSnapshot();
    ASSERT_TRUE(loaded_snapshot.has_value());
    
    // Verify snapshot contents
    EXPECT_EQ(loaded_snapshot->snapshot_id, snapshot_id.value());
    EXPECT_EQ(loaded_snapshot->shard_id, "test_shard_0");
    EXPECT_EQ(loaded_snapshot->total_entries, 1);
    
    // Verify data
    auto& partitions = loaded_snapshot->partitions;
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_TRUE(partitions.count(MetadataPartitionKey::SCHEMA) > 0);
    
    auto& schema_entries = partitions[MetadataPartitionKey::SCHEMA];
    EXPECT_EQ(schema_entries.size(), 1);
    EXPECT_TRUE(schema_entries.count("table1") > 0);
    EXPECT_EQ(schema_entries["table1"]["value"]["name"], "test_table");
}

// Test 6: MetadataShard with Persistence
TEST_F(MetadataWALTest, MetadataShardWithPersistence) {
    MetadataShardConfig config;
    config.shard_id = "test_shard_0";
    config.partitions = {
        MetadataPartitionKey::SCHEMA,
        MetadataPartitionKey::INDEX
    };
    config.enable_persistence = true;
    config.data_dir = test_dir_;
    config.snapshot_interval = 5;  // Snapshot every 5 operations
    config.enforce_strong_consistency = false;
    
    auto shard = std::make_unique<MetadataShard>(config, nullptr);
    EXPECT_TRUE(shard->initialize());
    EXPECT_TRUE(shard->start());
    
    // Put some entries
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table1", {{"name", "t1"}}));
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table2", {{"name", "t2"}}));
    EXPECT_TRUE(shard->put(MetadataPartitionKey::INDEX, "index1", {{"name", "i1"}}));
    
    // WALManager defaults to sync_on_write=true: each put() flushes to disk
    // synchronously before returning, so no sleep is needed here.
    // Poll with deadline to confirm files exist (guards against unexpected
    // async-flush configurations in future).
    const std::string wal_dir = test_dir_ + "/wal";
    const bool wal_files_visible = themis::test::poll_until([&] {
        if (!std::filesystem::exists(wal_dir)) {
          return false;
        }
        for (const auto& entry : std::filesystem::directory_iterator(wal_dir)) {
            if (entry.is_regular_file()) {
              return true;
            }
        }
        return false;
    }, std::chrono::seconds(5));

    ASSERT_TRUE(wal_files_visible) << "WAL files should appear within 5 s";
    
    shard->stop();
}

// Test 7: Recovery from WAL
TEST_F(MetadataWALTest, RecoveryFromWAL) {
    MetadataShardConfig config;
    config.shard_id = "test_shard_0";
    config.partitions = {MetadataPartitionKey::SCHEMA};
    config.enable_persistence = true;
    config.data_dir = test_dir_;
    config.snapshot_interval = 10;
    config.enforce_strong_consistency = false;
    
    // Phase 1: Create shard and add data
    {
        auto shard = std::make_unique<MetadataShard>(config, nullptr);
        EXPECT_TRUE(shard->initialize());
        EXPECT_TRUE(shard->start());
        
        // Add entries
        shard->put(MetadataPartitionKey::SCHEMA, "table1", {{"name", "t1"}});
        shard->put(MetadataPartitionKey::SCHEMA, "table2", {{"name", "t2"}});
        shard->put(MetadataPartitionKey::SCHEMA, "table3", {{"name", "t3"}});
        
        // Force snapshot creation
        shard->createPeriodicSnapshot();
        // createPeriodicSnapshot() is synchronous (holds storage_mutex_ and
        // writes the snapshot file before returning), so no sleep is needed.
        shard->stop();
    }
    
    // Phase 2: Create new shard and verify recovery
    {
        auto shard = std::make_unique<MetadataShard>(config, nullptr);
        EXPECT_TRUE(shard->initialize());  // Should recover here
        EXPECT_TRUE(shard->start());
        
        // Verify data was recovered
        auto entry1 = shard->get(MetadataPartitionKey::SCHEMA, "table1");
        ASSERT_TRUE(entry1.has_value());
        EXPECT_EQ(entry1->value["name"], "t1");
        
        auto entry2 = shard->get(MetadataPartitionKey::SCHEMA, "table2");
        ASSERT_TRUE(entry2.has_value());
        EXPECT_EQ(entry2->value["name"], "t2");
        
        auto entry3 = shard->get(MetadataPartitionKey::SCHEMA, "table3");
        ASSERT_TRUE(entry3.has_value());
        EXPECT_EQ(entry3->value["name"], "t3");
        
        shard->stop();
    }
}

// Test 8: Snapshot Threshold
TEST_F(MetadataWALTest, SnapshotThreshold) {
    MetadataWALConfig config;
    config.wal_directory = test_dir_ + "/wal";
    config.snapshot_directory = test_dir_ + "/snapshots";
    config.snapshot_interval = 5;  // Snapshot every 5 operations
    
    MetadataWAL wal(config);
    ASSERT_TRUE(wal.initialize());
    
    // Test threshold detection
    EXPECT_FALSE(wal.shouldCreateSnapshot(0));
    EXPECT_FALSE(wal.shouldCreateSnapshot(4));
    EXPECT_TRUE(wal.shouldCreateSnapshot(5));
    EXPECT_TRUE(wal.shouldCreateSnapshot(10));
}

// Test 9: Multiple Snapshots and Cleanup
TEST_F(MetadataWALTest, SnapshotCleanup) {
    MetadataSnapshotManager snapshot_mgr(test_dir_ + "/snapshots", 3);  // Keep only 3
    
    std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>> storage;
    
    // Create 5 snapshots
    for (int i = 1; i <= 5; ++i) {
        // Spin until the system_clock millisecond advances so that each
        // snapshot receives a unique timestamp-based ID.  This replaces a
        // blind sleep_for(10ms) which was a timing race on loaded CI agents.
        themis::test::wait_for_clock_advance_ms();
        
        auto snapshot_id = snapshot_mgr.createSnapshot(
            "test_shard_0",
            LSN(1, i * 100),
            storage
        );
        ASSERT_TRUE(snapshot_id.has_value());
    }
    
    // Should only have 3 snapshots (old ones cleaned up)
    auto snapshots = snapshot_mgr.listSnapshots();
    EXPECT_LE(snapshots.size(), 3);
}

// Test 10: WAL Entry Serialization
TEST_F(MetadataWALTest, WALEntrySerialization) {
    MetadataWALEntry entry;
    entry.type = MetadataWALEntryType::PUT;
    entry.partition = MetadataPartitionKey::SCHEMA;
    entry.key = "table1";
    entry.value = {{"name", "test_table"}};
    entry.version = 42;
    entry.timestamp = 1234567890;
    
    // Convert to WAL entry
    WALEntry wal_entry = entry.toWALEntry();
    
    // Verify conversion
    EXPECT_EQ(wal_entry.timestamp, entry.timestamp);
    EXPECT_TRUE(wal_entry.data.contains("partition"));
    EXPECT_EQ(wal_entry.data["partition"].get<int>(), static_cast<int>(MetadataPartitionKey::SCHEMA));
    EXPECT_EQ(wal_entry.data["key"].get<std::string>(), "table1");
    EXPECT_EQ(wal_entry.data["version"].get<uint64_t>(), 42);
    
    // Convert back
    MetadataWALEntry restored = MetadataWALEntry::fromWALEntry(wal_entry);
    
    // Verify restoration
    EXPECT_EQ(restored.type, entry.type);
    EXPECT_EQ(restored.partition, entry.partition);
    EXPECT_EQ(restored.key, entry.key);
    EXPECT_EQ(restored.version, entry.version);
}

// Test 11: Snapshot Checksum Verification
TEST_F(MetadataWALTest, SnapshotChecksumVerification) {
    MetadataSnapshot snapshot;
    snapshot.snapshot_id = 123;
    snapshot.last_applied_lsn = LSN(1, 100);
    snapshot.shard_id = "test_shard";
    snapshot.timestamp = 1234567890;
    snapshot.total_entries = 0;
    
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
    snapshot.total_entries = 999;
    EXPECT_FALSE(snapshot.verifyChecksum());
}

// Test 12: Delete Operations in Recovery
TEST_F(MetadataWALTest, DeleteOperationsInRecovery) {
    MetadataShardConfig config;
    config.shard_id = "test_shard_0";
    config.partitions = {MetadataPartitionKey::SCHEMA};
    config.enable_persistence = true;
    config.data_dir = test_dir_;
    config.snapshot_interval = 10;
    config.enforce_strong_consistency = false;
    
    // Phase 1: Create, add, delete data
    {
        auto shard = std::make_unique<MetadataShard>(config, nullptr);
        EXPECT_TRUE(shard->initialize());
        EXPECT_TRUE(shard->start());
        
        // Add entries
        shard->put(MetadataPartitionKey::SCHEMA, "table1", {{"name", "t1"}});
        shard->put(MetadataPartitionKey::SCHEMA, "table2", {{"name", "t2"}});
        shard->put(MetadataPartitionKey::SCHEMA, "table3", {{"name", "t3"}});
        
        // Delete one
        shard->remove(MetadataPartitionKey::SCHEMA, "table2");
        
        // put() and remove() both call logPut/logDelete which use WALManager
        // with sync_on_write=true by default — writes are flushed synchronously.
        // No sleep is needed before stop().
        shard->stop();
    }
    
    // Phase 2: Recover and verify deletion
    {
        auto shard = std::make_unique<MetadataShard>(config, nullptr);
        EXPECT_TRUE(shard->initialize());
        EXPECT_TRUE(shard->start());
        
        // table1 and table3 should exist
        EXPECT_TRUE(shard->get(MetadataPartitionKey::SCHEMA, "table1").has_value());
        EXPECT_TRUE(shard->get(MetadataPartitionKey::SCHEMA, "table3").has_value());
        
        // table2 should NOT exist (was deleted)
        EXPECT_FALSE(shard->get(MetadataPartitionKey::SCHEMA, "table2").has_value());
        
        shard->stop();
    }
}


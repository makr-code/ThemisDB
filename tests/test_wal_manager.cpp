/**
 * Unit tests for WAL Manager
 * 
 * Tests write-ahead log functionality for replica synchronization
 */

#include <gtest/gtest.h>
#include "sharding/wal_manager.h"
#include <filesystem>

using namespace themis::sharding;

class WALManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for WAL
        test_dir_ = "/tmp/themis_wal_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);
        
        // Configure WAL manager
        config_.wal_directory = test_dir_;
        config_.segment_size = 1024;  // Small for testing
        config_.max_segments = 5;
        config_.sync_on_write = true;
    }
    
    void TearDown() override {
        // Cleanup
        std::filesystem::remove_all(test_dir_);
    }
    
    WALManagerConfig config_;
    std::string test_dir_;
};

// ============================================================================
// LSN Tests
// ============================================================================

TEST_F(WALManagerTest, LSNComparison) {
    LSN lsn1(0, 0);
    LSN lsn2(0, 100);
    LSN lsn3(1, 0);
    
    EXPECT_TRUE(lsn1 < lsn2);
    EXPECT_TRUE(lsn2 < lsn3);
    EXPECT_TRUE(lsn1 < lsn3);
    
    EXPECT_TRUE(lsn1 == lsn1);
    EXPECT_FALSE(lsn1 == lsn2);
}

TEST_F(WALManagerTest, LSNStringConversion) {
    LSN lsn(5, 1024);
    std::string str = lsn.toString();
    EXPECT_EQ(str, "5/1024");
    
    LSN parsed = LSN::fromString(str);
    EXPECT_EQ(parsed.segment, 5);
    EXPECT_EQ(parsed.offset, 1024);
}

// ============================================================================
// WALEntry Tests
// ============================================================================

TEST_F(WALManagerTest, WALEntrySerialization) {
    WALEntry entry;
    entry.lsn = LSN(1, 100);
    entry.type = WALEntryType::INSERT;
    entry.timestamp = 1234567890;
    entry.transaction_id = "tx_123";
    entry.data = {
        {"table", "users"},
        {"id", 42},
        {"name", "Alice"}
    };
    
    auto bytes = entry.serialize();
    EXPECT_GT(bytes.size(), 0);
    
    auto deserialized = WALEntry::deserialize(bytes);
    EXPECT_EQ(deserialized.lsn.segment, entry.lsn.segment);
    EXPECT_EQ(deserialized.lsn.offset, entry.lsn.offset);
    EXPECT_EQ(deserialized.type, entry.type);
    EXPECT_EQ(deserialized.transaction_id, entry.transaction_id);
    EXPECT_EQ(deserialized.data["table"], "users");
    EXPECT_EQ(deserialized.data["id"], 42);
}

// ============================================================================
// WALManager Basic Tests
// ============================================================================

TEST_F(WALManagerTest, InitialState) {
    WALManager wal(config_);
    
    LSN current = wal.getCurrentLSN();
    EXPECT_EQ(current.segment, 0);
    EXPECT_EQ(current.offset, 0);
    
    auto stats = wal.getStatistics();
    EXPECT_EQ(stats.total_entries, 0);
}

TEST_F(WALManagerTest, AppendEntry) {
    WALManager wal(config_);
    
    WALEntry entry;
    entry.type = WALEntryType::INSERT;
    entry.data = {{"key", "value"}};
    
    LSN lsn = wal.append(entry);
    EXPECT_EQ(lsn.segment, 0);
    EXPECT_EQ(lsn.offset, 0);
    
    auto stats = wal.getStatistics();
    EXPECT_EQ(stats.total_entries, 1);
}

TEST_F(WALManagerTest, AppendMultipleEntries) {
    WALManager wal(config_);
    
    for (int i = 0; i < 10; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        
        wal.append(entry);
    }
    
    auto stats = wal.getStatistics();
    EXPECT_EQ(stats.total_entries, 10);
}

TEST_F(WALManagerTest, ReadEntry) {
    WALManager wal(config_);
    
    WALEntry entry;
    entry.type = WALEntryType::INSERT;
    entry.data = {{"test", "data"}};
    
    LSN lsn = wal.append(entry);
    wal.flush();
    
    auto read_entry = wal.read(lsn);
    ASSERT_TRUE(read_entry.has_value());
    EXPECT_EQ(read_entry->type, WALEntryType::INSERT);
    EXPECT_EQ(read_entry->data["test"], "data");
}

TEST_F(WALManagerTest, ReadRange) {
    WALManager wal(config_);
    
    std::vector<LSN> lsns;
    for (int i = 0; i < 5; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        
        lsns.push_back(wal.append(entry));
    }
    
    wal.flush();
    
    auto entries = wal.readRange(lsns[1], lsns[4]);
    EXPECT_EQ(entries.size(), 3);  // [1, 2, 3]
    EXPECT_EQ(entries[0].data["index"], 1);
    EXPECT_EQ(entries[1].data["index"], 2);
    EXPECT_EQ(entries[2].data["index"], 3);
}

TEST_F(WALManagerTest, ReadAllEntries) {
    WALManager wal(config_);
    
    for (int i = 0; i < 10; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        wal.append(entry);
    }
    
    wal.flush();
    
    LSN start(0, 0);
    auto entries = wal.readRange(start);
    EXPECT_EQ(entries.size(), 10);
}

// ============================================================================
// Segment Rotation Tests
// ============================================================================

TEST_F(WALManagerTest, SegmentRotation) {
    WALManager wal(config_);
    
    // Append entries until segment rotates
    // Each entry is about 50-100 bytes, segment is 1024 bytes
    for (int i = 0; i < 30; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {
            {"index", i},
            {"data", std::string(50, 'x')}  // Pad to make entry larger
        };
        wal.append(entry);
    }
    
    auto stats = wal.getStatistics();
    EXPECT_GT(stats.current_lsn.segment, 0);  // Should have rotated
}

// ============================================================================
// Checkpoint Tests
// ============================================================================

TEST_F(WALManagerTest, Checkpoint) {
    WALManager wal(config_);
    
    // Append some entries
    for (int i = 0; i < 5; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        wal.append(entry);
    }
    
    LSN checkpoint_lsn = wal.checkpoint();
    EXPECT_GT(checkpoint_lsn.offset, 0);
    
    // Read checkpoint entry
    auto checkpoint_entry = wal.read(checkpoint_lsn);
    ASSERT_TRUE(checkpoint_entry.has_value());
    EXPECT_EQ(checkpoint_entry->type, WALEntryType::CHECKPOINT);
}

// ============================================================================
// Truncation Tests
// ============================================================================

TEST_F(WALManagerTest, Truncate) {
    WALManager wal(config_);
    
    // Force segment rotation by appending many entries
    for (int i = 0; i < 100; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}, {"pad", std::string(50, 'x')}};
        wal.append(entry);
    }
    
    wal.flush();
    
    auto stats_before = wal.getStatistics();
    EXPECT_GT(stats_before.segments, 1);
    
    // Truncate old segments
    LSN truncate_lsn(1, 0);
    wal.truncate(truncate_lsn);
    
    LSN oldest = wal.getOldestLSN();
    EXPECT_EQ(oldest.segment, 1);
}

// ============================================================================
// Persistence Tests
// ============================================================================

TEST_F(WALManagerTest, PersistenceAcrossRestarts) {
    {
        // First instance - write entries
        WALManager wal(config_);
        
        for (int i = 0; i < 10; ++i) {
            WALEntry entry;
            entry.type = WALEntryType::INSERT;
            entry.data = {{"index", i}};
            wal.append(entry);
        }
        
        wal.flush();
        // wal destructor closes files
    }
    
    {
        // Second instance - should load existing WAL
        WALManager wal(config_);
        
        auto stats = wal.getStatistics();
        EXPECT_GT(stats.segments, 0);
        
        // Read entries
        LSN start(0, 0);
        auto entries = wal.readRange(start);
        EXPECT_EQ(entries.size(), 10);
        
        for (size_t i = 0; i < entries.size(); ++i) {
            EXPECT_EQ(entries[i].data["index"], i);
        }
    }
}

// ============================================================================
// Transaction Tests
// ============================================================================

TEST_F(WALManagerTest, TransactionEntries) {
    WALManager wal(config_);
    
    std::string tx_id = "tx_12345";
    
    // BEGIN
    WALEntry begin_entry;
    begin_entry.type = WALEntryType::BEGIN_TX;
    begin_entry.transaction_id = tx_id;
    begin_entry.data = {{"isolation", "READ_COMMITTED"}};
    wal.append(begin_entry);
    
    // INSERT
    WALEntry insert_entry;
    insert_entry.type = WALEntryType::INSERT;
    insert_entry.transaction_id = tx_id;
    insert_entry.data = {{"table", "users"}, {"id", 1}};
    wal.append(insert_entry);
    
    // COMMIT
    WALEntry commit_entry;
    commit_entry.type = WALEntryType::COMMIT_TX;
    commit_entry.transaction_id = tx_id;
    wal.append(commit_entry);
    
    wal.flush();
    
    LSN start(0, 0);
    auto entries = wal.readRange(start);
    EXPECT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].type, WALEntryType::BEGIN_TX);
    EXPECT_EQ(entries[1].type, WALEntryType::INSERT);
    EXPECT_EQ(entries[2].type, WALEntryType::COMMIT_TX);
    
    for (const auto& entry : entries) {
        EXPECT_EQ(entry.transaction_id, tx_id);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(WALManagerTest, EmptyDirectory) {
    // Should handle empty WAL directory
    WALManager wal(config_);
    
    auto stats = wal.getStatistics();
    EXPECT_EQ(stats.total_entries, 0);
    EXPECT_EQ(stats.current_lsn.segment, 0);
    EXPECT_EQ(stats.current_lsn.offset, 0);
}

TEST_F(WALManagerTest, ReadNonExistentEntry) {
    WALManager wal(config_);
    
    LSN lsn(999, 999);
    auto entry = wal.read(lsn);
    EXPECT_FALSE(entry.has_value());
}

TEST_F(WALManagerTest, FlushEmptyBuffer) {
    WALManager wal(config_);
    
    // Should not crash
    wal.flush();
    wal.flush();
}

TEST_F(WALManagerTest, MaxSegmentsCleanup) {
    config_.max_segments = 3;
    WALManager wal(config_);
    
    // Force creation of many segments
    for (int i = 0; i < 200; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}, {"pad", std::string(100, 'x')}};
        wal.append(entry);
    }
    
    wal.flush();
    
    auto stats = wal.getStatistics();
    EXPECT_LE(stats.segments, config_.max_segments);
}

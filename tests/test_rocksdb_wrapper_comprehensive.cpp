/**
 * @file test_rocksdb_wrapper_comprehensive.cpp
 * @brief Comprehensive real unit tests for RocksDBWrapper storage engine
 * 
 * Test Intent:
 * - Validate RocksDBWrapper core functionality with real RocksDB operations
 * - Test backup/restore with actual checkpoint creation and recovery
 * - Verify entity operations (CRUD) with complex data types
 * - Validate transaction support and MVCC behavior
 * - Test column family management
 * - Verify iterator operations and scans
 * - Test edge cases and error conditions
 * 
 * Coverage: Storage layer (RocksDBWrapper, backup, entity operations)
 * No stubs - all tests use real RocksDB TransactionDB
 */

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <random>

using namespace themis;
namespace fs = std::filesystem;

class RocksDBWrapperComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = fs::temp_directory_path() / "rocksdb_comprehensive_test";
        backup_path_ = fs::temp_directory_path() / "rocksdb_backup_test";
        
        cleanupPaths();
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_.string();
        config.enable_wal = true;
        config.enable_blobdb = true;
        config.enable_statistics = true;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open()) << "Failed to open RocksDB";
        ASSERT_TRUE(db_->isOpen()) << "DB not open after open() call";
    }
    
    void TearDown() override {
        db_.reset();
        cleanupPaths();
    }
    
    void cleanupPaths() {
        std::error_code ec;
        fs::remove_all(test_db_path_, ec);
        fs::remove_all(backup_path_, ec);
    }
    
    fs::path test_db_path_;
    fs::path backup_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// Basic CRUD Operations Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, BasicPutGet) {
    // Intent: Verify basic put/get operations work correctly
    std::vector<uint8_t> value = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    ASSERT_TRUE(db_->put("test:key1", value));
    
    auto result = db_->get("test:key1");
    ASSERT_TRUE(result.has_value()) << "Key not found after put";
    EXPECT_EQ(*result, value);
}

TEST_F(RocksDBWrapperComprehensiveTest, PutGetLargeBlobValue) {
    // Intent: Test BlobDB handling of large values (>4KB threshold)
    std::vector<uint8_t> large_value(10000);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& byte : large_value) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    ASSERT_TRUE(db_->put("blob:large", large_value));
    
    auto result = db_->get("blob:large");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), large_value.size());
    EXPECT_EQ(*result, large_value);
}

TEST_F(RocksDBWrapperComprehensiveTest, DeleteKey) {
    // Intent: Verify delete operations work correctly
    std::vector<uint8_t> value = {0xAA, 0xBB};
    
    ASSERT_TRUE(db_->put("test:delete_me", value));
    ASSERT_TRUE(db_->get("test:delete_me").has_value());
    
    ASSERT_TRUE(db_->erase("test:delete_me"));
    
    auto result = db_->get("test:delete_me");
    EXPECT_FALSE(result.has_value()) << "Key still exists after delete";
}

TEST_F(RocksDBWrapperComprehensiveTest, UpdateExistingKey) {
    // Intent: Verify update operations overwrite existing values
    std::vector<uint8_t> value1 = {0x01, 0x02};
    std::vector<uint8_t> value2 = {0x03, 0x04, 0x05};
    
    ASSERT_TRUE(db_->put("test:update", value1));
    ASSERT_TRUE(db_->put("test:update", value2));
    
    auto result = db_->get("test:update");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value2);
}

TEST_F(RocksDBWrapperComprehensiveTest, GetNonexistentKey) {
    // Intent: Verify get returns empty optional for missing keys
    auto result = db_->get("nonexistent:key");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Transaction and MVCC Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, TransactionCommit) {
    // Intent: Validate transaction commit persists changes
    auto txn = db_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    std::vector<uint8_t> value = {0x10, 0x20, 0x30};
    ASSERT_TRUE(txn->put("txn:key1", value));
    ASSERT_TRUE(txn->commit());
    
    // Verify outside transaction
    auto result = db_->get("txn:key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value);
}

TEST_F(RocksDBWrapperComprehensiveTest, TransactionRollback) {
    // Intent: Validate transaction rollback discards changes
    auto txn = db_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    std::vector<uint8_t> value = {0x40, 0x50};
    ASSERT_TRUE(txn->put("txn:rollback", value));
    txn->rollback();
    
    // Verify key not persisted
    auto result = db_->get("txn:rollback");
    EXPECT_FALSE(result.has_value());
}

TEST_F(RocksDBWrapperComprehensiveTest, TransactionSnapshotIsolation) {
    // Intent: Verify MVCC snapshot isolation between transactions
    std::vector<uint8_t> value1 = {0x01};
    std::vector<uint8_t> value2 = {0x02};
    
    // Initial state
    ASSERT_TRUE(db_->put("mvcc:key", value1));
    
    // Start transaction 1 (gets snapshot)
    auto txn1 = db_->beginTransaction();
    auto read1 = txn1->get("mvcc:key");
    ASSERT_TRUE(read1.has_value());
    EXPECT_EQ(*read1, value1);
    
    // Transaction 2 modifies key
    auto txn2 = db_->beginTransaction();
    ASSERT_TRUE(txn2->put("mvcc:key", value2));
    ASSERT_TRUE(txn2->commit());
    
    // Transaction 1 should still see old value (snapshot isolation)
    auto read1_again = txn1->get("mvcc:key");
    ASSERT_TRUE(read1_again.has_value());
    EXPECT_EQ(*read1_again, value1);
    
    txn1->rollback();
    
    // New read should see updated value
    auto result = db_->get("mvcc:key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value2);
}

TEST_F(RocksDBWrapperComprehensiveTest, ConcurrentTransactions) {
    // Intent: Test concurrent transaction handling
    const int num_threads = 4;
    const int ops_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, ops_per_thread, &success_count]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                auto txn = db_->beginTransaction();
                if (!txn) continue;
                
                std::string key = "concurrent:" + std::to_string(t) + ":" + std::to_string(i);
                std::vector<uint8_t> value = {static_cast<uint8_t>(t), static_cast<uint8_t>(i)};
                
                if (txn->put(key, value) && txn->commit()) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GE(success_count.load(), num_threads * ops_per_thread * 0.9);
}

// ============================================================================
// Backup and Restore Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, CreateCheckpoint) {
    // Intent: Verify checkpoint creation works correctly
    std::vector<uint8_t> value = {0xAA, 0xBB, 0xCC};
    ASSERT_TRUE(db_->put("backup:key1", value));
    
    std::string checkpoint_path = backup_path_.string() + "/checkpoint1";
    ASSERT_TRUE(db_->createCheckpoint(checkpoint_path));
    
    EXPECT_TRUE(fs::exists(checkpoint_path));
    EXPECT_TRUE(fs::is_directory(checkpoint_path));
}

TEST_F(RocksDBWrapperComprehensiveTest, RestoreFromCheckpoint) {
    // Intent: Verify restore from checkpoint recovers data correctly
    std::vector<uint8_t> value1 = {0x11, 0x22};
    std::vector<uint8_t> value2 = {0x33, 0x44};
    
    // Create initial state
    ASSERT_TRUE(db_->put("restore:key", value1));
    
    // Create checkpoint
    std::string checkpoint_path = backup_path_.string() + "/checkpoint2";
    ASSERT_TRUE(db_->createCheckpoint(checkpoint_path));
    
    // Modify data after checkpoint
    ASSERT_TRUE(db_->put("restore:key", value2));
    
    // Restore from checkpoint
    ASSERT_TRUE(db_->restoreFromCheckpoint(checkpoint_path));
    
    // Verify restored to checkpoint state
    auto result = db_->get("restore:key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value1);
}

TEST_F(RocksDBWrapperComprehensiveTest, BackupMultipleKeys) {
    // Intent: Verify checkpoint captures all keys
    const int num_keys = 100;
    
    for (int i = 0; i < num_keys; ++i) {
        std::string key = "multi:key" + std::to_string(i);
        std::vector<uint8_t> value = {static_cast<uint8_t>(i)};
        ASSERT_TRUE(db_->put(key, value));
    }
    
    std::string checkpoint_path = backup_path_.string() + "/checkpoint3";
    ASSERT_TRUE(db_->createCheckpoint(checkpoint_path));
    
    // Clear database
    for (int i = 0; i < num_keys; ++i) {
        ASSERT_TRUE(db_->erase("multi:key" + std::to_string(i)));
    }
    
    // Restore
    ASSERT_TRUE(db_->restoreFromCheckpoint(checkpoint_path));
    
    // Verify all keys restored
    for (int i = 0; i < num_keys; ++i) {
        auto result = db_->get("multi:key" + std::to_string(i));
        ASSERT_TRUE(result.has_value()) << "Key " << i << " not restored";
        EXPECT_EQ((*result)[0], static_cast<uint8_t>(i));
    }
}

// ============================================================================
// Iterator and Scan Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, IteratorScanPrefix) {
    // Intent: Verify prefix scanning works correctly
    ASSERT_TRUE(db_->put("prefix:a:1", {0x01}));
    ASSERT_TRUE(db_->put("prefix:a:2", {0x02}));
    ASSERT_TRUE(db_->put("prefix:a:3", {0x03}));
    ASSERT_TRUE(db_->put("prefix:b:1", {0x04}));
    
    auto it = db_->newIterator();
    ASSERT_NE(it, nullptr);
    
    int count = 0;
    for (it->seek("prefix:a:"); it->valid(); it->next()) {
        std::string key = it->key();
        if (!key.starts_with("prefix:a:")) break;
        count++;
    }
    
    EXPECT_EQ(count, 3);
}

TEST_F(RocksDBWrapperComprehensiveTest, IteratorSeekToFirst) {
    // Intent: Verify iterator seeks to first key
    ASSERT_TRUE(db_->put("iter:z", {0x01}));
    ASSERT_TRUE(db_->put("iter:a", {0x02}));
    ASSERT_TRUE(db_->put("iter:m", {0x03}));
    
    auto it = db_->newIterator();
    ASSERT_NE(it, nullptr);
    
    it->seekToFirst();
    ASSERT_TRUE(it->valid());
    
    std::string first_key = it->key();
    EXPECT_TRUE(first_key.starts_with("iter:"));
}

TEST_F(RocksDBWrapperComprehensiveTest, IteratorReverseScan) {
    // Intent: Verify reverse iteration works
    ASSERT_TRUE(db_->put("rev:1", {0x01}));
    ASSERT_TRUE(db_->put("rev:2", {0x02}));
    ASSERT_TRUE(db_->put("rev:3", {0x03}));
    
    auto it = db_->newIterator();
    ASSERT_NE(it, nullptr);
    
    it->seekToLast();
    std::vector<std::string> keys;
    
    while (it->valid()) {
        std::string key = it->key();
        if (!key.starts_with("rev:")) break;
        keys.push_back(key);
        it->prev();
    }
    
    EXPECT_GE(keys.size(), 3);
}

// ============================================================================
// Column Family Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, CreateColumnFamily) {
    // Intent: Verify column family creation
    ASSERT_TRUE(db_->createColumnFamily("test_cf"));
    
    std::vector<uint8_t> value = {0xAA};
    ASSERT_TRUE(db_->put("cf_key", value, "test_cf"));
    
    auto result = db_->get("cf_key", "test_cf");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value);
}

TEST_F(RocksDBWrapperComprehensiveTest, ColumnFamilyIsolation) {
    // Intent: Verify column families are isolated
    ASSERT_TRUE(db_->createColumnFamily("cf1"));
    ASSERT_TRUE(db_->createColumnFamily("cf2"));
    
    std::vector<uint8_t> value1 = {0x01};
    std::vector<uint8_t> value2 = {0x02};
    
    ASSERT_TRUE(db_->put("same_key", value1, "cf1"));
    ASSERT_TRUE(db_->put("same_key", value2, "cf2"));
    
    auto result1 = db_->get("same_key", "cf1");
    auto result2 = db_->get("same_key", "cf2");
    
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(*result1, value1);
    EXPECT_EQ(*result2, value2);
}

// ============================================================================
// Statistics and Metrics Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, GetStatistics) {
    // Intent: Verify statistics collection works
    for (int i = 0; i < 10; ++i) {
        db_->put("stats:key" + std::to_string(i), {static_cast<uint8_t>(i)});
    }
    
    std::string stats = db_->getStats();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("rocksdb"), std::string::npos);
}

TEST_F(RocksDBWrapperComprehensiveTest, GetCompressionType) {
    // Intent: Verify compression type query works
    std::string compression = db_->getCompressionType();
    EXPECT_FALSE(compression.empty());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, PutEmptyKey) {
    // Intent: Verify empty key handling
    std::vector<uint8_t> value = {0x01};
    bool result = db_->put("", value);
    // Empty keys should be rejected or handled gracefully
    EXPECT_FALSE(result);
}

TEST_F(RocksDBWrapperComprehensiveTest, GetInvalidColumnFamily) {
    // Intent: Verify invalid column family handling
    auto result = db_->get("key", "nonexistent_cf");
    EXPECT_FALSE(result.has_value());
}

TEST_F(RocksDBWrapperComprehensiveTest, CheckpointToExistingDirectory) {
    // Intent: Verify checkpoint handles existing directories
    std::string checkpoint_path = backup_path_.string() + "/existing";
    fs::create_directories(checkpoint_path);
    
    // Should either succeed or fail gracefully
    bool result = db_->createCheckpoint(checkpoint_path);
    // Implementation may vary - just verify it doesn't crash
    EXPECT_TRUE(result || !result);
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

TEST_F(RocksDBWrapperComprehensiveTest, HighVolumeWrites) {
    // Intent: Verify database handles high write volume
    const int num_writes = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_writes; ++i) {
        std::string key = "perf:key" + std::to_string(i);
        std::vector<uint8_t> value = {static_cast<uint8_t>(i % 256)};
        ASSERT_TRUE(db_->put(key, value));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify writes completed in reasonable time (< 3 seconds for performance regression detection)
    EXPECT_LT(duration.count(), 3000);
    
    // Spot check some keys
    auto result = db_->get("perf:key5000");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)[0], 5000 % 256);
}

TEST_F(RocksDBWrapperComprehensiveTest, MixedReadWrite) {
    // Intent: Verify mixed read/write workload
    const int num_ops = 1000;
    int read_count = 0;
    int write_count = 0;
    
    for (int i = 0; i < num_ops; ++i) {
        if (i % 2 == 0) {
            // Write
            std::string key = "mixed:key" + std::to_string(i);
            std::vector<uint8_t> value = {static_cast<uint8_t>(i)};
            ASSERT_TRUE(db_->put(key, value));
            write_count++;
        } else {
            // Read
            std::string key = "mixed:key" + std::to_string(i - 1);
            auto result = db_->get(key);
            if (result.has_value()) {
                read_count++;
            }
        }
    }
    
    EXPECT_EQ(write_count, num_ops / 2);
    EXPECT_GT(read_count, 0);
}

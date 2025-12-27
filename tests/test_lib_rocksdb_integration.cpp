#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>

#include "storage/rocksdb_wrapper.h"

using namespace themis;

// Test fixture for RocksDB library integration
class RocksDBLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_lib_rocksdb_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(test_db_path_);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_db_path_)) {
            std::error_code ec;
            std::filesystem::remove_all(test_db_path_, ec);
        }
    }

    std::string test_db_path_;
};

// Test 1: Library linking and basic initialization
TEST_F(RocksDBLibIntegrationTest, LibraryLinking) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.memtable_size_mb = 64;
    cfg.block_cache_size_mb = 128;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) << "RocksDB library failed to initialize";
}

// Test 2: Basic CRUD operations (verifying RocksDB API integration)
TEST_F(RocksDBLibIntegrationTest, BasicCRUDOperations) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_wal = true;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Test PUT operation
    EXPECT_TRUE(wrapper.put("test_key", "test_value"));
    
    // Test GET operation
    std::string value;
    EXPECT_TRUE(wrapper.get("test_key", value));
    EXPECT_EQ(value, "test_value");
    
    // Test DELETE operation
    EXPECT_TRUE(wrapper.remove("test_key"));
    EXPECT_FALSE(wrapper.get("test_key", value));
}

// Test 3: Transaction support (RocksDB TransactionDB API)
TEST_F(RocksDBLibIntegrationTest, TransactionSupport) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Start transaction
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr) << "Transaction creation failed";
    
    // Perform operations in transaction
    EXPECT_TRUE(wrapper.put("txn_key", "txn_value", txn.get()));
    
    // Commit transaction
    EXPECT_TRUE(wrapper.commit(txn.get()));
    
    // Verify data persisted
    std::string value;
    EXPECT_TRUE(wrapper.get("txn_key", value));
    EXPECT_EQ(value, "txn_value");
}

// Test 4: Batch write operations (WriteBatch API)
TEST_F(RocksDBLibIntegrationTest, BatchWriteOperations) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    auto batch = wrapper.createWriteBatch();
    ASSERT_NE(batch, nullptr);
    
    // Add multiple operations to batch
    wrapper.putBatch(batch.get(), "batch_key1", "value1");
    wrapper.putBatch(batch.get(), "batch_key2", "value2");
    wrapper.putBatch(batch.get(), "batch_key3", "value3");
    
    // Execute batch
    EXPECT_TRUE(wrapper.writeBatch(batch.get()));
    
    // Verify all keys exist
    std::string value;
    EXPECT_TRUE(wrapper.get("batch_key1", value));
    EXPECT_EQ(value, "value1");
    EXPECT_TRUE(wrapper.get("batch_key2", value));
    EXPECT_EQ(value, "value2");
    EXPECT_TRUE(wrapper.get("batch_key3", value));
    EXPECT_EQ(value, "value3");
}

// Test 5: Iterator support (RocksDB Iterator API)
TEST_F(RocksDBLibIntegrationTest, IteratorSupport) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Insert test data
    wrapper.put("key_a", "value_a");
    wrapper.put("key_b", "value_b");
    wrapper.put("key_c", "value_c");
    
    // Create iterator
    auto it = wrapper.createIterator();
    ASSERT_NE(it, nullptr);
    
    // Verify iteration works
    int count = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        count++;
    }
    EXPECT_GE(count, 3) << "Iterator should find at least 3 keys";
}

// Test 6: WAL (Write-Ahead Log) functionality
TEST_F(RocksDBLibIntegrationTest, WALFunctionality) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_wal = true;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Write data with WAL enabled
    EXPECT_TRUE(wrapper.put("wal_key", "wal_value"));
    
    // Close and reopen to verify WAL recovery
    wrapper.close();
    
    RocksDBWrapper wrapper2(cfg);
    ASSERT_TRUE(wrapper2.open());
    
    std::string value;
    EXPECT_TRUE(wrapper2.get("wal_key", value));
    EXPECT_EQ(value, "wal_value") << "WAL recovery failed";
}

// Test 7: Snapshot functionality (MVCC support)
TEST_F(RocksDBLibIntegrationTest, SnapshotFunctionality) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Write initial data
    wrapper.put("snap_key", "initial_value");
    
    // Create snapshot
    auto snapshot = wrapper.getSnapshot();
    ASSERT_NE(snapshot, nullptr);
    
    // Modify data after snapshot
    wrapper.put("snap_key", "modified_value");
    
    // Read from snapshot should return old value
    std::string value;
    EXPECT_TRUE(wrapper.getFromSnapshot(snapshot, "snap_key", value));
    EXPECT_EQ(value, "initial_value");
    
    // Regular read should return new value
    EXPECT_TRUE(wrapper.get("snap_key", value));
    EXPECT_EQ(value, "modified_value");
    
    wrapper.releaseSnapshot(snapshot);
}

// Test 8: Compression configuration
TEST_F(RocksDBLibIntegrationTest, CompressionConfiguration) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.compression_default = "lz4";
    cfg.compression_bottommost = "zstd";
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) << "Failed to open with compression settings";
    
    // Write compressible data
    std::string large_value(10000, 'A');
    EXPECT_TRUE(wrapper.put("compress_key", large_value));
    
    std::string value;
    EXPECT_TRUE(wrapper.get("compress_key", value));
    EXPECT_EQ(value, large_value);
}

// Test 9: BlobDB functionality for large values
TEST_F(RocksDBLibIntegrationTest, BlobDBFunctionality) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_blobdb = true;
    cfg.blob_size_threshold = 4096;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Write large value that should go to BlobDB
    std::string large_value(10000, 'B');
    EXPECT_TRUE(wrapper.put("blob_key", large_value));
    
    std::string value;
    EXPECT_TRUE(wrapper.get("blob_key", value));
    EXPECT_EQ(value, large_value);
}

// Test 10: Concurrent operations (thread safety)
TEST_F(RocksDBLibIntegrationTest, ConcurrentOperations) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.allow_concurrent_memtable_write = true;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    const int num_threads = 4;
    const int ops_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&wrapper, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
                std::string value = "value_" + std::to_string(i);
                wrapper.put(key, value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify some random keys
    std::string value;
    EXPECT_TRUE(wrapper.get("thread_0_key_0", value));
    EXPECT_TRUE(wrapper.get("thread_2_key_50", value));
}

// Test 11: Performance tuning parameters validation
TEST_F(RocksDBLibIntegrationTest, PerformanceTuningParameters) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_high_parallel_tuning = true;
    cfg.max_background_jobs = 8;
    cfg.enable_pipelined_write = true;
    
    RocksDBWrapper wrapper(cfg);
    const auto& applied_cfg = wrapper.getConfig();
    
    // Verify performance tuning was applied
    EXPECT_TRUE(applied_cfg.enable_high_parallel_tuning);
    EXPECT_TRUE(applied_cfg.enable_pipelined_write);
    EXPECT_GT(applied_cfg.background_threads_low, 0);
}

// Test 12: Async I/O configuration
TEST_F(RocksDBLibIntegrationTest, AsyncIOConfiguration) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_async_io = true;
    cfg.async_io_readahead_size_mb = 64;
    cfg.async_io_num_threads = 4;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) << "Failed to open with async I/O settings";
    
    // Insert and retrieve data
    wrapper.put("async_key", "async_value");
    std::string value;
    EXPECT_TRUE(wrapper.get("async_key", value));
    EXPECT_EQ(value, "async_value");
}

// Test 13: Column family support
TEST_F(RocksDBLibIntegrationTest, ColumnFamilySupport) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Create column family
    auto cf_handle = wrapper.createColumnFamily("test_cf");
    ASSERT_NE(cf_handle, nullptr) << "Failed to create column family";
    
    // Write to column family
    EXPECT_TRUE(wrapper.put("cf_key", "cf_value", nullptr, cf_handle));
    
    // Read from column family
    std::string value;
    EXPECT_TRUE(wrapper.get("cf_key", value, cf_handle));
    EXPECT_EQ(value, "cf_value");
}

// Test 14: Rollback transaction support
TEST_F(RocksDBLibIntegrationTest, TransactionRollback) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Write initial data
    wrapper.put("rollback_key", "original_value");
    
    // Start transaction
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    // Modify in transaction
    EXPECT_TRUE(wrapper.put("rollback_key", "modified_value", txn.get()));
    
    // Rollback
    EXPECT_TRUE(wrapper.rollback(txn.get()));
    
    // Verify original value is preserved
    std::string value;
    EXPECT_TRUE(wrapper.get("rollback_key", value));
    EXPECT_EQ(value, "original_value");
}

// Test 15: Error handling and edge cases
TEST_F(RocksDBLibIntegrationTest, ErrorHandlingEdgeCases) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Test getting non-existent key
    std::string value;
    EXPECT_FALSE(wrapper.get("non_existent_key", value));
    
    // Test deleting non-existent key (should succeed)
    EXPECT_TRUE(wrapper.remove("non_existent_key"));
    
    // Test empty key
    EXPECT_TRUE(wrapper.put("", "empty_key_value"));
    EXPECT_TRUE(wrapper.get("", value));
    EXPECT_EQ(value, "empty_key_value");
    
    // Test empty value
    EXPECT_TRUE(wrapper.put("empty_value_key", ""));
    EXPECT_TRUE(wrapper.get("empty_value_key", value));
    EXPECT_EQ(value, "");
}

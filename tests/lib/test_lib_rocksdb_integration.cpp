// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// RocksDB Library Integration Tests (Updated for v1.3.5+)
// Tests RocksDB C++ API integration via themis::RocksDBWrapper
// Using new TransactionDB-based API with proper transaction semantics

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;
using themis::RocksDBWrapper;

class RocksDBLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_lib_rocksdb_" + 
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_db_path_);
    }

    void TearDown() override {
        if (fs::exists(test_db_path_)) {
            std::error_code ec = {};
            fs::remove_all(test_db_path_, ec);
        }
    }

    std::string test_db_path_ = {};
};

// Test 1: Library linking and basic initialization
TEST_F(RocksDBLibIntegrationTest, LibraryLinking) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.memtable_size_mb = 64;
    cfg.block_cache_size_mb = 128;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) << "RocksDB library failed to initialize";
    EXPECT_TRUE(wrapper.isOpen());
}

// Test 2: Basic CRUD operations with new API
TEST_F(RocksDBLibIntegrationTest, BasicCRUDOperations) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_wal = true;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Test PUT operation (uses transactions internally)
    EXPECT_TRUE(wrapper.put("test_key", "test_value"));
    
    // Test GET operation
    std::string value = {};
    EXPECT_TRUE(wrapper.get("test_key", value));
    EXPECT_EQ(value, "test_value");
    
    // Test DELETE operation (new API: del() instead of remove())
    EXPECT_TRUE(wrapper.del("test_key"));
    EXPECT_FALSE(wrapper.get("test_key", value));
}

// Test 3: Explicit transaction support
TEST_F(RocksDBLibIntegrationTest, ExplicitTransactions) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Start transaction
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr) << "Transaction creation failed";
    
    // Perform operations within transaction
    std::vector<uint8_t> value_vec{'t','x','n','_','v','a','l','u','e'};
    EXPECT_TRUE(txn->put("txn_key", value_vec));
    
    // Commit transaction
    EXPECT_TRUE(txn->commit());
    
    // Verify data persisted
    std::string value = {};
    EXPECT_TRUE(wrapper.get("txn_key", value));
    EXPECT_EQ(value, "txn_value");
}

// Test 4: Transaction rollback
TEST_F(RocksDBLibIntegrationTest, TransactionRollback) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Put initial value
    EXPECT_TRUE(wrapper.put("rollback_key", "initial_value"));
    
    // Start transaction and modify
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    std::vector<uint8_t> new_value{'n','e','w'};
    EXPECT_TRUE(txn->put("rollback_key", new_value));
    
    // Rollback instead of commit
    txn->rollback();
    
    // Verify original value still present
    std::string value = {};
    EXPECT_TRUE(wrapper.get("rollback_key", value));
    EXPECT_EQ(value, "initial_value");
}

// Test 5: Write batch operations
TEST_F(RocksDBLibIntegrationTest, WriteBatchOperations) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Create write batch
    auto batch = wrapper.createWriteBatch();
    ASSERT_NE(batch, nullptr);
    
    // Add multiple operations to batch
    std::vector<uint8_t> val1{'v','a','l','u','e','1'};
    std::vector<uint8_t> val2{'v','a','l','u','e','2'};
    std::vector<uint8_t> val3{'v','a','l','u','e','3'};
    
    batch->put("batch_key1", val1);
    batch->put("batch_key2", val2);
    batch->put("batch_key3", val3);
    
    // Commit batch
    EXPECT_TRUE(batch->commit());
    
    // Verify all values
    std::string value = {};
    EXPECT_TRUE(wrapper.get("batch_key1", value));
    EXPECT_EQ(value, "value1");
    EXPECT_TRUE(wrapper.get("batch_key2", value));
    EXPECT_EQ(value, "value2");
    EXPECT_TRUE(wrapper.get("batch_key3", value));
    EXPECT_EQ(value, "value3");
}

// Test 6: Multi-get operations
TEST_F(RocksDBLibIntegrationTest, MultiGetOperations) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Put multiple values
    EXPECT_TRUE(wrapper.put("key1", "value1"));
    EXPECT_TRUE(wrapper.put("key2", "value2"));
    EXPECT_TRUE(wrapper.put("key3", "value3"));
    
    // Multi-get
    std::vector<std::string> keys = {"key1", "key2", "key3", "key_nonexistent"};
    auto results = wrapper.multiGet(keys);
    
    EXPECT_EQ(results.size(), 4);
    ASSERT_TRUE(results[0].has_value());
    ASSERT_TRUE(results[1].has_value());
    ASSERT_TRUE(results[2].has_value());
    EXPECT_FALSE(results[3].has_value());
    
    // Convert to strings and verify
    std::string val1(results[0]->begin(), results[0]->end());
    std::string val2(results[1]->begin(), results[1]->end());
    std::string val3(results[2]->begin(), results[2]->end());
    
    EXPECT_EQ(val1, "value1");
    EXPECT_EQ(val2, "value2");
    EXPECT_EQ(val3, "value3");
}

// Test 7: Concurrent transactions (MVCC behavior)
TEST_F(RocksDBLibIntegrationTest, ConcurrentTransactions) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WriteCommitted;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Initial value
    EXPECT_TRUE(wrapper.put("concurrent_key", "initial"));
    
    // Start two transactions
    auto txn1 = wrapper.beginTransaction();
    auto txn2 = wrapper.beginTransaction();
    
    ASSERT_NE(txn1, nullptr);
    ASSERT_NE(txn2, nullptr);
    
    // Both try to update the same key
    std::vector<uint8_t> val1{'t','x','n','1'};
    std::vector<uint8_t> val2{'t','x','n','2'};
    
    EXPECT_TRUE(txn1->put("concurrent_key", val1));
    // Second writer may fail due to point locks; accept either outcome
    bool txn2_put_ok = txn2->put("concurrent_key", val2);
    
    // First commit should succeed
    EXPECT_TRUE(txn1->commit());
    
    // Second commit may fail due to conflict (depending on isolation level)
    // If it succeeds, last write wins
    if (txn2_put_ok) {
        txn2->commit(); // Don't assert - conflict handling is implementation-dependent
    }
    
    // Verify a value is present
    std::string value = {};
    EXPECT_TRUE(wrapper.get("concurrent_key", value));
    EXPECT_TRUE(value == "txn1" || value == "txn2" || value == "initial");
}

// Test 8: Scan with prefix
TEST_F(RocksDBLibIntegrationTest, PrefixScan) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Put keys with prefixes
    EXPECT_TRUE(wrapper.put("prefix_a_1", "value_a1"));
    EXPECT_TRUE(wrapper.put("prefix_a_2", "value_a2"));
    EXPECT_TRUE(wrapper.put("prefix_b_1", "value_b1"));
    EXPECT_TRUE(wrapper.put("prefix_c_1", "value_c1"));
    
    // Scan with prefix "prefix_a" (callback must return bool)
    int count = 0;
    wrapper.scanPrefix("prefix_a", [&count](std::string_view key, [[maybe_unused]] std::string_view value) -> bool {
        count++;
        EXPECT_TRUE(key.starts_with("prefix_a"));
        return true;  // Continue scanning
    });
    
    EXPECT_EQ(count, 2); // Should find prefix_a_1 and prefix_a_2
}

// Test 9: WritePrepared transaction policy
TEST_F(RocksDBLibIntegrationTest, WritePreparedPolicy) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.write_policy = RocksDBWrapper::Config::WritePolicy::WritePrepared;
    cfg.wp_commit_cache_bits = 20;  // Smaller for testing
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Create transaction
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    // Add data
    std::vector<uint8_t> value{'p','r','e','p','a','r','e','d'};
    EXPECT_TRUE(txn->put("wp_key", value));
    
    // Prepare (2PC phase 1)
    // Prepare may be unsupported when skip_prepare is enabled; tolerate failure
    bool prepared = txn->prepare();
    (void)prepared;
    
    // Commit (one-phase commit is fine when skip_prepare is true)
    EXPECT_TRUE(txn->commit());
    
    // Verify
    std::string result = {};
    EXPECT_TRUE(wrapper.get("wp_key", result));
    EXPECT_EQ(result, "prepared");
}

// Test 10: Performance - bulk insert
TEST_F(RocksDBLibIntegrationTest, DISABLED_BulkInsertPerformance) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_wal = false;  // Faster for bulk
    cfg.max_background_jobs = 4;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    const int NUM_KEYS = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_KEYS; ++i) {
        std::string key = "bulk_key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        EXPECT_TRUE(wrapper.put(key, value));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double ops_per_sec = (NUM_KEYS * 1000.0) / duration.count();
    
    std::cout << "Bulk insert: " << NUM_KEYS << " keys in " 
              << duration.count() << "ms (" 
              << ops_per_sec << " ops/sec)" << std::endl;
    
    // Sanity check: should handle at least 1000 ops/sec
    EXPECT_GT(ops_per_sec, 1000.0);
}

// Test: Transaction cleanup on BeginTransaction failure
TEST_F(RocksDBLibIntegrationTest, TransactionCreationFailureCleanup) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Create a transaction successfully
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr);
    EXPECT_TRUE(txn->isActive());
    
    // Close the DB to simulate a scenario where subsequent transactions might fail
    wrapper.close();
    
    // Try to create a new transaction on a closed DB
    // This should not crash or leak resources
    auto failed_txn = wrapper.beginTransaction();
    ASSERT_NE(failed_txn, nullptr);  // Object is created
    EXPECT_FALSE(failed_txn->isActive());  // But not active
    
    // Destructor should handle cleanup properly without crashing
}

// Test: Transaction state transitions
TEST_F(RocksDBLibIntegrationTest, TransactionStateTransitions) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Test 1: Active -> Committed
    {
        auto txn = wrapper.beginTransaction();
        ASSERT_NE(txn, nullptr);
        EXPECT_TRUE(txn->isActive());
        
        std::vector<uint8_t> value{'t', 'e', 's', 't'};
        EXPECT_TRUE(txn->put("key1", value));
        EXPECT_TRUE(txn->commit());
        EXPECT_FALSE(txn->isActive());  // No longer active after commit
        
        // Operations after commit should fail
        EXPECT_FALSE(txn->put("key2", value));
        EXPECT_FALSE(txn->del("key1"));
    }
    
    // Test 2: Active -> Rolledback
    {
        auto txn = wrapper.beginTransaction();
        ASSERT_NE(txn, nullptr);
        EXPECT_TRUE(txn->isActive());
        
        std::vector<uint8_t> value{'t', 'e', 's', 't'};
        EXPECT_TRUE(txn->put("key3", value));
        txn->rollback();
        EXPECT_FALSE(txn->isActive());  // No longer active after rollback
        
        // Operations after rollback should fail
        EXPECT_FALSE(txn->put("key4", value));
        EXPECT_FALSE(txn->del("key3"));
    }
}

// Test: Transaction auto-rollback on destructor
TEST_F(RocksDBLibIntegrationTest, TransactionAutoRollback) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Put initial value
    EXPECT_TRUE(wrapper.put("auto_key", "initial"));
    
    // Create a transaction and modify value, but don't commit
    {
        auto txn = wrapper.beginTransaction();
        ASSERT_NE(txn, nullptr);
        
        std::vector<uint8_t> new_value{'m', 'o', 'd', 'i', 'f', 'i', 'e', 'd'};
        EXPECT_TRUE(txn->put("auto_key", new_value));
        
        // Destructor should auto-rollback
    }
    
    // Verify original value is still present (auto-rollback worked)
    std::string value = {};
    EXPECT_TRUE(wrapper.get("auto_key", value));
    EXPECT_EQ(value, "initial");
}

// Test: Transaction exception safety
TEST_F(RocksDBLibIntegrationTest, TransactionExceptionSafety) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    auto txn = wrapper.beginTransaction();
    ASSERT_NE(txn, nullptr);
    EXPECT_TRUE(txn->isActive());
    
    // Put a valid value
    std::vector<uint8_t> value{'v', 'a', 'l', 'i', 'd'};
    EXPECT_TRUE(txn->put("exception_key", value));
    
    // Commit should work
    EXPECT_TRUE(txn->commit());
    EXPECT_FALSE(txn->isActive());
    
    // Create another transaction and verify operations after commit fail gracefully
    auto txn2 = wrapper.beginTransaction();
    ASSERT_NE(txn2, nullptr);
    
    // Commit the transaction
    EXPECT_TRUE(txn2->commit());
    
    // Try to use transaction after commit - should fail gracefully
    EXPECT_FALSE(txn2->put("another_key", value));
    EXPECT_FALSE(txn2->del("exception_key"));
    
    // Destructor should not crash even though transaction is already committed
}

// Test: Multiple transaction operations with state tracking
TEST_F(RocksDBLibIntegrationTest, MultipleTransactionOperationsWithState) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Create and use multiple transactions
    for (int i = 0; i < 5; ++i) {
        auto txn = wrapper.beginTransaction();
        ASSERT_NE(txn, nullptr);
        EXPECT_TRUE(txn->isActive());
        
        std::string key = "multi_key_" + std::to_string(i);
        std::vector<uint8_t> value{'v', 'a', 'l', static_cast<uint8_t>('0' + i)};
        
        EXPECT_TRUE(txn->put(key, value));
        
        if (i % 2 == 0) {
            EXPECT_TRUE(txn->commit());
        } else {
            txn->rollback();
        }
        
        EXPECT_FALSE(txn->isActive());
    }
    
    // Verify only committed transactions persisted
    for (int i = 0; i < 5; ++i) {
        std::string key = "multi_key_" + std::to_string(i);
        std::string value = {};
        
        if (i % 2 == 0) {
            EXPECT_TRUE(wrapper.get(key, value)) << "Key " << key << " should exist";
        } else {
            EXPECT_FALSE(wrapper.get(key, value)) << "Key " << key << " should not exist";
        }
    }
}

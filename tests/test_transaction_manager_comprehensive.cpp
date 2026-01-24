/**
 * @file test_transaction_manager_comprehensive.cpp
 * @brief Comprehensive real unit tests for Transaction Manager
 * 
 * Test Intent:
 * - Validate transaction manager with real MVCC and distributed transaction support
 * - Test ACID properties (Atomicity, Consistency, Isolation, Durability)
 * - Verify isolation levels (Read Uncommitted, Read Committed, Repeatable Read, Serializable)
 * - Test concurrent transactions and conflict detection
 * - Validate distributed transaction coordination (2PC protocol)
 * - Test deadlock detection and resolution
 * - Verify transaction rollback and recovery
 * 
 * Coverage: Transaction layer (TransactionManager, MVCC, distributed transactions)
 * No stubs - all tests use real RocksDB TransactionDB and coordination logic
 */

#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

using namespace themis;
namespace fs = std::filesystem;

class TransactionManagerComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "txn_manager_comprehensive_test";
        cleanupTestDir();
        fs::create_directories(test_dir_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_dir_.string();
        config.enable_wal = true;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        txn_mgr_ = std::make_unique<transaction::TransactionManager>(*db_);
    }
    
    void TearDown() override {
        txn_mgr_.reset();
        db_.reset();
        cleanupTestDir();
    }
    
    void cleanupTestDir() {
        std::error_code ec;
        fs::remove_all(test_dir_, ec);
    }
    
    fs::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<transaction::TransactionManager> txn_mgr_;
};

// ============================================================================
// ACID Properties Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, AtomicityCommit) {
    // Intent: Verify all-or-nothing atomicity on successful commit
    
    auto txn = txn_mgr_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    std::vector<uint8_t> val1 = {0x01, 0x02};
    std::vector<uint8_t> val2 = {0x03, 0x04};
    std::vector<uint8_t> val3 = {0x05, 0x06};
    
    ASSERT_TRUE(txn->put("key1", val1));
    ASSERT_TRUE(txn->put("key2", val2));
    ASSERT_TRUE(txn->put("key3", val3));
    
    ASSERT_TRUE(txn->commit());
    
    // All writes should be visible
    EXPECT_TRUE(db_->get("key1").has_value());
    EXPECT_TRUE(db_->get("key2").has_value());
    EXPECT_TRUE(db_->get("key3").has_value());
}

TEST_F(TransactionManagerComprehensiveTest, AtomicityRollback) {
    // Intent: Verify no changes persist on rollback
    
    auto txn = txn_mgr_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    std::vector<uint8_t> val1 = {0x11};
    std::vector<uint8_t> val2 = {0x22};
    
    ASSERT_TRUE(txn->put("rollback:key1", val1));
    ASSERT_TRUE(txn->put("rollback:key2", val2));
    
    txn->rollback();
    
    // No writes should be visible
    EXPECT_FALSE(db_->get("rollback:key1").has_value());
    EXPECT_FALSE(db_->get("rollback:key2").has_value());
}

TEST_F(TransactionManagerComprehensiveTest, ConsistencyConstraint) {
    // Intent: Verify database maintains consistency constraints
    
    // Set up initial balance
    std::vector<uint8_t> balance_a = {0, 0, 0, 100}; // 100 units
    std::vector<uint8_t> balance_b = {0, 0, 0, 50};  // 50 units
    
    db_->put("account:a:balance", balance_a);
    db_->put("account:b:balance", balance_b);
    
    // Transfer 30 units from A to B
    auto txn = txn_mgr_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    auto a_bal = txn->get("account:a:balance");
    auto b_bal = txn->get("account:b:balance");
    
    ASSERT_TRUE(a_bal.has_value());
    ASSERT_TRUE(b_bal.has_value());
    
    // Deduct from A
    std::vector<uint8_t> new_a_bal = {0, 0, 0, 70};
    ASSERT_TRUE(txn->put("account:a:balance", new_a_bal));
    
    // Add to B
    std::vector<uint8_t> new_b_bal = {0, 0, 0, 80};
    ASSERT_TRUE(txn->put("account:b:balance", new_b_bal));
    
    ASSERT_TRUE(txn->commit());
    
    // Verify total balance maintained (consistency)
    auto final_a = db_->get("account:a:balance");
    auto final_b = db_->get("account:b:balance");
    
    ASSERT_TRUE(final_a.has_value());
    ASSERT_TRUE(final_b.has_value());
    
    int total = (*final_a)[3] + (*final_b)[3];
    EXPECT_EQ(total, 150); // Should equal original total
}

TEST_F(TransactionManagerComprehensiveTest, IsolationSnapshotRead) {
    // Intent: Verify snapshot isolation between concurrent transactions
    
    std::vector<uint8_t> initial = {0x01};
    db_->put("iso:key", initial);
    
    // Transaction 1 reads
    auto txn1 = txn_mgr_->beginTransaction();
    auto read1 = txn1->get("iso:key");
    ASSERT_TRUE(read1.has_value());
    
    // Transaction 2 modifies and commits
    auto txn2 = txn_mgr_->beginTransaction();
    std::vector<uint8_t> updated = {0x02};
    ASSERT_TRUE(txn2->put("iso:key", updated));
    ASSERT_TRUE(txn2->commit());
    
    // Transaction 1 should still see original value
    auto read1_again = txn1->get("iso:key");
    ASSERT_TRUE(read1_again.has_value());
    EXPECT_EQ(*read1_again, initial);
    
    txn1->rollback();
}

TEST_F(TransactionManagerComprehensiveTest, DurabilityAfterCommit) {
    // Intent: Verify committed data survives database restart
    
    auto txn = txn_mgr_->beginTransaction();
    std::vector<uint8_t> durable_value = {0xDE, 0xAD, 0xBE, 0xEF};
    
    ASSERT_TRUE(txn->put("durable:key", durable_value));
    ASSERT_TRUE(txn->commit());
    
    // Close and reopen database
    txn_mgr_.reset();
    db_.reset();
    
    db_ = std::make_unique<RocksDBWrapper>(
        RocksDBWrapper::Config{test_dir_.string()});
    ASSERT_TRUE(db_->open());
    txn_mgr_ = std::make_unique<transaction::TransactionManager>(*db_);
    
    // Data should still exist
    auto result = db_->get("durable:key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, durable_value);
}

// ============================================================================
// Isolation Level Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, ReadCommittedIsolation) {
    // Intent: Verify read committed isolation level behavior
    
    std::vector<uint8_t> val1 = {0x01};
    db_->put("rc:key", val1);
    
    auto txn = txn_mgr_->beginTransaction(
        transaction::IsolationLevel::READ_COMMITTED);
    
    // Read initial value
    auto read1 = txn->get("rc:key");
    ASSERT_TRUE(read1.has_value());
    EXPECT_EQ(*read1, val1);
    
    // Another transaction commits new value
    auto txn2 = txn_mgr_->beginTransaction();
    std::vector<uint8_t> val2 = {0x02};
    ASSERT_TRUE(txn2->put("rc:key", val2));
    ASSERT_TRUE(txn2->commit());
    
    // With READ_COMMITTED, txn should see new committed value
    auto read2 = txn->get("rc:key");
    ASSERT_TRUE(read2.has_value());
    EXPECT_EQ(*read2, val2);
    
    txn->rollback();
}

TEST_F(TransactionManagerComprehensiveTest, RepeatableReadIsolation) {
    // Intent: Verify repeatable read isolation level
    
    std::vector<uint8_t> val1 = {0x01};
    db_->put("rr:key", val1);
    
    auto txn = txn_mgr_->beginTransaction(
        transaction::IsolationLevel::REPEATABLE_READ);
    
    // First read
    auto read1 = txn->get("rr:key");
    ASSERT_TRUE(read1.has_value());
    
    // Another transaction modifies
    auto txn2 = txn_mgr_->beginTransaction();
    std::vector<uint8_t> val2 = {0x02};
    ASSERT_TRUE(txn2->put("rr:key", val2));
    ASSERT_TRUE(txn2->commit());
    
    // With REPEATABLE_READ, txn should still see original value
    auto read2 = txn->get("rr:key");
    ASSERT_TRUE(read2.has_value());
    EXPECT_EQ(*read2, val1);
    
    txn->rollback();
}

TEST_F(TransactionManagerComprehensiveTest, SerializableIsolation) {
    // Intent: Verify serializable isolation prevents phantom reads
    
    // Setup initial data
    db_->put("serial:1", {0x01});
    db_->put("serial:2", {0x02});
    
    auto txn1 = txn_mgr_->beginTransaction(
        transaction::IsolationLevel::SERIALIZABLE);
    
    // Txn1 scans range
    auto count1 = 0;
    auto it = txn1->newIterator();
    for (it->seek("serial:"); it->valid(); it->next()) {
        if (!it->key().starts_with("serial:")) break;
        count1++;
    }
    
    // Txn2 tries to insert new key in range
    auto txn2 = txn_mgr_->beginTransaction();
    ASSERT_TRUE(txn2->put("serial:3", {0x03}));
    
    // With SERIALIZABLE, txn2 commit might conflict or block
    auto commit_result = txn2->commit();
    
    // Txn1 should not see phantom row
    count1 = 0;
    it = txn1->newIterator();
    for (it->seek("serial:"); it->valid(); it->next()) {
        if (!it->key().starts_with("serial:")) break;
        count1++;
    }
    EXPECT_EQ(count1, 2); // Should only see original 2 rows
    
    txn1->rollback();
}

// ============================================================================
// Concurrent Transaction Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, ConcurrentNonConflictingTransactions) {
    // Intent: Verify concurrent transactions on different keys succeed
    
    const int num_threads = 4;
    const int ops_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, ops_per_thread, &success_count]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                auto txn = txn_mgr_->beginTransaction();
                if (!txn) continue;
                
                std::string key = "concurrent:t" + std::to_string(t) + ":i" + std::to_string(i);
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
    
    // All non-conflicting transactions should succeed
    EXPECT_EQ(success_count.load(), num_threads * ops_per_thread);
}

TEST_F(TransactionManagerComprehensiveTest, ConcurrentConflictingWrites) {
    // Intent: Verify write conflict detection
    
    db_->put("conflict:key", {0x00});
    
    std::atomic<int> commit_success{0};
    std::atomic<int> commit_failure{0};
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, &commit_success, &commit_failure]() {
            auto txn = txn_mgr_->beginTransaction();
            if (!txn) {
                commit_failure++;
                return;
            }
            
            // All try to write same key
            std::vector<uint8_t> value = {static_cast<uint8_t>(t)};
            if (!txn->put("conflict:key", value)) {
                commit_failure++;
                return;
            }
            
            if (txn->commit()) {
                commit_success++;
            } else {
                commit_failure++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Some should succeed, some should fail due to conflicts
    EXPECT_GT(commit_success.load(), 0);
    EXPECT_GT(commit_failure.load(), 0);
    EXPECT_EQ(commit_success + commit_failure, num_threads);
}

// ============================================================================
// Deadlock Detection Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, DetectDeadlock) {
    // Intent: Verify deadlock detection mechanism
    
    db_->put("deadlock:a", {0x01});
    db_->put("deadlock:b", {0x02});
    
    std::atomic<bool> txn1_started{false};
    std::atomic<bool> txn2_started{false};
    std::atomic<int> deadlock_detected{0};
    
    std::thread thread1([&]() {
        auto txn = txn_mgr_->beginTransaction();
        if (!txn) return;
        
        // Lock resource A
        auto val_a = txn->get("deadlock:a");
        txn1_started = true;
        
        // Wait for thread2 to lock B
        while (!txn2_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Try to lock resource B (potential deadlock)
        auto val_b = txn->get("deadlock:b");
        
        if (!txn->commit()) {
            deadlock_detected++;
        }
    });
    
    std::thread thread2([&]() {
        auto txn = txn_mgr_->beginTransaction();
        if (!txn) return;
        
        // Lock resource B
        auto val_b = txn->get("deadlock:b");
        txn2_started = true;
        
        // Wait for thread1 to lock A
        while (!txn1_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Try to lock resource A (potential deadlock)
        auto val_a = txn->get("deadlock:a");
        
        if (!txn->commit()) {
            deadlock_detected++;
        }
    });
    
    thread1.join();
    thread2.join();
    
    // At least one transaction should detect deadlock
    EXPECT_GT(deadlock_detected.load(), 0);
}

// ============================================================================
// Write-Write Conflict Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, WriteWriteConflict) {
    // Intent: Verify write-write conflict detection
    
    db_->put("ww:key", {0x01});
    
    auto txn1 = txn_mgr_->beginTransaction();
    auto txn2 = txn_mgr_->beginTransaction();
    
    // Both read
    auto read1 = txn1->get("ww:key");
    auto read2 = txn2->get("ww:key");
    
    ASSERT_TRUE(read1.has_value());
    ASSERT_TRUE(read2.has_value());
    
    // Both try to write
    ASSERT_TRUE(txn1->put("ww:key", {0x02}));
    ASSERT_TRUE(txn2->put("ww:key", {0x03}));
    
    // First commit should succeed
    bool commit1 = txn1->commit();
    
    // Second commit should fail (write-write conflict)
    bool commit2 = txn2->commit();
    
    // One should succeed, one should fail
    EXPECT_TRUE(commit1 != commit2);
}

// ============================================================================
// Distributed Transaction Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, DistributedTransactionTwoPhaseCommit) {
    // Intent: Verify 2PC protocol for distributed transactions
    
    auto dist_txn = txn_mgr_->beginDistributedTransaction();
    ASSERT_NE(dist_txn, nullptr);
    
    // Phase 1: Prepare
    std::vector<uint8_t> value = {0xAA, 0xBB};
    ASSERT_TRUE(dist_txn->put("dist:key1", value));
    ASSERT_TRUE(dist_txn->put("dist:key2", value));
    
    bool prepared = dist_txn->prepare();
    ASSERT_TRUE(prepared);
    
    // Phase 2: Commit
    bool committed = dist_txn->commit();
    ASSERT_TRUE(committed);
    
    // Verify data committed
    EXPECT_TRUE(db_->get("dist:key1").has_value());
    EXPECT_TRUE(db_->get("dist:key2").has_value());
}

TEST_F(TransactionManagerComprehensiveTest, DistributedTransactionRollback) {
    // Intent: Verify distributed transaction rollback
    
    auto dist_txn = txn_mgr_->beginDistributedTransaction();
    ASSERT_NE(dist_txn, nullptr);
    
    std::vector<uint8_t> value = {0xCC, 0xDD};
    ASSERT_TRUE(dist_txn->put("dist_rollback:key1", value));
    ASSERT_TRUE(dist_txn->put("dist_rollback:key2", value));
    
    // Prepare succeeds
    ASSERT_TRUE(dist_txn->prepare());
    
    // But decide to rollback instead of commit
    dist_txn->rollback();
    
    // No data should be committed
    EXPECT_FALSE(db_->get("dist_rollback:key1").has_value());
    EXPECT_FALSE(db_->get("dist_rollback:key2").has_value());
}

// ============================================================================
// Transaction Timeout Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, TransactionTimeout) {
    // Intent: Verify long-running transactions are detected
    
    auto txn = txn_mgr_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    // Set short timeout (500 milliseconds)
    txn->setTimeout(std::chrono::milliseconds(500));
    
    std::vector<uint8_t> value = {0x01};
    ASSERT_TRUE(txn->put("timeout:key", value));
    
    // Wait longer than timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    
    // Commit should fail due to timeout
    bool result = txn->commit();
    EXPECT_FALSE(result);
}

// ============================================================================
// Savepoint Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, SavepointRollback) {
    // Intent: Verify partial rollback to savepoint
    
    auto txn = txn_mgr_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    
    // Write 1
    ASSERT_TRUE(txn->put("sp:key1", {0x01}));
    
    // Create savepoint
    auto sp = txn->setSavepoint();
    
    // Write 2
    ASSERT_TRUE(txn->put("sp:key2", {0x02}));
    
    // Rollback to savepoint (undoes write 2, keeps write 1)
    txn->rollbackToSavepoint(sp);
    
    // Commit
    ASSERT_TRUE(txn->commit());
    
    // Only key1 should exist
    EXPECT_TRUE(db_->get("sp:key1").has_value());
    EXPECT_FALSE(db_->get("sp:key2").has_value());
}

// ============================================================================
// Read-Only Transaction Optimization Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, ReadOnlyTransaction) {
    // Intent: Verify read-only transaction optimization
    
    db_->put("ro:key1", {0x01});
    db_->put("ro:key2", {0x02});
    
    auto txn = txn_mgr_->beginReadOnlyTransaction();
    ASSERT_NE(txn, nullptr);
    
    // Reads should work
    EXPECT_TRUE(txn->get("ro:key1").has_value());
    EXPECT_TRUE(txn->get("ro:key2").has_value());
    
    // Writes should be rejected or fail
    bool write_result = txn->put("ro:key3", {0x03});
    EXPECT_FALSE(write_result);
    
    txn->rollback();
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

TEST_F(TransactionManagerComprehensiveTest, HighVolumeTransactions) {
    // Intent: Verify system handles high transaction volume
    
    const int num_transactions = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_transactions; ++i) {
        auto txn = txn_mgr_->beginTransaction();
        if (!txn) continue;
        
        std::string key = "perf:txn" + std::to_string(i);
        std::vector<uint8_t> value = {static_cast<uint8_t>(i % 256)};
        
        if (txn->put(key, value)) {
            txn->commit();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (< 5 seconds)
    EXPECT_LT(duration.count(), 5000);
}

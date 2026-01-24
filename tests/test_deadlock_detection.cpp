// Test deadlock detection in TransactionManager
// Copyright (c) 2024 ThemisDB. All rights reserved.

#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis;

class DeadlockDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup RocksDB with test database
        RocksDBWrapper::Config config;
        config.db_path = "/tmp/test_deadlock_db";
        config.enable_wal = false;  // Faster for tests
        config.memtable_size_mb = 16;
        config.block_cache_size_mb = 16;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        // Create index managers (simplified for test)
        sec_idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_ = std::make_unique<VectorIndexManager>(*db_);
        
        // Create transaction manager
        txn_mgr_ = std::make_unique<TransactionManager>(*db_, *sec_idx_, *graph_idx_, *vec_idx_);
    }
    
    void TearDown() override {
        txn_mgr_.reset();
        vec_idx_.reset();
        graph_idx_.reset();
        sec_idx_.reset();
        db_->close();
        db_.reset();
        
        // Cleanup test database
        std::filesystem::remove_all("/tmp/test_deadlock_db");
    }
    
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<GraphIndexManager> graph_idx_;
    std::unique_ptr<VectorIndexManager> vec_idx_;
    std::unique_ptr<TransactionManager> txn_mgr_;
};

TEST_F(DeadlockDetectionTest, EnableDisableDeadlockDetection) {
    // Test enabling/disabling deadlock detection
    txn_mgr_->setDeadlockDetection(true);
    
    // Give it a moment to activate
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    txn_mgr_->setDeadlockDetection(false);
    
    // Should complete without error
    SUCCEED();
}

TEST_F(DeadlockDetectionTest, SetDeadlockTimeout) {
    // Test setting deadlock timeout
    txn_mgr_->setDeadlockTimeout(std::chrono::milliseconds(500));
    txn_mgr_->setDeadlockTimeout(std::chrono::milliseconds(2000));
    
    // Should complete without error
    SUCCEED();
}

TEST_F(DeadlockDetectionTest, GetDeadlockStatistics) {
    // Test getting deadlock statistics
    uint64_t count = txn_mgr_->getDeadlockCount();
    EXPECT_EQ(count, 0);
    
    auto deadlocks = txn_mgr_->getDeadlocks();
    EXPECT_TRUE(deadlocks.empty());
}

TEST_F(DeadlockDetectionTest, NoDeadlockWithSequentialTransactions) {
    // Enable deadlock detection
    txn_mgr_->setDeadlockDetection(true);
    txn_mgr_->setDeadlockTimeout(std::chrono::milliseconds(100));
    
    // Create two sequential transactions (no conflict)
    auto txn1_id = txn_mgr_->beginTransaction();
    auto txn1 = txn_mgr_->getTransaction(txn1_id);
    ASSERT_NE(txn1, nullptr);
    
    // Commit first transaction
    txn_mgr_->commitTransaction(txn1_id);
    
    // Start second transaction
    auto txn2_id = txn_mgr_->beginTransaction();
    auto txn2 = txn_mgr_->getTransaction(txn2_id);
    ASSERT_NE(txn2, nullptr);
    
    // Commit second transaction
    txn_mgr_->commitTransaction(txn2_id);
    
    // Wait for deadlock detector to run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should not have detected any deadlocks
    EXPECT_EQ(txn_mgr_->getDeadlockCount(), 0);
}

// Note: Testing actual deadlocks would require simulating concurrent transactions
// that acquire locks in conflicting order. This is complex with RocksDB's internal
// locking and would require more sophisticated test infrastructure.



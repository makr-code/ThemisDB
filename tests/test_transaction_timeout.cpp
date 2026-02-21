/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_transaction_timeout.cpp                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 18:44:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     295                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f70e93ab6  2026-02-21  Add TwoPhaseCommitCoordinator for cross-shard transaction... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for transaction timeout / automatic rollback.
//
// Covers:
//   - Default timeout is disabled (0 ms)
//   - setTransactionTimeout / getTransactionTimeout round-trip
//   - abortTimedOutTransactions() with timeout disabled → 0 aborted
//   - abortTimedOutTransactions() manually sweeps expired transactions
//   - Transaction committed before timeout is not aborted
//   - Auto-abort via background detector loop
//   - getTimedOutCount() tracks count correctly
//   - Stats.total_timed_out reflects count

#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <chrono>
#include <filesystem>
#include <thread>

using namespace themis;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class TransactionTimeoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = fs::temp_directory_path() /
                   ("themis_txn_timeout_" +
                    std::to_string(std::chrono::system_clock::now()
                                       .time_since_epoch()
                                       .count()));

        RocksDBWrapper::Config cfg;
        cfg.db_path            = db_path_.string();
        cfg.enable_wal         = false;
        cfg.memtable_size_mb   = 16;
        cfg.block_cache_size_mb = 16;

        db_      = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        sec_idx_   = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_   = std::make_unique<VectorIndexManager>(*db_);

        txn_mgr_ = std::make_unique<TransactionManager>(
            *db_, *sec_idx_, *graph_idx_, *vec_idx_);
    }

    void TearDown() override {
        txn_mgr_.reset();
        vec_idx_.reset();
        graph_idx_.reset();
        sec_idx_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(db_path_);
    }

    fs::path db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<GraphIndexManager>     graph_idx_;
    std::unique_ptr<VectorIndexManager>    vec_idx_;
    std::unique_ptr<TransactionManager>    txn_mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Default timeout is disabled
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, DefaultTimeoutIsDisabled) {
    EXPECT_EQ(txn_mgr_->getTransactionTimeout().count(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// setTransactionTimeout / getTransactionTimeout round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, SetAndGetTimeout) {
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(500));
    EXPECT_EQ(txn_mgr_->getTransactionTimeout().count(), 500);

    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(0));
    EXPECT_EQ(txn_mgr_->getTransactionTimeout().count(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// abortTimedOutTransactions with timeout disabled → 0 aborted
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, NoAbortWhenTimeoutDisabled) {
    auto id = txn_mgr_->beginTransaction();
    EXPECT_NE(id, 0u);

    // Timeout disabled (0) → manual sweep aborts nothing
    size_t aborted = txn_mgr_->abortTimedOutTransactions();
    EXPECT_EQ(aborted, 0u);
    EXPECT_EQ(txn_mgr_->getTimedOutCount(), 0u);

    txn_mgr_->rollbackTransaction(id);
}

// ─────────────────────────────────────────────────────────────────────────────
// abortTimedOutTransactions manually sweeps expired transactions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, ManualSweepAbortsExpiredTransaction) {
    // Set a very short timeout
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(50));

    auto id = txn_mgr_->beginTransaction();
    ASSERT_NE(id, 0u);

    // Transaction has not expired yet
    EXPECT_EQ(txn_mgr_->abortTimedOutTransactions(), 0u);

    // Wait for expiry
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    size_t aborted = txn_mgr_->abortTimedOutTransactions();
    EXPECT_GE(aborted, 1u);
    EXPECT_GE(txn_mgr_->getTimedOutCount(), 1u);

    // Transaction should now be finished
    auto txn = txn_mgr_->getTransaction(id);
    if (txn) {
        EXPECT_TRUE(txn->isFinished());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Transaction committed before timeout is not aborted
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, CommittedTransactionNotAborted) {
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(200));

    auto id = txn_mgr_->beginTransaction();
    ASSERT_NE(id, 0u);

    // Commit well before the timeout
    auto status = txn_mgr_->commitTransaction(id);
    EXPECT_TRUE(status.ok);

    // Wait past the timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    size_t aborted = txn_mgr_->abortTimedOutTransactions();
    // The committed transaction must NOT be counted
    EXPECT_EQ(aborted, 0u);
    EXPECT_EQ(txn_mgr_->getTimedOutCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Rolled-back transaction before timeout is not double-aborted
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, RolledBackTransactionNotDoubleAborted) {
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(50));

    auto id = txn_mgr_->beginTransaction();
    ASSERT_NE(id, 0u);

    // Explicit rollback before timeout
    txn_mgr_->rollbackTransaction(id);

    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    // Sweep should not count already-finished transaction
    size_t aborted = txn_mgr_->abortTimedOutTransactions();
    EXPECT_EQ(aborted, 0u);
    EXPECT_EQ(txn_mgr_->getTimedOutCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// getTimedOutCount tracks multiple expirations
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, TimedOutCountAccumulates) {
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(50));

    auto id1 = txn_mgr_->beginTransaction();
    auto id2 = txn_mgr_->beginTransaction();
    ASSERT_NE(id1, 0u);
    ASSERT_NE(id2, 0u);

    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    size_t aborted = txn_mgr_->abortTimedOutTransactions();
    EXPECT_EQ(aborted, 2u);
    EXPECT_EQ(txn_mgr_->getTimedOutCount(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats.total_timed_out is populated
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, StatsTotalTimedOut) {
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(50));

    auto id = txn_mgr_->beginTransaction();
    ASSERT_NE(id, 0u);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    txn_mgr_->abortTimedOutTransactions();

    auto stats = txn_mgr_->getStats();
    EXPECT_GE(stats.total_timed_out, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Auto-abort via background detector loop
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, BackgroundLoopAbortsExpiredTransaction) {
    // The deadlock detector loop wakes every deadlock_timeout_ms (default 1000 ms).
    // Shorten it by setting deadlock timeout to a small value via setDeadlockTimeout
    // so the background thread fires quickly.
    txn_mgr_->setDeadlockTimeout(std::chrono::milliseconds(100));
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(50));

    auto id = txn_mgr_->beginTransaction();
    ASSERT_NE(id, 0u);

    // Wait enough time for:
    //   - The transaction to expire (50 ms)
    //   - The background loop to fire at least once (100 ms interval)
    //   - Some margin
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    EXPECT_GE(txn_mgr_->getTimedOutCount(), 1u);

    auto txn = txn_mgr_->getTransaction(id);
    if (txn) {
        EXPECT_TRUE(txn->isFinished());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Disabling timeout after expiry does not abort already-started transaction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, DisablingTimeoutStopsFutureSweeps) {
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(50));

    auto id1 = txn_mgr_->beginTransaction();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    // Disable timeout before sweeping
    txn_mgr_->setTransactionTimeout(std::chrono::milliseconds(0));

    size_t aborted = txn_mgr_->abortTimedOutTransactions();
    EXPECT_EQ(aborted, 0u);

    // The transaction is still active (no timeout sweep happened)
    auto txn = txn_mgr_->getTransaction(id1);
    if (txn) {
        EXPECT_FALSE(txn->isFinished());
    }
    txn_mgr_->rollbackTransaction(id1);
}

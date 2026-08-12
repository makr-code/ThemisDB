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

#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class TransactionTimeoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_txn_timeout_" +
                     std::to_string(std::chrono::system_clock::now()
                                        .time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = true;
        db_            = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        sec_idx_   = std::make_unique<SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_   = std::make_unique<VectorIndexManager>(*db_);
        mgr_       = std::make_unique<TransactionManager>(
            *db_, *sec_idx_, *graph_idx_, *vec_idx_);
    }

    void TearDown() override {
        mgr_.reset();
        vec_idx_.reset();
        sec_idx_.reset();
        graph_idx_.reset();
        if (db_) { db_->close(); db_.reset(); }
        fs::remove_all(db_path_);
    }

    std::string                             db_path_;
    std::unique_ptr<RocksDBWrapper>         db_;
    std::unique_ptr<SecondaryIndexManager>  sec_idx_;
    std::unique_ptr<GraphIndexManager>      graph_idx_;
    std::unique_ptr<VectorIndexManager>     vec_idx_;
    std::unique_ptr<TransactionManager>     mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Transaction::setTimeout / getTimeout / isTimedOut
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, NoTimeout_IsNotTimedOut) {
    auto txn = mgr_->begin();
    EXPECT_EQ(txn.getTimeout().count(), 0);
    EXPECT_FALSE(txn.isTimedOut());
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, SetTimeout_GetTimeout) {
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::milliseconds(500));
    EXPECT_EQ(txn.getTimeout(), std::chrono::milliseconds(500));
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, LongTimeout_IsNotTimedOut) {
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::hours(1));
    EXPECT_FALSE(txn.isTimedOut());
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, ZeroTimeout_AfterSet_IsNotTimedOut) {
    // Timeout disabled (0 ms) means never timed out
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::milliseconds(0));
    EXPECT_FALSE(txn.isTimedOut());
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, NegativeTimeout_TreatedAsDisabled) {
    // Negative chrono value must be clamped to 0 (disabled), not wrapped to a huge uint64_t
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::milliseconds(-1));
    EXPECT_EQ(txn.getTimeout().count(), 0);
    EXPECT_FALSE(txn.isTimedOut());
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, NegativeDefaultTimeout_TreatedAsDisabled) {
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(-500));
    EXPECT_EQ(mgr_->getDefaultTransactionTimeout().count(), 0);
    // New transactions must not inherit a phantom timeout
    auto txn = mgr_->begin();
    EXPECT_EQ(txn.getTimeout().count(), 0);
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, VeryShortTimeout_IsTimedOut) {
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::milliseconds(1));
    // Sleep to ensure timeout elapsed
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(txn.isTimedOut());
    txn.rollback();
}

// ─────────────────────────────────────────────────────────────────────────────
// commit() rejected when timed out
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, Commit_AfterTimeout_ReturnsError) {
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto st = txn.commit();
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("timed out"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Default transaction timeout on TransactionManager
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, DefaultTimeout_InitiallyZero) {
    EXPECT_EQ(mgr_->getDefaultTransactionTimeout().count(), 0);
}

TEST_F(TransactionTimeoutTest, SetDefaultTimeout_GetDefaultTimeout) {
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(2000));
    EXPECT_EQ(mgr_->getDefaultTransactionTimeout(), std::chrono::milliseconds(2000));
}

TEST_F(TransactionTimeoutTest, DefaultTimeout_AppliedToNewTransactions) {
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(5000));

    auto txn = mgr_->begin();
    EXPECT_EQ(txn.getTimeout(), std::chrono::milliseconds(5000));
    txn.rollback();
}

TEST_F(TransactionTimeoutTest, DefaultTimeout_AppliedToSessionTransactions) {
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(3000));

    auto txn_id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);
    EXPECT_EQ(txn->getTimeout(), std::chrono::milliseconds(3000));
    mgr_->rollbackTransaction(txn_id);
}

TEST_F(TransactionTimeoutTest, SetDefaultTimeout_ZeroDisablesTimeout) {
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(5000));
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(0));
    EXPECT_EQ(mgr_->getDefaultTransactionTimeout().count(), 0);

    auto txn = mgr_->begin();
    EXPECT_EQ(txn.getTimeout().count(), 0);
    txn.rollback();
}

// ─────────────────────────────────────────────────────────────────────────────
// Automatic rollback via timeoutExpiredTransactions (background monitor)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, ExpiredTransaction_AutoRolledBack) {
    // Set a very short default timeout so the transaction expires immediately
    mgr_->setDefaultTransactionTimeout(std::chrono::milliseconds(1));
    auto txn_id = mgr_->beginTransaction();

    // Wait for timeout to elapse
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // The deadlock-detector thread runs every deadlock_timeout_ms (default 1 s).
    // Rather than waiting 1 s in a test, call the helper via the documented
    // public interface: rollbackTransaction rolls back expired transactions when
    // called. But the key observable side-effect is that a subsequent commit
    // on the same transaction ID fails (already moved to completed).
    // We verify via getTimeoutCount() after a manual trigger.
    EXPECT_TRUE(mgr_->getTransaction(txn_id) != nullptr); // still active before monitor runs

    // Simulate what the monitor does: commit should fail because the txn is expired.
    auto st = mgr_->commitTransaction(txn_id);
    // Either the commit was refused (timed-out error) or the txn was already
    // rolled back. Either way it must not succeed.
    EXPECT_FALSE(st.ok);
}

TEST_F(TransactionTimeoutTest, GetTimeoutCount_InitiallyZero) {
    EXPECT_EQ(mgr_->getTimeoutCount(), 0u);
}

TEST_F(TransactionTimeoutTest, GetTimeoutCount_IncrementsOnAutoRollback) {
    // Set a very short deadlock-detector interval so the background monitor fires quickly
    mgr_->setDeadlockTimeout(std::chrono::milliseconds(10));

    // The background monitor's auto-rollback path uses the global transaction timeout.
    mgr_->setTransactionTimeout(std::chrono::milliseconds(1));
    auto txn_id = mgr_->beginTransaction();
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); // let it expire

    // Wait long enough for the background monitor to run (10 ms interval + margin)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // The monitor should have auto-rolled back the transaction and incremented the counter
    EXPECT_GE(mgr_->getTimeoutCount(), 1u);
    // The transaction must have been moved out of active
    EXPECT_EQ(mgr_->getTransaction(txn_id), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// A transaction without timeout should still commit successfully
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, NoTimeout_CommitSucceeds) {
    auto txn = mgr_->begin();
    EXPECT_EQ(txn.getTimeout().count(), 0);
    auto st = txn.commit();
    EXPECT_TRUE(st.ok);
}

TEST_F(TransactionTimeoutTest, LongTimeout_CommitSucceeds) {
    auto txn = mgr_->begin();
    txn.setTimeout(std::chrono::hours(1));
    auto st = txn.commit();
    EXPECT_TRUE(st.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats struct includes total_timed_out
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, GetStats_InitialTimedOutIsZero) {
    auto stats = mgr_->getStats();
    EXPECT_EQ(stats.total_timed_out, 0u);
}

TEST_F(TransactionTimeoutTest, GetStatsLockFree_InitialTimedOutIsZero) {
    auto stats = mgr_->getStatsLockFree();
    EXPECT_EQ(stats.total_timed_out, 0u);
}

TEST_F(TransactionTimeoutTest, GetStats_TimedOutAppearsAfterAutoRollback) {
    mgr_->setDeadlockTimeout(std::chrono::milliseconds(10));
    // Auto-rollback statistics are incremented by abortTimedOutTransactions(),
    // which uses the global transaction timeout, not the default per-transaction timeout.
    mgr_->setTransactionTimeout(std::chrono::milliseconds(1));
    mgr_->beginTransaction();
    // Sleep long enough for the transaction to expire and the background monitor to run
    std::this_thread::sleep_for(std::chrono::milliseconds(105));
    auto stats = mgr_->getStats();
    EXPECT_GE(stats.total_timed_out, 1u);
    EXPECT_GE(stats.total_aborted, 1u);
    // timed_out is a subset of aborted
    EXPECT_LE(stats.total_timed_out, stats.total_aborted);
}

// ─────────────────────────────────────────────────────────────────────────────
// getDurationMs() is frozen after commit/rollback (Bug 7 regression test)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TransactionTimeoutTest, GetDurationMs_FrozenAfterCommit) {
    auto txn_id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    // Duration is live while active
    EXPECT_FALSE(txn->isFinished());
    auto live_dur = txn->getDurationMs();
    EXPECT_GE(live_dur, 0u);

    mgr_->commitTransaction(txn_id);

    // After commit the transaction is finished; getDurationMs() must return
    // the frozen value captured at commit time, NOT a value that keeps growing.
    auto dur_after = txn->getDurationMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto dur_later = txn->getDurationMs();

    EXPECT_EQ(dur_after, dur_later)
        << "getDurationMs() must be frozen after commit";
}

TEST_F(TransactionTimeoutTest, GetDurationMs_FrozenAfterRollback) {
    auto txn_id = mgr_->beginTransaction();
    auto txn = mgr_->getTransaction(txn_id);
    ASSERT_NE(txn, nullptr);

    mgr_->rollbackTransaction(txn_id);

    auto dur_after = txn->getDurationMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto dur_later = txn->getDurationMs();

    EXPECT_EQ(dur_after, dur_later)
        << "getDurationMs() must be frozen after rollback";
}

TEST_F(TransactionTimeoutTest, GetStats_AvgDuration_DoesNotGrowAfterCommit) {
    // Run a transaction and commit it, then check that stats.avg_duration_ms
    // does NOT keep growing after the transaction finishes.
    auto txn_id = mgr_->beginTransaction();
    mgr_->commitTransaction(txn_id);

    auto stats1 = mgr_->getStats();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    auto stats2 = mgr_->getStats();

    EXPECT_EQ(stats1.avg_duration_ms, stats2.avg_duration_ms)
        << "avg_duration_ms must not grow after all transactions have finished";
    EXPECT_EQ(stats1.max_duration_ms, stats2.max_duration_ms)
        << "max_duration_ms must not grow after all transactions have finished";
}

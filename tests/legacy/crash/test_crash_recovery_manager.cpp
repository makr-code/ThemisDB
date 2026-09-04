// Phase 8 – Crash Recovery Manager tests
// Tests the transaction-level WAL, undo/redo log, and crash recovery procedure.

#include <gtest/gtest.h>
#include "transaction/crash_recovery_manager.h"
#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::transaction;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture: CrashRecoveryManager in isolation (no full TransactionManager)
// ─────────────────────────────────────────────────────────────────────────────

class CrashRecoveryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CrashRecoveryManagerTest on Windows due to timeout instability in WAL recovery fixture.";
#endif
        test_dir_ = (std::filesystem::temp_directory_path() /
                     ("themis_crm_test_" +
                      std::to_string(std::chrono::steady_clock::now()
                                         .time_since_epoch().count())))
                        .string();
        std::filesystem::create_directories(test_dir_);
        wal_path_ = test_dir_ + "/recovery.wal";

        db_path_ = test_dir_ + "/db";
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        crm_ = std::make_unique<CrashRecoveryManager>(wal_path_,
                                                       /*sync_on_write=*/false);
    }

    void TearDown() override {
        crm_.reset();
        db_.reset();
        std::filesystem::remove_all(test_dir_);
    }

    // Helper: simulate a "crash" by destroying and recreating the CRM
    // (the WAL file persists on disk)
    void simulateCrash() {
        crm_.reset();
        crm_ = std::make_unique<CrashRecoveryManager>(wal_path_,
                                                       /*sync_on_write=*/false);
    }

    std::string test_dir_;
    std::string wal_path_;
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<CrashRecoveryManager> crm_;
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. WAL creation and initial state
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, NoRecoveryNeededWhenWALAbsent) {
    // Fresh CRM without any prior WAL file
    EXPECT_FALSE(crm_->needsRecovery());
}

TEST_F(CrashRecoveryManagerTest, NoPendingTransactionsInitially) {
    EXPECT_EQ(crm_->pendingTransactionCount(), 0u);
}

TEST_F(CrashRecoveryManagerTest, MetricsAreZeroInitially) {
    auto m = crm_->getMetrics();
    EXPECT_EQ(m.total_begins_logged,     0u);
    EXPECT_EQ(m.total_operations_logged, 0u);
    EXPECT_EQ(m.total_commits_logged,    0u);
    EXPECT_EQ(m.total_aborts_logged,     0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Logging lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, LogBeginCreatesPendingEntry) {
    crm_->logBegin(42, IsolationLevel::READ_COMMITTED);
    EXPECT_EQ(crm_->pendingTransactionCount(), 1u);
    EXPECT_EQ(crm_->getMetrics().total_begins_logged, 1u);
}

TEST_F(CrashRecoveryManagerTest, LogCommitRemovesPendingEntry) {
    crm_->logBegin(1, IsolationLevel::READ_COMMITTED);
    crm_->logCommit(1);
    EXPECT_EQ(crm_->pendingTransactionCount(), 0u);
    EXPECT_EQ(crm_->getMetrics().total_commits_logged, 1u);
}

TEST_F(CrashRecoveryManagerTest, LogAbortRemovesPendingEntry) {
    crm_->logBegin(2, IsolationLevel::REPEATABLE_READ);
    crm_->logAbort(2);
    EXPECT_EQ(crm_->pendingTransactionCount(), 0u);
    EXPECT_EQ(crm_->getMetrics().total_aborts_logged, 1u);
}

TEST_F(CrashRecoveryManagerTest, LogOperationCountedInMetrics) {
    crm_->logBegin(5, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(5, "put", "key:1", std::nullopt, std::string("v1"));
    crm_->logOperation(5, "put", "key:2", std::string("old"),
                       std::string("new"));
    EXPECT_EQ(crm_->getMetrics().total_operations_logged, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. WAL persistence and reading
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, WALEntriesArePersisted) {
    crm_->logBegin(10, IsolationLevel::SERIALIZABLE);
    crm_->logOperation(10, "put", "row:A", std::nullopt, std::string("val_A"));
    crm_->logCommit(10);

    auto entries = crm_->readAllEntries();
    EXPECT_EQ(entries.size(), 3u);

    EXPECT_EQ(entries[0].type,   CrashRecoveryManager::EntryType::BEGIN);
    EXPECT_EQ(entries[0].txn_id, 10u);
    EXPECT_EQ(entries[1].type,   CrashRecoveryManager::EntryType::OPERATION);
    EXPECT_EQ(entries[1].operation.key, "row:A");
    EXPECT_EQ(entries[2].type,   CrashRecoveryManager::EntryType::COMMIT);
}

TEST_F(CrashRecoveryManagerTest, WALSurvivesSimulatedCrash) {
    crm_->logBegin(99, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(99, "put", "crash_key", std::nullopt,
                       std::string("crash_val"));
    // No commit → simulates crash

    simulateCrash();  // WAL file stays on disk

    auto entries = crm_->readAllEntries();
    EXPECT_GE(entries.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. needsRecovery detection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, NeedsRecoveryAfterBeginWithoutCommit) {
    crm_->logBegin(7, IsolationLevel::READ_COMMITTED);
    // No commit

    simulateCrash();
    EXPECT_TRUE(crm_->needsRecovery());
}

TEST_F(CrashRecoveryManagerTest, NoRecoveryNeededAfterCommit) {
    crm_->logBegin(8, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(8, "put", "k", std::nullopt, std::string("v"));
    crm_->logCommit(8);

    simulateCrash();
    EXPECT_FALSE(crm_->needsRecovery());
}

TEST_F(CrashRecoveryManagerTest, NoRecoveryNeededAfterAbort) {
    crm_->logBegin(9, IsolationLevel::READ_COMMITTED);
    crm_->logAbort(9);

    simulateCrash();
    EXPECT_FALSE(crm_->needsRecovery());
}

TEST_F(CrashRecoveryManagerTest, GetInFlightIdsPopulatedAfterNeedsRecovery) {
    crm_->logBegin(20, IsolationLevel::READ_COMMITTED);
    crm_->logBegin(21, IsolationLevel::READ_COMMITTED);
    crm_->logCommit(21); // 21 committed; 20 is in-flight

    simulateCrash();
    crm_->needsRecovery();  // triggers scan

    auto ids = crm_->getInFlightTransactionIds();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], 20u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. Recovery (undo operations)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, RecoverUndoesPutWhenKeyWasNew) {
    // Write a key directly to DB (simulates what the txn would have done)
    std::vector<uint8_t> val{'v','a','l'};
    db_->put("undo_key", val);
    ASSERT_TRUE(db_->get("undo_key").has_value());

    // Log the BEGIN + PUT (key had no prior value)
    crm_->logBegin(30, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(30, "put", "undo_key",
                        /*old_value=*/std::nullopt,
                        /*new_value=*/std::string("val"));
    // No commit → in-flight

    simulateCrash();

    auto result = crm_->recover(*db_);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.rolled_back, 1u);
    EXPECT_EQ(result.operations_undone, 1u);

    // The key should have been deleted (undo of "put without old value")
    EXPECT_FALSE(db_->get("undo_key").has_value());
}

TEST_F(CrashRecoveryManagerTest, RecoverRestoresOldValueOnPut) {
    // Old value = "old_data"
    std::string old_data = "old_data";
    std::string new_data = "new_data";

    // Write new_data as the "current" DB state (what the crashed txn wrote)
    std::vector<uint8_t> new_bytes(new_data.begin(), new_data.end());
    db_->put("restore_key", new_bytes);

    crm_->logBegin(31, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(31, "put", "restore_key",
                        old_data,   // had old value
                        new_data);
    // No commit

    simulateCrash();
    auto result = crm_->recover(*db_);
    EXPECT_TRUE(result.success);

    // DB should now have "old_data" back
    auto recovered = db_->get("restore_key");
    ASSERT_TRUE(recovered.has_value());
    std::string got(recovered->begin(), recovered->end());
    EXPECT_EQ(got, old_data);
}

TEST_F(CrashRecoveryManagerTest, RecoverCleanWALSucceeds) {
    // All transactions committed → recovery should be a no-op
    crm_->logBegin(50, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(50, "put", "k50", std::nullopt, std::string("v50"));
    crm_->logCommit(50);

    simulateCrash();
    auto result = crm_->recover(*db_);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.in_flight_found, 0u);
    EXPECT_FALSE(result.hadWorkToDo());
}

TEST_F(CrashRecoveryManagerTest, RecoverMultipleInFlightTransactions) {
    // Two in-flight txns, each writing a different key
    std::vector<uint8_t> bytes1{'a'}, bytes2{'b'};
    db_->put("multi_k1", bytes1);
    db_->put("multi_k2", bytes2);

    crm_->logBegin(60, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(60, "put", "multi_k1", std::nullopt, std::string("a"));

    crm_->logBegin(61, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(61, "put", "multi_k2", std::nullopt, std::string("b"));

    // Neither committed
    simulateCrash();
    auto result = crm_->recover(*db_);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.in_flight_found, 2u);
    EXPECT_EQ(result.rolled_back, 2u);
    EXPECT_EQ(result.operations_undone, 2u);

    EXPECT_FALSE(db_->get("multi_k1").has_value());
    EXPECT_FALSE(db_->get("multi_k2").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Checkpoint prevents re-recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, AfterRecoveryNoFurtherRecoveryNeeded) {
    crm_->logBegin(70, IsolationLevel::READ_COMMITTED);
    // No commit
    simulateCrash();

    ASSERT_TRUE(crm_->needsRecovery());
    crm_->recover(*db_);

    // After recovery a CHECKPOINT is written; now needsRecovery should be false
    simulateCrash();
    EXPECT_FALSE(crm_->needsRecovery());
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Prune log
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CrashRecoveryManagerTest, PruneLogRemovesCommittedEntries) {
    crm_->logBegin(80, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(80, "put", "pk", std::nullopt, std::string("pv"));
    crm_->logCommit(80);

    size_t removed = crm_->pruneLog();
    EXPECT_GT(removed, 0u);

    // WAL should now be smaller (or empty)
    auto entries = crm_->readAllEntries();
    for (const auto& e : entries) {
        EXPECT_NE(e.txn_id, 80u) << "Committed txn 80 should have been pruned";
    }
}

TEST_F(CrashRecoveryManagerTest, PruneLogRetainsInFlightEntries) {
    crm_->logBegin(90, IsolationLevel::READ_COMMITTED);
    crm_->logOperation(90, "put", "inf_key", std::nullopt, std::string("v"));
    // No commit – in-flight

    crm_->pruneLog();

    // In-flight entries must still be present
    auto entries = crm_->readAllEntries();
    bool found_begin = false;
    for (const auto& e : entries) {
        if (e.txn_id == 90u && e.type == CrashRecoveryManager::EntryType::BEGIN)
            found_begin = true;
    }
    EXPECT_TRUE(found_begin) << "In-flight txn 90 BEGIN should survive pruneLog";
}

TEST_F(CrashRecoveryManagerTest, PruneMetricIncremented) {
    crm_->logBegin(91, IsolationLevel::READ_COMMITTED);
    crm_->logCommit(91);
    crm_->pruneLog();
    EXPECT_EQ(crm_->getMetrics().wal_prune_count, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. TransactionManager integration
// ─────────────────────────────────────────────────────────────────────────────

class CrashRecoveryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = (std::filesystem::temp_directory_path() /
                     ("themis_cri_test_" +
                      std::to_string(std::chrono::steady_clock::now()
                                         .time_since_epoch().count())))
                        .string();
        std::filesystem::create_directories(test_dir_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_dir_ + "/db";
        cfg.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        sec_   = std::make_unique<SecondaryIndexManager>(*db_);
        graph_ = std::make_unique<GraphIndexManager>(*db_);
        vec_   = std::make_unique<VectorIndexManager>(*db_);
        mgr_   = std::make_unique<TransactionManager>(*db_, *sec_, *graph_, *vec_);
    }

    void TearDown() override {
        mgr_.reset();
        vec_.reset();
        graph_.reset();
        sec_.reset();
        db_->close();
        db_.reset();
        std::filesystem::remove_all(test_dir_);
    }

    std::string test_dir_ = {};
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sec_;
    std::unique_ptr<GraphIndexManager>     graph_;
    std::unique_ptr<VectorIndexManager>    vec_;
    std::unique_ptr<TransactionManager>    mgr_;
};

TEST_F(CrashRecoveryIntegrationTest, EnableCrashRecoveryCreatesCRM) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);
    EXPECT_NE(mgr_->getCrashRecoveryManager(), nullptr);
}

TEST_F(CrashRecoveryIntegrationTest, NeedsRecoveryFalseWhenDisabled) {
    // CRM not enabled
    EXPECT_FALSE(mgr_->needsCrashRecovery());
}

TEST_F(CrashRecoveryIntegrationTest, NeedsRecoveryFalseWhenWALIsClean) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);

    auto id = mgr_->beginTransaction();
    mgr_->commitTransaction(id);

    EXPECT_FALSE(mgr_->needsCrashRecovery());
}

TEST_F(CrashRecoveryIntegrationTest, BeginLogsToWAL) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);

    auto id = mgr_->beginTransaction(IsolationLevel::READ_COMMITTED);
    mgr_->rollbackTransaction(id);

    auto* crm = mgr_->getCrashRecoveryManager();
    ASSERT_NE(crm, nullptr);
    EXPECT_EQ(crm->getMetrics().total_begins_logged, 1u);
}

TEST_F(CrashRecoveryIntegrationTest, CommitLogsToWAL) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);

    auto id = mgr_->beginTransaction();
    mgr_->commitTransaction(id);

    auto* crm = mgr_->getCrashRecoveryManager();
    ASSERT_NE(crm, nullptr);
    EXPECT_EQ(crm->getMetrics().total_commits_logged, 1u);
}

TEST_F(CrashRecoveryIntegrationTest, RollbackLogsAbortToWAL) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);

    auto id = mgr_->beginTransaction();
    mgr_->rollbackTransaction(id);

    auto* crm = mgr_->getCrashRecoveryManager();
    ASSERT_NE(crm, nullptr);
    EXPECT_EQ(crm->getMetrics().total_aborts_logged, 1u);
}

TEST_F(CrashRecoveryIntegrationTest, CrashRecoverReturnsSuccessWhenWALClean) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);

    auto id = mgr_->beginTransaction();
    mgr_->commitTransaction(id);

    auto result = mgr_->crashRecover();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.in_flight_found, 0u);
}

TEST_F(CrashRecoveryIntegrationTest, CrashRecoverWhenDisabledReturnsSuccess) {
    // No enableCrashRecovery call
    auto result = mgr_->crashRecover();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.in_flight_found, 0u);
}

TEST_F(CrashRecoveryIntegrationTest, MultipleTransactionsAllLogged) {
    mgr_->enableCrashRecovery(test_dir_ + "/txn.wal", false);

    for (int i = 0; i < 5; ++i) {
        auto id = mgr_->beginTransaction();
        mgr_->commitTransaction(id);
    }

    auto* crm = mgr_->getCrashRecoveryManager();
    ASSERT_NE(crm, nullptr);
    EXPECT_EQ(crm->getMetrics().total_begins_logged,  5u);
    EXPECT_EQ(crm->getMetrics().total_commits_logged, 5u);
}

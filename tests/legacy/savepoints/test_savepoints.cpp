// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for savepoint support in RocksDBWrapper::TransactionWrapper:
//   - setSavePoint / rollbackToSavePoint / popSavePoint round-trip
//   - Nested savepoints (stack behaviour)
//   - rollback without savepoint returns error
//   - Writes after rollbackToSavePoint are visible / rolled-back correctly
//
// Also tests the named savepoint API on TransactionManager::Transaction:
//   - createSavepoint / rollbackToSavepoint / releaseSavepoint
//   - getSavepoints / hasSavepoint
//
// Low-level tests operate directly against RocksDB via TransactionDB so they
// do NOT require the full ThemisDB stack.  Named-savepoint tests use the
// TransactionManager::begin() direct-transaction API.

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "transaction/saga.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class SavepointTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_savepoint_test_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = true;
        db_            = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
    }

    void TearDown() override {
        if (db_) { db_->close(); db_.reset(); }
        fs::remove_all(db_path_);
    }

    // Helper: read a string value from the DB (outside any transaction).
    std::optional<std::string> readRaw(std::string_view key) {
        std::string out;
        if (db_->get(std::string(key), out)) return out;
        return std::nullopt;
    }

    std::string                    db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// ─────────────────────────────────────────────────────────────────────────────
// setSavePoint / rollbackToSavePoint basic
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SavepointTest, SetAndRollback_UndonesWritesSinceSavepoint) {
    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn && txn->isActive());

    // Write before savepoint
    ASSERT_TRUE(txn->put("before", std::vector<uint8_t>{'A'}));

    // Set a savepoint
    txn->setSavePoint();

    // Write after savepoint
    ASSERT_TRUE(txn->put("after", std::vector<uint8_t>{'B'}));

    // Rollback to savepoint: "after" should disappear, "before" should remain
    EXPECT_TRUE(txn->rollbackToSavePoint());

    // Commit
    ASSERT_TRUE(txn->commit());

    // Verify: "before" must be present, "after" must be absent
    EXPECT_TRUE(readRaw("before").has_value()) << "'before' should be in DB";
    EXPECT_FALSE(readRaw("after").has_value()) << "'after' was rolled back, must not be in DB";
}

TEST_F(SavepointTest, SetAndPop_KeepsWritesSinceSavepoint) {
    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn && txn->isActive());

    txn->put("before", std::vector<uint8_t>{'A'});
    txn->setSavePoint();
    txn->put("after", std::vector<uint8_t>{'B'});

    // popSavePoint discards the savepoint but keeps the writes
    EXPECT_TRUE(txn->popSavePoint());

    ASSERT_TRUE(txn->commit());

    EXPECT_TRUE(readRaw("before").has_value());
    EXPECT_TRUE(readRaw("after").has_value()) << "'after' must survive popSavePoint";
}

// ─────────────────────────────────────────────────────────────────────────────
// Nested savepoints (stack: LIFO)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SavepointTest, NestedSavepoints_RollbackMostRecent) {
    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn->isActive());

    txn->put("sp0_write", std::vector<uint8_t>{'0'});
    txn->setSavePoint();  // SP1

    txn->put("sp1_write", std::vector<uint8_t>{'1'});
    txn->setSavePoint();  // SP2

    txn->put("sp2_write", std::vector<uint8_t>{'2'});

    // Rollback to SP2 – only sp2_write is undone
    EXPECT_TRUE(txn->rollbackToSavePoint());

    // Rollback to SP1 – sp1_write is also undone
    EXPECT_TRUE(txn->rollbackToSavePoint());

    ASSERT_TRUE(txn->commit());

    EXPECT_TRUE(readRaw("sp0_write").has_value());
    EXPECT_FALSE(readRaw("sp1_write").has_value());
    EXPECT_FALSE(readRaw("sp2_write").has_value());
}

TEST_F(SavepointTest, NestedSavepoints_PopThenRollback) {
    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn->isActive());

    txn->setSavePoint();  // SP1
    txn->put("sp1_write", std::vector<uint8_t>{'1'});

    txn->setSavePoint();  // SP2
    txn->put("sp2_write", std::vector<uint8_t>{'2'});

    // Pop SP2 without rolling back – sp2_write stays
    EXPECT_TRUE(txn->popSavePoint());

    // Now rollback to SP1 – both sp1_write and sp2_write are undone
    // (SP2 was popped so the current top is SP1)
    EXPECT_TRUE(txn->rollbackToSavePoint());

    ASSERT_TRUE(txn->commit());

    EXPECT_FALSE(readRaw("sp1_write").has_value());
    EXPECT_FALSE(readRaw("sp2_write").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Error handling: rollback / pop without savepoint
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SavepointTest, RollbackWithoutSavepoint_ReturnsFalse) {
    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn->isActive());
    // No savepoint set
    EXPECT_FALSE(txn->rollbackToSavePoint());
    txn->rollback();
}

TEST_F(SavepointTest, PopWithoutSavepoint_ReturnsFalse) {
    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn->isActive());
    EXPECT_FALSE(txn->popSavePoint());
    txn->rollback();
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple deletes between savepoints
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SavepointTest, DeleteBetweenSavepoints_RolledBack) {
    // Seed DB with a key outside any transaction
    ASSERT_TRUE(db_->put("existing", std::string_view("original")));

    auto txn = db_->beginTransaction();
    ASSERT_TRUE(txn->isActive());

    txn->setSavePoint();
    ASSERT_TRUE(txn->del("existing"));

    // Roll back deletion
    EXPECT_TRUE(txn->rollbackToSavePoint());

    ASSERT_TRUE(txn->commit());

    // Key must still be present (deletion was rolled back)
    EXPECT_TRUE(readRaw("existing").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Named savepoint fixture (uses TransactionManager::Transaction)
// ─────────────────────────────────────────────────────────────────────────────

class NamedSavepointTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_named_sp_test_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
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
// createSavepoint / hasSavepoint / getSavepoints
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(NamedSavepointTest, CreateAndQuery) {
    auto txn = mgr_->begin();

    EXPECT_FALSE(txn.hasSavepoint("sp1"));
    EXPECT_TRUE(txn.getSavepoints().empty());

    EXPECT_TRUE(txn.createSavepoint("sp1").ok);

    EXPECT_TRUE(txn.hasSavepoint("sp1"));
    EXPECT_FALSE(txn.hasSavepoint("sp2"));

    auto sps = txn.getSavepoints();
    ASSERT_EQ(sps.size(), 1u);
    EXPECT_EQ(sps[0], "sp1");

    EXPECT_TRUE(txn.createSavepoint("sp2").ok);
    sps = txn.getSavepoints();
    ASSERT_EQ(sps.size(), 2u);
    EXPECT_EQ(sps[0], "sp1");
    EXPECT_EQ(sps[1], "sp2");

    txn.rollback();
}

TEST_F(NamedSavepointTest, CreateDuplicateName_ReturnsError) {
    auto txn = mgr_->begin();
    EXPECT_TRUE(txn.createSavepoint("sp").ok);
    auto st = txn.createSavepoint("sp");
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("sp"), std::string::npos);
    txn.rollback();
}

TEST_F(NamedSavepointTest, CreateEmptyName_ReturnsError) {
    auto txn = mgr_->begin();
    auto st = txn.createSavepoint("");
    EXPECT_FALSE(st.ok);
    txn.rollback();
}

// ─────────────────────────────────────────────────────────────────────────────
// rollbackToSavepoint
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(NamedSavepointTest, RollbackToSavepoint_UnknownName_ReturnsError) {
    auto txn = mgr_->begin();
    auto st = txn.rollbackToSavepoint("nonexistent");
    EXPECT_FALSE(st.ok);
    txn.rollback();
}

TEST_F(NamedSavepointTest, RollbackToSavepoint_RemovesSavepointAndNewer) {
    auto txn = mgr_->begin();
    ASSERT_TRUE(txn.createSavepoint("sp1").ok);
    ASSERT_TRUE(txn.createSavepoint("sp2").ok);
    ASSERT_TRUE(txn.createSavepoint("sp3").ok);

    // Rollback to sp2 – sp2 and sp3 are removed
    EXPECT_TRUE(txn.rollbackToSavepoint("sp2").ok);

    EXPECT_TRUE(txn.hasSavepoint("sp1"));
    EXPECT_FALSE(txn.hasSavepoint("sp2"));
    EXPECT_FALSE(txn.hasSavepoint("sp3"));

    auto sps = txn.getSavepoints();
    ASSERT_EQ(sps.size(), 1u);
    EXPECT_EQ(sps[0], "sp1");

    txn.rollback();
}

TEST_F(NamedSavepointTest, RollbackToSavepoint_AllSavepointsRemoved) {
    auto txn = mgr_->begin();
    ASSERT_TRUE(txn.createSavepoint("sp1").ok);
    ASSERT_TRUE(txn.createSavepoint("sp2").ok);

    EXPECT_TRUE(txn.rollbackToSavepoint("sp1").ok);

    EXPECT_TRUE(txn.getSavepoints().empty());
    EXPECT_FALSE(txn.hasSavepoint("sp1"));
    EXPECT_FALSE(txn.hasSavepoint("sp2"));

    txn.rollback();
}

// ─────────────────────────────────────────────────────────────────────────────
// releaseSavepoint
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(NamedSavepointTest, ReleaseSavepoint_UnknownName_ReturnsError) {
    auto txn = mgr_->begin();
    auto st = txn.releaseSavepoint("ghost");
    EXPECT_FALSE(st.ok);
    txn.rollback();
}

TEST_F(NamedSavepointTest, ReleaseSavepoint_RemovesSavepointAndNewer) {
    auto txn = mgr_->begin();
    ASSERT_TRUE(txn.createSavepoint("sp1").ok);
    ASSERT_TRUE(txn.createSavepoint("sp2").ok);
    ASSERT_TRUE(txn.createSavepoint("sp3").ok);

    // Releasing sp2 should also remove sp3
    EXPECT_TRUE(txn.releaseSavepoint("sp2").ok);

    EXPECT_TRUE(txn.hasSavepoint("sp1"));
    EXPECT_FALSE(txn.hasSavepoint("sp2"));
    EXPECT_FALSE(txn.hasSavepoint("sp3"));

    txn.rollback();
}

// ─────────────────────────────────────────────────────────────────────────────
// Error handling on finished transaction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(NamedSavepointTest, OperationsOnFinishedTransaction_ReturnError) {
    auto txn = mgr_->begin();
    txn.rollback();

    EXPECT_FALSE(txn.createSavepoint("sp").ok);
    EXPECT_FALSE(txn.rollbackToSavepoint("sp").ok);
    EXPECT_FALSE(txn.releaseSavepoint("sp").ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// SAGA integration: steps added after savepoint are trimmed on rollback
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(NamedSavepointTest, RollbackToSavepoint_TrimsSagaSteps) {
    auto txn = mgr_->begin();

    // Add a SAGA step before the savepoint
    int compensated_before = 0;
    int compensated_after  = 0;
    txn.getSaga().addStep("before_sp", [&compensated_before]() { ++compensated_before; });

    ASSERT_TRUE(txn.createSavepoint("sp").ok);

    // Add a SAGA step after the savepoint
    txn.getSaga().addStep("after_sp", [&compensated_after]() { ++compensated_after; });

    EXPECT_EQ(txn.getSaga().stepCount(), 2u);

    // Rollback to savepoint: step added after savepoint must be trimmed
    EXPECT_TRUE(txn.rollbackToSavepoint("sp").ok);
    EXPECT_EQ(txn.getSaga().stepCount(), 1u);

    // Full rollback executes only the remaining (before) step
    txn.rollback();
    EXPECT_EQ(compensated_before, 1);
    EXPECT_EQ(compensated_after,  0); // was trimmed, must NOT execute
}

TEST_F(NamedSavepointTest, ReleaseSavepoint_PreservesSagaSteps) {
    auto txn = mgr_->begin();

    int compensated = 0;
    txn.getSaga().addStep("step1", [&compensated]() { ++compensated; });

    ASSERT_TRUE(txn.createSavepoint("sp").ok);

    txn.getSaga().addStep("step2", [&compensated]() { ++compensated; });

    EXPECT_EQ(txn.getSaga().stepCount(), 2u);

    // releaseSavepoint must NOT trim SAGA steps (writes are kept)
    EXPECT_TRUE(txn.releaseSavepoint("sp").ok);
    EXPECT_EQ(txn.getSaga().stepCount(), 2u);

    txn.rollback();
    EXPECT_EQ(compensated, 2); // both steps compensated
}

TEST_F(NamedSavepointTest, RollbackToSavepoint_MultipleSagaStepsTrimmed) {
    auto txn = mgr_->begin();

    int before_count = 0;
    int after_count  = 0;

    txn.getSaga().addStep("pre1", [&before_count]() { ++before_count; });
    txn.getSaga().addStep("pre2", [&before_count]() { ++before_count; });

    ASSERT_TRUE(txn.createSavepoint("sp").ok);

    txn.getSaga().addStep("post1", [&after_count]() { ++after_count; });
    txn.getSaga().addStep("post2", [&after_count]() { ++after_count; });
    txn.getSaga().addStep("post3", [&after_count]() { ++after_count; });

    EXPECT_EQ(txn.getSaga().stepCount(), 5u);

    EXPECT_TRUE(txn.rollbackToSavepoint("sp").ok);
    EXPECT_EQ(txn.getSaga().stepCount(), 2u);

    txn.rollback();
    EXPECT_EQ(before_count, 2);
    EXPECT_EQ(after_count,  0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bug fix: rollbackToSavepoint partial-state — savepoints_ must stay in sync
// with the RocksDB stack when rollbackToSavePoint() fails after popSavePoint()
// calls have already consumed newer savepoints.
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(NamedSavepointTest, RollbackToSavepoint_FailureKeepsSavepointsConsistent) {
    auto txn = mgr_->begin();

    // Create three savepoints in order: sp1 (oldest), sp2, sp3 (newest)
    ASSERT_TRUE(txn.createSavepoint("sp1").ok);
    ASSERT_TRUE(txn.createSavepoint("sp2").ok);
    ASSERT_TRUE(txn.createSavepoint("sp3").ok);

    EXPECT_EQ(txn.getSavepoints().size(), 3u);

    // rollbackToSavepoint("sp1") internally pops sp3 and sp2 from the RocksDB
    // stack (popSavePoint × 2) before calling rollbackToSavePoint() to consume
    // sp1.  After success, savepoints_ must be fully cleared.
    // The failure branch (where rollbackToSavePoint() returns false and we must
    // still erase the entries for sp2/sp3) cannot be triggered without mocking
    // RocksDB internals, so the success path is the observable regression test.
    EXPECT_TRUE(txn.rollbackToSavepoint("sp1").ok);
    EXPECT_EQ(txn.getSavepoints().size(), 0u);
    EXPECT_FALSE(txn.hasSavepoint("sp1"));
    EXPECT_FALSE(txn.hasSavepoint("sp2"));
    EXPECT_FALSE(txn.hasSavepoint("sp3"));

    txn.rollback();
}

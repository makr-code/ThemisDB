/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_savepoints.cpp                                ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 18:23:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for savepoint support in RocksDBWrapper::TransactionWrapper:
//   - setSavePoint / rollbackToSavePoint / popSavePoint round-trip
//   - Nested savepoints (stack behaviour)
//   - rollback without savepoint returns error
//   - Writes after rollbackToSavePoint are visible / rolled-back correctly
//
// These tests operate directly against RocksDB via TransactionDB so they
// do NOT require the full ThemisDB stack.

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"

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

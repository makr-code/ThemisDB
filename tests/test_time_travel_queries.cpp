// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for time-travel queries against snapshot history.
// Covers:
//  - TransactionManager::readEntityAtTimestamp()
//  - TransactionManager::readEntityAtUnixMs()
//  - TransactionManager::readEntityAtSnapshot()
//  - TransactionManager::listEntityVersions()
//  - Behavior when no HistoryManager is configured (returns nullopt / empty)
//  - Snapshot-based time-travel via SnapshotManager tag

#include <gtest/gtest.h>
#include "storage/hlc.h"
#include "storage/history_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "transaction/transaction_manager.h"
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

using namespace themis;
using namespace themis::transaction;

// ─────────────────────────────────────────────────────────────────────────────
// Shared test fixture
// ─────────────────────────────────────────────────────────────────────────────

class TimeTravelQueryTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_time_travel_query_test";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path   = db_path_;
        cfg.enable_wal = true;
        db_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        clock_    = std::make_shared<HybridLogicalClock>();
        history_  = std::make_unique<HistoryManager>(db_, clock_);

        sec_idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        gph_idx_ = std::make_unique<GraphIndexManager>(*db_);
        vec_idx_ = std::make_unique<VectorIndexManager>(*db_);

        tx_mgr_ = std::make_unique<TransactionManager>(
                       *db_, *sec_idx_, *gph_idx_, *vec_idx_);
        tx_mgr_->setHistoryManager(history_.get());

        // Set up SnapshotManager (needs a changefeed for sequence tracking)
        auto* txn_db = db_->getRawDB();
        ASSERT_NE(txn_db, nullptr);
        changefeed_    = std::make_unique<Changefeed>(txn_db);
        snapshot_mgr_  = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        tx_mgr_->setSnapshotManager(snapshot_mgr_.get());
    }

    void TearDown() override {
        tx_mgr_.reset();
        snapshot_mgr_.reset();
        changefeed_.reset();
        vec_idx_.reset();
        gph_idx_.reset();
        sec_idx_.reset();
        history_.reset();
        db_->close();
        db_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    /// Write an entity in its own committed transaction and return the HLC
    /// timestamp recorded in the history entry.
    HLCTimestamp writeEntity(const std::string& table,
                              const std::string& pk,
                              const std::string& field_name,
                              int64_t field_value) {
        auto txn_id = tx_mgr_->beginTransaction();
        auto txn    = tx_mgr_->getTransaction(txn_id);
        BaseEntity e(pk);
        e.setField(field_name, field_value);
        EXPECT_TRUE(txn->putEntity(table, e).ok);
        EXPECT_TRUE(tx_mgr_->commitTransaction(txn_id).ok);

        const std::string live_key = "entity:" + table + ":" + pk;
        auto versions = history_->listVersions(live_key);
        EXPECT_FALSE(versions.empty());
        return versions.back().timestamp;
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>        db_;
    std::shared_ptr<HybridLogicalClock>    clock_;
    std::unique_ptr<HistoryManager>        history_;
    std::unique_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<GraphIndexManager>     gph_idx_;
    std::unique_ptr<VectorIndexManager>    vec_idx_;
    std::unique_ptr<TransactionManager>    tx_mgr_;
    std::unique_ptr<Changefeed>            changefeed_;
    std::unique_ptr<SnapshotManager>       snapshot_mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// readEntityAtTimestamp
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TimeTravelQueryTest, ReadEntityAtTimestamp_ReturnsCurrentVersion) {
    HLCTimestamp ts = writeEntity("products", "p1", "price", 100);

    auto result = tx_mgr_->readEntityAtTimestamp("products", "p1", ts);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->op, "put");
    EXPECT_EQ(result->base_key, "entity:products:p1");
    EXPECT_FALSE(result->value.empty());
    EXPECT_EQ(result->timestamp, ts);
}

TEST_F(TimeTravelQueryTest, ReadEntityAtTimestamp_ReturnsEarlierVersion) {
    HLCTimestamp ts1 = writeEntity("products", "p2", "price", 50);
    HLCTimestamp ts2 = writeEntity("products", "p2", "price", 75);

    EXPECT_LT(ts1, ts2);

    // Reading at ts1 should return the first version (price=50).
    auto r1 = tx_mgr_->readEntityAtTimestamp("products", "p2", ts1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->op, "put");
    EXPECT_EQ(r1->timestamp, ts1);

    // Reading at ts2 should return the second version (price=75).
    auto r2 = tx_mgr_->readEntityAtTimestamp("products", "p2", ts2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->op, "put");
    EXPECT_EQ(r2->timestamp, ts2);
}

TEST_F(TimeTravelQueryTest, ReadEntityAtTimestamp_BeforeFirstWrite_ReturnsNullopt) {
    writeEntity("products", "p3", "price", 99);

    // A timestamp before any write should return nullopt.
    auto r = tx_mgr_->readEntityAtTimestamp("products", "p3", HLCTimestamp(0));
    EXPECT_FALSE(r.has_value());
}

TEST_F(TimeTravelQueryTest, ReadEntityAtTimestamp_UnknownEntity_ReturnsNullopt) {
    auto r = tx_mgr_->readEntityAtTimestamp("products", "nonexistent",
                                             HLCTimestamp(~0ULL));
    EXPECT_FALSE(r.has_value());
}

TEST_F(TimeTravelQueryTest, ReadEntityAtTimestamp_NoHistoryManager_ReturnsNullopt) {
    // Disable history manager.
    tx_mgr_->setHistoryManager(nullptr);
    auto r = tx_mgr_->readEntityAtTimestamp("products", "p99",
                                             HLCTimestamp(~0ULL));
    EXPECT_FALSE(r.has_value());
    // Re-enable for other tests that may share the fixture.
    tx_mgr_->setHistoryManager(history_.get());
}

// ─────────────────────────────────────────────────────────────────────────────
// readEntityAtUnixMs
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TimeTravelQueryTest, ReadEntityAtUnixMs_ReturnsVersion) {
    HLCTimestamp ts = writeEntity("orders", "o1", "qty", 10);
    // The physical component of ts is the wall-clock ms at write time.
    int64_t write_unix_ms = static_cast<int64_t>(ts.physical());

    // Querying at or after that wall-clock time should find the record.
    auto r = tx_mgr_->readEntityAtUnixMs("orders", "o1", write_unix_ms);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->op, "put");
}

TEST_F(TimeTravelQueryTest, ReadEntityAtUnixMs_NegativeTimestamp_ReturnsNullopt) {
    writeEntity("orders", "o2", "qty", 5);
    auto r = tx_mgr_->readEntityAtUnixMs("orders", "o2", -1);
    EXPECT_FALSE(r.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// readEntityAtSnapshot
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TimeTravelQueryTest, ReadEntityAtSnapshot_ReturnsVersionAtTag) {
    // Write first version, then create a named snapshot.
    writeEntity("catalog", "c1", "rev", 1);

    // Small sleep so the tag timestamp is not before the write.
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto snap = snapshot_mgr_->createTag("v1-tag", "test snapshot", "test");
    ASSERT_TRUE(snap.has_value());

    // Write a second version after the snapshot.
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    writeEntity("catalog", "c1", "rev", 2);

    // Time-travel to the snapshot: should see rev=1 (first version).
    auto r = tx_mgr_->readEntityAtSnapshot("catalog", "c1", "v1-tag");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->op, "put");
    // The record must be the first version (timestamp <= tag timestamp).
    int64_t tag_unix_ms = snap->timestamp_ms;
    EXPECT_LE(static_cast<int64_t>(r->timestamp.physical()), tag_unix_ms);
}

TEST_F(TimeTravelQueryTest, ReadEntityAtSnapshot_UnknownTag_ReturnsNullopt) {
    writeEntity("catalog", "c2", "rev", 1);
    auto r = tx_mgr_->readEntityAtSnapshot("catalog", "c2", "no-such-tag");
    EXPECT_FALSE(r.has_value());
}

TEST_F(TimeTravelQueryTest, ReadEntityAtSnapshot_NoSnapshotManager_ReturnsNullopt) {
    tx_mgr_->setSnapshotManager(nullptr);
    auto r = tx_mgr_->readEntityAtSnapshot("catalog", "c3", "any-tag");
    EXPECT_FALSE(r.has_value());
    tx_mgr_->setSnapshotManager(snapshot_mgr_.get());
}

// ─────────────────────────────────────────────────────────────────────────────
// listEntityVersions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TimeTravelQueryTest, ListEntityVersions_MultipleWrites) {
    writeEntity("sessions", "s1", "count", 1);
    writeEntity("sessions", "s1", "count", 2);
    writeEntity("sessions", "s1", "count", 3);

    auto versions = tx_mgr_->listEntityVersions("sessions", "s1");
    ASSERT_EQ(versions.size(), 3u);

    // Must be strictly ascending.
    EXPECT_LT(versions[0].timestamp, versions[1].timestamp);
    EXPECT_LT(versions[1].timestamp, versions[2].timestamp);

    // All should be "put".
    for (const auto& v : versions) {
        EXPECT_EQ(v.op, "put");
        EXPECT_EQ(v.base_key, "entity:sessions:s1");
        EXPECT_FALSE(v.value.empty());
    }
}

TEST_F(TimeTravelQueryTest, ListEntityVersions_IncludesTombstone) {
    // Write then erase.
    writeEntity("sessions", "s2", "count", 1);

    auto txn_id = tx_mgr_->beginTransaction();
    auto txn    = tx_mgr_->getTransaction(txn_id);
    ASSERT_TRUE(txn->eraseEntity("sessions", "s2").ok);
    ASSERT_TRUE(tx_mgr_->commitTransaction(txn_id).ok);

    auto versions = tx_mgr_->listEntityVersions("sessions", "s2");
    ASSERT_GE(versions.size(), 2u);
    EXPECT_EQ(versions[0].op, "put");
    EXPECT_EQ(versions.back().op, "del");
}

TEST_F(TimeTravelQueryTest, ListEntityVersions_EmptyForUnknownEntity) {
    auto versions = tx_mgr_->listEntityVersions("sessions", "nonexistent");
    EXPECT_TRUE(versions.empty());
}

TEST_F(TimeTravelQueryTest, ListEntityVersions_NoHistoryManager_ReturnsEmpty) {
    tx_mgr_->setHistoryManager(nullptr);
    auto versions = tx_mgr_->listEntityVersions("sessions", "s99");
    EXPECT_TRUE(versions.empty());
    tx_mgr_->setHistoryManager(history_.get());
}

// ─────────────────────────────────────────────────────────────────────────────
// TimeTravelRecord fields
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TimeTravelQueryTest, TimeTravelRecord_TxnIdIsPopulated) {
    HLCTimestamp ts = writeEntity("audit", "a1", "action", 42);
    auto r = tx_mgr_->readEntityAtTimestamp("audit", "a1", ts);
    ASSERT_TRUE(r.has_value());
    // txn_id must be non-zero: it is the ID assigned by beginTransaction().
    EXPECT_GT(r->txn_id, 0u);
}

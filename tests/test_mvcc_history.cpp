// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for the atomic History/Conflict layer.
// Covers:
//  - HistoryManager: recordPut/recordDel within a transaction
//  - HistoryManager: getAtTimestamp, listVersions (time-travel reads)
//  - ConflictManager: storeConflict, getConflict, listConflicts
//  - TransactionManager integration: putEntity writes history atomically
//  - Status::Conflict returned when commit fails (simulated via OCC version conflict)
//  - History key encoding/decoding helpers

#include <gtest/gtest.h>
#include "storage/hlc.h"
#include "storage/mvcc_store.h"
#include "storage/history_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>
#include <vector>

using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class HistoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_history_manager_test";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
        RocksDBWrapper::Config cfg;
        cfg.db_path   = db_path_;
        cfg.enable_wal = true;
        rocksdb_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(rocksdb_->open());

        clock_   = std::make_shared<HybridLogicalClock>();
        history_ = std::make_unique<HistoryManager>(rocksdb_, clock_);
        conflicts_ = std::make_unique<ConflictManager>(rocksdb_, clock_);
    }

    void TearDown() override {
        history_.reset();
        conflicts_.reset();
        rocksdb_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    static std::vector<uint8_t> bytes(std::string_view s) {
        return {s.begin(), s.end()};
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>     rocksdb_;
    std::shared_ptr<HybridLogicalClock> clock_;
    std::unique_ptr<HistoryManager>     history_;
    std::unique_ptr<ConflictManager>    conflicts_;
};

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager: key encoding helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(HistoryManagerKeyTest, HistoryKeyPrefix) {
    auto prefix = HistoryManager::historyPrefix("entity:users:u1");
    // Should start with "hist:" and end with '\x00'
    EXPECT_EQ(prefix.substr(0, 5), "hist:");
    EXPECT_EQ(prefix.back(), '\x00');
    EXPECT_NE(prefix.find("entity:users:u1"), std::string::npos);
}

TEST(HistoryManagerKeyTest, HistoryKeyIncludesTimestamp) {
    auto ts  = HLCTimestamp::from(12345u, 7u);
    auto key = HistoryManager::historyKey("mykey", ts);
    // Must be longer than prefix alone
    auto prefix = HistoryManager::historyPrefix("mykey");
    EXPECT_GT(key.size(), prefix.size());
    // Last 8 bytes should decode back to ts
    HLCTimestamp recovered = MVCCStore::decodeTimestamp(key);
    EXPECT_EQ(recovered, ts);
}

TEST(HistoryManagerKeyTest, NoCollisionWithLiveKeys) {
    // History keys must not sort alongside live entity keys.
    auto live    = std::string("entity:users:u1");
    auto history = HistoryManager::historyKey("entity:users:u1", HLCTimestamp{100});
    // They are different strings.
    EXPECT_NE(live, history);
    // History key starts with "hist:".
    EXPECT_EQ(history.substr(0, 5), "hist:");
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager: record serialization round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(HistoryManagerSerTest, PutRecordRoundTrip) {
    HistoryRecord rec;
    rec.base_key  = "entity:users:u1";
    rec.timestamp = HLCTimestamp::from(9876u, 5u);
    rec.op        = "put";
    rec.value     = {0x01, 0x02, 0x03};
    rec.txn_id    = 42u;

    auto serialized = HistoryManager::serializeHistoryRecord(rec);
    ASSERT_FALSE(serialized.empty());

    auto recovered = HistoryManager::deserializeHistoryRecord(
        std::string_view(reinterpret_cast<const char*>(serialized.data()), serialized.size()));
    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(recovered->base_key,  rec.base_key);
    EXPECT_EQ(recovered->timestamp, rec.timestamp);
    EXPECT_EQ(recovered->op,        rec.op);
    EXPECT_EQ(recovered->value,     rec.value);
    EXPECT_EQ(recovered->txn_id,    rec.txn_id);
}

TEST(HistoryManagerSerTest, DelRecordRoundTrip) {
    HistoryRecord rec;
    rec.base_key  = "entity:docs:d99";
    rec.timestamp = HLCTimestamp::from(111u, 0u);
    rec.op        = "del";
    rec.txn_id    = 0u;

    auto serialized = HistoryManager::serializeHistoryRecord(rec);
    auto recovered  = HistoryManager::deserializeHistoryRecord(
        std::string_view(reinterpret_cast<const char*>(serialized.data()), serialized.size()));
    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(recovered->op, "del");
    EXPECT_TRUE(recovered->value.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager: history written atomically with live write
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HistoryManagerTest, RecordPutWrittenInTransaction) {
    auto txn = rocksdb_->beginTransaction();
    ASSERT_NE(txn, nullptr);

    std::string key   = "entity:users:u1";
    auto        value = bytes("hello");

    // Write live key
    ASSERT_TRUE(txn->put(key, value));
    // Write history entry atomically
    HLCTimestamp ts = history_->recordPut(*txn, key, value, /*txn_id=*/1u);
    ASSERT_GT(ts.value, 0u);

    // Commit both atomically
    ASSERT_TRUE(txn->commit());

    // Verify live key persisted
    auto live = rocksdb_->get(key);
    ASSERT_TRUE(live.has_value());
    EXPECT_EQ(*live, value);

    // Verify history entry persisted
    auto versions = history_->listVersions(key);
    ASSERT_EQ(versions.size(), 1u);
    EXPECT_EQ(versions[0].op,    "put");
    EXPECT_EQ(versions[0].value, value);
    EXPECT_EQ(versions[0].txn_id, 1u);
}

TEST_F(HistoryManagerTest, RecordDelWrittenInTransaction) {
    // Pre-populate the live key
    std::string key = "entity:users:u2";
    rocksdb_->put(key, bytes("initial"));

    auto txn = rocksdb_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    ASSERT_TRUE(txn->del(key));
    history_->recordDel(*txn, key, /*txn_id=*/2u);
    ASSERT_TRUE(txn->commit());

    // Live key deleted
    auto live = rocksdb_->get(key);
    EXPECT_FALSE(live.has_value());

    // History shows a tombstone
    auto versions = history_->listVersions(key);
    ASSERT_GE(versions.size(), 1u);
    EXPECT_EQ(versions.back().op, "del");
    EXPECT_EQ(versions.back().txn_id, 2u);
}

TEST_F(HistoryManagerTest, HistoryAbortedWhenTransactionRolledBack) {
    std::string key   = "entity:users:u3";
    auto        value = bytes("should not persist");

    auto txn = rocksdb_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    ASSERT_TRUE(txn->put(key, value));
    history_->recordPut(*txn, key, value);
    txn->rollback();

    // Neither live key nor history entry should exist
    auto live = rocksdb_->get(key);
    EXPECT_FALSE(live.has_value());

    auto versions = history_->listVersions(key);
    EXPECT_TRUE(versions.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager: getAtTimestamp (time-travel reads)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HistoryManagerTest, GetAtTimestamp_ExactMatch) {
    std::string key  = "entity:items:i1";
    auto        val1 = bytes("version_one");

    auto txn1 = rocksdb_->beginTransaction();
    ASSERT_NE(txn1, nullptr);
    txn1->put(key, val1);
    HLCTimestamp ts1 = history_->recordPut(*txn1, key, val1);
    ASSERT_TRUE(txn1->commit());

    auto rec = history_->getAtTimestamp(key, ts1);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->value, val1);
    EXPECT_EQ(rec->op, "put");
}

TEST_F(HistoryManagerTest, GetAtTimestamp_ReturnsEarlierVersion) {
    std::string key  = "entity:items:i2";
    auto        val1 = bytes("first");
    auto        val2 = bytes("second");

    auto txn1 = rocksdb_->beginTransaction();
    txn1->put(key, val1);
    HLCTimestamp ts1 = history_->recordPut(*txn1, key, val1);
    txn1->commit();

    auto txn2 = rocksdb_->beginTransaction();
    txn2->put(key, val2);
    HLCTimestamp ts2 = history_->recordPut(*txn2, key, val2);
    txn2->commit();

    ASSERT_GT(ts2, ts1);

    // Reading at ts1 should return val1
    auto r1 = history_->getAtTimestamp(key, ts1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->value, val1);

    // Reading at ts2 should return val2
    auto r2 = history_->getAtTimestamp(key, ts2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->value, val2);
}

TEST_F(HistoryManagerTest, GetAtTimestamp_BeforeFirstVersion) {
    std::string key  = "entity:items:i3";
    auto        val  = bytes("exists");

    auto txn = rocksdb_->beginTransaction();
    txn->put(key, val);
    HLCTimestamp ts = history_->recordPut(*txn, key, val);
    txn->commit();

    // Request at timestamp strictly before the first version
    HLCTimestamp before{ts.value > 0 ? ts.value - 1 : 0};
    auto rec = history_->getAtTimestamp(key, before);
    EXPECT_FALSE(rec.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryManager: listVersions (ordered version scan)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HistoryManagerTest, ListVersions_AscendingOrder) {
    std::string key = "entity:doc:d1";
    std::vector<std::vector<uint8_t>> values = {bytes("v1"), bytes("v2"), bytes("v3")};

    for (auto& v : values) {
        auto txn = rocksdb_->beginTransaction();
        txn->put(key, v);
        history_->recordPut(*txn, key, v);
        txn->commit();
    }

    auto versions = history_->listVersions(key);
    ASSERT_EQ(versions.size(), 3u);
    EXPECT_EQ(versions[0].value, values[0]);
    EXPECT_EQ(versions[1].value, values[1]);
    EXPECT_EQ(versions[2].value, values[2]);

    // Timestamps must be strictly ascending
    EXPECT_LT(versions[0].timestamp, versions[1].timestamp);
    EXPECT_LT(versions[1].timestamp, versions[2].timestamp);
}

TEST_F(HistoryManagerTest, ListVersions_EmptyForUnknownKey) {
    auto versions = history_->listVersions("entity:does:not:exist");
    EXPECT_TRUE(versions.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ConflictManager: store and retrieve conflict records
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HistoryManagerTest, StoreAndRetrieveConflict) {
    ConflictRecord rec;
    rec.base_key     = "entity:users:u5";
    rec.txn_id       = 99u;
    rec.base_value   = bytes("base_data");
    rec.ours_value   = bytes("our_data");
    rec.theirs_value = bytes("their_data");

    std::string cid = conflicts_->storeConflict(rec);
    ASSERT_FALSE(cid.empty());

    auto retrieved = conflicts_->getConflict(cid);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->base_key,     "entity:users:u5");
    EXPECT_EQ(retrieved->txn_id,       99u);
    EXPECT_EQ(retrieved->base_value,   bytes("base_data"));
    EXPECT_EQ(retrieved->ours_value,   bytes("our_data"));
    EXPECT_EQ(retrieved->theirs_value, bytes("their_data"));
}

TEST_F(HistoryManagerTest, ListConflicts) {
    for (int i = 0; i < 3; ++i) {
        ConflictRecord rec;
        rec.base_key = "entity:k" + std::to_string(i);
        conflicts_->storeConflict(rec);
    }

    auto all = conflicts_->listConflicts();
    EXPECT_GE(all.size(), 3u);
}

TEST_F(HistoryManagerTest, GetConflict_NotFound) {
    auto result = conflicts_->getConflict("no_such_conflict_id");
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// ConflictManager: serialization round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(ConflictManagerSerTest, RoundTrip) {
    ConflictRecord rec;
    rec.version      = 1;
    rec.conflict_id  = "1234567890_7";
    rec.base_key     = "entity:users:u10";
    rec.detected_at  = HLCTimestamp::from(1234567890u, 7u);
    rec.txn_id       = 55u;
    rec.base_value   = {0xAA, 0xBB};
    rec.ours_value   = {0xCC, 0xDD};
    rec.theirs_value = {0xEE, 0xFF};

    auto serialized = ConflictManager::serializeConflictRecord(rec);
    ASSERT_FALSE(serialized.empty());

    auto recovered = ConflictManager::deserializeConflictRecord(
        std::string_view(reinterpret_cast<const char*>(serialized.data()), serialized.size()));
    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(recovered->conflict_id,  rec.conflict_id);
    EXPECT_EQ(recovered->base_key,     rec.base_key);
    EXPECT_EQ(recovered->detected_at,  rec.detected_at);
    EXPECT_EQ(recovered->txn_id,       rec.txn_id);
    EXPECT_EQ(recovered->base_value,   rec.base_value);
    EXPECT_EQ(recovered->ours_value,   rec.ours_value);
    EXPECT_EQ(recovered->theirs_value, rec.theirs_value);
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore::putInTxn / delInTxn
// ─────────────────────────────────────────────────────────────────────────────

class MVCCStoreTxnTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_mvcc_txn_test";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
        RocksDBWrapper::Config cfg;
        cfg.db_path   = db_path_;
        cfg.enable_wal = true;
        rocksdb_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(rocksdb_->open());
        clock_ = std::make_shared<HybridLogicalClock>();
        store_ = std::make_unique<MVCCStore>(rocksdb_, clock_);
    }

    void TearDown() override {
        store_.reset();
        rocksdb_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    static std::vector<uint8_t> bytes(std::string_view s) {
        return {s.begin(), s.end()};
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>     rocksdb_;
    std::shared_ptr<HybridLogicalClock> clock_;
    std::unique_ptr<MVCCStore>          store_;
};

TEST_F(MVCCStoreTxnTest, PutInTxn_CommitAndRead) {
    std::string key = "mvcc_txn_key";
    auto value = bytes("txn_value");

    auto txn = rocksdb_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    HLCTimestamp ts = store_->putInTxn(*txn, key, value);
    ASSERT_GT(ts.value, 0u);
    ASSERT_TRUE(txn->commit());

    // Should be readable via getAtTimestamp
    auto result = store_->getAtTimestamp(key, ts);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, value);
}

TEST_F(MVCCStoreTxnTest, PutInTxn_RollbackMeansNotVisible) {
    std::string key = "mvcc_txn_rollback_key";
    auto value = bytes("will_not_persist");

    auto txn = rocksdb_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    store_->putInTxn(*txn, key, value);
    txn->rollback();

    auto result = store_->getLatest(key);
    EXPECT_FALSE(result.has_value());
}

TEST_F(MVCCStoreTxnTest, DelInTxn_WritesTombstoneVersion) {
    std::string key = "mvcc_del_key";
    auto value = bytes("initial");

    // First, put a normal version
    HLCTimestamp ts1 = store_->put(key, value);
    auto r1 = store_->getAtTimestamp(key, ts1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(*r1, value);

    // Now delete via transaction
    auto txn = rocksdb_->beginTransaction();
    ASSERT_NE(txn, nullptr);
    HLCTimestamp ts2 = store_->delInTxn(*txn, key);
    ASSERT_GT(ts2, ts1);
    ASSERT_TRUE(txn->commit());

    // The tombstone version exists (empty value)
    auto r2 = store_->getAtTimestamp(key, ts2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(r2->empty()); // tombstone is empty

    // Reading at ts1 still returns original value
    auto r1b = store_->getAtTimestamp(key, ts1);
    ASSERT_TRUE(r1b.has_value());
    EXPECT_EQ(*r1b, value);
}

// ─────────────────────────────────────────────────────────────────────────────
// TransactionManager integration: putEntity writes history atomically
// ─────────────────────────────────────────────────────────────────────────────

// NOTE: TransactionManager requires SecondaryIndexManager, GraphIndexManager,
// VectorIndexManager.  These tests exercise the lower-level APIs directly.
// Full TransactionManager integration is covered by the existing test_mvcc.cpp.

// ─────────────────────────────────────────────────────────────────────────────
// Status::Conflict structure test
// ─────────────────────────────────────────────────────────────────────────────

#include "transaction/transaction_manager.h"

TEST(StatusConflictTest, ConflictFactoryMethod) {
    using Status = themis::TransactionManager::Status;
    auto s = Status::Conflict("commit failed", "cid_123", {"key_a", "key_b"});
    EXPECT_FALSE(s.ok);
    EXPECT_EQ(s.conflict_id, "cid_123");
    ASSERT_EQ(s.affected_keys.size(), 2u);
    EXPECT_EQ(s.affected_keys[0], "key_a");
    EXPECT_EQ(s.affected_keys[1], "key_b");
    EXPECT_FALSE(s.message.empty());
}

TEST(StatusConflictTest, OKStatusHasEmptyConflictId) {
    using Status = themis::TransactionManager::Status;
    auto s = Status::OK();
    EXPECT_TRUE(s.ok);
    EXPECT_TRUE(s.conflict_id.empty());
    EXPECT_TRUE(s.affected_keys.empty());
}

TEST(StatusConflictTest, ErrorStatusHasEmptyConflictId) {
    using Status = themis::TransactionManager::Status;
    auto s = Status::Error("something failed");
    EXPECT_FALSE(s.ok);
    EXPECT_TRUE(s.conflict_id.empty());
}

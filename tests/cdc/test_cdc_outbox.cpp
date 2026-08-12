// Tests for CDC Outbox Pattern
// Covers: OutboxWriter::writeToOutbox, OutboxRelay::relayOnce, state transitions,
//         listRecords, removeRecord, purgePublished, error paths.

#include <gtest/gtest.h>
#include "cdc/outbox.h"
#include "cdc/changefeed.h"
#include "cdc/cdc_error.h"
#include "storage/rocksdb_wrapper.h"
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <stdexcept>

using namespace themis;
using namespace themis::cdc;
namespace fs = std::filesystem;

// ===== Test Fixture =====

class OutboxTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC outbox focused tests on Windows due to fixture crash in current runtime.";
#endif
        test_db_path_ = "/tmp/test_outbox_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path              = test_db_path_;
        cfg.memtable_size_mb     = 16;
        cfg.block_cache_size_mb  = 32;

        db_   = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        raw_db_     = db_->getDB();
        ASSERT_NE(raw_db_, nullptr);

        changefeed_ = std::make_unique<Changefeed>(raw_db_, nullptr);
        writer_     = std::make_unique<OutboxWriter>(raw_db_, nullptr);

        OutboxRelayConfig relay_cfg;
        relay_cfg.poll_interval     = std::chrono::milliseconds(50);
        relay_cfg.batch_size        = 50;
        relay_cfg.max_relay_attempts = 3;
        relay_ = std::make_unique<OutboxRelay>(raw_db_, nullptr, *changefeed_, relay_cfg);
    }

    void TearDown() override {
        relay_.reset();
        writer_.reset();
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }
    }

    OutboxRecord makeRecord(const std::string& key,
                            const std::string& collection = "orders",
                            const std::string& value      = "{}") {
        OutboxRecord r;
        r.collection = collection;
        r.key        = key;
        r.value      = value;
        r.event_type = Changefeed::ChangeEventType::EVENT_PUT;
        return r;
    }

    std::string                    test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    rocksdb::TransactionDB*         raw_db_{nullptr};
    std::unique_ptr<Changefeed>     changefeed_;
    std::unique_ptr<OutboxWriter>   writer_;
    std::unique_ptr<OutboxRelay>    relay_;
};

// ===== OutboxWriter Tests =====

TEST_F(OutboxTest, WriteToOutboxAssignsSequence) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    ASSERT_NE(txn, nullptr);

    OutboxRecord rec = makeRecord("orders:1");
    writer_->writeToOutbox(txn, rec);

    EXPECT_GE(rec.outbox_sequence, uint64_t(1));
    EXPECT_EQ(rec.state, OutboxState::PENDING);
    EXPECT_GT(rec.created_at_ms, int64_t(0));
    EXPECT_EQ(rec.relay_attempts, 0);

    txn->Commit();
    delete txn;
}

TEST_F(OutboxTest, WriteToOutboxSequencesAreMonotonic) {
    rocksdb::WriteOptions wo;
    auto* txn1 = raw_db_->BeginTransaction(wo);
    auto* txn2 = raw_db_->BeginTransaction(wo);

    OutboxRecord r1 = makeRecord("orders:1");
    OutboxRecord r2 = makeRecord("orders:2");

    writer_->writeToOutbox(txn1, r1);
    txn1->Commit();
    delete txn1;

    writer_->writeToOutbox(txn2, r2);
    txn2->Commit();
    delete txn2;

    EXPECT_LT(r1.outbox_sequence, r2.outbox_sequence);
}

TEST_F(OutboxTest, WriteToOutboxPersistsRecord) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);

    OutboxRecord rec = makeRecord("products:42");
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    // Verify the record is visible via relay scan
    auto records = relay_->listRecords(OutboxState::PENDING);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].key, "products:42");
    EXPECT_EQ(records[0].collection, "orders");
    EXPECT_EQ(records[0].state, OutboxState::PENDING);
}

TEST_F(OutboxTest, WriteToOutboxNullTxnThrows) {
    OutboxRecord rec = makeRecord("orders:1");
    EXPECT_THROW(writer_->writeToOutbox(nullptr, rec), CDCException);
}

TEST_F(OutboxTest, WriteToOutboxEmptyKeyThrows) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);

    OutboxRecord rec = makeRecord("");
    EXPECT_THROW(writer_->writeToOutbox(txn, rec), CDCException);

    txn->Rollback();
    delete txn;
}

TEST_F(OutboxTest, WriteToOutboxRollbackLeavesNoRecord) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);

    OutboxRecord rec = makeRecord("users:99");
    writer_->writeToOutbox(txn, rec);
    txn->Rollback();
    delete txn;

    // After rollback the outbox should be empty
    // (sequence counter was already advanced but the record was not committed)
    auto records = relay_->listAllRecords();
    bool found = false;
    for (const auto& r : records) {
        if (r.key == "users:99") { found = true; break; }
    }
    EXPECT_FALSE(found);
}

// ===== OutboxRelay Tests =====

TEST_F(OutboxTest, RelayOncePendingRecordBecomesPublished) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    OutboxRecord rec = makeRecord("items:1");
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    size_t published = relay_->relayOnce();
    EXPECT_EQ(published, 1u);

    auto published_records = relay_->listRecords(OutboxState::PUBLISHED);
    ASSERT_EQ(published_records.size(), 1u);
    EXPECT_EQ(published_records[0].key, "items:1");
    EXPECT_GT(published_records[0].published_at_ms, int64_t(0));
}

TEST_F(OutboxTest, RelayOnceForwardsEventToChangefeed) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    OutboxRecord rec = makeRecord("inventory:7", "inventory", R"({"qty":5})");
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    relay_->relayOnce();

    auto events = changefeed_->listEvents();
    ASSERT_GE(events.size(), 1u);
    bool found = false;
    for (const auto& e : events) {
        if (e.key == "inventory:7") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(OutboxTest, RelayOnceEmptyOutboxReturnsZero) {
    size_t published = relay_->relayOnce();
    EXPECT_EQ(published, 0u);
}

TEST_F(OutboxTest, RelayOnceMultipleRecords) {
    rocksdb::WriteOptions wo;
    for (int i = 0; i < 5; ++i) {
        auto* txn = raw_db_->BeginTransaction(wo);
        OutboxRecord r = makeRecord("k:" + std::to_string(i));
        writer_->writeToOutbox(txn, r);
        txn->Commit();
        delete txn;
    }

    size_t published = relay_->relayOnce();
    EXPECT_EQ(published, 5u);
    EXPECT_EQ(relay_->totalRelayed(), uint64_t(5));
}

TEST_F(OutboxTest, RelayOnceDeleteEventType) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    OutboxRecord rec;
    rec.collection = "orders";
    rec.key        = "orders:del";
    rec.event_type = Changefeed::ChangeEventType::EVENT_DELETE;
    // value is nullopt for DELETE
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    size_t published = relay_->relayOnce();
    EXPECT_EQ(published, 1u);

    auto events = changefeed_->listEvents();
    bool found = false;
    for (const auto& e : events) {
        if (e.key == "orders:del" &&
            e.type == Changefeed::ChangeEventType::EVENT_DELETE) {
            found = true; break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(OutboxTest, TotalRelayedAndFailedCounters) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    OutboxRecord rec = makeRecord("a:1");
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    relay_->relayOnce();
    EXPECT_EQ(relay_->totalRelayed(), uint64_t(1));
    EXPECT_EQ(relay_->totalFailed(),  uint64_t(0));
}

// ===== Maintenance Tests =====

TEST_F(OutboxTest, RemoveRecordDeletesEntry) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    OutboxRecord rec = makeRecord("x:1");
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    relay_->relayOnce();

    auto published = relay_->listRecords(OutboxState::PUBLISHED);
    ASSERT_EQ(published.size(), 1u);
    uint64_t seq = published[0].outbox_sequence;

    bool removed = relay_->removeRecord(seq);
    EXPECT_TRUE(removed);

    auto after = relay_->listRecords(OutboxState::PUBLISHED);
    EXPECT_EQ(after.size(), 0u);
}

TEST_F(OutboxTest, PurgePublishedRemovesAllPublished) {
    rocksdb::WriteOptions wo;
    for (int i = 0; i < 3; ++i) {
        auto* txn = raw_db_->BeginTransaction(wo);
        OutboxRecord r = makeRecord("purge:" + std::to_string(i));
        writer_->writeToOutbox(txn, r);
        txn->Commit();
        delete txn;
    }

    relay_->relayOnce();
    ASSERT_EQ(relay_->listRecords(OutboxState::PUBLISHED).size(), 3u);

    size_t removed = relay_->purgePublished();
    EXPECT_EQ(removed, 3u);
    EXPECT_EQ(relay_->listRecords(OutboxState::PUBLISHED).size(), 0u);
}

TEST_F(OutboxTest, ListAllRecordsReturnsAllStates) {
    rocksdb::WriteOptions wo;

    // Write two records; relay one
    auto* txn1 = raw_db_->BeginTransaction(wo);
    OutboxRecord r1 = makeRecord("a:1");
    writer_->writeToOutbox(txn1, r1);
    txn1->Commit();
    delete txn1;

    auto* txn2 = raw_db_->BeginTransaction(wo);
    OutboxRecord r2 = makeRecord("a:2");
    writer_->writeToOutbox(txn2, r2);
    txn2->Commit();
    delete txn2;

    // relay only first batch of 1
    OutboxRelayConfig cfg;
    cfg.batch_size = 1;
    cfg.max_relay_attempts = 3;
    OutboxRelay relay1(raw_db_, nullptr, *changefeed_, cfg);
    relay1.relayOnce();

    auto all = relay1.listAllRecords();
    EXPECT_EQ(all.size(), 2u);
}

// ===== Background thread smoke test =====

TEST_F(OutboxTest, BackgroundRelayPublishesRecord) {
    rocksdb::WriteOptions wo;
    auto* txn = raw_db_->BeginTransaction(wo);
    OutboxRecord rec = makeRecord("bg:1");
    writer_->writeToOutbox(txn, rec);
    txn->Commit();
    delete txn;

    relay_->start();
    // Give background thread time to relay (poll_interval = 50 ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    relay_->stop();

    auto published = relay_->listRecords(OutboxState::PUBLISHED);
    EXPECT_GE(published.size(), 1u);
}

// ===== JSON round-trip =====

TEST_F(OutboxTest, OutboxRecordJsonRoundTrip) {
    OutboxRecord r;
    r.outbox_sequence  = 42;
    r.collection       = "orders";
    r.key              = "orders:99";
    r.value            = R"({"amount":100})";
    r.event_type       = Changefeed::ChangeEventType::EVENT_PUT;
    r.state            = OutboxState::PUBLISHED;
    r.created_at_ms    = 1700000000000LL;
    r.published_at_ms  = 1700000001000LL;
    r.relay_attempts   = 1;
    r.failure_reason   = "";
    r.metadata         = {{"tx_id", "txn-123"}};

    nlohmann::json j = r.toJson();
    OutboxRecord r2  = OutboxRecord::fromJson(j);

    EXPECT_EQ(r2.outbox_sequence, r.outbox_sequence);
    EXPECT_EQ(r2.collection,      r.collection);
    EXPECT_EQ(r2.key,             r.key);
    ASSERT_TRUE(r2.value.has_value());
    EXPECT_EQ(*r2.value,          *r.value);
    EXPECT_EQ(r2.event_type,      r.event_type);
    EXPECT_EQ(r2.state,           r.state);
    EXPECT_EQ(r2.created_at_ms,   r.created_at_ms);
    EXPECT_EQ(r2.published_at_ms, r.published_at_ms);
    EXPECT_EQ(r2.relay_attempts,  r.relay_attempts);
}

TEST_F(OutboxTest, OutboxRecordJsonDeleteEventNullValue) {
    OutboxRecord r;
    r.outbox_sequence = 7;
    r.key             = "orders:del";
    r.event_type      = Changefeed::ChangeEventType::EVENT_DELETE;
    // value is nullopt

    nlohmann::json j = r.toJson();
    OutboxRecord r2  = OutboxRecord::fromJson(j);

    EXPECT_FALSE(r2.value.has_value());
    EXPECT_EQ(r2.event_type, Changefeed::ChangeEventType::EVENT_DELETE);
}

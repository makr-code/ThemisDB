// Test: CDC Dead-Letter Queue
// Tests for DeadLetterQueue: enqueue, list, replay, remove, drain

#include <gtest/gtest.h>
#include "cdc/dead_letter_queue.h"
#include "cdc/changefeed.h"
#include "cdc/changefeed_buffer.h"
#include "cdc/cdc_error.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <stdexcept>

using namespace themis;
using namespace themis::cdc;
namespace fs = std::filesystem;

// ===== Test Fixture =====

class DeadLetterQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC dead-letter-queue focused tests on Windows due to fixture crash in current runtime.";
#endif
        test_db_path_ = "/tmp/test_dlq_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        cfg.memtable_size_mb  = 16;
        cfg.block_cache_size_mb = 32;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        auto* raw = db_->getDB();
        ASSERT_NE(raw, nullptr);

        changefeed_ = std::make_unique<Changefeed>(raw, nullptr);
        dlq_        = std::make_unique<DeadLetterQueue>(raw, nullptr);
    }

    void TearDown() override {
        dlq_.reset();
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }
    }

    Changefeed::ChangeEvent makeEvent(const std::string& key,
                                      const std::string& value = "v") {
        Changefeed::ChangeEvent e;
        e.sequence    = 0;
        e.type        = Changefeed::ChangeEventType::EVENT_PUT;
        e.key         = key;
        e.value       = value;
        e.timestamp_ms = 0;
        return e;
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper>   db_;
    std::unique_ptr<Changefeed>       changefeed_;
    std::unique_ptr<DeadLetterQueue>  dlq_;
};

// ===== Enqueue Tests =====

TEST_F(DeadLetterQueueTest, EnqueueAssignsDlqSequence) {
    auto event = makeEvent("table:1");
    DLQEntry entry = dlq_->enqueue(event, "test error", 3);

    EXPECT_GE(entry.dlq_sequence, uint64_t(1));
    EXPECT_EQ(entry.event.key, "table:1");
    EXPECT_EQ(entry.failure_reason, "test error");
    EXPECT_EQ(entry.attempt_count, 3);
    EXPECT_GT(entry.enqueued_at_ms, int64_t(0));
}

TEST_F(DeadLetterQueueTest, EnqueueMultipleIncreasesSequence) {
    auto e1 = dlq_->enqueue(makeEvent("k1"), "err1", 1);
    auto e2 = dlq_->enqueue(makeEvent("k2"), "err2", 2);
    auto e3 = dlq_->enqueue(makeEvent("k3"), "err3", 3);

    EXPECT_LT(e1.dlq_sequence, e2.dlq_sequence);
    EXPECT_LT(e2.dlq_sequence, e3.dlq_sequence);
}

// ===== List / Size Tests =====

TEST_F(DeadLetterQueueTest, SizeReflectsEnqueueCount) {
    EXPECT_EQ(dlq_->size(), size_t(0));

    dlq_->enqueue(makeEvent("k1"), "e", 1);
    EXPECT_EQ(dlq_->size(), size_t(1));

    dlq_->enqueue(makeEvent("k2"), "e", 1);
    EXPECT_EQ(dlq_->size(), size_t(2));
}

TEST_F(DeadLetterQueueTest, ListEntriesReturnsAll) {
    dlq_->enqueue(makeEvent("a"), "err-a", 1);
    dlq_->enqueue(makeEvent("b"), "err-b", 2);
    dlq_->enqueue(makeEvent("c"), "err-c", 3);

    auto entries = dlq_->listEntries();
    ASSERT_EQ(entries.size(), size_t(3));

    // Verify oldest-first ordering
    EXPECT_LT(entries[0].dlq_sequence, entries[1].dlq_sequence);
    EXPECT_LT(entries[1].dlq_sequence, entries[2].dlq_sequence);
}

TEST_F(DeadLetterQueueTest, ListEntriesRespectsLimit) {
    for (int i = 0; i < 5; ++i) {
        dlq_->enqueue(makeEvent("k" + std::to_string(i)), "e", 1);
    }

    auto entries = dlq_->listEntries(3);
    EXPECT_EQ(entries.size(), size_t(3));
}

// ===== GetEntry Tests =====

TEST_F(DeadLetterQueueTest, GetEntryRoundTrip) {
    auto orig  = dlq_->enqueue(makeEvent("orders:99", "{\"qty\":5}"), "db write failed", 4);
    auto found = dlq_->getEntry(orig.dlq_sequence);

    EXPECT_EQ(found.dlq_sequence,   orig.dlq_sequence);
    EXPECT_EQ(found.event.key,      "orders:99");
    EXPECT_EQ(found.failure_reason, "db write failed");
    EXPECT_EQ(found.attempt_count,  4);
    ASSERT_TRUE(found.event.value.has_value());
    EXPECT_EQ(*found.event.value,   "{\"qty\":5}");
}

TEST_F(DeadLetterQueueTest, GetEntryThrowsOnMissingSequence) {
    EXPECT_THROW(dlq_->getEntry(99999), CDCException);
}

// ===== Remove Tests =====

TEST_F(DeadLetterQueueTest, RemoveDecreasesSize) {
    auto entry = dlq_->enqueue(makeEvent("k1"), "e", 1);
    EXPECT_EQ(dlq_->size(), size_t(1));

    EXPECT_TRUE(dlq_->remove(entry.dlq_sequence));
    EXPECT_EQ(dlq_->size(), size_t(0));
}

TEST_F(DeadLetterQueueTest, RemoveReturnsFalseForMissing) {
    EXPECT_FALSE(dlq_->remove(99999));
}

// ===== Drain Tests =====

TEST_F(DeadLetterQueueTest, DrainRemovesAllEntries) {
    dlq_->enqueue(makeEvent("a"), "e", 1);
    dlq_->enqueue(makeEvent("b"), "e", 1);
    dlq_->enqueue(makeEvent("c"), "e", 1);

    size_t deleted = dlq_->drain();
    EXPECT_EQ(deleted, size_t(3));
    EXPECT_EQ(dlq_->size(), size_t(0));
}

TEST_F(DeadLetterQueueTest, DrainOnEmptyQueueReturnsZero) {
    EXPECT_EQ(dlq_->drain(), size_t(0));
}

// ===== Replay Tests =====

TEST_F(DeadLetterQueueTest, ReplayRerecordsEventAndRemovesEntry) {
    auto event = makeEvent("replay-key", "replay-value");
    auto entry = dlq_->enqueue(event, "simulated failure", 3);

    uint64_t initial_changefeed_seq = changefeed_->getLatestSequence();

    Changefeed::ChangeEvent recorded = dlq_->replay(entry.dlq_sequence, *changefeed_);

    // New sequence should be > previous latest
    EXPECT_GT(recorded.sequence, initial_changefeed_seq);
    EXPECT_EQ(recorded.key, "replay-key");
    ASSERT_TRUE(recorded.value.has_value());
    EXPECT_EQ(*recorded.value, "replay-value");

    // Entry should have been removed from DLQ
    EXPECT_EQ(dlq_->size(), size_t(0));
}

TEST_F(DeadLetterQueueTest, ReplayThrowsOnMissingEntry) {
    EXPECT_THROW(dlq_->replay(99999, *changefeed_), CDCException);
}

// ===== DLQEntry JSON Serialization =====

TEST_F(DeadLetterQueueTest, DlqEntryRoundTripJson) {
    DLQEntry orig;
    orig.dlq_sequence   = 42;
    orig.event          = makeEvent("json-key", "json-value");
    orig.failure_reason = "round-trip test";
    orig.attempt_count  = 7;
    orig.enqueued_at_ms = 1700000000000LL;

    nlohmann::json j = orig.toJson();
    DLQEntry parsed  = DLQEntry::fromJson(j);

    EXPECT_EQ(parsed.dlq_sequence,   orig.dlq_sequence);
    EXPECT_EQ(parsed.event.key,      orig.event.key);
    EXPECT_EQ(parsed.failure_reason, orig.failure_reason);
    EXPECT_EQ(parsed.attempt_count,  orig.attempt_count);
    EXPECT_EQ(parsed.enqueued_at_ms, orig.enqueued_at_ms);
}

// ===== ChangefeedBuffer Integration =====

class DLQBufferIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "/tmp/test_dlq_buf_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        cfg.memtable_size_mb   = 16;
        cfg.block_cache_size_mb = 32;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        auto* raw = db_->getDB();

        changefeed_ = std::make_unique<Changefeed>(raw, nullptr);
        dlq_        = std::make_unique<DeadLetterQueue>(raw, nullptr);
    }

    void TearDown() override {
        dlq_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper>  db_;
    std::unique_ptr<Changefeed>      changefeed_;
    std::unique_ptr<DeadLetterQueue> dlq_;
};

TEST_F(DLQBufferIntegrationTest, GetAndSetDeadLetterQueue) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush        = false;
    cfg.max_retry_attempts = 0;

    ChangefeedBuffer buffer(changefeed_.get(), cfg);

    EXPECT_EQ(buffer.getDeadLetterQueue(), nullptr);

    buffer.setDeadLetterQueue(dlq_.get());
    EXPECT_EQ(buffer.getDeadLetterQueue(), dlq_.get());

    buffer.setDeadLetterQueue(nullptr);
    EXPECT_EQ(buffer.getDeadLetterQueue(), nullptr);
}

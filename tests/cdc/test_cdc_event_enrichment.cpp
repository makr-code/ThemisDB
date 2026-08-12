/*
 * Tests for CDC change event enrichment (before/after document snapshots).
 *
 * Validates that:
 * 1. ChangeEvent.before_snapshot / after_snapshot are serialized and deserialized
 *    correctly via toJson() / fromJson().
 * 2. INSERT events carry no before_snapshot and carry an after_snapshot.
 * 3. UPDATE events carry both before_snapshot and after_snapshot.
 * 4. DELETE events carry a before_snapshot and no after_snapshot.
 */

#include <gtest/gtest.h>
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>

using namespace themis;

class CDCEventEnrichmentTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC event-enrichment focused tests on Windows due to fixture crash in current runtime.";
#endif
        test_db_path_ = "./data/themis_cdc_enrichment_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* txn_db = db_->getRawDB();
        ASSERT_NE(txn_db, nullptr);

        changefeed_ = std::make_unique<Changefeed>(txn_db);
    }

    void TearDown() override {
        changefeed_.reset();
        db_.reset();

        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    // Helper: record a PUT event with optional before/after snapshots
    Changefeed::ChangeEvent recordPutWithSnapshots(
        const std::string& key,
        const std::string& value,
        std::optional<std::string> before_snap = std::nullopt,
        std::optional<std::string> after_snap = std::nullopt)
    {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = key;
        event.value = value;
        event.before_snapshot = std::move(before_snap);
        event.after_snapshot = std::move(after_snap);
        auto now = std::chrono::system_clock::now();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return changefeed_->recordEvent(event);
    }

    // Helper: record a DELETE event with optional before snapshot
    Changefeed::ChangeEvent recordDeleteWithSnapshot(
        const std::string& key,
        std::optional<std::string> before_snap = std::nullopt)
    {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_DELETE;
        event.key = key;
        event.before_snapshot = std::move(before_snap);
        auto now = std::chrono::system_clock::now();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        return changefeed_->recordEvent(event);
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
};

// ---------------------------------------------------------------------------
// Serialization round-trip tests
// ---------------------------------------------------------------------------

TEST_F(CDCEventEnrichmentTest, InsertEvent_NoBeforeSnapshot_HasAfterSnapshot) {
    const std::string after_val = R"({"name":"Alice","age":30})";
    auto recorded = recordPutWithSnapshots("users:alice", after_val,
                                           std::nullopt, after_val);

    EXPECT_FALSE(recorded.before_snapshot.has_value())
        << "INSERT should have no before_snapshot";
    ASSERT_TRUE(recorded.after_snapshot.has_value())
        << "INSERT should have an after_snapshot";
    EXPECT_EQ(*recorded.after_snapshot, after_val);
}

TEST_F(CDCEventEnrichmentTest, UpdateEvent_HasBothSnapshots) {
    const std::string before_val = R"({"name":"Alice","age":30})";
    const std::string after_val  = R"({"name":"Alice","age":31})";
    auto recorded = recordPutWithSnapshots("users:alice", after_val,
                                           before_val, after_val);

    ASSERT_TRUE(recorded.before_snapshot.has_value())
        << "UPDATE should have a before_snapshot";
    ASSERT_TRUE(recorded.after_snapshot.has_value())
        << "UPDATE should have an after_snapshot";
    EXPECT_EQ(*recorded.before_snapshot, before_val);
    EXPECT_EQ(*recorded.after_snapshot,  after_val);
}

TEST_F(CDCEventEnrichmentTest, DeleteEvent_HasBeforeSnapshot_NoAfterSnapshot) {
    const std::string before_val = R"({"name":"Alice","age":31})";
    auto recorded = recordDeleteWithSnapshot("users:alice", before_val);

    ASSERT_TRUE(recorded.before_snapshot.has_value())
        << "DELETE should have a before_snapshot";
    EXPECT_EQ(*recorded.before_snapshot, before_val);
    EXPECT_FALSE(recorded.after_snapshot.has_value())
        << "DELETE should have no after_snapshot";
}

// ---------------------------------------------------------------------------
// toJson() / fromJson() round-trip
// ---------------------------------------------------------------------------

TEST_F(CDCEventEnrichmentTest, ToJson_ContainsSnapshotFields) {
    const std::string before_val = R"({"x":1})";
    const std::string after_val  = R"({"x":2})";

    Changefeed::ChangeEvent event;
    event.type           = Changefeed::ChangeEventType::EVENT_PUT;
    event.key            = "col:key1";
    event.value          = after_val;
    event.before_snapshot = before_val;
    event.after_snapshot  = after_val;
    event.timestamp_ms   = 1000;

    auto j = event.toJson();

    ASSERT_TRUE(j.contains("before_snapshot"));
    ASSERT_TRUE(j.contains("after_snapshot"));
    EXPECT_EQ(j["before_snapshot"].get<std::string>(), before_val);
    EXPECT_EQ(j["after_snapshot"].get<std::string>(),  after_val);
}

TEST_F(CDCEventEnrichmentTest, ToJson_OmitsAbsentSnapshotFields) {
    Changefeed::ChangeEvent event;
    event.type        = Changefeed::ChangeEventType::EVENT_PUT;
    event.key         = "col:key1";
    event.value       = R"({"x":1})";
    event.timestamp_ms = 1000;
    // no before/after set

    auto j = event.toJson();

    EXPECT_FALSE(j.contains("before_snapshot"))
        << "before_snapshot should not appear when not set";
    EXPECT_FALSE(j.contains("after_snapshot"))
        << "after_snapshot should not appear when not set";
}

TEST_F(CDCEventEnrichmentTest, FromJson_RoundTrip_WithSnapshots) {
    const std::string before_val = R"({"status":"pending"})";
    const std::string after_val  = R"({"status":"active"})";

    Changefeed::ChangeEvent original;
    original.type            = Changefeed::ChangeEventType::EVENT_PUT;
    original.key             = "orders:42";
    original.value           = after_val;
    original.before_snapshot = before_val;
    original.after_snapshot  = after_val;
    original.timestamp_ms    = 5000;

    auto j       = original.toJson();
    auto decoded = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_EQ(decoded.key,        original.key);
    EXPECT_EQ(decoded.timestamp_ms, original.timestamp_ms);
    ASSERT_TRUE(decoded.before_snapshot.has_value());
    ASSERT_TRUE(decoded.after_snapshot.has_value());
    EXPECT_EQ(*decoded.before_snapshot, before_val);
    EXPECT_EQ(*decoded.after_snapshot,  after_val);
}

TEST_F(CDCEventEnrichmentTest, FromJson_RoundTrip_WithoutSnapshots) {
    Changefeed::ChangeEvent original;
    original.type         = Changefeed::ChangeEventType::EVENT_DELETE;
    original.key          = "orders:42";
    original.timestamp_ms = 6000;

    auto j       = original.toJson();
    auto decoded = Changefeed::ChangeEvent::fromJson(j);

    EXPECT_FALSE(decoded.before_snapshot.has_value());
    EXPECT_FALSE(decoded.after_snapshot.has_value());
}

// ---------------------------------------------------------------------------
// Persistent storage round-trip (write + read from changefeed)
// ---------------------------------------------------------------------------

TEST_F(CDCEventEnrichmentTest, PersistAndRetrieve_SnapshotsPreserved) {
    const std::string before_val = R"({"v":1})";
    const std::string after_val  = R"({"v":2})";

    auto recorded = recordPutWithSnapshots("items:100", after_val,
                                           before_val, after_val);
    uint64_t seq = recorded.sequence;

    // Retrieve via listEvents
    Changefeed::ListOptions opts;
    opts.from_sequence = seq - 1;
    opts.limit         = 10;
    auto events = changefeed_->listEvents(opts);

    ASSERT_FALSE(events.empty());
    auto& ev = events.front();
    EXPECT_EQ(ev.sequence, seq);
    ASSERT_TRUE(ev.before_snapshot.has_value());
    ASSERT_TRUE(ev.after_snapshot.has_value());
    EXPECT_EQ(*ev.before_snapshot, before_val);
    EXPECT_EQ(*ev.after_snapshot,  after_val);
}

TEST_F(CDCEventEnrichmentTest, PersistAndRetrieve_DeleteSnapshotPreserved) {
    const std::string before_val = R"({"deleted":true})";
    auto recorded = recordDeleteWithSnapshot("items:101", before_val);
    uint64_t seq = recorded.sequence;

    Changefeed::ListOptions opts;
    opts.from_sequence = seq - 1;
    opts.limit         = 10;
    auto events = changefeed_->listEvents(opts);

    ASSERT_FALSE(events.empty());
    auto& ev = events.front();
    EXPECT_EQ(ev.sequence, seq);
    ASSERT_TRUE(ev.before_snapshot.has_value());
    EXPECT_EQ(*ev.before_snapshot, before_val);
    EXPECT_FALSE(ev.after_snapshot.has_value());
}

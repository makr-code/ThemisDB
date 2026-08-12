// Test: CDC Operation Type Filtering
// Validates that listEvents() correctly filters by single and multiple ChangeEventType values
// using the event_types field in ListOptions (INSERT/UPDATE=PUT, DELETE).

#include <gtest/gtest.h>
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <set>
#include <thread>

using namespace themis;

class CDCOperationFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC operation-filter focused tests on Windows due to fixture crash in current runtime.";
#endif
        // Use thread ID + timestamp for a more collision-resistant unique path
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        test_db_path_ = "./data/themis_cdc_op_filter_" + std::to_string(tid) +
                        "_" + std::to_string(time(nullptr));
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;

        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* raw_db = db_->getDB();
        ASSERT_NE(raw_db, nullptr);
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
    }

    void TearDown() override {
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    Changefeed::ChangeEvent makeEvent(Changefeed::ChangeEventType type, const std::string& key) {
        Changefeed::ChangeEvent ev;
        ev.type = type;
        ev.key = key;
        if (type != Changefeed::ChangeEventType::EVENT_DELETE) {
            ev.value = "{\"k\":\"" + key + "\"}";
        }
        return ev;
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
};

// Filter to only PUT events using event_types (multi-type)
TEST_F(CDCOperationFilterTest, FilterByPutOperationType) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:2"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:3"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT, "tx:1"));

    Changefeed::ListOptions opts;
    opts.event_types = {Changefeed::ChangeEventType::EVENT_PUT};

    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 2u);
    for (const auto& ev : events) {
        EXPECT_EQ(ev.type, Changefeed::ChangeEventType::EVENT_PUT);
    }
}

// Filter to only DELETE events using event_types
TEST_F(CDCOperationFilterTest, FilterByDeleteOperationType) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:2"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:3"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:4"));

    Changefeed::ListOptions opts;
    opts.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};

    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 2u);
    for (const auto& ev : events) {
        EXPECT_EQ(ev.type, Changefeed::ChangeEventType::EVENT_DELETE);
    }
}

// Filter by multiple operation types (PUT + DELETE)
TEST_F(CDCOperationFilterTest, FilterByMultipleOperationTypes) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:2"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT, "tx:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK, "tx:2"));

    Changefeed::ListOptions opts;
    opts.event_types = {
        Changefeed::ChangeEventType::EVENT_PUT,
        Changefeed::ChangeEventType::EVENT_DELETE
    };

    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 2u);
    for (const auto& ev : events) {
        EXPECT_TRUE(ev.type == Changefeed::ChangeEventType::EVENT_PUT ||
                    ev.type == Changefeed::ChangeEventType::EVENT_DELETE);
    }
}

// Empty event_types set returns all events (no filter applied)
TEST_F(CDCOperationFilterTest, EmptyEventTypesReturnsAll) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:2"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT, "tx:1"));

    Changefeed::ListOptions opts;
    // event_types is empty → no filter
    auto events = changefeed_->listEvents(opts);
    EXPECT_EQ(events.size(), 3u);
}

// event_types takes precedence over legacy event_type (singular)
TEST_F(CDCOperationFilterTest, EventTypesPrecedenceOverLegacySingular) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:2"));

    Changefeed::ListOptions opts;
    // Set both: event_types (multi) should win
    opts.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};
    opts.event_type = Changefeed::ChangeEventType::EVENT_PUT; // should be ignored

    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, Changefeed::ChangeEventType::EVENT_DELETE);
}

// Legacy event_type (singular) still works when event_types is empty
TEST_F(CDCOperationFilterTest, LegacySingleEventTypeStillWorks) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "doc:2"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:3"));

    Changefeed::ListOptions opts;
    opts.event_type = Changefeed::ChangeEventType::EVENT_PUT; // legacy filter

    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 2u);
    for (const auto& ev : events) {
        EXPECT_EQ(ev.type, Changefeed::ChangeEventType::EVENT_PUT);
    }
}

// Combine event_types with key_prefix filter
TEST_F(CDCOperationFilterTest, CombineEventTypesWithKeyPrefix) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "orders:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "orders:2"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "users:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_DELETE, "users:2"));

    Changefeed::ListOptions opts;
    opts.key_prefix = "orders:";
    opts.event_types = {Changefeed::ChangeEventType::EVENT_PUT};

    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].key, "orders:1");
    EXPECT_EQ(events[0].type, Changefeed::ChangeEventType::EVENT_PUT);
}

// Filter returns no events when no match
TEST_F(CDCOperationFilterTest, FilterReturnsEmptyWhenNoMatch) {
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:1"));
    changefeed_->recordEvent(makeEvent(Changefeed::ChangeEventType::EVENT_PUT, "doc:2"));

    Changefeed::ListOptions opts;
    opts.event_types = {Changefeed::ChangeEventType::EVENT_DELETE};

    auto events = changefeed_->listEvents(opts);
    EXPECT_TRUE(events.empty());
}

// Test: CDC Cross-Collection Change Aggregation Stream
// Tests for CrossCollectionStream: registration, merged listing,
// filtering, cursor tracking, high-watermark, and error handling.

#include <gtest/gtest.h>
#include "cdc/cross_collection_stream.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace themis;
using namespace themis::cdc;

// ============================================================
// Test fixture: each collection gets its own RocksDB database so
// that the changefeed sequence counter and event key-space are
// fully isolated.  Sharing a single database would cause all
// feeds to read each other's events (they all scan the same
// "changefeed:..." key prefix) and share one sequence counter,
// making cross-collection tests non-deterministic.
// ============================================================

class CrossCollectionStreamTest : public ::testing::Test {
protected:
    // Open a fresh RocksDB database at the given path.
    static std::unique_ptr<RocksDBWrapper> openDB(const std::string& path) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
        }
        RocksDBWrapper::Config cfg;
        cfg.db_path             = path;
        cfg.memtable_size_mb    = 64;
        cfg.block_cache_size_mb = 128;
        auto db = std::make_unique<RocksDBWrapper>(cfg);
        if (!db->open()) {
            throw std::runtime_error("Failed to open test DB at " + path);
        }
        return db;
    }

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC cross-collection focused tests on Windows due to fixture crash in current runtime.";
#endif
        orders_db_    = openDB("./data/themis_cdc_xcs_orders");
        inventory_db_ = openDB("./data/themis_cdc_xcs_inventory");
        users_db_     = openDB("./data/themis_cdc_xcs_users");

        ASSERT_NE(orders_db_->getDB(),    nullptr);
        ASSERT_NE(inventory_db_->getDB(), nullptr);
        ASSERT_NE(users_db_->getDB(),     nullptr);

        Changefeed::RetentionPolicy ret;
        ret.enabled = false;

        orders_feed_    = std::make_unique<Changefeed>(orders_db_->getDB(),    nullptr, ret);
        inventory_feed_ = std::make_unique<Changefeed>(inventory_db_->getDB(), nullptr, ret);
        users_feed_     = std::make_unique<Changefeed>(users_db_->getDB(),     nullptr, ret);
    }

    void TearDown() override {
        stream_.reset();
        orders_feed_.reset();
        inventory_feed_.reset();
        users_feed_.reset();
        if (orders_db_) {
            orders_db_->close();
        }
        if (inventory_db_) {
            inventory_db_->close();
        }
        if (users_db_) {
            users_db_->close();
        }
        orders_db_.reset();
        inventory_db_.reset();
        users_db_.reset();
        for (const auto& p : {"./data/themis_cdc_xcs_orders",
                               "./data/themis_cdc_xcs_inventory",
                               "./data/themis_cdc_xcs_users"}) {
            if (std::filesystem::exists(p)) {
              std::filesystem::remove_all(p);
            }
        }
    }

    // Helper: build a ChangeEvent with explicit fields.
    static Changefeed::ChangeEvent makeEvent(
        const std::string& key,
        Changefeed::ChangeEventType type = Changefeed::ChangeEventType::EVENT_PUT,
        const std::string& value = "v",
        int64_t timestamp_ms = 0) {
        Changefeed::ChangeEvent ev;
        ev.key          = key;
        ev.type         = type;
        ev.value        = value;
        ev.timestamp_ms = timestamp_ms;
        return ev;
    }

    std::unique_ptr<RocksDBWrapper> orders_db_;
    std::unique_ptr<RocksDBWrapper> inventory_db_;
    std::unique_ptr<RocksDBWrapper> users_db_;

    std::unique_ptr<Changefeed>     orders_feed_;
    std::unique_ptr<Changefeed>     inventory_feed_;
    std::unique_ptr<Changefeed>     users_feed_;

    std::unique_ptr<CrossCollectionStream> stream_ =
        std::make_unique<CrossCollectionStream>();
};

// ============================================================
// Registration tests
// ============================================================

TEST_F(CrossCollectionStreamTest, AddAndQueryCollections) {
    EXPECT_EQ(stream_->collectionCount(), 0u);
    EXPECT_FALSE(stream_->hasCollection("orders"));

    stream_->addCollection("orders", orders_feed_.get());
    EXPECT_EQ(stream_->collectionCount(), 1u);
    EXPECT_TRUE(stream_->hasCollection("orders"));

    stream_->addCollection("inventory", inventory_feed_.get());
    EXPECT_EQ(stream_->collectionCount(), 2u);

    auto names = stream_->listCollections();
    std::unordered_set<std::string> name_set(names.begin(), names.end());
    EXPECT_TRUE(name_set.count("orders"));
    EXPECT_TRUE(name_set.count("inventory"));
}

TEST_F(CrossCollectionStreamTest, RemoveCollection) {
    stream_->addCollection("orders", orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    stream_->removeCollection("orders");
    EXPECT_FALSE(stream_->hasCollection("orders"));
    EXPECT_EQ(stream_->collectionCount(), 1u);

    // Removing non-existent collection is a no-op
    stream_->removeCollection("nonexistent");
    EXPECT_EQ(stream_->collectionCount(), 1u);
}

TEST_F(CrossCollectionStreamTest, ReplaceExistingCollection) {
    stream_->addCollection("orders", orders_feed_.get());
    stream_->addCollection("orders", inventory_feed_.get()); // replace
    EXPECT_EQ(stream_->collectionCount(), 1u);
    EXPECT_TRUE(stream_->hasCollection("orders"));
}

TEST_F(CrossCollectionStreamTest, AddCollectionRejectsEmptyName) {
    EXPECT_THROW(stream_->addCollection("", orders_feed_.get()),
                 std::invalid_argument);
}

TEST_F(CrossCollectionStreamTest, AddCollectionRejectsNullFeed) {
    EXPECT_THROW(stream_->addCollection("orders", nullptr),
                 std::invalid_argument);
}

// ============================================================
// Merged listing tests
// ============================================================

TEST_F(CrossCollectionStreamTest, ListEventsEmptyStream) {
    auto events = stream_->listEvents();
    EXPECT_TRUE(events.empty());
}

TEST_F(CrossCollectionStreamTest, ListEventsFromSingleCollection) {
    stream_->addCollection("orders", orders_feed_.get());

    orders_feed_->recordEvent(makeEvent("orders:1", Changefeed::ChangeEventType::EVENT_PUT, "a", 1000));
    orders_feed_->recordEvent(makeEvent("orders:2", Changefeed::ChangeEventType::EVENT_PUT, "b", 2000));

    auto events = stream_->listEvents();
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].collection, "orders");
    EXPECT_EQ(events[1].collection, "orders");
    EXPECT_EQ(events[0].event.key, "orders:1");
    EXPECT_EQ(events[1].event.key, "orders:2");
}

TEST_F(CrossCollectionStreamTest, ListEventsMergedAndSortedByTimestamp) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    // Interleave timestamps across collections
    orders_feed_->recordEvent(    makeEvent("orders:1",    Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));
    inventory_feed_->recordEvent( makeEvent("inventory:1", Changefeed::ChangeEventType::EVENT_PUT, "v", 500));
    orders_feed_->recordEvent(    makeEvent("orders:2",    Changefeed::ChangeEventType::EVENT_PUT, "v", 3000));
    inventory_feed_->recordEvent( makeEvent("inventory:2", Changefeed::ChangeEventType::EVENT_PUT, "v", 2000));

    auto events = stream_->listEvents();
    ASSERT_EQ(events.size(), 4u);
    EXPECT_EQ(events[0].event.timestamp_ms, 500);
    EXPECT_EQ(events[1].event.timestamp_ms, 1000);
    EXPECT_EQ(events[2].event.timestamp_ms, 2000);
    EXPECT_EQ(events[3].event.timestamp_ms, 3000);
}

TEST_F(CrossCollectionStreamTest, ListEventsAggregatedEventCarriesCollectionName) {
    stream_->addCollection("users", users_feed_.get());
    users_feed_->recordEvent(makeEvent("user:42"));

    auto events = stream_->listEvents();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].collection, "users");
    EXPECT_EQ(events[0].event.key, "user:42");
}

// ============================================================
// Filter tests
// ============================================================

TEST_F(CrossCollectionStreamTest, FilterByKeyPrefix) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    orders_feed_->recordEvent(    makeEvent("orders:US-1", Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));
    orders_feed_->recordEvent(    makeEvent("orders:EU-2", Changefeed::ChangeEventType::EVENT_PUT, "v", 2000));
    inventory_feed_->recordEvent( makeEvent("inventory:US-1", Changefeed::ChangeEventType::EVENT_PUT, "v", 1500));

    CrossCollectionStream::StreamOptions opts;
    opts.key_prefix = "orders:US-";
    auto events = stream_->listEvents(opts);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].event.key, "orders:US-1");
}

TEST_F(CrossCollectionStreamTest, FilterByEventType) {
    stream_->addCollection("orders", orders_feed_.get());

    orders_feed_->recordEvent(makeEvent("orders:1", Changefeed::ChangeEventType::EVENT_PUT,    "v", 1000));
    orders_feed_->recordEvent(makeEvent("orders:2", Changefeed::ChangeEventType::EVENT_DELETE, {}, 2000));

    CrossCollectionStream::StreamOptions opts;
    opts.event_types = { Changefeed::ChangeEventType::EVENT_DELETE };
    auto events = stream_->listEvents(opts);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].event.type, Changefeed::ChangeEventType::EVENT_DELETE);
    EXPECT_EQ(events[0].event.key, "orders:2");
}

TEST_F(CrossCollectionStreamTest, FilterByCollectionSubset) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());
    stream_->addCollection("users",     users_feed_.get());

    orders_feed_->recordEvent(    makeEvent("orders:1",    Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));
    inventory_feed_->recordEvent( makeEvent("inventory:1", Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));
    users_feed_->recordEvent(     makeEvent("user:1",      Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));

    auto events = stream_->listEventsFor({"orders", "users"});
    ASSERT_EQ(events.size(), 2u);

    std::unordered_set<std::string> col_set = {};

    for (const auto& e : events) {
      col_set.insert(e.collection);
    }
    EXPECT_TRUE(col_set.count("orders"));
    EXPECT_TRUE(col_set.count("users"));
    EXPECT_FALSE(col_set.count("inventory"));
}

// ============================================================
// Cursor / resume tests
// ============================================================

TEST_F(CrossCollectionStreamTest, FromSequenceCursorResumesCorrectly) {
    stream_->addCollection("orders", orders_feed_.get());

    auto ev1 = orders_feed_->recordEvent(makeEvent("orders:1", Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));
    auto ev2 = orders_feed_->recordEvent(makeEvent("orders:2", Changefeed::ChangeEventType::EVENT_PUT, "v", 2000));
    auto ev3 = orders_feed_->recordEvent(makeEvent("orders:3", Changefeed::ChangeEventType::EVENT_PUT, "v", 3000));

    CrossCollectionStream::StreamOptions opts;
    opts.from_sequence["orders"] = ev1.sequence; // start after first event

    auto events = stream_->listEvents(opts);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].event.sequence, ev2.sequence);
    EXPECT_EQ(events[1].event.sequence, ev3.sequence);
}

TEST_F(CrossCollectionStreamTest, IndependentCursorsPerCollection) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    auto o1 = orders_feed_->recordEvent(    makeEvent("orders:1",    Changefeed::ChangeEventType::EVENT_PUT, "v", 1000));
    auto o2 = orders_feed_->recordEvent(    makeEvent("orders:2",    Changefeed::ChangeEventType::EVENT_PUT, "v", 2000));
    auto i1 = inventory_feed_->recordEvent( makeEvent("inventory:1", Changefeed::ChangeEventType::EVENT_PUT, "v", 1500));
    (void)inventory_feed_->recordEvent(     makeEvent("inventory:2", Changefeed::ChangeEventType::EVENT_PUT, "v", 2500));

    CrossCollectionStream::StreamOptions opts;
    opts.from_sequence["orders"]    = o1.sequence; // skip first orders event
    opts.from_sequence["inventory"] = i1.sequence; // skip first inventory event

    auto events = stream_->listEvents(opts);
    ASSERT_EQ(events.size(), 2u);
    // Sorted by timestamp: orders:2 @2000, inventory:2 @2500
    EXPECT_EQ(events[0].event.sequence, o2.sequence);
    EXPECT_EQ(events[1].event.key, "inventory:2");
}

// ============================================================
// Limit tests
// ============================================================

TEST_F(CrossCollectionStreamTest, LimitCapsReturnedEvents) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    for (int i = 0; i < 10; ++i) {
        orders_feed_->recordEvent(
            makeEvent("orders:" + std::to_string(i), Changefeed::ChangeEventType::EVENT_PUT, "v",
                      static_cast<int64_t>(i * 100)));
        inventory_feed_->recordEvent(
            makeEvent("inventory:" + std::to_string(i), Changefeed::ChangeEventType::EVENT_PUT, "v",
                      static_cast<int64_t>(i * 100 + 50)));
    }

    CrossCollectionStream::StreamOptions opts;
    opts.limit = 5;
    auto events = stream_->listEvents(opts);
    EXPECT_EQ(events.size(), 5u);
}

// ============================================================
// High-watermark tests
// ============================================================

TEST_F(CrossCollectionStreamTest, HighWatermarkAllCollections) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    EXPECT_EQ(stream_->getHighWatermark(), 0u);

    auto ev = orders_feed_->recordEvent(makeEvent("orders:1"));
    EXPECT_GE(stream_->getHighWatermark(), ev.sequence);
}

TEST_F(CrossCollectionStreamTest, HighWatermarkSubsetOfCollections) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    // Only write to inventory
    auto inv_ev = inventory_feed_->recordEvent(makeEvent("inventory:1"));

    // Watermark restricted to orders (which has no events) should be 0
    uint64_t wm_orders = stream_->getHighWatermark({"orders"});
    EXPECT_EQ(wm_orders, 0u);

    // Watermark restricted to inventory should reflect the recorded event
    uint64_t wm_inventory = stream_->getHighWatermark({"inventory"});
    EXPECT_GE(wm_inventory, inv_ev.sequence);
}

// ============================================================
// AggregatedEvent JSON serialisation
// ============================================================

TEST_F(CrossCollectionStreamTest, AggregatedEventToJsonIncludesCollection) {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 42;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = "orders:1";
    ev.value        = "data";
    ev.timestamp_ms = 1000;

    AggregatedEvent ae{"orders", ev};
    auto j = ae.toJson();

    EXPECT_EQ(j["collection"].get<std::string>(), "orders");
    EXPECT_EQ(j["key"].get<std::string>(), "orders:1");
    EXPECT_EQ(j["sequence"].get<uint64_t>(), 42u);
}

// ============================================================
// listEventsFor convenience overload
// ============================================================

TEST_F(CrossCollectionStreamTest, ListEventsForEmptySetReturnsAll) {
    stream_->addCollection("orders",    orders_feed_.get());
    stream_->addCollection("inventory", inventory_feed_.get());

    orders_feed_->recordEvent(    makeEvent("orders:1",    Changefeed::ChangeEventType::EVENT_PUT, "v", 100));
    inventory_feed_->recordEvent( makeEvent("inventory:1", Changefeed::ChangeEventType::EVENT_PUT, "v", 200));

    // Empty collection_names set => all collections
    auto events = stream_->listEventsFor({});
    EXPECT_EQ(events.size(), 2u);
}

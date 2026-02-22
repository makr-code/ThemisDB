// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

// Tests: Real-time schema change notifications via changefeeds
// Validates that SchemaManager emits ChangeEvents to a registered Changefeed
// on setTableSchema / patchTableSchema / deleteTableSchema.

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <algorithm>

#include "metadata/schema_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;

// ============================================================================
// Test fixture
// ============================================================================

class SchemaChangefeedTest : public ::testing::Test {
protected:
    void SetUp() override {
        namespace fs = std::filesystem;
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string db_path = (fs::temp_directory_path() /
            ("test_schema_changefeed_" + std::to_string(now))).string();

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";

        index_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
        changefeed_ = std::make_unique<Changefeed>(db_->getRawDB(), nullptr);
    }

    void TearDown() override {
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
    }

    // Helper: build a minimal valid TableSchema
    static SchemaManager::TableSchema makeSchema(const std::string& name) {
        SchemaManager::TableSchema s;
        s.name = name;
        s.type = "relational";

        SchemaManager::PropertyInfo p;
        p.name = "id";
        p.type = "integer";
        p.nullable = false;
        s.properties.push_back(p);

        SchemaManager::IndexInfo idx;
        idx.name = "id";
        idx.type = "regular";
        idx.unique = true;
        idx.columns.push_back("id");
        s.indexes.push_back(idx);

        return s;
    }

    std::unique_ptr<RocksDBWrapper>       db_;
    std::unique_ptr<SecondaryIndexManager> index_mgr_;
    std::unique_ptr<Changefeed>           changefeed_;
};

// ============================================================================
// Tests
// ============================================================================

// Registering a changefeed does not break normal SchemaManager behaviour.
TEST_F(SchemaChangefeedTest, SetChangefeedDoesNotBreakNormalUsage) {
    SchemaManager mgr(*db_, index_mgr_.get());
    mgr.setChangefeed(changefeed_.get());

    auto schema = makeSchema("products");
    EXPECT_TRUE(mgr.setTableSchema("products", schema));
    EXPECT_TRUE(mgr.getTable("products").has_value());
}

// setTableSchema emits a schema_created event.
TEST_F(SchemaChangefeedTest, SetTableSchemaEmitsCreatedEvent) {
    SchemaManager mgr(*db_, index_mgr_.get());
    mgr.setChangefeed(changefeed_.get());

    uint64_t before = changefeed_->getLatestSequence();

    ASSERT_TRUE(mgr.setTableSchema("orders", makeSchema("orders")));

    auto events = changefeed_->listEvents();
    // At least one new event should have been added
    auto new_events_it = std::find_if(events.begin(), events.end(),
        [before](const Changefeed::ChangeEvent& e) {
            return e.sequence > before &&
                   e.key == "schema:orders" &&
                   e.type == Changefeed::ChangeEventType::EVENT_PUT;
        });
    ASSERT_NE(new_events_it, events.end())
        << "Expected a schema_created EVENT_PUT for 'orders'";

    EXPECT_EQ(new_events_it->metadata.value("event", ""), "schema_created");
    EXPECT_EQ(new_events_it->metadata.value("table", ""), "orders");
}

// patchTableSchema emits a schema_updated event.
TEST_F(SchemaChangefeedTest, PatchTableSchemaEmitsUpdatedEvent) {
    SchemaManager mgr(*db_, index_mgr_.get());
    mgr.setChangefeed(changefeed_.get());

    // First, create the schema so patch can find it
    ASSERT_TRUE(mgr.setTableSchema("users", makeSchema("users")));

    uint64_t before = changefeed_->getLatestSequence();

    nlohmann::json patch = {{"type", "document"}};
    ASSERT_TRUE(mgr.patchTableSchema("users", patch));

    auto events = changefeed_->listEvents();
    auto it = std::find_if(events.begin(), events.end(),
        [before](const Changefeed::ChangeEvent& e) {
            return e.sequence > before &&
                   e.key == "schema:users" &&
                   e.type == Changefeed::ChangeEventType::EVENT_PUT;
        });
    ASSERT_NE(it, events.end())
        << "Expected a schema_updated EVENT_PUT for 'users'";
    EXPECT_EQ(it->metadata.value("event", ""), "schema_updated");
}

// deleteTableSchema emits a schema_deleted event.
TEST_F(SchemaChangefeedTest, DeleteTableSchemaEmitsDeletedEvent) {
    SchemaManager mgr(*db_, index_mgr_.get());
    mgr.setChangefeed(changefeed_.get());

    ASSERT_TRUE(mgr.setTableSchema("invoices", makeSchema("invoices")));

    uint64_t before = changefeed_->getLatestSequence();

    ASSERT_TRUE(mgr.deleteTableSchema("invoices"));

    auto events = changefeed_->listEvents();
    auto it = std::find_if(events.begin(), events.end(),
        [before](const Changefeed::ChangeEvent& e) {
            return e.sequence > before &&
                   e.key == "schema:invoices" &&
                   e.type == Changefeed::ChangeEventType::EVENT_DELETE;
        });
    ASSERT_NE(it, events.end())
        << "Expected a schema_deleted EVENT_DELETE for 'invoices'";
    EXPECT_EQ(it->metadata.value("event", ""), "schema_deleted");
}

// Without a registered changefeed no events are emitted (no crash).
TEST_F(SchemaChangefeedTest, NoChangefeedRegisteredIsNoop) {
    SchemaManager mgr(*db_, index_mgr_.get());
    // Do NOT call setChangefeed

    uint64_t before = changefeed_->getLatestSequence();

    EXPECT_TRUE(mgr.setTableSchema("items", makeSchema("items")));

    // The standalone changefeed_ should still be at its original sequence
    EXPECT_EQ(changefeed_->getLatestSequence(), before)
        << "No events should be emitted when no changefeed is registered";
}

// Setting changefeed to nullptr disables notifications.
TEST_F(SchemaChangefeedTest, NullChangefeedDisablesNotifications) {
    SchemaManager mgr(*db_, index_mgr_.get());
    mgr.setChangefeed(changefeed_.get());
    ASSERT_TRUE(mgr.setTableSchema("cats", makeSchema("cats")));

    // Deregister
    mgr.setChangefeed(nullptr);
    uint64_t before = changefeed_->getLatestSequence();

    EXPECT_TRUE(mgr.setTableSchema("dogs", makeSchema("dogs")));

    EXPECT_EQ(changefeed_->getLatestSequence(), before)
        << "No events should be emitted after changefeed is set to nullptr";
}

// Events are ordered and carry monotonically increasing sequence numbers.
TEST_F(SchemaChangefeedTest, MultipleSchemaChangesAreOrdered) {
    SchemaManager mgr(*db_, index_mgr_.get());
    mgr.setChangefeed(changefeed_.get());

    uint64_t before = changefeed_->getLatestSequence();

    ASSERT_TRUE(mgr.setTableSchema("t1", makeSchema("t1")));
    ASSERT_TRUE(mgr.setTableSchema("t2", makeSchema("t2")));
    ASSERT_TRUE(mgr.deleteTableSchema("t1"));

    Changefeed::ListOptions opts;
    opts.from_sequence = before;
    auto events = changefeed_->listEvents(opts);

    ASSERT_GE(events.size(), 3u);

    // Sequences must be strictly increasing
    for (size_t i = 1; i < events.size(); ++i) {
        EXPECT_GT(events[i].sequence, events[i - 1].sequence)
            << "Events must be ordered by sequence";
    }
}

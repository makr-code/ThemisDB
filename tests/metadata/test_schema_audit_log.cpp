// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Tests for SchemaAuditLog

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <set>
#include <thread>

#include "metadata/schema_audit_log.h"
#include "storage/rocksdb_wrapper.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class SchemaAuditLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_audit_log_");
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
    }

    void TearDown() override {
        if (db_) {
          db_->close();
        }
    }

    std::unique_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(SchemaAuditLogTest, ConstructAndDestruct) {
    SchemaAuditLog log(*db_);
    SUCCEED();
}

// ============================================================================
// record() – basic write
// ============================================================================

TEST_F(SchemaAuditLogTest, RecordReturnsTrueOnSuccess) {
    SchemaAuditLog log(*db_);
    bool ok = log.record("users", "create", "alice", "initial schema", 1);
    EXPECT_TRUE(ok);
}

TEST_F(SchemaAuditLogTest, RecordWithMinimalArgs) {
    SchemaAuditLog log(*db_);
    bool ok = log.record("orders", "update");
    EXPECT_TRUE(ok);
}

TEST_F(SchemaAuditLogTest, RecordWithExtraMetadata) {
    SchemaAuditLog log(*db_);
    nlohmann::json extra = {{"migrated_from", "v1"}, {"applied_by", "ci"}};
    bool ok = log.record("products", "import", "system", "v1 to v2 migration", 5, extra);
    EXPECT_TRUE(ok);
}

// ============================================================================
// getHistory() – read back entries
// ============================================================================

TEST_F(SchemaAuditLogTest, EmptyHistoryOnFreshDb) {
    SchemaAuditLog log(*db_);
    auto history = log.getHistory("nonexistent_table");
    EXPECT_TRUE(history.empty());
}

TEST_F(SchemaAuditLogTest, HistoryContainsRecordedEntry) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("users", "create", "bob", "initial", 1));

    auto history = log.getHistory("users");
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].table_name, "users");
    EXPECT_EQ(history[0].operation, "create");
    EXPECT_EQ(history[0].author,    "bob");
    EXPECT_EQ(history[0].description, "initial");
    EXPECT_EQ(history[0].version,   1u);
}

TEST_F(SchemaAuditLogTest, HistoryContainsMultipleEntriesInOrder) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("users", "create", "alice", "created",  1));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(log.record("users", "update", "bob",   "added col", 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(log.record("users", "update", "carol", "renamed",   3));

    auto history = log.getHistory("users");
    ASSERT_EQ(history.size(), 3u);
    // Entries must be in ascending timestamp order (id-sorted)
    EXPECT_EQ(history[0].operation, "create");
    EXPECT_EQ(history[1].operation, "update");
    EXPECT_EQ(history[2].operation, "update");
    EXPECT_EQ(history[2].author,    "carol");
}

TEST_F(SchemaAuditLogTest, HistoryIsScopedToTable) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("users",    "create", "alice", "users table", 1));
    ASSERT_TRUE(log.record("products", "create", "bob",   "products",    1));

    auto users_history = log.getHistory("users");
    ASSERT_EQ(users_history.size(), 1u);
    EXPECT_EQ(users_history[0].table_name, "users");

    auto products_history = log.getHistory("products");
    ASSERT_EQ(products_history.size(), 1u);
    EXPECT_EQ(products_history[0].table_name, "products");
}

// ============================================================================
// getFullHistory()
// ============================================================================

TEST_F(SchemaAuditLogTest, FullHistorySpansAllTables) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("tableA", "create", "u1", "", 1));
    ASSERT_TRUE(log.record("tableB", "create", "u2", "", 1));
    ASSERT_TRUE(log.record("tableC", "create", "u3", "", 1));

    auto full = log.getFullHistory();
    ASSERT_EQ(full.size(), 3u);

    std::set<std::string> tables = {};

    for (const auto& e : full) {
      tables.insert(e.table_name);
    }
    EXPECT_EQ(tables.count("tableA"), 1u);
    EXPECT_EQ(tables.count("tableB"), 1u);
    EXPECT_EQ(tables.count("tableC"), 1u);
}

TEST_F(SchemaAuditLogTest, FullHistoryEmptyOnFreshDb) {
    SchemaAuditLog log(*db_);
    EXPECT_TRUE(log.getFullHistory().empty());
}

// ============================================================================
// getRecentHistory()
// ============================================================================

TEST_F(SchemaAuditLogTest, RecentHistoryReturnsAllWhenUnderLimit) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("t", "create"));
    ASSERT_TRUE(log.record("t", "update"));

    auto recent = log.getRecentHistory("t", 10);
    EXPECT_EQ(recent.size(), 2u);
}

TEST_F(SchemaAuditLogTest, RecentHistoryRespectsLimit) {
    SchemaAuditLog log(*db_);
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        ASSERT_TRUE(log.record("t", "update", "u", std::to_string(i), static_cast<uint64_t>(i)));
    }

    auto recent = log.getRecentHistory("t", 3);
    ASSERT_EQ(recent.size(), 3u);
    // Most recent entries should be versions 2, 3, 4
    EXPECT_EQ(recent[2].version, 4u);
}

// ============================================================================
// JSON export
// ============================================================================

TEST_F(SchemaAuditLogTest, HistoryToJSONIsArray) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("users", "create", "alice", "initial", 1));

    auto j = log.historyToJSON("users");
    EXPECT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 1u);
    EXPECT_EQ(j[0]["table_name"], "users");
    EXPECT_EQ(j[0]["operation"],  "create");
    EXPECT_EQ(j[0]["author"],     "alice");
    EXPECT_EQ(j[0]["version"],    1u);
}

TEST_F(SchemaAuditLogTest, FullHistoryToJSONIsArray) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("a", "create"));
    ASSERT_TRUE(log.record("b", "create"));

    auto j = log.fullHistoryToJSON();
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2u);
}

TEST_F(SchemaAuditLogTest, HistoryToJSONContainsTimestampField) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("users", "delete", "carol", "cleanup", 3));

    auto j = log.historyToJSON("users");
    ASSERT_FALSE(j.empty());
    EXPECT_TRUE(j[0].contains("timestamp"));
    // Timestamp must be a non-empty string
    EXPECT_FALSE(j[0]["timestamp"].get<std::string>().empty());
}

TEST_F(SchemaAuditLogTest, HistoryToJSONContainsExtraMetadata) {
    SchemaAuditLog log(*db_);
    nlohmann::json extra = {{"source", "migration"}, {"batch", 42}};
    ASSERT_TRUE(log.record("orders", "import", "system", "batch import", 7, extra));

    auto j = log.historyToJSON("orders");
    ASSERT_FALSE(j.empty());
    EXPECT_EQ(j[0]["metadata"]["source"], "migration");
    EXPECT_EQ(j[0]["metadata"]["batch"],  42);
}

// ============================================================================
// SchemaAuditEntry serialization round-trip
// ============================================================================

TEST_F(SchemaAuditLogTest, EntryToFromJSONRoundTrip) {
    SchemaAuditLog log(*db_);
    ASSERT_TRUE(log.record("users", "rollback", "admin", "emergency rollback", 5));

    auto history = log.getHistory("users");
    ASSERT_EQ(history.size(), 1u);

    auto j    = history[0].toJSON();
    auto copy = SchemaAuditEntry::fromJSON(j);

    EXPECT_EQ(copy.table_name,  "users");
    EXPECT_EQ(copy.operation,   "rollback");
    EXPECT_EQ(copy.author,      "admin");
    EXPECT_EQ(copy.description, "emergency rollback");
    EXPECT_EQ(copy.version,     5u);
}

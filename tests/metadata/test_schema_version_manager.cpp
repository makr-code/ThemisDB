// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

#include "metadata/schema_version_manager.h"
#include "metadata/schema_manager.h"
#include "metadata/schema_audit_log.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class SchemaVersionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_ver_");
        cfg.enable_blobdb = false;

        db_       = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        idx_mgr_  = std::make_unique<SecondaryIndexManager>(*db_);
        schema_   = std::make_unique<SchemaManager>(*db_, idx_mgr_.get());
    }

    void TearDown() override {
        if (db_) {
          db_->close();
        }
    }

    /// Insert a simple schema for testing
    void registerSchema(const std::string& table_name) {
        SchemaManager::TableSchema ts;
        ts.name = table_name;
        ts.type = "relational";
        SchemaManager::PropertyInfo p;
        p.name = "id";  p.type = "integer";
        ts.properties.push_back(p);
        schema_->setTableSchema(table_name, ts);
    }

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_mgr_;
    std::unique_ptr<SchemaManager>         schema_;
};

// ============================================================================
// Basic versioning
// ============================================================================

TEST_F(SchemaVersionManagerTest, CreateVersionUnknownTable) {
    SchemaVersionManager svm(*db_, *schema_);
    auto r = svm.createSchemaVersion("no_such_table");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::TABLE_NOT_FOUND);
}

TEST_F(SchemaVersionManagerTest, CreateFirstVersion) {
    registerSchema("users");
    SchemaVersionManager svm(*db_, *schema_);

    auto r = svm.createSchemaVersion("users", "admin", "initial schema");
    ASSERT_TRUE(r.ok) << r.error_message;
    EXPECT_EQ(r.value, 1u);
}

TEST_F(SchemaVersionManagerTest, VersionsIncrement) {
    registerSchema("orders");
    SchemaVersionManager svm(*db_, *schema_);

    auto r1 = svm.createSchemaVersion("orders", "admin", "v1");
    auto r2 = svm.createSchemaVersion("orders", "admin", "v2");
    auto r3 = svm.createSchemaVersion("orders", "admin", "v3");

    ASSERT_TRUE(r1.ok);
    ASSERT_TRUE(r2.ok);
    ASSERT_TRUE(r3.ok);
    EXPECT_EQ(r1.value, 1u);
    EXPECT_EQ(r2.value, 2u);
    EXPECT_EQ(r3.value, 3u);
}

TEST_F(SchemaVersionManagerTest, EmptyTableName) {
    SchemaVersionManager svm(*db_, *schema_);
    auto r = svm.createSchemaVersion("");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::TABLE_NOT_FOUND);
}

// ============================================================================
// getCurrentVersion
// ============================================================================

TEST_F(SchemaVersionManagerTest, GetCurrentVersionNoHistory) {
    SchemaVersionManager svm(*db_, *schema_);
    auto r = svm.getCurrentVersion("no_such_table");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::TABLE_NOT_FOUND);
}

TEST_F(SchemaVersionManagerTest, GetCurrentVersionAfterCreate) {
    registerSchema("items");
    SchemaVersionManager svm(*db_, *schema_);

    svm.createSchemaVersion("items");
    svm.createSchemaVersion("items");

    auto r = svm.getCurrentVersion("items");
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.value, 2u);
}

// ============================================================================
// getChangeHistory
// ============================================================================

TEST_F(SchemaVersionManagerTest, GetChangeHistoryEmpty) {
    SchemaVersionManager svm(*db_, *schema_);
    auto r = svm.getChangeHistory("no_table");
    ASSERT_TRUE(r.ok);  // No history = empty list, not an error
    EXPECT_TRUE(r.value.empty());
}

TEST_F(SchemaVersionManagerTest, GetChangeHistoryMultiple) {
    registerSchema("products");
    SchemaVersionManager svm(*db_, *schema_);

    svm.createSchemaVersion("products", "alice", "initial");
    svm.createSchemaVersion("products", "bob",   "add price");

    auto r = svm.getChangeHistory("products");
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.value.size(), 2u);

    EXPECT_EQ(r.value[0].version,     1u);
    EXPECT_EQ(r.value[0].author,      "alice");
    EXPECT_EQ(r.value[0].description, "initial");
    EXPECT_EQ(r.value[0].change_type, "create");

    EXPECT_EQ(r.value[1].version,     2u);
    EXPECT_EQ(r.value[1].author,      "bob");
    EXPECT_EQ(r.value[1].change_type, "update");
}

// ============================================================================
// getVersion
// ============================================================================

TEST_F(SchemaVersionManagerTest, GetVersionSpecific) {
    registerSchema("users");
    SchemaVersionManager svm(*db_, *schema_);

    svm.createSchemaVersion("users", "admin", "v1");
    svm.createSchemaVersion("users", "admin", "v2");

    auto r = svm.getVersion("users", 1);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.value.version,     1u);
    EXPECT_EQ(r.value.description, "v1");
}

TEST_F(SchemaVersionManagerTest, GetVersionNotFound) {
    registerSchema("users");
    SchemaVersionManager svm(*db_, *schema_);

    svm.createSchemaVersion("users");

    auto r = svm.getVersion("users", 99);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::VERSION_NOT_FOUND);
}

TEST_F(SchemaVersionManagerTest, GetVersionZeroInvalid) {
    SchemaVersionManager svm(*db_, *schema_);
    auto r = svm.getVersion("any_table", 0);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::INVALID_VERSION);
}

// ============================================================================
// rollbackToVersion
// ============================================================================

TEST_F(SchemaVersionManagerTest, RollbackToVersion) {
    registerSchema("orders");
    SchemaVersionManager svm(*db_, *schema_);

    // Create v1 snapshot
    svm.createSchemaVersion("orders", "admin", "v1");

    // Modify the schema (add a column)
    SchemaManager::TableSchema ts;
    ts.name = "orders";
    ts.type = "relational";
    SchemaManager::PropertyInfo p1; p1.name = "id";    p1.type = "integer";
    SchemaManager::PropertyInfo p2; p2.name = "extra"; p2.type = "string";
    ts.properties.push_back(p1);
    ts.properties.push_back(p2);
    schema_->setTableSchema("orders", ts);

    // Record v2
    svm.createSchemaVersion("orders", "admin", "added extra");

    // Verify current schema has 2 properties
    auto cur = schema_->getTable("orders");
    ASSERT_TRUE(cur.has_value());
    EXPECT_EQ(cur->properties.size(), 2u);

    // Roll back to v1
    auto rb = svm.rollbackToVersion("orders", 1, "admin");
    ASSERT_TRUE(rb.ok) << rb.error_message;

    // Schema should now have 1 property
    auto rolled = schema_->getTable("orders");
    ASSERT_TRUE(rolled.has_value());
    EXPECT_EQ(rolled->properties.size(), 1u);
}

TEST_F(SchemaVersionManagerTest, RollbackVersionNotFound) {
    registerSchema("tbl");
    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("tbl");

    auto r = svm.rollbackToVersion("tbl", 99);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::VERSION_NOT_FOUND);
}

// ============================================================================
// diffVersions
// ============================================================================

TEST_F(SchemaVersionManagerTest, DiffVersionsAddedProperty) {
    // v1: just "id"
    SchemaManager::TableSchema ts1;
    ts1.name = "tbl";  ts1.type = "relational";
    SchemaManager::PropertyInfo p1; p1.name = "id"; p1.type = "integer";
    ts1.properties.push_back(p1);
    schema_->setTableSchema("tbl", ts1);

    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("tbl", "admin", "v1");

    // v2: "id" + "name"
    SchemaManager::TableSchema ts2 = ts1;
    SchemaManager::PropertyInfo p2; p2.name = "name"; p2.type = "string";
    ts2.properties.push_back(p2);
    schema_->setTableSchema("tbl", ts2);
    svm.createSchemaVersion("tbl", "admin", "v2");

    auto r = svm.diffVersions("tbl", 1, 2);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_EQ(r.value["added"].size(), 1u);
    EXPECT_EQ(r.value["removed"].size(), 0u);
    EXPECT_EQ(r.value["added"][0]["name"], "name");
}

TEST_F(SchemaVersionManagerTest, DiffVersionsRemovedProperty) {
    // v1: "id" + "email"
    SchemaManager::TableSchema ts1;
    ts1.name = "tbl2";  ts1.type = "relational";
    SchemaManager::PropertyInfo p1; p1.name = "id";    p1.type = "integer";
    SchemaManager::PropertyInfo p2; p2.name = "email"; p2.type = "string";
    ts1.properties.push_back(p1);
    ts1.properties.push_back(p2);
    schema_->setTableSchema("tbl2", ts1);

    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("tbl2");

    // v2: only "id"
    SchemaManager::TableSchema ts2;
    ts2.name = "tbl2"; ts2.type = "relational";
    ts2.properties.push_back(p1);
    schema_->setTableSchema("tbl2", ts2);
    svm.createSchemaVersion("tbl2");

    auto r = svm.diffVersions("tbl2", 1, 2);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.value["removed"].size(), 1u);
    EXPECT_EQ(r.value["removed"][0]["name"], "email");
}

// ============================================================================
// JSON serialisation
// ============================================================================

TEST_F(SchemaVersionManagerTest, SchemaChangeToJSON) {
    registerSchema("json_test");
    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("json_test", "dev", "test version");

    auto r = svm.getVersion("json_test", 1);
    ASSERT_TRUE(r.ok);

    auto j = r.value.toJSON();
    EXPECT_EQ(j["version"],     1u);
    EXPECT_EQ(j["table_name"],  "json_test");
    EXPECT_EQ(j["author"],      "dev");
    EXPECT_EQ(j["description"], "test version");
    EXPECT_TRUE(j.contains("snapshot"));
    EXPECT_TRUE(j.contains("timestamp"));
}

TEST_F(SchemaVersionManagerTest, HistoryToJSON) {
    registerSchema("htj");
    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("htj", "a", "first");
    svm.createSchemaVersion("htj", "b", "second");

    auto j = svm.historyToJSON("htj");
    ASSERT_EQ(j.size(), 2u);
    EXPECT_EQ(j[0]["version"], 1u);
    EXPECT_EQ(j[1]["version"], 2u);
}

// ============================================================================
// SchemaAuditLog integration via setAuditLog()
// ============================================================================

class SchemaVersionManagerAuditTest : public ::testing::Test {
protected:
    void SetUp() override {
        namespace fs = std::filesystem;
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string path = (fs::temp_directory_path() /
            ("test_ver_audit_" + std::to_string(now))).string();

        RocksDBWrapper::Config cfg;
        cfg.db_path       = path;
        cfg.enable_blobdb = false;

        db_      = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        idx_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
        schema_  = std::make_unique<SchemaManager>(*db_, idx_mgr_.get());
        audit_   = std::make_unique<SchemaAuditLog>(*db_);
    }

    void TearDown() override {
        if (db_) {
          db_->close();
        }
    }

    void registerSchema(const std::string& name) {
        SchemaManager::TableSchema ts;
        ts.name = name; ts.type = "relational";
        SchemaManager::PropertyInfo p; p.name = "id"; p.type = "integer";
        ts.properties.push_back(p);
        schema_->setTableSchema(name, ts);
    }

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_mgr_;
    std::unique_ptr<SchemaManager>         schema_;
    std::unique_ptr<SchemaAuditLog>        audit_;
};

TEST_F(SchemaVersionManagerAuditTest, CreateVersionWritesToAuditLog) {
    registerSchema("users");

    SchemaVersionManager svm(*db_, *schema_);
    svm.setAuditLog(audit_.get());

    auto r = svm.createSchemaVersion("users", "alice", "initial version");
    ASSERT_TRUE(r.ok);

    // The audit log must contain at least one entry for "users"
    auto history = audit_->getHistory("users");
    EXPECT_FALSE(history.empty());
    EXPECT_EQ(history.back().table_name, "users");
    EXPECT_EQ(history.back().author,     "alice");
}

TEST_F(SchemaVersionManagerAuditTest, RollbackWritesToAuditLog) {
    registerSchema("orders");

    SchemaVersionManager svm(*db_, *schema_);
    svm.setAuditLog(audit_.get());

    // Create two versions so we can roll back
    svm.createSchemaVersion("orders", "bob", "v1");

    SchemaManager::TableSchema ts2;
    ts2.name = "orders"; ts2.type = "relational";
    SchemaManager::PropertyInfo p1; p1.name = "id";  p1.type = "integer";
    SchemaManager::PropertyInfo p2; p2.name = "qty"; p2.type = "integer";
    ts2.properties.push_back(p1);
    ts2.properties.push_back(p2);
    schema_->setTableSchema("orders", ts2);
    svm.createSchemaVersion("orders", "bob", "v2");

    auto r = svm.rollbackToVersion("orders", 1, "carol");
    ASSERT_TRUE(r.ok);

    // Audit log must now contain entries for both create and rollback
    auto history = audit_->getHistory("orders");
    bool found_rollback = false;
    for (const auto& e : history) {
        if (e.operation == "rollback") { found_rollback = true; break; }
    }
    EXPECT_TRUE(found_rollback) << "Expected a 'rollback' entry in audit log";
}

TEST_F(SchemaVersionManagerAuditTest, NoAuditLogSetDoesNotCrash) {
    registerSchema("products");

    SchemaVersionManager svm(*db_, *schema_);
    // Do NOT call setAuditLog – audit_log_ remains nullptr

    auto r = svm.createSchemaVersion("products", "sys", "no audit");
    EXPECT_TRUE(r.ok) << "createSchemaVersion must succeed even without audit log";
}

TEST_F(SchemaVersionManagerAuditTest, MultipleVersionsAllAppearInAuditLog) {
    registerSchema("items");

    SchemaVersionManager svm(*db_, *schema_);
    svm.setAuditLog(audit_.get());

    for (int i = 0; i < 3; ++i) {
        auto r = svm.createSchemaVersion("items", "dev", "change " + std::to_string(i));
        ASSERT_TRUE(r.ok);
    }

    auto history = audit_->getHistory("items");
    EXPECT_GE(history.size(), 3u) << "Each createSchemaVersion call must produce an audit entry";
}

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Migration regression tests: forward migrations and rollbacks across 5 schema versions.
// Verifies that the full version lifecycle works correctly end-to-end.

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

#include "metadata/schema_version_manager.h"
#include "metadata/schema_manager.h"
#include "metadata/schema_audit_log.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

// ============================================================================
// Test fixture
// ============================================================================

class MigrationRegressionTest : public ::testing::Test {
protected:
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper>     db;
    std::unique_ptr<SecondaryIndexManager> idx;
    std::unique_ptr<SchemaManager>      schema_mgr;
    std::unique_ptr<SchemaVersionManager> version_mgr;
    std::unique_ptr<SchemaAuditLog>     audit_log;

    void SetUp() override {
        db_path = makeTempDbPath("migration_regression_");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());

        idx        = std::make_unique<SecondaryIndexManager>(*db);
        schema_mgr = std::make_unique<SchemaManager>(*db, idx.get());
        version_mgr = std::make_unique<SchemaVersionManager>(*db, *schema_mgr);
        audit_log   = std::make_unique<SchemaAuditLog>(*db);
        version_mgr->setAuditLog(audit_log.get());
    }

    void TearDown() override {
        version_mgr.reset();
        audit_log.reset();
        schema_mgr.reset();
        idx.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    // Helper: build a schema with a given set of column names
    SchemaManager::TableSchema buildSchema(const std::string& table,
                                           const std::vector<std::string>& columns) {
        SchemaManager::TableSchema s;
        s.name = table;
        s.type = "relational";
        for (const auto& col : columns) {
            SchemaManager::PropertyInfo p;
            p.name    = col;
            p.type    = "string";
            p.indexed = false;
            s.properties.push_back(p);
        }
        return s;
    }

    // Helper: register schema and snapshot a version
    uint64_t registerAndVersion(const std::string& table,
                                 const std::vector<std::string>& columns,
                                 const std::string& author,
                                 const std::string& desc) {
        auto s = buildSchema(table, columns);
        EXPECT_TRUE(schema_mgr->setTableSchema(table, s));
        auto r = version_mgr->createSchemaVersion(table, author, desc);
        EXPECT_TRUE(r.ok) << r.error_message;
        return r.value;
    }
};

// ============================================================================
// Forward migration across 5 versions
// ============================================================================

TEST_F(MigrationRegressionTest, ForwardMigration5Versions) {
    const std::string TABLE = "employees";

    // v1: id
    uint64_t v1 = registerAndVersion(TABLE, {"id"},                            "alice", "initial");
    EXPECT_EQ(v1, 1u);

    // v2: id, name
    uint64_t v2 = registerAndVersion(TABLE, {"id", "name"},                   "bob",   "add name");
    EXPECT_EQ(v2, 2u);

    // v3: id, name, email
    uint64_t v3 = registerAndVersion(TABLE, {"id", "name", "email"},          "carol", "add email");
    EXPECT_EQ(v3, 3u);

    // v4: id, name, email, phone
    uint64_t v4 = registerAndVersion(TABLE, {"id", "name", "email", "phone"}, "dave",  "add phone");
    EXPECT_EQ(v4, 4u);

    // v5: id, name, email, phone, department
    uint64_t v5 = registerAndVersion(TABLE, {"id", "name", "email", "phone", "department"}, "eve", "add department");
    EXPECT_EQ(v5, 5u);

    // Verify current version
    auto cur = version_mgr->getCurrentVersion(TABLE);
    ASSERT_TRUE(cur.ok);
    EXPECT_EQ(cur.value, 5u);

    // Verify full history
    auto history = version_mgr->getChangeHistory(TABLE);
    ASSERT_TRUE(history.ok);
    ASSERT_EQ(history.value.size(), 5u);

    EXPECT_EQ(history.value[0].version, 1u);
    EXPECT_EQ(history.value[0].author,  "alice");
    EXPECT_EQ(history.value[0].change_type, "create");

    EXPECT_EQ(history.value[1].version, 2u);
    EXPECT_EQ(history.value[1].author,  "bob");
    EXPECT_EQ(history.value[1].change_type, "update");

    EXPECT_EQ(history.value[4].version, 5u);
    EXPECT_EQ(history.value[4].author,  "eve");

    // Verify column counts in snapshots
    EXPECT_EQ(history.value[0].snapshot.properties.size(), 1u);  // v1: id
    EXPECT_EQ(history.value[1].snapshot.properties.size(), 2u);  // v2: id, name
    EXPECT_EQ(history.value[4].snapshot.properties.size(), 5u);  // v5: 5 cols
}

// ============================================================================
// Rollback from v5 to v2
// ============================================================================

TEST_F(MigrationRegressionTest, RollbackFromV5ToV2) {
    const std::string TABLE = "products";

    registerAndVersion(TABLE, {"id"},              "alice", "v1");
    registerAndVersion(TABLE, {"id", "sku"},       "bob",   "v2 - add sku");
    registerAndVersion(TABLE, {"id", "sku", "x"},  "carol", "v3 - add x");
    registerAndVersion(TABLE, {"id", "sku", "x", "y"}, "dave", "v4 - add y");
    registerAndVersion(TABLE, {"id", "sku", "x", "y", "z"}, "eve", "v5 - add z");

    // Rollback to v2
    auto rb = version_mgr->rollbackToVersion(TABLE, 2u, "ops");
    ASSERT_TRUE(rb.ok) << rb.error_message;

    // Current live schema should now have 2 columns (id, sku)
    auto live = schema_mgr->getTable(TABLE);
    ASSERT_TRUE(live.has_value());
    EXPECT_EQ(live->properties.size(), 2u);
    EXPECT_EQ(live->properties[0].name, "id");
    EXPECT_EQ(live->properties[1].name, "sku");

    // A new version (v6) should have been recorded for the rollback
    auto cur = version_mgr->getCurrentVersion(TABLE);
    ASSERT_TRUE(cur.ok);
    EXPECT_EQ(cur.value, 6u);

    auto history = version_mgr->getChangeHistory(TABLE);
    ASSERT_TRUE(history.ok);
    ASSERT_EQ(history.value.size(), 6u);
    EXPECT_EQ(history.value[5].description, "Rollback to version 2");
    EXPECT_EQ(history.value[5].author, "ops");
}

// ============================================================================
// Rollback to v1 (initial) from v5
// ============================================================================

TEST_F(MigrationRegressionTest, RollbackToV1) {
    const std::string TABLE = "orders";

    registerAndVersion(TABLE, {"id"},                  "a", "create");
    registerAndVersion(TABLE, {"id", "amount"},        "b", "add amount");
    registerAndVersion(TABLE, {"id", "amount", "status"}, "c", "add status");
    registerAndVersion(TABLE, {"id", "amount", "status", "note"}, "d", "add note");
    registerAndVersion(TABLE, {"id", "amount", "status", "note", "ref"}, "e", "add ref");

    auto rb = version_mgr->rollbackToVersion(TABLE, 1u, "admin");
    ASSERT_TRUE(rb.ok) << rb.error_message;

    auto live = schema_mgr->getTable(TABLE);
    ASSERT_TRUE(live.has_value());
    EXPECT_EQ(live->properties.size(), 1u);
    EXPECT_EQ(live->properties[0].name, "id");
}

// ============================================================================
// Multiple rollbacks in sequence
// ============================================================================

TEST_F(MigrationRegressionTest, MultipleRollbacks) {
    const std::string TABLE = "inventory";

    registerAndVersion(TABLE, {"id"},          "a", "v1");
    registerAndVersion(TABLE, {"id", "qty"},   "b", "v2");
    registerAndVersion(TABLE, {"id", "qty", "loc"}, "c", "v3");

    // First rollback: v3 → v1  (creates v4)
    auto rb1 = version_mgr->rollbackToVersion(TABLE, 1u, "ops");
    ASSERT_TRUE(rb1.ok);

    {
        auto live = schema_mgr->getTable(TABLE);
        ASSERT_TRUE(live.has_value());
        EXPECT_EQ(live->properties.size(), 1u);
    }

    // Snapshot v4 after first rollback, then add column
    registerAndVersion(TABLE, {"id", "sku"}, "ops", "v5 - add sku after rollback");

    // Second rollback: back to v2  (creates v6)
    auto rb2 = version_mgr->rollbackToVersion(TABLE, 2u, "ops2");
    ASSERT_TRUE(rb2.ok);

    {
        auto live = schema_mgr->getTable(TABLE);
        ASSERT_TRUE(live.has_value());
        EXPECT_EQ(live->properties.size(), 2u);
    }

    auto cur = version_mgr->getCurrentVersion(TABLE);
    ASSERT_TRUE(cur.ok);
    EXPECT_EQ(cur.value, 6u);
}

// ============================================================================
// Version diff across migrations
// ============================================================================

TEST_F(MigrationRegressionTest, DiffAcrossMigrations) {
    const std::string TABLE = "catalog";

    registerAndVersion(TABLE, {"id"},                 "a", "v1");
    registerAndVersion(TABLE, {"id", "name", "sku"},  "b", "v2 – add name, sku");
    registerAndVersion(TABLE, {"id", "name"},          "c", "v3 – remove sku");

    auto diff_1_2 = version_mgr->diffVersions(TABLE, 1u, 2u);
    ASSERT_TRUE(diff_1_2.ok) << diff_1_2.error_message;
    EXPECT_TRUE(diff_1_2.value.contains("added"));
    auto added = diff_1_2.value["added"];
    EXPECT_EQ(added.size(), 2u);  // "name" and "sku" were added

    auto diff_2_3 = version_mgr->diffVersions(TABLE, 2u, 3u);
    ASSERT_TRUE(diff_2_3.ok);
    EXPECT_TRUE(diff_2_3.value.contains("removed"));
    auto removed = diff_2_3.value["removed"];
    EXPECT_EQ(removed.size(), 1u);  // "sku" was removed
}

// ============================================================================
// Rollback to invalid / nonexistent version
// ============================================================================

TEST_F(MigrationRegressionTest, RollbackToNonexistentVersion) {
    const std::string TABLE = "ghost";
    registerAndVersion(TABLE, {"id"}, "a", "v1");

    // Version 99 does not exist
    auto rb = version_mgr->rollbackToVersion(TABLE, 99u, "ops");
    EXPECT_FALSE(rb.ok);
    EXPECT_EQ(rb.error, VersionErrorCode::VERSION_NOT_FOUND);
}

TEST_F(MigrationRegressionTest, RollbackToVersionZero) {
    const std::string TABLE = "ghost2";
    registerAndVersion(TABLE, {"id"}, "a", "v1");

    auto rb = version_mgr->rollbackToVersion(TABLE, 0u, "ops");
    EXPECT_FALSE(rb.ok);
    EXPECT_EQ(rb.error, VersionErrorCode::INVALID_VERSION);
}

// ============================================================================
// Audit log populated by migrations
// ============================================================================

TEST_F(MigrationRegressionTest, AuditLogPopulatedDuringMigrations) {
    const std::string TABLE = "audit_test";

    registerAndVersion(TABLE, {"id"},         "alice", "initial schema");
    registerAndVersion(TABLE, {"id", "name"}, "bob",   "add name column");

    // Rollback triggers an audit entry too
    version_mgr->rollbackToVersion(TABLE, 1u, "carol");

    auto history = audit_log->getHistory(TABLE);
    // Expect at least 3 audit entries (v1 create, v2 update, rollback)
    EXPECT_GE(history.size(), 3u);

    // Check operation types
    bool found_create   = false;
    bool found_update   = false;
    bool found_rollback = false;
    for (const auto& e : history) {
        EXPECT_EQ(e.table_name, TABLE);
        if (e.operation == "create") {
          found_create   = true;
        }
        if (e.operation == "update") {
          found_update   = true;
        }
        if (e.operation == "rollback") {
          found_rollback = true;
        }
    }
    EXPECT_TRUE(found_create);
    EXPECT_TRUE(found_update);
    EXPECT_TRUE(found_rollback);
}

// ============================================================================
// getVersion() retrieves correct snapshot
// ============================================================================

TEST_F(MigrationRegressionTest, GetVersionRetrievesCorrectSnapshot) {
    const std::string TABLE = "snapshots";

    registerAndVersion(TABLE, {"id"},                           "a", "v1");
    registerAndVersion(TABLE, {"id", "name"},                   "b", "v2");
    registerAndVersion(TABLE, {"id", "name", "email"},          "c", "v3");
    registerAndVersion(TABLE, {"id", "name", "email", "phone"}, "d", "v4");
    registerAndVersion(TABLE, {"id", "name", "email", "phone", "dept"}, "e", "v5");

    for (uint64_t v = 1; v <= 5; ++v) {
        auto res = version_mgr->getVersion(TABLE, v);
        ASSERT_TRUE(res.ok) << "getVersion(" << v << "): " << res.error_message;
        EXPECT_EQ(res.value.version, v);
        EXPECT_EQ(res.value.snapshot.properties.size(), v);  // v columns at version v
    }
}

// ============================================================================
// historyToJSON round-trip
// ============================================================================

TEST_F(MigrationRegressionTest, HistoryToJSON) {
    const std::string TABLE = "json_test";

    registerAndVersion(TABLE, {"id"},         "a", "v1");
    registerAndVersion(TABLE, {"id", "name"}, "b", "v2");

    auto j = version_mgr->historyToJSON(TABLE);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 2u);

    EXPECT_EQ(j[0]["version"].get<uint64_t>(), 1u);
    EXPECT_EQ(j[0]["author"].get<std::string>(), "a");
    EXPECT_EQ(j[1]["version"].get<uint64_t>(), 2u);
    EXPECT_EQ(j[1]["author"].get<std::string>(), "b");
}

// ============================================================================
// validateMigration – lifecycle regression tests
// Exercises validateMigration as a gate within a full migration sequence:
//   validate → apply → create version → validate next change
// ============================================================================

TEST_F(MigrationRegressionTest, ValidateMigration_PassesOnFirstVersion) {
    // No prior version exists; any well-formed schema must pass validation.
    const std::string TABLE = "vm_first";

    SchemaManager::TableSchema ts = buildSchema(TABLE, {"id", "name"});
    auto result = version_mgr->validateMigration(TABLE, ts);

    ASSERT_TRUE(result.ok) << result.error_message;
    EXPECT_TRUE(result.value);

    // Confirm that validateMigration did NOT create a version entry.
    auto cur = version_mgr->getCurrentVersion(TABLE);
    EXPECT_FALSE(cur.ok);  // no version recorded yet
}

TEST_F(MigrationRegressionTest, ValidateMigration_PassesAfterRealChange) {
    // Validate must succeed when the new schema differs from the stored one.
    const std::string TABLE = "vm_change";

    registerAndVersion(TABLE, {"id"}, "a", "v1");

    SchemaManager::TableSchema ts = buildSchema(TABLE, {"id", "email"});
    auto result = version_mgr->validateMigration(TABLE, ts);

    ASSERT_TRUE(result.ok) << result.error_message;
    EXPECT_TRUE(result.value);
}

TEST_F(MigrationRegressionTest, ValidateMigration_RejectsIdenticalSchema) {
    // If the proposed schema is identical to the current snapshot, validation
    // must fail (no-op migrations are not allowed).
    const std::string TABLE = "vm_identical";

    registerAndVersion(TABLE, {"id", "qty"}, "a", "v1");

    // Re-validate the same column set.
    SchemaManager::TableSchema same = buildSchema(TABLE, {"id", "qty"});
    auto result = version_mgr->validateMigration(TABLE, same);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, VersionErrorCode::INVALID_VERSION);
    EXPECT_NE(result.error_message.find("identical"), std::string::npos);
}

TEST_F(MigrationRegressionTest, ValidateMigration_GatesFullMigrationLifecycle) {
    // Full lifecycle: validate → apply → snapshot → validate next change.
    const std::string TABLE = "vm_lifecycle";

    // Step 1: create initial schema (v1)
    registerAndVersion(TABLE, {"id"}, "alice", "initial");

    // Step 2: validate a new schema before applying it
    SchemaManager::TableSchema v2_schema = buildSchema(TABLE, {"id", "name"});
    {
        auto val = version_mgr->validateMigration(TABLE, v2_schema);
        ASSERT_TRUE(val.ok) << val.error_message;
    }

    // Step 3: apply and version it (v2)
    ASSERT_TRUE(schema_mgr->setTableSchema(TABLE, v2_schema));
    auto v2 = version_mgr->createSchemaVersion(TABLE, "bob", "add name");
    ASSERT_TRUE(v2.ok);
    EXPECT_EQ(v2.value, 2u);

    // Step 4: validate another new schema (v3 candidate)
    SchemaManager::TableSchema v3_schema = buildSchema(TABLE, {"id", "name", "email"});
    {
        auto val = version_mgr->validateMigration(TABLE, v3_schema);
        ASSERT_TRUE(val.ok) << val.error_message;
    }

    // Step 5: applying the identical schema again must be rejected by validation
    {
        auto val = version_mgr->validateMigration(TABLE, v2_schema);
        EXPECT_FALSE(val.ok);
        EXPECT_EQ(val.error, VersionErrorCode::INVALID_VERSION);
    }
}

TEST_F(MigrationRegressionTest, ValidateMigration_RejectsEmptyName) {
    const std::string TABLE = "vm_emptyname";

    SchemaManager::TableSchema ts;
    ts.name = "";           // empty – must be rejected
    ts.type = "relational";
    SchemaManager::PropertyInfo p;
    p.name = "id"; p.type = "string";
    ts.properties.push_back(p);

    auto result = version_mgr->validateMigration(TABLE, ts);
    EXPECT_FALSE(result.ok);
}

TEST_F(MigrationRegressionTest, ValidateMigration_RejectsNoColumns) {
    const std::string TABLE = "vm_nocols";

    SchemaManager::TableSchema ts;
    ts.name = TABLE;
    ts.type = "relational";
    // no properties

    auto result = version_mgr->validateMigration(TABLE, ts);
    EXPECT_FALSE(result.ok);
}

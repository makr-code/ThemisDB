// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Focused unit tests for SchemaMigrator (Online Schema Migration).
// Covers: add/drop columns, rename, type change, add/drop indexes,
//         partition tables, phase transitions, error paths, batch ops,
//         chaining API, abort-on-first-error, reset/reuse.

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "storage/online_schema_migration.h"
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;
using namespace themis::storage;
namespace fs = std::filesystem;

// ============================================================================
// Helpers
// ============================================================================

static std::string makeTempDir(const std::string& prefix) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

/// Build a simple relational schema with the given column names (type = "string").
static SchemaManager::TableSchema makeSchema(
    const std::string& table,
    const std::vector<std::string>& columns)
{
    SchemaManager::TableSchema s;
    s.name = table;
    s.type = "relational";
    for (const auto& col : columns) {
        SchemaManager::PropertyInfo p;
        p.name     = col;
        p.type     = "string";
        p.nullable = true;
        s.properties.push_back(p);
    }
    return s;
}

/// Find a PropertyInfo by name; nullptr if not found.
static const SchemaManager::PropertyInfo* findProp(
    const SchemaManager::TableSchema& schema, const std::string& name)
{
    for (const auto& p : schema.properties) {
        if (p.name == name) return &p;
    }
    return nullptr;
}

/// Find an IndexInfo by name; nullptr if not found.
static const SchemaManager::IndexInfo* findIndex(
    const SchemaManager::TableSchema& schema, const std::string& name)
{
    for (const auto& idx : schema.indexes) {
        if (idx.name == name) return &idx;
    }
    return nullptr;
}

// ============================================================================
// Fixture
// ============================================================================

class OnlineSchemaMigrationFocusedTests : public ::testing::Test {
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<SchemaManager>         schema_;

    void SetUp() override {
        db_path_ = makeTempDir("osm_test_");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        idx_    = std::make_unique<SecondaryIndexManager>(*db_);
        schema_ = std::make_unique<SchemaManager>(*db_, idx_.get());
    }

    void TearDown() override {
        schema_.reset();
        idx_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(db_path_);
    }

    /// Seed the schema manager with a table.
    void seed(const std::string& table, const std::vector<std::string>& cols) {
        auto s = makeSchema(table, cols);
        ASSERT_TRUE(schema_->setTableSchema(table, s));
    }

    /// Fetch the current schema for a table (asserts existence).
    SchemaManager::TableSchema fetch(const std::string& table) {
        auto opt = schema_->getTable(table);
        EXPECT_TRUE(opt.has_value()) << "Table '" << table << "' not found";
        return opt.value_or(SchemaManager::TableSchema{});
    }
};

// ============================================================================
// 1. Add column – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddColumn_Basic) {
    seed("users", {"id", "name"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("users", "email", "VARCHAR(255)");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.ops_applied, 1u);
    EXPECT_GT(result.version, 0u);

    auto schema = fetch("users");
    ASSERT_NE(findProp(schema, "email"), nullptr);
    EXPECT_EQ(findProp(schema, "email")->type, "string");
}

// ============================================================================
// 2. Add column – non-nullable
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddColumn_NotNullable) {
    seed("orders", {"id"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("orders", "total", "DECIMAL(10,2)", /*nullable=*/false);

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("orders");
    auto* p = findProp(schema, "total");
    ASSERT_NE(p, nullptr);
    EXPECT_FALSE(p->nullable);
}

// ============================================================================
// 3. Add column – duplicate rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddColumn_DuplicateRejected) {
    seed("items", {"id", "sku"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("items", "sku", "VARCHAR(64)");  // 'sku' already exists

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// 4. Drop column – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, DropColumn_Basic) {
    seed("products", {"id", "name", "deprecated_col"});

    SchemaMigrator migrator(*schema_);
    migrator.dropColumn("products", "deprecated_col");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("products");
    EXPECT_EQ(findProp(schema, "deprecated_col"), nullptr);
    ASSERT_NE(findProp(schema, "name"), nullptr);
}

// ============================================================================
// 5. Drop column – not found rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, DropColumn_NotFound) {
    seed("catalog", {"id", "title"});

    SchemaMigrator migrator(*schema_);
    migrator.dropColumn("catalog", "nonexistent");

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 6. Rename column – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, RenameColumn_Basic) {
    seed("customers", {"id", "email"});

    SchemaMigrator migrator(*schema_);
    migrator.renameColumn("customers", "email", "email_address");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("customers");
    EXPECT_EQ(findProp(schema, "email"), nullptr);
    ASSERT_NE(findProp(schema, "email_address"), nullptr);
}

// ============================================================================
// 7. Rename column – target name already exists rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, RenameColumn_TargetAlreadyExists) {
    seed("accounts", {"id", "login", "email"});

    SchemaMigrator migrator(*schema_);
    migrator.renameColumn("accounts", "login", "email");  // 'email' already exists

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 8. Rename column – identical names rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, RenameColumn_SameName) {
    seed("entries", {"id", "body"});

    SchemaMigrator migrator(*schema_);
    migrator.renameColumn("entries", "body", "body");

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 9. Change column type – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, ChangeColumnType_Basic) {
    seed("logs", {"id", "level"});

    SchemaMigrator migrator(*schema_);
    migrator.changeColumnType("logs", "level", "integer");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("logs");
    auto* p = findProp(schema, "level");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->type, "integer");
}

// ============================================================================
// 10. Change column type – also changes nullability
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, ChangeColumnType_WithNullability) {
    seed("metrics", {"id", "value"});

    SchemaMigrator migrator(*schema_);
    migrator.changeColumnType("metrics", "value", "double", /*nullable=*/false);

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("metrics");
    auto* p = findProp(schema, "value");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->type, "double");
    EXPECT_FALSE(p->nullable);
}

// ============================================================================
// 11. Change column type – column not found
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, ChangeColumnType_ColumnNotFound) {
    seed("events", {"id", "name"});

    SchemaMigrator migrator(*schema_);
    migrator.changeColumnType("events", "nonexistent", "integer");

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 12. Add index – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddIndex_Basic) {
    seed("users", {"id", "username"});

    SchemaMigrator migrator(*schema_);
    migrator.addIndex("users", "username");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("users");
    ASSERT_NE(findIndex(schema, "username"), nullptr);
    EXPECT_FALSE(findIndex(schema, "username")->unique);

    // Column should also be marked as indexed
    auto* p = findProp(schema, "username");
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(p->indexed);
}

// ============================================================================
// 13. Add index – unique
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddIndex_Unique) {
    seed("members", {"id", "email"});

    SchemaMigrator migrator(*schema_);
    migrator.addIndex("members", "email", /*unique=*/true);

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("members");
    auto* idx = findIndex(schema, "email");
    ASSERT_NE(idx, nullptr);
    EXPECT_TRUE(idx->unique);
}

// ============================================================================
// 14. Add index – column not found
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddIndex_ColumnNotFound) {
    seed("posts", {"id", "title"});

    SchemaMigrator migrator(*schema_);
    migrator.addIndex("posts", "nonexistent_col");

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 15. Add index – duplicate rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AddIndex_Duplicate) {
    seed("tags", {"id", "name"});

    SchemaMigrator migrator(*schema_);
    migrator.addIndex("tags", "name");
    auto r1 = migrator.migrate();
    ASSERT_TRUE(r1.success);

    // Attempt to add same index again
    migrator.addIndex("tags", "name");
    auto r2 = migrator.migrate();
    EXPECT_FALSE(r2.success);
}

// ============================================================================
// 16. Drop index – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, DropIndex_Basic) {
    seed("widgets", {"id", "slug"});

    // First add then drop
    SchemaMigrator migrator(*schema_);
    migrator.addIndex("widgets", "slug");
    ASSERT_TRUE(migrator.migrate().success);

    migrator.dropIndex("widgets", "slug");
    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("widgets");
    EXPECT_EQ(findIndex(schema, "slug"), nullptr);
    // Column should no longer be marked indexed
    auto* p = findProp(schema, "slug");
    ASSERT_NE(p, nullptr);
    EXPECT_FALSE(p->indexed);
}

// ============================================================================
// 17. Drop index – not found rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, DropIndex_NotFound) {
    seed("sessions", {"id", "token"});

    SchemaMigrator migrator(*schema_);
    migrator.dropIndex("sessions", "token");  // No index exists yet

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 18. Partition table – basic
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, PartitionTable_Basic) {
    seed("orders", {"id", "region", "amount"});

    SchemaMigrator migrator(*schema_);
    migrator.partitionTable("orders", "region", 4);

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("orders");
    auto* pkey  = findProp(schema, "__partition_key");
    auto* pnum  = findProp(schema, "__num_partitions");
    ASSERT_NE(pkey, nullptr);
    ASSERT_NE(pnum, nullptr);
    EXPECT_EQ(pkey->type, "string");
    EXPECT_EQ(pnum->type, "integer");
}

// ============================================================================
// 19. Partition table – invalid num_partitions < 2 rejected
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, PartitionTable_InvalidNumPartitions) {
    seed("events", {"id", "ts"});

    SchemaMigrator migrator(*schema_);
    migrator.partitionTable("events", "ts", 1);  // must be >= 2

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 20. Partition table – missing partition key column
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, PartitionTable_MissingKeyColumn) {
    seed("records", {"id", "data"});

    SchemaMigrator migrator(*schema_);
    migrator.partitionTable("records", "nonexistent_key", 4);

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
}

// ============================================================================
// 21. Multi-op batch – add column + add index in one migrate()
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, Batch_AddColumnAndIndex) {
    seed("users", {"id", "name"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("users", "phone", "VARCHAR(20)")
            .addIndex("users", "phone");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.ops_applied, 2u);

    auto schema = fetch("users");
    ASSERT_NE(findProp(schema, "phone"), nullptr);
    ASSERT_NE(findIndex(schema, "phone"), nullptr);
}

// ============================================================================
// 22. Multi-table batch – two tables in one migrate()
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, Batch_MultiTable) {
    seed("table_a", {"id", "col1"});
    seed("table_b", {"id", "col2"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("table_a", "extra_a", "boolean")
            .addColumn("table_b", "extra_b", "integer");

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.ops_applied, 2u);

    ASSERT_NE(findProp(fetch("table_a"), "extra_a"), nullptr);
    ASSERT_NE(findProp(fetch("table_b"), "extra_b"), nullptr);
}

// ============================================================================
// 23. Phase transitions
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, PhaseTransitions) {
    seed("phases_tbl", {"id"});

    SchemaMigrator migrator(*schema_);
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::IDLE);

    migrator.addColumn("phases_tbl", "col_a", "string");
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::PENDING);

    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::COMPLETED);
}

// ============================================================================
// 24. Phase = FAILED on error
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, PhaseFailedOnError) {
    seed("fail_tbl", {"id"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("fail_tbl", "id", "string");  // duplicate → failure

    migrator.migrate();
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::FAILED);
}

// ============================================================================
// 25. abort_on_first_error = false continues after error
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, AbortOnFirstError_False) {
    seed("multi_err", {"id", "a"});

    SchemaMigrator::Config cfg;
    cfg.abort_on_first_error = false;
    SchemaMigrator migrator(*schema_, cfg);

    // First op will fail (duplicate), second should still apply
    migrator.addColumn("multi_err", "a", "string")    // fail – duplicate
            .addColumn("multi_err", "b", "string");   // should succeed

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
    EXPECT_GE(result.errors.size(), 1u);

    // 'b' should have been added despite the earlier error
    auto schema = fetch("multi_err");
    ASSERT_NE(findProp(schema, "b"), nullptr);
}

// ============================================================================
// 26. Empty migrate – no ops staged
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, EmptyMigrate_NoOps) {
    seed("empty_tbl", {"id"});

    SchemaMigrator migrator(*schema_);
    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.ops_total, 0u);
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::IDLE);
}

// ============================================================================
// 27. Table not found error
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, TableNotFound_Error) {
    // Don't seed "ghost_table"
    SchemaMigrator migrator(*schema_);
    migrator.addColumn("ghost_table", "col", "string");

    auto result = migrator.migrate();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::FAILED);
}

// ============================================================================
// 28. Reset clears staged ops and returns to IDLE
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, Reset_ClearsOpsAndPhase) {
    seed("reset_tbl", {"id"});

    SchemaMigrator migrator(*schema_);
    migrator.addColumn("reset_tbl", "col_x", "string");
    EXPECT_EQ(migrator.pendingOps(), 1u);

    migrator.reset();
    EXPECT_EQ(migrator.pendingOps(), 0u);
    EXPECT_EQ(migrator.currentPhase(), OnlineDDLPhase::IDLE);
}

// ============================================================================
// 29. Reuse after successful migrate
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, Reuse_AfterSuccessfulMigrate) {
    seed("reuse_tbl", {"id"});

    SchemaMigrator migrator(*schema_);

    // First migration
    migrator.addColumn("reuse_tbl", "col_a", "string");
    auto r1 = migrator.migrate();
    ASSERT_TRUE(r1.success);
    EXPECT_EQ(r1.version, 1u);

    // Second migration (reuse without reset – ops cleared automatically)
    migrator.addColumn("reuse_tbl", "col_b", "integer");
    auto r2 = migrator.migrate();
    ASSERT_TRUE(r2.success);
    EXPECT_EQ(r2.version, 2u);

    auto schema = fetch("reuse_tbl");
    ASSERT_NE(findProp(schema, "col_a"), nullptr);
    ASSERT_NE(findProp(schema, "col_b"), nullptr);
}

// ============================================================================
// 30. Drop column also removes associated index
// ============================================================================

TEST_F(OnlineSchemaMigrationFocusedTests, DropColumn_RemovesAssociatedIndex) {
    seed("indexed_tbl", {"id", "code"});

    SchemaMigrator migrator(*schema_);
    // Add an index on "code" first
    migrator.addIndex("indexed_tbl", "code");
    ASSERT_TRUE(migrator.migrate().success);

    // Now drop the column – index should be removed too
    migrator.dropColumn("indexed_tbl", "code");
    auto result = migrator.migrate();
    ASSERT_TRUE(result.success);

    auto schema = fetch("indexed_tbl");
    EXPECT_EQ(findProp(schema, "code"), nullptr);
    EXPECT_EQ(findIndex(schema, "code"), nullptr);
}

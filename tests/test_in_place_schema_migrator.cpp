/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_in_place_schema_migrator.cpp                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     376                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Unit + integration tests for InPlaceSchemaMigrator.
// Covers: additive detection, in-place apply, version recording,
// error paths (non-additive, empty table name, config.strict_additive=false).

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "updates/in_place_schema_migrator.h"
#include "metadata/schema_manager.h"
#include "metadata/schema_version_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;
using namespace themis::updates;
namespace fs = std::filesystem;

// ============================================================================
// Helpers
// ============================================================================

static std::string makeTempDir(const std::string& prefix) {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

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

// ============================================================================
// Fixture: production-like DB + schema + version managers
// ============================================================================

class InPlaceSchemaMigratorTest : public ::testing::Test {
protected:
    std::string db_path_;

    std::unique_ptr<RocksDBWrapper>       db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<SchemaManager>        schema_;
    std::unique_ptr<SchemaVersionManager> version_;

    void SetUp() override {
        db_path_ = makeTempDir("ipmig_test_");

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        idx_     = std::make_unique<SecondaryIndexManager>(*db_);
        schema_  = std::make_unique<SchemaManager>(*db_, idx_.get());
        version_ = std::make_unique<SchemaVersionManager>(*db_, *schema_);
    }

    void TearDown() override {
        version_.reset();
        schema_.reset();
        idx_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(db_path_);
    }

    /// Seed the DB with a baseline schema and version snapshot.
    void seedSchema(const std::string& table,
                    const SchemaManager::TableSchema& s) {
        ASSERT_TRUE(schema_->setTableSchema(table, s));
        version_->createSchemaVersion(table, "test-seed", "baseline");
    }
};

// ============================================================================
// Phase 1: isAdditiveMigration static check
// ============================================================================

TEST(InPlaceSchemaMigratorStaticTest, IsAdditive_AddOneColumn) {
    auto from = makeSchema("t", {"id", "name"});
    auto to   = makeSchema("t", {"id", "name", "email"});
    EXPECT_TRUE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

TEST(InPlaceSchemaMigratorStaticTest, IsAdditive_AddMultipleColumns) {
    auto from = makeSchema("t", {"id"});
    auto to   = makeSchema("t", {"id", "a", "b", "c"});
    EXPECT_TRUE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

TEST(InPlaceSchemaMigratorStaticTest, NotAdditive_ColumnRemoved) {
    auto from = makeSchema("t", {"id", "name", "old_col"});
    auto to   = makeSchema("t", {"id", "name"});
    EXPECT_FALSE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

TEST(InPlaceSchemaMigratorStaticTest, NotAdditive_NoNewColumn) {
    auto from = makeSchema("t", {"id", "name"});
    auto to   = makeSchema("t", {"id", "name"});
    EXPECT_FALSE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

TEST(InPlaceSchemaMigratorStaticTest, NotAdditive_ColumnTypeChanged) {
    auto from = makeSchema("t", {"id", "value"});
    auto to   = makeSchema("t", {"id", "value", "extra"});

    // Change "value" type to "integer" in to_schema
    to.properties[1].type = "integer";

    EXPECT_FALSE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

TEST(InPlaceSchemaMigratorStaticTest, NotAdditive_NullabilityChanged) {
    auto from = makeSchema("t", {"id", "required_field"});
    auto to   = makeSchema("t", {"id", "required_field", "extra"});

    // Make required_field NOT NULL in to_schema
    to.properties[1].nullable = false;

    EXPECT_FALSE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

TEST(InPlaceSchemaMigratorStaticTest, IsAdditive_EmptyFromSchema) {
    // from is empty → any non-empty to is trivially additive (all columns are new)
    SchemaManager::TableSchema from;
    from.name = "t";
    auto to   = makeSchema("t", {"id", "name"});
    EXPECT_TRUE(InPlaceSchemaMigrator::isAdditiveMigration(from, to));
}

// ============================================================================
// Phase 2: Successful in-place apply
// ============================================================================

TEST_F(InPlaceSchemaMigratorTest, Apply_AddsColumn_ReturnsSuccess) {
    auto from = makeSchema("users", {"id", "name"});
    auto to   = makeSchema("users", {"id", "name", "email"});

    seedSchema("users", from);

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("users", from, to, *schema_, *version_, "ci-bot");

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.added_columns.size(), 1u);
    EXPECT_EQ(result.added_columns[0], "email");
    EXPECT_GT(result.schema_version, 0u);
}

TEST_F(InPlaceSchemaMigratorTest, Apply_UpdatesSchemaManager) {
    auto from = makeSchema("products", {"id", "name"});
    auto to   = makeSchema("products", {"id", "name", "price", "sku"});

    seedSchema("products", from);

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("products", from, to, *schema_, *version_);

    ASSERT_TRUE(result.success) << result.error_message;

    auto tbl = schema_->getTable("products");
    ASSERT_TRUE(tbl.has_value());

    bool price_found = false, sku_found = false;
    for (const auto& p : tbl->properties) {
        if (p.name == "price") price_found = true;
        if (p.name == "sku")   sku_found   = true;
    }
    EXPECT_TRUE(price_found);
    EXPECT_TRUE(sku_found);
}

TEST_F(InPlaceSchemaMigratorTest, Apply_RecordsNewVersion) {
    auto from = makeSchema("orders", {"id"});
    auto to   = makeSchema("orders", {"id", "customer_id", "amount"});

    seedSchema("orders", from);

    auto ver_before = version_->getCurrentVersion("orders");
    ASSERT_TRUE(ver_before.ok);

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("orders", from, to, *schema_, *version_, "admin");

    ASSERT_TRUE(result.success) << result.error_message;

    auto ver_after = version_->getCurrentVersion("orders");
    ASSERT_TRUE(ver_after.ok);
    EXPECT_GT(ver_after.value, ver_before.value);
    EXPECT_EQ(result.schema_version, ver_after.value);
}

TEST_F(InPlaceSchemaMigratorTest, Apply_AddedColumnsListIsCorrect) {
    auto from = makeSchema("events", {"id", "type"});
    auto to   = makeSchema("events", {"id", "type", "payload", "ts"});

    seedSchema("events", from);

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("events", from, to, *schema_, *version_);

    ASSERT_TRUE(result.success) << result.error_message;
    ASSERT_EQ(result.added_columns.size(), 2u);

    // Both "payload" and "ts" should be reported as added
    bool has_payload = false, has_ts = false;
    for (const auto& col : result.added_columns) {
        if (col == "payload") has_payload = true;
        if (col == "ts")      has_ts      = true;
    }
    EXPECT_TRUE(has_payload);
    EXPECT_TRUE(has_ts);
}

// ============================================================================
// Phase 3: Error paths
// ============================================================================

TEST_F(InPlaceSchemaMigratorTest, Apply_EmptyTableName_Fails) {
    auto from = makeSchema("t", {"id"});
    auto to   = makeSchema("t", {"id", "x"});

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("", from, to, *schema_, *version_);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(InPlaceSchemaMigratorTest, Apply_NonAdditive_StrictMode_Fails) {
    auto from = makeSchema("accts", {"id", "balance"});
    // Remove "balance" – destructive migration
    auto to   = makeSchema("accts", {"id", "new_col"});

    seedSchema("accts", from);

    InPlaceSchemaMigrator migrator;  // strict_additive = true (default)
    auto result = migrator.apply("accts", from, to, *schema_, *version_);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(InPlaceSchemaMigratorTest, Apply_NonAdditive_NonStrictMode_Proceeds) {
    // With strict_additive=false, a destructive migration is allowed through
    // (caller's responsibility to ensure safety).
    auto from = makeSchema("sessions", {"id", "token"});
    // to removes "token" but adds "jwt" – normally not additive
    auto to   = makeSchema("sessions", {"id", "jwt"});

    seedSchema("sessions", from);

    InPlaceSchemaMigrator::Config cfg;
    cfg.strict_additive = false;
    InPlaceSchemaMigrator migrator(cfg);

    auto result = migrator.apply("sessions", from, to, *schema_, *version_);
    // The migrator proceeds without the additive guard; SchemaManager decides
    EXPECT_TRUE(result.success) << result.error_message;
}

TEST_F(InPlaceSchemaMigratorTest, Apply_IdenticalSchemas_StrictMode_Fails) {
    auto from = makeSchema("logs", {"id", "message"});
    auto to   = makeSchema("logs", {"id", "message"});  // identical

    seedSchema("logs", from);

    InPlaceSchemaMigrator migrator;  // strict_additive = true
    auto result = migrator.apply("logs", from, to, *schema_, *version_);

    // isAdditiveMigration returns false for identical schemas → error
    EXPECT_FALSE(result.success);
}

// ============================================================================
// Phase 4: Version history correctness after in-place migration
// ============================================================================

TEST_F(InPlaceSchemaMigratorTest, VersionHistory_ContainsInPlaceEntry) {
    auto from = makeSchema("items", {"id", "name"});
    auto to   = makeSchema("items", {"id", "name", "qty"});

    seedSchema("items", from);

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("items", from, to, *schema_, *version_, "bot");
    ASSERT_TRUE(result.success) << result.error_message;

    auto history = version_->getChangeHistory("items");
    ASSERT_TRUE(history.ok);
    ASSERT_GE(history.value.size(), 2u);  // baseline + in-place migration

    const auto& last = history.value.back();
    EXPECT_EQ(last.author, "bot");
    EXPECT_NE(last.description.find("in-place"), std::string::npos);
}

// ============================================================================
// Phase 5: Idempotency guard via SchemaVersionManager::validateMigration
// ============================================================================

TEST_F(InPlaceSchemaMigratorTest, SecondApply_SameSchema_IsIdempotentError) {
    auto from = makeSchema("widgets", {"id"});
    auto to   = makeSchema("widgets", {"id", "weight"});

    seedSchema("widgets", from);

    InPlaceSchemaMigrator migrator;
    auto r1 = migrator.apply("widgets", from, to, *schema_, *version_, "bot");
    ASSERT_TRUE(r1.success) << r1.error_message;

    // Applying the same additive change again: from == to at this point
    // isAdditiveMigration(to, to) == false (no new columns) → error in strict mode
    auto r2 = migrator.apply("widgets", to, to, *schema_, *version_, "bot");
    EXPECT_FALSE(r2.success);
}

// ============================================================================
// Phase 6: Fresh table (empty from_schema) applied via apply()
// ============================================================================

TEST_F(InPlaceSchemaMigratorTest, Apply_FreshTable_EmptyFromSchema) {
    // No seed – this is a brand-new table that doesn't exist yet in the DB.
    SchemaManager::TableSchema from;   // empty
    from.name = "brand_new_table";
    auto to = makeSchema("brand_new_table", {"id", "value", "created_at"});

    InPlaceSchemaMigrator migrator;
    auto result = migrator.apply("brand_new_table", from, to, *schema_, *version_, "init");

    EXPECT_TRUE(result.success) << result.error_message;
    ASSERT_EQ(result.added_columns.size(), 3u);
    EXPECT_GT(result.schema_version, 0u);

    // Table should now be readable from SchemaManager
    auto tbl = schema_->getTable("brand_new_table");
    ASSERT_TRUE(tbl.has_value());
    EXPECT_EQ(tbl->properties.size(), 3u);
}

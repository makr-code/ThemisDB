// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Tests for SchemaVersionManager::validateMigration() (dry-run mode)

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

#include "metadata/schema_version_manager.h"
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class DryRunMigrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_dryrun_");
        cfg.enable_blobdb = false;

        db_      = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        idx_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
        schema_  = std::make_unique<SchemaManager>(*db_, idx_mgr_.get());
    }

    void TearDown() override {
        if (db_) {
          db_->close();
        }
    }

    /// Build a simple TableSchema with given name and column names
    SchemaManager::TableSchema makeSchema(
        const std::string& name,
        const std::vector<std::string>& columns) const
    {
        SchemaManager::TableSchema ts;
        ts.name = name;
        ts.type = "relational";
        for (const auto& c : columns) {
            SchemaManager::PropertyInfo p;
            p.name = c;
            p.type = "string";
            ts.properties.push_back(p);
        }
        return ts;
    }

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_mgr_;
    std::unique_ptr<SchemaManager>         schema_;
};

// ============================================================================
// validateMigration – empty/invalid inputs
// ============================================================================

TEST_F(DryRunMigrationTest, EmptyNameFails) {
    SchemaVersionManager svm(*db_, *schema_);
    auto schema = makeSchema("", {"id", "name"});
    auto result = svm.validateMigration("users", schema);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(DryRunMigrationTest, NoColumnsFails) {
    SchemaVersionManager svm(*db_, *schema_);
    auto schema = makeSchema("users", {});  // no columns
    auto result = svm.validateMigration("users", schema);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("column"), std::string::npos);
}

TEST_F(DryRunMigrationTest, DuplicateColumnFails) {
    SchemaVersionManager svm(*db_, *schema_);
    auto schema = makeSchema("users", {"id", "id"});  // duplicate
    auto result = svm.validateMigration("users", schema);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("duplicate"), std::string::npos);
}

// ============================================================================
// validateMigration – valid schema (no prior version)
// ============================================================================

TEST_F(DryRunMigrationTest, ValidSchemaNoVersionPasses) {
    SchemaVersionManager svm(*db_, *schema_);
    auto schema = makeSchema("orders", {"id", "user_id", "amount"});
    auto result = svm.validateMigration("orders", schema);
    EXPECT_TRUE(result.ok) << result.error_message;
    EXPECT_TRUE(result.value);
}

TEST_F(DryRunMigrationTest, ValidSchemaDoesNotPersist) {
    // validateMigration must NOT create a version record
    SchemaVersionManager svm(*db_, *schema_);
    auto schema = makeSchema("products", {"sku", "price"});
    svm.validateMigration("products", schema);

    // No version should exist after dry-run
    auto cur = svm.getCurrentVersion("products");
    EXPECT_FALSE(cur.ok);  // TABLE_NOT_FOUND because nothing was persisted
}

// ============================================================================
// validateMigration – schema identical to latest version fails
// ============================================================================

TEST_F(DryRunMigrationTest, IdenticalSchemaToCurrentVersionFails) {
    // First register and snapshot
    SchemaManager::TableSchema ts = makeSchema("items", {"id", "name"});
    schema_->setTableSchema("items", ts);

    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("items", "test", "initial");

    // Dry-run the identical schema
    auto result = svm.validateMigration("items", ts);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("identical"), std::string::npos);
}

TEST_F(DryRunMigrationTest, DifferentSchemaFromCurrentVersionPasses) {
    SchemaManager::TableSchema ts = makeSchema("items", {"id", "name"});
    schema_->setTableSchema("items", ts);

    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("items", "test", "initial");

    // Add a column → schema is different
    auto ts2 = makeSchema("items", {"id", "name", "price"});
    auto result = svm.validateMigration("items", ts2);
    EXPECT_TRUE(result.ok) << result.error_message;
}

// ============================================================================
// validateMigration – empty column name fails
// ============================================================================

TEST_F(DryRunMigrationTest, EmptyColumnNameFails) {
    SchemaVersionManager svm(*db_, *schema_);
    SchemaManager::TableSchema ts = makeSchema("tbl", {"id"});
    SchemaManager::PropertyInfo bad_col;
    bad_col.name = "";  // empty name
    bad_col.type = "string";
    ts.properties.push_back(bad_col);
    auto result = svm.validateMigration("tbl", ts);
    EXPECT_FALSE(result.ok);
}

// ============================================================================
// validateMigration does not affect getChangeHistory
// ============================================================================

TEST_F(DryRunMigrationTest, DryRunLeavesHistoryUnchanged) {
    SchemaManager::TableSchema ts = makeSchema("logs", {"id", "message"});
    schema_->setTableSchema("logs", ts);

    SchemaVersionManager svm(*db_, *schema_);
    svm.createSchemaVersion("logs", "admin", "v1");

    auto history_before = svm.getChangeHistory("logs");
    ASSERT_TRUE(history_before.ok);
    size_t count_before = history_before.value.size();

    // Dry-run
    auto different = makeSchema("logs", {"id", "message", "level"});
    svm.validateMigration("logs", different);

    auto history_after = svm.getChangeHistory("logs");
    ASSERT_TRUE(history_after.ok);
    EXPECT_EQ(history_after.value.size(), count_before);
}

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Unit + integration tests for SchemaMigrationTester.
// Covers the staging → production promotion workflow, built-in validations,
// user-defined test cases, and error paths.

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "updates/schema_migration_tester.h"
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
// Fixture: per-test staging directory + production DB
// ============================================================================

class SchemaMigrationTesterTest : public ::testing::Test {
protected:
    std::string staging_dir_ = {};
    std::string prod_db_path_ = {};

    std::unique_ptr<RocksDBWrapper>       prod_db_;
    std::unique_ptr<SecondaryIndexManager> prod_idx_;
    std::unique_ptr<SchemaManager>        prod_schema_;
    std::unique_ptr<SchemaVersionManager> prod_version_;

    void SetUp() override {
        staging_dir_  = makeTempDir("staging_test_");
        prod_db_path_ = makeTempDir("prod_db_test_");

        RocksDBWrapper::Config cfg;
        cfg.db_path = prod_db_path_;
        prod_db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(prod_db_->open());

        prod_idx_     = std::make_unique<SecondaryIndexManager>(*prod_db_);
        prod_schema_  = std::make_unique<SchemaManager>(*prod_db_, prod_idx_.get());
        prod_version_ = std::make_unique<SchemaVersionManager>(*prod_db_, *prod_schema_);
    }

    void TearDown() override {
        prod_version_.reset();
        prod_schema_.reset();
        prod_idx_.reset();
        prod_db_.reset();
        fs::remove_all(staging_dir_);
        fs::remove_all(prod_db_path_);
    }

    SchemaMigrationTester makeTester(bool cleanup_on_success = true,
                                     bool cleanup_on_failure = false) {
        SchemaMigrationTester::Config cfg;
        cfg.staging_directory       = staging_dir_;
        cfg.cleanup_staging_on_success = cleanup_on_success;
        cfg.cleanup_staging_on_failure = cleanup_on_failure;
        return SchemaMigrationTester(cfg);
    }
};

// ============================================================================
// Phase 1: Basic successful migration
// ============================================================================

TEST_F(SchemaMigrationTesterTest, SuccessfulMigration_AddColumn) {
    auto from = makeSchema("users", {"id", "name"});
    auto to   = makeSchema("users", {"id", "name", "email"});

    auto tester = makeTester();
    auto result = tester.testMigration("users", from, to);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_GT(result.test_results.size(), 0u);
    EXPECT_EQ(result.failedCount(), 0u);

    // Script should mention the new column
    EXPECT_FALSE(result.migration_script.empty());
    EXPECT_NE(result.migration_script.find("email"), std::string::npos);
}

TEST_F(SchemaMigrationTesterTest, SuccessfulMigration_NoBaselineSchema) {
    // from_schema is empty (fresh table)
    SchemaManager::TableSchema from;
    auto to = makeSchema("orders", {"order_id", "customer_id", "amount"});

    auto tester = makeTester();
    auto result = tester.testMigration("orders", from, to);

    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.failedCount(), 0u);
}

// ============================================================================
// Phase 2: Built-in validation failures
// ============================================================================

TEST_F(SchemaMigrationTesterTest, BuiltinValidation_DuplicateColumn_Fails) {
    auto from = makeSchema("items", {"id"});

    // Manually build a schema with duplicate columns
    SchemaManager::TableSchema to;
    to.name = "items";
    to.type = "relational";
    for (const std::string& col : {"id", "price", "price"}) {  // "price" duplicated
        SchemaManager::PropertyInfo p;
        p.name = col;
        p.type = "string";
        to.properties.push_back(p);
    }

    auto tester = makeTester(/*cleanup_on_success=*/true, /*cleanup_on_failure=*/true);
    auto result = tester.testMigration("items", from, to);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("price"), std::string::npos);

    // The 'no_duplicate_columns' test must be in the results and failed
    bool found = false;
    for (const auto& tr : result.test_results) {
        if (tr.name == "no_duplicate_columns") {
            found = true;
            EXPECT_FALSE(tr.passed);
        }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// Phase 3: User-defined test cases
// ============================================================================

TEST_F(SchemaMigrationTesterTest, UserTestCase_Pass) {
    auto from = makeSchema("products", {"id"});
    auto to   = makeSchema("products", {"id", "sku", "price"});

    auto tester = makeTester();
    tester.addTestCase({
        "check_sku_column",
        [](const SchemaManager& sm, const SchemaVersionManager&, std::string& err) {
            auto t = const_cast<SchemaManager&>(sm).getTable("products");
            if (!t) { err = "table 'products' not found"; return false; }
            for (const auto& p : t->properties)
                if (p.name == "sku") {
                  return true;
                }
            err = "column 'sku' not found";
            return false;
        }
    });

    auto result = tester.testMigration("products", from, to);
    EXPECT_TRUE(result.success) << result.error_message;

    bool found = false;
    for (const auto& tr : result.test_results)
        if (tr.name == "check_sku_column") { found = true; EXPECT_TRUE(tr.passed); }
    EXPECT_TRUE(found);
}

TEST_F(SchemaMigrationTesterTest, UserTestCase_Fail_BlocksPromotion) {
    auto from = makeSchema("accounts", {"id"});
    auto to   = makeSchema("accounts", {"id", "balance"});

    auto tester = makeTester(true, true);
    tester.addTestCase({
        "always_fail",
        [](const SchemaManager&, const SchemaVersionManager&, std::string& err) {
            err = "intentional failure";
            return false;
        }
    });

    auto result = tester.testMigration("accounts", from, to);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("intentional failure"), std::string::npos);
}

TEST_F(SchemaMigrationTesterTest, UserTestCase_Exception_ReportedAsFailure) {
    auto from = makeSchema("sessions", {"id"});
    auto to   = makeSchema("sessions", {"id", "token"});

    auto tester = makeTester(true, true);
    tester.addTestCase({
        "throwing_test",
        [](const SchemaManager&, const SchemaVersionManager&, std::string&) -> bool {
            throw std::runtime_error("boom");
        }
    });

    auto result = tester.testMigration("sessions", from, to);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("boom"), std::string::npos);
}

// ============================================================================
// Phase 4: passedCount / failedCount helpers
// ============================================================================

TEST_F(SchemaMigrationTesterTest, CountHelpers) {
    auto from = makeSchema("logs", {"id"});
    auto to   = makeSchema("logs", {"id", "message"});

    auto tester = makeTester();
    auto result = tester.testMigration("logs", from, to);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.passedCount() + result.failedCount(), result.test_results.size());
    EXPECT_EQ(result.failedCount(), 0u);
    EXPECT_GT(result.passedCount(), 0u);
}

// ============================================================================
// Phase 5: promoteToProduction
// ============================================================================

TEST_F(SchemaMigrationTesterTest, PromoteToProduction_AfterSuccess) {
    auto from = makeSchema("widgets", {"id", "name"});
    auto to   = makeSchema("widgets", {"id", "name", "weight"});

    // Seed production with 'from' schema
    ASSERT_TRUE(prod_schema_->setTableSchema("widgets", from));
    prod_version_->createSchemaVersion("widgets", "test", "initial");

    auto tester = makeTester();
    auto result = tester.testMigration("widgets", from, to);
    ASSERT_TRUE(result.success) << result.error_message;

    bool promoted = tester.promoteToProduction(
        result, *prod_db_, *prod_schema_, *prod_version_, "widgets", to, "ci-bot");
    EXPECT_TRUE(promoted);

    // Verify production now has the new schema
    auto prod_tbl = prod_schema_->getTable("widgets");
    ASSERT_TRUE(prod_tbl.has_value());
    bool weight_found = false;
    for (const auto& p : prod_tbl->properties)
        if (p.name == "weight") { weight_found = true; break; }
    EXPECT_TRUE(weight_found);

    // A new version should have been created
    auto cur_ver = prod_version_->getCurrentVersion("widgets");
    EXPECT_TRUE(cur_ver.ok);
    EXPECT_GE(cur_ver.value, 2u);  // baseline(1) + promoted(2)
}

TEST_F(SchemaMigrationTesterTest, PromoteToProduction_RefusedOnFailedResult) {
    auto from = makeSchema("events", {"id"});
    auto to   = makeSchema("events", {"id", "type"});

    auto tester = makeTester();
    tester.addTestCase({
        "always_fail",
        [](const SchemaManager&, const SchemaVersionManager&, std::string& err) {
            err = "block";
            return false;
        }
    });

    auto result = tester.testMigration("events", from, to);
    ASSERT_FALSE(result.success);

    bool promoted = tester.promoteToProduction(
        result, *prod_db_, *prod_schema_, *prod_version_, "events", to);
    EXPECT_FALSE(promoted);
}

// ============================================================================
// Phase 6: Staging cleanup behaviour
// ============================================================================

TEST_F(SchemaMigrationTesterTest, StagingDir_RemovedOnSuccess_WhenConfigured) {
    auto from = makeSchema("t1", {"id"});
    auto to   = makeSchema("t1", {"id", "value"});

    SchemaMigrationTester::Config cfg;
    cfg.staging_directory          = staging_dir_;
    cfg.cleanup_staging_on_success = true;
    SchemaMigrationTester tester(cfg);

    auto result = tester.testMigration("t1", from, to);
    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.staging_db_path.empty());  // cleared when cleaned up
}

TEST_F(SchemaMigrationTesterTest, StagingDir_KeptOnSuccess_WhenConfigured) {
    auto from = makeSchema("t2", {"id"});
    auto to   = makeSchema("t2", {"id", "value"});

    SchemaMigrationTester::Config cfg;
    cfg.staging_directory          = staging_dir_;
    cfg.cleanup_staging_on_success = false;
    SchemaMigrationTester tester(cfg);

    auto result = tester.testMigration("t2", from, to);
    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.staging_db_path.empty());
    EXPECT_TRUE(fs::exists(result.staging_db_path));
}

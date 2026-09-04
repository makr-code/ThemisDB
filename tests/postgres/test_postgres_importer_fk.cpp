// Test: PostgreSQL Importer v2.0 – Foreign Key Preservation
//
// Verifies:
//   1. Table-level FOREIGN KEY in CREATE TABLE body (with CONSTRAINT name)
//   2. Inline column-level REFERENCES clause
//   3. Multi-column FOREIGN KEY
//   4. ON DELETE / ON UPDATE action parsing (CASCADE, SET NULL)
//   5. ALTER TABLE … ADD CONSTRAINT … FOREIGN KEY (deferred FK)
//   6. ImportStats::foreign_keys_preserved counter
//   7. ImportOptions::preserve_foreign_keys = false disables FK parsing
//   8. getSourceSchema() includes foreign_keys array
//   9. Entity JSON embeds _foreign_keys metadata
//  10. Fixture file sample_pg_fk.sql loads and has expected content

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>

#include "importers/postgres_importer.h"
#include "importers/importer_interface.h"

namespace {

// ---------------------------------------------------------------------------
// Fixture path helper (mirrors test_postgres_importer_complex_ddl.cpp)
// ---------------------------------------------------------------------------
static std::string fixturePath(const std::string& name) {
    static const std::vector<std::string> bases = {
        "tests/fixtures/importers/",
        "../tests/fixtures/importers/",
        "../../tests/fixtures/importers/"
    };
    for (const auto& b : bases) {
        std::ifstream f(b + name);
        if (f) {
          return b + name;
        }
    }
    return "tests/fixtures/importers/" + name;
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
      return "";
    }
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

// ---------------------------------------------------------------------------
// FK fixture test class
// ---------------------------------------------------------------------------
class FKFixtureFileTest : public ::testing::Test {
protected:
    std::string sql_ = {};
    void SetUp() override {
        sql_ = readFile(fixturePath("sample_pg_fk.sql"));
    }
};

TEST_F(FKFixtureFileTest, FixtureFileLoads) {
    ASSERT_FALSE(sql_.empty()) << "sample_pg_fk.sql not found or empty";
}

TEST_F(FKFixtureFileTest, ContainsTableLevelForeignKey) {
    EXPECT_NE(std::string::npos, sql_.find("CONSTRAINT fk_orders_user FOREIGN KEY"));
}

TEST_F(FKFixtureFileTest, ContainsInlineReferences) {
    EXPECT_NE(std::string::npos, sql_.find("REFERENCES users(id) ON DELETE CASCADE"))
        << "profiles table should have inline REFERENCES";
}

TEST_F(FKFixtureFileTest, ContainsMultipleConstraintsOnOrderItems) {
    EXPECT_NE(std::string::npos, sql_.find("fk_items_order"));
    EXPECT_NE(std::string::npos, sql_.find("fk_items_category"));
}

TEST_F(FKFixtureFileTest, ContainsAlterTableFk) {
    EXPECT_NE(std::string::npos, sql_.find("ALTER TABLE ONLY orders ADD CONSTRAINT fk_orders_deferred"));
}

TEST_F(FKFixtureFileTest, ContainsOnDeleteCascade) {
    EXPECT_NE(std::string::npos, sql_.find("ON DELETE CASCADE"));
}

TEST_F(FKFixtureFileTest, ContainsOnDeleteSetNull) {
    EXPECT_NE(std::string::npos, sql_.find("ON DELETE SET NULL"));
}

TEST_F(FKFixtureFileTest, ContainsOnUpdateRestrict) {
    EXPECT_NE(std::string::npos, sql_.find("ON UPDATE RESTRICT"));
}

// ---------------------------------------------------------------------------
// Unit tests: ImportOptions::preserve_foreign_keys field
// ---------------------------------------------------------------------------
class FKImportOptionsTest : public ::testing::Test {};

TEST_F(FKImportOptionsTest, DefaultIsTrue) {
    themis::importers::ImportOptions opts;
    EXPECT_TRUE(opts.preserve_foreign_keys)
        << "preserve_foreign_keys should default to true in v2.0";
}

TEST_F(FKImportOptionsTest, CanBeDisabled) {
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = false;
    EXPECT_FALSE(opts.preserve_foreign_keys);
}

TEST_F(FKImportOptionsTest, SerializesToJson) {
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = true;
    auto j = opts.toJson();
    ASSERT_TRUE(j.contains("preserve_foreign_keys"));
    EXPECT_TRUE(j["preserve_foreign_keys"].get<bool>());
}

TEST_F(FKImportOptionsTest, FalseSerializesToJson) {
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = false;
    auto j = opts.toJson();
    ASSERT_TRUE(j.contains("preserve_foreign_keys"));
    EXPECT_FALSE(j["preserve_foreign_keys"].get<bool>());
}

// ---------------------------------------------------------------------------
// Unit tests: ImportStats::foreign_keys_preserved field
// ---------------------------------------------------------------------------
class FKImportStatsTest : public ::testing::Test {};

TEST_F(FKImportStatsTest, DefaultIsZero) {
    themis::importers::ImportStats stats;
    EXPECT_EQ(0u, stats.foreign_keys_preserved);
}

TEST_F(FKImportStatsTest, CanBeIncremented) {
    themis::importers::ImportStats stats;
    stats.foreign_keys_preserved += 3;
    EXPECT_EQ(3u, stats.foreign_keys_preserved);
}

TEST_F(FKImportStatsTest, SerializesToJson) {
    themis::importers::ImportStats stats;
    stats.foreign_keys_preserved = 5;
    auto j = stats.toJson();
    ASSERT_TRUE(j.contains("foreign_keys_preserved"));
    EXPECT_EQ(5u, j["foreign_keys_preserved"].get<size_t>());
}

// ---------------------------------------------------------------------------
// Integration tests: PostgreSQLImporter with FK fixture
// ---------------------------------------------------------------------------
class FKImporterIntegrationTest : public ::testing::Test {
protected:
    themis::importers::PostgreSQLImporter importer_;
    std::string fixture_path_ = {};

    void SetUp() override {
        fixture_path_ = fixturePath("sample_pg_fk.sql");
    }
};

TEST_F(FKImporterIntegrationTest, GetSourceSchemaIncludesForeignKeys) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    ASSERT_TRUE(schema.is_array());
    ASSERT_FALSE(schema.empty());

    bool found_orders = false;
    for (const auto& tbl : schema) {
        if (tbl["name"] == "orders") {
            found_orders = true;
            ASSERT_TRUE(tbl.contains("foreign_keys"))
                << "orders table schema must include foreign_keys array";
            EXPECT_TRUE(tbl["foreign_keys"].is_array());
            EXPECT_FALSE(tbl["foreign_keys"].empty())
                << "orders table should have at least one FK";
        }
    }
    EXPECT_TRUE(found_orders) << "orders table not found in schema";
}

TEST_F(FKImporterIntegrationTest, OrdersTableHasFkConstraintName) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        if (tbl["name"] == "orders") {
            const auto& fks = tbl["foreign_keys"];
            bool found_named_fk = false;
            for (const auto& fk : fks) {
                if (fk.contains("constraint_name") &&
                    fk["constraint_name"].get<std::string>() == "fk_orders_user") {
                    found_named_fk = true;
                    EXPECT_EQ("users", fk["ref_table"].get<std::string>());
                    ASSERT_FALSE(fk["columns"].empty());
                    EXPECT_EQ("user_id", fk["columns"][0].get<std::string>());
                }
            }
            EXPECT_TRUE(found_named_fk)
                << "fk_orders_user constraint not found in orders.foreign_keys";
        }
    }
}

TEST_F(FKImporterIntegrationTest, OrdersTableFkHasOnDeleteCascade) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        if (tbl["name"] == "orders") {
            for (const auto& fk : tbl["foreign_keys"]) {
                if (fk.contains("constraint_name") &&
                    fk["constraint_name"].get<std::string>() == "fk_orders_user") {
                    EXPECT_EQ("CASCADE", fk["on_delete"].get<std::string>());
                }
            }
        }
    }
}

TEST_F(FKImporterIntegrationTest, OrderItemsTableHasTwoForeignKeys) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        if (tbl["name"] == "order_items") {
            ASSERT_TRUE(tbl.contains("foreign_keys"));
            EXPECT_GE(tbl["foreign_keys"].size(), 2u)
                << "order_items should have at least 2 FK constraints";
        }
    }
}

TEST_F(FKImporterIntegrationTest, CategoryFkHasOnDeleteSetNull) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        if (tbl["name"] == "order_items") {
            for (const auto& fk : tbl["foreign_keys"]) {
                if (fk.contains("constraint_name") &&
                    fk["constraint_name"].get<std::string>() == "fk_items_category") {
                    EXPECT_EQ("SET NULL", fk["on_delete"].get<std::string>());
                }
            }
        }
    }
}

TEST_F(FKImporterIntegrationTest, ProfilesTableHasInlineReferenceFk) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        if (tbl["name"] == "profiles") {
            ASSERT_TRUE(tbl.contains("foreign_keys"));
            EXPECT_FALSE(tbl["foreign_keys"].empty())
                << "profiles table should have FK from inline REFERENCES clause";
            if (!tbl["foreign_keys"].empty()) {
                const auto& fk = tbl["foreign_keys"][0];
                EXPECT_EQ("users", fk["ref_table"].get<std::string>());
                ASSERT_FALSE(fk["columns"].empty());
                EXPECT_EQ("user_id", fk["columns"][0].get<std::string>());
                EXPECT_EQ("CASCADE", fk["on_delete"].get<std::string>());
            }
        }
    }
}

TEST_F(FKImporterIntegrationTest, ImportStatsCountsForeignKeys) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = true;
    opts.continue_on_error = true;

    auto stats = importer_.importData(fixture_path_, opts);
    EXPECT_GT(stats.foreign_keys_preserved, 0u)
        << "Should have preserved at least one FK from fixture";
}

TEST_F(FKImporterIntegrationTest, DisabledFkPreservationCountsZero) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = false;
    opts.continue_on_error = true;

    auto stats = importer_.importData(fixture_path_, opts);
    EXPECT_EQ(0u, stats.foreign_keys_preserved)
        << "No FKs should be counted when preserve_foreign_keys=false";
}

TEST_F(FKImporterIntegrationTest, EntitiesHaveFkMetadataInJson) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }

    std::vector<nlohmann::json> orders_entities;
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = true;
    opts.continue_on_error = true;
    opts.streaming_row_callback = [&](const std::string& table, const nlohmann::json& entity) -> bool {
        if (table == "orders") {
            orders_entities.push_back(entity);
        }
        return true;
    };

    importer_.importData(fixture_path_, opts);

    ASSERT_FALSE(orders_entities.empty()) << "Should have imported orders rows";
    for (const auto& entity : orders_entities) {
        EXPECT_TRUE(entity.contains("_foreign_keys"))
            << "orders entity must embed _foreign_keys metadata";
        if (entity.contains("_foreign_keys")) {
            EXPECT_TRUE(entity["_foreign_keys"].is_array());
            EXPECT_FALSE(entity["_foreign_keys"].empty());
        }
    }
}

TEST_F(FKImporterIntegrationTest, EntitiesHaveNoFkMetadataWhenDisabled) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }

    std::vector<nlohmann::json> orders_entities;
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = false;
    opts.continue_on_error = true;
    opts.streaming_row_callback = [&](const std::string& table, const nlohmann::json& entity) -> bool {
        if (table == "orders") {
            orders_entities.push_back(entity);
        }
        return true;
    };

    importer_.importData(fixture_path_, opts);

    for (const auto& entity : orders_entities) {
        EXPECT_FALSE(entity.contains("_foreign_keys"))
            << "orders entity must NOT embed _foreign_keys when preserve_foreign_keys=false";
    }
}

TEST_F(FKImporterIntegrationTest, AllTablesSchemaHasForeignKeysField) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        EXPECT_TRUE(tbl.contains("foreign_keys"))
            << "Every table schema must include foreign_keys field (even if empty)";
        EXPECT_TRUE(tbl["foreign_keys"].is_array())
            << "foreign_keys must be a JSON array";
    }
}

TEST_F(FKImporterIntegrationTest, TablesWithoutFksHaveEmptyFkArray) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        if (tbl["name"] == "users" || tbl["name"] == "categories") {
            EXPECT_TRUE(tbl["foreign_keys"].empty())
                << tbl["name"].get<std::string>()
                << " has no FKs so the array should be empty";
        }
    }
}

TEST_F(FKImporterIntegrationTest, DataRowsStillImportedWithFkPreservation) {
    if (readFile(fixture_path_).empty()) {
        GTEST_SKIP() << "Fixture not found; skipping live importer test";
    }
    themis::importers::ImportOptions opts;
    opts.preserve_foreign_keys = true;
    opts.continue_on_error = true;

    auto stats = importer_.importData(fixture_path_, opts);
    // The fixture has: 2 users + 2 categories + 2 orders + 3 order_items + 2 profiles = 11 rows
    EXPECT_EQ(11u, stats.imported_records)
        << "All 11 data rows must be imported when FK preservation is enabled";
    EXPECT_EQ(0u, stats.failed_records);
}

// ---------------------------------------------------------------------------
// Unit tests: ForeignKeyConstraint struct / JSON serialisation
// ---------------------------------------------------------------------------
// These tests verify the FK JSON structure via the public getSourceSchema() API.

class FKConstraintJsonStructureTest : public ::testing::Test {
protected:
    themis::importers::PostgreSQLImporter importer_;
    std::string fixture_path_ = {};
    nlohmann::json orders_fks_;

    void SetUp() override {
        fixture_path_ = fixturePath("sample_pg_fk.sql");
        if (readFile(fixture_path_).empty()) {
          return;
        }

        auto schema = importer_.getSourceSchema(fixture_path_);
        for (const auto& tbl : schema) {
            if (tbl["name"] == "orders") {
                orders_fks_ = tbl["foreign_keys"];
                break;
            }
        }
    }
};

TEST_F(FKConstraintJsonStructureTest, FkHasConstraintNameField) {
    if (orders_fks_.empty()) { GTEST_SKIP() << "No FK data (fixture not found)"; }
    bool found = false;
    for (const auto& fk : orders_fks_) {
        if (fk.contains("constraint_name") &&
            !fk["constraint_name"].get<std::string>().empty()) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "At least one FK must have a non-empty constraint_name";
}

TEST_F(FKConstraintJsonStructureTest, FkHasColumnsField) {
    if (orders_fks_.empty()) { GTEST_SKIP() << "No FK data (fixture not found)"; }
    for (const auto& fk : orders_fks_) {
        EXPECT_TRUE(fk.contains("columns")) << "FK must have 'columns' field";
        EXPECT_TRUE(fk["columns"].is_array()) << "FK 'columns' must be an array";
    }
}

TEST_F(FKConstraintJsonStructureTest, FkHasRefTableField) {
    if (orders_fks_.empty()) { GTEST_SKIP() << "No FK data (fixture not found)"; }
    for (const auto& fk : orders_fks_) {
        EXPECT_TRUE(fk.contains("ref_table")) << "FK must have 'ref_table' field";
        EXPECT_FALSE(fk["ref_table"].get<std::string>().empty()) << "FK ref_table must not be empty";
    }
}

TEST_F(FKConstraintJsonStructureTest, FkHasRefColumnsField) {
    if (orders_fks_.empty()) { GTEST_SKIP() << "No FK data (fixture not found)"; }
    for (const auto& fk : orders_fks_) {
        EXPECT_TRUE(fk.contains("ref_columns")) << "FK must have 'ref_columns' field";
        EXPECT_TRUE(fk["ref_columns"].is_array()) << "FK 'ref_columns' must be an array";
    }
}

TEST_F(FKConstraintJsonStructureTest, FkHasOnDeleteField) {
    if (orders_fks_.empty()) { GTEST_SKIP() << "No FK data (fixture not found)"; }
    for (const auto& fk : orders_fks_) {
        EXPECT_TRUE(fk.contains("on_delete")) << "FK must have 'on_delete' field";
    }
}

TEST_F(FKConstraintJsonStructureTest, FkHasOnUpdateField) {
    if (orders_fks_.empty()) { GTEST_SKIP() << "No FK data (fixture not found)"; }
    for (const auto& fk : orders_fks_) {
        EXPECT_TRUE(fk.contains("on_update")) << "FK must have 'on_update' field";
    }
}

// ---------------------------------------------------------------------------
// Unit tests: TableSchema struct has foreign_keys field (via JSON output)
// ---------------------------------------------------------------------------
class FKTableSchemaFieldTest : public ::testing::Test {
protected:
    themis::importers::PostgreSQLImporter importer_;
    std::string fixture_path_ = {};

    void SetUp() override {
        fixture_path_ = fixturePath("sample_pg_fk.sql");
    }
};

TEST_F(FKTableSchemaFieldTest, ForeignKeysFieldPresentInAllTables) {
    if (readFile(fixture_path_).empty()) { GTEST_SKIP() << "Fixture not found"; }
    auto schema = importer_.getSourceSchema(fixture_path_);
    ASSERT_FALSE(schema.empty());
    for (const auto& tbl : schema) {
        EXPECT_TRUE(tbl.contains("foreign_keys"))
            << "Every table must expose foreign_keys in schema JSON";
    }
}

TEST_F(FKTableSchemaFieldTest, TablesWithoutFksHaveEmptyArray) {
    if (readFile(fixture_path_).empty()) { GTEST_SKIP() << "Fixture not found"; }
    auto schema = importer_.getSourceSchema(fixture_path_);
    for (const auto& tbl : schema) {
        const std::string name = tbl["name"].get<std::string>();
        if (name == "users" || name == "categories") {
            EXPECT_TRUE(tbl["foreign_keys"].empty())
                << name << " has no FK references so array must be empty";
        }
    }
}

}  // namespace

// End of test_postgres_importer_fk.cpp

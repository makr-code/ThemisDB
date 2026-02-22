// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Unit tests for SchemaVersionManager::generateMigrationScript()

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>

#include "metadata/schema_version_manager.h"
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;

namespace {

std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

} // namespace

class MigrationScriptTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_migration_script_");
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        idx_       = std::make_unique<SecondaryIndexManager>(*db_);
        schema_    = std::make_unique<SchemaManager>(*db_, idx_.get());
        version_   = std::make_unique<SchemaVersionManager>(*db_, *schema_);
    }

    void TearDown() override {
        version_.reset();
        schema_.reset();
        idx_.reset();
        if (db_) db_->close();
    }

    /// Build a TableSchema with the given columns (all type "string", nullable).
    SchemaManager::TableSchema makeSchema(
        const std::string& table,
        const std::vector<std::string>& cols,
        const std::string& type = "string",
        bool nullable = true) const
    {
        SchemaManager::TableSchema ts;
        ts.name = table;
        ts.type = "relational";
        for (const auto& c : cols) {
            SchemaManager::PropertyInfo p;
            p.name     = c;
            p.type     = type;
            p.nullable = nullable;
            ts.properties.push_back(p);
        }
        return ts;
    }

    /// Register schema in SchemaManager and create a version snapshot.
    uint64_t snapshot(const std::string& table,
                      const SchemaManager::TableSchema& ts,
                      const std::string& author = "test",
                      const std::string& desc   = "") {
        EXPECT_TRUE(schema_->setTableSchema(table, ts));
        auto r = version_->createSchemaVersion(table, author, desc);
        EXPECT_TRUE(r.ok) << r.error_message;
        return r.value;
    }

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<SchemaManager>         schema_;
    std::unique_ptr<SchemaVersionManager>  version_;
};

// ============================================================================
// Error cases
// ============================================================================

TEST_F(MigrationScriptTest, NonexistentFromVersionFails) {
    const std::string TABLE = "t_nofrom";
    auto ts = makeSchema(TABLE, {"id"});
    snapshot(TABLE, ts, "a", "v1");

    auto r = version_->generateMigrationScript(TABLE, 99u, 1u);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::VERSION_NOT_FOUND);
}

TEST_F(MigrationScriptTest, NonexistentToVersionFails) {
    const std::string TABLE = "t_noto";
    auto ts = makeSchema(TABLE, {"id"});
    snapshot(TABLE, ts, "a", "v1");

    auto r = version_->generateMigrationScript(TABLE, 1u, 99u);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error, VersionErrorCode::VERSION_NOT_FOUND);
}

TEST_F(MigrationScriptTest, EmptyTableNameFails) {
    auto r = version_->generateMigrationScript("", 1u, 2u);
    EXPECT_FALSE(r.ok);
}

// ============================================================================
// ADD COLUMN
// ============================================================================

TEST_F(MigrationScriptTest, AddColumnProducesAddStatement) {
    const std::string TABLE = "t_add";

    auto v1_schema = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1_schema, "a", "initial");

    auto v2_schema = makeSchema(TABLE, {"id", "name"});
    snapshot(TABLE, v2_schema, "b", "add name");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    const std::string& script = r.value;
    EXPECT_NE(script.find("ADD COLUMN name"), std::string::npos);
    EXPECT_NE(script.find("VARCHAR"),         std::string::npos);
    // Script header comment
    EXPECT_NE(script.find("Migration"), std::string::npos);
    EXPECT_NE(script.find(TABLE),       std::string::npos);
}

TEST_F(MigrationScriptTest, AddMultipleColumns) {
    const std::string TABLE = "t_addmulti";

    auto v1 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1, "a", "v1");

    auto v2 = makeSchema(TABLE, {"id", "email", "phone"});
    snapshot(TABLE, v2, "b", "v2");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_NE(r.value.find("ADD COLUMN email"), std::string::npos);
    EXPECT_NE(r.value.find("ADD COLUMN phone"), std::string::npos);
}

TEST_F(MigrationScriptTest, NotNullColumnHasNotNullClause) {
    const std::string TABLE = "t_notnull";

    auto v1 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1, "a", "v1");

    // Build a schema with a non-nullable new column
    SchemaManager::TableSchema v2_schema;
    v2_schema.name = TABLE;
    v2_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name     = "id";
        p.type     = "string";
        p.nullable = true;
        v2_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name     = "required_col";
        p.type     = "string";
        p.nullable = false;
        v2_schema.properties.push_back(p);
    }
    snapshot(TABLE, v2_schema, "b", "add required_col");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;
    EXPECT_NE(r.value.find("NOT NULL"), std::string::npos);
}

// ============================================================================
// DROP COLUMN
// ============================================================================

TEST_F(MigrationScriptTest, DropColumnProducesDropStatement) {
    const std::string TABLE = "t_drop";

    auto v1 = makeSchema(TABLE, {"id", "obsolete"});
    snapshot(TABLE, v1, "a", "v1");

    auto v2 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v2, "b", "remove obsolete");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_NE(r.value.find("DROP COLUMN obsolete"), std::string::npos);
}

TEST_F(MigrationScriptTest, DropMultipleColumns) {
    const std::string TABLE = "t_dropmulti";

    auto v1 = makeSchema(TABLE, {"id", "col_a", "col_b"});
    snapshot(TABLE, v1, "a", "v1");

    auto v2 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v2, "b", "v2");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_NE(r.value.find("DROP COLUMN col_a"), std::string::npos);
    EXPECT_NE(r.value.find("DROP COLUMN col_b"), std::string::npos);
}

// ============================================================================
// ALTER COLUMN (type change)
// ============================================================================

TEST_F(MigrationScriptTest, TypeChangeProducesAlterStatement) {
    const std::string TABLE = "t_alter";

    // v1: column "amount" as string
    SchemaManager::TableSchema v1_schema;
    v1_schema.name = TABLE;
    v1_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";     p.type = "string"; p.nullable = true;
        v1_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "amount"; p.type = "string"; p.nullable = true;
        v1_schema.properties.push_back(p);
    }
    snapshot(TABLE, v1_schema, "a", "v1");

    // v2: column "amount" as double
    SchemaManager::TableSchema v2_schema;
    v2_schema.name = TABLE;
    v2_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";     p.type = "string"; p.nullable = true;
        v2_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "amount"; p.type = "double"; p.nullable = true;
        v2_schema.properties.push_back(p);
    }
    snapshot(TABLE, v2_schema, "b", "v2 – amount is now double");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_NE(r.value.find("ALTER COLUMN amount"), std::string::npos);
    EXPECT_NE(r.value.find("DOUBLE PRECISION"),    std::string::npos);
}

// ============================================================================
// ALTER COLUMN – nullability change only
// ============================================================================

TEST_F(MigrationScriptTest, NullabilityChangeProducesSetNotNull) {
    const std::string TABLE = "t_nullable";

    // v1: column "email" nullable
    SchemaManager::TableSchema v1_schema;
    v1_schema.name = TABLE; v1_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";    p.type = "string"; p.nullable = true;
        v1_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "email"; p.type = "string"; p.nullable = true;
        v1_schema.properties.push_back(p);
    }
    snapshot(TABLE, v1_schema, "a", "v1");

    // v2: column "email" not nullable
    SchemaManager::TableSchema v2_schema;
    v2_schema.name = TABLE; v2_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";    p.type = "string"; p.nullable = true;
        v2_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "email"; p.type = "string"; p.nullable = false;
        v2_schema.properties.push_back(p);
    }
    snapshot(TABLE, v2_schema, "b", "email is now required");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    // Separate SET NOT NULL statement expected, no TYPE statement
    EXPECT_NE(r.value.find("ALTER COLUMN email SET NOT NULL"), std::string::npos);
    EXPECT_EQ(r.value.find("ALTER COLUMN email TYPE"),         std::string::npos);
}

TEST_F(MigrationScriptTest, NullabilityRelaxedProducesDropNotNull) {
    const std::string TABLE = "t_dropnn";

    // v1: column "code" NOT NULL
    SchemaManager::TableSchema v1_schema;
    v1_schema.name = TABLE; v1_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";   p.type = "string"; p.nullable = true;
        v1_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "code"; p.type = "string"; p.nullable = false;
        v1_schema.properties.push_back(p);
    }
    snapshot(TABLE, v1_schema, "a", "v1");

    // v2: column "code" nullable
    SchemaManager::TableSchema v2_schema;
    v2_schema.name = TABLE; v2_schema.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";   p.type = "string"; p.nullable = true;
        v2_schema.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "code"; p.type = "string"; p.nullable = true;
        v2_schema.properties.push_back(p);
    }
    snapshot(TABLE, v2_schema, "b", "code is now optional");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_NE(r.value.find("ALTER COLUMN code DROP NOT NULL"), std::string::npos);
}

// ============================================================================
// No-op diff (same schema at both ends of a rollback)
// ============================================================================

TEST_F(MigrationScriptTest, IdenticalVersionsProducesHeaderOnly) {
    const std::string TABLE = "t_noop";

    // Create v1
    auto v1 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1, "a", "v1");

    // Rollback creates v2 with identical snapshot
    auto rb = version_->rollbackToVersion(TABLE, 1u, "ops");
    ASSERT_TRUE(rb.ok);

    // v1 and v2 have the same column set → diff is empty
    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    // The script should contain only the header comment, no DDL statements
    EXPECT_NE(r.value.find("Migration"), std::string::npos);
    EXPECT_EQ(r.value.find("ADD COLUMN"),   std::string::npos);
    EXPECT_EQ(r.value.find("DROP COLUMN"),  std::string::npos);
    EXPECT_EQ(r.value.find("ALTER COLUMN"), std::string::npos);
}

// ============================================================================
// Type mapping
// ============================================================================

TEST_F(MigrationScriptTest, TypeMappingInteger) {
    const std::string TABLE = "t_int";
    auto v1 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1, "a", "v1");

    SchemaManager::TableSchema v2;
    v2.name = TABLE; v2.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";    p.type = "string";  p.nullable = true;
        v2.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "count"; p.type = "integer"; p.nullable = true;
        v2.properties.push_back(p);
    }
    snapshot(TABLE, v2, "b", "add count");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;
    EXPECT_NE(r.value.find("INTEGER"), std::string::npos);
}

TEST_F(MigrationScriptTest, TypeMappingBoolean) {
    const std::string TABLE = "t_bool";
    auto v1 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1, "a", "v1");

    SchemaManager::TableSchema v2;
    v2.name = TABLE; v2.type = "relational";
    {
        SchemaManager::PropertyInfo p;
        p.name = "id";      p.type = "string";  p.nullable = true;
        v2.properties.push_back(p);
    }
    {
        SchemaManager::PropertyInfo p;
        p.name = "active";  p.type = "boolean"; p.nullable = true;
        v2.properties.push_back(p);
    }
    snapshot(TABLE, v2, "b", "add active flag");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;
    EXPECT_NE(r.value.find("BOOLEAN"), std::string::npos);
}

// ============================================================================
// Combined add + drop in one migration
// ============================================================================

TEST_F(MigrationScriptTest, CombinedAddAndDrop) {
    const std::string TABLE = "t_combined";

    auto v1 = makeSchema(TABLE, {"id", "legacy_col"});
    snapshot(TABLE, v1, "a", "v1");

    auto v2 = makeSchema(TABLE, {"id", "new_col"});
    snapshot(TABLE, v2, "b", "v2");

    auto r = version_->generateMigrationScript(TABLE, 1u, 2u);
    ASSERT_TRUE(r.ok) << r.error_message;

    EXPECT_NE(r.value.find("ADD COLUMN new_col"),      std::string::npos);
    EXPECT_NE(r.value.find("DROP COLUMN legacy_col"),  std::string::npos);
}

// ============================================================================
// Reverse migration (downgrade script)
// ============================================================================

TEST_F(MigrationScriptTest, DowngradeMigrationScript) {
    const std::string TABLE = "t_downgrade";

    auto v1 = makeSchema(TABLE, {"id"});
    snapshot(TABLE, v1, "a", "v1");

    auto v2 = makeSchema(TABLE, {"id", "extra"});
    snapshot(TABLE, v2, "b", "v2 add extra");

    // Generate a reverse (v2→v1) script
    auto r = version_->generateMigrationScript(TABLE, 2u, 1u);
    ASSERT_TRUE(r.ok) << r.error_message;

    // Going backward means the column added in v2 is now "removed"
    EXPECT_NE(r.value.find("DROP COLUMN extra"), std::string::npos);
}

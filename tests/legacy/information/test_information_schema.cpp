// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

#include "metadata/information_schema.h"
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class InformationSchemaTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_is_");
        cfg.enable_blobdb = false;

        db_       = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        idx_mgr_  = std::make_unique<SecondaryIndexManager>(*db_);
        schema_   = std::make_unique<SchemaManager>(*db_, idx_mgr_.get());
    }

    void TearDown() override {
        if (db_) db_->close();
    }

    void insertRow(const std::string& table, const std::string& id,
                   BaseEntity::FieldMap fields)
    {
        BaseEntity entity = BaseEntity::fromFields(id, fields);
        db_->put(table + ":" + id, entity.serialize());
    }

    std::unique_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<SecondaryIndexManager>   idx_mgr_;
    std::unique_ptr<SchemaManager>           schema_;
};

// ============================================================================
// INFORMATION_SCHEMA.TABLES
// ============================================================================

TEST_F(InformationSchemaTest, GetTablesEmpty) {
    InformationSchema is(*schema_);
    auto tables = is.getTables();
    EXPECT_TRUE(tables.empty());
}

TEST_F(InformationSchemaTest, GetTablesOneTable) {
    insertRow("products", "p1", {{"name", std::string("Widget")}, {"price", double(9.99)}});
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto tables = is.getTables();

    ASSERT_EQ(tables.size(), 1u);
    EXPECT_EQ(tables[0].table_name,    "products");
    EXPECT_EQ(tables[0].table_catalog, "def");
    EXPECT_EQ(tables[0].table_schema,  "main");
    EXPECT_EQ(tables[0].table_type,    "BASE TABLE");
    EXPECT_EQ(tables[0].engine,        "ThemisDB");
    EXPECT_EQ(tables[0].row_count,     1u);
}

TEST_F(InformationSchemaTest, GetTablesMultiple) {
    insertRow("users",   "u1", {{"email", std::string("a@b.c")}});
    insertRow("orders",  "o1", {{"amount", double(1.0)}});
    insertRow("items",   "i1", {{"sku", std::string("ABC")}});
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto tables = is.getTables();

    EXPECT_EQ(tables.size(), 3u);

    std::set<std::string> names;
    for (const auto& t : tables) names.insert(t.table_name);
    EXPECT_TRUE(names.count("users"));
    EXPECT_TRUE(names.count("orders"));
    EXPECT_TRUE(names.count("items"));
}

// ============================================================================
// INFORMATION_SCHEMA.COLUMNS
// ============================================================================

TEST_F(InformationSchemaTest, GetColumnsEmpty) {
    InformationSchema is(*schema_);
    auto cols = is.getColumns();
    EXPECT_TRUE(cols.empty());
}

TEST_F(InformationSchemaTest, GetColumnsForTable) {
    insertRow("employees", "e1", {
        {"first_name", std::string("Jane")},
        {"salary",     double(50000.0)},
        {"active",     true},
    });
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto all_cols = is.getColumns();
    EXPECT_FALSE(all_cols.empty());

    // Filter for single table
    auto emp_cols = is.getColumns(std::string_view("employees"));
    EXPECT_FALSE(emp_cols.empty());

    // Check mandatory fields
    for (const auto& col : emp_cols) {
        EXPECT_EQ(col.table_catalog, "def");
        EXPECT_EQ(col.table_schema,  "main");
        EXPECT_EQ(col.table_name,    "employees");
        EXPECT_FALSE(col.column_name.empty());
        EXPECT_FALSE(col.data_type.empty());
        EXPECT_TRUE(col.is_nullable == "YES" || col.is_nullable == "NO");
        EXPECT_GE(col.ordinal_position, 1u);
    }
}

TEST_F(InformationSchemaTest, GetColumnsDataTypeMapping) {
    insertRow("types_test", "t1", {
        {"str_col",  std::string("hello")},
        {"int_col",  int64_t(42)},
        {"dbl_col",  double(3.14)},
        {"bool_col", true},
    });
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto cols = is.getColumns(std::string_view("types_test"));
    EXPECT_FALSE(cols.empty());

    std::map<std::string, std::string> type_map;
    for (const auto& col : cols) {
        type_map[col.column_name] = col.data_type;
    }

    EXPECT_EQ(type_map.at("str_col"),  "VARCHAR");
    EXPECT_EQ(type_map.at("int_col"),  "BIGINT");
    EXPECT_EQ(type_map.at("dbl_col"),  "DOUBLE");
    EXPECT_EQ(type_map.at("bool_col"), "BOOLEAN");
}

TEST_F(InformationSchemaTest, GetColumnsOrdinalPosition) {
    insertRow("ord_test", "o1", {{"a", std::string("1")}, {"b", std::string("2")}});
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto cols = is.getColumns(std::string_view("ord_test"));
    ASSERT_GE(cols.size(), 2u);

    // All ordinals should be >= 1 and unique
    std::set<uint32_t> positions;
    for (const auto& col : cols) {
        EXPECT_GE(col.ordinal_position, 1u);
        positions.insert(col.ordinal_position);
    }
    EXPECT_EQ(positions.size(), cols.size());
}

// ============================================================================
// INFORMATION_SCHEMA.STATISTICS
// ============================================================================

TEST_F(InformationSchemaTest, GetStatisticsEmpty) {
    InformationSchema is(*schema_);
    auto stats = is.getStatistics();
    EXPECT_TRUE(stats.empty());
}

TEST_F(InformationSchemaTest, GetStatisticsReflectsIndexes) {
    insertRow("indexed_table", "r1", {{"col_a", std::string("x")}});

    // Register a custom schema with an index
    SchemaManager::TableSchema ts;
    ts.name = "indexed_table";
    ts.type = "relational";
    SchemaManager::PropertyInfo p;
    p.name    = "col_a";
    p.type    = "string";
    p.indexed = true;
    ts.properties.push_back(p);
    SchemaManager::IndexInfo idx;
    idx.name   = "col_a_idx";
    idx.type   = "regular";
    idx.unique = true;
    idx.columns = {"col_a"};
    ts.indexes.push_back(idx);
    schema_->setTableSchema("indexed_table", ts);

    InformationSchema is(*schema_);
    auto stats = is.getStatistics(std::string_view("indexed_table"));
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].table_name,  "indexed_table");
    EXPECT_EQ(stats[0].index_name,  "col_a_idx");
    EXPECT_EQ(stats[0].column_name, "col_a");
    EXPECT_EQ(stats[0].non_unique,  "0");  // unique index => non_unique = 0
    EXPECT_EQ(stats[0].seq_in_index, 1u);
}

// ============================================================================
// INFORMATION_SCHEMA.KEY_COLUMN_USAGE
// ============================================================================

TEST_F(InformationSchemaTest, GetKeyColumnUsageOnlyForUniqueIndexes) {
    SchemaManager::TableSchema ts;
    ts.name = "kcu_test";
    ts.type = "relational";
    SchemaManager::PropertyInfo p;
    p.name = "id";  p.type = "integer";
    ts.properties.push_back(p);

    SchemaManager::IndexInfo unique_idx;
    unique_idx.name    = "pk_id";
    unique_idx.type    = "regular";
    unique_idx.unique  = true;
    unique_idx.columns = {"id"};
    ts.indexes.push_back(unique_idx);

    SchemaManager::IndexInfo non_unique_idx;
    non_unique_idx.name    = "idx_id";
    non_unique_idx.type    = "regular";
    non_unique_idx.unique  = false;
    non_unique_idx.columns = {"id"};
    ts.indexes.push_back(non_unique_idx);

    schema_->setTableSchema("kcu_test", ts);

    InformationSchema is(*schema_);
    auto kcu = is.getKeyColumnUsage(std::string_view("kcu_test"));

    // Only unique index should appear in KEY_COLUMN_USAGE
    ASSERT_EQ(kcu.size(), 1u);
    EXPECT_EQ(kcu[0].constraint_name, "pk_id");
    EXPECT_EQ(kcu[0].column_name,     "id");
    EXPECT_EQ(kcu[0].table_name,      "kcu_test");
}

// ============================================================================
// JSON serialisation
// ============================================================================

TEST_F(InformationSchemaTest, ToJSONStructure) {
    insertRow("json_test", "j1", {{"x", int64_t(1)}});
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto j = is.toJSON();

    EXPECT_TRUE(j.contains("tables"));
    EXPECT_TRUE(j.contains("columns"));
    EXPECT_TRUE(j.contains("statistics"));
    EXPECT_TRUE(j.contains("key_column_usage"));

    EXPECT_TRUE(j["tables"].is_array());
    EXPECT_TRUE(j["columns"].is_array());
    EXPECT_TRUE(j["statistics"].is_array());
    EXPECT_TRUE(j["key_column_usage"].is_array());
}

TEST_F(InformationSchemaTest, TablesToJSON) {
    insertRow("t1", "r1", {{"a", std::string("v")}});
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto j = is.tablesToJSON();

    ASSERT_FALSE(j.empty());
    EXPECT_TRUE(j[0].contains("table_name"));
    EXPECT_TRUE(j[0].contains("table_type"));
    EXPECT_TRUE(j[0].contains("engine"));
}

TEST_F(InformationSchemaTest, ColumnsToJSON) {
    insertRow("col_json_tbl", "r1", {{"field", std::string("val")}});
    schema_->refreshCache();

    InformationSchema is(*schema_);
    auto j = is.columnsToJSON("col_json_tbl");

    ASSERT_FALSE(j.empty());
    EXPECT_TRUE(j[0].contains("column_name"));
    EXPECT_TRUE(j[0].contains("data_type"));
    EXPECT_TRUE(j[0].contains("is_nullable"));
}

TEST_F(InformationSchemaTest, ISTableToJSON) {
    ISTable t;
    t.table_catalog = "def";
    t.table_schema  = "main";
    t.table_name    = "foo";
    t.table_type    = "BASE TABLE";
    t.row_count     = 42;
    t.engine        = "ThemisDB";

    auto j = t.toJSON();
    EXPECT_EQ(j["table_name"],    "foo");
    EXPECT_EQ(j["row_count"],     42u);
    EXPECT_EQ(j["engine"],        "ThemisDB");
}

TEST_F(InformationSchemaTest, ISColumnToJSON) {
    ISColumn c;
    c.table_catalog    = "def";
    c.table_schema     = "main";
    c.table_name       = "bar";
    c.column_name      = "id";
    c.ordinal_position = 1;
    c.data_type        = "BIGINT";
    c.is_nullable      = "NO";

    auto j = c.toJSON();
    EXPECT_EQ(j["column_name"],      "id");
    EXPECT_EQ(j["data_type"],        "BIGINT");
    EXPECT_EQ(j["ordinal_position"], 1u);
    EXPECT_EQ(j["is_nullable"],      "NO");
}

TEST_F(InformationSchemaTest, ISStatisticToJSON) {
    ISStatistic s;
    s.table_catalog = "def";
    s.table_schema  = "main";
    s.table_name    = "baz";
    s.index_name    = "idx_col";
    s.column_name   = "col";
    s.seq_in_index  = 1;
    s.index_type    = "BTREE";
    s.non_unique    = "1";

    auto j = s.toJSON();
    EXPECT_EQ(j["index_name"],  "idx_col");
    EXPECT_EQ(j["index_type"],  "BTREE");
    EXPECT_EQ(j["non_unique"],  "1");
}

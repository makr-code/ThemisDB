// PostgreSQL Importer v2.0 – Integration Tests
//
// Tests run against the full PostgreSQLImporter class with real dump strings
// written to temporary files.  All tests are self-contained and clean up
// their temp files on exit.

#include <gtest/gtest.h>
#include "importers/postgres_importer.h"
#include "importers/relationship_mapper.h"

#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Helper: write a string to a temp file and return the path.
static std::string writeTempFile(const std::string& content,
                                  const std::string& suffix = ".sql") {
    static std::atomic<int> counter{0};
    std::string path = (fs::temp_directory_path() /
                        ("themis_pg_integ_v2_" +
                         std::to_string(counter.fetch_add(1)) + suffix)).string();
    std::ofstream f(path, std::ios::trunc);
    f << content;
    return path;
}

struct TempFile {
    std::string path;
    explicit TempFile(const std::string& content, const std::string& suffix = ".sql")
        : path(writeTempFile(content, suffix)) {}
    ~TempFile() { try { fs::remove(path); } catch (...) {} }
};

} // namespace

// ============================================================================
// Integration: getSourceSchema() v2 enriched output
// ============================================================================

TEST(PostgresIntegrationV2, GetSourceSchemaWithForeignKeys) {
    TempFile dump(R"sql(
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    email VARCHAR(255) UNIQUE
);

CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    amount DECIMAL(10,2) DEFAULT 0.00,
    CONSTRAINT fk_orders_users FOREIGN KEY (user_id)
        REFERENCES users(id) ON DELETE CASCADE
);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object()) << "Expected object, got: " << schema.dump();
    ASSERT_TRUE(schema.contains("tables"));
    ASSERT_TRUE(schema.contains("relationships"));

    const auto& tables = schema["tables"];
    EXPECT_GE(tables.size(), 2u);

    // Find orders table
    nlohmann::json orders_json;
    for (const auto& t : tables) {
        if (t["name"] == "orders") { orders_json = t; break; }
    }
    ASSERT_FALSE(orders_json.is_null()) << "orders table not found";
    ASSERT_TRUE(orders_json.contains("foreign_keys"));
    ASSERT_GE(orders_json["foreign_keys"].size(), 1u);

    const auto& fk = orders_json["foreign_keys"][0];
    EXPECT_EQ(fk["target_table"], "users");
    EXPECT_EQ(fk["on_delete"], "CASCADE");

    // relationships array should contain one edge
    const auto& rels = schema["relationships"];
    EXPECT_GE(rels.size(), 1u);
    EXPECT_EQ(rels[0]["cardinality"], "MANY_TO_ONE");
}

TEST(PostgresIntegrationV2, GetSourceSchemaWithIndexes) {
    TempFile dump(R"sql(
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    sku VARCHAR(50),
    name TEXT
);

CREATE UNIQUE INDEX idx_sku ON products(sku);
CREATE INDEX idx_name ON products USING btree (name);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object());
    const auto& tables = schema["tables"];

    nlohmann::json products_json;
    for (const auto& t : tables) {
        if (t["name"] == "products") { products_json = t; break; }
    }
    ASSERT_FALSE(products_json.is_null());
    ASSERT_TRUE(products_json.contains("indexes"));
    EXPECT_GE(products_json["indexes"].size(), 1u);

    // Find the unique index
    bool found_unique = false;
    for (const auto& idx : products_json["indexes"]) {
        if (idx.value("unique", false)) { found_unique = true; break; }
    }
    EXPECT_TRUE(found_unique);
}

TEST(PostgresIntegrationV2, GetSourceSchemaAlterTableForeignKey) {
    TempFile dump(R"sql(
CREATE TABLE categories (
    id SERIAL PRIMARY KEY,
    name TEXT
);

CREATE TABLE articles (
    id SERIAL PRIMARY KEY,
    category_id INTEGER,
    title TEXT
);

ALTER TABLE ONLY articles
    ADD CONSTRAINT fk_articles_category FOREIGN KEY (category_id)
        REFERENCES categories(id) ON DELETE SET NULL;
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object());
    const auto& tables = schema["tables"];

    nlohmann::json articles_json;
    for (const auto& t : tables) {
        if (t["name"] == "articles") { articles_json = t; break; }
    }
    ASSERT_FALSE(articles_json.is_null());
    ASSERT_GE(articles_json["foreign_keys"].size(), 1u);
    EXPECT_EQ(articles_json["foreign_keys"][0]["target_table"], "categories");
    EXPECT_EQ(articles_json["foreign_keys"][0]["on_delete"], "SET NULL");
}

TEST(PostgresIntegrationV2, GetSourceSchemaNoForeignKeys) {
    TempFile dump(R"sql(
CREATE TABLE standalone (
    id SERIAL PRIMARY KEY,
    data TEXT
);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object());
    const auto& rels = schema["relationships"];
    EXPECT_EQ(rels.size(), 0u);
    EXPECT_EQ(schema["circular_references"].size(), 0u);
}

// ============================================================================
// Integration: importData() with preserve_relationships
// ============================================================================

TEST(PostgresIntegrationV2, ImportDataCounts) {
    TempFile dump(R"sql(
CREATE TABLE users (id SERIAL PRIMARY KEY, name TEXT);
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id),
    amount DECIMAL
);
INSERT INTO users VALUES (1, 'Alice');
INSERT INTO orders VALUES (1, 1, 100.00);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");

    themis::importers::ImportOptions opts;
    opts.preserve_relationships = true;
    opts.continue_on_error = true;

    auto stats = importer.importData(dump.path, opts);

    EXPECT_EQ(stats.tables_processed, 2u);
    EXPECT_EQ(stats.imported_records, 2u);
    // inline REFERENCES FK should be counted
    EXPECT_GE(stats.relationships_processed, 1u);
}

// ============================================================================
// Integration: validate_references
// ============================================================================

TEST(PostgresIntegrationV2, ValidateFKReferencesMissing) {
    TempFile dump(R"sql(
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL
);
ALTER TABLE ONLY orders
    ADD CONSTRAINT fk_orders_missing FOREIGN KEY (user_id)
        REFERENCES nonexistent_users(id);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");

    themis::importers::ImportOptions opts;
    opts.preserve_relationships = true;
    opts.validate_references    = true;
    opts.continue_on_error      = true;

    auto stats = importer.importData(dump.path, opts);

    // Should have at least one warning about the missing table
    bool found_warning = false;
    for (const auto& e : stats.structured_errors) {
        if (e.message.find("nonexistent_users") != std::string::npos) {
            found_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_warning) << "Expected warning about nonexistent_users not found";
}

// ============================================================================
// Integration: RelationshipMapper with real schemas
// ============================================================================

TEST(PostgresIntegrationV2, RelationshipMapperFromForeignKeys) {
    TempFile dump(R"sql(
CREATE TABLE users (id SERIAL PRIMARY KEY, name TEXT);
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    CONSTRAINT fk_orders_users FOREIGN KEY (user_id) REFERENCES users(id)
);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object());
    const auto& rels = schema["relationships"];
    ASSERT_GE(rels.size(), 1u);

    // The auto-mapped relationship should be MANY_TO_ONE
    bool found = false;
    for (const auto& rel : rels) {
        if (rel["source_table"] == "orders" && rel["target_table"] == "users") {
            EXPECT_EQ(rel["cardinality"], "MANY_TO_ONE");
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected orders→users relationship not found";
}

// ============================================================================
// Integration: Circular Reference Detection
// ============================================================================

TEST(PostgresIntegrationV2, CircularReferenceDetected) {
    TempFile dump(R"sql(
CREATE TABLE a (id SERIAL PRIMARY KEY, b_id INTEGER);
CREATE TABLE b (id SERIAL PRIMARY KEY, a_id INTEGER);

ALTER TABLE a ADD CONSTRAINT fk_a_b FOREIGN KEY (b_id) REFERENCES b(id);
ALTER TABLE b ADD CONSTRAINT fk_b_a FOREIGN KEY (a_id) REFERENCES a(id);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object());
    const auto& cycles = schema["circular_references"];
    EXPECT_GT(cycles.size(), 0u) << "Expected circular reference to be detected";
}

// ============================================================================
// Integration: Column defaults and constraints in schema
// ============================================================================

TEST(PostgresIntegrationV2, ColumnDefaultsPreserved) {
    TempFile dump(R"sql(
CREATE TABLE accounts (
    id SERIAL PRIMARY KEY,
    balance DECIMAL DEFAULT 0.00,
    active BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW()
);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");
    auto schema = importer.getSourceSchema(dump.path);

    ASSERT_TRUE(schema.is_object());
    const auto& tables = schema["tables"];
    nlohmann::json tbl;
    for (const auto& t : tables) if (t["name"] == "accounts") { tbl = t; break; }
    ASSERT_FALSE(tbl.is_null());

    ASSERT_TRUE(tbl.contains("column_defaults"));
    const auto& defs = tbl["column_defaults"];
    EXPECT_TRUE(defs.contains("balance") || defs.contains("active") || defs.contains("created_at"))
        << "Expected at least one default to be preserved";
}

// ============================================================================
// Integration: Backward compatibility – v1.x behaviour unchanged
// ============================================================================

TEST(PostgresIntegrationV2, BackwardCompatV1TableParsing) {
    // A plain table with no FKs must still parse correctly with v2.0
    TempFile dump(R"sql(
CREATE TABLE legacy_data (
    id INTEGER,
    name VARCHAR(100),
    value FLOAT
);
INSERT INTO legacy_data VALUES (1, 'foo', 3.14);
)sql");

    themis::importers::PostgreSQLImporter importer;
    importer.initialize("{}");

    themis::importers::ImportOptions opts;
    opts.preserve_relationships = false;
    auto stats = importer.importData(dump.path, opts);

    EXPECT_EQ(stats.tables_processed, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
    EXPECT_EQ(stats.relationships_processed, 0u);
}

// ============================================================================
// Main
// ============================================================================

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <thread>

#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/secondary_index_metadata_cache.h"

using namespace themis;

// Helper to create temporary database path
static std::string makeTempDbPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto base = fs::temp_directory_path() / (name + std::to_string(now));
    return base.string();
}

// Test fixture for SchemaManager tests
class SchemaManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        SecondaryIndexMetadataCache::instance().clear();

        // Create temporary database
        RocksDBWrapper::Config cfg;
        cfg.db_path = makeTempDbPath("test_schema_mgr_");
        cfg.enable_blobdb = false;
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        
        index_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
    }
    
    void TearDown() override {
        // Close database
        if (db_) {
            db_->close();
        }

        SecondaryIndexMetadataCache::instance().clear();
        
        // Clean up temporary files
        // (filesystem cleanup is automatic on temp directory)
    }
    
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> index_mgr_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(SchemaManagerTest, EmptyDatabase) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto tables = schema_mgr.getAllTables();
    EXPECT_EQ(tables.size(), 0) << "Empty database should have no tables";
    
    auto metadata = schema_mgr.getDatabaseMetadata();
    EXPECT_EQ(metadata.table_count, 0);
    EXPECT_EQ(metadata.total_rows, 0);
    EXPECT_FALSE(metadata.version.empty());
}

TEST_F(SchemaManagerTest, DiscoverSingleTable) {
    // Insert some test data
    BaseEntity::FieldMap fields{
        {"name", std::string("Alice")},
        {"age", int64_t(30)},
        {"active", true}
    };
    
    BaseEntity entity = BaseEntity::fromFields("user1", fields);
    db_->put("users:user1", entity.serialize());
    
    // Create schema manager and discover
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto tables = schema_mgr.getAllTables();
    ASSERT_EQ(tables.size(), 1) << "Should discover 1 table";
    EXPECT_EQ(tables[0].name, "users");
    EXPECT_EQ(tables[0].estimated_row_count, 1);
    
    // Check properties
    EXPECT_GE(tables[0].properties.size(), 3) << "Should have at least 3 properties";
    
    bool found_name = false;
    bool found_age = false;
    bool found_active = false;
    
    for (const auto& prop : tables[0].properties) {
        if (prop.name == "name") {
            found_name = true;
            EXPECT_EQ(prop.type, "string");
        } else if (prop.name == "age") {
            found_age = true;
            EXPECT_EQ(prop.type, "integer");
        } else if (prop.name == "active") {
            found_active = true;
            EXPECT_EQ(prop.type, "boolean");
        }
    }
    
    EXPECT_TRUE(found_name) << "Should find 'name' property";
    EXPECT_TRUE(found_age) << "Should find 'age' property";
    EXPECT_TRUE(found_active) << "Should find 'active' property";
}

TEST_F(SchemaManagerTest, InternalBinaryKeyspacesDoNotCrashSchemaExport) {
    // Simulate internal security keyspaces that store opaque binary payloads,
    // not BaseEntity-serialized records.
    std::vector<uint8_t> opaque_kek = {0x00, 0xFF, 0x13, 0x37, 0x42, 0x10};
    std::vector<uint8_t> opaque_dek = {0xAB, 0xCD, 0xEF, 0x01, 0x02, 0x03};

    db_->put("kek:active", opaque_kek);
    db_->put("dek:v1", opaque_dek);

    SchemaManager schema_mgr(*db_, index_mgr_.get());

    json exported;
    EXPECT_NO_THROW(exported = schema_mgr.toJSON())
        << "Schema export must ignore internal binary keyspaces instead of crashing";

    ASSERT_TRUE(exported.contains("status"));
    EXPECT_EQ(exported["status"], "success");
}

TEST_F(SchemaManagerTest, DiscoverMultipleTables) {
    // Insert data into multiple tables
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    BaseEntity user2 = BaseEntity::fromFields("u2", {{"name", std::string("Bob")}});
    BaseEntity order1 = BaseEntity::fromFields("o1", {{"total", 99.99}});
    
    db_->put("users:u1", user1.serialize());
    db_->put("users:u2", user2.serialize());
    db_->put("orders:o1", order1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto tables = schema_mgr.getAllTables();
    ASSERT_EQ(tables.size(), 2) << "Should discover 2 tables";
    
    // Check table names
    std::vector<std::string> table_names;
    for (const auto& table : tables) {
        table_names.push_back(table.name);
    }
    
    EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "users") != table_names.end());
    EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "orders") != table_names.end());
}

TEST_F(SchemaManagerTest, GetTableByName) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Get existing table
    auto table_opt = schema_mgr.getTable("users");
    ASSERT_TRUE(table_opt.has_value()) << "Should find 'users' table";
    EXPECT_EQ(table_opt->name, "users");
    
    // Get non-existing table
    auto not_found = schema_mgr.getTable("nonexistent");
    EXPECT_FALSE(not_found.has_value()) << "Should not find non-existent table";
}

TEST_F(SchemaManagerTest, IndexDiscovery) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {
        {"name", std::string("Alice")},
        {"email", std::string("alice@example.com")}
    });
    db_->put("users:u1", user1.serialize());
    
    // Create indexes
    auto st1 = index_mgr_->createIndex("users", "name");
    ASSERT_TRUE(st1.ok) << "Failed to create index: " << st1.message;
    
    auto st2 = index_mgr_->createRangeIndex("users", "email");
    ASSERT_TRUE(st2.ok) << "Failed to create range index: " << st2.message;
    
    // Create schema manager and check indexes
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto table_opt = schema_mgr.getTable("users");
    ASSERT_TRUE(table_opt.has_value());
    
    EXPECT_GE(table_opt->indexes.size(), 2) << "Should have at least 2 indexes";
    
    // Check that properties are marked as indexed
    bool name_indexed = false;
    bool email_indexed = false;
    
    for (const auto& prop : table_opt->properties) {
        if (prop.name == "name" && prop.indexed) {
            name_indexed = true;
        } else if (prop.name == "email" && prop.indexed) {
            email_indexed = true;
        }
    }
    
    EXPECT_TRUE(name_indexed) << "name property should be marked as indexed";
    EXPECT_TRUE(email_indexed) << "email property should be marked as indexed";
}

// ============================================================================
// Cache Tests
// ============================================================================

TEST_F(SchemaManagerTest, CacheMechanism) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // First call - should build cache
    auto tables1 = schema_mgr.getAllTables();
    ASSERT_EQ(tables1.size(), 1);
    
    // Insert more data (should not be reflected immediately due to cache)
    BaseEntity order1 = BaseEntity::fromFields("o1", {{"total", 99.99}});
    db_->put("orders:o1", order1.serialize());
    
    // Second call - should use cache (still 1 table)
    auto tables2 = schema_mgr.getAllTables();
    EXPECT_EQ(tables2.size(), 1) << "Cache should return old data";
    
    // Force refresh
    schema_mgr.refreshCache();
    
    // Third call - should have new data
    auto tables3 = schema_mgr.getAllTables();
    EXPECT_EQ(tables3.size(), 2) << "After refresh should see new table";
}

TEST_F(SchemaManagerTest, CacheTTL) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Set very short TTL for testing
    schema_mgr.setCacheTTL(std::chrono::seconds(1));
    
    // First call - builds cache
    auto tables1 = schema_mgr.getAllTables();
    ASSERT_EQ(tables1.size(), 1);
    
    // Insert more data
    BaseEntity order1 = BaseEntity::fromFields("o1", {{"total", 99.99}});
    db_->put("orders:o1", order1.serialize());
    
    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should auto-refresh and see new data
    auto tables2 = schema_mgr.getAllTables();
    EXPECT_EQ(tables2.size(), 2) << "Cache should auto-refresh after TTL";
}

// ============================================================================
// JSON Export Tests
// ============================================================================

TEST_F(SchemaManagerTest, JSONExport) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {
        {"name", std::string("Alice")},
        {"age", int64_t(30)}
    });
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto json = schema_mgr.toJSON();
    
    // Check JSON structure
    EXPECT_TRUE(json.contains("status"));
    EXPECT_EQ(json["status"], "success");
    
    EXPECT_TRUE(json.contains("metadata"));
    EXPECT_TRUE(json.contains("tables"));
    EXPECT_TRUE(json.contains("relationships"));
    
    // Check metadata
    auto& metadata = json["metadata"];
    EXPECT_TRUE(metadata.contains("version"));
    EXPECT_TRUE(metadata.contains("table_count"));
    EXPECT_TRUE(metadata.contains("capabilities"));
    
    // Check tables array
    EXPECT_TRUE(json["tables"].is_array());
    EXPECT_GE(json["tables"].size(), 1);
}

TEST_F(SchemaManagerTest, TableToJSON) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Export existing table
    auto json = schema_mgr.tableToJSON("users");
    EXPECT_EQ(json["status"], "success");
    EXPECT_TRUE(json.contains("table"));
    EXPECT_EQ(json["table"]["name"], "users");
    
    // Export non-existing table
    auto json_not_found = schema_mgr.tableToJSON("nonexistent");
    EXPECT_EQ(json_not_found["status"], "error");
    EXPECT_TRUE(json_not_found.contains("message"));
}

TEST_F(SchemaManagerTest, CapabilitiesJSON) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto json = schema_mgr.getCapabilitiesJSON();
    
    EXPECT_EQ(json["status"], "success");
    EXPECT_TRUE(json.contains("version"));
    EXPECT_TRUE(json.contains("capabilities"));
    EXPECT_TRUE(json["capabilities"].is_array());
    
    // Check for expected capabilities
    auto& caps = json["capabilities"];
    bool has_multi_model = false;
    for (const auto& cap : caps) {
        if (cap == "multi-model") {
            has_multi_model = true;
        }
    }
    EXPECT_TRUE(has_multi_model) << "Should have 'multi-model' capability";
}

// ============================================================================
// Database Metadata Tests
// ============================================================================

TEST_F(SchemaManagerTest, DatabaseMetadata) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    BaseEntity user2 = BaseEntity::fromFields("u2", {{"name", std::string("Bob")}});
    db_->put("users:u1", user1.serialize());
    db_->put("users:u2", user2.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    auto metadata = schema_mgr.getDatabaseMetadata();
    
    EXPECT_FALSE(metadata.version.empty());
    EXPECT_EQ(metadata.table_count, 1);
    EXPECT_EQ(metadata.total_rows, 2);
    EXPECT_FALSE(metadata.capabilities.empty());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(SchemaManagerTest, PerformanceDiscoveryTime) {
    // Insert test data (20 tables, 10 rows each)
    for (int table_idx = 0; table_idx < 20; ++table_idx) {
        std::string table_name = "table" + std::to_string(table_idx);
        
        for (int row_idx = 0; row_idx < 10; ++row_idx) {
            std::string pk = "pk" + std::to_string(row_idx);
            BaseEntity entity = BaseEntity::fromFields(pk, {
                {"field1", std::string("value")},
                {"field2", int64_t(42)},
                {"field3", 3.14}
            });
            
            db_->put(table_name + ":" + pk, entity.serialize());
        }
    }
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Measure discovery time
    auto start = std::chrono::steady_clock::now();
    auto tables = schema_mgr.getAllTables();
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(tables.size(), 20) << "Should discover 20 tables";
    EXPECT_LT(duration_ms, 1000) << "Discovery should take less than 1 second (was " << duration_ms << "ms)";
    
    std::cout << "Discovery time for 20 tables (200 rows): " << duration_ms << "ms" << std::endl;
}

TEST_F(SchemaManagerTest, PerformanceCacheHitRate) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // First call - builds cache
    auto start_first = std::chrono::steady_clock::now();
    auto tables1 = schema_mgr.getAllTables();
    auto end_first = std::chrono::steady_clock::now();
    auto duration_first = std::chrono::duration_cast<std::chrono::microseconds>(end_first - start_first).count();
    
    // Second call - should hit cache (much faster)
    auto start_second = std::chrono::steady_clock::now();
    auto tables2 = schema_mgr.getAllTables();
    auto end_second = std::chrono::steady_clock::now();
    auto duration_second = std::chrono::duration_cast<std::chrono::microseconds>(end_second - start_second).count();
    
    EXPECT_LT(duration_second, duration_first) << "Cache hit should be faster than initial build";
    
    std::cout << "First call (build): " << duration_first << "µs" << std::endl;
    std::cout << "Second call (cache hit): " << duration_second << "µs" << std::endl;
    std::cout << "Speedup: " << (double)duration_first / duration_second << "x" << std::endl;
}

// ============================================================================
// Custom Schema Management Tests (PUT/PATCH)
// ============================================================================

TEST_F(SchemaManagerTest, SetCustomSchema) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Create custom schema
    SchemaManager::TableSchema schema;
    schema.name = "products";
    schema.type = "relational";
    
    SchemaManager::PropertyInfo prop1;
    prop1.name = "id";
    prop1.type = "integer";
    prop1.indexed = true;
    prop1.nullable = false;
    schema.properties.push_back(prop1);
    
    SchemaManager::PropertyInfo prop2;
    prop2.name = "name";
    prop2.type = "string";
    prop2.nullable = true;
    schema.properties.push_back(prop2);
    
    SchemaManager::IndexInfo idx;
    idx.name = "id";
    idx.type = "regular";
    idx.unique = true;
    idx.columns.push_back("id");
    schema.indexes.push_back(idx);
    
    // Store custom schema
    bool success = schema_mgr.setTableSchema("products", schema);
    ASSERT_TRUE(success) << "Should successfully store custom schema";
    
    // Verify schema was stored
    auto table_opt = schema_mgr.getTable("products");
    ASSERT_TRUE(table_opt.has_value()) << "Should find custom schema";
    EXPECT_EQ(table_opt->name, "products");
    EXPECT_EQ(table_opt->type, "relational");
    EXPECT_EQ(table_opt->properties.size(), 2);
    EXPECT_EQ(table_opt->indexes.size(), 1);
}

TEST_F(SchemaManagerTest, CustomSchemaPersistedAcrossInstances) {
    // Create and store custom schema
    {
        SchemaManager schema_mgr(*db_, index_mgr_.get());
        
        SchemaManager::TableSchema schema;
        schema.name = "customers";
        schema.type = "document";
        
        SchemaManager::PropertyInfo prop;
        prop.name = "email";
        prop.type = "string";
        schema.properties.push_back(prop);
        
        bool success = schema_mgr.setTableSchema("customers", schema);
        ASSERT_TRUE(success);
    }
    
    // Create new SchemaManager instance - should load persisted schema
    {
        SchemaManager schema_mgr2(*db_, index_mgr_.get());
        
        auto table_opt = schema_mgr2.getTable("customers");
        ASSERT_TRUE(table_opt.has_value()) << "Custom schema should be loaded from storage";
        EXPECT_EQ(table_opt->name, "customers");
        EXPECT_EQ(table_opt->type, "document");
        EXPECT_EQ(table_opt->properties.size(), 1);
        EXPECT_EQ(table_opt->properties[0].name, "email");
    }
}

TEST_F(SchemaManagerTest, PatchSchemaAddProperty) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Create initial schema
    SchemaManager::TableSchema schema;
    schema.name = "items";
    schema.type = "relational";
    
    SchemaManager::PropertyInfo prop1;
    prop1.name = "id";
    prop1.type = "integer";
    schema.properties.push_back(prop1);
    
    schema_mgr.setTableSchema("items", schema);
    
    // Patch: add new property
    nlohmann::json updates;
    updates["properties"] = nlohmann::json::array();
    updates["properties"].push_back({
        {"name", "description"},
        {"type", "string"},
        {"nullable", true}
    });
    
    bool success = schema_mgr.patchTableSchema("items", updates);
    ASSERT_TRUE(success) << "Patch should succeed";
    
    // Verify patch was applied
    auto table_opt = schema_mgr.getTable("items");
    ASSERT_TRUE(table_opt.has_value());
    EXPECT_EQ(table_opt->properties.size(), 2);
    
    bool found_description = false;
    for (const auto& prop : table_opt->properties) {
        if (prop.name == "description") {
            found_description = true;
            EXPECT_EQ(prop.type, "string");
            EXPECT_TRUE(prop.nullable);
        }
    }
    EXPECT_TRUE(found_description) << "Should find new 'description' property";
}

TEST_F(SchemaManagerTest, PatchSchemaUpdateType) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Create initial schema
    SchemaManager::TableSchema schema;
    schema.name = "records";
    schema.type = "relational";
    schema_mgr.setTableSchema("records", schema);
    
    // Patch: change type
    nlohmann::json updates;
    updates["type"] = "document";
    
    bool success = schema_mgr.patchTableSchema("records", updates);
    ASSERT_TRUE(success);
    
    // Verify update
    auto table_opt = schema_mgr.getTable("records");
    ASSERT_TRUE(table_opt.has_value());
    EXPECT_EQ(table_opt->type, "document");
}

TEST_F(SchemaManagerTest, PatchNonExistentTable) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    nlohmann::json updates;
    updates["type"] = "document";
    
    bool success = schema_mgr.patchTableSchema("nonexistent", updates);
    EXPECT_FALSE(success) << "Patch should fail for non-existent table";
}

TEST_F(SchemaManagerTest, DeleteCustomSchema) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Create custom schema
    SchemaManager::TableSchema schema;
    schema.name = "temp_table";
    schema.type = "relational";
    schema_mgr.setTableSchema("temp_table", schema);
    
    // Verify it exists
    auto table_opt1 = schema_mgr.getTable("temp_table");
    ASSERT_TRUE(table_opt1.has_value());
    
    // Delete it
    bool deleted = schema_mgr.deleteTableSchema("temp_table");
    EXPECT_TRUE(deleted) << "Should successfully delete custom schema";
    
    // Verify it's gone
    auto table_opt2 = schema_mgr.getTable("temp_table");
    EXPECT_FALSE(table_opt2.has_value()) << "Deleted schema should not be found";
}

TEST_F(SchemaManagerTest, DeleteNonExistentSchema) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    bool deleted = schema_mgr.deleteTableSchema("nonexistent");
    EXPECT_FALSE(deleted) << "Delete should return false for non-existent schema";
}

// ============================================================================
// Schema Validation Tests
// ============================================================================

TEST_F(SchemaManagerTest, ValidateEmptyTableName) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "";  // Invalid: empty name
    schema.type = "relational";
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_FALSE(error.empty()) << "Should reject empty table name";
    EXPECT_NE(error.find("name is required"), std::string::npos);
}

TEST_F(SchemaManagerTest, ValidateInvalidTableName) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "table@name!";  // Invalid: special characters
    schema.type = "relational";
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_FALSE(error.empty()) << "Should reject invalid characters in table name";
    EXPECT_NE(error.find("invalid characters"), std::string::npos);
}

TEST_F(SchemaManagerTest, ValidateInvalidTableType) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "test";
    schema.type = "invalid_type";  // Invalid type
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_FALSE(error.empty()) << "Should reject invalid table type";
    EXPECT_NE(error.find("Invalid table type"), std::string::npos);
}

TEST_F(SchemaManagerTest, ValidateDuplicatePropertyNames) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "test";
    schema.type = "relational";
    
    SchemaManager::PropertyInfo prop1;
    prop1.name = "id";
    prop1.type = "integer";
    schema.properties.push_back(prop1);
    
    SchemaManager::PropertyInfo prop2;
    prop2.name = "id";  // Duplicate name
    prop2.type = "string";
    schema.properties.push_back(prop2);
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_FALSE(error.empty()) << "Should reject duplicate property names";
    EXPECT_NE(error.find("Duplicate property name"), std::string::npos);
}

TEST_F(SchemaManagerTest, ValidateInvalidPropertyType) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "test";
    schema.type = "relational";
    
    SchemaManager::PropertyInfo prop;
    prop.name = "field";
    prop.type = "invalid_type";  // Invalid type
    schema.properties.push_back(prop);
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_FALSE(error.empty()) << "Should reject invalid property type";
    EXPECT_NE(error.find("Invalid property type"), std::string::npos);
}

TEST_F(SchemaManagerTest, ValidateIndexReferencesNonExistentProperty) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "test";
    schema.type = "relational";
    
    SchemaManager::PropertyInfo prop;
    prop.name = "id";
    prop.type = "integer";
    schema.properties.push_back(prop);
    
    SchemaManager::IndexInfo idx;
    idx.name = "email_idx";
    idx.type = "regular";
    idx.columns.push_back("email");  // References non-existent property
    schema.indexes.push_back(idx);
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_FALSE(error.empty()) << "Should reject index referencing non-existent property";
    EXPECT_NE(error.find("non-existent property"), std::string::npos);
}

TEST_F(SchemaManagerTest, ValidateValidSchema) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    SchemaManager::TableSchema schema;
    schema.name = "valid_table";
    schema.type = "relational";
    
    SchemaManager::PropertyInfo prop;
    prop.name = "id";
    prop.type = "integer";
    schema.properties.push_back(prop);
    
    SchemaManager::IndexInfo idx;
    idx.name = "id_idx";
    idx.type = "regular";
    idx.columns.push_back("id");
    schema.indexes.push_back(idx);
    
    std::string error = schema_mgr.validateSchema(schema);
    EXPECT_TRUE(error.empty()) << "Valid schema should pass validation, but got: " << error;
}

// ============================================================================
// JSON Parsing Tests
// ============================================================================

TEST_F(SchemaManagerTest, ParseTableSchemaFromJSON) {
    nlohmann::json j = {
        {"name", "users"},
        {"type", "relational"},
        {"properties", {
            {
                {"name", "id"},
                {"type", "integer"},
                {"indexed", true},
                {"nullable", false}
            },
            {
                {"name", "email"},
                {"type", "string"}
            }
        }},
        {"indexes", {
            {
                {"name", "id_idx"},
                {"type", "regular"},
                {"unique", true},
                {"columns", {"id"}}
            }
        }}
    };
    
    auto schema = SchemaManager::parseTableSchema(j);
    
    EXPECT_EQ(schema.name, "users");
    EXPECT_EQ(schema.type, "relational");
    EXPECT_EQ(schema.properties.size(), 2);
    EXPECT_EQ(schema.indexes.size(), 1);
    
    // Check first property
    EXPECT_EQ(schema.properties[0].name, "id");
    EXPECT_EQ(schema.properties[0].type, "integer");
    EXPECT_TRUE(schema.properties[0].indexed);
    EXPECT_FALSE(schema.properties[0].nullable);
    
    // Check index
    EXPECT_EQ(schema.indexes[0].name, "id_idx");
    EXPECT_TRUE(schema.indexes[0].unique);
    EXPECT_EQ(schema.indexes[0].columns.size(), 1);
    EXPECT_EQ(schema.indexes[0].columns[0], "id");
}

TEST_F(SchemaManagerTest, ParseTableSchemaMinimal) {
    nlohmann::json j = {
        {"name", "simple_table"}
    };
    
    auto schema = SchemaManager::parseTableSchema(j);
    
    EXPECT_EQ(schema.name, "simple_table");
    EXPECT_EQ(schema.type, "relational");  // Default type
    EXPECT_EQ(schema.properties.size(), 0);
    EXPECT_EQ(schema.indexes.size(), 0);
}

TEST_F(SchemaManagerTest, ParseTableSchemaMissingName) {
    nlohmann::json j = {
        {"type", "relational"}
    };
    
    EXPECT_THROW({
        auto schema = SchemaManager::parseTableSchema(j);
    }, std::runtime_error) << "Should throw when name is missing";
}

TEST_F(SchemaManagerTest, ParseTableSchemaInvalidJSON) {
    nlohmann::json j = {
        {"name", "test"},
        {"properties", "not_an_array"}  // Invalid: should be array
    };
    
    // Should not throw, but properties should be empty
    auto schema = SchemaManager::parseTableSchema(j);
    EXPECT_EQ(schema.name, "test");
    EXPECT_EQ(schema.properties.size(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(SchemaManagerTest, CustomSchemaOverridesDiscovered) {
    // Insert actual data
    BaseEntity user1 = BaseEntity::fromFields("u1", {
        {"name", std::string("Alice")},
        {"age", int64_t(30)}
    });
    db_->put("users:u1", user1.serialize());
    
    SchemaManager schema_mgr(*db_, index_mgr_.get());
    
    // Get discovered schema
    auto discovered = schema_mgr.getTable("users");
    ASSERT_TRUE(discovered.has_value());
    
    // Create custom schema with different properties
    SchemaManager::TableSchema custom_schema;
    custom_schema.name = "users";
    custom_schema.type = "document";  // Different type
    
    SchemaManager::PropertyInfo prop;
    prop.name = "email";  // Different property
    prop.type = "string";
    custom_schema.properties.push_back(prop);
    
    schema_mgr.setTableSchema("users", custom_schema);
    
    // Get schema again - should return custom schema
    auto custom = schema_mgr.getTable("users");
    ASSERT_TRUE(custom.has_value());
    EXPECT_EQ(custom->type, "document") << "Custom schema should override discovered";
    EXPECT_EQ(custom->properties.size(), 1) << "Should have custom properties, not discovered";
    EXPECT_EQ(custom->properties[0].name, "email");
}

// ============================================================================
// Adaptive TTL Tests
// ============================================================================

TEST_F(SchemaManagerTest, AdaptiveTTLDefaultDisabled) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());

    // By default adaptive TTL is off; getEffectiveTTL should return base TTL
    auto base = std::chrono::seconds(60);
    schema_mgr.setCacheTTL(base);
    EXPECT_EQ(schema_mgr.getEffectiveTTL().count(), base.count())
        << "Effective TTL should equal fixed TTL when adaptive mode is off";
}

TEST_F(SchemaManagerTest, AdaptiveTTLNoMutationsUsesMaxTTL) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());

    AdaptiveTTLConfig config;
    config.min_ttl = std::chrono::seconds(5);
    config.max_ttl = std::chrono::seconds(300);
    config.window = std::chrono::seconds(60);
    config.scale_factor = 1.0;
    schema_mgr.setCacheTTL(std::chrono::seconds(60));
    schema_mgr.enableAdaptiveTTL(config);

    // No mutations recorded → rate = 0 → effective = base = 60s, clamped to max=300s
    // base / (1 + 1.0 * 0) = 60, which is within [5, 300]
    auto effective = schema_mgr.getEffectiveTTL();
    EXPECT_EQ(effective.count(), 60)
        << "Zero mutation rate should give effective TTL equal to base TTL";
}

TEST_F(SchemaManagerTest, AdaptiveTTLHighMutationRateReducesTTL) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());

    AdaptiveTTLConfig config;
    config.min_ttl = std::chrono::seconds(5);
    config.max_ttl = std::chrono::seconds(300);
    config.window = std::chrono::seconds(60);
    config.scale_factor = 60.0;  // Aggressive: 1 mut/s halves the TTL at scale 60
    schema_mgr.setCacheTTL(std::chrono::seconds(60));
    schema_mgr.enableAdaptiveTTL(config);

    // Record many mutations in a short window to drive rate up
    for (int i = 0; i < 30; ++i) {
        schema_mgr.recordMutation("orders");
    }

    auto effective = schema_mgr.getEffectiveTTL();
    EXPECT_LT(effective.count(), 60)
        << "High mutation rate should reduce effective TTL below base TTL";
    EXPECT_GE(effective.count(), config.min_ttl.count())
        << "Effective TTL must not drop below configured minimum";
}

TEST_F(SchemaManagerTest, AdaptiveTTLClampedToMinimum) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());

    AdaptiveTTLConfig config;
    config.min_ttl = std::chrono::seconds(10);
    config.max_ttl = std::chrono::seconds(300);
    config.window = std::chrono::seconds(60);
    config.scale_factor = 1000.0;  // Extremely aggressive to force floor
    schema_mgr.setCacheTTL(std::chrono::seconds(60));
    schema_mgr.enableAdaptiveTTL(config);

    // Flood mutations so rate is very high
    for (int i = 0; i < 60; ++i) {
        schema_mgr.recordMutation("hot_table");
    }

    auto effective = schema_mgr.getEffectiveTTL();
    EXPECT_EQ(effective.count(), config.min_ttl.count())
        << "Effective TTL must be clamped to min_ttl under extreme mutation rate";
}

TEST_F(SchemaManagerTest, AdaptiveTTLDisableRestoresFixedTTL) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());

    auto base = std::chrono::seconds(45);
    schema_mgr.setCacheTTL(base);

    schema_mgr.enableAdaptiveTTL();
    schema_mgr.recordMutation("some_table");

    // Disable adaptive TTL
    schema_mgr.disableAdaptiveTTL();

    EXPECT_EQ(schema_mgr.getEffectiveTTL().count(), base.count())
        << "After disabling adaptive TTL, effective TTL must equal fixed TTL";
}

TEST_F(SchemaManagerTest, AdaptiveTTLCacheExpiresEarlierUnderLoad) {
    // Insert test data
    BaseEntity user1 = BaseEntity::fromFields("u1", {{"name", std::string("Alice")}});
    db_->put("users:u1", user1.serialize());

    SchemaManager schema_mgr(*db_, index_mgr_.get());

    AdaptiveTTLConfig config;
    config.min_ttl = std::chrono::seconds(1);
    config.max_ttl = std::chrono::seconds(300);
    config.window = std::chrono::seconds(10);
    config.scale_factor = 100.0;
    schema_mgr.setCacheTTL(std::chrono::seconds(60));
    schema_mgr.enableAdaptiveTTL(config);

    // Prime the cache
    auto tables1 = schema_mgr.getAllTables();
    ASSERT_EQ(tables1.size(), 1);

    // Insert a new table's data
    BaseEntity order1 = BaseEntity::fromFields("o1", {{"total", 99.99}});
    db_->put("orders:o1", order1.serialize());

    // Record many mutations to drive effective TTL to min (1s)
    for (int i = 0; i < 60; ++i) {
        schema_mgr.recordMutation("orders");
    }

    // Wait just over min_ttl to let the adaptive TTL expire
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Cache should auto-refresh and discover the new table
    auto tables2 = schema_mgr.getAllTables();
    EXPECT_EQ(tables2.size(), 2)
        << "After adaptive TTL expires the cache should refresh and pick up new table";
}

TEST_F(SchemaManagerTest, RecordMutationMultipleTablesIndependent) {
    SchemaManager schema_mgr(*db_, index_mgr_.get());

    AdaptiveTTLConfig config;
    config.min_ttl = std::chrono::seconds(5);
    config.max_ttl = std::chrono::seconds(300);
    config.window = std::chrono::seconds(60);
    config.scale_factor = 60.0;
    schema_mgr.setCacheTTL(std::chrono::seconds(60));
    schema_mgr.enableAdaptiveTTL(config);

    // Only mutate one table
    for (int i = 0; i < 20; ++i) {
        schema_mgr.recordMutation("hot_table");
    }
    // cold_table receives no mutations

    // Effective TTL is driven by the hottest table
    auto effective = schema_mgr.getEffectiveTTL();
    EXPECT_LT(effective.count(), 60)
        << "TTL should be reduced because hot_table has a high mutation rate";
    EXPECT_GE(effective.count(), config.min_ttl.count())
        << "Effective TTL must respect the configured minimum";
}


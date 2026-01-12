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

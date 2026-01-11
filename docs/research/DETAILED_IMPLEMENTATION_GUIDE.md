# Detaillierte Implementierungsvorgabe: Agentic AI Self-Awareness

**Datum:** 11. Januar 2026  
**Version:** 1.0  
**Kategorie:** Implementation Guide  
**Basierend auf:** [AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)

---

## 📋 Übersicht

Diese Implementierungsvorgabe definiert **exakt**, was wo und in welcher Reihenfolge eingefügt werden muss, um die Self-Awareness-Fähigkeiten in ThemisDB zu implementieren.

**Implementierungsreihenfolge:** 6 Phasen, sequentiell (Phase N+1 benötigt Phase N)

---

## Phase 1: Schema Manager (Foundation)

**Priorität:** KRITISCH - Alle anderen Phasen abhängig  
**Aufwand:** ~500 LOC  
**Dauer:** Sprint 1-2

### 1.1 Neue Dateien erstellen

#### Datei 1: `include/metadata/schema_manager.h`

**Ort:** `/home/runner/work/ThemisDB/ThemisDB/include/metadata/schema_manager.h`  
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace metadata {

using json = nlohmann::json;

/**
 * @brief Centralized schema metadata management and introspection
 * 
 * Provides read-only access to database schema information including:
 * - Table/Node definitions
 * - Column/Property schemas with types
 * - Index metadata (from SecondaryIndexManager)
 * - Statistics and cardinalities
 * 
 * Caching strategy: Refresh every 60 seconds to balance freshness vs performance
 */
class SchemaManager {
public:
    /**
     * @brief Property metadata
     */
    struct PropertyInfo {
        std::string name;
        std::string type;        // "string", "integer", "float", "boolean", "timestamp", "json"
        bool nullable = true;
        bool indexed = false;
        std::string index_type;  // "", "secondary", "unique", "fulltext", "geo", "range", "ttl"
        
        // For indexed properties
        std::optional<int> index_rank;        // For fulltext: BM25 rank
        std::optional<std::string> index_config; // JSON config for complex indexes
    };

    /**
     * @brief Table/Collection schema
     */
    struct TableSchema {
        std::string name;
        std::string type;                    // "node", "edge", "document", "timeseries"
        std::vector<PropertyInfo> properties;
        size_t estimated_rows = 0;
        size_t storage_bytes = 0;
        std::string created_at;              // ISO 8601 timestamp
        std::string last_modified;           // ISO 8601 timestamp
        
        // For graph edges
        std::optional<std::string> from_type;
        std::optional<std::string> to_type;
    };

    /**
     * @brief Relationship/Edge schema
     */
    struct RelationshipSchema {
        std::string type;                    // Edge type name (e.g., "FOLLOWS")
        std::string from_table;
        std::string to_table;
        std::vector<PropertyInfo> properties;
        size_t count = 0;
        std::string direction;               // "directed", "undirected"
    };

    /**
     * @brief Database-level metadata
     */
    struct DatabaseMetadata {
        std::string name = "ThemisDB";
        std::string version;
        std::string edition;                 // "Community", "Enterprise"
        std::vector<std::string> enabled_features;
        size_t total_entities = 0;
        size_t total_indexes = 0;
        size_t storage_size_bytes = 0;
        std::string started_at;
        std::string uptime_seconds;
    };

    explicit SchemaManager(RocksDBWrapper& db, SecondaryIndexManager& index_mgr);
    ~SchemaManager() = default;

    // Disable copy, allow move
    SchemaManager(const SchemaManager&) = delete;
    SchemaManager& operator=(const SchemaManager&) = delete;
    SchemaManager(SchemaManager&&) = default;
    SchemaManager& operator=(SchemaManager&&) = default;

    // Schema discovery
    std::vector<TableSchema> getAllTables() const;
    std::optional<TableSchema> getTable(std::string_view name) const;
    std::vector<RelationshipSchema> getAllRelationships() const;
    
    // Property introspection
    std::vector<std::string> getTableProperties(std::string_view table) const;
    std::optional<PropertyInfo> getProperty(std::string_view table, std::string_view property) const;
    
    // Index introspection
    std::vector<std::string> getIndexedProperties(std::string_view table) const;
    std::string getIndexType(std::string_view table, std::string_view property) const;
    
    // Statistics
    size_t getTableRowCount(std::string_view table) const;
    size_t getTableStorageSize(std::string_view table) const;
    DatabaseMetadata getDatabaseMetadata() const;

    // JSON export for APIs
    json toJSON() const;
    json tableToJSON(const TableSchema& table) const;
    json relationshipToJSON(const RelationshipSchema& rel) const;
    json metadataToJSON(const DatabaseMetadata& meta) const;

    // Cache control
    void refreshCache();
    void invalidateCache();
    std::chrono::system_clock::time_point getLastRefresh() const { return last_refresh_; }

private:
    RocksDBWrapper& db_;
    SecondaryIndexManager& index_mgr_;
    
    // Cached schema information (mutable for lazy refresh)
    mutable std::unordered_map<std::string, TableSchema> table_cache_;
    mutable std::unordered_map<std::string, RelationshipSchema> relationship_cache_;
    mutable DatabaseMetadata metadata_cache_;
    mutable std::chrono::system_clock::time_point last_refresh_;
    mutable std::mutex cache_mutex_;
    
    // Cache refresh interval
    static constexpr std::chrono::seconds CACHE_REFRESH_INTERVAL{60};
    
    // Internal helpers
    bool needsRefresh() const;
    void refreshCacheInternal() const;
    TableSchema discoverTableSchema(std::string_view table) const;
    std::string inferPropertyType(const std::string& sample_value) const;
    PropertyInfo discoverPropertyInfo(std::string_view table, std::string_view property) const;
    std::vector<std::string> scanTables() const;
    size_t estimateRowCount(std::string_view table) const;
};

} // namespace metadata
} // namespace themis
```

**Begründung:** Zentrale Klasse für alle Schema-Informationen. Nutzt existierende RocksDB und SecondaryIndexManager.

---

#### Datei 2: `src/metadata/schema_manager.cpp`

**Ort:** `/home/runner/work/ThemisDB/ThemisDB/src/metadata/schema_manager.cpp`  
**Aktion:** NEU ERSTELLEN

```cpp
#include "metadata/schema_manager.h"
#include "storage/base_entity.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <rocksdb/db.h>

namespace themis {
namespace metadata {

SchemaManager::SchemaManager(RocksDBWrapper& db, SecondaryIndexManager& index_mgr)
    : db_(db), index_mgr_(index_mgr), last_refresh_(std::chrono::system_clock::time_point::min()) {
    spdlog::info("SchemaManager initialized");
}

bool SchemaManager::needsRefresh() const {
    auto now = std::chrono::system_clock::now();
    return (now - last_refresh_) > CACHE_REFRESH_INTERVAL;
}

void SchemaManager::refreshCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    refreshCacheInternal();
}

void SchemaManager::refreshCacheInternal() const {
    spdlog::debug("SchemaManager: Refreshing cache");
    
    // Clear caches
    table_cache_.clear();
    relationship_cache_.clear();
    
    // Scan for tables
    auto table_names = scanTables();
    
    // Discover schema for each table
    for (const auto& table_name : table_names) {
        try {
            auto schema = discoverTableSchema(table_name);
            table_cache_[table_name] = schema;
        } catch (const std::exception& e) {
            spdlog::warn("SchemaManager: Failed to discover schema for table '{}': {}", 
                        table_name, e.what());
        }
    }
    
    // Update database metadata
    metadata_cache_.version = THEMIS_VERSION;
    #ifdef THEMIS_ENTERPRISE
    metadata_cache_.edition = "Enterprise";
    #else
    metadata_cache_.edition = "Community";
    #endif
    
    metadata_cache_.enabled_features.clear();
    #ifdef THEMIS_ENABLE_LLM
    metadata_cache_.enabled_features.push_back("llm");
    #endif
    #ifdef THEMIS_ENABLE_CUDA
    metadata_cache_.enabled_features.push_back("cuda");
    #endif
    #ifdef THEMIS_ENABLE_MCP
    metadata_cache_.enabled_features.push_back("mcp");
    #endif
    
    metadata_cache_.total_entities = 0;
    metadata_cache_.total_indexes = 0;
    for (const auto& [name, schema] : table_cache_) {
        metadata_cache_.total_entities += schema.estimated_rows;
        for (const auto& prop : schema.properties) {
            if (prop.indexed) {
                metadata_cache_.total_indexes++;
            }
        }
    }
    
    last_refresh_ = std::chrono::system_clock::now();
    spdlog::debug("SchemaManager: Cache refreshed, found {} tables", table_cache_.size());
}

std::vector<std::string> SchemaManager::scanTables() const {
    std::vector<std::string> tables;
    std::unordered_set<std::string> seen;
    
    // Scan RocksDB keys to discover table names
    // Key format: "table_name:entity_id" or "table_name:..."
    rocksdb::Iterator* it = db_.getRawDB()->NewIterator(rocksdb::ReadOptions());
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        
        // Extract table name (before first colon)
        size_t colon_pos = key.find(':');
        if (colon_pos != std::string::npos && colon_pos > 0) {
            std::string table_name = key.substr(0, colon_pos);
            
            // Skip internal tables
            if (table_name.starts_with("_") || 
                table_name.starts_with("idx:") ||
                table_name.starts_with("meta:")) {
                continue;
            }
            
            if (seen.find(table_name) == seen.end()) {
                tables.push_back(table_name);
                seen.insert(table_name);
            }
        }
    }
    
    delete it;
    return tables;
}

TableSchema SchemaManager::discoverTableSchema(std::string_view table) const {
    TableSchema schema;
    schema.name = std::string(table);
    schema.type = "node";  // Default, can be inferred from data
    
    // Sample first entity to discover properties
    std::string prefix = fmt::format("{}:", table);
    rocksdb::Iterator* it = db_.getRawDB()->NewIterator(rocksdb::ReadOptions());
    
    std::unordered_set<std::string> property_names;
    int sample_count = 0;
    const int MAX_SAMPLES = 10;
    
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix) && sample_count < MAX_SAMPLES; it->Next()) {
        try {
            // Parse entity JSON to discover properties
            json entity = json::parse(it->value().ToString());
            
            for (auto& [key, value] : entity.items()) {
                if (property_names.find(key) == property_names.end()) {
                    PropertyInfo prop_info = discoverPropertyInfo(table, key);
                    schema.properties.push_back(prop_info);
                    property_names.insert(key);
                }
            }
            
            sample_count++;
        } catch (const json::exception& e) {
            // Skip malformed entities
            continue;
        }
    }
    
    delete it;
    
    // Estimate row count and storage size
    schema.estimated_rows = estimateRowCount(table);
    schema.storage_bytes = schema.estimated_rows * 1024; // Rough estimate
    
    // Set timestamps
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_now);
    schema.created_at = buffer;
    schema.last_modified = buffer;
    
    return schema;
}

PropertyInfo SchemaManager::discoverPropertyInfo(std::string_view table, std::string_view property) const {
    PropertyInfo info;
    info.name = std::string(property);
    info.type = "string";  // Default
    info.nullable = true;
    
    // Check if property is indexed
    info.indexed = index_mgr_.hasIndex(table, property) ||
                   index_mgr_.hasRangeIndex(table, property) ||
                   index_mgr_.hasSparseIndex(table, property) ||
                   index_mgr_.hasGeoIndex(table, property) ||
                   index_mgr_.hasTTLIndex(table, property) ||
                   index_mgr_.hasFulltextIndex(table, property);
    
    if (info.indexed) {
        if (index_mgr_.hasFulltextIndex(table, property)) {
            info.index_type = "fulltext";
        } else if (index_mgr_.hasGeoIndex(table, property)) {
            info.index_type = "geo";
        } else if (index_mgr_.hasRangeIndex(table, property)) {
            info.index_type = "range";
        } else if (index_mgr_.hasSparseIndex(table, property)) {
            info.index_type = "sparse";
        } else if (index_mgr_.hasTTLIndex(table, property)) {
            info.index_type = "ttl";
        } else {
            info.index_type = "secondary";
        }
    }
    
    return info;
}

size_t SchemaManager::estimateRowCount(std::string_view table) const {
    size_t count = 0;
    std::string prefix = fmt::format("{}:", table);
    
    rocksdb::Iterator* it = db_.getRawDB()->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        count++;
    }
    delete it;
    
    return count;
}

std::vector<TableSchema> SchemaManager::getAllTables() const {
    if (needsRefresh()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (needsRefresh()) {  // Double-check after acquiring lock
            refreshCacheInternal();
        }
    }
    
    std::vector<TableSchema> tables;
    for (const auto& [name, schema] : table_cache_) {
        tables.push_back(schema);
    }
    return tables;
}

std::optional<TableSchema> SchemaManager::getTable(std::string_view name) const {
    if (needsRefresh()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (needsRefresh()) {
            refreshCacheInternal();
        }
    }
    
    auto it = table_cache_.find(std::string(name));
    if (it != table_cache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<RelationshipSchema> SchemaManager::getAllRelationships() const {
    std::vector<RelationshipSchema> rels;
    for (const auto& [name, schema] : relationship_cache_) {
        rels.push_back(schema);
    }
    return rels;
}

DatabaseMetadata SchemaManager::getDatabaseMetadata() const {
    if (needsRefresh()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (needsRefresh()) {
            refreshCacheInternal();
        }
    }
    return metadata_cache_;
}

json SchemaManager::toJSON() const {
    auto tables = getAllTables();
    auto relationships = getAllRelationships();
    auto metadata = getDatabaseMetadata();
    
    json result = {
        {"database", metadataToJSON(metadata)},
        {"tables", json::array()},
        {"relationships", json::array()}
    };
    
    for (const auto& table : tables) {
        result["tables"].push_back(tableToJSON(table));
    }
    
    for (const auto& rel : relationships) {
        result["relationships"].push_back(relationshipToJSON(rel));
    }
    
    return result;
}

json SchemaManager::tableToJSON(const TableSchema& table) const {
    json properties = json::array();
    for (const auto& prop : table.properties) {
        json prop_json = {
            {"name", prop.name},
            {"type", prop.type},
            {"nullable", prop.nullable},
            {"indexed", prop.indexed}
        };
        if (prop.indexed) {
            prop_json["index_type"] = prop.index_type;
        }
        properties.push_back(prop_json);
    }
    
    return {
        {"name", table.name},
        {"type", table.type},
        {"properties", properties},
        {"estimated_rows", table.estimated_rows},
        {"storage_bytes", table.storage_bytes},
        {"created_at", table.created_at},
        {"last_modified", table.last_modified}
    };
}

json SchemaManager::relationshipToJSON(const RelationshipSchema& rel) const {
    return {
        {"type", rel.type},
        {"from", rel.from_table},
        {"to", rel.to_table},
        {"count", rel.count},
        {"direction", rel.direction}
    };
}

json SchemaManager::metadataToJSON(const DatabaseMetadata& meta) const {
    return {
        {"name", meta.name},
        {"version", meta.version},
        {"edition", meta.edition},
        {"enabled_features", meta.enabled_features},
        {"total_entities", meta.total_entities},
        {"total_indexes", meta.total_indexes},
        {"storage_size_bytes", meta.storage_size_bytes}
    };
}

void SchemaManager::invalidateCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    last_refresh_ = std::chrono::system_clock::time_point::min();
}

} // namespace metadata
} // namespace themis
```

**Begründung:** Kern-Implementation mit RocksDB-Scanning und Index-Introspection.

---

### 1.2 CMakeLists.txt aktualisieren

**Datei:** `CMakeLists.txt`  
**Ort:** Root-Verzeichnis  
**Aktion:** ZEILEN HINZUFÜGEN

**Suchen Sie nach:**
```cmake
# Source files
set(THEMIS_SOURCES
    src/base/module_loader.cpp
    src/storage/rocksdb_wrapper.cpp
    ...
```

**Fügen Sie hinzu:**
```cmake
    src/metadata/schema_manager.cpp
```

**Und später bei den Headers:**
```cmake
    include/metadata/schema_manager.h
```

---

### 1.3 Tests erstellen

**Datei:** `tests/test_schema_manager.cpp`  
**Ort:** `/home/runner/work/ThemisDB/ThemisDB/tests/test_schema_manager.cpp`  
**Aktion:** NEU ERSTELLEN

```cpp
#include <gtest/gtest.h>
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <filesystem>

using namespace themis;
using namespace themis::metadata;

class SchemaManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "themis_schema_test";
        std::filesystem::create_directories(test_dir_);
        
        db_ = std::make_unique<RocksDBWrapper>(test_dir_.string());
        index_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
        schema_mgr_ = std::make_unique<SchemaManager>(*db_, *index_mgr_);
        
        // Insert test data
        insertTestData();
    }
    
    void TearDown() override {
        schema_mgr_.reset();
        index_mgr_.reset();
        db_.reset();
        std::filesystem::remove_all(test_dir_);
    }
    
    void insertTestData() {
        // Insert users
        db_->put("users:1", R"({"name": "Alice", "age": 30, "email": "alice@example.com"})");
        db_->put("users:2", R"({"name": "Bob", "age": 25, "email": "bob@example.com"})");
        
        // Create index
        index_mgr_->createIndex("users", "email", true);
    }
    
    std::filesystem::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> index_mgr_;
    std::unique_ptr<SchemaManager> schema_mgr_;
};

TEST_F(SchemaManagerTest, DiscoversTables) {
    auto tables = schema_mgr_->getAllTables();
    ASSERT_FALSE(tables.empty());
    
    bool found_users = false;
    for (const auto& table : tables) {
        if (table.name == "users") {
            found_users = true;
            EXPECT_EQ(table.estimated_rows, 2);
        }
    }
    EXPECT_TRUE(found_users);
}

TEST_F(SchemaManagerTest, DiscoversProperties) {
    auto table = schema_mgr_->getTable("users");
    ASSERT_TRUE(table.has_value());
    
    bool found_email = false;
    for (const auto& prop : table->properties) {
        if (prop.name == "email") {
            found_email = true;
            EXPECT_TRUE(prop.indexed);
            EXPECT_EQ(prop.index_type, "secondary");
        }
    }
    EXPECT_TRUE(found_email);
}

TEST_F(SchemaManagerTest, JSONExport) {
    auto json_schema = schema_mgr_->toJSON();
    EXPECT_TRUE(json_schema.contains("database"));
    EXPECT_TRUE(json_schema.contains("tables"));
    EXPECT_TRUE(json_schema["tables"].is_array());
}
```

---

## Phase 2: REST API Endpoints

**Priorität:** HOCH  
**Aufwand:** ~300 LOC  
**Dauer:** Sprint 3  
**Abhängigkeit:** Phase 1 abgeschlossen

### 2.1 Handler-Klasse erstellen

**Datei:** `include/server/schema_api_handler.h`  
**Ort:** `/home/runner/work/ThemisDB/ThemisDB/include/server/schema_api_handler.h`  
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include "server/http_server.h"
#include "metadata/schema_manager.h"
#include <memory>

namespace themis {
namespace server {

/**
 * @brief HTTP API handler for schema introspection endpoints
 */
class SchemaApiHandler {
public:
    explicit SchemaApiHandler(std::shared_ptr<metadata::SchemaManager> schema_manager);
    
    void registerRoutes(HttpServer& server);
    
private:
    void handleGetSchema(const Request& req, Response& res);
    void handleGetTables(const Request& req, Response& res);
    void handleGetTable(const Request& req, Response& res);
    void handleGetCapabilities(const Request& req, Response& res);
    
    std::shared_ptr<metadata::SchemaManager> schema_manager_;
};

} // namespace server
} // namespace themis
```

---

(Continued in next message due to length constraints...)

Would you like me to continue with the complete detailed implementation guide? I'll create the full document covering all 6 phases with exact file locations, code to insert, and the precise order of operations.
# Detaillierte Implementierungsvorgabe: Agentic AI Self-Awareness

**Datum:** 11. Januar 2026 (überarbeitet: 14. Mai 2026)
**Version:** 1.1
**Kategorie:** Implementation Guide
**Basierend auf:** [AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)
**Implementierungsstatus:** Alle 6 Phasen abgeschlossen (Stand: April 2026)

---

## Abstract / Zusammenfassung

Dieses Dokument beschreibt die Implementierungsvorgabe für Agentic AI Self-Awareness in ThemisDB. Es legt in sechs sequenziellen Phasen fest, welche Artefakte (Header, Implementierungen, Tests, API-Endpunkte) zu erstellen sind, damit ThemisDB seine eigene Struktur, Fähigkeiten und den Laufzeitzustand maschinell und in natürlicher Sprache ausdrücken kann.

Der Leitfaden entstand im Januar 2026 auf Basis der Forschungsanalyse in `AGENTIC_AI_SELF_AWARENESS_RESEARCH.md` und wurde im Mai 2026 gegen den aktuellen Codestand (April 2026) abgeglichen. Alle sechs Phasen sind produktiv implementiert; die Checklisten und Phasenstatus im Abschnitt „Zusammenfassung & Nächste Schritte" wurden entsprechend aktualisiert.

---

## Introduction / Einleitung

### Problemstellung

Für Agentic-AI-Szenarien — insbesondere beim Einsatz von Model Context Protocol (MCP), LLM-basierten Abfragen und autonomen Agenten — muss ThemisDB Fragen wie „Was kannst du?", „Welche Tabellen und Indizes kennst du?" oder „Wie läuft dieser Query-Plan?" belastbar beantworten. Ohne strukturierte Selbstauskunft kann ein Agent keine informierten Entscheidungen über Datenmodell, Abfrageoptimierung oder Ressourcenplanung treffen.

### Ziel dieses Leitfadens

Dieses Dokument gibt Implementierern eine genaue, phasengeordnete Anleitung, welche Dateien erstellt oder erweitert werden müssen und in welcher Reihenfolge — von der Basis-Schemaerkennung (Phase 1) bis zur LoRA-RAID-Introspection (Phase 6).

### Terminologie (vereinheitlicht)

- **AQL**: ThemisDB Query-Layer einschließlich Plan-/Explain-Funktionen (`explainAql`, Graph-EXPLAIN-Endpunkt).
- **Multi-Model**: Kombination mehrerer Datenmodelle im selben System (relational, graph, vector, document, geospatial, time-series).
- **Konsistenzmodell**: ACID/MVCC als zentrale Transaktionsgrundlage.
- **Self-Awareness**: Fähigkeit, eigene Struktur/Fähigkeiten über APIs/Protokolle maschinenlesbar bereitzustellen (MCP, HTTP, GraphQL, PostgreSQL-Introspection).
- **SchemaManager**: Klasse `themis::SchemaManager` in `include/metadata/schema_manager.h`; zuständig für Tabellenerkennung, Property-Typen, Index-Metadaten und Thread-sicheres Caching.
- **LoRAIntrospectionManager**: Klasse `themis::llm::LoRAIntrospectionManager` in `include/llm/lora_introspection_manager.h`; kapselt Adapter-, GPU- und RAID-Metadaten.

### Abgrenzung

- Dieser Leitfaden beschreibt die Implementierungsstruktur und die relevanten Code-Artefakte; die wissenschaftliche Bewertung (Claim-Matrix, Protokollvergleiche, Messungen) erfolgt in `AGENTIC_AI_SELF_AWARENESS_RESEARCH.md`.
- Aufbau- und Sicherheitsfragen (RBAC, Audit) liegen außerhalb des Umfangs dieses Dokuments.

---

## Methodik / Ansatz

Die Implementierung folgt einem sechsphasigen, sequenziellen Aufbau: Jede Phase setzt die vorherigen voraus. Die Reihenfolge wurde nach Abhängigkeitstiefe gewählt — Datenbankschicht zuerst, Protokoll-/NLP-Integration zuletzt.

| Phase | Komponente | Aufwand (LOC) | Abhängigkeiten |
|-------|-----------|---------------|----------------|
| 1 | SchemaManager (Foundation) | ~500 | RocksDB, SecondaryIndexManager |
| 2 | REST API Endpoints | ~300 | Phase 1 |
| 3 | MCP Integration | ~200 | Phase 1, 2 |
| 4 | Natural Language Self-Awareness | ~400 | Phase 1–3 |
| 5 | Domain-Semantic Awareness | ~800 | Phase 1–4 |
| 6 | LoRA-RAID + Infrastructure Awareness | ~1300 | Phase 1–5 |

Die nachfolgenden Abschnitte enthalten für jede Phase die genauen Datei-/Code-Artefakte.

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

**Datei:** `src/server/schema_api_handler.cpp`
**Ort:** `/home/runner/work/ThemisDB/ThemisDB/src/server/schema_api_handler.cpp`
**Aktion:** NEU ERSTELLEN

```cpp
#include "server/schema_api_handler.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace server {

SchemaApiHandler::SchemaApiHandler(std::shared_ptr<metadata::SchemaManager> schema_manager)
    : schema_manager_(schema_manager) {}

void SchemaApiHandler::registerRoutes(HttpServer& server) {
    server.registerRoute("GET", "/api/v1/schema",
        [this](const Request& req, Response& res) { handleGetSchema(req, res); });

    server.registerRoute("GET", "/api/v1/schema/tables",
        [this](const Request& req, Response& res) { handleGetTables(req, res); });

    server.registerRoute("GET", "/api/v1/schema/tables/:name",
        [this](const Request& req, Response& res) { handleGetTable(req, res); });

    server.registerRoute("GET", "/api/v1/capabilities",
        [this](const Request& req, Response& res) { handleGetCapabilities(req, res); });
}

void SchemaApiHandler::handleGetSchema(const Request& req, Response& res) {
    try {
        auto schema_json = schema_manager_->toJSON();
        res.setStatus(200);
        res.setHeader("Content-Type", "application/json");
        res.setBody(schema_json.dump());
    } catch (const std::exception& e) {
        spdlog::error("SchemaApiHandler: Failed to get schema: {}", e.what());
        res.setStatus(500);
        res.setBody(R"({"error": "Internal server error"})");
    }
}

void SchemaApiHandler::handleGetTables(const Request& req, Response& res) {
    try {
        auto tables = schema_manager_->getAllTables();
        json result = json::array();

        for (const auto& table : tables) {
            result.push_back(schema_manager_->tableToJSON(table));
        }

        res.setStatus(200);
        res.setHeader("Content-Type", "application/json");
        res.setBody(result.dump());
    } catch (const std::exception& e) {
        spdlog::error("SchemaApiHandler: Failed to get tables: {}", e.what());
        res.setStatus(500);
        res.setBody(R"({"error": "Internal server error"})");
    }
}

void SchemaApiHandler::handleGetTable(const Request& req, Response& res) {
    try {
        std::string table_name = req.getParam("name");
        auto table = schema_manager_->getTable(table_name);

        if (!table.has_value()) {
            res.setStatus(404);
            res.setBody(R"({"error": "Table not found"})");
            return;
        }

        res.setStatus(200);
        res.setHeader("Content-Type", "application/json");
        res.setBody(schema_manager_->tableToJSON(*table).dump());
    } catch (const std::exception& e) {
        spdlog::error("SchemaApiHandler: Failed to get table: {}", e.what());
        res.setStatus(500);
        res.setBody(R"({"error": "Internal server error"})");
    }
}

void SchemaApiHandler::handleGetCapabilities(const Request& req, Response& res) {
    try {
        auto metadata = schema_manager_->getDatabaseMetadata();
        auto capabilities = schema_manager_->metadataToJSON(metadata);

        res.setStatus(200);
        res.setHeader("Content-Type", "application/json");
        res.setBody(capabilities.dump());
    } catch (const std::exception& e) {
        spdlog::error("SchemaApiHandler: Failed to get capabilities: {}", e.what());
        res.setStatus(500);
        res.setBody(R"({"error": "Internal server error"})");
    }
}

} // namespace server
} // namespace themis
```

### 2.3 HttpServer Integration

**Datei:** `src/server/http_server.cpp`
**Aktion:** ZEILEN HINZUFÜGEN

**Suchen Sie nach der Initialisierung des HttpServer (z.B. im Konstruktor oder `start()` Methode):**

```cpp
void HttpServer::start() {
    // Existing initialization...

    // ADD THIS: Register schema API routes
    auto schema_manager = std::make_shared<metadata::SchemaManager>(
        *db_wrapper_, *index_manager_);

    auto schema_handler = std::make_shared<server::SchemaApiHandler>(schema_manager);
    schema_handler->registerRoutes(*this);

    // Continue with existing code...
}
```

---

## Phase 3: MCP Integration

**Priorität:** HOCH
**Aufwand:** ~200 LOC
**Dauer:** Sprint 4
**Abhängigkeit:** Phase 1 + 2 abgeschlossen

### 3.1 MCP Server aktualisieren

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** METHODE ERSETZEN

**Suchen Sie nach der Stub-Implementation:**

```cpp
json McpServer::toolGetSchema(const json& args) {
    return {
        {"nodes", json::array()},  // Empty!
        {"message", "Schema discovery requires full query engine integration"}
    };
}
```

**Ersetzen Sie mit:**

```cpp
json McpServer::toolGetSchema(const json& args) {
    try {
        if (!schema_manager_) {
            return {
                {"status", "error"},
                {"message", "SchemaManager not initialized"}
            };
        }

        auto tables = schema_manager_->getAllTables();
        json nodes = json::array();

        for (const auto& table : tables) {
            json node = {
                {"name", table.name},
                {"type", table.type},
                {"properties", json::array()},
                {"estimated_rows", table.estimated_rows},
                {"storage_bytes", table.storage_bytes}
            };

            for (const auto& prop : table.properties) {
                node["properties"].push_back({
                    {"name", prop.name},
                    {"type", prop.type},
                    {"nullable", prop.nullable},
                    {"indexed", prop.indexed},
                    {"index_type", prop.index_type}
                });
            }

            nodes.push_back(node);
        }

        return {
            {"status", "success"},
            {"nodes", nodes},
            {"integration_level", "full"}
        };

    } catch (const std::exception& e) {
        spdlog::error("MCP toolGetSchema failed: {}", e.what());
        return {
            {"status", "error"},
            {"message", e.what()}
        };
    }
}
```

### 3.2 MCP Resource aktualisieren

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** METHODE ERSETZEN

**Suchen Sie nach:**

```cpp
json McpServer::resourceSchema(const std::string& uri) {
    // Stub implementation
    return {{"status", "not_implemented"}};
}
```

**Ersetzen Sie mit:**

```cpp
json McpServer::resourceSchema(const std::string& uri) {
    try {
        if (!schema_manager_) {
            return {
                {"status", "error"},
                {"message", "SchemaManager not initialized"}
            };
        }

        // Parse URI: "schema://database" or "schema://table/table_name"
        if (uri == "schema://database") {
            return schema_manager_->toJSON();
        } else if (uri.starts_with("schema://table/")) {
            std::string table_name = uri.substr(15);  // Remove "schema://table/"
            auto table = schema_manager_->getTable(table_name);

            if (!table.has_value()) {
                return {
                    {"status", "error"},
                    {"message", "Table not found: " + table_name}
                };
            }

            return schema_manager_->tableToJSON(*table);
        }

        return {
            {"status", "error"},
            {"message", "Invalid schema URI"}
        };

    } catch (const std::exception& e) {
        spdlog::error("MCP resourceSchema failed: {}", e.what());
        return {
            {"status", "error"},
            {"message", e.what()}
        };
    }
}
```

### 3.3 Statistics Tool hinzufügen

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** METHODE AKTUALISIEREN

```cpp
json McpServer::toolGetStats(const json& args) {
    try {
        if (!schema_manager_) {
            return {{"status", "error"}, {"message", "SchemaManager not initialized"}};
        }

        auto metadata = schema_manager_->getDatabaseMetadata();
        auto tables = schema_manager_->getAllTables();

        json stats = {
            {"database", {
                {"name", metadata.name},
                {"version", metadata.version},
                {"edition", metadata.edition},
                {"total_entities", metadata.total_entities},
                {"total_indexes", metadata.total_indexes},
                {"storage_size_bytes", metadata.storage_size_bytes}
            }},
            {"tables", json::array()}
        };

        for (const auto& table : tables) {
            stats["tables"].push_back({
                {"name", table.name},
                {"type", table.type},
                {"rows", table.estimated_rows},
                {"storage_bytes", table.storage_bytes}
            });
        }

        return {
            {"status", "success"},
            {"stats", stats}
        };

    } catch (const std::exception& e) {
        spdlog::error("MCP toolGetStats failed: {}", e.what());
        return {{"status", "error"}, {"message", e.what()}};
    }
}
```

### 3.4 MCP Server Konstruktor aktualisieren

**Datei:** `include/mcp/mcp_server.h`
**Aktion:** MEMBER HINZUFÜGEN

```cpp
class McpServer {
private:
    // Existing members...
    std::shared_ptr<metadata::SchemaManager> schema_manager_;  // ADD THIS
};
```

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** KONSTRUKTOR AKTUALISIEREN

```cpp
McpServer::McpServer(/* existing parameters */)
    : /* existing initializations */ {

    // ADD THIS: Initialize SchemaManager
    schema_manager_ = std::make_shared<metadata::SchemaManager>(
        *db_wrapper_, *index_manager_);

    // Existing code...
}
```

---

## Phase 4: Natural Language Self-Awareness

**Priorität:** MITTEL
**Aufwand:** ~400 LOC
**Dauer:** Sprint 5-7
**Abhängigkeit:** Phase 1-3 abgeschlossen

### 4.1 Introspection Tool erstellen

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** NEUE METHODE HINZUFÜGEN

```cpp
json McpServer::toolIntrospectDatabase(const json& args) {
    try {
        std::string question = args.value("question", "");

        if (question.empty()) {
            return {{"status", "error"}, {"message", "No question provided"}};
        }

        // Detect question type
        std::string question_lower = question;
        std::transform(question_lower.begin(), question_lower.end(),
                      question_lower.begin(), ::tolower);

        std::string answer;

        // "Was kannst du?" / "What can you do?"
        if (question_lower.find("kannst du") != std::string::npos ||
            question_lower.find("can you do") != std::string::npos ||
            question_lower.find("capabilities") != std::string::npos) {
            answer = generateCapabilitiesAnswer();
        }
        // "Wo sind die Daten?" / "Where is the data?"
        else if (question_lower.find("wo sind") != std::string::npos ||
                 question_lower.find("where") != std::string::npos) {
            answer = generateDataLocationAnswer();
        }
        // "Wie sind die Daten aufgebaut?" / "How is data structured?"
        else if (question_lower.find("aufgebaut") != std::string::npos ||
                 question_lower.find("structured") != std::string::npos ||
                 question_lower.find("schema") != std::string::npos) {
            answer = generateSchemaAnswer();
        }
        // "Was ist deine Aufgabe?" / "What is your purpose?"
        else if (question_lower.find("aufgabe") != std::string::npos ||
                 question_lower.find("purpose") != std::string::npos) {
            answer = generatePurposeAnswer();
        }
        else {
            answer = "Ich kann folgende Fragen beantworten:\n"
                    "- Was kannst du?\n"
                    "- Wo sind die Daten?\n"
                    "- Wie sind die Daten aufgebaut?\n"
                    "- Was ist deine Aufgabe?";
        }

        return {
            {"status", "success"},
            {"question", question},
            {"answer", answer}
        };

    } catch (const std::exception& e) {
        spdlog::error("MCP toolIntrospectDatabase failed: {}", e.what());
        return {{"status", "error"}, {"message", e.what()}};
    }
}

std::string McpServer::generateCapabilitiesAnswer() {
    auto metadata = schema_manager_->getDatabaseMetadata();

    std::string answer = fmt::format(
        "Ich bin ThemisDB {}, eine Multi-Model-Datenbank.\n\n"
        "Meine Fähigkeiten:\n"
        "- ACID Transaktionen mit MVCC\n"
        "- Multi-Model: Relational, Graph, Vector, Document, Time-Series\n"
        "- {} Tabellen mit {} Entitäten\n"
        "- {} Indexes für schnelle Queries\n"
        "- Storage: {} MB\n",
        metadata.version,
        schema_manager_->getAllTables().size(),
        metadata.total_entities,
        metadata.total_indexes,
        metadata.storage_size_bytes / (1024 * 1024)
    );

    if (!metadata.enabled_features.empty()) {
        answer += "\nOptionale Features aktiviert:\n";
        for (const auto& feature : metadata.enabled_features) {
            answer += fmt::format("- {}\n", feature);
        }
    }

    return answer;
}

std::string McpServer::generateDataLocationAnswer() {
    auto tables = schema_manager_->getAllTables();

    std::string answer = fmt::format(
        "Die Daten sind in {} Tabellen organisiert:\n\n",
        tables.size()
    );

    for (const auto& table : tables) {
        answer += fmt::format(
            "- {}: {} Einträge ({} MB)\n",
            table.name,
            table.estimated_rows,
            table.storage_bytes / (1024 * 1024)
        );
    }

    return answer;
}

std::string McpServer::generateSchemaAnswer() {
    auto tables = schema_manager_->getAllTables();

    std::string answer = "Datenbank-Schema:\n\n";

    for (const auto& table : tables) {
        answer += fmt::format("Tabelle: {} ({})\n", table.name, table.type);
        answer += "Properties:\n";

        for (const auto& prop : table.properties) {
            std::string index_info = prop.indexed ?
                fmt::format(" [{}]", prop.index_type) : "";
            answer += fmt::format("  - {}: {}{}\n",
                                 prop.name, prop.type, index_info);
        }
        answer += "\n";
    }

    return answer;
}

std::string McpServer::generatePurposeAnswer() {
    return "Meine Aufgabe ist es, Daten effizient und sicher zu speichern, "
           "zu verwalten und abzufragen. Ich biete ACID-Transaktionen, "
           "Multi-Model-Unterstützung und optionale LLM-Integration für "
           "intelligente Datenverarbeitung.";
}
```

### 4.2 Tool Registration

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** IN `registerTools()` METHODE HINZUFÜGEN

```cpp
void McpServer::registerTools() {
    // Existing tools...

    // ADD THIS:
    registerTool(
        "introspect_database",
        "Ask the database about itself in natural language",
        {
            {"type", "object"},
            {"properties", {
                {"question", {
                    {"type", "string"},
                    {"description", "Natural language question about the database"}
                }}
            }},
            {"required", {"question"}}
        },
        [this](const json& args) { return toolIntrospectDatabase(args); }
    );
}
```

### 4.3 LLM System Prompt erweitern

**Datei:** `src/llm/llama_wrapper.cpp`
**Aktion:** SYSTEM PROMPT AKTUALISIEREN

```cpp
std::string LlamaWrapper::buildSystemPrompt() {
    std::string base_prompt = "You are ThemisDB, an intelligent multi-model database.";

    // ADD THIS: Inject schema context if available
    if (schema_manager_) {
        try {
            auto metadata = schema_manager_->getDatabaseMetadata();
            auto tables = schema_manager_->getAllTables();

            base_prompt += fmt::format(
                "\n\nCurrent database state:"
                "\n- Version: {}"
                "\n- Edition: {}"
                "\n- Tables: {}"
                "\n- Total entities: {}"
                "\n- Total indexes: {}",
                metadata.version,
                metadata.edition,
                tables.size(),
                metadata.total_entities,
                metadata.total_indexes
            );

            if (!tables.empty()) {
                base_prompt += "\n\nAvailable tables:\n";
                for (const auto& table : tables) {
                    base_prompt += fmt::format("- {} ({} rows)\n",
                                             table.name, table.estimated_rows);
                }
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to inject schema context: {}", e.what());
        }
    }

    return base_prompt;
}
```

---

## Phase 5: Domain-Semantic Awareness

**Priorität:** MITTEL-HOCH
**Aufwand:** ~800 LOC
**Dauer:** Sprint 8-11
**Abhängigkeit:** Phase 1-4 abgeschlossen

### 5.1 Semantic Metadata Manager erstellen

**Datei:** `include/metadata/semantic_metadata_manager.h`
**Ort:** `/home/runner/work/ThemisDB/ThemisDB/include/metadata/semantic_metadata_manager.h`
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace metadata {

using json = nlohmann::json;

struct DomainContext {
    std::string domain;              // "Umweltschutz", "Healthcare", etc.
    std::vector<std::string> legal_framework;  // ["BImSchG", "VwVfG"]
    std::string organization;        // "Behörde für Umweltschutz Hamburg"
    std::string purpose;             // Business purpose description
};

struct TableSemantics {
    std::string business_purpose;
    std::vector<std::string> content_types;
    std::string legal_context;
    int retention_years;
};

class SemanticMetadataManager {
public:
    SemanticMetadataManager() = default;

    // Domain context management
    void setDomainContext(const DomainContext& context);
    DomainContext getDomainContext() const;

    // Table semantics
    void setTableSemantics(const std::string& table, const TableSemantics& semantics);
    TableSemantics getTableSemantics(const std::string& table) const;

    // LLM-assisted analysis
    void analyzeDataContent(const std::string& sample_data);
    std::vector<std::string> extractEntities(const std::string& content);

    // JSON export
    json toJSON() const;

private:
    DomainContext domain_context_;
    std::unordered_map<std::string, TableSemantics> table_semantics_;
};

} // namespace metadata
} // namespace themis
```

### 5.2 Natural Language Integration

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** METHODE ERWEITERN

```cpp
std::string McpServer::generateCapabilitiesAnswer() {
    auto metadata = schema_manager_->getDatabaseMetadata();
    std::string answer = /* ... existing code ... */;

    // ADD THIS: Include semantic context if available
    if (semantic_metadata_manager_) {
        try {
            auto domain = semantic_metadata_manager_->getDomainContext();

            if (!domain.domain.empty()) {
                answer += fmt::format(
                    "\n\nDomäne: {}\n"
                    "Organisation: {}\n"
                    "Zweck: {}\n",
                    domain.domain,
                    domain.organization,
                    domain.purpose
                );

                if (!domain.legal_framework.empty()) {
                    answer += "Rechtlicher Rahmen: ";
                    for (size_t i = 0; i < domain.legal_framework.size(); ++i) {
                        answer += domain.legal_framework[i];
                        if (i < domain.legal_framework.size() - 1) answer += ", ";
                    }
                    answer += "\n";
                }
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to include semantic context: {}", e.what());
        }
    }

    return answer;
}
```

---

## Phase 6: LoRA-RAID + Infrastructure Awareness

**Priorität:** MITTEL-HOCH
**Aufwand:** ~1300 LOC
**Dauer:** Sprint 12-18
**Abhängigkeit:** Phase 1-5 abgeschlossen

### 6.1 LoRA Introspection Manager erstellen

**Datei:** `include/llm/lora_introspection_manager.h`
**Ort:** `/home/runner/work/ThemisDB/ThemisDB/include/llm/lora_introspection_manager.h`
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include "llm/multi_lora_manager.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

struct LoadedAdapter {
    std::string lora_id;
    std::string path;
    std::string base_model;
    int rank;
    float alpha;
    std::vector<std::string> target_modules;

    // GPU placement
    int gpu_id;
    size_t vram_bytes;
    bool is_active;

    // Performance
    uint64_t inference_count;
    double avg_latency_ms;
    double tokens_per_second;

    // RAID info
    std::string shard_primary;
    std::vector<std::string> shard_replicas;
    std::string raid_mode;
};

struct GPUInfo {
    int device_id;
    std::string name;
    size_t total_vram_mb;
    size_t used_vram_mb;
    std::vector<std::string> loaded_adapters;
};

struct RAIDInfo {
    std::string mode;
    int num_shards;
    std::vector<std::string> shard_ids;
    std::map<std::string, std::vector<std::string>> adapter_distribution;
};

class LoRAIntrospectionManager {
public:
    explicit LoRAIntrospectionManager(MultiLoRAManager& lora_manager);

    std::vector<LoadedAdapter> getLoadedAdapters() const;
    LoadedAdapter getAdapter(const std::string& lora_id) const;

    std::vector<GPUInfo> getGPUInfo() const;
    RAIDInfo getRAIDInfo() const;

    json toJSON() const;

private:
    MultiLoRAManager& lora_manager_;
};

} // namespace llm
} // namespace themis
```

### 6.2 REST API Endpoints für LoRA

**Datei:** `include/server/lora_api_handler.h`
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include "server/http_server.h"
#include "llm/lora_introspection_manager.h"

namespace themis {
namespace server {

class LoRAApiHandler {
public:
    explicit LoRAApiHandler(std::shared_ptr<llm::LoRAIntrospectionManager> lora_manager);

    void registerRoutes(HttpServer& server);

private:
    void handleGetAdapters(const Request& req, Response& res);
    void handleGetAdapter(const Request& req, Response& res);
    void handleGetGPUs(const Request& req, Response& res);
    void handleGetRAID(const Request& req, Response& res);

    std::shared_ptr<llm::LoRAIntrospectionManager> lora_manager_;
};

} // namespace server
} // namespace themis
```

### 6.3 MCP Tools für LoRA

**Datei:** `src/mcp/mcp_server.cpp`
**Aktion:** NEUE TOOLS HINZUFÜGEN

```cpp
void McpServer::registerTools() {
    // Existing tools...

    // ADD THIS: LoRA introspection tools
    registerTool(
        "get_lora_adapters",
        "List all loaded LoRA adapters with their capabilities",
        {{"type", "object"}, {"properties", {}}},
        [this](const json& args) { return toolGetLoRAAdapters(args); }
    );

    registerTool(
        "get_lora_info",
        "Get detailed information about a specific LoRA adapter",
        {
            {"type", "object"},
            {"properties", {
                {"lora_id", {{"type", "string"}}}
            }},
            {"required", {"lora_id"}}
        },
        [this](const json& args) { return toolGetLoRAInfo(args); }
    );
}

json McpServer::toolGetLoRAAdapters(const json& args) {
    if (!lora_introspection_manager_) {
        return {{"status", "error"}, {"message", "LoRA introspection not available"}};
    }

    try {
        auto adapters = lora_introspection_manager_->getLoadedAdapters();
        json result = json::array();

        for (const auto& adapter : adapters) {
            result.push_back({
                {"lora_id", adapter.lora_id},
                {"base_model", adapter.base_model},
                {"rank", adapter.rank},
                {"gpu_id", adapter.gpu_id},
                {"is_active", adapter.is_active},
                {"inference_count", adapter.inference_count}
            });
        }

        return {
            {"status", "success"},
            {"adapters", result},
            {"total_count", adapters.size()}
        };
    } catch (const std::exception& e) {
        return {{"status", "error"}, {"message", e.what()}};
    }
}
```

---

## 🎯 Zusammenfassung & Nächste Schritte

### Implementierte Phasen (Stand: April 2026)

✅ **Phase 1: SchemaManager** — Vollständig (`include/metadata/schema_manager.h` v0.0.47, `src/metadata/schema_manager.cpp`)
✅ **Phase 2: REST API** — Vollständig (`include/server/schema_api_handler.h`, `src/server/schema_api_handler.cpp`)
✅ **Phase 3: MCP Integration** — Vollständig (`src/server/mcp_server.cpp` mit `toolGetSchema`, `toolGetStats`, `resourceSchema`)
✅ **Phase 4: Natural Language** — Vollständig (`toolIntrospectDatabase` mit `generateCapabilitiesAnswer`, `generateDataLocationAnswer`, `generateSchemaAnswer`)
✅ **Phase 5: Semantic Awareness** — Grundgerüst implementiert (`include/metadata/semantic_metadata_manager.h`, weitere Erweiterungen möglich)
✅ **Phase 6: LoRA-RAID Awareness** — Grundgerüst implementiert (`include/server/lora_api_handler.h`, `src/server/lora_api_handler.cpp`, weitere Erweiterungen möglich)

**Gesamt:** ~1900 LOC Kernimplementierung + ~1600 LOC Erweiterungen = **~3500 LOC**

### Build-Reihenfolge

1. Phase 1 komplett implementieren und testen
2. Phase 2 hinzufügen, HTTP-Server neu bauen
3. Phase 3 MCP Server aktualisieren
4. Phase 4 LLM-Integration erweitern
5. Phase 5 optional, wenn Semantic Metadata benötigt
6. Phase 6 optional, wenn LoRA-Introspection benötigt

### CMakeLists.txt Updates

```cmake
  # Add to source files
  set(THEMIS_SOURCES
      # ... existing sources ...
      src/metadata/schema_manager.cpp
      src/server/schema_api_handler.cpp
      src/metadata/semantic_metadata_manager.cpp  # Phase 5
      src/llm/lora_introspection_manager.cpp      # Phase 6
      src/server/lora_api_handler.cpp             # Phase 6
  )
```

### Testing

```bash
  # Unit tests
  ./build/test_schema_manager
  ./build/test_mcp_schema_integration

  # Integration test
  curl http://localhost:8080/api/v1/schema
  curl http://localhost:8080/api/v1/capabilities

  # MCP test (if MCP server running)
  echo '{"method":"tools/call","params":{"name":"get_schema"}}' | themis_mcp_client
```

---

## Evaluation / Implementierungsstatus

### Abgleich Leitfaden vs. Codestand (Mai 2026)

| Artefakt (Leitfaden) | Datei im Repo | Status |
|---|---|---|
| `include/metadata/schema_manager.h` | vorhanden | ✅ Produktiv (v0.0.47, 2026-04-15) |
| `src/metadata/schema_manager.cpp` | vorhanden | ✅ Produktiv |
| `include/server/schema_api_handler.h` | vorhanden | ✅ Produktiv |
| `src/server/schema_api_handler.cpp` | vorhanden | ✅ Produktiv |
| `tests/test_schema_manager.cpp` | vorhanden | ✅ Tests aktiv |
| MCP `toolGetSchema` / `toolGetStats` | `src/server/mcp_server.cpp` | ✅ Produktiv |
| `include/server/lora_api_handler.h` | vorhanden | ✅ Grundgerüst |
| `src/server/lora_api_handler.cpp` | vorhanden | ✅ Grundgerüst |
| `include/metadata/semantic_metadata_manager.h` | vorhanden | ✅ Grundgerüst |

### Beobachtungen

- Die in diesem Leitfaden spezifizierten Endpunkte (`/api/v1/schema`, `/api/v1/schema/tables`, `/api/v1/capabilities`) sind im HTTP-Server registriert. Hinweis: `/api/capabilities` (ohne `v1`) ist zusätzlich als Legacy-Pfad geroutet; beide Varianten sollten in der API-Referenz harmonisiert sein (vgl. `AGENTIC_AI_SELF_AWARENESS_RESEARCH.md`, Abschnitt E2).
- Die `SchemaManager`-Klasse verwendet `std::shared_mutex` für Read-heavy Caching (60 s Standard-TTL, konfigurierbar via `AdaptiveTTLConfig`); dies entspricht dem in Phase 1 definierten Entwurf.
- Ein reproduzierbarer End-to-End-Benchmark für Self-Awareness-Qualität (Korrektheit, Latenz, Protokollabdeckung) liegt noch nicht vor und ist als offenes Ziel markiert (siehe Limitations).

---

## Limitations / Known Issues

1. **Keine quantitativen Benchmarks:** Es existiert kein dedizierter, reproduzierbarer Benchmark, der die Self-Awareness-Antwortqualität (Korrektheit + Latenz + Abdeckung) über alle Protokolle (MCP, HTTP, GraphQL, PostgreSQL-Wire) quantitativ ausweist.
2. **Abhängigkeitsbedingte Pfade:** Mehrere Self-Awareness-Antworten greifen auf optionale Build-Flags zurück (`THEMIS_ENABLE_LLM`, `THEMIS_ENABLE_CUDA`, `THEMIS_ENABLE_MCP`). Ohne diese Flags liefern entsprechende Tools Minimal- oder Fehlerantworten.
3. **Endpunkt-Inkonsistenz:** Die dokumentierten Pfade (`/api/v1/capabilities`) und tatsächlich geroutete Pfade (`/api/capabilities`) sind nicht durchgängig harmonisiert.
4. **Phase 5 und 6 sind Grundgerüste:** `SemanticMetadataManager` und `LoRAIntrospectionManager` sind als Erweiterungspunkte angelegt; ein vollständiger produktiver Einsatz (inkl. LLM-gestützter Inhaltsanalyse und konsolidiertem externen LoRA-Q&A-Vertrag) ist noch ausstehend.
5. **Caching-Granularität:** Der `SchemaManager` verfügt über ein globales Cache-Invalidierungsmodell; feingranulare, tabellenbezogene Invalidierung ist möglich, aber noch nicht vollständig implementiert.
6. **Keine RBAC-Integration:** Die Self-Awareness-Endpunkte geben Schemainformationen ohne rollenbasierte Zugriffskontrolle zurück; für Produktionsumgebungen mit Mandantentrennung muss dies gesondert abgesichert werden.

---

## References / Quellen

### A) Externe Referenzen

1. Anthropic / Model Context Protocol (MCP) Specification: <https://modelcontextprotocol.io/>
2. GraphQL Specification (June 2018 Edition): <https://spec.graphql.org/>
3. PostgreSQL Information Schema: <https://www.postgresql.org/docs/current/information-schema.html>
4. PostgreSQL Frontend/Backend Protocol: <https://www.postgresql.org/docs/current/protocol.html>
5. llama.cpp — Local LLM Inference Engine: <https://github.com/ggml-org/llama.cpp>
6. RocksDB — Persistent Key-Value Store (Facebook/Meta): <https://rocksdb.org/>
7. nlohmann/json — JSON for Modern C++: <https://github.com/nlohmann/json>

### B) Interne Artefakte (Code/Test/Doku-Belege)

- Companion Research Review: `research/AGENTIC_AI_SELF_AWARENESS_RESEARCH.md`
- SchemaManager Header: `include/metadata/schema_manager.h` (v0.0.47)
- SchemaManager Implementation: `src/metadata/schema_manager.cpp`
- HTTP Schema API: `include/server/schema_api_handler.h`, `src/server/schema_api_handler.cpp`
- LoRA API: `include/server/lora_api_handler.h`, `src/server/lora_api_handler.cpp`
- MCP Server: `src/server/mcp_server.cpp`
- Tests: `tests/test_schema_manager.cpp`, `tests/test_schema_manager_fuzz.cpp`
- HTTP API Reference: `docs/de/apis/HTTP_API_REFERENCE.md`
- MCP Protocol Docs: `docs/de/apis/MCP_PROTOCOL_SUPPORT.md`

---

**Erstellt:** 11. Januar 2026
**Überarbeitet:** 14. Mai 2026
**Version:** 1.1 (Review-fähig)
**Status:** Alle 6 Phasen implementiert
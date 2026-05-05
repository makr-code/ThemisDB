# Agentic AI Self-Awareness: Beispiel-Implementierung

**Datum:** 11. Januar 2026  
**Version:** 1.0  
**Status:** Implementation Example  
**Kategorie:** Agentic AI, Code Example

---

## 📋 Überblick

Dieses Dokument zeigt konkrete Code-Beispiele, wie die "Self-Awareness"-Funktionalität in ThemisDB implementiert werden könnte, basierend auf den Ergebnissen des [Research Documents](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md).

---

## 🎯 Ziel-Interaktion

**User fragt:**
```
"Was kannst du?"
```

**ThemisDB (via LLM) antwortet:**
```
Ich bin ThemisDB v1.4.0-alpha, eine Multi-Model-Datenbank mit folgenden Fähigkeiten:

📊 Datenmodelle:
- Relational (SQL-ähnliche Queries)
- Graph (Cypher-Queries, BFS/Dijkstra/A* Traversals)
- Vector (HNSW/FAISS Similarity Search, GPU-accelerated)
- Document (JSON mit flexiblem Schema)
- Time-Series (Gorilla Compression, Continuous Aggregates)

🔒 Transaktionen:
- ACID-Garantien mit MVCC Snapshot Isolation
- Write-Write Conflict Detection
- Session-basierte und direkte API

🧠 KI-Integration:
- Optional: Eingebettete LLM-Engine (llama.cpp)
- Grammar-Constrained Generation (JSON/XML/CSV)
- Vision Support (LLaVA Multi-Modal)
- Hybrid Search (BM25 + Vector für RAG)

📡 Protokolle:
- HTTP/1.1, HTTP/2 (mit Server Push)
- gRPC, PostgreSQL Wire Protocol
- Model Context Protocol (MCP)
- WebSocket, SSE

🎯 Aktueller Datenbestand:
- 3 Tabellen: users (1.2M Einträge), products (450K), orders (2.8M)
- 12 aktive Indexes (7 Secondary, 3 Fulltext, 2 Geo)
- Speichergröße: 5.2 GB
```

---

## 🔧 Implementierungs-Komponenten

### 1. Schema Manager Klasse

**Datei:** `include/metadata/schema_manager.h`

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace metadata {

using json = nlohmann::json;

/**
 * @brief Manages database schema metadata and introspection
 * 
 * Provides centralized access to:
 * - Table/Node definitions
 * - Column/Property schemas
 * - Index metadata
 * - Statistics and cardinalities
 */
class SchemaManager {
public:
    struct PropertyInfo {
        std::string name;
        std::string type; // "string", "integer", "float", "boolean", "timestamp"
        bool nullable = true;
        bool indexed = false;
        std::string index_type; // "", "secondary", "fulltext", "geo", etc.
    };

    struct TableSchema {
        std::string name;
        std::string type; // "node", "edge", "document", "timeseries"
        std::vector<PropertyInfo> properties;
        size_t estimated_rows = 0;
        size_t storage_bytes = 0;
        std::string created_at;
        std::string last_modified;
    };

    struct RelationshipSchema {
        std::string type; // Edge type name, e.g., "FOLLOWS"
        std::string from_table;
        std::string to_table;
        std::vector<PropertyInfo> properties;
        size_t count = 0;
    };

    struct DatabaseMetadata {
        std::string name = "ThemisDB";
        std::string version;
        std::string edition; // "Community", "Enterprise"
        std::vector<std::string> enabled_features;
        size_t total_entities = 0;
        size_t total_indexes = 0;
        size_t storage_size_bytes = 0;
    };

    explicit SchemaManager(RocksDBWrapper& db);

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

    // JSON export for MCP/API
    json toJSON() const;
    json tableToJSON(const TableSchema& table) const;

private:
    RocksDBWrapper& db_;
    
    // Cached schema information (updated periodically)
    mutable std::unordered_map<std::string, TableSchema> table_cache_;
    mutable std::unordered_map<std::string, RelationshipSchema> relationship_cache_;
    mutable DatabaseMetadata metadata_cache_;
    mutable std::chrono::system_clock::time_point last_refresh_;
    
    void refreshCache() const;
    TableSchema discoverTableSchema(std::string_view table) const;
};

} // namespace metadata
} // namespace themis
```

---

### 2. REST API Endpoint

**Datei:** `src/server/schema_api_handler.cpp`

```cpp
#include "server/schema_api_handler.h"
#include "metadata/schema_manager.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace server {

void SchemaApiHandler::registerRoutes(HttpServer& server) {
    // GET /api/v1/schema - Full database schema
    server.registerRoute("GET", "/api/v1/schema", 
        [this](const Request& req, Response& res) {
            handleGetSchema(req, res);
        });

    // GET /api/v1/schema/tables - List all tables
    server.registerRoute("GET", "/api/v1/schema/tables",
        [this](const Request& req, Response& res) {
            handleGetTables(req, res);
        });

    // GET /api/v1/schema/tables/:name - Single table schema
    server.registerRoute("GET", "/api/v1/schema/tables/:name",
        [this](const Request& req, Response& res) {
            handleGetTable(req, res);
        });

    // GET /api/v1/capabilities - Database capabilities
    server.registerRoute("GET", "/api/v1/capabilities",
        [this](const Request& req, Response& res) {
            handleGetCapabilities(req, res);
        });
}

void SchemaApiHandler::handleGetSchema(const Request& req, Response& res) {
    try {
        auto schema = schema_manager_.toJSON();
        
        res.setStatus(200);
        res.setHeader("Content-Type", "application/json");
        res.setBody(schema.dump(2));
        
        spdlog::info("Schema request served successfully");
    } catch (const std::exception& e) {
        spdlog::error("Failed to get schema: {}", e.what());
        
        json error = {
            {"error", "internal_server_error"},
            {"message", e.what()}
        };
        
        res.setStatus(500);
        res.setHeader("Content-Type", "application/json");
        res.setBody(error.dump(2));
    }
}

void SchemaApiHandler::handleGetCapabilities(const Request& req, Response& res) {
    json capabilities = {
        {"database", {
            {"name", "ThemisDB"},
            {"version", THEMIS_VERSION},
            {"edition", 
                #ifdef THEMIS_ENTERPRISE
                "Enterprise"
                #else
                "Community"
                #endif
            }
        }},
        {"features", {
            {"multi_model", json::array({"relational", "graph", "vector", "document", "time-series"})},
            {"transactions", "ACID with MVCC snapshot isolation"},
            {"llm_integration", 
                #ifdef THEMIS_ENABLE_LLM
                true
                #else
                false
                #endif
            },
            {"gpu_acceleration", json::array({
                #ifdef THEMIS_ENABLE_CUDA
                "CUDA",
                #endif
                #ifdef THEMIS_ENABLE_VULKAN
                "Vulkan",
                #endif
                "CPU"
            })},
            {"protocols", json::array({
                "HTTP/1.1",
                #ifdef THEMIS_ENABLE_HTTP2
                "HTTP/2",
                #endif
                #ifdef THEMIS_ENABLE_GRPC
                "gRPC",
                #endif
                #ifdef THEMIS_ENABLE_POSTGRES_WIRE
                "PostgreSQL Wire",
                #endif
                #ifdef THEMIS_ENABLE_MCP
                "MCP",
                #endif
                "REST"
            })},
            {"indexes", json::array({
                "secondary", "range", "sparse", "geo", "ttl", "fulltext", "composite"
            })}
        }},
        {"limits", {
            {"max_entity_size_bytes", 100 * 1024 * 1024}, // 100 MB
            {"max_query_complexity", 1000},
            {"max_concurrent_connections", 10000}
        }},
        {"statistics", schema_manager_.getDatabaseMetadata()}
    };

    res.setStatus(200);
    res.setHeader("Content-Type", "application/json");
    res.setBody(capabilities.dump(2));
}

} // namespace server
} // namespace themis
```

---

### 3. Vollständige MCP Schema-Integration

**Datei:** `src/server/mcp_server.cpp` (Update)

```cpp
// Replace stub implementation with real schema discovery
json McpServer::toolGetSchema(const json& args) {
    spdlog::info("MCP Tool 'get_schema' called");
    
    if (!schema_manager_) {
        return {
            {"status", "error"},
            {"message", "Schema manager not initialized"}
        };
    }

    try {
        auto tables = schema_manager_->getAllTables();
        auto relationships = schema_manager_->getAllRelationships();
        
        json nodes = json::array();
        for (const auto& table : tables) {
            nodes.push_back(schema_manager_->tableToJSON(table));
        }
        
        json edges = json::array();
        for (const auto& rel : relationships) {
            edges.push_back({
                {"type", rel.type},
                {"from", rel.from_table},
                {"to", rel.to_table},
                {"properties", rel.properties},
                {"count", rel.count}
            });
        }
        
        return {
            {"status", "success"},
            {"integration_level", "full"},
            {"nodes", nodes},
            {"edges", edges},
            {"total_tables", tables.size()},
            {"total_relationships", relationships.size()}
        };
        
    } catch (const std::exception& e) {
        spdlog::error("Schema discovery failed: {}", e.what());
        return {
            {"status", "error"},
            {"message", std::string("Schema discovery failed: ") + e.what()}
        };
    }
}

json McpServer::resourceSchema(const std::string& uri) {
    if (!schema_manager_) {
        return {
            {"error", "Schema manager not initialized"}
        };
    }
    
    return schema_manager_->toJSON();
}

json McpServer::resourceStats(const std::string& uri) {
    if (!schema_manager_) {
        return {
            {"error", "Schema manager not initialized"}
        };
    }
    
    auto metadata = schema_manager_->getDatabaseMetadata();
    
    return {
        {"status", "connected"},
        {"database_open", db_ && db_->isOpen()},
        {"total_entities", metadata.total_entities},
        {"total_indexes", metadata.total_indexes},
        {"storage_size_bytes", metadata.storage_size_bytes},
        {"version", metadata.version},
        {"edition", metadata.edition}
    };
}
```

---

### 4. LLM Self-Awareness System Prompt

**Datei:** `config/llm_system_prompts.yaml`

```yaml
# System Prompt for Self-Awareness
self_awareness_prompt: |
  You are ThemisDB {version}, a {edition} edition multi-model database.
  
  Your Capabilities:
  - Multi-Model Storage: {data_models}
  - Transaction Support: {transaction_model}
  - Query Languages: {query_languages}
  - Protocols: {protocols}
  - Indexes: {index_types}
  
  Current Database State:
  - Tables: {table_count}
  - Total Entities: {entity_count}
  - Storage Size: {storage_size}
  - Active Indexes: {index_count}
  
  Available Tables:
  {table_schemas}
  
  When a user asks about your capabilities ("Was kannst du?"), describe your
  features in a clear, structured manner. When asked about data structure
  ("Wie sind die Daten aufgebaut?"), provide table schemas with properties
  and indexes. When asked about your purpose ("Was ist deine Aufgabe?"),
  explain your role as a multi-model database with ACID guarantees.
  
  Use the following tools to answer queries:
  - get_schema: Get detailed schema information
  - get_stats: Get database statistics
  - query: Execute database queries
  
  Always provide accurate, helpful information based on the current database state.

# Prompt for "Was kannst du?" question
what_can_you_do_prompt: |
  Ich bin ThemisDB {version}, eine {edition} Multi-Model-Datenbank mit:
  
  📊 **Datenmodelle:**
  {data_models_list}
  
  🔒 **Transaktionen:**
  - ACID-Garantien mit MVCC Snapshot Isolation
  - Write-Write Conflict Detection
  - Atomic Updates über alle Index-Typen
  
  🧠 **KI-Integration:**
  {llm_features_list}
  
  📡 **Protokolle:**
  {protocols_list}
  
  🎯 **Aktueller Datenbestand:**
  - {table_count} Tabellen mit {entity_count} Einträgen
  - {index_count} aktive Indexes
  - Speichergröße: {storage_size}
  
  Ich kann komplexe Queries über Graph, Vector und Document Models ausführen
  und biete vollständige ACID-Garantien für alle Operationen.

# Prompt for "Wie sind die Daten aufgebaut?" question
data_structure_prompt: |
  Die Datenbank enthält folgende Tabellen:
  
  {table_schemas_detailed}
  
  Jede Tabelle hat definierte Properties und kann mit verschiedenen
  Index-Typen optimiert werden (Secondary, Range, Fulltext, Geo, etc.).
  
  Relationships zwischen Tabellen:
  {relationship_schemas}
```

---

### 5. MCP Tool für Natural Language Introspection

**Datei:** `src/server/mcp_server.cpp` (Extension)

```cpp
void McpServer::registerDefaultTools() {
    // ... existing tools ...

    // NEW: Self-awareness introspection tool
    registerTool("introspect_database", 
        "Answer natural language questions about database structure, capabilities, and purpose",
        {
            {"type", "object"},
            {"properties", {
                {"question", {
                    {"type", "string"}, 
                    {"description", "Natural language question like 'Was kannst du?' or 'Wie sind die Daten aufgebaut?'"}
                }}
            }},
            {"required", {"question"}}
        },
        [this](const json& args) { return toolIntrospectDatabase(args); });
}

json McpServer::toolIntrospectDatabase(const json& args) {
    std::string question = args.at("question");
    spdlog::info("MCP Tool 'introspect_database' called with question: {}", question);
    
    // Detect question type
    std::string question_lower = toLower(question);
    
    json context;
    std::string prompt_template;
    
    if (contains(question_lower, "was kannst du") || 
        contains(question_lower, "what can you do") ||
        contains(question_lower, "capabilities")) {
        
        // Get capabilities
        context = getCapabilitiesContext();
        prompt_template = loadPromptTemplate("what_can_you_do_prompt");
        
    } else if (contains(question_lower, "daten aufgebaut") || 
               contains(question_lower, "data structure") ||
               contains(question_lower, "schema")) {
        
        // Get schema information
        context = toolGetSchema({});
        prompt_template = loadPromptTemplate("data_structure_prompt");
        
    } else if (contains(question_lower, "aufgabe") || 
               contains(question_lower, "purpose") ||
               contains(question_lower, "zweck")) {
        
        // Explain purpose
        prompt_template = R"(
            Ich bin ThemisDB, eine Multi-Model-Datenbank. Meine Hauptaufgabe ist:
            
            1. **Datenspeicherung:** Strukturierte und unstrukturierte Daten 
               sicher und effizient speichern
            
            2. **ACID-Garantien:** Vollständige Transaktions-Isolation mit 
               MVCC für konsistente Daten
            
            3. **Flexible Queries:** Unterstützung verschiedener Abfrage-Modelle
               (Graph, Vector, Document, Relational, Time-Series)
            
            4. **KI-Integration:** Optional eingebettete LLMs für intelligente
               Datenanalyse und RAG-Workflows
            
            Ich kombiniere die Vorteile verschiedener Datenbank-Typen in einem
            System mit einheitlicher Transaktions-Garantie.
        )";
        
    } else {
        // General question - use LLM to reason
        context = {
            {"schema", toolGetSchema({})},
            {"capabilities", getCapabilitiesContext()},
            {"stats", toolGetStats({})}
        };
        prompt_template = loadPromptTemplate("self_awareness_prompt");
    }
    
    // Inject context into prompt template
    std::string final_prompt = injectContext(prompt_template, context);
    
    #ifdef THEMIS_ENABLE_LLM
    // Use LLM to generate natural language response
    std::string response = THEMIS_LLM_GENERATE(final_prompt);
    
    return {
        {"status", "success"},
        {"question", question},
        {"answer", response},
        {"context_used", context}
    };
    #else
    // Fallback: Return structured data without LLM
    return {
        {"status", "success"},
        {"question", question},
        {"answer", final_prompt},
        {"note", "LLM not enabled - returning template response"},
        {"context_used", context}
    };
    #endif
}
```

---

### 6. Beispiel-Nutzung

#### a) Via REST API

```bash
# Get full schema
curl http://localhost:8765/api/v1/schema

# Get capabilities
curl http://localhost:8765/api/v1/capabilities

# Get specific table
curl http://localhost:8765/api/v1/schema/tables/users
```

**Response Example:**
```json
{
  "database": {
    "name": "ThemisDB",
    "version": "1.4.0-alpha",
    "edition": "Community"
  },
  "tables": [
    {
      "name": "users",
      "type": "node",
      "properties": [
        {
          "name": "id",
          "type": "string",
          "nullable": false,
          "indexed": true,
          "index_type": "primary"
        },
        {
          "name": "email",
          "type": "string",
          "nullable": false,
          "indexed": true,
          "index_type": "unique"
        },
        {
          "name": "age",
          "type": "integer",
          "nullable": true,
          "indexed": true,
          "index_type": "range"
        }
      ],
      "estimated_rows": 1234567,
      "storage_bytes": 157286400
    }
  ]
}
```

#### b) Via MCP (mit Claude Desktop)

```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "introspect_database",
    "arguments": {
      "question": "Was kannst du?"
    }
  }
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "content": [{
      "type": "text",
      "text": "Ich bin ThemisDB v1.4.0-alpha, eine Multi-Model-Datenbank mit:\n\n📊 Datenmodelle:\n- Relational (SQL-ähnliche Queries)\n- Graph (Cypher, BFS/Dijkstra)\n- Vector (HNSW/FAISS, GPU-accelerated)\n- Document (JSON, flexibles Schema)\n- Time-Series (Gorilla Compression)\n\n🔒 Transaktionen:\n- ACID mit MVCC Snapshot Isolation\n\n🧠 KI-Integration:\n- Optional: llama.cpp embedded\n- Grammar-Constrained Generation\n- Vision Support (LLaVA)\n\n📡 Protokolle:\n- HTTP/1.1, HTTP/2, gRPC\n- PostgreSQL Wire, MCP\n\n🎯 Aktuell:\n- 3 Tabellen (users, products, orders)\n- 1.2M + 450K + 2.8M = 4.45M Entities\n- 12 Indexes, 5.2 GB Storage"
    }]
  }
}
```

#### c) Via Python Client (mit LLM)

```python
from themis_client import ThemisClient

client = ThemisClient("http://localhost:8765")

# Natural language query
response = client.mcp.introspect("Was kannst du?")
print(response.answer)

# Get schema programmatically
schema = client.get_schema()
print(f"Tables: {[t['name'] for t in schema['tables']]}")

# Get capabilities
caps = client.get_capabilities()
print(f"Features: {caps['features']}")
```

---

## 📊 Vergleich: Vorher vs. Nachher

| Feature | Vorher (Stub) | Nachher (Vollständig) |
|---------|---------------|----------------------|
| **MCP get_schema** | ❌ Placeholder | ✅ Echte Schema-Daten |
| **REST /schema** | ❌ Nicht vorhanden | ✅ Vollständiger Endpoint |
| **Capabilities Endpoint** | ❌ Nicht vorhanden | ✅ `/api/v1/capabilities` |
| **Natural Language Q&A** | ❌ Nicht möglich | ✅ "Was kannst du?" funktioniert |
| **Table Introspection** | ⚠️ Nur in C++ Code | ✅ Über API verfügbar |
| **Index Metadata** | ❌ Nicht exposed | ✅ In Schema enthalten |
| **Statistics** | ❌ Hardcoded 0 | ✅ Echte RocksDB Stats |

---

## 🔍 Integration mit bestehendem Code

### Benötigte Änderungen

**Minimale Code-Änderungen:**
1. ✅ `SchemaManager` Klasse hinzufügen (~500 LOC)
2. ✅ `SchemaApiHandler` hinzufügen (~300 LOC)
3. ✅ MCP Server um `schema_manager_` erweitern (~200 LOC Updates)
4. ✅ System-Prompts in `config/` (~100 LOC YAML)

**Gesamt:** ~1100 LOC neue Code + ~200 LOC Updates

### Kompatibilität

- ✅ **Backwards Compatible:** REST API ist neu, ändert nichts bestehendes
- ✅ **Optional Features:** LLM-Integration bleibt optional (`-DTHEMIS_ENABLE_LLM=ON`)
- ✅ **Performance:** Schema-Cache verhindert übermäßige RocksDB-Scans
- ✅ **Thread-Safe:** SchemaManager nutzt `mutable` Cache mit Refresh-Logic

---

## 🎯 Nächste Schritte für Implementierung

1. **Phase 1: Core Schema Manager**
   - [ ] `SchemaManager` Klasse implementieren
   - [ ] RocksDB Key-Scanning für Table-Discovery
   - [ ] Property-Type Detection aus `BaseEntity`

2. **Phase 2: REST API**
   - [ ] `SchemaApiHandler` implementieren
   - [ ] `/api/v1/schema` Endpoints hinzufügen
   - [ ] `/api/v1/capabilities` implementieren

3. **Phase 3: MCP Integration**
   - [ ] `toolGetSchema` mit echten Daten füllen
   - [ ] `resourceSchema` auf `SchemaManager` umstellen
   - [ ] `toolIntrospectDatabase` implementieren

4. **Phase 4: LLM Self-Awareness**
   - [ ] System-Prompts erstellen
   - [ ] Context-Injection implementieren
   - [ ] Natural Language Tests

5. **Phase 5: Testing & Documentation**
   - [ ] Unit Tests für `SchemaManager`
   - [ ] Integration Tests für REST API
   - [ ] MCP Client Tests (Claude Desktop)
   - [ ] User Documentation aktualisieren

---

## 📚 Referenzen

- [Research Document](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)
- [MCP Protocol Specification](https://modelcontextprotocol.io/)
- [ThemisDB MCP Support](../apis/MCP_PROTOCOL_SUPPORT.md)
- [LLM Integration Guide](../llm/README.md)

---

**Erstellt:** 11. Januar 2026  
**Autor:** Implementation Example  
**Version:** 1.0  
**Status:** Proof-of-Concept Design

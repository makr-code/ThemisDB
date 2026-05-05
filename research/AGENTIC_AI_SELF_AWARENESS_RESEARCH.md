# Research: Agentic AI Self-Awareness in ThemisDB

**Datum:** 11. Januar 2026  
**Version:** 1.0  
**Status:** Research Document  
**Kategorie:** Agentic AI, Database Introspection, Self-Awareness

---

## 📋 Zusammenfassung (Executive Summary)

Diese Recherche analysiert die vorhandenen "Self-Awareness"-Fähigkeiten von ThemisDB in Kombination mit llama.cpp für Agentic AI-Anwendungen. Das Ziel ist es zu bestimmen, welche Mechanismen bereits existieren, die es einem LLM ermöglichen, die Datenbank über sich selbst, ihre Daten und deren Struktur zu befragen.

**Kernfrage:** Kann ein Nutzer die Datenbank fragen:
- "Was kannst du?"
- "Wo sind die Daten?"
- "Wie sind die Daten aufgebaut?"
- "Was ist deine Aufgabe?"
- **"Welche Behördendaten speicherst du?"** (Domain-spezifisch)
- **"Welche LoRA-Adapter sind geladen und was können sie?"** (LoRA-RAID-Verbund)

**Update (11.01.2026):** Erweitert um:
1. **Domain-Specific Semantic Awareness** - die Fähigkeit, nicht nur technische Metadaten (Tabellen, Spalten) zu liefern, sondern auch **Geschäftsdomäne und Zweck** zu verstehen und zu kommunizieren.
2. **LoRA-RAID Verbund Awareness** - Transparenz über geladene LoRA-Adapter, deren Herkunft, Fähigkeiten und GPU/RAID-Verteilung.

---

## 🎯 Fragestellung

> Gibt es eine Form der self-awareness der Themis (+llama.cpp) die kommunizieren kann welche Daten in der DB gespeichert sind und wie die benutzt werden können (Agentic AI). Also ich frage die DB: Was kannst du? Wo sind die Daten? Wie sind die Daten aufgebaut? Was ist deine Aufgabe? usw.

---

## ✅ Vorhandene Funktionen (Existing Capabilities)

### 1. **Model Context Protocol (MCP) Server** ✅ **VOLLSTÄNDIG IMPLEMENTIERT**

**Status:** Seit v1.3.0 implementiert  
**Dokumentation:** `docs/de/apis/MCP_PROTOCOL_SUPPORT.md`  
**Source Code:** `src/server/mcp_server.cpp`, `include/server/mcp_server.h`

#### Architektur

```
┌─────────────────┐
│   LLM Client    │  (Claude, GPT, etc.)
│   (User Query)  │
└────────┬────────┘
         │ Natural Language Query
         │ "Was kannst du?"
         ▼
┌─────────────────┐
│  MCP Server     │  ← Anthropic Model Context Protocol
│  (ThemisDB)     │
├─────────────────┤
│ • Tools         │ ← Database operations as callable tools
│ • Resources     │ ← Schema, Stats, Metadata (read-only)
│ • Prompts       │ ← Pre-defined query templates
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   ThemisDB      │
│   Core Engine   │
└─────────────────┘
```

#### MCP Capabilities

**a) Tools** - Datenbank-Operationen als LLM-Tools:

| Tool | Beschreibung | Self-Awareness Level |
|------|--------------|----------------------|
| `get_schema` | Retrieve database schema | ✅ **KERN-FEATURE** |
| `get_stats` | Get database statistics | ✅ **KERN-FEATURE** |
| `query` | Execute Cypher/SQL queries | ✅ Operational |
| `put_entity` | Create/update entities | ⚠️ Modifying |
| `get_entity` | Retrieve entity by key | ✅ Read-only |
| `delete_entity` | Delete entity | ⚠️ Modifying |
| `create_index` | Create database index | ⚠️ Schema-modifying |

**b) Resources** - Read-only Kontext:

| Resource URI | Beschreibung | Verfügbarkeit |
|--------------|--------------|---------------|
| `schema://database` | Database schema information | ✅ Implementiert |
| `stats://database` | Database statistics | ✅ Implementiert |
| `metadata://database` | Database metadata | ✅ Implementiert |
| `examples://queries` | Example query patterns | ✅ Implementiert |

**c) Prompts** - Query Templates:

| Prompt | Beschreibung | Use Case |
|--------|--------------|----------|
| `simple_query` | Generate simple Cypher query | Beginner queries |
| `complex_query` | Generate complex queries | Advanced operations |
| `entity_operation` | Entity CRUD operations | Data management |

#### MCP Transports

| Transport | Status | Use Case |
|-----------|--------|----------|
| **stdio** | ✅ Vollständig | Claude Desktop, CLI tools |
| **SSE** (Server-Sent Events) | ✅ Vollständig | Web browsers, real-time updates |
| **WebSocket** | ✅ Vollständig | Bidirectional communication |

#### Aktuelle Limitierungen

**WICHTIG:** Die MCP-Implementierung ist aktuell auf **Minimal Integration** begrenzt:

```cpp
json McpServer::toolGetSchema(const json& args) {
    return {
        {"status", "success"},
        {"message", "Schema discovery requires full query engine integration"},
        {"integration_level", "minimal"},
        {"nodes", json::array()},
        {"edges", json::array()},
        {"properties", json::object()},
        {"note", "Full schema discovery available in production integration"}
    };
}
```

**Was fehlt:**
- ❌ Vollständige Schema-Discovery (erfordert Query Engine Integration)
- ❌ Echte Statistiken über Knoten/Kanten-Anzahl
- ❌ Property-Graph Schema-Informationen
- ❌ Index-Metadaten-Abfragen

---

### 2. **LLM-Integration mit llama.cpp** ✅ **OPTIONAL VERFÜGBAR**

**Status:** Seit v1.3.0, erweitert in v1.4.0-alpha  
**Build-Flag:** `-DTHEMIS_ENABLE_LLM=ON`  
**Dokumentation:** `docs/de/llm/README.md`

#### Implementierte Features

| Feature | Version | Beschreibung |
|---------|---------|--------------|
| **Embedded LLM Engine** | v1.3.0 | Native llama.cpp integration |
| **Grammar-Constrained Generation** | v1.4.0-alpha | Garantiert valide JSON/XML/CSV outputs |
| **RoPE Scaling** | v1.4.0-alpha | 4K → 32K context window extension |
| **Vision Support** | v1.4.0-alpha | Multi-modal LLMs (LLaVA) |
| **Flash Attention** | v1.4.0-alpha | 15-25% speedup |
| **Speculative Decoding** | v1.4.0-alpha | 2-3x faster inference |
| **Continuous Batching** | v1.4.0-alpha | 2x+ throughput |

#### LLM MCP Tools (wenn aktiviert)

```cpp
#ifdef THEMIS_ENABLE_LLM
// Zusätzliche MCP-Tools bei aktiviertem LLM
- llm_complete: Text generation
- llm_embed: Text embeddings
- llm_chat: Chat completion
- database_query_with_llm: Query + LLM analysis
#endif
```

#### ReAct Agent Grammar

**Datei:** `src/llm/grammars/react_agent.gbnf`

```gbnf
# ReAct Agent Grammar - Structured format for thought/action/observation cycles
root ::= step+
step ::= thought "\n" action "\n" observation "\n"?

thought ::= "Thought:" ws text
action ::= "Action:" ws action-name ws json-args
action-name ::= "search" | "calculate" | "lookup" | "finish"

observation ::= "Observation:" ws text
```

**Bedeutung für Self-Awareness:**
- ✅ Strukturierte Reasoning-Loops
- ✅ Tool-Calling mit JSON-Arguments
- ✅ Explizite Thought-Action-Observation-Zyklen
- ⚠️ **Noch nicht für Schema-Introspection genutzt**

---

### 3. **Secondary Index Manager** ✅ **VOLLSTÄNDIG IMPLEMENTIERT**

**Source:** `include/index/secondary_index.h`, `src/index/secondary_index.cpp`

#### Index-Typen mit Metadaten

| Index Type | Self-Awareness Capability |
|------------|---------------------------|
| **Regular** | Basic equality lookups |
| **Range** | Lexicographic range scans |
| **Sparse** | NULL-aware indexes |
| **Geo** | Geospatial queries (lat/lon) |
| **TTL** | Time-to-live automatic cleanup |
| **Fulltext** | Inverted index with BM25 scoring |

#### Introspection-Methods

```cpp
// Index existence checks
bool hasIndex(std::string_view table, std::string_view column) const;
bool hasCompositeIndex(std::string_view table, const std::vector<std::string>& columns) const;
bool hasRangeIndex(std::string_view table, std::string_view column) const;
bool hasSparseIndex(std::string_view table, std::string_view column) const;
bool hasGeoIndex(std::string_view table, std::string_view column) const;
bool hasTTLIndex(std::string_view table, std::string_view column) const;
bool hasFulltextIndex(std::string_view table, std::string_view column) const;

// Configuration retrieval
std::optional<FulltextConfig> getFulltextConfig(std::string_view table, std::string_view column) const;
```

**Problem:**
- ✅ Introspection-Methods existieren im Code
- ❌ **NICHT über MCP/HTTP-API exposed**
- ❌ Keine zentrale "list all indexes" Funktion

---

### 4. **HTTP API** ✅ **TEILWEISE VERFÜGBAR**

**Dokumentation:** `docs/de/apis/HTTP_API_REFERENCE.md`

#### Relevante Endpoints

| Endpoint | Method | Self-Awareness Feature |
|----------|--------|------------------------|
| `/health` | GET | Service status |
| `/metrics` | GET | Prometheus metrics |
| `/entities/:key` | GET | Entity retrieval |
| `/query` | POST | AQL/Cypher query execution |
| `/index/create` | POST | Index creation |

**Fehlende Endpoints:**
- ❌ `/schema` - Get all tables/node types
- ❌ `/indexes` - List all indexes
- ❌ `/stats/summary` - Aggregated statistics
- ❌ `/metadata` - Database metadata

---

### 5. **GraphQL API** ⚠️ **BEGRENZT VERFÜGBAR**

**Source:** `include/api/graphql.h`, `src/api/graphql.cpp`

**Status:** Teilweise implementiert, aber keine Introspection-Features dokumentiert.

**GraphQL Introspection-Standard:**
- ✅ GraphQL hat eingebaute `__schema` und `__type` queries
- ❌ **Nicht klar ob in ThemisDB aktiviert**

---

### 6. **PostgreSQL Wire Protocol** ✅ **IMPLEMENTIERT**

**Build-Flag:** `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON`  
**Dokumentation:** `docs/POSTGRESQL_WIRE_PROTOCOL.md`

**PostgreSQL Introspection-Features:**
```sql
-- Standard PostgreSQL Introspection
SELECT * FROM information_schema.tables;
SELECT * FROM information_schema.columns;
\d tablename -- psql describe table
```

**Status:** ⚠️ **UNKLAR** ob `information_schema` in ThemisDB implementiert ist.

---

## ❌ Fehlende Funktionen (Missing Capabilities)

### 1. **Zentrale Schema-Discovery API** ❌ **NICHT IMPLEMENTIERT**

**Was fehlt:**
- Keine `/schema` REST endpoint
- Keine `SHOW TABLES` / `DESCRIBE TABLE` AQL-Befehle
- Keine zentrale Metadaten-Registry

**Was benötigt wird:**

```
GET /api/v1/schema
{
  "tables": [
    {
      "name": "users",
      "type": "node",
      "properties": ["name", "age", "email"],
      "indexes": [
        {"column": "email", "type": "unique"},
        {"column": "age", "type": "range"}
      ]
    }
  ],
  "relationships": [
    {
      "type": "FOLLOWS",
      "from": "users",
      "to": "users",
      "properties": ["since"]
    }
  ]
}
```

---

### 2. **Vollständige MCP Schema-Integration** ❌ **STUB IMPLEMENTATION**

**Aktueller Status:** Minimal Integration (Stubs)

```cpp
// Aktuell nur Placeholder
json McpServer::resourceSchema(const std::string& uri) {
    return {
        {"nodes", json::array()},
        {"edges", json::array()},
        {"message", "Schema discovery available in full integration"},
        {"note", "Minimal integration supports key-value operations only"}
    };
}
```

**Was fehlt:**
- ❌ Echte Schema-Discovery aus RocksDB
- ❌ Property-Graph Schema (Nodes/Edges)
- ❌ Index-Metadaten
- ❌ Constraint-Informationen

---

### 3. **Self-Describing Capabilities Endpoint** ❌ **NICHT IMPLEMENTIERT**

**Idee:** Endpoint der die Fähigkeiten der Datenbank beschreibt

```
GET /api/v1/capabilities
{
  "database": {
    "name": "ThemisDB",
    "version": "1.4.0-alpha",
    "edition": "Community"
  },
  "features": {
    "multi_model": ["relational", "graph", "vector", "document", "time-series"],
    "llm_integration": true,
    "gpu_acceleration": ["CUDA", "Vulkan"],
    "protocols": ["HTTP/1.1", "HTTP/2", "gRPC", "PostgreSQL Wire", "MCP"],
    "indexes": ["secondary", "range", "sparse", "geo", "ttl", "fulltext"],
    "transactions": "ACID with MVCC snapshot isolation"
  },
  "limits": {
    "max_entity_size": "100MB",
    "max_query_complexity": 1000,
    "max_concurrent_connections": 10000
  },
  "statistics": {
    "total_entities": 1234567,
    "total_indexes": 42,
    "storage_size_bytes": 52428800
  }
}
```

---

### 4. **Natural Language Schema Query via LLM** ❌ **NICHT IMPLEMENTIERT**

**Idee:** LLM kann natürliche Sprachfragen beantworten

**Beispiele:**

| Frage | Erwartete Antwort |
|-------|-------------------|
| "Was kannst du?" | "Ich bin ThemisDB v1.4.0-alpha, eine Multi-Model-Datenbank mit ACID-Transaktionen, Graph/Vector/Document-Support, und optionaler LLM-Integration." |
| "Welche Tabellen gibt es?" | "Aktuell existieren folgende Tabellen: users, products, orders. Die users-Tabelle hat 1.2M Einträge." |
| "Wie ist die users-Tabelle aufgebaut?" | "Die users-Tabelle enthält folgende Felder: id (Primary Key), name (String), email (String, indexed), age (Integer, range-indexed), created_at (Timestamp)." |
| "Was ist deine Aufgabe?" | "Ich speichere und verwalte strukturierte und unstrukturierte Daten mit vollständiger ACID-Garantie und ermögliche komplexe Queries über Graph, Vector und Document Models." |

**Was benötigt wird:**

1. **Schema-Kontext für LLM:**
   - Vollständiges Schema als MCP Resource
   - Statistiken über Datenmenge
   - Index-Informationen
   - Constraint-Informationen

2. **LLM Prompt Engineering:**
   - System-Prompt mit DB-Capabilities
   - Schema-Kontext in jedem Request
   - Tool-Calling für dynamische Schema-Queries

3. **ReAct Agent Loop:**
   ```
   User: "Welche Tabellen gibt es?"
   
   Thought: Ich muss die Schema-Informationen abfragen
   Action: get_schema {}
   Observation: {"tables": ["users", "products", "orders"]}
   
   Thought: Ich habe die Tabellen erhalten
   Action: finish {"answer": "Es gibt 3 Tabellen: users, products, orders"}
   ```

---

### 5. **Domain-Specific Semantic Awareness** ❌ **NICHT IMPLEMENTIERT**

**Idee:** LLM versteht nicht nur technisches Schema, sondern auch **Geschäftsdomäne und Zweck** der Daten

**Problem:** Aktuell kann ThemisDB nur technische Metadaten liefern (Tabellennamen, Spalten, Typen). Es fehlt **semantisches Verständnis** über den Inhalt und Zweck.

**Beispiel - Behördendaten:**

| Frage | Technische Antwort (aktuell möglich) | Semantische Antwort (benötigt) |
|-------|--------------------------------------|--------------------------------|
| "Welche Behördendaten speicherst du?" | "Ich speichere Tabellen: documents, agencies, permits, inspections" | "Ich speichere die Dokumente der **Behörde für Umweltschutz Hamburg** im **Akten-Layout nach VwVfG** und fokussiere mich auf **immissionsschutzrechtliche** Verfahren **nach BImSchG** für **genehmigungsbedürftige Anlagen**." |
| "Was ist der Zweck dieser Daten?" | "Die documents-Tabelle hat 1.2M Einträge mit Properties: id, title, content, created_at" | "Die Daten dienen der **Genehmigungsverfahren für Industrieanlagen** gemäß **Bundesimmissionsschutzgesetz (BImSchG)**. Ich speichere Antragsunterlagen, Gutachten, Genehmigungsbescheide und Überwachungsprotokolle." |

**Was benötigt wird:**

1. **Metadaten-Layer für Business Context:**
   ```json
   {
     "database_purpose": {
       "domain": "Umweltschutz / Immissionsschutz",
       "legal_framework": ["BImSchG", "VwVfG", "9. BImSchV"],
       "organization": "Behörde für Umweltschutz Hamburg",
       "data_classification": "Behördliche Verwaltungsakten"
     },
     "tables": [
       {
         "name": "documents",
         "business_purpose": "Genehmigungsrelevante Dokumente nach BImSchG",
         "document_types": [
           "Antragsunterlagen",
           "Gutachten (Emissionsschutz, Lärmschutz)",
           "Genehmigungsbescheide",
           "Überwachungsprotokolle"
         ],
         "legal_retention": "30 Jahre nach VwVfG",
         "sensitivity": "VS-NfD (Nur für den Dienstgebrauch)"
       }
     ]
   }
   ```

2. **LLM-gestützte Querschnitts-Metadaten-Auswertung:**
   - **Content Analysis:** LLM analysiert gespeicherte Dokumente und extrahiert Domänen-Kontext
   - **Pattern Recognition:** Erkennung von Fachbegriffen (BImSchG, Genehmigungsverfahren, etc.)
   - **Entity Extraction:** Behörden, Rechtsnormen, Anlagentypen
   - **Cross-Table Semantics:** Verknüpfung zwischen documents ↔ agencies ↔ permits

3. **Semantic Metadata Store:**
   ```cpp
   class SemanticMetadataManager {
   public:
       struct DomainContext {
           std::string domain;              // "Umweltschutz"
           std::vector<std::string> legal_framework; // ["BImSchG", "VwVfG"]
           std::string organization;        // "Behörde XY"
           std::string purpose;             // Business purpose
       };
       
       struct TableSemantics {
           std::string business_purpose;
           std::vector<std::string> content_types;
           std::string legal_context;
           int retention_years;
       };
       
       DomainContext getDomainContext() const;
       TableSemantics getTableSemantics(std::string_view table) const;
       
       // LLM-assisted analysis
       void analyzeDataContent(const std::string& sample_data);
       void extractEntities(const std::string& content);
   };
   ```

4. **Integration in MCP Self-Awareness:**
   ```cpp
   json McpServer::toolIntrospectDatabase(const json& args) {
       std::string question = args.at("question");
       
       if (contains(question, "behördendaten") || 
           contains(question, "authority data") ||
           contains(question, "welche daten")) {
           
           // Get semantic context
           auto domain = semantic_metadata_->getDomainContext();
           auto table_semantics = semantic_metadata_->getTableSemantics("documents");
           
           // Build context-aware response
           std::string prompt = fmt::format(R"(
               Ich speichere die Dokumente der **{}** im **Akten-Layout nach {}** 
               und fokussiere mich auf {} Verfahren nach {} für {}.
               
               Konkret speichere ich folgende Dokumenttypen:
               {}
               
               Rechtlicher Rahmen: {}
               Aufbewahrungsfrist: {} Jahre
           )",
               domain.organization,
               "VwVfG", // Verwaltungsverfahrensgesetz
               domain.domain,
               domain.legal_framework[0], // BImSchG
               "genehmigungsbedürftige Anlagen",
               join(table_semantics.content_types, ", "),
               join(domain.legal_framework, ", "),
               table_semantics.retention_years
           );
           
           return {
               {"status", "success"},
               {"answer", prompt}
           };
       }
   }
   ```

5. **Automatische Metadaten-Extraktion:**
   - **Beim Einfügen neuer Daten:** LLM analysiert Content und extrahiert Metadaten
   - **Periodisches Scanning:** Regelmäßige Analyse bestehender Daten
   - **Manual Curation:** Admin kann Domänen-Kontext manuell pflegen

**Implementation Path:**

| Phase | Beschreibung | Aufwand |
|-------|--------------|---------|
| **5a** | Semantic Metadata Store (Datenstruktur) | ~200 LOC |
| **5b** | LLM Content Analysis (Sample-based) | ~300 LOC |
| **5c** | Entity Extraction (Fachbegriffe, Rechtsnormen) | ~200 LOC |
| **5d** | Integration in MCP introspect_database | ~100 LOC |
| **TOTAL** | | **~800 LOC** |

**Use Cases:**

- **Behörden:** "Welche Genehmigungsverfahren verwaltest du?"
- **Gesundheitswesen:** "Welche Patientendaten speicherst du und warum?"
- **Logistik:** "Welche Sendungsdaten verfolgst du?"
- **Finance:** "Welche Transaktionsdaten archivierst du für Compliance?"

**Herausforderungen:**

1. **Privacy/Security:** Sensitive Daten nicht an externes LLM senden
   - Lösung: Lokales LLM (llama.cpp) für Content-Analyse
   
2. **Accuracy:** LLM muss Domänen-Kontext korrekt erkennen
   - Lösung: Fine-tuned Models für spezifische Domains
   
3. **Performance:** Content-Analyse kann teuer sein
   - Lösung: Sampling-based Analysis + Caching

**Priorität:** MITTEL-HOCH (abhängig von Use Case)

---

### 6. **Query Explanation & Analysis** ❌ **NICHT IMPLEMENTIERT**

**PostgreSQL hat EXPLAIN:**
```sql
EXPLAIN ANALYZE SELECT * FROM users WHERE age > 25;
```

**ThemisDB fehlt:**
- ❌ `EXPLAIN` Befehl für AQL/Cypher
- ❌ Query Plan Visualization
- ❌ Performance Cost Estimation
- ❌ Index Usage Analysis

---

### 7. **Audit & Provenance Tracking** ⚠️ **TEILWEISE IMPLEMENTIERT**

**Was existiert:**
- ✅ Audit Logging (Enterprise)
- ✅ Version Manager für Content
- ✅ MVCC Transaction History

**Was fehlt für Self-Awareness:**
- ❌ "Wer hat diese Daten erstellt?"
- ❌ "Wann wurde diese Tabelle zuletzt geändert?"
- ❌ "Welche Queries wurden am häufigsten ausgeführt?"

---

## 🔧 Implementierungs-Empfehlungen (Implementation Recommendations)

### Phase 1: Schema-Discovery API (Priorität: HOCH)

**Ziel:** Vollständige Schema-Introspection

**Tasks:**
1. **Schema Manager Klasse erstellen**
   ```cpp
   class SchemaManager {
   public:
       struct TableSchema {
           std::string name;
           std::string type; // "node", "edge", "document"
           std::vector<std::string> properties;
           std::vector<IndexInfo> indexes;
       };
       
       std::vector<TableSchema> getAllTables() const;
       TableSchema getTable(std::string_view name) const;
       std::vector<std::string> getTableProperties(std::string_view table) const;
   };
   ```

2. **REST API Endpoints**
   - `GET /api/v1/schema` - Vollständiges Schema
   - `GET /api/v1/schema/tables` - Liste aller Tabellen
   - `GET /api/v1/schema/tables/:name` - Einzelne Tabelle
   - `GET /api/v1/schema/indexes` - Alle Indexes

3. **MCP Integration**
   ```cpp
   json McpServer::resourceSchema(const std::string& uri) {
       // Echte Implementation statt Stub
       auto schema_mgr = getSchemaManager();
       auto tables = schema_mgr->getAllTables();
       
       json nodes = json::array();
       for (const auto& table : tables) {
           nodes.push_back({
               {"name", table.name},
               {"type", table.type},
               {"properties", table.properties},
               {"indexes", table.indexes}
           });
       }
       
       return {
           {"status", "success"},
           {"nodes", nodes},
           {"edges", getEdgeSchema()},
           {"integration_level", "full"}
       };
   }
   ```

---

### Phase 2: Self-Describing Capabilities (Priorität: MITTEL)

**Ziel:** Datenbank kann ihre Fähigkeiten beschreiben

**Tasks:**
1. **Capabilities Endpoint**
   - `GET /api/v1/capabilities`
   - Listet Features, Limits, Statistiken

2. **MCP Resource: `capabilities://database`**
   ```json
   {
     "features": ["multi_model", "transactions", "llm"],
     "protocols": ["HTTP/1.1", "gRPC", "MCP"],
     "statistics": {...}
   }
   ```

3. **Build-time Feature Detection**
   ```cpp
   json getCapabilities() {
       json capabilities = {
           {"database", {
               {"name", "ThemisDB"},
               {"version", THEMIS_VERSION}
           }},
           {"features", {
               {"llm_integration", 
                   #ifdef THEMIS_ENABLE_LLM
                   true
                   #else
                   false
                   #endif
               }
           }}
       };
       return capabilities;
   }
   ```

---

### Phase 3: Natural Language Self-Awareness (Priorität: MITTEL-NIEDRIG)

**Ziel:** LLM kann Fragen über die DB beantworten

**Tasks:**
1. **System Prompt für DB-Awareness**
   ```
   You are ThemisDB v1.4.0-alpha, a multi-model database with:
   - ACID transactions with MVCC
   - Support for relational, graph, vector, document, and time-series data
   - Optional LLM integration with llama.cpp
   - Multiple protocols: HTTP, gRPC, PostgreSQL Wire, MCP
   
   Current schema:
   {schema_context}
   
   Current statistics:
   {stats_context}
   
   You can answer questions about:
   - Database capabilities ("Was kannst du?")
   - Schema structure ("Wie sind die Daten aufgebaut?")
   - Data location ("Wo sind die Daten?")
   - Your purpose ("Was ist deine Aufgabe?")
   ```

2. **Context Injection in MCP**
   ```cpp
   json McpServer::handleRequest(const json& request) {
       // Inject schema context into LLM prompts
       if (request["method"] == "prompts/get") {
           json schema = resourceSchema("schema://database");
           json stats = resourceStats("stats://database");
           
           // Add to prompt context
           addContext("schema", schema);
           addContext("stats", stats);
       }
   }
   ```

3. **ReAct Agent Tools**
   ```cpp
   registerTool("introspect_database", "Answer questions about database structure and capabilities",
       {
           {"type", "object"},
           {"properties", {
               {"question", {{"type", "string"}, {"description", "Natural language question"}}}
           }}
       },
       [this](const json& args) { 
           return toolIntrospectDatabase(args); 
       });
   ```

---

### Phase 4: Query Explanation (Priorität: NIEDRIG)

**Ziel:** EXPLAIN-ähnliche Funktionalität

**Tasks:**
1. **AQL EXPLAIN Befehl**
   ```
   EXPLAIN MATCH (u:User)-[:FOLLOWS]->(f:User) WHERE u.age > 25 RETURN f
   ```

2. **Query Plan Visualization**
   - Execution steps
   - Index usage
   - Estimated costs

---

### 8. **LoRA-RAID Verbund Awareness** ❌ **NICHT IMPLEMENTIERT**

**Idee:** Self-Awareness über **LoRA-Adapter im RAID-Verbund** - welcher LoRA-Adapter woher kommt und was er kann

**Problem:** ThemisDB hat ein umfangreiches **Multi-LoRA-Management-System** mit RAID-ähnlicher Verteilung über GPUs, aber keine Introspection-API dafür.

**Existierende Infrastruktur:**

ThemisDB implementiert (seit v1.3.3+) ein vLLM-inspiriertes Multi-LoRA-System:

| Komponente | Status | Beschreibung |
|------------|--------|--------------|
| **MultiLoRAManager** | ✅ Implementiert | Multi-LoRA Inference mit Adapter-Switching |
| **LoRAMetadataCache** | ✅ Implementiert | Lock-free Metadata Storage |
| **Multi-GPU Support** | ✅ Implementiert | LoRA-Verteilung über GPUs (ROUND_ROBIN, DATA_PARALLEL, MODEL_PARALLEL) |
| **RAID-Style Orchestration** | ✅ Implementiert | RAIDLoRAPipelineOrchestrator |
| **LoRA Quantization** | ✅ Implementiert | INT8/INT4 Compression |
| **Introspection API** | ❌ Fehlt | Keine Self-Awareness über LoRA-Adapter |

**Beispiel-Fragen die beantwortet werden sollen:**

| Frage | Erwartete Antwort |
|-------|-------------------|
| "Welche LoRA-Adapter sind geladen?" | "Aktuell sind 3 LoRA-Adapter geladen: **medical-qa** (GPU 0, Rank 16), **legal-summarization** (GPU 1, Rank 32), **code-generation** (GPU 2, Rank 8)" |
| "Woher kommt der medical-qa Adapter?" | "Der **medical-qa** LoRA-Adapter stammt von `models/lora/medical-qa.safetensors`, basiert auf **Llama-3-8B**, hat Rank 16 und ist auf **target_modules: [q_proj, v_proj, k_proj]** trainiert." |
| "Was kann der legal-summarization Adapter?" | "Der **legal-summarization** Adapter ist spezialisiert auf **Zusammenfassung juristischer Dokumente** (§§, Urteile, Verträge). Er wurde trainiert auf **deutsche Rechtsprechung** und kann Dokumente nach BGB, StGB, VwVfG verarbeiten." |
| "Wie ist die GPU-Verteilung?" | "LoRA-Adapter sind auf **4 GPUs** verteilt (NVIDIA A100 40GB). Strategie: **ROUND_ROBIN**. GPU 0: medical-qa (2.1 GB), GPU 1: legal-summarization (4.3 GB), GPU 2: code-generation (1.2 GB), GPU 3: leer. VRAM-Nutzung: 32% Durchschnitt." |
| "Welche LoRA-Adapter sind im RAID-Verbund?" | "RAID-Konfiguration: **STRIPE_MIRROR** (RAID 10). LoRA-Adapter werden über 3 Shards verteilt und gespiegelt. Primary Shard für medical-qa: Shard 0, Replicas: Shard 1, Shard 2. Latency: 12ms, Throughput: 450 tokens/s." |

**Was benötigt wird:**

1. **LoRA Introspection API** (~400 LOC)
2. **REST API Endpoints** (~300 LOC): `/api/v1/lora/adapters`, `/api/v1/lora/gpus`, `/api/v1/lora/raid`
3. **MCP Tools & Resources Integration** (~200 LOC)
4. **Natural Language LoRA Awareness** (~200 LOC)
5. **Semantic Metadata Store für LoRA-Adapter** (~200 LOC)

**Total:** ~1300 LOC

**Priorität:** MITTEL-HOCH (besonders für Multi-Tenant-Szenarien mit mehreren spezialisierten LoRA-Adaptern)

---

## 📊 Feature-Matrix: Vorhandene vs. Fehlende Funktionen

| Feature | Status | Version | Priorität | Aufwand |
|---------|--------|---------|-----------|---------|
| **MCP Server** | ✅ Implementiert | v1.3.0 | - | - |
| MCP Tools (get_schema, get_stats) | ⚠️ Stub | v1.3.0 | HOCH | MITTEL |
| MCP Resources (schema://, stats://) | ⚠️ Stub | v1.3.0 | HOCH | MITTEL |
| **LLM Integration** | ✅ Optional | v1.3.0+ | - | - |
| ReAct Agent Grammar | ✅ Implementiert | v1.4.0 | - | - |
| **Secondary Index Manager** | ✅ Implementiert | v1.0+ | - | - |
| Index Introspection Methods | ✅ Implementiert | v1.0+ | - | - |
| **Schema Discovery API** | ❌ Fehlt | - | HOCH | HOCH |
| REST /schema Endpoint | ❌ Fehlt | - | HOCH | MITTEL |
| MCP Full Schema Integration | ❌ Fehlt | - | HOCH | MITTEL |
| **Capabilities Endpoint** | ❌ Fehlt | - | MITTEL | NIEDRIG |
| **Natural Language Q&A** | ❌ Fehlt | - | MITTEL | HOCH |
| LLM Self-Awareness Prompts | ❌ Fehlt | - | MITTEL | MITTEL |
| **Domain-Specific Semantic Awareness** | ❌ Fehlt | - | MITTEL-HOCH | HOCH |
| Semantic Metadata Store | ❌ Fehlt | - | MITTEL-HOCH | MITTEL |
| LLM Content Analysis | ❌ Fehlt | - | MITTEL | HOCH |
| Business Context Understanding | ❌ Fehlt | - | HOCH (Behörden) | HOCH |
| **LoRA-RAID Verbund Awareness** | ❌ Fehlt | - | MITTEL-HOCH | HOCH |
| LoRA Introspection API | ❌ Fehlt | - | MITTEL-HOCH | MITTEL |
| REST /api/v1/lora/* Endpoints | ❌ Fehlt | - | MITTEL | MITTEL |
| MCP LoRA Tools & Resources | ❌ Fehlt | - | MITTEL | NIEDRIG |
| LoRA Semantic Metadata | ❌ Fehlt | - | MITTEL | MITTEL |
| **Query Explanation** | ❌ Fehlt | - | NIEDRIG | HOCH |
| EXPLAIN Command | ❌ Fehlt | - | NIEDRIG | HOCH |
| **Audit & Provenance** | ⚠️ Teilweise | v1.3.0 | NIEDRIG | MITTEL |

**Legende:**
- ✅ **Vollständig implementiert**
- ⚠️ **Teilweise implementiert** (Stub/Minimal)
- ❌ **Nicht implementiert**

---

## 🔍 Analyse: Was ist bereits vorhanden?

### Stärken (Strengths)

1. **MCP Server als Basis:**
   - ✅ Vollständige Transport-Implementierung (stdio, SSE, WebSocket)
   - ✅ Tool/Resource/Prompt-Architektur vorhanden
   - ✅ JSON-RPC Protocol-Handling
   - ✅ LLM-Tool Integration (wenn LLM aktiviert)

2. **LLM Integration:**
   - ✅ llama.cpp vollständig integriert
   - ✅ Grammar-Constrained Generation (JSON, XML, CSV, ReAct)
   - ✅ Vision Support, Flash Attention, Speculative Decoding
   - ✅ Embedding Generation für RAG

3. **Index-System:**
   - ✅ Vollständiges Secondary Index System
   - ✅ Introspection-Methods im Code
   - ✅ 7 verschiedene Index-Typen (Regular, Range, Sparse, Geo, TTL, Fulltext, Composite)

4. **Multi-Protocol Support:**
   - ✅ HTTP/REST API
   - ✅ gRPC
   - ✅ PostgreSQL Wire Protocol
   - ✅ MCP (Model Context Protocol)
   - ✅ GraphQL (teilweise)

### Schwächen (Weaknesses)

1. **Fehlende Schema-Discovery:**
   - ❌ Keine zentrale Schema-Registry
   - ❌ Keine `/schema` REST endpoint
   - ❌ MCP `get_schema` ist nur Stub
   - ❌ Keine `SHOW TABLES` / `DESCRIBE TABLE` Befehle

2. **Unvollständige MCP-Integration:**
   - ❌ Tools/Resources nur mit Placeholders
   - ❌ "Minimal integration" statt "Full integration"
   - ❌ Keine echten Schema/Stats-Daten

3. **Keine Natural Language Self-Awareness:**
   - ❌ LLM kann nicht über DB-Structure antworten
   - ❌ Keine System-Prompts für Self-Awareness
   - ❌ Fehlender Context für "Was kannst du?"-Fragen

4. **Fehlende Query Explanation:**
   - ❌ Kein EXPLAIN-Befehl
   - ❌ Keine Query Plan Visualization

---

## 💡 Schlussfolgerungen & Empfehlungen

### Zusammenfassung

**ThemisDB hat bereits eine solide Basis für Agentic AI Self-Awareness:**
- ✅ MCP Server ist implementiert
- ✅ LLM Integration ist vorhanden (optional)
- ✅ Index-System mit Introspection existiert
- ⚠️ **ABER:** Die kritischen Komponenten sind nur als Stubs implementiert

**Hauptproblem:**
Die **MCP-Integration ist "minimal"** - Tools wie `get_schema` und Resources wie `schema://database` liefern nur Placeholder-Daten statt echte Schema-Informationen.

### Empfohlene Roadmap

#### ✅ **Kurzfristig (1-2 Sprints):**
1. **Vollständige MCP Schema-Integration**
   - Echte `get_schema` Implementation
   - Echte `get_stats` Implementation
   - Schema-Discovery aus RocksDB

2. **REST API Endpoints**
   - `GET /api/v1/schema`
   - `GET /api/v1/capabilities`
   - `GET /api/v1/stats/summary`

#### ⏰ **Mittelfristig (3-4 Sprints):**
3. **Natural Language Self-Awareness**
   - System-Prompts für DB-Awareness
   - LLM Context Injection (Schema, Stats)
   - "Was kannst du?"-Fragen beantworten

4. **Enhanced Introspection Tools**
   - Index-Metadaten über API
   - Constraint-Informationen
   - Audit-Daten

#### 🔮 **Langfristig (5+ Sprints):**
5. **Domain-Specific Semantic Awareness**
   - Semantic Metadata Store
   - LLM Content Analysis (Sample-based)
   - Entity Extraction (Fachbegriffe, Rechtsnormen)
   - Business Context Understanding
   - Integration für Behörden-Use-Cases

6. **LoRA-RAID Verbund Awareness**
   - LoRA Introspection API
   - REST API Endpoints (/api/v1/lora/*)
   - MCP Tools & Resources für LoRA
   - Natural Language LoRA Awareness
   - Semantic Metadata für LoRA-Adapter

7. **Query Explanation**
   - EXPLAIN Command
   - Query Plan Visualization
   - Performance Analysis

8. **Advanced Provenance**
   - "Wer hat was wann erstellt?"
   - Query Statistics
   - Data Lineage Tracking

---

## 📚 Weiterführende Dokumentation

### Existierende Docs
- [MCP Protocol Support](../apis/MCP_PROTOCOL_SUPPORT.md)
- [LLM Integration README](../llm/README.md)
- [HTTP API Reference](../apis/HTTP_API_REFERENCE.md)
- [PostgreSQL Wire Protocol](../architecture/POSTGRESQL_WIRE_PROTOCOL.md)

### Zu erstellende Docs
- [ ] Schema Discovery API Specification
- [ ] Self-Awareness Implementation Guide
- [ ] Natural Language Query Examples
- [ ] MCP Full Integration Tutorial

---

## 🎯 Nächste Schritte

1. **Issue erstellen:** "Implement Full MCP Schema Integration"
2. **Proof-of-Concept:** Schema-Discovery aus RocksDB
3. **REST API Design:** `/schema` Endpoint Specification
4. **LLM Prompt Engineering:** System-Prompts für Self-Awareness
5. **Testing:** Integration Tests für Schema-Queries

---

**Erstellt:** 11. Januar 2026  
**Autor:** Research Analysis  
**Version:** 1.0  
**Status:** Abgeschlossen

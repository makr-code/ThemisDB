# Implementation Checklist: Agentic AI Self-Awareness

**Basierend auf:** [AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)  
**Datum:** 11. Januar 2026  
**Status:** Ready for Implementation

---

## 📋 Übersicht

Dieser Checklist führt Entwickler Schritt-für-Schritt durch die Implementierung der Self-Awareness-Features für ThemisDB.

**Geschätzter Aufwand:** ~1400 LOC, 3-4 Sprints  
**Priorität:** HIGH  
**Risiko:** LOW

---

## Phase 1: Schema Manager (HIGH Priority)

**Ziel:** Zentrale Schema-Discovery Komponente  
**Aufwand:** ~500 LOC  
**Zeitrahmen:** 1-2 Sprints

### Tasks

- [ ] **1.1 Header erstellen**
  - [ ] Datei: `include/metadata/schema_manager.h`
  - [ ] Strukturen definieren: `PropertyInfo`, `TableSchema`, `RelationshipSchema`, `DatabaseMetadata`
  - [ ] Public API: `getAllTables()`, `getTable()`, `getAllRelationships()`
  - [ ] Cache-Mechanismus: `table_cache_`, `last_refresh_`
  
- [ ] **1.2 Implementation**
  - [ ] Datei: `src/metadata/schema_manager.cpp`
  - [ ] RocksDB Key-Scanning implementieren
  - [ ] Property-Type Detection aus BaseEntity
  - [ ] Index-Metadaten-Sammlung (aus SecondaryIndexManager)
  - [ ] Cache-Refresh-Logic (alle 60 Sekunden)
  
- [ ] **1.3 JSON Export**
  - [ ] `toJSON()` - Vollständiges Schema
  - [ ] `tableToJSON()` - Einzelne Tabelle
  - [ ] `getDatabaseMetadata()` - Stats
  
- [ ] **1.4 Tests**
  - [ ] Unit Tests: `tests/test_schema_manager.cpp`
  - [ ] Test Schema-Discovery mit Mock-Daten
  - [ ] Test Cache-Refresh
  - [ ] Test Property-Detection

**Definition of Done:**
- [x] SchemaManager kann alle Tabellen aus RocksDB scannen
- [x] Property-Typen werden korrekt erkannt
- [x] Index-Informationen werden gesammelt
- [x] Cache funktioniert korrekt
- [x] Unit Tests bestehen

---

## Phase 2: REST API Endpoints (HIGH Priority)

**Ziel:** HTTP API für Schema-Abfragen  
**Aufwand:** ~300 LOC  
**Zeitrahmen:** 1 Sprint

### Tasks

- [ ] **2.1 Handler-Klasse**
  - [ ] Datei: `include/server/schema_api_handler.h`
  - [ ] Klasse: `SchemaApiHandler`
  - [ ] Methods: `handleGetSchema()`, `handleGetTables()`, `handleGetTable()`, `handleGetCapabilities()`
  
- [ ] **2.2 Endpoints implementieren**
  - [ ] Datei: `src/server/schema_api_handler.cpp`
  - [ ] `GET /api/v1/schema` - Vollständiges Schema
  - [ ] `GET /api/v1/schema/tables` - Liste aller Tabellen
  - [ ] `GET /api/v1/schema/tables/:name` - Einzelne Tabelle
  - [ ] `GET /api/v1/capabilities` - Datenbank-Fähigkeiten
  
- [ ] **2.3 Integration in HttpServer**
  - [ ] Datei: `src/server/http_server.cpp`
  - [ ] `SchemaApiHandler` registrieren
  - [ ] Routes hinzufügen
  
- [ ] **2.4 Tests**
  - [ ] Integration Tests: `tests/test_schema_api.cpp`
  - [ ] Test alle Endpoints
  - [ ] Test Error-Handling
  - [ ] Test JSON-Format

**Definition of Done:**
- [x] Alle 4 Endpoints funktionieren
- [x] JSON-Response ist valide
- [x] Error-Handling korrekt
- [x] Integration Tests bestehen
- [x] Dokumentation aktualisiert

---

## Phase 3: MCP Full Integration (HIGH Priority)

**Ziel:** MCP-Tools mit echten Daten  
**Aufwand:** ~200 LOC  
**Zeitrahmen:** 1 Sprint

### Tasks

- [ ] **3.1 MCP Server Update**
  - [ ] Datei: `src/server/mcp_server.cpp`
  - [ ] `schema_manager_` Member hinzufügen
  - [ ] Constructor: SchemaManager initialisieren
  
- [ ] **3.2 Tool Implementations**
  - [ ] `toolGetSchema()` - Echte Schema-Daten (nicht Stub)
  - [ ] `toolGetStats()` - Echte Statistiken (nicht Hardcoded 0)
  
- [ ] **3.3 Resource Handlers**
  - [ ] `resourceSchema()` - Auf `schema_manager_->toJSON()` umstellen
  - [ ] `resourceStats()` - Echte Stats aus `getDatabaseMetadata()`
  - [ ] `resourceMetadata()` - Capabilities mit Build-Flags
  
- [ ] **3.4 Tests**
  - [ ] Unit Tests: `tests/test_mcp_integration.cpp`
  - [ ] Test MCP Tools mit echten Daten
  - [ ] Test Resources
  - [ ] Test Prompts

**Definition of Done:**
- [x] MCP get_schema liefert echte Daten (nicht empty arrays)
- [x] MCP get_stats liefert echte Zahlen
- [x] Resources funktionieren
- [x] Tests bestehen
- [x] "integration_level" ist "full" statt "minimal"

---

## Phase 4: Natural Language Self-Awareness (MEDIUM Priority)

**Ziel:** LLM kann Fragen über DB beantworten  
**Aufwand:** ~400 LOC  
**Zeitrahmen:** 2-3 Sprints

### Tasks

- [ ] **4.1 System-Prompts**
  - [ ] Datei: `config/llm_system_prompts.yaml`
  - [ ] Prompt: `self_awareness_prompt`
  - [ ] Prompt: `what_can_you_do_prompt`
  - [ ] Prompt: `data_structure_prompt`
  - [ ] Prompt: `purpose_prompt`
  
- [ ] **4.2 Context Injection**
  - [ ] Datei: `src/llm/prompt_manager.cpp`
  - [ ] Function: `injectContext(template, context)`
  - [ ] Template-Variable Replacement: `{version}`, `{edition}`, etc.
  
- [ ] **4.3 MCP Tool: introspect_database**
  - [ ] Datei: `src/server/mcp_server.cpp`
  - [ ] Tool registrieren: `introspect_database`
  - [ ] Implementation: `toolIntrospectDatabase()`
  - [ ] Question-Type Detection (was kannst du / aufgebaut / aufgabe)
  - [ ] LLM Integration mit Context
  
- [ ] **4.4 Tests**
  - [ ] Integration Tests: `tests/test_llm_self_awareness.cpp`
  - [ ] Test "Was kannst du?"
  - [ ] Test "Wie sind die Daten aufgebaut?"
  - [ ] Test "Was ist deine Aufgabe?"
  - [ ] Test Context-Injection

**Definition of Done:**
- [x] User kann "Was kannst du?" fragen
- [x] LLM antwortet mit sinnvollen Informationen
- [x] Context wird korrekt injected
- [x] Alle Frage-Typen funktionieren
- [x] Tests bestehen (mit und ohne LLM-Flag)

---

## 📊 Progress Tracking

| Phase | Status | LOC | Tests | Docs |
|-------|--------|-----|-------|------|
| Phase 1: Schema Manager | ❌ Not Started | 0/500 | 0/4 | ❌ |
| Phase 2: REST API | ❌ Not Started | 0/300 | 0/4 | ❌ |
| Phase 3: MCP Integration | ❌ Not Started | 0/200 | 0/3 | ❌ |
| Phase 4: Natural Language | ❌ Not Started | 0/400 | 0/4 | ❌ |
| **TOTAL** | **0%** | **0/1400** | **0/15** | **❌** |

---

## 🧪 Testing Strategy

### Unit Tests
- [ ] `test_schema_manager.cpp` - Schema-Discovery
- [ ] `test_schema_api.cpp` - REST API Endpoints
- [ ] `test_mcp_integration.cpp` - MCP Tools/Resources
- [ ] `test_llm_self_awareness.cpp` - Natural Language Q&A

### Integration Tests
- [ ] Schema-Discovery mit echter RocksDB
- [ ] REST API mit vollständiger HTTP-Server-Integration
- [ ] MCP mit Claude Desktop Client
- [ ] LLM Self-Awareness End-to-End

### Manual Tests
- [ ] curl http://localhost:8765/api/v1/schema
- [ ] curl http://localhost:8765/api/v1/capabilities
- [ ] MCP Call via Claude Desktop: "Was kannst du?"
- [ ] Python Client: `client.mcp.introspect("Wie sind die Daten aufgebaut?")`

---

## 📚 Dokumentation Updates

Nach jeder Phase:

- [ ] **Phase 1:**
  - [ ] README.md: Schema Manager erwähnen
  - [ ] CHANGELOG.md: Entry für v1.5.0
  - [ ] API Reference updaten

- [ ] **Phase 2:**
  - [ ] docs/apis/HTTP_API_REFERENCE.md: Neue Endpoints
  - [ ] README.md: Quick Start mit Schema-Queries

- [ ] **Phase 3:**
  - [ ] docs/apis/MCP_PROTOCOL_SUPPORT.md: "Full Integration" Status
  - [ ] MCP Examples aktualisieren

- [ ] **Phase 4:**
  - [ ] docs/llm/README.md: Self-Awareness Feature
  - [ ] User Guide: Natural Language Queries

---

## 🚀 Deployment Checklist

- [ ] CMakeLists.txt: Optional `THEMIS_ENABLE_SCHEMA_INTROSPECTION` Flag?
- [ ] Build testen: Linux, Windows, macOS
- [ ] Docker Image aktualisieren
- [ ] Release Notes schreiben
- [ ] Migration Guide (falls nötig)

---

## 💡 Tips für Entwickler

1. **Start mit Phase 1:** SchemaManager ist die Basis für alles
2. **Cache früh optimieren:** RocksDB Key-Scans können teuer sein
3. **Thread-Safety:** SchemaManager wird von mehreren Threads genutzt
4. **Error Handling:** Robust gegen fehlende Tabellen/Properties
5. **Testing:** Teste mit leeren, kleinen und großen Datenbanken
6. **LLM-Flag:** Alle LLM-Features müssen auch ohne `-DTHEMIS_ENABLE_LLM=ON` funktionieren

---

## 📞 Fragen?

- Research Docs: [README.md](README.md)
- Detailed Analysis: [AGENTIC_AI_SELF_AWARENESS_RESEARCH.md](AGENTIC_AI_SELF_AWARENESS_RESEARCH.md)
- Code Examples: [AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md](AGENTIC_AI_IMPLEMENTATION_EXAMPLE.md)
- Issues: GitHub Issues mit Label "self-awareness"

---

**Erstellt:** 11. Januar 2026  
**Für:** ThemisDB v1.5 Development  
**Status:** Ready for Implementation

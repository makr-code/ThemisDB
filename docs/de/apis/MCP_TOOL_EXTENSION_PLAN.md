# MCP Tool Extension Plan — ThemisDB

**Version:** 1.0  
**Status:** 📋 Planungsphase  
**Erstellt:** 2026-08-17  
**Ziel-Branch:** `develop`  
**Wave-Alignment:** Wave B/C (Q4 2026 – Q1 2027)

---

## Inhaltsverzeichnis

- [Ausgangslage](#ausgangslage)
- [Ziel](#ziel)
- [Aktuell registrierte Tools (Bestand)](#aktuell-registrierte-tools-bestand)
- [Neue Tool-Gruppen](#neue-tool-gruppen)
  - [Gruppe 1: Knowledge Graph / Retrieval](#gruppe-1-knowledge-graph--retrieval)
  - [Gruppe 2: Vector Search & Hybrid Search](#gruppe-2-vector-search--hybrid-search)
  - [Gruppe 3: Plugin & LLM Management](#gruppe-3-plugin--llm-management)
  - [Gruppe 4: Operations & Monitoring](#gruppe-4-operations--monitoring)
  - [Gruppe 5: Updates & Maintenance](#gruppe-5-updates--maintenance)
  - [Gruppe 6: Security & Audit](#gruppe-6-security--audit)
  - [Gruppe 7: Schema & Index (Ergänzungen)](#gruppe-7-schema--index-ergänzungen)
- [Implementierungsphasen](#implementierungsphasen)
- [API-Kontrakt je Tool](#api-kontrakt-je-tool)
- [Integrationspunkte in mcp_server.cpp](#integrationspunkte-in-mcp_servercpp)
- [Test-Strategie](#test-strategie)
- [Fehlerbehandlung & Fehlercodes](#fehlerbehandlung--fehlercodes)
- [Akzeptanzkriterien](#akzeptanzkriterien)
- [Production Readiness Checklist](#production-readiness-checklist)
- [Known Issues & Limitations](#known-issues--limitations)
- [Breaking Changes](#breaking-changes)

---

## Ausgangslage

Der ThemisDB MCP Server (`src/server/mcp_server.cpp`, `include/server/mcp_server.h`) ist produktionsreif und registriert aktuell **19 Tools** via `registerDefaultTools()` und `attachAIOrchestrator()`. Das Protokoll basiert auf JSON-RPC 2.0 über stdio, HTTP, und WebSocket-Transport.

**Bestehende Implementierung:**
- Maturity Score: 86/100 (Gap Summary: 5 offene Punkte, davon 3 Stubs)
- Registrierung via `McpServer::registerTool(name, description, input_schema, handler)`
- Handler-Typ: `std::function<json(const json& args)>`

---

## Ziel

Die Tool-Palette des ThemisDB MCP Servers von 19 auf **~40 Tools** erweitern, um:

1. KI-gestützte RAG/Retrieval-Workflows vollständig abzudecken
2. Knowledge-Graph-Navigation für LLM-Agenten bereitzustellen
3. Betriebliche Observability-Tools für Agenten zugänglich zu machen
4. Plugin- und LLM-Lifecycle-Management über MCP zu ermöglichen
5. Security- und Audit-Operationen für privilegierte Agenten zu exponieren

---

## Aktuell registrierte Tools (Bestand)

| Tool-Name | Gruppe | Status |
|---|---|---|
| `query` | Data Access | ✅ Produktiv |
| `put_entity` | Data Access | ✅ Produktiv |
| `get_entity` | Data Access | ✅ Produktiv |
| `delete_entity` | Data Access | ✅ Produktiv |
| `get_schema` | Schema | ✅ Produktiv |
| `get_stats` | Monitoring | ✅ Produktiv |
| `create_index` | Index | ✅ Produktiv |
| `drop_index` | Index | ✅ Produktiv |
| `list_indexes` | Index | ✅ Produktiv |
| `ai_cleanup_snapshots` | Maintenance | ⚠️ Stub |
| `llm_complete` | LLM | ✅ Produktiv |
| `llm_embed` | LLM | ✅ Produktiv |
| `llm_chat` | LLM | ✅ Produktiv |
| `database_query_with_llm` | LLM + Query | ✅ Produktiv |
| `get_error_info` | Diagnostics | ✅ Produktiv |
| `search_errors` | Diagnostics | ✅ Produktiv |
| `introspect_database` | Diagnostics | ✅ Produktiv |
| `llm_orchestrate` | AI Orchestration | ✅ Produktiv |
| `llm_list_modes` | AI Orchestration | ✅ Produktiv |

---

## Neue Tool-Gruppen

### Gruppe 1: Knowledge Graph / Retrieval

**Wave:** B (Q4 2026)  
**Priorität:** Hoch  
**Implementierungsaufwand:** Mittel (Anbindung an bestehenden `graph_api_handler`)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `kg_neighbours` | Nachbarn eines Knotens abrufen (Tiefe, Kantentypen konfigurierbar) | Q4 2026 |
| `kg_shortest_path` | Kürzester Pfad zwischen zwei Knoten (Dijkstra / BFS) | Q4 2026 |
| `kg_subgraph` | Teilgraph um einen Anker-Knoten extrahieren | Q1 2027 |
| `kg_node_properties` | Alle Eigenschaften eines Knotens abrufen | Q4 2026 |

**Design Constraints:**
- Maximale Antwortgröße begrenzen (default: 1.000 Knoten/Kanten, konfigurierbar)
- Zyklen im Graphen müssen sicher behandelt werden (Besuchsliste)
- Fehlerpfad bei nicht-existierendem Knoten: JSON-Fehler mit Code, keine Exception

### Gruppe 2: Vector Search & Hybrid Search

**Wave:** B (Q4 2026)  
**Priorität:** Hoch  
**Implementierungsaufwand:** Mittel (Anbindung an `vector_api_handler`, `llm_embed`)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `semantic_search` | Vektor-basiertes semantisches Retrieval (kNN) | Q4 2026 |
| `hybrid_search` | BM25 + Vektor kombiniert mit Gewichtungsparameter | Q4 2026 |
| `rag_retrieve` | RAG-Pipeline: Embed → Search → Rerank → Return | Q4 2026 |
| `vector_index_list` | Verfügbare Vektor-Indizes auflisten | Q4 2026 |

**Design Constraints:**
- `semantic_search` und `hybrid_search` akzeptieren sowohl Text-Query (auto-embed) als auch direkten Embedding-Vektor
- `rag_retrieve` gibt Quellen + relevante Chunks in strukturierter Form zurück (für LLM-Context)
- Maximale Kandidatenanzahl (`top_k`) begrenzt auf 200 (konfigurierbar)

### Gruppe 3: Plugin & LLM Management

**Wave:** B/C (Q4 2026 – Q1 2027)  
**Priorität:** Mittel  
**Implementierungsaufwand:** Mittel (Anbindung an LLMPluginManager)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `plugin_list` | Geladene Plugins auflisten mit Status und Metadaten | Q4 2026 |
| `plugin_load` | Plugin dynamisch laden (mit Signaturvalidierung) | Q1 2027 |
| `plugin_unload` | Plugin entladen (mit Drain-Wartezeit) | Q1 2027 |
| `llm_model_list` | Verfügbare LLM-Modelle und deren Status | Q4 2026 |
| `llm_model_status` | Detailierter Status eines spezifischen Modells | Q4 2026 |

**Design Constraints:**
- `plugin_load` und `plugin_unload` erfordern erhöhte Berechtigungen (`admin`-Scope)
- `plugin_load` muss Signaturvalidierung synchron durchführen; lädt nicht bei Validierungsfehler
- Status-Tools sind read-only, keine Berechtigungsanforderung über Standard-Auth hinaus

### Gruppe 4: Operations & Monitoring

**Wave:** C (Q1 2027)  
**Priorität:** Mittel  
**Implementierungsaufwand:** Gering–Mittel (Anbindung an `monitoring_api_handler`, `health_error_service`)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `health_check` | Server-Health inkl. Shard-Status und Subsystem-Zustand | Q1 2027 |
| `metrics_snapshot` | Aktuelle Performance-Metriken (Latenz-Perzentile, Durchsatz) | Q1 2027 |
| `shard_status` | Sharding-Topologie, Verteilung und Replikationsstatus | Q1 2027 |
| `compaction_trigger` | RocksDB Compaction manuell anstoßen | Q1 2027 |
| `connection_pool_status` | Status des Connection Pools | Q1 2027 |

**Design Constraints:**
- `compaction_trigger` erfordert `admin`-Scope, alle anderen erfordern `read`-Scope
- Metriken sind Snapshot-basiert (nicht Streaming) — für Streaming: SSE-Endpoint
- `shard_status` gibt strukturiertes JSON zurück, das direkt für LLM-Analyse geeignet ist

### Gruppe 5: Updates & Maintenance

**Wave:** C (Q1 2027)  
**Priorität:** Mittel  
**Implementierungsaufwand:** Mittel (Anbindung an `update_api_handler`, Updates-Modul)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `update_list_pending` | Ausstehende Updates/Migrationen auflisten | Q1 2027 |
| `update_apply` | Schema-/Daten-Migration anwenden | Q1 2027 |
| `update_rollback` | Letzten Update-Batch zurückrollen | Q1 2027 |
| `backup_create` | Snapshot/Backup initiieren | Q1 2027 |
| `backup_list` | Verfügbare Backups auflisten | Q1 2027 |
| `backup_restore` | Backup wiederherstellen (mit Bestätigungs-Token) | Q2 2027 |

**Design Constraints:**
- `backup_restore` ist destruktiv: erfordert `confirm_token` (einmaliges Token aus `backup_list`) zur Bestätigung
- `update_apply` und `update_rollback` erfordern `admin`-Scope
- Fehlercodes folgen dem Updates-Modul-Bereich [7400–7499]
- `update_rollback` nutzt das bestehende Isolation-Modell aus dem Updates-Modul

### Gruppe 6: Security & Audit

**Wave:** C/D (Q1–Q2 2027)  
**Priorität:** Niedrig–Mittel  
**Implementierungsaufwand:** Gering (Anbindung an `audit_api_handler`, `auth_middleware`)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `audit_log_query` | Audit-Log abfragen (RBAC-gefiltert, nur eigene Scopes) | Q1 2027 |
| `permission_check` | Berechtigungs-Check für Ressource/Aktion | Q1 2027 |
| `token_validate` | Auth-Token validieren und Claims inspizieren | Q1 2027 |
| `security_scan_status` | Letzten Sicherheits-Scan-Status abrufen | Q2 2027 |

**Design Constraints:**
- `audit_log_query` gibt immer nur Einträge zurück, die für den aktuellen Auth-Scope sichtbar sind (RBAC-gefiltert)
- `token_validate` gibt Claims zurück, aber **niemals** den Token selbst zurück
- `security_scan_status` ist read-only, kein `admin`-Scope erforderlich

### Gruppe 7: Schema & Index (Ergänzungen)

**Wave:** B (Q4 2026)  
**Priorität:** Hoch  
**Implementierungsaufwand:** Gering (Erweiterung bestehender Schema-Tools)

| Tool-Name | Beschreibung | Ziel |
|---|---|---|
| `schema_diff` | Schema-Vergleich zwischen zwei Versionen | Q4 2026 |
| `schema_validate` | Schema gegen bekannte Constraints validieren | Q4 2026 |
| `explain_query` | Query-Execution-Plan ohne Ausführung abrufen | Q4 2026 |

---

## Implementierungsphasen

### Phase 1: Design / API-Kontrakt (Q3 2026)

- [ ] JSON-Schema für alle neuen Tools definieren (input + output)
- [ ] Auth-Scope-Anforderungen pro Tool festlegen (read / write / admin)
- [ ] Fehlercode-Mapping für neue Gruppen dokumentieren
- [ ] Rückwärtskompatibilität bestehender Tools verifizieren

### Phase 2: Core-Implementierung (Q4 2026 — Gruppe 1, 2, 7)

- [ ] `kg_neighbours`, `kg_shortest_path`, `kg_node_properties`, `kg_subgraph` implementieren
- [ ] `semantic_search`, `hybrid_search`, `rag_retrieve`, `vector_index_list` implementieren
- [ ] `schema_diff`, `schema_validate`, `explain_query` implementieren
- [ ] `plugin_list`, `llm_model_list`, `llm_model_status` implementieren

### Phase 3: Fehlerbehandlung & Edge Cases (Q4 2026)

- [ ] Alle neuen Handler mit vollständiger Exception-Safety ausstatten
- [ ] Eingabe-Validierung für alle Tool-Arguments implementieren (JSON-Schema-Prüfung)
- [ ] Timeout-Handling für langlaufende Tools (Abbruch nach konfigurierbarem Budget)
- [ ] Leerer-Ergebnis-Pfad für alle Such-Tools definieren und testen

### Phase 4: Tests (Q4 2026 – Q1 2027)

- [ ] Unit-Tests für jeden neuen Tool-Handler (GTest)
- [ ] Integration-Tests mit echtem MCP-Client-Protokoll
- [ ] Negative-Tests: ungültige Argumente, fehlende Pflichtfelder, Scope-Verletzungen
- [ ] Fuzz-Tests für Query-/Search-Parameter

### Phase 5: Performance / Hardening (Q1 2027 — Gruppe 3, 4, 5)

- [ ] Benchmarks für `semantic_search` und `rag_retrieve` (Ziel: p99 ≤ 500ms bei k=10)
- [ ] Benchmarks für `kg_neighbours` (Ziel: p99 ≤ 200ms bei depth=3, fan-out=50)
- [ ] `plugin_load` / `plugin_unload` mit Drain-Semantik implementieren
- [ ] `update_apply` / `update_rollback` mit Updates-Modul-Integration implementieren
- [ ] Alle Operations & Monitoring Tools implementieren

### Phase 6: Dokumentation & Abnahme (Q1–Q2 2027 — Gruppe 6)

- [ ] `audit_log_query`, `permission_check`, `token_validate` implementieren
- [ ] `MCP_API_SPECIFICATION.md` mit neuen Tools aktualisieren
- [ ] Beispiele und Integration-Guides ergänzen
- [ ] Production Readiness Checklist für alle Gruppen abzeichnen

---

## API-Kontrakt je Tool

### `kg_neighbours`

```json
{
  "name": "kg_neighbours",
  "description": "Retrieve neighbours of a node in the knowledge graph",
  "inputSchema": {
    "type": "object",
    "required": ["node_id"],
    "properties": {
      "node_id":    { "type": "string", "description": "Node identifier" },
      "depth":      { "type": "integer", "minimum": 1, "maximum": 5, "default": 1 },
      "edge_types": { "type": "array", "items": { "type": "string" }, "description": "Filter by edge type; empty = all" },
      "max_nodes":  { "type": "integer", "minimum": 1, "maximum": 1000, "default": 100 },
      "collection": { "type": "string", "description": "Graph collection name" }
    }
  }
}
```

**Ausgabe:**
```json
{
  "node_id": "node/42",
  "depth_reached": 2,
  "nodes": [{ "id": "...", "properties": {} }],
  "edges": [{ "from": "...", "to": "...", "type": "...", "weight": 1.0 }],
  "truncated": false
}
```

### `semantic_search`

```json
{
  "name": "semantic_search",
  "description": "Semantic vector search over embedded documents",
  "inputSchema": {
    "type": "object",
    "required": ["query"],
    "properties": {
      "query":      { "type": "string", "description": "Text query (auto-embedded) OR base64 float32 vector" },
      "top_k":      { "type": "integer", "minimum": 1, "maximum": 200, "default": 10 },
      "collection": { "type": "string" },
      "filter":     { "type": "object", "description": "Optional metadata pre-filter" },
      "threshold":  { "type": "number", "minimum": 0.0, "maximum": 1.0, "description": "Minimum similarity score" }
    }
  }
}
```

**Ausgabe:**
```json
{
  "results": [
    { "id": "...", "score": 0.92, "content": "...", "metadata": {} }
  ],
  "total_candidates_scanned": 50000,
  "query_embedding_model": "nomic-embed-text"
}
```

### `rag_retrieve`

```json
{
  "name": "rag_retrieve",
  "description": "Full RAG pipeline: embed query, search, rerank, return context chunks",
  "inputSchema": {
    "type": "object",
    "required": ["query"],
    "properties": {
      "query":           { "type": "string" },
      "top_k":           { "type": "integer", "default": 5, "maximum": 20 },
      "collection":      { "type": "string" },
      "rerank":          { "type": "boolean", "default": true },
      "include_sources": { "type": "boolean", "default": true },
      "max_chunk_tokens":{ "type": "integer", "default": 512 }
    }
  }
}
```

**Ausgabe:**
```json
{
  "context_chunks": [
    { "rank": 1, "content": "...", "score": 0.95, "source": { "id": "...", "collection": "..." } }
  ],
  "total_tokens_estimate": 1024,
  "retrieval_latency_ms": 45
}
```

### `health_check`

```json
{
  "name": "health_check",
  "description": "Get server health status including shard and subsystem state",
  "inputSchema": {
    "type": "object",
    "properties": {
      "include_shards":     { "type": "boolean", "default": true },
      "include_subsystems": { "type": "boolean", "default": true }
    }
  }
}
```

**Ausgabe:**
```json
{
  "status": "healthy",
  "uptime_seconds": 86400,
  "shards": [{ "id": 0, "status": "ok", "leader": true }],
  "subsystems": { "rocksdb": "ok", "llm": "ok", "network": "ok" },
  "version": "1.5.0"
}
```

### `explain_query`

```json
{
  "name": "explain_query",
  "description": "Retrieve query execution plan without executing the query",
  "inputSchema": {
    "type": "object",
    "required": ["query"],
    "properties": {
      "query":    { "type": "string" },
      "language": { "type": "string", "enum": ["aql", "cypher", "sql"], "default": "aql" }
    }
  }
}
```

**Ausgabe:**
```json
{
  "plan": {
    "nodes": [{ "type": "IndexScan", "index": "idx_name", "estimated_cost": 12.5 }],
    "estimated_total_cost": 12.5,
    "optimizations_applied": ["index_scan", "filter_pushdown"]
  }
}
```

---

## Integrationspunkte in mcp_server.cpp

Alle neuen Tools werden in `McpServer::registerDefaultTools()` registriert.  
Handler-Methoden werden als private Member in `McpServer` deklariert (analog zu bestehenden `toolQuery`, `toolLLMChat` etc.).

**Neue private Handler (Deklaration in `mcp_server.h`):**

```cpp
// Gruppe 1: Knowledge Graph
json toolKgNeighbours(const json& args);
json toolKgShortestPath(const json& args);
json toolKgSubgraph(const json& args);
json toolKgNodeProperties(const json& args);

// Gruppe 2: Vector / Hybrid / RAG
json toolSemanticSearch(const json& args);
json toolHybridSearch(const json& args);
json toolRagRetrieve(const json& args);
json toolVectorIndexList(const json& args);

// Gruppe 3: Plugin & LLM Management
json toolPluginList(const json& args);
json toolPluginLoad(const json& args);
json toolPluginUnload(const json& args);
json toolLlmModelList(const json& args);
json toolLlmModelStatus(const json& args);

// Gruppe 4: Operations
json toolHealthCheck(const json& args);
json toolMetricsSnapshot(const json& args);
json toolShardStatus(const json& args);
json toolCompactionTrigger(const json& args);
json toolConnectionPoolStatus(const json& args);

// Gruppe 5: Updates & Backup
json toolUpdateListPending(const json& args);
json toolUpdateApply(const json& args);
json toolUpdateRollback(const json& args);
json toolBackupCreate(const json& args);
json toolBackupList(const json& args);
json toolBackupRestore(const json& args);

// Gruppe 6: Security & Audit
json toolAuditLogQuery(const json& args);
json toolPermissionCheck(const json& args);
json toolTokenValidate(const json& args);
json toolSecurityScanStatus(const json& args);

// Gruppe 7: Schema (Ergänzungen)
json toolSchemaDiff(const json& args);
json toolSchemaValidate(const json& args);
json toolExplainQuery(const json& args);
```

**Backend-Abhängigkeiten:**

| Handler-Gruppe | Voraussetzung (inject via McpServer::Config oder constructor) |
|---|---|
| KG-Tools | `graph_api_handler.h` / `GraphApiHandler` |
| Vector/RAG-Tools | `vector_api_handler.h` + `LLMPluginManager` für auto-embed |
| Plugin-Tools | `LLMPluginManager` (schon via `attachAIOrchestrator` verfügbar) |
| Operations-Tools | `health_error_service.h`, `monitoring_api_handler.h`, `shard_repair_api_handler.h` |
| Updates-Tools | `update_api_handler.h` |
| Audit-Tools | `audit_api_handler.h`, `auth_middleware.h` |
| Schema-Tools | `schema_api_handler.h`, `query_api_handler.h` |

---

## Test-Strategie

### Unit-Tests (je Gruppe ein Test-File)

| Datei | Testfälle |
|---|---|
| `tests/server/test_mcp_kg_tools.cpp` | 16 Tests: kg_neighbours (depth 1/2/3, Zyklen, nicht-exist. Knoten), kg_shortest_path, kg_subgraph |
| `tests/server/test_mcp_search_tools.cpp` | 16 Tests: semantic_search (Text/Vektor-Input, threshold, filter), hybrid_search, rag_retrieve |
| `tests/server/test_mcp_plugin_tools.cpp` | 12 Tests: plugin_list, plugin_load (Signatur OK/NOK), plugin_unload |
| `tests/server/test_mcp_ops_tools.cpp` | 12 Tests: health_check (all-ok, shard-down), metrics_snapshot, shard_status, compaction_trigger |
| `tests/server/test_mcp_update_tools.cpp` | 16 Tests: update_list_pending, update_apply, update_rollback, backup_create, backup_restore (mit/ohne Token) |
| `tests/server/test_mcp_audit_tools.cpp` | 12 Tests: audit_log_query (RBAC-Filter), permission_check, token_validate |

### Integration-Tests

- [ ] End-to-end MCP-Protokoll-Test mit stdio-Transport für alle neuen Tool-Gruppen
- [ ] Auth-Scope-Tests: Anfragen ohne ausreichende Berechtigungen müssen mit `-32003` (Unauthorized) fehlschlagen
- [ ] Timeout-Tests: Tools die länger als Budget dauern, müssen abgebrochen werden

---

## Fehlerbehandlung & Fehlercodes

Neue Tools folgen der bestehenden Fehler-Taxonomie. Neue MCP-spezifische Codes:

| Code | Bedeutung |
|---|---|
| `-32001` | Internal error (generisch) |
| `-32003` | Unauthorized / insufficient scope |
| `-32601` | Tool handler not available |
| `-32602` | Invalid params / schema violation |
| `7400–7499` | Updates-Modul-Fehler (bestehender Bereich) |

Alle Handler müssen `try/catch` für `std::exception` implementieren und niemals eine Exception nach außen propagieren.

---

## Akzeptanzkriterien

- [ ] Alle neuen Tools erscheinen in der `tools/list`-Antwort des MCP-Servers
- [ ] Jedes Tool hat ein vollständiges `inputSchema` (JSON-Schema Draft 7)
- [ ] Alle Tools mit `admin`-Scope geben `-32003` bei fehlendem Scope zurück
- [ ] `semantic_search` p99 Latenz ≤ 500ms bei `top_k=10`, 100k Dokumente
- [ ] `kg_neighbours` p99 Latenz ≤ 200ms bei `depth=3`, fan-out ≤ 50
- [ ] `rag_retrieve` gibt strukturierte Chunks zurück, direkt verwendbar als LLM-Kontext
- [ ] `backup_restore` ohne `confirm_token` schlägt fehl (kein unbeabsichtigtes Restore)
- [ ] Alle Tests GREEN auf `develop`
- [ ] `MCP_API_SPECIFICATION.md` vollständig aktualisiert

---

## Production Readiness Checklist

- [ ] Phase 1 (API-Kontrakt) abgeschlossen
- [ ] Phase 2 (KG + Search + Schema) implementiert und getestet
- [ ] Phase 3 (Fehlerbehandlung) vollständig
- [ ] Phase 4 (Tests) alle GREEN
- [ ] Phase 5 (Plugin + Ops + Updates) implementiert
- [ ] Phase 6 (Audit + Docs) abgeschlossen
- [ ] `release_critical` CI Gate GREEN auf `develop`
- [ ] Server-Modul-ROADMAP.md aktualisiert
- [ ] MCP_API_SPECIFICATION.md v2.0 veröffentlicht
- [ ] Kein Tool als Stub registriert (alle Handler implementiert)

---

## Known Issues & Limitations

- `ai_cleanup_snapshots` ist im Bestand als Stub markiert — wird im Rahmen von Gruppe 5 (Backup-Tools) vollständig implementiert
- `plugin_load` und `plugin_unload` sind von der Plugin-Governance (private Plugin-Submodule) abhängig — Community-Edition erhält ggf. eingeschränkte Plugin-Liste
- `compaction_trigger` wirkt sich direkt auf RocksDB-Performance aus; darf nicht von automatisierten Agenten ohne Rate-Limit aufgerufen werden

---

## Breaking Changes

Keine. Alle neuen Tools sind additiv. Bestehende Tool-Registrierungen bleiben unverändert.

Die `tools/list`-Antwort wächst von ~19 auf ~40 Einträge — MCP-Clients müssen dies tolerieren (konform mit MCP-Protokoll-Spezifikation).

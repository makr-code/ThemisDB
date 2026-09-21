---
Author: ThemisDB Maintainers
Created: 2026-08-31
Last Updated: 2026-09-21
Status: active
---
# ThemisDB — Actionable TODOs (Reality Check)

**Erstellt:** 2026-09-21T04:49:00Z  
**Commit:** `e46947c2`  
**Branch:** `develop`  
**Basis:** Reality-Check-Scan nach Entfernung aller `@note Gap Summary` False-Positives  
**Marker nach Cleanup:** 239 (vorher 1712; 1473 False Positives entfernt)

---

## Zusammenfassung

| Kategorie | Anzahl |
|-----------|-------:|
| Actionable TODO (echter Handlungsbedarf) | **48** |
| Documented STUB/SIMULATION NOTE (governance-konform) | 86 |
| Env-gated STUB (`THEMIS_ALLOW_*`) | 53 |
| MOCK (env-gated oder context) | 3 |
| STUB comment / reference | 49 |

Die 48 actionable TODOs sind nachfolgend thematisch gruppiert und, wo vorhanden,
mit offenen GitHub-Issues verknüpft.

---

## Gruppe 1 — Chimera Adapter: Unvollständige Backend-Anbindung (29 TODOs)

**Kritisch · alle drei Adapter haben keine echten Verbindungen/Operationen**

### Issue-Bezug
- Issue #6478 betrifft `distributed_transaction_manager` (Guarded Stub) — kein direkter Chimera-Issue vorhanden.
- Kein dediziertes offenes Issue für Chimera-Adapter-Implementierung gefunden.

### MongoDB Adapter (`src/chimera/mongodb_adapter.cpp`)

| Zeile | TODO |
|------:|------|
| 69 | Actual `mongocxx::client` / `mongocxx::uri` creation |
| 114 | Translate AQL → MongoDB aggregation pipeline and execute |
| 139 | Convert `RelationalRow` → BSON document and insert into collection |
| 163 | Batch insert documents via `bulk_write` |
| 237 | Store node as document in nodes collection |
| 251 | Store edge as document with source/target node references |
| 311 | Serialize doc to BSON and insert into named collection |
| 336 | Batch insert BSON documents via `insert_many` |
| 361 | Execute `find()` with BSON filter and limit, map to `Documents` |
| 387 | Execute `update_many()` with BSON filter and update document |
| 619 | Rollback-to-savepoint logic via `mongocxx` session |

### Qdrant Adapter (`src/chimera/qdrant_adapter.cpp`)

| Zeile | TODO |
|------:|------|
| 68 | Actual gRPC channel creation to Qdrant endpoint |
| 150 | Upsert point via gRPC `UpsertPoints` RPC |
| 198 | Execute KNN search via gRPC `Search` RPC with payload filter |
| 224 | Create collection with `VectorParams` (size, distance metric) via gRPC |

### Neo4j Adapter (`src/chimera/neo4j_adapter.cpp`)

| Zeile | TODO |
|------:|------|
| 66 | Actual `neo4j::Driver` creation via bolt URI |
| 196 | `CREATE (node:Label {properties})` via Cypher session |
| 218 | `CREATE (from)-[rel:TYPE]->(to)` via Cypher session |
| 244 | `shortestPath()` Cypher query with `max_depth` bound |
| 270 | BFS/DFS Cypher traversal query up to `max_depth` |
| 295 | Arbitrary Cypher query → map results to `GraphPath` |
| 324 | Create node with collection label + document properties via Cypher |
| 349 | Batch `UNWIND + CREATE` nodes via Cypher |
| 374 | `MATCH (n:collection {filter}) RETURN n LIMIT limit` |
| 400 | `MATCH (n:collection {filter}) SET n += updates` |
| 445 | Commit transaction via Neo4j session |
| 470 | Rollback transaction via Neo4j session |

**Handlungsbedarf:** Alle drei Adapter sind strukturell vorhanden, aber die
eigentliche I/O-Logik (Treiber-Init, Queries, Batch-Ops, Transaktionen) fehlt.
→ Empfehlung: Issue `chimera: implement MongoDB / Qdrant / Neo4j adapter backends` erstellen.

---

## Gruppe 2 — Analytics: Streaming Window Tracked TODOs (9 Einträge)

**Status: Alle 8 `TODO(v1.8.0)` als RESOLVED markiert, ein Fix-Kommentar**

| Zeile | TODO | Status |
|------:|------|--------|
| 53 | `TODO(v1.8.0) #1` — idle_timeout background thread | ✅ RESOLVED |
| 58 | `TODO(v1.8.0) #2` — partition_key in InternalWindow | ✅ RESOLVED |
| 62 | `TODO(v1.8.0) #3` — SessionWindow::expiryLoop | ✅ RESOLVED |
| 66 | `TODO(v1.8.0) #4` — StreamingWindowPipeline::Config | ✅ RESOLVED |
| 71 | `TODO(v1.8.0) #5` — O(N) duplicate-detection | ✅ RESOLVED |
| 76 | `TODO(v1.8.0) #6` — `calcPercentile()` const ref | ✅ RESOLVED |
| 80 | `TODO(v1.8.0) #7` — `SessionWindow::computeResult()` | ✅ RESOLVED |
| 84 | `TODO(v1.8.0) #8` — double-close guard | ✅ RESOLVED |
| 172 | Fix-Kommentar zu #6 | ✅ Dokumentation |

**Handlungsbedarf:** Kein echter offener TODO — alle RESOLVED. Die Kommentarblöcke
können mit einem Follow-up-Commit bereinigt werden (optionale Housekeeping-Aufgabe).
→ **Diese 9 können aus der actionable Liste gestrichen werden** (false positive im aktuellen Scan).

---

## Gruppe 3 — Tensor: Fehlende Implementierungsschritte (4 TODOs)

| Datei | Zeile | TODO | Bezug |
|-------|------:|------|-------|
| `src/tensor/compression_strategy.cpp` | 48 | Wire to actual `TensorTrainDecomposer` | `src/tensor/ROADMAP.md` |
| `src/tensor/compression_strategy.cpp` | 332 | Implement strategy registry | `src/tensor/ROADMAP.md` |
| `src/tensor/tensor_routing_strategy.cpp` | 81 | Parse `created_at`, age-based freshness decay | Tracked |
| `src/tensor/tensor_routing_strategy.cpp` | 98 | Compute age-based freshness from timestamp | Tracked |
| `src/tensor/tensor_routing_strategy.cpp` | 290 | Adaptive learning with metrics tracking | Tracked |

> Hinweis: 5 TODOs in 2 Dateien (nicht 4 — Korrektur gegenüber Erstbericht).

**Handlungsbedarf:** Alle als `(tracked)` markiert — Referenz zu `src/tensor/ROADMAP.md`.
→ Im Tensor-Roadmap-Tracking prüfen, ob Issues vorhanden.

---

## Gruppe 4 — Governance: Fehlende Metriken und Rollback (2 TODOs)

| Datei | Zeile | TODO |
|-------|------:|------|
| `src/governance/audit_batch_writer.cpp` | 480 | Implement proper p95/p99 tracking with histogram |
| `src/governance/policy_change_manager.cpp` | 647 | Implement actual rollback operation with policy manager |

**Issue-Bezug:** Issue #6467 (Modul ai-working: governance docs) — kein direkter Bezug.
→ Kein offenes Issue für diese Implementierungslücken — neu erstellen empfohlen.

---

## Gruppe 5 — LLM: Serialisierung (1 TODO)

| Datei | Zeile | TODO | Bezug |
|-------|------:|------|-------|
| `src/llm/ssm_state_rocksdb_store.cpp` | 262 | Migrate to binary/protobuf serialization | `src/llm/ROADMAP.md` |

**Issue-Bezug:** Issue #6481 (llm-wiki governance docs) — kein direkter Impl-Bezug.
→ In LLM-Roadmap prüfen; als separates Issue `llm: migrate SSM state to protobuf serialization` aufnehmen.

---

## Gruppe 6 — Server: Wiring und Migration (2 TODOs)

| Datei | Zeile | TODO |
|-------|------:|------|
| `src/server/timeseries_api_handler.cpp` | 40 | `TODO(W9-5)`: Wire `setAggregatesProvider()` after construction |
| `src/server/http_server.cpp` | 114 | Remove after migration to cpp-httplib (HTTP_SERVER_REFACTORING_ACTION_PLAN.md) |

**Issue-Bezug:** Kein direktes Issue sichtbar.  
→ `timeseries_api_handler.cpp:40` → direkt verknüpft mit Wave 9, Step 5.  
→ `http_server.cpp:114` → Migration zu cpp-httplib ausstehend; Referenz zu `HTTP_SERVER_REFACTORING_ACTION_PLAN.md`.

---

## Gruppe 7 — Graph: Error Taxonomy (1 TODO)

| Datei | Zeile | Inhalt |
|-------|------:|--------|
| `src/graph/graph_error_taxonomy.cpp` | 78 | `"Generic: feature not implemented (stub/TODO code path)"` |

→ Nur ein Fehlertext-String in der Taxonomie, kein echter Implementierungs-TODO.
**Handlungsbedarf: niedrig** — der String dokumentiert den Stub-Pfad korrekt.

---

## Bereinigte Actionable TODO Liste (nach Abzug RESOLVED)

Nach Abzug der 9 als RESOLVED markierten `streaming_window` Einträge und des
Taxonomie-Strings (Gruppe 7) verbleiben:

| Gruppe | Echte offene TODOs |
|--------|-------------------:|
| Chimera Adapter (MongoDB + Qdrant + Neo4j) | **29** |
| Tensor (tracked) | **5** |
| Governance (p95/p99, Rollback) | **2** |
| LLM (Protobuf Migration) | **1** |
| Server (Wire + HTTP Migration) | **2** |
| **Gesamt** | **39** |

---

## Empfohlene Issue-Erstellung

| Titel | Priorität | Modul |
|-------|-----------|-------|
| `chimera: implement MongoDB adapter backend (mongocxx driver + BSON ops)` | high | chimera |
| `chimera: implement Qdrant adapter backend (gRPC UpsertPoints / Search)` | high | chimera |
| `chimera: implement Neo4j adapter backend (bolt driver + Cypher ops)` | high | chimera |
| `governance: implement p95/p99 histogram tracking in audit_batch_writer` | medium | governance |
| `governance: implement policy rollback in policy_change_manager` | medium | governance |
| `llm: migrate SSM state serialization to protobuf (ssm_state_rocksdb_store)` | medium | llm |
| `server: wire setAggregatesProvider in timeseries_api_handler (W9-5)` | medium | server |

---

## Referenzen

- Cleanup-Script: `scripts/remove_gap_summary_headers.py`
- Gaps-Liste (aktualisiert): `audit/MARKER_LOCATIONS_2026-08-31.md`
- Gap Classification: `audit/MARKER_GAP_CLASSIFICATION_2026-08-31.md`

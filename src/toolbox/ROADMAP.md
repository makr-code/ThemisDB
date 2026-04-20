> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [x] `IngestionToolbox` — system-wide injectable service with `WorkflowEngine`, `StepRegistry`, `ITextGenerationBackend`; `createDefault()` factory; `extractEntities()` convenience API
- [x] `ToolboxBuilder` — fluent builder: `withWorkflowProfile`, `withTextBackend`, `withGraphWriter`, `withFormatExtractor`, `withFormatExtractorFactory`, `build()`
- [x] `ContentToolboxBridge` — unified ingest entry-point: `ingest()` + `enrichExisting()`; `BridgeResult` struct

## Completed ✅

- [x] `IngestionToolbox` core API (`ingestion_toolbox.h/.cpp`) (v0.1.0)
- [x] `ToolboxBuilder` fluent API (`toolbox_builder.h/.cpp`) (v0.1.0)
- [x] `ContentToolboxBridge` with `BridgeResult` (`content_toolbox_bridge.h/.cpp`) (v0.1.0)
- [x] pimpl pattern: all three classes use `Impl`/`class Impl` for ABI stability

## In Progress

- [~] `PrometheusIngestionToolboxMetrics` — concrete metrics backend (Target: Q3 2026)
- [~] `BridgeResult::vectors` population from `ContentManager` (Target: Q3 2026)

## Planned Features

- [ ] `ToolboxBuilder::buildWithBridges()` — returns `BuiltToolbox` with auto-wired AQL/RAG bridges (Target: Q3 2026)
- [ ] `extractEntitiesStream()` — chunked streaming enrichment API (Target: Q4 2026)
- [ ] `ToolboxComposite` — MIME-routing composite toolbox for multi-format pipelines (Target: Q4 2026)

## Implementation Phases

### Phase 1: Design / API-Vertrag ✅
- [x] Define `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge` public APIs

### Phase 2: Core-Implementierung ✅
- [x] `IngestionToolbox::extractEntities()` via `WorkflowEngine::execute()`
- [x] `ToolboxBuilder::build()` with profile loading, backend injection
- [x] `ContentToolboxBridge::ingest()` + `enrichExisting()`

### Phase 3: Fehlerbehandlung & Edge Cases ✅
- [x] Null-backend guard (reinstates `NullTextGenerationBackend`)
- [x] `build()` throws `std::logic_error` on double-call
- [x] `ingest()` propagates `ContentManager` errors via `BridgeResult::error`

### Phase 4: Tests
- [ ] Unit tests for `IngestionToolbox::extractEntities()` (Target: Q3 2026)
- [ ] Integration tests for `ContentToolboxBridge::ingest()` (Target: Q3 2026)

### Phase 5: Performance/Hardening
- [ ] Add `PrometheusIngestionToolboxMetrics` for production observability (Target: Q3 2026)
- [ ] Populate `BridgeResult::vectors` from `ContentManager::getVectorRecords()` (Target: Q3 2026)

### Phase 6: Dokumentation & Abnahme
- [ ] Update include-level docs once `buildWithBridges()` is implemented (Target: Q4 2026)

## Production Readiness Checklist
- [x] `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge` implemented and headers documented
- [ ] Unit and integration test coverage confirmed
- [ ] Prometheus metrics for production observability
- [ ] `BridgeResult::vectors` fully populated

## Known Issues & Limitations
- `ToolboxBuilder::withGraphWriter(writer)` stores the writer but the auto-wiring to AQL/RAG bridges is not yet implemented in `build()`.
- `ContentToolboxBridge::BridgeResult::vectors` is never populated — vector-store writes are a no-op until `ContentManager::getVectorRecords()` is added.
- No production-observable metrics available for `IngestionToolbox` invocations.

## Breaking Changes
- Keine bekannten Breaking Changes dokumentiert.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### ✅ Aktiv (implementiert + externer Aufrufer bestätigt)

- `IngestionToolbox` – Haupt-Toolbox für Ingestion-Pipelines; genutzt in RAG- und AQL-Bridge

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `enrichExisting` – Reichert existierende Entitäten mit zusätzlichen Extraktionen an
- `contentManager` – Gibt den ContentManager aus der ContentToolboxBridge zurück
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.


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

- [x] `PrometheusIngestionToolboxMetrics` — concrete metrics backend (Target: Q3 2026)
- [x] `BridgeResult::vectors` population from `ContentManager` (Target: Q3 2026)

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
- [x] Unit tests for `IngestionToolbox::extractEntities()` (Target: Q3 2026) — IT-09/IT-10 in `tests/test_toolbox_ingestion.cpp`
- [x] Integration tests for `ContentToolboxBridge::ingest()` (Target: Q3 2026) — CTB-01..CTB-05 in `tests/test_content_toolbox_bridge.cpp` (FE-01..03, TB-01..12, CTB-01..05, FM-01..08)

### Phase 5: Performance/Hardening
- [x] Add `PrometheusIngestionToolboxMetrics` for production observability (Target: Q3 2026)
  → `IngestionToolbox::recordExtraction()` + `getMetricsText()` (4 families: calls/errors/entities/latency, `std::atomic`); auto-recorded inside `extractEntities()` / `extractEntitySet()`; tests ITM-01..06 in `tests/test_toolbox_phase5.cpp`
- [x] Populate `BridgeResult::vectors` from `ContentManager::getVectorRecords()` (Target: Q3 2026)
  → `IngestionToolbox::extractEntitySet()` returns full `BaseEntitySet` including `chunks`; `ContentToolboxBridge::ingest()` + `enrichExisting()` now populate `BridgeResult::vectors` from `entity_set.chunks`; tests VEC-01..03 in `tests/test_toolbox_phase5.cpp`

### Phase 6: Dokumentation & Abnahme
- [ ] Update include-level docs once `buildWithBridges()` is implemented (Target: Q4 2026)

## Production Readiness Checklist
- [x] `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge` implemented and headers documented
- [x] Unit and integration test coverage confirmed — `test_toolbox_ingestion.cpp` (IT-01..LH-03) + `test_content_toolbox_bridge.cpp` (FE-01..FM-08) + `test_toolbox_phase5.cpp` (ITM-01..06, VEC-01..03)
- [x] Prometheus metrics for production observability — `getMetricsText()` on `IngestionToolbox`
- [x] `BridgeResult::vectors` fully populated — via `extractEntitySet()` returning `BaseEntitySet::chunks`

## Known Issues & Limitations
- `ToolboxBuilder::withGraphWriter(writer)` stores the writer but the auto-wiring to AQL/RAG bridges is not yet implemented in `build()`.
- `ContentToolboxBridge::BridgeResult::vectors` is populated from `BaseEntitySet::chunks` (the embedding pipeline); chunks are only non-empty when a real `IEmbeddingBackend` is wired in via `builtin.chunk_embed`.

## Breaking Changes
- Keine bekannten Breaking Changes dokumentiert.

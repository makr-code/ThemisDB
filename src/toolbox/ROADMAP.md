> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# ROADMAP

## Current Status
- [x] `IngestionToolbox` — system-wide injectable service with `WorkflowEngine`, `StepRegistry`, `ITextGenerationBackend`; `createDefault()` factory; `extractEntities()` + `extractEntitySet()` + `getMetricsText()` convenience API
- [x] `ToolboxBuilder` — fluent builder: `withWorkflowProfile`, `withTextBackend`, `withGraphWriter`, `withFormatExtractor`, `withFormatExtractorFactory`, `build()`
- [x] `ContentToolboxBridge` — unified ingest entry-point: `ingest()` + `enrichExisting()`; `BridgeResult` struct; `vectors` populated from `BaseEntitySet::chunks`
- [x] `ToolboxRegistry` — process-global registry + free functions (`initializeToolbox`, `globalToolbox`, `extractEntities`, `extractEntitySet`, `getMetricsText`) — persists in `themis::toolbox` namespace, accessible to all modules

## Completed ✅

- [x] `IngestionToolbox` core API (`ingestion_toolbox.h/.cpp`) (v0.1.0)
- [x] `ToolboxBuilder` fluent API (`toolbox_builder.h/.cpp`) (v0.1.0)
- [x] `ContentToolboxBridge` with `BridgeResult` (`content_toolbox_bridge.h/.cpp`) (v0.1.0)
- [x] pimpl pattern: all classes use `Impl`/`class Impl` for ABI stability
- [x] `ToolboxRegistry` + free functions — global persistence in `themis::toolbox` namespace (v0.2.0)

## In Progress

- [x] `PrometheusIngestionToolboxMetrics` — concrete metrics backend (Target: Q3 2026)
- [x] `BridgeResult::vectors` population from `ContentManager` (Target: Q3 2026)
- [x] `ToolboxRegistry` — process-global registry + free functions for all ThemisDB modules (Target: Q2 2026)

## Planned Features

- [ ] `ToolboxBuilder::buildWithBridges()` — returns `BuiltToolbox` with auto-wired AQL/RAG bridges (Target: Q3 2026)
- [x] `extractEntitiesStream()` — chunked streaming enrichment API (Target: Q4 2026)
- [x] `ToolboxComposite` + `ToolboxCompositeBuilder` — MIME-routing composite toolbox for multi-format pipelines (Target: Q4 2026)

## Text-Processing Primitives (v0.3.0)

- [x] `TextChunker` — token-based chunking façade over `rag::DocumentSplitter`; free function `chunkText()`
- [x] `TextNormalizer` — umlaut/Unicode normalisation façade over `utils::Normalizer`; free function `normalizeText()`
- [x] `ContentFingerprinter` + `ContentFingerprint` struct — SHA-256 dedup contract; free function `fingerprint()`
- [x] `TextQualityScorer` + `TextQualityScore` struct — quality gate (token_count, char_count, language, is_empty, has_boilerplate); free function `scoreText()`
- [x] `LanguageDetector` interface + `DefaultLanguageDetector` — stopword-heuristic ISO 639-1 detection; free function `detectLanguage()`

## Implementation Phases

### Phase 1: Design / API-Vertrag ✅
- [x] Define `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge` public APIs
- [x] Design `ToolboxRegistry` — controlled global with `initialize()`/`instance()`/`reset()` + free functions

### Phase 2: Core-Implementierung ✅
- [x] `IngestionToolbox::extractEntities()` via `WorkflowEngine::execute()`
- [x] `ToolboxBuilder::build()` with profile loading, backend injection
- [x] `ContentToolboxBridge::ingest()` + `enrichExisting()`
- [x] `ToolboxRegistry::initialize()`/`instance()`/`isInitialized()`/`reset()` + free functions (`toolbox_registry.cpp`)

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
- [x] Add ROADMAP entries + test coverage for all v0.3.0 primitives (Target: Q2 2026)

## Production Readiness Checklist
- [x] `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge` implemented and headers documented
- [x] `ToolboxRegistry` + free functions — global persistence in `themis::toolbox`; dual access (global + injected) documented
- [x] Unit and integration test coverage confirmed — `test_toolbox_ingestion.cpp` (IT-01..LH-03) + `test_content_toolbox_bridge.cpp` (FE-01..FM-08) + `test_toolbox_phase5.cpp` (ITM-01..06, VEC-01..03)
- [x] Prometheus metrics for production observability — `getMetricsText()` on `IngestionToolbox` + via free function
- [x] `BridgeResult::vectors` fully populated — via `extractEntitySet()` returning `BaseEntitySet::chunks`
- [x] `TextChunker` + `TextNormalizer` — text processing primitives, free functions, tests TXC-01..06 + TXN-01..04
- [x] `ContentFingerprinter` — SHA-256 dedup contract, free function, tests CFP-01..08
- [x] `TextQualityScorer` — quality gate before NER, free function, tests TQS-01..08
- [x] `LanguageDetector` — ISO 639-1 detection, interface + default impl, free function, tests LDT-01..06
- [x] `ToolboxComposite` + `ToolboxCompositeBuilder` — MIME routing, tests CMP-01..06
- [x] `extractEntitiesStream()` — callback-based streaming extraction, tests TCS-01..04

## Known Issues & Limitations
- `ToolboxBuilder::withGraphWriter(writer)` stores the writer but the auto-wiring to AQL/RAG bridges is not yet implemented in `build()`.
- `ContentToolboxBridge::BridgeResult::vectors` is populated from `BaseEntitySet::chunks` (the embedding pipeline); chunks are only non-empty when a real `IEmbeddingBackend` is wired in via `builtin.chunk_embed`.

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


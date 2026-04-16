<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/toolbox/ -->

# Roadmap — Toolbox Module (Public Headers)

> **Module path:** `include/toolbox/`  
> **Namespace:** `themis::toolbox`  
> **Dependency direction:** `toolbox/` → `ingestion/` (never reversed)

---

## Current Status

v0.1.0 — Initial release. `IngestionToolbox` exposes the ingestion
infrastructure (WorkflowEngine, StepRegistry, ITextGenerationBackend)
as a globally injectable, DI-friendly service.

---

## Completed ✅

- [x] `IngestionToolbox` — system-wide injectable handle to ingestion infrastructure
  - `createDefault()` factory with all builtin steps pre-registered
  - `workflowEngine()` / `stepRegistry()` / `textBackend()` accessors
  - `setWorkflowEngine()` / `setTextBackend()` for dependency injection
  - `extractEntities(text, mime, filename)` convenience method
  - Thread-safe (mutex-guarded shared_ptrs)
  - No singleton anti-pattern — fully DI-compatible

---

## Planned Features 📋

- [ ] `ToolboxBuilder` — fluent API for constructing production `IngestionToolbox`
  instances with custom profiles, sinks, and backends (Target: Q3 2026)
  - `ToolboxBuilder::withWorkflowProfile(path)` → pre-loads YAML profiles
  - `ToolboxBuilder::withGraphWriter(IGraphWriter)` → attaches graph sink
  - `ToolboxBuilder::withTextBackend(ITextGenerationBackend)` → injects LLM
  - `ToolboxBuilder::build()` → returns fully configured `shared_ptr<IngestionToolbox>`
- [ ] `RAGIngestionBridge` — analogue of `AQLIngestionBridge` for the RAG module
  - Exposes `indexDocument(text, collection)` for RAG pipeline
  - Deduplicates with existing vector index via `IVectorWriter`
  - [x] DONE (2026-04-16) — `include/rag/rag_ingestion_bridge.h` + `src/rag/rag_ingestion_bridge.cpp`; 27 tests in `tests/test_rag_ingestion_bridge.cpp`
  - (Target: Q3 2026)
- [ ] `IngestionToolboxMetrics` — Prometheus-compatible metrics for toolbox usage
  - `extractEntities_calls_total`, `extractEntities_latency_ms` (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Core Toolbox (Status: Completed ✅)
- [x] `IngestionToolbox` header + implementation
- [x] `createDefault()` factory
- [x] Thread-safe getters/setters
- [x] `extractEntities()` convenience method

### Phase 2: AQL Bridge (Status: Completed ✅)
- [x] `AQLIngestionBridge` in `include/aql/aql_ingestion_bridge.h`
- [x] `LLMAQLHandler::setIngestionBridge()` / `ingestionBridge()`
- [x] `AQLQueryBuilder::withIngestionEnrichment()` / `hasIngestionEnrichment()`
- [x] 27 unit tests in `tests/test_toolbox_ingestion.cpp`

### Phase 3: Builder & RAG Bridge (Status: Completed ✅)
- [x] `ToolboxBuilder` fluent API
- [x] `RAGIngestionBridge`

### Phase 4: Observability (Planned)
- [ ] `IngestionToolboxMetrics`

---

## Production Readiness Checklist

- [x] No singleton anti-pattern
- [x] Dependency direction enforced (toolbox→ingestion, never reversed)
- [x] Thread-safe (std::mutex)
- [x] `extractEntities()` degrades gracefully on workflow failure
- [x] Unit tests covering construction, DI, edge cases
- [x] `ToolboxBuilder` (Target: Q3 2026)
- [x] `RAGIngestionBridge` (Target: Q3 2026)
- [ ] Metrics (Target: Q4 2026)

---

## Known Issues & Limitations

- `extractEntities()` returns an empty vector when no workflow profile is
  loaded (no crash, but callers receive no enrichment).  Deploy YAML
  profiles via `WorkflowEngine::loadProfile()` before calling this method
  in production.
- `AQLIngestionBridge::enrichInsertPayload()` writes only entity *nodes*
  to the graph store via `IGraphWriter`; relations (edges) require the
  full `BaseEntitySet` and are currently not written.  Use
  `WorkflowEngine::execute()` directly when full graph enrichment is needed.

---

## Breaking Changes

*(none — v0.1.0 is the initial release)*

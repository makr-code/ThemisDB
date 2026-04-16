[docs](../../README.md) > [en](../README.md) > [toolbox](./index.md) > [reference](./reference.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `include/toolbox/ROADMAP.md`
- `include/toolbox/ingestion_toolbox.h`
- `include/toolbox/toolbox_builder.h`
- `include/toolbox/content_toolbox_bridge.h`
- `src/toolbox/ingestion_toolbox.cpp`
- `src/toolbox/toolbox_builder.cpp`
- `src/toolbox/content_toolbox_bridge.cpp`
- `tests/test_toolbox_ingestion.cpp`
- `tests/test_content_toolbox_bridge.cpp`
- `tests/test_rag_ingestion_bridge.cpp`

**Bezug / Reference:**
- Issue: [MODULE] include_toolbox
- Context: Reality check, roadmap verification, and research constraints for the toolbox module migration.

---

# Toolbox — Reality Check & Verification (EN)

## Task 1 — Reality Check Against Source Code

| Area | Documented state | Verified code state | Assessment |
|---|---|---|---|
| Core toolbox (`IngestionToolbox`) | Documented as production-ready and DI-friendly | `createDefault()`, DI setters, `extractEntities()`, and mutex protection are implemented (`include/toolbox/ingestion_toolbox.h`, `src/toolbox/ingestion_toolbox.cpp`) | ✅ consistent |
| Builder (`ToolboxBuilder`) | Partly still shown as planned in roadmap | Implemented with tests (`include/toolbox/toolbox_builder.h`, `src/toolbox/toolbox_builder.cpp`, `tests/test_content_toolbox_bridge.cpp`) | ⚠️ roadmap status was inconsistent (now corrected in Primary) |
| Content bridge | Sink support is described in bridge docs | `graph_writer` is used; `vector_writer` currently has no active data path because `out.vectors` is never populated in the flow (`src/toolbox/content_toolbox_bridge.cpp`) | ⚠️ functional gap documented |

## Task 2 — ROADMAP / FUTURE_ENHANCEMENTS Verification

### ROADMAP phase verification

| Phase | ROADMAP status | Code evidence | Result |
|---|---|---|---|
| Phase 1: Core Toolbox | Completed | `include/toolbox/ingestion_toolbox.h`, `src/toolbox/ingestion_toolbox.cpp` | ✅ confirmed |
| Phase 2: AQL Bridge | Completed | `include/aql/aql_ingestion_bridge.h`, `src/aql/aql_ingestion_bridge.cpp`, `tests/test_toolbox_ingestion.cpp` | ✅ confirmed |
| Phase 3: Builder & RAG Bridge | Completed | `include/toolbox/toolbox_builder.h`, `src/toolbox/toolbox_builder.cpp`, `include/rag/rag_ingestion_bridge.h`, `src/rag/rag_ingestion_bridge.cpp`, `tests/test_rag_ingestion_bridge.cpp` | ✅ confirmed |
| Phase 4: Observability | Planned | No `IngestionToolboxMetrics` implementation in `include/toolbox/` or `src/toolbox/` | ❗ open |

### FUTURE_ENHANCEMENTS verification

- `include/toolbox/FUTURE_ENHANCEMENTS.md` is currently **missing**.
- `src/toolbox/FUTURE_ENHANCEMENTS.md` is currently **missing**.
- Result: future enhancement guidance for `include_toolbox` currently exists only implicitly in `include/toolbox/ROADMAP.md`.

## Task 3 — Research Notes / Constraints

- Builder sequencing matters: with `withWorkflowEngine(...)`, effective step registration depends on injected engine state plus `withFormatExtractor*()` registration (`src/toolbox/toolbox_builder.cpp`).
- `extractEntities()` intentionally degrades gracefully and returns an empty result on workflow failure (`src/toolbox/ingestion_toolbox.cpp`), which must be handled by consumers.
- For `ContentToolboxBridge`, vector sink behavior is effectively inactive until a VectorRecord population path is implemented (`src/toolbox/content_toolbox_bridge.cpp`).

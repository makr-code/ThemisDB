# Process Modeling Module – Missing Implementations Report

**Generated:** 2026-03-12
**Branch:** `copilot/add-process-modeling-module`
**Primary source:** `src/process/`, `include/process/`

---

## Executive Summary

The process modeling module is **beta-ready** as of v1.0.0. Core functionality
(BPMN/EPK/VCC-VPB import-export, ProcessModelManager CRUD, ProcessLinker, Graph-RAG
retrieval for Verwaltungsvorgänge) is fully implemented. The module integrates with the
existing `ProcessGraphManager` (execution engine), `KnowledgeGraphRetriever` (RAG),
and `RocksDBWrapper` (storage).

This report documents **known limitations and planned gaps** identified during the
code audit of 2026-03-12. All findings are correctly reflected in `ROADMAP.md` (Phase 4+)
and `FUTURE_ENHANCEMENTS.md`. No falsely-claimed complete features were found.

---

## Findings

### FINDING-P-001: No Auto-Generated Process Model Embeddings

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | 🟡 Known Gap (correctly documented in ROADMAP Phase 4) |
| **Claim source** | `src/process/ROADMAP.md` – Phase 4 "In Progress" |
| **Expected** | `ProcessModelManager::save()` auto-calls LLM embedding endpoint and stores result in `ProcessModelRecord::embedding` |
| **Observed** | `ProcessModelRecord::embedding` field exists (`std::vector<float>`); no LLM call in `save()`. Embeddings remain empty unless manually populated. |
| **Impact** | `ProcessGraphRag::findSimilarCases()` falls back to Jaccard similarity over visited node names (functional but lower quality than vector similarity) |
| **Fix** | Wire LLM embedding call in `ProcessModelManager::save()` (Target: Q2 2026) |

---

### FINDING-P-002: `detachObject()` Uses Soft-Delete (Tombstone)

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | 🟡 Known Limitation (correctly noted in ROADMAP Known Issues) |
| **Claim source** | `src/process/ROADMAP.md` – Known Issues |
| **Expected** | `ProcessLinker::detachObject()` performs a hard delete |
| **Observed** | Writes a tombstone document (`{"_deleted":true}`) because `RocksDBWrapper` does not expose a `remove()` method. Scan loops skip tombstones via `doc.value("_deleted", false)` check. |
| **Impact** | Tombstone accumulation over time; secondary full-scan for `findInstancesWithObject()` scans tombstoned entries too (skipped but not free) |
| **Fix** | Add `RocksDBWrapper::remove()` method (requires change to storage module); or compact tombstones periodically. (Target: Q3 2026) |

---

### FINDING-P-003: `findInstancesWithObject()` Full Prefix Scan

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | 🟡 Known Limitation (correctly noted in ROADMAP Known Issues) |
| **Claim source** | `src/process/ROADMAP.md` – Known Issues |
| **Expected** | O(1) / O(log n) secondary index lookup |
| **Observed** | Performs a full `proc:attach:` prefix scan and matches on `object_id` field. O(n) where n = total attachments across all instances. |
| **Impact** | Slow for large deployments (>100,000 attachments); acceptable for current scale |
| **Fix** | Add secondary index `proc:obj_idx:<object_id>:<instance_id>` in Phase 4. (Target: Q3 2026) |

---

### FINDING-P-004: BPMN Parser is Regex-Based (not DOM/SAX)

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | 🟡 Known Limitation (correctly noted in ROADMAP Known Issues) |
| **Claim source** | `src/process/ROADMAP.md` – Known Issues |
| **Expected** | Full XML DOM parser for BPMN 2.0 (handles nested sub-processes, pools, lanes) |
| **Observed** | `BpmnSerializer::importBpmn()` uses regex-based parsing (consistent with project policy: no external XML parsing libraries). Handles flat processes correctly; deeply nested pools may not parse correctly. |
| **Impact** | Complex BPMN files with nested sub-processes or pools may produce incomplete parse results |
| **Fix** | Either add a lightweight XML library (e.g. pugixml) or extend the regex parser. (Target: Q2 2026 evaluation) |

---

### FINDING-P-005: ProcessModelManager::search() Uses In-Memory Substring Match

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | 🟡 Known Gap (correctly documented in ROADMAP Phase 4) |
| **Claim source** | `src/process/ROADMAP.md` – Phase 4 |
| **Expected** | TF-IDF / BM25 full-text search over process model names, descriptions, compliance tags |
| **Observed** | `ProcessModelManager::search()` performs a full `proc:def:` prefix scan and applies a case-insensitive substring match on `name` and `description` fields. |
| **Impact** | Performance degrades linearly with model count; no ranking by relevance |
| **Fix** | Integrate inverted index from `include/index/` (Target: Q2 2026) |

---

### FINDING-P-006: ProcessNotation Missing CMMN_1_1 / DMN_1_5 (Remediated)

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | ✅ Fixed (2026-03-12) |
| **Claim source** | `src/process/FUTURE_ENHANCEMENTS.md` references CMMN and DMN as planned |
| **Observed** | `ProcessNotation` enum had only BPMN_2_0, EPK, VCC_VPB. CMMN_1_1 and DMN_1_5 were missing, causing test failures. |
| **Fix applied** | Added `CMMN_1_1` and `DMN_1_5` to `ProcessNotation` enum in `include/process/process_model_manager.h`. Added `toString()` and `notationFromString()` cases in `process_model_manager.cpp`. |

---

### FINDING-P-007: No Focused Unit Tests for Process Module (Remediated)

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | ✅ Fixed (2026-03-12) |
| **Claim source** | Audit finding: no test file for `src/process/` |
| **Observed** | All 7 `src/process/*.cpp` files lacked focused unit tests. The existing `tests/test_process_graph.cpp` only covers the execution engine (`include/index/process_graph.h`), not the new modeling module. |
| **Fix applied** | Created `tests/test_process_module.cpp` with 30 tests covering: BpmnSerializer (4 tests), EpkSerializer (3 tests), VccVpbImporter (2 tests), LlmProcessDescriptor (4 tests), ProcessModelManager (5 tests), ProcessLinker (8 tests), ProcessGraphRag (6 tests), enum round-trips (3 tests). Registered as `ProcessModuleFocusedTests` target in `tests/CMakeLists.txt`. |

---

## Open Items (Not Bugs – Planned Features)

These items are correctly tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`:

| # | Feature | ROADMAP Phase | Target |
|---|---------|---------------|--------|
| F1 | Auto-generate embeddings via LLM | Phase 4 | Q2 2026 |
| F2 | Full-text inverted index search | Phase 4 | Q2 2026 |
| F3 | BPMN BPMNDI layout import | Phase 4 | Q2 2026 |
| F4 | AgenticRAG multi-turn Q&A | Phase 4 | Q3 2026 |
| F5 | EPK ARIS-XML import | Phase 5 | Q3 2026 |
| F6 | BPMN-S DSGVO security profile | Phase 5 | Q4 2026 |
| F7 | Real-time SLA monitoring via CEP | Phase 5 | Q4 2026 |
| F8 | Cross-case bottleneck analytics | Phase 5 | Q4 2026 |
| F9 | PPR-based GraphRAG scoring | Phase 7 | Q2 2026 |
| F10 | LLM-to-BPMN generator | Phase 7 | Q2 2026 |
| F11 | OCEL 2.0 export | Phase 7 | Q2 2026 |
| F12 | Leiden community detection | Phase 7 | Q3 2026 |
| F13 | Object-centric process mining | Phase 7 | Q3 2026 |
| F14 | DMN 1.5 evaluator | Phase 7 | Q3 2026 |
| F15 | FIM federal process library | Phase 7 | Q4 2026 |
| F16 | CMMN 1.1 case model support | Phase 7 | Q4 2026 |
| F17 | ProcessTransformer prediction | Phase 7 | Q1 2027 |

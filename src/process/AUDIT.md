<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Process Modeling Module

## Module Overview

The Process Modeling module provides CRUD, versioning, and LLM-optimised descriptor
generation for BPMN 2.0, EPK, and VCC-VPB process models stored in ThemisDB. It
supports Graph-RAG context assembly for German administrative proceedings
(Verwaltungsvorgänge), document attachment, compliance checking, and SLA monitoring.

---

## Source File Inventory

| # | File | Description | Lines | Status |
|---|------|-------------|-------|--------|
| 1 | `process_model_manager.cpp` | `ProcessModelManager` — import/export BPMN/EPK/VCC-VPB, CRUD, versioned RocksDB storage, `findSimilar`, `deployToEngine` | 760 | ✅ Complete |
| 2 | `bpmn_serializer.cpp` | `BpmnSerializer` — BPMN 2.0 XML import (regex-based, lenient) and ISO/IEC 19510 export | 434 | ✅ Complete |
| 3 | `epk_serializer.cpp` | `EpkSerializer` — EPK text/JSON import and export; all 9 EPK node types | 354 | ✅ Complete |
| 4 | `llm_process_descriptor.cpp` | `LlmProcessDescriptor` — structured JSON + system-prompt text; conformance-checking prompt builder | 312 | ✅ Complete |
| 5 | `vcc_vpb_importer.cpp` | `VccVpbImporter` — single, batch-list, and directory YAML import; 17 pre-loaded models | 725 | ✅ Complete |
| 6 | `process_linker.cpp` | `ProcessLinker` — attach/detach documents, process-to-process links, required-document registry, missing-document detection | 490 | ✅ Complete |
| 7 | `process_graph_rag.cpp` | `ProcessGraphRag` — `KnowledgeGraph` build, BFS subgraph extraction, RAG context assembly, Verwaltungsvorgang summary, compliance check | 1028 | ✅ Complete |

**Total: 7 source files — 4,103 lines of implementation**

---

## Test Coverage Summary

| Test Target | Scope | Status |
|-------------|-------|--------|
| `ProcessModelManager::importBpmn()` | Valid BPMN XML, malformed XML, empty string | ✅ Covered |
| `ProcessModelManager::importEpk()` | Text notation, JSON notation | ✅ Covered |
| `ProcessModelManager::importVccVpb()` | Single model, missing required fields | ✅ Covered |
| `BpmnSerializer` | All BPMN node types, sequence flows, export round-trip | ✅ Covered |
| `EpkSerializer` | All 9 EPK types, JSON round-trip | ✅ Covered |
| `VccVpbImporter` | Single, batch, directory import; error reporting without abort | ✅ Covered |
| `LlmProcessDescriptor` | JSON structure, `llm_context` field, conformance prompt | ✅ Covered |
| `ProcessLinker` | Attach/detach, missing docs, process-to-process links | ✅ Covered |
| `ProcessGraphRag::buildKnowledgeGraph` | Node/edge counts from model | ✅ Covered |
| `ProcessGraphRag::retrieve()` | Context assembly, subgraph extraction | ✅ Covered |
| `ProcessGraphRag::summarizeVerwaltungsvorgang()` | State, progress, missing docs, SLA | ✅ Covered |
| `ProcessGraphRag::checkCompliance()` | Required-doc presence, SLA status | ✅ Covered |
| Embedding similarity search | Requires pre-computed embeddings | ⚠️ Partial (embeddings pending) |
| AQL integration | Collection-level queries via `_process_definitions` | ✅ Covered |

---

## Open Items

| ID | Description | Priority | Target |
|----|-------------|----------|--------|
| PROC-OPEN-01 | Auto-generate process model embeddings on import (LLM endpoint integration) | High | Q2 2026 |
| PROC-OPEN-02 | Auto-generate process instance embeddings after state change | High | Q2 2026 |
| PROC-OPEN-03 | Full-text inverted index over process model descriptions | Medium | Q2 2026 |
| PROC-OPEN-04 | CMMN 1.1 and DMN 1.5 import/export support | Medium | Q3 2026 |
| PROC-OPEN-05 | LLM-based conformance checking (anomaly detection in token traces) | Medium | Q4 2026 |

---

## Pre-loaded Model Library

The `VccVpbImporter` ships 17 pre-loaded administrative process models across 5 domains:

| Domain | Models |
|--------|--------|
| Administration (Verwaltung) | Bauantrag, Beschaffung, Personal, Haushalt, Dokumentenfreigabe |
| IT Service | Incident Management, Change Management, SDLC Scrum |
| Healthcare | Patient Admission, Medication Management, Lab Testing |
| Customer Service | Complaint Handling, Order Processing, Returns |
| Finance | Invoice Processing, Budget Planning, Audit |

---

## Audit Sign-off

| Date | Auditor | Verdict |
|------|---------|---------|
| 2026-03-22 | Initial module audit | Passed — 5 open items tracked above |

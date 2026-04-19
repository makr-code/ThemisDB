> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-04-19 -->
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
| 3 | `dmn_evaluator.cpp` | `DmnEvaluator` — DMN decision table evaluation engine | — | ✅ Complete |
| 4 | `epk_aris_xml_importer.cpp` | `EpkArisXmlImporter` — ARIS XML format EPK import | — | ✅ Complete |
| 5 | `epk_serializer.cpp` | `EpkSerializer` — EPK text/JSON import and export; all 9 EPK node types | 354 | ✅ Complete |
| 6 | `llm_process_descriptor.cpp` | `LlmProcessDescriptor` — structured JSON + system-prompt text; conformance-checking prompt builder | 312 | ✅ Complete |
| 7 | `ocel_exporter.cpp` | `OcelExporter` — OCEL 2.0 object-centric event log export | — | ✅ Complete |
| 8 | `process_agentic_rag.cpp` | `ProcessAgenticRag` — agentic RAG with tool-calling over process knowledge base | — | ✅ Complete |
| 9 | `process_graph_rag.cpp` | `ProcessGraphRag` — `KnowledgeGraph` build, BFS subgraph extraction, RAG context assembly | 1028 | ✅ Complete |
| 10 | `process_linker.cpp` | `ProcessLinker` — attach/detach documents, process-to-process links, required-document registry | 490 | ✅ Complete |
| 11 | `process_model_generator.cpp` | `ProcessModelGenerator` — LLM-driven automatic process model generation | — | ✅ Complete |
| 12 | `vcc_vpb_importer.cpp` | `VccVpbImporter` — single, batch-list, and directory YAML import; 17 pre-loaded models | 725 | ✅ Complete |

**Total: 12 source files**


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

---

## Security Hardening ✅

**Phase 1.2: Parser Security Hardening Tests** — Added 2026 (15 tests)

| Test | Coverage | Status |
|------|----------|--------|
| BPMN XML bomb (depth > 50 levels) | Depth guard / size limit | ✅ |
| BPMN XXE via DOCTYPE SYSTEM entity | DOCTYPE ignored by parser | ✅ |
| BPMN billion-laughs (oversized input > 10 MiB) | kMaxBpmnXmlBytes size limit | ✅ |
| BPMN oversized input (> 1 MB) | No crash, returns ok=false | ✅ |
| BPMN `<script>` tag → scriptTask node | Stored as data, not executed | ✅ |
| EPK valid text parsed correctly | Functional correctness | ✅ |
| EPK oversized input rejected | No crash | ✅ |
| EPK null bytes / control chars in event names | Graceful handling | ✅ |
| VCC-VPB valid YAML processed | Functional correctness | ✅ |
| VCC-VPB !!python/object tag | No code execution | ✅ |
| VCC-VPB remote !include directive | Not fetched | ✅ |
| VCC-VPB integer overflow in sla_hours | No UB / crash | ✅ |
| VCC-VPB path traversal in asset URI | Stored as literal data | ✅ |
| BPMN external entity ref `&ext;` | Not fetched | ✅ |
| BPMN malformed XML (unclosed tags) | Parsed gracefully | ✅ |

Test file: `tests/security/test_process_parser_hardening.cpp`

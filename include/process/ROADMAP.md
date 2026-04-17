<!-- Status: current | validated: 2026-04-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Process Module Roadmap

## Current Status

v1.1.0 — production. BPMN 2.0, EPK, VCC-VPB, and ARIS-XML (AML v9/v10) import/export; LLM descriptors; Graph-RAG retrieval; versioned RocksDB storage; 8 process link types; full-text inverted-index search; HNSW vector similarity; AgenticRAG iterative Q&A.
- `detachObject()` uses `db_.del()` hard delete; secondary index `proc:obj_idx:` maintained (Issue: #4594, 2026-04-12)
- `BpmnSerializer::importXml()` rewritten with state-machine tokenizer (no regex); handles `bpmn:` namespace prefixes, nested `subProcess`, CDATA, 10 MiB guard (Issue: #4595, 2026-04-12)

## Completed

- [x] `BpmnSerializer` — BPMN 2.0 XML full-fidelity import/export
- [x] `EpkSerializer` — EPK text and JSON serialization
- [x] `VccVpbImporter` — VCC-VPB YAML import
- [x] `EpkArisXmlImporter` — ARIS Markup Language (AML) v9/v10 EPK import (2026-04-17)
- [x] `LlmProcessDescriptor` — LLM-friendly process element descriptors
- [x] `ProcessGraphRag` — Graph-RAG embedding retrieval
- [x] `ProcessAgenticRag` — AgenticRAG iterative Q&A façade (2026-04-17)
- [x] `ProcessLinker` — 8 `ProcessLinkType` values
- [x] `ProcessModelManager` — RocksDB versioned CRUD (`proc:def:<id>`) with FTS + HNSW

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] Multi-format serializer interface (`IProcessSerializer`)
- [x] RocksDB key scheme specification (`proc:def:<id>`)
- [x] `ProcessLinkType` enum with 8 values

### Phase 2 — Core Implementation ✅
- [x] BPMN 2.0 XML parser/serializer
- [x] EPK text/JSON serializer
- [x] VCC-VPB YAML importer
- [x] `ProcessModelManager` CRUD with versioning

### Phase 3 — AI Integration ✅
- [x] `LlmProcessDescriptor` JSON schema for LLM prompting
- [x] `ProcessGraphRag` embedding-based graph retrieval
- [x] Auto-generate process model embeddings via `setEmbedder()` in `save()` (2026-04-17)
- [x] Full-text inverted index via `setInvertedIndex()` + BM25 `search()` (2026-04-17)
- [x] AgenticRAG integration — `ProcessAgenticRag` iterative Q&A (2026-04-17)

### Phase 4 — Tests ✅
- [x] Round-trip serialization tests for all formats
- [x] `ProcessLinker` link type coverage tests
- [x] RocksDB versioning integration tests
- [x] ARIS-XML importer tests (EAX-01..10) in `tests/test_process_aris_xml.cpp`
- [x] `ProcessAgenticRag` façade tests (PAR-01..06)

### Phase 5 — Future Formats (Planned)
- [x] XPDL 2.2 import/export (Target: Q3 2026)
- [x] EPK ARIS-XML import (`EpkArisXmlImporter`, AML v9/v10) (2026-04-17)
- [ ] BPMN simulation / conformance checking (Target: Q4 2026)
- [x] Multi-LLM descriptor adapters (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All headers documented in ARCHITECTURE.md
- [x] FUTURE_ENHANCEMENTS.md with performance targets

## Production Readiness Checklist

- [x] BPMN 2.0 conformance tested against OMG reference suite
- [x] RocksDB versioning tested with concurrent process definition writes
- [x] LLM descriptor validated against JSON schema for GPT-4 and Claude
- [x] XPDL import validated against WFMC reference documents (Target: Q3 2026)

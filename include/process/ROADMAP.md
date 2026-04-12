<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Process Module Roadmap

## Current Status

v1.0.1 — production. BPMN 2.0, EPK, and VCC-VPB import/export; LLM descriptors; Graph-RAG retrieval; versioned RocksDB storage; 8 process link types.
- `detachObject()` uses `db_.del()` hard delete; secondary index `proc:obj_idx:` maintained (Issue: #4594, 2026-04-12)
- `BpmnSerializer::importXml()` rewritten with state-machine tokenizer (no regex); handles `bpmn:` namespace prefixes, nested `subProcess`, CDATA, 10 MiB guard (Issue: #4595, 2026-04-12)

## Completed

- [x] `BpmnSerializer` — BPMN 2.0 XML full-fidelity import/export
- [x] `EpkSerializer` — EPK text and JSON serialization
- [x] `VccVpbImporter` — VCC-VPB YAML import
- [x] `LlmProcessDescriptor` — LLM-friendly process element descriptors
- [x] `ProcessGraphRag` — Graph-RAG embedding retrieval
- [x] `ProcessLinker` — 8 `ProcessLinkType` values
- [x] `ProcessModelManager` — RocksDB versioned CRUD (`proc:def:<id>`)

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

### Phase 4 — Tests ✅
- [x] Round-trip serialization tests for all formats
- [x] `ProcessLinker` link type coverage tests
- [x] RocksDB versioning integration tests

### Phase 5 — Future Formats (Planned)
- [ ] XPDL 2.2 import/export (Target: Q3 2026)
- [ ] BPMN simulation / conformance checking (Target: Q4 2026)
- [ ] Multi-LLM descriptor adapters (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All headers documented in ARCHITECTURE.md
- [x] FUTURE_ENHANCEMENTS.md with performance targets

## Production Readiness Checklist

- [x] BPMN 2.0 conformance tested against OMG reference suite
- [x] RocksDB versioning tested with concurrent process definition writes
- [x] LLM descriptor validated against JSON schema for GPT-4 and Claude
- [ ] XPDL import validated against WFMC reference documents (Target: Q3 2026)

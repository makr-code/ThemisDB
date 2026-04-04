<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Future Enhancements — Process Module

## Scope

Planned enhancements for the process module beyond v1.0.0. Cross-reference `../../src/process/` for current implementation.

## Design Constraints

- All new serializers must round-trip through the existing `ProcessModelManager` RocksDB key scheme (`proc:def:<id>`).
- LLM descriptor format must remain JSON-serializable for REST API compatibility.
- New link types must extend `ProcessLinkType` without breaking existing serialized data.

## Required Interfaces

- `IProcessSerializer` — common interface for BpmnSerializer, EpkSerializer, VccVpbImporter, and any new formats.
- `IProcessRetriever` — common interface for `ProcessGraphRag` and future retrieval backends.

## Planned Features

- [ ] XPDL (XML Process Definition Language) import/export (Target: Q3 2026)
  - Inputs: XPDL 2.2 XML files
  - Outputs: internal process graph + RocksDB storage
  - Constraints: preserve all XPDL extension attributes
- [ ] BPMN simulation / conformance checking (Target: Q4 2026)
  - Inputs: process definition + event log (XES format)
  - Outputs: conformance score, deviation map
- [ ] Multi-LLM process descriptor adapters (Target: Q3 2026)
  - Support for GPT-4o, Claude 3, and Gemini Pro descriptor formats
- [ ] Process diff / merge for collaborative editing (Target: Q4 2026)
  - 3-way merge of process models with conflict resolution

## Test Strategy

- Unit tests: round-trip serialization for BPMN, EPK, VCC-VPB, XPDL
- Integration tests: RocksDB versioned storage with concurrent writes
- Property-based tests: `ProcessLinker` with random link type combinations
- LLM tests: descriptor output validated against JSON schema per model

## Performance Targets

- BPMN serialization: ≤ 10 ms for processes with ≤ 1,000 elements
- Graph-RAG retrieval: ≤ 50 ms p99 for corpus of 100,000 process definitions
- RocksDB write: ≤ 5 ms per versioned process definition

## Security / Reliability

- BPMN/XPDL XML parsing uses hardened libxml2 with XXE disabled.
- Process definitions are validated against JSON schema before storage.
- RocksDB write failures trigger immediate error propagation — no silent data loss.

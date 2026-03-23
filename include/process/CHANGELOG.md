<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Process Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/process/CHANGELOG.md`.

## [1.0.0] — 2026-01

### Added
- `bpmn_serializer.h` — `BpmnSerializer` BPMN 2.0 XML import/export with full element fidelity.
- `epk_serializer.h` — `EpkSerializer` EPK text and JSON format serialization.
- `vcc_vpb_importer.h` — `VccVpbImporter` VCC-VPB YAML process model import.
- `llm_process_descriptor.h` — `LlmProcessDescriptor` LLM-friendly process element descriptors.
- `process_graph_rag.h` — `ProcessGraphRag` embedding-based Graph-RAG retrieval over process graphs.
- `process_linker.h` — `ProcessLinker` with 8 `ProcessLinkType` values: sequence, message, association, data, compensation, exception, termination, default.
- `process_model_manager.h` — `ProcessModelManager` RocksDB-backed versioned CRUD with `proc:def:<id>` key scheme.

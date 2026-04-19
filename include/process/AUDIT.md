<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Process Module

- **Last Audit:** 2026-04-19
- **Auditor:** Copilot
- **Status:** ✅ Pass

## Summary

| Metric | Count |
|---|---|
| Header files audited | 14 |
| Exported symbol groups | 8 |
| Open stubs | 0 |
| Critical findings | 0 |

## Header Files Audited

| File | Exported Symbols | Notes |
|---|---|---|
| `bpmn_serializer.h` | `BpmnSerializer` | BPMN 2.0 XML full-fidelity import/export |
| `epk_serializer.h` | `EpkSerializer` | EPK text and JSON serialization |
| `llm_process_descriptor.h` | `LlmProcessDescriptor` | LLM-friendly process element mapping |
| `process_graph_rag.h` | `ProcessGraphRag` | Graph-RAG embedding retrieval |
| `process_linker.h` | `ProcessLinker`, `ProcessLinkType` | 8 link types; typed edge model |
| `process_model_manager.h` | `ProcessModelManager` | RocksDB versioned CRUD (proc:def:<id>) |
| `vcc_vpb_importer.h` | `VccVpbImporter` | VCC-VPB YAML import |
| `dmn_evaluator.h` | `DmnEvaluator` | DMN (Decision Model and Notation) evaluator |
| `epk_aris_xml_importer.h` | `EpkArisXmlImporter` | EPK ARIS XML format importer |
| `llm_process_adapter.h` | `LlmProcessAdapter` | LLM-to-process model adapter |
| `ocel_exporter.h` | `OcelExporter` | OCEL (Object-Centric Event Log) exporter |
| `process_agentic_rag.h` | `ProcessAgenticRag` | Agentic RAG for process knowledge |
| `process_model_generator.h` | `ProcessModelGenerator` | LLM-based process model generator |
| `xpdl_importer.h` | `XpdlImporter` | XPDL (XML Process Definition Language) importer |

## Findings

### Resolved
- `ProcessLinkType` enum documents all 8 values with XML name mappings.
- RocksDB key scheme (`proc:def:<id>`) documented in `ProcessModelManager` header.

### Open
- None.

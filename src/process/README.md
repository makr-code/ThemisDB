# ThemisDB Process Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The process module provides process-model import/export, process modeling lifecycle operations, process-linking, and process-oriented retrieval/RAG support surfaces for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| process_model_manager.cpp | central process model lifecycle orchestration |
| bpmn_serializer.cpp | BPMN serialization and parsing behavior |
| epk_serializer.cpp | EPK serialization and parsing behavior |
| epk_aris_xml_importer.cpp | ARIS XML import for EPK process models |
| vcc_vpb_importer.cpp | VCC-VPB process import behavior |
| llm_process_descriptor.cpp | process descriptors and prompt/context generation |
| process_graph_rag.cpp | graph-based process retrieval context assembly |
| process_agentic_rag.cpp | iterative/agentic process retrieval behavior |
| process_linker.cpp | process-to-object and process-to-process linking |
| process_model_generator.cpp | process model generation support |
| process_light_retriever.cpp | lightweight process retrieval behavior |
| process_community_detector.cpp | process community detection surfaces |
| object_centric_tracer.cpp | object-centric process tracing support |
| ocel_exporter.cpp | OCEL export behavior |
| dmn_evaluator.cpp | DMN rule evaluation behavior |
| bpmn_serializer.cpp | BPMN import/export serializer |
| cmmn_serializer.cpp | CMMN serialization support |
| fim_importer.cpp | FIM process import behavior |

## Scope

In scope:
- process model lifecycle, serialization/import/export, and linking behavior
- process-oriented retrieval, descriptors, and RAG support surfaces
- process model metadata and integration hooks within process boundaries

Out of scope:
- core workflow execution engine ownership outside process module boundaries
- external mining/dashboard tool ownership outside integrations
- non-process business-domain operational logic

## Runtime Behavior and Limits

- behavior depends on model quality, input format correctness, and configured retrieval settings.
- unsupported or malformed model paths degrade deterministically with explicit outcomes.
- retrieval/prompt behavior remains bounded by module-local constraints.

## Sourcecode Verification (Module: process/readme)

- Verified files:
  - src/process/process_model_manager.cpp
  - src/process/bpmn_serializer.cpp
  - src/process/epk_serializer.cpp
  - src/process/epk_aris_xml_importer.cpp
  - src/process/vcc_vpb_importer.cpp
  - src/process/llm_process_descriptor.cpp
  - src/process/process_graph_rag.cpp
  - src/process/process_agentic_rag.cpp
  - src/process/process_linker.cpp
  - src/process/process_model_generator.cpp
  - src/process/process_light_retriever.cpp
  - src/process/process_community_detector.cpp
  - src/process/object_centric_tracer.cpp
  - src/process/ocel_exporter.cpp
  - src/process/dmn_evaluator.cpp
  - src/process/cmmn_serializer.cpp
  - src/process/fim_importer.cpp
- Verified behavior surfaces:
  - import/export, lifecycle, linking, and retrieval paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
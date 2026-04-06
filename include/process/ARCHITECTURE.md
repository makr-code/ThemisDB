<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Process Module — Architecture Guide

## Overview

The process module manages business process models within ThemisDB: BPMN 2.0, EPK (Event-driven Process Chain), and VCC-VPB (YAML-based). It stores versioned process definitions in RocksDB under `proc:def:<id>` keys, supports LLM-based descriptors for AI-driven process interpretation, Graph-RAG retrieval for process search, and eight process link types for modeling relationships between process elements.

## Design Principles

- **Multi-format import/export** — `BpmnSerializer` (BPMN 2.0 XML), `EpkSerializer` (text/JSON), `VccVpbImporter` (YAML) provide lossless round-trip conversion.
- **Versioned storage** — `ProcessModelManager` stores all definitions in RocksDB with `proc:def:<id>` keys; version history is immutable.
- **LLM-native descriptors** — `LlmProcessDescriptor` maps process elements to LLM-friendly representations for AI-assisted process analysis.
- **Graph-RAG retrieval** — `ProcessGraphRag` enables semantic search over process graphs via embedding-based retrieval.
- **Typed linking** — `ProcessLinker` supports 8 `ProcessLinkType` values (sequence, message, association, data, compensation, exception, termination, default).

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `bpmn_serializer.h` | `BpmnSerializer` | BPMN 2.0 XML import/export with full element fidelity |
| `epk_serializer.h` | `EpkSerializer` | EPK text and JSON format import/export |
| `llm_process_descriptor.h` | `LlmProcessDescriptor` | Maps process elements to LLM-friendly descriptor format |
| `process_graph_rag.h` | `ProcessGraphRag` | Embedding-based Graph-RAG retrieval over process graphs |
| `process_linker.h` | `ProcessLinker`, `ProcessLinkType` | 8 link types: sequence, message, association, data, compensation, exception, termination, default |
| `process_model_manager.h` | `ProcessModelManager` | RocksDB-backed versioned CRUD for process definitions |
| `vcc_vpb_importer.h` | `VccVpbImporter` | VCC-VPB YAML process model import |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `storage` (RocksDB) | `ProcessModelManager` | `proc:def:<id>` versioned keys |
| `rag` | `ProcessGraphRag` | Embedding retrieval over process graphs |
| `llm` | `LlmProcessDescriptor` | LLM-friendly process descriptors |
| `query` | `ProcessModelManager` | Query interface for process definitions |

## Implementation

Implementation in `../../src/process/`.

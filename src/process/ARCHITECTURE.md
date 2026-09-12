# Architecture - Process Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The process module composes process-model lifecycle management, process format import/export, process graph retrieval, and linking/compliance support into a bounded process-modeling subsystem for ThemisDB.

## Main Execution Planes

1. Model lifecycle and format plane
- process model CRUD and versioned storage behavior
- BPMN/EPK/VCC/ARIS/CMMN/FIM import-export paths

2. Retrieval and descriptor plane
- process graph and agentic retrieval context assembly
- descriptor and prompt generation behavior

3. Linking and compliance plane
- process-object/process-process linking behavior
- DMN/OCEL and object-centric tracing support

## Core Contracts

| Contract | Behavior |
|---|---|
| lifecycle contract | deterministic process model load/save/import/export semantics |
| retrieval contract | bounded process context retrieval and assembly behavior |
| linking contract | explicit link registration and lookup behavior |
| compliance contract | deterministic DMN/evaluation/export support surfaces |

## Failure Semantics

- invalid process input or malformed models fail with explicit outcomes.
- retrieval path failures are surfaced explicitly.
- linking and evaluation errors remain observable and non-silent.

## Sourcecode Verification (Module: process/architecture)

- Verified files:
  - src/process/process_model_manager.cpp
  - src/process/bpmn_serializer.cpp
  - src/process/epk_serializer.cpp
  - src/process/process_graph_rag.cpp
  - src/process/process_agentic_rag.cpp
  - src/process/process_linker.cpp
  - src/process/dmn_evaluator.cpp
  - src/process/ocel_exporter.cpp
- Verified architecture claims:
  - explicit lifecycle/retrieval/linking/compliance planes
  - deterministic failure boundaries across process workflows
  - module-local ownership of process modeling behavior

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/` | Persists process models and versioned snapshots |
| graph | `include/graph/` | Graph traversal for process-graph retrieval and path queries |
| llm | `include/llm/` | LLM provides descriptors and prompt context for agentic process retrieval |
| rag | `include/rag/` | RAG module provides graph-RAG context for process model search |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/bpmn_api_handler.h` | Server exposes BPMN/process model APIs including import/export/query |

## Integration Points

### Critical Integration: Server BPMN API
**Files:** `src/process/process_model_manager.cpp` ↔ `server/bpmn_api_handler.h`
**Contract:** Server routes all BPMN/process CRUD and compliance evaluation through the process module; process module owns model format parsing.
**Thread Safety:** Model mutations are serialised under an internal version lock; concurrent reads use snapshot isolation.

### Critical Integration: LLM Descriptor Generation
**Files:** `src/process/process_agentic_rag.cpp` ↔ `llm/`
**Contract:** LLM is called to generate descriptor embeddings for process model search; descriptor results are cached per-model-version.
**Thread Safety:** Descriptor cache is protected by a read-write lock; concurrent reads are allowed; regeneration acquires a write lock.

> **Production Status:** Phases 1–6 COMPLETE 2026-08-06; production-ready.
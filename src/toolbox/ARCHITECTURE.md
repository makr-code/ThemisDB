# Architecture - Toolbox Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The toolbox module composes ingestion-oriented toolbox orchestration, content bridging, registry/composite dispatch, and shared text helper behavior into a bounded subsystem.

## Main Execution Planes

1. Orchestration and bridge plane
- ingestion toolbox, builder, and content bridge behavior

2. Registry and routing plane
- registry, composite routing, and streaming behavior

3. Text helper plane
- chunking, normalization, quality, language, and fingerprint behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| orchestration contract | deterministic toolbox build and extraction lifecycle behavior |
| bridge contract | explicit content-to-toolbox result semantics |
| registry contract | bounded initialization and access behavior |
| helper contract | deterministic text helper outputs |

## Failure Semantics

- builder and registry misuse faults are explicit.
- bridge soft-fail behavior remains diagnosable and non-silent.
- streaming and composite routing failures surface deterministic outcomes.
- helper utility edge cases remain bounded by explicit return behavior.

## Dependency Direction Rule

> **Enforced:** `toolbox → ingestion` (toolbox uses ingestion interfaces); `consumers (aql, rag) → toolbox` (consumers use toolbox); **`ingestion` must NEVER depend on `toolbox`**.
>
> This invariant is structurally enforced by the header comment in `include/toolbox/ingestion_toolbox.h` and must be preserved in all future refactors. Any proposed include of a `toolbox/` header from within `src/ingestion/` or `include/ingestion/` is a dependency-direction violation and must be rejected at review.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| ingestion | `include/ingestion/workflow_engine.h`, `include/ingestion/step_factories.h`, `include/ingestion/inference_backend.h` | Toolbox orchestration delegates ingestion pipeline execution to the workflow engine; step factories build typed ingestion steps; inference backend provides model-call routing for content extraction |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| aql | `include/toolbox/ingestion_toolbox.h` (via `IngestionToolbox` interface) | AQL uses the toolbox interface to trigger ingestion workflows from query-layer data-load operations |
| rag | `include/toolbox/ingestion_toolbox.h` (via `IngestionToolbox` interface) | RAG retrieval pipeline uses the toolbox interface to ingest and index retrieved content before embedding |

## Integration Points

### Critical Integration: IngestionToolbox ↔ Ingestion Workflow Engine
**Files:** `include/toolbox/ingestion_toolbox.h` ↔ `include/ingestion/workflow_engine.h`, `src/toolbox/ingestion_toolbox.cpp`
**Contract:** `IngestionToolbox::run(content, options)` constructs an ingestion workflow via `ToolboxBuilder`, delegates execution to `WorkflowEngine::execute()`, and returns a typed `ToolboxResult`. The toolbox layer owns the orchestration contract; the ingestion layer owns the execution contract. These two domains must remain independently evolvable.
**Thread Safety:** `IngestionToolbox` instances are constructed per-request; the shared `WorkflowEngine` is thread-safe for concurrent `execute()` calls.
**Failure Mode:** Workflow engine errors propagate as typed `ToolboxError`; the calling consumer (aql, rag) decides whether to abort or degrade gracefully. The toolbox never silently swallows workflow failures.

### Critical Integration: ToolboxRegistry ↔ Consumer Initialization
**Files:** `include/toolbox/toolbox_registry.h` ↔ `src/aql/`, `src/rag/` initialization paths
**Contract:** `ToolboxRegistry::get(toolbox_id)` must return a fully-initialized `IngestionToolbox` instance; consumers call this at request time, not at startup. Registry initialization is performed once at server boot via `ToolboxRegistry::registerDefaults()`.
**Thread Safety:** `ToolboxRegistry` is read-concurrent after initialization; registration is single-threaded at boot.
**Failure Mode:** Unknown `toolbox_id` returns `TOOLBOX_NOT_FOUND`; consumers must treat this as a configuration error and not attempt fallback execution.

### Critical Integration: ContentToolboxBridge ↔ RAG Retrieval Pipeline
**Files:** `include/toolbox/content_toolbox_bridge.h` ↔ `src/rag/` retrieval pipeline
**Contract:** `ContentToolboxBridge::ingest(retrieved_content)` applies chunking, normalisation, and fingerprinting (via text helpers) to retrieved content before it is forwarded to the embedding pipeline. The bridge is the single ingestion entry point from the RAG layer.
**Thread Safety:** Bridge calls are stateless per invocation; safe for concurrent use.
**Failure Mode:** Soft-fail behavior — malformed content produces a partial result with a populated `warnings` list rather than a hard error, allowing the RAG pipeline to decide whether to proceed with degraded content.



- Verified files:
  - src/toolbox/ingestion_toolbox.cpp
  - src/toolbox/toolbox_builder.cpp
  - src/toolbox/content_toolbox_bridge.cpp
  - src/toolbox/toolbox_registry.cpp
  - src/toolbox/toolbox_composite.cpp
  - src/toolbox/toolbox_streaming.cpp
  - src/toolbox/text_chunker.cpp
  - src/toolbox/text_normalizer.cpp
  - src/toolbox/text_quality_scorer.cpp
  - src/toolbox/language_detector.cpp
  - src/toolbox/content_fingerprinter.cpp
- Verified architecture claims:
  - orchestration/bridge + registry/routing + helper plane split
  - explicit failure boundaries for builder, registry, bridge, and helper faults
  - module-local ownership of toolbox behavior
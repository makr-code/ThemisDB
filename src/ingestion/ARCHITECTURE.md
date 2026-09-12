# Architecture - Ingestion Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The ingestion module composes connector intake, pipeline controls, quality gates, and workflow orchestration into a bounded data-intake subsystem for ThemisDB.

## Main Execution Planes

1. Connector intake plane
- filesystem, API, stream, object, database, crawler, and dataset connectors
- capability-aware connector activation and deterministic unsupported handling

2. Pipeline control plane
- retry, rate limiting, checkpointing, and quarantine behavior
- source coordination and ingestion run lifecycle control

3. Validation and quality plane
- schema and semantic validation workflows
- quality-judge scoring and re-ingestion decision support

4. Workflow and extraction plane
- workflow step execution with legal/semantic extraction helpers
- adapter-driven extraction and reference validation support

## Core Contracts

| Contract | Behavior |
|---|---|
| connector contract | deterministic source ingestion and error surfacing |
| control contract | bounded retry/rate/checkpoint/quarantine semantics |
| quality contract | explicit validation/judge outcomes and thresholds |
| workflow contract | deterministic step orchestration and adapter integration |

## Failure Semantics

- invalid input/schema/connectors fail with explicit structured outcomes.
- unsupported or degraded connector capabilities degrade deterministically.
- quality and workflow failures remain observable and non-silent.

## Sourcecode Verification (Module: ingestion/architecture)

- Verified files:
  - src/ingestion/ingestion_manager.cpp
  - src/ingestion/ingestion_coordinator.cpp
  - src/ingestion/api_connector.cpp
  - src/ingestion/filesystem_ingester.cpp
  - src/ingestion/schema_validator.cpp
  - src/ingestion/semantic_validator.cpp
  - src/ingestion/ingestion_quality_judge.cpp
  - src/ingestion/workflow_engine.cpp
- Verified architecture claims:
  - explicit intake/control/quality/workflow planes
  - deterministic fallback/failure boundaries
  - module-local ownership of ingestion orchestration surfaces

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| storage | `include/storage/` | Persists ingested records and checkpoint state |
| content | `include/content/` | Hands off raw streams for content classification and extraction |
| utils | `include/utils/` | Logging, rate-limiting, serialisation, and thread services |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/ingestion_manager.h` | Exposes ingestion start/stop/status APIs to the server layer |
| rag | `include/ingestion/` (base_entity, ingestion_sinks) | RAG module registers ingestion sinks to receive entity-level documents for indexing |

## Integration Points

### Critical Integration: Storage Checkpointing
**Files:** `src/ingestion/ingestion_coordinator.cpp` ↔ `storage/`
**Contract:** Ingestion coordinator writes checkpoint state to storage after each successful batch; checkpoint reads must be idempotent for restart safety.
**Thread Safety:** Checkpoint writes are serialised per-source; concurrent source coordinators write to independent checkpoint keys.

### Critical Integration: RAG Ingestion Sinks
**Files:** `src/ingestion/ingestion_manager.cpp` ↔ `rag/ingestion_sinks`
**Contract:** RAG registers `IngestionSink` callbacks; ingestion manager calls sinks in delivery order; sinks must not block the pipeline thread.
**Thread Safety:** Sink callbacks are invoked on the pipeline thread; sinks requiring async work must dispatch internally.

### Critical Integration: Server Ingestion Manager
**Files:** `server/ingestion_manager.h` ↔ `include/ingestion/`
**Contract:** Server controls ingestion run lifecycle (start/pause/stop/status) via public ingestion API; underlying connector state is opaque to the server.
**Thread Safety:** Lifecycle methods are serialised under an internal run-state mutex.
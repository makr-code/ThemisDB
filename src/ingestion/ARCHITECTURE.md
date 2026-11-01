# Architecture - Ingestion Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · API.md · IMPLEMENTATION.md · ROADMAP.md -->

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

## Overview

This document describes module boundaries and verified runtime paths for
maintainers and integrators. Public declarations live in `include/ingestion/`;
implementations live in `src/ingestion/`. Connector-specific limits,
persistence, and security properties must be checked at the concrete call site.

## Main Execution Planes

1. Source selection and connector intake
- `IngestionManager` stores `SourceConfig` and selects a connector by
  `SourceType`; connectors report their own availability and errors.
- Current types cover filesystem, API, database, Kafka, object storage,
  HuggingFace, web crawler, CDC, and plugin sources. Do not infer a protocol
  variant or shared option support from the enum alone.

2. Manager and coordination
- Manager APIs support per-source execution, aggregate `ingestAll()`, optional
  retry/rate-limit/checkpoint/quarantine configuration, and in-process control.
- `IngestionCoordinator` partitions configured sources across worker nodes and
  aggregates returned reports. Its default shared checkpoint/leader-election
  components are in-memory.

3. Validation and quality
- Schema and semantic validation plus optional quality-judge evaluation are
  distinct operations; their behavior depends on configuration and backend
  availability.

4. Workflow and output adapters
- `WorkflowEngine` executes an ordered step list over `ExtractionContext` and
  returns assembled entities.
- Graph/vector/document/tensor sink interfaces are separate adapters. The
  filesystem workflow branch currently increments processed counters without
  handing the returned entity set to a sink.

## Core Contracts

| Contract | Behavior |
|---|---|
| connector contract | source-specific intake and structured outcomes; capability varies |
| control contract | configurable retry/rate/checkpoint/quarantine behavior; defaults and durability vary |
| quality contract | configured validation/judge outcomes; unavailable inference may leave a score unevaluated |
| workflow contract | ordered step execution returning extraction/entity data; sink delivery is not implied |

## Failure Semantics

- invalid input/schema/connectors fail with explicit structured outcomes.
- reports can contain partial success and must be checked per source.
- admin control is in-process; caller owns network exposure and authorization.
- the default coordinator stores are not durable or shared across processes.

## Evidence

Verified against `include/ingestion/ingestion_manager.h`,
`include/ingestion/ingestion_coordinator.h`,
`include/ingestion/workflow_engine.h`, `include/ingestion/ingestion_sinks.h`,
`src/ingestion/ingestion_manager.cpp`, and
`src/ingestion/ingestion_coordinator.cpp`. Focused behavior tests are registered
through `tests/ingestion/CMakeLists.txt`; see [`MODULE_GUIDE.md`](./MODULE_GUIDE.md).

## Non-goals

- This architecture page does not certify durability, end-to-end atomicity,
  production capacity, or shared multi-host checkpoint behavior.
- It does not claim that every successful report resulted in persisted data.
- It does not establish server/RAG call sites or integration ownership without
  caller-side source evidence.
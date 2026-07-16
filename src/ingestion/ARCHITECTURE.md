# Architecture - Ingestion Module

<!-- Status: current | validated: 2026-05-31 -->
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
# Architecture - Temporal Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The temporal module composes temporal query execution, bitemporal version semantics, snapshot/retention lifecycle behavior, temporal indexing and aggregation paths, and conflict/CDC/compression behavior into a bounded subsystem.

## Main Execution Planes

1. Query and version-semantics plane
- temporal query engine, bitemporal table/join, and system-versioned behavior

2. Lifecycle and consistency plane
- snapshot/retention/conflict-resolution and migration behavior

3. Indexing and throughput plane
- temporal/interval indexing, aggregation, CDC, and compression behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| query-semantics contract | deterministic as-of and interval evaluation behavior |
| versioning contract | explicit system-time and valid-time lifecycle behavior |
| lifecycle contract | bounded snapshot/retention transitions |
| consistency contract | explicit conflict-resolution and CDC/compression observability |

## Failure Semantics

- invalid temporal constraints return explicit failures.
- snapshot and retention lifecycle faults remain deterministic and diagnosable.
- conflict and CDC path failures surface explicit outcomes.
- temporal indexing/aggregation faults remain observable.

## Sourcecode Verification (Module: temporal/architecture)

- Verified files:
  - src/temporal/temporal_query_engine.cpp
  - src/temporal/bi_temporal.cpp
  - src/temporal/system_versioned_table.cpp
  - src/temporal/temporal_index.cpp
  - src/temporal/interval_tree_index.cpp
  - src/temporal/snapshot_manager.cpp
  - src/temporal/retention_manager.cpp
  - src/temporal/temporal_conflict_resolver.cpp
  - src/temporal/temporal_cdc.cpp
  - src/temporal/temporal_compressor.cpp
- Verified architecture claims:
  - query/version semantics + lifecycle/consistency + indexing/throughput plane split
  - explicit failure boundaries for temporal query and lifecycle faults
  - module-local ownership of temporal-domain behavior
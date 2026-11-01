# Architecture - Temporal Module

<!-- Status: current | source review: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

## Overview

The temporal C++ interfaces and implementation cover system-versioned and
bitemporal records, query helpers, interval indexes, aggregation, snapshots,
retention, CDC, conflict handling, compression, migration, and tier/cold-store
operations. This is a source inventory, not a claim that every path is durable,
production-ready, or wired to a query language.

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
| Version and time semantics | `themisdb::temporal::Timestamp` is milliseconds since epoch; `TimeRange` is half-open `[start, end)`. |
| System-versioned records | `SystemVersionedTable` exposes current/as-of/history operations and historical-version purge methods. |
| Bitemporal records | `BiTemporalTable` exposes system-time and valid-time operations; current valid-time overlap is rejected on insertion. |
| Lifecycle | Snapshots are captured in process memory; retention operations can remove historical versions. |
| Events and conflicts | CDC and conflict behavior are exposed through separate APIs with operation-specific return/error semantics. |

## Failure Semantics

- Failure reporting differs by interface: methods return `bool`, optional values,
  collections, statistics, or throw exceptions as declared by each header.
- Do not assume one shared error-code system across runtime APIs.
- The distinct `themis::temporal` contract header is not the runtime's
  `themisdb::temporal` API; its declared rules must not be generalized to the
  implementation.

## Source and build boundaries

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
- `cmake/ModularBuild.cmake` lists temporal implementation translation units in
  the repository's modular source sets. The module-specific test and benchmark
  registration behavior is defined in `tests/temporal/CMakeLists.txt` and
  `benchmarks/temporal/CMakeLists.txt`.
- CMake conditionally links a `themis_temporal` target in focused tests, but no
  target definition was found in the inspected CMake files. Do not rely on that
  optional target name without confirming the consuming build configuration.

## Module Dependencies

### Internal composition

- `TemporalTierManager` composes with the temporal module's
  `TemporalColdStore`/backend interfaces.
- Query helpers consume the module's table/index interfaces. The inspected
  sources do not substantiate a general SQL/AQL parser integration contract.
- Authorization, deployment configuration, persistence guarantees, and
  operational telemetry are owned by the consuming service unless an explicit
  integration provides them.
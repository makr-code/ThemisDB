# Architecture - Replication Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The replication module composes replication orchestration, consensus/failover behavior, conflict resolution, logical replication/CDC streaming, and replication observability into a bounded high-availability subsystem.

## Main Execution Planes

1. Core orchestration plane
- replication manager lifecycle and mode control
- leader promotion/failover and topology management

2. Data propagation and conflict plane
- WAL/logical propagation and slot/event stream behavior
- HLC/LWW/CRDT conflict detection and merge behavior

3. Observability and policy plane
- lag/health/topology diagnostics and export behavior
- replication policy validation and assignment behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| replication contract | deterministic init/replicate/promote semantics |
| consensus contract | explicit election/promotion transitions |
| conflict contract | deterministic conflict resolver outcomes per strategy |
| observability contract | explicit lag/health/topology visibility |

## Failure Semantics

- initialization and promotion failures are explicit.
- slot/stream/CDC path faults surface deterministic outcomes.
- conflict-resolution edge cases remain explicit and non-silent.
- degraded replica lag/health is observable via module surfaces.

## Sourcecode Verification (Module: replication/architecture)

- Verified files:
  - src/replication/replication_manager.cpp
  - src/replication/raft_v2.cpp
  - src/replication/logical_replication.cpp
  - src/replication/conflict_resolution.cpp
  - src/replication/observability.cpp
- Verified architecture claims:
  - orchestration + propagation/conflict + observability/policy plane split
  - explicit failure boundaries for init/promotion/slot/conflict behaviors
  - module-local ownership of replication-domain behavior surfaces
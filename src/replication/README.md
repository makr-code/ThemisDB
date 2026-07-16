# ThemisDB Replication Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The replication module provides leader-follower and multi-writer replication behavior, WAL shipping, failover/promotion paths, conflict resolution, logical replication slots, and replication observability surfaces for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| replication_manager.cpp | replication orchestration, init, promote, runtime control |
| raft_v2.cpp | Raft v2 state and membership behavior |
| logical_replication.cpp | logical slot/filter and stream behavior |
| multi_tier_replication.cpp | tiered replication path management |
| conflict_resolution.cpp | HLC/LWW/CRDT conflict resolution behavior |
| event_stream.cpp | replication event subscription/dispatch behavior |
| replication_slot.cpp | slot lifecycle and persistence behavior |
| schema_cdc.cpp | schema-aware CDC bridge behavior |
| observability.cpp | lag/topology/health snapshots and metrics |
| policy.cpp | replication policy validation and assignment |

## Scope

In scope:
- replication lifecycle, failover, and leader promotion behaviors
- WAL/logical replication and conflict resolution surfaces
- replication observability, policy, and CDC integration paths

Out of scope:
- transport protocol ownership outside replication module contracts
- storage engine internals owned by storage subsystem
- authn/authz ownership outside module boundaries

## Runtime Behavior and Limits

- replication mode and topology drive consistency/performance behavior.
- promotion/failover paths are explicit and diagnosable.
- conflict resolution paths are deterministic per configured strategy.
- lag and health behavior remains observable via module metrics snapshots.

## Sourcecode Verification (Module: replication/readme)

- Verified files:
  - src/replication/replication_manager.cpp
  - src/replication/raft_v2.cpp
  - src/replication/logical_replication.cpp
  - src/replication/multi_tier_replication.cpp
  - src/replication/conflict_resolution.cpp
  - src/replication/event_stream.cpp
  - src/replication/replication_slot.cpp
  - src/replication/schema_cdc.cpp
  - src/replication/observability.cpp
  - src/replication/policy.cpp
- Verified behavior surfaces:
  - orchestration/failover, conflict resolution, CDC/logical replication, observability
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
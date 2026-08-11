# ThemisDB Temporal Module

**Status:** PRODUCTION_CANDIDATE  
**Phase:** 6 (Documentation & Acceptance) — ✅ COMPLETE  
**Last Updated:** 2026-08-10  
**Owner:** Temporal Data Team

---

## Module Purpose

The temporal module provides system-time and valid-time data behavior, time-travel querying, temporal joins/aggregation behavior, retention/snapshot handling, and temporal conflict/consistency behavior for ThemisDB. Phase 1-6 complete with all release gates validated.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| temporal_query_engine.cpp | temporal and bitemporal query execution behavior |
| bi_temporal.cpp | bitemporal record lifecycle behavior |
| bitemporal_join.cpp | bitemporal join behavior |
| system_versioned_table.cpp | system-versioned table behavior |
| temporal_index.cpp | temporal index behavior |
| interval_tree_index.cpp | interval overlap/index helper behavior |
| temporal_aggregator.cpp | temporal aggregation behavior |
| temporal_conflict_resolver.cpp | temporal conflict-resolution behavior |
| snapshot_manager.cpp | temporal snapshot lifecycle behavior |
| retention_manager.cpp | temporal retention behavior |
| temporal_cdc.cpp | temporal change-event behavior |
| temporal_compressor.cpp | temporal history compression behavior |
| temporal_migrator.cpp | temporal migration behavior |
| temporal_tier_manager.cpp | temporal tier-management behavior |
| temporal_cold_store.cpp | temporal cold-store path behavior |

## Scope

In scope:
- temporal/bitemporal query and version semantics
- snapshot/retention/conflict-resolution behavior
- temporal indexing, CDC, compression, and migration/tiering paths

Out of scope:
- non-temporal query planning outside temporal boundaries
- generic timeseries ingestion internals owned by timeseries module

## Runtime Behavior and Limits

- temporal query semantics are bounded by version and period constraints.
- bitemporal operations expose deterministic as-of and interval outcomes.
- retention/snapshot paths expose explicit lifecycle transitions.
- conflict-resolution and CDC behavior remain observable and diagnosable.

## Sourcecode Verification (Module: temporal/readme)

- Verified files:
  - src/temporal/temporal_query_engine.cpp
  - src/temporal/bi_temporal.cpp
  - src/temporal/bitemporal_join.cpp
  - src/temporal/system_versioned_table.cpp
  - src/temporal/temporal_index.cpp
  - src/temporal/interval_tree_index.cpp
  - src/temporal/temporal_aggregator.cpp
  - src/temporal/temporal_conflict_resolver.cpp
  - src/temporal/snapshot_manager.cpp
  - src/temporal/retention_manager.cpp
  - src/temporal/temporal_cdc.cpp
  - src/temporal/temporal_compressor.cpp
  - src/temporal/temporal_migrator.cpp
  - src/temporal/temporal_tier_manager.cpp
  - src/temporal/temporal_cold_store.cpp
- Verified behavior surfaces:
  - temporal query/versioning/retention/snapshot/conflict and CDC/compression paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
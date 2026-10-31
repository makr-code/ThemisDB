---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Temporal implementation guide

This guide is for developers changing temporal internals. Public declarations
are under [`include/temporal/`](../../include/temporal/); implementations are
owned by `src/temporal/`. The public `themisdb::temporal` types and
`themis::temporal` contract header are distinct surfaces; follow
[`MODELING.md`](MODELING.md) before changing timestamp or interval behavior.

## Implementation areas

| Responsibility | Main implementation |
|---|---|
| System/valid-time versions and queries | `bi_temporal.cpp`, `system_versioned_table.cpp`, `temporal_query_engine.cpp` |
| Range lookup and aggregation | `interval_tree_index.cpp`, `temporal_index.cpp`, `temporal_aggregator.cpp` |
| Snapshot and retention lifecycle | `snapshot_manager.cpp`, `retention_manager.cpp` |
| Change events and history compression | `temporal_cdc.cpp`, `temporal_compressor.cpp` |
| Conflict and migration paths | `temporal_conflict_resolver.cpp`, `temporal_migrator.cpp` |
| Cold storage and tiers | `temporal_cold_store.cpp`, `temporal_tier_manager.cpp` |

## Invariants to preserve

1. Keep system time and valid time separate. Preserve the runtime
   millisecond/half-open `TimeRange` convention and handle empty ranges
   consistently.
2. System-versioned updates close the prior version and open a new one;
   current rows are represented by an open-ended system-time range.
3. Snapshot reads use captured table scans. Snapshot handles are process-local,
   must be released or age/count-collected, and queries for unknown or released
   handles return an empty result.
4. Retention applies to historical versions. The current version is retained;
   the in-memory archive is not durable storage. `scheduleTable` stores a
   reference, so the table must outlive its registration or the scheduler must
   be stopped first.
5. In-memory CDC replay is bounded. An overflow can make replay incomplete;
   subscription callbacks are synchronous, and callback work is on the
   publisher's path. Use `CDCPersistentLog` only when a caller explicitly
   configures its WAL lifecycle.
6. User-provided conflict resolvers must satisfy the documented deterministic,
   commutative, and idempotent requirements for CRDT merging; test those
   properties rather than assuming them.

## Change and test workflow

Add focused coverage alongside the affected `tests/temporal/test_*.cpp` file.
`tests/temporal/CMakeLists.txt` registers those sources as focused tests with a
120-second timeout. Relevant existing coverage includes temporal query,
system-versioned table, retention, snapshot, interval index, CDC, compression,
conflict, and high-cardinality stress tests. Update the public header contract
and this module documentation when behavior or edge cases change. For
performance-sensitive changes, update and run the mapped cases in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

Exact model backend not verifiable from final document text alone.

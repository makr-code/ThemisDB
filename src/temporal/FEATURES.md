---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Temporal module features

This page summarizes behavior exposed by the current temporal headers. It is an
API overview, not a release-readiness statement. The root
[`ROADMAP.md`](../../ROADMAP.md) is authoritative for module maturity and
release blockers.

## Runtime surfaces

| Capability | Public interface | Source-backed scope |
|---|---|---|
| System-time history | [`SystemVersionedTable`](../../include/temporal/system_versioned_table.h) | Insert, update, delete, current/as-of reads, history scans, and historical-version purging. |
| Bitemporal records | [`BiTemporalTable`](../../include/temporal/bi_temporal.h) | System and valid time, as-of queries, current valid-time queries, overlap/gap inspection, and merge. |
| Temporal queries and joins | [`TemporalQueryEngine`](../../include/temporal/temporal_query_engine.h), [`BiTemporalJoin`](../../include/temporal/bitemporal_join.h) | C++ query helpers over versioned tables; no claim that SQL/AQL syntax is wired to these classes. |
| Indexing and aggregation | [`TemporalIndex`](../../include/temporal/temporal_index.h), [`IntervalTreeIndex`](../../include/temporal/interval_tree_index.h), [`TemporalAggregator`](../../include/temporal/temporal_aggregator.h) | Time/range lookup, interval lookup, and window aggregation APIs. |
| Lifecycle | [`TemporalSnapshotManager`](../../include/temporal/snapshot_manager.h), [`RetentionManager`](../../include/temporal/retention_manager.h) | In-memory snapshots and policy-driven historical-version cleanup. |
| Change and conflict handling | [`TemporalCDC`](../../include/temporal/temporal_cdc.h), [`TemporalConflictResolver`](../../include/temporal/temporal_conflict_resolver.h) | Bounded in-memory event replay, an optional filesystem WAL API, and configured conflict strategies. |
| History storage transforms | [`TemporalCompressor`](../../include/temporal/temporal_compressor.h), [`TemporalTierManager`](../../include/temporal/temporal_tier_manager.h), [`TemporalColdStore`](../../include/temporal/temporal_cold_store.h), [`TemporalMigrator`](../../include/temporal/temporal_migrator.h) | Compression, hot/warm/cold history operations, cold-store backends, and migration helpers. |

## Important boundary

The runtime types in `themisdb::temporal` use millisecond timestamps and
half-open ranges. `include/temporal/temporal_api_contract.h` is a separate
`themis::temporal` contract surface with different named timestamp constants;
do not treat that header as proof that its stated contracts are implemented by
the runtime classes. See [`MODELING.md`](MODELING.md).

The temporal interfaces do not establish caller authorization, deployment
configuration, a CLI, or release acceptance. See
[`SECURITY.md`](SECURITY.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md),
and the root roadmap for those boundaries.

Exact model backend not verifiable from final document text alone.

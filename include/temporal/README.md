# Temporal public API index

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

This page indexes the interfaces declared in `include/temporal/`. The headers
are the authority for exact signatures, defaults, and return behavior; this
index does not promise ABI stability, production approval, or query-language
integration.

## Runtime headers (`themisdb::temporal`)

| Header | Main interface | Scope |
|---|---|---|
| [`temporal_types.h`](temporal_types.h) | `Timestamp`, `TimeRange`, `VersionedDocument` | Millisecond runtime types and half-open ranges. |
| [`system_versioned_table.h`](system_versioned_table.h) | `SystemVersionedTable` | System-time writes, current/as-of reads, history, and purge. |
| [`bi_temporal.h`](bi_temporal.h) | `BiTemporalTable` | System-time and valid-time records and queries. |
| [`temporal_query_engine.h`](temporal_query_engine.h) | `TemporalQueryEngine` | C++ temporal predicates, table queries, and joins. |
| [`bitemporal_join.h`](bitemporal_join.h) | `BiTemporalJoin` | Join over supplied bitemporal rows. |
| [`temporal_index.h`](temporal_index.h) | `TemporalIndex` | Temporal interval entries and lookup. |
| [`interval_tree_index.h`](interval_tree_index.h) | `IntervalTreeIndex` | In-memory point, overlap, and key lookup. |
| [`temporal_aggregator.h`](temporal_aggregator.h) | `TemporalAggregator` | Windowed aggregation and trend helpers. |
| [`snapshot_manager.h`](snapshot_manager.h) | `TemporalSnapshotManager` | In-memory snapshots, lookup, diff, and collection. |
| [`retention_manager.h`](retention_manager.h) | `RetentionManager` | Policy enforcement and optional scheduling. |
| [`temporal_cdc.h`](temporal_cdc.h) | `TemporalCDC`, `CDCPersistentLog` | Bounded in-memory CDC and separate filesystem WAL API. |
| [`temporal_conflict_resolver.h`](temporal_conflict_resolver.h) | `TemporalConflictResolver` | Conflict policies, merge resolvers, history, and manual resolution. |
| [`temporal_compressor.h`](temporal_compressor.h) | `TemporalCompressor` | Compression helpers for system-versioned history. |
| [`temporal_migrator.h`](temporal_migrator.h) | `TemporalMigrator` | Migration analysis/backfill/verification helpers. |
| [`temporal_cold_store.h`](temporal_cold_store.h) | `TemporalColdStore` | Cold-store interface and in-memory/filesystem backends. |
| [`temporal_tier_manager.h`](temporal_tier_manager.h) | `TemporalTierManager` | Hot/warm/cold history operations and compaction. |

## Separate contract header

[`temporal_api_contract.h`](temporal_api_contract.h) declares symbols in
`themis::temporal`, not `themisdb::temporal`. It contains nanosecond-named
constants and contract statements that differ from the runtime's millisecond
and half-open `TimeRange` semantics. Do not mix the surfaces or infer that the
header's written requirements are implemented by the runtime APIs.

## Build and integration

The inspected CMake files do not establish a standalone public target named
`themisdb_temporal`. Temporal implementation translation units are registered
through repository module source lists. Follow root [`SETUP.md`](../../SETUP.md)
and the consuming target's actual CMake configuration rather than copying an
unverified `target_link_libraries` example.

For behavior and implementation boundaries see
[`../../src/temporal/README.md`](../../src/temporal/README.md) and
[`../../src/temporal/API.md`](../../src/temporal/API.md).

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

# Temporal API reference

This is an integration-oriented index of the `themisdb::temporal` C++ API.
Exact signatures, defaults, and return types are defined by the linked public
headers; this page does not introduce a new API contract.

## Main runtime interfaces

| Interface | Use | Important behavior |
|---|---|---|
| [`SystemVersionedTable`](../../include/temporal/system_versioned_table.h) | Insert, update, delete, read current rows or history | Mutations version rows; update returns false when no current row exists. |
| [`BiTemporalTable`](../../include/temporal/bi_temporal.h) | Track and query system and valid time | Current valid-time overlaps for a key are rejected; as-of methods take both time coordinates where applicable. |
| [`TemporalQueryEngine`](../../include/temporal/temporal_query_engine.h) | As-of/range predicates, joins, sequenced and non-sequenced query behavior | Time-range methods use `TimeRange`; joins apply the supplied predicate to scanned row pairs. |
| [`IntervalTreeIndex`](../../include/temporal/interval_tree_index.h) | Point, overlap, and key-oriented interval lookup | Entries contain half-open ranges; shared reads and exclusive writes are documented by the header. |
| [`TemporalSnapshotManager`](../../include/temporal/snapshot_manager.h) | Capture, query, diff, collect, and release table snapshots | Captures scans in memory; invalid or released handles do not produce snapshot rows. |
| [`RetentionManager`](../../include/temporal/retention_manager.h) | Register policies and enforce/schedule history cleanup | Current versions are kept; scheduled table references must remain valid. |
| [`TemporalCDC`](../../include/temporal/temporal_cdc.h) | Subscribe, publish, and replay change events | In-memory log defaults to 65,536 events and overwrites oldest events by default. |
| [`CDCPersistentLog`](../../include/temporal/temporal_cdc.h) | Append and replay CDC WAL segments | `open()` is required before append; I/O failures throw; WAL records contain serialized event payloads. |
| [`TemporalConflictResolver`](../../include/temporal/temporal_conflict_resolver.h) | Resolve conflicts using configured or injected strategies | Manual conflicts remain unresolved until explicitly handled; custom resolver constraints apply. |

## Inputs, units, and edge cases

`themisdb::temporal::Timestamp` is milliseconds since epoch;
`TimeRange::contains` uses `[start, end)`. Query timestamps, ranges, and
serialized timestamps must use that convention. A missing key, unmatched
filter, absent snapshot, or released snapshot can yield an empty result; callers
must distinguish these cases from transport or storage failures using the
specific operation's return value and status.

Do not treat [`temporal_api_contract.h`](../../include/temporal/temporal_api_contract.h)
as a synonym for these runtime declarations: it is in namespace
`themis::temporal` and defines nanosecond-named constants and separate endpoint
rules. See [`MODELING.md`](MODELING.md) before crossing those surfaces.

The interfaces are C++ library APIs, not a command-line administration
contract. The example `themis-admin` commands in
[`docs/operability/RUNBOOK_TEMPORAL_STORE.md`](../../docs/operability/RUNBOOK_TEMPORAL_STORE.md)
are secondary operational guidance and are not declared by these headers.

Exact model backend not verifiable from final document text alone.

# Temporal public-header architecture

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

## Scope

`include/temporal/` contains 17 public headers: 16 runtime headers and the
separate `temporal_api_contract.h`. Runtime declarations are primarily in
`themisdb::temporal`; `temporal_api_contract.h` declares `themis::temporal`.
These namespaces and their timestamp/interval rules must not be conflated.

The implementation and module-level status are documented in
[`../../src/temporal/ARCHITECTURE.md`](../../src/temporal/ARCHITECTURE.md) and
the root [`../../ROADMAP.md`](../../ROADMAP.md).

## Header groups

| Area | Headers |
|---|---|
| Types and versioning | `temporal_types.h`, `system_versioned_table.h`, `bi_temporal.h` |
| Query and joins | `temporal_query_engine.h`, `bitemporal_join.h` |
| Indexing and aggregation | `temporal_index.h`, `interval_tree_index.h`, `temporal_aggregator.h` |
| Lifecycle | `snapshot_manager.h`, `retention_manager.h` |
| Events and conflict handling | `temporal_cdc.h`, `temporal_conflict_resolver.h` |
| Storage and history operations | `temporal_cold_store.h`, `temporal_tier_manager.h`, `temporal_compressor.h`, `temporal_migrator.h` |
| Distinct declared contract | `temporal_api_contract.h` |

## Contract boundaries

- Runtime `Timestamp` is milliseconds since epoch, and runtime `TimeRange`
  contains `[start, end)`, as declared in `temporal_types.h`.
- `temporal_api_contract.h` declares nanosecond-named constants and additional
  contract statements in another namespace. It is not a synonym for the runtime
  declarations and must not be used to infer runtime behavior.
- Return and failure behavior is header-specific. The APIs use combinations of
  booleans, optionals, collections, statistics, and exceptions; this directory
  does not define one shared `Result<T>` or error contract.
- These headers do not by themselves establish SQL:2011 conformance, AQL/SQL
  parser integration, service authorization, or a separately linkable CMake
  target.

---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-11-19
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera adapter contract: source-backed notes

**Purpose / audience:** help developers distinguish checked-in declarations from contract guarantees. This document is not a frozen compatibility or release contract.

## Contract source selection

`database_adapter.hpp` first includes `external/chimera/include/chimera/database_adapter.hpp` when that file is available. If absent, it defines a minimal compatibility surface locally, including an `ErrorCode` enum with values 0–15 and basic `Result<T>`, scalar, row, and table types. Compatibility macros map several capability names in the external-header case. Consequently, the fallback enum and shim types must not be presented as the canonical external CHIMERA ABI or as proof of cross-version stability.

`themisdb_adapter.hpp` declares `ThemisDBAdapter` as an implementation of `IDatabaseAdapter`, `IAsyncDatabaseAdapter`, `IStreamingAdapter`, and `IPreparedStatementAdapter`. It exposes connection, relational, vector, graph, document, transaction, async, streaming, and prepared-statement methods. The header alone does not guarantee all methods delegate to a live engine; see the implementation and [`../../src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md).

## Observable limits

- `connect()` checks for a non-empty `themisdb://` authority but only changes local connected state; it makes no network connection. Non-empty option maps return `UNSUPPORTED`; they are not silently ignored.
- The adapter accepts injected query, vector-index, and graph-index pointers; engine-specific compilation/dispatch is conditional.
- Streaming uses `ThemisDBResultStream` over a `RelationalTable` value. Do not claim a server-side cursor from this declaration.
- Prepared statements are implemented locally; do not describe them as a backend plan cache without additional evidence.
- The fallback `ErrorCode` includes `DISPATCH_FAILED` (14) and `CAPABILITY_MISMATCH` (15), but the external header may be a different version.
- In the checked-in external v0.0.37 header, `PREPARED_STATEMENTS` and `CONNECTION_POOLING` compatibility macros alias `SHARDING` and `REPLICATION`. Those aliases preserve compilation only; `ThemisDBAdapter` does not report them as capabilities, even when the corresponding local API/provider exists. A caller must not interpret an aliased token as proof of the distinct feature.
- The MongoDB, Neo4j, and Qdrant adapters also do not report `CONNECTION_POOLING`; no adapter-specific pool provider is established by the checked-in implementation.
- `ThemisDBAdapter` capability discovery is conservative: full-text, distributed-query, geospatial, time-series, secondary-index, and transaction capabilities are not reported by the current local implementation. Transaction methods maintain adapter-local bookkeeping; they do not imply ACID behavior.
- Retry classification reads the result's concrete `error_code`. A non-empty `retry_on_codes` set is an allowlist; otherwise explicitly enabled category flags select only connection, resource-exhaustion, or internal errors (all category flags default off). Retries also require caller assertion of idempotency and are bounded by count and backoff, not by an operation deadline.
- The local MongoDB, Neo4j, and Qdrant classes contain conditional backend surfaces; their declarations do not establish production support.

## Change and validation rule

Treat the selected external or fallback header, compiled definitions, corresponding implementation, and focused tests as a unit when describing a contract. This change compiled the local retry and adapter translation units with the checked-in external header; a configured focused GTest run and ABI validation remain outstanding.

## References

`database_adapter.hpp`, `themisdb_adapter.hpp`, `mongodb_adapter.hpp`, `neo4j_adapter.hpp`, `qdrant_adapter.hpp`, corresponding `src/chimera/*.cpp`, `tests/chimera/*.cpp`.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.

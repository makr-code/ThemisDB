---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `include/sharding/distributed_transaction.h`, `tests/sharding/test_sharding_contract_hardening_focused.cpp`
Related issues: #7093, #7167

# Sharding API contract

## Scope and versioning

The normative sharding behavior is specified in `include/sharding/sharding_api_contract.h`; that header states its contracts are stable within v1.x. Its contract and the actual declaration being changed take precedence over this summary.

## Public contract groups

- **Routing:** stable key mapping for an unchanged ring; bounded fallback to the next shard(s), then `SHARD_UNAVAILABLE` when exhausted.
- **Distributed transactions:** participants record prepare state before voting; commit/abort are idempotent; unresolved `IN_DOUBT` work blocks conflicting writes.
- **WAL:** ordered, durable append/replay contract; checksum corruption halts replay with `WAL_CORRUPTION`.
- **Migration:** explicit state transitions, conflict rejection, and rollback/fault reporting for incomplete work.
- **Router calls:** `ShardRouter` accepts resolver/executor dependencies and exposes point operations; `ShardResult.success` must be checked before consuming its data.

## `ShardRouter` public surface

The declarations in `include/sharding/shard_router.h` expose:

| Declaration | Declared behavior / result |
|---|---|
| `ShardRouter::Config` | `local_shard_id`; `scatter_timeout_ms` defaults to 30000; `max_concurrent_shards` defaults to 10; query pushdown defaults on and result caching defaults off |
| Constructor | Takes shared `URNResolver` and `RemoteExecutor`, a config reference, and optional `PrometheusMetrics` / `TrueTime` shared pointers |
| `get(URN, optional snapshot timestamp)` | Returns `optional<nlohmann::json>` |
| `put(URN, JSON)` and `del(URN)` | Return `bool` |
| `analyzeQuery(string)` | Returns `RoutingStrategy` |
| `executeQuery(string)` | Returns combined JSON; header notes scatter-gather fallback when single-shard extraction is inconclusive |
| `scatterGather(string)` | Returns one `ShardResult` record per invoked shard |
| `executeOnShards(string, shard_ids)` | Returns per-shard result records; header says unknown or unhealthy IDs are skipped with a warning |

These are declaration/header semantics. They do not establish that each transport, caching, or
query-pushdown path is enabled or conformant in every build. The implementation is
`src/sharding/shard_router.cpp`; `tests/sharding/CMakeLists.txt` conditionally registers the
contract-hardening and focused sharding tests.

## Caller and maintainer obligations

- Do not bypass the bounded fallback or reinterpret exhausted routing as success.
- Do not resolve an uncertain transaction without its coordinator/recovery path.
- Preserve canonical error codes and versioning rules in the contract header.
- Test the changed contract in `tests/sharding/test_sharding_contract_hardening_focused.cpp` or the closest focused suite registered in `tests/sharding/CMakeLists.txt`.

See `IMPLEMENTATION.md` for source mapping and `MODULE_GUIDE.md` for the change workflow.

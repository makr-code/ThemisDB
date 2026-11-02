---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-02"
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
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `include/sharding/epoch_fencing.h`, `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/two_phase_commit_participant.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_client_adapter.h`, `include/sharding/distributed_transaction.h`, `include/sharding/cross_shard_transaction.h`, `src/sharding/epoch_fencing.cpp`, `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/two_phase_commit_participant.cpp`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/distributed_transaction.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/test_epoch_fencing.cpp`
Related issues: #7093, #7167
Related milestone: DOC-WEEKLY-2026-W45

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

## Shard write fencing and transaction credentials

- `EpochFencingManager` accepts a token only for its configured shard and only
  when its nonzero epoch exactly equals the current epoch. Empty or mismatched
  issuer strings, wrong-shard tokens, stale epochs, and future epochs are
  rejected. `applyWithToken()` checks and applies the storage callback while
  holding the epoch-transition shared guard; `bumpEpoch()` takes the exclusive
  guard and therefore waits for an active guarded write to finish.
- The 2PC coordinator obtains a target-shard token from its configured provider,
  includes it in the serialized PREPARE payload, and rejects writes when the
  provider or token is invalid. The participant validates the token before
  invoking prepare-time validation/locking, retains it with prepared state, and
  writes it to participant WAL. Before applying COMMIT, it revalidates the token
  atomically with the storage callback through `applyWithToken()`.
- `ShardRPCClient::prepare(txn_id, operations)` returns false without dispatch;
  callers must use the explicit-token overload. The token is carried in the
  existing gRPC `transaction_data` payload. Distributed and cross-shard
  transaction write paths likewise fail closed when no nonzero, issuer-bearing,
  target-scoped token is supplied.
- A failed Phase 2 acknowledgement returns coordinator `ERROR`; the decision
  remains unresolved rather than being marked complete. WAL-backed coordinator
  configurations retain the recorded decision for recovery. Remote abort
  adapters report the actual acknowledgement status.

The issuer is an identity string checked for emptiness/equality, not a
cryptographically signed credential or authenticated-principal binding. These
contracts do not establish shared durable DR metadata or genuine multi-process
failover/recovery E2E behavior; both remain open blockers.

## Caller and maintainer obligations

- Do not bypass the bounded fallback or reinterpret exhausted routing as success.
- Do not resolve an uncertain transaction without its coordinator/recovery path.
- Preserve canonical error codes and versioning rules in the contract header.
- Test the changed contract in `tests/sharding/test_sharding_contract_hardening_focused.cpp` or the closest focused suite registered in `tests/sharding/CMakeLists.txt`.

See `IMPLEMENTATION.md` for source mapping and `MODULE_GUIDE.md` for the change workflow.

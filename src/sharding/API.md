---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-19"
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
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `include/sharding/epoch_fencing.h`, `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/two_phase_commit_participant.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_client_adapter.h`, `include/sharding/distributed_transaction.h`, `include/sharding/cross_shard_transaction.h`, `include/sharding/mtls_client.h`, `include/sharding/remote_executor.h`, `src/sharding/mtls_client.cpp`, `src/sharding/remote_executor.cpp`, `tests/sharding/test_mtls_client_cancellation.cpp`, `src/sharding/epoch_fencing.cpp`, `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/two_phase_commit_participant.cpp`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/distributed_transaction.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/test_epoch_fencing.cpp`
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

## Durable 2PC recovery (DIS-06 / #8200)

`TwoPhaseCommitCoordinator` owns the immutable COMMIT/ABORT decision for the named
2PC coordinator pair. Its WAL is required for writes: BEGIN is synced before
PREPARE, the decision before phase two, and completion after every acknowledgement,
even when `sync_wal_writes` is false. `prepare()` permits explicit lifecycle
staging; a restart without a durable decision selects durable ABORT. `abort()`
cannot replace COMMIT. `commit()` success means completed delivery, not merely
a durable COMMIT decision; inspect `getTransactionDecision()` to distinguish them.
Concurrent drivers of the same transaction fail closed.

`CrossShardTransactionCoordinator` delegates only its 2PC path; its lifecycle,
SSI handling and other protocols remain distinct. With persistence enabled,
the canonical log lives under `data_dir/two_phase_commit`, separate from
`data_dir/wal`. Configure `participant_endpoint_resolver` with trusted current
transport/TLS settings for persisted shard identities. Missing endpoints or
unresolved older CrossShard 2PC records block recovery rather than guessing a
decision. There is no automatic WAL migration or coordinator API removal.

Participants require a WAL for writes, sync PREPARE before voting YES and terminal
records before ACK; missing WAL configuration leaves readiness false.
`prepare_timeout` is an uncertainty-reporting threshold: the existing
`abortTimedOutTransactions()` API returns zero, retains prepared locks and
never writes ABORT. Delayed authoritative decisions and duplicate requests
remain accepted. Application callbacks must be transaction-ID idempotent
across a crash between applying changes and recording COMMIT.

Participant `recovery_complete` means WAL replay and lock restoration finished;
it does not mean every in-doubt transaction is resolved. `ready`,
`recovery_status`, and `active_in_doubt` expose resolution blockers. Recovered,
expired or failed-apply prepares block health/new PREPARE while decision RPCs
remain available. Ordinary live prepares do not block readiness.
Terminal history is retained for replay idempotency, including decisions older
than the WAL serving cache; retention/compaction requires a separately agreed
retry horizon and is not introduced here. Coordinator, participant and
replication WAL encodings are unchanged and are not interchangeable.

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

## Request-context-aware transport (`MTLSClient`, `RemoteExecutor`)

Additive overloads taking `const themis::query::RequestContext&`
(`include/query/request_context.h`: deadline + cancellation token); existing
overloads are unchanged.

| Declaration | Behavior |
|---|---|
| `MTLSClient::get(endpoint, path, ctx)`, `MTLSClient::post(endpoint, path, body, ctx)` | Resolve/connect/TLS handshake/write/read run as async Boost.Asio operations on a private per-call `io_context` driven by the calling thread. A 10 ms poll timer evaluates the token, request deadline and per-stage timeout (`connect_timeout_ms` for resolve/connect/handshake, `request_timeout_ms` for write/read) and aborts the pending operation. All handlers complete before return; no thread/socket/timer outlives the call. |
| `MTLSClient::Response::cancelled` / `timed_out` / `stage_timed_out` | `cancelled`: token cancelled. `timed_out`: the supplied context deadline expired (which may be a per-shard child budget). `stage_timed_out`: a connect/request stage timeout fired while the context was live; it is exclusive with `timed_out` and never retried. A refused or failed connection sets none. Retry back-off honours the context. |
| `RemoteExecutor::get(shard, path, ctx)`, `RemoteExecutor::post(shard, path, body, ctx)` | Check the context before the circuit breaker, then propagate it into the transport. `Result::cancelled` / `timed_out` / `stage_timed_out` mirror the response. A cancel is not recorded as a circuit-breaker failure; a timeout is. |

Explicit limits (QRY-07 is not complete):

- Cancellation is observed by polling (~10 ms), not by wake-up.
- `MTLSClient::put/del`, the `authorization_header` `post` overload,
  `RemoteExecutor::put/del/executeQuery/postBinary`, the connection-pool path and
  all legacy overloads remain synchronous and non-interruptible. On the legacy
  path `tcp_stream::expires_after()` does not bound blocking Beast calls, so its
  configured timeouts are not enforced.
- In-flight abort is tested for the TLS handshake against a non-responding peer;
  write/read abort uses the same mechanism without a dedicated test, and there is
  no end-to-end test against a real shard server.

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

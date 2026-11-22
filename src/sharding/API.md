---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-22"
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
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `include/sharding/consistent_hash.h`, `include/sharding/epoch_fencing.h`, `include/sharding/raft_consensus.h`, `include/sharding/raft_state.h`, `include/sharding/raft_log.h`, `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/two_phase_commit_participant.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_client_adapter.h`, `include/sharding/distributed_transaction.h`, `include/sharding/cross_shard_transaction.h`, `include/sharding/mtls_client.h`, `include/sharding/remote_executor.h`, `include/storage/mvcc_store.h`, `include/storage/rocksdb_wrapper.h`, `src/sharding/consistent_hash.cpp`, `src/storage/mvcc_store.cpp`, `src/sharding/mtls_client.cpp`, `src/sharding/remote_executor.cpp`, `src/sharding/raft_consensus.cpp`, `src/sharding/raft_state.cpp`, `src/sharding/raft_log.cpp`, `tests/sharding/test_dis03_routing_contract_focused.cpp`, `tests/storage/test_mvcc_range_export.cpp`, `tests/sharding/test_mtls_client_cancellation.cpp`, `tests/test_raft_persistence_focused.cpp`, `src/sharding/epoch_fencing.cpp`, `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/two_phase_commit_participant.cpp`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/distributed_transaction.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/test_epoch_fencing.cpp`
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

`RaftConsensusAdapter::propose` returns the index assigned by the Raft proposal only
when the existing quorum mechanism reports commit; it no longer guesses the next
index or reports a failed proposal as successful. A missing result does not prove an
operation can never commit after leadership changes. This synchronous path can block
in its replication callback and does not establish durable network replication.
`RaftState` promotes a candidate on its self-vote only when membership contains
exactly that one node. Multi-node elections now send `VoteRequest` values through
the configured `VoteRequestCallback`; only responses with the expected voter ID
and election term count, and a higher-term response steps the candidate down
after durable hard-state persistence. Candidates retry after an election timeout.
The vote handler rejects candidates outside the configured membership and
candidates with a stale log, and requires a healthy durable journal before a
multi-node vote can be granted. A multi-node candidate also refuses to start
elections without its own healthy durable journal. The shard gRPC service now
exposes `RequestVote` and `AppendEntries`; `ShardRPCClient` requires verified
mTLS, a configured local node ID, an expected remote member ID, a verified DNS
SAN for the server certificate, and a positive timeout capped at five seconds.
AppendEntries is bounded to 256 entries, 1 MiB per command, and 3 MiB aggregate
command payload.
The server maps the authenticated client certificate SAN to a configured member,
resolves the logical group, checks membership, and only then calls the durable
`RaftState` handlers. Responses must identify the configured remote member.
`RaftShardManager` can construct those clients through
`raft_peer_config_resolver` and installs vote, append, and heartbeat callbacks.
Without that resolver, multi-node elections and proposals remain fail-closed.
`RaftShardManager::initializeShard()` uses its `shard_id` argument as the
logical Raft group key; the local voting member is the node ID in
the nested `RaftConfig::node_id` field of `RaftShardManager::Config::raft_config`.
Existing configurations must set this local node ID; no group-ID fallback is applied.
Membership must
contain that non-empty local ID exactly once, and all member IDs must be unique
and non-empty. The group ID is never substituted for a node identity.
The Raft proposal path checks the original term and leader state when advancing
the commit index; a quorum failure leaves the log entry for Raft recovery instead
of truncating the tail. A failed result can therefore become committed later.
`ReplicationCallback` is source-incompatible with the former boolean callback:
it now exchanges `AppendEntriesRequest` and `AppendEntriesResponse`; C++ consumers
must rebuild against the matching headers. A response counts toward quorum only
when its term matches the proposal and its `match_index` covers the requested
entry. The follower handler requires contiguous entries with nondecreasing terms
from the previous-log anchor (whose term must be zero at index zero), rejects
same-index/term payload conflicts, protects committed entries from truncation,
and reports only the prefix verified by that request. A transport must return
success only after the follower has durably appended the requested entries.
The local `handleAppendEntries()` handler also refuses to acknowledge requests
unless a healthy durable journal is configured; volatile follower state is not
a replication ACK. Multi-node leaders likewise require a healthy local durable
journal before appending or invoking the transport callback.
Heartbeat AppendEntries responses with a higher term cause the leader to persist
that term and step down; delayed successful responses cannot regress tracked
replica progress.

The local proposal path refuses new entries while any local tail entry is
uncommitted; that check and append are atomic under the log lock. This
deliberately fails closed after a quorum failure or restart and does not
implement conflict repair. The `DistributedGateway` no longer installs its
in-process callback; multi-node proposals fail closed until a real peer
transport is configured. The Linux gRPC test source now includes manager-driven
vote/append transport and authenticated identity rejection, plus two- and
three-process follower crash/restart cases for durable log prefixes, persisted
vote rejection, volatile journals, stale terms, missing prefixes, bounded RPC
timeout, quorum loss, and separate leader-process SIGKILL/restart recovery.
These additions have not been compiled or run in the current Windows
benchmark configuration. The separate leader-process case covers leader
SIGKILL/restart and journal recovery, and the follower processes now run
independent Raft managers for re-election after leader loss; that test waits
for a quorum follower journal to apply the leader's commit through normal
heartbeats before killing the leader. Runtime evidence for follower
failover, unavailable-journal fault injection, safe membership, and an end-to-end
quorum-backed 2PC decision store remains open.

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
Configuration/record additions require rebuilding C++ consumers against matching
headers; mixed-version binary ABI compatibility is not claimed.

CrossShard supplies its TrueTime/snapshot-aware timestamp policy through
`commit_timestamp_provider`. A single timestamp is recorded in the existing
optional `commit_timestamp_ns` decision metadata and reused for all endpoint
participants and recovery retries. The binary WAL encoding is unchanged; this
does not make the participant or replication logs compatible with coordinator
logs. `getTransactionRecord()` exposes a value snapshot for lifecycle/participant
status synchronization; it does not transfer ownership of internal state.

Roll out only after quiescing older coordinators and preserving all log domains.
Unresolved older records require explicit maintainer reconciliation. Rolling
back before the first new 2PC write is different from downgrading an activated
coordinator: older CrossShard binaries do not consume the new canonical log
directory. Do not downgrade after activation without an explicitly reviewed
state-reconciliation/migration procedure; merely deleting the new directory
or draining its pending count is not a safe rollback.

Participants require a WAL for writes, sync PREPARE before voting YES and terminal
records before ACK; missing WAL configuration leaves readiness false.
`prepare_timeout` is an uncertainty-reporting threshold: the existing
`abortTimedOutTransactions()` API returns zero, retains prepared locks and
never writes ABORT. Delayed authoritative decisions and duplicate requests
remain accepted. Application callbacks must be transaction-ID idempotent
across a crash between applying changes and recording COMMIT, and must atomically
persist business writes before returning true; participant WAL durability alone
does not make an asynchronous application write durable.

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

### Shard transfer (DIS-03)

`AutoRebalancer` now requires a `MigrationExecutor` instead of a `DataMigrator`.
`makeShardTransferExecutor()` connects a trusted, storage-owned immutable range
export to `ShardRPCClient::transferShard()`. The source adapter supplies the
exact assigned manifest, a verified-mTLS target client, and a bounded indexed
chunk reader. The adapter must use its durable assignment to bind the approved
operation to a migration ID, source, destination, range and topology epoch; the
migration ID is independently assigned and need not equal the operation ID.
The manifest also binds the snapshot watermark, chunk count, byte count and
ordered-payload SHA-256.

Storage adapters can use `materializeShardRangeSnapshot()` to encode a pinned,
ordered scan of live logical records for a half-open token range. The helper
validates ordering and token bounds and produces an immutable, indexed export
of up to 64 MiB. It neither derives routing tokens nor opens the backing
snapshot; those are the adapter's responsibility. The receiver's local
`readVerified()` API can inspect the persisted stream only for the exact,
still-current verified manifest. It is not a network read or cutover API.

Do not wire a production scanner by hashing MVCC base keys directly.
`RoutingKeyDescriptor` and `resolveRoutingToken()` now expose typed,
versioned token resolution; `RingOwnershipSnapshot` captures exact segments
and `ownsKeyInSnapshot()` checks token ownership against the snapshot epoch.
These helpers are not connected to the existing routing calls or to durable
assignments. `MVCCStore::SnapshotReader` now provides one-sequence enumeration,
version selection and explicit `Put`/`Delete` records, but it is process-local
and cannot resume after source restart. Production export remains disabled
until these primitives are composed with durable assignment authorization,
range quiescence, durable export/checkpoints and recoverable fenced cutover.

The destination exposes dedicated PREPARE, stream, verify and abort RPCs only
when configured with a `ShardTransferStore`, verified client certificates and
explicit transfer identity bindings. The store requires a durable assignment
and rechecks its currency on each request. PREPARE and chunk acknowledgements
reflect durable staging; successful verification does not publish the range.
This API neither uses 2PC PREPARE nor falls back to HTTP or local staging on
the sender.

The supplied executor reports at most `TRANSFER_VERIFIED`; only an executor
with an authoritative fenced cutover may return `OWNERSHIP_COMMITTED`.
Application composition must supply the export and authority adapters; the
server does not provision them automatically.

### DIS-03 routing and snapshot primitives

The [architecture record](ARCHITECTURE.md#dis-03-routing--und-snapshot-grundlagen)
describes the implemented boundaries and the remaining production gates. These
are local C++ primitives, not new RPC fields or a production transfer
composition.

| Implemented boundary | Contract |
|---|---|
| `RoutingKeyDescriptor` and `resolveRoutingToken()` | Exact key bytes, key type, supported algorithm/version and nonzero epoch resolve to a token. Partition-key FNV-1a/mix behavior has fixed vectors; UUID resolution is available only with xxHash and the explicit xxHash64 seed-0 identifier. Unsupported descriptors fail closed. Existing routing calls are unchanged. |
| `RingOwnershipSnapshot` and `snapshotOwnership()` | Copies actual ring entries into immutable, normalized half-open intervals; computes a SHA-256 fingerprint over the epoch and entries. It represents the full-ring singleton and `2^64` endpoint explicitly. The supplied epoch and fingerprint are not authority credentials. |
| `ownsKeyInSnapshot()` | Resolves a descriptor, requires its epoch to equal the snapshot epoch, and checks exact token ownership. This does not authorize an assignment or prevent topology changes. |
| `RocksDBWrapper::SnapshotHandle` and `MVCCStore::SnapshotReader` | Own one RocksDB snapshot and its sequence; select the latest committed visible version per logical key under one HLC watermark; report tombstones explicitly and propagate cancellation, limits, iterator, and callback errors. The reader is process-local, not a durable transfer export. |
| Durable transfer and ownership cutover | Not provided by these primitives. Durable assignment, export/resume, write quiescence, staging composition and authoritative fenced ownership commit remain required before rebalance completion. |

Manifest evolution must explicitly bind key type/algorithm version, actual
ownership segments, export format, snapshot sequence/HLC and logical record
count alongside the existing assignment fields and digest. The current
ascending-range manifest and key/value framing cannot silently acquire these
semantics; version negotiation and migration of callers require review.
Assignment admission and target publication remain separate authority steps.

#### Token compatibility vectors for review

`partition-key/fnv1a-mix64-v1` names the existing `hashKey()` algorithm, not a
replacement hash. Input/output are hexadecimal bytes and unsigned 64-bit
hexadecimal tokens. The focused routing test asserts these values; execution
evidence remains pending until the target is built and run.

| Input bytes | Expected token |
|---|---|
| `61` | `82a2a958a9bece5b` |
| `616263` | `33ebaf9927cbc5bd` |
| `610062` | `ab78f5eca36d0e2b` |
| `ff` | `1bbd5c813c69a8d7` |

The existing hash primitive maps an empty byte string to `efd01f60ba992926`;
that is not approval to admit an empty routing key. Empty logical keys remain
invalid for export. URN vectors must use the actual UUID string bytes with
xxHash64 seed 0 and compare against deployed placement fixtures before
as existing UUID string bytes; treating a UUID as binary or changing its
spelling is not allowed. The explicit descriptor rejects UUID token resolution
when xxHash is unavailable; `std::hash`-based placements have no portable
token and are not silently assigned the xxHash contract.

- Do not bypass the bounded fallback or reinterpret exhausted routing as success.
- Do not resolve an uncertain transaction without its coordinator/recovery path.
- Preserve canonical error codes and versioning rules in the contract header.
- Test the changed contract in `tests/sharding/test_sharding_contract_hardening_focused.cpp` or the closest focused suite registered in `tests/sharding/CMakeLists.txt`.

See `IMPLEMENTATION.md` for source mapping and `MODULE_GUIDE.md` for the change workflow.

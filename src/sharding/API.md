---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-24"
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
Canonical references: `include/sharding/sharding_api_contract.h`, `include/sharding/shard_router.h`, `include/sharding/consistent_hash.h`, `include/sharding/epoch_fencing.h`, `include/sharding/raft_consensus.h`, `include/sharding/raft_shard_manager.h`, `include/sharding/raft_state.h`, `include/sharding/raft_log.h`, `include/sharding/raft_two_phase_commit_decision_store.h`, `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/two_phase_commit_participant.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_server.h`, `include/sharding/shard_artifact_fragment_store.h`, `include/sharding/shard_rpc_client_adapter.h`, `proto/sharding/shard_rpc.proto`, `include/sharding/distributed_transaction.h`, `include/sharding/cross_shard_transaction.h`, `include/sharding/mtls_client.h`, `include/sharding/remote_executor.h`, `include/storage/mvcc_store.h`, `include/storage/rocksdb_wrapper.h`, `src/sharding/consistent_hash.cpp`, `src/storage/mvcc_store.cpp`, `src/sharding/mtls_client.cpp`, `src/sharding/remote_executor.cpp`, `src/sharding/raft_consensus.cpp`, `src/sharding/raft_shard_manager.cpp`, `src/sharding/raft_state.cpp`, `src/sharding/raft_log.cpp`, `src/sharding/raft_two_phase_commit_decision_store.cpp`, `tests/sharding/test_dis03_routing_contract_focused.cpp`, `tests/storage/test_mvcc_range_export.cpp`, `tests/sharding/test_mtls_client_cancellation.cpp`, `tests/test_raft_persistence_focused.cpp`, `tests/test_two_phase_commit.cpp`, `src/sharding/epoch_fencing.cpp`, `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/two_phase_commit_participant.cpp`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/shard_rpc_server.cpp`, `src/sharding/shard_artifact_fragment_store.cpp`, `src/main_server.cpp`, `src/sharding/distributed_transaction.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/test_epoch_fencing.cpp`
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

An uncommitted log tail is retained after quorum failure. Every proposal
invocation appends a distinct entry after that tail, including when its command
bytes are identical to an earlier proposal. Retries therefore have normal
at-least-once semantics; this API has no request ID or idempotency contract. The
leader advances commit only after a quorum has durably acknowledged the full
prefix through a current-term entry, so older retained entries become committed
only as part of that verified prefix.
Replication requests and production heartbeats resume from each follower's
tracked `next_index`; conflicts backtrack that index and later calls retry the
missing prefix. Each AppendEntries request is bounded to 256 entries, 1 MiB per
command and 3 MiB aggregate command payload, with the configured transport's
bounded deadline and durable-ack contract unchanged. Multi-node proposals over
the 1 MiB command limit are rejected before durable append, so an unsupported
entry cannot block future prefix replication. `InstallRaftSnapshot` now streams
bounded chunks over the same authenticated Raft transport. The server verifies
the member identity, group, deadline, chunk sequence, checksums, and aggregate
size (at most 512 MiB and one concurrent installation), then atomically installs
the snapshot file. Concurrent streams fail with `RESOURCE_EXHAUSTED`. A success
response is sent only after a configured installer reports success and the
durable Raft boundary matches the requested index/term. `RaftShardManager` can
send that snapshot automatically when tracked peer progress falls behind
compaction, then resume AppendEntries.
Both paths remain fail-closed without their explicit snapshot-manager and
application-restorer callbacks. After the receiver has durably installed the
matching Raft journal anchor, `RaftShardManager::restoreActiveTwoPhaseCommitDecisionStores()`
refreshes all live decision-store instances for that group from the snapshot's
2PC extension; it rejects a mismatched anchor or a missing extension when active
stores exist. The application installer must call this before acknowledging and
must coordinate application-state activation with the journal/2PC restore so a
crash cannot expose a mixed state. The helper does not itself make application
state activation crash-atomic. The multi-process runtime gate remains open in
[#8348](https://github.com/makr-code/ThemisDB/issues/8348).

The `DistributedGateway` no longer installs its in-process callback; multi-node
proposals fail closed until a real peer transport is configured. The Linux gRPC
test source also includes manager-driven vote/append transport and authenticated
identity rejection, plus two- and three-process follower crash/restart cases for
durable log prefixes, persisted vote rejection, volatile journals, stale terms,
missing prefixes, bounded RPC timeout, quorum loss, and separate leader-process
SIGKILL/restart recovery. These additions have not been compiled or run in the
current Windows benchmark configuration. The separate leader-process case covers
leader SIGKILL/restart and journal recovery, and the follower processes now run
independent Raft managers for re-election after leader loss; that test waits for
a quorum follower journal to apply the leader's commit through normal heartbeats
before killing the leader. Runtime evidence for follower failover,
unavailable-journal fault injection, safe membership, and an end-to-end
quorum-backed 2PC decision store remains open.

## Tensor fragment RPC

`FetchTensorFragment` is an additive server-streaming RPC for immutable
fragment bytes. The client requires a configured expected peer identity and
verified mTLS/DNS SAN, sends a positive deadline and byte ceiling, and does not
retry. It validates shard/artifact/correlation identity on every frame,
contiguous sequence numbers, a single final frame, consistent size/digest
metadata, and the total response size. Streams are limited to 4096 frames,
4 MiB per frame, and the configured client byte ceiling (at most 1 GiB).
The server additionally applies its configured maximum response bytes,
maximum deadline (default 30 seconds), and chunk size (default 1 MiB).

The server requires an explicit X.509 identity-to-storage-principal binding,
the exact locally served shard ID, and a configured `TensorFragmentReader`.
That callback receives opaque artifact IDs rather than filesystem paths and
must be thread-safe, enforce any additional tenant policy, obey the byte cap,
and return complete payload bytes with a lowercase SHA-256. Missing readers or
bindings fail closed. `ShardRPCClient` validates the stream and digest format;
it does not establish that the bytes match a manifest. The optional
`ShardRPCFragmentFetcher` performs that final SHA-256 comparison against the
trusted artifact manifest before returning success.

`ShardArtifactFragmentStore` defines a dedicated versioned RocksDB mapping:
the exact `(principal_id, shard_id, artifact_id)` tuple is hex-encoded into
`themis/shard-artifact-fragment/v1/`; metadata and payload are committed
atomically. Fragments are immutable, identical replays are synchronously
committed again, conflicting bytes are rejected, and reads check stored size metadata
before loading the payload and then verify SHA-256. The principal is the
authorization namespace resolved from the verified client certificate; host
publishers must select a tenant-bound principal and store the bytes before
publishing the matching manifest placement. Fragment transactions explicitly
enable and synchronize the RocksDB WAL independently of the database defaults;
power-loss behavior remains untested and depends on the storage stack honoring
sync operations. This mapping intentionally does
not reinterpret `TensorNetworkStorageEngine` field keys.

`src/main_server.cpp` now starts `ShardRPCServer` only when
`sharding.tensor_fragment_rpc.enabled` is explicitly true. Startup requires an
explicit `sharding.shard_id` or non-empty `SHARD_ID`, listener address, server
certificate, private key, client CA, and non-empty X.509 `identity_bindings`
mapping to storage principals. It always requires mTLS/client-certificate
verification, fails startup on invalid or unavailable configuration, and
stops the listener before closing the shared database. The server uses the
same RocksDB-backed fragment store as its reader. No mapping to tensor-field
keys or automatic metadata/payload publisher is implied. ShardRPC transports
opaque bytes; the separate distributed-tensor
[`GraphSummaryProjectionCodec`](../distributed_tensor/README.md#graph-summary-projection)
accepts only its versioned adapter-similarity format and does not change the
transport or storage contract.

`ShardTransferStoreTests` now includes source regressions for principal/shard
isolation, digest verification, size bounds, immutable replay/conflict
behavior, and database restart persistence. The Linux-only
`ShardTransferGrpcTests` includes an additional unrun case composing the
local publisher and RocksDB fragment reader with the mTLS adapter and a
fixture summary.
Production startup, automatic publishing, and gRPC-to-RocksDB runtime evidence
remain unverified.

Compatibility: the protobuf addition is additive and leaves existing RPCs
unchanged. The new `ShardRPCClient::Config` and `ShardRPCServer::Config`
members change C++ object layout, and `ShardArtifactFragmentStore` adds a
public C++ API, so consumers must rebuild against matching headers;
mixed-version binary ABI compatibility is not claimed.

## Durable 2PC recovery (DIS-06 / #8200)

`TwoPhaseCommitCoordinator` owns the immutable COMMIT/ABORT decision for the named
2PC coordinator pair. Its WAL is required for writes: BEGIN is synced before
PREPARE, the decision before phase two, and completion after every acknowledgement,
even when `sync_wal_writes` is false. `prepare()` permits explicit lifecycle
staging; a restart without a durable decision selects durable ABORT. `abort()`
cannot replace COMMIT. `commit()` success means completed delivery, not merely
a durable COMMIT decision; inspect `getTransactionDecision()` to distinguish them.
Concurrent drivers of the same transaction fail closed.
For CrossShard adapter staging, completed PREPARE is exposed as `PREPARED`;
failed preparation selects and durably records ABORT before phase two. If ABORT
delivery remains in-doubt, the enclosing commit returns `UNKNOWN` without
immediately sending a duplicate ABORT; explicit recovery may retry delivery.

`CrossShardTransactionCoordinator` delegates only its 2PC path; its lifecycle,
SSI handling and other protocols remain distinct. With persistence enabled,
the canonical log lives under `data_dir/two_phase_commit`, separate from
`data_dir/wal`. Configure `participant_endpoint_resolver` with trusted current
transport/TLS settings for shard identities. For a new endpoint registration,
a configured resolver supplies the full RPC configuration and its endpoint must
exactly match the requested endpoint; a missing result, mismatch, or resolver
exception rejects the participant. It is called outside the transaction lock and
transaction state is rechecked afterward. Existing endpoint registrations remain
in use; without a resolver, initial registration retains the default RPC settings.
During endpoint-backed WAL replay, the resolved endpoint must exactly match the recorded endpoint;
missing resolvers or entries and mismatches fail recovery before phase-two RPCs.
Previously registered participants remain usable under their registration contract.
Missing endpoints or
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

The lower-level `TwoPhaseCommitCoordinator::Config` accepts an optional
`decision_writer`/`decision_lookup` pair, which must be installed together.
`CrossShardTransactionConfig` exposes only a shared `raft_decision_store` for
quorum-backed decisions; it passes that store's writer and lookup to the
canonical coordinator. If the store is absent, CrossShard retains its existing
local-WAL-only behavior.
`RaftShardManager::createTwoPhaseCommitDecisionStore()` creates the store for a
managed shard, restores any installed decision snapshot, and retains that
shard's Raft instance. Assigning the shared store to CrossShard configuration
installs both callbacks and retains the store's lifetime. The store requires
the current leader, a quorum, and a healthy durable journal; it serializes
writers sharing the same Raft instance,
scans committed and retained-uncommitted entries, and rejects conflicting
decisions. The lookup returns only committed decisions. A missing or corrupt
log entry, unavailable authority, term change, or conflicting history fails
closed.

When configured, a normal commit must pass the authority before local decision
WAL or phase two; recovery checks the authority first, rejects
local/authoritative conflicts, and reconciles a quorum decision into the local
WAL before replaying phase two. If the authority has no committed decision,
recovery can proceed with ABORT only after the writer confirms it. If these
callbacks are absent, behavior remains local-WAL-only.

`RaftSnapshotManager` writes the legacy snapshot format when no extensions are
supplied; it also reads those legacy files. Named extensions use the version-2
format, with an independent SHA-256 checksum per extension. Older binaries do
not read version-2 files. `RaftTwoPhaseCommitDecisionStore::createAndInstallSnapshot()` captures all
committed decisions through the requested boundary, persists them under
`kSnapshotExtensionName`, installs the application snapshot, compacts Raft, and
advances the shared in-memory decision state as one serialized operation.
Callers still choose when to snapshot and must provide the complete application
state. The lower-level `captureSnapshotState()` remains available for custom
snapshot assembly. After loading and installing a snapshot, startup can
construct the store with the `RaftSnapshotManager` overload. It loads the
snapshot at the journal's installed boundary and restores the named extension
before the store is returned; a missing snapshot, mismatched boundary, or
missing decision extension throws. The low-level constructor and
`restoreSnapshotState()` remain available for callers that manage snapshot
loading themselves. Restore rejects corrupt, wrong-group, regressing,
boundary-mismatched, or conflicting state; lookups and writes remain fail-closed
if the compacted Raft boundary has no restored decision state.

The manager also serializes compaction with Raft state transitions and keeps the
in-memory snapshot index and term aligned with the durable log anchor. Snapshot
creation remains an explicit operation. `RaftShardManager` can construct the
snapshot-aware store, but callers must invoke the factory and inject the shared
store into CrossShard configuration; no store is created automatically during
manager startup. The store does not route requests to a remote leader: the
coordinator must receive the current leader's Raft instance or provide an
external leader-routing layer.
Follower snapshot transport and automatic catch-up are now available through
explicit mTLS peer and restore callbacks, but the application must bind those
callbacks to its transactional state restore and to any live decision-store
refresh. Leader routing, crash/failover, and release-platform evidence remain
required before claiming end-to-end quorum-backed 2PC.
Adding the extension map changes the public `RaftSnapshot` C++ layout; consumers
must rebuild against matching headers and libraries. The transfer-size setting
also changes `RaftSnapshotManager::Config` layout; mixed-version binary ABI is
not guaranteed.
The snapshot-install RPC is additive on the protobuf wire, but the new snapshot
resolver, installer and timeout fields change public Raft/RPC config-struct
layouts; C++ consumers must rebuild together. An older peer that does not
implement the RPC cannot catch up across compaction and remains unacknowledged.

The receiver-side journal primitive `RaftState::installSnapshotFromLeader()`
accepts only another configured member, a non-stale term, and a snapshot
boundary with a positive index and term. It
atomically rewrites the durable Raft journal and hard state; a
matching local boundary preserves its suffix, while a conflicting committed
boundary is rejected without changing the journal. If the boundary does not
match only uncommitted local history, that suffix is discarded. The caller must
validate and durably store the snapshot payload before calling this method, and
restore the application state before serving requests. This primitive does not
transfer snapshot bytes, apply the application snapshot, or provide the mTLS
RPC path; follower catch-up is therefore not yet end-to-end.
The separate payload boundary `RaftSnapshotManager::installTransferredSnapshot()`
validates a complete serialized snapshot file, enforces
`Config::max_transfer_bytes`, and installs it atomically. The RPC caller must
coordinate the file with journal installation and application-state restoration.
Its chunk overload rejects inconsistent indices/counts, missing or misplaced
final markers, empty chunks, oversized aggregate input, and per-chunk SHA-256
mismatches before installing any file. This protects the snapshot-file boundary;
it does not authenticate the sender or install the Raft journal/application state.

Roll out only after quiescing older coordinators and preserving all log domains.
Unresolved older records require explicit maintainer reconciliation. Rolling
back before the first new 2PC write is different from downgrading an activated
coordinator: older CrossShard binaries do not consume the new canonical log
directory. Do not downgrade after activation without an explicitly reviewed
state-reconciliation/migration procedure; this includes any version-2 Raft
snapshot carrying decision extensions. Older readers cannot consume that
format; merely deleting the new directory or draining its pending count is not
a safe rollback.

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

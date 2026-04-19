> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Sharding Module - Future Enhancements

This document covers planned enhancements to ThemisDB's sharding subsystem, which implements horizontal scaling through pluggable consensus algorithms (`ConsensusFactory` supporting Raft, Gossip, and Paxos), cross-shard distributed transactions (2PC, 3PC, SAGA, Percolator), deadlock detection, metadata sharding, and the `ShardRepairEngine` (anti-entropy with Reed-Solomon erasure coding). The module is currently in Beta state and requires RPC integration hardening, a production-ready cross-shard transaction coordinator, and improved observability before General Availability.

## Design Constraints

- All consensus algorithm implementations must be interchangeable at runtime via `consensus_factory.cpp` without requiring a cluster restart; the `IConsensusAdapter` contract must not be broken between adapter implementations.
- Cross-shard transactions must be crash-recoverable: any node failure during a 2PC/3PC prepare/commit phase must be detectable and resolvable via WAL replay (`transaction_wal.cpp`, `wal_manager.cpp`).
- The `ShardRepairEngine` anti-entropy scan must run as a background operation and must not consume more than 10% of IOPS on a shard node during normal operating hours.
- Shard topology changes (`shard_topology.cpp`) must be propagated to all nodes within one gossip round-trip (≤500 ms on a 100-node cluster) before any new request is routed to a new shard.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IConsensusAdapter::propose() / commit()` | `consensus_factory.cpp`, `raft_consensus_adapter.cpp`, `gossip_consensus_adapter.cpp` | Must be non-blocking; timeouts mandatory |
| `CrossShardTransaction::execute(saga_steps)` | `cross_shard_transaction.cpp`, `distributed_transaction.cpp` | Must return idempotency token for retry |
| `ShardRouter::route(key)` | `shard_router.cpp`, `adaptive_shard_router.cpp`, `locality_aware_router.cpp` | Consistent hash ring; must reflect topology within SLO |
| `ShardRepairEngine::schedule_scan()` | `shard_repair_engine.cpp` | Accepts priority hint; integrates with `shard_resource_manager.cpp` throttle |
| `RpcServiceRegistry` (sharding)  | `shard_rpc_client.cpp`, `shard_rpc_server.cpp` | mTLS required; retry policy injected at construction |
| `MetadataShard::lookup() / write()` | `metadata_shard.cpp`, `metadata_snapshot.cpp` | Must be linearizable; backed by Raft quorum |

## Planned Features

### `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode
**Priority:** High
**Target Version:** v1.8.0

`gpu_erasure_coder_opencl.cpp` now provides a complete Reed-Solomon erasure
coding implementation over GF(2^8). When an OpenCL device is found, parity
computation is offloaded to the GPU via an NDRange kernel; the class falls back
to an equivalent CPU path when no device is available.

**Implementation Notes:**
- `[x]` Implement `OpenCLErasureCoder::encode()`: compile a Galois Field GF(2^8) multiply kernel via `clCreateProgramWithSource` at construction; enqueue an NDRange kernel to compute parity blocks in parallel.
- `[x]` Implement `OpenCLErasureCoder::decode()`: perform syndrome computation and Gaussian elimination on the GPU to recover erased data blocks.
- `[x]` Implement `batchEncode()`: batch multiple stripe operations into a single kernel dispatch.
- `[x]` Add CPU/GPU parity test for encode+decode round-trip with 1, 2, and 3 erasures.

---

### [x] `CrossShardTransaction`: Hardcode Coordinator ID and Missing Compensation RPC
**Priority:** High
**Target Version:** v1.8.0

`cross_shard_transaction.cpp` had 3 critical TODOs (all resolved):
- Line 1743 (was 1898): `TODO: Replace with proper compensation RPC` — SAGA compensation actions now call `ShardRPCClient::compensate(txn_id, operation)`.
- Line 2426 (was 2581): `coordinator_id = "coordinator-1"` replaced with `config_.coordinator_id`.
- Line 2444 (was 2605): same hardcoded coordinator ID replaced.

**Implementation Notes:**
- `[x]` Added `coordinator_id` field to `CrossShardTransactionConfig`; callers inject `DistributedCoordinator::getLocalShardId()` before constructing the coordinator; all `"coordinator-1"` literals replaced with `config_.coordinator_id`.
- `[x]` Line 1743: SAGA compensation RPC implemented via `ShardRPCClient::compensate(txn_id, operation)`; bounded retry queue (max_retries=3, exponential backoff) already present and unchanged.

---

### `OrphanDetector`: Wire to `DistributedCoordinator` Transaction List
**Priority:** Medium
**Target Version:** v1.8.0

`orphan_detector.cpp` lines 50 and 67 have: "TODO: Access coordinator's transactions" and "TODO: Get transaction from coordinator". The orphan detector cannot inspect the coordinator's in-flight transaction list, making orphan detection non-functional.

**Implementation Notes:**
- `[x]` Inject `DistributedCoordinator*` into `OrphanDetector` constructor; call `coordinator->listInFlightTransactions()` at line 50 to get the authoritative transaction list.
- `[x]` At line 67: call `coordinator->getTransaction(txn_id)` to fetch transaction metadata.

---


**Priority:** High
**Target Version:** v0.9.0

Complete the RPC integration between `shard_rpc_client.cpp` / `shard_rpc_server.cpp` and the mTLS transport layer (`mtls_client.cpp`, `mtls_connection_pool.cpp`). Add automatic retry with exponential backoff and circuit-breaker integration (`circuit_breaker.cpp`) for all cross-shard RPC calls.

**Implementation Notes:**
- Implement `Shard RpcRetryPolicy` in `shard_rpc_client.cpp` using the existing `circuit_breaker.cpp` interface; classify gRPC status codes into retryable (`UNAVAILABLE`, `DEADLINE_EXCEEDED`) and non-retryable (`INVALID_ARGUMENT`, `ALREADY_EXISTS`).
- Wire `mtls_connection_pool.cpp` into `shard_rpc_client.cpp` to reuse TLS sessions; max pool size should be configurable via `gossip_config_manager.cpp`.
- Instrument every RPC call via `operational_metrics.cpp` and `prometheus_metrics.cpp` with labels for `shard_id`, `method`, and `outcome`.
- Certificate rotation events from `utils/pki_client.cpp` must trigger a graceful connection drain in `mtls_connection_pool.cpp` without dropping in-flight requests.

**Performance Targets:**
- Cross-shard RPC P99 latency (LAN): <5 ms excluding consensus overhead.
- Connection pool hit rate: >95% under sustained 10k RPS cross-shard traffic.
- Circuit-breaker open-to-half-open recovery time: configurable, default 5 s.

---

### [ ] Percolator-Style Distributed Transaction Coordinator
**Priority:** High
**Target Version:** v0.10.0

Implement a Percolator-style MVCC transaction protocol in `cross_shard_transaction.cpp` as an alternative to 2PC for read-heavy cross-shard workloads. The coordinator uses TrueTime timestamps (`truetime.cpp`) for snapshot isolation and defers lock cleanup to asynchronous background workers.

**Implementation Notes:**
- Add `PercolatorCoordinator` class to `cross_shard_transaction.cpp`; reuse `transaction_wal.cpp` for coordinator state persistence.
- `truetime.cpp` must expose a `TrueTime::now_with_uncertainty()` method returning an `[earliest, latest]` interval; commit waits until `now > commit_ts + max_uncertainty`.
- Integrate lock cleanup with `orphan_detector.cpp` to reclaim stale Percolator locks left by failed coordinators.
- `distributed_transaction.cpp` must select between 2PC and Percolator based on a per-transaction `isolation_level` hint.

**Performance Targets:**
- Percolator commit latency for 10-shard transaction: <20 ms P99 on LAN.
- Lock cleanup throughput via `orphan_detector.cpp`: >1000 stale locks/s.

---

### [~] Adaptive Shard Rebalancer with Load-Based Splitting
**Priority:** Medium
**Target Version:** v0.10.0

Extend `auto_rebalancer.cpp` and `shard_load_detector.cpp` to automatically split hot shards when CPU or storage utilization exceeds configurable thresholds. The rebalancer uses `predictive_detector.cpp` ML-based load forecasting to initiate splits before saturation occurs.

**Implementation Notes:**
- Add a `HotShardSplitPolicy` class to `auto_rebalancer.cpp` that consumes `shard_load_detector.cpp` metrics and emits split proposals to `rebalance_operation.cpp`.
- Integrate with `predictive_detector.cpp` to forecast load 5 minutes ahead; trigger pre-emptive split when predicted load exceeds 80% of capacity.
- `data_migrator.cpp` must support live migration with dual-write semantics: old shard accepts writes during migration, new shard catches up via `wal_shipper.cpp`, then atomic cutover via `shard_topology.cpp`.
- Emit split/merge events to `utils/audit_logger.cpp` for compliance audit trail.

**Performance Targets:**
- Shard split migration downtime (read unavailability): 0 ms (dual-write protocol).
- Write latency increase during live migration: <20% above baseline P99.
- Rebalancer decision cycle: <10 s from load threshold breach to split proposal.

---

### [~] Reed-Solomon Repair Engine Parallelisation
**Priority:** Medium
**Target Version:** v1.6.0

Parallelise the anti-entropy scan and Reed-Solomon reconstruction in `shard_repair_engine.cpp` to exploit multi-core hardware. The current implementation is single-threaded; large shards (~100 GB) take hours to fully scan. GPU-accelerated erasure coding via `gpu_erasure_coder.cpp` should be optionally engaged for bulk repair.

**Implementation Notes:**
- Refactor `shard_repair_engine.cpp` to use a work-stealing thread pool (`utils/thread_pool_manager.cpp`) for parallel segment scanning; partition the shard key space into scan bands, one per worker thread.
- Gate GPU erasure coding behind a runtime feature flag in `shard_resource_manager.cpp`; fall back to CPU path (`gpu_erasure_coder_opencl.cpp`) when no CUDA device is present.
- Throttle repair I/O using token-bucket rate limiter in `shard_resource_manager.cpp` to enforce the 10% IOPS budget constraint.
- Report repair progress via `slo_monitor.cpp` so operators can track time-to-full-repair.

**Performance Targets:**
- Anti-entropy scan throughput: >1 GB/s per node on NVMe with 8 parallel workers.
- GPU Reed-Solomon reconstruction: >4 GB/s on NVIDIA A10 (`gpu_erasure_coder.cu`).
- IOPS consumption during repair: <10% of node peak IOPS.

---

### [ ] Raft Snapshot Compaction and Log Truncation
**Priority:** High
**Target Version:** v0.9.0

Implement automated Raft log snapshot compaction in `raft_log.cpp` and `raft_wal_integration.cpp` to prevent unbounded WAL growth. Snapshots are compressed with ZSTD (`utils/zstd_codec.cpp`) and stored via `metadata_snapshot.cpp`, then transferred to lagging replicas via `wal_shipper.cpp`.

**Implementation Notes:**
- Add `RaftSnapshotManager` to `raft_log.cpp` that triggers compaction when log size exceeds a configurable threshold (default 512 MB); store snapshot index and term in `raft_state.cpp`.
- Use `utils/zstd_codec.cpp` for snapshot compression; target compression ratio >3× for typical metadata payloads.
- `wal_shipper.cpp` must support chunked snapshot transfer with checksums to tolerate network interruption during lagging-replica catch-up.
- `paxos_snapshot.cpp` and `paxos_wal.cpp` must receive equivalent snapshot compaction support for parity with the Raft path.

**Performance Targets:**
- Snapshot creation time for 1 GB Raft state: <10 s.
- Compressed snapshot size: <35% of uncompressed (ZSTD level 3).
- Lagging replica catch-up via snapshot transfer: >200 MB/s on 10 GbE LAN.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Cover `PercolatorCoordinator`, `HotShardSplitPolicy`, `RaftSnapshotManager` |
| Integration | All consensus adapters | Raft / Gossip / Paxos adapter end-to-end under injected partition scenarios |
| Chaos | 3-node and 5-node clusters | Inject leader failure, network partition, disk full; verify recovery and WAL replay |
| Performance | P99 < budgets above | Cross-shard txn benchmark, repair throughput, snapshot creation timing |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| Cross-shard RPC P99 latency | ~18 ms | <5 ms | gRPC + mTLS connection pool benchmark |
| 2PC commit latency (5 shards) | ~35 ms | <15 ms | Distributed txn benchmark with Raft quorum |
| Percolator commit latency (10 shards) | N/A | <20 ms | New coordinator benchmark |
| Shard anti-entropy scan (100 GB) | ~6 hours | <30 min | Parallel repair with 8-worker pool |
| Raft snapshot creation (1 GB) | N/A | <10 s | `RaftSnapshotManager` benchmark |
| Topology change propagation | ~1.2 s | <500 ms | Gossip round-trip measurement on 100-node cluster |

## Security / Reliability

- [ ] All cross-shard RPC calls must use mTLS with certificates issued by the cluster CA via `utils/pki_client.cpp`; plaintext connections must be rejected by `shard_rpc_server.cpp`.
- [ ] Transaction WAL entries (`transaction_wal.cpp`) must be encrypted at rest using keys derived by `utils/hkdf_helper.cpp`; raw WAL files must not be readable without the cluster key.
- [ ] `orphan_detector.cpp` must enforce a maximum lock age policy to prevent indefinite Percolator lock accumulation from coordinator failures.
- [ ] Shard topology changes must be signed by the admin key (managed via `utils/lek_manager.cpp`) before acceptance by `shard_topology.cpp` to prevent unauthorized repartitioning.
- [?] Clarify whether cross-shard transaction logs containing legal case document keys require field-level encryption before WAL persistence.
- [ ] `ShardRepairEngine` must validate Reed-Solomon parity checksums before writing repaired data to prevent silent data corruption from propagating.

---

### [x] Hardware Node Migration Support
**Priority:** High
**Target Version:** v1.5.0 (Beta complete — Phase 5 hardening planned Q3 2026)

#### Scope

Handle the full lifecycle of migrating a ThemisDB shard node to replacement hardware without disrupting the consistent hash ring, consensus quorum, or client request routing.

#### Design Constraints

- The consistent hash ring is built from logical **shard IDs**, not from hostnames or IP addresses. Consequently, moving a node to new hardware (new endpoint) does **not** affect ring positions as long as the shard ID is preserved.
- A `NodeIdentity` file (`node_identity.json`) on the node's persistent storage stores the stable logical shard identity (shard\_id, cluster\_name, token\_start, token\_end). This file **must be copied** to the new hardware before initiating migration.
- Endpoint replacement must be **atomic at the topology level**: a single `ShardTopology::addShard()` upsert switches the primary endpoint; no intermediate state exists where the shard is reachable at neither old nor new endpoint.
- During the drain period (configurable, default 60 s) the old hardware should continue serving read-only traffic until all in-flight operations complete. Active drain is planned for Phase 5.

#### Required Interfaces

| Interface | File | Notes |
|-----------|------|-------|
| `HardwareMigrationManager::replaceEndpoint()` | `src/sharding/hardware_migration_manager.cpp` | Atomic endpoint update; ring positions unchanged |
| `HardwareMigrationManager::createAndSaveIdentity()` | same | Provisions a fresh NodeIdentity file on first boot |
| `HardwareMigrationManager::loadIdentity()` | same | Loads identity for new hardware to confirm correct shard_id |
| `HardwareMigrationManager::validateRingStability()` | same | Post-migration assertion |
| `NodeIdentity::saveTo() / loadFrom()` | same | JSON persistence |

#### Implementation Notes

- `HardwareMigrationManager::replaceEndpoint()` holds `mutex_` during the entire update to prevent concurrent topology changes.
- If `verify_ring_stability` is enabled, a snapshot is captured before and after the update; any detected ring change rolls back the topology update and returns an error.
- The `ConsistentHashRing` is **read-only** during `replaceEndpoint()`; the method never calls `addShard()` or `removeShard()` on the ring.
- Raft peer-address update (so the consensus layer knows the new endpoint) is not yet automated; operators must currently update the Raft configuration manually via `RaftConfiguration::addNode()` / `removeNode()` followed by `commitConfiguration()`. Automation is planned in Phase 5.

#### Migration Runbook

1. **Provision new hardware** and install ThemisDB.
2. **Copy** the `node_identity.json` file from the old node to the same path on the new node.
3. **Start ThemisDB** on the new hardware. It reads the identity file and registers itself with the cluster under the same shard\_id.
4. **Invoke** `replaceEndpoint(shard_id, new_endpoint)` on any cluster node (or via the future admin API).
5. **Verify** the topology update with `GET /api/v1/shards/{shard_id}` and confirm the ring is stable.
6. **Decommission** the old hardware after the drain period.

#### Test Strategy

| Test | Coverage |
|------|----------|
| `NodeIdentityJsonRoundTrip` | JSON serialise/deserialise |
| `NodeIdentityDiskRoundTrip` | File write then read |
| `ReplaceEndpointSucceeds` | Happy path: endpoint updated, ring unchanged |
| `ReplaceEndpointPreservesOtherFields` | Datacenter, rack, token range unchanged |
| `ReplaceEndpointFailsForUnknownShard` | Error path |
| `ReplaceEndpointFailsForEmptyNewEndpoint` | Validation |
| `RingPositionsUnchangedAfterEndpointReplacement` | Core guarantee |
| `MultipleEndpointReplacementsKeepRingStable` | Repeated migrations |

#### Performance Targets

- `replaceEndpoint()` latency: <1 ms for in-memory topology (no etcd write); <10 ms with etcd write.
- `NodeIdentity::loadFrom()` latency: <5 ms on local NVMe (file size ~200 bytes).
- Topology propagation to all cluster nodes: ≤500 ms (existing gossip SLO).

#### Security / Reliability

- The `node_identity.json` file must be protected with filesystem permissions `0600` to prevent unauthorised reading of the shard assignment.
- Hardware migration requests from the admin API must require a signed operator certificate validated by the cluster CA.
- The `createAndSaveIdentity()` method refuses to overwrite an existing identity file to prevent accidental re-assignment of a shard.

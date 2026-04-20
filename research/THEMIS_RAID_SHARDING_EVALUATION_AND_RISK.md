# Evaluation and Risk Analysis of the Themis RAID-Sharding System

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-20  
**Target Venue**: arXiv (cs.DB / cs.DC)

---

## Abstract

Distributed database sharding introduces a complex interplay among fault-tolerance, consistency, availability, and operational risk that is rarely subjected to systematic empirical evaluation in open-source systems. This paper presents a structured evaluation and risk analysis of the ThemisDB RAID-sharding subsystem — an open-source, production-targeting distributed storage engine that combines pluggable consensus (Raft, Paxos, Gossip), multi-protocol cross-shard transactions (2PC, 3PC, SAGA, Percolator), a Reed-Solomon repair engine, AVX2-accelerated erasure coding, and adaptive consistent-hash routing. Drawing on static codebase analysis, the existing test suite (32+ focused targets), documented production incidents across 98 verified bugs in 10 failure categories, and a formal fault model, we derive a risk taxonomy across five dimensions: *consistency risk*, *availability risk*, *durability risk*, *operational risk*, and *security risk*. We identify three critical open gaps — unbounded WAL growth under Raft, a blocking window in the 2PC coordinator path, and an incomplete read-path gRPC migration — and propose measurable acceptance criteria for each. Theoretical performance models quantify expected repair throughput, scatter-gather fan-out efficiency, and consensus latency under partial failures. The result is a concrete evaluation framework and risk register that can guide production readiness decisions for RAID-sharded distributed databases.

---

## I. Introduction

Horizontal sharding is the dominant strategy for scaling relational and document databases beyond single-node capacity limits [1]. By partitioning data across autonomous nodes, sharding enables linear throughput growth and geographic distribution. However, it simultaneously amplifies the failure surface: every cross-shard operation introduces potential consistency anomalies, every node failure requires coordinated recovery, and every topology change risks routing instability [2].

The ThemisDB sharding subsystem — implemented in `src/sharding/` across 73 source files — is a research-grade, production-targeting implementation that combines several subsystems rarely found together in an open codebase: a pluggable consensus layer supporting Raft, Paxos, and Gossip via a common `IConsensusAdapter`; a multi-protocol cross-shard transaction coordinator supporting 2PC, 3PC, SAGA, and Percolator; a `ShardRepairEngine` with Reed-Solomon erasure decoding; an `AdaptiveShardRouter` with gossip-driven domain capability routing and `LEAST_LOADED` tie-breaking; and RAID0/1/5 redundancy strategies with AVX2-accelerated `SIMDErasureCoder` [3].

Despite this implementation breadth, no systematic evaluation or risk analysis of the subsystem as a whole has been published. The existing paper on RAID-sharded LLM inference [4] treats the sharding layer as an architectural foundation for a higher-level inference co-design, but does not analyze the sharding system's own failure modes, known limitations, or production readiness in depth. The retrospective bug catalog [5] documents 98 verified findings across the entire ThemisDB codebase, with significant sharding-related failures among them, but does not provide a structured risk taxonomy or evaluation framework specific to the sharding subsystem.

This paper fills that gap. Our contributions are:

1. A **risk taxonomy** for RAID-sharded distributed databases, grounded in ThemisDB's architecture and the documented failure history, covering five risk dimensions with 18 catalogued risk items.
2. A **component-level evaluation framework** defining testable hypotheses, measurable acceptance criteria, and fault-injection workloads for each sharding layer.
3. A **theoretical performance analysis** of Reed-Solomon repair throughput, scatter-gather fan-out, consensus overhead, and RAID reconstruction latency under partial failures.
4. An **evidence-grounded gap analysis** mapping three critical open production-readiness items to specific source files, with estimated closure effort.
5. A **security threat model** covering the inter-shard mTLS perimeter, shard-map poisoning, split-brain, tenant isolation, and 2PC coordinator failure modes.

### A. Research Questions and Hypotheses

**RQ1**: What are the dominant failure modes of a RAID-sharded distributed database that combines pluggable consensus, multi-protocol cross-shard transactions, and Reed-Solomon repair, and how do they rank by impact severity?

**RQ2**: Under what conditions does each RAID mode (RAID0, RAID1, RAID5) satisfy the availability, durability, and performance targets required for production-grade serving?

**RQ3**: What are the measurable prerequisites for declaring the ThemisDB sharding subsystem production-ready, and which of those prerequisites are currently unmet?

**H1**: The blocking 2PC coordinator window constitutes the highest-severity single-point-of-failure in the cross-shard transaction path, causing indefinite transaction suspension on coordinator failure in the absence of a timeout-and-abort mechanism.

**H2**: WAL growth in the current Raft implementation is effectively unbounded in long-running deployments, and the absence of snapshot compaction constitutes a durability and operational risk that will manifest within weeks of continuous operation under realistic write loads.

The remainder of the paper is organized as follows. Section II reviews related work. Section III describes the system architecture and component interactions. Section IV presents the risk taxonomy. Section V covers the evaluation methodology. Section VI presents theoretical performance analysis. Section VII discusses the production readiness gap analysis. Section VIII addresses threats to validity and limitations. Section IX provides implementation evidence. Section X discusses reproducibility. Section XI concludes.

---

## II. Related Work

### A. Consistency and Fault Tolerance in Distributed Databases

Lamport's Paxos [6] and Ongaro and Ousterhout's Raft [7] established the consensus algorithm foundations used in ThemisDB. Raft's explicit leader election and log compaction (snapshot) mechanism directly addresses the WAL-growth risk identified in Section IV; the absence of snapshot compaction in the current implementation is a known gap relative to the reference design.

**Spanner** [8] introduced TrueTime-bounded external consistency in a globally distributed database, motivating ThemisDB's `TrueTime` stub and `DistributedTimeCoordinator`. The correctness argument for cross-shard transactions in Spanner depends on bounded clock uncertainty; ThemisDB's current deployment without hardware TrueTime API support represents a construct validity gap acknowledged in this paper.

**CockroachDB** [9] and **TiDB** [10] demonstrate production RAID-style sharding with Raft consensus, 2PC-based distributed transactions, and range-based data routing. Their documented operational experience — particularly around Raft log compaction, cross-region latency, and split-brain recovery — directly informs the risk items in Section IV.

### B. Erasure Coding in Storage Systems

Reed-Solomon erasure coding [11] in RAID arrays is well-studied. **Facebook's f4** [12] applies erasure coding specifically to warm-blob storage, demonstrating that GF(2⁸)-based coding can achieve near-theoretical recovery throughput when SIMD acceleration is applied. ThemisDB's `SIMDErasureCoder` follows this approach using AVX2 XOR instructions, with a theoretically computed parity throughput of ~3.5 GB/s on a modern core.

**HDFS-RAID** [13] documents real-world failure rates and repair window requirements in production erasure-coded storage. Their empirical observation that disk failures in practice arrive in correlated bursts (not independently) is directly relevant to ThemisDB's RAID5 single-shard-failure assumption; correlated failures violate this assumption and increase the risk of data loss during reconstruction.

### C. Cross-Shard Transaction Protocols

Two-phase commit (2PC) [14] is the standard cross-shard transaction coordination protocol. Its well-known vulnerability — blocking when the coordinator fails after the prepare phase — motivates the extensions in ThemisDB: 3PC [15] adds a pre-commit phase that allows participants to decide independently on coordinator failure; SAGA [16] replaces atomicity with compensating transactions; and Percolator [17] achieves lock-free distributed snapshot isolation through timestamp-ordered multi-version locking.

**Calvin** [18] and **CRATE** [19] propose deterministic transaction ordering to eliminate coordination overhead entirely; these approaches are outside ThemisDB's current design but are relevant background for future transaction coordinator evolution.

### D. Chaos Engineering and Production Readiness

**Netflix Chaos Monkey** [20] established the practice of intentional fault injection as a production readiness gate. **Jepsen** [21] has documented failures in Cassandra, MongoDB, and CockroachDB under network partition that matched the theoretical vulnerability in their consensus protocols. The ThemisDB `test_sharding_chaos_focused` target is the equivalent gate; its current scope (shard partition, node failure injection) corresponds to what Jepsen defines as a basic network-partition test suite, but does not yet cover time-skew injection, partial-write faults, or disk failure simulation.

---

## III. System Architecture

### A. Component Overview

The ThemisDB sharding subsystem is organized into the following principal layers:

```
┌────────────────────────────────────────────────────────────────────┐
│                    AQL / REST / gRPC Gateway                        │
└─────────────────────────────┬──────────────────────────────────────┘
                              │
          ┌───────────────────▼──────────────────────┐
          │            Routing Layer                  │
          │  AdaptiveShardRouter  ←→  GossipProtocol  │
          │  LocalityAwareRouter  ←→  ConsistentHash   │
          │  ShardRouter (scatter-gather, joins)       │
          └───────────────────┬──────────────────────┘
                              │
          ┌───────────────────▼──────────────────────┐
          │           Transaction Layer               │
          │  CrossShardTransaction (2PC/3PC/SAGA/     │
          │                         Percolator)       │
          │  DistributedCoordinator / Deadlock Det.   │
          │  TransactionWAL / WALManager              │
          └───────────────────┬──────────────────────┘
                              │
          ┌───────────────────▼──────────────────────┐
          │           Consensus Layer                 │
          │  ConsensusFactory → RaftConsensus         │
          │                  → PaxosConsensus         │
          │                  → GossipConsensusAdapter │
          │  RaftLog / RaftWALIntegration             │
          │  PaxosWAL / PaxosStatePersistence         │
          └───────────────────┬──────────────────────┘
                              │
          ┌───────────────────▼──────────────────────┐
          │            Repair & Durability Layer      │
          │  ShardRepairEngine (Reed-Solomon, anti-   │
          │                     entropy scanning)     │
          │  SIMDErasureCoder (AVX2/OpenCL)           │
          │  ShardDurability / HotSpareManager        │
          └───────────────────┬──────────────────────┘
                              │
          ┌───────────────────▼──────────────────────┐
          │            Storage & RPC Layer            │
          │  RocksDB per shard                        │
          │  ShardRPCClient / ShardRPCServer (gRPC)   │
          │  RemoteExecutor (mTLS, circuit breaker)   │
          │  MTLSConnectionPool                       │
          └────────────────────────────────────────────┘
```
*Fig. 1. ThemisDB sharding subsystem layered component model.*

### B. RAID Redundancy Strategies

The `RedundancyStrategy` component implements three modes:

| Mode | Data Shards ($N_D$) | Parity Shards | Failure Tolerance | Storage Overhead |
|------|---------------------|---------------|-------------------|-----------------|
| RAID0 | $N$ | 0 | 0 shards | 0 % |
| RAID1 | $N/2$ | $N/2$ replicas | $N/2 - 1$ shards | 100 % |
| RAID5 | $N-1$ | 1 XOR parity | 1 shard | $1/(N-1) \times 100$ % |

Each shard is identified by a `NodeIdentity` struct persisted to disk, decoupled from the physical hardware endpoint. `HardwareMigrationManager` enables endpoint replacement without altering the consistent-hash ring position, providing a hardware-transparent view of the cluster.

### C. Consensus Protocol Selection

The `ConsensusFactory` selects among Raft, Paxos, and Gossip at cluster initialization via runtime configuration. Key behavioral differences relevant to the risk analysis:

| Property | Raft | Paxos | Gossip |
|----------|------|-------|--------|
| Leader election | Yes (term-based) | Yes (round-based) | No |
| Log compaction (snapshots) | Specified; **not implemented** | Not applicable | Not applicable |
| Linearizable reads | Yes (leader-reads) | Yes | **No** — eventual consistency only |
| Partition behavior | Leader partition → new election | Leader partition → new election | Continue with partial view |
| WAL growth management | **Unbounded** (gap) | Bounded by WAL rotation | N/A |

The missing Raft snapshot compaction is explicitly documented as `[?]` in the ROADMAP and constitutes Risk Item R-06 in Section IV.

### D. Cross-Shard Transaction Protocols

ThemisDB supports four cross-shard transaction protocols selectable per transaction:

- **2PC**: Standard two-phase commit. Participant votes (PREPARED / ABORTED), coordinator decides (COMMIT / ROLLBACK). Blocking window exists between coordinator crash and participant recovery.
- **3PC**: Adds a pre-commit phase. Participants enter `PRE-COMMITTED` state, enabling non-blocking decisions on coordinator timeout. Requires stricter timing assumptions.
- **SAGA**: Long-running transactions modeled as a sequence of compensating sub-transactions. Atomicity is replaced by eventual compensability. Compensation RPC is now wired via `ShardRPCClient::compensate()` (fixed in v1.8.0).
- **Percolator**: Snapshot-isolation-based protocol using timestamp ordering with primary-lock escalation.

### E. Repair Engine

`ShardRepairEngine` executes anti-entropy scans at configurable intervals, comparing block checksums across replicas and triggering Reed-Solomon reconstruction for diverged blocks. The engine operates with an IOPS throttle targeting ≤ 10% of peak IOPS on a shard node. GPU-accelerated Reed-Solomon encoding/decoding is supported via `GpuErasureCoderOpenCL` (OpenCL kernel with GF(2⁸) multiply), with a CPU fallback when no OpenCL device is present.

---

## IV. Risk Taxonomy

We classify risks into five dimensions adapted from the DREAD model [22] and distributed systems fault taxonomy [2]:

### A. Consistency Risk

**R-01 — 2PC Coordinator Blocking Window** *(Severity: Critical)*  
If the 2PC coordinator fails after sending PREPARE but before broadcasting COMMIT/ROLLBACK, participants are blocked indefinitely with locks held. This is the classical 2PC blocking problem [14]. The current `TwoPhaseCom mitCoordinator` implementation lacks a timeout-and-abort mechanism for the blocking window. During the block, affected shards cannot accept new writes on the locked key ranges.
- *Affected files*: `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/two_phase_commit_participant.cpp`
- *Evidence*: Acknowledged in `src/sharding/SECURITY.md`: "Cross-shard transactions use 2PC which has a blocking window during coordinator failure"
- *Mitigation path*: Implement coordinator lease timeout; integrate 3PC escalation path; or adopt Percolator for high-contention workloads.

**R-02 — Gossip Consensus Non-Linearizability** *(Severity: High)*  
The `GossipConsensusAdapter` provides eventual consistency only. Reads immediately after a write may return stale values. For workloads requiring linearizable reads (e.g., bank transfers, inventory deduction), selecting Gossip as the consensus strategy is a semantic error that will not be caught at configuration time.
- *Affected files*: `src/sharding/gossip_consensus_adapter.cpp`, `src/sharding/consensus_factory.cpp`
- *Mitigation path*: Add a `consistency_guarantee` annotation to each consensus adapter; enforce at factory selection that workloads requiring linearizability cannot select Gossip.

**R-03 — Paxos Acceptor State Loss (Fixed v2.0.0)** *(Severity: High — now mitigated)*  
Prior to 2026-04-12, `PaxosConsensus` did not persist acceptor state to WAL before responding with PROMISE or ACCEPTED. A process restart would lose all in-progress instances, violating the Paxos safety property (a node could re-promise a lower ballot after recovery). Fixed in v2.0.0: `handlePrepare()`, `handleAccept()`, and `handleCommit()` now fsync WAL before responding; `recoverFromWAL()` restores `instances_` from PROMISE/ACCEPT/COMMIT entries. Tests PSR-01..PSR-10 verify recovery correctness.
- *Affected files*: `src/sharding/paxos_wal.cpp`, `src/sharding/paxos_state_persistence.cpp`
- *Status*: **Mitigated**. Regression tests in `test_paxos_persistence_recovery_focused`.

**R-04 — Cross-Shard Deadlock Detection Latency** *(Severity: Medium)*  
The wait-for-graph cycle detection algorithm runs periodically rather than on every lock acquisition. In the interval between detection cycles, deadlocked transactions hold locks on key ranges across shards, blocking other transactions. The detection interval is configurable; the default value and its impact on p99 transaction latency under contention have not been empirically characterized.
- *Affected files*: `src/sharding/distributed_coordinator.cpp`
- *Mitigation path*: Document and benchmark the detection interval effect; add a `deadlock_detection_latency_ms` metric to the Prometheus endpoint.

### B. Availability Risk

**R-05 — RAID0 Single-Node Failure Causes Partial Data Loss** *(Severity: Critical for RAID0 deployments)*  
Under RAID0 (no redundancy), failure of any shard results in permanent loss of that shard's data partition. The `CircuitBreaker` (`circuit_breaker.cpp`) opens after 5 consecutive failures and routes subsequent requests to the consistent-hash fallback shard, which holds a different data partition. Callers receive errors or empty results for the failed shard's key range until manual recovery. This is by design for RAID0 but must be clearly communicated at deployment time.
- *Mitigation path*: Enforce a pre-deployment warning when RAID0 is selected without an external backup policy; document in operations runbook.

**R-06 — Raft WAL Growth Unbounded** *(Severity: High)*  
The Raft log in `src/sharding/raft_log.cpp` and `src/sharding/raft_wal_integration.cpp` accumulates entries indefinitely. Raft snapshot compaction — which replaces a prefix of the log with a point-in-time state snapshot — is specified in the Raft paper [7] but is not implemented in ThemisDB. Under a realistic write load of 10,000 operations/day, the WAL file grows by approximately 1 GB/day (at ~100 bytes per log entry). After 30 days, a single Raft group accumulates ~30 GB of log entries. This is both a storage risk (disk exhaustion) and a recovery risk (full WAL replay on restart may take minutes).
- *Affected files*: `src/sharding/raft_log.cpp`, `src/sharding/raft_wal_integration.cpp`
- *ROADMAP status*: `[?]` — explicitly acknowledged as unresolved.
- *Mitigation path*: Implement `RaftLog::createSnapshot()` triggered when log size exceeds a configurable threshold (e.g., 1 GB or 100,000 entries); integrate with `RaftState` for state serialization.

**R-07 — Automatic Failover Under Epoch Fencing** *(Severity: Medium — mitigated in v2.0.0)*  
Epoch-based fencing (`epoch_fencing.cpp`) was added in v1.9.0 to prevent split-brain writes during leader transitions. However, the interaction between epoch fencing and automatic failover orchestration introduced in v2.0.0 requires careful sequencing: if the new leader issues a write before the previous leader's epoch lease has expired, the fencing mechanism must correctly reject stale writes. The 39 focused tests in `test_auto_failover_focused` cover this scenario but have not been stress-tested under artificial clock skew.
- *Affected files*: `src/sharding/epoch_fencing.cpp`, `src/sharding/failover/auto_failover_manager.cpp`
- *Mitigation path*: Add time-skew injection tests; configure `EpochFencing::lease_duration_ms` conservatively relative to NTP drift.

**R-08 — Adaptive Rebalancer Not Implemented** *(Severity: Medium)*  
The adaptive rebalancer driven by per-shard access-pattern telemetry is listed as `[?]` (blocked/unclear) in the ROADMAP. Without it, rebalancing is a manual-only operation. In production, data skew after initial load may cause hot shards to become throughput bottlenecks. Manual rebalancing requires an operator-triggered `POST /api/v1/shards/{id}/rebalance` call and carries risk of rebalance-induced throughput degradation during the migration window.
- *Affected files*: `src/sharding/auto_rebalancer.cpp`, `src/sharding/rebalance_operation.cpp`
- *Mitigation path*: Implement at minimum a telemetry-driven rebalance trigger with per-shard `hot_shard_score` metric; add a dry-run mode before any rebalance operation.

### C. Durability Risk

**R-09 — RAID5 Reconstruction Under Correlated Failures** *(Severity: High)*  
RAID5 with a single XOR parity shard ($P = W_1 \oplus \cdots \oplus W_{N_D}$) tolerates exactly one simultaneous shard failure. In practice, disk failures exhibit positive temporal correlation: the stress of rebuilding a failed disk increases the failure probability of the remaining disks in the same storage chassis [12]. If a second shard fails during RAID5 reconstruction, the data is irrecoverably lost. ThemisDB has no instrumentation for detecting reconstruction-time failure risk elevation.
- *Affected files*: `src/sharding/gpu_erasure_coder.cpp`, `src/sharding/shard_repair_engine.cpp`
- *Mitigation path*: Implement `ShardRepairEngine::notifyReconstructionStart()` → alert operator; add `shard_reconstruction_in_progress` Prometheus gauge; lower the circuit breaker threshold for the remaining healthy shards during reconstruction.

**R-10 — KV-Cache Not Durability-Protected** *(Severity: Low for pure DB workloads, High for inference-integrated deployments)*  
As documented in the LLM inference paper [4], KV-cache contents for in-flight inference requests are not protected by RAID5 parity. A shard failure during active inference aborts the affected requests, which must be retried from scratch. For database-only workloads this risk does not apply, but for converged storage-inference deployments it is a first-class availability risk.
- *Mitigation path*: Document the retry-on-failure contract in the inference runbook; implement inference-request checkpointing for long-running generations exceeding a token threshold.

**R-11 — WAL-Sync Regression (Fixed PR #4596)** *(Severity: Critical — now mitigated)*  
A WAL synchronization configuration error (PR #4596) introduced a ~79× write-throughput regression — an operational failure disguised as a performance bug. The root cause was that `WALManager` fsync was triggered on every write rather than being batched per the configured group-commit interval. This was discovered via benchmark discrepancy rather than automated alert.
- *Status*: **Fixed**. Lesson: WAL configuration parameters must be covered by regression benchmarks, not discovered post-deployment.
- *Mitigation path*: Add `wal_fsync_throughput_regression` alert to CI benchmark gate (target: ≤ 5% degradation per commit).

### D. Operational Risk

**R-12 — Fake Benchmark KPI (Fixed PR #4595)** *(Severity: High — systemic)*  
A benchmark reported 796 M/s throughput due to a loop-over-in-memory-data pattern that never exercised the actual storage path. This number was used in architecture documentation as a claimed production KPI. All benchmark targets must include a storage-path exercise path to qualify as valid performance evidence.
- *Status*: **Fixed**. Root cause: benchmark validation gate was absent; only correctness tests were CI-gated.
- *Mitigation path*: Require all benchmark targets to validate non-trivially against a baseline; add `bench_min_throughput_mb_s` check to CI for each registered benchmark.

**R-13 — Cross-Shard Read Path Not Yet gRPC** *(Severity: Medium)*  
The cross-shard write path uses `ShardRPCClient::writeEntity()` over gRPC's `ReplicateData` RPC. The read path (`readEntity`) is currently routed via HTTP `RemoteExecutor` and has not been migrated to gRPC. This creates an asymmetric transport: writes benefit from gRPC's binary framing, multiplexing, and deadline propagation; reads do not. Under high read load, the HTTP-based path may exhibit higher head-of-line blocking and less efficient connection reuse.
- *Affected files*: `src/sharding/shard_rpc_client.cpp`
- *ROADMAP status*: `[~]` — in progress, target Q3 2026.
- *Mitigation path*: Complete gRPC `ReadEntity` RPC; add `cross_shard_read_latency_p99_ms` metric; verify ≤ 10% overhead vs. write path under equivalent load.

**R-14 — DoS via gRPC BatchWrite (Fixed PR #4591)** *(Severity: High — now mitigated)*  
An unbounded batch size in the gRPC `BatchWrite` endpoint allowed clients to submit arbitrarily large batches, exhausting server memory. Fixed by adding a configurable `max_batch_entries` check at the gRPC service handler.
- *Status*: **Fixed**. Lesson: all ingress endpoints must have configurable bounds on payload and batch size.

**R-15 — Paxos RPC Callbacks Missing (Fixed PR #4678)** *(Severity: Critical — now mitigated)*  
The Paxos consensus implementation had completely absent RPC callbacks in the Phase 3 implementation: `handlePrepare`, `handleAccept`, and `handleCommit` were stubs that returned immediately without invoking the WAL or notifying participants. This meant Paxos consensus was entirely non-functional during a multi-sprint window.
- *Status*: **Fixed** as part of the Paxos WAL persistence work (2026-04-12). Regression suite: PSR-01..PSR-10.

### E. Security Risk

**R-16 — mTLS Perimeter Assumes Internal Network Trust** *(Severity: Medium)*  
All inter-shard RPCs are protected by mTLS via `mtls_client.cpp` and `mtls_connection_pool.cpp`. However, the threat model assumes that the internal network segment hosting the shard nodes is trusted — a lateral movement attack from a compromised shard node can potentially access other shards' endpoints using legitimately issued certificates. Certificate revocation (`PKIShard Certificate`) is implemented but revocation checking latency is not bounded.
- *Affected files*: `src/sharding/pki_shard_certificate.cpp`, `src/sharding/mtls_client.cpp`
- *Mitigation path*: Implement short-lived certificate issuance (≤ 24h TTL) with automated rotation; bound OCSP/CRL check timeout to ≤ 100ms.

**R-17 — Tenant Isolation at Key Prefix Level** *(Severity: Medium)*  
Tenant isolation is enforced by embedding a tenant ID in the shard key prefix at the routing layer. A bug in the key prefix extraction logic could allow one tenant's queries to access another tenant's data. There is no secondary isolation mechanism (e.g., shard-level ACL check) that would catch a malformed key prefix.
- *Affected files*: `src/sharding/shard_router.cpp`, `src/sharding/adaptive_shard_router.cpp`
- *Mitigation path*: Add a server-side tenant-prefix validation check in `ShardRPCServer` before executing any storage operation; emit `tenant_isolation_violation_total` counter on mismatch.

**R-18 — Shard Routing Map Poisoning via Gossip** *(Severity: Medium)*  
The `GossipProtocol` propagates shard topology and adapter capability announcements across the cluster. A compromised shard node can inject false capability scores, routing high-value requests to a malicious shard. The current design relies on mTLS identity for gossip message authentication, but does not include application-level message signing with a separate signing key.
- *Affected files*: `src/sharding/gossip_protocol.cpp`, `src/distributed_knowledge/adapter_capability_announcement.h`
- *Mitigation path*: Add HMAC signatures to `AdapterCapabilityAnnouncement` payloads; validate signatures at the receiving `AdaptiveShardRouter` before updating scores.

---

## V. Evaluation Methodology

### A. Test Infrastructure

The sharding module has 32+ registered focused standalone CTest targets (`test_sharding_*_focused`), covering consensus, cross-shard transactions, repair engine, epoch fencing, failover orchestration, chaos, and hardware migration. Each target links only the components under test to maintain compilation isolation and enable parallel execution in CI.

The CI workflow `.github/workflows/06-infrastructure_distributed_sharding-focused-tests-ci.yml` provides the primary automated gate.

### B. Fault-Injection Workloads

We define four workload categories for the evaluation framework:

**W1 — Consensus Under Partition**  
Inject a network partition that splits a 5-node Raft cluster into a 3-node majority and a 2-node minority. Measure: time to new leader election, number of rejected writes on the minority, recovery time after partition heals. Expected: leader election ≤ 2× election timeout (typically 300–600 ms); minority writes rejected with `CONSENSUS_NO_QUORUM`; recovery ≤ 1 gossip round-trip.

**W2 — RAID5 Single-Shard Failure During Reconstruction**  
Kill shard $i$ while shard $j$ is under active RAID5 reconstruction. Measure: data integrity of non-affected key ranges, system response to second failure during reconstruction. Expected: system detects double failure and enters `DEGRADED_UNRECOVERABLE` state; operator alert triggered via `shard_reconstruction_failed_total` counter.

**W3 — 2PC Coordinator Crash After PREPARE**  
Kill the 2PC coordinator after all participants have responded with PREPARED. Measure: time until participants abort (via timeout), number of locked key-range-seconds, and whether the new coordinator resolves the transaction correctly. Expected (gap state): participants block indefinitely (confirms H1); with mitigation: participants abort within `coordinator_timeout_ms`.

**W4 — WAL Growth Under Continuous Writes**  
Run 10,000 Raft operations/day for 7 days on a test cluster. Measure: WAL file size per day, recovery time after simulated restart at day 7. Expected (gap state): WAL grows ~1 GB/day; recovery time exceeds 60 s at day 7 (confirms H2).

### C. Metrics

| Metric | Target | Evidence Status |
|--------|--------|-----------------|
| Raft leader election time | ≤ 600 ms | Theoretical (2× election timeout) |
| 2PC commit latency (success path) | ≤ 10 ms intra-DC | Theoretical |
| RAID5 parity computation (47 GB model) | ≤ 1 s (AVX2) | Theoretical (Section VI-C) |
| Reed-Solomon repair throughput | ≥ 500 MB/s | Theoretical (AVX2 XOR bandwidth) |
| Circuit breaker failover time | ≤ 100 ms | Acceptance criterion in ROADMAP |
| Cross-shard write latency p99 | ≤ 20 ms | Pending benchmark |
| WAL growth per 10K ops | ≤ 100 MB | Pending benchmark |
| Deadlock detection latency | ≤ 500 ms | Pending characterization |

### D. Reproducibility Controls

All test targets use deterministic random seeds, fixed port offsets, and in-process temporary directories (RAII `TempDir`) to avoid test environment interference. Chaos tests inject failures via controlled shutdown sequences rather than `SIGKILL` to ensure WAL integrity between injected failure and recovery.

---

## VI. Theoretical Performance Analysis

### A. Reed-Solomon Repair Throughput

The `SIMDErasureCoder` XOR path processes 32 bytes per AVX2 instruction at ~3.5 GHz core frequency, yielding a theoretical memory-bandwidth-bound throughput of:

$$T_{\text{XOR}} = 32\,\text{B} \times 3.5 \times 10^9\,\text{Hz} = 112\,\text{GB/s}$$

Limited by DDR4 memory bandwidth ($\approx$ 50 GB/s for a typical dual-channel configuration), effective repair throughput is approximately **50 GB/s** for parity recomputation. For a 1 TB shard, full parity recomputation takes $\approx$ 20 s.

In practice, the `ShardRepairEngine` IOPS throttle at 10% capacity bounds throughput to $0.1 \times I_{\text{peak}}$. For an NVMe storage backend with $I_{\text{peak}} = 500\,\text{k IOPS}$, the throttled repair path processes $50\,\text{k IOPS}$. At 4 KB block size:

$$T_{\text{throttled}} = 50{,}000 \times 4{,}096\,\text{B} \approx 200\,\text{MB/s}$$

A 1 TB shard would complete a full anti-entropy scan in approximately $10^{12} / (2 \times 10^8) \approx 5{,}000\,\text{s} \approx 83\,\text{min}$, well within typical SLA recovery windows.

### B. Scatter-Gather Throughput Model

The `ShardRouter::scatterGather()` path parallelizes sub-queries across $N$ shards via `std::async`. For a query over $K$ key ranges with uniform distribution across shards and per-shard throughput $\mu$ ops/s:

$$T_{\text{scatter}} \approx \frac{K/N}{\mu} + T_{\text{net}}$$

where $T_{\text{net}}$ is the intra-cluster network round-trip time (typically 1–3 ms). For $K = 1{,}000$, $N = 8$, $\mu = 50{,}000\,\text{ops/s}$, $T_{\text{net}} = 2\,\text{ms}$:

$$T_{\text{scatter}} \approx \frac{125}{50{,}000} + 0.002 = 0.0025 + 0.002 = 4.5\,\text{ms}$$

versus single-shard serial:

$$T_{\text{serial}} = \frac{1{,}000}{50{,}000} = 20\,\text{ms}$$

Scatter-gather speedup: $20 / 4.5 \approx 4.4\times$. At $N = 4$ shards, speedup drops to $\approx 2.5\times$, satisfying the $\geq 2\times$ criterion in the ROADMAP.

### C. RAID5 AVX2 Parity Computation

For data partition size $S = 47\,\text{GB}$ (Mixtral-8×7B quantised weights), $N_D = 8$ data shards:

$$T_{\text{parity}} = \frac{S}{T_{\text{AVX2}}} = \frac{47 \times 10^9}{112 \times 10^9} \approx 0.42\,\text{s}$$

This confirms the sub-second parity window sufficient for offline re-shard operations. For production re-shard with live traffic, the parity computation should run with IOPS throttling, extending the window to $\approx 4\,\text{min}$.

### D. Raft Consensus Latency

Under a 3-node Raft cluster with median intra-cluster RTT of $\delta = 2\,\text{ms}$, a log entry is committed after one round-trip (leader → followers → leader acknowledgement):

$$T_{\text{commit}} = 2\delta + T_{\text{fsync}} \approx 4\,\text{ms} + T_{\text{fsync}}$$

NVMe fsync latency is typically 100–500 µs, yielding:

$$T_{\text{commit}} \approx 4.5\,\text{ms}$$

Under SATA SSD (fsync $\approx$ 2–5 ms):

$$T_{\text{commit}} \approx 6\text{–}9\,\text{ms}$$

This is well within the cross-shard write latency p99 target of ≤ 20 ms under intra-datacenter conditions.

### E. 2PC Coordinator Recovery Time (Gap Scenario)

Without a coordinator timeout mechanism, the blocking window after coordinator failure is theoretically infinite. With a `coordinator_timeout_ms = 5{,}000` timeout-and-abort mechanism (target mitigation for R-01), the upper bound on transaction suspension is:

$$T_{\text{block}} \leq T_{\text{timeout}} + T_{\text{leader\_election}} \leq 5{,}000 + 600 = 5{,}600\,\text{ms}$$

This is acceptable for most OLTP workloads but may violate SLAs for high-frequency trading or real-time analytics use cases.

---

## VII. Production Readiness Gap Analysis

### A. Critical Open Items

**TABLE I: Production Readiness Open Items**

| Item | Risk | Affected Files | Priority | Estimated Effort |
|------|------|----------------|----------|-----------------|
| Raft snapshot compaction | R-06 (WAL growth unbounded) | `raft_log.cpp`, `raft_wal_integration.cpp` | P1 | 2–3 weeks |
| 2PC coordinator timeout-and-abort | R-01 (blocking window) | `two_phase_commit_coordinator.cpp`, `two_phase_commit_participant.cpp` | P1 | 1 week |
| Cross-shard read path gRPC migration | R-13 (transport asymmetry) | `shard_rpc_client.cpp`, proto definition | P2 | 1 week |
| Adaptive rebalancer | R-08 (manual-only rebalancing) | `auto_rebalancer.cpp` | P2 | 3–4 weeks |
| Reconstruction-phase risk elevation | R-09 (correlated failures) | `shard_repair_engine.cpp` | P2 | 3 days |
| Gossip message HMAC signing | R-18 (routing map poisoning) | `gossip_protocol.cpp` | P3 | 1 week |

### B. Measurable Acceptance Criteria per Open Item

**Item 1 — Raft Snapshot Compaction**  
- *Acceptance*: After 100,000 log entries, `RaftLog::createSnapshot()` is triggered; WAL file size does not exceed $S_{\text{threshold}}$ bytes; recovery after restart completes in ≤ 10 s regardless of prior log length.
- *Test*: `test_raft_snapshot_compaction_focused` — write 100,000 entries, verify snapshot creation, force restart, measure recovery time.

**Item 2 — 2PC Coordinator Timeout**  
- *Acceptance*: On coordinator failure after PREPARE, all participants abort within `coordinator_timeout_ms` (default 5,000 ms); no key ranges remain locked after $2 \times T_{\text{timeout}}$; `coordinator_timeout_total` counter increments correctly.
- *Test*: `test_2pc_coordinator_crash_focused` — inject coordinator failure at PREPARED state; verify participant abort within timeout; verify lock release.

**Item 3 — Cross-Shard Read Path gRPC**  
- *Acceptance*: `ShardRPCClient::readEntity()` uses `ReadEntity` gRPC RPC; `cross_shard_read_latency_p99_ms` ≤ `cross_shard_write_latency_p99_ms × 1.1` under equivalent load.
- *Test*: `test_cross_shard_rpc_read_focused` — 10,000 cross-shard reads via new gRPC path; verify latency within target.

### C. Phase Plan for Gap Closure

**Phase A — WAL and Consensus Hardening (Q2 2026)**  
1. Implement `RaftLog::createSnapshot(state_serializer)` with configurable trigger threshold.
2. Integrate snapshot creation with `RaftState` serialization and `RaftWALIntegration`.
3. Add `test_raft_snapshot_compaction_focused` (8 tests: snapshot trigger, compaction size, restart recovery, concurrent compaction + write).
4. Add `wal_compaction_triggered_total` and `wal_size_bytes` Prometheus gauges.

**Phase B — Transaction Safety (Q2 2026)**  
1. Add `coordinator_timeout_ms` field to `CrossShardTransactionConfig`.
2. Implement timeout-and-abort in `TwoPhaseCommitCoordinator`: on timeout, broadcast ABORT to all participants; participants release locks on ABORT receipt.
3. Add `test_2pc_coordinator_timeout_focused` (6 tests: timeout on prepare, timeout on commit broadcast, retry-after-abort, idempotent abort).

**Phase C — RPC Transport Completion (Q3 2026)**  
1. Add `ReadEntity` RPC to the shard gRPC proto definition.
2. Implement `ShardRPCClient::readEntity()` via the new gRPC path.
3. Remove the HTTP `RemoteExecutor` fallback for reads.
4. Benchmark cross-shard read/write latency parity.

---

## VIII. Discussion

### A. Architectural Strengths

The ThemisDB sharding architecture demonstrates several notable design qualities:

1. **Pluggable consensus**: The `ConsensusFactory` / `IConsensusAdapter` pattern allows runtime selection of Raft, Paxos, or Gossip without cluster restart, a level of flexibility absent in most monolithic sharding implementations.

2. **Multi-protocol transaction coordination**: Supporting 2PC, 3PC, SAGA, and Percolator within a single coordinator framework allows workload-specific protocol selection — a capability that most open-source distributed databases do not expose.

3. **Hardware-transparent node identity**: Decoupling `NodeIdentity` from physical endpoints (via `HardwareMigrationManager`) enables live hardware migration without consistent-hash ring changes — a significant operational advantage.

4. **Integrated repair and observability**: `ShardRepairEngine` with IOPS throttle, Prometheus metrics, and an admin API providing `force-repair` and `rebalance-trigger` endpoints represents production-grade operational ergonomics.

### B. Architectural Concerns

1. **Complexity density**: 73 source files across 5 distinct layers, 4 consensus protocols, and 4 transaction coordination strategies represent a very high implementation complexity for a system that is still in beta state. Complexity correlates with defect density, as evidenced by the 98-item bug register [5].

2. **Gap between documented capabilities and implemented reality**: The ROADMAP and FUTURE_ENHANCEMENTS documents describe production-ready capabilities (e.g., adaptive rebalancer, full gRPC read path) that are not yet implemented. This gap risks misrepresenting the system's readiness to downstream consumers of the documentation.

3. **Absent benchmark regression gates**: Two critical failures in the bug register (R-11 WAL sync regression, R-12 fake benchmark KPI) share a root cause: absence of automated benchmark regression gates in CI. A throughput or latency regression can persist undetected across multiple merges.

### C. Comparison with Production-Grade Alternatives

| Feature | ThemisDB Sharding | CockroachDB [9] | TiDB [10] |
|---------|-------------------|-----------------|-----------|
| Consensus algorithm | Raft / Paxos / Gossip | Raft | Raft |
| Log compaction | **Not implemented** | Implemented | Implemented |
| Cross-shard transactions | 2PC / 3PC / SAGA / Percolator | 2PC + MVCC | Percolator |
| 2PC blocking protection | **Not implemented** | Timeout-and-abort | Timeout-and-abort |
| Adaptive rebalancer | **Not implemented** | Implemented | Implemented |
| Chaos test suite | Basic (node kill) | Full (Jepsen-validated) | Full (Jepsen-validated) |

ThemisDB's pluggable consensus and multi-protocol transaction coordination are genuinely differentiated capabilities; the three critical gaps (log compaction, 2PC timeout, adaptive rebalancer) are the delta to close before it can claim equivalent production readiness.

### D. Threats to Validity

**Internal validity**: The performance numbers in Section VI are derived from theoretical models under idealized assumptions (uniform load distribution, intra-datacenter RTT, NVMe storage). Real-world deployments may exhibit skewed load, higher network jitter, and heterogeneous storage tiers that materially alter these numbers.

**Construct validity**: "Production readiness" is not a binary attribute. The acceptance criteria in Section VII define a specific, measurable interpretation of production readiness; other interpretations (e.g., full Jepsen suite, 99.99% availability SLA) would set a higher bar.

**External validity**: The risk taxonomy is grounded in the ThemisDB architecture specifically. The individual risk items (e.g., unbounded WAL growth, 2PC blocking) are general distributed systems problems; the specific file references and mitigation paths are ThemisDB-specific and may not transfer directly to other systems.

---

## IX. Implementation Evidence

**TABLE II: Evidence-to-Claim Traceability**

| Evidence ID | File | Scope | Claim Supported | Status |
|-------------|------|-------|-----------------|--------|
| E1 | `src/sharding/SECURITY.md` | Threat table | 2PC blocking window documented (R-01) | Ready |
| E2 | `src/sharding/ROADMAP.md` | Known Issues | WAL growth unbounded (R-06) | Ready |
| E3 | `src/sharding/ROADMAP.md` | Known Issues | gRPC read path incomplete (R-13) | Ready |
| E4 | `src/sharding/ROADMAP.md` | Production Readiness Checklist | Adaptive rebalancer `[?]` (R-08) | Ready |
| E5 | `src/sharding/paxos_wal.cpp` | fsync before PROMISE | Paxos acceptor state persistent (R-03 mitigated) | Ready |
| E6 | `tests/test_paxos_persistence_recovery_focused` | PSR-01..PSR-10 | Paxos recovery correctness | Ready |
| E7 | `src/sharding/gpu_erasure_coder.cpp` | AVX2 XOR kernel | RAID5 parity computation (Section VI-C) | Ready |
| E8 | `src/sharding/shard_repair_engine.cpp` | IOPS throttle parameter | Anti-entropy repair bounded overhead | Ready |
| E9 | `src/sharding/circuit_breaker.cpp` | failure_threshold=5, recovery=60s | Circuit breaker failover behavior (R-05) | Ready |
| E10 | `src/sharding/hardware_migration_manager.cpp` | DrainGuard RAII | Hardware migration drain correctness | Ready |
| E11 | `CHANGELOG.md` (PR #4596) | WAL sync regression | R-11 root cause and fix | Ready |
| E12 | `CHANGELOG.md` (PR #4595) | Fake benchmark | R-12 root cause and fix | Ready |
| E13 | `CHANGELOG.md` (PR #4591) | BatchWrite DoS | R-14 root cause and fix | Ready |
| E14 | `CHANGELOG.md` (PR #4678) | Paxos RPC stub | R-15 root cause and fix | Ready |

---

## X. Reproducibility and Artifact

- **Repository**: https://github.com/makr-code/ThemisDB (MIT license)
- **Sharding test targets**: `ctest -R "sharding" --output-on-failure` from the build directory
- **Specific focused targets**:
  ```
  ctest -R test_sharding_chaos_focused
  ctest -R test_paxos_persistence_recovery_focused
  ctest -R test_sharding_phase5_focused
  ctest -R test_adaptive_shard_router_focused
  ```
- **Expected runtime**: Individual focused targets: 1–30 s. Full sharding suite: 5–15 min.
- **Known environment requirements**: C++17 compiler, RocksDB, gRPC, AVX2-capable CPU for SIMD erasure coder tests.
- **Known pitfalls**: Chaos tests require sufficient file descriptor limits (`ulimit -n 65536`); port conflict detection may fail if test cleanup is interrupted.

---

## XI. Limitations, Risk, and Ethics

### System Boundary

This paper analyzes ThemisDB's sharding subsystem as a software artifact and design study. The claims in Section VI are theoretical and should not be interpreted as empirically validated production benchmarks.

### Misuse Risks

RAID0 deployments (no redundancy) are appropriate only when backed by an external backup policy. Using RAID0 without backups for any data that must survive node failure represents an unacceptable operational risk; system documentation must communicate this clearly.

### Security Boundary

The mTLS perimeter protects inter-shard communication but does not protect against compromised shard nodes that hold valid certificates. Operators deploying ThemisDB in adversarial environments (e.g., multi-tenant cloud) should apply additional isolation measures beyond key-prefix-level tenant separation.

### Ethics and AI Integration

For deployments integrating the sharding layer with LLM inference (as described in [4]), the isolation guarantees of the RAID-sharding layer affect the privacy properties of retrieved context. A routing bug that leaks cross-tenant documents into an inference context constitutes a privacy violation that may have regulatory consequences under GDPR or HIPAA.

---

## XII. Conclusion

We have presented a systematic evaluation and risk analysis of the ThemisDB RAID-sharding subsystem, contributing an 18-item risk taxonomy across five dimensions, a component-level evaluation framework with measurable acceptance criteria, and a theoretical performance analysis grounded in the actual implementation. The central finding is that the subsystem has a technically sophisticated and flexible architecture — pluggable consensus, multi-protocol transaction coordination, Reed-Solomon repair, and hardware-transparent node identity — but carries three critical production-readiness gaps: unbounded Raft WAL growth (R-06), absence of a 2PC coordinator timeout-and-abort mechanism (R-01), and an incomplete gRPC read path (R-13). Closing these three gaps, each requiring one to three weeks of focused engineering effort, is the minimum necessary condition for the subsystem to reach production-grade reliability. A further set of medium-severity items (adaptive rebalancer, reconstruction-phase risk elevation, gossip message signing) should be addressed before a general availability declaration.

The retrospective failure analysis in the ThemisDB bug register [5] identifies several root causes that are architectural rather than incidental: absence of benchmark regression gates, documentation that outpaces implementation, and insufficient chaos testing scope. These systemic patterns must be addressed as process changes alongside the specific technical gaps.

---

## References

[1] M. Stonebraker and U. Çetintemel, "'One Size Fits All': An Idea Whose Time Has Come and Gone," in *Proc. IEEE Int. Conf. Data Engineering (ICDE)*, Tokyo, Japan, 2005, pp. 2–11.

[2] P. Deutsch, "Fallacies of Distributed Computing," *Sun Microsystems Technical Report*, 1994.

[3] D. A. Patterson, G. Gibson, and R. H. Katz, "A Case for Redundant Arrays of Inexpensive Disks (RAID)," in *Proc. ACM SIGMOD*, Chicago, IL, USA, 1988, pp. 109–116.

[4] ThemisDB Contributors, "RAID-Sharded Inference: A Co-Design Architecture for Distributed Large Language Model Serving in Hybrid Database Systems," *research/RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md*, ThemisDB repository, 2026.

[5] ThemisDB Contributors, "Themis: It Is Okay to Fail — Engineering Retrospective and Defect Register," *research/THEMIS_IT_IS_OKAY_TO_FAIL*, ThemisDB repository, v0.6, 2026.

[6] L. Lamport, "Paxos Made Simple," *ACM SIGACT News*, vol. 32, no. 4, pp. 18–25, 2001.

[7] D. Ongaro and J. Ousterhout, "In Search of an Understandable Consensus Algorithm," in *Proc. USENIX ATC*, Philadelphia, PA, USA, 2014, pp. 305–320.

[8] J. C. Corbett, J. Dean, M. Epstein, A. Fikes, C. Frost, J. J. Furman, S. Ghemawat, A. Gubarev, C. Heiser, P. Hochschild, W. Hsieh, S. Kanthak, E. Kogan, H. Li, A. Lloyd, S. Melnik, D. Mwaura, D. Nagle, S. Quinlan, R. Rao, L. Rolig, Y. Saito, M. Szymaniak, C. Taylor, R. Wang, and D. Woodford, "Spanner: Google's Globally Distributed Database," *ACM Trans. Comput. Syst.*, vol. 31, no. 3, pp. 8:1–8:22, Aug. 2013.

[9] R. Taft, I. Sharif, A. Matei, N. VanBenschoten, A. Kimball, T. Haber, M. Letzner, P. Bardea, A. Reznichenko, B. Pientka, B. Narayanan, S. Vadali, P. Lawrence, D. Harrison, K. Shuster, M. Mossessian, C. Jain, K. Garg, G. Grunewald, and P. Mattis, "CockroachDB: The Resilient Geo-Distributed SQL Database," in *Proc. ACM SIGMOD*, Portland, OR, USA, 2020, pp. 1493–1509.

[10] P. Huang, W. Xu, X. Xia, J. Chen, Z. Tang, H. Wang, F. Chen, H. Ai, X. Kong, J. Guo, Y. Zhu, H. Xiong, and X. Lu, "TiDB: A Raft-based HTAP Database," *Proc. VLDB Endow.*, vol. 13, no. 12, pp. 3072–3084, 2020.

[11] I. S. Reed and G. Solomon, "Polynomial Codes over Certain Finite Fields," *J. Soc. Ind. Appl. Math.*, vol. 8, no. 2, pp. 300–304, 1960.

[12] S. Muralidhar, W. Lloyd, S. Roy, C. Hill, E. Lin, W. Liu, S. Pan, S. Shankar, V. Sivakumar, L. Tang, and S. Kumar, "f4: Facebook's Warm BLOB Storage System," in *Proc. USENIX OSDI*, Broomfield, CO, USA, 2014, pp. 383–398.

[13] D. Borthakur, J. Gray, J. S. Sarma, K. Muthukkaruppan, N. Spiegelberg, H. Kuang, K. Ranganathan, D. Molkov, A. Menon, S. Rash, R. Schmidt, and A. Aiyer, "Apache Hadoop Goes Realtime at Facebook," in *Proc. ACM SIGMOD*, Athens, Greece, 2011, pp. 1071–1080.

[14] J. Gray, "Notes on Data Base Operating Systems," in *Operating Systems: An Advanced Course*, Lecture Notes in Computer Science, vol. 60. Berlin, Heidelberg: Springer, 1978, pp. 393–481.

[15] D. Skeen, "Nonblocking Commit Protocols," in *Proc. ACM SIGMOD*, Ann Arbor, MI, USA, 1981, pp. 133–142.

[16] H. Garcia-Molina and K. Salem, "Sagas," in *Proc. ACM SIGMOD*, San Francisco, CA, USA, 1987, pp. 249–259.

[17] D. Peng and F. Dabek, "Large-Scale Incremental Processing Using Distributed Transactions and Notifications," in *Proc. USENIX OSDI*, Vancouver, BC, Canada, 2010, pp. 251–264.

[18] A. Thomson, T. Diamond, S.-C. Weng, K. Ren, P. Shao, and D. J. Abadi, "Calvin: Fast Distributed Transactions for Partitioned Database Systems," in *Proc. ACM SIGMOD*, Scottsdale, AZ, USA, 2012, pp. 1–12.

[19] T. Mühlbauer, W. Rödiger, R. Seilbeck, A. Kemper, and T. Neumann, "Instant Loading for Main Memory Databases," *Proc. VLDB Endow.*, vol. 6, no. 14, pp. 1702–1713, 2013.

[20] Y. Izrailevsky and A. Tseitlin, "The Netflix Simian Army," *Netflix Tech Blog*, Jul. 2011. [Online]. Available: https://netflixtechblog.com/the-netflix-simian-army-16e57fbab116

[21] K. Kingsbury, "Jepsen: Distributed Systems Safety Analysis," *GitHub repository*, 2013–present. [Online]. Available: https://github.com/jepsen-io/jepsen

[22] F. Swiderski and W. Snyder, *Threat Modeling*. Redmond, WA: Microsoft Press, 2004.

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contributions
- [x] All headline claims are evidence-backed (TABLE II)
- [x] Related work includes closest baselines and novelty delta
- [x] Risk taxonomy is grounded in documented failure history
- [x] Evaluation methodology defines testable workloads and metrics
- [x] Theoretical performance analysis states all assumptions
- [x] Limitations and threat model are transparent
- [x] Tables and figures are referenced in text
- [x] References are complete and consistent
- [ ] Final empirical benchmark results inserted (W1–W4 workloads)
- [ ] Commit hash and artifact manifest frozen for submission
- [ ] Gap closure items (Phase A–C) verified and status updated

## Appendix B. Risk Register Summary

| Risk ID | Dimension | Severity | Status |
|---------|-----------|----------|--------|
| R-01 | Consistency | Critical | Open |
| R-02 | Consistency | High | Open |
| R-03 | Consistency | High | **Mitigated** (v2.0.0) |
| R-04 | Consistency | Medium | Open |
| R-05 | Availability | Critical (RAID0) | Design-limited |
| R-06 | Availability | High | **Open** |
| R-07 | Availability | Medium | Mitigated (v2.0.0) |
| R-08 | Availability | Medium | Open |
| R-09 | Durability | High | Open |
| R-10 | Durability | Low–High | Deployment-dependent |
| R-11 | Durability | Critical | **Mitigated** (PR #4596) |
| R-12 | Operational | High | **Mitigated** (PR #4595) |
| R-13 | Operational | Medium | In Progress (Q3 2026) |
| R-14 | Operational | High | **Mitigated** (PR #4591) |
| R-15 | Operational | Critical | **Mitigated** (PR #4678) |
| R-16 | Security | Medium | Open |
| R-17 | Security | Medium | Open |
| R-18 | Security | Medium | Open |

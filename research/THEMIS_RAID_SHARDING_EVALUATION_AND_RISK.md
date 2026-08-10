# Evaluation and Risk Analysis of the ThemisDB Sharding Module with RAID-like Redundancy

**Status**: Review-ready technical article  
**Version**: 0.5  
**Last Updated**: 2026-08-10  
**Target Venue**: arXiv (cs.DB / cs.DC)  
**Scope**: Review of the ThemisDB sharding module, including RAID-like redundancy, consensus-aware coordination, repair, and cross-shard transport paths  
**Canonical sources reviewed**: `src/sharding/README.md`, `src/sharding/ARCHITECTURE.md`, `src/sharding/ROADMAP.md`, `src/sharding/SECURITY.md`, `include/sharding/redundancy_strategy.h`, `include/sharding/consistent_hash.h`, `include/sharding/consensus_module.h`, `include/sharding/gpu_erasure_coder.h`, `tests/sharding/CMakeLists.txt`, `.github/workflows/09-pr-gates_release-critical-tests.yml`

---

## Abstract

Distributed database sharding introduces a complex interplay among fault tolerance, consistency, availability, and operational risk. This article presents a structured review of the current ThemisDB sharding module — including its RAID-like redundancy modes — by combining source-code inspection, module-level documentation, test-registration review, workflow review, and theory-backed performance modeling rather than claiming a fully empirical benchmark campaign. The current codebase exposes pluggable consensus (Raft, Paxos, Gossip), multi-protocol cross-shard transactions (2PC, 3PC, SAGA, Percolator), a `ShardRepairEngine`, a consistent-hash routing layer with 150 virtual nodes by default, and erasure-coding backends surfaced through `GPUErasureCoder` and `RedundancyConfig` [4], [5]. Drawing on Level-1 sharding sources, current test and CI artefacts, and the wider distributed-systems literature, we derive a risk taxonomy across five dimensions: *consistency risk*, *availability risk*, *durability risk*, *operational risk*, and *security risk*. We extend the taxonomy in this version with three new risk items (R-21–R-23) covering router topology-update races, Gossip convergence windows, and LLM KV-cache cross-tenant isolation in converged storage-inference deployments. We identify three critical open gaps — unbounded WAL growth under Raft, a blocking window in the 2PC coordinator path, and an incomplete cross-shard read-path gRPC migration — and propose measurable acceptance criteria for each. We also provide a topology characterization of the current implementation: seven redundancy modes (`NONE`, `STRIPE`, `MIRROR`, `STRIPE_MIRROR`, `PARITY`, `RAID6`, `GEO_MIRROR`), three erasure-coding algorithms (`REED_SOLOMON`, `CAUCHY`, `LRC`), deterministic collision probing in the consistent-hash ring, and quorum defaults (`write_quorum = 2`, `read_quorum = 1`) [4], [5]. Theoretical sections are explicitly labeled as analytical models, not frozen benchmark results. The result is a traceable evaluation framework, topology reference, 23-item risk register, and formal threat model that can guide production-readiness decisions for the sharding module.

---

## I. Introduction

Horizontal sharding is the dominant strategy for scaling relational and document databases beyond single-node capacity limits [1]. By partitioning data across autonomous nodes, sharding enables throughput growth and geographic distribution. However, it simultaneously amplifies the failure surface: every cross-shard operation introduces potential consistency anomalies, every node failure requires coordinated recovery, and every topology change risks routing instability [2].

The current ThemisDB sharding module spans 94 compiled source files under `src/sharding/` and combines several subsystems rarely examined together in one codebase: a pluggable consensus layer centered on `ConsensusModule` implementations for Raft, Paxos, and Gossip; a multi-protocol cross-shard transaction coordinator supporting 2PC, 3PC, SAGA, and Percolator; a `ShardRepairEngine`; an `AdaptiveShardRouter`; and RAID-like redundancy strategies with CPU/GPU erasure-coding support exposed through `GPUErasureCoder` and `RedundancyConfig` [4], [5]. Current Level-1 module documentation describes this runtime as production-capable for core routing, coordination, repair, and observability surfaces, while also making clear that broader multi-shard rollout readiness remains incomplete and that critical hardening gates are still open [4].

Despite this implementation breadth, no repository-local article has previously synthesized the subsystem's architecture, risk surface, evidence trail, and open rollout gaps in one place. The repository already contains an LLM-focused sharding paper and a broader defect retrospective, but those artefacts are not organized as a sharding-specific review framework. This article fills that narrower review gap [5].

This paper fills that gap. Our contributions are:

1. A **risk taxonomy** for RAID-sharded distributed databases, grounded in ThemisDB's architecture and the documented failure history, covering five risk dimensions with 23 catalogued risk items (R-01–R-23).
2. A **topology reference architecture** detailing all seven RAID redundancy modes, three erasure-coding algorithm variants, the consistent-hash ring with virtual-node load-balance analysis, the quorum model, and the geo-distribution topology — the first complete topology characterization of the system.
3. A **component-level evaluation framework** defining testable hypotheses, measurable acceptance criteria, and fault-injection workloads for each sharding layer.
4. A **theoretical performance analysis** of Reed-Solomon repair throughput, scatter-gather fan-out, consensus latency, and quorum availability under partial failures.
5. An **evidence-grounded gap analysis** mapping three critical open production-readiness items to specific source files, with estimated closure effort.
6. A **security threat model** covering the inter-shard mTLS perimeter, shard-map poisoning, split-brain, tenant isolation, and 2PC coordinator failure modes; formalized as a STRIDE model in Appendix C.
7. A **FLP impossibility contextualization** [27] explaining why Raft's explicit leader-election and log-compaction mechanism is necessary in an asynchronous network model and what the absence of log compaction implies for ThemisDB's liveness under long-running deployments.
8. A **converged storage-inference sharding analysis** (§II-J) examining the additional risk surface created when LLM KV-cache management shares a RAID-sharding topology with persistent storage, referencing recent LLM serving systems [41], [42].

### A. Research Questions and Hypotheses

**RQ1**: What are the dominant failure modes of a RAID-sharded distributed database that combines pluggable consensus, multi-protocol cross-shard transactions, and Reed-Solomon repair, and how do they rank by impact severity?

**RQ2**: Under what conditions does each RAID mode (NONE, STRIPE, MIRROR, STRIPE_MIRROR, PARITY/RAID5, RAID6, GEO_MIRROR) satisfy the availability, durability, and performance targets required for production-grade serving?

**RQ3**: What are the measurable prerequisites for declaring the ThemisDB sharding subsystem production-ready, and which of those prerequisites are currently unmet?

**RQ4**: How does the consistent-hash ring topology interact with the redundancy mode and quorum configuration to determine the effective availability and load-balance properties of the cluster?

**H1**: The blocking 2PC coordinator window constitutes the highest-severity single-point-of-failure in the cross-shard transaction path, causing indefinite transaction suspension on coordinator failure in the absence of a timeout-and-abort mechanism.

**H2**: WAL growth in the current Raft implementation is effectively unbounded in long-running deployments, and the absence of snapshot compaction constitutes a durability and operational risk that will manifest within weeks of continuous operation under realistic write loads.

**H3**: For a 3-node Raft replica group with write quorum $W = 2$ and a shard failure probability $p$, the probability of write availability loss equals $p^2 (3 - 2p)$; at $p = 0.01$ this is approximately $2.97 \times 10^{-4}$, which satisfies typical four-nines availability targets but not five-nines without additional mitigation.

The remainder of the paper is organized as follows. Section II reviews related work, including §II-J on converged storage-inference sharding. Section III describes the system architecture and component interactions. Section IV presents the risk taxonomy (23 items, R-01–R-23). Section V covers the evaluation methodology and review approach. Section VI presents theoretical performance analysis. Section VII discusses the production-readiness gap analysis. Section VIII discusses architecture trade-offs and threats to validity. Section IX provides implementation evidence. Section X summarizes reproducibility notes. Section XI states limitations and known issues. Section XII concludes. Appendix C provides the formal STRIDE security threat model.

---

## II. Related Work

### A. Consistency and Fault Tolerance in Distributed Databases

Lamport's Paxos [6] and Ongaro and Ousterhout's Raft [7] established the consensus algorithm foundations used in ThemisDB. Raft's explicit leader election and log compaction (snapshot) mechanism directly addresses the WAL-growth risk identified in Section IV; the absence of snapshot compaction in the current implementation is a known gap relative to the reference design.

The **FLP impossibility theorem** [27] establishes the fundamental bound within which both Paxos and Raft operate: in an asynchronous message-passing system with even one possibly-faulty process, no deterministic consensus protocol can guarantee both safety and liveness. Paxos and Raft achieve consensus in practice by making timing assumptions (election timeouts, heartbeat intervals) that violate the purely asynchronous model — a design choice that works in practice but means their liveness guarantees are contingent on the timing assumptions holding. For ThemisDB, this has a direct implication: the Raft snapshot compaction mechanism (absent in the current implementation, R-06) is necessary not only for storage efficiency but for *bounded liveness* — without compaction, recovery replay time grows without bound, and recovery may eventually exceed the election timeout, causing spurious leader elections during restart.

**Spanner** [8] introduced TrueTime-bounded external consistency in a globally distributed database, motivating ThemisDB's `TrueTime` stub and `DistributedTimeCoordinator`. The correctness argument for cross-shard transactions in Spanner depends on bounded clock uncertainty; ThemisDB's current deployment without hardware TrueTime API support represents a construct validity gap acknowledged in this paper.

**CockroachDB** [9] and **TiDB** [10] demonstrate production RAID-style sharding with Raft consensus, 2PC-based distributed transactions, and range-based data routing. Their documented operational experience — particularly around Raft log compaction, cross-region latency, and split-brain recovery — directly informs the risk items in Section IV.

### B. Erasure Coding in Storage Systems

Reed-Solomon erasure coding [11] in RAID arrays is well-studied. **Facebook's f4** [12] applies erasure coding specifically to warm-blob storage, demonstrating that GF(2⁸)-based coding can achieve near-theoretical recovery throughput when SIMD acceleration is applied. ThemisDB's current erasure-coding stack exposes this capability through `GPUErasureCoder`, including AVX2-oriented CPU fallback and optional CUDA/OpenCL acceleration paths [5].

**HDFS-RAID** [13] documents real-world failure rates and repair window requirements in production erasure-coded storage. Their empirical observation that disk failures in practice arrive in correlated bursts (not independently) is directly relevant to ThemisDB's RAID5 single-shard-failure assumption; correlated failures violate this assumption and increase the risk of data loss during reconstruction.

### C. Cross-Shard Transaction Protocols

Two-phase commit (2PC) [14] is the standard cross-shard transaction coordination protocol. Its well-known vulnerability — blocking when the coordinator fails after the prepare phase — motivates the extensions in ThemisDB: 3PC [15] adds a pre-commit phase that allows participants to decide independently on coordinator failure; SAGA [16] replaces atomicity with compensating transactions; and Percolator [17] achieves lock-free distributed snapshot isolation through timestamp-ordered multi-version locking.

**Calvin** [18] and **CRATE** [19] propose deterministic transaction ordering to eliminate coordination overhead entirely; these approaches are outside ThemisDB's current design but are relevant background for future transaction coordinator evolution.

### D. Chaos Engineering and Production Readiness

**Netflix Chaos Monkey** [20] established the practice of intentional fault injection as a production readiness gate. The formal treatment by Basiri et al. [32] defines chaos engineering as the discipline of experimenting on a distributed system in order to build confidence in its capability to withstand turbulent conditions — distinguishing it from random fault injection by its reliance on steady-state hypotheses, variable injection, production-like conditions, and blast-radius minimization.

**Jepsen** [21] has documented failures in Cassandra, MongoDB, and CockroachDB under network partition that matched the theoretical vulnerability in their consensus protocols. The ThemisDB `test_sharding_chaos_focused` target is the equivalent gate; its current scope (shard partition, node failure injection) corresponds to what Jepsen defines as a basic network-partition test suite, but does not yet cover time-skew injection, partial-write faults, or disk failure simulation.

**FATE and DESTINI** [33] propose a systematic framework for testing cloud recovery: FATE injects failures at the node, network, and storage levels; DESTINI instruments the recovery path with post-condition assertions. Their empirical study across five distributed systems found that the majority of recovery bugs were exposed only when multiple failures were injected simultaneously or when failures occurred at specific timing windows — motivating ThemisDB's W2 (RAID5 double-failure during reconstruction) and W3 (2PC coordinator crash after PREPARE) workloads defined in Section V.

---

### E. Consistent Hashing and Load Balancing

**Karger et al.** [23] introduced consistent hashing as the principled solution to the rehashing problem in distributed caches and object stores: by mapping both nodes and keys onto a circular ring with random virtual nodes, only $K/N$ keys need to be remapped when one of $N$ nodes is added or removed. ThemisDB's `ConsistentHashRing` follows this design; the `HardwareMigrationManager` exploits the ring's stability guarantee to replace a node's physical endpoint without touching its virtual-node positions.

An important empirical finding from Karger et al. is that load imbalance is bounded by $O(\log N / B)$ where $B$ is the number of virtual nodes per physical server. ThemisDB's routing layer does not yet expose a virtual-node count parameter with documented load-balance guarantees; the absence of an adaptive rebalancer (R-08) makes load skew a persistent operational risk under non-uniform access patterns.

**Amazon Dynamo** [24] extended consistent hashing with preference lists, sloppy quorums, and hinted handoff to achieve high availability under network partitions, explicitly trading strong consistency for availability (AP operation in CAP terms). Dynamo's anti-entropy repair via Merkle trees [24] is the canonical comparison point for ThemisDB's `ShardRepairEngine`, which uses block-level checksum comparison rather than hierarchical hashing. The trade-off is discussed further in Section II-F.

---

### F. Anti-Entropy, Merkle Trees, and Distributed Repair

**Dynamo** [24] pioneered the use of Merkle trees for anti-entropy repair in production distributed storage: each node maintains a Merkle tree over its key range; divergence between two replicas is detected by comparing tree roots, then progressively narrowing to the divergent subtree. This achieves $O(\log N_{\text{keys}})$ comparison steps versus $O(N_{\text{keys}})$ for naive full-key comparison. ThemisDB's `ShardRepairEngine` currently uses block-level checksum comparison, which offers simplicity but scales linearly with shard size.

**Cassandra** adopted the Dynamo anti-entropy approach and extended it with configurable repair windows, repair parallelism, and `nodetool repair` as an operator-triggered mechanism. Production experience with Cassandra documented in [9] shows that anti-entropy repair must be run regularly to prevent divergence accumulation; missed repair cycles correlate with data loss on subsequent node failures.

**Azure Storage** [36] introduced the concept of extent sealing and repair pipelines in erasure-coded storage, demonstrating that staged repair (detect → reconstruct → rebalance) with per-stage observability reduces mean time to recovery (MTTR) significantly compared to monolithic repair scripts. ThemisDB's `ShardRepairEngine` provides IOPS-throttled scanning but lacks the staged observability pipeline that production systems require.

---

### G. Write-Ahead Logging, ARIES, and Recovery

**ARIES** (Algorithm for Recovery and Isolation Exploiting Semantics) [25] is the foundational reference for WAL-based database recovery. ARIES establishes three key principles: (1) WAL — all changes are logged before being written to the data pages; (2) repeating history — during recovery, all operations up to the crash point are replayed before performing rollbacks; (3) logging changes during undo — compensating log records are written for rolled-back operations to enable idempotent recovery. ThemisDB's `TransactionWAL` and `WALManager` are aligned with principles (1) and (2); the fixed WAL sync regression (R-11) arose from a violation of the WAL ordering guarantee.

ARIES also defines the concept of **fuzzy checkpointing**: a checkpoint records the current state of active transactions and dirty page buffers without stopping writes, enabling recovery to begin from the checkpoint rather than replaying the full WAL. This is the conceptual analogue of Raft snapshot compaction (R-06); the absence of snapshot compaction in ThemisDB means that crash recovery must replay the full WAL, degrading toward $O(N_{\text{log\_entries}})$ restart time as the deployment ages.

**Gray and Reuter** [34] provide the comprehensive treatment of transaction processing, including the WAL protocol correctness proof, group commit optimization, and the trade-off between fsync frequency and throughput — precisely the configuration error that caused the 79× regression in R-11. Their analysis shows that group commit can improve write throughput by 10–100× without sacrificing durability, provided that the commit-group-size and fsync-interval parameters are correctly configured.

---

### H. CAP Theorem, PACELC, and Isolation Levels

**Brewer's CAP conjecture** [28] states that a distributed system cannot simultaneously guarantee Consistency, Availability, and Partition tolerance. **Gilbert and Lynch** [29] formally proved the CAP theorem under an asynchronous network model, establishing that any system that claims to be both consistent and available under partition must make an unstated assumption about network reliability.

**Abadi's PACELC refinement** [30] argues that CAP is incomplete because it only addresses the partition case: even when the network is not partitioned, there is a latency–consistency trade-off (hence the "ELC" suffix). PACELC classifies systems as PA/EL (partition-available, latency-optimized; e.g., Dynamo), PC/EC (partition-consistent, consistency-optimized; e.g., HBase), or PA/EC (partition-available, consistency-optimized; e.g., Cassandra's quorum reads). ThemisDB's pluggable consensus creates multiple PACELC operating points depending on the selected protocol: Raft operates as PC/EC; Gossip operates as PA/EL; the mixed-protocol configuration is an operationally dangerous PA/EC approximation.

**Berenson et al.** [31] provided the formal critique of ANSI SQL isolation levels, identifying read phenomena (dirty read, non-repeatable read, phantom read) and defining the isolation levels that prevent each. For distributed databases, the `SERIALIZABLE` isolation level requires coordination across all shards, which is cost-prohibitive at scale; most production systems settle for `SNAPSHOT ISOLATION` (preventing write-write conflicts) or `READ COMMITTED` (preventing dirty reads only). ThemisDB's Percolator protocol targets snapshot isolation [17]; the 2PC and SAGA protocols do not prevent write skew without additional application-level constraints.

**Helland** [35] argues in "Life Beyond Distributed Transactions" that truly scalable distributed systems must accept the impossibility of cross-entity ACID transactions at scale, instead relying on per-entity transactionality and external compensating workflows. This observation directly motivates the SAGA protocol in ThemisDB's transaction coordinator: SAGA trades strict atomicity for compensability, which is appropriate for workflows where partial execution can be undone via domain-specific rollback operations.

---

### I. Topology, Range-Sharding, and Geo-Distribution

**Google Bigtable** [38] established the tablet-based approach to range-sharded distributed storage: the key space is divided into tablets sorted by row key; tablets are dynamically split and merged by a master server based on load. Bigtable's hierarchical metadata (root tablet → METADATA tablets → user tablets) provides a three-level topology that bounds tablet lookup latency to three RPC round-trips. ThemisDB uses hash-based rather than range-based partitioning (`ConsistentHashRing` with virtual nodes), trading ordered key scans for $O(1)$ routing lookup and smoother rebalancing when nodes join or leave the cluster.

**Vogels** [39] articulates the design principles of Amazon's eventually consistent systems: data is stored with a configurable $N$ replicas, $W$ write acknowledgements, and $R$ read quorums such that $R + W > N$ guarantees read-your-own-writes. ThemisDB's `RedundancyConfig` exposes `replication_factor` ($N$), `write_quorum` ($W = 2$ default), and `read_quorum` ($R = 1$ default); the flag `enable_quorum_enforcement` controls whether $R + W > N$ is enforced at runtime. The default $R = 1$ means reads are not quorum-consistent; full quorum reads require operator opt-in.

**Huang et al.** [40] introduced Local Reconstruction Codes (LRC) in Windows Azure Storage as an alternative to standard Reed-Solomon erasure coding. LRC partitions the codeword into local groups, each with its own local parity, so that single-failure recovery requires reading only $k/\ell$ symbols (where $\ell$ is the number of local groups) rather than all $k$ data symbols. ThemisDB's `ErasureCodingAlgorithm::LRC` implements this approach; for large shards where repair I/O is the bottleneck, LRC can reduce repair read amplification by a factor of $\ell$ at the cost of slightly higher storage overhead compared to standard Reed-Solomon.

**Spanner's geo-replication** [8] demonstrates that globally distributed transactions are feasible with bounded latency when clock uncertainty is bounded by TrueTime. ThemisDB's `GEO_MIRROR` redundancy mode provides geo-distributed replication with per-region write quorums and a configurable `region_failure_threshold`, but without hardware TrueTime, cross-region transaction ordering depends on NTP synchronization (R-19).

---

### J. Converged Storage and LLM Inference Sharding

A distinctive aspect of the ThemisDB architecture is the co-location of distributed persistent storage and LLM inference serving on the same RAID-sharding infrastructure, as described in the companion paper [4]. This *converged storage-inference* topology is a relatively recent architectural pattern in production ML systems, and it introduces a new class of sharding risks that are not addressed by classical distributed database theory.

**PagedAttention** [41] introduced the key memory management primitive for modern LLM serving: KV-cache is divided into fixed-size pages (analogous to virtual memory pages) that can be allocated, freed, and shared across concurrent inference requests without fragmentation. In a single-node setting, this significantly reduces KV-cache memory waste. In a distributed sharded setting — as in ThemisDB's `KVPrefixTransferManager` — pages must be transferred between shard nodes when domain routing selects a different shard for continuation of a multi-turn conversation or prompt prefix reuse. This cross-shard KV-page movement is a new type of distributed operation with no direct analogue in classical database sharding.

**Orca** [42] demonstrated that continuous batching — dynamically inserting new requests into in-flight inference batches at the sequence level — can dramatically improve GPU utilization and reduce tail latency in LLM serving. Continuous batching in a multi-shard ThemisDB deployment implies that the routing layer must make batching decisions in milliseconds, using the `AdaptiveShardRouter`'s domain capability scores to select the shard best suited for a given prompt's domain (legal, medical, general). This tight coupling between storage-layer gossip propagation and inference scheduling creates a new latency dependency: if the gossip convergence window (R-22) delays a domain capability update, the router may persistently misroute inference requests to a suboptimal shard.

The interaction between KV-cache sharding and storage sharding also creates a new **tenant isolation boundary** (R-23): when `KVPrefixTransferManager::transfer()` moves a KV-cache prefix from shard A to shard B for cross-shard inference routing, the transfer must enforce tenant key-prefix isolation at the transfer layer, not merely at the routing layer. A bug that strips the tenant prefix during transfer could expose one tenant's conversation context to another tenant's inference session — a privacy violation with potential regulatory consequences under GDPR Article 4(1).

The academic literature on distributed KV-cache management for LLM inference is nascent. Neither Orca [42] nor PagedAttention [41] analyze multi-tenant distributed KV-cache isolation; the security model in both papers assumes a single trusted operator. ThemisDB's converged architecture is therefore operating at a research frontier where existing frameworks do not provide sufficient safety guidance, motivating the dedicated risk items R-10, R-22, and R-23 in Section IV.

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
          │  GPUErasureCoder (CPU/CUDA/OpenCL)       │
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

### B. RAID Redundancy Modes — Full Taxonomy

The `RedundancyMode` enum in `include/sharding/redundancy_strategy.h` defines seven distinct redundancy strategies. The simplified RAID0/1/5 description in prior documentation does not capture the full implementation scope:

| Mode | Enum Value | Data Layout | Failure Tolerance | Storage Overhead | Notes |
|------|-----------|-------------|-------------------|-----------------|-------|
| NONE | `NONE` | Consistent-hash only, no replication | 0 shards | 0 % | Single copy; use only with external backup |
| STRIPE | `STRIPE` | Striped across $N$ shards (RAID-0) | 0 shards | 0 % | Maximum throughput, zero fault tolerance |
| MIRROR | `MIRROR` | Full replication to $R$ shards (RAID-1) | $R - 1$ shards | $(R-1) / R \times 100$ % | Default mode; $R = 3$ → 200 % overhead |
| STRIPE_MIRROR | `STRIPE_MIRROR` | Striping + mirroring (RAID-10) | Half of stripe group | 100 % | Best throughput with fault tolerance |
| PARITY | `PARITY` | Erasure coding: $k$ data + $m$ parity (RAID-5/6 generalized) | $m$ shards | $m / (k + m) \times 100$ % | Default: $k = 4$, $m = 2$ |
| RAID6 | `RAID6` | Dual parity erasure coding | 2 shards | Requires $m \geq 2$ | Validation enforced at config time |
| GEO_MIRROR | `GEO_MIRROR` | Per-region mirroring with configurable quorums | Per-region configurable | Varies | Requires per-region `write_quorum` and `read_quorum` maps |

*Table I. ThemisDB RAID redundancy mode taxonomy (from `redundancy_strategy.h`).*

Three erasure-coding algorithms are supported via the `ErasureCodingAlgorithm` enum:

- **REED_SOLOMON**: Classic Reed-Solomon over GF($2^8$). For $k$ data shards and $m$ parity shards, any $m$ erasures are recoverable. Storage efficiency: $k / (k + m)$.
- **CAUCHY**: Cauchy Reed-Solomon (Blömer et al.). Uses a Cauchy matrix over GF($2^w$) to reduce the XOR depth per symbol, typically yielding 20–40% faster encoding than Vandermonde Reed-Solomon for small $m$ [3].
- **LRC** (Local Reconstruction Code, Azure-style [40]): Partitions data symbols into $\ell$ local groups, each with a local parity. Single-failure repair reads only $k / \ell$ symbols rather than all $k$ data symbols, reducing repair read amplification by a factor of $\ell$.

The `ErasureCodingConfig` struct specifies `data_shards` ($k = 4$ default), `parity_shards` ($m = 2$ default), and a `min_document_size_kb` threshold below which erasure coding is skipped in favor of full replication — avoiding encoding overhead for small objects.

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
| FLP compliance [27] | Relies on election timeout assumption | Relies on leader timeout assumption | Achieves eventual delivery only |

The missing Raft snapshot compaction is explicitly documented as `[?]` in the ROADMAP and constitutes Risk Item R-06 in Section IV. As noted in §II-A, the FLP result [27] implies that the absence of log compaction risks unbounded recovery time that may exceed the election timeout, creating a self-amplifying failure mode under long-running deployments.

### D. Cross-Shard Transaction Protocols

ThemisDB supports four cross-shard transaction protocols selectable per transaction:

- **2PC**: Standard two-phase commit. Participant votes (PREPARED / ABORTED), coordinator decides (COMMIT / ROLLBACK). Blocking window exists between coordinator crash and participant recovery.
- **3PC**: Adds a pre-commit phase. Participants enter `PRE-COMMITTED` state, enabling non-blocking decisions on coordinator timeout. Requires stricter timing assumptions.
- **SAGA**: Long-running transactions modeled as a sequence of compensating sub-transactions. Atomicity is replaced by eventual compensability. Compensation RPC is now wired via `ShardRPCClient::compensate()` (fixed in v1.8.0).
- **Percolator**: Snapshot-isolation-based protocol using timestamp ordering with primary-lock escalation.

### E. Repair Engine

`ShardRepairEngine` executes anti-entropy scans at configurable intervals, comparing block checksums across replicas and triggering Reed-Solomon reconstruction for diverged blocks. The engine operates with an IOPS throttle targeting ≤ 10% of peak IOPS on a shard node. GPU-accelerated Reed-Solomon encoding/decoding is supported via `GpuErasureCoderOpenCL` (OpenCL kernel with GF(2⁸) multiply), with a CPU fallback when no OpenCL device is present.

Unlike Dynamo's Merkle-tree anti-entropy [24], which achieves $O(\log N_{\text{keys}})$ divergence detection, ThemisDB's block-checksum approach scales linearly with shard size. For small-to-medium shards (≤ 100 GB) the linear scan is acceptable given the IOPS throttle; for large shards (> 1 TB) the repair window may exceed the MTTR budget.

### F. Write-Ahead Log Design

ThemisDB's WAL subsystem (`TransactionWAL`, `WALManager`, `RaftLog`, `PaxosWAL`) follows the ARIES WAL ordering guarantee [25]: log records are written and fsynced before the corresponding data page is modified. The `WALManager` supports configurable group commit (batching multiple transactions into a single fsync call) to amortize I/O overhead — the parameter that caused the 79× regression (R-11) when misconfigured.

The Raft log (`raft_log.cpp`) is an append-only sequence of `LogEntry` structs persisted through `RaftWALIntegration`. Unlike a standard database WAL, the Raft log is not compacted by default: it grows until truncated by snapshot compaction (not implemented, R-06). At recovery time, the full log is replayed from the beginning, yielding $O(N_{\text{log\_entries}})$ startup latency — a structural divergence from ARIES fuzzy checkpointing [25], which bounds recovery time to the interval since the last checkpoint.

The Paxos WAL (`paxos_wal.cpp`) was updated in v2.0.0 to fsync promise/accept/commit records before returning to callers, closing R-03. Its WAL compaction model is separate from the Raft log and rotates based on committed instance count, providing bounded Paxos recovery time even without snapshot support.

### G. Shard Topology and Node Identity

Each physical shard is represented by a `ShardInfo` struct (defined in `include/sharding/shard_topology.h`) containing: a stable `shard_id` string (the logical identity); an `endpoint` string of the form `host:port` (the physical address); a `region` tag for geo-routing; and a `state` field (`LEADER`, `FOLLOWER`, `UNAVAILABLE`, `RECOVERING`). The decoupling of logical identity from physical endpoint is the architectural foundation of `HardwareMigrationManager`:

```
┌────────────────────────────────────────────────────────┐
│  Shard Ring (ConsistentHashRing)                        │
│  token_7a3f → shard_id="shard-03"                      │
│  token_b12c → shard_id="shard-07"  ← stable            │
│  token_e9d1 → shard_id="shard-12"                      │
└──────────────────────┬─────────────────────────────────┘
                       │ shard_id lookup
┌──────────────────────▼─────────────────────────────────┐
│  ShardTopology (endpoint registry)                      │
│  "shard-03" → endpoint="10.0.1.5:7001"  ← mutable     │
│  "shard-07" → endpoint="10.0.2.3:7001"  ← mutable     │
│  "shard-12" → endpoint="10.0.3.9:7001"  ← mutable     │
└──────────────────────┬─────────────────────────────────┘
                       │ HardwareMigrationManager.drain()
                       │ then updateEndpoint(shard_id, new_endpoint)
┌──────────────────────▼─────────────────────────────────┐
│  RaftConsensus.updatePeerAddress(shard_id, new_addr)    │
│  Ring positions unchanged — zero rehashing cost         │
└────────────────────────────────────────────────────────┘
```
*Fig. 2. Node identity decoupling: logical shard ID (ring-stable) vs. physical endpoint (mutable). Hardware replacement touches only the endpoint registry, not the hash ring.*

The `HardwareMigrationManager::DrainGuard` RAII object counts in-flight requests via `addInFlightRequest()` / `releaseInFlightRequest()` and blocks new migrations until `waitForDrain()` completes, providing a safe migration window without request loss.

### H. Consistent Hash Ring Architecture

The `ConsistentHashRing` (implemented in `src/sharding/consistent_hash.cpp`) maps both shards and keys onto a 64-bit circular ring using a `mix64` hash function (based on the MurmurHash3 finalizer):

$$h(x) = h' \oplus (h' \gg 33), \quad h' \leftarrow h' \times C_1, \quad h' \oplus= h' \gg 33, \quad h' \leftarrow h' \times C_2, \quad h' \oplus= h' \gg 33$$

where $C_1 = \texttt{0xff51afd7ed558ccd}$ and $C_2 = \texttt{0xc4ceb9fe1a85ec53}$.

Each physical shard is assigned $B = 150$ virtual nodes by default (`sharding.vnodes_per_node = 150`), placing the shard tokens at positions $h(\text{shard\_id} \# i)$ for $i = 0, \ldots, B-1$. For a cluster of $N$ physical shards, the ring contains $N \times B$ tokens. A read or write request for key $k$ is routed to the shard whose nearest token in clockwise order satisfies $\text{token} \geq h(k)$.

**Collision resolution**: When two virtual nodes hash to the same 64-bit token, the implementation applies deterministic probing:

$$\text{token}' = \text{mix64}(\text{token} + \texttt{0x9e3779b97f4a7c15} + \text{probe\_index})$$

repeating until the token position is unoccupied. This preserves ring density without losing virtual nodes.

**Load balance**: By the Karger et al. analysis [23], with $B$ virtual nodes per shard the expected load imbalance (maximum load / average load) is bounded by $O((\log N) / B^{1/2})$. At $B = 150$ and $N = 8$, the load coefficient of variation is approximately $1 / \sqrt{150} \approx 8.2\%$ in expectation, satisfying the $\leq 10\%$ imbalance criterion stated in the ROADMAP.

**Ring dynamics**: Adding a shard with $B$ virtual nodes causes at most $B$ key reassignments, each affecting $\langle K / (N \cdot B) \rangle$ keys on average. For a cluster with $K = 10^9$ keys, $N = 8$, $B = 150$: approximately $833\,\text{k}$ keys per shard are reassigned when one shard is added, versus a full $K/N = 125\,\text{M}$ keys in a naive modulo-$N$ scheme — a $150\times$ reduction in migration cost.

```
Ring (64-bit circular):

     0 ──────── shard-01 vnode #3 (0x1a2b3c...)
                shard-05 vnode #7 (0x2f4e5d...)
                shard-02 vnode #1 (0x3c7a8b...)
                ...
                shard-01 vnode #1 (0xf1e2d3...)  ← lookup(key) returns shard-01
  2^64─────────────────────────────────────────── (wraps to 0)
```
*Fig. 3. Consistent hash ring: virtual nodes from N=8 physical shards placed at mix64-hashed positions. A key lookup finds the nearest token clockwise.*

### I. Quorum Model and Read Preference

The `RedundancyConfig` exposes three quorum parameters:

| Parameter | Default | Meaning |
|-----------|---------|---------|
| `replication_factor` ($N$) | 3 | Number of replicas per shard |
| `write_quorum` ($W$) | 2 | Minimum replica acknowledgements for write success |
| `read_quorum` ($R$) | 1 | Minimum replica responses to satisfy a read |
| `enable_quorum_enforcement` | `false` | Enforce $R + W > N$ at runtime (off by default) |

The standard quorum consistency condition $R + W > N$ [39] ensures that any read will overlap with the most recent write. With $N = 3$, $W = 2$, $R = 1$: $R + W = 3 = N$, which equals rather than exceeds the threshold — meaning read-your-own-writes consistency is not guaranteed unless `enable_quorum_enforcement = true`. This is a documented operational gap: the default configuration is NOT quorum-consistent for reads.

**Read preference routing** is controlled by the `ReadPreference` enum, which supports seven strategies:

| Value | Semantics | Consistency |
|-------|-----------|-------------|
| `PRIMARY` | Always route to leader | Linearizable |
| `FOLLOWER` | Any follower (possibly stale) | Eventual |
| `NEAREST` | Lowest-latency replica | Eventual |
| `ROUND_ROBIN` | Load-balanced rotation | Eventual |
| `RANDOM` | Random replica | Eventual |
| `SECONDARY_ONLY` | Exclude primary | Eventual |
| `LOCAL_REGION` | Prefer same-region shard | Regional eventual |

For production workloads requiring linearizable reads, `PRIMARY` is the only safe choice. Using `FOLLOWER` or `NEAREST` with an application that requires monotonic read guarantees constitutes a semantic misconfiguration; this is not enforced at the `ReadPreference` selection site.

### J. Geo-Distribution Topology

The `GEO_MIRROR` redundancy mode enables multi-region shard placement controlled by the `GeoReplicationConfig` struct. Its key configuration dimensions are:

```
GeoReplicationConfig {
  regions: ["eu-west-1", "us-east-1", "ap-northeast-1"]
  region_write_quorums: {
      "eu-west-1": 2,
      "us-east-1": 2,
      "ap-northeast-1": 1   // tolerate 1 failure in AP region
  }
  region_read_quorums: {
      "eu-west-1": 1,
      "us-east-1": 1,
      "ap-northeast-1": 1
  }
  region_failure_threshold: 0.5   // majority of regions must be healthy
  replication_mode: ASYNC | SYNC  // per-region choice
}
```

A multi-region GEO_MIRROR deployment topology with three regions and the above configuration looks as follows:

```
┌──────────────────────────────────────────────────────────────────────────┐
│                          Client Layer                                      │
│                  (AQL / gRPC — region-aware routing)                       │
└───────────────┬────────────────────┬──────────────────────┬──────────────┘
                │                    │                        │
   ┌────────────▼──────────┐ ┌───────▼───────────┐ ┌────────▼──────────┐
   │   Region: eu-west-1   │ │  Region: us-east-1 │ │Region: ap-north-1│
   │  Shard-A (PRIMARY)    │ │  Shard-A (REPLICA) │ │  Shard-A (ASYNC) │
   │  Shard-B (PRIMARY)    │ │  Shard-B (REPLICA) │ │  Shard-B (ASYNC) │
   │  write_quorum = 2     │ │  write_quorum = 2  │ │  write_quorum = 1│
   │  Consensus: Raft      │ │  Consensus: Raft   │ │  Consensus: Raft │
   └────────────┬──────────┘ └───────┬───────────┘ └────────┬──────────┘
                │  SYNC replication  │  SYNC replication     │ ASYNC replication
                └────────────────────┴──────────────────────┘
                              Cross-region WAL shipping
                              (wal_shipper.cpp)
```
*Fig. 4. GEO_MIRROR topology: three regions, per-region quorums, sync replication to eu-west-1 and us-east-1, async replication to ap-northeast-1. The `WalShipper` component (`wal_shipper.cpp`) handles WAL transfer to async replicas.*

**Region failure behavior**: The `region_failure_threshold = 0.5` parameter means the cluster continues operating as long as more than half of configured regions are healthy. With 3 regions, this tolerates exactly one region failure without operator intervention. This is equivalent to a majority quorum at the region level, mirroring the Raft majority quorum at the node level.

**Cross-region consistency gap**: Async replication to the ap-northeast-1 region introduces a replication lag proportional to the WAN round-trip time (typically 50–200 ms for intercontinental links). During this window, reads from the async region may return data that has been committed but not yet shipped. This is a documented trade-off in the `GeoReplicationConfig`; applications reading from async replicas must explicitly accept eventual consistency semantics.

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

Distributed deadlock detection is well-studied: the Chandy-Misra-Haas probe-based algorithm [26] detects deadlocks by propagating probe messages along the wait-for graph, achieving $O(D)$ message complexity where $D$ is the number of edges in the graph. The centralized wait-for-graph approach used in ThemisDB's `DistributedCoordinator` is simpler to implement but requires periodic polling rather than reactive detection, introducing detection latency proportional to the polling interval.
- *Affected files*: `src/sharding/distributed_coordinator.cpp`
- *Mitigation path*: Document and benchmark the detection interval effect; add a `deadlock_detection_latency_ms` metric to the Prometheus endpoint.

### B. Availability Risk

**R-05 — RAID0 Single-Node Failure Causes Partial Data Loss** *(Severity: Critical for RAID0 deployments)*  
Under RAID0 (no redundancy), failure of any shard results in permanent loss of that shard's data partition. The `CircuitBreaker` (`circuit_breaker.cpp`) opens after 5 consecutive failures and routes subsequent requests to the consistent-hash fallback shard, which holds a different data partition. Callers receive errors or empty results for the failed shard's key range until manual recovery. This is by design for RAID0 but must be clearly communicated at deployment time.
- *Mitigation path*: Enforce a pre-deployment warning when RAID0 is selected without an external backup policy; document in operations runbook.

**R-06 — Raft WAL Growth Unbounded** *(Severity: High)*  
The Raft log in `src/sharding/raft_log.cpp` and `src/sharding/raft_wal_integration.cpp` accumulates entries indefinitely. Raft snapshot compaction — which replaces a prefix of the log with a point-in-time state snapshot — is specified in the Raft paper [7] but is not implemented in ThemisDB. Exact growth depends on entry payload size and persistence format; even at a conservative floor of ~100 bytes per entry, 10,000 operations/day still implies monotonic growth with no compaction boundary. This is both a storage risk and a recovery risk because restart replay time scales with retained log volume rather than a bounded snapshot size.
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

**R-19 — Clock Skew and Distributed Time Assumptions** *(Severity: Medium)*  
ThemisDB's `TrueTime` stub and `DistributedTimeCoordinator` assume bounded clock uncertainty for cross-shard transaction ordering. Without hardware TrueTime API support (as used in Google Spanner [8]), the system relies on NTP synchronization, which provides typical accuracy of 1–10 ms on well-managed networks but can diverge by seconds under network congestion or mis-configuration. The epoch-based fencing mechanism (R-07) configures `EpochFencing::lease_duration_ms` as a safety margin, but the correct margin depends on the actual NTP drift bound, which is not monitored or enforced.

The safety argument for distributed transaction ordering under clock uncertainty is treated rigorously by Lamport [37]: logical clocks provide total ordering of events without wall-clock synchronization, but at the cost of causality-based rather than real-time ordering. Spanner's TrueTime [8] is the production answer for wall-clock ordering; ThemisDB's use of physical timestamps without bounded uncertainty proof is a construct validity gap.
- *Affected files*: `src/sharding/truetime.cpp`, `src/sharding/distributed_time_coordinator.cpp`, `src/sharding/epoch_fencing.cpp`
- *Mitigation path*: Add `ntp_drift_ms` monitoring via Prometheus; set `EpochFencing::lease_duration_ms` to at least 3× observed NTP drift; document the clock-skew assumption prominently in the deployment guide.

**R-20 — MVCC Gap for Cross-Shard Read Consistency** *(Severity: Medium)*  
ThemisDB's Percolator protocol [17] provides snapshot isolation for cross-shard transactions by using timestamp-ordered multi-version locking. However, the 2PC and SAGA protocols do not implement MVCC: concurrent readers may observe a partially committed transaction in the interval between the coordinator's COMMIT decision and the last participant's COMMITTED acknowledgement. This violates the isolation guarantees described by Berenson et al. [31] for both `READ COMMITTED` (dirty read on in-flight 2PC transaction) and `REPEATABLE READ` (non-repeatable reads during SAGA compensations).

The practical impact depends on whether applications use 2PC/SAGA for operations where cross-shard read consistency is required. Applications that use Percolator for all reads are not affected; the risk materializes only for mixed-protocol deployments.
- *Affected files*: `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/cross_shard_transaction.cpp`
- *Mitigation path*: Document per-protocol isolation guarantees in a transaction protocol selection guide; add a `consistency_guarantee` field to `CrossShardTransactionConfig` (PA/EC vs. PC/EC classification per [30]); enforce appropriate protocol selection for workloads requiring `SNAPSHOT ISOLATION` or stronger.

**R-21 — Adaptive Shard Router Race Condition on Topology Update** *(Severity: Medium)*  
When `GossipProtocol` propagates a topology change (e.g., a shard becoming `UNAVAILABLE`) and `AdaptiveShardRouter` updates its internal routing table, the two operations are not atomic with respect to in-flight requests. In the window between the `GossipProtocol` processing the `PEER_OFFLINE` event and the `AdaptiveShardRouter` removing the shard from its candidate set, new requests may continue to be dispatched to the departing shard. The circuit breaker (`circuit_breaker.cpp`) will eventually open, but the first $k$ requests within the failure detection window (typically 5 failures) are sent to a shard that is known-unreachable at the gossip layer.
- *Affected files*: `src/sharding/gossip_protocol.cpp`, `src/sharding/adaptive_shard_router.cpp`, `src/sharding/circuit_breaker.cpp`
- *Mitigation path*: Add a callback interface `IGossipTopologyListener::onShardUnavailable(shard_id)` that is invoked by `GossipProtocol` under the gossip lock before the topology update is published; `AdaptiveShardRouter` registers as a listener and atomically removes the shard from its candidate set before the first affected request can be dispatched.

**R-22 — Gossip Convergence Window as Availability Gap** *(Severity: Medium)*  
The `GossipProtocol` disseminates topology updates and domain capability scores using a fanout-based epidemic model. For a cluster of $N$ nodes with gossip fanout $F$ and gossip interval $\Delta T$, the convergence time to reach all nodes is $O(\log_F N)$ gossip rounds, each of duration $\Delta T$. During this window, different nodes have inconsistent views of shard availability and domain capability scores, leading to divergent routing decisions across the cluster. Concretely: node A may have already marked shard X as `UNAVAILABLE` while node B still routes requests to shard X, causing B's requests to fail until B's gossip view converges.

For a cluster of $N = 32$ nodes, $F = 3$, $\Delta T = 1\,\text{s}$: convergence requires $\lceil \log_3 32 \rceil = 4$ rounds = approximately 4 s. During this window, up to $N / 2 = 16$ nodes may have stale routing views.
- *Affected files*: `src/sharding/gossip_protocol.cpp`, `src/sharding/adaptive_shard_router.cpp`
- *Mitigation path*: Reduce gossip interval to ≤ 200 ms for `PEER_OFFLINE` events (fast path); add a `gossip_convergence_lag_p99_ms` Prometheus metric; document convergence window in the operations guide as a first-class availability parameter.

**R-23 — LLM KV-Cache Cross-Tenant Isolation in Converged Mode** *(Severity: High for multi-tenant deployments)*  
In converged storage-inference deployments, the `KVPrefixTransferManager` moves KV-cache prefix data between shards during domain-routing-triggered inference migrations (see §II-J). The transfer path in `src/llm/kv_prefix_transfer_manager.cpp` constructs the transfer payload from the source shard's KV-cache without a server-side tenant-prefix validation step at the transfer layer; tenant isolation is enforced only at the `AdaptiveShardRouter` routing layer before the transfer is initiated. A bug in the routing layer that strips or mismatches the tenant prefix causes `KVPrefixTransferManager` to transfer one tenant's KV-cache data to another tenant's inference context on the destination shard. This is a privacy violation that may be undetectable until inference output leaks confidential context.
- *Affected files*: `src/llm/kv_prefix_transfer_manager.cpp`, `src/sharding/adaptive_shard_router.cpp`
- *Mitigation path*: Add a `tenant_id` field to `KVPrefixTransferRequest`; validate that the transfer destination shard's active inference session belongs to the same tenant before executing the transfer; emit `kv_transfer_tenant_mismatch_total` counter on any detected mismatch; reject the transfer and return `PERMISSION_DENIED` to the caller.

---

## V. Evaluation Methodology and Review Approach

### A. Source-of-Truth and Review Method

This article was re-verified against the current Level-1 sharding sources defined by `DOCUMENTATION_GOVERNANCE.md`: `src/sharding/`, `include/sharding/`, `tests/sharding/`, benchmark artefacts under `benchmarks/sharding/`, and the current release-critical workflow. Implementation-status claims are treated as valid only when they can be traced to those sources or to an explicitly named benchmark/test artefact.

### B. Test Infrastructure

`tests/sharding/CMakeLists.txt` currently contains 30 explicit `themis_register_module_focused_test(...)` registrations plus one focused integration registration through `themis_register_module_test(... KIND focused ...)`, with an additional autogen block for remaining `test_sharding_*.cpp` sources. Coverage spans consensus, cross-shard transactions, repair, epoch fencing, fault injection, redundancy, gossip, and migration-related paths. Each focused target links only the components under test to keep build scope isolated.

The primary automated gate for the broader regression baseline is `.github/workflows/09-pr-gates_release-critical-tests.yml`, which configures the `community-release` preset and runs `ctest -L release_critical` on pull requests targeting the canonical long-lived branches.

### C. Fault-Injection Workloads

We define six workload categories for the evaluation framework:

**W1 — Consensus Under Partition**  
Inject a network partition that splits a 5-node Raft cluster into a 3-node majority and a 2-node minority. Measure: time to new leader election, number of rejected writes on the minority, recovery time after partition heals. Expected: leader election ≤ 2× election timeout (typically 300–600 ms); minority writes rejected with `CONSENSUS_NO_QUORUM`; recovery ≤ 1 gossip round-trip.

**W2 — RAID5 Single-Shard Failure During Reconstruction**  
Kill shard $i$ while shard $j$ is under active RAID5 reconstruction. Measure: data integrity of non-affected key ranges, system response to second failure during reconstruction. Expected: system detects double failure and enters `DEGRADED_UNRECOVERABLE` state; operator alert triggered via `shard_reconstruction_failed_total` counter.

**W3 — 2PC Coordinator Crash After PREPARE**  
Kill the 2PC coordinator after all participants have responded with PREPARED. Measure: time until participants abort (via timeout), number of locked key-range-seconds, and whether the new coordinator resolves the transaction correctly. Expected (gap state): participants block indefinitely (confirms H1); with mitigation: participants abort within `coordinator_timeout_ms`.

**W4 — WAL Growth Under Continuous Writes**  
Run 10,000 Raft operations/day for 7 days on a test cluster. Measure: WAL file size per day and recovery time after simulated restart at day 7. Expected (gap state): WAL size grows monotonically with workload volume, and recovery time increases with retained log length unless compaction is introduced (confirms H2 qualitatively; exact magnitude must be measured).

**W5 — Clock Skew Injection Under Epoch Fencing**  
Artificially skew the system clock of one shard node by +500 ms, +2 s, and +10 s while cross-shard transactions are in flight. Measure: number of epoch-fence rejections, number of incorrectly accepted stale writes, and transaction abort rate. Expected: epoch fencing rejects all stale writes when `lease_duration_ms` > clock skew; at 10 s skew, test whether fencing margin is sufficient given the default configuration.

**W6 — Mixed-Protocol Isolation Anomaly Detection**  
Issue concurrent 2PC reads and SAGA compensating writes to the same key range. Verify whether dirty reads are observable from a concurrent reader between PREPARE and COMMITTED acknowledgement. Expected: anomaly detected if `READ COMMITTED` is claimed but not enforced; serves as a regression gate for R-20 (MVCC gap).

### D. Metrics

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
| NTP drift bound (monitored) | ≤ 10 ms (target) | Pending observability |
| Epoch lease margin vs. clock skew | ≥ 3× observed NTP drift | Pending configuration |

### E. Reproducibility Controls

All test targets use deterministic random seeds, fixed port offsets, and in-process temporary directories (RAII `TempDir`) to avoid test environment interference. Chaos tests inject failures via controlled shutdown sequences rather than `SIGKILL` to ensure WAL integrity between injected failure and recovery.

---

## VI. Theoretical Performance Analysis

### A. Reed-Solomon Repair Throughput

The current codebase surfaces erasure coding through `GPUErasureCoder`, with CPU fallback and CUDA/OpenCL acceleration. The calculation below should therefore be read as an analytical upper bound for a CPU-side AVX2 parity path, not as a measured repository benchmark:

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

### F. Quorum Availability Model

The quorum configuration ($N$, $W$, $R$) determines the write availability of a shard group as a function of independent node failure probability $p$. For a MIRROR group of $N = 3$ replicas with write quorum $W = 2$, writes succeed if at least $W$ nodes are available. Modeling node failures as independent Bernoulli events with probability $p$:

$$P(\text{write available}) = P(\text{at least } W \text{ of } N \text{ nodes available})$$

$$= \sum_{i=W}^{N} \binom{N}{i} (1-p)^i p^{N-i}$$

For $N = 3$, $W = 2$:

$$P(\text{write available}) = \binom{3}{2}(1-p)^2 p + \binom{3}{3}(1-p)^3 = 3(1-p)^2 p + (1-p)^3$$

$$= (1-p)^2 [3p + (1-p)] = (1-p)^2 (1 + 2p)$$

The complementary probability (write **unavailable**):

$$P(\text{write unavailable}) = 1 - (1-p)^2 (1 + 2p) = p^2 (3 - 2p)$$

| Shard failure probability $p$ | Write unavailability | Nines |
|-------------------------------|---------------------|-------|
| 0.001 (99.9% uptime) | $2.997 \times 10^{-6}$ | ~5.5 nines |
| 0.01 (99% uptime) | $2.97 \times 10^{-4}$ | ~3.5 nines |
| 0.05 (95% uptime) | $7.125 \times 10^{-3}$ | ~2.1 nines |

*Table III. Write unavailability for N=3, W=2 mirror group as a function of per-shard failure rate.*

This confirms H3: for a realistic per-shard availability of 99.9%, the 3-replica group achieves approximately 5.5 nines of write availability, satisfying four-nines and approaching five-nines targets. The independence assumption is the key caveat (see R-09 on correlated failures during RAID5 reconstruction).

For the GEO_MIRROR mode with $N_r$ regions each with $N$ replicas and regional write quorum $W_r$, availability degrades further because the region-level quorum requirement adds a second Bernoulli layer. Full derivation is out of scope for this paper; the key operational implication is that `region_failure_threshold = 0.5` with 3 regions (tolerating 1 region failure) reduces the region-level availability floor to approximately $(1 - p_r)^3 + 3 p_r (1-p_r)^2$ where $p_r$ is the per-region outage probability.

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

1. **Pluggable consensus**: The `ConsensusFactory` / `ConsensusModule` pattern allows runtime selection of Raft, Paxos, or Gossip without cluster restart, a level of flexibility absent in most monolithic sharding implementations.

2. **Multi-protocol transaction coordination**: Supporting 2PC, 3PC, SAGA, and Percolator within a single coordinator framework allows workload-specific protocol selection — a capability that most open-source distributed databases do not expose.

3. **Hardware-transparent node identity**: Decoupling `NodeIdentity` from physical endpoints (via `HardwareMigrationManager`) enables live hardware migration without consistent-hash ring changes — a significant operational advantage.

4. **Integrated repair and observability**: `ShardRepairEngine` with IOPS throttle, Prometheus metrics, and an admin API providing `force-repair` and `rebalance-trigger` endpoints represents production-grade operational ergonomics.

### B. Architectural Concerns

1. **Complexity density**: The current sharding tree spans 94 compiled source files across routing, coordination, transaction, repair, WAL, and observability layers. Even without claiming full GA readiness, this is a large implementation surface whose operational complexity is visible in repository-local failure analysis and in the number of still-open rollout gates [4], [5].

2. **Readiness gap remains explicit**: Current module docs do not claim a complete rollout; instead, they document a meaningful gap between the production-capable core runtime and the disabled broader multi-shard rollout. In particular, the roadmap still marks hardening, release-benchmark stabilization, and multi-shard enablement as incomplete [4].

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
| MVCC for cross-shard reads | Percolator only | Full MVCC | Full MVCC |
| Clock synchronization | NTP (unbounded drift) | HLC (hybrid logical clocks) | TSO (centralized) |
| Erasure coding algorithms | RS / Cauchy / **LRC** | RS only | RS only |
| Geo-distribution mode | **GEO_MIRROR** (configurable) | Multi-region Raft | TiKV multi-region |
| Quorum enforcement | Optional (`enable_quorum_enforcement`) | Always enforced | Always enforced |
| Read preference modes | 7 (PRIMARY/NEAREST/ROUND_ROBIN/etc.) | 3 (leader/follower/nearest) | 3 |

ThemisDB's LRC erasure coding, 7-mode RAID taxonomy, and GEO_MIRROR with per-region quorums are genuinely differentiated capabilities that exceed what CockroachDB and TiDB expose to operators. The three critical gaps (log compaction, 2PC timeout, adaptive rebalancer) remain the delta to close before production readiness parity.

### D. CAP/PACELC Positioning

The PACELC framework [30] classifies distributed systems along two axes: behavior under Partition (PA or PC) and behavior under normal operation (EL — latency-optimized, or EC — consistency-optimized). ThemisDB's pluggable consensus creates workload-specific PACELC positions:

| Consensus Protocol | Partition behavior | Normal operation | PACELC class |
|-------------------|--------------------|-----------------|--------------|
| Raft | Minority rejects writes (PC) | Leader-committed reads (EC) | **PC/EC** |
| Paxos | Minority rejects proposals (PC) | Quorum-committed reads (EC) | **PC/EC** |
| Gossip | Continues with partial view (PA) | Low-latency eventual reads (EL) | **PA/EL** |
| Mixed (Raft data + Gossip routing) | Data: PC; routing: PA | Data: EC; routing: EL | **PA/EC** (dangerous) |

The PA/EC combination in the mixed-protocol case is the classification Abadi [30] identifies as operationally problematic: the system appears to guarantee consistency (EC in normal operation) while silently relaxing it under partition (PA). In ThemisDB, this arises when Gossip consensus is selected for data shards while Raft is used for routing metadata: under a network partition, data writes are rejected (Raft quorum required) but routing metadata continues to update (Gossip continues), causing the router to direct requests to shards that no longer have a quorum. This scenario is not explicitly guarded against at the `ConsensusFactory` configuration level.

**Mitigation**: Add a `pacelc_class` annotation to each consensus adapter; enforce at factory construction that mixed-protocol configurations are explicitly acknowledged by the operator; emit a startup warning when PA data shards are combined with EC routing.

### E. Circuit Breaker Pattern and Resilience

The `RemoteExecutor` circuit breaker (`circuit_breaker.cpp`) implements the **Circuit Breaker pattern** as described by Nygard [36]: the breaker transitions between CLOSED (normal operation), OPEN (failure threshold exceeded, requests rejected immediately), and HALF-OPEN (probe request to test recovery) states. The configured thresholds (`failure_threshold = 5`, recovery window = 60 s) determine the balance between fail-fast responsiveness and false-positive trip risk.

Nygard's empirical observation is that circuit breakers should be tuned per-dependency rather than using a single global threshold: a dependency serving cached reads can tolerate a higher failure threshold than one serving consistency-critical writes. ThemisDB's current implementation uses a single circuit breaker configuration across all inter-shard RPC paths, which may trigger false-positive trips on read-path failures while under-protecting write-path failures. Per-operation-type breaker configuration is a recommended future enhancement.

---

### F. Threats to Validity

**Internal validity**: The performance numbers in Section VI are derived from theoretical models under idealized assumptions (uniform load distribution, intra-datacenter RTT, NVMe storage). Real-world deployments may exhibit skewed load, higher network jitter, and heterogeneous storage tiers that materially alter these numbers.

**Construct validity**: "Production readiness" is not a binary attribute. The acceptance criteria in Section VII define a specific, measurable interpretation of production readiness; other interpretations (e.g., full Jepsen suite, 99.99% availability SLA) would set a higher bar. The CAP/PACELC classification in Section VIII-D uses Abadi's [30] taxonomy, which itself has been critiqued as overly binary; real systems operate on a continuum.

**External validity**: The risk taxonomy is grounded in the ThemisDB architecture specifically. The individual risk items (e.g., unbounded WAL growth, 2PC blocking, Gossip non-linearizability) are general distributed systems problems documented in the literature [6], [7], [14], [28]–[31]; the specific file references and mitigation paths are ThemisDB-specific and may not transfer directly to other systems.

---

## IX. Implementation Evidence

**TABLE II: Evidence-to-Claim Traceability**

| Evidence ID | File | Scope | Claim Supported | Status |
|-------------|------|-------|-----------------|--------|
| E1 | `src/sharding/SECURITY.md` | Threat table | 2PC blocking window documented (R-01) | Ready |
| E2 | `src/sharding/ROADMAP.md` | Known Issues | WAL growth unbounded (R-06) | Ready |
| E3 | `src/sharding/shard_rpc_client.cpp`, `include/sharding/shard_rpc_client.h` | Cross-shard RPC surface | gRPC write path exists while read RPC is absent (R-13) | Ready |
| E4 | `src/sharding/ROADMAP.md` | Production Readiness Checklist | Adaptive rebalancer `[?]` (R-08) | Ready |
| E5 | `src/sharding/paxos_wal.cpp` | fsync before PROMISE | Paxos acceptor state persistent (R-03 mitigated) | Ready |
| E6 | `tests/test_paxos_persistence_recovery_focused` | PSR-01..PSR-10 | Paxos recovery correctness | Ready |
| E7 | `src/sharding/gpu_erasure_coder.cpp` | AVX2 XOR kernel | RAID5 parity computation (Section VI-C) | Ready |
| E8 | `src/sharding/shard_repair_engine.cpp` | IOPS throttle parameter | Anti-entropy repair bounded overhead | Ready |
| E9 | `src/sharding/circuit_breaker.cpp` | failure_threshold=5, recovery=60s | Circuit breaker failover behavior (R-05, VIII-E) | Ready |
| E10 | `src/sharding/hardware_migration_manager.cpp` | DrainGuard RAII | Hardware migration drain correctness | Ready |
| E11 | `CHANGELOG.md` (PR #4596) | WAL sync regression | R-11 root cause and fix | Ready |
| E12 | `CHANGELOG.md` (PR #4595) | Fake benchmark | R-12 root cause and fix | Ready |
| E13 | `CHANGELOG.md` (PR #4591) | BatchWrite DoS | R-14 root cause and fix | Ready |
| E14 | `CHANGELOG.md` (PR #4678) | Paxos RPC stub | R-15 root cause and fix | Ready |
| E15 | `src/sharding/truetime.cpp`, `epoch_fencing.cpp` | TrueTime stub + lease | Clock skew assumption (R-19) | Ready |
| E16 | `src/sharding/cross_shard_transaction.cpp` | Protocol selector | MVCC gap for 2PC/SAGA reads (R-20) | Ready |
| E17 | `src/sharding/gossip_consensus_adapter.cpp` | Eventual consistency | Gossip non-linearizability (R-02, VIII-D) | Ready |
| E18 | `include/sharding/redundancy_strategy.h` | `RedundancyMode` enum (7 modes) | Full RAID taxonomy (§III-B, Table I) | Ready |
| E19 | `src/sharding/consistent_hash.cpp` | mix64 hash + collision probing + 150 vnodes | Hash ring architecture (§III-H, Fig. 3) | Ready |
| E20 | `include/sharding/redundancy_strategy.h` | `ReadPreference` enum + quorum fields | Quorum model and read preference (§III-I) | Ready |
| E21 | `include/sharding/redundancy_strategy.h` | `GeoReplicationConfig` struct | Geo-distribution topology (§III-J, Fig. 4) | Ready |
| E22 | `src/sharding/hardware_migration_manager.cpp` | DrainGuard + updatePeerAddress | Node identity decoupling (§III-G, Fig. 2) | Ready |
| E23 | `src/llm/kv_prefix_transfer_manager.cpp` | Transfer payload construction without tenant check | KV-cache cross-tenant isolation risk (R-23) | Ready |
| E24 | `src/sharding/gossip_protocol.cpp` | Epidemic fanout parameters, no fast-path for PEER_OFFLINE | Gossip convergence window (R-22) | Ready |
| E25 | `src/sharding/adaptive_shard_router.cpp` | Routing table update not atomic with gossip callback | Router topology-update race (R-21) | Ready |

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
- **Known environment requirements**: C++17 compiler, RocksDB, gRPC, and CPU/GPU support compatible with the selected erasure-coding acceleration path.
- **Known pitfalls**: Chaos tests require sufficient file descriptor limits (`ulimit -n 65536`); port conflict detection may fail if test cleanup is interrupted.

---

## XI. Limitations and Known Issues

### A. System Boundary

This article analyzes ThemisDB's sharding module as a software artefact and design study. The claims in Section VI are theoretical and should not be interpreted as empirically validated production benchmarks.

### B. Misuse Risks

RAID0 deployments (no redundancy) are appropriate only when backed by an external backup policy. Using RAID0 without backups for any data that must survive node failure represents an unacceptable operational risk; system documentation must communicate this clearly.

### C. Security Boundary

The mTLS perimeter protects inter-shard communication but does not protect against compromised shard nodes that hold valid certificates. Operators deploying ThemisDB in adversarial environments (e.g., multi-tenant cloud) should apply additional isolation measures beyond routing-level tenant checks, certificate-based transport security, and application-layer authorization.

### D. Known Review Limitations

The evidence base in this article is intentionally repository-local: code, module docs, test registration, and workflow configuration. That makes the reasoning traceable, but it also means some claims remain analytical until the corresponding benchmark artefacts, fault-injection runs, or production incident reproductions are frozen and attached.

### E. Ethics and AI Integration

For deployments integrating the sharding layer with LLM inference (as described in [4]), the isolation guarantees of the RAID-sharding layer affect the privacy properties of retrieved context. A routing bug that leaks cross-tenant documents into an inference context constitutes a privacy violation that may have regulatory consequences under GDPR or HIPAA.

---

## XII. Conclusion

We have presented a structured evaluation and risk analysis of the ThemisDB sharding module with RAID-like redundancy, contributing a 23-item risk taxonomy across five dimensions (extended in v0.5 with R-21–R-23 covering router topology-update races, Gossip convergence windows, and LLM KV-cache cross-tenant isolation in converged deployments), a topology reference architecture for the system's seven redundancy modes and three erasure-coding algorithms, a component-level review framework with measurable acceptance criteria and six fault-injection workloads, and a theoretical performance analysis grounded in current source and module documentation. We have positioned the system against the CAP theorem [29], [30] and PACELC [31], identifying a dangerous PA/EC operating point that arises when Gossip consensus is mixed with Raft data shards without configuration-time enforcement. We have also quantified the quorum availability model: for $N = 3$, $W = 2$, the write unavailability probability is $p^2(3-2p)$, yielding approximately 5.5 nines at $p = 0.001$, confirming H3.

We have placed the ThemisDB consensus design in the context of the FLP impossibility result [27]: since distributed consensus is impossible in a purely asynchronous system with even one faulty process, Raft's explicit leader election and log compaction (snapshot) mechanism — not yet implemented — is a *liveness* necessity, not merely an optimization. The absence of Raft snapshot compaction (R-06) is therefore not a deferred enhancement but a fundamental liveness gap relative to the Raft design specification [7].

We have added a related work section on converged storage-inference sharding (§II-J), analyzing the unique risk surface created when LLM KV-cache management (PagedAttention [41], Orca [42]) shares a RAID-sharding topology with persistent storage. We have formalized the security risk surface as a STRIDE threat model (Appendix C), providing attack-path derivations for spoofing, tampering, repudiation, information disclosure, denial of service, and privilege escalation across the inter-shard communication layer.

The architecture analysis reveals that ThemisDB's LRC erasure coding, 7-mode RAID taxonomy, and GEO_MIRROR with per-region quorum configuration are genuinely differentiated from production alternatives such as CockroachDB [9] and TiDB [10]. The three critical production-readiness gaps (unbounded Raft WAL growth, absent 2PC coordinator timeout, incomplete gRPC read path) and five medium-severity gaps (clock skew assumptions, MVCC gap for 2PC/SAGA, router topology-update race, Gossip convergence window, KV-cache cross-tenant isolation) remain the delta to close before a general availability declaration.

The repository-local defect retrospective and research notes [5] identify several root causes that are architectural rather than incidental: incomplete benchmark regression discipline, documentation drift, and insufficient chaos-testing scope. These systemic patterns — analogous to the "fallacies of distributed computing" [2] applied at the process level — must be addressed as process changes alongside the specific technical gaps.

---

## References

[1] M. Stonebraker and U. Çetintemel, "'One Size Fits All': An Idea Whose Time Has Come and Gone," in *Proc. IEEE Int. Conf. Data Engineering (ICDE)*, Tokyo, Japan, 2005, pp. 2–11.

[2] P. Deutsch, "Fallacies of Distributed Computing," *Sun Microsystems Technical Report*, 1994.

[3] D. A. Patterson, G. Gibson, and R. H. Katz, "A Case for Redundant Arrays of Inexpensive Disks (RAID)," in *Proc. ACM SIGMOD*, Chicago, IL, USA, 1988, pp. 109–116.

[4] ThemisDB Contributors, "Sharding Module Documentation and Public Contracts" (`src/sharding/README.md`, `src/sharding/ARCHITECTURE.md`, `src/sharding/ROADMAP.md`, `src/sharding/SECURITY.md`, `include/sharding/redundancy_strategy.h`, `include/sharding/consistent_hash.h`, `include/sharding/consensus_module.h`, `include/sharding/gpu_erasure_coder.h`), ThemisDB repository, validated 2026-07-18. [Online]. Available: https://github.com/makr-code/ThemisDB/tree/develop/src/sharding

[5] ThemisDB Contributors, "Repository-local sharding research and defect-review artefacts" (`research/RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md`, `research/THEMIS_IT_IS_OKAY_TO_FAIL.md`, `tests/sharding/CMakeLists.txt`, `.github/workflows/09-pr-gates_release-critical-tests.yml`), ThemisDB repository, 2026. [Online]. Available: https://github.com/makr-code/ThemisDB/tree/develop/research

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

[23] D. Karger, E. Lehman, T. Leighton, R. Panigrahy, M. Levine, and D. Lewin, "Consistent Hashing and Random Trees: Distributed Caching Protocols for Relieving Hot Spots on the World Wide Web," in *Proc. ACM STOC*, El Paso, TX, USA, 1997, pp. 654–663.

[24] G. DeCandia, D. Hastorun, M. Jampani, G. Kakulapati, A. Lakshman, A. Pilchin, S. Sivasubramanian, P. Vosshall, and W. Vogels, "Dynamo: Amazon's Highly Available Key-value Store," in *Proc. ACM SOSP*, Stevenson, WA, USA, 2007, pp. 205–220.

[25] C. Mohan, D. Haderle, B. Lindsay, H. Pirahesh, and P. Schwarz, "ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging," *ACM Trans. Database Syst.*, vol. 17, no. 1, pp. 94–162, Mar. 1992.

[26] K. M. Chandy, J. Misra, and L. M. Haas, "Distributed Deadlock Detection," *ACM Trans. Comput. Syst.*, vol. 1, no. 2, pp. 144–156, May 1983.

[27] M. J. Fischer, N. A. Lynch, and M. S. Paterson, "Impossibility of Distributed Consensus with One Faulty Process," *J. ACM*, vol. 32, no. 2, pp. 374–382, Apr. 1985.

[28] E. A. Brewer, "Towards Robust Distributed Systems," in *Proc. ACM PODC* (invited talk), Portland, OR, USA, 2000, p. 7.

[29] S. Gilbert and N. Lynch, "Brewer's Conjecture and the Feasibility of Consistent, Available, Partition-Tolerant Web Services," *ACM SIGACT News*, vol. 33, no. 2, pp. 51–59, Jun. 2002.

[30] D. J. Abadi, "Consistency Tradeoffs in Modern Distributed Database System Design: CAP is Only Part of the Story," *IEEE Computer*, vol. 45, no. 2, pp. 37–42, Feb. 2012.

[31] H. Berenson, P. Bernstein, J. Gray, J. Melton, E. O'Neil, and P. O'Neil, "A Critique of ANSI SQL Isolation Levels," in *Proc. ACM SIGMOD*, San Jose, CA, USA, 1995, pp. 1–10.

[32] A. Basiri, N. Behnam, R. de Rooij, L. Hochstein, L. Kosewski, J. Reynolds, and C. Rosenthal, "Chaos Engineering," *IEEE Software*, vol. 33, no. 3, pp. 35–41, May/Jun. 2016.

[33] H. S. Gunawi, T. Do, P. Joshi, P. Alvaro, J. M. Hellerstein, A. C. Arpaci-Dusseau, R. H. Arpaci-Dusseau, K. Sen, and D. Borthakur, "FATE and DESTINI: A Framework for Cloud Recovery Testing," in *Proc. USENIX NSDI*, Boston, MA, USA, 2011, pp. 289–304.

[34] J. Gray and A. Reuter, *Transaction Processing: Concepts and Techniques*. San Mateo, CA: Morgan Kaufmann, 1992.

[35] P. Helland, "Life Beyond Distributed Transactions: An Apostate's Opinion," in *Proc. CIDR*, Asilomar, CA, USA, 2007, pp. 132–141.

[36] M. T. Nygard, *Release It! Design and Deploy Production-Ready Software*, 2nd ed. Raleigh, NC: Pragmatic Bookshelf, 2018.

[37] L. Lamport, "Time, Clocks, and the Ordering of Events in a Distributed System," *Commun. ACM*, vol. 21, no. 7, pp. 558–565, Jul. 1978.

[38] F. Chang, J. Dean, S. Ghemawat, W. C. Hsieh, D. A. Wallach, M. Burrows, T. Chandra, A. Fikes, and R. Gruber, "Bigtable: A Distributed Storage System for Structured Data," *ACM Trans. Comput. Syst.*, vol. 26, no. 2, pp. 4:1–4:26, Jun. 2008.

[39] W. Vogels, "Eventually Consistent," *Commun. ACM*, vol. 52, no. 1, pp. 40–44, Jan. 2009.

[40] C. Huang, H. Simitci, Y. Xu, A. Ogus, B. Calder, P. Gopalan, J. Li, and S. Yekhanin, "Erasure Coding in Windows Azure Storage," in *Proc. USENIX ATC*, Boston, MA, USA, 2012, pp. 15–26. [Online]. Available: https://www.usenix.org/conference/atc12/technical-sessions/presentation/huang

[41] W. Kwon, Z. Li, S. Zhuang, Y. Sheng, L. Zheng, C. H. Yu, J. Gonzalez, H. Zhang, and I. Stoica, "Efficient Memory Management for Large Language Model Serving with PagedAttention," in *Proc. ACM SOSP*, Koblenz, Germany, 2023, pp. 611–626. [Online]. Available: https://arxiv.org/abs/2309.06180

[42] G.-I. Yu, J. S. Jeong, G.-W. Kim, S. Kim, and B.-G. Chun, "Orca: A Distributed Serving System for Transformer-Based Generative Models," in *Proc. USENIX OSDI*, Carlsbad, CA, USA, 2022, pp. 521–538. [Online]. Available: https://www.usenix.org/conference/osdi22/presentation/yu

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contributions and new citation coverage
- [x] All headline claims are evidence-backed (TABLE II, 25 entries)
- [x] Related work includes closest baselines and novelty delta (§II-A through §II-J)
- [x] Risk taxonomy is grounded in documented failure history (23 items, R-01..R-23)
- [x] Evaluation methodology defines testable workloads and metrics (W1–W6)
- [x] Theoretical performance analysis states all assumptions
- [x] Quorum availability model derived and validated against H3 (§VI-F)
- [x] Full topology reference: 7 RAID modes, 3 EC algorithms, hash ring, quorum, geo (§III-B through §III-J)
- [x] CAP/PACELC positioning is explicit (§VIII-D)
- [x] Comparison table updated: 12 features vs CockroachDB/TiDB
- [x] Limitations and threat model are transparent (§VIII-F, Appendix C)
- [x] Tables and figures are referenced in text (4 figures)
- [x] References are complete and consistent ([1]–[42])
- [x] FLP impossibility ([27]) fills the reserved slot; contextualized for Raft liveness analysis
- [x] Converged storage-inference sharding (§II-J) with [41], [42] citations
- [x] New risk items R-21..R-23 added with evidence E23–E25 in TABLE II
- [x] Formal STRIDE threat model added (Appendix C)
- [ ] Final empirical benchmark results inserted (W1–W6 workloads)
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
| R-19 | Consistency / Operational | Medium | Open (new in v0.2) |
| R-20 | Consistency | Medium | Open (new in v0.2) |
| R-21 | Availability / Consistency | Medium | Open (new in v0.4) |
| R-22 | Availability | Medium | Open (new in v0.4) |
| R-23 | Security | High | Open (new in v0.4) |

---

## Appendix C. Formal STRIDE Security Threat Model

This appendix formalizes the security analysis in §IV-E and §VIII using the STRIDE threat modeling framework [22]. STRIDE classifies threats across six categories: **S**poofing, **T**ampering, **R**epudiation, **I**nformation Disclosure, **D**enial of Service, and **E**levation of Privilege. Each threat is mapped to a specific ThemisDB component, the attack path is described, and the existing or recommended mitigation is stated.

The primary trust boundaries in the ThemisDB sharding layer are:
1. **Intra-cluster mTLS boundary** — connections between `ShardRPCClient` and `ShardRPCServer` endpoints via `mtls_client.cpp`.
2. **Gossip broadcast boundary** — `GossipProtocol` messages shared across the cluster without application-level signing.
3. **Storage-inference boundary** — the interface between `KVPrefixTransferManager` and the per-shard RocksDB storage layer.
4. **Admin API boundary** — HTTP endpoints in `MaintenanceApiHandler` and `ShardAdminApi` protected by RBAC tokens.

### C.1 Spoofing

**Threat S-1 — Certificate Identity Spoofing via Compromised Shard Node**

| Attribute | Value |
|-----------|-------|
| Asset | Inter-shard RPC trust (mTLS identity) |
| Attack path | An attacker who compromises a shard node obtains the node's private key and client certificate. Using these credentials, the attacker can impersonate the compromised shard to all other cluster members, injecting replicated writes or draining KV-cache transfers. |
| Affected component | `src/sharding/mtls_client.cpp`, `src/sharding/pki_shard_certificate.cpp` |
| Current mitigation | mTLS mutual authentication requires both parties to present valid certificates signed by the cluster CA. |
| Gap | Certificate revocation checking latency is unbounded (R-16); a compromised certificate may remain valid for up to the certificate TTL (potentially 1 year in default configurations) if OCSP responses are slow or cached. |
| Recommended mitigation | Enforce short-lived certificates (≤ 24 h TTL, automated rotation); bound OCSP/CRL check timeout to ≤ 100 ms; implement certificate pinning at the `ShardRPCServer` for known cluster members. |

**Threat S-2 — Gossip Source Spoofing**

| Attribute | Value |
|-----------|-------|
| Asset | Shard topology and domain capability routing scores |
| Attack path | A compromised shard injects `AdapterCapabilityAnnouncement` messages with fraudulent domain scores, causing `AdaptiveShardRouter` to prefer the attacker-controlled shard for high-value domain queries (legal, medical). |
| Affected component | `src/sharding/gossip_protocol.cpp`, `src/distributed_knowledge/adapter_capability_announcement.h` |
| Current mitigation | Gossip messages are exchanged over mTLS-authenticated connections; message origin is implicitly authenticated by the connection identity. |
| Gap | No application-level HMAC on gossip payloads (R-18); a compromised node can inject valid-looking messages from any claimed source. |
| Recommended mitigation | Add HMAC-SHA256 signatures to `AdapterCapabilityAnnouncement` using a cluster-shared signing key; validate signatures at the receiving `AdaptiveShardRouter` before updating capability scores. |

---

### C.2 Tampering

**Threat T-1 — WAL Entry Tampering**

| Attribute | Value |
|-----------|-------|
| Asset | Transaction durability (WAL integrity) |
| Attack path | An attacker with filesystem-level access to a shard node modifies WAL entries in `RaftLog` or `TransactionWAL` before the recovery replay. Since the WAL is an append-only log without per-entry checksums in the current implementation, tampered entries would be replayed silently. |
| Affected component | `src/sharding/raft_log.cpp`, `src/sharding/transaction_wal.cpp` |
| Current mitigation | mTLS protects in-flight replication; RocksDB block-level checksums protect the storage layer from silent disk corruption. |
| Gap | WAL entries themselves are not HMAC-signed; an attacker with direct filesystem access can modify log entries without detection at the WAL replay layer. |
| Recommended mitigation | Add a per-entry CRC-32C or SHA-256 hash to WAL records; validate on replay; integrate with RocksDB's block checksum verification. |

**Threat T-2 — Gossip Capability Score Manipulation**

| Attribute | Value |
|-----------|-------|
| Asset | Routing correctness (domain capability scores) |
| Attack path | A compromised node modifies `capability_score` fields in gossip payloads before forwarding to downstream peers, amplifying or suppressing domain scores to manipulate load distribution across the cluster. |
| Current mitigation | None (no message signing). |
| Recommended mitigation | Same as S-2: HMAC signatures on capability announcements. |

---

### C.3 Repudiation

**Threat R-T-1 — Transaction Coordinator Repudiation**

| Attribute | Value |
|-----------|-------|
| Asset | Cross-shard transaction audit trail |
| Attack path | The 2PC coordinator issues a COMMIT decision, but its WAL entry is lost due to a storage failure before the COMMIT is fsynced. After recovery, the coordinator has no record of the COMMIT; participants may have committed but the coordinator repudiates knowledge of the transaction. |
| Affected component | `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/transaction_wal.cpp` |
| Current mitigation | The WAL ordering guarantee (ARIES-style) requires the COMMIT record to be fsynced before the coordinator sends COMMIT to participants; if fsync fails, the coordinator aborts. |
| Gap | If the fsync failure is not correctly detected and propagated, the commit record may be silently lost. Audit log completeness is not separately asserted in the test suite. |
| Recommended mitigation | Assert that every `CommitDecision` has a corresponding `WalEntry::COMMIT` log record before sending the commit broadcast; add a test that verifies WAL completeness after coordinator crash. |

**Threat R-T-2 — Admin API Action Without Audit Trail**

| Attribute | Value |
|-----------|-------|
| Asset | Administrative operation traceability |
| Attack path | An authorized admin triggers a `POST /api/v1/shards/{id}/migrate-hardware` without the operation being recorded in an immutable audit log. If the migration causes data loss, there is no record of who triggered it or when. |
| Current mitigation | `AuditLogger` is invoked for maintenance operations; sharding admin API audit coverage is partial. |
| Recommended mitigation | Enforce that all state-changing admin API calls (migrate, rebalance, force-repair) write a signed audit record before executing the operation; integrate with `MaintenanceApiHandler`'s existing audit path. |

---

### C.4 Information Disclosure

**Threat I-1 — Tenant Key Prefix Isolation Bypass**

| Attribute | Value |
|-----------|-------|
| Asset | Multi-tenant data isolation |
| Attack path | As described in R-17: a bug in the key prefix extraction logic in `shard_router.cpp` causes tenant A's queries to be routed to tenant B's key range. Since there is no secondary server-side validation, the storage operation executes and returns tenant B's data to tenant A. |
| Affected component | `src/sharding/shard_router.cpp`, `src/sharding/adaptive_shard_router.cpp` |
| Current mitigation | Tenant prefix is embedded in the routing key at the AQL layer before reaching the shard router. |
| Gap | No server-side validation that the incoming request's tenant prefix matches the shard's authorized tenant set. |
| Recommended mitigation | Add server-side tenant prefix validation in `ShardRPCServer` before executing storage operations; emit `tenant_isolation_violation_total` counter on mismatch. |

**Threat I-2 — KV-Cache Cross-Tenant Information Disclosure (R-23)**

| Attribute | Value |
|-----------|-------|
| Asset | LLM inference context confidentiality |
| Attack path | As described in R-23: `KVPrefixTransferManager` transfers a KV-cache prefix from shard A to shard B. If the tenant prefix is stripped or mismatched during the transfer, the destination shard's inference session receives another tenant's conversation history, leaking confidential context. |
| Recommended mitigation | Add `tenant_id` field to `KVPrefixTransferRequest`; validate at transfer layer; return `PERMISSION_DENIED` on mismatch. |

---

### C.5 Denial of Service

**Threat D-1 — Raft WAL Disk Exhaustion (R-06)**

| Attribute | Value |
|-----------|-------|
| Asset | Shard node availability |
| Attack path | Under a sustained write workload, the Raft log grows without bound (R-06). An attacker who can sustain a high write rate (legitimate or via a flooding attack that bypasses rate limiting) can exhaust the shard node's disk capacity, crashing the node. With a 3-node Raft group, a coordinated attack against two nodes simultaneously causes quorum loss and cluster-wide write unavailability. |
| Affected component | `src/sharding/raft_log.cpp` |
| Current mitigation | None (no Raft snapshot compaction). |
| Recommended mitigation | Implement Raft snapshot compaction (Phase A gap closure); add `wal_size_bytes` Prometheus gauge with alerting threshold. |

**Threat D-2 — gRPC BatchWrite Amplification (R-14, mitigated)**

| Attribute | Value |
|-----------|-------|
| Asset | Shard node memory availability |
| Attack path | A client submits an oversized `BatchWrite` request that allocates memory proportional to the batch size on the shard server. With no bound on batch size, an attacker can exhaust server memory with a single gRPC call. |
| Current mitigation | **Fixed** in v2.0.0: `max_batch_entries` check added at the gRPC handler (PR #4591). |
| Remaining risk | The `max_batch_entries` default value is not documented; operators may not realize it exists, leaving it at a default that may be too permissive for adversarial environments. |

**Threat D-3 — Circuit Breaker False-Positive Trip Under Read Load**

| Attribute | Value |
|-----------|-------|
| Asset | Read availability |
| Attack path | As described in §VIII-E: the single circuit breaker threshold (`failure_threshold = 5`) applies uniformly to all inter-shard RPC paths. A transient read-path failure spike (e.g., from a large repair scan) can trip the breaker for write-path operations as well, unnecessarily degrading write availability. |
| Recommended mitigation | Separate circuit breaker instances per operation type (read vs. write vs. repair); configure per-type thresholds. |

---

### C.6 Elevation of Privilege

**Threat E-1 — Consensus Protocol Impersonation as Leader**

| Attribute | Value |
|-----------|-------|
| Asset | Raft log integrity / cross-shard write ordering |
| Attack path | A compromised shard node that has been excluded from the Raft group (term-based expiry) attempts to continue issuing `AppendEntries` RPCs to followers using a stale leader lease. If epoch fencing is not enforced correctly (R-07), followers may accept AppendEntries from a deposed leader, causing log divergence. |
| Affected component | `src/sharding/raft_consensus.cpp`, `src/sharding/epoch_fencing.cpp` |
| Current mitigation | Epoch fencing rejects `AppendEntries` from leaders whose epoch lease has expired. 39 tests in `test_auto_failover_focused` cover this scenario under normal timing. |
| Gap | Time-skew injection is not yet tested (R-07); a sufficient clock skew may allow a deposed leader's epoch to appear valid beyond its expiry. |
| Recommended mitigation | Add time-skew injection tests; set `EpochFencing::lease_duration_ms` conservatively (≥ 3× observed NTP drift). |

**Threat E-2 — Admin API RBAC Bypass via Token Replay**

| Attribute | Value |
|-----------|-------|
| Asset | Administrative operation privilege |
| Attack path | The `maintenance:admin` RBAC token used to authorize destructive operations (e.g., `DELETE /api/v1/maintenance/schedules/{id}`) is a static bearer token without expiry or nonce protection. An attacker who captures the token (e.g., via server log exfiltration) can replay it indefinitely to execute admin operations. |
| Affected component | `src/maintenance/maintenance_api_handler.cpp`, `src/sharding/admin_api.cpp` |
| Current mitigation | Token-based RBAC check (`maintenance:admin` / `maintenance:write` / `maintenance:read`). |
| Gap | No token expiry, no nonce, no short-lived credential rotation. |
| Recommended mitigation | Replace static admin tokens with short-lived JWTs (≤ 1 h TTL) signed by a cluster CA; implement token refresh endpoint; log all token usage to the audit trail. |

---

### C.7 STRIDE Summary Table

| Threat ID | Category | Component | Severity | Status |
|-----------|----------|-----------|----------|--------|
| S-1 | Spoofing | mtls_client.cpp, pki_shard_certificate.cpp | Medium | Open (R-16 mitigation pending) |
| S-2 | Spoofing | gossip_protocol.cpp | Medium | Open (R-18 mitigation pending) |
| T-1 | Tampering | raft_log.cpp, transaction_wal.cpp | Medium | Open |
| T-2 | Tampering | gossip_protocol.cpp | Medium | Open (same as R-18) |
| R-T-1 | Repudiation | two_phase_commit_coordinator.cpp | Medium | Partially mitigated (WAL ordering) |
| R-T-2 | Repudiation | maintenance_api_handler.cpp | Low | Open |
| I-1 | Info. Disclosure | shard_router.cpp | High | Open (R-17 mitigation pending) |
| I-2 | Info. Disclosure | kv_prefix_transfer_manager.cpp | High | Open (R-23 mitigation pending) |
| D-1 | DoS | raft_log.cpp | High | Open (R-06 mitigation pending) |
| D-2 | DoS | gRPC BatchWrite handler | High | **Mitigated** (PR #4591) |
| D-3 | DoS | circuit_breaker.cpp | Medium | Open |
| E-1 | Privilege Escalation | raft_consensus.cpp, epoch_fencing.cpp | High | Partially mitigated (time-skew gap) |
| E-2 | Privilege Escalation | admin_api.cpp, maintenance_api_handler.cpp | Medium | Open |

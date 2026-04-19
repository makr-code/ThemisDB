> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Sharding Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/sharding/`

---

## 1. Overview

The Sharding module provides ThemisDB's horizontal scaling infrastructure: pluggable
consensus algorithms (Raft, Gossip, Multi-Paxos), hash-based and range-based shard routing,
cross-shard SAGA/2PC/3PC/Percolator transactions, automatic rebalancing, and the
`ShardRepairEngine` for self-healing shard topology.

---

## 2. Design Principles

- **Pluggable Consensus** – `ConsensusFactory` selects Raft, Gossip, or Paxos at runtime;
  each is encapsulated behind the same `IConsensusModule` interface.
- **Cross-Shard Transactions** – SAGA (compensating transactions) is the default for long-
  running cross-shard operations; 2PC/3PC are available for strong atomicity.
- **Virtual Nodes** – consistent hashing with virtual nodes enables smooth rebalancing
  without full data migration.
- **Self-Healing** – `ShardRepairEngine` continuously scans for degraded shards and
  triggers erasure-coded recovery automatically.
- **TrueTime** – `truetime.cpp` provides bounded-uncertainty timestamps for global
  consistency across geographically distributed nodes.

---

## 3. Component Architecture

### 3.1 Key Components (selected)

| File | Role |
|---|---|
| `shard_manager.cpp` (via `raft_shard_manager.cpp`) | Shard topology and routing orchestrator |
| `shard_router.cpp` | Hash/range-based shard routing |
| `adaptive_shard_router.cpp` | Load-adaptive + domain-based routing; `updateAdapterCapability()` / `routeByDomain()` |
| `consistent_hash.cpp` | Consistent hashing with virtual nodes |
| `consensus_factory.cpp` | Runtime consensus selection (Raft/Gossip/Paxos) |
| `raft_consensus.cpp` / `raft_consensus_adapter.cpp` | Raft consensus implementation |
| `gossip_protocol.cpp` / `gossip_consensus_adapter.cpp` | Gossip protocol |
| `paxos_consensus.cpp` | Multi-Paxos implementation |
| `cross_shard_transaction.cpp` | SAGA transaction coordinator |
| `two_phase_commit_coordinator.cpp` / `_participant.cpp` | 2PC implementation |
| `distributed_transaction.cpp` | Distributed transaction lifecycle |
| `shard_repair_engine.cpp` | Self-healing anti-entropy and erasure recovery |
| `gpu_erasure_coder.cpp` / `.cu` | GPU-accelerated Reed-Solomon erasure coding |
| `auto_rebalancer.cpp` | Automatic shard rebalancing |
| `data_migrator.cpp` | Live data migration between shards |
| `metadata_shard.cpp` | Horizontally partitioned metadata |
| `raft_log.cpp` / `raft_wal_integration.cpp` | Raft WAL and log management |
| `truetime.cpp` | TrueTime API for bounded-uncertainty timestamps |
| `health_monitor.cpp` / `health_check.cpp` | Cluster health monitoring |
| `shard_rpc_client.cpp` / `shard_rpc_server.cpp` | Cross-shard RPC |
| `mtls_client.cpp` / `mtls_connection_pool.cpp` | mTLS-secured inter-shard connections |
| `quorum_manager.cpp` | Quorum size management for write operations |
| `hot_spare_manager.cpp` | Hot spare management for rapid failover |
| `sharding_manager_edition.cpp` | Edition-based shard limits |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Write Request (key: "user:12345")                   │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     ShardRouter                                  │
│   consistent_hash(key) → shard_id                               │
│   adaptive routing: load-balanced selection                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  Target Shard Node                               │
│   Consensus: Raft (leader) / Gossip / Paxos                     │
│   WAL → replicate to replica nodes                              │
└──────────────────────────┬──────────────────────────────────────┘
                           │ cross-shard write?
┌──────────────────────────▼──────────────────────────────────────┐
│             CrossShardTransactionCoordinator                     │
│   SAGA: compensating transactions per shard                     │
│   2PC: prepare → commit (atomic)                                │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Hash-Based Shard Routing

```
Write(key="user:12345", value={...})
    │
    ├─ consistent_hash("user:12345") → virtual_node 47 → shard 3
    │
    ├─ shard_rpc_client.forward(shard_3, write_request)
    │
    ├─ Shard 3 leader: WAL → Raft consensus → replicate to followers
    │
    └─ ack to client
```

### 4.2 Cross-Shard SAGA

```
Transfer(from_account: shard_1, to_account: shard_3, amount: 100)
    │
    ├─ SAGA coordinator:
    │       ├─ Step 1: debit from_account (shard_1)
    │       │       → success → record compensating action: credit
    │       ├─ Step 2: credit to_account (shard_3)
    │       │       → failure →
    │       │               compensate: credit from_account (shard_1)
    │       └─ all steps succeeded → commit
    │
    └─ atomic across shards (eventual consistency for SAGA)
```

### 4.3 Shard Repair

```
ShardRepairEngine background scan:
    │
    ├─ scan all shards for degraded documents (erasure check)
    │
    ├─ degraded doc found (1 shard failed out of 3):
    │       ├─ fetch surviving chunks from healthy shards
    │       ├─ gpu_erasure_coder: Reed-Solomon decode (GPU-accelerated)
    │       └─ write recovered chunk to replacement shard
    │
    └─ prometheus metrics: repair_attempts++, repair_successes++
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/network/` | Wire protocol for inter-shard RPC |
| **Uses** | `src/replication/` | Intra-shard WAL replication |
| **Uses** | `src/storage/` | Per-shard RocksDB instance |
| **Uses** | `src/gpu/` | VRAM for GPU erasure coding |
| **Uses** | `src/observability/` | Shard health and repair metrics |
| **Provides to** | `src/server/` | Distributed query routing |
| **Provides to** | `src/transaction/` | Distributed transaction coordination |

---

## 6. Threading & Concurrency Model

- Raft leader election and log replication are event-driven (dedicated I/O thread).
- Cross-shard transaction coordinator uses a state machine per transaction.
- `ShardRepairEngine` runs on a background thread with configurable scan interval.
- `ConsistentHash` is read-only after topology changes; topology updates use exclusive lock.
- `GPUErasureCoder` uses the `src/gpu/` module's stream manager for async GPU work.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Virtual nodes | Smooth rebalancing; configurable vnodes per physical node |
| Gossip protocol | O(log N) cluster state propagation without central coordinator |
| GPU erasure coding | Reed-Solomon on GPU: 10× throughput vs. CPU for large shards |
| Adaptive routing | Routes to least-loaded shard dynamically |
| WAL batching | Multiple entries batched per Raft round-trip |

---

## 8. Security Considerations

- All inter-shard RPC uses mTLS (`mtls_client.cpp`).
- Shard RPC requests are signed (`signed_request.cpp`) to prevent replay attacks.
- PKI certificates per shard (`pki_shard_certificate.cpp`) for mutual authentication.
- Edition gates (`sharding_manager_edition.cpp`) limit shard count by deployment tier.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `sharding.consensus` | "raft" | Consensus: raft / gossip / paxos |
| `sharding.replication_factor` | 3 | Replicas per shard |
| `sharding.vnodes_per_node` | 150 | Virtual nodes for consistent hashing |
| `sharding.repair.enabled` | true | Enable ShardRepairEngine |
| `sharding.repair.scan_interval_s` | 300 | Anti-entropy scan interval |
| `sharding.gpu_erasure.enabled` | auto | GPU-accelerated erasure coding |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Shard node failure | Raft election; reroute to replica; alert |
| Cross-shard SAGA failure | Execute compensating actions; log |
| 2PC participant timeout | Abort transaction; compensate |
| Erasure recovery failure | Mark shard as FAILED; notify operator; hot spare |
| Rebalancing failure | Rollback migration; log; retry with backoff |

---

## 11. Known Limitations & Future Work

- Full RPC integration between sharding and network modules is in progress (write path uses gRPC `ReplicateData`; read path uses HTTP `RemoteExecutor`).
- Cross-region replication with WAN link optimization is planned.
- Percolator optimistic concurrency is experimental.

---

## 12. References

- `src/sharding/README.md` — module overview
- `docs/sharding/` — sharding documentation
- `docs/DISTRIBUTED_ARCHITECTURE.md` — distributed architecture overview
- `ARCHITECTURE.md` (root) — full system architecture

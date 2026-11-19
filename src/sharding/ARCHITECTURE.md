---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "architecture-governance"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/sharding/shard_router.h"
  - "../../include/sharding/distributed_coordinator.h"
  - "../../include/sharding/consensus_factory.h"
  - "../../include/sharding/cross_shard_transaction.h"
  - "../../include/sharding/wal_manager.h"
  - "../../include/sharding/auto_rebalancer.h"
  - "shard_router.cpp"
  - "cross_shard_transaction.cpp"
  - "wal_manager.cpp"
  - "../../include/sharding/detail/two_phase_commit_contract.h"
  - "../../include/sharding/two_phase_commit_coordinator.h"
  - "../../include/sharding/distributed_transaction.h"
  - "../../tests/test_two_phase_commit.cpp"
  - "../../ROADMAP.md"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Sharding Module Architecture

## Kontext

`src/sharding/` owns partition topology, shard routing, distributed coordination/consensus adapters, cross-shard transaction orchestration, shard WAL integration, and migration/rebalance/repair operations. It does not own the underlying storage engine or the separate replication module's replica lifecycle.

## Komponenten

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Topology and routing | [`ShardRouter`](../../include/sharding/shard_router.h), [`ShardTopology`](../../include/sharding/shard_topology.h), [`AdaptiveShardRouter`](../../include/sharding/adaptive_shard_router.h) | Select routing strategy and address requests against configured shard topology. |
| Coordination | [`DistributedCoordinator`](../../include/sharding/distributed_coordinator.h), [`ConsensusFactory`](../../include/sharding/consensus_factory.h), [`QuorumManager`](../../include/sharding/quorum_manager.h) | Coordinate tasks and choose among the available consensus implementations. |
| Cross-shard transactions | [`CrossShardTransactionCoordinator`](../../include/sharding/cross_shard_transaction.h), [`TwoPhaseCommitCoordinator`](../../include/sharding/two_phase_commit_coordinator.h) | Preserve both public APIs; CrossShard retains its lifecycle/SSI/multi-protocol facade while delegating 2PC participant calls, durable decisions, and replay to the canonical coordinator. |
| Durability and operations | [`WALManager`](../../include/sharding/wal_manager.h), [`WALApplier`](../../include/sharding/wal_applier.h), [`AutoRebalancer`](../../include/sharding/auto_rebalancer.h), [`DataMigrator`](../../include/sharding/data_migrator.h) | Record/apply shard WAL state and manage rebalance/migration work. |

The source contains multiple consensus adapters (including Raft, Paxos, and Gossip); this architecture does not treat one adapter as the only supported implementation.

## Schnittstellen

`ShardRouter` addresses shard requests. `CrossShardTransactionCoordinator` and
`TwoPhaseCommitCoordinator` retain separate public lifecycles; CrossShard is a facade over the
canonical 2PC execution and recovery path rather than a second 2PC decision authority.

Both public APIs are retained. `TwoPhaseCommitCoordinator::commit()` is the standalone 2PC
entry point. CrossShard's separate prepare/commit/abort lifecycle delegates participant dispatch,
durable decision recording, and phase-two replay to the canonical coordinator while retaining
transaction validation and non-2PC protocols. A prepare failure selects ABORT. The durable
decision must precede phase two; a phase-two acknowledgement failure leaves an in-doubt state and
never changes the selected decision. When canonical WAL persistence is enabled, BEGIN records
include endpoint mappings so startup can restore transport and re-drive the selected phase-two
decision. Unresolved legacy CrossShard 2PC WAL/snapshot records fail startup; the old recovery
path does not replay them as a second authority.

CrossShard `TransactionWAL` and snapshots remain responsible for its non-2PC protocols. This
adapter does not establish a general cross-coordinator WAL format or close the separate DIS-06
recovery boundary. `DistributedTransactionCoordinator`, declared in
`distributed_transaction.h`, is a third 2PC-capable public surface with its own transaction,
endpoint, and recovery paths; it is not part of this contract. No direct production instantiations
of the named pair were found in `src/` or `include/` during the DIS-05 source review.

## Datenfluss

1. A query/request is classified and routed using the current topology and routing policy.
2. Remote execution and shard-local operations return results/status to the routing/coordinator path.
3. Cross-shard transactions coordinate participant work; the CrossShard 2PC lifecycle delegates prepare, decision, and phase two to `TwoPhaseCommitCoordinator`. Its canonical WAL records participant endpoints for restart replay; CrossShard WAL/snapshots remain for other protocols.
4. Rebalance, migration, and repair components act on topology/health information and report operation state to their callers.

Evidence includes [`shard_router.cpp`](shard_router.cpp), [`cross_shard_transaction.cpp`](cross_shard_transaction.cpp), [`wal_manager.cpp`](wal_manager.cpp), and [`auto_rebalancer.cpp`](auto_rebalancer.cpp).

## Sequenz (kritischer Ablauf)

```mermaid
sequenceDiagram
    participant C as Coordinator
    participant S as Shards
    C->>S: PREPARE
    S-->>C: Votes
    C->>C: Persist selected COMMIT or ABORT
    C->>S: Apply selected decision
    S-->>C: Acknowledgements
```

Kurzinterpretation: Phase two follows only after the configured decision record
succeeds; missing acknowledgements leave that decision unresolved.

## Fehlerpfade und Resilienz

- Invalid or empty topology and unavailable target shards prevent normal routing; remote execution errors are distinct from local planning errors.
- Quorum/consensus outcomes can prevent a coordination decision. An in-doubt transaction is a separate recovery state, not a confirmed commit.
- WAL persistence/replay errors affect shard state recovery; migration/rebalance conflicts can leave an operation incomplete and require the operation-specific recovery path.
- Timeouts, circuit breakers, and health monitors are component-specific; no universal automatic failover behavior is assumed.

## Nicht-Ziele

- Owning record persistence, RocksDB internals, or general backup/PITR.
- Owning replication-manager membership and WAL-to-replica delivery.
- Query-language parsing or client transport protocol handling.
- Assuming every configured consensus backend, cloud adapter, or optional hardware path is enabled.

## Verweise

| Boundary | Evidence and link |
|---|---|
| Query federation | `query_federation.cpp` requires a `sharding::ShardRouter`; see [query architecture](../query/ARCHITECTURE.md). |
| Storage WAL | `wal_manager.cpp` includes `storage/wal_storage.h`; see [storage architecture](../storage/ARCHITECTURE.md). |
| Replication separation | Replication and sharding have separate public contracts; see [replication architecture](../replication/ARCHITECTURE.md). |

- [Module README](README.md) · [API notes](API.md) · [module roadmap](ROADMAP.md)
- [`sharding_api_contract.h`](../../include/sharding/sharding_api_contract.h) · [`shard_router.h`](../../include/sharding/shard_router.h) · [`cross_shard_transaction.h`](../../include/sharding/cross_shard_transaction.h)

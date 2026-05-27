# ADR-010: Centralised Coordinator-Side Distributed Deadlock Detection via Wait-For Graph

**Status:** Accepted  
**Date:** 2026-05-27  
**Deciders:** @makr-code  
**Modules Affected:** `src/sharding/`, `include/sharding/`  
**Related Research:**
- [Tarjan (1972) — Depth-First Search and Linear Graph Algorithms](../papers/tarjan_scc_1972.md)
- [THEMIS RAID Sharding Evaluation and Risk](../THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md) §IV-A R-13

---

## Context

Cross-shard transactions in ThemisDB can form circular lock-wait dependencies that span
multiple shards. Transaction T1 might hold lock `X` on Shard-A while waiting for lock `Y`
on Shard-B; simultaneously T2 holds `Y` on Shard-B and waits for `X` on Shard-A. Without
cluster-wide visibility neither shard's local detector can see the cycle. The result is
permanent blockage (issue #5396).

Three architectural approaches were considered for detecting such distributed deadlocks.

## Decision Drivers

- **Correctness**: every deadlock cycle must eventually be detected and resolved.
- **Simplicity**: solution must not introduce a new consensus layer or distributed
  coordination service (no ZooKeeper, etcd, or Raft ring for deadlock data).
- **Pluggability**: the same `CrossShardTransactionCoordinator` that already coordinates
  2PC/3PC/SAGA must be the natural home for this feature.
- **Testability**: detection must be triggerable deterministically in unit tests without
  real inter-process RPCs.
- **Configuration**: operators must be able to tune detection interval and victim policy.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Centralised coordinator-side WFG (chosen)** | No new services; reuses existing `ShardRPCClient`; deterministic in tests via `polled_wait_for_edge_collector` hook; O(V+E) Tarjan SCC | Polling introduces detection latency ≈ `deadlock_detection_interval`; coordinator is a single point of collection (not detection failure — only delay) |
| **B: Probe-based (Chandy-Misra-Haas)** | Reactive — zero steady-state cost; O(D) messages per probe | Requires per-shard implementation of probe propagation; harder to test; more complex failure modes |
| **C: Distributed coordination service (etcd/ZK)** | Fully decentralised; high availability | Adds operational dependency; significant complexity; overkill for current scale |

## Decision

**Chosen: Option A — Centralised coordinator-side WFG with periodic polling**

`CrossShardTransactionCoordinator::deadlockDetectionThread()` runs every
`deadlock_detection_interval` (default: 1 s) and:

1. **Merges** locally push-reported wait-for edges (`reportDistributedWait`) with
   remotely pulled edges (`ShardRPCClient::collectWaitForEdges()` per `shard_endpoints`
   entry).  Unknown/stale polled edges are filtered before analysis.
2. **Detects** cycles using Tarjan's SCC algorithm (see
   [tarjan_scc_1972.md](../papers/tarjan_scc_1972.md)): any SCC of size > 1 is a deadlock.
3. **Resolves** each independent cycle by aborting exactly one victim, selected according
   to `deadlock_victim_policy`:
   - `YOUNGEST` (default) — aborts the most recently started transaction, giving long-
     running transactions priority to complete.
   - `OLDEST` — aborts the earliest-started transaction; useful when new transactions
     should be preferred.
   - `RANDOM` — non-deterministic selection; useful as a tie-breaker baseline.

## Consequences

### Positive

- Zero new external dependencies; detection runs entirely within the existing coordinator
  process.
- `polled_wait_for_edge_collector` override enables fully deterministic unit tests
  (no real network I/O required).
- `deadlocked_transactions_` Prometheus counter exposes deadlock rate for alerting.
- `isDeadlocked()` query API allows callers to check deadlock state programmatically.
- Independent SCC cycles are each resolved in a single detection pass (one victim per
  cycle, not one victim globally).

### Negative / Trade-offs

- Detection latency is bounded below by `deadlock_detection_interval` (default 1 s).
  *Mitigation*: interval is configurable; operators can tune for latency vs. CPU trade-off.
- Polling all `shard_endpoints` adds periodic RPC fan-out.
  *Mitigation*: polling is lightweight (one HTTP/gRPC call per shard); volume scales with
  the number of shards, not with the transaction rate.
- Coordinator is a single collection point.
  *Mitigation*: collection failure only delays detection, it does not cause incorrect
  behaviour; unresolved cycles are retried every interval.

### Neutral

- The `RANDOM` policy introduces non-determinism in victim selection; this is acceptable
  because any resolved cycle is a correct outcome.

## Validation

- [x] Unit tests cover push-based detection (2-cycle), pull-based edge injection,
      `OLDEST` policy, `RANDOM` policy, and multi-cycle independent resolution.
- [x] `deadlocked_transactions_` counter incremented per victim abort.
- [x] `isDeadlocked()` query API tested.
- [x] AUDIT.md compliance row updated.
- [x] ROADMAP.md feature entry updated.
- [x] CHANGELOG `[Unreleased]` entry written.

## Follow-up Actions

- [ ] Add `deadlock_detection_latency_ms` Prometheus histogram metric (as noted in
  `THEMIS_RAID_SHARDING_EVALUATION_AND_RISK.md` §IV-A R-13 mitigation path).
- [ ] Benchmark detection interval impact on p99 transaction latency under contention.
- [ ] Evaluate Chandy-Misra-Haas probe-based detection as a future reactive alternative
  (target: v3.0.0, after shard-side RPC infrastructure matures).

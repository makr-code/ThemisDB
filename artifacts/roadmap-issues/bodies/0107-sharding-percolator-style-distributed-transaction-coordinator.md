### Context

This issue implements the roadmap item 'Percolator-Style Distributed Transaction Coordinator' for the sharding domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: [ ] Percolator-Style Distributed Transaction Coordinator

### Goal

Deliver the scoped changes for Percolator-Style Distributed Transaction Coordinator in src/sharding/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Add `PercolatorCoordinator` class to `cross_shard_transaction.cpp`; reuse `transaction_wal.cpp` for coordinator state persistence.
- [ ] `truetime.cpp` must expose a `TrueTime::now_with_uncertainty()` method returning an `[earliest, latest]` interval; commit waits until `now > commit_ts + max_uncertainty`.
- [ ] Integrate lock cleanup with `orphan_detector.cpp` to reclaim stale Percolator locks left by failed coordinators.
- [ ] `distributed_transaction.cpp` must select between 2PC and Percolator based on a per-transaction `isolation_level` hint.
- [ ] Percolator commit latency for 10-shard transaction: <20 ms P99 on LAN.
- [ ] Lock cleanup throughput via `orphan_detector.cpp`: >1000 stale locks/s.

### Relationships

- Roadmap row: #107 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#-percolator-style-distributed-transaction-coordinator
- Source key: roadmap:107:sharding:v1.7.0:percolator-style-distributed-transaction-coordinator

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:107:sharding:v1.7.0:percolator-style-distributed-transaction-coordinator -->
<!-- roadmap-ref: row=107;module=sharding;target=v1.7.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#-percolator-style-distributed-transaction-coordinator -->

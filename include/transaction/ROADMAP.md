<!-- Status: current | validated: 2026-04-06 -->

# Transaction Module — Public Header Roadmap

## Current Status

Production-ready.  v1.8.0 headers shipped 2026-03-15.  All 15 public headers are
stable under semantic versioning.

## Completed

- [x] Core ACID transaction API (`transaction_manager.h`) — v1.0.0
- [x] MVCC snapshot management (`snapshot_manager.h`) — v1.0.0
- [x] Row/range lock manager (`lock_manager.h`) — v1.0.0
- [x] Distributed 2PC transaction manager — v1.0.0
- [x] Crash recovery via WAL (`crash_recovery_manager.h`) — v1.0.0
- [x] Saga base pattern (`saga.h`) — v1.0.0
- [x] `IsolationLevel` enum header — v1.1.0
- [x] Global transaction ID registry — v1.1.0
- [x] Optimistic merge engine — v1.2.0
- [x] Transaction auditor — v1.3.0
- [x] Deadlock predictor — v1.4.0
- [x] Transaction batcher — v1.5.0
- [x] Branch manager — v1.6.0
- [x] Distributed saga + orchestrator — v1.7.0
- [x] Read-only fast path + guards on all write paths — v1.8.0

## Planned Features

- [ ] Autonomous savepoint API on `Transaction` (Target: Q2 2026)
  - `savepoint()`, `rollbackTo(SavepointId)`, `releaseSavepoint()`
  - Must integrate with MVCC snapshot nesting
- [ ] Async saga step execution in `SagaOrchestrator` (Target: Q3 2026)
  - `co_await`-based step dispatch; C++20 coroutines
  - Compensation steps must remain synchronous for safety
- [ ] Cross-shard read snapshot export (Target: Q3 2026)
  - Allow a `Snapshot` to be serialised and imported on a remote shard
  - Required for globally consistent read replicas
- [ ] Configurable group-commit window in `TransactionBatcher` (Target: Q4 2026)
  - Expose `setMaxLatencyMs()` and `setMaxBatchSize()` on the public header
- [ ] Formal TLA+ model for distributed saga compensation (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract
- Define savepoint types and IDs; extend `Transaction` interface.
- Specify coroutine traits for async saga steps.
- Snapshot serialisation format (cross-shard export).

### Phase 2 — Core Implementation
- Implement nested MVCC snapshot bookkeeping for savepoints.
- Integrate C++20 coroutine executor into `SagaOrchestrator`.
- Implement snapshot serialise/deserialise in `SnapshotManager`.

### Phase 3 — Error Handling & Edge Cases
- Savepoint rollback across lock escalations.
- Async saga step timeout and partial-completion handling.
- Snapshot import version mismatch detection.

### Phase 4 — Tests
- Unit tests: savepoint + rollback round-trips.
- Integration tests: async saga across 3-node cluster.
- Property-based tests: cross-shard snapshot consistency.

### Phase 5 — Performance / Hardening
- Savepoint overhead < 2 % on OLTP benchmarks.
- Async saga step throughput ≥ 50k steps/s per node.
- Snapshot export serialisation ≤ 1 ms for typical snapshot.

### Phase 6 — Documentation & Sign-off
- Update this ROADMAP, ARCHITECTURE, AUDIT, CHANGELOG.
- API reference Doxygen annotations on all new symbols.
- Security review of cross-shard snapshot import path.

## Production Readiness Checklist

- [x] All headers compile under C++17 with `-Wall -Wextra`
- [x] No raw owning pointer APIs
- [x] Thread-safety guarantees documented per class
- [x] `IsolationLevel` enum ABI-stable
- [x] Read-only fast path validated by benchmark suite
- [ ] Savepoint API finalised
- [ ] Async saga coroutine headers validated on all target compilers

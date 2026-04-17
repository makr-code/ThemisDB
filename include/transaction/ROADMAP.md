<!-- Status: current | validated: 2026-04-10 -->

# Transaction Module — Public Header Roadmap

## Current Status

Production-ready.  v1.9.0 headers shipped 2026-04-09.  All 17 public headers are
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
- [x] Savepoint API on `Transaction` (`createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`, `getSavepoints`, `hasSavepoint`) — v1.8.0 (already in `transaction_manager.h`)
- [x] Snapshot serialise/deserialise in `SnapshotManager` (`serialize`, `deserialize`) — v1.8.0 (partial; remote-shard import not yet wired)

## Planned Features

- [ ] Async saga step execution in `SagaOrchestrator` (Target: Q3 2026)
  - `co_await`-based step dispatch; C++20 coroutines
  - Compensation steps must remain synchronous for safety
- [ ] Cross-shard read snapshot export — remote import path (Target: Q3 2026)
  - Wire `SnapshotManager::serialize/deserialize` to remote-shard RPC
  - Required for globally consistent read replicas
- [ ] `setMaxLatencyMs()` / `setMaxBatchSize()` convenience helpers on `TransactionBatcher` (Target: Q4 2026)
  - Currently: use `setBatchConfig({.window = ..., .max_batch_size = ...})` directly
- [ ] Formal TLA+ model for distributed saga compensation (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract
- Specify coroutine traits for async saga steps.
- Remote-shard snapshot import RPC contract.

### Phase 2 — Core Implementation
- Integrate C++20 coroutine executor into `SagaOrchestrator`.
- Implement remote-shard snapshot import in `SnapshotManager`.

### Phase 3 — Error Handling & Edge Cases
- Async saga step timeout and partial-completion handling.
- Snapshot import version mismatch detection.

### Phase 4 — Tests
- Integration tests: async saga across 3-node cluster.
- Property-based tests: cross-shard snapshot consistency.

### Phase 5 — Performance / Hardening
- Async saga step throughput ≥ 50k steps/s per node.
- Snapshot export serialisation ≤ 1 ms for typical snapshot.

### Phase 6 — Documentation & Sign-off
- Update this ROADMAP, ARCHITECTURE, AUDIT, CHANGELOG.
- API reference Doxygen annotations on all new symbols.
- Security review of cross-shard snapshot import path.

### Phase 7 — TransactionSemanticAdvisor — IMPL-B5 (Target: Q3 2026)

> *Paper 2 — Layer 5: Transaction Semantics & Conflict Prediction*
> Issue: [docs/issues/optimization_layers/IMPL-B5-transaction-semantics.md](../../docs/issues/optimization_layers/IMPL-B5-transaction-semantics.md)

- [ ] New header: `include/transaction/transaction_semantic_advisor.h`
- [ ] `TransactionSemanticAdvisor::analyzeBatch(const TransactionBatch&)` — detects conflict-prone write-set patterns and suggests ordering or batching hints
- [ ] `BatchAffinityHint { hint_type, affected_keys, confidence, retry_reduction_estimate }` output struct
- [ ] `DeadlockPredictor` integration: `TransactionSemanticAdvisor` consults `DeadlockPredictor` scores to weight hints
- [ ] Writes `DecisionRecord` to `AIDecisionAuditor` for every non-trivial hint (L9 audit trail)
- [ ] GDPR field guard: advisor must not access or log GDPR-tagged field values
- [ ] Performance target: hint latency ≤ 2 ms p99 at 10 k transactions/s
- [ ] 8 unit tests `TSA-01` … `TSA-08` in `tests/test_transaction_semantic_advisor.cpp`:
  - `TSA-01` conflict detected → BatchAffinityHint with confidence ≥ 0.8
  - `TSA-02` non-conflicting batch → no hint emitted
  - `TSA-03` DeadlockPredictor high score → hint confidence boosted
  - `TSA-04` `DecisionRecord` written to `AIDecisionAuditor` for each hint
  - `TSA-05` GDPR-tagged key values absent from hint payload
  - `TSA-06` hint reduces retry count by ≥ 15 % in simulation scenario
  - `TSA-07` hint latency ≤ 2 ms on 1 000-transaction batch
  - `TSA-08` concurrency: advisor is thread-safe under 8-thread hammer test

## Production Readiness Checklist

- [x] All headers compile under C++17 with `-Wall -Wextra`
- [x] No raw owning pointer APIs
- [x] Thread-safety guarantees documented per class
- [x] `IsolationLevel` enum ABI-stable
- [x] Read-only fast path validated by benchmark suite
- [x] Savepoint API in `transaction_manager.h` — shipped v1.8.0
- [ ] Async saga coroutine headers validated on all target compilers

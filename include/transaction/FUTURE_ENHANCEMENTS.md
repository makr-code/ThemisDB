> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/transaction/FUTURE_ENHANCEMENTS.md -->

# Transaction Module — Public Header Future Enhancements

**Module Path:** `include/transaction/`
**Canonical implementation enhancements:** [`../../src/transaction/FUTURE_ENHANCEMENTS.md`](../../src/transaction/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/transaction/`. Implementation-level enhancements are in:

→ [`../../src/transaction/FUTURE_ENHANCEMENTS.md`](../../src/transaction/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` `ITransactionManager` and `IDistributedTransactionManager` must remain pure interfaces.
- `[x]` `IsolationLevel` enum values must not be reordered — downstream code uses numeric comparison.
- `[x]` `[[nodiscard]]` on all methods returning transaction handles or error codes.
- `[x]` `#pragma once` on every header.
- `[x]` Saga headers must remain compilable without network headers (no gRPC includes in saga.h).

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `ITransactionManager::begin() / commit() / rollback()` | `transaction_manager.h` | Query engine, storage, server | ✅ Stable |
| `IDistributedTransactionManager::prepare() / commit() / abort()` | `distributed_transaction_manager.h` | RPC service, sharding | ✅ Stable |
| `SnapshotManager::createSnapshot()` | `snapshot_manager.h` | MVCC store, query engine | ✅ Stable |
| `LockManager::acquireLock() / releaseLock()` | `lock_manager.h` | Transaction manager | ✅ Stable |
| `DeadlockPredictor::reportDistributedWait()` | `deadlock_predictor.h` | Cross-shard coordinator | ✅ Stable |
| `SagaOrchestrator::execute()` | `saga_orchestrator.h` | Business logic layer | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `transaction_metrics.h` — `ITransactionMetricsSink` with per-transaction counters (commit latency, abort rate, lock contention events).
- Extend `ITransactionManager` with `executeInTransaction(Callable)` — RAII scoped transaction helper that auto-rollbacks on exception.
- `transaction_context.h` — propagatable context struct for distributed trace ID, isolation level, and timeout; replaces ad-hoc parameter passing.

### Medium-Term (Q4 2026)

- `optimistic_lock_manager.h` — OCC-based validation-phase locking for read-heavy, low-contention workloads; interoperable with `ITransactionManager`.
- `saga_checkpoint.h` — durable saga checkpoint interface for pause/resume of long-running distributed sagas spanning multiple Raft epochs.
- Harden `in_doubt_recovery_coordinator.h` for multi-region partitions: add `recoverFromPartition(RegionId)` method.

### Long-Term

- Unified optimistic+pessimistic hybrid lock manager that selects strategy per-table based on observed contention history.
- Streaming saga compensations: extend `DistributedSaga` to stream partial compensation results back to the client as a `ResultStream`.

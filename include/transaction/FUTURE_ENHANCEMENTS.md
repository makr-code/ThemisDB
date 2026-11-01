---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Build:** Modular builds declare `themis_transaction` in `cmake/ModularBuild.cmake`; no `themis::transaction` alias was found. Monolithic source integration is defined in `cmake/CMakeLists.txt`.

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/transaction/FUTURE_ENHANCEMENTS.md -->

# Transaction Module — Public Header Future Enhancements

**Module Path:** `include/transaction/`
**Canonical implementation enhancements:** [`../../src/transaction/FUTURE_ENHANCEMENTS.md`](../../src/transaction/FUTURE_ENHANCEMENTS.md)
**Last Updated:** 2026-10-31
**Source level:** 1 · **SOT domain:** api-contract
**Canonical references:** `include/transaction/transaction_manager.h`, `include/transaction/distributed_transaction_manager.h`, `include/transaction/transaction_coordinator.h`. **Related issue:** #7390.

---

## Scope

Planned enhancements to the **public header contract** in `include/transaction/`. Implementation-level enhancements are in:

→ [`../../src/transaction/FUTURE_ENHANCEMENTS.md`](../../src/transaction/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Preserve the declared contracts of `TransactionManager`, `DistributedTransactionManager`, and the separately declared `ITransactionCoordinator`; `ITransactionManager` and `IDistributedTransactionManager` are not declared in the inspected headers.
- `[x]` `IsolationLevel` enum values must not be reordered — downstream code uses numeric comparison.
- `[ ]` Evaluate and document `[[nodiscard]]` use for transaction handles and status/result-returning methods; the current headers do not mark all such methods.
- `[x]` `#pragma once` on every header.
- `[x]` Saga headers must remain compilable without network headers (no gRPC includes in saga.h).

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `TransactionManager::beginTransaction() / commitTransaction() / rollbackTransaction()` | `transaction_manager.h` | TODO: verify current consumer list | Declared; stability not established here |
| `DistributedTransactionManager` participant/prepare/commit/abort API | `distributed_transaction_manager.h` | TODO: verify current consumer list | Declared; stability not established here |
| `SnapshotManager::createTag()` | `snapshot_manager.h` | TODO: verify consumers | Declared; API stability is not established here |
| `LockManager::acquireLock() / releaseLock()` | `lock_manager.h` | Transaction manager | ✅ Stable |
| `DeadlockPredictor::reportDistributedWait()` | `deadlock_predictor.h` | Cross-shard coordinator | ✅ Stable |
| `SagaOrchestrator` API | `saga_orchestrator.h` | TODO: verify consumers | Declared; stability not established here |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `transaction_metrics.h` — `ITransactionMetricsSink` with per-transaction counters (commit latency, abort rate, lock contention events).
- If a future `ITransactionManager` interface is introduced, consider an `executeInTransaction(Callable)` RAII helper that rolls back on exception; no such interface is currently declared.
- `transaction_context.h` — propagatable context struct for distributed trace ID, isolation level, and timeout; replaces ad-hoc parameter passing.

### Medium-Term (Q4 2026)

- `optimistic_lock_manager.h` — proposed OCC-based validation-phase locking for read-heavy workloads; any interoperability contract depends on a future manager interface and is not currently established.
- `saga_checkpoint.h` — durable saga checkpoint interface for pause/resume of long-running distributed sagas spanning multiple Raft epochs.
- Harden `in_doubt_recovery_coordinator.h` for multi-region partitions: add `recoverFromPartition(RegionId)` method.

### Long-Term

- Unified optimistic+pessimistic hybrid lock manager that selects strategy per-table based on observed contention history.
- Streaming saga compensations: extend `DistributedSaga` to stream partial compensation results back to the client as a `ResultStream`.

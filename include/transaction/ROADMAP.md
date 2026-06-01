> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/transaction/ROADMAP.md -->

# Transaction Module — Public Header Roadmap

**Module Path:** `include/transaction/`
**Canonical implementation roadmap:** [`../../src/transaction/ROADMAP.md`](../../src/transaction/ROADMAP.md)

---

## Overview

Tracks public transaction API contract stability, planned header additions, and breaking-change history. Feature roadmap items affecting both implementation and headers are tracked in:

→ [`../../src/transaction/ROADMAP.md`](../../src/transaction/ROADMAP.md)

---

## Current Status

All transaction headers are present and `#pragma once` guarded. `ITransactionManager` and `IDistributedTransactionManager` are stable for v1.x. `DeadlockPredictor` supports distributed wait-for edges via `reportDistributedWait`/`clearDistributedWaits`. `SagaOrchestrator` and `DistributedSaga` are production-enabled.

---

## Completed ✅

- [x] `transaction_manager.h` — local transaction interface
- [x] `global_transaction_manager.h` / `distributed_transaction_manager.h` — distributed 2PC/3PC
- [x] `isolation_level.h` — isolation level enum (ReadUncommitted → Serializable)
- [x] `snapshot_manager.h` — MVCC snapshot lifecycle
- [x] `lock_manager.h` — row- and range-level locking
- [x] `deadlock_predictor.h` — wait-for graph + distributed wait-for edges
- [x] `crash_recovery_manager.h` / `in_doubt_recovery_coordinator.h`
- [x] `saga.h` / `saga_orchestrator.h` / `distributed_saga.h` / `saga_plugin_bridge.h`
- [x] `transaction_auditor.h` / `transaction_batcher.h` / `transaction_semantic_advisor.h`
- [x] `branch_manager.h` — named branch lifecycle
- [x] `merge_engine.h` — MVCC merge resolution

---

## In Progress

- [ ] Harden `in_doubt_recovery_coordinator.h` interface for multi-region Raft partitions (Target: 2026-Q3)

---

## Planned

- [ ] `transaction_metrics.h` — structured per-transaction metrics emitted via `ITransactionMetricsSink` (Target: 2026-Q3)
- [ ] `optimistic_lock_manager.h` — OCC-style optimistic locking interface for low-contention workloads (Target: 2026-Q4)
- [ ] `saga_checkpoint.h` — durable checkpoint protocol for long-running distributed sagas (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Any breaking change requires a MAJOR version bump; see `VERSIONING.md`.

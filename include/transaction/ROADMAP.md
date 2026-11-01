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
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/transaction/ROADMAP.md -->

# Transaction Module — Public Header Roadmap

**Module Path:** `include/transaction/`
**Canonical implementation roadmap:** [`../../src/transaction/ROADMAP.md`](../../src/transaction/ROADMAP.md)
**Last Updated:** 2026-10-31; **Source level:** 1; **SOT domain:** api-contract. **Canonical references:** `include/transaction/*.h`, `src/transaction/ROADMAP.md`, and `tests/transaction/CMakeLists.txt`. **Related issue:** #7390.

---

## Overview

Tracks public transaction API contract stability, planned header additions, and breaking-change history. Feature roadmap items affecting both implementation and headers are tracked in:

→ [`../../src/transaction/ROADMAP.md`](../../src/transaction/ROADMAP.md)

---

## Current Status

Transaction-related headers are present, including `ITransactionCoordinator`, `IRecoverableTwoPhaseCoordinator`, `DistributedTransactionManager`, and saga APIs. The headers do not define `ITransactionManager` or `IDistributedTransactionManager`, nor do they establish a transaction-specific v1.x stability guarantee. `DeadlockPredictor` exposes distributed wait reporting/clearing methods. This inventory does not make a release-readiness claim.

---

## Completed ✅

- [x] `transaction_manager.h` — concrete local `TransactionManager` and nested `Transaction`
- [x] `global_transaction_manager.h` / `distributed_transaction_manager.h` — distributed coordination APIs (protocol guarantees are in each header)
- [x] `isolation_level.h` — isolation level enum (ReadUncommitted → Serializable)
- [x] `snapshot_manager.h` — MVCC snapshot lifecycle
- [x] `lock_manager.h` — row- and range-level locking
- [x] `deadlock_predictor.h` — wait-for graph + distributed wait-for edges
- [x] `crash_recovery_manager.h` / `in_doubt_recovery_coordinator.h`
- [x] `saga.h` / `saga_orchestrator.h` / `distributed_saga.h` / `saga_plugin_bridge.h`
- [x] `transaction_auditor.h` / `transaction_batcher.h` / `transaction_semantic_advisor.h`
- [x] `branch_manager.h` — named branch lifecycle
- [x] `merge_engine.h` — MVCC merge resolution
- [x] `transaction_coordinator.h` — `ITransactionCoordinator` declaration and protocol identifiers (Issue #5374); concrete support for every enum protocol is not implied

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

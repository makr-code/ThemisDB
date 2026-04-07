# Transaktions-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/transaction/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** ACID-Transaktionen  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Transaktions-Modul stellt ACID-konforme Transaktionsverwaltung für ThemisDB bereit, einschließlich verteilter Sagas, Deadlock-Erkennung und konfigurierbarer Isolationsebenen.

**Primäre Quelle:** [`src/transaction/`](../../../src/transaction/) · [`include/transaction/`](../../../include/transaction/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| TransactionManager | `transaction_manager.h` | *(impl. in global_transaction_manager)* | Lokale Transaktionsverwaltung |
| GlobalTransactionManager | `global_transaction_manager.h` | `global_transaction_manager.cpp` | Verteilte Transaktionskoordination (2PC) |
| DistributedTransactionManager | `distributed_transaction_manager.h` | `distributed_transaction_manager.cpp` | Cross-Shard-Transaktionskoordination |
| LockManager | `lock_manager.h` | `lock_manager.cpp` | Pessimistisches Locking (Row/Table/Range) |
| DeadlockPredictor | `deadlock_predictor.h` | `deadlock_predictor.cpp` | Proaktive Deadlock-Erkennung und -Auflösung |
| Saga | `saga.h` | `saga.cpp` | Saga-Muster für lange Transaktionen |
| DistributedSaga | `distributed_saga.h` | `distributed_saga.cpp` | Verteilte Saga-Orchestrierung |
| SagaOrchestrator | `saga_orchestrator.h` | `saga_orchestrator.cpp` | Saga-Step-Koordination und Kompensation |
| CrashRecoveryManager | `crash_recovery_manager.h` | `crash_recovery_manager.cpp` | WAL-basierte Crash-Recovery |
| BranchManager | `branch_manager.h` | `branch_manager.cpp` | XA-Transaktionszweige |
| MergeEngine | `merge_engine.h` | `merge_engine.cpp` | MVCC-Merge und Snapshot-Isolation |
| TransactionBatcher | `transaction_batcher.h` | *(impl. in transaction_manager)* | Transaktions-Batching für Durchsatzoptimierung |
| TransactionAuditor | `transaction_auditor.h` | *(impl. in global_transaction_manager)* | Audit-Log für Transaktionen |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/transaction/README.md`](../../../src/transaction/README.md) | Modulübersicht |

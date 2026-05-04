# Transaktions-Modul

<!-- Status: current | validated: 2026-04-10 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/transaction/README.md · ../../../src/transaction/ROADMAP.md -->

**Stand:** 10. April 2026  
**Version:** v1.9.0  
**Kategorie:** ACID-Transaktionen  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Transaktions-Modul stellt ACID-konforme Transaktionsverwaltung für ThemisDB bereit.
Es umfasst MVCC-Snapshot-Isolation, drei Isolationsebenen (Read Committed, Snapshot, Serializable/SSI),
Optimistic Concurrency Control (OCC), Two-Phase Commit (2PC) für Cross-Shard-Transaktionen,
SAGA-Orchestrierung (sequential und parallel/DAG), Named Savepoints, Write Batching,
Read-Only Fast Path und ein Transaction Audit Trail.

**Primäre Quelle:** [`src/transaction/`](../../../src/transaction/) · [`include/transaction/`](../../../include/transaction/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| TransactionManager | `transaction_manager.h` | `transaction_manager.cpp` | ACID-Transaktionen, Savepoints, OCC, Bulk-API, Read-Only-Fast-Path |
| GlobalTransactionManager | `global_transaction_manager.h` | `global_transaction_manager.cpp` | Multi-Region ACID mit TrueTime-2PC |
| DistributedTransactionManager | `distributed_transaction_manager.h` | `distributed_transaction_manager.cpp` | Cross-Shard 2PC-Koordinator (v1.9.0, PERF-D4) |
| LockManager | `lock_manager.h` | `lock_manager.cpp` | Pessimistisches Locking (Row/Table/Range) |
| DeadlockPredictor | `deadlock_predictor.h` | `deadlock_predictor.cpp` | ML-inspirierte Deadlock-Prognose und Timeout-Empfehlung |
| SnapshotManager | `snapshot_manager.h` | `snapshot_manager.cpp` | Named Snapshots/Tags für PITR, serialize/deserialize |
| CrashRecoveryManager | `crash_recovery_manager.h` | `crash_recovery_manager.cpp` | WAL-basierte Crash-Recovery |
| Saga | `saga.h` | `saga.cpp` | Saga-Basis-Muster mit Compensating Actions |
| DistributedSaga | `distributed_saga.h` | `distributed_saga.cpp` | Verteilte Saga-Koordination über Services |
| SagaOrchestrator | `saga_orchestrator.h` | `saga_orchestrator.cpp` | DAG-basierte parallele Saga-Ausführung, Retry, Conditional Steps |
| BranchManager | `branch_manager.h` | `branch_manager.cpp` | Git-ähnliches Branching und Merging |
| MergeEngine | `merge_engine.h` | `merge_engine.cpp` | Konflikt-bewusster Branch-Merge |
| TransactionBatcher | `transaction_batcher.h` | `transaction_batcher.cpp` | Write-Batching mit konfigurierbarem Batch-Window, adaptiver Größe |
| TransactionAuditor | `transaction_auditor.h` | `transaction_auditor.cpp` | Append-Only Audit-Log, queryAuditLog(), exportToKafka()/exportToS3() (Stubs) |

---

## Test-Abdeckung

| Testdatei | Suite | Testfälle | Beschreibung |
|-----------|-------|-----------|--------------|
| `test_transaction_manager.cpp` | `TransactionManagerFocusedTests` | — | Kern-ACID, OCC, Savepoints, Timeouts |
| `test_transaction_manager_comprehensive.cpp` | — | — | Erweiterte Integration |
| `test_transaction_distributed_2pc.cpp` | `DistributedTxnManagerTest` + `Distributed2PCPerfTests` | **43** | 2PC-Koordinator + PERF-D4 Performance |
| `test_adaptive_deadlock_prevention.cpp` | `AdaptiveDeadlockPreventionFocusedTests` | **34** | DeadlockPredictor API |
| `test_transaction_batcher.cpp` | `TransactionBatcherFocusedTests` | **26** | Write-Batching, Adaptive Window |
| `test_transaction_auditor.cpp` | `TransactionAuditorFocusedTests` | **25** | Audit-Log, Query, Thread-Safety |
| `test_savepoints.cpp` | — | **20** | Named Savepoints |
| `test_transaction_bulk.cpp` | — | **12** | Bulk-API Atomizität |
| `test_transaction_occ.cpp` | `OccTest` | **13** | OCC Version-Tracking und Konflikt-Erkennung |
| `test_transaction_ssi.cpp` / `test_ssi_predicate_locking.cpp` | — | — | SSI Predicate Locking |
| `test_distributed_saga.cpp` | — | — | DistributedSaga DAG, Retry, Compensation |
| `test_saga_orchestrator.cpp` | — | — | SagaOrchestrator Steps |
| `test_saga_operation.cpp` | — | **8** | SAGA-Kompensation für Index/Graph/Vektor |
| `test_global_transaction_manager.cpp` | — | — | TrueTime-2PC Multi-Region |
| `test_transaction_isolation_levels.cpp` | `TransactionIsolationLevelsFocusedTests` | — | Isolationsebenen |

---

## Offene Punkte / Stubs

| Komponente | Befund | Kritikalität |
|------------|--------|--------------|
| `TransactionAuditor::exportToKafka()` | Gibt `Status::Error("not yet implemented")` zurück | niedrig |
| `TransactionAuditor::exportToS3()` | Gibt `Status::Error("not yet implemented")` zurück | niedrig |
| `SnapshotManager::serialize/deserialize` | Lokal implementiert; Cross-Shard-Import nicht verdrahtet | niedrig |
| Async Saga Steps (`co_await`) | Nicht implementiert; geplant Q3 2026 | offen |

Detaillierter Report: [`MISSING_IMPLEMENTATIONS.md`](./MISSING_IMPLEMENTATIONS.md) · [`missing-implementations.json`](./missing-implementations.json)

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/transaction/README.md`](../../../src/transaction/README.md) | Modulübersicht, Delivery-Status |
| [`src/transaction/ROADMAP.md`](../../../src/transaction/ROADMAP.md) | Phasenstatus, alle Feature-Einträge |
| [`src/transaction/FUTURE_ENHANCEMENTS.md`](../../../src/transaction/FUTURE_ENHANCEMENTS.md) | Geplante Features, Design-Constraints, Referenzen |
| [`src/transaction/ARCHITECTURE.md`](../../../src/transaction/ARCHITECTURE.md) | Architektur-Details |
| [`src/transaction/SECURITY.md`](../../../src/transaction/SECURITY.md) | Sicherheitsrichtlinien |
| [`include/transaction/ROADMAP.md`](../../../include/transaction/ROADMAP.md) | Header-Interface-Roadmap |

---

## Inventar

Vollständiges Datei-Inventar: [`inventory.md`](./inventory.md)

---

## Navigation

[docs](../../index.md) › [de](../index.md) › [transaction](./README.md)

# Transaktions-Modul — Primäres Inventar

<!-- Status: current | validated: 2026-04-10 -->
<!-- Primärdokumentation: ../../../src/transaction/ | ../../../include/transaction/ -->

**Datum:** 10. April 2026  
**Modul:** `transaction`  
**Modulpfad:** `src/transaction/` + `include/transaction/`

---

## 1. Dokumentationsdateien im Modul

### `src/transaction/`

| Datei | Beschreibung |
|-------|--------------|
| `src/transaction/README.md` | Modulübersicht, Komponentenliste, Delivery-Status |
| `src/transaction/ROADMAP.md` | Implementierungsstatus Phasen 1–8 + PERF-D4, Production-Readiness-Checkliste |
| `src/transaction/FUTURE_ENHANCEMENTS.md` | Geplante Features, Design-Constraints, Teststrategien, Performance-Ziele, Sicherheits-Hinweise |
| `src/transaction/ARCHITECTURE.md` | Architekturdetails: MVCC, Lock-Hierarchie, SAGA-Ausführungsmodell, 2PC-Protokoll |
| `src/transaction/AUDIT.md` | Sicherheitsaudit-Protokoll, abgedeckte Bedrohungen |
| `src/transaction/SECURITY.md` | Sicherheitsrichtlinien: Isolation Boundaries, SAGA-Compensation Safety, Audit-Log-Schutz |
| `src/transaction/CHANGELOG.md` | Versionierter Änderungsnachweis |

### `include/transaction/`

| Datei | Beschreibung |
|-------|--------------|
| `include/transaction/ROADMAP.md` | Header-Interface-Roadmap (öffentliche API-Stabilität) |
| `include/transaction/FUTURE_ENHANCEMENTS.md` | Header-seitige geplante Erweiterungen |
| `include/transaction/README.md` | Übersicht der öffentlichen Header |
| `include/transaction/ARCHITECTURE.md` | Header-Architektur-Hinweise |
| `include/transaction/AUDIT.md` | Header-Audit-Informationen |
| `include/transaction/SECURITY.md` | Header-seitige Sicherheitshinweise |
| `include/transaction/CHANGELOG.md` | Header-Änderungsnachweis |

---

## 2. Quellcode-Dateien (`src/transaction/`)

| Quelldatei | Header | Beschreibung | Status |
|------------|--------|--------------|--------|
| `transaction_manager.cpp` | `transaction_manager.h` | ACID-Transaktionen, MVCC, OCC, Savepoints, Bulk-API, Read-Only-Fast-Path, SSI | 🟢 Production-Ready |
| `distributed_transaction_manager.cpp` | `distributed_transaction_manager.h` | 2PC-Koordinator für Cross-Shard-Transaktionen; Thread-Pool (PERF-D4), Batch-Prepare-Window | 🟢 Production-Ready |
| `global_transaction_manager.cpp` | `global_transaction_manager.h` | Multi-Region ACID mit TrueTime-2PC | 🟢 Production-Ready |
| `lock_manager.cpp` | `lock_manager.h` | Row/Table/Range-Locks, Shrinking-Phase-Enforcement (SSI) | 🟢 Production-Ready |
| `deadlock_predictor.cpp` | `deadlock_predictor.h` | ML-inspirierte Deadlock-Prognose (Pair-Conflict-Matrix), Timeout-Empfehlung | 🟢 Production-Ready |
| `snapshot_manager.cpp` | `snapshot_manager.h` | Named Snapshots/Tags für PITR; serialize/deserialize (lokal); CDC-Integration | 🟢 Production-Ready |
| `crash_recovery_manager.cpp` | `crash_recovery_manager.h` | WAL-basierte Crash-Recovery | 🟢 Production-Ready |
| `saga.cpp` | `saga.h` | SAGA-Basis-Muster mit Compensating Actions | 🟢 Production-Ready |
| `saga_orchestrator.cpp` | `saga_orchestrator.h` | DAG-basierte parallele Saga-Ausführung, Retry-Policies, Conditional Steps | 🟢 Production-Ready |
| `distributed_saga.cpp` | `distributed_saga.h` | Cross-Service-SAGA-Koordination, Compensation-Retry | 🟢 Production-Ready |
| `branch_manager.cpp` | `branch_manager.h` | Git-ähnliches Branching/Merging | 🟢 Production-Ready |
| `merge_engine.cpp` | `merge_engine.h` | Konflikt-bewusster Branch-Merge | 🟢 Production-Ready |
| `transaction_batcher.cpp` | `transaction_batcher.h` | Write-Batching mit konfigurierbarem Window (1–100 ms), Adaptive Sizing, Per-Table-Policy | 🟢 Production-Ready |
| `transaction_auditor.cpp` | `transaction_auditor.h` | Append-Only Audit-Log; exportToKafka/exportToS3 als **Stubs** | 🟠 Beta (Export-Stubs) |

---

## 3. Test-Dateien

| Testdatei | Testsuiten | Testfälle | Modul |
|-----------|-----------|-----------|-------|
| `tests/test_transaction_manager.cpp` | `TransactionManagerFocusedTests` | — | TransactionManager |
| `tests/test_transaction_manager_comprehensive.cpp` | — | — | TransactionManager |
| `tests/test_transaction_distributed_2pc.cpp` | `DistributedTxnManagerTest`, `Distributed2PCPerfTests` | **43** | DistributedTransactionManager |
| `tests/test_adaptive_deadlock_prevention.cpp` | `AdaptiveDeadlockPreventionFocusedTests` | **34** | DeadlockPredictor |
| `tests/test_transaction_batcher.cpp` | `TransactionBatcherFocusedTests` | **26** | TransactionBatcher |
| `tests/test_transaction_auditor.cpp` | `TransactionAuditorFocusedTests` | **25** | TransactionAuditor |
| `tests/test_savepoints.cpp` | — | **20** | Transaction Savepoints |
| `tests/test_transaction_bulk.cpp` | — | **12** | Bulk-API |
| `tests/test_transaction_occ.cpp` | `OccTest` | **13** | OCC |
| `tests/test_transaction_isolation_levels.cpp` | `TransactionIsolationLevelsFocusedTests` | — | Isolationsebenen |
| `tests/test_transaction_isolation.cpp` | `DbTransactionIsolationFocusedTests` | — | Isolationsebenen |
| `tests/test_transaction_ssi.cpp` | — | — | SSI |
| `tests/test_ssi_predicate_locking.cpp` | — | — | SSI Predicate Locking |
| `tests/test_transaction_timeout.cpp` | — | — | Transaction Timeout |
| `tests/test_transaction_retry.cpp` | — | — | Transaction Retry |
| `tests/test_distributed_saga.cpp` | — | — | DistributedSaga |
| `tests/test_saga_orchestrator.cpp` | — | — | SagaOrchestrator |
| `tests/test_saga_operation.cpp` | — | **8** | SAGA-Compensation |
| `tests/test_saga_logger.cpp` | `SAGALoggerFocusedTests` | — | SAGA Logger |
| `tests/test_saga_compactor.cpp` | `SAGACompactorFocusedTests` | — | SAGA Compactor |
| `tests/test_saga_concurrent_execution.cpp` | — | — | Parallele SAGA-Ausführung |
| `tests/test_global_transaction_manager.cpp` | — | — | GlobalTransactionManager |
| `tests/test_distributed_transactions.cpp` | `DistributedTransactionsFocusedTests` | — | Verteilte Transaktionen |
| `tests/test_multi_shard_transactions.cpp` | `MultiShardTransactionFocusedTests` | — | Multi-Shard |
| `tests/test_sharding_transaction_wal.cpp` | `ShardingTransactionWALFocusedTests` | — | Sharding WAL |
| `tests/test_aql_multi_statement_transaction.cpp` | `AQLMultiStatementTransactionFocusedTests` | — | AQL-Transaktionen |
| `tests/test_postgres_transactions.cpp` | `PostgresTransactionFocusedTests` | — | Postgres-Adapter-Transaktionen |
| `tests/test_tenant_transaction_namespace.cpp` | — | — | Mandanten-Isolation |

---

## 4. CI-Workflow-Dateien

| Datei | Beschreibung |
|-------|--------------|
| `.github/workflows/02-feature-modules_transactions_transaction-distributed-2pc-ci.yml` | 2PC-Koordinator CI |
| `.github/workflows/02-feature-modules_transactions_adaptive-deadlock-prevention-ci.yml` | Deadlock Prevention CI |
| `.github/workflows/02-feature-modules_transactions_transaction-write-batching-ci.yml` | Write Batching CI |
| `.github/workflows/02-feature-modules_transactions_transaction-audit-trail-ci.yml` | Audit Trail CI |
| `.github/workflows/02-feature-modules_transactions_transaction-occ-ci.yml` | OCC CI |
| `.github/workflows/02-feature-modules_transactions_transaction-ssi-ci.yml` | SSI CI |
| `.github/workflows/02-feature-modules_transactions_transaction-savepoints-ci.yml` | Savepoints CI |
| `.github/workflows/02-feature-modules_transactions_saga-orchestration-engine-ci.yml` | SAGA Orchestration CI |
| `.github/workflows/02-feature-modules_transactions_transaction-saga-orchestration-ci.yml` | SAGA Integration CI |
| `.github/workflows/02-feature-modules_transactions_percolator-distributed-transaction-coordinator-ci.yml` | Percolator CI |

---

*Generiert: 2026-04-10 · Modul: `transaction`*

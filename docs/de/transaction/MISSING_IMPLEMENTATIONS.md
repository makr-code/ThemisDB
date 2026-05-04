# Transaktions-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-10 -->
<!-- Primärdokumentation: ../../../src/transaction/ -->

Dieser Report dokumentiert Befunde aus dem Reality-Check (Primary-Docs ↔ Sourcecode)
für das `transaction`-Modul. Prüfstand: 2026-04-10 | Branch: `copilot/update-module-transaction-documentation`

---

## 1. TransactionAuditor — exportToKafka / exportToS3 sind Stubs (TXN-MISSING-001)

| Feld | Wert |
|------|------|
| **Claim-Quelle** | `src/transaction/ROADMAP.md` Phase 8; `src/transaction/FUTURE_ENHANCEMENTS.md` §"Transaction Audit Trail" |
| **Erwartet** | `TransactionAuditor::exportToKafka(topic)` exportiert den Audit-Log an einen Kafka-Topic; `exportToS3(bucket, prefix)` lädt in S3-kompatibler Speicher hoch |
| **Beobachtet** | Beide Methoden geben `Status::Error("exportToKafka: not yet implemented")` bzw. `Status::Error("exportToS3: not yet implemented")` zurück — keine echte Verbindung |
| **Evidence** | `src/transaction/transaction_auditor.cpp`, Zeilen 98–109 |
| **Roadmap-Status** | In ROADMAP.md Phase 8 korrekt als "placeholder stubs" beschrieben (`exportToKafka()` / `exportToS3()` placeholder stubs returning `Status::Error`) |
| **Kritikalität** | niedrig — als Stub explizit dokumentiert |
| **Vorschlag** | Issue: *[transaction] Implement TransactionAuditor::exportToKafka / exportToS3 real integration* · Labels: `type:feature`, `priority:low`, `area:transaction` |

---

## 2. SnapshotManager::serialize/deserialize — kein Cross-Shard-Import (TXN-MISSING-002)

| Feld | Wert |
|------|------|
| **Claim-Quelle** | `include/transaction/ROADMAP.md` Planned Features §"Cross-shard read snapshot export" |
| **Erwartet** | `SnapshotManager::serialize()` / `deserialize()` erlauben den Transport eines `Snapshot` auf einen Remote-Shard via RPC für globally consistent read replicas |
| **Beobachtet** | `serialize()` / `deserialize()` sind implementiert (`include/transaction/snapshot_manager.h` Zeilen 273–278), jedoch nur lokal; kein RPC-Transport oder Remote-Import-Pfad |
| **Evidence** | `include/transaction/snapshot_manager.h:273–278` |
| **Roadmap-Status** | Offen — in `include/transaction/ROADMAP.md` als Planned Feature `[ ]` geführt (Target: Q3 2026) |
| **Kritikalität** | niedrig — explizit als zukünftiges Feature geplant |
| **Vorschlag** | Issue: *[transaction] Wire SnapshotManager serialize/deserialize to cross-shard RPC import* · Labels: `type:feature`, `priority:low`, `area:transaction`, `area:sharding` |

---

## 3. SagaOrchestrator — kein Async/Coroutine-Step-Dispatch (TXN-MISSING-003)

| Feld | Wert |
|------|------|
| **Claim-Quelle** | `include/transaction/ROADMAP.md` Planned Features §"Async saga step execution in SagaOrchestrator" |
| **Erwartet** | `co_await`-basierter Step-Dispatch; C++20 Coroutinen für asynchrone Saga-Steps |
| **Beobachtet** | `SagaOrchestrator` verwendet synchrone `std::function<void()>` für Forward- und Compensate-Actions; keine Coroutinen-Unterstützung |
| **Evidence** | `include/transaction/saga_orchestrator.h` (SAGAStep::forward / compensate = `std::function<void()>`) |
| **Roadmap-Status** | Offen — in `include/transaction/ROADMAP.md` als Planned Feature `[ ]` geführt (Target: Q3 2026) |
| **Kritikalität** | niedrig — explizit als zukünftiges Feature geplant |
| **Vorschlag** | Issue: *[transaction] Add C++20 coroutine-based async step dispatch to SagaOrchestrator* · Labels: `type:feature`, `priority:low`, `area:transaction` |

---

## 4. ROADMAP-Testanzahl-Drift: 2PC (32 → 43), OCC (11 → 13), Deadlock (keine Zahl → 34) (TXN-DRIFT-001)

| Feld | Wert |
|------|------|
| **Claim-Quelle** | `src/transaction/ROADMAP.md` Phase 6 ("32 tests"); `src/transaction/FUTURE_ENHANCEMENTS.md` §OCC ("11 unit tests") |
| **Erwartet** | Test-Anzahlen stimmen mit tatsächlichen Tests überein |
| **Beobachtet** | `tests/test_transaction_distributed_2pc.cpp`: 43 Tests (32 in `DistributedTxnManagerTest` + 11 in `Distributed2PCPerfTests` durch PERF-D4); `tests/test_transaction_occ.cpp`: 13 Tests; `tests/test_adaptive_deadlock_prevention.cpp`: 34 Tests |
| **Evidence** | `grep -c "^TEST_F\|^TEST("` auf den jeweiligen Testdateien |
| **Status** | ✅ **Behoben** — ROADMAP.md und FUTURE_ENHANCEMENTS.md in diesem PR korrigiert |
| **Kritikalität** | niedrig — nur Dokumentationsdrift, keine funktionalen Auswirkungen |

---

## 5. include/transaction/ROADMAP.md: Savepoint-API als offen markiert, obwohl implementiert (TXN-DRIFT-002)

| Feld | Wert |
|------|------|
| **Claim-Quelle** | `include/transaction/ROADMAP.md` Planned Features §"Autonomous savepoint API on Transaction" |
| **Erwartet** | `[ ]`-Checkbox deutet auf nicht implementiert hin |
| **Beobachtet** | `createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`, `getSavepoints`, `hasSavepoint` sind bereits in `include/transaction/transaction_manager.h` (Zeilen 354–386) implementiert und in Tests abgedeckt |
| **Evidence** | `include/transaction/transaction_manager.h:354–386`; `tests/test_savepoints.cpp` (20 Tests) |
| **Status** | ✅ **Behoben** — `include/transaction/ROADMAP.md` in diesem PR auf `[x]` korrigiert |
| **Kritikalität** | niedrig — nur Dokumentationsdrift |

---

## Zusammenfassung

| ID | Titel | Kritikalität | Status |
|----|-------|--------------|--------|
| TXN-MISSING-001 | `exportToKafka` / `exportToS3` sind Stubs | niedrig | offen |
| TXN-MISSING-002 | Cross-Shard-Snapshot-Import nicht implementiert | niedrig | offen (Planned Q3 2026) |
| TXN-MISSING-003 | Kein Async/Coroutine-Step-Dispatch in SagaOrchestrator | niedrig | offen (Planned Q3 2026) |
| TXN-DRIFT-001 | Testanzahl-Drift in ROADMAP/FUTURE_ENHANCEMENTS | niedrig | ✅ behoben |
| TXN-DRIFT-002 | Savepoint-API als offen markiert, obwohl implementiert | niedrig | ✅ behoben |

**Fazit:** Das Transaktions-Modul ist production-ready. Alle wesentlichen Features (2PC, SAGA, OCC, SSI, Savepoints, Deadlock-Prävention, Write-Batching, Audit-Trail) sind vollständig implementiert. Offene Punkte betreffen ausschließlich explizit als Stubs oder als zukünftige Features dokumentierte Bereiche.

<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · PRIMARY_SOURCES.md · ../../../src/maintenance/ROADMAP.md -->

# Fehlende Implementierungen — Maintenance-Modul

**Erstellt:** 2026-04-15  
**Auditor:** Copilot  
**Modul-Version:** v2.0.0  
**Basis:** Reality-Check gegen Quellcode (Issue #4463)

---

## Zusammenfassung

| ID | Claim-Quelle | Behaupteter Stand | Beobachteter Stand | Schwere |
|----|--------------|-------------------|--------------------|---------|
| MI-01 | `src/maintenance/ROADMAP.md` Planned | REPLICA_VALIDATION wird an Sharding-Modul verdrahtet | Handler-Klasse vorhanden; Startup-Aufruf fehlt | Mittel |
| MI-02 | `src/maintenance/ROADMAP.md` Long-term | Raft-basierte `IDistributedLock`-Implementierung | Nur `InProcessDistributedLock` verfügbar | Mittel |
| MI-03 | `src/maintenance/FUTURE_ENHANCEMENTS.md` | TSAN-Test: 8 parallele `listSchedules` + 1 `createSchedule` | Kein TSAN-Test gefunden in `test_database_maintenance_orchestrator.cpp` | Gering |
| MI-04 | `src/maintenance/FUTURE_ENHANCEMENTS.md` | Restart-Persistenz-Integrationstest (create 3, restart, verify 3) | Kein expliziter Restart-Integrationstest in Unit-Test-Datei | Gering |
| MI-05 | `src/maintenance/FUTURE_ENHANCEMENTS.md` | ML-Wartungsauswirkungs-Vorhersage | Nicht implementiert | Gering (v3.0.0) |

---

## Detaillierte Einträge

### MI-01 — REPLICA_VALIDATION Startup-Wiring

**Claim-Quelle:** `src/maintenance/ROADMAP.md` § "Planned Features / Short-term (v1.2.0)"  
**Zitat:** *"Wire `REPLICA_VALIDATION` task to sharding/replica module once available"*

**Erwartet:**  
`orchestrator.registerTaskHandler(REPLICA_VALIDATION, make_shared<ReplicaValidationHandler>(replica_mgr))` in `http_server.cpp` oder äquivalentem Startup-Code.

**Beobachtet:**  
- `ReplicaValidationHandler` existiert: `include/maintenance/maintenance_task_handler_impls.h` ✅  
- Startup-Aufruf in `http_server.cpp` fehlt (kein `registerTaskHandler(REPLICA_VALIDATION, ...)` gefunden)  
- Unregistrierter Task-Typ: `executeTask()` gibt SKIPPED zurück (korrekte Fallback-Logik)

**Evidence:**  
```
grep -r "REPLICA_VALIDATION" src/ include/ → nur Handler-Klasse, kein registerTaskHandler-Aufruf
```

**Issue-Titelvorschlag:** `[maintenance] Wire REPLICA_VALIDATION handler to sharding module at startup`  
**Label-Vorschläge:** `area:maintenance`, `type:integration`, `priority:medium`

---

### MI-02 — Raft-basierte IDistributedLock-Implementierung

**Claim-Quelle:** `src/maintenance/ROADMAP.md` § "Long-term (v2.0.0)"  
**Zitat:** *"Distributed maintenance coordination via Raft – prevent two nodes running same schedule"*

**Erwartet:**  
Produktive Raft-basierte `IDistributedLock`-Implementierung, die `tryAcquire/release` an `src/replication/raft_v2.cpp` delegiert.

**Beobachtet:**  
- `IDistributedLock` Interface: `include/maintenance/i_distributed_lock.h` ✅  
- `InProcessDistributedLock` für Einzelknoten/Tests: `include/maintenance/i_distributed_lock.h` ✅  
- Raft-basierte Implementierung: **nicht vorhanden** in `src/replication/` oder `src/maintenance/`

**Evidence:**  
```
grep -r "IDistributedLock" src/replication/ → keine Treffer
find src/replication/ -name "*.h" -o -name "*.cpp" | xargs grep -l "IDistributedLock" → keine Treffer
```

**Issue-Titelvorschlag:** `[maintenance] Implement Raft-backed IDistributedLock for production multi-node`  
**Label-Vorschläge:** `area:maintenance`, `area:replication`, `type:feature`, `priority:medium`, `milestone:v2.1.0`

---

### MI-03 — TSAN-Konkurrenz-Test

**Claim-Quelle:** `src/maintenance/FUTURE_ENHANCEMENTS.md` § "`schedules_mutex_` Read-Path Upgrade"  
**Zitat:** *"Add a TSAN-enabled test with 8 concurrent `listSchedules` threads + 1 `createSchedule` thread"*

**Erwartet:**  
Testfall mit 8 parallelen `listSchedules`-Threads und 1 `createSchedule`-Thread (TSAN-kompatibel).

**Beobachtet:**  
- `tests/test_database_maintenance_orchestrator.cpp`: kein dedizierter Konkurrenz-Stresstest dieser Art gefunden  
- Allgemeine Thread-Safety-Tests vorhanden, aber kein gezielter 8+1 Reader/Writer-Test

**Evidence:**  
```
grep -n "listSchedules.*thread\|concurrent.*listSchedules" tests/test_database_maintenance_orchestrator.cpp → keine Treffer
```

**Issue-Titelvorschlag:** `[maintenance] Add TSAN concurrent-read stress test (8 listSchedules + 1 createSchedule)`  
**Label-Vorschläge:** `area:maintenance`, `type:test`, `priority:low`

---

### MI-04 — Restart-Persistenz-Integrationstest

**Claim-Quelle:** `src/maintenance/FUTURE_ENHANCEMENTS.md` § "Schedule Persistence (RocksDB)"  
**Zitat:** *"Add a restart-persistence integration test: create 3 schedules, restart the orchestrator, verify all 3 are present."*

**Erwartet:**  
Integrationstest: erstellt 3 Pläne → stoppt Orchestrator → startet neu → prüft, dass alle 3 Pläne geladen wurden.

**Beobachtet:**  
- `MaintenanceScheduleStore` Unit-Tests vorhanden  
- Kein expliziter "restart + loadAll" Integrationstest mit echtem RocksDB-Backend gefunden in `tests/test_database_maintenance_orchestrator.cpp`

**Evidence:**  
```
grep -n "restart\|loadAll\|load_all" tests/test_database_maintenance_orchestrator.cpp → keine Treffer
```

**Issue-Titelvorschlag:** `[maintenance] Add restart-persistence integration test for MaintenanceScheduleStore`  
**Label-Vorschläge:** `area:maintenance`, `type:test`, `priority:low`

---

### MI-05 — ML-basierte Wartungsauswirkungs-Vorhersage

**Claim-Quelle:** `src/maintenance/ROADMAP.md` § "Long-term / Planned"  
**Zitat:** *"Maintenance impact prediction – ML model to predict CPU/memory impact before execution (Target: v3.0.0)"*

**Erwartet:**  
ML-Modell, das CPU/Memory-Auswirkung vor der Ausführung vorhersagt.

**Beobachtet:**  
- Keinerlei Implementierung vorhanden (erwartet für v3.0.0 — planmäßig noch nicht fällig)

**Issue-Titelvorschlag:** `[maintenance] ML-based maintenance impact prediction (CPU/memory forecast)`  
**Label-Vorschläge:** `area:maintenance`, `type:feature`, `priority:low`, `milestone:v3.0.0`

---

## Abgezeichnete / Falsch als fehlend markierte Punkte

Die folgenden Punkte waren in älteren Dokumenten als offen markiert, sind aber **implementiert**:

| Punkt | Implementierungs-Evidence |
|-------|--------------------------|
| RocksDB-Persistenz | `src/maintenance/maintenance_schedule_store.cpp`, `include/maintenance/maintenance_schedule_store.h` |
| Force-Run `{"force": true}` | `database_maintenance_orchestrator.h::triggerNow(id, force)`, Tests in `test_database_maintenance_orchestrator.cpp` |
| DAG-Abhängigkeitsgraph | `maintenance_schedule.h::MaintenanceTaskDependency`, `resolveTaskExecutionOrder()` |
| STORAGE_COMPACTION verdrahtet | `http_server.cpp`, Issue #4587 |
| MVCC_CLEANUP verdrahtet | `http_server.cpp`, Issue #4586 |
| `IMaintenanceTaskHandler`-Registry | `registerTaskHandler()`, `listTaskHandlers()` |
| `shared_mutex`-Upgrade | `database_maintenance_orchestrator.h` Members |
| Multi-Mandanten-Isolation | `TenantMaintenanceConfig`, 15 Tests MT-01..MT-15 |
| IDistributedLock Interface | `include/maintenance/i_distributed_lock.h` |

---

*Erstellt von Copilot · Issue #4463 · 2026-04-15*

# Maintenance-Modul

<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/maintenance/README.md -->

**Stand:** 15. April 2026  
**Version:** v2.0.0  
**Kategorie:** Datenbankwartung  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Maintenance-Modul orchestriert geplante Datenbankwartungsaufgaben: Index-Rebuild, Statistik-Updates, Speicher-Kompaktierung, MVCC-Bereinigung, Replikat-Validierung und benutzerdefinierte Aufgaben-Handler. Seit v2.0.0 unterstützt es vollständige Mandantenisolation und verteilte Koordination.

**Primäre Quelle:** [`src/maintenance/`](../../../src/maintenance/) · [`include/maintenance/`](../../../include/maintenance/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| DatabaseMaintenanceOrchestrator | `database_maintenance_orchestrator.h` | `database_maintenance_orchestrator.cpp` | Haupt-Orchestrator für alle Wartungsaufgaben; v2.0.0 |
| TenantMaintenanceConfig | `database_maintenance_orchestrator.h` | *(im Orchestrator)* | Pro-Mandanten-Konfiguration (Wartungsfenster, Job-Quota) |
| IDistributedLock | `i_distributed_lock.h` | *(header-only)* | Verteilte Sperr-Schnittstelle; `InProcessDistributedLock` für Einzelknoten |
| MaintenanceScheduleStore | `maintenance_schedule_store.h` | `maintenance_schedule_store.cpp` | RocksDB-Persistenz für Wartungspläne |
| MaintenanceTaskHandlerImpls | `maintenance_task_handler_impls.h` | `maintenance_registry.cpp` | Implementierungen für Standard-Wartungsaufgaben |
| IMaintenanceTaskHandler | `i_maintenance_task_handler.h` | *(interface)* | Erweiterbare Task-Handler-Schnittstelle |
| MaintenanceTask | `maintenance_task.h` | *(header-only)* | Aufgaben-Datenstruktur; 19 Task-Typen |
| MaintenanceSchedule | `maintenance_schedule.h` | *(header-only)* | Cron-basierte Zeitplanung; DAG-Abhängigkeiten |
| MaintenanceHealthReport | `maintenance_health_report.h` | *(header-only)* | Wartungsgesundheitsbericht |

---

## Implementierte Features (v2.0.0)

| Feature | Version | Status |
|---------|---------|--------|
| Schedule CRUD (POST/GET/PUT/PATCH/DELETE) | v1.0.0 | ✅ |
| Cron-basierte Ausführung via `TaskScheduler` | v1.0.0 | ✅ |
| Wartungsfenster-Durchsetzung (UTC) | v1.0.0 | ✅ |
| Sequenzielle Task-Ausführung mit `halt_on_task_failure` | v1.0.0 | ✅ |
| 11 Prometheus-kompatible Metriken | v1.0.0 | ✅ |
| RBAC: `maintenance:read/write/admin` | v1.0.0 | ✅ |
| RocksDB-Persistenz (`MaintenanceScheduleStore`) | v1.1.0 | ✅ |
| Force-Run `{"force": true}` | v1.1.0 | ✅ |
| DAG-Abhängigkeitsgraph (Kahn's Topologiesortierung) | v1.2.0 | ✅ |
| `IMaintenanceTaskHandler`-Registry | v1.2.0 | ✅ |
| `STORAGE_COMPACTION` mit `CompactionManager` verdrahtet | v1.2.0 | ✅ |
| `MVCC_CLEANUP` mit `MvccStore` verdrahtet | v1.2.0 | ✅ |
| `shared_mutex` für Lese-Parallelität | v1.2.0 | ✅ |
| `IDistributedLock` (verteilte Koordination) | v2.0.0 | ✅ |
| Multi-Mandanten-Isolation (`TenantMaintenanceConfig`) | v2.0.0 | ✅ |

## Offene Punkte

| Punkt | Ziel |
|-------|------|
| `REPLICA_VALIDATION` an Sharding-Modul verdrahten | v2.1.0 |
| Raft-basierte `IDistributedLock`-Implementierung | v2.1.0 |

---

## REST-API-Endpunkte (15)

| Methode | Pfad | Beschreibung |
|---------|------|--------------|
| `POST` | `/api/v1/maintenance/schedules` | Plan erstellen |
| `GET` | `/api/v1/maintenance/schedules` | Alle Pläne auflisten (mit `?tenant_id=`-Filter) |
| `GET` | `/api/v1/maintenance/schedules/{id}` | Plan nach ID abrufen |
| `PUT` | `/api/v1/maintenance/schedules/{id}` | Plan ersetzen |
| `PATCH` | `/api/v1/maintenance/schedules/{id}` | Teilaktualisierung |
| `DELETE` | `/api/v1/maintenance/schedules/{id}` | Plan löschen |
| `POST` | `/api/v1/maintenance/schedules/{id}/enable` | Plan aktivieren |
| `POST` | `/api/v1/maintenance/schedules/{id}/disable` | Plan deaktivieren |
| `POST` | `/api/v1/maintenance/schedules/{id}/run` | Ad-hoc-Ausführung; optional `{"force": true}` |
| `GET` | `/api/v1/maintenance/jobs` | Letzte Jobs auflisten |
| `GET` | `/api/v1/maintenance/jobs/{id}` | Job-Details abrufen |
| `POST` | `/api/v1/maintenance/jobs/{id}/cancel` | Laufenden Job abbrechen |
| `GET` | `/api/v1/maintenance/health` | Aggregierter Gesundheitsbericht |
| `GET` | `/api/v1/maintenance/task-handlers` | Registrierte Task-Handler auflisten |
| `GET` | `/api/v1/maintenance/status` | Orchestrator-Status-Snapshot |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/maintenance/README.md`](../../../src/maintenance/README.md) | Modulübersicht |
| [`src/maintenance/ARCHITECTURE.md`](../../../src/maintenance/ARCHITECTURE.md) | Architektur-Leitfaden |
| [`src/maintenance/ROADMAP.md`](../../../src/maintenance/ROADMAP.md) | Implementierungs-Roadmap |
| [`src/maintenance/CHANGELOG.md`](../../../src/maintenance/CHANGELOG.md) | Änderungsprotokoll |
| [`src/maintenance/FUTURE_ENHANCEMENTS.md`](../../../src/maintenance/FUTURE_ENHANCEMENTS.md) | Geplante Verbesserungen |
| [`docs/maintenance/ORCHESTRATOR_DESIGN.md`](../../maintenance/ORCHESTRATOR_DESIGN.md) | Orchestrator-Designdokument |
| [`docs/maintenance/MODULE_INTEGRATION_GUIDE.md`](../../maintenance/MODULE_INTEGRATION_GUIDE.md) | Modul-Integrationsleitfaden |
| [`docs/de/maintenance/MISSING_IMPLEMENTATIONS.md`](./MISSING_IMPLEMENTATIONS.md) | Fehlende Implementierungen |

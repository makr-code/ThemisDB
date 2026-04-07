# Maintenance-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/maintenance/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Datenbankwartung  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Maintenance-Modul orchestriert geplante Datenbankwartungsaufgaben: Vakuumierung, Index-Rebuild, Statistik-Updates und benutzerdefinierte Aufgaben-Handler.

**Primäre Quelle:** [`src/maintenance/`](../../../src/maintenance/) · [`include/maintenance/`](../../../include/maintenance/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| DatabaseMaintenanceOrchestrator | `database_maintenance_orchestrator.h` | `database_maintenance_orchestrator.cpp` | Haupt-Orchestrator für alle Wartungsaufgaben |
| MaintenanceScheduleStore | `maintenance_schedule_store.h` | `maintenance_schedule_store.cpp` | Persistenz für Wartungspläne |
| MaintenanceTaskHandlerImpls | `maintenance_task_handler_impls.h` | `maintenance_registry.cpp` | Implementierungen für Standard-Wartungsaufgaben |
| IMaintenanceTaskHandler | `i_maintenance_task_handler.h` | *(interface)* | Erweiterbare Task-Handler-Schnittstelle |
| MaintenanceTask | `maintenance_task.h` | *(header-only)* | Aufgaben-Datenstruktur |
| MaintenanceSchedule | `maintenance_schedule.h` | *(header-only)* | Cron-basierte Zeitplanung |
| MaintenanceHealthReport | `maintenance_health_report.h` | *(header-only)* | Wartungsgesundheitsbericht |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/maintenance/README.md`](../../../src/maintenance/README.md) | Modulübersicht |

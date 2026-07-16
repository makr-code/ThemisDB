# Scheduler-Modul – Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/scheduler/ -->

Dieser Report dokumentiert Punkte, die in der Primärdokumentation des Scheduler-Moduls als
erledigt markiert sind oder als geplante Features gelistet werden, aber beim Abgleich mit dem
Sourcecode als **nicht vollständig umgesetzt** befunden wurden.

**Prüfstand:** 2026-03-10 | **Branch:** `develop`

> **Hinweis:** Dieser Report ist ein transparenter Befund-Report. Issue-Erstellung und
> Priorisierung obliegen dem Engineering-Team.

---

## Befund 1 – Priority-basiertes Scheduling ✅ Behoben

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Priority-Based Scheduling" + `include/scheduler/task_scheduler.h` `ScheduledTask::Priority` enum |
| **Claim** | Priority-basiertes Scheduling mit `Priority::HIGH`, `NORMAL`, `LOW` – Tasks mit höherer Priorität sollen bevorzugt ausgeführt werden |
| **Erwartet** | Dispatch-Loop sortiert/priorisiert fällige Tasks nach `priority`-Feld vor der Ausführung |
| **Beobachtet (behoben)** | `std::sort` nach `static_cast<int>(priority)` absteigend nach `adjustConcurrencyLimit()` und vor dem Dispatch-Loop eingebaut (`src/scheduler/task_scheduler.cpp`) |
| **Geprüfte Pfade** | `include/scheduler/task_scheduler.h:134-139`, `src/scheduler/task_scheduler.cpp` |
| **Status** | ✅ **Behoben** – `tasks_to_execute` wird jetzt vor der Ausführung nach Priorität (HIGH → NORMAL → LOW) sortiert |

---

## Befund 2 – Raft-basierte Distributed Coordination: nur Gossip implementiert

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Distributed Task Coordination with Raft" |
| **Claim** | Raft-basierte Leader Election und Task-State-Replikation als v1.7.0-Target |
| **Erwartet** | `DistributedTaskCoordinator` mit Raft-Konsensus-Integration, Raft-Log-basierter Task-State-Replikation, Clock-Synchronisation |
| **Beobachtet** | Gossip-basierte Leader Election via `DistributedCoordinator` (Sharding-Modul) implementiert; Raft-Integration für `DistributedTaskCoordinator` fehlt; `src/scheduler/FUTURE_ENHANCEMENTS.md` listet diese explizit als `[ ]` (offen) |
| **Geprüfte Pfade** | `src/scheduler/distributed_task_coordinator.cpp`, `include/scheduler/distributed_task_coordinator.h`, `src/scheduler/FUTURE_ENHANCEMENTS.md:102-113` |
| **ROADMAP-Status** | `[ ]` offen (FUTURE_ENHANCEMENTS §"Distributed Task Coordination with Raft") |
| **Issue-Titelvorschlag** | `feat(scheduler): integrate Raft-based consensus for DistributedTaskCoordinator` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `scheduler`, `distributed`, `status:open` |

---

## Befund 3 – Task Checkpointing und Resume: nicht implementiert

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Task Checkpointing and Resume" |
| **Claim** | Checkpoint-Persistenz-API, automatisches Resume nach Fehler, Fortschritts-Tracking |
| **Erwartet** | `ITaskCheckpoint`-Schnittstelle oder äquivalente API für inkrementelle Verarbeitung und Auto-Resume |
| **Beobachtet** | Keine Checkpoint-bezogenen Symbole in `task_scheduler.h` oder `task_scheduler.cpp`; kein `ITaskCheckpoint`-Header gefunden; Retry-Mechanismus vorhanden, aber kein Checkpoint-basiertes Resume |
| **Geprüfte Pfade** | `include/scheduler/task_scheduler.h` (vollständig), `src/scheduler/task_scheduler.cpp` (vollständig) |
| **ROADMAP-Status** | `[ ]` offen (FUTURE_ENHANCEMENTS §"Task Checkpointing and Resume") |
| **Issue-Titelvorschlag** | `feat(scheduler): implement task checkpoint persistence and auto-resume API` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `scheduler`, `status:open` |

---

## Befund 4 – Grafana-Dashboard ✅ Behoben

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Observability Enhancements" |
| **Claim** | Grafana-Dashboard für Scheduler-Metriken (Concurrency-Limit, Queue-Depth, Task-Erfolge/Fehler) |
| **Erwartet** | JSON-Dashboard-Definition in `config/grafana/dashboards/` oder `deploy/kubernetes/monitoring/grafana-dashboards/` |
| **Beobachtet (behoben)** | `config/grafana/dashboards/themisdb-scheduler-dashboard.json` erstellt; enthält 5 stat-Panels (Registered/Active/Running/ConcurrencyLimit/QueueDepth), 2 Zeitreihen-Panels (Concurrency & Queue Depth / Task Counts), 2 Raten-Panels (Success vs Failure / Failure Rate %), 4 Per-Task-Panels (Avg Duration / Failure Rate / Top-10-Slowest / Enabled Status), 1 Last-Run-Tabelle; `$task_name`-Variable für Filterung |
| **Geprüfte Pfade** | `config/grafana/dashboards/themisdb-scheduler-dashboard.json` |
| **Status** | ✅ **Behoben** – Dashboard mit allen `themis_scheduler_*` Metriken implementiert |

---

## Befund 5 – WebSocket Event Streaming: nicht implementiert

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Observability Enhancements"; `include/scheduler/README.md` §"Future Enhancements" (WebSocket subscriptions für Real-time Task Status) |
| **Claim** | WebSocket-basiertes Real-Time-Event-Streaming für Task-Status-Updates |
| **Erwartet** | WebSocket-Endpunkt (z.B. `WS /api/v1/scheduler/events`) für Echtzeit-Benachrichtigungen über Task-Starts, -Abschlüsse und -Fehler |
| **Beobachtet** | Nur HTTP-Polling-basierte API in `TaskSchedulerApiHandler`; kein WebSocket-Handler in `src/server/task_scheduler_api_handler.cpp` |
| **Geprüfte Pfade** | `src/server/task_scheduler_api_handler.cpp`, `include/server/task_scheduler_api_handler.h` |
| **ROADMAP-Status** | `[ ]` offen (FUTURE_ENHANCEMENTS §"Observability Enhancements") |
| **Issue-Titelvorschlag** | `feat(scheduler): WebSocket endpoint for real-time task status event streaming` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `scheduler`, `observability`, `status:open` |

---

## Befund 6 – Multi-Tenancy: nicht implementiert

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Multi-Tenancy Support" |
| **Claim** | Per-Tenant-Quota-Tracking, Ressourcenisolierung, Task-Namespace-Trennung |
| **Erwartet** | `tenant_id`-Feld in `ScheduledTask`, Quota-Manager, Namespace-Isolierung im Dispatch-Loop |
| **Beobachtet** | Kein `tenant_id`-Feld in `ScheduledTask`; kein Quota-Tracking-Code; Tasks werden gemeinsam in einem globalen Namespace verwaltet |
| **Geprüfte Pfade** | `include/scheduler/task_scheduler.h` (ScheduledTask-Definition vollständig), `src/scheduler/task_scheduler.cpp` |
| **ROADMAP-Status** | `[ ]` offen (FUTURE_ENHANCEMENTS §"Multi-Tenancy Support") |
| **Issue-Titelvorschlag** | `feat(scheduler): multi-tenancy support with per-tenant quotas and namespace isolation` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `scheduler`, `multi-tenancy`, `status:open` |

---

## Befund 7 – Dynamische Ressourcenzuteilung (cgroups): nicht implementiert

| Feld | Inhalt |
|------|--------|
| **Claim-Quelle** | `src/scheduler/FUTURE_ENHANCEMENTS.md` §"Dynamic Resource Allocation" |
| **Claim** | cgroups-Integration für CPU/Memory/IO-Quota-Enforcement pro Task |
| **Erwartet** | cgroups-v2-basierte Ressourcenlimitierung bei Task-Ausführung |
| **Beobachtet** | `ScheduledTask::timeout` und `max_retries` sind implementiert; cgroups-Integration fehlt vollständig; kein `cgroup_*`-Symbol im Scheduler-Code |
| **Geprüfte Pfade** | `include/scheduler/task_scheduler.h`, `src/scheduler/task_scheduler.cpp` |
| **ROADMAP-Status** | `[ ]` offen (FUTURE_ENHANCEMENTS §"Dynamic Resource Allocation") |
| **Issue-Titelvorschlag** | `feat(scheduler): cgroups-v2 resource limits for task execution (CPU, memory, I/O)` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `scheduler`, `infrastructure`, `status:open` |

# DatabaseMaintenanceOrchestrator – Architecture & Design

## Overview

The `DatabaseMaintenanceOrchestrator` is the **central coordinator** for all
recurring and on-demand database maintenance in ThemisDB.  It preserves the
existing modular architecture by acting purely as an *orchestrator*: it
decides **when** and **what** to run, but always delegates the actual work to
the owning module.

```
┌────────────────────────────────────────────────────────────────────┐
│                  DatabaseMaintenanceOrchestrator                   │
│                                                                    │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  Schedule Registry (CRUD)  ← operator API                   │  │
│  │  std::map<id, MaintenanceScheduleEntry>                      │  │
│  └────────────────────┬────────────────────────────────────────┘  │
│                       │ registers cron tasks                       │
│                       ▼                                            │
│              ┌──────────────────┐                                  │
│              │  TaskScheduler   │  ← existing cron scheduler       │
│              └────────┬─────────┘                                  │
│                       │ fires function callback                    │
│                       ▼                                            │
│              ┌──────────────────────────┐                          │
│              │  executeSchedule()       │                          │
│              │  for each task_type:     │                          │
│              │    executeTask() ───────►│IndexMaintenanceManager   │
│              │                  ───────►│(future) StorageModule    │
│              │                  ───────►│(future) ReplicaModule    │
│              └──────────────────────────┘                          │
│                                                                    │
│  ┌──────────────────────────────────────┐                          │
│  │  Job Tracker                         │                          │
│  │  std::map<id, OrchestratorJob>       │  ← 24 h retention        │
│  └──────────────────────────────────────┘                          │
│                                                                    │
│  ┌──────────────────────────────────────┐                          │
│  │  Health Probe Registry               │                          │
│  │  std::map<name, HealthProbe>         │  ← modules register here │
│  └──────────────────────────────────────┘                          │
└────────────────────────────────────────────────────────────────────┘
```

## Design Principles

### 1. Modularity preserved
Each module keeps its own maintenance logic (`IndexMaintenanceManager`, etc.).
The orchestrator only calls standardised entry points
(`triggerMaintenanceCheck()`, `listActiveJobs()`, …) and never duplicates
module internals.

### 2. Full schedule CRUD
Operators manage maintenance schedules as first-class entities via a REST API.
Every schedule is a `MaintenanceScheduleEntry` stored in memory, supporting:
- **Create** (POST) – validates, assigns UUID and timestamps, derives cron
- **Read one / list** (GET)
- **Full replace** (PUT) – preserves immutable fields, updates TaskScheduler
- **Partial update** (PATCH) – only provided JSON fields are modified
- **Delete** (DELETE) – deregisters from TaskScheduler, removes entry

### 3. Cron-based scheduling via TaskScheduler
The existing `TaskScheduler` (cron / interval / CDC / webhook triggers) is
reused for time-based firing.  The orchestrator registers one `ScheduledTask`
per enabled `MaintenanceScheduleEntry` and deregisters it on disable/delete.

### 4. DAG task ordering within a schedule
Each `MaintenanceScheduleEntry.tasks` list defines the execution order.
If `halt_on_task_failure = true`, subsequent tasks are skipped when any task
fails.  Cancellation is supported at any point.

### 5. Health aggregation
Any module calls `registerHealthProbe(name, probe)` once at start-up.
`getHealthReport()` invokes all probes synchronously (< 10 ms each) and
derives an `overall_status` as the worst of all signals.

## HTTP API

### Schedule CRUD

| Method | Path | Description |
|--------|------|-------------|
| POST   | `/api/v1/maintenance/schedules` | Create schedule |
| GET    | `/api/v1/maintenance/schedules` | List all schedules |
| GET    | `/api/v1/maintenance/schedules/{id}` | Get schedule |
| PUT    | `/api/v1/maintenance/schedules/{id}` | Full replace |
| PATCH  | `/api/v1/maintenance/schedules/{id}` | Partial update |
| DELETE | `/api/v1/maintenance/schedules/{id}` | Delete schedule |
| POST   | `/api/v1/maintenance/schedules/{id}/run` | Trigger immediately |

### Jobs

| Method | Path | Description |
|--------|------|-------------|
| GET    | `/api/v1/maintenance/jobs` | List jobs (`?active_only=true`) |
| GET    | `/api/v1/maintenance/jobs/{id}` | Get job |
| POST   | `/api/v1/maintenance/jobs/{id}/cancel` | Cancel job |

### Observability

| Method | Path | Description |
|--------|------|-------------|
| GET    | `/api/v1/maintenance/status` | Orchestrator status |
| GET    | `/api/v1/maintenance/health` | Aggregated health report |

## Authentication

All endpoints require authentication via the existing `requireAccess()` RBAC
mechanism:

| Action | Required permission |
|--------|---------------------|
| Read status/health/list | `maintenance:read` |
| Create/update/delete/patch schedule | `maintenance:write` |
| Cancel job | `maintenance:write` |
| Trigger immediate run | `maintenance:admin` |

## Key Data Structures

### `MaintenanceScheduleEntry`
```json
{
  "id":               "uuid",
  "name":             "Daily Maintenance",
  "description":      "...",
  "frequency":        "daily",
  "cron_expression":  "0 2 * * *",
  "tasks":            ["metrics_collection", "fragmentation_monitoring"],
  "enabled":          true,
  "enforce_window":   true,
  "window_start_hour": 2,
  "window_end_hour":  6,
  "halt_on_task_failure": false,
  "created_at_ms":    1712345678000,
  "updated_at_ms":    1712345678000,
  "last_run_ms":      0,
  "next_run_ms":      0,
  "last_run_state":   ""
}
```

### `OrchestratorJob`
```json
{
  "id":             "uuid",
  "schedule_id":    "uuid",
  "task_type":      "consistency_check",
  "state":          "succeeded",
  "error_message":  "",
  "result_summary": "check completed",
  "progress_pct":   100.0,
  "started_at_ms":  1712345678000,
  "finished_at_ms": 1712345700000,
  "duration_ms":    22000
}
```

## Thread Safety

All public methods acquire internal mutexes.  Three separate mutexes guard:
1. `schedules_` map
2. `jobs_` map
3. `health_probes_` map

Jobs run in detached `std::thread`s.  Cancellation is cooperative
(job thread checks state flag).

## Extensibility

To add a new module's maintenance operation:
1. Add a value to `MaintenanceTaskType` enum in `maintenance_task.h`.
2. Add the string conversion in `taskTypeToString` / `taskTypeFromString`.
3. Add a `case` in `DatabaseMaintenanceOrchestrator::executeTask()`.
4. Optionally, call `registerHealthProbe()` from the module's start-up code.

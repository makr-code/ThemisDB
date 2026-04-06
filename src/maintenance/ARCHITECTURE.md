<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md -->

# Maintenance Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Module Path:** `src/maintenance/`

---

## 1. Overview

The Maintenance module provides a centralized orchestration layer for all database
maintenance operations in ThemisDB. It exposes a REST API for schedule management,
integrates with the `TaskScheduler` for cron-based execution, enforces maintenance
windows, and aggregates per-module health signals.

---

## 2. Design Principles

- **Single Orchestrator** — `DatabaseMaintenanceOrchestrator` is the sole entry point
  for all maintenance scheduling; individual modules register health probes rather than
  managing their own schedules.
- **Non-Invasive** — module-delegated tasks succeed immediately if the module has not
  registered a handler; no module is required to integrate.
- **Safety by Default** — `halt_on_task_failure` gate stops a task list at first failure;
  maintenance windows prevent off-hours disruption; 24-hour job TTL prevents accumulation.
- **Observable** — 11 Prometheus-compatible metrics; all state changes audit-logged.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|------|------|
| `database_maintenance_orchestrator.cpp` | Core orchestrator: schedule CRUD, job execution, health aggregation |
| `maintenance_registry.cpp` | Default schedule bundles (daily/weekly/monthly/quarterly) |
| `include/maintenance/maintenance_task.h` | `MaintenanceTaskType` (19 types), `OrchestratorJob`, `MaintenanceJobState` |
| `include/maintenance/maintenance_schedule.h` | `MaintenanceScheduleEntry` with JSON serialization |
| `include/maintenance/maintenance_health_report.h` | `MaintenanceHealthReport`, `ModuleHealthSignal` |
| `include/maintenance/database_maintenance_orchestrator.h` | Public API |

### 3.2 Maintenance Task Types (19)

```
INDEX_REBUILD        INDEX_OPTIMIZE       INDEX_CONSISTENCY_CHECK
STORAGE_COMPACTION   WAL_ARCHIVING        BACKUP_VERIFICATION
METRICS_COLLECTION   LOG_ROTATION         CACHE_WARM
DEAD_LETTER_DRAIN    REPLICA_VALIDATION   MVCC_CLEANUP
SCHEMA_VALIDATION    RETENTION_ENFORCEMENT STATISTICS_UPDATE
SECURITY_SCAN        AUDIT_LOG_FLUSH      BLOOM_FILTER_REBUILD
CUSTOM
```

### 3.3 Job Lifecycle

```
PENDING → RUNNING → SUCCEEDED
                 → FAILED
                 → CANCELLED (via cancelJob())
                 → SKIPPED   (outside maintenance window)
```

---

## 4. Threading Model

Three independent mutexes prevent contention:

| Mutex | Guards |
|-------|--------|
| `schedules_mutex_` | `schedules_` map (CRUD operations) |
| `jobs_mutex_` | `jobs_` map (job lifecycle and TTL pruning) |
| `health_probes_mutex_` | `health_probes_` map (probe registration and invocation) |

---

## 5. REST API Endpoints (11)

| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/api/v1/maintenance/schedules` | Create schedule |
| `GET` | `/api/v1/maintenance/schedules` | List all schedules |
| `GET` | `/api/v1/maintenance/schedules/{id}` | Get schedule by ID |
| `PUT` | `/api/v1/maintenance/schedules/{id}` | Replace schedule |
| `PATCH` | `/api/v1/maintenance/schedules/{id}` | Partial update |
| `DELETE` | `/api/v1/maintenance/schedules/{id}` | Delete schedule |
| `POST` | `/api/v1/maintenance/schedules/{id}/enable` | Enable schedule |
| `POST` | `/api/v1/maintenance/schedules/{id}/disable` | Disable schedule |
| `GET` | `/api/v1/maintenance/jobs` | List recent jobs |
| `GET` | `/api/v1/maintenance/jobs/{id}` | Get job details |
| `GET` | `/api/v1/maintenance/health` | Aggregated health report |

RBAC scopes: `maintenance:read` · `maintenance:write` · `maintenance:admin`

---

## 6. Observability

11 Prometheus-compatible metrics exported via `MetricsCollector`:

- `maintenance_schedules_total` — active schedule count
- `maintenance_jobs_executed_total` — cumulative job executions
- `maintenance_jobs_succeeded_total` / `_failed_total` / `_skipped_total`
- `maintenance_job_duration_ms` — job execution latency histogram
- `maintenance_window_enforcements_total` — times a job was skipped by window
- `maintenance_circuit_open_total` — halt-on-failure activations
- `maintenance_health_probes_total` — health probe invocations
- `maintenance_health_degraded_modules` — current degraded module count
- `maintenance_task_types_executed` — per-task-type execution counter

---

## 7. Integration Points

| Module | Integration |
|--------|------------|
| `scheduler` | `TaskScheduler::registerFunction()` for cron-based execution |
| `storage` | `IndexMaintenanceManager` for `INDEX_*` tasks |
| `utils` | `AuditLogger::logEvent()` for all CRUD and job events |
| `observability` | `MetricsCollector` for 11 counters and histograms |
| `server` | `MaintenanceApiHandler` wires 16 HTTP route cases |

---

## 8. Known Limitations

- Schedules are in-memory only (lost on restart); RocksDB persistence planned v1.1.0
- Task list implies total order; explicit DAG dependency graph planned v1.2.0
- Module-delegated tasks not yet wired to real implementations (see `MODULE_INTEGRATION_GUIDE.md`)

<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ROADMAP.md -->

# Maintenance Module — Architecture Guide

**Version:** 2.0.0  
**Last Updated:** 2026-04-15  
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
| `maintenance_schedule_store.cpp` | RocksDB-backed schedule persistence |
| `include/maintenance/maintenance_task.h` | `MaintenanceTaskType` (19 types), `OrchestratorJob`, `MaintenanceJobState` |
| `include/maintenance/maintenance_schedule.h` | `MaintenanceScheduleEntry` with JSON serialization and `MaintenanceTaskDependency` |
| `include/maintenance/maintenance_schedule_store.h` | `MaintenanceScheduleStore` CRUD interface |
| `include/maintenance/maintenance_health_report.h` | `MaintenanceHealthReport`, `ModuleHealthSignal` |
| `include/maintenance/database_maintenance_orchestrator.h` | Public API; `TenantMaintenanceConfig` |
| `include/maintenance/i_maintenance_task_handler.h` | `IMaintenanceTaskHandler` base interface |
| `include/maintenance/maintenance_task_handler_impls.h` | Built-in handlers: `StorageCompactionHandler`, `ReplicaValidationHandler`, `MvccCleanupHandler`, `FunctionMaintenanceTaskHandler` |
| `include/maintenance/i_distributed_lock.h` | `IDistributedLock` interface; `InProcessDistributedLock` in-process implementation |

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

Five independent mutexes / shared mutexes prevent contention:

| Mutex | Type | Guards |
|-------|------|--------|
| `schedules_mutex_` | `std::shared_mutex` | `schedules_` map (reads use `shared_lock`, writes use `unique_lock`) |
| `jobs_mutex_` | `std::shared_mutex` | `jobs_` map (reads use `shared_lock`, writes use `unique_lock`) |
| `probes_mutex_` | `std::mutex` | `health_probes_` map (exclusive, cheap probe registry) |
| `handlers_mutex_` | `std::shared_mutex` | `task_handlers_` map (reads use `shared_lock`) |
| `tenant_configs_mutex_` | `std::shared_mutex` | `tenant_configs_` map (reads use `shared_lock`) |
| `dist_lock_mutex_` | `std::mutex` | `dist_lock_` shared pointer swap |

---

## 5. REST API Endpoints (15)

| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/api/v1/maintenance/schedules` | Create schedule |
| `GET` | `/api/v1/maintenance/schedules` | List all schedules (accepts `?tenant_id=` filter) |
| `GET` | `/api/v1/maintenance/schedules/{id}` | Get schedule by ID |
| `PUT` | `/api/v1/maintenance/schedules/{id}` | Replace schedule |
| `PATCH` | `/api/v1/maintenance/schedules/{id}` | Partial update |
| `DELETE` | `/api/v1/maintenance/schedules/{id}` | Delete schedule |
| `POST` | `/api/v1/maintenance/schedules/{id}/enable` | Enable schedule |
| `POST` | `/api/v1/maintenance/schedules/{id}/disable` | Disable schedule |
| `POST` | `/api/v1/maintenance/schedules/{id}/run` | Ad-hoc trigger; optional `{"force": true}` |
| `GET` | `/api/v1/maintenance/jobs` | List recent jobs |
| `GET` | `/api/v1/maintenance/jobs/{id}` | Get job details |
| `POST` | `/api/v1/maintenance/jobs/{id}/cancel` | Cancel running job |
| `GET` | `/api/v1/maintenance/health` | Aggregated health report |
| `GET` | `/api/v1/maintenance/task-handlers` | List registered task handlers |
| `GET` | `/api/v1/maintenance/status` | Orchestrator status snapshot |

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
| `storage` | `IndexMaintenanceManager` for `INDEX_*` tasks; `CompactionManager` via `StorageCompactionHandler`; `MvccStore` via `MvccCleanupHandler` |
| `utils` | `AuditLogger::logEvent()` for all CRUD and job events |
| `observability` | `MetricsCollector` for 11 counters and histograms |
| `server` | `MaintenanceApiHandler` wires 15 HTTP routes |
| `replication` | `IDistributedLock` injection point (production: Raft-backed implementation) |

---

## 8. Known Limitations

- `REPLICA_VALIDATION` wiring to sharding/replica module pending (handler class `ReplicaValidationHandler` exists; startup call site not yet added)
- Raft-backed `IDistributedLock` not yet implemented; use `InProcessDistributedLock` for single-node or test deployments

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Maintenance Module - Future Enhancements

## Scope

Centralized database maintenance orchestration (`database_maintenance_orchestrator.cpp`) and default schedule bundles (`maintenance_registry.cpp`). The orchestrator provides schedule CRUD, cron-based dispatch via `TaskScheduler`, sequential DAG execution of 19 task types, maintenance window enforcement, audit logging, observability metrics, per-module health probe registry, and the `MaintenanceApiHandler` (11 HTTP REST endpoints). Enhancements focus on persistence, explicit DAG dependencies, module task wiring, and distributed coordination.

---

## Design Constraints

- `[x]` Schedules must survive server restarts — in-memory-only schedules are explicitly documented as a known limitation in `ROADMAP.md`.
- `[ ]` `schedules_mutex_` is held exclusively for all read operations (`listSchedules`, `getSchedule`) — upgrade to `shared_mutex` to reduce read contention.
- `[ ]` `halt_on_task_failure` semantics must be preserved: a failed task stops execution of subsequent tasks in the same run; parallel task execution must not be introduced without preserving this contract.
- `[ ]` All admin operations (`DELETE`, `PATCH`, `POST/run`) must be atomic with respect to the running cron job; no partial state must be visible to concurrent readers.
- `[ ]` Module-delegated tasks (`STORAGE_COMPACTION`, `REPLICA_VALIDATION`, `MVCC_CLEANUP`, `METRICS_COLLECTION`) must dispatch through a registered `IMaintenanceTaskHandler` interface — direct module coupling in `executeTask()` is forbidden.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `DatabaseMaintenanceOrchestrator::registerTaskHandler(type, handler)` | Storage, sharding, replication modules | Registers a real implementation for a delegated task type |
| `IMaintenanceTaskHandler::execute(schedule_id, task_type, params) → Result` | `executeTask()` in orchestrator | Called when the cron job fires for this task type |
| `MaintenanceScheduleStore::save(entry)` / `load(id)` / `loadAll()` | `DatabaseMaintenanceOrchestrator` | RocksDB-backed persistence; replaces in-memory `schedules_` map |
| `MaintenanceApiHandler` REST API | HTTP server, admin CLI | 11 endpoints; RBAC scopes `maintenance:read/write/admin` |
| `DistributedLock::tryAcquire(key, ttl)` | Orchestrator cron dispatch path | Prevents two nodes running the same schedule simultaneously |

---

## Planned Features

### Schedule Persistence (RocksDB)
**Priority:** High
**Target Version:** v1.1.0

Schedules are currently in-memory (`std::unordered_map<std::string, MaintenanceScheduleEntry> schedules_`). They are lost on every server restart. Operators must re-create all schedules after each deployment.

**Implementation Notes:**
- `[x]` Add a `MaintenanceScheduleStore` class wrapping the existing `StorageEngine` API; key format: `maint_sched::{id}` (UTF-8 JSON value).
- `[x]` In `DatabaseMaintenanceOrchestrator::start()`, call `MaintenanceScheduleStore::loadAll()` and populate `schedules_` before registering cron jobs.
- `[x]` In `createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule` — persist the change to RocksDB inside the `schedules_mutex_` critical section (write-through).
- `[x]` Corrupt schedule JSON on load: log `WARN` and skip that entry; all valid entries must be loaded.
- `[x]` Add a restart-persistence integration test: create 3 schedules, restart the orchestrator, verify all 3 are present.

**Performance Targets:**
- `loadAll()` at startup: ≤ 100 ms for 10 000 stored schedules.

---

### Force-Run Endpoint: Window Override
**Priority:** High
**Target Version:** v1.1.0

There is no way to trigger a schedule outside its maintenance window without editing the window configuration. Operators need an emergency override for urgent maintenance.

**Implementation Notes:**
- `[x]` Add `POST /api/v1/maintenance/schedules/{id}/run` with optional body `{"force": true}`.
- `[x]` When `force: true`, bypass the UTC window check in `executeSchedule()`; set `forced: true` in the audit log entry.
- `[x]` Require `maintenance:admin` scope for the force flag; `maintenance:write` allows manual trigger within the window only.
- `[x]` Unit test: schedule with a window that excludes the current hour; force-run triggers execution; regular run is skipped.

---

### Explicit Per-Task DAG with `depends_on`
**Priority:** Medium
**Target Version:** v1.2.0

Task execution order is currently determined by list order in `MaintenanceScheduleEntry::tasks`. There are no explicit dependency declarations, making it impossible to express "run WAL rotation before compaction" without relying on position.

**Implementation Notes:**
- `[ ]` Add `MaintenanceTaskDependency` struct: `{ task_type: MaintenanceTaskType, depends_on: vector<MaintenanceTaskType> }`.
- `[ ]` Add `MaintenanceScheduleEntry::task_dependencies` field (optional; defaults to sequential list order).
- `[ ]` Implement topological sort of the dependency graph using `ModuleDependencyResolver` (already implemented in `src/base/module_loader.cpp`).
- `[ ]` Cycle detection: reject schedule creation / update with a cycle; return `ERR_UTIL_INVALID_ARGUMENT`.
- `[ ]` Tests: DAG ordering correctness, cycle rejection, cascading failure with `halt_on_task_failure`.

**Performance Targets:**
- Topological sort: O(V+E); V=19 max task types — negligible overhead.

---

### Module Task Wiring: `IMaintenanceTaskHandler` Registry
**Priority:** Medium
**Target Version:** v1.2.0

`executeTask()` in `database_maintenance_orchestrator.cpp` succeeds immediately for all delegated task types (`STORAGE_COMPACTION`, `REPLICA_VALIDATION`, `MVCC_CLEANUP`, etc.) without calling any real module code. This is documented in `ROADMAP.md` as a known limitation.

**Implementation Notes:**
- `[x]` Add `registerTaskHandler(MaintenanceTaskType, std::shared_ptr<IMaintenanceTaskHandler>)` to the orchestrator public API.
- `[~]` `StorageModule` registers a handler for `STORAGE_COMPACTION` that calls `CompactionManager::triggerCompaction()`. (`StorageCompactionHandler` impl provided in `maintenance_task_handler_impls.h`; startup wiring call site pending.)
- `[~]` `ShardingModule` registers a handler for `REPLICA_VALIDATION` that calls the consistency checker. (`ReplicaValidationHandler` impl provided; startup wiring call site pending.)
- `[~]` `StorageEngine` registers a handler for `MVCC_CLEANUP` that triggers MVCC tombstone GC. (`MvccCleanupHandler` impl provided; startup wiring call site pending.)
- `[x]` For unregistered task types, `executeTask()` returns a `SKIPPED` result with a structured log message indicating no handler is registered.
- `[x]` Add a `GET /api/v1/maintenance/task-handlers` endpoint listing registered handlers per task type (useful for diagnosing unregistered tasks).

---

### `schedules_mutex_` Read-Path Upgrade
**Priority:** Medium
**Target Version:** v1.2.0

`database_maintenance_orchestrator.cpp` uses `std::lock_guard<std::mutex>` (exclusive) for all read operations (`listSchedules`, `getSchedule`, `listJobs`, `getJob`). Under concurrent admin API load, all readers serialize unnecessarily.

**Implementation Notes:**
- `[ ]` Replace `std::mutex schedules_mutex_` and `std::mutex jobs_mutex_` with `std::shared_mutex`; upgrade `listSchedules`, `getSchedule`, `listJobs`, `getJob` to `std::shared_lock`.
- `[ ]` Keep all write operations (`createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule`, `pruneOldJobs`) on `std::unique_lock`.
- `[ ]` Add a TSAN-enabled test with 8 concurrent `listSchedules` threads + 1 `createSchedule` thread.

---

### Distributed Maintenance Coordination via Raft
**Priority:** Low
**Target Version:** v2.0.0

In a multi-node cluster, each node independently schedules and fires maintenance jobs. Two nodes may run the same schedule concurrently, causing compaction storms or double maintenance.

**Implementation Notes:**
- `[x]` Integrate with the existing Raft-based distributed lock (`src/replication/raft_v2.cpp` or a dedicated distributed lock service) to elect a single maintenance leader per schedule.
- `[x]` Before firing a scheduled job, the orchestrator calls `DistributedLock::tryAcquire(schedule_id, ttl=window_duration_ms)`; only the node that acquires the lock runs the job.
- `[x]` Non-leader nodes log "schedule {id} skipped — lock held by peer {node_id}" at DEBUG level.
- `[x]` Lock TTL must be ≥ estimated task duration + 30 s safety margin; configurable per schedule.

---

### Multi-Tenant Schedule Isolation
**Priority:** Low
**Target Version:** v2.0.0

All schedules currently share a single global namespace and window. In a SaaS deployment, different tenants need independent maintenance windows and quotas.

**Implementation Notes:**
- `[x]` Add `MaintenanceScheduleEntry::tenant_id` (optional; empty = global/system schedule).
- `[x]` Per-tenant window enforcement: tenant's schedule fires only when the current hour is within that tenant's configured maintenance window, loaded from the tenant config.
- `[x]` Per-tenant quota: max N concurrent running maintenance jobs per tenant; enforced in `executeSchedule()`.
- `[x]` Admin API: `GET /api/v1/maintenance/schedules?tenant_id={id}` filters by tenant.

**Implementation Details:**
- `TenantMaintenanceConfig` struct added to `database_maintenance_orchestrator.h`: `enforce_window`, `window_start_hour`, `window_end_hour`, `max_concurrent_jobs`.
- `DatabaseMaintenanceOrchestrator::setTenantMaintenanceConfig(tenant_id, config)` / `getTenantMaintenanceConfig(tenant_id)` — thread-safe via `tenant_configs_mutex_`.
- `listSchedules(tenant_id_filter = "")` — empty filter returns all, non-empty returns only matching tenant.
- `MaintenanceApiHandler::listSchedules(tenant_id = "")` — API handler passes filter to orchestrator.
- `OrchestratorJob::tenant_id` populated from parent schedule in `triggerNow()` and `registerWithScheduler()`.
- 15 unit tests (MT-01..MT-15) in `test_database_maintenance_orchestrator.cpp`.

---

## Test Strategy

- **Unit tests** (already ≥40): extend with persistence round-trip, force-run, DAG cycle detection, unregistered handler `SKIPPED` result.
- **Integration tests**: restart-persistence (RocksDB); concurrent admin API stress (TSAN); distributed lock with mock Raft.
- **Performance benchmarks**: `loadAll()` with 10 K schedules; `listSchedules()` under 8 concurrent readers.

## Performance Targets

- `loadAll()` at startup with 10 K schedules: ≤ 100 ms.
- `listSchedules()` read path under 8 concurrent admin API requests: ≤ 2 ms p99.
- Topological sort of 19-node task DAG: ≤ 1 µs.

## Security / Reliability

- All schedule mutations are audit-logged via `AuditLogger::logEvent()` with caller identity and HLC timestamp.
- Force-run requires `maintenance:admin` JWT scope.
- Distributed lock prevents concurrent execution of the same schedule across cluster nodes.
- `halt_on_task_failure` ensures a single failed task stops cascading damage.

---

*Last Updated: 2026-03-12*
*Module Version: v1.0.0*

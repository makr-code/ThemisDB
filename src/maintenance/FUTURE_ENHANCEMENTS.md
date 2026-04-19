> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Maintenance Module - Future Enhancements

## Scope

Centralized database maintenance orchestration (`database_maintenance_orchestrator.cpp`) and default schedule bundles (`maintenance_registry.cpp`). The orchestrator provides schedule CRUD, cron-based dispatch via `TaskScheduler`, sequential DAG execution of 19 task types, maintenance window enforcement, audit logging, observability metrics, per-module health probe registry, and the `MaintenanceApiHandler` (11 HTTP REST endpoints). Enhancements focus on persistence, explicit DAG dependencies, module task wiring, and distributed coordination.

---

## Design Constraints

- `[x]` Schedules must survive server restarts — implemented via `MaintenanceScheduleStore` (RocksDB, v1.1.0).
- `[x]` `schedules_mutex_` is held exclusively for all read operations (`listSchedules`, `getSchedule`) — upgraded to `shared_mutex` in v1.2.0; read operations now use `std::shared_lock`.
- `[ ]` `halt_on_task_failure` semantics must be preserved: a failed task stops execution of subsequent tasks in the same run; parallel task execution must not be introduced without preserving this contract.
- `[ ]` All admin operations (`DELETE`, `PATCH`, `POST/run`) must be atomic with respect to the running cron job; no partial state must be visible to concurrent readers.
- `[x]` Module-delegated tasks (`STORAGE_COMPACTION`, `REPLICA_VALIDATION`, `MVCC_CLEANUP`, `METRICS_COLLECTION`) must dispatch through a registered `IMaintenanceTaskHandler` interface — direct module coupling in `executeTask()` is forbidden. Implemented in v1.2.0.

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
**Target Version:** v1.2.0 ✅ *Implemented*

Task execution order is currently determined by list order in `MaintenanceScheduleEntry::tasks`. There are no explicit dependency declarations, making it impossible to express "run WAL rotation before compaction" without relying on position.

**Implementation Notes:**
- `[x]` Add `MaintenanceTaskDependency` struct: `{ task_type: MaintenanceTaskType, depends_on: vector<MaintenanceTaskType> }`.
- `[x]` Add `MaintenanceScheduleEntry::task_dependencies` field (optional; defaults to sequential list order).
- `[x]` Implement topological sort of the dependency graph using Kahn's algorithm in `DatabaseMaintenanceOrchestrator::resolveTaskExecutionOrder()`.
- `[x]` Cycle detection: reject schedule creation / update with a cycle; return `ERR_UTIL_INVALID_ARGUMENT`.
- `[x]` Tests: DAG ordering correctness, cycle rejection, cascading failure with `halt_on_task_failure`.

**Performance Targets:**
- Topological sort: O(V+E); V=19 max task types — negligible overhead.

---

### Module Task Wiring: `IMaintenanceTaskHandler` Registry
**Priority:** Medium
**Target Version:** v1.2.0 ✅ *Implemented*

`executeTask()` in `database_maintenance_orchestrator.cpp` succeeds immediately for all delegated task types (`STORAGE_COMPACTION`, `REPLICA_VALIDATION`, `MVCC_CLEANUP`, etc.) without calling any real module code. This is documented in `ROADMAP.md` as a known limitation.

**Implementation Notes:**
- `[x]` Add `registerTaskHandler(MaintenanceTaskType, std::shared_ptr<IMaintenanceTaskHandler>)` to the orchestrator public API.
- `[x]` `StorageModule` registers a handler for `STORAGE_COMPACTION` that calls `CompactionManager::compactAll()`. (`StorageCompactionHandler` impl in `maintenance_task_handler_impls.h`; wired in `http_server.cpp`, Issue #4587.)
- `[~]` `ShardingModule` registers a handler for `REPLICA_VALIDATION` that calls the consistency checker. (`ReplicaValidationHandler` impl provided; startup wiring call site pending — Issue: REPLICA_VALIDATION wiring.)
- `[x]` `StorageEngine` registers a handler for `MVCC_CLEANUP` that triggers MVCC tombstone GC. (`MvccCleanupHandler` impl provided; wired in `http_server.cpp`, Issue #4586.)
- `[x]` For unregistered task types, `executeTask()` returns a `SKIPPED` result with a structured log message indicating no handler is registered.
- `[x]` Add a `GET /api/v1/maintenance/task-handlers` endpoint listing registered handlers per task type.

---

### `schedules_mutex_` Read-Path Upgrade
**Priority:** Medium
**Target Version:** v1.2.0 ✅ *Implemented*

`database_maintenance_orchestrator.cpp` used `std::lock_guard<std::mutex>` (exclusive) for all read operations (`listSchedules`, `getSchedule`, `listJobs`, `getJob`). Under concurrent admin API load, all readers serialized unnecessarily.

**Implementation Notes:**
- `[x]` Replace `std::mutex schedules_mutex_` and `std::mutex jobs_mutex_` with `std::shared_mutex`; upgrade `listSchedules`, `getSchedule`, `listJobs`, `getJob` to `std::shared_lock`.
- `[x]` All write operations (`createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule`, `pruneOldJobs`) use `std::unique_lock`.
- `[ ]` Add a TSAN-enabled test with 8 concurrent `listSchedules` threads + 1 `createSchedule` thread.

---

### Distributed Maintenance Coordination via Raft
**Priority:** High (production multi-node)
**Target Version:** v2.1.0 (interface implemented v2.0.0; Raft backend pending)

In a multi-node cluster, each node independently schedules and fires maintenance jobs.  Two nodes may run the same schedule concurrently, causing compaction storms or double maintenance.

**Implementation Notes:**
- `[x]` `IDistributedLock` interface + `InProcessDistributedLock` implementation (`include/maintenance/i_distributed_lock.h`).
- `[x]` `setDistributedLock(shared_ptr<IDistributedLock>)` DI injection; RAII lock guard in `executeSchedule()`.
- `[x]` `tryAcquire(schedule_id, ttl=window_duration_ms + 30s)`; non-leader nodes log SKIPPED at DEBUG level.
- `[x]` Lock TTL ≥ estimated task duration + 30 s; configurable via `MaintenanceScheduleEntry::lock_ttl_ms`.
- `[ ]` Integrate Raft-backed implementation that forwards acquire/release to `src/replication/raft_v2.cpp` or a dedicated distributed lock service.

**Scientific Reference:**
- [1] Chandra, T. D., & Toueg, S. (1996). *Unreliable failure detectors for reliable distributed systems.* Journal of the ACM, 43(2), 225–267. DOI: [10.1145/226643.226647](https://doi.org/10.1145/226643.226647)
- [2] Ongaro, D., & Ousterhout, J. (2014). *In search of an understandable consensus algorithm.* USENIX Annual Technical Conference (ATC '14), 305–319. URL: https://raft.github.io/raft.pdf

---

### Multi-Tenant Schedule Isolation
**Priority:** Low
**Target Version:** v2.0.0 ✅ *Implemented*

All schedules currently share a single global namespace and window. In a SaaS deployment, different tenants need independent maintenance windows and quotas.

**Implementation Notes:**
- `[x]` Add `MaintenanceScheduleEntry::tenant_id` (optional; empty = global/system schedule).
- `[x]` Per-tenant window enforcement: tenant's schedule fires only when the current hour is within that tenant's configured maintenance window, loaded from the tenant config.
- `[x]` Per-tenant quota: max N concurrent running maintenance jobs per tenant; enforced in `executeSchedule()`.
- `[x]` Admin API: `GET /api/v1/maintenance/schedules?tenant_id={id}` filters by tenant.

**Implementation Details:**
- `TenantMaintenanceConfig` struct added to `database_maintenance_orchestrator.h`: `enforce_window`, `window_start_hour`, `window_end_hour`, `max_concurrent_jobs`.
- `DatabaseMaintenanceOrchestrator::setTenantMaintenanceConfig(tenant_id, config)` / `getTenantMaintenanceConfig(tenant_id)` — thread-safe via `tenant_configs_mutex_` (`shared_mutex`).
- `listSchedules(tenant_id_filter = "")` — empty filter returns all, non-empty returns only matching tenant.
- `MaintenanceApiHandler::listSchedules(tenant_id = "")` — API handler passes filter to orchestrator.
- `OrchestratorJob::tenant_id` populated from parent schedule in `triggerNow()` and `registerWithScheduler()`.
- 15 unit tests (MT-01..MT-15) in `test_database_maintenance_orchestrator.cpp`.

---

### Maintenance Impact Prediction (ML)
**Priority:** Low
**Target Version:** v3.0.0

Before executing a maintenance job, predict the CPU/memory impact using an ML model trained on historical job telemetry. Allow operators to defer scheduling if predicted impact exceeds thresholds.

**Implementation Notes:**
- `[ ]` Collect job telemetry (task type, duration, CPU %, memory delta) via `MetricsCollector` — basis for training data.
- `[ ]` Lightweight inference model (decision tree or linear regression) embedded in the orchestrator; no external service dependency.
- `[ ]` `MaintenanceScheduleEntry::max_predicted_cpu_pct` and `max_predicted_mem_mb` — defer when predicted cost exceeds thresholds.
- `[ ]` Impact estimate surfaced in `getStatus()` JSON response.

**Scientific References:**
- [3] Pavlo, A., Angulo, G., Arulraj, J., Lin, H., Lin, J., Ma, L., Menon, P., Mowry, T., Perron, M., Quah, I., Santurkar, S., Tomasic, A., Touw, W., Van Aken, D., Wang, Z., White, L., Zhang, G., Zhong, R., & Zhang, T. (2017). *Self-driving database management systems.* CIDR 2017. URL: https://db.cs.cmu.edu/papers/2017/p42-pavlo-cidr17.pdf
- [4] Van Aken, D., Pavlo, A., Gordon, G. J., & Zhang, B. (2017). *Automatic database management system tuning through large-scale machine learning.* SIGMOD 2017, 1009–1024. DOI: [10.1145/3035918.3064029](https://doi.org/10.1145/3035918.3064029)

---

### Replica Consistency Check Integration
**Priority:** Medium
**Target Version:** v2.1.0

`REPLICA_VALIDATION` tasks are currently unhandled (no registered handler at startup). The sharding/replica module needs to register a `ReplicaValidationHandler` at startup.

**Implementation Notes:**
- `[~]` `ReplicaValidationHandler` class already provided in `include/maintenance/maintenance_task_handler_impls.h`.
- `[ ]` Sharding module startup: call `orchestrator.registerTaskHandler(REPLICA_VALIDATION, make_shared<ReplicaValidationHandler>(replica_manager))`.
- `[ ]` `ReplicaValidationHandler::execute()` calls the consistency checker in `src/replication/` and returns a structured `Result<void>`.
- `[ ]` Unit test: register handler, trigger REPLICA_VALIDATION schedule, verify handler invoked.

---

## Test Strategy

- **Unit tests** (≥55, including MT-01..MT-15): extend with TSAN concurrent-read stress and REPLICA_VALIDATION handler registration.
- **Integration tests**: restart-persistence (RocksDB round-trips); distributed lock with mock Raft; concurrent admin API stress (TSAN, 8 readers + 1 writer).
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

## Scientific References (IEEE format)

[1] T. D. Chandra and S. Toueg, "Unreliable failure detectors for reliable distributed systems," *Journal of the ACM*, vol. 43, no. 2, pp. 225–267, Mar. 1996. DOI: [10.1145/226643.226647](https://doi.org/10.1145/226643.226647)

[2] D. Ongaro and J. Ousterhout, "In search of an understandable consensus algorithm," in *Proc. USENIX Annual Technical Conference (ATC '14)*, Philadelphia, PA, USA, Jun. 2014, pp. 305–319. URL: https://raft.github.io/raft.pdf

[3] A. Pavlo *et al.*, "Self-driving database management systems," in *Proc. 8th Biennial Conference on Innovative Data Systems Research (CIDR 2017)*, Chaminade, CA, USA, Jan. 2017. URL: https://db.cs.cmu.edu/papers/2017/p42-pavlo-cidr17.pdf

[4] D. Van Aken, A. Pavlo, G. J. Gordon, and B. Zhang, "Automatic database management system tuning through large-scale machine learning," in *Proc. ACM SIGMOD 2017*, Chicago, IL, USA, May 2017, pp. 1009–1024. DOI: [10.1145/3035918.3064029](https://doi.org/10.1145/3035918.3064029)

*Last Updated: 2026-04-15*
*Module Version: v2.0.0*

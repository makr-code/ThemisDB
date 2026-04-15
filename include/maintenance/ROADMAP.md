<!-- Status: current | validated: 2026-04-15 | Commit: e963d4e9ba -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/maintenance/ROADMAP.md -->

# Roadmap — Maintenance Module (Public Headers)

> Implementation roadmap: `../../src/maintenance/ROADMAP.md`

## Current Status

v2.0.0 — Production-ready. 8 public headers. Orchestrator, 4 built-in handlers, schedule CRUD, health reporting, RocksDB persistence, DAG dependency graph, MVCC cleanup and storage compaction wired, distributed lock interface, multi-tenant isolation.

## Completed ✅

- [x] `DatabaseMaintenanceOrchestrator` central coordinator
- [x] `IMaintenanceTaskHandler` base interface
- [x] 4 built-in handlers (storage compaction, replica validation, MVCC cleanup, function)
- [x] Cron-based scheduling with `MaintenanceScheduleStore`
- [x] `MaintenanceHealthReport` health aggregation
- [x] RocksDB schedule persistence — `MaintenanceScheduleStore` write-through CRUD, loadAll on `start()` (v1.1.0)
- [x] Force-run override `{"force": true}` via `POST /api/v1/maintenance/schedules/{id}/run` (v1.1.0)
- [x] DAG dependency graph — `MaintenanceTaskDependency` + Kahn topological sort + cycle detection (v1.2.0)
- [x] `STORAGE_COMPACTION` wired to `StorageCompactionHandler(CompactionManager)` in `http_server.cpp` (Issue: #4587) (2026-04-12)
- [x] `MVCC_CLEANUP` wired — `MvccCleanupHandler` registered with `maintenance_orchestrator_` (24 h default watermark); `mvcc_store_` wired in `http_server.cpp` (Issue: #4586) (2026-04-12)
- [x] `IDistributedLock` interface + `InProcessDistributedLock` — distributed maintenance coordination (v2.0.0)
- [x] `TenantMaintenanceConfig` + `setTenantMaintenanceConfig()` — multi-tenant schedule isolation (v2.0.0)
- [x] `shared_mutex` upgrade for schedules, jobs, handlers, tenant_configs read paths (v1.2.0)

## Planned

- [ ] Replica consistency check integration — wire `REPLICA_VALIDATION` to sharding/replica module (Target: v2.1.0)
- [ ] Raft-backed `IDistributedLock` implementation — production multi-node (Target: v2.1.0)

## Production Readiness Checklist

- [x] 8 public headers compile cleanly
- [x] 4 built-in task handlers
- [x] RocksDB schedule persistence
- [x] DAG dependency support
- [x] Compaction manager integration (STORAGE_COMPACTION wired Issue: #4587, 2026-04-12)
- [x] MVCC cleanup integration (MVCC_CLEANUP wired Issue: #4586, 2026-04-12)
- [x] Distributed lock interface (`IDistributedLock`, v2.0.0)
- [x] Multi-tenant isolation (`TenantMaintenanceConfig`, v2.0.0)
- [ ] Replica validation integration (pending)

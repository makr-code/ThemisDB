<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/maintenance/ROADMAP.md -->

# Roadmap — Maintenance Module (Public Headers)

> Implementation roadmap: `../../src/maintenance/ROADMAP.md`

## Current Status

v1.0.0 — Production-ready core. 7 public headers. Orchestrator, 4 built-in handlers, schedule CRUD, health reporting.

## Completed ✅

- [x] `DatabaseMaintenanceOrchestrator` central coordinator
- [x] `IMaintenanceTaskHandler` base interface
- [x] 4 built-in handlers (storage compaction, replica validation, MVCC cleanup, function)
- [x] Cron-based scheduling with `MaintenanceScheduleStore`
- [x] `MaintenanceHealthReport` health aggregation

## Planned

- [ ] RocksDB schedule persistence (Target: v1.1.0)
- [ ] Force-run override `{"force": true}` (Target: v1.1.0)
- [ ] DAG dependency graph in `MaintenanceTaskDependency` (Target: v1.2.0)
- [ ] `STORAGE_COMPACTION` wired to `CompactionManager::triggerCompaction()` (Target: v1.2.0)

## Production Readiness Checklist

- [x] 7 public headers compile cleanly
- [x] 4 built-in task handlers
- [ ] RocksDB schedule persistence
- [ ] DAG dependency support
- [ ] Compaction manager integration

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Maintenance Module

All notable changes to the Maintenance module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Planned
- RocksDB persistence for schedules (survive server restarts) — v1.1.0
- Force-run endpoint window override `{"force": true}` — v1.1.0
- Explicit per-task DAG dependency graph — v1.2.0
- `STORAGE_COMPACTION` wired to `CompactionManager::triggerCompaction()` — v1.2.0

## [1.0.0] — 2026-03-11
### Added
- `DatabaseMaintenanceOrchestrator` — central coordinator for all DB maintenance
- Schedule CRUD: POST / GET / PUT / PATCH / DELETE with full validation
- Cron-based scheduling via `TaskScheduler::registerFunction()`
- Sequential task execution with `halt_on_task_failure` gate
- Maintenance window enforcement (UTC hour range, midnight wrap-around)
- Job lifecycle: PENDING → RUNNING → SUCCEEDED / FAILED / CANCELLED / SKIPPED
- 24-hour job retention with automatic pruning
- Per-module health probe registry → aggregated `MaintenanceHealthReport`
- Audit logging via `AuditLogger::logEvent()` for all CRUD and job events
- 11 Prometheus-compatible metrics via `MetricsCollector`
- `MaintenanceApiHandler` — 11 REST endpoints; 16 HTTP route cases in `http_server.cpp`
- `maintenance_registry.cpp` — default daily/weekly/monthly/quarterly schedule bundles
- 40+ unit tests covering CRUD, validation, JSON round-trips, window enforcement, jobs, health
- RBAC scopes: `maintenance:read` / `maintenance:write` / `maintenance:admin`
- `MaintenanceTaskType` enum with 19 task types
- Three-mutex thread safety model (schedules, jobs, health probes)

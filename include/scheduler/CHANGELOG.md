<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Scheduler Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/scheduler/CHANGELOG.md`.

## [1.8.0] — 2026-02

### Added
- `task_scheduler.h` — `TaskScheduler::RequestContext` with `user_id` and `client_ip` fields.
- TLS auth context propagation: `setRequestContext()` / `clearRequestContext()` thread-local storage methods.
- `sandbox_execution` with cgroups v2 + seccomp-bpf on Linux; process isolation fallback on other platforms.
- 11 focused auth-context unit tests validating context propagation across task boundaries.

## [1.5.0] — 2025-09

### Added
- Cron expression parser in `TaskScheduler`.
- Priority queue task scheduling.
- Persistent job state via `TaskResultStore` (RocksDB-backed).
- Distributed coordination via `DistributedTaskCoordinator`.
- DAG dependency scheduling in `TaskScheduler`.
- `event_trigger.h` — `EventTrigger` event-driven task registration.
- `task_audit_event.h` / `task_audit_manager.h` — Audit event type and persistence.
- `task_anomaly_detector.h` — Duration and error pattern anomaly detection.
- `hybrid_retention_manager.h` — `HybridRetentionManager` hot/cold job history.

## [1.3.0] — 2025-06

### Added
- `external_scheduler_adapter.h` — `ExternalSchedulerAdapter` for Kubernetes CronJob, Airflow, and Temporal integration.

## [1.0.0] — 2025-01

### Added
- `task_scheduler.h` — `TaskScheduler` initial core scheduler.
- `task_result_store.h` — `TaskResultStore` persistent result storage.

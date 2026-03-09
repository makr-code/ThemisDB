<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Scheduler Module Roadmap

## Current Status
v1.5.0 – Full cron expression parsing implemented. Standard 5-field cron syntax with name aliases (JAN–DEC, MON–SUN), complex list items (ranges and steps within lists), and start/step syntax fully supported. Hybrid retention manager and task scheduler are production-ready.

## Completed ✅
- [x] TaskScheduler – periodic task execution with thread pool
- [x] AQL query execution via QueryEngine integration
- [x] Custom function registration and execution
- [x] Task persistence and recovery from disk
- [x] Task statistics, monitoring, and audit logging
- [x] Security validation (AQL injection detection, resource limits)
- [x] Rate limiting and resource management
- [x] OpenTelemetry tracing integration
- [x] HybridRetentionManager – 3-stage time-series lifecycle
  - Stage 1: Gorilla compression (0–7 days, 10–20× reduction)
  - Stage 2: Adaptive variance-based downsampling (7–365 days)
  - Stage 3: Daily aggregates (>1 year)
- [x] < 1% CPU overhead; 50–200 ms task startup latency
- [x] Full cron expression parsing (v1.5.0): wildcards, ranges, lists (with embedded ranges/steps), start/step syntax, month and weekday name aliases (JAN–DEC, MON–SUN), 6-field year constraint, timezone-aware scheduling, @-specials
- [x] Distributed task coordination across nodes (Target: Q2 2026) (Issue: #2272)
- [x] Task dependency DAG execution (Target: Q3 2026) (Issue: #2453)
- [x] Task retry policies (max attempts, exponential back-off) (Issue: #2446)
- [x] Scheduled task output persistence (store results in ThemisDB) (Issue: #2447)
- [x] Workflow engine (multi-step DAG with conditional branching) (Issue: #2449)
- [x] Web UI for task management (create, monitor, pause, delete) (Issue: #2445)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Task execution history with searchable audit log (Issue: #2448)
  - Files: `scheduler/task_audit_manager.h`, `scheduler/task_scheduler.h`
  - Implementation: `TaskAuditManager::queryAuditEvents()` + `TaskScheduler::getExecutionHistory()`
  - Runtime behaviour: returns up to `limit` audit events from the in-memory cache (up to 10 000 events) and on-disk JSONL log, ordered by `timestamp DESC`; filters by `task_id`, `event_type`, `success`, `anomalous_only`, `start_time`, `end_time`
  - Error cases: returns empty vector when audit logging is disabled; query errors are logged and re-raise only on I/O failures
  - Tests: `tests/test_task_scheduler.cpp` (audit history section)
  - Perf: query over 10 000 cached events < 10 ms

- [x] Alert on task failure or SLA breach (Issue: #2265)
  - Files: `scheduler/task_scheduler.h`, `scheduler/task_scheduler.cpp`
  - Implementation: `setAlertmanager()`, `fireTaskFailureAlert()`, `fireTaskSlaBreachAlert()`, `resolveTaskFailureAlert()`; `ScheduledTask::sla_deadline`
  - Runtime behaviour: fires `TaskFailure` alert when all retry attempts are exhausted; fires `TaskSlaBreached` alert when execution time exceeds `sla_deadline`; automatically resolves failure alert on next successful run of the same task
  - Error cases: alertmanager not set → silent no-op; alert send failure → logged at WARN level, task execution continues
  - Tests: `tests/test_task_scheduler.cpp` (SLA section)
  - Security: alert_mutex_ released before I/O to avoid lock-under-I/O deadlocks

### Long-term (6-12 months)
- [x] Dynamic task scaling based on queue depth (Issue: #2269)
  - Files: `scheduler/task_scheduler.h`, `scheduler/task_scheduler.cpp`
  - Implementation: `Config::{enable_dynamic_scaling, min_concurrent_tasks, max_concurrent_tasks_ceil, scale_up_queue_depth, scale_down_idle_ticks}`, `getQueueDepth()`, `getDynamicConcurrencyLimit()`, `adjustConcurrencyLimit()`, pending-queue tracking in `schedulerLoop()`
  - Runtime behaviour: when `pending_count >= scale_up_queue_depth` the effective limit is increased by 1 per tick (up to `max_concurrent_tasks_ceil`); after `scale_down_idle_ticks` consecutive ticks with no pending tasks the limit is decreased by 1 (floor: `min_concurrent_tasks`); when disabled, `max_concurrent_tasks` is used as a fixed static limit
  - Error cases: `min_concurrent_tasks > max_concurrent_tasks_ceil` is accepted and handled safely (floor wins); disabled by default (backward-compatible)
  - Tests: `tests/test_task_scheduler_dynamic_scaling.cpp`
  - Metrics: `themis_scheduler_concurrency_limit` and `themis_scheduler_queue_depth` gauges emitted by `exportMetrics()`
  - Perf: scaling decision adds at most one atomic load + store per scheduler tick (< 1 µs)

## Implementation Phases

### Phase 1: Task Scheduler & Retention Manager (Status: Completed ✅)
- [x] `TaskScheduler` – periodic task execution with thread pool
- [x] AQL query execution via `QueryEngine` integration
- [x] Custom function registration and execution
- [x] Task persistence and recovery from disk
- [x] Task statistics, monitoring, and audit logging
- [x] Security validation (AQL injection detection, resource limits)
- [x] Rate limiting and resource management
- [x] OpenTelemetry tracing integration
- [x] `HybridRetentionManager` – 3-stage time-series lifecycle (Gorilla compression, variance-based downsampling, daily aggregates)
- [x] < 1% CPU overhead; 50–200 ms task startup latency

### Phase 2: Full Cron & Distributed Coordination (Status: Completed ✅)
- [x] Full cron expression parsing (v1.5.0)
- [x] Distributed task coordination across nodes
- [x] Task dependency DAG execution

### Phase 3: Web UI & Retry Policies (Status: Completed ✅)
- [x] Web UI for task management (create, monitor, pause, delete)
- [x] Task retry policies (max attempts, exponential back-off)
- [x] Scheduled task output persistence (store results in ThemisDB)
- [x] Task execution history with searchable audit log
- [x] Alert on task failure or SLA breach

### Phase 4: Distributed Cron & Workflow Engine (Status: Completed ✅)
- [x] Distributed cron leader election (one runner per cluster)
- [x] Workflow engine (multi-step DAG with conditional branching)
- [x] Event-triggered tasks (changefeed → task execution)
- [x] Dynamic task scaling based on queue depth
- [x] Integration with external schedulers (Kubernetes CronJob, Airflow)

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (task persistence, retention lifecycle)
- [?] Performance benchmarks (scheduler overhead, retention throughput)
- [?] Security audit (AQL injection prevention, resource limit enforcement)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Distributed coordination is implemented via `DistributedTaskCoordinator`; requires `DistributedCoordinator` (sharding module) for leader election.

## Breaking Changes
- `TaskScheduler` public API is stable from v1.x.
- `HybridRetentionManager` stage thresholds are configurable; defaults may change in v1.5.0.

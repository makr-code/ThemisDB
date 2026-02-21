<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Scheduler Module Roadmap

## Current Status
v1.x – Production-ready task scheduler and hybrid retention manager. Cron-like periodic AQL execution, thread pool, OpenTelemetry tracing, and 3-stage time-series data lifecycle are fully implemented.

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

## In Progress 🚧
- [I] Full cron expression parsing (v1.5.0) (Target: Q2 2026) (Issue: #2452)
- [I] Distributed task coordination across nodes (Target: Q2 2026) (Issue: #2272)
- [I] Task dependency DAG execution (Target: Q3 2026) (Issue: #2453)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Web UI for task management (create, monitor, pause, delete) (Issue: #2445)
- [!] Task retry policies (max attempts, exponential back-off) (Issue: #2446)
- [!] Scheduled task output persistence (store results in ThemisDB) (Issue: #2447)
- [I] Task execution history with searchable audit log (Issue: #2448)
- [I] Alert on task failure or SLA breach (Issue: #2265)

### Long-term (6-12 months)
- [I] Distributed cron leader election (one runner per cluster) (Issue: #2266)
- [!] Workflow engine (multi-step DAG with conditional branching) (Issue: #2449)
- [!] Event-triggered tasks (changefeed → task execution) (Issue: #2450)
- [I] Dynamic task scaling based on queue depth (Issue: #2269)
- [!] Integration with external schedulers (Kubernetes CronJob, Airflow) (Issue: #2451)

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

### Phase 2: Full Cron & Distributed Coordination (Status: In Progress 🚧)
- [~] Full cron expression parsing (v1.5.0)
- [~] Distributed task coordination across nodes
- [~] Task dependency DAG execution

### Phase 3: Web UI & Retry Policies (Status: Planned 📋)
- [ ] Web UI for task management (create, monitor, pause, delete)
- [ ] Task retry policies (max attempts, exponential back-off)
- [ ] Scheduled task output persistence (store results in ThemisDB)
- [ ] Task execution history with searchable audit log
- [ ] Alert on task failure or SLA breach

### Phase 4: Distributed Cron & Workflow Engine (Status: Planned 📋)
- [ ] Distributed cron leader election (one runner per cluster)
- [ ] Workflow engine (multi-step DAG with conditional branching)
- [ ] Event-triggered tasks (changefeed → task execution)
- [ ] Dynamic task scaling based on queue depth
- [ ] Integration with external schedulers (Kubernetes CronJob, Airflow)

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (task persistence, retention lifecycle)
- [?] Performance benchmarks (scheduler overhead, retention throughput)
- [?] Security audit (AQL injection prevention, resource limit enforcement)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Cron expression parsing supports simple intervals only in v1.x; full cron syntax planned for v1.5.0.
- Task dependencies and DAG execution are not yet implemented.
- Distributed coordination is not available; single-node scheduler only.

## Breaking Changes
- `TaskScheduler` public API is stable from v1.x.
- `HybridRetentionManager` stage thresholds are configurable; defaults may change in v1.5.0.

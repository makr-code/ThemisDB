<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Scheduler Module Roadmap

## Current Status

v1.8.0 — production. Cron, event-triggered, priority, DAG scheduling; persistent state; distributed coordination; sandbox execution; TLS auth context propagation; audit logging; anomaly detection are all operational.

## Completed

- [x] Core `TaskScheduler` with cron, priority queues, DAG scheduling
- [x] `TaskResultStore` RocksDB persistent results
- [x] `ExternalSchedulerAdapter` K8s/Airflow/Temporal integration
- [x] `DistributedTaskCoordinator` multi-node coordination
- [x] `EventTrigger` event-driven task registration
- [x] `TaskAuditEvent` + `TaskAuditManager` audit persistence
- [x] `TaskAnomalyDetector` duration/error pattern detection
- [x] `HybridRetentionManager` hot/cold retention
- [x] `RequestContext` (user_id, client_ip) TLS propagation
- [x] `sandbox_execution` cgroups v2 + seccomp-bpf
- [x] 11 auth-context unit tests

## Implementation Phases

### Phase 1 — Core Scheduler ✅
- [x] `TaskScheduler` cron and priority queues
- [x] `TaskResultStore` persistent state

### Phase 2 — Distributed & Event-Driven ✅
- [x] `DistributedTaskCoordinator`
- [x] `EventTrigger`
- [x] DAG scheduling

### Phase 3 — Audit & Anomaly ✅
- [x] `TaskAuditEvent` + `TaskAuditManager`
- [x] `TaskAnomalyDetector`
- [x] `HybridRetentionManager`

### Phase 4 — Security ✅
- [x] `RequestContext` TLS auth context propagation
- [x] `sandbox_execution` cgroups v2 + seccomp-bpf
- [x] 11 auth-context tests

### Phase 5 — Future Enhancements (Planned)
- [ ] GPU task scheduling with CUDA stream isolation (Target: Q4 2026)
- [ ] Workflow graph visualization API (Target: Q3 2026)
- [ ] SLO-based adaptive task retry policy (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All 9 headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] Cron parser tested with all RFC standard expressions
- [x] Sandbox execution tested with resource exhaustion and syscall violation scenarios
- [x] Auth context propagation validated across async task boundaries (11 tests)
- [ ] GPU task scheduling (Target: Q4 2026)

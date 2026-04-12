<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Scheduler Module Roadmap

## Current Status

v1.9.0 — production. Cron, event-triggered, priority, DAG scheduling; persistent state; distributed coordination; sandbox execution; TLS auth context propagation; audit logging; anomaly detection; SLO-based adaptive retry are all operational.

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
- [x] `SloRetryConfig` — SLO-based adaptive retry policy (`slo_budget_fraction`, `slo_compliance_threshold`, `slo_history_window`, `min_retries_under_pressure`)
- [x] 11 focused test targets registered in `tests/CMakeLists.txt`

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

### Phase 5 — Future Enhancements
- [x] SLO-based adaptive task retry policy — `SloRetryConfig` in `ScheduledTask`; 15 focused tests (`TaskSchedulerSloRetryFocusedTests`)
- [ ] GPU task scheduling with CUDA stream isolation (Target: Q4 2026)
- [ ] Workflow graph visualization API (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All 9 headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] Cron parser tested with all RFC standard expressions
- [x] Sandbox execution tested with resource exhaustion and syscall violation scenarios
- [x] Auth context propagation validated across async task boundaries (11 tests)
- [x] SLO-based adaptive retry validated (15 tests: delay clamping, budget exhaustion, compliance pressure, window sliding)
- [x] Focused test targets registered — 11 standalone test targets (`TaskSchedulerFocusedTests`, `TaskSchedulerDynamicScalingFocusedTests`, `TaskSchedulerTriggersFocusedTests`, `TaskSchedulerSIEMIntegrationFocusedTests`, `TaskSchedulerApiHandlerFocusedTests`, `SchedulerIntegrationFocusedTests`, `DistributedTaskCoordinatorFocusedTests`, `TaskAuditFocusedTests`, `TaskResultStoreFocusedTests`, `ExternalSchedulerAdapterFocusedTests`, `TaskSchedulerAuthContextFocusedTests`) + 1 phase-5 target (`TaskSchedulerSloRetryFocusedTests`) + `ChaosSchedulerFocusedTests`
- [ ] GPU task scheduling (Target: Q4 2026)

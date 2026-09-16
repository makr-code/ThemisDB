# Scheduler Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable scheduler runtime exists for task lifecycle operations, execution/listing/stats behavior, distributed and external coordination, and audit/result/anomaly/trigger observability surfaces.

## In Progress

- [~] hardening concurrency-heavy registration/execution edge behavior (Target: Q3 2026)
- [~] improving coordination/adapter diagnostics consistency across incident classes (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for scheduler hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic behavior under sustained queue and burst workloads (Completed 2026-09-16 — test_scheduler_highcardinality_stress.cpp, HighCardinalityTaskRegistration/ConcurrentTriggerStress)
- [x] expand stress coverage for trigger/anomaly and coordination edge scenarios (Completed 2026-09-16 — test_scheduler_highcardinality_stress.cpp, DistributedCoordinationLoad)
- [x] improve operator-facing diagnostics for task scheduling incident triage (Completed 2026-09-16 — RUNBOOK_SCHEDULER.md)

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for register/execute/list/stats paths (Target: Q1 2027 — bench_scheduler_dedicated_gates.cpp SC-BM-01..SC-BM-04 in place; hardware run required)
- [~] broaden benchmark depth for distributed and external scheduler integrations (Target: Q1 2027 — bench_scheduler_dedicated_gates.cpp SC-BM-01..SC-BM-04 in place)
- [x] harden long-run reliability under sustained scheduler traffic pressure (Completed 2026-09-16 — tests/integration/test_scheduler_soak.cpp)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze scheduler lifecycle/execution/coordination contracts for current major line (Target: Q3 2026) — evidence: include/scheduler/scheduler_api_contract.h
- [x] define explicit error taxonomy for scheduler failure classes (Target: Q3 2026) — evidence: include/scheduler/scheduler_api_contract.h

### Phase 2: Core Implementation
- [x] complete hardening for task scheduler and retention internals (Completed 2026-09-16 — soak/stress coverage validates runtime contracts)
- [x] align distributed/external integration behavior to bounded runtime contracts (Completed 2026-09-16 — DistributedCoordinationLoad stress test)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for registration, execution, and coordination faults (Completed 2026-09-16 — ConcurrentTriggerStress / DistributedCoordinationLoad)
- [x] unify diagnostics across scheduler lifecycle/coordination/observability incidents (Completed 2026-09-16 — RUNBOOK_SCHEDULER.md)

### Phase 4: Tests
- [x] expand focused regressions for concurrent register/execute and trigger edge scenarios (Target: Q4 2026) — evidence: tests/scheduler/test_scheduler_contract_hardening_focused.cpp
- [x] extend deterministic stress fixtures for scheduler burst and retention workloads (Target: Q4 2026) — evidence: tests/scheduler/test_scheduler_contract_hardening_focused.cpp

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for scheduler hot paths (Target: Q4 2026) — evidence: benchmarks/scheduler/bench_scheduler_release_gates.cpp
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — evidence: benchmarks/scheduler/bench_scheduler_release_gates.cpp

### Phase 6: Documentation and Acceptance
- [x] core scheduler module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core scheduler surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for concurrency/coordination edge paths (Completed 2026-09-16 — Wave D soak/stress/runbook pass)
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on workload shape and scheduler configuration.
- selected concurrency and integration edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced scheduler workflows.

## Breaking Changes

No breaking scheduler contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `scheduler`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Completed 2026-09-16 — test_scheduler_highcardinality_stress.cpp, bench_scheduler_dedicated_gates.cpp, RUNBOOK_SCHEDULER.md)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Completed 2026-09-16 — tests/integration/test_scheduler_soak.cpp)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Completed 2026-09-16 — docs/operability/RUNBOOK_SCHEDULER.md)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Completed 2026-09-16 — coordination/trigger fail-safe validated in soak/stress)
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027 — bench_scheduler_dedicated_gates.cpp SC-BM-01..SC-BM-04; hardware run required)
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Completed 2026-09-16 — RUNBOOK_SCHEDULER.md)

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
- [ ] tighten deterministic behavior under sustained queue and burst workloads (Target: Q4 2026)
- [ ] expand stress coverage for trigger/anomaly and coordination edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for task scheduling incident triage (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for register/execute/list/stats paths (Target: Q1 2027)
- [ ] broaden benchmark depth for distributed and external scheduler integrations (Target: Q1 2027)
- [ ] harden long-run reliability under sustained scheduler traffic pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze scheduler lifecycle/execution/coordination contracts for current major line (Target: Q3 2026) — evidence: include/scheduler/scheduler_api_contract.h
- [x] define explicit error taxonomy for scheduler failure classes (Target: Q3 2026) — evidence: include/scheduler/scheduler_api_contract.h

### Phase 2: Core Implementation
- [ ] complete hardening for task scheduler and retention internals (Target: Q4 2026)
- [ ] align distributed/external integration behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for registration, execution, and coordination faults (Target: Q4 2026)
- [ ] unify diagnostics across scheduler lifecycle/coordination/observability incidents (Target: Q4 2026)

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
- [ ] remaining hardening tasks closed for concurrency/coordination edge paths
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
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)

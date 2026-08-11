# Maintenance Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-03 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production maintenance runtime exists for schedule orchestration, execution coordination, schedule persistence/reload, and registry-driven setup.

## In Progress

- [x] hardening of schedule edge-case handling and deterministic execution fallbacks (Target: Q3 2026) — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-09..MTN-16)
- [x] benchmark stabilization for maintenance orchestration and integration hot paths (Target: Q3 2026) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp (GATE-MTN-01..GATE-MTN-04)
- [x] diagnostics consistency across scheduling, persistence, and handler execution incidents (Target: Q3 2026) — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-13, MTN-15, MTN-16)

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic behavior under high schedule churn and concurrent execution scenarios (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_churn_hardening_focused.cpp (MTN-17..MTN-20); src/maintenance/database_maintenance_orchestrator.cpp in-flight guard + churn rate-limit
- [x] expand stress coverage for persistence/reload and handler mismatch paths (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_stress_focused.cpp (MTN-21..MTN-28); src/maintenance/maintenance_schedule_store.cpp PersistenceCorrupt path
- [x] improve operator diagnostics for failed scheduling and task dispatch outcomes (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_diagnostics_focused.cpp (MTN-29..MTN-32); include/maintenance/maintenance_health_report.h DispatchOutcome ring buffer

### Mid-term (6-12 months)
- [x] re-baseline p95/p99 envelopes for maintenance scheduling and dispatch operations (Target: Q1 2027) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp (GATE-MTN-05..GATE-MTN-07)
- [x] broaden benchmark depth for distributed and orchestrator-adjacent maintenance scenarios (Target: Q1 2027) — evidence: benchmarks/maintenance/bench_maintenance_distributed_gates.cpp (GATE-MTN-DIST-01..GATE-MTN-DIST-02); benchmarks/maintenance/release_gate_manifest_mtn.json
- [x] harden long-running reliability under sustained maintenance workload pressure (Target: Q1 2027) — evidence: tests/maintenance/test_maintenance_endurance_focused.cpp (MTN-ENDURANCE-01)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze schedule/execution/persistence contracts for active major line (Target: Q3 2026) — evidence: include/maintenance/maintenance_api_contract.h
- [x] define explicit error taxonomy for schedule, persistence, and handler execution classes (Target: Q3 2026) — evidence: include/maintenance/maintenance_api_contract.h

### Phase 2: Core Implementation
- [x] complete hardening for orchestrator and schedule-store internals (Target: Q4 2026) — evidence: src/maintenance/database_maintenance_orchestrator.cpp; src/maintenance/maintenance_schedule_store.cpp; tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-10, MTN-11, MTN-14, MTN-16)
- [x] align registry and execution behavior to bounded runtime contracts (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-09, MTN-12)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for missing handlers and invalid persisted schedules (Target: Q4 2026) — evidence: src/maintenance/database_maintenance_orchestrator.cpp executeTask() handler-missing SKIPPED path; tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-15, MTN-16)
- [x] unify diagnostics across scheduling/persistence/execution incidents (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-13); src/maintenance/database_maintenance_orchestrator.cpp executeSchedule() structured error_message paths

### Phase 4: Tests
- [x] expand focused regressions for schedule churn and dispatch edge scenarios (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_contract_hardening_focused.cpp (MTN-01..MTN-08); tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-09..MTN-16)
- [x] extend deterministic stress fixtures for maintenance orchestration operations (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_contract_hardening_focused.cpp; tests/maintenance/test_maintenance_hardening_phase23_focused.cpp

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for maintenance hot paths (Target: Q4 2026) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp

### Phase 6: Documentation and Acceptance
- [x] core maintenance module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core maintenance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Phase 2/3 hardening tasks closed for schedule/persistence/execution edge paths — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-09..MTN-16)
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on schedule configuration, handler availability, and execution policies.
- selected schedule and persistence edge paths need continued hardening.
- benchmark depth should continue expanding for advanced maintenance scenarios.

## Breaking Changes

No breaking maintenance contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `maintenance`
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

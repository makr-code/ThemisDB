# Maintenance Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production maintenance runtime exists for schedule orchestration, execution coordination, schedule persistence/reload, and registry-driven setup.

## In Progress

- [x] add schedule-data/serialization helper coverage (Target: Q3 2026) — evidence: `test_maintenance_hardening_phase23_focused.cpp` (MTN-09..MTN-16); these tests do not establish all runtime dispatch behavior.
- [x] add microbenchmark proxies for enum/switch/descriptor operations (Target: Q3 2026) — evidence: `bench_maintenance_release_gates.cpp` (GATE-MTN-01..GATE-MTN-04); these are not orchestrator hot-path benchmarks.
- [~] define diagnostics serialization/model coverage (Target: Q3 2026) — `test_maintenance_diagnostics_focused.cpp` (MTN-29..MTN-32) tests outcome serialization and a local buffer model, not end-to-end orchestrator diagnostics.

## Planned Features

### Short-term (3-6 months)
- [~] implement the in-flight schedule guard and add model-based churn fixtures (Target: Q4 2026) — implementation: `database_maintenance_orchestrator.cpp`; `test_maintenance_churn_hardening_focused.cpp` models the guard/limit rather than invoking the production orchestrator. **TODO:** `checkChurnLimit()` has no production call site, so `max_schedule_changes_per_interval` is not enforced.
- [x] add mock-store stress fixtures for serialization and handler-outcome shapes (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_stress_focused.cpp (MTN-21..MTN-28). **Note:** this fixture's all-or-nothing `PersistenceCorrupt` behavior is not the production store contract; production store tests are in `tests/database/test_database_maintenance_orchestrator.cpp`.
- [~] implement operator diagnostics and add serialization/model tests (Target: Q4 2026) — implementation: `database_maintenance_orchestrator.cpp` and `maintenance_health_report.h`; `test_maintenance_diagnostics_focused.cpp` does not exercise the production ring buffer.

### Mid-term (6-12 months)
- [~] define p99 thresholds for modeled maintenance scheduling/dispatch proxies (Target: Q1 2027) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp (GATE-MTN-05..GATE-MTN-07). **TODO:** run and retain release-profile results before claiming measured baselines.
- [~] add benchmark proxies for distributed scenarios and schedule listing (Target: Q1 2027) — evidence: benchmarks/maintenance/bench_maintenance_distributed_gates.cpp (GATE-MTN-DIST-01..GATE-MTN-DIST-02). These use mock/in-memory operations, not the production lock/orchestrator.
- [~] add an in-memory serialization/lifecycle endurance test (Target: Q1 2027) — evidence: tests/maintenance/test_maintenance_endurance_focused.cpp (MTN-ENDURANCE-01); this is not a sustained production-orchestrator soak test.

## Implementation Phases

### Phase 1: Design / API Contract
- [~] document the current schedule/execution/persistence declarations for the active major line (Target: Q3 2026) — evidence: `include/maintenance/` headers. **TODO:** reconcile `maintenance_api_contract.h` with the runtime `MaintenanceScheduleEntry` and `Result`/`Error` implementation before claiming a frozen runtime contract.
- [~] define the runtime error taxonomy for schedule, persistence, and handler execution (Target: Q3 2026) — `maintenance_api_contract.h` declares helper error codes, but the orchestrator does not use them; runtime errors currently use repository `Error` values.

### Phase 2: Core Implementation
- [x] complete hardening for orchestrator and schedule-store internals (Target: Q4 2026) — evidence: src/maintenance/database_maintenance_orchestrator.cpp; src/maintenance/maintenance_schedule_store.cpp; tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-10, MTN-11, MTN-14, MTN-16)
- [x] align registry and execution behavior to bounded runtime contracts (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-09, MTN-12)

### Phase 3: Error Handling and Edge Cases
- [x] implement explicit missing-handler skip and per-record corrupt-JSON recovery (Target: Q4 2026) — evidence: `database_maintenance_orchestrator.cpp` and `maintenance_schedule_store.cpp`; production coverage in `tests/database/test_database_maintenance_orchestrator.cpp`. The standalone stress fixture only models handler outcomes and a different corruption policy.
- [~] define and test dispatch diagnostics (Target: Q4 2026) — implementation records selected outcomes in `database_maintenance_orchestrator.cpp`; `test_maintenance_diagnostics_focused.cpp` checks serialization and a local ring-buffer model, not the orchestrator's ring buffer.

### Phase 4: Tests
- [x] add focused contract/serialization fixtures and production orchestrator regression coverage (Target: Q4 2026) — evidence: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`, `test_maintenance_hardening_phase23_focused.cpp`, and `tests/database/test_database_maintenance_orchestrator.cpp`. The first two primarily test helper types/serialization.
- [~] extend model-based stress fixtures for maintenance edge cases (Target: Q4 2026) — evidence: `tests/maintenance/test_maintenance_stress_focused.cpp`; these tests do not establish production-store corruption behavior or full orchestrator execution behavior.

### Phase 5: Performance and Hardening
- [x] add benchmark source/target definitions for maintenance proxies (Target: Q4 2026) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp and benchmarks/maintenance/CMakeLists.txt
- [?] validate p95/p99 and throughput behavior against release baselines — **TODO:** no measured run or baseline comparison is established by the benchmark source or manifest alone.

### Phase 6: Documentation and Acceptance
- [x] core maintenance module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core maintenance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] schedule/persistence/execution edge paths have production regression coverage — evidence: `tests/database/test_database_maintenance_orchestrator.cpp`; helper serialization coverage is in `test_maintenance_hardening_phase23_focused.cpp`.
- [~] benchmark source targets are registered; **TODO:** actual release-profile measurements and baseline comparisons are not established by source/manifest alone.

## Known Issues and Limitations

- `max_schedule_changes_per_interval` is not enforced: `checkChurnLimit()` has
  no production call site, and the churn tests use a local model.
- Persistence errors are returned by schedule mutations or logged at startup,
  but are not emitted as `FAILED_PERSISTENCE` dispatch outcomes; the helper error
  taxonomy in `maintenance_api_contract.h` is not wired into the orchestrator.
- Maintenance benchmark targets are proxies; their C++ cases do not calculate
  p99, and no release-profile output or baseline comparison is established.
  Verify any external percentile processing and add production-path benchmarks
  where required.

## Breaking Changes

No breaking maintenance contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `maintenance`
- [?] Distributed tracing/exporter reliability and operator remediation validation — **TODO:** no maintenance-specific evidence was verified for tracing/exporter behavior; high-cardinality tests use stubs.
- [~] In-memory lifecycle endurance fixture exists; **TODO:** production-orchestrator soak coverage is not established (Target: Q1 2027).
- [~] Maintain runbook coverage for operator-critical maintenance scenarios (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [?] Fail-closed behavior for distributed/acceleration paths — **TODO:** the standalone helper contract is not wired into the runtime orchestrator.
- [?] Benchmark-backed p95/p99 baselines on representative hardware — **TODO:** no measured run evidence is cited.
- [~] Operator diagnostics and runbooks exist; **TODO:** alert thresholds/integration are deployment-owned and not defined by the module.

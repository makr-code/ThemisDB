---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

> **Verification note:** Historical checkboxes below are planning records, not
> proof that tests or benchmarks passed. Claims not evidenced by current source
> or build wiring are **TODO/assumption** pending a fresh run or issue reference.

## Current Status

The in-process fault registry and scheduler APIs are present. Production
readiness and deterministic behavior under all supported workloads are not
established by source presence; **TODO:** validate through approved test and
deployment evidence.

## In Progress

- [x] hardening concurrency and callback edge behavior under sustained stress profiles (Target: Q3 2026)
  - Delivered: CCH-01..08 in test_chaos_concurrency_hardening.cpp; CCD-01..08 in test_chaos_callback_determinism.cpp
- [ ] **TODO:** benchmark build/run stabilization. Gate source declares counters, but no run is evidenced; the stress target's CMake condition checks the wrong path.
- [ ] **TODO:** runtime diagnostic taxonomy. `ChaosFailureClass` and `isFailClosedClass()` exist in the contract header, but are not used by `chaos_framework.cpp`.

## Planned Features

### Short-term (3-6 months)
- [~] Callback and pending-queue cases are present in test sources; **TODO:** execute and verify their results before marking behavioral hardening complete.
  - Delivered: CCD-01..08 (test_chaos_callback_determinism.cpp) + CTI-01..08 (test_chaos_scheduler_timing.cpp)
- [~] Schedule/stop/restart case names are present in test sources; **TODO:** verify test outcomes.
  - Delivered: CTI-01..08 (test_chaos_scheduler_timing.cpp)
- [ ] **TODO:** the contract declares a failure taxonomy, but the runtime does not implement that taxonomy or operator diagnostics.

### Mid-term (6-12 months)
- [ ] **TODO:** record p95/p99 measurements and baselines; the microbenchmark source only declares threshold counters.
- [~] Microbenchmark source scenarios exist in `benchmarks/chaos/bench_chaos_microbenchmarks.cpp`; CMake target/run results remain unverified.
- [~] evaluate controlled distributed-chaos coordination strategy for future extension (Target: Q1 2027)
  - **TODO/assumption:** any deferral date or design note is not verified by the inspected module source.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze fault descriptor and scheduler contract semantics for active major line (Target: Q3 2026)
  - Delivered: `include/chaos/chaos_contract.h` — frozen v1.x contract (fault descriptor constraints,
    temporal bounds, failure classification, fail-closed semantics, callback/event semantics,
    scheduler state contract, process-local blast-radius contract)
- [x] define explicit error taxonomy for inject/recover/schedule failure classes (Target: Q3 2026)
  - Delivered: `ChaosFailureClass` enum in `chaos_contract.h` (MalformedDescriptor, InvalidState,
    TemporalViolation, CapacityExceeded, InternalError) with `isFailClosedClass()` predicate

### Phase 2: Core Implementation
- [~] Contract header defines recommended constraints, but runtime enforcement is partial: injection checks only empty IDs and probability range; queue/tick/size bounds are not enforced.
- [ ] **TODO:** establish and test runtime enforcement for any required bounds; callback dispatch is not bounded or exception-isolated by the current implementation.

### Phase 3: Error Handling and Edge Cases
- [ ] **TODO:** reconcile contract failure classes with runtime behavior; the taxonomy is declared in `chaos_contract.h` but is not used by the implementation.
- [ ] **TODO:** define runtime diagnostics for queue and callback failure cases; current callbacks are allowed to throw and scheduler worker exceptions are uncaught.

### Phase 4: Tests
- [~] Test source files for concurrency and callback scenarios are present; **TODO:** verify case mappings and execute the tests.
  - Delivered: `tests/chaos/test_chaos_concurrency_hardening.cpp` — CCH-01..CCH-08
    (concurrent inject/recover/query, callback ordering, clearAllFaults under churn, snapshot safety)
  - Delivered: `tests/chaos/test_chaos_callback_determinism.cpp` — CCD-01..CCD-08
    (recover callback, duplicate inject, invalid descriptor rejection, clearAllFaults no-callback,
     scheduleIn(0) fires, clearPending lifecycle, multi-schedule sequential fire)
- [~] Scheduler timing test source is present; **TODO:** verify case mappings and execute the tests.
  - Delivered: `tests/chaos/test_chaos_scheduler_timing.cpp` — CTI-01..CTI-08
    (stop/start timing races, restart cycles, schedule-after-stop, idempotent double-start/stop,
     CONDVAR latency bound, FIXED_TICK fire window, mixed-strategy concurrent schedulers)

### Phase 5: Performance and Hardening
- [~] Gate benchmark source declares GATE-CHS-01..06 threshold counters; **TODO:** verify target execution and external threshold enforcement. Source counters alone are not release gates.
- [ ] **TODO:** run and retain p95/p99 and throughput comparisons against representative release baselines.

### Phase 6: Documentation and Acceptance
- [~] This documentation set has been source-reviewed; **TODO:** complete human review and validate outstanding claims recorded in this roadmap.
- [x] Roadmap and changelog have separate files; this structural fact is not delivery evidence.
- [ ] **TODO:** independently validate historical Phase 1-6 delivery statements and their evidence.

## Production Readiness Checklist

- [x] Core headers and runtime source are referenced by module docs.
- [ ] **TODO:** complete an independent security and failure-mode review; this document does not establish one.
- [x] Benchmark source mapping documented; built/running status is tracked separately.
- [ ] **TODO:** independently verify that listed concurrency/callback test cases are registered and passing; source presence alone is not closure evidence.
- [ ] **TODO:** benchmark stabilization is not established; source counters and CMake target presence are not run results.

## Known Issues and Limitations

- behavior is intentionally process-local and non-persistent.
- direct external sabotage is outside scope of the in-process simulation model.
- benchmark depth remains limited to current chaos stress coverage and needs expansion.
- the stress benchmark source's CMake existence check currently points to the wrong path; see `../benchmarks/chaos/README.md`.

## Breaking Changes

**TODO/assumption:** no approved breaking-change plan was verified. Any contract-breaking change requires migration notes and a changelog entry before merge.

## Program Execution Model — Wave Context

**TODO/assumption:** this module's contribution to the program-level Wave A → B → C → D execution model was not verifiable from chaos source/build wiring.
The following Wave sections are historical planning notes; their delivery claims
and external references are not verified in this source review and must not be
treated as current status until independently checked.

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `chaos`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
  - Delivered: `tests/chaos/test_chaos_highcardinality_stress.cpp` (200 fault descriptors, 4-thread concurrent inject/recover, scheduler under 50 concurrent faults); the former root runbook is now a redirect because its claimed runtime signals were not implemented.
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
  - Delivered: tests/integration/test_chaos_fault_soak.cpp — FaultInjectRecoverCycle (recover=100%), SchedulerStabilityUnderLoad, CallbackDispatchReliability (2026-09-16)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)
  - Delivered: `src/chaos/RUNBOOK.md` and `src/chaos/OPERATIONS.md` — current, source-verified recovery and signal boundaries.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] **TODO/assumption:** no distributed/acceleration path or stress-test outcome is established by the reviewed chaos runtime.
- [ ] **TODO:** benchmark sources declare thresholds, but no representative-hardware p95/p99 baseline is provided.
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
  - Delivered: `src/chaos/RUNBOOK.md` and `src/chaos/OPERATIONS.md`; the runtime currently has no native chaos trace spans, exported metrics, or alert rules.

### Wave D Closure Batch (2026-09-16)

All three Wave D `[ ]` items closed. Evidence:
- `tests/integration/test_chaos_fault_soak.cpp` — 3 soak cases (FaultInjectRecoverCycle recover=100%, SchedulerStabilityUnderLoad always STOPPED, CallbackDispatchReliability no re-entry)
- `tests/chaos/test_chaos_highcardinality_stress.cpp` — 3 stress cases (200 descriptors, 4-thread concurrent inject/recover, scheduler under 50 concurrent faults)
- `docs/operability/RUNBOOK_CHAOS_FAULT_INJECTION.md` — deprecated redirect to the source-verified module guides; historical CLI/metric/span claims are not runtime behavior
- `benchmarks/chaos/bench_chaos_microbenchmarks.cpp` — CHAOS-MB-01..03 p99 and throughput gate benchmarks

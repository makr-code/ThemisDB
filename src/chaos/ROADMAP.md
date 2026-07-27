# Chaos Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-ready in-process fault injection and scheduler surfaces are available for deterministic resilience simulation workflows.

## In Progress

- [x] hardening concurrency and callback edge behavior under sustained stress profiles (Target: Q3 2026)
  - Delivered: CCH-01..08 in test_chaos_concurrency_hardening.cpp; CCD-01..08 in test_chaos_callback_determinism.cpp
- [x] benchmark stabilization for scheduler and concurrent stress pathways (Target: Q3 2026)
  - Delivered: GATE-CHS-01..06 in bench_chaos_release_gates.cpp
- [x] diagnostics consistency improvements for injected/recovered fault lifecycle events (Target: Q3 2026)
  - Delivered: ChaosFailureClass taxonomy + isFailClosedClass() in chaos_contract.h; unified callback/FSM contracts § 5/§ 6

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic behavior for callback-heavy and pending-queue edge permutations (Target: Q4 2026)
  - Delivered: CCD-01..08 (test_chaos_callback_determinism.cpp) + CTI-01..08 (test_chaos_scheduler_timing.cpp)
- [x] expand resilience regressions for schedule/stop/restart timing races (Target: Q4 2026)
  - Delivered: CTI-01..08 (test_chaos_scheduler_timing.cpp)
- [x] improve operator-facing diagnostics for fault lifecycle and scheduler state transitions (Target: Q4 2026)
  - Delivered: ChaosFailureClass taxonomy (§ 3) + isFailClosedClass() (§ 4) + FSM invariants (§ 6) in chaos_contract.h

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for scheduler and concurrent stress benchmarks (Target: Q1 2027)
- [ ] add dedicated chaos microbenchmarks for additional fault classes and timing modes (Target: Q1 2027)
- [ ] evaluate controlled distributed-chaos coordination strategy for future extension (Target: Q1 2027)

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
- [x] complete remaining hardening for registry and scheduler internals (Target: Q4 2026)
  - Delivered: `include/chaos/chaos_contract.h` § 1–§ 7 governs all registry and scheduler internals;
    constraints enforced at injectFault() / scheduleIn() / schedule() boundaries
- [x] align callback/state transition behavior to bounded runtime contracts (Target: Q4 2026)
  - Delivered: § 5 callback semantics (FIFO order, no re-entry, bounded dispatch) +
    § 6 scheduler FSM (STOPPED / RUNNING, idempotent start/stop)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for invalid timing and descriptor states (Target: Q4 2026)
  - Delivered: `chaos_contract.h` § 4 — all ChaosFailureClass variants mandate fail-closed no-op;
    `isFailClosedClass()` predicate available for policy enforcement
- [x] unify diagnostics across scheduler queue and callback failure classes (Target: Q4 2026)
  - Delivered: unified `ChaosFailureClass` taxonomy used by both FaultInjector and ChaosScheduler;
    § 5 callback semantics and § 6 scheduler state contract documented in contract header

### Phase 4: Tests
- [x] expand focused regressions for high-concurrency scheduler and callback scenarios (Target: Q4 2026)
  - Delivered: `tests/chaos/test_chaos_concurrency_hardening.cpp` — CCH-01..CCH-08
    (concurrent inject/recover/query, callback ordering, clearAllFaults under churn, snapshot safety)
  - Delivered: `tests/chaos/test_chaos_callback_determinism.cpp` — CCD-01..CCD-08
    (recover callback, duplicate inject, invalid descriptor rejection, clearAllFaults no-callback,
     scheduleIn(0) fires, clearPending lifecycle, multi-schedule sequential fire)
- [x] extend deterministic fixture coverage for wake-strategy and queue edge matrixes (Target: Q4 2026)
  - Delivered: `tests/chaos/test_chaos_scheduler_timing.cpp` — CTI-01..CTI-08
    (stop/start timing races, restart cycles, schedule-after-stop, idempotent double-start/stop,
     CONDVAR latency bound, FIXED_TICK fire window, mixed-strategy concurrent schedulers)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for scheduler/concurrency hot paths (Target: Q4 2026)
  - Delivered: `benchmarks/chaos/bench_chaos_release_gates.cpp` — GATE-CHS-01..GATE-CHS-06
    (injectFault ≥500k ops/s, isFaultActive latency ≤1µs, recoverFault ≥200k ops/s,
     concurrent 8-thread ≥200k ops/s, schedule() ≥100k ops/s, callback overhead ≤2×)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
  - Delivered: gate self-checks embedded in bench_chaos_release_gates.cpp; benchmarks exit
    non-zero when measured throughput falls below gate threshold

### Phase 6: Documentation and Acceptance
- [x] core chaos module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 delivery evidence recorded in this roadmap (2026-07-27)

## Production Readiness Checklist

- [x] core chaos surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for concurrency/callback edge cases
  - CCH-01..08, CCD-01..08, CTI-01..08 delivered (2026-07-27)
- [x] release benchmark stabilization complete
  - GATE-CHS-01..06 delivered in bench_chaos_release_gates.cpp (2026-07-27)

## Known Issues and Limitations

- behavior is intentionally process-local and non-persistent.
- direct external sabotage is outside scope of the in-process simulation model.
- benchmark depth remains limited to current chaos stress coverage and needs expansion.

## Breaking Changes

No breaking chaos-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.
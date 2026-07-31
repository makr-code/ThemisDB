# benchmarks/failover

Mirrored benchmark folder for `src/failover`.

## Benchmark files

### bench_failover_release_gates.cpp

Phase 5 original hot-path release gates (FRG family).

| Gate ID      | Benchmark | Threshold      |
|--------------|-----------|----------------|
| GATE-FRG-01  | FRG-01    | p99 ≤ 500 µs   |
| GATE-FRG-02  | FRG-02    | p99 ≤ 5 ms     |
| GATE-FRG-03  | FRG-03    | p99 ≤ 200 µs   |
| GATE-FRG-04  | FRG-04    | p99 ≤ 100 µs   |
| GATE-FRG-05  | FRG-05    | p99 ≤ 50 µs    |
| GATE-FRG-06  | FRG-06    | p99 ≤ 1 ms     |

### bench_failover_phase2_phase3_gates.cpp

Phase 5 release gates for the Phase 2/3 hardening delivery (FP23 family).
Covers the six hot paths introduced or hardened in Phase 2 (Core Implementation)
and Phase 3 (Error Handling and Edge Cases).

| Gate ID       | Hot path benchmarked                                     | Threshold    |
|---------------|----------------------------------------------------------|--------------|
| GATE-FP23-01  | `canTransition()` state-machine switch dispatch          | p99 ≤ 100 µs |
| GATE-FP23-02  | `preventSplitBrain()` fail-closed null-fencing-mgr path  | p99 ≤ 200 µs |
| GATE-FP23-03  | `executePlan()` try_to_lock concurrency guard            | p99 ≤ 100 µs |
| GATE-FP23-04  | `attemptRecovery()` batch stats flush (1 lock, 3 incr.)  | p99 ≤ 200 µs |
| GATE-FP23-05  | `emitDiagnostic()` code→event-type dispatch + empty vec  | p99 ≤ 100 µs |
| GATE-FP23-06  | `triggerManualFailover()` full-queue drop + stats incr.  | p99 ≤ 200 µs |

Canonical seed: `kP23CanonicalSeed = 42`. All benchmarks are self-contained
(no I/O, no threads, no external managers).

See `src/failover/ROADMAP.md` Phase 5 for benchmark evidence and gate status.


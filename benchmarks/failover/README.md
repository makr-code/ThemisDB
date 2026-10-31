---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Failover Benchmarks

Benchmark targets for `src/failover`.

`benchmarks/failover/CMakeLists.txt` adds the three Google Benchmark executables
only when `THEMIS_BUILD_BENCHMARKS` is enabled. The IDs and thresholds below are
declared workload targets, not recorded results or evidence that a gate passed.

## Benchmark files

### `bench_failover_release_gates.cpp`

Phase 5 original hot-path release gates (FRG family).
These cases use synthetic/in-memory mocks (for example no heartbeat network send,
no election network, and no epoch-store I/O); they do not measure end-to-end
failover.

| Gate ID      | Benchmark | Threshold      |
|--------------|-----------|----------------|
| GATE-FRG-01  | FRG-01    | p99 ≤ 500 µs   |
| GATE-FRG-02  | FRG-02    | p99 ≤ 5 ms     |
| GATE-FRG-03  | FRG-03    | p99 ≤ 200 µs   |
| GATE-FRG-04  | FRG-04    | p99 ≤ 100 µs   |
| GATE-FRG-05  | FRG-05    | p99 ≤ 50 µs    |
| GATE-FRG-06  | FRG-06    | p99 ≤ 1 ms     |

### `bench_failover_phase2_phase3_gates.cpp`

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

The benchmark uses inline mock logic for several paths and disables external
effects; source registration does not establish a performance result.

### `bench_failover_wave_b_gates.cpp`

Wave B target family FWB-01..08 covers adaptive-interval calculation, simulated
quorum-log serialization/CRC without file I/O, topology snapshots/diffs, GC
grace calculation, a quorum fast path, recovery-stat accounting, and concurrent
queue operations. Several cases are standalone simulations rather than calls
through the full manager. The source declares five repetitions; it does not
include a recorded hardware baseline here.

FWB-02 has inconsistent declared thresholds in its source: the overview says
`p99 ≤ 1 ms` (and `≤ 100 µs` for the in-memory path), while the gate table labels
it `p99 ≤ 100 µs`. None of the three benchmark files configures a p99 percentile
statistic or contains a threshold assertion; these labels are not enforceable
pass/fail gates as currently written.

Run the built targets on representative hardware before claiming latency
compliance. Benchmark source/registration and threshold labels alone are not a
pass/fail result.

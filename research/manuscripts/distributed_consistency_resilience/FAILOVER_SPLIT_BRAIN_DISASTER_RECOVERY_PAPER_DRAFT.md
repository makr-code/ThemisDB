# Failover, Split-Brain Prevention, and Disaster Recovery in an ACID-AI Database

**Status**: REVIEW_CANDIDATE  
**Version**: 0.2  
**Last Updated**: 2026-08-10  
**Target Venue**: VLDB 2027 / ICDE 2027

---

## Metadata

- **Scientific Delta**: Formalize fail-closed split-brain prevention, bounded failover state transitions, and disaster-recovery execution semantics in a database that also hosts AI-adjacent workloads.
- **Canonical Evidence Sources**: `src/failover/ROADMAP.md`, `benchmarks/failover/README.md`, `tests/failover/test_failover_phase2_phase3_focused.cpp`, `tests/failover/test_failover_contract_hardening_focused.cpp`.
- **Required Experiments**: multi-node failover storm evaluation, dependency-degraded quorum evaluation, latency distribution under repeated recovery cycles.
- **Open Risks / Claim Boundaries**: current evidence is strong for focused tests and hot-path benchmarks, but large-scale repeated failover storms are still planned work.
- **Overlap / Successor / Predecessor**: complements sharding and distributed ACID manuscripts; should not duplicate global distributed-system exposition.

## Abstract

This draft targets the resilience layer of ThemisDB's failover subsystem. The paper scope is narrower than a generic HA paper: it focuses on fail-closed split-brain prevention, deterministic state-machine transitions, concurrent disaster-recovery execution guards, and diagnostic surfaces that remain observable under dependency degradation. The repository already contains contract, focused-test, and hot-path benchmark evidence for these behaviors. The remaining gap is large-scale empirical evaluation under repeated or coordinated failover storms. The paper should therefore present a strong design-and-evidence story while keeping scale claims bounded to currently verified behaviour.

## I. Introduction

ThemisDB contains a production failover runtime that must preserve operational safety even when quorum or fencing dependencies degrade. The research gap is not generic failover, but failover that exposes deterministic, repository-verifiable safety contracts while coexisting with ACID and AI-oriented database workflows. This matters because ambiguous recovery behavior quickly invalidates reliability claims elsewhere in the system.

### Contributions

1. A failover state-machine formulation with explicit safe and unsafe transitions.
2. A fail-closed split-brain prevention policy with observable diagnostic emission.
3. A bounded disaster-recovery execution model with concurrency rejection semantics.

## II. Related Work

- failover and leader-election patterns in distributed databases
- fencing and quorum safety mechanisms
- observability and diagnostic surfacing for operator actionability
- novelty delta: repository-grounded linkage between state-machine safety, fail-closed behavior, and benchmarked hot-path costs

## III. System Model / Repository Scope

- subsystem: `src/failover/`
- adjacent dependencies: fencing/quorum managers, event callbacks, recovery plans
- failure model: missing fencing manager, invalid plan, queue saturation, concurrent DR execution, retry exhaustion

## IV. Method / Design

### A. `canTransition()` State Machine

The failover module exposes a finite state machine with explicit transition validity checks. `canTransition(from, to)` encodes all legal transitions as a switch-based table. Illegal transitions (e.g., RECOVERING→ACTIVE without passing through HEALTHY) are rejected immediately without side effects. This enables deterministic formal reasoning about reachability and closed-world safety semantics.

```
State graph (simplified):
  HEALTHY → DETECTING → FAILING_OVER → RECOVERING → HEALTHY
  Any state → FAILED (terminal on unrecoverable error)
  Impossible: FAILED → HEALTHY (requires explicit recovery plan)
```

### B. `preventSplitBrain()` Fail-Closed Policy

When the fencing manager dependency is absent (null pointer), `preventSplitBrain()` does not attempt to resolve quorum ambiguity. It immediately returns a `QUORUM_UNAVAILABLE` error code and emits a diagnostic event. This fail-closed contract guarantees that the system prefers availability sacrifice over potential dual-primary conditions.

The policy is explicit in the contract: no heuristic fallback, no timeout-based promotion. Any caller that ignores the returned error code proceeds at its own risk; the diagnostic emission provides operator-visible evidence of the skipped check.

### C. `executePlan()` Concurrency Guard

Disaster-recovery plan execution is guarded by `std::unique_lock<std::mutex>(plan_mutex_, std::try_to_lock)`. A concurrent invocation that fails to acquire the lock receives an explicit `DR_ALREADY_IN_PROGRESS` error. This prevents interleaved partial-execution states that would corrupt recovery progress. The non-blocking try pattern ensures the rejection is O(1) overhead even under high concurrency.

### D. Batch Statistics and Diagnostic Emission

`attemptRecovery()` accumulates per-attempt statistics (`total_attempts`, `failed_attempts`) using a batch-flush pattern: a single `std::lock_guard` acquisition covers all counter increments for one recovery cycle. This replaces per-counter lock acquisitions and bounds the overhead of observability to a constant per batch.

`emitDiagnostic()` maps `FailoverErrorCode` values to `FailoverEventType` identifiers and iterates the registered callback vector. The zero-subscriber path completes in a single vector size check.

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `src/failover/ROADMAP.md` | Phases 2–6 | state-machine, fail-closed quorum handling, diagnostics, acceptance status | ready |
| E2 | `tests/failover/test_failover_phase2_phase3_focused.cpp` | P23-01..P23-08 | deterministic regression coverage for transitions, split-brain prevention, concurrent DR rejection | ready |
| E3 | `tests/failover/test_failover_contract_hardening_focused.cpp` | FCH-01..FCH-16 | contract and error taxonomy coverage | ready |
| E4 | `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp` | FP23-01..FP23-06 | benchmarked hot-path cost of safety checks and diagnostics | ready |
| E5 | `benchmarks/failover/README.md` | gate table | threshold definitions for failover hot paths | ready |

### E2 Detail — Focused Test Coverage (P23-01..P23-08)

| Test ID | Assertion | Method |
|---|---|---|
| P23-01 | `canTransition()` returns false for all impossible state pairs | state-table exhaustive check |
| P23-02 | `canTransition()` returns true for all valid forward transitions | forward-path enumeration |
| P23-03 | `preventSplitBrain()` fails closed (returns error) when fencing manager is null | null-pointer injection |
| P23-04 | `attemptRecovery()` stats are correct after max retry exhaustion | retry-count validation |
| P23-05 | `triggerManualFailover()` drops requests when queue is full and increments drop-stat | queue saturation |
| P23-06 | `executePlan()` rejects concurrent invocation with explicit error | try_to_lock contention |
| P23-07 | `emitDiagnostic()` fires registered callback for QUORUM_UNAVAILABLE | callback dispatch |
| P23-08 | `attemptRecovery()` batch-flushes stats (total==N, failed==N) | batch-flush correctness |

### E4 Detail — Benchmark Gate Table (FP23-01..FP23-06)

| Gate ID | Benchmark | Operation | Threshold |
|---|---|---|---|
| GATE-FP23-01 | FP23-01 | `canTransition()` state table lookup | p99 ≤ 100 µs |
| GATE-FP23-02 | FP23-02 | `preventSplitBrain()` fail-closed null-fencing path | p99 ≤ 200 µs |
| GATE-FP23-03 | FP23-03 | `executePlan()` concurrency guard (try_to_lock, uncontested) | p99 ≤ 100 µs |
| GATE-FP23-04 | FP23-04 | `attemptRecovery()` batch stats flush | p99 ≤ 200 µs |
| GATE-FP23-05 | FP23-05 | `emitDiagnostic()` error-code dispatch | p99 ≤ 100 µs |
| GATE-FP23-06 | FP23-06 | `triggerManualFailover()` full-queue rejection | p99 ≤ 200 µs |

All six gates are hard release blockers. Regression > 10% vs. baseline blocks promotion.

## VI. Experimental Methodology

### A. Setup
- focused-test baseline already complete (P23-01..P23-08, FCH-01..FCH-16)
- benchmark protocol uses canonical seed 42 and minimum 10 repetitions per gate (per `benchmarks/MEASUREMENT_HYGIENE.md`)
- all benchmark registrations use `UseRealTime()` for I/O-adjacent paths; state-machine paths use CPU time
- next: multi-node chaos harness with repeated recovery cycles, controlled via `research/experiments/distributed_consistency_resilience/`

### B. Workloads
- W1: normal failover state transitions (valid and invalid, covering full transition table)
- W2: dependency-degraded split-brain prevention (fencing manager null-injected)
- W3: concurrent DR execution and queue saturation (goroutine-parallel trigger load)
- W4 (planned): multi-node repeated failover storm with dependency injection

### C. Metrics
- p95/p99 latency for each of the six hot paths (FP23-01..FP23-06 gates)
- recovery completion / rejection counts per workload run
- diagnostic emission completeness (events emitted vs. expected per error code)
- false-unsafe / false-safe transition rate (should be zero in all valid inputs)

## VII. Results

### A. Primary Results
- current repository evidence supports deterministic transition and fail-closed safety claims
- microbenchmark-backed costs are available for six hot paths

### B. Ablations / Sensitivity
- compare with and without fencing dependency
- compare plan validation success vs invalid-plan rejection paths

### C. Negative Results
- large-scale multi-node failover storm evidence is not yet complete

## VIII. Discussion

The strongest paper angle is operational safety under degraded coordination rather than generic availability. The draft should avoid implying validated cluster-scale results that the repository does not yet provide.

### Supported claims
- deterministic transition table and fail-closed split-brain path (`E1`, `E2`, `E4`)
- observable diagnostics and bounded DR concurrency semantics (`E1`, `E2`, `E3`, `E4`)

### Deferred claims
- production-scale resilience under repeated multi-node storms
- end-to-end impact on AI-facing workloads during coordinated recovery

## IX. Reproducibility & Artifact

- focused tests under `tests/failover/`
- benchmarks under `benchmarks/failover/`
- next artifact: experiment protocol in `research/experiments/`

## X. Limitations, Risk, Ethics

- not a proof of full cluster availability under all topologies
- depends on external manager availability and surrounding orchestration quality

## XI. Conclusion

ThemisDB already exposes a repository-grounded failover safety story worth formalizing as its own systems manuscript. The immediate next step is scaled recovery experimentation, not additional design inflation.

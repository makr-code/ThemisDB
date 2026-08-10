# Failover, Split-Brain Prevention, and Disaster Recovery in an ACID-AI Database

**Status**: ACTIVE_DRAFT  
**Version**: 0.1  
**Last Updated**: 2026-08-10  
**Target Venue**: arXiv (cs.DB / cs.DC / cs.SE)

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

- formalize `canTransition()` state table and failure reachability
- describe `preventSplitBrain()` fail-closed path when fencing dependency is absent
- describe `executePlan()` concurrency guard and batch statistics behavior
- define operator-facing diagnostics as part of the safety contract, not an afterthought

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `src/failover/ROADMAP.md` | Phases 2-6 | state-machine, fail-closed quorum handling, diagnostics, acceptance status | ready |
| E2 | `tests/failover/test_failover_phase2_phase3_focused.cpp` | P23-01..P23-08 | deterministic regression coverage for transitions, split-brain prevention, concurrent DR rejection | ready |
| E3 | `tests/failover/test_failover_contract_hardening_focused.cpp` | FCH-01..FCH-16 | contract and error taxonomy coverage | ready |
| E4 | `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp` | FP23-01..FP23-06 | benchmarked hot-path cost of safety checks and diagnostics | ready |
| E5 | `benchmarks/failover/README.md` | gate table | threshold definitions for failover hot paths | ready |

## VI. Experimental Methodology

### A. Setup
- focused-test baseline already exists
- benchmark protocol uses canonical seed 42 and repeated runs
- next step: multi-node chaos harness with repeated recovery cycles

### B. Workloads
- W1: normal failover state transitions
- W2: dependency-degraded split-brain prevention
- W3: concurrent DR execution and queue saturation

### C. Metrics
- p95/p99 latency for failover hot paths
- recovery completion / rejection counts
- diagnostic emission completeness
- false-unsafe / false-safe transition rate

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

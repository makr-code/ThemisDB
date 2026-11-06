---
Author: "ThemisDB Team"
Created: "2026-09-21"
Last Updated: "2026-11-06"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Execution Module Roadmap

## Current Status

The execution module has a bounded deadline scheduler and a bounded fixed-worker
pool. No implementation drift was reported by the issue. The seven governance
documents are present and their stale status claims have been corrected. The
checked-in Doxygen XML is stale relative to the current headers, and no
configured test build is available in this checkout; validation and maintainer
acceptance evidence remain pending. The next implementation work is true work
stealing, elastic worker scaling, and other explicitly planned features.

- [x] Live implementation drift in `src/execution/` and `include/execution/` reviewed against current source and tests (Target: Q3 2026)
- [x] Governance documentation files present and aligned to current behavior (Target: Q4 2026)
- [ ] Regenerate and validate Doxygen XML against the current public headers (Target: Q4 2026)
- [ ] Elastic worker scaling and true steal-path activation implemented (Target: Q1 2027)
- [ ] Request-path server integration advanced beyond construction/teardown wiring (Target: Q1 2027)

## Planned Features

- [x] Restore missing governance docs: `AUDIT.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md`, `MODULE_GAPS.md`, `PERFORMANCE_EXPECTATIONS.md`, `PRODUCTION_REQUIREMENTS.md`, `SECURITY.md` (Target: Q3 2026)
- [x] Replace stale module claims about urgent buckets, fairness reservations, dynamic thread retirement, and live work stealing with source-verified descriptions (Target: Q3 2026)
- [x] Public-header comments describe the current implementation and reserved configuration fields (Target: Q3 2026)
- [ ] Activate per-worker queue population and actual cross-worker steal behavior (Target: Q1 2027)
- [ ] Implement elastic worker growth/shrink semantics or narrow the public naming/contract accordingly (Target: Q1 2027)
- [ ] Add queue-expiry / cancellation policy for stale scheduler entries where required by upstream consumers (Target: Q1 2027)
- [ ] Add structured observability/export hooks for scheduler and thread-pool metrics (Target: Q1 2027)

## v2.5.0 / v3.0.0 Research & Wave Plan

This is a proposed module plan, not an approved release schedule. The version
split follows the project transition described in the root
[`FUTURE_ENHANCEMENTS.md`](../../FUTURE_ENHANCEMENTS.md); no module-specific
dates or release acceptance have been supplied. Evidence and literature
applicability are summarized in [RESEARCH.md](RESEARCH.md).

### Maturity assessment

**Current maturity: bounded, component-level runtime primitives; end-to-end
execution readiness is not yet demonstrated.** The scheduler has bounded
admission and earliest-deadline-first dequeue. The pool has bounded submission,
a fixed worker set, graceful-shutdown logic, and exception accounting. Tests
cover focused scheduler behavior and pool integration. However, the pool does
not perform active stealing or elasticity, the server only constructs and
tears down the primitives, and the stress/benchmark harnesses use local models.
There is no production-runtime performance baseline, cancellation/expiry
contract, or module-owned telemetry export.

### Version objectives

| Target | Objective | Release boundary |
|---|---|---|
| **v2.5.0 — prove and harden the current contract** | Establish production-runtime correctness, performance, overload, and operational evidence for the existing bounded design. | Prefer additive instrumentation and tests. Do not claim active stealing, elastic scaling, deadline expiry, or request-path integration unless implemented and validated. |
| **v3.0.0 — evolve the execution architecture** | Introduce only the runtime policies justified by v2.5 workload evidence: true work stealing, optional elastic worker lifecycle, explicit cancellation/expiry semantics, and approved host integration. | Treat policy/config/API changes as compatibility-sensitive. Require contract review and measured comparison before enabling new behavior by default. |

Effort below is an engineering-week estimate for one engineer, excluding
cross-module review, CI queue time, and release scheduling. It is a planning
range, not a delivery commitment.

### v2.5.0 waves

#### Wave 1 — Contract and production baseline (Priority: High; Estimate: 1–2 engineering weeks)

- **Goal:** Replace model-only performance claims with a reproducible baseline for the production scheduler and pool.
- **Tasks:**
  - [ ] Add or extend focused tests and benchmark cases for production `QueryScheduler` and `WorkStealingThreadPool` paths (Target: v2.5.0 Wave 1).
  - [ ] Record queue-wait, enqueue/dequeue, dispatch/drain latency distributions, throughput, workload shape, build mode, and hardware (Target: v2.5.0 Wave 1).
  - [ ] Verify accepted work is neither lost nor duplicated under concurrent producers (Target: v2.5.0 Wave 1).
- **Dependencies:** Existing scheduler/pool APIs, focused CTest cases, and access to named representative release hardware.
- **Risk:** Environment variance or benchmark overhead can obscure small changes; do not compare different hardware/configurations as equivalent.
- **Success metrics:** Publish repeatable p50/p95/p99 and throughput results for both production primitives; zero lost or duplicate tasks among accepted work in the stress workload; retain workload/configuration metadata with every result.
- **Validation:** Run the three focused execution CTest cases and `test_resource_pooling`; run repeated Release benchmarks and a bounded concurrent-producer stress test; record compiler, hardware, queue capacity, worker count, and build flags.

#### Wave 2 — Overload, lifecycle, and observability contract (Priority: High; Estimate: 2–3 engineering weeks)

- **Goal:** Make bounded admission and shutdown behavior diagnosable without weakening fail-closed resource limits.
- **Tasks:**
  - [ ] Specify stable meanings for capacity timeout, LOW shedding, shutdown rejection, completion reporting, and `waitAll()` (Target: v2.5.0 Wave 2).
  - [ ] Define the host-owned telemetry contract for queue depth, rejection/shed counts, queue wait, task outcome, and worker count (Target: v2.5.0 Wave 2).
  - [ ] Decide whether to add counters/hooks or expose existing snapshots through the integrating service (Target: v2.5.0 Wave 2).
- **Dependencies:** Wave 1 workload and measurement baseline; agreement from the server/operations owners on metric names and ownership.
- **Risk:** New public metrics or hooks can create API/ABI commitments and high-cardinality labels; keep identifiers bounded and review compatibility before implementation.
- **Success metrics:** Every overload/shutdown outcome is distinguishable in tests and observable at the approved integration boundary; queue limits remain finite; no new unbounded retry or task retention path.
- **Validation:** Add focused tests for full queues, timeout and shutdown wakeups, LOW shedding, exception accounting, and metrics consistency; run ThreadSanitizer where supported and review metric labels for bounded cardinality.

#### Wave 3 — v2.5.0 reliability and release gate (Priority: High; Estimate: 1–2 engineering weeks)

- **Goal:** Close correctness and operational evidence gaps while preserving the bounded fixed-worker architecture.
- **Tasks:**
  - [ ] Add production-primitive stress cases for mixed task durations, saturation, exceptions, and repeated shutdown (Target: v2.5.0 Wave 3).
  - [ ] Document caller retry/backpressure requirements and clarify that deadlines do not cancel work (Target: v2.5.0 Wave 3).
  - [ ] Publish benchmark baseline and test configuration; resolve or explicitly defer stale-Doxygen and wiki-context gaps (Target: v2.5.0 Wave 3).
- **Dependencies:** Waves 1–2; Doxygen toolchain and representative CI/hardware evidence.
- **Risk:** Longer stress runs can be flaky or expensive; keep deterministic unit checks separate from soak evidence and document any unavailable platform.
- **Success metrics:** No deadlock, unexpected acceptance after shutdown, lost/duplicate accepted task, or queue-capacity violation in the required stress suite; zero unexplained sanitizer findings; no more than 5% regression against Wave 1 for the same workload/configuration unless a reviewed trade-off is accepted.
- **Validation:** Focused CTest plus stress/soak; AddressSanitizer/ThreadSanitizer where supported; rerun the identical Release benchmark configuration; complete Doxygen and documentation-link checks before the v2.5.0 module sign-off.

### v3.0.0 waves

#### Wave 4 — True work-stealing dispatch (Priority: High; Estimate: 3–5 engineering weeks)

- **Goal:** Replace reserved per-worker queue scaffolding with a production steal path only if Wave 1 workloads show a bottleneck that it can address.
- **Tasks:**
  - [ ] Specify owner/thief deque behavior, task ordering, queue capacity accounting, lock ordering, and shutdown interaction (Target: v3.0.0 Wave 4).
  - [ ] Route and steal production `WorkItem`s (Target: v3.0.0 Wave 4).
  - [ ] Benchmark balanced and skewed task distributions against the central-queue baseline (Target: v3.0.0 Wave 4).
- **Dependencies:** Wave 1 baseline, Wave 2 contract review, and Wave 3 correctness gate.
- **Risk:** Deques add synchronization and ordering complexity; stealing can regress locality, fairness, or tail latency for non-stealable/database operations.
- **Success metrics:** Candidate target, to be confirmed after Wave 1: at least 10% throughput improvement on a documented skewed workload, with no more than 5% p95 regression on the balanced workload; no task loss/duplication or starvation in bounded-duration stress runs.
- **Validation:** Deterministic unit tests for owner-pop/thief-steal/empty races and shutdown; stress tests under ThreadSanitizer; same-hardware repeated benchmarks for both distributions. Do not enable by default if the target is not met.

#### Wave 5 — Explicit lifecycle and cancellation policies (Priority: Medium; Estimate: 2–4 engineering weeks)

- **Goal:** Define safe worker scaling and queued-work expiry/cancellation semantics without confusing deadlines with interruption of running callables.
- **Tasks:**
  - [ ] Decide whether `max_threads` is a runtime target and, if approved, add bounded worker growth/retirement with a documented `idle_timeout_ms` contract (Target: v3.0.0 Wave 5).
  - [ ] Specify cancellation/expiry status and ownership for queued work (Target: v3.0.0 Wave 5).
  - [ ] Keep running-callable interruption out of scope unless a cooperative-cancellation API is separately approved (Target: v3.0.0 Wave 5).
- **Dependencies:** Wave 2 API contract; Wave 4 queue/lifecycle invariants if work stealing is adopted; upstream consumer requirements for cancellation.
- **Risk:** Lifecycle races can strand work or block shutdown; automatic expiry can silently drop valid long-running work if SLA and deadline meanings are conflated.
- **Success metrics:** Active workers never exceed configured `max_threads`; accepted work reaches exactly one terminal state; queued work never exceeds its configured capacity; shutdown terminates within its documented bound when tasks cooperate or the bound is explicitly reported as best-effort.
- **Validation:** State-transition and race tests for grow/shrink, cancellation/expiry, saturation, and shutdown; repeated stress under sanitizers; measure resource use and tail latency against the v2.5 baseline.

#### Wave 6 — Host integration and v3.0.0 acceptance (Priority: High; Estimate: 2–4 engineering weeks)

- **Goal:** Demonstrate value through an explicitly owned request/consumer path and complete release-grade operational evidence.
- **Tasks:**
  - [ ] With server-owner approval, connect a real consumer to admission and dispatch (Target: v3.0.0 Wave 6).
  - [ ] Define retry, overload response, metrics export, and teardown ownership (Target: v3.0.0 Wave 6).
  - [ ] Update public contract, operations/security documentation, and migration notes for incompatible policy changes (Target: v3.0.0 Wave 6).
- **Dependencies:** Waves 1–5 as applicable; server integration design and compatibility review.
- **Risk:** Integration can amplify retries or duplicate execution unless admission outcomes and task ownership are handled end to end; a generic executor may not be suitable for every database operation.
- **Success metrics:** The end-to-end workload meets the service owner's approved latency/throughput and error-budget targets; saturation remains bounded and observable; no duplicate side effects or unbounded retry amplification in fault tests.
- **Validation:** End-to-end mixed-load and overload tests, shutdown/restart and failure-injection tests, security review of task ownership and resource limits, plus same-hardware comparison with the v2.5 baseline and maintainer sign-off.

### Trade-offs, assumptions, and open questions

- The research literature supports testing work stealing and state-aware query scheduling, but its results rely on workload/system assumptions not established for ThemisDB. The cited percentages and theoretical bounds are not module targets or guarantees.
- Work stealing, elasticity, and cancellation increase algorithmic complexity and alter lifecycle/ordering contracts. Prefer the fixed-worker central queue if measured workload gains do not justify those costs.
- The candidate 10%/5% targets in Wave 4 are proposed decision thresholds, not existing release commitments; confirm or revise them against Wave 1 measurement variance before implementation.
- The v2.5.0/v3.0.0 release association is inferred from root enhancement governance; dates, exact scope, integration owner, hardware matrix, and API/ABI policy remain unconfirmed.
- `ai_context/developer_llm_wiki/` has no curated execution-module context in its current index. Its `WIKI_STATUS.json` was generated on 2026-11-02 and reports a large source delta; refresh and verify that compiled context before treating it as current release evidence.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze `QueryScheduler` admission/dequeue/result-reporting API in `include/execution/query_scheduler.h` (Target: Q3 2026)
- [x] Freeze `WorkStealingThreadPool` submission/statistics/shutdown API in `include/execution/thread_pool_manager.h` (Target: Q3 2026)
- [x] Document reserved-but-currently-unused configuration fields (`urgent_window_ms`, `default_sla_ms`) and runtime-semantics gaps for `max_threads` / `idle_timeout_ms` (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] Implement deadline-ordered admission queue with timeout-based backpressure and low-priority shedding in `src/execution/query_scheduler.cpp` (Target: Q3 2026)
- [x] Implement bounded central-dispatch worker pool with graceful shutdown and exception accounting in `src/execution/thread_pool_manager.cpp` (Target: Q3 2026)
- [ ] Activate true work-stealing dispatch from reserved per-worker queues (Target: Q1 2027)

### Phase 3: Error Handling & Edge Cases
- [x] Reject scheduler and thread-pool submissions after shutdown begins (Target: Q3 2026)
- [x] Return explicit failure (`0` / `false`) on queue-capacity timeout and low-priority shedding (Target: Q3 2026)
- [ ] Decide and implement stale-query eviction/cancellation behavior instead of retaining expired entries until dequeue (Target: Q1 2027)

### Phase 4: Tests
- [x] Keep scheduler behavior and metrics coverage in `tests/execution/` (Target: Q3 2026)
- [x] Keep thread-pool behavior coverage in `tests/integration/test_resource_pooling.cpp` (Target: Q3 2026)
- [x] Retain the high-cardinality local-model simulation with its non-production scope explicit (Target: Q4 2026)
- [ ] Add production-runtime capacity stress tests for `QueryScheduler` and `WorkStealingThreadPool` (Target: Q1 2027)
- [ ] Add focused tests for any future true-steal and elastic-scaling implementation before enabling those behaviors (Target: Q1 2027)

### Phase 5: Performance/Hardening
- [x] Maintain dedicated execution benchmark coverage in `benchmarks/execution/bench_execution_dedicated_gates.cpp` (Target: Q3 2026)
- [ ] Establish production-runtime queue and dispatch baselines on representative release hardware; current benchmark artifacts use standalone queue models (Target: Q1 2027)
- [ ] Add benchmark-backed guardrails for any future elastic-scaling or cancellation behavior (Target: Q1 2027)

### Phase 6: Documentation & Acceptance
- [x] Keep the full governance document set for the execution module aligned with source (Target: Q4 2026)
- [x] Align README, architecture, roadmap, and feature narratives to the live implementation (Target: Q4 2026)
- [ ] Regenerate Doxygen XML and resolve any confirmed public-API findings (Target: Q4 2026)
- [ ] Attach a successful focused CTest run and maintainer sign-off for the module-specific acceptance gate (Target: Q4 2026)

## Production Readiness Checklist

- [x] Public API declarations exist for scheduler and thread pool
- [x] Core implementation exists for bounded scheduler and fixed worker pool
- [x] Focused regression coverage exists for scheduler/thread-pool paths
- [x] A dedicated execution benchmark harness exists; it does not measure the production scheduler or pool
- [x] Governance documentation set restored and aligned
- [x] Public headers document current implementation behavior
- [ ] Current Doxygen XML evidence validated against the public headers
- [ ] Focused CTest validation evidence attached; current checkout has no configured test build
- [ ] Maintainer acceptance completed for the current documentation issue

## Known Issues & Limitations

- The scheduler orders by computed deadline; `SLAPriority` is not an independent bucket scheduler.
- `urgent_window_ms` and `default_sla_ms` are compatibility fields only in the current implementation.
- The thread pool does not currently push work into per-thread deques or perform active stealing.
- `max_threads` and `idle_timeout_ms` do not yet provide elastic scaling behavior.
- No built-in cancellation, stale-query eviction, or external metrics exporter exists.

## Breaking Changes

None currently planned. Any future narrowing of the thread-pool contract or removal of unused config fields must be announced here before release.

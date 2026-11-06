---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-06
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Chaos Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

> **Evidence rule:** `[x]` denotes verified implementation or execution evidence,
> not source-file presence. The plans and numeric KPIs below are proposals unless
> explicitly labelled measured evidence. This module is a process-local fault
> state simulator, not infrastructure fault injection or a compliance platform.

## Current Status

`FaultInjector` holds process-local fault state; `ChaosScheduler` queues timed
descriptors and injects them into that registry. Consumers must query the
registry and implement the simulated effect. Neither component changes real
network, disk, process, or cluster behavior, and state is not persistent.

**Verified from source (not runtime measurements):**

- `injectFault()` rejects only an empty target ID and probabilities `< 0` or
  `> 1`; it does not sample `FaultSpec::probability`, and NaN passes those
  comparisons. `schedule()` does not perform equivalent descriptor validation.
- Contract recommendations for duration, tick interval, ID/description length,
  pending queue size, and callback count are not enforced. The scheduler queue
  is unbounded and is scanned linearly on every tick.
- Callbacks are synchronous. Recovery callbacks execute while the injector
  mutex is held; registration is unsynchronized; exceptions are uncaught, and a
  scheduler worker exception can escape. `ChaosFailureClass` /
  `isFailClosedClass()` are declared in the contract but unused by the runtime.
- The runtime exports no metrics, traces, alerts, or persistent audit records.
  Optional server integration is unresolved; it is not a supported or
  production-ready control surface and must not be enabled as an operational
  recommendation.
- Core test sources and CMake registrations exist, including lifecycle,
  callback, concurrency, and scheduler timing cases; **no CI pass is asserted
  here**. `tests/integration/test_chaos_fault_soak.cpp` and
  `test_chaos_highcardinality_stress.cpp` use stub/simulation classes, not the
  live runtime, and are not runtime soak evidence.
- Benchmark sources include release-gate and microbenchmark scenarios, but no
  actual results or enforced thresholds are verified. `bench_chaos_stress` is
  omitted because `benchmarks/CMakeLists.txt` checks for
  `bench_chaos_stress.cpp` at the benchmark root while the source is in
  `benchmarks/chaos/`.

**Measured evidence:** none reviewed for runtime soak, performance, or
representative hardware. Existing values in benchmark source (including 5 µs
p99 and 100k ops/s) are code-declared hypotheses, not measured results,
release gates, or hardware-independent guarantees.

**Local evidence references:** [contract](../../include/chaos/chaos_contract.h),
[public API](../../include/chaos/chaos_framework.h),
[runtime](chaos_framework.cpp);
[chaos test registration](../../tests/chaos/CMakeLists.txt),
[framework tests](../../tests/chaos/test_chaos_framework.cpp),
[callback tests](../../tests/chaos/test_chaos_callback_determinism.cpp),
[concurrency tests](../../tests/chaos/test_chaos_concurrency_hardening.cpp),
[scheduler timing tests](../../tests/chaos/test_chaos_scheduler_timing.cpp),
[stub high-cardinality tests](../../tests/chaos/test_chaos_highcardinality_stress.cpp),
[stub integration soak](../../tests/integration/test_chaos_fault_soak.cpp);
[benchmark CMake](../../benchmarks/CMakeLists.txt),
[stress benchmark](../../benchmarks/chaos/bench_chaos_stress.cpp),
[release gates](../../benchmarks/chaos/bench_chaos_release_gates.cpp),
[microbenchmarks](../../benchmarks/chaos/bench_chaos_microbenchmarks.cpp).
Context: [research](RESEARCH.md), [security](SECURITY.md),
[operations](OPERATIONS.md), [model assumptions](MODELING.md),
[module enhancements](FUTURE_ENHANCEMENTS.md),
[root roadmap](../../ROADMAP.md),
[root enhancements](../../FUTURE_ENHANCEMENTS.md).

## In Progress or Planned Features

All items below are proposed work; release assignment is planning, not a root
release-status assertion. Sequence must remain consistent with root
[`ROADMAP.md`](../../ROADMAP.md) Wave A → B → C → D gates.
Priority and effort figures are provisional planning estimates in engineer-weeks,
not measured estimates or an approved schedule; validate them with maintainers
when scope and dependencies are confirmed.

### v2.5.0 — near/mid-term candidate

#### Wave A — Evidence and usable scenarios
- **Priority:** P1. **Provisional effort:** 2–3 engineer-weeks (planning estimate;
  not measured or approved schedule).
- **Goal:** make the simulator's supported boundary and test scenarios
  repeatable and understandable to module consumers.
- **Tasks:** [ ] fix benchmark source discovery; [ ] run registered live-runtime
  unit/concurrency/timing tests in CI and publish run IDs; [ ] add controlled
  scenario examples whose SUT explicitly observes simulated state, with setup,
  expected state, recovery, and cleanup steps.
- **Dependencies:** maintainers confirm intended v2.5.0 scope; available CI and
  benchmark dependencies; each scenario has a real consumer/test fixture.
- **Risk / trade-off:** examples may be mistaken for real network/disk failure
  tests; keep the process-local boundary prominent and avoid inflating coverage
  counts from stub fixtures.
- **Proposed KPIs:** every published scenario has trigger, observable assertion,
  recovery, cleanup, and failure criterion; all selected live-runtime tests have
  archived CI outcomes; no pending or active simulated faults remain at scenario
  completion.
- **Validation:** build/run the registered focused tests; inspect test sources
  for live `FaultInjector`/`ChaosScheduler` use; execute examples in isolated
  test processes and retain CI logs/artifacts. Source presence alone does not
  satisfy these KPIs.

#### Wave B — Contract enforcement and safe failure behavior
- **Priority:** P1, after Wave A. **Provisional effort:** 4–6 engineer-weeks
  (planning estimate; not measured or approved schedule).
- **Goal:** close safety gaps between the documented contract and runtime.
- **Tasks:** [ ] decide and enforce finite probability validation (including
  NaN); [ ] enforce or explicitly revise recommended descriptor, timing, queue,
  and callback bounds; [ ] reconcile runtime failures with the declared
  `ChaosFailureClass` taxonomy; [ ] define exception, callback, and scheduler
  worker failure semantics; [ ] make callback registration/concurrent use
  semantics safe and specify recovery callback lock behavior.
- **Dependencies:** Wave A source/test baseline; API compatibility and error
  semantics decision; review of lock ordering and callback lifecycle.
- **Risk / trade-off:** rejection limits or callback semantics may break existing
  callers; synchronous callbacks preserve simple ordering but can block hot
  paths. Any contract change needs migration notes.
- **Proposed KPIs:** tests cover every accepted/rejected boundary and each
  failure class; queue and callback counts never exceed the documented bound;
  callback throws cannot terminate the scheduler process; sanitizer-backed
  concurrent registration/dispatch tests pass.
- **Validation:** boundary, exception-injection, deadlock/re-entry, and
  concurrent lifecycle tests against the live implementation; run relevant
  sanitizers and verify no state mutation on rejected input. Values and policies
  must be approved before they become release gates.

#### Wave C — Scalability and performance evidence
- **Priority:** P2, after Waves A/B. **Provisional effort:** 3–5 engineer-weeks
  (planning estimate; not measured or approved schedule).
- **Goal:** determine practical capacity and prevent unbounded work from
  surprising test harnesses.
- **Tasks:** [ ] benchmark registry, callbacks, enqueue/fire, expiry, and
  scheduler queue scan across controlled cardinalities and thread counts; [ ]
  compare alternatives to per-tick full-queue scans if measurements justify
  them; [ ] define reproducible benchmark environment, percentile reporting,
  baseline retention, and threshold enforcement.
- **Dependencies:** Waves A/B; corrected target wiring; representative hardware
  and benchmark CI artifact retention.
- **Risk / trade-off:** changing the queue structure or tick policy may trade
  memory/complexity for latency; absolute thresholds vary by hardware and must
  not be treated as portable guarantees.
- **Proposed KPIs:** record p50/p95/p99, throughput, queue depth, and resource
  use at documented scales; no unbounded queue growth in configured-cap tests;
  approve thresholds only after repeatable baselines and variance review.
- **Validation:** run microbenchmarks and stress benchmarks on named hardware,
  repeat runs, compare against retained baselines, and verify gate tooling
  actually fails on an injected threshold violation. No measured targets are
  established yet.

### v3.0.0 — strategic maturity (not a v2.5.0 commitment)

#### Wave D — Governed operations and architecture boundary
- **Priority:** P2 strategic, after Waves B/C. **Provisional effort:** 4–8
  engineer-weeks (planning estimate; not measured or approved schedule).
- **Goal:** mature scenario governance and operational evidence while retaining
  a clearly bounded simulation boundary.
- **Tasks:** [ ] define scenario ownership, approval, scope/allowlist, expiry,
  abort/rollback, and evidence-retention requirements; [ ] specify harness-owned
  metrics, traces, alerts, and durable audit handoff (the module itself does not
  currently provide them); [ ] evaluate a stable adapter boundary for callers
  that orchestrate isolated test environments; [ ] conduct security, privacy,
  failure-mode, and maintainability reviews before any integration decision.
- **Dependencies:** Wave B safe API and Wave C baselines; root Wave C security
  and Wave D operability sequencing; explicit architecture and product approval.
- **Risk / trade-off:** operational controls and integration adapters add
  dependencies and can create a false impression that this library safely
  controls production systems. Keep destructive/infrastructure operations
  outside this process-local module unless separately designed, authorized, and
  reviewed; the unresolved server option is not a shortcut.
- **Proposed KPIs:** every approved scenario has named owner, target boundary,
  preconditions, stop conditions, recovery/rollback, and retained evidence;
  every operational signal has an identified producing system and test; no
  production-control claim is made from simulator state.
- **Validation:** tabletop failure/abort review, security and architecture
  sign-off, end-to-end harness tests in isolated environments, and audit of
  retained execution artifacts. These are future acceptance criteria, not
  current capabilities or compliance certification.

**Research principles applied as design guidance, not evidence of compliance:**
define a steady-state hypothesis, realistic variables, automation, and a
minimized blast radius ([Principles of Chaos Engineering](https://principlesofchaos.org/));
consider anticipation, withstanding, recovery, and adaptation
([NIST SP 800-160 Vol. 2 Rev. 1](https://doi.org/10.6028/NIST.SP.800-160v2r1));
and define measured steady state plus stop conditions before experiments
([AWS FIS stop conditions](https://docs.aws.amazon.com/fis/latest/userguide/stop-conditions.html)).
These principles do not make this module a compliance/evidence platform.

## Implementation Phases

Phases are the delivery breakdown for the waves above; their status is evidence
status, not a claim that prior historical wave batches passed.

1. **Phase 1 — Evidence and contract:** [~] headers, runtime, tests, and
   benchmark wiring reviewed; execution evidence and contract/runtime alignment
   remain open.
2. **Phase 2 — Core safety:** [ ] enforce approved input/resource limits and
   implement explicit failure semantics (v2.5.0 Wave B).
3. **Phase 3 — Error handling:** [ ] specify callback and worker exception
   behavior, synchronization, and diagnostics (v2.5.0 Wave B).
4. **Phase 4 — Live-runtime tests:** [ ] run and retain CI evidence; replace
   stub-only soak/stress claims with live-runtime coverage where appropriate
   (v2.5.0 Wave A).
5. **Phase 5 — Performance and scale:** [ ] repair benchmark discovery, gather
   representative p95/p99 baselines, then decide measured thresholds
   (v2.5.0 Wave C).
6. **Phase 6 — Governance and acceptance:** [ ] complete scenario controls,
   evidence retention, review, and release handoff only after prior gates
   (strategic v3.0.0 Wave D).

## Production Readiness Checklist

- [x] Process-local simulation boundary and principal validation gaps are
  source-verified and documented.
- [ ] Run selected live-runtime focused tests and retain CI outcomes; do not
  infer pass status from registrations or source.
- [ ] Resolve descriptor, queue, callback, exception, and scheduler-worker
  safety semantics; validate with live-runtime tests and sanitizers.
- [ ] Repair benchmark target discovery; run and retain repeatable
  representative-hardware measurements and verify threshold enforcement.
- [ ] Complete isolated-scenario recovery/abort validation and independent
  security/architecture review. Do not treat this checklist as production
  authorization for the optional server integration.

## Known Issues & Limitations

- Faults are labels/state for consumers to query; no real infrastructure fault
  is enacted, probability is not sampled, and state is neither distributed nor
  persisted.
- NaN probability passes the current range comparisons. Recommended contract
  bounds are not enforced. Callback registration is unsynchronized, recovery
  callbacks run under the injector mutex, callback exceptions are uncaught, and
  scheduler worker exceptions can escape.
- Scheduler queue capacity is unbounded and every tick scans the queue. No
  built-in metrics, traces, alerts, or persistent audit trail exist.
- Benchmark root-path check omits `bench_chaos_stress`; declared thresholds in
  release-gate/microbenchmark sources have not been verified by actual runs.
- The integration soak and high-cardinality stress source files use stubs, so
  they do not prove live-runtime soak, scalability, or recovery behavior.
- Optional server integration remains unresolved and is not suitable to
  recommend for production.

## Breaking Changes

No approved breaking-change plan is evidenced. Validate API compatibility
before enforcing new limits or changing callback/error semantics; document any
contract break with migration notes and the applicable release/changelog entry.
Target issue: [#7408](https://github.com/makr-code/ThemisDB/issues/7408).

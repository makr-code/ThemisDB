---
Author: "ThemisDB Contributors"
Created: "2026-07-27"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source/build-definition review: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production runtime foundations exist for secure module loading, sandboxing, dependency handling, hot reload, registry client integration, and plugin runtime composition.

## In Progress

- [x] hardening of runtime edge behavior across loader/sandbox/reload paths (Target: Q3 2026) — completed 2026-07-27: reload rollback regression suite + sandbox degraded-state tests delivered in test_base_reload_regression.cpp
- [x] base hot-path benchmark target definitions delivered (Target: Q3 2026) — completed 2026-07-27: GATE-BASE-01..06 source/labels in `benchmarks/base/bench_base_hot_paths.cpp`; this does not assert a measured gate pass.
- [x] consistency tightening for dependency and registry failure diagnostics (Target: Q3 2026) — completed 2026-07-27: unified taxonomy delivered in base_error_taxonomy.h with all diagnostic builders

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic error surfaces for sandbox/runtime-degraded states (Target: Q4 2026) — completed 2026-07-27: see tests/base/test_base_future_enhancements.cpp suites 3, 7 (WasmSandboxValidation, SandboxWasmIsolation)
- [x] expand regression coverage for reload/dependency edge permutations (Target: Q4 2026) — completed 2026-07-27: see test_base_future_enhancements.cpp suites 6, 8, 9 (AbiChecker, AdvancedDependency, PluginGraphExtended)
- [x] improve operator-facing diagnostics for module activation and rollback classes (Target: Q4 2026) — completed 2026-07-27: see test_base_future_enhancements.cpp suite 10 (OperatorDiagnosticsTest)

### Mid-term (6-12 months)
- [~] define and measure p95/p99 and throughput envelopes for base benchmark mappings (Target: Q1 2027) — GATE-BASE-07..12 source/labels exist in `benchmarks/base/bench_base_wasm_sandbox.cpp`; representative-hardware measurement/sign-off remains pending.
- [x] reduce proxy-like mappings through additional dedicated base microbenchmarks (Target: Q1 2027) — completed 2026-07-27: dedicated wasm/sandbox/AbiChecker/taxonomy benchmarks in bench_base_wasm_sandbox.cpp
- [x] finalize remaining wasm/runtime backend hardening for production profiles (Target: Q1 2027) — completed 2026-07-27: wasm validation-only mode, fuel-budget, host-function allowlist tests delivered

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze loader/sandbox/reload contract semantics for active major line (Target: Q3 2026) — completed 2026-07-27
- [x] define explicit failure taxonomy for dependency and registry intake paths (Target: Q3 2026) — completed 2026-07-27: see include/themis/base/base_error_taxonomy.h

### Phase 2: Core Implementation
- [x] complete remaining hardening in loader, sandbox, and hot-reload internals (Target: Q4 2026) — completed 2026-07-27
- [x] align runtime and registry behavior to shared bounded execution contracts (Target: Q4 2026) — completed 2026-07-27: taxonomy codes and format builders enforce contract boundaries

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed artifacts and degraded runtime backends (Target: Q4 2026) — completed 2026-07-27: BASE_SANDBOX_DEGRADED and BASE_LOADER_* codes in base_error_taxonomy.h
- [x] unify diagnostics across load-order, compatibility, and rollback failures (Target: Q4 2026) — completed 2026-07-27: resolveDescription() and isKnownCode() provide runtime unification

### Phase 4: Tests
- [x] expand focused regressions for reload rollback and dependency conflict scenarios (Target: Q4 2026) — completed 2026-07-27: tests/base/test_base_reload_regression.cpp (10 test suites, 40+ cases)
- [x] add deterministic test-source coverage for sandbox/runtime and registry permutations (Target: Q4 2026) — suites exist in `tests/base/test_base_future_enhancements.cpp`; module-local test CMake is auto-discovered by `tests/CMakeLists.txt`.

### Phase 5: Performance and Hardening
- [x] define benchmark release-gate cases for base hot paths (Target: Q4 2026) — GATE-BASE-01..06 in `benchmarks/base/bench_base_hot_paths.cpp`; this is source coverage, not measured gate validation.
- [?] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — gate labels are embedded in benchmark source; representative hardware results and comparison evidence remain pending.

### Phase 6: Documentation and Acceptance
- [x] core base module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] future enhancement artefacts cross-referenced in FUTURE_ENHANCEMENTS.md — completed 2026-07-27

## Production Readiness Checklist

- [x] core base runtime surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for runtime and backend edge cases — completed 2026-07-27
- [x] benchmark source/target definitions delivered — `benchmarks/base/bench_base_hot_paths.cpp`; passing target envelopes have not been established here.
- [x] wasm/sandbox and AbiChecker dedicated benchmark source delivered — `benchmarks/base/bench_base_wasm_sandbox.cpp`; representative-hardware results remain pending.
- [x] future enhancement test coverage delivered (short-term + mid-term targets) — completed 2026-07-27: test_base_future_enhancements.cpp

## Known Issues and Limitations

- behavior remains partly capability-dependent on enabled runtime backends/options.
- wasm execution paths require an injected WasmRuntime for execution; test source coverage and module-local CMake registration are present, but the current base C++ test targets could not be configured because the diagnostic CMake run stopped at the missing `fmt` dependency.
- `tests/CMakeLists.txt` discovers module CMake files with `CONFIGURE_DEPENDS` and adds `tests/base/`. Base `test_*.cpp` targets are focused unit tests; the focused soak target has a 7200-second timeout. The helper labels tests with `module:base`, `tier:unit`/`tier:soak`, `kind:focused`, and `focus`; the local file adds `base` and `soak`. Registration does not establish build or test success.
- `src/base/buffer_pool.cpp` and `resource_pool_manager.cpp` are registered in modular `THEMIS_BASE_SOURCES` and the monolithic core source list; focused coverage is in `tests/base/test_resource_pools.cpp`. The sources pass a direct C++ syntax check, but the CMake test target remains unbuilt because configuration is blocked by missing `fmt`.
- GATE-BASE-07..12 thresholds are benchmark labels; CI enforcement requires a benchmark-comparison step. Wave D delivers `benchmarks/baselines/base/wave_d_baselines.json` with defined thresholds; wiring the comparison step into CI is tracked as a post-Wave-D follow-up. p95/p99 values on representative hardware are a prerequisite for closing the benchmark Program-Level Success Criterion (see ROADMAP.md §Program-Level Success Criteria).

## Breaking Changes

No breaking base-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `base`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — completed 2026-09-16: `trace_context.h` integration in `hot_reload_manager.cpp` (ScopedSpan on reloadModule/rollback), `remote_registry_client.cpp` retry_exhausted_count/timeout_count counters + ObservabilityHook firing, test coverage in `tests/base/test_base_wave_d_tracing.cpp`
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — completed 2026-09-16: `tests/base/test_base_soak.cpp` delivers reload/rollback/re-register soak loop + stats monotonicity + concurrent soak; THEMIS_SOAK_ITERATIONS-parameterized; registered under `ctest -L soak`
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — completed 2026-09-16: `src/base/RUNBOOK.md` covers BASE_LOADER_*/BASE_SANDBOX_*/BASE_RELOAD_*/BASE_DEP_*/BASE_REGISTRY_* codes with diagnostics, remediation steps, and Prometheus alert reference

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — completed 2026-09-16: test_base_wave_d_tracing.cpp FailClosedTest suite verifies all BASE_LOADER_*/BASE_SANDBOX_* error paths leave no partial state and emit error-coded spans
- [?] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027) — PENDING HUMAN SIGN-OFF: `benchmarks/baselines/base/wave_d_baselines.json` defines thresholds; measured results must be collected on representative hardware and committed before marking [x]; see wave_d_baselines.json `_status` field
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — completed 2026-09-16: `src/base/RUNBOOK.md` covers all error-code groups with diagnostics, remediation, and Prometheus alert reference; `remediationHint()` on every taxonomy code

## Wave A → D Gap-Closure Execution (2026-08-24)

- [x] **Wave A (critical runtime closure pass)**: transport-scheme and path-sanitization hardening in `remote_registry_client.cpp` completed (batches B/C).
- [x] **Wave B (high-risk resource/lifecycle pass)**: curl RAII cleanup guards and exception-safe cleanup in `remote_registry_client.cpp` completed (batch C).
- [x] **Wave C (concurrency/runtime correctness pass)**: `hot_reload_manager.cpp` stale-slot/null-loader reload/rollback hardening completed (batch D).
- [~] **Wave D (operability + long-duration confidence)**: remaining observability/runbook soak tasks continue under existing Q1 2027 items above.
- [x] **Wave D CLOSED (2026-09-16)**: distributed tracing (ScopedSpan in hot_reload_manager), exporter reliability counters (retry_exhausted_count/timeout_count + ObservabilityHook in remote_registry_client), high-cardinality stress tests, long-duration soak tests, operator runbook, and benchmark baseline file all delivered. Benchmark p95/p99 hardware sign-off pending — see ROADMAP.md Program-Level Success Criteria.

## Proposed v2.5.0 → v3.0.0 Base Evolution Targets

> **Planning proposal only:** these are proposed engineering targets, not release
> commitments, approved scope, or completed work. Release inclusion remains
> subject to issue/milestone planning, implementation evidence, and the manual
> release-manager gates in [`RELEASE_STRATEGY.md`](../../RELEASE_STRATEGY.md).
> Bracketed tasks below are open proposals, not status updates to the historical
> checklist above. Root [`FUTURE_ENHANCEMENTS.md`](../../FUTURE_ENHANCEMENTS.md)
> describes the evidence-first v2.5.0 → v3.0.0 rollout.

### Current maturity assessment (qualitative, source-verified)

Base has **implemented runtime foundations with meaningful unit/regression test
and benchmark-source coverage, but incomplete operational/performance sign-off
and several contract gaps**. Loader, registry, dependency analysis, optional
sandbox/WASM, reload/rollback, and resource-pool APIs exist. Existing tests
include reload/regression, future-enhancement, Wave D, soak, and resource-pool
suites; CMake registration is evidence of discoverability, not that those tests
were built or passed. Benchmark sources and thresholds exist, but
GATE-BASE-01..12 representative-hardware results and human sign-off remain
pending. No measured performance result is asserted here.

Maturity is conditional, not a blanket security/readiness claim: the canonical
loader is [`src/themis/module_loader.cpp`](../themis/module_loader.cpp), not the
deprecated `src/base/module_loader.cpp`; sandboxing is optional and
platform-dependent; HTTP is accepted by the registry with a warning; and the
reload sequence has an availability gap with best-effort restoration.

Supporting module evidence: [architecture](ARCHITECTURE.md),
[research notes](RESEARCH.md), [security notes](SECURITY.md),
[performance expectations](PERFORMANCE_EXPECTATIONS.md),
[future enhancements](FUTURE_ENHANCEMENTS.md), and [API contract](API.md).
These documents describe source-backed behavior and validation definitions;
they do not establish that tests passed or that benchmark targets were measured.

### Gap-to-code map and ordering rationale

| Priority | Evidence-backed gap | Relevant code / existing evidence | Proposed treatment |
|---|---|---|---|
| P0 — release evidence | GATE-BASE-01..12 have definitions and thresholds, but representative-hardware measurements and human sign-off are pending. Thresholds are not results. | [`bench_base_hot_paths.cpp`](../../benchmarks/base/bench_base_hot_paths.cpp), [`bench_base_wasm_sandbox.cpp`](../../benchmarks/base/bench_base_wasm_sandbox.cpp), [`wave_d_baselines.json`](../../benchmarks/baselines/base/wave_d_baselines.json) | Collect reproducible data and sign off before using the gates as performance claims. |
| P1 — lifecycle/security contracts | Reload unloads the old and temporary modules before loading under the canonical key; there is an availability gap and restoration is best-effort. `HotReloadManager::Config::verifySignature` is declared but unused; loader policy controls verification. | [`hot_reload_manager.cpp`](hot_reload_manager.cpp), [`hot_reload_manager.h`](../../include/themis/base/hot_reload_manager.h), [reload regression tests](../../tests/base/test_base_reload_regression.cpp), [`src/themis/module_loader.cpp`](../themis/module_loader.cpp) | Make trust-policy ownership explicit and test reload failure states before considering replacement/rollback design changes. |
| P1 — intake and isolation | Registry accepts both HTTP and HTTPS; sandbox controls depend on configuration/platform support, and WASM execution needs an injected runtime. | [`remote_registry_client.cpp`](remote_registry_client.cpp), [`module_sandbox.cpp`](module_sandbox.cpp), [`wasm_plugin_sandbox.cpp`](wasm_plugin_sandbox.cpp), [`wasm_runtime_injector.cpp`](wasm_runtime_injector.cpp) | Define explicit deployment policy and validate supported/degraded paths; do not claim universal transport protection or isolation. |
| P2 — resource-pool contract | `ResourcePoolManager::Config::leak_timeout` is unused; this class does not implement borrow reclamation or own thread pools. Its existing pool benchmarks are not included in the two base benchmark targets. | [`resource_pool_manager.cpp`](resource_pool_manager.cpp), [`resource_pool_manager.h`](../../include/base/resource_pool_manager.h), [resource-pool tests](../../tests/base/test_resource_pools.cpp) | Decide whether to implement a safe, scoped reclamation contract or remove/deprecate the misleading configuration; establish measurements before optimization claims. |
| Strategic — component boundary | Current WASM adapter/runtime injection is not evidence that the WebAssembly Component Model is implemented. | [`wasm_runtime_injector.h`](../../include/themis/base/wasm_runtime_injector.h), [`wasm_plugin_sandbox.h`](../../include/themis/base/wasm_plugin_sandbox.h) | Evaluate a versioned component/interface boundary behind an opt-in adapter; preserve native and existing runtime compatibility unless evidence supports migration. |

### Proposed Wave 1 — Evidence and v2.5.0 release-readiness

- **Goal:** close the highest-priority evidence gap without changing runtime
  behavior or presenting benchmark definitions as measurements.
- **Proposed tasks:**
  - [ ] Run GATE-BASE-01..12 on named representative hardware; record binary,
    compiler/build configuration, host/OS, run commands, repetitions, results,
    and variance in the baseline artifact (Target: v2.5.0).
  - [ ] Audit benchmark fixtures and metric semantics, especially p99
    collection, failed-load fixture paths, and applicable pool coverage; amend
    definitions where the metric does not measure its stated gate (Target:
    v2.5.0).
  - [ ] Obtain human review/sign-off for every threshold and record any
    unsupported or nonrepresentative platform instead of treating it as a pass
    (Target: v2.5.0).
- **Dependencies:** access to representative hardware and a reproducible
  release-profile build; existing benchmark target definitions; human owner for
  sign-off. No runtime change is a prerequisite.
- **Effort estimate:** 2–4 person-weeks (measurement, benchmark audit, and
  review; hardware scheduling excluded).
- **Risks / trade-offs:** host noise and platform diversity may make a single
  result misleading; strict gates can reject useful changes if thresholds lack
  validated context. Report per-host data and do not silently pool unlike
  machines.
- **Numerical success KPIs (proposed acceptance criteria, not current results):**
  1. 12/12 gates have an attributable measurement record and explicit
     pass/fail/not-applicable disposition.
  2. At least 10 repeated samples per gate on each declared representative
     host, with median, dispersion, and host context recorded.
  3. 100% of thresholds with a human sign-off record; zero gates represented as
     passed solely because a source label or threshold exists.
- **Validation strategy:** execute both benchmark binaries in the documented
  release build; repeat runs and use randomized interleaving where supported to
  expose run-order effects; review variance and machine context using the
  [Google Benchmark guide](https://google.github.io/benchmark/user_guide.html).
  Validate checked-in results against target definitions; confirm normal unit
  and release-critical CI independently. A measurement is not a security test.

### Proposed Wave 2 — v2.5.0 lifecycle, trust-policy, and pool contracts

- **Goal:** resolve concrete correctness/security ambiguity and close
  source-backed contract gaps while retaining current public behavior unless a
  separately reviewed compatibility decision is made.
- **Proposed tasks:**
  - [ ] Define one authoritative signature/trust-policy path for reload
    candidates; either wire `verifySignature` to that policy with explicit
    semantics or deprecate/remove it through the normal compatibility process.
    Add tests proving configured rejection and permissive modes, without
    assuming signature verification is globally enabled (Target: v2.5.0).
  - [ ] Extend reload regression coverage for failures at each reload phase,
    failed best-effort restoration, state callback failures, and concurrent
    observation; specify caller-visible state and recovery guidance. Do not
    claim uninterrupted service unless an implementation and test prove it
    (Target: v2.5.0).
  - [ ] Decide whether `leak_timeout` means supported reclamation or should be
    deprecated/removed; document ownership/lifetime rules and add boundary and
    concurrency tests for the selected contract (Target: v2.5.0).
  - [ ] Define a configurable registry transport policy (including how HTTP is
    handled) and test both policy outcomes; retain explicit documentation that
    TLS/configuration is not universal until callers and defaults establish
    otherwise (Target: v2.5.0).
- **Dependencies:** Wave 1 measurement audit for benchmark-sensitive changes;
  API/compatibility review for public configuration; threat-model and platform
  review for transport/sandbox behavior.
- **Effort estimate:** 6–10 person-weeks (implementation, compatibility
  decisions, tests, and review).
- **Risks / trade-offs:** stricter policy may break existing deployments;
  signature-option changes can create API compatibility issues; resource
  reclamation risks invalidating live borrowers; the unload/reload availability
  gap may remain until a safe replacement design is validated.
- **Numerical success KPIs (proposed):**
  1. 100% of configured reload phases have at least one deterministic
     success/failure regression case, including a failed restoration case.
  2. 100% of public trust/transport configuration options have a test for each
     documented policy outcome; zero claims of universal enforcement.
  3. 100% of `leak_timeout` references either point to tested implemented
     behavior or are explicitly deprecated/removed with a migration note.
  4. Zero newly introduced sanitizer findings in the exercised base unit and
     reload-regression suites.
- **Validation strategy:** focused base unit/integration tests, reload and
  resource-pool concurrency tests, ASan/UBSan and TSan where supported, security
  review of trust/transport failure paths, API compatibility checks, and
  release-profile benchmarks for affected paths. Confirm sandbox/WASM tests
  separately on supported platforms; registration alone is insufficient.

### Proposed Wave 3 — v3.0.0 strategic lifecycle and isolation boundary

- **Goal:** decide and, only if justified, implement a versioned module
  replacement/isolation architecture with explicit compatibility and
  fail-safe behavior.
- **Proposed tasks:**
  - [ ] Specify and prototype reload transition alternatives (including
    availability, ABI constraints, state transfer, rollback, and failure
    containment); compare against the existing unload/reload behavior before
    choosing an approach (Target: v3.0.0).
  - [ ] Evaluate a WebAssembly Component Model adapter with explicit
    component/interface model contracts and a supported runtime matrix; keep it
    experimental until runtime and compatibility tests pass (Target: v3.0.0).
  - [ ] Define staged rollout and automatic stop/rollback criteria for module
    runtime changes; make rollout partial and time-limited, with an evaluation
    period before promotion (Target: v3.0.0).
  - [ ] Produce build provenance for release module artifacts at the selected
    [SLSA v1.1 level](https://slsa.dev/spec/v1.1/levels), subject to release
    pipeline feasibility; state clearly that provenance records how an artifact
    was built and **does not establish that its code is safe** (Target:
    v3.0.0).
- **Dependencies:** Wave 2 trust-policy contract; supported platform/runtime
  inventory; v3 API/ABI and migration policy; release engineering and security
  review. The [WebAssembly Component Model documentation](https://component-model.bytecodealliance.org/)
  is an evaluation reference, not evidence of present implementation.
- **Effort estimate:** 10–18 person-weeks (architecture, prototype, integration,
  compatibility, security review; external runtime/vendor work excluded).
- **Risks / trade-offs:** an atomic/overlap replacement may not be safe for all
  native modules or ABIs; component runtimes add operational dependencies;
  provenance may increase build/release complexity without proving artifact
  behavior; rollout can expose mixed-version compatibility defects.
- **Numerical success KPIs (proposed):**
  1. 100% of supported module transition states and recovery outcomes have
     documented contracts and automated tests before general availability.
  2. 0 unhandled partial-activation states across at least 1,000 injected
     reload-failure iterations per supported test platform.
  3. 100% of v3 release module artifacts in the selected scope have verifiable
     provenance metadata; provenance is never used as a substitute for
     signature/trust or code review.
  4. For any canary-enabled rollout, expose no more than 10% of eligible
     instances initially, evaluate for at least 30 minutes, and promote only
     after predeclared health checks pass; stop/rollback on any critical
     security or correctness regression.
- **Validation strategy:** fault-injection and ABI-compatibility tests, repeated
  reload/rollback stress and soak, platform-specific sandbox/runtime security
  tests, provenance verification, and time-bounded partial-rollout exercises.
  Apply the [Google SRE canarying guidance](https://sre.google/workbook/canarying-releases/)
  for partial rollout with explicit evaluation; limits above are proposed
  acceptance values and require operational review.

### Proposed Wave 4 — v3.0.0 resource and performance maturity

- **Goal:** establish measured resource-pool behavior and only then decide
  whether to implement reclamation, extend coverage, or keep the smaller
  existing contract.
- **Proposed tasks:**
  - [ ] Add workload benchmarks for buffer and connection-slot acquisition,
    release, contention, timeout, and saturation; publish workload and host
    context separately from results (Target: v3.0.0).
  - [ ] Implement the Wave 2 pool-contract decision (safe reclamation or
    explicit no-reclamation contract) and verify shutdown, borrower lifetime,
    and saturation accounting under concurrency (Target: v3.0.0).
  - [ ] Establish regression thresholds only after representative-hardware
    baselines and variance review; integrate comparison into the agreed release
    validation path (Target: v3.0.0).
- **Dependencies:** Wave 1 measurement process; Wave 2 public contract decision;
  representative host access and benchmark CI/release-owner agreement.
- **Effort estimate:** 5–9 person-weeks (benchmarks, design/implementation, and
  validation).
- **Risks / trade-offs:** reclamation policy can conflict with borrower
  ownership; contention results depend on workload and hardware; threshold
  enforcement can create noisy failures if variance and host classes are not
  accounted for.
- **Numerical success KPIs (proposed):**
  1. At least 6 named pool workloads (acquire/release, contention, timeout,
     saturation, shutdown, and buffer-size classes) have benchmark cases.
  2. 100% of documented pool lifecycle and shutdown invariants have automated
     tests; zero leaked or double-released handles in the exercised sanitizer
     runs.
  3. 100% of enforced pool performance thresholds have a representative-hardware
     baseline, variance record, and owner-approved comparison policy.
- **Validation strategy:** focused resource-pool unit and concurrency tests,
  stress/soak with sanitizers, repeated Google Benchmark runs with host and
  variance context, and deterministic baseline comparison. Do not infer pool
  performance from the existing loader/WASM benchmarks.

### Assumptions and knowledge gaps

- Release labels v2.5.0 and v3.0.0 are planning horizons only. The root
  [`RELEASE_STRATEGY.md`](../../RELEASE_STRATEGY.md) identifies `develop` as
  the working branch and releases as manually gated; this proposal does not
  assign dates or promise inclusion.
- Person-week estimates are planning ranges, exclude external-runtime/vendor
  work, and require engineering-owner review.
- This proposal does not establish recent test pass rates, current benchmark
  measurements, production adoption, supported-platform completeness, or
  security certification. Those require fresh CI, representative-hardware, and
  security evidence.
- It is unclear which representative hardware, compiler/build profiles, and
  performance variance policy will be authoritative for base gates; Wave 1
  must resolve these before a pass/fail maturity claim.
- It is unclear whether reload needs zero-downtime replacement for all module
  types, whether the unused signature option should be retained for API
  compatibility, and whether `leak_timeout` has callers relying on its current
  no-op behavior. These are decision items, not assumed requirements.
- The feasibility and selected conformance level for SLSA provenance, a
  Component Model runtime, and canary control-plane integration are unverified.
  Provenance is supply-chain traceability, not proof that code is safe.

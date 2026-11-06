---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Historical validation annotation: 2026-07-28; unresolved evidence is identified in current entries below. -->
<!-- Issues: #5638 (historical status evidence); #7412 (v2.5.0/v3.0.0 research and wave plan) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · MODULE_EVIDENCE.md -->

## Current Status

Core runtime foundations are delivered: `ConcernsContext`, observability/caching/security concern interfaces, runtime concern replacement APIs, production adapters (spdlog, OpenTelemetry-family tracers, Prometheus, Redis cache path), and plugin-based adapter loading via dlopen/LoadLibrary (Issue #1706). Adapter metadata-digest validation and optional SHA-256 plugin-file integrity checks are available; these checks do not authenticate a publisher (see [`TRUST.md`](TRUST.md)).

The module is **feature-capable but not yet evidence-complete for release-level performance, concurrency, and plugin-trust claims**. The scoped benchmark target currently measures index/query/storage paths rather than Core concerns; focused tests exist, but `test_core_highcardinality_stress.cpp` exercises local stand-in types rather than Core implementations. `AdapterRegistry` defaults to `kTrustAll`, and its optional adjacent SHA-256 digest is not publisher authentication. These gaps are the basis for the milestone plan below, not evidence that the underlying paths are failing.

## In Progress

- [~] Produce executable Core concern benchmark and real-implementation stress evidence (Target: v2.5.0)

## Planned Features

- [x] Plugin-based adapter loading (no recompile needed) (Issue: #1706 — Implemented: 2026-07-28)
- [x] Adapter metadata-digest validation and optional plugin-file SHA-256 integrity checking (Implemented: 2026-07-28; not publisher authentication; see `TRUST.md`)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Stabilize current concern interfaces (`ILogger`, `ITracer`, `IMetrics`, `ICache`, `ISecrets`, `IFeatureFlags`, `IAuditLog`) as the active baseline
- [x] Define `AdapterMetadata` / `AdapterValidator` / `AdapterSignature` structs (`include/core/concerns/adapter_metadata.h`) (Implemented: 2026-07-28)
- [x] Define `AdapterRegistry` class with `registerAdapter`, `resolve<T>`, `hotSwap`, `count`, `hasAdapter` (`include/core/concerns/adapter_registry.h`) (Implemented: 2026-07-28)
- [x] Define adapter metadata-digest contract and validation envelope — `AdapterSignature`, `SignedAdapterValidator`, `canonicalString()`, `sha256Hex()` (Implemented: 2026-07-28; not publisher authentication)

### Phase 2: Core Implementation
- [x] Deliver `ConcernsContext` creation and runtime replacement surfaces
- [x] Add `ConcernsContext::resolve<T>()` generic type-safe adapter resolution bridging built-in concern types and `AdapterRegistry` (Implemented: 2026-07-28)
- [x] Implement `AdapterRegistry` non-template methods (`count`, `hasAdapter`, `loadFromPlugin`) in `src/core/concerns/adapter_registry.cpp` (Implemented: 2026-07-28)
- [x] Add runtime plugin loading path that avoids core-module recompilation — `loadFromPlugin()` via dlopen/LoadLibraryA, `plugin_api.h` ABI contract, `THEMIS_DEFINE_PLUGIN_INIT` macro (Implemented: 2026-07-28)

### Phase 3: Error Handling and Edge Cases
- [x] Fail-fast null replacement protection in runtime `replace*` APIs
- [x] `registerAdapter` rejects empty id, null adapter pointers, and `apiVersion < kCurrentApiVersion` with `std::invalid_argument` (Implemented: 2026-07-28)
- [x] `hotSwap` drain timeout emits structured warning to `std::cerr` (Implemented: 2026-07-28)
- [x] `ICircuitBreaker::call(fn, fallback)` inline template — guards fn with `allowRequest()`, records success/failure (Implemented: 2026-07-28)
- [x] Harden plugin load/unload failure semantics (digest mismatch, ABI mismatch, bootstrap errors) — `AdapterTrustPolicy::kRequireSignature`, `.sig` digest-file check, non-zero init return handling (Implemented: 2026-07-28; digest check is not publisher authentication)

### Phase 4: Tests
- [x] Focused module test registration in `tests/core/CMakeLists.txt` (`module_core_<test_stem>_focused`)
- [x] `tests/core/test_adapter_registry_focused.cpp` — 10 tests (AR_01..AR_10) (Implemented: 2026-07-28)
- [x] `tests/core/test_circuit_breaker_focused.cpp` — 8 tests (CB_01..CB_08) (Implemented: 2026-07-28)
- [x] `tests/core/test_concerns_context_focused.cpp` — 18 tests (CCT_01..CCT_18) (Implemented: 2026-07-28)
- [~] Refresh focused build/test execution evidence for current cycle (Target: v2.5.0)

### Phase 5: Performance and Hardening
- [x] `AdapterRegistry::kHotSwapTimeoutMs{100}` SLO constant defined; drain loop uses 1 ms sleep × 100 iterations (Implemented: 2026-07-28)
- [x] `kCurrentApiVersion = 1` constant in `adapter_metadata.h`; registration enforces `apiVersion >= kCurrentApiVersion` (Implemented: 2026-07-28)
- [x] `AdapterSignature` digest record defined in `adapter_metadata.h` and checked by `SignedAdapterValidator` (Implemented: 2026-07-28; not publisher authentication)
- [x] Add adapter metadata-digest validation and plugin-file integrity checking to release hardening flow — `SignedAdapterValidator`, `AdapterTrustPolicy`, `loadFromPlugin` SHA-256 file verification (Implemented: 2026-07-28; not publisher authentication)

### Phase 6: Documentation and Acceptance
- [x] Keep roadmap/future/architecture synchronization explicit and source-traceable
- [x] Update `src/core/ROADMAP.md` with Phase 1–5 checkboxes for implemented items (2026-07-28)
- [x] Update `src/core/MODULE_EVIDENCE.md` with Implementation Coverage section (2026-07-28)
- [~] Keep module evidence and issue closure checklist current for the active status issue (Target: v2.5.0)

## Production Readiness Checklist

- [x] Core concerns architecture documented and source-verifiable (`README.md`, `ARCHITECTURE.md`)
- [x] Runtime replacement APIs and production-mode constraints documented (`ARCHITECTURE.md`, `SECURITY.md`)
- [x] Focused core tests are configured (`tests/core/CMakeLists.txt`)
- [x] Three focused test suites added: `test_concerns_context_focused`, `test_adapter_registry_focused`, `test_circuit_breaker_focused` (2026-07-28)
- [~] Fresh executable-level focused build/test evidence refreshed for this cycle (see `MODULE_EVIDENCE.md`; Target: v2.5.0)
- [x] Plugin runtime loading and digest-integrity hardening completed — Issue #1706 resolved + Q4 2026 hardening delivered (2026-07-28; not publisher authentication)

## Known Issues & Limitations

- Context propagation across async/thread boundaries still requires caller-managed header propagation (`startSpanFromHeaders` / `injectContext`)

## Breaking Changes

- No breaking core-module contract planned in this cycle
- Future publisher-signature enforcement may require trusted signing keys and adapter packaging metadata for external adapters (migration notes required before rollout)

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `core`
- [!] Production-path high-cardinality/concurrent exporter stress and long-duration soak evidence remain open: `test_core_highcardinality_stress.cpp` currently exercises local stubs, not Core adapters (Target: v2.5.0)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- Direct Core p95/p99 benchmarks must be available on representative hardware before Core release sign-off (Target: v2.5.0)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [!] Replace the stub-only Core stress evidence with production-path failure and recovery validation (Target: v2.5.0)
- [!] Core benchmark-backed p95/p99 baselines on representative hardware are not verifiable from the scoped `benchmarks/core/bench_core_performance.cpp`, which has no direct Core concern cases. Root [`ROADMAP.md`](../../ROADMAP.md) schedules a cross-module p95/p99 refresh set for Q4 2026, but does not confirm Core-specific inclusion or acceptance criteria.
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)

## v2.5.0 / v3.0.0 Research and Wave Plan

This plan is a module-level proposal, not a release-date commitment. v2.5.0 is the next minor line identified by [`VERSIONING.md`](../../VERSIONING.md); v3.0.0 is a strategic major-version target with no release date set here. Each wave is ordered by dependency, includes a provisional person-week estimate, and requires `release_critical` CI to remain green on `develop`. Estimates exclude cross-module rollout and external security-review lead time.

### Current Maturity and Evidence-Based Gaps

| Area | Source-backed current state | Gap / opportunity |
|---|---|---|
| Performance and scaling | `ConcernsContext::resolve<T>()` and `AdapterRegistry::resolve<T>()` use shared locks; hot-swap waits up to 100 ms. `benchmarks/core/bench_core_performance.cpp` contains no Core concern cases. | No direct Core latency, throughput, allocation, or memory baseline. Do not infer Core performance from the current benchmark target. |
| Usability and interoperability | Interfaces cover logging, tracing, metrics, caching, secrets, feature flags, audit, and circuit breaking. Context propagation has helper APIs and `ContextPropagation::propagate()`. | Thread-local context does not cross arbitrary boundaries automatically; the caller must propagate headers/context. W3C malformed/absent/versioned-header behavior needs conformance evidence. |
| Security and resilience | Plugin loading has ABI checks and optional adjacent SHA-256 checking. `TRUST.md` documents that the digest is not publisher authentication; registry trust defaults to `kTrustAll`. | No trusted signer identity/key lifecycle, provenance gate, or plugin isolation is established. Existing high-cardinality stress cases use stubs rather than Core adapters. |
| Governance and maintainability | Focused tests are auto-discovered by `tests/core/CMakeLists.txt`; module docs separate API, trust, operations, test evidence, and roadmap. | Registration is not execution evidence. Historical `FUTURE_ENHANCEMENTS.md` v1.x targets and illustrative thresholds must not be treated as current release commitments. |
| Operational readiness | `ConcernsContext` exposes health/readiness, flush, and shutdown; Core runbook material assigns deployment and dependency operation to consuming services. | No Core-specific benchmark artifact, live-backend recovery evidence, or sustained production-path soak result is currently asserted. |

### v2.5.0 — Compatibility-Preserving Hardening

**Priority:** P0 for evidence, trust-boundary and concurrency gaps; P1 for ergonomics.
**Provisional total:** 9–16 engineer-weeks.
**Compatibility:** Prefer additive APIs and configuration. Any production-default change to plugin admission requires an explicit migration note and release-owner approval.

#### Wave 1 — Contract, Benchmark, and Baseline (P0; 2–3 engineer-weeks)
- **Goal:** Replace inferred/borrowed performance claims with reproducible measurements of actual Core paths.
- [ ] Add direct benchmark cases for context construction, built-in and custom `resolve`, closed/open circuit-breaker calls, metric updates/cardinality, cache hit/miss, trace start/end, and adapter swap/drain (Target: v2.5.0).
- [ ] Benchmark no-op/in-process and external-adapter costs separately; publish build revision, hardware, compiler, configuration, workload, repetitions, raw output, throughput, p50/p95/p99, allocation/memory, and error counts (Target: v2.5.0).
- **Dependencies:** Existing `benchmarks/core` registration; benchmark environment and a fixed representative workload.
- **Risk / trade-off:** Microbenchmarks can overstate production wins and are sensitive to host noise; do not set absolute latency SLOs until a baseline is collected.
- **Success metric / validation:** Every listed Core operation has a reproducible baseline and raw artifact; repeat runs report variance and a 95% confidence interval. Compare candidates with identical workloads and report tail latency, not only mean throughput.

#### Wave 2 — Concurrency and Context Propagation (P0; 3–4 engineer-weeks)
- **Goal:** Establish safe, interoperable behavior across concurrent replacement and async/request boundaries.
- [ ] Add tests against real `ConcernsContext`, registry, and production adapter implementations for concurrent resolve/replace, outstanding handles, drain timeout, shutdown, and recovery (Target: v2.5.0).
- [ ] Exercise supported trace boundaries with W3C `traceparent`/`tracestate`: valid, absent, malformed, unsupported-version, and unsampled inputs; verify invalid context does not fail the request (Target: v2.5.0).
- [ ] Run applicable core tests under ThreadSanitizer and capture the exact target/result in `MODULE_EVIDENCE.md` (Target: v2.5.0).
- **Dependencies:** Wave 1 harness and a documented list of supported transport/async boundaries.
- **Risk / trade-off:** ThreadSanitizer and real exporters add runtime and environment constraints; keep those checks in suitable CI tiers without weakening normal `release_critical` gates.
- **Success metric / validation:** No races, deadlocks, dangling handles, lost valid trace-parent links, or request failures from invalid propagation headers across repeated stress runs; record all tested boundaries and sanitizer results.

#### Wave 3 — Plugin Trust and Bounded Observability (P0; 3–5 engineer-weeks)
- **Goal:** Make adapter admission and telemetry resource limits explicit and verifiable.
- [ ] Specify and implement a production trust policy that verifies plugin artifact integrity and an authorized signer identity before loading; keep development/test policy explicit and make rejection reasons observable (Target: v2.5.0).
- [ ] Test absent/altered artifacts, untrusted signer, missing trust metadata, ABI mismatch, and attacker-writable or unexpected plugin paths; reject before library loading where validation permits (Target: v2.5.0).
- [ ] Verify metric cardinality bounds under generated unique values, including memory growth, overflow/drop accounting, and safe handling of user/request identifiers (Target: v2.5.0).
- **Dependencies:** Threat model, key-distribution owner, and agreement on production versus development defaults; see [`TRUST.md`](TRUST.md).
- **Risk / trade-off:** Signing verifies origin/integrity under a configured trust root, not plugin code safety. Changing the permissive default can break existing deployments; retain migration and rollback guidance, and assess process isolation separately.
- **Success metric / validation:** All negative trust cases fail closed before load; only configured signer identities are admitted. Cardinality stays within the configured budget and every rejected/dropped measurement is observable without exposing sensitive labels.

#### Wave 4 — Failure Recovery and Operational Evidence (P1; 1–4 engineer-weeks)
- **Goal:** Validate real adapter behavior during dependency faults and extended operation.
- [ ] Replace stub-only high-cardinality/concurrent-init/exporter stress assertions with tests that exercise Core implementations; inject exporter, cache, logger, and adapter lifecycle failures (Target: v2.5.0).
- [ ] Run a representative 24-hour staging soak with bounded fault injection and documented stop conditions; capture memory, queue/cardinality, errors, p95/p99, and recovery time (Target: v2.5.0).
- [ ] Update `MODULE_EVIDENCE.md` and `RUNBOOK.md` with reproducible commands, observed limits, operator signals, and rollback steps (Target: v2.5.0).
- **Dependencies:** Waves 1–3; staging backend and operator-defined SLO/stop thresholds.
- **Risk / trade-off:** Staging is not proof of every production failure mode; minimize blast radius and do not claim remote-service availability from in-process tests.
- **Success metric / validation:** No sustained resource growth or SLO breach during the agreed soak; fault recovery returns to the pre-injection steady state within the operator-approved recovery objective, with no silent loss.

### v3.0.0 — Versioned Contracts and Strategic Isolation

**Priority:** P1 architecture/security investment.
**Provisional total:** 15–25 engineer-weeks; schedule is deliberately unset.
**Compatibility:** Breaking API/ABI changes are allowed only with a migration guide and the major-version release process in [`VERSIONING.md`](../../VERSIONING.md).

#### Wave 5 — Versioned Concern and Error Contracts (P1; 4–6 engineer-weeks)
- **Goal:** Make adapter compatibility, lifecycle outcomes, and thread-safety contracts uniform and machine-checkable.
- [ ] Define versioned contracts for all public concern interfaces, validator requirements, registration errors, ownership, cancellation/timeout, and shutdown semantics (Target: v3.0.0).
- [ ] Inventory call sites and adapter implementations; provide an automated migration guide/checklist for any breaking contract change (Target: v3.0.0).
- **Dependencies:** v2.5.0 concurrency evidence and API compatibility inventory.
- **Risk / trade-off:** Uniform contracts can expose behavior differences in existing adapters and require coordinated downstream migration.
- **Success metric / validation:** All in-tree adapters pass contract conformance tests; API/ABI compatibility report and migration instructions are reviewed before the v3 freeze.

#### Wave 6 — Plugin Supply-Chain Assurance and Isolation Decision (P1; 4–8 engineer-weeks)
- **Goal:** Establish repeatable artifact provenance and reduce host exposure to third-party native plugins.
- [ ] Integrate signed build provenance and trusted-key rotation/revocation into packaging and admission; record the trust root and verification result in auditable deployment evidence (Target: v3.0.0).
- [ ] Complete a plugin threat-model decision on in-process versus process-isolated execution, including privilege boundaries and failure containment; implement isolation only if the approved threat model requires it (Target: v3.0.0).
- **Dependencies:** v2.5.0 signer policy, release packaging/provenance pipeline, and security review.
- **Risk / trade-off:** Provenance does not establish that code is benign; isolation adds IPC latency, deployment complexity, and a new runtime boundary.
- **Success metric / validation:** Release artifacts have verifiable provenance and signer identity; tampering, revoked/untrusted identities, and ABI incompatibility are denied; the approved isolation model has negative/failure-path tests.

#### Wave 7 — Measured Scaling and Resource Budgets (P1; 3–5 engineer-weeks)
- **Goal:** Optimize demonstrated bottlenecks without speculative lock-free or pooling changes.
- [ ] Use Wave 1 profiles to select only measured contention/allocation bottlenecks; define per-operation throughput, p95/p99, CPU, and memory budgets before implementation (Target: v3.0.0).
- [ ] Re-run identical workloads at representative concurrency/cardinality scales; retain a simple baseline path for comparison and document any trade-off (Target: v3.0.0).
- **Dependencies:** v2.5.0 raw benchmark artifacts and supported-hardware matrix.
- **Risk / trade-off:** More throughput can worsen tail latency, memory, or fairness; reject complexity when the improvement is within measurement noise.
- **Success metric / validation:** For each accepted optimization, repeated-run 95% confidence intervals show at least 10% improvement in its named target metric, with no greater than 5% regression in other predeclared core KPIs. These are proposed gates and must be approved against the Wave 1 baseline.

#### Wave 8 — Release-Grade Resilience and Governance (P1; 4–6 engineer-weeks)
- **Goal:** Make Core lifecycle, trust, and operations evidence repeatable across supported release builds.
- [ ] Extend the staging soak to 72 hours and run controlled fault scenarios with explicit steady-state hypotheses, automated abort conditions, and recovery assertions (Target: v3.0.0).
- [ ] Gate release evidence on focused tests, sanitizer results, benchmark artifacts, plugin provenance checks, threat-model review, API migration docs, and operator runbooks (Target: v3.0.0).
- **Dependencies:** Waves 5–7, supported CI/staging matrix, and release/security sign-off.
- **Risk / trade-off:** A long-running test is expensive and can be flaky; make artifacts attributable to exact revisions and classify infrastructure failures separately from product failures.
- **Success metric / validation:** All release-gate evidence is current for the candidate revision; 72-hour soak has no unresolved SLO breach, unbounded resource trend, or unexplained recovery failure; `release_critical` remains green.

### Cross-Wave KPIs, Risks, and Open Assumptions

- **KPIs:** Core operation throughput and p50/p95/p99; run-to-run variance/confidence interval; allocation and resident-memory trend; configured versus observed metric cardinality; plugin admission/rejection outcomes; race/deadlock count; injected-fault recovery time; sanitizer/test and `release_critical` status.
- **Global risks:** Benchmark environment drift; test doubles mistaken for production evidence; trusted-key ownership and rotation not assigned; production plugin-default changes affecting deployments; v3 contract breaks without downstream migration capacity.
- **Open assumptions / knowledge gaps:** No Core-specific performance baseline or representative hardware matrix is checked in; the exact async/protocol boundaries owned by Core consumers need confirmation; signer/trust-root ownership and plugin isolation requirements need a security decision; v3.0.0 release date is not set. Absolute latency and recovery SLOs require product/operator approval after Wave 1 measurements.
- **Evidence discipline:** Registration/source inspection is not execution evidence. Update [`MODULE_EVIDENCE.md`](MODULE_EVIDENCE.md) only with commands, revision, environment, and observed outcomes; preserve failed or unavailable runs as explicit gaps.

### Research and Standards Basis

The method and security/interoperability rationale are summarized with citations in [`RESEARCH.md`](RESEARCH.md#research-basis-for-v250-and-v300). These references inform proposed tests and gates; they are not claims of Core conformance or measured results.

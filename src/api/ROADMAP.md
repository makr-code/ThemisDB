---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: review | validated: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production API adapter surfaces exist for GraphQL, gRPC, WebSocket, tracing middleware, and OTLP export integration.

## In Progress

- [x] protocol hardening and consistency pass for advanced API transport behaviors (Target: Q3 2026)
  - Evidence: test_api_transport_hardening.cpp (13 tests validating fail-closed behavior, version negotiation, bounded resources)
- [x] benchmark and release-gate consolidation for API transport paths (Target: Q3 2026)
  - Evidence: bench_api_transport.cpp (13 benchmarks covering parsing, serialization, validation, tracing overhead)
- [x] observability and transport reliability alignment under sustained concurrency (Target: Q3 2026)
  - Evidence: test_api_observability.cpp (12 tests validating metrics, bounded queues, thread safety)

## Planned Features

### Short-term (3-6 months)
- [x] complete remaining API surface specification and contract consistency tasks (Target: Q4 2026)
  - Evidence: api_transport_contracts.h, api_error_taxonomy.h, api_transport_policy.h/cpp
- [x] strengthen degraded-mode handling for optional transport features (Target: Q4 2026)
  - Evidence: `tests/api/test_api_degraded_mode.cpp` covers capability unavailability, transient degradation, policy+degraded composition, and concurrent mixed traffic.
- [x] extend integration diagnostics for protocol-level failure classes (Target: Q4 2026)
  - Evidence: `DegradedModeDiagnosticsTest/AllFailureClassesHaveActionableMessages` validates that every failure class produces an ERR_-prefixed actionable message for operator triage

### Mid-term (6-12 months)
- [x] expand direct benchmark coverage for currently proxy-like API goals (Target: Q1 2027)
  - Evidence: `benchmarks/api/bench_api_transport.cpp` + `benchmarks/api/bench_api_release_gates.cpp` + `benchmarks/server/bench_api_endpoints.cpp` cover mock transport helpers, policy gates, and server endpoint loops.
- [x] re-baseline API latency and throughput envelopes for representative load profiles (Target: Q1 2027)
  - Evidence: gate cases GATE-API-01..06 and sustained/mixed loops exist in `benchmarks/api/bench_api_release_gates.cpp`; Wave D soak scenarios are in `tests/api/test_api_wave_d_stress.cpp`. Target-label mapping and measured baseline status remain unverified; see `PERFORMANCE_EXPECTATIONS.md`.
- [x] harden multi-transport operational controls across deployment topologies (Target: Q1 2027)
  - Evidence: fail-closed transport policy middleware, bounded gRPC startup lock reacquisition timeout, and explicit GraphQL WS protocol error responses for malformed/pre-init/unknown message types.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] lock transport-surface contracts for active major line (Target: Q3 2026)
  - Evidence: include/api/api_transport_contracts.h — `ITransportContract`, `TransportContractValidator`, `TransportCapability`, `kSupportedApiVersions`, `kMaxPayloadBytes`, `kMaxPathBytes`
- [x] define explicit failure contracts across GraphQL/gRPC/WebSocket adaptation paths (Target: Q3 2026)
  - Evidence: include/api/api_transport_contracts.h — `TransportFailureClass` enum with 9 canonical failure classes and Doxygen-documented HTTP status mapping

### Phase 2: Core Implementation
- [x] close remaining hardening deltas in protocol-adapter and middleware surfaces (Target: Q4 2026)
  - Evidence: include/api/api_transport_policy.h + src/api/api_transport_policy.cpp — `TransportPolicyMiddleware` enforces 5 policy rules (malformed request, path length, payload limit, version check, Content-Type for mutating methods)
- [x] align gRPC and WebSocket edge behavior with shared API policy contracts (Target: Q4 2026)
  - Evidence: `ITransportContract` interface implemented by `TransportPolicyMiddleware`; `TransportCapability` flags enable per-deployment capability advertising for gRPC and WebSocket adapters

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed payload and unsupported capability states (Target: Q4 2026)
  - Evidence: `TransportPolicyMiddleware::handle()` returns immediately on any policy violation without calling inner handler; validated in test_api_phase4_concurrency.cpp and test_api_degraded_mode.cpp
- [x] unify error taxonomy across transport adapters and middleware paths (Target: Q4 2026)
  - Evidence: include/api/api_error_taxonomy.h — `ApiErrorTaxonomy` with `toErrorCode()`, `toHttpStatus()`, `toMessage()`, `isClientError()` covering all 9 `TransportFailureClass` values

### Phase 4: Tests
- [x] expand focused regressions for high-concurrency and transport-edge scenarios (Target: Q4 2026)
  - Evidence: tests/api/test_api_phase4_concurrency.cpp — 14 tests covering 32×50 concurrency matrix, payload boundary, path length boundary, method×version×payload combination matrix, taxonomy correctness
- [x] extend deterministic integration matrix coverage for protocol combinations (Target: Q4 2026)
  - Evidence: `Phase4MatrixTest/ValidProtocolCombinationsSucceed` — 6 methods × 3 version states × 2 content-type variants = 36 deterministic combinations validated

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for API parsing/execution/serialization hot paths (Target: Q4 2026)
  - Evidence: `benchmarks/api/bench_api_release_gates.cpp` — GATE-API-01..06 with documented target budgets; see `PERFORMANCE_EXPECTATIONS.md` for mapping caveat and baseline-validation limits.
- [x] validate p95/p99 envelopes under representative concurrency profiles (Target: Q4 2026)
  - Evidence: `BM_PolicySustainedGetThroughput`, `BM_PolicySustainedPostThroughput`, `BM_PolicyMixedRequestTypes` benchmarks establishing sequential and mixed-type throughput baselines

### Phase 6: Documentation and Acceptance
- [x] core API docs aligned to source-verifiable behavior
- [x] roadmap/future vs changelog role separation synchronized

## Production Readiness Checklist

- [x] core transport adapter surfaces documented and source-verified
- [x] security and failure handling documented at module level
- [~] benchmark inventory and mapping caveat documented in `PERFORMANCE_EXPECTATIONS.md`; target-label reconciliation remains open
- [x] remaining API hardening items closed (protocol hardening + concurrency tests complete)
- [!] benchmark cases and commented target budgets exist; a passing release-baseline result and semantic target-map reconciliation are not established by this documentation review
- [x] Phase 1: transport-surface contracts locked (api_transport_contracts.h)
- [x] Phase 2: hardening deltas closed — TransportPolicyMiddleware enforces 5 canonical rules
- [x] Phase 3: error taxonomy unified — ApiErrorTaxonomy covers all 9 failure classes
- [x] Phase 4: concurrency/edge regressions expanded (14 tests in test_api_phase4_concurrency.cpp)
- [x] Phase 5: release-gate benchmarks locked (GATE-API-01..06 in bench_api_release_gates.cpp)
- [x] Q4 2026 degraded-mode hardening covered in `tests/api/test_api_degraded_mode.cpp`; per-file test counts are not used as status evidence.

## Known Issues and Limitations

- transport surfaces remain configuration/capability dependent by deployment profile.
- some API surfaces may require feature flags for optional protocol support (WebSocket, gRPC reflection).
- `benchmarks/benchmark_target_mapping.json` currently assigns several API target labels to unrelated cases; do not use those entries as direct release evidence until reconciled.
- remaining medium-severity cleanup is tracked in MODULE_GAPS.md Wave A/B/C backlog; no open critical API transport blocker remains.

## Research Assessment and v2.5.0 / v3.0.0 Wave Plan

**Assessment date:** 2026-11-06. This is a module-level proposal, not a release commitment; root `ROADMAP.md` and release governance remain authoritative.

### Current maturity and evidence-based gaps

**Overall maturity: 3/5 — integrated adapter layer with focused contract and resilience tests; not yet release-qualified by reproducible, representative API performance and security evidence.** The scale is an engineering assessment: 1=concept, 2=partial implementation, 3=integrated and focused-tested, 4=measured and security-validated, 5=operationally qualified.

| Area | Assessment | Repository evidence and gap |
|---|---|---|
| Performance and scale | Partial | `benchmarks/api/bench_api_transport.cpp` and `bench_api_release_gates.cpp` cover helper/policy paths; `benchmarks/server/bench_api_endpoints.cpp` covers in-process endpoint cases. The six GATE-API budgets are source comments, not measured passes. `PERFORMANCE_EXPECTATIONS.md` documents both the unrelated target mappings and lack of an archived baseline result. These do not establish end-to-end capacity or tail latency under offered load. |
| Application scenarios and usability | Integrated, with compatibility gaps | GraphQL, gRPC, WebSocket, and tracing/export surfaces exist. The GraphQL parser documents unsupported inline fragments and directives; `IGRPCBridge` has no concrete implementation. The API boundary does not own HTTP routing, authentication, query execution, or storage, so end-to-end behavior depends on `server`, `auth`, `query`, and CDC integration. |
| Security and resilience | Strong request-validation foundation; controls need verification | Shared transport policy and protocol handlers reject malformed/unsupported input where installed; gRPC TLS startup fails closed, WebSocket CDC checks authorization, and the OTLP queue is bounded. `src/api/SECURITY.md` says production HTTP route installation is unverified; `include/api/SECURITY.md` records uncapped active rate-limiter keys and an unverified `QueryAllowList` call site. Production GraphQL limit/introspection configuration also needs call-site verification. OWASP API4/API5 make resource consumption and function-level authorization explicit API risks; control coverage must be verified at the integrated entry point, not inferred from helper tests. |
| Governance and maintainability | Contracts and focused tests exist; evidence traceability is incomplete | `api_transport_contracts.h` and `api_error_taxonomy.h` define shared contracts, and `tests/api/CMakeLists.txt` registers focused tests. Benchmark labels need reconciliation and the generated Doxygen snapshot is stale (`IMPLEMENTATION_SUMMARY.md`). Optional protocol build flags and the API/server ownership split need to remain visible in validation. |
| Architecture and operations | Clear boundary; deployment-dependent | `ARCHITECTURE.md` keeps transport adaptation separate from server routing and query/storage behavior. Optional gRPC/WebSocket support and telemetry integration vary by build/deployment. OpenTelemetry's HTTP conventions recommend duration and active-request signals; instrumentation must avoid sensitive or unbounded-cardinality attributes. |

The existing roadmap's Wave D section says p95/p99 baselines exist, while the source-oriented `PERFORMANCE_EXPECTATIONS.md` says benchmark runs and target mappings do not establish a passing release baseline. Until a dated, reproducible result artifact is linked, treat the baseline and release-gate status as **[!] unverified**, not as a measured pass.

### Research findings and design implications

- **Tail latency and scalability:** Dean and Barroso, *The Tail at Scale* (Communications of the ACM 56(2), 2013, [doi:10.1145/2408776.2408794](https://doi.org/10.1145/2408776.2408794)), show why latency variability becomes important at service scale. Use offered-load tests and report percentile distributions, throughput, errors, and saturation; sequential microbenchmarks alone cannot support capacity claims. Constant-throughput tools such as [wrk2](https://github.com/giltene/wrk2) account for coordinated omission in latency reporting.
- **Security and resource governance:** [OWASP API Security Top 10 2023, API4](https://api-security.owasp.org/editions/2023/en/0xa4-unrestricted-resource-consumption/) recommends explicit request/data bounds and rate limits; [API5](https://api-security.owasp.org/editions/2023/en/0xa5-broken-function-level-authorization/) emphasizes authorization checks at every function boundary. Therefore test the composed production handler path and bound state growth, not just individual policy helpers.
- **Protocol and operations interoperability:** [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110) is the HTTP semantics baseline for status and response behavior. [OpenTelemetry HTTP metric conventions](https://opentelemetry.io/docs/specs/semconv/http/http-metrics/) define request-duration and active-request signals; follow their semantic conventions while excluding raw query strings, identifiers, and other sensitive/high-cardinality values.

These are engineering standards and systems research, not a claim that the module has completed standards conformance or workload validation. Performance targets below are proposed acceptance gates; capture a representative baseline before using them as release evidence.

### v2.5.0 — compatible hardening and evidence closure

#### Wave 1 — Benchmark integrity and runtime baseline

- **Priority / effort:** P0; 2–3 engineering weeks, excluding dedicated hardware provisioning.
- **Goal:** Make API performance claims reproducible and representative without changing public contracts.
- **Tasks:**
  - [ ] Reconcile API entries in `benchmarks/benchmark_target_mapping.json` with the actual benchmark cases and record the owning test/benchmark source (Target: v2.5.0).
  - [ ] Capture a dated baseline for GATE-API-01..06 and representative GraphQL/HTTP endpoint workloads; report p50/p95/p99, throughput, errors, offered load, and host/build configuration (Target: v2.5.0).
  - [ ] Add or configure a constant-offered-load endpoint test with at least three load levels up to measured saturation; retain raw results and link them from this roadmap (Target: v2.5.0).
- **Dependencies:** `benchmarks/server/bench_api_endpoints.cpp`, benchmark CI, and representative hardware; coordinate endpoint setup with `src/server/`.
- **Risk / trade-off:** Shared or frequency-variable runners can make small deltas noisy; microbenchmark budgets must not be misrepresented as end-to-end SLOs.
- **Success metrics:** Zero API target labels mapped to unrelated cases; all six documented gate budgets have recorded measured outcomes; each load profile records percentile latency, throughput, and error rate. After establishing a stable baseline, flag regressions exceeding 5% in p95 or p99 for the same workload and environment.
- **Validation:** Run API focused tests and benchmark targets; repeat each fixed-load profile at least five times on pinned representative hardware; verify the generator sustains its configured arrival rate and publish the configuration and results.

#### Wave 2 — Bounded admission and authorization assurance

- **Priority / effort:** P0; 3–5 engineering weeks.
- **Goal:** Verify and close high-impact resource-consumption and authorization gaps across the composed API entry paths.
- **Tasks:**
  - [ ] Inventory production HTTP routes and verify where `TransportPolicyMiddleware` and authentication middleware are installed; document any intentional exceptions with their compensating controls (Target: v2.5.0).
  - [ ] Trace `QueryAllowList::enabled_` to production request enforcement; either connect the configured allow-list to the GraphQL execution path with fail-closed tests or remove/rename the misleading state through the compatible API process (Target: v2.5.0).
  - [ ] Trace production GraphQL entry points and verify they use the intended `QueryLimits` profile, including introspection policy; explicitly document any development-only `defaults()` use (Target: v2.5.0).
  - [ ] Add a configurable upper bound and deterministic full-capacity behavior for distinct rate-limiter keys; prove idle-key cleanup does not permit unbounded growth (Target: v2.5.0).
  - [ ] Add negative authorization and resource-limit integration tests for each enabled transport, coordinating HTTP/auth-owned checks with `src/server/` and `src/auth/` (Target: v2.5.0).
- **Dependencies:** Confirmed production call sites, deployment defaults, and agreement from the owners of server/auth integration.
- **Risk / trade-off:** A stricter quota or default deny can affect existing clients; define configuration and migration behavior before enabling enforcement by default.
- **Success metrics:** Every production route has a documented policy/auth disposition; a disallowed query is rejected before resolver invocation; production GraphQL routes use the approved introspection/limit profile; every unauthorized test case is denied and auditable; rate-limiter key state never exceeds its configured capacity under a test with at least 10× that many distinct keys.
- **Validation:** Route-wiring review plus integration tests for middleware bypasses, GraphQL production/default limit profiles, disabled/enabled allow-list behavior, concurrency at capacity, eviction/denial semantics, and authorization failures; run sanitizer-enabled focused tests and a bounded-cardinality stress test.

#### Wave 3 — Operational signals and deployment consistency

- **Priority / effort:** P1; 2–4 engineering weeks.
- **Goal:** Make overload, protocol degradation, and telemetry loss observable without coupling telemetry delivery to database-operation success.
- **Tasks:**
  - [ ] Define ownership and emit/test request-duration and active-request signals for API-owned adapters, using OpenTelemetry HTTP semantic conventions where applicable (Target: v2.5.0).
  - [ ] Ensure exporter queue depth, retries, and dropped-span counts are observable and documented with operator actions; keep metric attributes low-cardinality and exclude payloads, query strings, and client identifiers (Target: v2.5.0).
  - [ ] Validate protocol capability and configuration reporting for enabled/disabled gRPC, WebSocket, and OTLP builds (Target: v2.5.0).
- **Dependencies:** Wave 1 baseline; coordination with `src/server/` and `src/observability/` for listener-owned metrics and existing telemetry conventions.
- **Risk / trade-off:** Added signals consume CPU and storage; avoid raw labels and establish sampling/aggregation rules before broad rollout.
- **Success metrics:** Integration tests observe the documented signals for every enabled transport; no sensitive or per-request identifier is used as a metric attribute; tracing/metrics add no more than 5% p95 latency over the Wave 1 baseline under the same profile.
- **Validation:** Focused observability and degraded-mode tests, mixed-load exporter saturation test, and benchmark comparison with instrumentation disabled/enabled.

### v3.0.0 — explicit cross-transport contract evolution

Any public breaking change below requires consumer inventory, an approved migration path, and synchronized API/changelog documentation. The v3.0.0 target is strategic; it does not authorize an API break by itself.

#### Wave 4 — Versioned contract and GraphQL capability definition

- **Priority / effort:** P1; 4–6 engineering weeks, including consumer migration design.
- **Goal:** Publish an explicit, testable API contract for supported protocol and GraphQL behavior.
- **Tasks:**
  - [ ] Specify version negotiation, capability discovery, canonical error fields, and transport-specific status mapping for HTTP/gRPC/WebSocket (Target: v3.0.0).
  - [ ] Decide the supported GraphQL specification subset; specify inline-fragment and directive semantics before implementation, or keep those constructs explicitly rejected (Target: v3.0.0).
  - [ ] Inventory external uses of deprecated `Parser::error()` and gateway-hook methods and document removal or replacement steps (Target: v3.0.0).
- **Dependencies:** Wave 1 evidence, consumer/call-site inventory, and API design approval.
- **Risk / trade-off:** Expanding syntax or changing error shapes can alter resolver behavior and break generated/client code; defer unsupported semantics rather than ship ambiguous partial support.
- **Success metrics:** Every declared capability and failure class has a contract test across each applicable transport; every deprecated symbol has a verified use inventory and migration disposition; unsupported syntax remains deterministically rejected.
- **Validation:** Contract conformance matrix, parser fixtures for accepted/rejected grammar, compatibility tests against v2 behavior, and reviewed migration notes before removing or changing any public symbol.

#### Wave 5 — Shared policy and lifecycle cleanup

- **Priority / effort:** P1; 3–5 engineering weeks.
- **Goal:** Reduce duplicate transport behavior and retire abstractions that have no verified production use.
- **Tasks:**
  - [ ] Define one transport-neutral admission/error policy contract and map its bounded-input, quota, capability, and failure semantics onto each supported adapter (Target: v3.0.0).
  - [ ] Based on a documented consumer need, either provide a tested concrete `IGRPCBridge` implementation or retire the unused interface through the approved major-version migration (Target: v3.0.0).
  - [ ] Remove deprecated gateway-hook and parser error paths only after Wave 4 call-site review and compatibility approval (Target: v3.0.0).
- **Dependencies:** Wave 2 security behavior and Wave 4 contract approval; gRPC build/runtime availability.
- **Risk / trade-off:** Consolidation can obscure protocol-specific behavior or introduce lock/latency regressions; preserve explicit adapter boundaries and avoid a universal abstraction without consumer evidence.
- **Success metrics:** Shared contract tests pass for 100% of enabled adapters; no removed symbol remains in in-repository call sites; policy and error behavior match across transports for equivalent failure classes.
- **Validation:** Cross-transport integration matrix in gRPC/WebSocket enabled and disabled build profiles; API ABI/API compatibility check; concurrency and performance regression tests against Wave 1 baselines.

#### Wave 6 — Release and compliance qualification

- **Priority / effort:** P1; 3–5 engineering weeks, plus external security and representative-hardware review.
- **Goal:** Qualify the API boundary for the v3 contract with auditable security, performance, and operations evidence.
- **Tasks:**
  - [ ] Maintain a versioned inventory of API-owned public capabilities and link server-owned HTTP routes to their owning contracts and authorization checks (Target: v3.0.0).
  - [ ] Run malformed-input fuzz/property tests and sustained mixed-transport load with overload, cancellation, exporter failure, and recovery cases (Target: v3.0.0).
  - [ ] Publish a release evidence bundle containing test, benchmark, security review, configuration matrix, and runbook results; mark any unrun scenario as an explicit limitation (Target: v3.0.0).
- **Dependencies:** Waves 1–5, release-critical CI on `develop`, representative hardware, and security/operations reviewers.
- **Risk / trade-off:** Cross-module routes, authentication, and release gates are not owned by `api`; sign-off can be blocked by missing server/auth evidence outside this module.
- **Success metrics:** 100% of inventoried public API capabilities have an owner, version, auth boundary, and failure contract; zero unresolved critical/high API-boundary findings at sign-off; all supported build profiles pass focused tests; benchmark results meet the approved v3 budgets with no unexplained regression.
- **Validation:** `release_critical` CI, focused API and cross-module integration suites, sanitizers/fuzz corpus, representative fixed-load benchmark, security review, and operator runbook walkthrough.

### Assumptions, open questions, and residual risks

- No production traffic mix, representative hardware configuration, or dated raw API benchmark result was found in the scoped source/test/benchmark review; Wave 1 must establish these before committing absolute latency or capacity SLOs.
- The roadmap's prior statement that representative p95/p99 baselines exist is not supported by the benchmark source and performance-expectations document reviewed here; a linked result artifact is required to resolve the discrepancy.
- HTTP routing and general authentication are owned outside `api`; production route installation of transport/auth middleware, GraphQL safe-limit configuration, and allow-list enforcement remain to be verified with `server` and `auth`. Optional gRPC/WebSocket support also means build-profile coverage is required.
- The required OpenAPI/API inventory scope and whether `IGRPCBridge` has external consumers are not established by this review. Resolve both with the owning teams before Wave 4/5 contract decisions.
- External references were reviewed on 2026-11-06; standards evolve. Recheck the linked RFC, OWASP, and OpenTelemetry sources at each major contract review.

## Breaking Changes

No breaking API module contract is planned for v2.5.0. The v3.0.0 waves may propose contract changes only after consumer inventory, API design approval, and migration notes; any approved change must be recorded in the changelog before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `api`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — Evidence: tests/api/test_api_wave_d_stress.cpp (WaveDStressTest suite); ERR_OTLP_-prefixed messages in src/api/otlp_exporter.cpp
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — Evidence: WaveDSoakTest::SoakSimulation_100kRequests_NoBoundedResourceLeak and SoakSimulation_ConcurrentMixedLoad_NoLeak in tests/api/test_api_wave_d_stress.cpp
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — Evidence: docs/API_TRANSPORT_RUNBOOK.md (GraphQL, gRPC, WebSocket, OTLP, ERR_ codes, alerting thresholds, escalation path)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — Evidence: retry-with-backoff loop in OtlpExporter::flushBatch drops spans to dropped_count_ (never silently discards); queue overflow drops oldest span with WARN log.
- [!] Representative-hardware p95/p99 baseline evidence is not verified (Target: Q1 2027) — API benchmark sources and soak tests exist, but no dated, reproducible result artifact was found in the reviewed scope; see `PERFORMANCE_EXPECTATIONS.md` and Wave 1.
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — Evidence: docs/API_TRANSPORT_RUNBOOK.md provides diagnosis commands, ERR_-prefixed remediation steps, Prometheus alert rules, and escalation path.

---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Issue: "makr-code/ThemisDB#7448"
---

> Exact model backend not verifiable from final document text alone.

# RPC gRPC Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

`src/rpc_grpc` contains an optional gRPC RPC plugin with lifecycle, TLS/mTLS,
service registration, a bidirectional stream helper, and manual observability
hooks. `THEMIS_PLUGIN_RPC_GRPC` and an available gRPC package are required for
the shared-library target; Protobuf is required after gRPC is found.

**Maturity assessment:** implementation is present, with focused unit and
conditional loopback integration test sources, but the inspected evidence does
not include a current live-server benchmark baseline or release-profile test
results. Treat the module as implemented but not release-validated; this is a
qualitative assessment, not a measured maturity score or a project-wide release
status.

The in-tree benchmarks are contract microbenchmarks and in-process simulations,
not end-to-end server measurements. This roadmap does not claim that a
performance threshold has passed.

## In Progress

- [~] Establish live-server benchmark scenarios and a representative-hardware
  baseline before setting absolute throughput or latency promises (Target:
  v2.5.0 Wave 1).
- [~] Specify runtime error, timeout, cancellation, and supported-configuration
  behavior before implementation changes (Target: v2.5.0 Wave 2).

## Planned Features

- [ ] Complete the v2.5.0 hardening waves and attach run-specific validation
  evidence (Target: v2.5.0).
- [ ] Complete the v3.0.0 contract and scale waves only after v2.5.0 evidence
  identifies the remaining architectural limits (Target: v3.0.0).

## Research-Based Gap Analysis

The assessment is based on the code/test evidence and primary technical
references in [`RESEARCH.md`](RESEARCH.md). In summary:

- The registered module benchmarks cover contract operations and in-process
  stubs, not live RPC latency, throughput, TLS, or server saturation.
- Generic timeout, connection, rate-limit, compression, and worker-count
  configuration fields are declared upstream but not applied by this plugin.
- Stream writes are synchronous; finish does not cancel an in-flight read/write,
  and shutdown has no module-configurable drain deadline.
- The optional admin listener uses insecure credentials. Metrics are explicitly
  recorded, expose cumulative latency rather than a histogram, and are not
  exported by a scrape endpoint or automatically wired interceptor.
- These are source observations, not proof of an incident or a failing
  production workload. No hardware baseline, customer workload profile,
  compliance target, or operator SLO was supplied.

## Release Plan and Wave Steps

The efforts below are rough engineering person-week estimates, excluding
external gRPC package/runner provisioning and release approval. Targets are
proposals until module owners approve representative workloads and security
policy. A benchmark declaration or passing stub-only test is not runtime
evidence.

### v2.5.0 — measurable runtime hardening

#### Wave 1: Live performance and capacity baseline (Priority P0; 2–3 person-weeks)

- **Goal:** Replace contract/stub-only performance claims with reproducible
  measurements of the actual plugin listener.
- **Tasks:**
  - [ ] Add live loopback/client-server scenarios for unary request, bidirectional
    stream, TLS/mTLS startup/reload, and graceful shutdown using small, medium,
    and near-configured-limit payloads at increasing concurrency.
  - [ ] Record p50/p95/p99 latency, throughput, errors, CPU/RSS, client/server
    hardware, gRPC/Protobuf versions, build options, payload, concurrency, and
    raw output; retain the exact source revision with each result.
  - [ ] Run at least five repetitions per scenario on a named representative
    host; investigate repeat-to-repeat variance above 5% before treating a run
    as a baseline.
- **Dependencies:** gRPC-enabled Release build and a reproducible client/host
  environment; this wave does not depend on a runtime API change.
- **Risks / trade-offs:** loopback tests can be client- or host-limited and do
  not represent a production network. Keep client capacity and server hardware
  distinct where practical; publish the environment rather than extrapolating.
- **Success metrics and validation:** 100% of the agreed scenarios have five
  attributable raw runs and percentile/throughput/error summaries. Establish a
  per-host baseline first; subsequent release gate proposal is no more than 10%
  p95 or p99 regression at the same workload with no unexplained nominal-load
  errors. This is a proposed comparison rule, not a current performance claim.
  Validate with the benchmark-enabled build and the live integration suite.

#### Wave 2: Explicit runtime contract and bounded operations (Priority P0; 3–5 person-weeks)

- **Goal:** Make configuration, lifecycle errors, deadlines, cancellation, and
  stream backpressure predictable to plugin users.
- **Tasks:**
  - [ ] Define which shared `RPCServerConfig` fields are supported; implement
    the selected fields or reject unsupported non-default settings rather than
    silently implying they take effect.
  - [ ] Specify a programmatic registration/lifecycle error result and align it
    with `RpcGrpcError` only after a compatibility review of
    `IRPCServer`/plugin callers.
  - [ ] Define stream queue capacity, zero-capacity validation, write failure
    propagation, cancellation/deadline semantics, and service lifetime
    responsibilities; implement only the agreed contract.
  - [ ] Add a configurable graceful-stop deadline and deterministic behavior
    when in-flight calls exceed it.
- **Dependencies:** Wave 1 workload parameters; review of
  `include/plugins/rpc_plugin_interface.h` and external plugin callers before
  choosing any signature change.
- **Risks / trade-offs:** changing shared interfaces can break plugins; bounded
  shutdown may terminate long-running calls. Prefer additive configuration and
  an explicitly versioned contract over compatibility shims.
- **Success metrics and validation:** every supported configuration value has a
  positive and invalid-input test; unsupported values are rejected or
  documented as unsupported. Cancellation and shutdown tests complete within
  the configured deadline plus 100 ms on the designated test host; no test
  leaves an unbounded waiter. Run focused unit and real loopback integration
  tests, including a slow-reader and blocked-call case.

#### Wave 3: Secure observability and resilience gate (Priority P1; 3–5 person-weeks)

- **Goal:** Remove ambiguous insecure exposure and provide production-usable,
  bounded request telemetry and credential/lifecycle evidence.
- **Tasks:**
  - [ ] Disable the insecure `admin_port` listener by default and either remove
    it or require an explicitly designed, authenticated and encrypted
    alternative; do not silently fall back to plaintext.
  - [ ] Define certificate/key/CA validation and supported TLS policy against
    current project policy and supported gRPC dependencies; test malformed,
    mismatched, expired, and rejected credentials.
  - [ ] Add an automatically wired instrumentation path with RPC duration
    histograms and bounded attributes aligned with the selected OpenTelemetry
    semantic-convention version; specify compatibility for current metric text.
  - [ ] Exercise credential reload/rollback, readiness transitions, stream
    cancellation, and resource exhaustion during sustained concurrent traffic.
- **Dependencies:** Wave 2 lifecycle/error semantics; approved TLS and
  observability policy; Wave 1 baseline for load parameters.
- **Risks / trade-offs:** instrumentation adds hot-path overhead and changes
  metric consumers; secure removal or redesign of `admin_port` may require
  deployment migration.
- **Success metrics and validation:** default release configuration opens no
  plaintext admin listener; all credential-negative tests fail closed; every
  accepted request path emits one duration observation with bounded labels;
  no unbounded per-request values become metric dimensions. Measure
  instrumentation overhead against Wave 1 and keep p99 regression within the
  agreed 10% budget. Validate with security tests, loopback TLS/mTLS integration,
  concurrency stress, and a refreshed benchmark run.

### v3.0.0 — versioned architecture and scale evolution

#### Wave 4: Transport architecture decision (Priority P1; 2–4 person-weeks)

- **Goal:** Select or retain the synchronous runtime based on measured
  application workloads, rather than adopting async solely as a presumed
  optimization.
- **Tasks:**
  - [ ] Compare the current synchronous server with a bounded async/callback
    prototype for representative unary, streaming, and mixed workloads.
  - [ ] Measure tail latency, throughput, CPU/RSS, thread count, cancellation,
    and overload behavior at the Wave 1 concurrency and payload points.
  - [ ] Record a decision with compatibility, maintainability, and operational
    costs; retain synchronous operation if the alternative does not meet the
    approved performance and reliability gates.
- **Dependencies:** v2.5.0 Waves 1–3, representative workload owner, and
  supported gRPC version matrix.
- **Risks / trade-offs:** an async rewrite raises lifecycle, thread-safety, and
  debugging complexity; benchmark gains on a synthetic test may not transfer
  to service workloads.
- **Success metrics and validation:** same-host repeated comparison has at least
  five runs per scenario; selected design is no more than 5% worse in p99 or
  throughput than the incumbent on any release-critical workload and improves
  at least one approved scaling objective, or the documented decision is to
  retain the incumbent. Validate through integration, cancellation, load, and
  soak tests for the selected design.

#### Wave 5: Versioned RPC plugin contract (Priority P1; 4–6 person-weeks)

- **Goal:** Resolve divergence between descriptive contract types and runtime
  behavior and prepare a deliberate major-version migration.
- **Tasks:**
  - [ ] Define versioned server configuration, typed errors, registration
    outcomes, timeout/cancellation, ownership, readiness, and TLS contracts.
  - [ ] Map every public/runtime contract invariant to an implementation point
    and focused test; remove unused declarations or wire them into runtime
    behavior rather than claiming enforcement.
  - [ ] Publish a compatibility matrix for plugin interface, gRPC/Protobuf
    versions, and supported deployment platforms, with migration instructions
    for every breaking change.
- **Dependencies:** Wave 2 error and lifecycle decisions and the Wave 4
  architecture decision; API and plugin-consumer review.
- **Risks / trade-offs:** correcting the contract can be source/ABI breaking;
  preserving both old and new interfaces is not assumed and requires explicit
  human approval if proposed.
- **Success metrics and validation:** 100% of normative contract requirements
  map to runtime tests; all supported consumers compile against the v3 contract;
  each intentional incompatibility has a migration test/example and changelog
  entry. Validate with compatibility builds, focused contract tests, and
  supported-platform CI.

#### Wave 6: Multi-tenant capacity and operational proof (Priority P2; 4–8 person-weeks)

- **Goal:** Bound resource use and demonstrate stable operations under
  representative sustained and failure workloads.
- **Tasks:**
  - [ ] Based on confirmed service needs, define admission control, per-service
    concurrency/resource limits, overload status, and safe recovery behavior.
  - [ ] Add tenant/service isolation tests, sustained load and restart/reload
    fault injection, and operator guidance for readiness, saturation, and
    certificate rotation.
  - [ ] Retain release evidence for security review, dependency/support matrix,
    benchmark baselines, test runs, and runbook drills; record compliance
    evidence only for explicitly selected frameworks.
- **Dependencies:** v3.0.0 Waves 4–5, a representative multi-tenant workload,
  deployment/operations owners, and approved retention/security requirements.
- **Risks / trade-offs:** quotas can reject legitimate bursts and impose
  per-request overhead; a long soak on one host is not proof of distributed
  failure recovery or regulatory compliance.
- **Success metrics and validation:** under the approved workload, configured
  resource bounds are never exceeded, no tenant bypasses another's limits, and
  RSS growth over the final four hours of a 24-hour soak is at most 5% with no
  unexplained errors. Validate with adversarial quota tests, failure injection,
  security review, benchmark comparison, runbook exercise, and exact
  release-profile CTest results. The 5% soak criterion is a proposed gate to
  validate on designated hardware, not an observed result.

## Cross-Wave Trade-offs and Open Assumptions

- **Latency versus safety:** smaller deadlines and queues bound resource use but
  may reject legitimate long-running work; derive defaults from real workloads.
- **Telemetry versus overhead/privacy:** histogram and trace dimensions help
  operators, but high-cardinality or sensitive attributes create cost and
  disclosure risks. Allowlist dimensions and avoid payload/caller secrets.
- **Availability versus credential changes:** current TLS reload restarts the
  listener and can fail rollback; do not promise zero downtime without a
  separately validated rotation design.
- **Compatibility versus contract clarity:** v2.5.0 should prefer nonbreaking
  behavior where feasible; v3.0.0 is the point for an explicitly reviewed
  major-contract change, not automatic permission for legacy fallbacks.
- **Unknowns:** issue #7448 supplies no named deployments, traffic distributions,
  regulatory target, supported platform matrix, or SLO. Owners must confirm
  these before treating the proposed numeric gates or effort estimates as
  commitments.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Define the shared RPC plugin interface and contract header
  (`include/plugins/rpc_plugin_interface.h`,
  `include/rpc_grpc/rpc_grpc_api_contract.h`).
- [x] Document runtime versus contract-only types and configuration behavior
  (`API.md`, `IMPLEMENTATION.md`).

### Phase 2: Core Implementation
- [x] Implement the optional gRPC shared-library target and plugin lifecycle
  (`src/rpc_grpc/CMakeLists.txt`, `grpc_plugin.cpp`).
- [x] Implement listener setup, credentials, service registration, and shutdown.

### Phase 3: Error Handling and Edge Cases
- [x] Validate required host and TLS file inputs; avoid insecure fallback when
  TLS configuration fails.
- [x] Implement TLS reload rollback attempt and explicit handling for null,
  late, and duplicate service registration.
- [ ] Decide whether lifecycle/registration errors need a programmatic result
  channel instead of diagnostic-only reporting (Target: Q1 2027).

### Phase 4: Tests
- [x] Add focused contract and circuit-breaker test sources under
  `tests/rpc_grpc`.
- [x] Add lifecycle/integration and test-local stress cases; gRPC-dependent
  coverage remains conditional on the configured build.
- [ ] Record passing test results for the exact release configuration before
  using them as release evidence (Target: ongoing).

### Phase 5: Performance and Hardening
- [x] Register contract microbenchmarks and dedicated in-process benchmark
  scenarios (`benchmarks/rpc_grpc`).
- [ ] Add live plugin workloads, measured p95/p99 results, and automated
  baseline comparison; source labels alone are not validation evidence
  (Target: Q1 2027).

### Phase 6: Documentation and Acceptance
- [x] Document API, lifecycle, configuration, and test/benchmark boundaries.
- [~] Regenerate the Doxygen reference from current source when Doxygen is
  available; the checked-in generated page is an older snapshot
  (Target: next documentation refresh).

## Production Readiness Checklist

- [x] Runtime build conditions and the plugin/API boundary are documented.
- [x] TLS, insecure admin listener, registration, and reload limitations are
  documented.
- [x] Focused test and benchmark source locations are documented.
- [ ] Current test results and measured performance baselines are attached to a
  specific revision before release claims.

## Known Issues & Limitations

- `registerService()` returns `void`; rejection is reported diagnostically.
- The runtime does not consume all types or invariants declared by
  `rpc_grpc_api_contract.h`.
- TLS reload drains and restarts the listener, causing downtime; rollback can
  also fail.
- A valid `admin_port` binds a second listener with insecure credentials.
- `BidiStreamAdapter` does not interrupt an in-flight gRPC read/write on finish;
  zero queue depth can leave a writer waiting until finish.
- Metrics are manually recorded and are not exposed through a scrape endpoint.
- Module benchmark sources do not establish live gRPC latency or passed
  p95/p99 thresholds.

## Breaking Changes

No breaking change is proposed by this documentation update. Any future
contract change must include compatibility analysis, migration notes, tests,
and a changelog entry.

## Program Context

This module contributes to the repository-level roadmap; program release gates
and branch requirements remain defined by [`../../ROADMAP.md`](../../ROADMAP.md).
This page records module behavior and does not assert current CI or release
status.

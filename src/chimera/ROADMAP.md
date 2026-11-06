---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-11-06
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera module roadmap

**Purpose:** evidence-based plan for stabilizing the CHIMERA adapter integration for v2.5.0, then deciding and delivering strategic contract and scale improvements for v3.0.0. These are proposed targets, not release commitments or evidence of completed work.

## Current Status

**Assessment date:** 2026-11-06. **Maturity:** source-defined, integration-unvalidated; no numeric maturity score is supportable from the evidence reviewed.

CHIMERA has a broad adapter surface with potential use in portable relational, document, vector, and graph access, plus asynchronous, streaming, prepared-statement, retry, batch, and transaction-context interfaces. The ThemisDB adapter and local MongoDB, Neo4j, and Qdrant adapters are present in the source tree. Engine dispatch and vendor paths are conditional, however, and source presence does not establish that a given adapter works against a live backend.

| Area | Source-backed observation | Current implication |
|---|---|---|
| Build and architecture | `THEMIS_BUILD_CHIMERA` defaults to `OFF`. `cmake/ChimeraAdapters.cmake` mixes local ThemisDB code with sources in `external/chimera`; it does not compile every local implementation. | No default-build or production-support claim. Establish the exact supported dependency revision and adapter/driver matrix first. |
| Contract and usability | `include/chimera/database_adapter.hpp` can select an external contract or a local fallback shim. The ThemisDB adapter exposes multiple model interfaces, but engine-specific dispatch is conditional. Vendor adapters may return `NOT_IMPLEMENTED`. | The effective API and feature set depend on build configuration. Callers need an accurate per-adapter capability/support description. |
| Resilience | `RetryExecutor::should_retry` does not inspect the actual error code: a non-empty `retry_on_codes` set returns true for any failed result, and category flags are only coarse gates. Backoff is capped and jittered, but the wrapper does not establish a retry deadline or operation idempotency. | Incorrect retry policy can repeat permanent failures or duplicate non-idempotent operations. No production retry guarantee is established. |
| Transactions | `TransactionContext` records state, operations, and savepoints locally. Its implementation does not coordinate backend commits or provide distributed atomicity. | Describe it as local transaction metadata unless a separately designed backend protocol is implemented and validated. |
| Tests | Focused adapter, stream, prepared-statement, MongoDB, and stub-based stress test sources exist. Root CMake registers selected tests only when CHIMERA is enabled; local helper and root source lists need reconciliation. No configure/build/test run is evidenced here. | A checked-in test is not a passing or registered test. Track the generated target inventory and results per supported configuration. |
| Performance | The C++ adapter benchmarks use a default in-process ThemisDB adapter; the dedicated-gate cases use local stubs. The Python runner and example CSV are not live adapter measurements. The benchmark schema and runner workload shapes differ. | Existing results cannot establish live-backend throughput, tail latency, or scalability. Establish a reproducible baseline before setting absolute SLOs. |
| Security and operations | Connection authentication, TLS, credentials, dependency versions, and failure behavior vary by compiled driver. Credential masking in an adapter is not a complete secret-handling control. | Security posture must be reviewed per enabled backend and deployment configuration; the default-off build is not itself a security control. |

This assessment is based on `src/chimera/{ARCHITECTURE,MODULE_EVIDENCE,MODULE_GAPS,SECURITY,PERFORMANCE_EXPECTATIONS}.md`, `include/chimera/*.hpp`, `src/chimera/*.cpp`, `CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/*`, and `benchmarks/chimera/*`. These source-near documents are the module evidence; their references to issue #7336 are historical, not validation of the current roadmap.

## Research and technical basis

| Finding | Application to CHIMERA |
|---|---|
| YCSB uses configurable workloads to compare data-serving systems; benchmark results depend on workload shape and reported performance measures, not just an API microbenchmark. | Define representative reads/writes, payloads, concurrency, and backend configuration; report throughput and latency distributions (including p95/p99) for live adapters. Do not compare stub timings with a production backend. |
| AWS Well-Architected REL05-BP03 recommends limiting retries, exponential backoff with jitter, timeouts, retrying only when appropriate, and verifying idempotency. | Classify concrete errors and operation retry-safety; bound attempts and elapsed time; test permanent, transient, and exhausted-retry cases. |
| The Sagas model uses compensating actions for long-lived distributed work; compensation is not equivalent to a single ACID transaction. | Do not imply that local transaction bookkeeping coordinates multiple databases. Only add cross-adapter saga semantics if a product use case requires it, and document compensation limitations. |
| NIST SSDF and the OWASP Secrets Management guidance treat security as a lifecycle concern, including secure development, access control, and secret handling. | Include dependency provenance, least-privilege credentials, TLS/auth configuration, log redaction, and security tests in each supported-driver release gate. |

References:

- Cooper et al., “Benchmarking Cloud Serving Systems with YCSB,” SoCC 2010, [DOI: 10.1145/1807128.1807152](https://doi.org/10.1145/1807128.1807152).
- AWS Well-Architected, [REL05-BP03 Control and limit retry calls](https://docs.aws.amazon.com/wellarchitected/2023-04-10/framework/rel_mitigate_interaction_failure_limit_retries.html).
- Garcia-Molina and Salem, “Sagas,” SIGMOD 1987, [DOI: 10.1145/38713.38742](https://doi.org/10.1145/38713.38742).
- NIST, [SP 800-218 Secure Software Development Framework](https://csrc.nist.gov/pubs/sp/800/218/final).
- OWASP, [Secrets Management Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Secrets_Management_Cheat_Sheet.html).

## Planned Features

### v2.5.0 — stabilization and evidence closure

**Priority:** P0, correctness and supportability before breadth. Do not claim a new backend is supported until its build, contract, integration, and security checks pass.

- [ ] Reconcile the enabled CMake source/test/benchmark wiring with the files actually present locally and in the selected external CHIMERA checkout; record the supported build/driver matrix. (Target: v2.5.0)
- [ ] Make retry decisions from the concrete result error and explicit retry policy; define operation idempotency, attempt limits, timeout/deadline behavior, and final-error behavior. (Target: v2.5.0)
- [ ] State the actual transaction contract and test its local state transitions; do not describe transaction metadata as cross-database atomicity. (Target: v2.5.0)
- [ ] Review each initially supported adapter configuration for authentication, TLS, secret redaction, dependency provenance, unsupported operations, and fail-closed behavior. (Target: v2.5.0)
- [ ] Replace or clearly separate synthetic benchmark cases from live-adapter measurements; reconcile benchmark schema with executed workload definitions. (Target: v2.5.0)
- [ ] Publish per-configuration build, focused-test, security, and benchmark evidence before making adapter support or performance claims. (Target: v2.5.0)

### v3.0.0 — explicit contracts and scalable interoperability

**Priority:** P1, contingent on v2.5.0 evidence and product confirmation of supported use cases.

- [ ] Define a versioned CHIMERA-facing compatibility contract, including external-header/version selection and a migration policy for any intentional API or ABI break. (Target: v3.0.0)
- [ ] Provide capability discovery that distinguishes native, emulated, and unsupported operations for every supported adapter; preserve explicit errors rather than silently degrading semantics. (Target: v3.0.0)
- [ ] Decide whether multi-adapter transactions are required. If required, design and test a durable compensation/saga contract; otherwise keep cross-adapter atomicity explicitly unsupported. (Target: v3.0.0)
- [ ] Specify common timeout, cancellation, backpressure, retry-budget, and observability semantics without hiding backend-specific limitations. (Target: v3.0.0)
- [ ] Validate scale and failure behavior using reproducible mixed workloads and representative backend deployments; set release thresholds from measured baselines. (Target: v3.0.0)

## Wave Plan

Effort is an initial engineering estimate in person-weeks, excluding vendor access lead time and release-manager scheduling. Waves are gated in order; an unsupported backend can be removed from the declared support matrix rather than given an unverified pass.

### v2.5.0 waves

#### Wave 1 — source, contract, and build matrix (P0; 1–2 person-weeks)

- **Goal:** one reproducible definition of what is built and supported.
- **Tasks:** pin or record the external CHIMERA revision; select supported adapter/driver combinations; reconcile `cmake/ChimeraAdapters.cmake`, root `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, and benchmark registration; label conditional and fallback paths.
- **Dependencies:** product/maintainer decision on required backends and access to their development dependencies.
- **Risk / trade-off:** restricting the initial matrix may reduce apparent breadth, but avoids shipping unsupported adapters.
- **Success metrics:** 100% of source paths in enabled targets exist or are explicitly external/conditional; every declared supported configuration completes a clean configure and build; every intended focused test appears in the generated test inventory.
- **Validation:** clean CMake configure/build for each supported option/driver combination; inspect targets and run all discovered focused tests. Save exact configure options, compiler, dependency revision, and results.

#### Wave 2 — retry and transaction semantics (P0; 2–3 person-weeks)

- **Goal:** make failure behavior bounded, explainable, and safe for the operation being retried.
- **Tasks:** classify concrete error codes; define retryable classes and idempotency requirements; enforce maximum attempts and elapsed-time bounds; make local transaction-state semantics explicit; add tests for success, transient/permanent failures, retry exhaustion, savepoints, and terminal states.
- **Dependencies:** Wave 1 support matrix and agreement on the public retry/transaction contract.
- **Risk / trade-off:** narrower retry defaults may surface errors sooner; broader retry rules risk duplicate writes and retry storms.
- **Success metrics:** 100% of declared retry categories have positive and negative tests; permanent and unsupported errors are never retried; attempts never exceed policy; transaction states never claim backend coordination without an implemented backend contract.
- **Validation:** deterministic unit tests with controlled results and timing; fault injection for transient failures; run focused tests under thread/concurrency sanitizers where supported.

#### Wave 3 — adapter security and integration (P0; 2–4 person-weeks)

- **Goal:** establish a minimum secure integration baseline for each supported driver.
- **Tasks:** review TLS/auth configuration and dependency versions; verify secret redaction across errors/logging; document least-privilege connection requirements; test unavailable-driver and unsupported-operation behavior; add live integration tests for the selected backend(s).
- **Dependencies:** Waves 1–2; secure test credentials and a disposable backend environment.
- **Risk / trade-off:** live integration environments increase CI cost and secret-management burden; avoid widening support before these controls are in place.
- **Success metrics:** every declared driver has documented auth/TLS settings and a passing integration test; test/log output contains no seeded credential values; missing dependencies and unsupported operations fail explicitly.
- **Validation:** static dependency/security review, redaction regression tests, no-credential default test, and integration tests against the exact supported driver build.

#### Wave 4 — production-relevant benchmark and v2.5.0 gate (P0; 2–3 person-weeks)

- **Goal:** replace source-only or synthetic performance claims with reproducible adapter evidence.
- **Tasks:** align benchmark schema and runner; benchmark representative live read/write/batch/search operations; control hardware, data size, warm-up, concurrency, and build configuration; capture throughput, p50/p95/p99 latency, errors/retries, CPU, and memory.
- **Dependencies:** Waves 1–3 and stable backend test infrastructure.
- **Risk / trade-off:** noisy shared runners can create false regressions; use a fixed benchmark environment and publish variance rather than overfit to one run.
- **Success metrics:** at least five measured repetitions per declared workload with complete environment metadata; 100% of supported adapters have a baseline; after baseline review, investigate any p95 or p99 regression greater than 10% against the same environment. This is a proposed guard, not a current SLO.
- **Validation:** execute the same versioned workload against baseline and candidate builds; archive raw results and summary; keep stub-only cases separately labeled and excluded from production gates.

### v3.0.0 waves

#### Wave 5 — versioned capability contract (P1; 2–3 person-weeks)

- **Goal:** let callers reason consistently about adapter behavior without flattening backend differences.
- **Tasks:** specify public compatibility/version rules; define capability status and operation/error semantics; add conformance tests for every declared adapter; document migration for deliberate breaking changes.
- **Dependencies:** v2.5.0 support matrix and evidence; human approval of any public API/ABI break.
- **Risk / trade-off:** a richer capability model increases contract complexity; prefer explicit unsupported states over false cross-backend parity.
- **Success metrics:** 100% of declared capabilities are exercised by conformance tests for every supported adapter; no capability is reported as available where invocation returns an undocumented unsupported result.
- **Validation:** compile and run the same contract suite per adapter configuration; review generated API docs and migration guidance.

#### Wave 6 — multi-adapter workflow semantics (P1; 3–5 person-weeks, only if required)

- **Goal:** resolve whether cross-adapter workflows need compensation or should remain unsupported.
- **Tasks:** document a supported use case and failure model; if approved, design durable saga state, idempotent steps, compensation, and recovery; otherwise make unsupported cross-adapter atomicity explicit in the API/docs.
- **Dependencies:** product decision; Wave 5 contract; durable storage and recovery design if saga support is selected.
- **Risk / trade-off:** compensation cannot guarantee rollback of irreversible external side effects; adding this feature is not equivalent to distributed ACID.
- **Success metrics:** every workflow step has a tested retry/compensation rule; fault injection at each step results in a documented terminal state with no silent partial-success result. If no use case is approved, success is an explicit unsupported contract, not implementation.
- **Validation:** state-machine and crash/restart tests, duplicate-delivery tests, and multi-backend integration tests using disposable services.

#### Wave 7 — bounded scale and operations (P1; 3–5 person-weeks)

- **Goal:** make adapter performance and degraded behavior observable under concurrent load.
- **Tasks:** implement only contract-approved timeout/cancellation/backpressure behavior; export consistent operation latency, error, retry, and saturation telemetry; test connection and service failure recovery.
- **Dependencies:** Waves 5–6 decisions and representative backend/CI capacity.
- **Risk / trade-off:** telemetry and queues add overhead and may expose sensitive labels; keep metric cardinality bounded and exclude credentials/query text.
- **Success metrics:** soak tests report throughput, p95/p99, error/retry rate, and resource use; cancellation/deadline tests finish within configured bounds; telemetry contains no secrets or unbounded per-request labels.
- **Validation:** sustained mixed-load tests, backend interruption/recovery tests, resource-limit tests, and telemetry/secret-redaction checks.

#### Wave 8 — v3.0.0 compatibility and release evidence (P1; 1–2 person-weeks)

- **Goal:** deliver a supportable, auditable module contract for the v3.0.0 release candidate.
- **Tasks:** freeze the supported matrix; run the full adapter conformance, integration, security, and benchmark suite; publish upgrade notes and known limitations; obtain maintainer sign-off.
- **Dependencies:** Waves 5–7 and project-level release gates; root `ROADMAP.md` and `RELEASE_STRATEGY.md` remain authoritative for release status.
- **Risk / trade-off:** release sign-off can be delayed by external driver or representative-hardware availability.
- **Success metrics:** all declared configurations build and pass required suites; no unresolved release-blocking security/correctness findings; every performance claim links to a reproducible run; migration documentation covers every approved break.
- **Validation:** clean release-candidate matrix, retained CI artifacts, security review, benchmark comparison, and documented human approval.

## Implementation Phases

These phases are execution checkpoints, not claims that work is underway.

### Phase 1 — Design / API contract
- [ ] Approve the supported adapter/driver matrix and record the external CHIMERA revision. (Target: v2.5.0)
- [ ] Define retry classification, idempotency, local transaction semantics, and explicit unsupported behavior. (Target: v2.5.0)
- [ ] Decide whether v3.0.0 requires capability-contract or multi-adapter transaction changes. (Target: v3.0.0)

### Phase 2 — Core implementation
- [ ] Reconcile CMake source and test registrations with the approved support matrix. (Target: v2.5.0)
- [ ] Implement the approved retry/error behavior and capability reporting without unapproved compatibility fallbacks. (Target: v2.5.0 / v3.0.0)

### Phase 3 — Error handling and edge cases
- [ ] Cover unavailable drivers, invalid configuration, permanent/transient failures, retry exhaustion, timeout, cancellation, and partial operation outcomes. (Target: v2.5.0)
- [ ] Specify compensation and recovery edge cases before enabling any cross-adapter workflow. (Target: v3.0.0)

### Phase 4 — Tests
- [ ] Build and run all focused and integration tests for every supported adapter/driver combination. (Target: v2.5.0)
- [ ] Run shared contract/conformance and multi-backend fault-injection tests for the v3.0.0 contract. (Target: v3.0.0)

### Phase 5 — Performance and hardening
- [ ] Capture reproducible live-adapter baselines and security evidence; separate synthetic benchmarks. (Target: v2.5.0)
- [ ] Validate scale, degraded operation, bounded telemetry, and release thresholds on representative infrastructure. (Target: v3.0.0)

### Phase 6 — Documentation and acceptance
- [ ] Keep module support, limitations, and validation evidence aligned with the selected build matrix. (Target: v2.5.0)
- [ ] Publish v3.0.0 migration notes and obtain human release approval; project release status remains governed by root documents. (Target: v3.0.0)

## Risks and trade-offs

| Risk | Impact | Mitigation / decision rule |
|---|---|---|
| External CHIMERA contract or revision differs from the fallback shim. | Build/API incompatibility and false portability claims. | Pin the revision, compile both supported header-selection paths if both are supported, and treat them as separate configurations. |
| Retrying writes without idempotency can duplicate side effects. | Data correctness and cascading-load risk. | Retry only explicitly classified transient errors and retry-safe operations within bounded attempts/deadlines. |
| The common interface hides backend-specific semantics. | Callers may assume unsupported guarantees. | Capability discovery plus explicit `NOT_IMPLEMENTED`/unsupported results; do not claim parity based on shared method names. |
| Transaction terminology implies atomicity not present in the implementation. | Partial writes may be misrepresented as rolled back. | Keep current context metadata separate from backend transaction guarantees; require a product decision before saga work. |
| Benchmark environment or synthetic workloads mislead optimization decisions. | Invalid SLOs and regressions missed in production. | Benchmark live operations on fixed hardware, publish workload/configuration and distributions, and keep stubs out of release gates. |
| More vendor drivers expand attack surface and maintenance cost. | Dependency, credential, TLS, and CI burden grows. | Support a backend only after dependency, security, integration, and lifecycle ownership are documented. |

## Success KPIs

- **Build support:** 100% of declared configurations configure and build from a clean directory; every target source exists in the selected checkout.
- **Test evidence:** 100% of declared adapter configurations have discovered and passing focused/contract tests; no unsupported operation is silently reported as successful.
- **Retry correctness:** all retryable and non-retryable error classes are covered; attempt and time budgets are never exceeded; no permanent error is retried.
- **Security:** all supported drivers have documented authentication/TLS/dependency setup; zero seeded secrets appear in test logs or benchmark artifacts.
- **Performance:** five or more repetitions per workload on fixed hardware; report throughput, p50/p95/p99, errors/retries, CPU, and memory. Use the proposed >10% p95/p99 regression investigation guard only after establishing and reviewing a baseline.
- **Governance:** every release support/performance claim links to its build, test, security, or benchmark evidence; module claims do not override root release governance.

## Production Readiness Checklist

- [ ] Select and record the external CHIMERA revision and supported backend/driver combinations. (Target: v2.5.0)
- [ ] Resolve CMake source-list and focused-test registration discrepancies; prove every declared configuration builds. (Target: v2.5.0)
- [ ] Verify concrete retry classification, idempotency rules, and bounded time/attempt behavior. (Target: v2.5.0)
- [ ] Document transaction-context behavior separately from backend atomicity. (Target: v2.5.0)
- [ ] Complete per-driver security, secret-redaction, dependency, and integration checks. (Target: v2.5.0)
- [ ] Establish live-adapter benchmark baselines and reconcile workload/schema definitions. (Target: v2.5.0)
- [ ] Approve versioned capability semantics and migration policy for v3.0.0. (Target: v3.0.0)
- [ ] Validate v3.0.0 failure recovery, scale, and operational telemetry on representative infrastructure. (Target: v3.0.0)
- [ ] Obtain human module and release-gate sign-off; do not infer it from this roadmap. (Target: v3.0.0)

## Known Issues & Limitations

- No build, test, security review, or benchmark was run for this roadmap update.
- `THEMIS_BUILD_CHIMERA` is off by default; enabling it relies on external source and driver configuration.
- Root test CMake contains project-local source references not present in the current `src/chimera/` inventory (including adapter-factory and several vendor adapter filenames). Configure/build impact is not verified.
- Existing benchmark source includes in-process adapter and stub cases; neither establishes live-backend performance. The Python runner, schema, and demo CSV are not a live adapter baseline.
- Retry classification is not based on the actual error code, and transaction context is not distributed transaction coordination.
- The Developer LLM Wiki `WIKI_STATUS.json` was generated 2026-11-02 and reports a large source delta; it is a freshness signal, not current Chimera validation evidence.
- Product-required backend set, workload/SLO profile, external dependency lifecycle/license, supported deployment environments, and representative CI hardware remain open assumptions.
- Version labels v2.5.0 and v3.0.0 are planning scopes from issue #7409, not confirmed milestone dates or release commitments.

## Breaking Changes

No v2.5.0 public API break is proposed by this roadmap. A v3.0.0 contract or capability redesign may require API/ABI changes; scope and migration policy must be approved and documented before implementation. Do not add a compatibility fallback or legacy path without explicit human approval.

## References

- Module sources: `include/chimera/*.hpp`, `src/chimera/*.cpp`.
- Build and tests: `CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `tests/chimera/*.cpp`.
- Benchmarks: `benchmarks/CMakeLists.txt`, `benchmarks/chimera/*.cpp`, `benchmarks/chimera/benchmark_config_schema.yaml`, `external/chimera/run_ci_benchmarks.py`.
- Related module evidence: [`ARCHITECTURE.md`](ARCHITECTURE.md), [`MODULE_EVIDENCE.md`](MODULE_EVIDENCE.md), [`MODULE_GAPS.md`](MODULE_GAPS.md), [`SECURITY.md`](SECURITY.md), [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md).
- Governance: [`../../ROADMAP.md`](../../ROADMAP.md), [`../../FUTURE_ENHANCEMENTS.md`](../../FUTURE_ENHANCEMENTS.md), [`../../RELEASE_STRATEGY.md`](../../RELEASE_STRATEGY.md), [`../../DOCUMENTATION_GOVERNANCE.md`](../../DOCUMENTATION_GOVERNANCE.md).

Issue/milestone: #7409 / v2.5.0 and v3.0.0 planning; milestone dates unconfirmed.

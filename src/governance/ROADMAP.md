---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-11-06"
Status: "review"
Source Level: "level1"
SOT Domain: "module-behavior"
Canonical References: "src/governance sources and include/governance headers; tests/governance/CMakeLists.txt; benchmarks/governance/CMakeLists.txt; root ROADMAP.md"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issues #7349, #7422"
---

Exact model backend not verifiable from final document text alone.

# Governance Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Last Updated: 2026-11-06 | v2.5.0/v3.0.0 research and Wave plan, Issue #7422 -->
<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
-->

## Current Status

**Maturity: implemented capability base; release validation is incomplete.** The
module contains policy evaluation and lifecycle, compliance rules/reporting,
masking, lineage/model governance, audit utilities, and OPA integration. This is
a source-based maturity assessment, not a production-readiness or compliance
certification claim.

Evidence and limits:

- `src/governance/policy_engine.cpp`, `policy_manager*.cpp`, and
  `include/governance/policy_engine.h` provide policy evaluation and lifecycle
  surfaces. `OpaAdapter` supports REST and an injected WASM evaluator; there is
  no built-in WASM runtime. REST failure may reach native evaluation, which is
  not equivalent to the external policy. Explicit WASM mode without a working
  injected evaluator returns strict deny and does not switch evaluator
  ([trust assumptions](TRUST.md)).
- Compliance, data protection, and model-governance implementations include
  `compliance_framework.cpp`, `data_masker.cpp`, `data_lineage.cpp`, and
  `model_governance.cpp`. These separate APIs are not an automatic enforcement
  pipeline; their callers must consume and enforce the decisions.
- Focused test sources cover OPA fallback, policy simulation, lineage
  backpressure, compliance time windows, audit integrity, and lifecycle
  behavior. `tests/governance/CMakeLists.txt` currently excludes
  `test_operational_audit_evidence` and
  `test_policy_versioning_and_approval`; source presence is not runnable-test
  evidence.
- The policy-latency benchmark target is optional
  (`THEMIS_BUILD_BENCHMARKS=ON`). Its YAML-backed cases ignore load failure, so
  they can benchmark heuristic fallback instead of the configured policy.
  No current benchmark result or hardware baseline is asserted
  ([performance expectations](PERFORMANCE_EXPECTATIONS.md),
  [benchmark notes](../../benchmarks/governance/README.md)).
- Lineage and operational-audit surfaces have bounded in-process data; this does
  not establish durable retention. Compliance reports summarize supplied
  controls and evidence, not independent certification.
- Root [`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for
  project-wide release readiness and Wave C. This module roadmap cannot close
  those gates.

## In Progress / Planned Features

Priorities are proposed for release planning; release dates and milestone
assignments require maintainer confirmation.

### v2.5.0 — compatibility-preserving hardening

1. **P0 — close executable contract gaps (Wave 1):** resolve dependencies and
   register the quarantined audit-evidence and versioning/approval suites;
   specify behavior across native, REST-OPA, and injected-WASM evaluation and
   conflict/fallback outcomes.
2. **P1 — make performance evidence trustworthy (Wave 2):** fail or clearly mark
   benchmark cases when configuration loading fails; record reproducible build,
   policy, workload, hardware, and percentile results for policy and audit paths.
3. **P1 — verify enforcement and resilience at caller boundaries (Wave 3):**
   exercise masking, export/query permission, subject-rights, lineage limits,
   audit failure/backpressure, and policy rollback through representative
   integrations.

### v3.0.0 — explicit architecture and contract evolution

1. **P1 — durable evidence and lifecycle contracts (Wave 4):** specify and
   validate persistence, retention, recovery, and signing-key lifecycle through
   approved storage/key-management boundaries; in-memory state alone is not
   durable evidence.
2. **P1 — real local OPA WASM evaluation (Wave 5, blocked on approval):** select
   and approve a runtime/dependency, then implement compiled-bundle evaluation
   with strict failure behavior. Until approved and integrated, explicit WASM
   mode remains strict-deny when its evaluator is unavailable.
3. **P2 — versioned policy and AI-governance contracts (Wave 6):** define
   tenant-isolation and model lifecycle/evidence contracts, migration behavior,
   and compatibility policy before any v3 breaking API or policy-semantics
   change.

## Research and Gap Analysis

The research informs evaluation methods and design constraints; it does not
prove that this module meets a regulatory framework or benchmark target.

| Research / technical finding | Current code evidence | Gap and roadmap response |
|---|---|---|
| OPA's policy-performance guidance recommends policy profiling, workload-aware measurements, and suitable data structures for latency-sensitive decisions. | `bench_governance_policy_latency.cpp` has policy and permission cases, but best-effort YAML loading can invalidate the intended workload; no current result is recorded. | Wave 2 makes configuration and measured outcome explicit, records a reproducible baseline, then applies regression gates. |
| NIST SP 800-92 treats log management as an organizational lifecycle involving infrastructure, processes, and operational handling, rather than merely generating log records. | `AuditBatchWriter` exposes bounded buffering, checkpoints, and backpressure; lineage/audit APIs may retain in-process data. Persistent sink, retention, and key lifecycle are not established by these declarations. | Wave 3 verifies integration/failure handling; Wave 4 specifies and validates durable evidence and lifecycle boundaries. |
| NIST AI RMF 1.0 organizes AI risk management around Govern, Map, Measure, and Manage, with governance cross-cutting and documented risk treatment. | `model_governance.cpp` and `AI_ML_IMPACT_ASSESSMENT.md` implement related controls, but the code inventory does not establish a complete, traceable AI lifecycle workflow. | Wave 6 maps lifecycle evidence and tenant boundaries to explicit contracts and tests. |

### State-of-the-art references

- Open Policy Agent, [Policy Performance](https://www.openpolicyagent.org/docs/policy-performance):
  profiling, policy structure, and performance measurement guidance.
- NIST, [SP 800-92: Guide to Computer Security Log Management](https://csrc.nist.gov/pubs/sp/800/92/final):
  log-management infrastructure and process guidance.
- NIST, [AI Risk Management Framework 1.0 — AI RMF Core](https://airc.nist.gov/airmf-resources/airmf/5-sec-core/):
  Govern, Map, Measure, Manage functions and documented risk management.

## Implementation Phases

Estimates are planning ranges in person-weeks, not commitments or measured
delivery durations. Waves may be resequenced only after dependency and release
scope review.

### Phase 1 — Design / API contract (Wave 1, v2.5.0; 1–2 person-weeks)

- **Goal:** define decision, conflict, fallback, error, and evidence contracts
  before implementation changes.
- **Tasks:** create a behavior matrix for native/REST/WASM, including absent,
  denied, malformed, timed-out, and unavailable evaluators; mark caller
  enforcement boundaries; resolve the two quarantined test targets' dependency
  blockers.
- **Dependencies:** current module interfaces, `tests/governance/CMakeLists.txt`,
  and maintainer agreement on the intended external-policy semantics.
- **Risk / trade-off:** resolving linkage may expose wider core dependency
  coupling; avoid hiding it by treating omitted tests as passed.
- **Success metric / validation:** every evaluation state has a specified
  structured outcome and an assigned test; both currently excluded suites
  either build and run or retain a documented, owner-approved blocker.

### Phase 2 — Core implementation (Wave 1, v2.5.0; 2–3 person-weeks)

- **Goal:** deliver the smallest compatibility-preserving fixes needed to meet
  the Phase 1 contract.
- **Tasks:** align diagnostics/metrics across fallback and denial paths; finish
  only the test/build wiring needed to run audit-evidence and
  versioning/approval coverage; do not enable unapproved evaluator or persistence
  implementations.
- **Dependencies:** Phase 1 contract and a buildable focused target.
- **Risk / trade-off:** behavior changes can affect callers that relied on
  implicit native fallback; preserve explicit selection and release notes.
- **Success metric / validation:** all contract-matrix cases return the expected
  structured result; unavailable or failed selected evaluators never grant an
  allow decision.

### Phase 3 — Error handling and edge cases (Wave 3, v2.5.0; 2–4 person-weeks)

- **Goal:** verify that policy and data-governance controls hold at integration
  boundaries under failure and pressure.
- **Tasks:** add focused tests for caller-side enforcement, masking and export
  restrictions, lineage capacity/eviction, audit sink failure/backpressure,
  partial subject-rights operations, and rollback failure reporting.
- **Dependencies:** Phases 1–2 and availability of representative callers/test
  fixtures; identify the owning module when a caller is outside governance.
- **Risk / trade-off:** module-only tests cannot prove enforcement by every
  downstream caller; scope claims to tested integrations.
- **Success metric / validation:** all identified high-risk failure cases have
  assertions on structured outcomes and no fail-open behavior; focused suites
  pass in the supported build profile.

### Phase 4 — Tests and reproducible benchmark baseline (Wave 2, v2.5.0;
2–3 person-weeks)

- **Goal:** make performance and reliability comparisons repeatable and
  workload-valid.
- **Tasks:** require successful YAML load for YAML-labelled cases; identify
  effective policy and benchmark outcome; record binary, case/filter, build,
  policy, hardware, p50/p95/p99, and baseline; validate audit benchmark
  counters and result checks before treating its source-commented gates as
  release criteria.
- **Dependencies:** Phase 1 case definitions and a representative runner.
- **Risk / trade-off:** stricter setup checks may reveal missing configuration
  or unstable results; do not replace unavailable hardware data with synthetic
  estimates.
- **Success metric / validation:** 100% of mapped cases report the intended
  configuration and outcome; a same-host baseline is available. The existing
  source-commented policy p99 target (<0.5 ms) and 10% regression ceiling remain
  provisional until measured and approved for a named release profile.

### Phase 5 — Performance and security hardening (Waves 3–5, v2.5.0/v3.0.0;
3–8 person-weeks per release scope)

- **Goal:** close measured hot-path and evidence-durability risks without
  weakening fail-closed behavior.
- **Tasks:** apply only baseline-supported optimizations; for v3, design durable
  audit/lineage integration and evaluate an approved WASM runtime against a
  threat model, policy corpus, and resource limits.
- **Dependencies:** Phases 1–4; Wave 4 requires approved storage/key-management
  contracts, and Wave 5 requires maintainer approval of runtime and dependency.
- **Risk / trade-off:** durable sinks and WASM add operational/dependency costs;
  no persistence or evaluator may be implied by an in-memory helper or bundle
  path.
- **Success metric / validation:** no >10% regression from the approved same-host
  baseline; prove recovery/retention guarantees against the selected sink.
  WASM enablement additionally requires allow/deny parity for the approved
  corpus and fail-closed tests for missing, invalid, and failed evaluation.

### Phase 6 — Documentation and acceptance (Waves 1–6; 1–2 person-weeks)

- **Goal:** publish accurate operational and migration contracts and complete
  human acceptance for the planned release scope.
- **Tasks:** update API/runbook/trust and migration documentation where behavior
  changes; publish benchmark manifests and test evidence; map residual risks to
  root release gates without claiming to close them.
- **Dependencies:** implementation and validation evidence for the selected
  release waves; maintainer decision on v2.5.0/v3.0.0 milestone dates.
- **Risk / trade-off:** documentation may overstate deployment guarantees if
  evidence or caller ownership is unclear; label unverified claims as open.
- **Success metric / validation:** every shipped roadmap item links to code,
  tests, and (where relevant) a measured benchmark; the module owner and release
  reviewer sign off the residual-risk list.

## Wave Plan and Release Gates

Each wave below has its goal, tasks, dependencies, risk, and measurable exit
criteria; the phases above give the corresponding validation and design
sequence.

### v2.5.0

| Wave | Goal and tasks | Dependencies | Primary risk / trade-off | Validation strategy | Measurable exit criteria |
|---|---|---|---|---|---|
| **Wave 1 — Contracts and runnable coverage** (P0; 3–5 person-weeks) | Specify evaluator/fallback and conflict contracts; resolve/build/run quarantined audit and versioning suites; implement only contract-required diagnostics or fixes. | Maintainer semantics decision; focused build dependencies. | Test quarantine may reflect unresolved dependency coupling, not merely CMake configuration. | Build and run the two focused targets plus table-driven native/REST/WASM success, deny, timeout, malformed, and unavailable-evaluator cases. | 100% of contract states have expected results and tests; both excluded suites pass or have an explicit maintained blocker; zero allow on selected-evaluator failure. |
| **Wave 2 — Measured hot paths** (P1; 2–3 person-weeks) | Correct YAML setup verification; capture policy and audit benchmark baseline and environment manifest; gate regressions. | Wave 1, representative hardware/build, benchmark harness corrections. | Existing 0.5 ms and audit throughput/signing/recovery figures are source-level targets, not verified measurements. | Run every mapped case against a verified policy/configuration; confirm benchmark outcomes and report p50/p95/p99 on the same host/build as baseline. | Every mapped benchmark case records successful setup, outcome, and percentiles; approved same-host policy p99 target and ≤10% regression budget are met. |
| **Wave 3 — Integration and resilience** (P1; 2–4 person-weeks) | Test caller enforcement, masking/export, lineage bounds, audit pressure/failure, subject rights, and rollback. | Wave 1 contracts; participating caller modules and their fixtures. | Governance unit tests cannot establish universal downstream enforcement or durable retention. | Run focused caller integration tests and deterministic stress/fault cases for rejected, partial, capacity, and sink-failure outcomes. | 100% of identified critical integration/failure cases are covered and pass; no tested protected action is authorized after failed selected evaluation. |

### v3.0.0

| Wave | Goal and tasks | Dependencies | Primary risk / trade-off | Validation strategy | Measurable exit criteria |
|---|---|---|---|---|---|
| **Wave 4 — Durable evidence and lifecycle** (P1; 4–6 person-weeks) | Define storage adapter, retention/deletion, recovery, and signing-key lifecycle contracts; validate with an approved durable backend. | Storage and key-management owners approve interfaces and operational ownership. | Stronger retention improves evidence but raises privacy, cost, and deletion obligations. | Run crash/restart, retention/deletion, integrity-tamper, and key-rotation tests against the selected backend; review operational recovery evidence. | Crash/restart tests meet a maintainer-approved retention/recovery SLO; integrity verification and key-rotation tests pass; no durability claim relies only on process memory. |
| **Wave 5 — Local OPA WASM runtime** (P1; 4–8 person-weeks after approval) | Select runtime; evaluate compiled bundles; enforce resource/error limits; preserve explicit evaluator selection. | Maintainer approval of runtime/dependency, licensing/security review, bundle format and policy corpus. | Runtime footprint and policy feature compatibility may constrain deployment; no runtime is currently integrated. | Execute approved allow/deny and REST-comparison fixtures; inject missing, malformed, unsupported, timeout, and runtime-error cases; measure target-hardware latency and memory. | Approved allow/deny corpus has 100% expected parity; failure cases all fail closed; latency and memory budgets are set from recorded target-hardware results before enablement. |
| **Wave 6 — Versioned tenant and AI-governance contracts** (P2; 3–6 person-weeks) | Define tenant-isolation and AI model lifecycle evidence; version policy semantics and APIs; publish migration/deprecation path for any approved breaking change. | Waves 1, 3, and 4; caller and model-governance owners agree on boundaries. | Broader policy expressiveness can increase evaluation cost and migration burden. | Run cross-tenant positive/negative policy matrix, model lifecycle evidence tests, API compatibility checks, and migration fixtures. | Cross-tenant negative tests show zero unauthorized decisions in the defined corpus; every governed model lifecycle state has traceable evidence and tests; all breaking changes have migration guidance and version approval. |

## Risk and Trade-offs

- **Availability vs. policy equivalence:** native fallback after REST failure may
  preserve availability but does not execute the failed OPA policy. Keep the
  fallback observable and explicitly selected; do not conflate it with an OPA
  allow.
- **Fail-closed vs. operability:** strict deny when selected WASM evaluation is
  absent protects authorization, but is not a usable substitute for a real
  evaluator. Block production enablement until Wave 5 passes.
- **Evidence vs. privacy/retention:** durable audit improves investigation and
  compliance evidence but introduces retention, deletion, key-management, and
  sensitive-data risks. Wave 4 must define ownership and lifecycle before
  implementation.
- **Performance vs. measurement validity:** a fast fallback benchmark is not
  evidence of configured-policy performance. Validate configuration, outcome,
  workload, and hardware before using thresholds for release decisions.
- **Framework mapping vs. certification:** NIST/OPA references guide engineering
  practices; neither roadmap completion nor a generated report certifies legal
  or regulatory compliance.

## Production Readiness Checklist

- [ ] Focused governance tests, including the two currently excluded suites,
  build and pass in the supported release profile.
- [ ] Policy, masking, export, OPA selection, and fallback integration evidence
  is reviewed by the owning callers.
- [ ] Benchmarks use verified configuration and publish reproducible environment
  and percentile data against an approved baseline.
- [ ] Audit/lineage durability, retention, and key lifecycle are stated
  accurately; no in-memory-only evidence is described as durable.
- [ ] Wave C security-production evidence is refreshed and accepted through the
  root roadmap; governance alone does not close Wave C.
- [ ] Any v3 breaking API or policy-semantics change has an approved migration
  plan and release notes.
- [ ] Residual risks and human release sign-off are recorded.

## Known Issues & Limitations

- A failed OPA REST evaluation can lead to native evaluation, which may have
  different policy semantics.
- Explicit WASM mode requires an injected evaluator; a bundle path alone is not
  evaluation, and missing/failed evaluation returns strict deny.
- Two focused test sources are excluded from `tests/governance/CMakeLists.txt`;
  current registration is not proof of test execution.
- YAML-backed policy benchmark cases ignore load failure; no current result,
  release baseline, or target-hardware measurement is asserted.
- Bounded in-process lineage/audit state does not establish durable evidence,
  retention, or crash recovery.
- Compliance and AI-impact reports are summaries of configured rules and input
  evidence, not independent legal findings or certification.
- Release dates, performance hardware/profile, OPA WASM dependency/runtime, and
  durable backend/key-management ownership remain open assumptions requiring
  maintainer decisions.

## Breaking Changes

No v2.5.0 breaking change is proposed by this plan. v3.0.0 may revise policy
semantics, tenant-isolation contracts, or public interfaces only after the
contract and migration work in Wave 6 is approved. No compatibility guarantee
or removal schedule is asserted before that decision.

## Research and Source References

- [Governance architecture](ARCHITECTURE.md), [trust assumptions](TRUST.md),
  [performance expectations](PERFORMANCE_EXPECTATIONS.md), and
  [module test guide](MODULE_GUIDE.md).
- [Policy latency benchmark](../../benchmarks/governance/bench_governance_policy_latency.cpp),
  [audit benchmark](../../benchmarks/governance/bench_audit_export_high_volume.cpp),
  [governance tests](../../tests/governance/CMakeLists.txt), and
  [benchmark registration](../../benchmarks/governance/CMakeLists.txt).
- [Repository release readiness](../../ROADMAP.md) and
  [release strategy](../../RELEASE_STRATEGY.md).
- Research sources: [OPA policy performance](https://www.openpolicyagent.org/docs/policy-performance),
  [NIST SP 800-92](https://csrc.nist.gov/pubs/sp/800/92/final), and
  [NIST AI RMF Core](https://airc.nist.gov/airmf-resources/airmf/5-sec-core/).

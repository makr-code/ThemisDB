# Document Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 (source/configuration and literature review; no tests run) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · DEVELOPMENT_STATUS_2026_07_28.md -->
<!--
Last Updated: 2026-11-06
Source level: L1
SOT domain: module-behavior
Canonical references: README.md; SECURITY.md; PERFORMANCE_EXPECTATIONS.md; ../../ROADMAP.md; ../../VERSIONING.md; ../../include/document/; ../../tests/document/CMakeLists.txt; ../../benchmarks/document/
Related: Issue #7415
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone.
-->

## Current Status

Document runtime interfaces and in-memory/reference implementations exist for store/manager, lifecycle, schema validation, diff/merge, XDOMEA, diagnostics, encrypted-entity helpers, and store-backed round-trip snapshots. This source review does not certify production deployment readiness.

## Delivered source surfaces

- [x] hardening edge-case consistency for schema validation and merge conflict semantics (Target: Q3 2026)
- [x] benchmark source cases for in-memory store/list and diff/merge paths (Target: Q3 2026)
- [x] diagnostics consistency improvements for store and round-trip persistence failures (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic conflict handling for multi-branch merge permutations (Target: Q4 2026)
- [x] expand regression coverage for schema sealing/version transition edge cases (Target: Q4 2026)
- [x] improve operator diagnostics for document round-trip and exchange failures (Target: Q4 2026)

### Mid-term (6-12 months)
- [~] re-baseline measured latency/throughput envelopes on representative hardware (Target: Q1 2027)
  - The `DOC-BM-01..04` source emits thresholds for synthetic stubs; it is not production-path p95/p99 evidence. See PERFORMANCE_EXPECTATIONS.md.
- [x] add dedicated benchmark coverage for diff/merge and round-trip workflows (Target: Q1 2027)
  - Delivered as in-memory interface benchmarks in `bench_document_diff_merge.cpp` and `bench_document_round_trip.cpp`; hardware baselines remain separate evidence.
- [~] harden long-running reliability under document churn and conflict-heavy workloads (Target: Q1 2027)
  - Soak test created: tests/integration/test_document_store_soak.cpp (2026-09-16); full 60-min hardware run pending Wave D sign-off

## Implementation Phases

### Phase 1: Design / API Contract
- [x] document store/manager/lifecycle/schema interfaces and error behavior (Target: Q3 2026; this does not assert ABI or source-compatibility guarantees)
- [x] define explicit error taxonomy for not-found, conflict, and schema-violation classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] complete hardening for manager orchestration and round-trip persistence internals (Target: Q4 2026)
- [x] align schema/merge behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] expose explicit errors/reports for invalid arguments, missing schemas, and merge conflicts (Target: Q4 2026)
- [x] unify diagnostics across store, lifecycle, merge, and exchange failure paths (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for schema, diff/merge, and round-trip edge scenarios (Target: Q4 2026)
- [x] extend deterministic fixture coverage for XDOMEA and multi-version document cases (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] define source-level benchmark checks for in-memory store, diff/merge, and round-trip paths (Target: Q4 2026)
- [~] validate measured latency/throughput against representative release baselines (Target: Q4 2026)
  - TODO: attach benchmark output and the release workflow's gate-consumption evidence; the source definitions alone do not prove a baseline run.

### Phase 6: Documentation and Acceptance
- [x] core document module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core document surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for schema/merge/round-trip edge paths
- [~] representative-hardware benchmark stabilization complete

## Known Issues and Limitations

- module implementation is largely header-first with selected src-level persistence wiring.
- representative-hardware benchmark output and full soak execution/sign-off are not established by this source/configuration review.
- the four `DOC-BM` serialization-gate cases are synthetic stubs; do not treat them as production-path measurements.

## Breaking Changes

No breaking change is identified by this source review; planned compatibility policy is not established by the headers. Any approved contract-breaking change requires migration notes and a changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable. This roadmap does not independently certify CI status or Wave D sign-off.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `document`
- [~] Validate applicable high-cardinality stress and operator guidance (Target: Q1 2027)
  - Source artifacts exist: `tests/document/test_document_highcardinality_stress.cpp` and `docs/operability/RUNBOOK_DOCUMENT_STORE.md`. Execution and operational sign-off are not verified here.
- [~] Validate long-duration soak coverage for primary paths (Target: Q1 2027)
  - `tests/integration/test_document_store_soak.cpp` exists. TODO: attach a completed run record and reviewer sign-off.
- [x] Provide a module runbook artifact for operator-critical scenarios (Target: Q1 2027)
  - `docs/operability/RUNBOOK_DOCUMENT_STORE.md` exists; this file's presence is not proof of alert/trace integration.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] Distributed/acceleration criteria: no such document-module path was identified in the reviewed headers or implementation. TODO: confirm applicability with the program owner before assigning a status.
- [~] Representative-hardware benchmark baselines exist (Target: Q1 2027)
  - Threshold definitions exist; representative-hardware results and sign-off remain TODO.
- [~] Operator-critical paths have verified diagnostics/alerts/runbook integration (Target: Q1 2027)
  - Runbook exists; alert and trace wiring require confirmation in the owning integration sources.

### Wave D Closure Batch (2026-09-16)

Artifacts present at the paths below; this is not an execution/sign-off record:
- `tests/integration/test_document_store_soak.cpp` — soak test source.
- `tests/document/test_document_highcardinality_stress.cpp` — stress test source.
- `docs/operability/RUNBOOK_DOCUMENT_STORE.md` — operator runbook source.
- `benchmarks/document/bench_document_serialization_gates.cpp` — four synthetic stub cases that emit threshold counters, not production-path p95 measurements.

## Evidence-Based Roadmap: v2.5.0 and v3.0.0 (proposals)

This section records proposed investigation and delivery measures, not approved commitments. Targets are release lanes, not dates. Root `../../ROADMAP.md` identifies `v2.4.0-alpha` as current, and `../../VERSIONING.md` identifies `v2.5.0` as the next minor feature target. No approval or release date for `v3.0.0` was found. Any scope, API break, or release commitment requires the relevant owner/release approval.

### Maturity snapshot

- **Implemented boundary:** `README.md` and the public headers in `../../include/document/` describe predominantly in-memory/reference implementations for store/manager, lifecycle, schema checks, diff/merge, XDOMEA, diagnostics, and encrypted-entity helpers. `src/document/round_trip_editor.cpp` wires round-trip snapshots to a supplied store. `IDocumentStore` is an interface; its existence does not imply a durable backend.
- **Validation and exchange boundary:** `InMemoryDocumentSchemaEvolution` currently checks configured required fields and JSON types; it is not a general JSON Schema validator. Round-trip persistence does not itself call schema validation. The in-memory XDOMEA parser is not XSD validation.
- **Encryption boundary:** `InMemoryDocumentManager::createEncrypted()` stores the JSON supplied by the caller as-is, and `InMemoryEncryptedEntity::reencrypt()` updates key-ID state but does not re-cipher stored document bytes. These helpers are not a production encryption/key-rotation path.
- **Test evidence:** `tests/document/CMakeLists.txt` registers focused targets from the six direct `test_document_*.cpp` files (including schema, store, round-trip, diagnostics, XDOMEA, and high-cardinality cases). Registration/source presence is not evidence that tests were executed or passed.
- **Benchmark evidence:** `benchmarks/document/CMakeLists.txt` conditionally registers three in-memory-interface benchmark suites and a synthetic serialization-gates executable when benchmark support is enabled and found. Existing latency checks named `p99` compare a mean calculated from elapsed time and iterations; they do not calculate p99. The four `DOC-BM-01..04` counters use synthetic string/vector stubs and neither exercise production document operations nor fail on a missed threshold. No representative-hardware results or release-workflow gate consumption are established by this source review.
- **Security/operations boundary:** `SECURITY.md` leaves authorization, durable-store encryption, retention, backup, audit, and production XDOMEA validation as integration/deployment questions. This roadmap does not infer a production backend, security policy, or release readiness from interfaces, tests, or benchmark source.

### Research and evidence basis

Local evidence is the module source-of-truth set: [`README.md`](README.md), [`SECURITY.md`](SECURITY.md), [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), the public contracts in [`../../include/document/document_store.h`](../../include/document/document_store.h), [`../../include/document/document_manager.h`](../../include/document/document_manager.h), [`../../include/document/document_schema_evolution.h`](../../include/document/document_schema_evolution.h), and [`../../include/document/xdomea_connector.h`](../../include/document/xdomea_connector.h); focused-test registration in [`../../tests/document/CMakeLists.txt`](../../tests/document/CMakeLists.txt); and benchmark inventory, gate behavior, and registration in [`../../benchmarks/document/README.md`](../../benchmarks/document/README.md), [`../../benchmarks/document/RELEASE_GATES.md`](../../benchmarks/document/RELEASE_GATES.md), [`../../benchmarks/document/DOCUMENT_BENCHMARK_COVERAGE.md`](../../benchmarks/document/DOCUMENT_BENCHMARK_COVERAGE.md), and [`../../benchmarks/document/CMakeLists.txt`](../../benchmarks/document/CMakeLists.txt). These establish source/configuration facts, not execution results.

External references inform candidate methods and controls, not current implementation claims or mandatory product requirements:

- Cooper et al., [“Benchmarking Cloud Serving Systems with YCSB,” SoCC 2010](https://doi.org/10.1145/1807128.1807152): supports comparing systems with explicitly specified, workload-specific workloads. It does not provide a document-module workload or performance target.
- Koskela et al., [“Principles for Automated and Reproducible Benchmarking,” SC-W 2023](https://doi.org/10.1145/3624062.3624133): supports recording reproducible setup and benchmark metadata. Its HPC context is not direct evidence about ThemisDB’s workloads.
- [JSON Schema Validation, Draft 2020-12](https://json-schema.org/draft/2020-12/json-schema-validation): a candidate contract if broader interoperable JSON validation is selected; it is not the current in-memory validator’s behavior.
- [RFC 8785, JSON Canonicalization Scheme](https://www.rfc-editor.org/rfc/rfc8785): an optional deterministic representation to evaluate only if a defined hash/signature use case needs it; it does not itself provide cryptographic signing or encryption.
- [NIST SP 800-92](https://doi.org/10.6028/NIST.SP.800-92) and the [OWASP Database Security Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Database_Security_Cheat_Sheet.html): provide general log-management, least-privilege, and backup/security guidance. They do not prove this module implements those deployment controls.

### Proposed measures by release lane

#### v2.5.0 — next minor lane; proposed, not approved

- Establish comparable in-memory workload definitions and a reproducible baseline before setting new absolute performance budgets.
- Correct the mismatch between `p99` gate names and mean-based checks: either implement a reviewed percentile measurement/reporting method or rename the checks to state that they measure means. Preserve historical numbers as historical thresholds until baselines and owners review them.
- Keep synthetic `DOC-BM` instrumentation explicitly separate from production-path/interface performance evidence.
- Specify whether the existing required-field/type validator is sufficient for the module contract or whether a broader schema contract is proposed; JSON Schema 2020-12 is a candidate, not an assumed requirement.
- Make encryption-helper boundaries explicit and define caller/deployment responsibilities for authorization, audit, backup/restore, key lifecycle, and XDOMEA XSD validation. Do not claim a concrete durable store or actual `createEncrypted()` encryption.

#### v3.0.0 — major lane; conditional exploration, no release date or approval found

- First decide whether a durable backend, expanded schema contract, or public API/wire compatibility change belongs in this module. The `IDocumentStore` interface alone is not evidence that such a backend exists or is committed.
- If an owner approves a breaking contract, define versioning, migration/compatibility policy, failure semantics, and security ownership before implementation; otherwise defer the change rather than imply it is in scope.
- Consider durable transaction/recovery, access-control and collection-isolation boundaries, persisted encryption/key rotation, auditability, retention, and backup/restore only with an identified owning integration and threat model.
- Consider canonical JSON only for a documented deterministic hash/signature requirement, with compatibility and security review. Do not treat canonicalization as encryption.

### Sequential proposed waves

Each wave is conditional on its dependencies and owner review. Person-week values are rough engineering effort estimates, excluding external approvals and hardware procurement; they are not schedule commitments. “Validation required” describes future exit evidence, not tests run for this roadmap update.

#### Wave 1 — Characterize contracts and establish benchmark evidence (v2.5.0 proposal)

- **Proposed priority:** P0 — prerequisite for evidence-based v2.5.0 performance claims.
- **Goal:** Replace source-only performance assumptions with reproducible, workload-specific evidence while preserving current behavior until reviewed.
- **Tasks:**
  - [ ] Document a workload matrix for store CRUD/list/count, schema validation, diff/merge, and round-trip; fix document sizes, operation mix, concurrency, warm-up, sample duration, and build/runtime settings. (Target: v2.5.0, proposed)
  - [ ] Record benchmark command, hardware/OS/compiler/build flags, input parameters, raw output, and revision with each baseline artifact. (Target: v2.5.0, proposed)
  - [ ] Resolve the `p99` label/mean-comparison mismatch by implementing a reviewed percentile method or relabeling mean-based checks; keep thresholds separate from baseline results. (Target: v2.5.0, proposed)
  - [ ] Mark synthetic `DOC-BM-01..04` outputs as non-gating proxies unless replaced by a separately reviewed production/interface-path benchmark. (Target: v2.5.0, proposed)
- **Dependencies:** Representative hardware and a reproducible build; agreement on workload dimensions, statistical reporting, and artifact ownership.
- **Risk / trade-off:** Percentile collection and repetitions increase runtime and storage; mean-only reporting is cheaper but cannot substantiate tail-latency claims. In-memory results remain non-persistent-backend results.
- **Measurable success metrics:** Every selected workload has a versioned definition and reproducible run metadata; each baseline contains at least five independent runs with per-run results and reported run-to-run variation; raw per-operation latency samples support any claimed percentile; no active gate is described as p99 unless its implementation computes the specified percentile; no synthetic result is counted as a production/interface-path result. Do not add absolute performance targets before baseline review.
- **Validation required:** Unit tests for any benchmark-statistic/helper code; integration/configuration check that registered executables build; repeat benchmark runs on the declared host with raw output retained and variability reported; governance review of gate naming, artifact provenance, and release-workflow consumption. No security test is implied by this performance wave.
- **Estimate:** 2–4 person-weeks.
- **Unknown assumptions:** Availability and stability of representative hardware; acceptable percentile estimator and sample count; whether release automation consumes these benchmark targets; reproducibility tolerances across builds.

#### Wave 2 — Decide and verify schema/exchange validation contract (v2.5.0 proposal)

- **Proposed priority:** P1 — required before expanding validation or making interoperability claims.
- **Goal:** Bound validation semantics so callers can distinguish current required-field/type checks from any future schema or XML validation.
- **Tasks:**
  - [ ] Inventory current schema semantics, default handling, unknown fields, version transitions, and round-trip validation behavior; define compatibility expectations before changing code. (Target: v2.5.0, proposed)
  - [ ] Decide whether to retain the current field-descriptor contract or propose a JSON Schema 2020-12-compatible contract; document unsupported features and migration implications. (Target: v2.5.0, proposed)
  - [ ] Specify XDOMEA XML parsing versus XSD validation responsibilities and select an owning implementation boundary before adding validation. (Target: v2.5.0, proposed)
  - [ ] Add regression cases for the accepted contract, including missing required fields, wrong types, unknown fields, schema-version errors, and malformed/nonconforming XML as applicable. (Target: v2.5.0, proposed)
- **Dependencies:** Wave 1 workload/behavior inventory; product and integration-owner decision on schema compatibility and XDOMEA ownership.
- **Risk / trade-off:** Broader validation can reject documents accepted today and add dependencies, CPU cost, or migration burden; keeping the narrow contract limits interoperability.
- **Measurable success metrics:** Every supported validation rule has at least one positive and one relevant negative test; tests pin down unknown-field and default semantics; documentation states whether the validator is field-descriptor-only or supports a named schema draft; no claim of XSD validation without an XSD validator and evidence.
- **Validation required:** Focused unit tests and schema-version integration tests; benchmark any new validator against Wave 1 workloads with identical fixtures; security review for untrusted schema/XML resource limits and parser configuration; governance/API review for compatibility and dependency changes.
- **Estimate:** 2–5 person-weeks, depending on whether the scope is specification-only or implementation.
- **Unknown assumptions:** Schema draft and feature subset; whether validation runs on writes, reads, or round-trip operations; XML schema owner, source of schemas, and safe resource limits.

#### Wave 3 — Close module/deployment security boundaries (v2.5.0 proposal)

- **Proposed priority:** P1 — required before claiming module or deployment security guarantees.
- **Goal:** Prevent helper names and interfaces from being mistaken for deployed authorization, encryption, key rotation, auditing, or recovery controls.
- **Tasks:**
  - [ ] Specify caller/integration responsibilities for authorization and collection isolation; avoid treating `IDocumentStore` or `IDocumentManager` as an authorization policy. (Target: v2.5.0, proposed)
  - [ ] Review `createEncrypted()` and `reencrypt()` naming/contract options and document that current in-memory behavior stores supplied JSON as-is and does not re-cipher bytes; change public behavior only after compatibility review. (Target: v2.5.0, proposed)
  - [ ] Assign ownership and evidence requirements for durable-store encryption/key lifecycle, audit logging, retention, backup/restore, and least privilege using the deployment threat model. (Target: v2.5.0, proposed)
  - [ ] Keep XDOMEA import/export limits and malformed-input handling explicit; validate only if an approved implementation boundary is selected. (Target: v2.5.0, proposed)
- **Dependencies:** Wave 2 boundary decisions; store/deployment owners, security reviewer, and a documented threat model.
- **Risk / trade-off:** Secure defaults and added audit/backup controls may require integration APIs and operational overhead; module-only changes cannot secure an unspecified concrete backend.
- **Measurable success metrics:** Each listed control has a named owning layer and a verification artifact or is explicitly recorded as out of module scope; focused tests prove current helper semantics and fail closed for invalid inputs where applicable; no security capability is claimed solely from a helper name or interface.
- **Validation required:** Focused unit and integration tests for any changed contract; security review and threat-model sign-off; access-control/least-privilege tests at the concrete integration boundary; backup/restore and audit evidence where a durable owner exists; governance approval for compatibility changes.
- **Estimate:** 2–4 person-weeks for boundary/contract work; implementation and operational exercises are additional and unestimated until the owner and backend are known.
- **Unknown assumptions:** Concrete store and deployment are unspecified; applicable data classification/regulatory requirements; key-management system, audit retention, backup frequency, and responsible operations team.

#### Wave 4 — Decide v3.0.0 scope and compatibility policy (conditional exploration)

- **Proposed priority:** P0 — mandatory scope gate before any v3.0.0 implementation commitment.
- **Goal:** Make an explicit, reviewed go/no-go decision on major-version changes before committing to a backend or breaking contract.
- **Tasks:**
  - [ ] Compare the current interface/reference implementation with candidate durable-store and validation requirements; record whether any requirement belongs to this module or another integration. (Target: v3.0.0, conditional; no date/approval)
  - [ ] If a breaking change is justified, define compatibility boundaries, API/ABI and data migration plan, failure/recovery semantics, and support policy. (Target: v3.0.0, conditional)
  - [ ] Record owner, threat model, deployment context, and acceptance evidence for every selected security/recovery requirement. (Target: v3.0.0, conditional)
- **Dependencies:** Waves 1–3 evidence; explicit release/product, module, storage, and security-owner decisions.
- **Risk / trade-off:** A durable backend or contract expansion can increase operational complexity and break consumers; postponement limits scope but retains the current in-memory/reference boundary.
- **Measurable success metrics:** A decision record lists approved scope, deferred items, owners, migration compatibility, and measurable exit evidence; absent approval, all implementation tasks remain deferred and no release date or commitment is asserted.
- **Validation required:** Design/API review and compatibility analysis; prototype-focused unit/integration tests only after scope approval; threat-model and governance sign-off for selected security controls. Benchmark comparison must use Wave 1 workload definitions; a prototype is not a release result.
- **Estimate:** 1–3 person-weeks for decision/design; implementation not estimated before scope approval.
- **Unknown assumptions:** Product demand, backward-compatibility constraints, concrete backend choice, supported deployment topologies, and approval timing.

#### Wave 5 — Implement only the approved v3.0.0 contract (conditional)

- **Proposed priority:** P0 only for capabilities approved by Wave 4; otherwise deferred.
- **Goal:** Deliver an approved major-version capability without representing a proposal as an existing implementation.
- **Tasks:**
  - [ ] If approved, implement the selected durable backend or contract extension behind reviewed interfaces, including explicit durability, transaction, error, and recovery semantics. (Target: v3.0.0, conditional; implementation scope TBD)
  - [ ] If schema or canonicalization changes are approved, implement only the agreed subset and define persisted-data/API compatibility behavior. (Target: v3.0.0, conditional)
  - [ ] Add migration guidance and versioned contract tests for every breaking behavior. (Target: v3.0.0, conditional)
- **Dependencies:** Wave 4 approval, named implementation owner, backend design, migration plan, security review, and test environment.
- **Risk / trade-off:** Durability, indexing, concurrency, migration, and recovery add complexity and can change latency/error semantics; canonicalization only solves representation determinism and does not secure data.
- **Measurable success metrics:** Contract tests cover every approved API/data migration rule; fault-injection tests verify each documented recovery guarantee; security checks cover the agreed trust boundaries; benchmark results report the same Wave 1 workload dimensions and environment metadata, with budgets set only after reviewed baselines.
- **Validation required:** Unit, integration, migration, crash/recovery and fault-injection tests for selected capabilities; end-to-end backup/restore and access-control tests where applicable; representative-hardware benchmark runs; security and release-governance review. Test/build execution is required evidence, not inferred from registration.
- **Estimate:** Not estimable before Wave 4 selects backend and scope; planning placeholder 6–16+ person-weeks, explicitly high uncertainty.
- **Unknown assumptions:** Backend and transaction model; durability and recovery objective; data volume and workload; migration volume/window; supported platform matrix; operational ownership.

#### Wave 6 — Verify operational and release evidence for approved v3.0.0 scope (conditional)

- **Proposed priority:** P0 release gate for the approved v3.0.0 scope.
- **Goal:** Demonstrate only the approved capability’s reliability, security, compatibility, and workload behavior in its intended deployment context.
- **Tasks:**
  - [ ] Run approved churn, conflict, high-cardinality, and soak scenarios with recorded environment, duration, workload, failures, and artifact links. (Target: v3.0.0, conditional)
  - [ ] Verify backup/restore, audit, least-privilege, key lifecycle, and alert/runbook integration for the concrete deployment, where applicable. (Target: v3.0.0, conditional)
  - [ ] Review baseline-derived performance budgets, CI/release gate consumption, migration evidence, and residual risks before any readiness statement. (Target: v3.0.0, conditional)
- **Dependencies:** Wave 5 implementation and test environment; representative hardware; deployment, security, operations, and release owners.
- **Risk / trade-off:** Long-running and production-like validation is resource-intensive; results are only applicable to the tested configuration and cannot establish universal performance or security.
- **Measurable success metrics:** Each approved release criterion links to a reproducible artifact and reviewer; soak duration/workload and recovery outcomes meet values approved in Wave 4; benchmark budgets are derived from measured baseline and documented variance rather than assumed in advance; all untested contexts are explicitly excluded from claims.
- **Validation required:** Unit/integration and fault-injection suites; repeated representative-hardware benchmarks; security/governance and migration reviews; operational backup/restore and runbook exercises. Record execution outcomes before changing any checklist status.
- **Estimate:** 3–8 person-weeks, excluding test infrastructure provisioning and elapsed soak time.
- **Unknown assumptions:** Approved deployment profiles, soak duration and load model, accepted recovery objectives, security/compliance obligations, and release-gate integration.

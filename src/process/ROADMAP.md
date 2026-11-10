---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-10
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; ROADMAP.md; include/process/*.h; src/process/*.cpp; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; benchmarks/compare_expectations.py; src/process/RESEARCH.md; src/process/IMPLEMENTATION.md; src/process/PERFORMANCE_EXPECTATIONS.md
Related_Issue: "#7441"
Related_Milestone: DOC-WEEKLY-2026-45
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Roadmap

**AI provenance:** Exact model backend not verifiable from final document text alone.

This roadmap is an evidence-aware planning index, not a release approval. Earlier versions contained fixed test/benchmark totals, pass claims, and readiness assertions that do not follow from the current source tree. No current completion percentage or release status is asserted here.

## Current Status

### Maturity assessment

The module has implemented source surfaces for process-model lifecycle and format handlers, linking, graph/context retrieval, analytics, external orchestration profiles, federation, conflict resolution, audit, diagnostics, and telemetry. This is a source inventory, not a maturity or release-readiness claim. `ProcessLightRetriever` now caps returned bytes, GLOBAL report count, LOCAL prompt lines, and LOCAL traversal depth, and reports elapsed time and returned bytes; synchronous dependencies cannot be preempted to guarantee a hard timeout. Focused budget tests have been added, but their configured execution remains pending. Other evidence gaps remain: profile activation/rollback is a sequence of writes without an atomicity guarantee; audit persistence is an in-memory no-op and its checksum is CRC32; telemetry's collector send path logs and returns success rather than demonstrating external OTLP transport. Federation maturity and guarantees remain unverified. Test registration is not evidence of a passing build or test run, and there is no current Process performance baseline or comparator-enforced expectation. See the [implementation notes](IMPLEMENTATION.md), [performance expectations](PERFORMANCE_EXPECTATIONS.md), and the source evidence below.

## In Progress

- [~] Reconcile public contracts and module documentation with implementation behavior; keep unsupported guarantees explicitly marked (Target: v2.5.0 hardening wave).
- [~] Verify the implemented `ProcessLightRetriever` budgets by executing the added production-class focused cases in a configured build (Target: v2.5.0 hardening wave).

## Planned Features

- [ ] Specify and test recovery for partial orchestration-profile state updates (Target: v2.5.0 hardening wave).
- [ ] Reconcile audit persistence/integrity and telemetry export behavior with their public contracts; do not claim unsupported guarantees (Target: v2.5.0 hardening wave).
- [ ] Establish reproducible build/test evidence and a benchmark baseline before publishing module readiness or performance status (Target: v2.5.0 hardening wave).
- [ ] Evaluate standards-based process-model/event interchange and the long-term audit, telemetry, and federation contracts (Target: v3.0.0 strategic wave).

## Research findings and gap analysis

| Area | Source-based finding | Gap / research implication |
|---|---|---|
| Retrieval limits | [`process_light_retriever.cpp`](process_light_retriever.cpp) selects AUTO by keyword, limits GLOBAL reports, caps returned bytes and LOCAL prompt lines, clamps LOCAL traversal depth, and reports time/size metrics. New cases in [`test_process_light_retriever.cpp`](../../tests/process/test_process_light_retriever.cpp) exercise the production class; configured execution is pending. Synchronous dependencies prevent hard timeout cancellation. | Run focused cases against a configured build and retain the result; do not describe the soft post-operation timeout check as a hard deadline. |
| Profile state | [`process_orchestration_registry.cpp`](process_orchestration_registry.cpp) demotes a prior profile, persists profile/revision, then writes the active-domain pointer. Rollback reuses activation. The [registry lifecycle tests](../../tests/process/test_process_orchestration_registry.cpp) do not inject failures. | Choose and specify recovery semantics; test failures between writes. Do not promise transactionality without a supporting storage transaction. |
| Audit and telemetry | [`process_audit_logger.cpp`](process_audit_logger.cpp) uses CRC32 for entries/checksums; persistence/load methods are in-memory/no-op success. [`process_telemetry_integration.cpp`](process_telemetry_integration.cpp) emits simplified JSON and its collector send method logs the endpoint and returns true; trace-context parsing is simplified. | Decide durability, threat model, wire format, and transport boundaries; test the implemented contract. CRC32 is not adversarial tamper evidence, and a successful send return is not proof of collector delivery. |
| Federation | Federation code and contracts exist, but implementation guarantees require direct verification; documentation describes alpha/core status. | Establish supported failure, quorum, consistency, and security semantics from implementation and fault tests. Comments alone do not prove Raft or Byzantine guarantees. |
| Verification/performance | Process benchmarks register 12 targets; the comparator finds zero Process expectation rows. Four PM-BM names/comments are not enforced thresholds and use in-process stubs, not OS-process benchmarks. Release coverage 14/14 is registration coverage only. Historical mining output is not a current baseline; the latest visible performance run failed during configure. | Capture reproducible configured build/test/benchmark evidence. Map cases to comparator-enforced expectations only after a representative workload owner confirms a baseline and proposed gates. |

External research informs evaluation design, not claims about current code:

- [OMG BPMN 2.0.2](https://www.omg.org/spec/BPMN/2.0.2/PDF/) formalizes process models, conformance, and interchange; use it to identify any intended model-conformance scope and fixtures.
- [OCEL 2.0](https://arxiv.org/abs/2403.01975) ([specification](https://www.ocel-standard.org/2.0/ocel20_specification.pdf)) describes object/event relations and JSON, XML, and relational exchange; assess whether and how process instances map to those concepts before selecting an interchange contract.
- The [Process Mining Manifesto](https://doi.org/10.1007/978-3-642-28108-2_19) distinguishes discovery, conformance checking, and enhancement; use these as separate evaluation questions rather than infer quality from feature presence.
- [NIST SP 800-92](https://csrc.nist.gov/pubs/sp/800/92/final) and [NIST SP 800-218 SSDF](https://csrc.nist.gov/pubs/sp/800/218/final) provide log-management and secure-development considerations for audit lifecycle and evidence practices.
- The [W3C Trace Context Recommendation](https://www.w3.org/TR/trace-context-1/) is a reference for interoperable trace-context parsing/propagation tests.

## Release wave plans

These are planning proposals for v2.5.0 and v3.0.0, not release approvals or delivery commitments. Engineer-week ranges are rough planning estimates, not measured durations. No calendar dates or schedule commitments are assigned; none are established here by the root release SOT. All numeric criteria below are proposed acceptance gates and must be confirmed by the relevant maintainer/workload owner before adoption.

### v2.5.0 — near-term hardening

- **Objective:** Close or explicitly narrow the highest-risk implementation/contract gaps and produce reproducible focused verification without asserting module-wide readiness.
- **Estimated effort:** 4–8 engineer-weeks total (planning estimate; excludes external dependency/CI remediation and is not a schedule commitment).

#### Wave 1 — bounded retrieval (P0; 1–2 engineer-weeks)

- **Goal:** Make declared `ProcessLightRetriever` resource behavior truthful and testable on production code paths.
- **Tasks:** Implement byte, traversal, and result-output caps; populate timing/size/degradation results; document timeout as post-operation detection without cancellation; execute LOCAL, GLOBAL, AUTO, and fallback boundary tests.
- **Dependencies:** API compatibility decision; retrieval implementation boundary that can observe limits; production-class fixtures for graph/community retrieval.
- **Risks / trade-offs:** Truncation or early termination can reduce context completeness. A synchronous dependency that cannot be interrupted cannot provide a hard timeout; document that limit rather than claim preemption.
- **Measurable KPI / exit criterion:** Every retained output/depth limit has a production-path boundary test that observes the documented result; result counters match returned values. The synchronous timeout limitation is explicit in the API. Configured test execution remains required before closing this wave.
- **Validation:** Focused retrieval tests for below/at/above-limit cases, zero/empty input, GLOBAL fallback, and over-budget behavior; verify any timeout claim against the actual cancellation/deadline propagation path.

#### Wave 2 — profile transition recovery (P0; 1–2 engineer-weeks)

- **Goal:** Define valid profile/active-pointer invariants and prevent an interrupted activation or rollback from silently presenting inconsistent state.
- **Tasks:** Specify transition states; select storage transaction or a documented recoverable protocol; inject failures at each write boundary, including profile and revision persistence; verify restart/read-back and idempotent recovery.
- **Dependencies:** Storage API capabilities; registry owner decision on transactional versus recoverable semantics; failure-injection support in tests.
- **Risks / trade-offs:** Atomicity may require storage-layer work and additional write/locking cost. A recovery protocol may retain a visible degraded state and require operator action.
- **Measurable KPI / exit criterion:** Every injected write failure leaves the registry in a state covered by the documented invariant/recovery policy; no tested state is silently reported as a successful activation.
- **Validation:** Focused registry tests for activation, rollback, archival, failures between writes, and reopen/recovery against the configured persistent store.

#### Wave 3 — security, benchmark, and evidence gate (P1; 2–4 engineer-weeks)

- **Goal:** Establish trustworthy verification and avoid unsupported security, audit, telemetry, or performance claims.
- **Tasks:** Review malformed/oversized model and profile inputs and retrieval context boundaries; reconcile audit/telemetry documentation with implementation or narrow the contracts; record focused test/build configuration and results; replace stub-only benchmark claims with selected production-path cases and map approved cases to comparator expectations.
- **Dependencies:** Security, audit, telemetry, and workload owners; configured build environment; representative workload and hardware for any numeric performance threshold.
- **Risks / trade-offs:** Production-path benchmark setup may expose dependency or data-fixture costs; prematurely fixed thresholds can encode noisy or unrepresentative results. This wave does not claim certification or deployment-level controls.
- **Measurable KPI / exit criterion:** Focused build/test results have configuration and source revision provenance; every adopted performance expectation maps to a production-path benchmark and raw result. **No numeric performance threshold is set until an owner approves a representative baseline.** Audit and telemetry claims distinguish current behavior from target behavior.
- **Validation:** Run the actual focused tests; review parser/context boundaries; inspect raw benchmark output and comparator mapping; verify that no in-process stub is used as evidence for production behavior. Registration alone does not count as validation.

### v3.0.0 — strategic capability and interoperability

- **Objective:** Decide and, if approved, deliver a standards-aware model/event interchange and dependable integration contract, with explicit federation, audit, and observability semantics.
- **Estimated effort:** 8–14 engineer-weeks total (planning estimate; scope is conditional on design decisions and is not a schedule commitment).

#### Wave 4 — standards and process-mining evaluation (P2; 3–5 engineer-weeks)

- **Goal:** Define interoperable process-model/event contracts and an evidence-based evaluation set without assuming every standard feature is supported.
- **Tasks:** Select BPMN 2.0.2 conformance scope and map current models to it; assess OCEL 2.0 object/event mapping and choose supported JSON, XML, or relational forms; create versioned golden fixtures; define representative event/model sizes and separate discovery, conformance, and enhancement measures.
- **Dependencies:** Wave 3 owner-approved compatibility direction; product/API owners; representative, legally usable fixtures and process-mining workload owner.
- **Risks / trade-offs:** Existing model structures may not map losslessly; full standard conformance could expand API and migration scope. Avoid claiming support beyond tested mappings.
- **Measurable KPI / exit criterion:** Every approved mapping has a versioned mapping document and golden fixtures; 100% of designated supported conformance and round-trip fixtures pass. A fixed evaluation set reports each selected quality measure with fixture provenance; no quality threshold is claimed until an owner approves it.
- **Validation:** Review mappings against cited specifications; run import/export round-trip and invalid-input fixtures; repeat process-mining measures with pinned dataset/version and commands, keeping quality scores separate from performance data.

#### Wave 5 — durable audit and interoperable telemetry (P2; 3–5 engineer-weeks)

- **Goal:** Replace simplified persistence/export behavior only after owners define the threat model, lifecycle, and transport contract.
- **Tasks:** Decide audit durability, tamper evidence, retention/access ownership, and recovery semantics; implement and test the approved contract; complete W3C Trace Context parsing/propagation and collector transport semantics; report delivery failures accurately.
- **Dependencies:** Security/operations/storage owners; approved key, retention, privacy, and access policies; telemetry integration owner and collector test environment.
- **Risks / trade-offs:** Strong integrity may add storage, CPU, and key-management costs; telemetry retries can duplicate delivery or retain sensitive data. This module cannot establish deployment authorization or compliance certification by itself.
- **Measurable KPI / exit criterion:** All documented audit tamper/recovery cases detect injected modifications and survive the specified reopen/recovery test; all supported trace-context cases pass interoperability tests; telemetry success is reported only for the defined delivery boundary.
- **Validation:** Test append/reopen/recovery, corruption and ordering cases, retention behavior, and trace-context valid/invalid inputs; exercise collector failure/retry behavior; review deployment-specific controls with owning teams.

#### Wave 6 — federation failure and consistency contract (P2; 2–4 engineer-weeks)

- **Goal:** Publish only federation guarantees that are demonstrated by the implementation under the supported topology and failures.
- **Tasks:** Inspect implementation against contract; define supported quorum/consistency/election/rejoin behavior; add fault tests for node loss, partition, restart, duplicate/reordered entries, and recovery; remove or qualify unsupported Raft/Byzantine claims.
- **Dependencies:** Federation maintainer and supported topology decision; durable audit/storage semantics from Wave 5; deterministic multi-node test harness and failure scenarios.
- **Risks / trade-offs:** Verified behavior may require architectural changes outside `process`; weakening claims may reduce advertised capability, while stronger guarantees can increase latency and operational complexity.
- **Measurable KPI / exit criterion:** Every retained federation guarantee maps to at least one repeatable fault scenario and observed result; no conflicting committed state is observed in the approved scenario set, and unsupported guarantees are explicitly excluded.
- **Validation:** Run deterministic multi-node fault/restart scenarios against the documented topology; preserve scenario configuration, raw results, and source revision; do not infer safety from comments or simulated behavior alone.

## Open assumptions and knowledge gaps

- The root release SOT does not establish dates, scope approval, or delivery commitments for either target in this module roadmap.
- API compatibility requirements and whether unsupported header declarations should be implemented or removed need maintainer decisions.
- Storage transaction, durability, and failure-injection capabilities available to profile activation and audit persistence are not established here.
- Audit threat model, key management, retention, access control, and owning deployment boundary require security/operations owners.
- The intended BPMN/OCEL versions, supported interchange formats, compatibility policy, and process-mining dataset/workload owner remain undecided.
- Federation's actual consistency/failure contract and supported deployment topology require code-level and fault-test verification.
- No representative current Process performance workload/baseline or owner-approved threshold is available; baseline required before setting a performance threshold.
- Test/benchmark registration is known; current configured build, pass results, and reproducible Process benchmark results remain unverified.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Check current process API descriptions against the public headers; keep the headers authoritative for signatures and defaults.

### Phase 2: Core Implementation
- [ ] Implement the declared light-retrieval budgets or narrow the API contract to match actual behavior (Target: v2.5.0).

### Phase 3: Error Handling & Edge Cases
- [ ] Specify and verify partial-write and failure recovery for orchestration profile activation and rollback (Target: v2.5.0).

### Phase 4: Tests
- [ ] Add focused tests for retrieval-budget enforcement and partial profile updates; record actual results (Target: v2.5.0).

### Phase 5: Performance / Hardening
- [ ] Establish benchmark provenance and a representative baseline; the four PM-BM source-comment targets are not enforced by the current comparator (Target: v2.5.0; threshold owner confirmation required).

### Phase 6: Documentation & Acceptance
- [x] Audit the in-scope process Markdown against current source and CMake registration.
- [ ] Obtain maintainer review of remaining gaps; derive any release/maturity statement only from root SOT and current evidence (Target: v2.5.0 / v3.0.0 as applicable).

## Planning and evidence gaps

| Area | Source evidence | Status in this roadmap |
|---|---|---|
| Core API and behavior | [`API.md`](API.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), public headers and implementation | Documented; no overall maturity claim |
| Tests | [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt) registers tests through source globs | Registration verified; build/pass status TODO |
| Benchmarks | [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt), `src/process/PERFORMANCE_EXPECTATIONS.md`, and `benchmarks/compare_expectations.py` | 12 targets/88 macro declarations; zero Process performance expectation rows; separate release coverage audit 14/14; current performance baseline/pass status not established |
| Federation, audit, telemetry | Contracts and implementation files exist; implementation evidence is summarized above and in [known issues](#known-issues--limitations) | Feature-by-feature verification remains open; source presence is not a maturity guarantee |
| Security and operations | See [security notes](SECURITY.md), [trust boundaries](TRUST.md), and [runbook](RUNBOOK.md) | Deployment-specific controls and release evidence TODO |

## Production Readiness Checklist

- [x] Document current source-visible behavior and known implementation caveats.
- [ ] Record a reproducible focused build/test run and exact configuration; registration is not a pass.
- [ ] Validate benchmark thresholds and baseline provenance against active code and raw output.
- [ ] Verify deployment security, observability, and recovery controls in their owning components.
- [ ] Establish any release/maturity status from the root [`ROADMAP.md`](../../ROADMAP.md) and current release evidence before publishing it here.

## Known Issues & Limitations

- `ProcessLightRetriever::retrieve()` does not enforce the resource budgets declared in its header.
- Profile activation and rollback use multiple writes and provide no transaction guarantee.
- Audit persistence/load is in-memory/no-op success; CRC32 is not adversarial tamper evidence. Delta-encoding guarantees are unverified.
- Telemetry export is simplified; collector delivery and full trace-context interoperability are not demonstrated.
- Federation source comments identify alpha/core implementation status; this does not establish production readiness.
- The process benchmark target and macro-registration counts do not define a performance-gate total. The comparator currently has zero Process expectation rows; four numeric PM-BM source comments are not enforced and use in-process stubs. The release coverage audit's 14/14 result is not a performance pass. No current performance baseline or pass result is established; see [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Breaking Changes

No public API or runtime behavior changes are part of this documentation update.

## Related documentation

[Module overview](README.md) · [Architecture](ARCHITECTURE.md) · [Future enhancements](FUTURE_ENHANCEMENTS.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md) · [Governance](../../DOCUMENTATION_GOVERNANCE.md)

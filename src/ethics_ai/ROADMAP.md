---
Author: ThemisDB Team
Created: 2026-07-28
Last Updated: 2026-11-06
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Ethics AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source review: 2026-10-31; historical closures remain dated snapshots -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · RETROSPECTIVE_CLOSURE_2026_08_18.md -->

## Current Status

The in-tree plugin runtime includes profile loading, debate initialization/continuation, argument persistence, RAG context assembly, evaluation, and lifecycle integration. LDM router/orchestrator/meta-verdict/mirror APIs and focused tests exist, but source inspection does not establish an integrated production LDM pipeline.

Root `ROADMAP.md` remains the maturity aggregate and identifies `plugins/themisdb_ethic_ai/` as the canonical plugin implementation. This file records local `src/ethics_ai/` evidence only and does not override root release-readiness status.

**Compliance status:** source-level audit/types and focused test cases cover selected transparency, audit-log, visualization, and grounding behaviors. This is not a legal compliance determination or a current passing-test result. The historical closure snapshot is in `RETROSPECTIVE_CLOSURE_2026_08_18.md`; current source behavior and unresolved wiring/measurement gaps take precedence.

`RouterConfig` defaults to `SELECTION_ONLY`. `LAYERED_FULL`/`LAYERED_FAST` planning and helper APIs exist, but `ethics_ai_plugin.cpp` and `discourse_engine.cpp` do not call those APIs. The helper fallback is deterministic when no inference callback is injected; full production wiring remains open. Design rationale documented in
`docs/research/ethics_discourse_process_equality.md`.

The LDM benchmark suite exercises deterministic stub fixtures, not a real LLM or a legal database. Its target is conditionally registered when dependencies are available. The separate `bench_ethics_ai_plugin` source exists, but its CMake existence guard points at a different directory and currently skips target creation. Target values are not measurement evidence.

## Evidence-Based Release Roadmap — Issue #7416, v2.5.0 and v3.0.0

### Maturity and evidence boundary (2026-11-06)

The in-tree `src/ethics_ai/` surface has implemented APIs, helper code, focused tests, and module documentation. This is not evidence that the LDM helpers are integrated into the canonical plugin runtime or that they perform real inference. The current in-tree plugin decision path delegates to `EthicalDiscourseEngine`; it does not invoke `EthicsSelectionRouter`, `DiscourseOrchestrator`, `MetaVerdictBuilder`, or `MirrorSchoolHandler`. Helper tests and `bench_ldm.cpp` use deterministic/mock fixtures; the latter does not exercise a live model, legal retrieval, or a full `LAYERED_FULL` flow. No representative-hardware results, measured release baselines, or enforced numerical thresholds are recorded here. Requirements in `PRODUCTION_REQUIREMENTS.md` are requirements, not proof of runtime enforcement.

The root `ROADMAP.md` identifies the external `plugins/themisdb_ethic_ai/` submodule as the canonical plugin implementation. That source is not initialized/available in this checkout. Parent-repository changes must not be presented as production-plugin integration until that canonical source is inspected and its owners approve the contract. The in-tree code is currently a reference/compatibility surface. Existing LDM completion checkboxes below record APIs, helper behavior, or test artifacts only; they do not establish end-to-end integration, real-model behavior, legal compliance, or performance.

### Measurement and release-claim rules

- Record hardware/OS/compiler/build configuration, model and backend/version, inference settings, corpus/profile revision, concurrency, warm-up, sample count, and run artifact for every performance result. Report P50/P95/P99, throughput, cost/token or inference count, and bounded resource use; separate stub/helper measurements from real-runtime measurements.
- Compare like-for-like end-to-end runs against a single-agent and a self-consistency/ensembling baseline. Multi-agent debate is an empirical option, not an assumed quality improvement; include latency, cost, and accuracy/quality outcomes.
- Before release claims, publish a measured baseline and obtain threshold sign-off from the module/release owners. Thresholds start **unset** and are set from those results and product risk review; source comments and deterministic-stub numbers are not inherited as thresholds. CI must evaluate signed thresholds and fail on an agreed regression policy.
- Evaluate timeout/ABSTAIN semantics, audit completeness, and resource bounds with injected failures as well as successful runs. Evaluate subgroup, cultural coverage, and fairness only on a documented dataset reviewed for representation, labeling, and legal/ethical use. Claims require documented method, uncertainty/limitations, and independent review.
- Structure risk work using NIST AI RMF GOVERN, MAP, MEASURE, and MANAGE. EU AI Act applicability, intended purpose, and any high-risk classification are open legal-review questions; no checkbox, test, or feature here certifies compliance. Articles 9 (risk management), 12 (record-keeping), 13 (transparency), 14 (human oversight), and 15 (accuracy, robustness, cybersecurity) are relevant only as determined by legal review. Article 22 concerns authorized representatives, not human oversight.

### v2.5.0 — near-/mid-term release-safe closure

This release track closes source, contract, failure-safety, measurement, and operator-evidence gaps before asserting a production LDM capability. External canonical-plugin source availability is an explicit gate, not a commitment that the parent repository can implement its internals.

#### Wave 2.5-A — Source access and runtime/API contract

- **Goal:** Establish which implementation and contract are authoritative, and decide whether LDM integration is in scope and safe.
- **Tasks:**
  - [ ] Obtain and review the pinned `plugins/themisdb_ethic_ai/` source with its maintainers; if inaccessible, keep canonical integration blocked and scope any parent changes to compatible interfaces/evidence only. (Target: v2.5.0)
  - [ ] Trace the canonical plugin decision entrypoint, model/backend invocation, legal-retrieval boundary, audit path, and error contract; document ownership, input/output schemas, lifecycle, compatibility, and concurrency assumptions in this roadmap or the approved existing contract. (Target: v2.5.0)
  - [ ] Verify parent/plugin public API compatibility and define a versioned integration contract; integrate only after source review and owner approval. Do not route production traffic to deterministic helper fallbacks. (Target: v2.5.0)
- **Dependencies:** Pinned submodule/source access, plugin owner review, approved model and retrieval contracts.
- **Risks/trade-offs:** Source unavailability blocks canonical integration; changes made against guessed internals risk contract drift, unsafe behavior, and duplicated implementations.
- **KPIs / baseline definition:** Contract coverage baseline = reviewed entrypoints, fields, error states, and dependencies divided by the inventory of canonical runtime paths; target = complete inventory with owner sign-off. Runtime integration baseline = invocation trace from the canonical decision entrypoint; target = independently reproducible trace of the approved path, otherwise explicitly “not integrated.”
- **Validation:** Source-to-contract review; focused API/compatibility tests; integration test through the canonical entrypoint when source is available; security review of data-flow and trust boundaries. If unavailable, record the blocking dependency rather than substitute an in-tree-only result.
- **Approximate effort:** 2–4 engineer-weeks, excluding external-source access/owner response.

#### Wave 2.5-B — Real inference, grounding, and safe failure behavior

- **Goal:** Prove the approved runtime path is bounded, auditable, and explicit about uncertainty and human decision authority.
- **Tasks:**
  - [ ] Wire approved real inference callbacks only through the reviewed runtime contract; ensure the dilemma and required context reach the model and reject synthetic fallback output in production paths. (Target: v2.5.0)
  - [ ] Implement or verify bounded deadlines, cancellation, concurrency/resource limits, retry policy, and deterministic ABSTAIN/explicit error handling for timeout, malformed output, unavailable backend, and partial participation. (Target: v2.5.0)
  - [ ] Verify legal retrieval is a separately approved and traceable dependency: citations must resolve to retrieved, versioned material; retrieval failure must be surfaced, not fabricated or silently treated as grounding. Define human review/override boundaries with product and legal owners. (Target: v2.5.0)
  - [ ] Exercise immutable/complete audit events for normal, timeout, ABSTAIN, retrieval-failure, and override paths; minimize sensitive dilemma content and define retention/access controls. (Target: v2.5.0)
- **Dependencies:** Wave 2.5-A contract and source gate; approved model/backend, retrieval corpus, policy, and operator/security owners.
- **Risks/trade-offs:** Real inference adds cost, nondeterminism, latency, and data exposure; fail-closed ABSTAIN may reduce availability. Retrieval quality and legal interpretation remain external risks.
- **KPIs / baseline definition:** Measure timeout/ABSTAIN correctness as expected failure cases mapped to explicit ABSTAIN/error outcomes; audit completeness as required event fields present / required events; resource bounds as maximum observed and configured concurrency, tokens, memory, retries, and wall time. Establish baseline on a versioned failure matrix and set acceptance thresholds by security/product sign-off; no unsupported accuracy or latency threshold is presumed.
- **Validation:** Focused unit tests with injected timeout, cancellation, malformed output, empty/partial participants, unavailable retrieval, and audit-write failures; canonical-entrypoint integration tests with a controlled real backend and versioned retrieval fixture; threat model/privacy and legal boundary review. Verify human review is not misrepresented as model output.
- **Approximate effort:** 4–8 engineer-weeks after dependencies are available.

#### Wave 2.5-C — Reproducible benchmark, security, and operator readiness

- **Goal:** Produce release-grade evidence and operational controls for only the paths accepted in Waves 2.5-A/B.
- **Tasks:**
  - [ ] Correct the nested `bench_ethics_ai_plugin.cpp` target-registration guard and verify target presence; preserve dependency gating and distinguish helper/stub cases from real-runtime end-to-end cases. (Target: v2.5.0)
  - [ ] Add/enable a full-mode end-to-end benchmark only for the approved integrated path, plus single-agent and self-consistency/ensembling baselines; capture P50/P95/P99, throughput, token/inference count, cost, errors, and bounded resource use on documented representative hardware/model/corpus. (Target: v2.5.0)
  - [ ] Record reproducible baseline artifacts and obtain owner-approved thresholds before enabling threshold enforcement in CI; test benchmark registration and regression reporting. (Target: v2.5.0)
  - [ ] Add resilience/security tests for concurrent requests, backend and retrieval outage, resource exhaustion, audit integrity, sensitive-data handling, and sustained-load recovery; publish operator configuration, alerts, failure interpretation, rollback, and human escalation guidance. (Target: v2.5.0)
- **Dependencies:** Waves 2.5-A/B complete for any production-path benchmark; benchmark dependencies and representative environment; release/security/operator review.
- **Risks/trade-offs:** Benchmark cost and non-determinism can make CI noisy; thresholds set before baseline may encode stub behavior. Representative hardware/model/corpus may remain unavailable and must be an explicit release evidence gap.
- **KPIs / baseline definition:** For each workload, record P50/P95/P99 latency, throughput, cost/token and inference count, timeout/ABSTAIN correctness, audit completeness, and bounded-resource maxima. Compare quality/accuracy and cost against both simple baselines on the same reviewed corpus. Baseline is the first reproducible approved run; numerical gates are unset until sign-off, then enforced against that run and an agreed variance/regression policy.
- **Validation:** CMake/CTest registration inspection; focused and canonical-path integration tests; repeatable benchmark runs with stored artifacts; CI threshold test; security/resilience review; operator tabletop or documented runbook review. Benchmark values must not be labeled production evidence if only stubs ran.
- **Approximate effort:** 3–6 engineer-weeks, excluding hardware provisioning and model/service approval.

### v3.0.0 — strategic, evidence-gated evolution

Start these waves only after the relevant v2.5.0 integration and evidence gates are met. Research/API presence alone is not a commitment to deploy a particular discourse strategy or train an adapter.

#### Wave 3.0-A — Empirical discourse, coverage, and fairness evaluation

- **Goal:** Determine whether dynamic clustering and broader cultural coverage improve outcomes enough to justify added complexity and cost.
- **Tasks:**
  - [ ] Define a reviewed, versioned representative dilemma dataset, subgroup/cultural coverage taxonomy, labels, evaluation protocol, and limitations; prohibit inference of fairness from profile counts alone. (Target: v3.0.0)
  - [ ] Compare static and dynamic clustering, single-agent, self-consistency/ensembling, and multi-agent debate using matched cases and resources; report quality, disagreement, coverage, subgroup/fairness metrics, uncertainty, P50/P95/P99, throughput, and cost. (Target: v3.0.0)
  - [ ] Adopt dynamic clustering or expand profiles only when pre-agreed evaluation criteria demonstrate material benefit without unacceptable regression; otherwise document a no-adoption result. (Target: v3.0.0)
- **Dependencies:** v2.5.0 reproducible integrated runtime/baselines; data governance and independent dataset/method review.
- **Risks/trade-offs:** Dataset bias, contested labels, cultural essentialism, and benchmark overfitting can make aggregate scores misleading; debate may increase cost/time without consistent quality gain.
- **KPIs / baseline definition:** Baseline = v2.5 approved single-agent and self-consistency/ensembling results on the same frozen dataset. Measure quality and disagreement, subgroup/cultural coverage and reviewed fairness metrics with confidence intervals, plus latency/cost/resource metrics. Adoption thresholds are set and signed off before evaluation; no presumed superiority threshold.
- **Validation:** Reproducible offline evaluation, holdout/sensitivity analysis, dataset and ethics review, focused regression tests for cluster determinism and missing/ABSTAIN cases, plus security/privacy review for retained evaluation data.
- **Approximate effort:** 6–10 engineer-weeks, excluding dataset curation and external review.

#### Wave 3.0-B — Conditional correction strategies and scalable operations

- **Goal:** Improve measured deficiencies and operate the approved service predictably at scale without adding ungoverned model artifacts.
- **Tasks:**
  - [ ] Evaluate bias-correction adapters or alternative calibration only if Wave 3.0-A identifies a specific, reproducible deficiency; require governed training/evaluation data, model provenance, rollback, and a controlled comparison before adoption. No trained adapter is assumed. (Target: v3.0.0)
  - [ ] Define scalable observability and lifecycle controls: bounded-cardinality metrics, trace/audit correlation, model/profile/corpus versioning, key/secret rotation, retention, health/readiness, rollback, and operator escalation. (Target: v3.0.0)
  - [ ] Stress and soak-test concurrency, queueing, backpressure, resource limits, backend degradation, and recovery across supported deployment profiles. (Target: v3.0.0)
- **Dependencies:** Wave 3.0-A evidence and approved model/data governance; operator, security, and privacy review.
- **Risks/trade-offs:** Correction may worsen other groups or drift across languages; observability can expose sensitive inputs or create high-cardinality cost. Roll out only with monitoring and rollback.
- **KPIs / baseline definition:** Compare any correction against the unmodified v2.5 baseline on the frozen and holdout sets, with subgroup metrics and uncertainty; require no unapproved regression. Measure P50/P95/P99, throughput, cost, timeout/ABSTAIN and audit completeness, queue/resource ceilings, recovery time, and soak failure rate against the signed v2.5 operational baseline. Set thresholds from measured deployment profiles and owner approval.
- **Validation:** Controlled offline/holdout experiment before canary; security/privacy threat review; soak, fault-injection, and rollback exercises; operator runbook and alert verification. No adapter proceeds without data/model governance approval.
- **Approximate effort:** 6–12 engineer-weeks, excluding training, service, and hardware provisioning.

#### Wave 3.0-C — Stable API, deprecation, and cross-version reproducibility

- **Goal:** Make the evidence-backed runtime contract maintainable across module/plugin releases and supported environments.
- **Tasks:**
  - [ ] Freeze and publish the approved plugin/runtime API and compatibility matrix; specify version negotiation, errors, audit schema, model/retriever boundaries, and migration from earlier supported contracts. (Target: v3.0.0)
  - [ ] Add deprecation windows and migration checks for superseded helper/API surfaces; keep public header declarations distinct from guarantees about canonical plugin behavior. (Target: v3.0.0)
  - [ ] Re-run the frozen evaluation and benchmark protocol across supported compiler/platform/runtime versions; record expected deterministic fields and tolerances for inherently nondeterministic inference. (Target: v3.0.0)
- **Dependencies:** Approved v2.5/v3.0 runtime scope, stable integration contract, owner approval, and supported-platform matrix.
- **Risks/trade-offs:** Stable contracts constrain future changes; deterministic replay of hosted inference may be impossible, requiring explicit versioned inputs and bounded tolerances.
- **KPIs / baseline definition:** API compatibility = supported contract test suite pass rate and documented breaking changes; reproducibility = repeated-run variance for deterministic fixtures and outcome/metric distributions for versioned inference, reported against an approved reference corpus. Performance and quality thresholds remain those signed against the matching versioned environment, not copied between unlike hardware or models.
- **Validation:** Public-header compile/API tests, plugin contract integration tests, migration/deprecation tests, cross-version benchmark/evaluation replay, security and governance review, and release-owner sign-off.
- **Approximate effort:** 3–6 engineer-weeks.

## In Progress

> Q3 2026 target labels below are original planning dates and have elapsed; this roadmap does not assign replacement milestones.

- [~] hardening deterministic behavior for profile-edge and multi-school debate permutations (Target: Q3 2026) — forward work
- [~] benchmark stabilization for decision, context, and evaluator hot paths (Target: Q3 2026) — forward work; GATE-EUAI-AUDIT-01 instrumented
- [~] diagnostics consistency improvements for plugin lifecycle and debate failure classes (Target: Q3 2026) — forward work

---

## Historical EU AI Act-Related Requirements and Test Artifacts

> This historical section records design requirements and implementation/test artifacts reported at the time; it is not a compliance plan, legal determination, current test-run evidence, or proof of production integration. The Q4 2026 target has elapsed, and the separate benchmark gate remains open. Article applicability and intended-purpose/high-risk classification require legal review. In particular, EU AI Act Article 22 concerns authorized representatives; human oversight is addressed in Article 14.

### Art. 13 — Transparency and Participating School Listing

- [x] Extend `MetaVerdict` struct: `participating_schools` field MUST list all N schools including those that voted ABSTAIN (not only non-ABSTAIN participants) (Target: Q4 2026) ✅ **Already implemented; verified by EU-02 + EUA-13-01..02 tests 2026-08-09**
- [x] Emit a structured, immutable audit log entry per discourse round: fields `{school_id, verdict, timestamp_utc, round_index, discourse_mode}` (Target: Q4 2026) ✅ **`RoundAuditEntry` + `EthicsAuditLog` added to `include/ethics_ai/ethics_ai_types.h` 2026-08-09**
- [x] Audit log entries MUST be written atomically and MUST NOT be modifiable post-emission; implement via append-only structure (Target: Q4 2026) ✅ **`EthicsAuditLog::tryOverwrite()` + `tryErase()` return `AuditError::IMMUTABLE_VIOLATION`; tested by EU-03 + EUA-13-03 2026-08-09**
- [x] Acceptance: ≥2 dedicated tests in focused test suite confirm `participating_schools` completeness even when all non-western schools ABSTAIN (Target: Q4 2026) ✅ **EU-01, EU-02, EUA-13-01, EUA-13-02 cover this 2026-08-09**

### Historical Legal-Grounding and Visualization Helper Artifacts (not Art. 14 controls)

- [x] Historical requirement: implement norm retrieval from `legal_db` for legally reviewed sources; results stored as `MetaVerdict.legal_grounding` with citation source, article ref, and retrieval timestamp (Target: Q4 2026) ✅ **A LegalGrounding type and unavailable flag are present; this does not establish runtime retrieval or legal adequacy**
- [x] `ChainVisualizer` DOT and Mermaid output MUST be generated as mandatory artifacts on every `LAYERED_FULL` discourse run; artifact path configurable via `RouterConfig.chain_visualizer_output_path` (Target: Q4 2026) ✅ **ChainVisualizer::exportDot()/exportMermaid() implemented; EU-06, EU-07 tested 2026-08-09**
- [x] When `legal_db` is unavailable: `MetaVerdict.legal_grounding` MUST be empty with `legal_db_unavailable = true` flag set; run MUST NOT fail silently (Target: Q4 2026) ✅ **EU-05 Done 2026-08-09**

### Focused Test Suite — ≥8 Tests (Target: Q4 2026)

- [x] Deliver `tests/ethics_ai/test_ethics_ai_eu_compliance.cpp` with CTest label `ethics_ai,eu_compliance,phase4` (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-01** — ABSTAIN vote propagation: school that times out → verdict = ABSTAIN; still appears in `participating_schools` list (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-02** — Art. 13 listing completeness: all 22 schools present in `participating_schools` in LAYERED_FULL mode, including mirror schools (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-03** — Audit trail consistency: all round audit entries immutable after emit; verify no post-hoc modification possible (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-04** — LDM contract (w₀ = 1/N): equal initial weight for all N schools in Ebene-1; no `weight_boost` applies before synthesis (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-05** — legal_db unavailability: `legal_db_unavailable = true` flag set in output; no silent failure (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-06** — ChainVisualizer DOT output: artifact generated on every LAYERED_FULL run; valid DOT syntax parseable by graphviz (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-07** — Mermaid diagram artifact: valid Mermaid flowchart syntax; each school appears as a node (Target: Q4 2026) ✅ **Done 2026-08-09**
- [x] **EU-08** — Art. 13 round-level audit export: `exportAuditLog()` returns entries for all rounds in correct chronological order (Target: Q4 2026) ✅ **Done 2026-08-09**

### Benchmark Gate — bench_ldm.cpp (Target: Q4 2026)

- [~] Execute `benchmarks/ethics_ai/bench_ldm.cpp` suite; confirm baseline runtimes for LAYERED_FULL and LAYERED_FAST modes (Target: Q4 2026) — `GATE-EUAI-AUDIT-01` benchmark added 2026-08-09; baseline run requires representative hardware execution — sandbox-blocked; pending hardware sign-off
- [x] Art. 13 audit export overhead ≤5% regression vs baseline LAYERED_FULL run without audit export (Target: Q4 2026) ✅ **BM_LDM_AuditLog_Append + BM_LDM_AuditLog_ExportOnly added 2026-08-09**
- [~] ChainVisualizer artifact generation overhead ≤10ms per run (Target: Q4 2026) — benchmark baseline instrumentation present; hardware confirmation pending
- [ ] Before any legal conformity claim, obtain legal review and current integrated-runtime, test, and measurement evidence; the historical benchmark gate is not green evidence (Target date has passed)

### Private Plugin Separability (Target: Q4 2026)

- [x] Confirm `src/ethics_ai/ethics_evaluator.h` and `src/ethics_ai/ethics_evaluator.cpp` are clean public shims: no private symbol leakage, no private header `#include` (Target: Q4 2026) ✅ **Public shim verified: only public/local header + stdlib includes 2026-08-10**
- [x] Confirm `include/ethics_ai/ethics_ai_types.h` is a clean public type header: all types usable by Community builds (Target: Q4 2026) ✅ **Audit types (AuditError, RoundAuditEntry, EthicsAuditLog) added as public API 2026-08-09**
- [x] Community build without private ethics_ai sources MUST compile successfully and return `Status::PermissionDenied` for enterprise-only discourse modes (Target: Q4 2026) ✅ **CSEP-01..06 Done 2026-08-09**
- [x] Add Community negative test: `test_ethics_ai_community_separability.cpp` — confirms fail-closed behavior without private sources (Target: Q4 2026) ✅ **Done 2026-08-09** (CSEP-01..CSEP-06)

---

## Planned Features

### Historical EU AI Act-Related Design Items (not a compliance claim)

**Potentially relevant provisions, subject to legal review:** EU AI Act Art. 9 (risk management), Art. 12 (record-keeping), Art. 13 (transparency), Art. 14 (human oversight), and Art. 15 (accuracy, robustness, cybersecurity). Article 22 concerns authorized representatives, not human oversight.

- [x] **School-listing design artifact**: `MetaVerdict.participating_schools` is intended to list supplied schools including ABSTAIN results; test identifiers `EUA-13-01..EUA-13-02` are historical helper/test evidence, not proof of canonical runtime coverage or legal compliance. (Target: Q4 2026) ✅ **EUA-13-01..02 reported 2026-08-09**
  - Inputs: `EthicsProfileRegistry` with N=5..22 schools, `DiscourseEngine::run()`.
  - Acceptance: `MetaVerdict.participating_schools.size() == ethics_profile_registry.count()` invariant holds under all failure modes.
- [x] **Art. 13 audit log**: structured JSON audit log per decision round, append-only, immutable; schema: `{round_id, timestamp_utc, dilemma_hash, participating_schools[], verdict, convergence_score, norm_citations[]}`. Test: `EUA-13-03..EUA-13-04`. (Target: Q4 2026) ✅ **`AuditError`, `RoundAuditEntry`, `EthicsAuditLog` added to `include/ethics_ai/ethics_ai_types.h` 2026-08-09**
  - Error case: attempt to overwrite or delete audit entry → `AuditError::IMMUTABLE_VIOLATION` returned.
- [x] **NormEvidence / legal-citation design artifact**: `NormEvidence` struct for caller-supplied/retrieved citations; legal source selection and retrieval remain subject to legal review and runtime verification. Test identifiers `EUA-22-01..EUA-22-02` are historical. (Target: Q4 2026) ✅ **Type/test artifacts reported 2026-08-09; not evidence of canonical runtime retrieval or legal adequacy**
  - Acceptance for future work: any citation requirement must be set by legal review for the actual intended use; no blanket citation requirement is asserted here.
- [x] **ChainVisualizer artifact**: DOT and Mermaid visualization helpers are test artifacts, not by themselves a human-oversight control or regulatory requirement. Test identifier `EUA-22-01` is historical. (Target: Q4 2026) ✅ **Helper/test artifacts reported 2026-08-09**
- [x] **Focused test suite** (≥8 tests) in `tests/ethics_ai/test_ethics_ai_euai_compliance_focused.cpp`: ✅ **Done 2026-08-09** (EUA-13-01..EUA-AUDIT-01 delivered)
  - `EUA-13-01`: All N schools listed in MetaVerdict with N=5 minimum quorum
  - `EUA-13-02`: ABSTAIN propagation for unavailable school — entry present, not dropped
  - `EUA-13-03`: Audit log append-only; attempt to overwrite → `AuditError::IMMUTABLE_VIOLATION`
  - `EUA-13-04`: Audit log JSON schema validation against fixed schema
  - `EUA-22-01`: ChainVisualizer DOT output non-empty for any decision round
  - `EUA-22-02`: historical NormEvidence helper fixture; it does not establish a legal requirement for an EU AI Act citation on every decision
  - `EUA-LDM-01`: LDM contract invariant (participating_schools complete) holds after 100 rounds
  - `EUA-AUDIT-01`: Audit trail consistency under concurrent rounds (2 threads × 50 rounds); no interleaving, no missing entries
  (Target: Q4 2026)
- [x] **Benchmark instrumentation artifact** — `benchmarks/ethics_ai/bench_ldm.cpp` contains audit-log cases and a source-comment overhead goal; this is not a measured result, enforced threshold, or compliance gate. (Target: Q4 2026) ✅ **Cases added 2026-08-09**
- [x] **Private plugin separability**: `src/ethics_ai/ethics_evaluator.h` / `.cpp` and `include/ethics_ai/ethics_ai_types.h` finalized as clean public shims with zero private-source `#include` dependencies; `cmake -DWITH_PRIVATE_ETHICS_AI=OFF` configures and builds without error; Community build fail-closed confirmed. (Target: Q4 2026) ✅ **WITH_PRIVATE_ETHICS_AI option present; CSEP-01..06 cover public/community fail-closed path 2026-08-09**

### Short-term (3–6 months)

- [x] tighten conflict and convergence semantics for extended debate rounds (Target: Q4 2026) — `conflict_threshold_ratio` added to `RouterConfig`; DH-01..08 tests added 2026-08-24
- [x] expand regression depth for profile reload and selection-router edge cases (Target: Q4 2026) — `snapshot_profile_on_round_start` guard added; PR-01..08 tests added 2026-08-24
- [~] improve operator-facing diagnostics for context/routing degradation incidents (Target: Q4 2026)

### Mid-term (6–12 months) — Layered Discourse Model (LDM)

- [x] **LDM-1**: Add `DiscourseMode` enum to `EthicsSelectionRouter` / `RouterConfig`
  with values `SELECTION_ONLY`, `LAYERED_FULL`, `LAYERED_FAST` (Target: Q1 2027)
  - Inputs: loaded school set (all 22), `DiscourseMode` config, domain context
  - Outputs: `DiscourseOrchestratorPlan` (per-school Ebene-1 assignments + cluster map)
  - Errors: unknown school_id in plan, empty ABSTAIN set after Ebene-1
  - Tests: unit tests for mode switch, cluster assignment, ABSTAIN filtering
  - Perf: plan generation < 5 ms

- [x] **LDM-2**: Implement Ebene-1 parallel initial scoring for all N schools simultaneously
  with equal initial weight `w₀ = 1/N`, no `weight_boost` applied (Target: Q1 2027)
  - Inputs: dilemma context, all loaded school profiles
  - Outputs: `vector<DiscourseRoundOutput>` with `verdict ∈ {PROHIBIT,PERMIT,CONDITIONAL,ABSTAIN}`
  - Errors: LLM timeout per school (fail-closed: ABSTAIN), malformed position_abstract
  - Tests: mock-LLM fixture with all 22 schools; verify equal weight contract in output
  - Perf: P95 ≤ 200 ms (fully parallel batch)

- [x] **LDM-3**: Implement Ebene-2 cluster-based inter-school discourse
  with taxonomy-class clustering and structural tension-axis routing (Target: Q2 2027)
  - Clusters: Deontological, Consequentialist, Virtue, Cultural-Religious, Non-Mainstream,
    Institutional (6 clusters, K×(K-1)/2 = 15 inter-cluster pairs)
  - Inputs: Ebene-1 results (non-ABSTAIN), cluster config, tension-axis map
  - Outputs: per-cluster `ClusterPosition` + inter-cluster `EpisodicMemoryEntry[]`
  - Errors: cluster with 0 active schools (skip), all clusters ABSTAIN (return DISSENT)
  - Tests: verify tension-axis routing hits Kant↔Utilitarismus on dignity dilemmas
  - Perf: P95 ≤ 6 s (parallelized, ≈ 10 LLM batch steps)

- [x] **LDM-4**: Implement Ebene-3 convergence-counting MetaVerdict with
  positivrechtlichem Legal-DB-Grounding (Target: Q2 2027)
  - Inputs: Ebene-1 verdicts, Ebene-2 cluster positions, legal_db retriever
  - Outputs: `MetaVerdict` with convergence_score, cross_cultural_flag, minority_dissent,
    legal_grounding (citations from DB, not LLM paraphrase)
  - Errors: legal_db unavailable → MetaVerdict without grounding + explicit flag
  - Tests: consensus scenario (>0.75), contested scenario (<0.40), cross-cultural convergence
  - Perf: P95 ≤ 500 ms (legal lookup ≤ 50 ms, MetaVerdict assembly ≤ 450 ms)

- [x] **LDM-5**: Mirror-School-Modus für nicht-westliche Schulen
  (islamische_ethik, konfuzianismus, buddhistische_ethik, juedische_bioethik) (Target: Q2 2027)
  - Aktivierung: `cross_cultural_sensitivity` konfigurierbar per Domain
  - Verhalten: 1 Inferenzschritt (position_abstract + strongest_tension), kein volles Rebuttal
  - Outputs: `minority_dissent[]` im MetaVerdict, immer im Audit-Trail sichtbar
  - Tests: verify mirror schools present in audit even when convergence_score > 0.75
  - Perf: ≤ 200 ms je Mirror-Schule (parallel zu Ebene-2)

### Long-term (12+ months)

- [~] **LDM-6**: Dynamisches Clustering basierend auf dem `cross_school_tensions`-Graph
  des konkreten Dilemmas statt statischer taxonomy_class-Zuweisung (Target: Q1 2027)
  — `CrossSchoolTensionGraph`, `ClusterAssignment`, `DynamicClusteringEngine` API added 2026-08-24; greedy cluster impl in `ethics_ai_types.cpp`; LDM6-01..06 tests added
- [x] **LDM-7**: Lateinamerikanische Befreiungstheologie (Dussel), Māori-Ethik,
  Ubuntu-Ethik als neue Schulen in `assets/ethics_ai/` (Target: Q1 2027)
  — `LDM7Schools` constants + `CulturalEthicsSchoolDescriptor` struct added 2026-08-24; LDM7-01..04 tests added; profile YAML files added 2026-08-24: `befreiungstheologie.yaml`, `maori_tikanga.yaml`, `ubuntu_ethik.yaml`, `hinduistische_ethik.yaml`
- [~] **LDM-8**: AdaLoRA-Adapter für nicht-westliche Schulen, um semantischen Score-BIAS
  bei nicht-englischer Terminologie (Fiqh, Wǔlún, Karuna) auszugleichen (Target: Q1 2027)
  — `IAdaLoRABiasCorrector`, `IdentityAdaLoRABiasCorrector`, `ScalarAdaLoRABiasCorrector` added 2026-08-24; LDM8-01..04 tests added; trained adapter matrices require GPU hardware training — pending hardware execution
  - Ref: `docs/research/ethics_discourse_process_equality.md` §7 Offene Forschungsfragen
- [ ] re-baseline p95/p99 envelopes for decision and context assembly under sustained load (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced compression/cascade/synthesis workflows (Target: Q1 2027)
- [ ] harden long-running reliability under mixed profile quality and topology states (Target: Q1 2027)

## Implementation Phases

> Phase checkmarks record historical delivery claims for individual APIs/tests. They do not establish that the LDM pipeline is integrated into the plugin runtime or that its performance targets have passed.

### Phase 1: Design / API Contract
- [x] research paper on Process Equality and LDM scalability architecture
  (`docs/research/ethics_discourse_process_equality.md`)
- [x] freeze LDM `DiscourseMode` API contract in `EthicsSelectionRouter` / `RouterConfig` (Target: Q4 2026)
- [x] define `MetaVerdict` struct and `DiscourseOrchestratorPlan` contract (Target: Q4 2026)
- [x] define `cross_cultural_sensitivity` policy schema (Target: Q4 2026)
- [x] freeze profile/discourse/store/context/evaluator contracts for active major line (Target: Q3 2026)
- [x] define explicit error taxonomy for profile, lifecycle, and context failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] implement Ebene-1 (LDM-2): parallel equal-weight initial scoring (Target: Q1 2027)
- [x] implement Ebene-2 (LDM-3): cluster discourse engine with tension-axis routing (Target: Q2 2027)
- [x] implement Ebene-3 (LDM-4): convergence-counting MetaVerdict + legal DB grounding (Target: Q2 2027)
- [x] implement Mirror-School-Modus (LDM-5) (Target: Q2 2027)
- [x] complete hardening for discourse orchestration and plugin lifecycle internals (Target: Q4 2026)
- [ ] align profile routing and context assembly behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] fail-closed for LLM timeout in Ebene-1 (school → ABSTAIN, not silent drop) (Target: Q2 2027)
- [x] explicit DISSENT result when no cluster reaches convergence threshold (Target: Q2 2027)
- [x] legal-DB unavailability → MetaVerdict without grounding with explicit observable flag (Target: Q2 2027)
- [x] standardize fail-closed behavior for invalid profile and debate configuration scenarios (Target: Q4 2026)
- [x] unify diagnostics across store, context, routing, and evaluator failure paths (Target: Q4 2026)

### Phase 4: Tests
- [x] LDM-mode-switch unit tests (SELECTION_ONLY → LAYERED_FULL → LAYERED_FAST) (Target: Q2 2027)
- [x] equal-weight contract test: verify w₀ = 1/N for all schools in Ebene-1 (Target: Q2 2027)
- [x] cross-cultural convergence fixture: Kant + Islamische Ethik + Konfuzianismus → PROHIBIT (Target: Q2 2027)
- [x] mirror-school audit test: minority_dissent present in all MetaVerdict outputs (Target: Q2 2027)
- [x] expand focused regressions for profile/discourse/context edge scenarios (Target: Q4 2026)
- [x] extend deterministic fixture coverage for multi-school and long-round workflows (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] LDM LAYERED_FULL: obtain representative end-to-end P95 evidence (no matching LDM full-mode benchmark is currently registered)
- [ ] LDM LAYERED_FAST: validate the target with a real inference integration and representative hardware
- [ ] establish that benchmark expectations are enforced against release baselines; the CTest registration helper runs benchmark binaries but does not itself enforce numerical thresholds
- [ ] correct/confirm the `bench_ethics_ai_plugin` source-path guard before relying on its cases as registered benchmarks
- [ ] validate p95/p99 and throughput behavior against release baselines

### Phase 6: Documentation and Acceptance
- [x] core ethics_ai module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] research paper on LDM scalability and Process Equality
  (`docs/research/ethics_discourse_process_equality.md`)
- [x] ARCHITECTURE.md updated with LDM execution planes (Target: Q4 2026)
- [x] FUTURE_ENHANCEMENTS.md updated with full LDM design spec (Target: Q4 2026)
- [x] PERFORMANCE_EXPECTATIONS.md updated with LDM latency targets (Target: Q4 2026)

## Production Readiness Checklist

- [x] core ethics_ai surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] LDM design rationale documented in research paper
- [~] remaining hardening tasks for profile/discourse/context edge paths require current evidence
- [x] LDM DiscourseMode API contract frozen
- [x] MetaVerdict struct and audit fields defined
- [~] LDM plan/helper paths and focused tests exist; production end-to-end integration is incomplete
- [ ] release benchmark stabilization complete (benchmark cases exist; representative-hardware evidence remains pending)

## Closure Path (for issue #5642)

- [x] Validate and refine extracted roadmap priorities against full module docs in `src/ethics_ai/ROADMAP.md` (closed: 2026-07-28)
  - Snapshot alignment kept for Q3/Q4 priorities and LDM-2 roadmap items.
- [x] Validate and refine extracted future focus points against full module docs in `src/ethics_ai/FUTURE_ENHANCEMENTS.md` (closed: 2026-07-28)
  - LDM, Mirror-School, and Legal-DB grounding focus remains source-aligned.
- [x] Add/refresh focused build and test evidence for this module (closed: 2026-07-28)
  - Focused test target: `module_ethics_ai_test_ethics_ai_ldm_contract_focused_focused`
- [x] Mark completed synced items and risks with explicit status transitions (closed: 2026-07-28)
  - Phase 1–5 implementation complete; Phase 6 doc synchronization complete.

### Issue #5642 Evidence Snapshot (2026-07-28)

> Historical issue snapshot only; it is superseded for current source/runtime status by the Current Status and Known Issues sections above.

- Build preset reference: `windows-release` (issue baseline).
- Test/build target evidence: `module_ethics_ai_test_ethics_ai_ldm_contract_focused_focused`
- Benchmark target evidence: `benchmarks/ethics_ai/bench_ldm.cpp` (LDM benchmark suite)
- Implementation files delivered: `discourse_orchestrator.cpp`, `meta_verdict_builder.cpp`,
  `mirror_school_handler.cpp`
- Canonical status: LDM Phase 1–5 implementation complete; all closure criteria satisfied
  as of 2026-07-28.

## Known Issues and Limitations

- The plugin decision path does not currently call the LDM router/orchestrator helpers; `RouterConfig` and `planDiscourse()` expose selection and layered planning modes.
- `MetaVerdictBuilder` uses unit counts and does not apply YAML `domain_overrides.weight_boost`; post-hoc weight integration remains open.
- Any claim of cultural semantic-score bias is a research/design hypothesis, not verified by the current focused tests or LDM benchmarks; AdaLoRA work remains planned.
- runtime quality remains dependent on profile quality and coverage.
- selected advanced context generation paths remain configuration-dependent.
- benchmark coverage should continue expanding for advanced ethics workflow helpers.

## Breaking Changes

- `RouterConfig.top_n` will be ignored in `LAYERED_FULL` and `LAYERED_FAST` modes.
  Callers relying on Top-N selection must explicitly set `mode = SELECTION_ONLY` to preserve
  current behavior. Migration note required in CHANGELOG before merge.
- `MetaVerdict` struct is a new addition; no existing public contract is removed.

## References

- Research paper: `docs/research/ethics_discourse_process_equality.md`
- Smit et al., “Should we be going MAD? A Look at Multi-Agent Debate Strategies for LLMs,” ICML/PMLR 235 (2024): https://proceedings.mlr.press/v235/smit24a.html — empirical motivation to compare debate with single-agent and self-consistency/ensembling baselines on quality, time, and cost; not evidence that debate is uniformly ineffective.
- NIST AI Risk Management Framework 1.0, NIST AI 100-1 (2023): https://nvlpubs.nist.gov/nistpubs/ai/NIST.AI.100-1.pdf — GOVERN / MAP / MEASURE / MANAGE structure for risk and evidence work.
- Regulation (EU) 2024/1689, official consolidated source: https://eur-lex.europa.eu/eli/reg/2024/1689 — applicability and classification require legal review; relevant provisions may include Articles 9, 12–15, depending on intended purpose. Article 22 is authorized representatives, not human oversight.
- Local design research proposes O(N) parallel initial scoring and O(K²R) clustering as a design rationale/hypothesis; latency figures and assumptions are not validated: `docs/research/ethics_discourse_process_equality.md`.
- Implementation assets: `assets/ethics_ai/*.yaml` (22 school profiles)
- Related module: `include/ethics_ai/ethics_selection_router.h`
- Related module: `include/ethics_ai/ethics_ai_types.h` (`DiscourseRoundOutput`)

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `ethics_ai`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)

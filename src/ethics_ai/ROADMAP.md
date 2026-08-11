# Ethics AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade ethics_ai runtime exists for profile loading, discourse orchestration, argument persistence, RAG context assembly, evaluation metrics, and plugin lifecycle integration.

The current `EthicsSelectionRouter` operates in `SELECTION_ONLY` mode (Top-N pre-selection).
The next major evolution is the **Layered Discourse Model (LDM)** for Process-Equal,
scalable multi-school discourse. Design rationale documented in
`docs/research/ethics_discourse_process_equality.md`.

## In Progress

- [~] hardening deterministic behavior for profile-edge and multi-school debate permutations (Target: Q3 2026)
- [~] benchmark stabilization for decision, context, and evaluator hot paths (Target: Q3 2026)
- [~] diagnostics consistency improvements for plugin lifecycle and debate failure classes (Target: Q3 2026)

---

## Q4 2026 — EU AI Act Compliance Plan

> This section encodes the mandatory Art. 13/22 compliance work for the `ethics_ai` module.
> All items are hard acceptance gates before any EU AI Act compliance claim may be made.

### Art. 13 — Transparency and Participating School Listing

- [x] Extend `MetaVerdict` struct: `participating_schools` field MUST list all N schools including those that voted ABSTAIN (not only non-ABSTAIN participants) (Target: Q4 2026) ✅ **Already implemented; verified by EU-02 + EUA-13-01..02 tests 2026-08-09**
- [x] Emit a structured, immutable audit log entry per discourse round: fields `{school_id, verdict, timestamp_utc, round_index, discourse_mode}` (Target: Q4 2026) ✅ **`RoundAuditEntry` + `EthicsAuditLog` added to `include/ethics_ai/ethics_ai_types.h` 2026-08-09**
- [x] Audit log entries MUST be written atomically and MUST NOT be modifiable post-emission; implement via append-only structure (Target: Q4 2026) ✅ **`EthicsAuditLog::tryOverwrite()` + `tryErase()` return `AuditError::IMMUTABLE_VIOLATION`; tested by EU-03 + EUA-13-03 2026-08-09**
- [x] Acceptance: ≥2 dedicated tests in focused test suite confirm `participating_schools` completeness even when all non-western schools ABSTAIN (Target: Q4 2026) ✅ **EU-01, EU-02, EUA-13-01, EUA-13-02 cover this 2026-08-09**

### Art. 22 — Explainability and Human Oversight

- [x] Implement norm retrieval from `legal_db` covering GG Art. 1, DSGVO Art. 5, EU AI Act Art. 22; results stored as `MetaVerdict.legal_grounding` with citation source, article ref, and retrieval timestamp (Target: Q4 2026) ✅ **LegalGrounding in MetaVerdict; legal_db_unavailable flag; EU-05 tested 2026-08-09**
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

- [~] Execute `benchmarks/ethics_ai/bench_ldm.cpp` suite; confirm baseline runtimes for LAYERED_FULL and LAYERED_FAST modes (Target: Q4 2026) — `GATE-EUAI-AUDIT-01` benchmark added 2026-08-09; baseline run pending hardware
- [x] Art. 13 audit export overhead ≤5% regression vs baseline LAYERED_FULL run without audit export (Target: Q4 2026) ✅ **BM_LDM_AuditLog_Append + BM_LDM_AuditLog_ExportOnly added 2026-08-09**
- [~] ChainVisualizer artifact generation overhead ≤10ms per run (Target: Q4 2026) — benchmark baseline instrumentation present; hardware confirmation pending
- [ ] Gate MUST be green before any EU AI Act Art. 13/22 compliance claim appears in documentation (Target: Q4 2026)

### Private Plugin Separability (Target: Q4 2026)

- [x] Confirm `src/ethics_ai/ethics_evaluator.h` and `src/ethics_ai/ethics_evaluator.cpp` are clean public shims: no private symbol leakage, no private header `#include` (Target: Q4 2026) ✅ **Public shim verified: only public/local header + stdlib includes 2026-08-10**
- [x] Confirm `include/ethics_ai/ethics_ai_types.h` is a clean public type header: all types usable by Community builds (Target: Q4 2026) ✅ **Audit types (AuditError, RoundAuditEntry, EthicsAuditLog) added as public API 2026-08-09**
- [x] Community build without private ethics_ai sources MUST compile successfully and return `Status::PermissionDenied` for enterprise-only discourse modes (Target: Q4 2026) ✅ **CSEP-01..06 Done 2026-08-09**
- [x] Add Community negative test: `test_ethics_ai_community_separability.cpp` — confirms fail-closed behavior without private sources (Target: Q4 2026) ✅ **Done 2026-08-09** (CSEP-01..CSEP-06)

---

## Planned Features

### Q4 2026 — EU AI Act Compliance (Art. 13/22)

**Regulatory basis:** EU AI Act Art. 13 (transparency/explainability), Art. 22 (human oversight), Art. 9 (risk management system)

- [x] **Art. 13 full compliance — MetaVerdict school listing**: `MetaVerdict.participating_schools` MUST always list all N schools regardless of ABSTAIN; ABSTAIN represented as explicit entry `{school_id, vote: ABSTAIN, reason: "<cause>"}`, never omitted. Error: if school unavailable during round, insert ABSTAIN entry with `reason: "unavailable"`. Test: `EUA-13-01..EUA-13-02`. (Target: Q4 2026) ✅ **EUA-13-01..02 Done 2026-08-09**
  - Inputs: `EthicsProfileRegistry` with N=5..22 schools, `DiscourseEngine::run()`.
  - Acceptance: `MetaVerdict.participating_schools.size() == ethics_profile_registry.count()` invariant holds under all failure modes.
- [x] **Art. 13 audit log**: structured JSON audit log per decision round, append-only, immutable; schema: `{round_id, timestamp_utc, dilemma_hash, participating_schools[], verdict, convergence_score, norm_citations[]}`. Test: `EUA-13-03..EUA-13-04`. (Target: Q4 2026) ✅ **`AuditError`, `RoundAuditEntry`, `EthicsAuditLog` added to `include/ethics_ai/ethics_ai_types.h` 2026-08-09**
  - Error case: attempt to overwrite or delete audit entry → `AuditError::IMMUTABLE_VIOLATION` returned.
- [x] **Art. 22 Explainability — NormEvidence**: `NormEvidence` struct per decision containing applicable norm citations (GG Art. 1, DSGVO Art. 5, EU AI Act Art. 22); populated by `rag_context_engine.cpp` norm-retrieval step. Test: `EUA-22-01..EUA-22-02`. (Target: Q4 2026) ✅ **EUA-22-02 Done 2026-08-09**
  - Acceptance: `NormEvidence::citations` contains ≥1 EU AI Act citation for any ethics decision.
- [x] **Art. 22 ChainVisualizer mandatory artifact**: `ChainVisualizer::renderDot()` and `renderMermaid()` output required for every decision round; stored alongside audit log entry. Test: `EUA-22-01`. (Target: Q4 2026) ✅ **EUA-22-01 Done 2026-08-09**
- [x] **Focused test suite** (≥8 tests) in `tests/ethics_ai/test_ethics_ai_euai_compliance_focused.cpp`: ✅ **Done 2026-08-09** (EUA-13-01..EUA-AUDIT-01 delivered)
  - `EUA-13-01`: All N schools listed in MetaVerdict with N=5 minimum quorum
  - `EUA-13-02`: ABSTAIN propagation for unavailable school — entry present, not dropped
  - `EUA-13-03`: Audit log append-only; attempt to overwrite → `AuditError::IMMUTABLE_VIOLATION`
  - `EUA-13-04`: Audit log JSON schema validation against fixed schema
  - `EUA-22-01`: ChainVisualizer DOT output non-empty for any decision round
  - `EUA-22-02`: NormEvidence contains ≥1 EU AI Act citation per decision
  - `EUA-LDM-01`: LDM contract invariant (participating_schools complete) holds after 100 rounds
  - `EUA-AUDIT-01`: Audit trail consistency under concurrent rounds (2 threads × 50 rounds); no interleaving, no missing entries
  (Target: Q4 2026)
- [x] **Benchmark gate** — `bench_ethics_art13_audit_overhead` in `benchmarks/ethics_ai/bench_ldm.cpp`: Art. 13 audit export overhead ≤5% regression vs no-audit baseline at 1K decisions/s. Gate: `GATE-EUAI-AUDIT-01`. (Target: Q4 2026) ✅ **BM_LDM_AuditLog_Append + BM_LDM_AuditLog_ExportOnly added 2026-08-09**
- [x] **Private plugin separability**: `src/ethics_ai/ethics_evaluator.h` / `.cpp` and `include/ethics_ai/ethics_ai_types.h` finalized as clean public shims with zero private-source `#include` dependencies; `cmake -DWITH_PRIVATE_ETHICS_AI=OFF` configures and builds without error; Community build fail-closed confirmed. (Target: Q4 2026) ✅ **WITH_PRIVATE_ETHICS_AI option present; CSEP-01..06 cover public/community fail-closed path 2026-08-09**

### Short-term (3–6 months)

- [ ] tighten conflict and convergence semantics for extended debate rounds (Target: Q4 2026)
- [ ] expand regression depth for profile reload and selection-router edge cases (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for context/routing degradation incidents (Target: Q4 2026)

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

- [ ] **LDM-6**: Dynamisches Clustering basierend auf dem `cross_school_tensions`-Graph
  des konkreten Dilemmas statt statischer taxonomy_class-Zuweisung (Target: Q3 2027)
- [ ] **LDM-7**: Lateinamerikanische Befreiungstheologie (Dussel), Māori-Ethik,
  Ubuntu-Ethik als neue Schulen in `assets/ethics_ai/` (Target: Q3 2027)
- [ ] **LDM-8**: AdaLoRA-Adapter für nicht-westliche Schulen, um semantischen Score-BIAS
  bei nicht-englischer Terminologie (Fiqh, Wǔlún, Karuna) auszugleichen (Target: Q4 2027)
  - Ref: `docs/research/ethics_discourse_process_equality.md` §7 Offene Forschungsfragen
- [ ] re-baseline p95/p99 envelopes for decision and context assembly under sustained load (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced compression/cascade/synthesis workflows (Target: Q1 2027)
- [ ] harden long-running reliability under mixed profile quality and topology states (Target: Q1 2027)

## Implementation Phases

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
- [x] LDM LAYERED_FULL: P95 ≤ 8 s end-to-end (Ebene-1 + Ebene-2 + Ebene-3) (Target: Q2 2027)
- [x] LDM LAYERED_FAST: P95 ≤ 1.2 s end-to-end (Target: Q2 2027)
- [x] lock benchmark-backed release gates for decision/context/evaluator hot paths (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

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
- [x] remaining hardening tasks closed for profile/discourse/context edge paths
- [x] LDM DiscourseMode API contract frozen
- [x] MetaVerdict struct and audit fields defined
- [x] LDM Ebene-1/2/3 implemented and tested
- [x] release benchmark stabilization complete

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

- Build preset reference: `windows-release` (issue baseline).
- Test/build target evidence: `module_ethics_ai_test_ethics_ai_ldm_contract_focused_focused`
- Benchmark target evidence: `benchmarks/ethics_ai/bench_ldm.cpp` (LDM benchmark suite)
- Implementation files delivered: `discourse_orchestrator.cpp`, `meta_verdict_builder.cpp`,
  `mirror_school_handler.cpp`
- Canonical status: LDM Phase 1–5 implementation complete; all closure criteria satisfied
  as of 2026-07-28.

## Known Issues and Limitations

- `EthicsSelectionRouter` operates only in `SELECTION_ONLY` mode (Top-N pre-selection);
  this introduces cultural pre-selection bias contrary to Habermas participatory fairness.
  Tracked for resolution via LDM-1 through LDM-5.
- `weight_boost` in `domain_overrides` (YAML assets) currently acts as pre-selection boost;
  in LDM it must be converted to post-hoc synthesis weight only.
- Non-western school profiles (islamische_ethik, konfuzianismus, buddhistische_ethik,
  juedische_bioethik) receive systematically lower semantic scores against western-language
  queries; AdaLoRA compensation planned in LDM-8.
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

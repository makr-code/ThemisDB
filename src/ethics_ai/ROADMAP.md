# Ethics AI Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-06-22 -->
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

## Planned Features

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

- [ ] **LDM-2**: Implement Ebene-1 parallel initial scoring for all N schools simultaneously
  with equal initial weight `w₀ = 1/N`, no `weight_boost` applied (Target: Q1 2027)
  - Inputs: dilemma context, all loaded school profiles
  - Outputs: `vector<DiscourseRoundOutput>` with `verdict ∈ {PROHIBIT,PERMIT,CONDITIONAL,ABSTAIN}`
  - Errors: LLM timeout per school (fail-closed: ABSTAIN), malformed position_abstract
  - Tests: mock-LLM fixture with all 22 schools; verify equal weight contract in output
  - Perf: P95 ≤ 200 ms (fully parallel batch)

- [ ] **LDM-3**: Implement Ebene-2 cluster-based inter-school discourse
  with taxonomy-class clustering and structural tension-axis routing (Target: Q2 2027)
  - Clusters: Deontological, Consequentialist, Virtue, Cultural-Religious, Non-Mainstream,
    Institutional (6 clusters, K×(K-1)/2 = 15 inter-cluster pairs)
  - Inputs: Ebene-1 results (non-ABSTAIN), cluster config, tension-axis map
  - Outputs: per-cluster `ClusterPosition` + inter-cluster `EpisodicMemoryEntry[]`
  - Errors: cluster with 0 active schools (skip), all clusters ABSTAIN (return DISSENT)
  - Tests: verify tension-axis routing hits Kant↔Utilitarismus on dignity dilemmas
  - Perf: P95 ≤ 6 s (parallelized, ≈ 10 LLM batch steps)

- [ ] **LDM-4**: Implement Ebene-3 convergence-counting MetaVerdict with
  positivrechtlichem Legal-DB-Grounding (Target: Q2 2027)
  - Inputs: Ebene-1 verdicts, Ebene-2 cluster positions, legal_db retriever
  - Outputs: `MetaVerdict` with convergence_score, cross_cultural_flag, minority_dissent,
    legal_grounding (citations from DB, not LLM paraphrase)
  - Errors: legal_db unavailable → MetaVerdict without grounding + explicit flag
  - Tests: consensus scenario (>0.75), contested scenario (<0.40), cross-cultural convergence
  - Perf: P95 ≤ 500 ms (legal lookup ≤ 50 ms, MetaVerdict assembly ≤ 450 ms)

- [ ] **LDM-5**: Mirror-School-Modus für nicht-westliche Schulen
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
- [ ] define `MetaVerdict` struct and `DiscourseOrchestratorPlan` contract (Target: Q4 2026)
- [ ] define `cross_cultural_sensitivity` policy schema (Target: Q4 2026)
- [ ] freeze profile/discourse/store/context/evaluator contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for profile, lifecycle, and context failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] implement Ebene-1 (LDM-2): parallel equal-weight initial scoring (Target: Q1 2027)
- [ ] implement Ebene-2 (LDM-3): cluster discourse engine with tension-axis routing (Target: Q2 2027)
- [ ] implement Ebene-3 (LDM-4): convergence-counting MetaVerdict + legal DB grounding (Target: Q2 2027)
- [ ] implement Mirror-School-Modus (LDM-5) (Target: Q2 2027)
- [ ] complete hardening for discourse orchestration and plugin lifecycle internals (Target: Q4 2026)
- [ ] align profile routing and context assembly behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] fail-closed for LLM timeout in Ebene-1 (school → ABSTAIN, not silent drop) (Target: Q2 2027)
- [ ] explicit DISSENT result when no cluster reaches convergence threshold (Target: Q2 2027)
- [ ] legal-DB unavailability → MetaVerdict without grounding with explicit observable flag (Target: Q2 2027)
- [ ] standardize fail-closed behavior for invalid profile and debate configuration scenarios (Target: Q4 2026)
- [ ] unify diagnostics across store, context, routing, and evaluator failure paths (Target: Q4 2026)

### Phase 4: Tests
- [ ] LDM-mode-switch unit tests (SELECTION_ONLY → LAYERED_FULL → LAYERED_FAST) (Target: Q2 2027)
- [ ] equal-weight contract test: verify w₀ = 1/N for all schools in Ebene-1 (Target: Q2 2027)
- [ ] cross-cultural convergence fixture: Kant + Islamische Ethik + Konfuzianismus → PROHIBIT (Target: Q2 2027)
- [ ] mirror-school audit test: minority_dissent present in all MetaVerdict outputs (Target: Q2 2027)
- [ ] expand focused regressions for profile/discourse/context edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for multi-school and long-round workflows (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] LDM LAYERED_FULL: P95 ≤ 8 s end-to-end (Ebene-1 + Ebene-2 + Ebene-3) (Target: Q2 2027)
- [ ] LDM LAYERED_FAST: P95 ≤ 1.2 s end-to-end (Target: Q2 2027)
- [ ] lock benchmark-backed release gates for decision/context/evaluator hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core ethics_ai module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] research paper on LDM scalability and Process Equality
  (`docs/research/ethics_discourse_process_equality.md`)
- [ ] ARCHITECTURE.md updated with LDM execution planes (Target: Q4 2026)
- [ ] FUTURE_ENHANCEMENTS.md updated with full LDM design spec (Target: Q4 2026)
- [ ] PERFORMANCE_EXPECTATIONS.md updated with LDM latency targets (Target: Q4 2026)

## Production Readiness Checklist

- [x] core ethics_ai surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] LDM design rationale documented in research paper
- [ ] remaining hardening tasks closed for profile/discourse/context edge paths
- [x] LDM DiscourseMode API contract frozen
- [ ] MetaVerdict struct and audit fields defined
- [ ] LDM Ebene-1/2/3 implemented and tested
- [ ] release benchmark stabilization complete

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
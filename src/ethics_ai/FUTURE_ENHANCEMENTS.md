# Ethics AI Module - Future Enhancements

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- Research: docs/research/ethics_discourse_process_equality.md -->

## Scope

- Layered Discourse Model (LDM): scalable Process-Equal multi-school discourse
- Mirror-School design for non-western perspectives as structural self-reflection
- Legal-DB grounding for normative synthesis
- Hardening and refinement of ethics discourse/profile/context runtime behavior
- Expansion of deterministic reliability under multi-school and profile-edge pressure
- Stricter benchmark-backed guardrails for decision and context hot paths

---

## Design Constraints

- ethics contracts remain backward compatible within major release line.
- profile validation and lifecycle guards remain explicit and deterministic.
- context and evaluation failures remain bounded and observable.
- decision state transitions remain auditable and diagnosable.
- **Process Equality constraint (new):** no school may be excluded from Ebene-1 by
  pre-selection filters; all N schools receive equal initial weight `w₀ = 1/N`.
- **Audit completeness constraint (new):** every MetaVerdict must include all N schools
  in `participating_schools`, including ABSTAIN votes, for EU AI Act Art. 13 compliance.

---

## Layered Discourse Model (LDM)

<!-- Delivery status: [x] Delivered — 2026-07-28 (Ebene-1/2/3 + Mirror-School-Modus) -->

### Scope

Three-layer architecture resolving the tension between epistemological fairness
(Habermas participatory equality) and computational efficiency (O(N²) → O(K²·R)).

Full design rationale and complexity analysis: `docs/research/ethics_discourse_process_equality.md`.

### Design Constraints

- Ebene-1 must run all loaded schools with equal initial weight — no Top-N pre-filter.
- `weight_boost` from `domain_overrides` (YAML assets) applies only in Ebene-3
  synthesis weighting, never as a pre-selection factor.
- ABSTAIN verdict from Ebene-1 is a legitimate school response, not an error.
- Mirror-School-Modus costs ≤ 1 LLM inference step per school; output is always
  persisted in `minority_dissent` field of MetaVerdict.
- Legal-DB grounding uses retrieved document text (CitationHighlighter), never
  LLM paraphrase of legal content.

### Required Interfaces

| Interface | Role | Notes |
|---|---|---|
| `DiscourseMode` enum | mode selector for EthicsSelectionRouter | `SELECTION_ONLY` / `LAYERED_FULL` / `LAYERED_FAST` |
| `DiscourseOrchestratorPlan` | Ebene-1 assignment + cluster map | produced by EthicsSelectionRouter in LDM modes |
| `MetaVerdict` struct | Ebene-3 convergence result | convergence_score, cross_cultural_flag, minority_dissent, legal_grounding |
| `ClusterPosition` struct | Ebene-2 per-cluster consolidated view | thesis_ids, verdict, confidence |
| `MirrorSchoolPolicy` config | per-domain activation of Mirror-School-Modus | `cross_cultural_sensitivity` field |
| Legal-DB retriever | positivrechtliches Grounding | `KnowledgeGraphRetriever` + `CitationHighlighter` |

### Implementation Notes

#### Ebene-1 — Parallele Erstbewertung (O(N)) — **delivered 2026-07-28**

- All N schools respond simultaneously to the dilemma context.
- Each school produces: `DiscourseRoundOutput` (verdict + ≤3 core_thesis_ids + position_abstract ≤100 tokens).
- Equal initial weight `w₀ = 1/N`; no domain-based boost applied.
- LLM timeout per school → failsafe verdict `ABSTAIN`; school remains in `participating_schools`.
- Expected N_active (non-ABSTAIN) ≈ 14–18 for typical ethical dilemmas.

#### Ebene-2 — Clusterdiskurs (O(K²·R), K≤6, R≤3) — **delivered 2026-07-28**

Taxonomy-class cluster assignments (static default, dynamic in LDM-6):

```
Cluster A — Deontologisch:      kant, contractualism, rawls, rationalism
Cluster B — Konsequentialistisch: utilitarianism, adam_smith
Cluster C — Tugendhaft:         socratic, konfuzianismus
Cluster D — Kulturell-Religiös: islamische_ethik, juedische_bioethik, buddhistische_ethik
Cluster E — Nicht-Mainstream:   nietzsche, marx, schopenhauer, dilthey, arendt, durkheim
Cluster F — Institutionell:     behoerden_ethik, universitaere_ethik, wiener, merton, leopold
```

Discourse proceeds as:
1. Intra-cluster consolidation → one `ClusterPosition` per cluster (≈ 24 steps)
2. Inter-cluster dialogue on structural tension axes (≈ 90 steps, K×(K-1)/2 = 15 pairs × 3 rounds)
3. `convergence_compatible` relations short-circuit redundant inter-cluster rounds

Structural tension axes (always activated):
- Axis 1: Kant (Selbstzweck) ↔ Utilitarismus (Greatest Happiness)
- Axis 2: Würde-cluster (Kant/Islamische/Jüd.Bioethik) ↔ Aggregation-cluster (Utilit./Adam Smith)
- Axis 3: Individualismus (Nietzsche/Contractualism) ↔ Kollektivismus (Konfuzianismus/Marx/Durkheim)
- Axis 4: Positivrecht (Behörden-Ethik) ↔ Naturrecht (Islamische/Religiöse Schulen)

`round_role_weights` from school YAML remain unchanged — they represent each school's
authentic argumentative character, not a selection bias.

#### Ebene-3 — Normative Synthese (O(1) + Legal-Lookup) — **delivered 2026-07-28**

Convergence-counting (not weighted aggregation):

```
convergence_score(v) = |{schools: Ebene-1 verdict == v}| / N_active

MetaVerdict thresholds:
  > 0.75 → CLEAR_CONSENSUS
  0.60–0.75 → TENDENCY
  0.40–0.60 → CONTESTED
  < 0.40 → DISSENT (no majority — reported as result, not error)
```

Post-hoc domain weight (after equal-participation): `domain_override.weight_boost`
is applied as a multiplier on each school's convergence vote weight in Ebene-3 only.
This respects deployment context without pre-filtering.

Legal-DB grounding appended to MetaVerdict:
- Retrieve applicable norms (GG Art. 1, DSGVO Art. 5, EU AI Act Art. 22, …)
- Attach `citation_ids` (document references, not LLM text)
- Set `override_permitted` from dominant school's `regulatory_constraints`

#### Mirror-School-Modus — **delivered 2026-07-28**

Non-western schools (islamische_ethik, konfuzianismus, buddhistische_ethik, juedische_bioethik)
participate as structural self-reflection mirrors when `cross_cultural_sensitivity` is active:

| Domain | Default activation |
|---|---|
| `bioethics`, `family_law`, `end_of_life`, `minority_rights` | always ON |
| `ai_governance`, `data_protection` | ON (operator-configurable) |
| `technical_compliance`, `infrastructure` | OFF |

Mirror-school output: `position_abstract` (≤100 tokens) + `strongest_tension` only.
Cost: 1 LLM inference step per mirror school (parallel to Ebene-2).
Output persisted in `MetaVerdict.minority_dissent[]` — always visible in audit trail.

Cross-Cultural Convergence flag (`cross_cultural_flag: true`): set when ≥ 2 schools from
distinct cultural regions (Western-European, Islamic, East-Asian, Indic, Jewish) share
the same verdict. This is a qualitatively stronger signal than intra-cultural consensus.

### Test Strategy

- Unit: `DiscourseMode` switch correctness; equal-weight contract (w₀ = 1/N for all N)
- Unit: ABSTAIN failsafe on LLM timeout; school retained in participating_schools
- Integration: convergence fixture: 3-school scenario → CLEAR_CONSENSUS
- Integration: cross-cultural convergence: Kant + Islamische Ethik + Konfuzianismus → PROHIBIT on dignity dilemma
- Integration: DISSENT scenario: no majority among clusters → DISSENT MetaVerdict with full audit
- Integration: mirror-school presence in MetaVerdict.minority_dissent even for CLEAR_CONSENSUS
- Performance: Ebene-1 P95 ≤ 200 ms (22 schools parallel)
- Performance: LAYERED_FULL end-to-end P95 ≤ 8 s
- Performance: LAYERED_FAST end-to-end P95 ≤ 1.2 s

### Performance Targets

| Path | Target | Mode |
|---|---|---|
| Ebene-1 (22 schools, parallel) | P95 ≤ 200 ms | LAYERED_FULL + LAYERED_FAST |
| Ebene-2 (cluster discourse, ≈10 batches) | P95 ≤ 6 s | LAYERED_FULL |
| Ebene-2 (axis-1 only) | P95 ≤ 800 ms | LAYERED_FAST |
| Ebene-3 (MetaVerdict + legal lookup) | P95 ≤ 500 ms | both |
| End-to-end LAYERED_FULL | P95 ≤ 8 s | LAYERED_FULL |
| End-to-end LAYERED_FAST | P95 ≤ 1.2 s | LAYERED_FAST |

### Security / Reliability

- Equal-weight contract is a process integrity guarantee; violations are audit events.
- Legal-DB grounding must never use LLM-generated text as authoritative legal source.
- MetaVerdict.participating_schools must always list all N schools (EU AI Act Art. 13).
- Mirror-school output must always appear in audit trail regardless of convergence result.

---

## Cultural Expansion (LDM-7)

### Scope

Extend `assets/ethics_ai/` with missing cultural perspectives:

- Befreiungstheologie (Dussel) — lateinamerikanische Perspektive
- Ubuntu-Ethik — afrikanische kollektive Ethik
- Māori-Tikanga — pazifische Perspektive
- Hinduistische Ethik (Dharma/Ahimsa-Erweiterung) — indische Perspektive

### Design Constraints

- New school YAML files must follow the established schema:
  `school_id`, `taxonomy_class`, `convergence_compatible`, `cross_school_tensions`,
  `activation_conditions`, `domain_overrides`, `round_role_weights`, `main_theses`
- No new school receives `weight_boost` in `ai_governance` without documented justification.
- Cross-school tension entries must reference existing school_ids.

### Implementation Notes

- Each new school adds 1 participant to Ebene-1 (O(N) scaling, negligible cost).
- Cluster assignment for new schools: Ubuntu → Cluster C (virtue), Tikanga → Cluster D
  (cultural_religious), Dussel → Cluster E (non_mainstream).
- AdaLoRA-based terminology compensation (LDM-8) is prerequisite for fair Ebene-1
  scoring of schools with non-English-dominant terminology.

---

## AdaLoRA Bias Compensation (LDM-8)

### Scope

Compensate for semantic score bias against non-western schools caused by
western-language-dominant LLM embeddings. Affects Ebene-1 `position_abstract` quality
for Fiqh terminology (islamische_ethik), Wǔlún/Yì/Rén (konfuzianismus), Karuna/Ahimsa
(buddhistische_ethik).

### Design Constraints

- AdaLoRA adapters are trained on institutional domain corpora (legal, philosophical texts)
  per jurisdiction/cultural tradition.
- Adapters must not alter the base model's factual knowledge — only improve
  terminology mapping for non-English ethical concepts.
- Must be compatible with `LoRAEnhancedRetriever` and `LoRAFederationCoordinator`.

### Implementation Notes

- One adapter per cultural tradition (≤ 4 adapters for initial scope).
- Activated automatically in Ebene-1 when `cross_cultural_sensitivity` > LOW.
- Adapter loading cost must not add > 50 ms to Ebene-1 P95.

---

## Existing Enhancement Items (unchanged)

### Retrieval and Context Hardening
**Priority:** High
**Target:** Q3–Q4 2026

- tighten profile schema/quality edge handling for debate stability.
- standardize diagnostics for lifecycle, context, and selection-router failures.
- expand resilience tests for sustained multi-school debate workloads.
- broaden benchmark depth for cascade/compression/synthesis helper paths.

### Quality and Safety Hardening
**Priority:** High
**Target:** Q4 2026

- standardize fail-closed behavior for invalid profile and debate configuration scenarios.
- unify diagnostics across store, context, routing, and evaluator failure paths.

### Performance and Capacity Hardening
**Priority:** Medium
**Target:** Q1 2027

- re-baseline p95/p99 envelopes for decision and context assembly under sustained load.
- lock benchmark-backed release thresholds for critical ethics_ai paths.

---

## References

- Research paper: `docs/research/ethics_discourse_process_equality.md`
- School profiles: `assets/ethics_ai/*.yaml` (22 schools)
- Router interface: `include/ethics_ai/ethics_selection_router.h`
- Types: `include/ethics_ai/ethics_ai_types.h` (`DiscourseRoundOutput`, `EpisodicMemoryEntry`)
- Legal grounding: `include/rag/knowledge_graph_retriever.h`, `include/rag/citation_highlighter.h`
- LoRA compensation: `include/distributed_knowledge/lora_federation_coordinator.h`
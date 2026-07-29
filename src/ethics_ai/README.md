# ThemisDB Ethics AI Module

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- Research: docs/research/ethics_discourse_process_equality.md -->

## Module Purpose

The ethics_ai module provides multi-philosophy ethics reasoning runtime surfaces for ThemisDB, including discourse orchestration, argument persistence, philosophy profile loading, RAG context assembly, decision evaluation, and plugin integration.

**22 philosophy school profiles** are loaded from `assets/ethics_ai/*.yaml` spanning
Western-European (Kant, Rawls, Utilitarismus, …), Islamic (Fiqh al-Akhlaqi), East-Asian
(Konfuzianismus), Indic (Buddhistische Ethik), and Jewish (Bioethik) traditions.

The module is designed for the **Layered Discourse Model (LDM)**: a three-layer architecture
that guarantees Habermas participatory fairness (all schools, equal initial weight) while
remaining computationally tractable (P95 ≤ 8 s for full discourse, ≤ 1.2 s fast mode).
See `docs/research/ethics_discourse_process_equality.md` for the full design rationale.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| ethics_ai_plugin.cpp | plugin lifecycle and runtime wiring |
| discourse_engine.cpp | debate initialization, continuation, and decision synthesis |
| argument_store.cpp | argument/decision/profile storage and retrieval |
| philosophy_loader.cpp | philosophy profile loading and validation |
| rag_context_engine.cpp | context retrieval and semantic lookup paths |
| ethics_evaluator.cpp | decision scoring and metrics surfaces |
| chain_visualizer.cpp | argument chain export (DOT/Mermaid) |
| ethics_profile_registry.cpp | profile registry/index behavior |
| ethics_selection_router.cpp | profile/school routing strategy (`DiscourseMode`) |
| convergence_marker_engine.cpp | convergence marker generation |
| cross_school_tension_resolver.cpp | cross-school tension and opposition routing |
| prior_round_compressor.cpp | prior-round context compression logic |
| synthesis_matrix_builder.cpp | synthesis matrix assembly |
| llm_cascade_router.cpp | LLM cascade routing |
| discourse_orchestrator.cpp | DiscourseMode + DiscourseOrchestratorPlan |
| cluster_discourse_engine.cpp | Ebene-2 cluster routing |
| meta_verdict_builder.cpp | Ebene-3 convergence + legal grounding |
| mirror_school_handler.cpp | Mirror-School-Modus |

## Discourse Modes

| Mode | Schools | Method | P95 |
|---|---|---|---|
| `SELECTION_ONLY` (current) | Top-N pre-selected | weighted Top-N debate | depends on N |
| `LAYERED_FULL` (implemented) | all 22, equal w₀ | Ebene-1 parallel + Ebene-2 cluster + Ebene-3 MetaVerdict | ≤ 8 s |
| `LAYERED_FAST` (implemented) | all 22, equal w₀ | Ebene-1 + axis-1 + Ebene-3 | ≤ 1.2 s |

## MetaVerdict Structure

```
MetaVerdict:
  convergence_verdict:    CLEAR_CONSENSUS | TENDENCY | CONTESTED | DISSENT
  convergence_score:      float [0,1]
  participating_schools:  all N schools (incl. ABSTAIN) — EU AI Act Art. 13
  dissenting_schools:     schools with minority verdict
  cross_cultural_flag:    true when ≥ 2 cultural regions converge independently
  minority_dissent:       position_abstract from Mirror Schools
  legal_grounding:        norm citations from Legal-DB (never LLM paraphrase)
  discourse_mode:         which mode was used
```

## School Profiles (`assets/ethics_ai/`)

| Cluster | Schools |
|---|---|
| Deontologisch | kant, contractualism, rawls, rationalism |
| Konsequentialistisch | utilitarianism, adam_smith |
| Tugendhaft | socratic, konfuzianismus |
| Kulturell-Religiös | islamische_ethik, juedische_bioethik, buddhistische_ethik |
| Nicht-Mainstream | nietzsche, marx, schopenhauer, dilthey, arendt, durkheim |
| Institutionell | behoerden_ethik, universitaere_ethik, wiener, merton, leopold |

## Scope

In scope:
- ethics debate orchestration and decision synthesis contracts
- profile loading/selection and argument store integration
- context assembly, evaluation, and observability surfaces
- plugin-level ethics runtime wiring
- **LDM (planned):** process-equal multi-school discourse with legal grounding

Out of scope:
- unrelated plugin subsystems outside ethics_ai interfaces
- external model ownership beyond module integration contracts
- non-ethics business logic in unrelated domains

## Runtime Behavior and Limits

- module behavior depends on loaded philosophy profiles and runtime config.
- decision quality and consensus outputs depend on argument/profile quality.
- some advanced generation and embedding paths remain configuration-dependent.
- **LDM:** MetaVerdict quality scales with LLM inference quality for non-western school
  profiles; AdaLoRA compensation (LDM-8) planned for Q4 2027.

## Sourcecode Verification (Module: ethics_ai/readme)

- Verified files:
  - src/ethics_ai/ethics_ai_plugin.cpp
  - src/ethics_ai/discourse_engine.cpp
  - src/ethics_ai/argument_store.cpp
  - src/ethics_ai/philosophy_loader.cpp
  - src/ethics_ai/rag_context_engine.cpp
  - src/ethics_ai/ethics_evaluator.cpp
  - src/ethics_ai/chain_visualizer.cpp
  - src/ethics_ai/ethics_profile_registry.cpp
  - src/ethics_ai/ethics_selection_router.cpp
  - src/ethics_ai/convergence_marker_engine.cpp
  - src/ethics_ai/cross_school_tension_resolver.cpp
  - src/ethics_ai/prior_round_compressor.cpp
  - src/ethics_ai/synthesis_matrix_builder.cpp
  - src/ethics_ai/llm_cascade_router.cpp
  - src/ethics_ai/discourse_orchestrator.cpp
  - src/ethics_ai/cluster_discourse_engine.cpp
  - src/ethics_ai/meta_verdict_builder.cpp
  - src/ethics_ai/mirror_school_handler.cpp
- Verified behavior surfaces:
  - ethics decision/discourse orchestration and plugin lifecycle behavior
  - profile/store/RAG/evaluator integration boundaries
  - advanced context/routing/compression utility surfaces
  - LDM Ebene-1/2/3 and Mirror-School-Modus surfaces
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
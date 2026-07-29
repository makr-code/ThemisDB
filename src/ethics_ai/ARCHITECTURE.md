# Architecture - Ethics AI Module

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- Research: docs/research/ethics_discourse_process_equality.md -->

## Overview

The ethics_ai module composes philosophy profile ingestion, argument persistence, discourse orchestration, context assembly, evaluation, and plugin lifecycle wiring into a coherent ethics reasoning runtime for ThemisDB.

The current production mode is `SELECTION_ONLY` (Top-N school pre-selection via
`EthicsSelectionRouter`). The **Layered Discourse Model (LDM)** adds
`LAYERED_FULL` and `LAYERED_FAST` modes that satisfy Habermas participatory fairness
while remaining computationally tractable. See `FUTURE_ENHANCEMENTS.md` and
`docs/research/ethics_discourse_process_equality.md` for design details.

## Main Execution Planes

### 1. Profile and Configuration Plane
- profile loading, validation, and registry/index operations
- philosophy selection and routing across school sets
- **LDM extension:** `DiscourseMode` selection; `DiscourseOrchestratorPlan` generation
  assigning all N schools to Ebene-1 with equal initial weight `w₀ = 1/N`

### 2. Discourse and Synthesis Plane
- debate initialization/continuation and decision generation
- cross-school tension resolution and synthesis support
- **LDM extension (Ebene-1):** parallel equal-weight initial scoring for all N schools;
  `verdict ∈ {PROHIBIT, PERMIT, CONDITIONAL, ABSTAIN}` per school
- **LDM extension (Ebene-2):** taxonomy-class cluster discourse (K≤6 clusters)
  along four structural tension axes; O(K²·R) instead of O(N²·R)
- **LDM extension (Ebene-3):** convergence-counting `MetaVerdict` with
  `cross_cultural_flag`, `minority_dissent[]`, and positivrechtlichem Legal-DB-Grounding

### 3. Memory/Context Plane
- argument and decision storage/retrieval contracts
- RAG context assembly, prior-round compression, and matrix synthesis
- **LDM extension:** `ClusterPosition` persistence per Ebene-2 cluster;
  `EpisodicMemoryEntry` for inter-cluster tension outcomes

### 4. Evaluation and Operations Plane
- ethics scoring and metrics emission
- plugin lifecycle integration and runtime observability behavior
- **LDM extension:** MetaVerdict audit fields (`participating_schools`, `convergence_score`,
  `discourse_mode`) for EU AI Act Art. 13 transparency compliance

### 5. Cultural Self-Reflection Plane (LDM Mirror-School-Modus)
- non-western schools (islamische_ethik, konfuzianismus, buddhistische_ethik,
  juedische_bioethik) participate as structural self-reflection mirrors
- activated per-domain via `cross_cultural_sensitivity` policy
- output: `minority_dissent[]` in MetaVerdict — always present in audit trail
- cost: ≤ 1 LLM inference step per mirror school (parallel to Ebene-2)

## Core Contracts

| Contract | Behavior |
|---|---|
| profile contract | explicit profile validation/load and lookup behavior |
| discourse contract | deterministic debate and decision orchestration semantics |
| **process equality contract (LDM)** | all N schools participate in Ebene-1 with equal initial weight |
| **audit completeness contract (LDM)** | MetaVerdict lists all N schools incl. ABSTAIN |
| context contract | bounded argument memory and context retrieval behavior |
| evaluation/ops contract | explicit scoring, metrics, and plugin lifecycle surfaces |
| legal grounding contract | MetaVerdict grounding uses DB citations, never LLM paraphrase |

## LDM Discourse Mode Overview

```
DiscourseMode::SELECTION_ONLY   (current production)
  └─ EthicsSelectionRouter: Top-N pre-selection
  └─ Discourse: top_n schools only
  └─ MetaVerdict: weighted aggregate

DiscourseMode::LAYERED_FULL     (implemented)
  └─ Ebene-1: all N schools, parallel, equal w₀=1/N         O(N)
  └─ Ebene-2: K≤6 clusters × 3 rounds, tension-axis routing  O(K²·R)
  └─ Ebene-3: convergence-count + legal-DB grounding          O(1)+lookup
  └─ Mirror: non-western schools, 1 step parallel to Ebene-2
  └─ MetaVerdict: convergence_score + cross_cultural_flag + minority_dissent

DiscourseMode::LAYERED_FAST     (implemented)
  └─ Ebene-1: all N schools parallel (identical to LAYERED_FULL)
  └─ Ebene-2: axis-1 only (Kant↔Utilitarismus) + domain-relevant cluster
  └─ Ebene-3: MetaVerdict from Ebene-1 convergence + legal-DB
  └─ Target: P95 ≤ 1.2 s end-to-end
```

## Failure Semantics

- invalid profile or school selection paths fail with structured status/error states.
- missing storage/context inputs produce explicit non-silent failure behavior.
- lifecycle misuse (uninitialized plugin paths) fails deterministically.
- **LDM:** LLM timeout in Ebene-1 → school receives `ABSTAIN` (fail-closed); never silent drop.
- **LDM:** Legal-DB unavailable → MetaVerdict emitted without grounding with explicit observable flag.
- **LDM:** All N schools ABSTAIN → `DISSENT` MetaVerdict; never silently promoted to `SELECTION_ONLY`.

## Sourcecode Verification (Module: ethics_ai/architecture)

- Verified files:
  - src/ethics_ai/ethics_ai_plugin.cpp
  - src/ethics_ai/discourse_engine.cpp
  - src/ethics_ai/argument_store.cpp
  - src/ethics_ai/philosophy_loader.cpp
  - src/ethics_ai/rag_context_engine.cpp
  - src/ethics_ai/ethics_evaluator.cpp
  - src/ethics_ai/ethics_profile_registry.cpp
  - src/ethics_ai/ethics_selection_router.cpp
  - src/ethics_ai/discourse_orchestrator.cpp (DiscourseMode + DiscourseOrchestratorPlan)
  - src/ethics_ai/cluster_discourse_engine.cpp (Ebene-2 cluster routing)
  - src/ethics_ai/meta_verdict_builder.cpp (Ebene-3 convergence-count + legal grounding)
  - src/ethics_ai/mirror_school_handler.cpp (Mirror-School-Modus)
- Verified architecture claims:
  - explicit profile/discourse/context/evaluation planes
  - bounded deterministic failure behavior in lifecycle and profile paths
  - module-local orchestration for ethics reasoning runtime flows
  - LDM Ebene-1/2/3 and Mirror-School-Modus implemented and verified
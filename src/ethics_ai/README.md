---
Author: ThemisDB Team
Created: 2026-07-28
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Ethics AI Module

<!-- Status: current | source review: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- Research: docs/research/ethics_discourse_process_equality.md -->

## Module Purpose

The `ethics_ai` module contains a plugin runtime for profile loading, debate initialization and continuation, argument/decision persistence, RAG context assembly, evaluation, and plugin lifecycle. The plugin's current decision flow is in `EthicalDiscourseEngine`; the LDM router/orchestrator/builder APIs are separate source surfaces and are not called from `ethics_ai_plugin.cpp` or `discourse_engine.cpp`.

**Authority boundary:** root `ROADMAP.md` identifies `plugins/themisdb_ethic_ai/` as the canonical plugin implementation and records this repository path as a reference/compatibility surface. The source observations below describe only the in-tree `src/ethics_ai/` implementation; they do not establish canonical external-plugin or release behavior.

The repository currently has 26 YAML files under `assets/ethics_ai/`. This count is not a claim that the plugin loads all of them: runtime profile loading depends on the configured profile directory. Focused LDM tests and benchmarks use a separate deterministic 22-school mock registry.

The public source includes `SELECTION_ONLY`, `LAYERED_FULL`, and `LAYERED_FAST` router modes and LDM helper classes. The fixed `ETHICS_AI_SOURCES` list compiles 12 translation units, including the orchestrator, meta-verdict, and mirror helpers; compiling these units does not wire them into the plugin decision flow. `EthicsAIPlugin` delegates decisions to `EthicalDiscourseEngine` and does not call `EthicsSelectionRouter`, `DiscourseOrchestrator`, `MetaVerdictBuilder`, or `MirrorSchoolHandler`. Without an injected callback, the orchestrator fallback ignores dilemma text and chooses synthetic output from the school ID (`school_id.size() % 3` in the current implementation; its source comment describes this as hash-based). `MirrorSchoolHandler` also has a deterministic fallback. The meta-verdict builder does not consume cluster positions. See [ARCHITECTURE.md](ARCHITECTURE.md), [ROADMAP.md](ROADMAP.md), and [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md). Design rationale: `docs/research/ethics_discourse_process_equality.md`.

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
| discourse_orchestrator.cpp | LDM Ebene-1/2 helper paths; deterministic fallback if no inference function is injected |
| meta_verdict_builder.cpp | MetaVerdict assembly from supplied school outputs; cluster positions are currently unused |
| mirror_school_handler.cpp | Mirror-school helper with deterministic fallback if no inference function is injected |

## Discourse Modes

| Mode | Current source behavior |
|---|---|
| `SELECTION_ONLY` (default `RouterConfig`) | `planDiscourse()` returns an empty LDM plan; the existing router `route()` path is separate |
| `LAYERED_FULL` | `planDiscourse()` gathers the registry's indexed schools and creates taxonomy clusters/tension-axis labels; helper calls can execute Ebene-1/2 |
| `LAYERED_FAST` | `planDiscourse()` gathers indexed schools and assigns equal initial weight; it does not build the full cluster map |

These are source/API descriptions, not evidence of integrated production use or achieved latency. Current LDM benchmarks use deterministic stub fixtures; see [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

## MetaVerdict Structure

```
MetaVerdict:
  convergence_verdict:    CLEAR_CONSENSUS | TENDENCY | CONTESTED | DISSENT
  convergence_score:      float [0,1]
  participating_schools:  every supplied Ebene-1 result (incl. ABSTAIN)
  dissenting_schools:     schools with minority verdict
  cross_cultural_flag:    true when ≥ 2 cultural regions converge independently
  minority_dissent:       position_abstract from Mirror Schools
  legal_grounding:        caller-supplied grounding; no lookup in builder
  discourse_mode:         which mode was used
```

## School Profile Data (`assets/ethics_ai/`)

The asset directory contains 26 YAML profile files. The focused LDM fixtures exercise 22 IDs; the router enumerates whichever records its configured registry returns. See the asset files and `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`.

## Scope

In scope:
- ethics debate orchestration and decision synthesis contracts
- profile loading/selection and argument store integration
- context assembly, evaluation, and observability surfaces
- plugin-level ethics runtime wiring
- LDM router/orchestrator/meta-verdict/mirror helper APIs; full production inference and pipeline integration remain unverified

Out of scope:
- unrelated plugin subsystems outside ethics_ai interfaces
- external model ownership beyond module integration contracts
- non-ethics business logic in unrelated domains

## Runtime Behavior and Limits

- module behavior depends on loaded philosophy profiles and runtime config.
- decision quality and consensus outputs depend on argument/profile quality.
- some advanced generation and embedding paths remain configuration-dependent.
- LDM helper defaults are deterministic synthetic outputs unless an inference function is injected; the current plugin flow has no such injection.
- LDM benchmark targets use mock registries and do not establish real-LLM or representative-hardware performance.

## Sourcecode Verification (Module: ethics_ai/readme)

- Principal runtime files inspected:
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
  - src/ethics_ai/meta_verdict_builder.cpp
  - src/ethics_ai/mirror_school_handler.cpp
- `src/ethics_ai/CMakeLists.txt` lists a fixed set of 12 translation units for `ethics_ai_plugin`; the directory contains 21 `.cpp` files, and not all are included in that target. The current source tree has no `cluster_discourse_engine.cpp`.
- Verified behavior surfaces:
  - ethics decision/discourse orchestration and plugin lifecycle behavior
  - profile/store/RAG/evaluator integration boundaries
  - advanced context/routing/compression utility surfaces
  - LDM helper surfaces and their default deterministic fallbacks
- Note:
  - LDM production integration and hardware performance are not established by this source review
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
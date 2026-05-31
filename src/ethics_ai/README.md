# ThemisDB Ethics AI Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The ethics_ai module provides multi-philosophy ethics reasoning runtime surfaces for ThemisDB, including discourse orchestration, argument persistence, philosophy profile loading, RAG context assembly, decision evaluation, and plugin integration.

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
| ethics_selection_router.cpp | profile/school routing strategy |
| convergence_marker_engine.cpp | convergence marker generation |
| cross_school_tension_resolver.cpp | cross-school tension and opposition routing |
| prior_round_compressor.cpp | prior-round context compression logic |
| synthesis_matrix_builder.cpp | synthesis matrix assembly |
| llm_cascade_router.cpp | LLM cascade routing |

## Scope

In scope:
- ethics debate orchestration and decision synthesis contracts
- profile loading/selection and argument store integration
- context assembly, evaluation, and observability surfaces
- plugin-level ethics runtime wiring

Out of scope:
- unrelated plugin subsystems outside ethics_ai interfaces
- external model ownership beyond module integration contracts
- non-ethics business logic in unrelated domains

## Runtime Behavior and Limits

- module behavior depends on loaded philosophy profiles and runtime config.
- decision quality and consensus outputs depend on argument/profile quality.
- some advanced generation and embedding paths remain configuration-dependent.

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
- Verified behavior surfaces:
  - ethics decision/discourse orchestration and plugin lifecycle behavior
  - profile/store/RAG/evaluator integration boundaries
  - advanced context/routing/compression utility surfaces
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md
# Architecture - Ethics AI Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The ethics_ai module composes philosophy profile ingestion, argument persistence, discourse orchestration, context assembly, evaluation, and plugin lifecycle wiring into a coherent ethics reasoning runtime for ThemisDB.

## Main Execution Planes

1. Profile and configuration plane
- profile loading, validation, and registry/index operations
- philosophy selection and routing across school sets

2. Discourse and synthesis plane
- debate initialization/continuation and decision generation
- cross-school tension resolution and synthesis support

3. Memory/context plane
- argument and decision storage/retrieval contracts
- RAG context assembly, prior-round compression, and matrix synthesis

4. Evaluation and operations plane
- ethics scoring and metrics emission
- plugin lifecycle integration and runtime observability behavior

## Core Contracts

| Contract | Behavior |
|---|---|
| profile contract | explicit profile validation/load and lookup behavior |
| discourse contract | deterministic debate and decision orchestration semantics |
| context contract | bounded argument memory and context retrieval behavior |
| evaluation/ops contract | explicit scoring, metrics, and plugin lifecycle surfaces |

## Failure Semantics

- invalid profile or school selection paths fail with structured status/error states.
- missing storage/context inputs produce explicit non-silent failure behavior.
- lifecycle misuse (uninitialized plugin paths) fails deterministically.

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
- Verified architecture claims:
  - explicit profile/discourse/context/evaluation planes
  - bounded deterministic failure behavior in lifecycle and profile paths
  - module-local orchestration for ethics reasoning runtime flows
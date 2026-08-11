# ThemisDB AQL Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete; Phase 4-5 ongoing | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The AQL module provides language-assistance and LLM-adjacent capabilities around ThemisDB AQL workflows, including NL-to-AQL translation support, query validation/linting support, assistant tooling, and related runtime bridges.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| llm_aql_handler.cpp | LLM command and NL-to-AQL orchestration |
| aql_query_validator.cpp | structural and schema-aware validation checks |
| aql_query_builder.cpp | programmatic AQL construction |
| aql_schema_provider.cpp | schema context provisioning for AQL assistance |
| aql_syntax_highlighter.cpp | syntax highlighting and annotation utilities |
| aql_fewshot_example_library.cpp | few-shot example retrieval for translation support |
| aql_confidence_scorer.cpp | confidence scoring for generated AQL |
| aql_conversation_context.cpp | bounded conversational context management |
| docs_assistant_functions.cpp | docs-assistant helpers and function lookup |
| aql_agent.cpp | agent-style orchestration helpers |

## Scope

In scope:
- AQL assistance and translation support surfaces
- validation, linting, highlighting, and template/few-shot tooling
- conversation/context and scoring utilities for generated queries
- integration bridges for embedding/classification and ingestion-adjacent helpers

Out of scope:
- core query execution engine internals
- storage/index execution backends
- non-AQL protocol adapter logic outside this module

## Runtime Behavior and Limits

- behavior depends on configured providers, feature flags, and selected helper paths.
- generated query flows combine translation, validation, and confidence/annotation stages.
- some advanced integration surfaces are capability-dependent and may degrade to structured fallback behavior.

## Sourcecode Verification (Module: aql/readme)

- Verified files:
  - src/aql/llm_aql_handler.cpp
  - src/aql/aql_query_validator.cpp
  - src/aql/aql_query_builder.cpp
  - src/aql/aql_schema_provider.cpp
  - src/aql/aql_syntax_highlighter.cpp
  - src/aql/aql_fewshot_example_library.cpp
  - src/aql/aql_confidence_scorer.cpp
  - src/aql/aql_conversation_context.cpp
  - src/aql/docs_assistant_functions.cpp
  - src/aql/aql_agent.cpp
- Verified behavior surfaces:
  - translation/validation/highlighting assistance paths
  - scoring/context and few-shot support paths
  - agent and bridge-oriented support utilities
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md
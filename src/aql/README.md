---
Author: "ThemisDB Team"
Created: "2026-08-10"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB AQL Module

<!-- The former maturity/phase snapshot was dated 2026-08-10 and is not current status evidence. -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The AQL module provides assistance around AQL authoring, including NL-to-AQL
translation support, query validation and builder helpers, assistant tooling,
and related runtime bridges. It does not own the core parser or query execution
engine; see [`ARCHITECTURE.md`](ARCHITECTURE.md).

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| `llm_aql_handler.cpp` | LLM operation and NL-to-AQL assistance orchestration |
| `aql_query_validator.cpp` | string/builder checks, optionally using supplied collection metadata |
| aql_query_builder.cpp | programmatic AQL construction |
| aql_schema_provider.cpp | schema context provisioning for AQL assistance |
| aql_syntax_highlighter.cpp | syntax highlighting and annotation utilities |
| `aql_fewshot_example_library.cpp` | few-shot example retrieval with lexical default and optional embeddings |
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
- generated-query flow applies configured validation behavior; parser-backed
  checking is used when a parser service is available, with a weaker
  string-level fallback otherwise.
- optional provider/bridge paths depend on configuration and can have
  component-specific fallback behavior.
- parsing does not execute a query or establish authorization.

## Source inventory checked (2026-10-31)

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
- Behavior surfaces inspected:
  - translation/validation/highlighting assistance paths
  - scoring/context and few-shot support paths
  - agent and bridge-oriented support utilities
- Note:
  - this inventory review does not certify build/test/security status.
  - API details and known implementation/contract drift are in
    [`API.md`](API.md).
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md

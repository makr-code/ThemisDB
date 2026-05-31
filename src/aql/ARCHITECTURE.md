# Architecture - AQL Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The AQL module composes assistance-oriented components around AQL authoring and LLM-augmented workflows. It sits beside the core query execution stack and focuses on generation quality, validation confidence, and developer/operator support surfaces.

## Main Execution Planes

1. Translation and command plane
- NL-to-AQL and LLM command orchestration
- integration with model/provider-facing paths

2. Validation and quality plane
- structural and schema-aware checks
- syntax highlighting and annotation support
- confidence scoring and explainability helpers

3. Context and tooling plane
- conversation context and few-shot retrieval
- template/query-builder and assistant utility paths
- agent and bridge utilities for composed workflows

## Core Contracts

| Contract | Behavior |
|---|---|
| translation interfaces | transform NL and command inputs into AQL-oriented outputs |
| validation interfaces | enforce structural/schema checks and issue reporting |
| context/scoring interfaces | manage quality signals and bounded conversation state |
| tooling interfaces | expose helper utilities for docs, templates, and orchestration |

## Failure Semantics

- malformed or unsupported flows return structured errors rather than silent acceptance.
- validation/highlighting/scoring stages provide bounded diagnostics surfaces.
- capability-dependent flows degrade through explicit fallback behavior paths.

## Sourcecode Verification (Module: aql/architecture)

- Verified files:
  - src/aql/llm_aql_handler.cpp
  - src/aql/aql_query_validator.cpp
  - src/aql/aql_query_builder.cpp
  - src/aql/aql_syntax_highlighter.cpp
  - src/aql/aql_confidence_scorer.cpp
  - src/aql/aql_conversation_context.cpp
  - src/aql/aql_fewshot_example_library.cpp
- Verified architecture claims:
  - separation of translation, validation, and tooling planes
  - dedicated quality/diagnostic helper surfaces
  - capability-sensitive integration behavior in assistance paths
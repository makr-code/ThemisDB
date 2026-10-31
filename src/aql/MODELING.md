---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Assistance Modeling

## Purpose

This document describes the module's conceptual and data model for developers
working on AQL assistance. “Model” here means the processing and state model;
it does not assert that `src/aql/` trains or owns a particular language model.
Provider and model lifecycle are external to these assistance contracts.

## Translation state model

The main translation flow can be reasoned about as the following sequence:

1. **Request:** Natural-language text and optional schema context enter an
   `LLMAQLHandler` translation method.
2. **Context assembly:** The handler may include configured prompt context and
   selected examples from `AQLFewShotExampleLibrary`.
3. **Generation:** The configured LLM produces a candidate AQL string.
4. **Validation:** When parser validation is configured, the candidate is
   checked through `query::AQLParserService`; validation policy controls
   whether issues are logged, rejected, or used for retry feedback.
5. **Result:** The caller receives the query or a structured error. Confidence
   and validation diagnostics are separate information and are not an
   authorization decision.

The public result type `LLMAQLHandler::AQLTranslationResult` pairs an AQL
string with an `AQLConfidenceScore`; the public score field is
`overall_confidence`. Consult the score type and scorer implementation for its
range and fallback behavior. The score is not a calibrated correctness or
authorization signal.

## Context and example state

`AQLConversationContext::Config` bounds retained context with `max_turns`
(default 50) and `max_history_tokens` (default 8192). The implementation
evicts oldest user/assistant pairs to respect configured limits; a zero value
disables the corresponding limit. This is a history-eviction policy, not a
hard request-size cap: a single new turn can exceed the budget when there are
no older pairs to evict, and the LLM call occurs before post-response
eviction. Callers must enforce any stricter request bound separately.
Optional episodic compaction is gated by an explicit enable flag, token
trigger, and compressor availability.

Few-shot examples have an ID, natural-language query, AQL query, domain,
description, and tags. `findRelevant()` optionally filters by domain and
returns at most the requested number of examples, ordered by relevance.
Without an embedding provider, relevance uses the existing lexical Jaccard
path. With a provider, semantic similarity is used where cached/query
embeddings are available, and missing embeddings fall back to the lexical
score.

## Evaluation and constraints

- Keep generation, parsing, schema validation, scoring, and authorization as
  distinct concepts in code and docs.
- Test boundary conditions for empty input, retry exhaustion, unavailable
  providers, context eviction, absent embeddings, and candidate count limits.
- Treat provider outputs and user-supplied context as untrusted input.
- Do not compare model variants from latency alone; use the evaluation method
  in [`RESEARCH.md`](RESEARCH.md) and the reproducible test inventory in
  [`TESTING_COVERAGE.md`](TESTING_COVERAGE.md).

## Source references

- [`include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h)
- [`include/aql/aql_conversation_context.h`](../../include/aql/aql_conversation_context.h)
- [`include/aql/aql_fewshot_example_library.h`](../../include/aql/aql_fewshot_example_library.h)
- [`aql_conversation_context.cpp`](aql_conversation_context.cpp)
- [`aql_fewshot_example_library.cpp`](aql_fewshot_example_library.cpp)
- [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md)

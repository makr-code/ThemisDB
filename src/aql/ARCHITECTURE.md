---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Architecture - AQL Module

<!-- Links: README.md · API.md · IMPLEMENTATION.md -->

## Overview

`src/aql/` implements AQL authoring assistance and related helper surfaces. It
does not own the core parser or query execution engine; public declarations are
in [`include/aql/`](../../include/aql/) and parser/execution services are owned
by [`src/query/`](../query/). The assistance APIs do not replace execution-time
authorization.

## Component groups

1. **Translation and validation:** `LLMAQLHandler` coordinates prompt input,
   LLM-backed candidate generation, candidate validation, and configured
   validation/retry behavior. `AQLQueryValidator` separately provides
   string/builder checks, optionally using supplied collection metadata.
2. **Context and quality helpers:** conversation history, few-shot example
   selection, confidence scoring, syntax highlighting, and query-building
   helpers support authoring workflows.
3. **Adapters and orchestration:** embedding, classification, ingestion, and
   agent helpers connect selected AQL assistance paths to other interfaces.

These are component responsibilities, not guarantees that every deployment
enables every provider or helper.

## Translation dataflow

For `LLMAQLHandler::translateNLToAQL()` the implementation currently:

1. Applies input sanitization and configured length limits to the natural
   language request and schema context.
2. Builds the prompt and invokes the configured chat/provider path.
3. Removes Markdown fences from the response and validates the candidate.
   When a `query::AQLParserService` is available, validation calls its
   `parse()` method. Otherwise, it falls back to `AQLQueryValidator`'s
   string-level checks; that is not equivalent to parser-backed validation.
4. Applies the configured validation mode and retry limit. Retry feedback is
   sanitized/delimited before being used in a later prompt.
5. Performs collection-access and schema-scope checks before returning the
   generated query.

Parser acceptance is not execution, semantic correctness, or an authorization
decision. See [`API.md`](API.md) for the documented contract/implementation
drift around empty input and the internal validation helper.

## Optional few-shot embedding path

`AQLFewShotExampleLibrary` filters examples and ranks them using lexical
Jaccard relevance by default. When an `IEmbeddingProvider` is supplied, it can
rank using cosine similarity and falls back to lexical relevance when an
embedding is unavailable. `LLMAQLEmbeddingBridge` adapts
`LLMAQLHandler::executeEmbed()` and returns an empty vector for empty input or
handler exceptions.

## Integration boundary

- Public types and configuration: [`include/aql/`](../../include/aql/).
- Parser service contract and execution: [`include/query/`](../../include/query/)
  and [`src/query/`](../query/).
- Focused module tests: [`tests/aql/`](../../tests/aql/).
- Benchmark sources and target registration:
  [`benchmarks/aql/`](../../benchmarks/aql/).

The exact composition of optional implementations depends on build targets and
feature configuration; inspect `cmake/ModularBuild.cmake` and the relevant
module CMake files for a particular build.

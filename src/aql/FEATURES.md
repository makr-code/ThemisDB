---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Assistance Features

## Overview

The AQL module provides assistance components around AQL authoring. Its
source-supported areas include LLM-backed NL-to-AQL translation, query
builder/validator helpers, conversation context, few-shot examples, confidence
scoring, syntax highlighting, and selected adapters. The core parser and query
execution engine are outside `src/aql/`; see
[`ARCHITECTURE.md`](ARCHITECTURE.md).

## Feature areas

- **Translation:** `LLMAQLHandler` exposes translation, streaming, confidence,
  batch, and example-assisted entry points. Provider calls and validation
  behavior depend on handler configuration.
- **Validation and authoring:** `AQLQueryValidator` checks strings/builders and
  can use supplied collection metadata; `AQLQueryBuilder` constructs queries.
- **Context and examples:** `AQLConversationContext` maintains bounded
  conversation history. `AQLFewShotExampleLibrary` ranks examples lexically by
  default and can use an optional embedding provider.
- **Supporting tools:** public headers also declare highlighting, confidence,
  autocomplete, agent, model-routing, migration, rollback-suggestion,
  classification, ingestion, and token/metrics helpers.

These features do not imply that every deployment enables each integration or
that generated AQL is authorized or semantically correct.

## Dataflow and evidence

The translation path sanitizes bounded inputs, requests a candidate from the
configured provider, validates it (parser-backed when a parser service is
available, otherwise with a weaker string-level fallback), and applies
configured validation/retry and collection-scope checks. Parsing does not
execute a query. Detailed behavior and known contract drift are recorded in
[`API.md`](API.md) and [`IMPLEMENTATION.md`](IMPLEMENTATION.md).

Focused source and test references:

- Public interface declarations: [`include/aql/`](../../include/aql/).
- Focused tests: [`tests/aql/`](../../tests/aql/).
- Benchmarks: [`benchmarks/aql/`](../../benchmarks/aql/).

## Scope and sources

This page is a feature index, not a production-readiness or customer-outcome
claim. For current design boundaries, consult
[`ARCHITECTURE.md`](ARCHITECTURE.md); for change planning, consult
[`ROADMAP.md`](ROADMAP.md) and [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md).

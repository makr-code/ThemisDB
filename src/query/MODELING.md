---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Query Module Modeling Notes

## Modeling goal

This document explains the conceptual model behind the Query module as implemented in the query compiler, parser, optimizer, and engine. It is focused on the internal execution abstraction, not on product narrative or marketing wording.

## Primary model layers

The module is organized around a layered model:

1. `AQL` / query syntax layer
   - `aql_parser.cpp` parses SQL-like or AQL statements into AST nodes.
   - `aql_parser_json.cpp` covers the JSON object form of AQL input.
   - Syntax-level validation is the first safety boundary before execution.

2. Translator / representation layer
   - `aql_translator.cpp` converts AST into the internal query representation used by the planner and executor.
   - This representation captures statements, predicates, projections, loops, collection targets, and subquery scopes.

3. Planner / optimizer layer
   - `query_optimizer.cpp` and `optimizer_cost_model.cpp` choose strategy and order for scan, filter, join, or hybrid execution.
   - Adaptive planner behavior is refined by runtime statistics and by workload-sensitive cache policies.

4. Runtime execution layer
   - `query_engine.cpp` executes optimized plans against storage and index components.
   - Execution behavior includes filter evaluation, aggregate processing, result streaming, and distributed merge logic.

5. Cache and coordination layer
   - `query_cache.cpp` and `semantic_cache.cpp` provide cache behavior for repeated or semantically similar queries.
   - `query_cache_manager.cpp` coordinates between exact and adaptive cache mechanisms.

## Domain model

The Query module models the world as a declarative execution graph. A query is not just a string. It is a structured set of operations that can be reasoned about in terms of:

- collection or table access,
- predicate selectivity and cost,
- result cardinality,
- distributed routing,
- fallback behavior under timeout or overload,
- result materialization and streaming.

At the AST level, expressions encode comparisons, arithmetic, boolean logic, function calls, field access, aggregations, and nested subqueries. The planner later turns these into an execution strategy whose primary goal is to reduce compute and storage cost while preserving correctness.

## Query execution semantics

The core execution semantics of the Query module are intentionally aligned to a few invariants:

- a query can be read and planned before execution;
- predicates and projections are evaluated with explicit ordering;
- results may be streamed instead of fully materialized;
- filters and access checks must remain explicit over the full execution path;
- failure is reported as failure, not silently converted into an output that hides the issue.

These invariants keep the runtime explainable and make tests and operator debugging manageable.

## Planner model

The optimizer is modeled around estimated cost rather than hardcoded behavior. Cost estimation is derived from:

- predicate cardinality,
- output size,
- selectivity of index or scan paths,
- join or traversal branching,
- caching behavior and repeated query patterns.

This is the basis for `chooseOrderForAndQuery`-style planning and for the more advanced adaptive optimizer behavior documented in `ARCHITECTURE.md` and `ROADMAP.md`.

## Continuous / hybrid execution model

The Query module also includes execution concepts for:

- continuous queries (`continuous_query_engine.cpp`, `continuous_query_planner.cpp`),
- federation across clusters (`query_federation.cpp`, `cross_cluster_federation.cpp`),
- vector and similarity-based retrieval (`vectorized_execution.cpp`, `semantic_cache.cpp`, engine entry points tied to vector search),
- FTS and geospatial integrations (`fts_executor.cpp`, `geospatial_query_rewrite.cpp`).

These are not isolated features. They are variations on the same core model: a declarative request enters the system, the planner chooses a route, and the runtime executes with explicit safety and timeout guardrails.

## Failure modeling

The implementation treats query failures as part of the model rather than as exceptional side noise:

- malformed input is a parse error,
- invalid execution state is a planner/runtime denial,
- access violations are explicit security or policy decisions,
- network or cluster error is a federation failure boundary,
- resource exhaustion is a timeout or backpressure outcome.

This is why the Query module is operationally strong: it does not pretend that every runtime error is recoverable.

## Operational design implications

The modeling approach has two practical consequences:

1. Query code should stay domain-specific and avoid absorbing storage or transport concerns that belong elsewhere.
2. Changes should respect the planner/executor contract: a planner change should be paired with execution and validation updates, and a runtime change should remain observable to operators.

The model is deliberately narrow around query semantics and execution contracts. That makes the system easier to test, easier to debug, and easier to reason about under partial failure.

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

# Query Module API and Contract Notes

## Purpose

This document summarizes the main contract surfaces in the Query module as implemented in the code under `include/query` and the surrounding runtime code under `src/query`.

## Primary interfaces

### AQL parser

`include/query/aql_parser.h` defines the parser contract used to parse AQL input into an AST.

Key responsibilities:

- parse query statements and expressions,
- validate syntax and malformed input,
- surface structured errors instead of crashing on invalid AQL,
- build an AST that downstream translation stages can consume.

### Query execution engine

`include/query/query_engine.h` contains the execution-side contract for running parsed and optimized queries.

Key responsibilities:

- execute query plans against the target data model,
- evaluate filters and projections,
- handle access validation checks,
- support range, OR/AND, graph, and hybrid query execution,
- incorporate runtime cancellation and dispatch controls.

### Cache interfaces

`include/query/query_cache.h`, `include/query/query_cache_manager.h`, and `include/query/semantic_cache.h` expose the cache contracts used to optimize repeated or similar queries.

Their responsibilities include:

- exact-match result caching,
- fingerprint and dependency-based invalidation,
- optional semantic lookup by embedding similarity,
- result-size and TTL enforcement,
- stats and observability around hit rate and memory consumption.

## Execution contract

The query path is expected to obey the following runtime order:

1. parse text into a query structure,
2. translate the structure into an internal representation,
3. optimize or rewrite the plan,
4. validate access and security constraints,
5. execute the plan with runtime cancellation/timeout checks,
6. stream or materialize the result,
7. update cache and metrics if applicable.

This model is the basis of the Query module's reliability and explainability under production conditions.

## Failure semantics

The API is designed to keep failures explicit. In the codebase, failure paths typically arise from:

- invalid AQL syntax,
- result size or cache memory limits,
- expired or invalid cache entries,
- access-denial callbacks,
- timeout, cancellation, or federation error states.

These should remain visible and traceable as part of the query lifecycle instead of being silently converted into misleading successful results.

## Threading and concurrency expectations

The parser and translation layers are designed to be usable in a concurrent query environment without sharing mutable parser state in the wrong places. Execution resources such as query cancellation and collection access callbacks are intentionally explicit caveats for the runtime path.

The module should remain conservative with state sharing: stateful runtime decisions should belong to the engine or associated runtime objects, not to a globally hidden query context.

## Contract stability guidance

When evolving the public or semi-public query surface:

- keep planner and executor interfaces aligned,
- update cache and timeout semantics together,
- keep AQL validation and execution policy changes tied to tests,
- avoid introducing broad hidden fallback behavior in the query path,
- preserve result and error semantics that operators can rely on.

## Summary

The Query module's API is a bounded execution contract: parse, optimize, validate, execute, stream, and cache. The contract is not a generic passthrough. Its value is in making correctness and failure modes explicit for callers and operators.

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

# Query Module Guide

## Who this module is for

This module is for developers working on AQL parsing, optimizer behavior, distributed query execution, cache semantics, or runtime safety within ThemisDB. It is the module to consult when a production issue involves query correctness, planner behavior, or bounded execution under load.

## Module responsibilities

The Query module owns the execution path that turns declarative queries into runtime behavior. Its responsibilities include:

- parsing AQL and related query forms,
- translating queries into a planner/execution representation,
- choosing a plan based on cost and workload signals,
- executing filters, scans, results, and aggregations,
- managing exact and semantic cache reuse,
- enforcing access, timeout, and cancellation boundaries,
- providing distributed or federated execution for multi-shard workloads.

## Key files

- `src/query/aql_parser.cpp` — syntax parsing and AST creation
- `src/query/aql_translator.cpp` — AST to internal query representation
- `src/query/query_optimizer.cpp` — plan ordering and query rewrite decisions
- `src/query/query_engine.cpp` — execution engine
- `src/query/query_cache.cpp` — exact-match cache behavior
- `src/query/semantic_cache.cpp` — similarity-based cache behavior
- `src/query/query_federation.cpp` / `src/query/cross_cluster_federation.cpp` — distributed execution
- `src/query/continuous_query_engine.cpp` — continuous query runtime
- `src/query/query_canceller.cpp` — cancellation contract and runtime control

## Working conventions

### 1. Keep planning and execution explicit

A change in the optimizer should be traceable to execution behavior. Avoid introducing hidden planner rewrites that are impossible to understand from the execution path or test output.

### 2. Keep runtime safety visible

When changing a query or federation path, verify that access checks, cancellation, and timeout handling remain explicit. Silent fallback behavior is a design smell in this module.

### 3. Preserve cache correctness

Cache changes should be paired with validity checks: TTL semantics, dependency invalidation, and memory-bound assertions matter more than raw performance win alone.

### 4. Keep distributed failure semantics narrow and explainable

Federated query logic should not hide network or cluster failures as normal execution success. Query results must remain clearly partitioned by source and failure status.

## Development checklist

Before landing a query change, verify the following:

- syntax and parser behavior remain deterministic,
- planner changes still map to observable execution choices,
- access checks still fire in runtime-critical paths,
- cancellation and timeout behavior remain explicit,
- cache updates do not create stale or unsafe reuse,
- relevant tests cover the changed path.

## Testing guidance

Target the tests closest to the behavior under change, especially:

- parser validation and query safety tests,
- optimizer and cache regression suites,
- federation and timeout tests,
- continuous query and cancellation tests.

## Summary

The Query module should remain easy to reason about: parse → plan → validate → execute → cache. If a change makes that pipeline less explainable, it is likely in the wrong layer or missing an explicit failure contract.
